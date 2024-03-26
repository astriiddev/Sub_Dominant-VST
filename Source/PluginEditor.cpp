/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SubdominantAudioProcessorEditor::SubdominantAudioProcessorEditor (SubdominantAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel(&newLookAndFeel);

    setRepaintsOnMouseActivity(false);

    for (int i = 0; i < 4; i++)
    {
        const juce::Colour colour = i < 3 ? juce::Colours::blue : juce::Colours::limegreen;

        initRotarySlider(&squareWaveVolSlider[i], &squareWaveVolLabel[i], squareWaveName[i], colour);

        squareWaveVolAttachment[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
            (audioProcessor.getAPVTS(), squareWaveAttachName[i], squareWaveVolSlider[i]);
    }

    initRotarySlider(&filterSlider, &filterLabel, "FLTR", juce::Colours::limegreen);
    filterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.getAPVTS(), "FILTER AMOUNT", filterSlider);

    initRotarySlider(&clipSlider, &clipLabel, "GAIN", juce::Colours::limegreen);
    clipAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.getAPVTS(), "GAIN AMOUNT", clipSlider);

    initRotarySlider(&blendSlider, &blendLabel, "BLEND", juce::Colours::limegreen);
    blendAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.getAPVTS(), "BLEND AMOUNT", blendSlider);

    initRotarySlider(&masterVolSlider, &masterVolLabel, "VOLUME", juce::Colours::limegreen);
    masterVolAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.getAPVTS(), "MASTER VOLUME", masterVolSlider);

    initHrzntlSlider(&inSubOctLPFSlider, &inSubOctLPFLabel, "SUB OCT GLITCH");
    inSubOctLPFAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.getAPVTS(), "SUB GLITCH AMOUNT", inSubOctLPFSlider);

    initRadioButtons(&instButton, "INST", 1001);
    initRadioButtons(&lineButton, "LINE", 1001);
    instButton.onClick = [&] { audioProcessor.setLineInstState(0); };
    lineButton.onClick = [&] { audioProcessor.setLineInstState(1); };

    setSize (600, 600);
}

SubdominantAudioProcessorEditor::~SubdominantAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

//==============================================================================
void SubdominantAudioProcessorEditor::paint (juce::Graphics& g)
{
    bool lineInstState = (bool) audioProcessor.getLineInstState();

    g.fillAll (juce::Colours::darkblue);

    if (lineInstState != lineButton.getToggleState() || lineInstState == instButton.getToggleState())
    {
        lineButton.setToggleState(lineInstState, juce::dontSendNotification);
        instButton.setToggleState(!lineInstState, juce::dontSendNotification);
    }

    g.setColour(juce::Colours::gold);
    g.fillRect(0, 200, 400, 400);
    g.setColour(juce::Colours::darkred);
    g.fillRect(0, 400, 200, 200);

    g.setFont(newLookAndFeel.getCustomFont().withHeight(30.f));

    g.setColour(juce::Colours::darkred.darker(0.4f).withAlpha(0.8f));
    g.drawText("SUB_DOMINANT", getWidth() - 200, getHeight() - 110, 192, 42, juce::Justification::bottomRight, false);
    g.setColour(juce::Colours::red.darker(0.4f));
    g.drawText("SUB_DOMINANT", getWidth() - 200, getHeight() - 110, 190, 40, juce::Justification::bottomRight, false);

    g.setColour(juce::Colours::green.withAlpha(0.8f));
    g.drawText("OCTAVE  FUZZ", getWidth() - 200, getHeight() - 80, 192, 42, juce::Justification::bottomRight, false);
    g.setColour(juce::Colours::limegreen);
    g.drawText("OCTAVE  FUZZ", getWidth() - 200, getHeight() - 80, 190, 40, juce::Justification::bottomRight, false);

    g.setColour(juce::Colours::gold.withAlpha(0.4f));
    g.drawText("by  _astriid_", getWidth() - 200, getHeight() - 50, 192, 42, juce::Justification::bottomRight, false);
    g.setColour(juce::Colours::yellow);
    g.drawText("by  _astriid_", getWidth() - 200, getHeight() - 50, 190, 40, juce::Justification::bottomRight, false);
}

