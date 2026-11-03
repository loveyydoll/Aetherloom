// SPDX-License-Identifier: AGPL-3.0-only
#include "PluginEditor.h"

ScenePad::ScenePad(juce::AudioProcessorValueTreeState& state)
    : x(state.getParameter("morphX")), y(state.getParameter("morphY")) {
    setTitle("Scene blend"); setDescription("Glow, Glass, Haze and Fracture; use EDIT for keyboard-accessible X/Y sliders.");
    startTimerHz(30);
}
void ScenePad::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff292d42)); g.setColour(juce::Colour(0xffe7cdb4)); g.setFont(14.0f);
    g.drawText("GLOW", 10, 8, 100, 24, juce::Justification::left);
    g.drawText("GLASS", getWidth()-110, 8, 100, 24, juce::Justification::right);
    g.drawText("HAZE", 10, getHeight()-32, 100, 24, juce::Justification::left);
    g.drawText("FRACTURE", getWidth()-110, getHeight()-32, 100, 24, juce::Justification::right);
    const auto px = 12.0f+x->getValue()*static_cast<float>(getWidth()-24);
    const auto py = 12.0f+y->getValue()*static_cast<float>(getHeight()-24);
    g.setColour(juce::Colour(0xffaa9de8)); g.fillEllipse(px-7, py-7, 14, 14);
}
void ScenePad::mouseDown(const juce::MouseEvent& event) { x->beginChangeGesture(); y->beginChangeGesture(); mouseDrag(event); }
void ScenePad::mouseDrag(const juce::MouseEvent& event) {
    x->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, (event.position.x-12)/static_cast<float>(getWidth()-24)));
    y->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, (event.position.y-12)/static_cast<float>(getHeight()-24))); repaint();
}
void ScenePad::mouseUp(const juce::MouseEvent&) { x->endChangeGesture(); y->endChangeGesture(); }

AetherloomEditor::AetherloomEditor(AetherloomProcessor& p)
    : AudioProcessorEditor(p), processor(p), keyboard(p.keyboard, juce::MidiKeyboardComponent::horizontalKeyboard), pad(p.parameters) {
    addAndMakeVisible(play); addAndMakeVisible(edit); addAndMakeVisible(mode); addAndMakeVisible(patches);
    addAndMakeVisible(keyboard); addAndMakeVisible(pad);
    play.onClick = [this] { setEditing(false); }; edit.onClick = [this] { setEditing(true); };
    mode.addItemList({"WARM", "BLEND", "GLITCH"}, 1);
    modeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.parameters, "mode", mode);
    for (int i = 0; i < p.getNumPrograms(); ++i) patches.addItem(p.getProgramName(i), i+1);
    patches.setSelectedId(p.getCurrentProgram()+1, juce::dontSendNotification);
    patches.setTooltip("Load an original factory patch. All controls remain editable; projects save the edited sound.");
    patches.onChange = [this] { processor.setCurrentProgram(patches.getSelectedId()-1); };
    const std::array<const char*, 15> ids{"colour","gain","cutoff","attack","decay","sustain","release","mix","sub","fm","motion","rate","morphX","morphY","morph"};
    const std::array<const char*, 15> names{"Colour","Level","Brightness (Hz)","Fade in (s)","Fall (s)","Hold","Fade out (s)","Body / Harmonic","Sub weight","Glass (FM)","Motion depth","Motion speed (Hz)","Glow / Glass","Still / Fracture","Scene amount"};
    const std::array<const char*, 15> tips{
        "Harmonic colour within WARM, BLEND or GLITCH.", "Smoothed output level.", "Low-pass filter brightness.",
        "Time to reach full volume.", "Time to fall to the held level.", "Level while the key is held.", "Time to fade after key release.",
        "Blend the character oscillator with its second harmonic.", "Blend in a sine oscillator one octave below.",
        "Second oscillator bends the character oscillator's phase for a glassy FM tone.", "How far the LFO moves brightness.",
        "Free-running-per-note motion speed; measured in Hz, not host-synced beats.",
        "Scene X: Glow to Glass (top), Haze to Fracture (bottom).", "Scene Y: top pair to bottom pair.",
        "0: use EDIT timbre controls. 1: use the four scene timbres. Envelope and brightness stay editable."};
    for (size_t i = 0; i < sliders.size(); ++i) {
        sliders[i].setSliderStyle(juce::Slider::LinearHorizontal);
        sliders[i].setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 24);
        sliders[i].setName(names[i]); sliders[i].setTooltip(tips[i]);
        labels[i].setText(names[i], juce::dontSendNotification);
        labels[i].attachToComponent(&sliders[i], false);
        addAndMakeVisible(sliders[i]); addAndMakeVisible(labels[i]);
        attachments.push_back(std::make_unique<SliderAttachment>(p.parameters, ids[i], sliders[i]));
    }
    setSize(820, 700); setEditing(false); startTimerHz(10);
}
void AetherloomEditor::timerCallback() { patches.setSelectedId(processor.getCurrentProgram()+1, juce::dontSendNotification); }
void AetherloomEditor::setEditing(bool value) {
    editing = value; play.setToggleState(!value, juce::dontSendNotification); edit.setToggleState(value, juce::dontSendNotification);
    for (size_t i = 0; i < sliders.size(); ++i) {
        const auto visible = value || i < 2 || i == 14;
        sliders[i].setVisible(visible); labels[i].setVisible(visible);
    }
    pad.setVisible(!value); resized(); repaint();
}
void AetherloomEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff191b27)); g.setColour(juce::Colour(0xffe7cdb4));
    g.setFont(26.0f); g.drawText("AETHERLOOM", 24, 12, 300, 40, juce::Justification::centredLeft);
    g.setFont(14.0f); g.drawText(editing ? "EDIT / three oscillators, envelope, FM and motion" : "PLAY / load a patch, raise Scene amount, explore the pad", 24, 58, 760, 24, juce::Justification::centredLeft);
    if (!editing) g.drawText("Blend timbres. Keep your envelope and brightness.", 24, 460, 760, 40, juce::Justification::centred);
}
void AetherloomEditor::resized() {
    play.setBounds(490, 20, 80, 30); edit.setBounds(580, 20, 80, 30); mode.setBounds(670, 20, 125, 30);
    patches.setBounds(24, 94, 310, 30);
    for (size_t i = 0; i < sliders.size(); ++i) {
        const auto column = static_cast<int>(i / 8); const auto row = static_cast<int>(i % 8);
        if (editing) sliders[i].setBounds(24 + column*402, 160 + row*54, 370, 30);
        else if (i < 2 || i == 14) sliders[i].setBounds(24, 175 + (i == 14 ? 2 : static_cast<int>(i))*90, 310, 30);
    }
    pad.setBounds(375, 150, 420, 280);
    keyboard.setBounds(24, getHeight()-86, getWidth()-48, 66);
}
