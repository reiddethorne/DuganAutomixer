/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr auto backgroundColour = 0xff222428;
constexpr auto borderColour     = 0xff42464a;
constexpr auto meterColour      = 0xff54c878;
constexpr auto primaryText      = 0xfff0f1f2;
constexpr auto secondaryText    = 0xff92979d;

constexpr float meterFloorDb = -60.0f;
constexpr float meterInset = 3.0f;
constexpr int scaleLabelHeight = 14;
constexpr float scaleVerticalInset = scaleLabelHeight / 2.0f;
}

//==============================================================================
GainMeter::GainMeter()
{
    setOpaque (true);
}

void GainMeter::setGain (float newGain)
{
    const auto clampedGain = juce::jlimit (0.0f, 1.0f, newGain);

    if (clampedGain == gain)
        return;

    gain = clampedGain;
    repaint();
}

void GainMeter::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff101113));

    const auto bounds = getLocalBounds().toFloat().reduced (meterInset);
    const auto fill = bounds.withTop (bounds.getBottom()
                                      - bounds.getHeight() * gain);

    g.setColour (juce::Colour (meterColour));
    g.fillRect (fill);

    g.setColour (juce::Colour (borderColour));
    g.drawRect (bounds, 1.0f);
}

//==============================================================================
GainMeterBackground::GainMeterBackground()
{
    setBufferedToImage (true);
    addAndMakeVisible (gainMeter);
}

void GainMeterBackground::setGain (float newGain)
{
    gainMeter.setGain (newGain);
}

void GainMeterBackground::paint (juce::Graphics& g)
{
    auto meterBounds = getLocalBounds();
    const auto scaleBounds = meterBounds.removeFromLeft (scaleWidth);
    const auto meterArea = meterBounds.toFloat().reduced (
        0.0f, scaleVerticalInset);

    g.setFont (juce::FontOptions (10.0f));

    // The bar is linear gain, so labels use their equivalent amplitude values.
    const auto drawScaleMark = [&] (float linearGain, const char* label)
    {
        const auto y = meterArea.getBottom()
                       - meterArea.getHeight() * linearGain;

        g.setColour (juce::Colour (borderColour));
        g.drawHorizontalLine (juce::roundToInt (y),
                              meterArea.getX() - 5.0f,
                              meterArea.getX() - 1.0f);

        g.setColour (juce::Colour (secondaryText));
        g.drawText (label, scaleBounds.getX(),
                    juce::roundToInt (y) - scaleLabelHeight / 2,
                    scaleBounds.getWidth() - 9, scaleLabelHeight,
                    juce::Justification::centredRight);
    };

    drawScaleMark (1.0f, "0");
    drawScaleMark (juce::Decibels::decibelsToGain (-3.0f), "-3");
    drawScaleMark (juce::Decibels::decibelsToGain (-6.0f), "-6");
    drawScaleMark (juce::Decibels::decibelsToGain (-12.0f), "-12");
    drawScaleMark (0.0f, "-inf");
}

void GainMeterBackground::resized()
{
    auto meterBounds = getLocalBounds();
    meterBounds.removeFromLeft (scaleWidth);
    meterBounds.reduce (0, juce::roundToInt (scaleVerticalInset - meterInset));
    gainMeter.setBounds (meterBounds);
}

//==============================================================================
ChannelStrip::ChannelStrip()
{
    channelLabel.setJustificationType (juce::Justification::centred);
    channelLabel.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    channelLabel.setColour (juce::Label::textColourId,
                            juce::Colour (primaryText));

    gainValueLabel.setJustificationType (juce::Justification::centred);
    gainValueLabel.setFont (juce::FontOptions (12.0f));
    gainValueLabel.setColour (juce::Label::textColourId,
                              juce::Colour (secondaryText));
    gainValueLabel.setText ("GAIN  -inf dB", juce::dontSendNotification);

    addAndMakeVisible (channelLabel);
    addAndMakeVisible (gainValueLabel);
    addAndMakeVisible (gainMeterBackground);
}

void ChannelStrip::setChannelNumber (int channelNumber)
{
    channelLabel.setText ("INPUT " + juce::String (channelNumber),
                          juce::dontSendNotification);
}

void ChannelStrip::setGain (float newGain)
{
    gainMeterBackground.setGain (newGain);

    const auto gainDb = juce::Decibels::gainToDecibels (
        juce::jlimit (0.0f, 1.0f, newGain), meterFloorDb);
    const auto value = gainDb <= meterFloorDb + 0.1f
                           ? juce::String ("-inf")
                           : juce::String (gainDb, 1);
    gainValueLabel.setText ("GAIN  " + value + " dB",
                            juce::dontSendNotification);
}

void ChannelStrip::resized()
{
    auto area = getLocalBounds().reduced (12, 10);
    channelLabel.setBounds (area.removeFromTop (34));
    gainValueLabel.setBounds (area.removeFromTop (24));
    area.removeFromTop (6);

    auto meterBounds = area.reduced (0, 8).withSizeKeepingCentre (
        92, area.getHeight() - 16);
    meterBounds.translate (-GainMeterBackground::getScaleWidth() / 2, 0);
    gainMeterBackground.setBounds (meterBounds);
}

//==============================================================================
DuganAutomixerAudioProcessorEditor::DuganAutomixerAudioProcessorEditor (DuganAutomixerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

    for (int channel = 0; channel < channelCount; ++channel)
    {
        channelStrips[channel].setChannelNumber (channel + 1);
        addAndMakeVisible (channelStrips[channel]);
    }

    setSize (500, 320);

    // Start the timer to update the meter levels
    startTimerHz(30); // Update at 30 Hz
}

DuganAutomixerAudioProcessorEditor::~DuganAutomixerAudioProcessorEditor()
{
    // Stop the timer when the editor is destroyed
    stopTimer();
}

//==============================================================================
void DuganAutomixerAudioProcessorEditor::timerCallback()
{
    for (int channel = 0; channel < channelCount; ++channel)
    {
        channelStrips[channel].setGain (
            audioProcessor.getMeterGain (channel));
    }
}

//==============================================================================
void DuganAutomixerAudioProcessorEditor::paint (juce::Graphics& g)
{
    // The editor is opaque, so every background pixel must be painted.
    g.fillAll (juce::Colour (backgroundColour));
}

void DuganAutomixerAudioProcessorEditor::resized()
{
    // Divide the available area evenly between channels.
    auto area = getLocalBounds().reduced (18);

    for (int channel = 0; channel < channelCount; ++channel)
    {
        const auto channelsLeft = channelCount - channel;
        channelStrips[channel].setBounds (
            area.removeFromLeft (area.getWidth() / channelsLeft).reduced (6, 0));
    }
}
