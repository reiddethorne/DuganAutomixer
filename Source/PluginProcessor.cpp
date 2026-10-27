/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <array>
#include <cmath>

//==============================================================================
DuganAutomixerAudioProcessor::DuganAutomixerAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::discreteChannels(automixerChannelCount), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::discreteChannels(automixerChannelCount), true)
                     #endif
                       )
#endif
{
    for (auto& meterGain : meterGains)
        meterGain.store (0.0f, std::memory_order_relaxed);
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
        const auto seconds = milliseconds * 0.001f;
        return std::exp ( -1.0 / (seconds * sampleRate));
    } ;

    attackCoefficient = coefficientForTime (10.0f);  // 10 ms attack time
    releaseCoefficient = coefficientForTime (100.0f); // 100 ms release time
    gainCoefficient = coefficientForTime (30.0f); // 30 ms gain time

    powerEnvelopes.fill (0.0f);
    smoothedGains.fill (std::sqrt (1.0f / static_cast<float> (automixerChannelCount))); // Start with equal gain for all channels  
    
    bypassMix.reset (sampleRate, 0.01); // 10 ms smoothing time for bypass button
    bypassMix.setCurrentAndTargetValue ( isAutomixerBypassed() ? 0.0f : 1.0f ); 
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
   
    const auto layout = juce::AudioChannelSet::discreteChannels(automixerChannelCount);

    return layouts.getMainInputChannelSet() == layout
        && layouts.getMainOutputChannelSet() == layout;

  #endif
}
#endif

void DuganAutomixerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    const auto numSamples = buffer.getNumSamples();

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
    
    for(int sample = 0; sample < numSamples; ++sample){

        float totalPower = automixerChannelCount * powerFloor;

        for (int channel = 0; channel < totalNumInputChannels; ++channel) 
        {
            // Measure the power of each channel without modifying the audio data
            auto* channelData = buffer.getWritePointer (channel);
            float currentSample = channelData[sample];

            //measure power as square of the sample value
            const float instantaneousPower = currentSample * currentSample; 

            float coefficient = (instantaneousPower > powerEnvelopes[channel]) ? attackCoefficient : releaseCoefficient;
            powerEnvelopes[channel] = coefficient * powerEnvelopes[channel] + (1.0f - coefficient) * instantaneousPower;

            totalPower += powerEnvelopes[channel];
        }

        // Advance bypassMix to get the wet value for this sample
        const auto wet = bypassMix.getNextValue();
        
        for (int channel = 0; channel < automixerChannelCount; ++channel)
        {
            auto* channelData = buffer.getWritePointer (channel);

            const float targetGain = std::sqrt(
                (powerEnvelopes[channel] + powerFloor) 
                / totalPower); 

            smoothedGains[channel] = 
                gainCoefficient * smoothedGains[channel]
                + (1.0f - gainCoefficient) * targetGain;

            const auto effectiveGain = 1.0f + wet * (smoothedGains[channel] - 1.0f);

            channelData[sample] *= effectiveGain;
        }
    }

    for (int channel = 0; channel < automixerChannelCount; ++channel)
    {
        // Update the atomic meter gain for each channel to be used in the GUI
        meterGains[channel].store (smoothedGains[channel], std::memory_order_relaxed);
        // Relaxed memory order is used here because we don't need strict ordering for the meter display, and it can improve performance
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
