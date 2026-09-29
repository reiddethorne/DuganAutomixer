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
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

    // Set the text for the labels and their justification
    titleLabel.setText("Dugan Automixer", juce::dontSendNotification);
    channelOneLabel.setText("Channel 1", juce::dontSendNotification);
    channelTwoLabel.setText("Channel 2", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    channelOneLabel.setJustificationType(juce::Justification::centred);
    channelTwoLabel.setJustificationType(juce::Justification::centred);
    // Add the labels to the editor and set their font sizes
    addAndMakeVisible(titleLabel);
    addAndMakeVisible(channelOneLabel);
    addAndMakeVisible(channelTwoLabel);
    titleLabel.setFont(
        juce::FontOptions(20.0f, juce::Font::bold));
    channelOneLabel.setFont(juce::FontOptions(15.0f));
    channelTwoLabel.setFont(juce::FontOptions(15.0f));
    setSize (500, 320);

    // Start the timer to update the meter levels
    startTimerHz(30); // Update at 30 Hz
}

DuganAutomixerAudioProcessorEditor::~DuganAutomixerAudioProcessorEditor()
{
    // Stop the timer when the editor is destroyed
    stopTimer();
}

//==============================================================================
void DuganAutomixerAudioProcessorEditor::timerCallback()
{
    channelOneMeterLevel = audioProcessor.getMeterGain (0);
    channelTwoMeterLevel = audioProcessor.getMeterGain (1);

    repaint();
}

//==============================================================================
void DuganAutomixerAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    // Draw meter backgrounds
    g.setColour (juce::Colours::black.withAlpha(0.4f));
    g.fillRect(channelOneMeterBounds.toFloat());
    g.fillRect(channelTwoMeterBounds.toFloat());

    // Limit the meter levels to the range [0, 1]
    const auto channelOneLevel = juce::jlimit (
    0.0f, 1.0f, channelOneMeterLevel);

    const auto channelTwoLevel = juce::jlimit (
    0.0f, 1.0f, channelTwoMeterLevel);

    
    // Draw the fill for channel one
    auto channelOneFill = channelOneMeterBounds.toFloat();

    const auto channelOneFillHeight =
    channelOneFill.getHeight() * channelOneLevel;

    const auto channelOneBottom = channelOneFill.getBottom();

    channelOneFill.setY (
        channelOneBottom - channelOneFillHeight);

    channelOneFill.setHeight (channelOneFillHeight);

    //Draw the fill for channel two

    auto channelTwoFill = channelTwoMeterBounds.toFloat();

    const auto channelTwoFillHeight =
    channelTwoFill.getHeight() * channelTwoLevel;

    const auto channelTwoBottom = channelTwoFill.getBottom();

    channelTwoFill.setY (
        channelTwoBottom - channelTwoFillHeight);

    channelTwoFill.setHeight (channelTwoFillHeight);

    // Set the fill color and draw the filled rectangles for both channels
    g.setColour (juce::Colours::limegreen);
    g.fillRect (channelOneFill);
    g.fillRect (channelTwoFill);

    // Draw meter outlines last so they are on top of the fill
    g.setColour (juce::Colours::grey);
    g.drawRect(channelOneMeterBounds.toFloat());
    g.drawRect(channelTwoMeterBounds.toFloat());

    

}

void DuganAutomixerAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    // Get the bounds of the editor and reduce it by 20 pixels on all sides and 50 pixels from the top for the title
    auto area = getLocalBounds().reduced (20);
    titleLabel.setBounds (area.removeFromTop (50));

    // Split the remaining area into halves for the two channels
    auto leftArea =
        area.removeFromLeft (area.getWidth() / 2);
    // Set the bounds for the channel labels and meters
    channelOneLabel.setBounds (
        leftArea.removeFromTop (35).reduced (5));

    channelTwoLabel.setBounds (
        area.removeFromTop (35).reduced (5));

    // After removing the label area from each channel region, use the remaining area for the meters
    channelOneMeterBounds = leftArea.reduced (60, 10);
    channelTwoMeterBounds = area.reduced (60, 10);
}
