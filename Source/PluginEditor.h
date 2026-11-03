// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "PluginProcessor.h"

class ScenePad final : public juce::Component, private juce::Timer {
public:
    explicit ScenePad(juce::AudioProcessorValueTreeState& state);
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
private:
    void timerCallback() override { repaint(); }
    juce::RangedAudioParameter *x, *y;
};
class AetherloomEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit AetherloomEditor(AetherloomProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    AetherloomProcessor& processor;
    juce::TextButton play{"PLAY"}, edit{"EDIT"};
    juce::ComboBox mode, patches;
    juce::MidiKeyboardComponent keyboard;
    ScenePad pad;
    std::array<juce::Slider, 15> sliders;
    std::array<juce::Label, 15> labels;
    std::vector<std::unique_ptr<SliderAttachment>> attachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;
    juce::TooltipWindow tooltips{this, 500};
    bool editing = false;
    void setEditing(bool);
    void timerCallback() override;
};
