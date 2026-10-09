/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>

//==============================================================================
// A self-contained gain meter. Each instance stores and draws one channel's
// current automixer gain, which makes it easy to add more meters later.
class GainMeter : public juce::Component
{
public:
    GainMeter() = default;
    ~GainMeter() override = default;

    void setGain (float newGain);
    void paint (juce::Graphics& g) override;

private:
    float gain = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GainMeter)
};

//==============================================================================
// Groups the label and gain meter belonging to one automixer channel.
class ChannelStrip : public juce::Component
{
public:
    ChannelStrip();

    void setChannelNumber (int channelNumber);
    void setGain (float newGain);
    void resized() override;

private:
    juce::Label channelLabel;
    GainMeter gainMeter;

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
    juce::Label titleLabel;

    static constexpr int channelCount =
        DuganAutomixerAudioProcessor::automixerChannelCount;

    std::array<ChannelStrip, channelCount> channelStrips;

    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DuganAutomixerAudioProcessorEditor)
};
