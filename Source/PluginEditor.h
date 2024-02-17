/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "NewLookAndFeel.h"

//==============================================================================
/**
*/
class SubdominantAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                         public juce::AudioProcessorValueTreeState::Listener
{
public:
    SubdominantAudioProcessorEditor (SubdominantAudioProcessor&);
    ~SubdominantAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void initRotarySlider(juce::Slider*, juce::Label*, const juce::String&, const juce::Colour&);
    void initHrzntlSlider(juce::Slider*, juce::Label*, const juce::String&);
    void initRadioButtons(juce::ToggleButton*, const juce::String&, const int);

    void parameterChanged(const juce::String& parameterID, float newValue) override;

    juce::ToggleButton instButton, lineButton;

    juce::Slider squareWaveVolSlider[4];
    juce::Slider filterSlider, inSubOctLPFSlider, clipSlider, 
                 blendSlider, masterVolSlider;

    juce::Label squareWaveVolLabel[4];
    juce::Label filterLabel, inSubOctLPFLabel, clipLabel, 
                blendLabel, masterVolLabel;

    juce::String squareWaveName[4] = { "SUB2", "SUB1", "DOM", "NORM" };
    juce::String squareWaveAttachName[4] = { "SUB2 VOLUME","SUB1 VOLUME",
                                             "DOM VOLUME", "NORM VOLUME", };

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> squareWaveVolAttachment[4], filterAttachment,
                                                                          inSubOctLPFAttachment, clipAttachment, 
                                                                          blendAttachment, masterVolAttachment;

    NewLookAndFeel newLookAndFeel;

    SubdominantAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SubdominantAudioProcessorEditor)
};
