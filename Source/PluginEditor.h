/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"


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
    juce::Label channelOneLabel;
    juce::Label channelTwoLabel;
    juce::Rectangle<int> channelOneMeterBounds;
    juce::Rectangle<int> channelTwoMeterBounds;

    void timerCallback() override;
    float channelOneMeterLevel = 0.0f;
    float channelTwoMeterLevel = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DuganAutomixerAudioProcessorEditor)
};
