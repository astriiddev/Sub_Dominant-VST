/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SubdominantAudioProcessor::SubdominantAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), APVTS(*this, nullptr, "Parameters", createParameters())
#endif
{
    APVTS.state.addListener(this);

    clearOnePoleFilterState(&rectLPF);
    clearOnePoleFilterState(&filterLPF);
    clearTwoPoleFilterState(&inSubOctLPF);
    setupOnePoleFilter(getSampleRate(), 20000., &filterLPF);
    setupTwoPoleFilter(getSampleRate(), 159., 0.660225, &inSubOctLPF);
}

SubdominantAudioProcessor::~SubdominantAudioProcessor()
{
}

//==============================================================================
const juce::String SubdominantAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SubdominantAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SubdominantAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SubdominantAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SubdominantAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SubdominantAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SubdominantAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SubdominantAudioProcessor::setCurrentProgram (int /*index*/)
{
}

const juce::String SubdominantAudioProcessor::getProgramName (int /*index*/)
{
    return {};
}

void SubdominantAudioProcessor::changeProgramName (int /*index*/, const juce::String& /*newName*/)
{
}

//==============================================================================
void SubdominantAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    setupOnePoleFilter(sampleRate, 1591., &rectLPF);
}

void SubdominantAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SubdominantAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void SubdominantAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    const float* inL = buffer.getReadPointer(0);
    const float* inR = totalNumInputChannels > 1 ? buffer.getReadPointer(1) : nullptr;

    float* outL = buffer.getWritePointer(0);
    float* outR = totalNumInputChannels > 1 ? buffer.getWritePointer(1) : nullptr;

    int numSamples = buffer.getNumSamples();

    updateParams();

    while (--numSamples >= 0)
    {
        const float sampL = *inL++;
        const float sampR = inR == nullptr ? sampL : *inR++;

        float filtered[2] = { 0.f, 0.f }, out[2] = { 0.f, 0.f };

        muteCounter = std::abs(sampL) <= 0.01f && std::abs(sampR) <= 0.01f ? muteCounter + 1.f : 0.f;

        inputGain(&sampL, &sampR, &lm386);
        
        fullWaveRect(&sampL, &sampR, &rectifier);

        twoPoleLPFilter(&inSubOctLPF, &lm386.left, &lm386.right, &filtered[0], &filtered[1]);

        cd4024one[0].incPulseCounter(&filtered[0]);
        cd4024one[1].incPulseCounter(&filtered[1]);

        subOctOne.left  = cd4024one[0].generatePulseWave();
        subOctOne.right = cd4024one[1].generatePulseWave();

        cd4024two[0].incPulseCounter(&subOctOne.left);
        cd4024two[1].incPulseCounter(&subOctOne.right);

        subOctOne.left  *= subOctOne.volume;
        subOctOne.right *= subOctOne.volume;

        subOctTwo.left  = cd4024two[0].generatePulseWave();
        subOctTwo.right = cd4024two[1].generatePulseWave();

        subOctTwo.left  *= subOctTwo.volume;
        subOctTwo.right *= subOctTwo.volume;

        mixWaves();

        onePoleLPFilter(&filterLPF, &mix.left, &mix.right, &out[0], &out[1]);

        out[0] = (out[0] * blend) + (sampL * std::abs(blend - 1.f));
        out[1] = (out[1] * blend) + (sampR * std::abs(blend - 1.f));

        if (outR == nullptr)
        {
            *outL++ = rampVolume((out[0] + out[1]) * 0.5f * masterVol, muteCounter);
        }
        else
        {
            *outL++ = rampVolume(out[0] * masterVol, muteCounter);
            *outR++ = rampVolume(out[1] * masterVol, muteCounter);
        }
    }
}

//==============================================================================
bool SubdominantAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SubdominantAudioProcessor::createEditor()
{
    return new SubdominantAudioProcessorEditor (*this);
}

//==============================================================================
void SubdominantAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ValueTree state = APVTS.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void SubdominantAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(APVTS.state.getType()))
        {
            APVTS.replaceState(juce::ValueTree::fromXml(*xmlState));
            clearTwoPoleFilterState(&inSubOctLPF);
            clearOnePoleFilterState(&filterLPF);
            paramsUpdated = true;
            updateParams();
        }
}

void SubdominantAudioProcessor::clearOnePoleFilterState(OnePoleFilter_t* f)
{
    f->tmpL = f->tmpR = 0.0;
    f->val = -1.0;
}

