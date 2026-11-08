/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>
#include <functional>

//==============================================================================
// Draws only the animated portion of one channel's gain meter. The component
// is opaque so repainting it never requires JUCE to redraw the static artwork
// behind it.
class GainMeter : public juce::Component
{
public:
    GainMeter();
    ~GainMeter() override = default;

    void setGain (float newGain);
    void paint (juce::Graphics& g) override;

private:
    float gain = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GainMeter)
};

//==============================================================================
// Owns the static meter scale and the animated meter layered on top of it.
// The scale is buffered because it changes only when this component is resized.
class GainMeterBackground : public juce::Component
{
public:
    GainMeterBackground();
    ~GainMeterBackground() override = default;

    void setGain (float newGain);
    void paint (juce::Graphics& g) override;
    void resized() override;

    static constexpr int getScaleWidth() noexcept { return scaleWidth; }

private:
    static constexpr int scaleWidth = 40;
    GainMeter gainMeter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GainMeterBackground)
};

// Input level meter with a dBFS scale.
class InputMeterBackground : public juce::Component
{
public:
    InputMeterBackground();
    ~InputMeterBackground() override = default;

    void setLevel (float newLevel);
    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    static constexpr int scaleWidth = 32;
    GainMeter meter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InputMeterBackground)
};

//==============================================================================
// Groups the label and gain meter belonging to one automixer channel.
class ChannelStrip : public juce::Component
{
public:
    ChannelStrip();

    void setChannelNumber (int channelNumber);
    void setInputPeak (float newPeak);
    void setGain (float newGain);
    void setMuted (bool shouldBeMuted);
    void resized() override;

    std::function<void (bool)> onMuteChanged;

private:
    juce::Label channelLabel;
    juce::Label inputValueLabel;
    juce::Label gainValueLabel;
    InputMeterBackground inputMeter;
    GainMeterBackground gainMeterBackground;
    juce::ToggleButton muteButton { "Mute" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChannelStrip)
};

//==============================================================================
/**
*/
class DuganAutomixerAudioProcessorEditor  : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    DuganAutomixerAudioProcessorEditor (DuganAutomixerAudioProcessor&);
    ~DuganAutomixerAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    DuganAutomixerAudioProcessor& audioProcessor;

    static constexpr int channelCount =
        DuganAutomixerAudioProcessor::automixerChannelCount;

    std::array<ChannelStrip, channelCount> channelStrips;
    juce::ToggleButton bypassButton { "Automix On" };
    juce::Label outputPeakLabel;
    InputMeterBackground outputPeakMeter;

    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DuganAutomixerAudioProcessorEditor)
};
