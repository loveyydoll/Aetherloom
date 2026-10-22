// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "PluginProcessor.h"

class AetherloomEditor final : public juce::AudioProcessorEditor
{
public:
    explicit AetherloomEditor(AetherloomProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    juce::TextButton play{"PLAY"}, edit{"EDIT"};
    juce::ComboBox mode;
    juce::MidiKeyboardComponent keyboard;
    std::array<juce::Slider, 7> sliders;
    std::array<juce::Label, 7> labels;
    std::vector<std::unique_ptr<SliderAttachment>> attachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;
    bool editing = false;
    void setEditing(bool);
};
