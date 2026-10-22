// SPDX-License-Identifier: AGPL-3.0-only
#include "PluginEditor.h"

AetherloomEditor::AetherloomEditor(AetherloomProcessor& processor)
    : AudioProcessorEditor(processor), keyboard(processor.keyboard, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    addAndMakeVisible(play); addAndMakeVisible(edit); addAndMakeVisible(mode); addAndMakeVisible(keyboard);
    play.onClick = [this] { setEditing(false); };
    edit.onClick = [this] { setEditing(true); };
    mode.addItemList({"WARM", "BLEND", "GLITCH"}, 1);
    modeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.parameters, "mode", mode);
    const std::array<const char*, 7> ids{"colour", "gain", "cutoff", "attack", "decay", "sustain", "release"};
    const std::array<const char*, 7> names{"Colour", "Level", "Cutoff (Hz)", "Attack (s)", "Decay (s)", "Sustain", "Release (s)"};
    for (size_t i = 0; i < sliders.size(); ++i)
    {
        sliders[i].setSliderStyle(juce::Slider::LinearHorizontal);
        sliders[i].setTextBoxStyle(juce::Slider::TextBoxRight, false, 85, 24);
        labels[i].setText(names[i], juce::dontSendNotification);
        addAndMakeVisible(sliders[i]); addAndMakeVisible(labels[i]);
        attachments.push_back(std::make_unique<SliderAttachment>(processor.parameters, ids[i], sliders[i]));
    }
    setSize(680, 510);
    setEditing(false);
}
void AetherloomEditor::setEditing(bool value)
{
    editing = value;
    play.setToggleState(!value, juce::dontSendNotification);
    edit.setToggleState(value, juce::dontSendNotification);
    for (size_t i = 2; i < sliders.size(); ++i) { sliders[i].setVisible(value); labels[i].setVisible(value); }
    repaint();
}
void AetherloomEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff191b27));
    g.setColour(juce::Colour(0xffe7cdb4));
    g.setFont(26.0f); g.drawText("AETHERLOOM", 24, 12, 300, 40, juce::Justification::centredLeft);
    g.setFont(14.0f);
    g.drawText(editing ? "EDIT / envelope + low-pass filter" : "PLAY / choose a character and play the keys", 24, 56, 620, 24, juce::Justification::centredLeft);
    if (!editing) g.drawText("Early character engine / deeper sound design planned", 24, 240, 620, 50, juce::Justification::centred);
}
void AetherloomEditor::resized()
{
    play.setBounds(365, 20, 80, 30); edit.setBounds(455, 20, 80, 30); mode.setBounds(545, 20, 110, 30);
    for (size_t i = 0; i < sliders.size(); ++i)
    {
        const auto y = 90 + static_cast<int>(i) * 42;
        labels[i].setBounds(24, y, 105, 32); sliders[i].setBounds(135, y, getWidth() - 160, 32);
    }
    keyboard.setBounds(24, getHeight() - 100, getWidth() - 48, 76);
}
