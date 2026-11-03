// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "PluginProcessor.h"

class ScenePad final : public juce::Component, private juce::Timer {
public:
    explicit ScenePad(juce::AudioProcessorValueTreeState&);
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
private:
    void timerCallback() override;
    float lastX=-1,lastY=-1;
    juce::RangedAudioParameter *x,*y;
};
class SynthControl final : public juce::Component {
public:
    SynthControl(juce::AudioProcessorValueTreeState&,const aetherloom::ParameterSpec&);
    void resized() override;
    aetherloom::Page page;
    bool hybridOnly=false;
private:
    juce::Label label;
    juce::Slider slider;
    juce::ComboBox choice;
    bool isChoice=false,isRoute=false;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> choiceAttachment;
};
class AetherloomEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit AetherloomEditor(AetherloomProcessor&);
    ~AetherloomEditor() override { setLookAndFeel(nullptr); }
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    juce::LookAndFeel_V4 style;
    AetherloomProcessor& processor;
    juce::TextButton play{"PLAY"},edit{"EDIT"},reload{"RELOAD"},randomise{"MUTATE"},save{"SAVE"},load{"LOAD"},panic{"PANIC"};
    juce::ComboBox engine,mode,patches,category;
    juce::TextEditor search;
    juce::Label status;
    juce::MidiKeyboardComponent keyboard;
    ScenePad pad;
    juce::Viewport viewport;
    juce::Component panel;
    std::array<juce::TextButton,6> pages;
    std::vector<std::unique_ptr<SynthControl>> controls;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> engineAttachment,modeAttachment;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::TooltipWindow tooltips{this,500};
    aetherloom::Page currentPage=aetherloom::Page::Oscillators;
    bool editing=false;
    float displayedPeak=0;
    int lastProgram=-1;
    bool lastEdited=false;
    void setEditing(bool);
    void filterPatches();
    void chooseFile(bool writing);
    void timerCallback() override;
};
