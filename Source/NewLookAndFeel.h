/*
  ==============================================================================

    NewLookAndFeel.h
    Created: 14 Jan 2024 7:07:33am
    Author:  _astriid_

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
class NewLookAndFeel  : public juce::LookAndFeel_V4
{
public:
    NewLookAndFeel()
    {
        bauTypeface = juce::Typeface::createSystemTypefaceFor(BinaryData::typoround_bold_otf, BinaryData::typoround_bold_otfSize);
        bauFont = juce::Font(bauTypeface);
    }

    ~NewLookAndFeel() override {}

    juce::Font getCustomFont()
    {
        return bauFont;
    }

    juce::Font getLabelFont(juce::Label& label) override
    {
        const float height = label.getFont().getHeight();
        return bauFont.withHeight(height);
    }


    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
        const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider&) override
    {
        auto radius = (float)juce::jmin(width / 2, height / 2) - 4.0f;
        auto centreX = (float)x + (float)width * 0.5f;
        auto centreY = (float)y + (float)height * 0.5f;
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // fill
        g.setColour(juce::Colours::darkred);
        g.fillEllipse(rx, ry, rw, rw);

        // outline
        g.setColour(juce::Colours::green);
        g.drawEllipse(rx, ry, rw, rw, 4.0f);

        juce::Path p;
        auto pointerLength = radius * 0.33f;
        p.addEllipse(-pointerLength * 0.5f, -radius * 0.8f, pointerLength, pointerLength);
        p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));

        // pointer
        g.setColour(juce::Colours::yellow);
        g.fillPath(p);
    }

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
        bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto fontSize = (float)button.getHeight()   ;
        auto tickWidth = fontSize * 1.1f;

        drawTickBox(g, button, 4.0f, ((float)button.getHeight() - tickWidth) * 0.5f,
            tickWidth, tickWidth,
            button.getToggleState(),
            button.isEnabled(),
            shouldDrawButtonAsHighlighted,
            shouldDrawButtonAsDown);

        g.setColour(button.findColour(juce::ToggleButton::textColourId));
        g.setFont(bauFont.withHeight(fontSize));

        if (!button.isEnabled())
            g.setOpacity(0.5f);

        g.drawFittedText(button.getButtonText(),
            button.getLocalBounds().withTrimmedLeft(juce::roundToInt(tickWidth) + 10)
            .withTrimmedRight(2),
            juce::Justification::centredLeft, 10);
    }

    void drawTickBox(juce::Graphics& g, juce::Component& component, float x, float y, float w, float h, const bool ticked,
        const bool /*isEnabled*/, const bool /*shouldDrawButtonAsHighlighted*/, const bool /*shouldDrawButtonAsDown*/) override
    {
        juce::Rectangle<float> tickBounds(x, y, w, h);

        if (ticked)
        {
            g.setColour(component.findColour(juce::ToggleButton::tickColourId));
            g.fillRect(tickBounds);
        }
        else
        {
            g.setColour(component.findColour(juce::ToggleButton::tickDisabledColourId));
            g.drawRect(tickBounds, 2.0f);
        }
    }

private:
    juce::Font bauFont;
    juce::Typeface::Ptr bauTypeface;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NewLookAndFeel)
};
