/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DuganAutomixerAudioProcessorEditor::DuganAutomixerAudioProcessorEditor (DuganAutomixerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    titleLabel.setText("Dugan Automixer", juce::dontSendNotification);
    channelOneLabel.setText("Channel 1", juce::dontSendNotification);
    channelTwoLabel.setText("Channel 2", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    channelOneLabel.setJustificationType(juce::Justification::centred);
    channelTwoLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    addAndMakeVisible(channelOneLabel);
    addAndMakeVisible(channelTwoLabel);
    titleLabel.setFont(
        juce::FontOptions(20.0f, juce::Font::bold));
    channelOneLabel.setFont(juce::FontOptions(15.0f));
    channelTwoLabel.setFont(juce::FontOptions(15.0f));
    setSize (500, 320);
}

DuganAutomixerAudioProcessorEditor::~DuganAutomixerAudioProcessorEditor()
{
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
    auto area = getLocalBounds().reduced (20);

    titleLabel.setBounds (area.removeFromTop (50));

    auto leftArea =
        area.removeFromLeft (area.getWidth() / 2);

    channelOneLabel.setBounds (
        leftArea.removeFromTop (35).reduced (5));

    channelTwoLabel.setBounds (
        area.removeFromTop (35).reduced (5));
}
