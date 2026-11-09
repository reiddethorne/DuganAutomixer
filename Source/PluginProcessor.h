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
    static constexpr int automixerChannelCount = 4; // Number of channels for the automixer

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
    float getAndClearInputPeak (int channel) noexcept;
    float getAndClearOutputPeak() noexcept;

    void setOutputGainDb (float newGainDb) noexcept;
    float getOutputGainDb() const noexcept;
    void setInputName (int channel, const juce::String& newName,
                       bool notifyHost = true);
    juce::String getInputName (int channel) const;

    void setAutomixerBypassed (bool shouldBypass) noexcept
    {
        bypassed.store (shouldBypass, std::memory_order_relaxed);
    }

    bool isAutomixerBypassed() const noexcept
    {
        return bypassed.load (std::memory_order_relaxed);
    }

    void setChannelMuted (int channel, bool shouldBeMuted) noexcept
    {
        if (juce::isPositiveAndBelow (channel, automixerChannelCount))
            channelMuted[channel].store (shouldBeMuted, std::memory_order_relaxed);
    }

    bool isChannelMuted (int channel) const noexcept
    {
        return juce::isPositiveAndBelow (channel, automixerChannelCount)
            && channelMuted[channel].load (std::memory_order_relaxed);
    }

private:
    //==============================================================================
    std::array<std::atomic<float>, automixerChannelCount> meterGains {}; // Atomic variables to hold the current gain for each channel
    std::array<std::atomic<float>, automixerChannelCount> inputPeaks {};
    std::atomic<float> outputPeak { 0.0f };
    std::atomic<float> outputGainDb { 0.0f };
    std::atomic<bool> bypassed { false };
    std::array<std::atomic<bool>, automixerChannelCount> channelMuted {};
    std::array<juce::String, automixerChannelCount> inputNames;
    mutable juce::CriticalSection inputNamesLock;
    std::array<float, automixerChannelCount> powerEnvelopes {};
    std::array<float, automixerChannelCount> smoothedGains {};
    std::array<juce::SmoothedValue<float>, automixerChannelCount> muteGains;

    float attackCoefficient = 0.0f;
    float releaseCoefficient = 0.0f;
    float gainCoefficient = 0.0f;

    juce::SmoothedValue<float> bypassMix {1.0f};
    juce::SmoothedValue<float> outputGain {1.0f};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DuganAutomixerAudioProcessor)
};
