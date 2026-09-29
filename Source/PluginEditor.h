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
class DuganAutomixerAudioProcessorEditor  : public juce::AudioProcessorEditor
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DuganAutomixerAudioProcessorEditor)
};