void SubdominantAudioProcessor::setupOnePoleFilter(const double audioRate, const double cutOff, OnePoleFilter_t* f)
{
    const double a = cutOff < audioRate / 2.0 ? 2.0 - std::cos((twoPi * cutOff) / audioRate) :
                             2.0 - std::cos((twoPi * ((audioRate / 2.0) - 1E-4)) / audioRate);

    const double b = a - std::sqrt((a * a) - 1.0);

    f->a1 = 1.0 - b;
    f->a2 = b;
}

void SubdominantAudioProcessor::onePoleLPFilter(OnePoleFilter_t* f, const float* inL, const float* inR, float* outL, float* outR)
{
    f->tmpL = (*inL * f->a1) + (f->tmpL * f->a2);
    *outL = (float)f->tmpL;

    f->tmpR = (*inR * f->a1) + (f->tmpR * f->a2);
    *outR = (float)f->tmpR;
}

void SubdominantAudioProcessor::clearTwoPoleFilterState(TwoPoleFilter_t* f)
{
    f->tmpL[0] = f->tmpL[1] = f->tmpL[2] = f->tmpL[3] = 0.0;
    f->tmpR[0] = f->tmpR[1] = f->tmpR[2] = f->tmpR[3] = 0.0;

    f->val = -1.f;
}

void SubdominantAudioProcessor::setupTwoPoleFilter(const double audioRate, const double cutOff, const double qFactor, TwoPoleFilter_t* f)
{
    const double a = cutOff < audioRate / 2.0 ? 1.0 / std::tan((pi * cutOff) / audioRate) : 
                              1.0 / std::tan((pi * ((audioRate / 2.0) - 1E-4)) / audioRate);

    const double b = 1.0 / qFactor;

    f->a1 = 1.0 / (1.0 + b * a + a * a);
    f->a2 = 2.0 * f->a1;
    f->b1 = 2.0 * (1.0 - a * a) * f->a1;
    f->b2 = (1.0 - b * a + a * a) * f->a1;
}

void SubdominantAudioProcessor::twoPoleLPFilter(TwoPoleFilter_t* f, const float* inL, const float* inR, float* outL, float* outR)
{
    const double LOut = (*inL * f->a1) + (f->tmpL[0] * f->a2) + (f->tmpL[1] * f->a1) - (f->tmpL[2] * f->b1) - (f->tmpL[3] * f->b2);
    const double ROut = (*inR * f->a1) + (f->tmpR[0] * f->a2) + (f->tmpR[1] * f->a1) - (f->tmpR[2] * f->b1) - (f->tmpR[3] * f->b2);

    // shift states

    f->tmpL[1] = f->tmpL[0];
    f->tmpL[0] = *inL;
    f->tmpL[3] = f->tmpL[2];
    f->tmpL[2] = LOut;

    f->tmpR[1] = f->tmpR[0];
    f->tmpR[0] = *inR;
    f->tmpR[3] = f->tmpR[2];
    f->tmpR[2] = ROut;

    // set output

    *outL = (float)LOut;
    *outR = (float)ROut;
}

void SubdominantAudioProcessor::inputGain(const float* inL, const float* inR, squarewave_t* out) const
{
    /* total output gain of 200, 10:1 pad for "inst level" */
    float gain[2] = { lineInstState ? *inL * 200.f : *inL * 20.f, lineInstState ? *inR * 200.f : *inR * 20.f };

    /* output is already harshly squared off even without accounting for output gain*/
    out->left  = gain[0] > 1.f ? 1.f : gain[0] < -1.f ? -1.f : gain[0];
    out->right = gain[1] > 1.f ? 1.f : gain[1] < -1.f ? -1.f : gain[1];
}

void SubdominantAudioProcessor::fullWaveRect(const float* inL, const float* inR, squarewave_t* out) const
{
    /* invert phase of negative amplitude, applying gain/clipping */
    float rectify[2] = { std::abs(*inL * 20.f) > 1.f ? 2.f : std::abs(*inL * 20.f),
                         std::abs(*inR * 20.f) > 1.f ? 2.f : std::abs(*inR * 20.f) };

    /* re-center back to +/- 1.0 range */
    out->left  = (rectify[0] - 1.f) * out->volume;
    out->right = (rectify[1] - 1.f) * out->volume;
}

