/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>

//==============================================================================
/**
*/
class DuganAutomixerAudioProcessor  : public juce::AudioProcessor
{
public:
    static constexpr int automixerChannelCount = 2;

    //==============================================================================
    DuganAutomixerAudioProcessor();
    ~DuganAutomixerAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;


    //==============================================================================
    // Custom method to get the current gain for a specific channel for the meter display
    float getMeterGain (int channel) const noexcept;

private:
    //==============================================================================
    std::array<std::atomic<float>, automixerChannelCount> meterGains
    { 0.0f, 0.0f }; // Atomic variables to hold the current gain for each channel

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DuganAutomixerAudioProcessor)
};
