/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <array>
#include <cmath>

namespace
{
void accumulatePeak (std::atomic<float>& destination, float peak) noexcept
{
    auto previousPeak = destination.load (std::memory_order_relaxed);
    while (previousPeak < peak
           && ! destination.compare_exchange_weak (
               previousPeak, peak, std::memory_order_relaxed))
    {
    }
}
}

//==============================================================================
DuganAutomixerAudioProcessor::DuganAutomixerAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::discreteChannels(automixerChannelCount), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    for (auto& meterGain : meterGains)
        meterGain.store (0.0f, std::memory_order_relaxed);

    for (auto& inputPeak : inputPeaks)
        inputPeak.store (0.0f, std::memory_order_relaxed);

    for (auto& muted : channelMuted)
        muted.store (false, std::memory_order_relaxed);
}

DuganAutomixerAudioProcessor::~DuganAutomixerAudioProcessor()
{
}

//==============================================================================
const juce::String DuganAutomixerAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DuganAutomixerAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool DuganAutomixerAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool DuganAutomixerAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double DuganAutomixerAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int DuganAutomixerAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int DuganAutomixerAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DuganAutomixerAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String DuganAutomixerAudioProcessor::getProgramName (int index)
{
    return {};
}

void DuganAutomixerAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void DuganAutomixerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    // Calculate the smoothing coefficient in milliseconds for gain smoothing
    const auto coefficientForTime = [sampleRate] (float milliseconds)
    {
        const auto seconds = milliseconds * 0.001f; //convert milliseconds to seconds
        return std::exp ( -1.0  // e^(-1/t) formula for smoothing coefficient, exponential decay
            / (seconds * sampleRate)); // calculate number of samples in given time
    } ;

    attackCoefficient = coefficientForTime (10.0f);  // 10 ms attack time
    releaseCoefficient = coefficientForTime (100.0f); // 100 ms release time
    gainCoefficient = coefficientForTime (30.0f); // 30 ms gain time

    powerEnvelopes.fill (0.0f);
    smoothedGains.fill (1.0f / static_cast<float> (automixerChannelCount));

    for (int channel = 0; channel < automixerChannelCount; ++channel)
    {
        muteGains[channel].reset (sampleRate, 0.01); // 10 ms mute fade
        muteGains[channel].setCurrentAndTargetValue (
            isChannelMuted (channel) ? 0.0f : 1.0f);
    }
    
    bypassMix.reset (sampleRate, 0.01); // 10 ms smoothing time for bypass button
    bypassMix.setCurrentAndTargetValue ( isAutomixerBypassed() ? 0.0f : 1.0f ); 
    for (auto& inputPeak : inputPeaks)
        inputPeak.store (0.0f, std::memory_order_relaxed);
    outputPeak.store (0.0f, std::memory_order_relaxed);
}

void DuganAutomixerAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool DuganAutomixerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
   
    const auto inputLayout = juce::AudioChannelSet::discreteChannels (automixerChannelCount);

    return layouts.getMainInputChannelSet() == inputLayout
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();

  #endif
}
#endif

void DuganAutomixerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    const auto numSamples = buffer.getNumSamples();
    auto* leftOutput = buffer.getWritePointer (0);
    auto* rightOutput = buffer.getWritePointer (1);

    std::array<bool, automixerChannelCount> muted {};
    for (int channel = 0; channel < automixerChannelCount; ++channel)
    {
        muted[channel] = isChannelMuted (channel);
        muteGains[channel].setTargetValue (muted[channel] ? 0.0f : 1.0f);
    }

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, numSamples);

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.

    const float powerFloor = 1.0e-9f; // Power floor to avoid division by zero during silence

    bypassMix.setTargetValue (isAutomixerBypassed() ? 0.0f : 1.0f); // Set the target value for the bypassMix based on the bypass state
    std::array<float, automixerChannelCount> blockInputPeaks {};
    float blockOutputPeak = 0.0f;
    
    for(int sample = 0; sample < numSamples; ++sample){

        float totalPower = automixerChannelCount * powerFloor;

        for (int channel = 0; channel < totalNumInputChannels; ++channel) 
        {
            const auto* channelData = buffer.getReadPointer (channel);
            const float currentSample = channelData[sample];
            blockInputPeaks[channel] = juce::jmax (
                blockInputPeaks[channel], std::abs (currentSample));

            // A muted channel must not reduce the gains of the other channels.
            if (muted[channel])
            {
                powerEnvelopes[channel] = 0.0f;
                continue;
            }

            // Measure the power of each channel without modifying the audio data
            // Measure power as the square of the sample value
            const float instantaneousPower = currentSample * currentSample; 

            float coefficient = (instantaneousPower > powerEnvelopes[channel]) ? attackCoefficient : releaseCoefficient;
            powerEnvelopes[channel] = coefficient * powerEnvelopes[channel] + (1.0f - coefficient) * instantaneousPower;

            totalPower += powerEnvelopes[channel];
        }

        // Advance bypassMix to get the wet value for this sample
        const auto wet = bypassMix.getNextValue();
        
        float mixedOutput = 0.0f;

        for (int channel = 0; channel < automixerChannelCount; ++channel)
        {
            const auto* channelData = buffer.getReadPointer (channel);

            // Classic Dugan law: use the linear power share, not its square root.
            const float targetGain =
                (powerEnvelopes[channel] + powerFloor) / totalPower;

            // Smooth gain for each channel using gainCoefficient
            smoothedGains[channel] =
                gainCoefficient * smoothedGains[channel]
                + (1.0f - gainCoefficient) * targetGain;
            const auto effectiveGain = 1.0f + wet * (smoothedGains[channel] - 1.0f);
            const auto muteGain = muteGains[channel].getNextValue();

            // Mute remains active when the automixer is bypassed. The smoothed
            // mute gain prevents discontinuities when the button is toggled.
            mixedOutput += channelData[sample] * effectiveGain * muteGain;
        }

        // The automixer produces one mono mix, sent equally to both sides of
        // the stereo output bus.
        leftOutput[sample] = mixedOutput;
        rightOutput[sample] = mixedOutput;
        blockOutputPeak = juce::jmax (blockOutputPeak, std::abs (mixedOutput));
    }

    // Keep the highest peaks until the UI reads them.
    for (int channel = 0; channel < automixerChannelCount; ++channel)
        accumulatePeak (inputPeaks[channel], blockInputPeaks[channel]);

    accumulatePeak (outputPeak, blockOutputPeak);

    const auto wet = bypassMix.getCurrentValue();
    for (int channel = 0; channel < automixerChannelCount; ++channel)
    {
        // Update the atomic meter gain for each channel to be used in the GUI
        meterGains[channel].store (
            (1.0f + wet * (smoothedGains[channel] - 1.0f))
                * muteGains[channel].getCurrentValue(),
            std::memory_order_relaxed);
        // Relaxed memory order is where the atomic value is still safe, but there is no ordering guarantee for other memory operations.
        // It is used here because we don't need strict ordering for the meter display, and it can improve performance.
    }
}

//==============================================================================
float DuganAutomixerAudioProcessor::getMeterGain (
    int channel) const noexcept
{
    if (! juce::isPositiveAndBelow (channel, automixerChannelCount))
        return 0.0f;

    return meterGains[channel].load (
        std::memory_order_relaxed);
}

float DuganAutomixerAudioProcessor::getAndClearOutputPeak() noexcept
{
    return outputPeak.exchange (0.0f, std::memory_order_relaxed);
}

float DuganAutomixerAudioProcessor::getAndClearInputPeak (
    int channel) noexcept
{
    if (! juce::isPositiveAndBelow (channel, automixerChannelCount))
        return 0.0f;

    return inputPeaks[channel].exchange (0.0f, std::memory_order_relaxed);
}

//==============================================================================
bool DuganAutomixerAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* DuganAutomixerAudioProcessor::createEditor()
{
    return new DuganAutomixerAudioProcessorEditor (*this);
}

//==============================================================================
void DuganAutomixerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void DuganAutomixerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DuganAutomixerAudioProcessor();
}