void SubdominantAudioProcessorEditor::resized()
{
    const float slider_x = 0.03f;

    clipSlider.setBoundsRelative(slider_x, 0.1f, 0.2f, 0.2f);
    filterSlider.setBoundsRelative(slider_x + 0.2f, 0.1f, 0.2f, 0.2f);
    squareWaveVolSlider[3].setBoundsRelative(slider_x + 0.4f, 0.1f, 0.2f, 0.2f);

    for (int i = 0; i < 3; i++)
        squareWaveVolSlider[i].setBoundsRelative(slider_x + (i * 0.2f), 0.4f, 0.2f, 0.2f);

    masterVolSlider.setBoundsRelative(slider_x + 0.7f, 0.1f, 0.2f, 0.2f);
    blendSlider.setBoundsRelative(slider_x + 0.7f, 0.4f, 0.2f, 0.2f);
    
    inSubOctLPFSlider.setBoundsRelative(0.02f, 0.94f, 0.19f, 0.05f);

    lineButton.setBoundsRelative(0.35f, 0.95f, 0.1f, 0.03f);
    instButton.setBoundsRelative(0.47f, 0.95f, 0.1f, 0.03f);
}

void SubdominantAudioProcessorEditor::initRotarySlider(juce::Slider* s, juce::Label* l, const juce::String& name, const juce::Colour& colour)
{
    const juce::Font labelFont = l == nullptr ? 0 : l->getFont().boldened().withHeight(24.f);
    
    if (s == nullptr) return;

    addAndMakeVisible(*s);
    s->setScrollWheelEnabled(false);
    s->setRepaintsOnMouseActivity(false);
    s->setMouseCursor(juce::MouseCursor::PointingHandCursor);
    
    s->setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    s->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);

    if (l == nullptr) return;

    l->setFont(labelFont);
    l->setJustificationType(juce::Justification::centredTop);
    l->attachToComponent(s, false);
    l->setColour(juce::Label::textColourId, colour);

    if (name.isEmpty()) return;

    l->setText(name, juce::dontSendNotification);
}

void SubdominantAudioProcessorEditor::initHrzntlSlider(juce::Slider* s, juce::Label* l, const juce::String& name)
{
    const juce::Font labelFont = l == nullptr ? 0 : l->getFont().boldened().withHeight(24.f);

    if (s == nullptr) return;

    addAndMakeVisible(*s);
    s->setScrollWheelEnabled(false);
    s->setRepaintsOnMouseActivity(false);
    s->setMouseCursor(juce::MouseCursor::PointingHandCursor);

    s->setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    s->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);

    s->setColour(juce::Slider::ColourIds::trackColourId, juce::Colours::blue.darker(0.4f));
    s->setColour(juce::Slider::ColourIds::backgroundColourId, juce::Colours::darkblue.darker(0.2f));
    s->setColour(juce::Slider::ColourIds::thumbColourId, juce::Colours::yellow);

    if (l == nullptr) return;

    l->setFont(16.f);
    l->setJustificationType(juce::Justification::centredBottom);
    l->attachToComponent(s, false);
    l->setColour(juce::Label::textColourId, juce::Colours::yellow);

    if (name.isEmpty()) return;

    l->setText(name, juce::dontSendNotification);
}

void SubdominantAudioProcessorEditor::initRadioButtons(juce::ToggleButton* b, const juce::String& name, const int group)
{
    const juce::Font buttonFont = newLookAndFeel.getCustomFont().withHeight(24.f);

    if (b == nullptr) return;

    addAndMakeVisible(*b);
    b->setRadioGroupId(group, juce::NotificationType::dontSendNotification);
    b->setMouseCursor(juce::MouseCursor::PointingHandCursor);

    b->setColour(juce::ToggleButton::tickColourId, juce::Colours::blue);
    b->setColour(juce::ToggleButton::textColourId, juce::Colours::blue);
    b->setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::blue);

    if (name.isEmpty()) return;

    b->setButtonText(name);
}

void SubdominantAudioProcessorEditor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID.containsIgnoreCase("LINE/INST"))
    {
        const bool on = (bool) newValue;
        lineButton.setToggleState(on, juce::dontSendNotification);
        instButton.setToggleState(!on, juce::dontSendNotification);
    }
}
