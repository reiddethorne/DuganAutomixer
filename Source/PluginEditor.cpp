/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
void GainMeter::setGain (float newGain)
{
    gain = juce::jlimit (0.0f, 1.0f, newGain);
    repaint();
}

void GainMeter::paint (juce::Graphics& g)
{
    // Keep some space on the left side of the component for the scale.
    auto meterBounds = getLocalBounds();
    auto scaleBounds = meterBounds.removeFromLeft (40);
    const auto meterBoundsFloat = meterBounds.toFloat();

    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillRect (meterBoundsFloat);

    // Reduce the fill from the top so that it grows upward from the bottom.
    auto fillBounds = meterBoundsFloat;
    const auto fillHeight = fillBounds.getHeight() * gain;
    fillBounds.setY (fillBounds.getBottom() - fillHeight);
    fillBounds.setHeight (fillHeight);

    g.setColour (juce::Colours::limegreen);
    g.fillRect (fillBounds);

    g.setColour (juce::Colours::grey);
    g.drawRect (meterBoundsFloat);

    // Convert each dB value to linear gain so its mark matches the meter fill.
    const auto drawScaleMark = [&] (float linearGain, const char* label)
    {
        const auto markY = meterBoundsFloat.getBottom()
                           - meterBoundsFloat.getHeight() * linearGain;
        const auto labelY = juce::jlimit (0,
                                         getHeight() - 16,
                                         juce::roundToInt (markY) - 8);

        g.drawHorizontalLine (juce::roundToInt (markY),
                              meterBoundsFloat.getX() - 5.0f,
                              meterBoundsFloat.getX());

        g.drawText (label,
                    scaleBounds.getX(),
                    labelY,
                    scaleBounds.getWidth() - 8,
                    16,
                    juce::Justification::centredRight);
    };

    drawScaleMark (1.0f, "0");
    drawScaleMark (juce::Decibels::decibelsToGain (-3.0f), "-3");
    drawScaleMark (juce::Decibels::decibelsToGain (-6.0f), "-6");
    drawScaleMark (juce::Decibels::decibelsToGain (-12.0f), "-12");
    drawScaleMark (0.0f, "-inf");
}

//==============================================================================
ChannelStrip::ChannelStrip()
{
    channelLabel.setJustificationType (juce::Justification::centred);
    channelLabel.setFont (juce::FontOptions (15.0f));

    addAndMakeVisible (channelLabel);
    addAndMakeVisible (gainMeter);
}

void ChannelStrip::setChannelNumber (int channelNumber)
{
    channelLabel.setText ("Channel " + juce::String (channelNumber),
                          juce::dontSendNotification);
}

void ChannelStrip::setGain (float newGain)
{
    gainMeter.setGain (newGain);
}

void ChannelStrip::resized()
{
    auto area = getLocalBounds();
    channelLabel.setBounds (area.removeFromTop (35).reduced (5));

    // Leave room around the meter while keeping its scale on the left.
    gainMeter.setBounds (
        area.withTrimmedLeft (20).withTrimmedRight (60).reduced (0, 10));
}

//==============================================================================
DuganAutomixerAudioProcessorEditor::DuganAutomixerAudioProcessorEditor (DuganAutomixerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

    // Set up the title and each repeated channel strip.
    titleLabel.setText("Dugan Automixer", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    titleLabel.setFont(
        juce::FontOptions(20.0f, juce::Font::bold));

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
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void DuganAutomixerAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    // Get the bounds of the editor and reduce it by 20 pixels on all sides and 50 pixels from the top for the title
    auto area = getLocalBounds().reduced (20);
    titleLabel.setBounds (area.removeFromTop (50));

    // Divide the remaining width evenly between all channel strips.
    for (int channel = 0; channel < channelCount; ++channel)
    {
        const auto remainingChannels = channelCount - channel;
        const auto stripWidth = area.getWidth() / remainingChannels;

        channelStrips[channel].setBounds (
            area.removeFromLeft (stripWidth));
    }
}
