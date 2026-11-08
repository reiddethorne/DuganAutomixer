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

float levelToMeterPosition (float level)
{
    const auto levelDb = juce::Decibels::gainToDecibels (level, meterFloorDb);
    return juce::jlimit (0.0f, 1.0f,
                         (levelDb - meterFloorDb) / -meterFloorDb);
}
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
InputMeterBackground::InputMeterBackground()
{
    setBufferedToImage (true);
    addAndMakeVisible (meter);
}

void InputMeterBackground::setLevel (float newLevel)
{
    meter.setGain (levelToMeterPosition (newLevel));
}

void InputMeterBackground::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    const auto scaleBounds = bounds.removeFromLeft (scaleWidth);
    const auto meterArea = bounds.toFloat().reduced (0.0f,
                                                     scaleVerticalInset);

    g.setFont (juce::FontOptions (9.0f));

    const auto drawMark = [&] (float decibels, const char* label)
    {
        const auto proportion = (decibels - meterFloorDb) / -meterFloorDb;
        const auto y = meterArea.getBottom()
                       - meterArea.getHeight() * proportion;

        g.setColour (juce::Colour (borderColour));
        g.drawHorizontalLine (juce::roundToInt (y),
                              meterArea.getX() - 4.0f,
                              meterArea.getX() - 1.0f);

        g.setColour (juce::Colour (secondaryText));
        g.drawText (label, scaleBounds.getX(),
                    juce::roundToInt (y) - scaleLabelHeight / 2,
                    scaleBounds.getWidth() - 7, scaleLabelHeight,
                    juce::Justification::centredRight);
    };

    drawMark (0.0f, "0");
    drawMark (-12.0f, "-12");
    drawMark (-24.0f, "-24");
    drawMark (-48.0f, "-48");
    drawMark (-60.0f, "-60");
}

void InputMeterBackground::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromLeft (scaleWidth);
    bounds.reduce (0, juce::roundToInt (scaleVerticalInset - meterInset));
    meter.setBounds (bounds);
}

