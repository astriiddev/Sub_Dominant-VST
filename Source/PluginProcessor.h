/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PulseGen.h"

//==============================================================================
/**
*/

constexpr double pi = juce::MathConstants<double>::pi;
constexpr double twoPi = juce::MathConstants<double>::twoPi;

class SubdominantAudioProcessor  : public juce::AudioProcessor,
                                   public juce::ValueTree::Listener
{
public:
    //==============================================================================
    SubdominantAudioProcessor();
    ~SubdominantAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return APVTS; }

    void setLineInstState(const float state)
    {
        APVTS.getParameter("LINE/INST")->beginChangeGesture();
        APVTS.getParameterAsValue("LINE/INST").setValue(state);
        APVTS.getParameter("LINE/INST")->endChangeGesture();
    }

    int& getLineInstState() { return lineInstState; }

private:
    typedef struct SquareWave
    {
        float left;
        float right;
        float volume;
    } squarewave_t;

    typedef struct OnePoleFilter_t
    {
    public:

        double tmpL, tmpR, a1, a2, val;

    } OnePoleFilter_t;

    typedef struct  TwoPoleFilter_t
    {
    public:

        double tmpL[4], tmpR[4], a1, a2, b1, b2, val;

    } TwoPoleFilter_t;

    void clearOnePoleFilterState(OnePoleFilter_t* f);
    void setupOnePoleFilter(const double audioRate, const double cutOff, OnePoleFilter_t* f);
    void onePoleLPFilter(OnePoleFilter_t* f, const float* inL, const float* inR, float* outL, float* outR);

    void clearTwoPoleFilterState(TwoPoleFilter_t* f);
    void setupTwoPoleFilter(const double audioRate, const double cutOff, const double qFactor, TwoPoleFilter_t* f);
    void twoPoleLPFilter(TwoPoleFilter_t* f, const float* inL, const float* inR, float* outL, float* outR);

    void inputGain(const float* inL, const float* inR, squarewave_t* out)const;
    void fullWaveRect(const float* inL, const float* inR, squarewave_t* out) const;

    void mixWaves();
    float rampVolume(const float input, const float counter);

    squarewave_t lm386 = { 0.f, 0.f, 0.5f };

    squarewave_t subOctOne = { 0.f, 0.f, 0.5f },
                 subOctTwo = { 0.f, 0.f, 0.5f };

    squarewave_t rectifier = { 0.f, 0.f, 0.5f };
    squarewave_t mix  = { 0.f, 0.f, 0.5f };

    OnePoleFilter_t rectLPF, filterLPF;
    TwoPoleFilter_t inSubOctLPF;

    int lineInstState = 1;

    float filterAmount = 0.5f, muteCounter = 0.f;;
    float masterVol = 0.5f, blend = 0.f;

    PulseGen cd4024one[2], cd4024two[2];

    bool paramsUpdated = true;
    juce::AudioProcessorValueTreeState APVTS;

    void updateParams();
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
    void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged,
                                    const juce::Identifier& property) override;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SubdominantAudioProcessor)
};