void SubdominantAudioProcessor::mixWaves()
{
    mix.right = ((lm386.right * lm386.volume * 0.3f) + (rectifier.right * 0.3f) + (subOctOne.right * 0.2f + subOctTwo.right * 0.2f)) * (10.f * mix.volume);
    mix.left  = ((lm386.left  * lm386.volume * 0.3f) + (rectifier.left  * 0.3f) + (subOctOne.left  * 0.2f + subOctTwo.left  * 0.2f)) * (10.f * mix.volume);

    mix.left  = mix.left  > 1.f ? 1.f : mix.left < -1.f ? -1.f : mix.left;
    mix.right = mix.right > 1.f ? 1.f : mix.right< -1.f ? -1.f : mix.right;
}

float SubdominantAudioProcessor::rampVolume(const float input, const float counter)
{
    const float counterMin = 128.f;

    if (counter < counterMin) return input;

    return input * (counterMin / counter);
}

void SubdominantAudioProcessor::updateParams()
{
    double glitchval = 0.f;
    double filterval = 0.f;

    if (!paramsUpdated) return;

    glitchval = (double) APVTS.getRawParameterValue("SUB GLITCH AMOUNT")->load();
    filterval = (double) APVTS.getRawParameterValue("FILTER AMOUNT")->load();

    if (glitchval != inSubOctLPF.val || inSubOctLPF.val < 0.f)
    {
        clearTwoPoleFilterState(&inSubOctLPF);
        inSubOctLPF.val = glitchval;
        
        /*
         * logarithmic frequency sweep from 159hz - 20khz based on:
         *   https://stackoverflow.com/questions/32320028/convert-linear-audio-frequency-distribution-to-logarithmic-perceptual-distributi 
         */

        glitchval = 159. * pow(20000. / 159., inSubOctLPF.val / 1.);

        setupTwoPoleFilter(getSampleRate(), glitchval, 0.660225, &inSubOctLPF);
    }

    if (filterval != filterLPF.val || filterLPF.val < 0.f)
    {
        clearOnePoleFilterState(&filterLPF);
        filterLPF.val = filterval;

        /*
         * logarithmic frequency sweep from 14.5hz - 20khz based on:
         *   https://stackoverflow.com/questions/32320028/convert-lineaaudio-frequency-distribution-to-logarithmic-perceptual-distributi
         */

        filterval = 1446.8 * pow(20000. / 1446.8, filterLPF.val / 1.);

        setupOnePoleFilter(getSampleRate(), filterval, &filterLPF);
    }

    lm386.volume = APVTS.getRawParameterValue("NORM VOLUME")->load();
    rectifier.volume = APVTS.getRawParameterValue("DOM VOLUME")->load();
    subOctOne.volume = APVTS.getRawParameterValue("SUB1 VOLUME")->load();
    subOctTwo.volume = APVTS.getRawParameterValue("SUB2 VOLUME")->load();

    lineInstState = (int)APVTS.getRawParameterValue("LINE/INST")->load();

    mix.volume = APVTS.getRawParameterValue("GAIN AMOUNT")->load();
    blend = APVTS.getRawParameterValue("BLEND AMOUNT")->load();
    masterVol = APVTS.getRawParameterValue("MASTER VOLUME")->load();

    paramsUpdated = false;
}

juce::AudioProcessorValueTreeState::ParameterLayout SubdominantAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"NORM VOLUME", 1 }, "Norm Volume", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.5f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"DOM VOLUME", 1 },  "Dom Volume", juce::NormalisableRange<float>(0.f, 1.f, 0.01f),  0.5f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"SUB1 VOLUME", 1 }, "Sub1 Volume", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.5f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"SUB2 VOLUME", 1 }, "Sub2 Volume", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.5f));

    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"FILTER AMOUNT", 1 }, "Filter Amount", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 1.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"SUB GLITCH AMOUNT", 1 }, "Sub Glitch Amount", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.01f));

    parameters.push_back(std::make_unique<juce::AudioParameterInt>(juce::ParameterID{ "LINE/INST", 1 }, "Line/Inst", 0, 1, 1));

    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"GAIN AMOUNT", 1 }, "Gain Amount", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.5f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"BLEND AMOUNT", 1 }, "Blend Amount", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ "MASTER VOLUME", 1 }, "Master Volume", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.5f));


    return { parameters.begin(), parameters.end() };
}

void SubdominantAudioProcessor::valueTreePropertyChanged(juce::ValueTree& /*treeWhosePropertyHasChanged*/, const juce::Identifier& /*property*/)
{
    paramsUpdated = true;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SubdominantAudioProcessor();
}