//==============================================================================
ChannelStrip::ChannelStrip()
{
    channelLabel.setJustificationType (juce::Justification::centred);
    channelLabel.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    channelLabel.setColour (juce::Label::textColourId,
                            juce::Colour (primaryText));

    inputValueLabel.setJustificationType (juce::Justification::centred);
    inputValueLabel.setFont (juce::FontOptions (11.0f));
    inputValueLabel.setColour (juce::Label::textColourId,
                               juce::Colour (secondaryText));
    inputValueLabel.setText ("IN  -inf dBFS", juce::dontSendNotification);

    gainValueLabel.setJustificationType (juce::Justification::centred);
    gainValueLabel.setFont (juce::FontOptions (12.0f));
    gainValueLabel.setColour (juce::Label::textColourId,
                              juce::Colour (secondaryText));
    gainValueLabel.setText ("GAIN  -inf dB", juce::dontSendNotification);

    addAndMakeVisible (channelLabel);
    addAndMakeVisible (inputValueLabel);
    addAndMakeVisible (gainValueLabel);
    addAndMakeVisible (inputMeter);
    addAndMakeVisible (gainMeterBackground);
    addAndMakeVisible (muteButton);

    muteButton.onClick = [this]
    {
        if (onMuteChanged)
            onMuteChanged (muteButton.getToggleState());
    };
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

void ChannelStrip::setInputPeak (float newPeak)
{
    inputMeter.setLevel (newPeak);

    const auto peakDb = juce::Decibels::gainToDecibels (newPeak, meterFloorDb);
    const auto value = peakDb <= meterFloorDb + 0.1f
                           ? juce::String ("-inf")
                           : juce::String (peakDb, 1);
    inputValueLabel.setText ("IN  " + value + " dBFS",
                             juce::dontSendNotification);
}

void ChannelStrip::setMuted (bool shouldBeMuted)
{
    muteButton.setToggleState (shouldBeMuted, juce::dontSendNotification);
}

void ChannelStrip::resized()
{
    auto area = getLocalBounds().reduced (6, 10);
    channelLabel.setBounds (area.removeFromTop (30));
    inputValueLabel.setBounds (area.removeFromTop (20));
    gainValueLabel.setBounds (area.removeFromTop (20));
    muteButton.setBounds (area.removeFromBottom (26));
    area.removeFromTop (6);

    auto meterBounds = area.reduced (0, 8).withSizeKeepingCentre (
        142, area.getHeight() - 16);
    const auto inputMeterBounds = meterBounds.removeFromLeft (50);
    meterBounds.removeFromLeft (4);
    gainMeterBackground.setBounds (meterBounds);
    inputMeter.setBounds (inputMeterBounds);
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
        channelStrips[channel].setMuted (audioProcessor.isChannelMuted (channel));
        channelStrips[channel].onMuteChanged = [this, channel] (bool shouldBeMuted)
        {
            audioProcessor.setChannelMuted (channel, shouldBeMuted);
        };
        addAndMakeVisible (channelStrips[channel]);
    }

    setSize (840, 320);

    // Start the timer to update the meter levels
    startTimerHz(30); // Update at 30 Hz

    bypassButton.setToggleState (! audioProcessor.isAutomixerBypassed(),
                                 juce::dontSendNotification);
    bypassButton.onClick = [this]
    {
        audioProcessor.setAutomixerBypassed (
            ! bypassButton.getToggleState());
    };

    addAndMakeVisible (bypassButton);

    outputPeakLabel.setText ("OUT  -inf dBFS", juce::dontSendNotification);
    outputPeakLabel.setJustificationType (juce::Justification::centred);
    outputPeakLabel.setColour (juce::Label::textColourId,
                               juce::Colour (secondaryText));
    addAndMakeVisible (outputPeakLabel);
    addAndMakeVisible (outputPeakMeter);

    outputGainLabel.setText ("GAIN", juce::dontSendNotification);
    outputGainLabel.setJustificationType (juce::Justification::centred);
    outputGainLabel.setColour (juce::Label::textColourId,
                               juce::Colour (secondaryText));

    outputGainSlider.setSliderStyle (juce::Slider::LinearVertical);
    outputGainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow,
                                      false, 70, 18);
    outputGainSlider.setRange (-24.0, 6.0, 0.1);
    outputGainSlider.setTextValueSuffix (" dB");
    outputGainSlider.setValue (audioProcessor.getOutputGainDb(),
                               juce::dontSendNotification);
    outputGainSlider.onValueChange = [this]
    {
        audioProcessor.setOutputGainDb (
            static_cast<float> (outputGainSlider.getValue()));
    };

    addAndMakeVisible (outputGainLabel);
    addAndMakeVisible (outputGainSlider);
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
        channelStrips[channel].setInputPeak (
            audioProcessor.getAndClearInputPeak (channel));
        channelStrips[channel].setMuted (
            audioProcessor.isChannelMuted (channel));
    }

    bypassButton.setToggleState (
        ! audioProcessor.isAutomixerBypassed(),
        juce::dontSendNotification);
    outputGainSlider.setValue (
        audioProcessor.getOutputGainDb(),
        juce::dontSendNotification);

    const auto peak = audioProcessor.getAndClearOutputPeak();
    outputPeakMeter.setLevel (peak);
    const auto peakDb = juce::Decibels::gainToDecibels (peak, meterFloorDb);
    const auto value = peakDb <= meterFloorDb + 0.1f
                           ? juce::String ("-inf")
                           : juce::String (peakDb, 1);
    outputPeakLabel.setText ("OUT  " + value + " dBFS",
                             juce::dontSendNotification);
    outputPeakLabel.setColour (juce::Label::textColourId,
                               peak >= 1.0f ? juce::Colours::red
                                            : juce::Colour (secondaryText));
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
    
    auto controls = area.removeFromRight (110);
    bypassButton.setBounds (controls.removeFromTop (32));
    outputPeakLabel.setBounds (controls.removeFromTop (28));
    auto gainArea = controls.removeFromRight (48);
    outputGainLabel.setBounds (gainArea.removeFromTop (18));
    outputGainSlider.setBounds (gainArea);
    outputPeakMeter.setBounds (
        controls.withSizeKeepingCentre (50, controls.getHeight() - 12));

    area.removeFromTop (6);

    for (int channel = 0; channel < channelCount; ++channel)
    {
        const auto channelsLeft = channelCount - channel;
        channelStrips[channel].setBounds (
            area.removeFromLeft (area.getWidth() / channelsLeft).reduced (6, 0));
    }
}
