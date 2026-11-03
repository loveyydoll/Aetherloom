// SPDX-License-Identifier: AGPL-3.0-only
#include "PluginEditor.h"
#include "FactoryPatches.h"

ScenePad::ScenePad(juce::AudioProcessorValueTreeState& state)
    : x(state.getParameter("morphX")), y(state.getParameter("morphY")) {
    setTitle("Scene blend"); setDescription("Glow, Glass, Haze and Fracture; use EDIT for keyboard-accessible X/Y sliders.");
    startTimerHz(20);
}
void ScenePad::timerCallback() {
    if (!isShowing()) return;
    const auto newX = x->getValue(), newY = y->getValue();
    if (newX != lastX || newY != lastY) { lastX = newX; lastY = newY; repaint(); }
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

SynthControl::SynthControl(juce::AudioProcessorValueTreeState& state,const aetherloom::ParameterSpec& spec)
    : page(spec.page),isChoice(spec.choices!=nullptr),isRoute(spec.page==aetherloom::Page::Matrix) {
    hybridOnly=state.getParameter(spec.id)->getVersionHint()>=3 && spec.page!=aetherloom::Page::Effects
        && spec.page!=aetherloom::Page::Arp && juce::String(spec.id)!="space";
    label.setText(spec.name,juce::dontSendNotification); label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::FontOptions(13)); addAndMakeVisible(label);
    if (isChoice) {
        choice.addItemList(juce::StringArray::fromTokens(spec.choices,"|",""),1); choice.setTooltip(spec.name);
        addAndMakeVisible(choice); choiceAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(state,spec.id,choice);
    } else {
        slider.setName(spec.name); slider.setTooltip(juce::String(spec.name)+" — host-automatable; double-click to restore default.");
        slider.setSliderStyle(isRoute ? juce::Slider::LinearHorizontal:juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(isRoute ? juce::Slider::TextBoxRight:juce::Slider::TextBoxBelow,false,76,20);
        slider.setDoubleClickReturnValue(true,spec.initial); addAndMakeVisible(slider);
        sliderAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(state,spec.id,slider);
    }
}
void SynthControl::resized() {
    label.setBounds(0,0,getWidth(),20);
    if (isChoice) choice.setBounds(8,isRoute ? 24:48,getWidth()-16,28);
    else slider.setBounds(4,22,getWidth()-8,getHeight()-24);
}
AetherloomEditor::AetherloomEditor(AetherloomProcessor& p)
    : AudioProcessorEditor(p),processor(p),keyboard(p.keyboard,juce::MidiKeyboardComponent::horizontalKeyboard),pad(p.parameters) {
    for (auto* button:{&play,&edit,&reload,&randomise,&save,&load,&panic}) addAndMakeVisible(button);
    for (auto* combo:{&engine,&mode,&patches,&category}) addAndMakeVisible(combo);
    addAndMakeVisible(search); addAndMakeVisible(status); addAndMakeVisible(keyboard); addAndMakeVisible(pad); addAndMakeVisible(viewport);
    engine.addItemList({"CLASSIC","HYBRID"},1); mode.addItemList({"WARM","BLEND","GLITCH"},1);
    engineAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.parameters,"engine",engine);
    modeAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.parameters,"mode",mode);
    play.onClick=[this] { setEditing(false); }; edit.onClick=[this] { setEditing(true); };
    reload.onClick=[this] { processor.setCurrentProgram(processor.getCurrentProgram()); };
    randomise.onClick=[this] { processor.randomiseTimbre(); }; panic.onClick=[this] { processor.requestPanic(); };
    save.onClick=[this] { chooseFile(true); }; load.onClick=[this] { chooseFile(false); };
    reload.setTooltip("Reload the selected factory patch, discarding current edits.");
    randomise.setTooltip("Mutate oscillators and filter within musical ranges. Keeps envelopes, clock and effects.");
    panic.setTooltip("Release every voice, clear arpeggiator ownership and reset effect tails on the next audio block.");
    category.addItemList({"All sounds","Classic","Keys","Pads","Bass","Leads","Textures","Sequences"},1);
    category.setSelectedId(1,juce::dontSendNotification); category.onChange=[this] { filterPatches(); };
    search.setTextToShowWhenEmpty("Find a sound…",juce::Colours::grey); search.onTextChange=[this] { filterPatches(); };
    patches.onChange=[this] { if (patches.getSelectedId()>0) processor.setCurrentProgram(patches.getSelectedId()-1); };
    filterPatches();
    viewport.setViewedComponent(&panel,false); viewport.setScrollBarsShown(true,false);
    const std::array<const char*,6> titles{"OSCILLATORS","FILTERS / AMP","MOTION","MOD MATRIX","EFFECTS","ARP / STEPS"};
    for (size_t i=0;i<pages.size();++i) {
        pages[i].setButtonText(titles[i]); addAndMakeVisible(pages[i]);
        pages[i].onClick=[this,i] { currentPage=static_cast<aetherloom::Page>(i+1); resized(); repaint(); };
    }
    for (const auto& spec:aetherloom::specs) if (juce::String(spec.id)!="engine") controls.push_back(std::make_unique<SynthControl>(p.parameters,spec));
    const std::array<aetherloom::ParameterSpec,16> legacy{{
        {"gain","Level",0,1,0.7f,1,aetherloom::Page::Play}, {"cutoff","Brightness (Hz)",20,18000,3500,0.25f,aetherloom::Page::Play},
        {"colour","Colour",0,1,0.35f,1,aetherloom::Page::Play}, {"morph","Scene amount",0,1,0,1,aetherloom::Page::Play},
        {"cutoff","Filter 1 cutoff (Hz)",20,18000,3500,0.25f,aetherloom::Page::Filters},
        {"attack","Amp attack (s)",0.001f,3,0.015f,0.35f,aetherloom::Page::Filters}, {"decay","Amp decay (s)",0.001f,3,0.3f,0.35f,aetherloom::Page::Filters},
        {"sustain","Amp sustain",0,1,0.7f,1,aetherloom::Page::Filters}, {"release","Amp release (s)",0.001f,5,0.5f,0.35f,aetherloom::Page::Filters},
        {"mix","Classic harmonic mix",0,1,0,1,aetherloom::Page::Oscillators}, {"sub","Sub weight",0,1,0,1,aetherloom::Page::Oscillators},
        {"fm","Character FM",0,1,0,1,aetherloom::Page::Oscillators}, {"motion","Character motion",0,1,0,1,aetherloom::Page::Motion},
        {"rate","Classic motion Hz",0.05f,12,1,0.4f,aetherloom::Page::Motion},
        {"morphX","Scene X",0,1,0,1,aetherloom::Page::Motion}, {"morphY","Scene Y",0,1,0,1,aetherloom::Page::Motion}
    }};
    for (const auto& spec:legacy) controls.push_back(std::make_unique<SynthControl>(p.parameters,spec));
    for (auto& control:controls) panel.addAndMakeVisible(*control);
    setSize(1020,780); setEditing(false); startTimerHz(15);
}
void AetherloomEditor::filterPatches() {
    const auto selected=processor.getCurrentProgram()+1;
    patches.clear(juce::dontSendNotification);
    const auto wanted=category.getText(),query=search.getText();
    for (int i=0;i<processor.getNumPrograms();++i) {
        const auto group=i<6 ? juce::String("Classic"):juce::String(aetherloom::hybridPatches()[static_cast<size_t>(i-6)].category);
        const auto name=processor.getProgramName(i);
        if ((category.getSelectedId()==1 || wanted==group) && name.containsIgnoreCase(query)) patches.addItem(name,i+1);
    }
    patches.setSelectedId(selected,juce::dontSendNotification);
}
void AetherloomEditor::chooseFile(bool writing) {
    chooser=std::make_unique<juce::FileChooser>(writing ? "Save Aetherloom sound":"Load Aetherloom sound",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("My sound.aetherpreset"),"*.aetherpreset");
    const auto flags=juce::FileBrowserComponent::canSelectFiles | (writing ? juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::warnAboutOverwriting:juce::FileBrowserComponent::openMode);
    chooser->launchAsync(flags,[safe=juce::Component::SafePointer<AetherloomEditor>(this),writing](const juce::FileChooser& selected) {
        if (!safe) return;
        auto file=selected.getResult(); if (file==juce::File{}) return;
        if (writing) file=file.withFileExtension(".aetherpreset");
        const auto ok=writing ? safe->processor.savePreset(file):safe->processor.loadPreset(file);
        safe->status.setText(ok ? (writing ? "Sound saved":"Sound loaded"):"Could not read or write that sound",juce::dontSendNotification);
    });
}
void AetherloomEditor::timerCallback() {
    const auto program=processor.getCurrentProgram();
    const auto dirty=processor.isPatchEdited();
    const auto hybrid=processor.parameters.getRawParameterValue("engine")->load()>0.5f;
    mode.setEnabled(!hybrid);
    for (auto& control:controls) control->setEnabled(!control->hybridOnly || hybrid);
    if (program!=lastProgram || dirty!=lastEdited) {
        lastProgram=program; lastEdited=dirty;
        patches.setSelectedId(program+1,juce::dontSendNotification);
        status.setText(processor.getProgramName(program)+(dirty ? "  • edited":"  • factory"),juce::dontSendNotification);
    }
    const auto peak=std::max(processor.getOutputPeak(),displayedPeak*0.82f);
    if (std::abs(peak-displayedPeak)>0.001f) { displayedPeak=peak; repaint(820,66,176,20); }
}
void AetherloomEditor::setEditing(bool value) {
    editing=value; play.setToggleState(!value,juce::dontSendNotification); edit.setToggleState(value,juce::dontSendNotification);
    pad.setVisible(!value); resized(); repaint();
}
void AetherloomEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff131622)); g.setColour(juce::Colour(0xffe7cdb4));
    g.setFont(juce::FontOptions(25)); g.drawText("AETHERLOOM",24,14,245,36,juce::Justification::left);
    g.setFont(juce::FontOptions(12)); g.setColour(juce::Colour(0xffaaa8c5));
    g.drawText(editing ? "HYBRID tools need the Hybrid engine. Classic keeps the earlier character sound.":"Start with a patch. Raise Scene amount to blend the corners. Macros shape Hybrid + effects.",24,115,970,24,juce::Justification::left);
    g.setColour(juce::Colour(0xff30354d)); g.fillRoundedRectangle(820,72,176,8,4);
    g.setColour(juce::Colour(0xffb1a2e9)); g.fillRoundedRectangle(820,72,std::min(176.0f,displayedPeak*176),8,4);
    if (!editing) {
        g.setColour(juce::Colour(0xffaaa8c5)); g.setFont(juce::FontOptions(14));
        g.drawText("GLOW → GLASS / HAZE → FRACTURE",500,530,490,26,juce::Justification::centred);
        g.drawText("Three oscillators · dual filters · eight modulation routes",500,560,490,24,juce::Justification::centred);
    }
}
void AetherloomEditor::resized() {
    patches.setBounds(270,18,280,30); engine.setBounds(560,18,135,30);
    play.setBounds(710,18,84,30); edit.setBounds(800,18,84,30); panic.setBounds(900,18,96,30);
    category.setBounds(24,66,125,28); search.setBounds(159,66,170,28); mode.setBounds(339,66,112,28);
    reload.setBounds(463,66,80,28); randomise.setBounds(551,66,80,28); save.setBounds(639,66,72,28); load.setBounds(719,66,72,28);
    for (size_t i=0;i<pages.size();++i) {
        pages[i].setVisible(editing); pages[i].setBounds(24+static_cast<int>(i)*163,150,155,30);
        pages[i].setToggleState(currentPage==static_cast<aetherloom::Page>(i+1),juce::dontSendNotification);
    }
    viewport.setBounds(24,editing ? 192:150,editing ? 972:440,editing ? 458:500);
    const auto columns=!editing ? 2:currentPage==aetherloom::Page::Matrix ? 3:6;
    const auto cellWidth=(viewport.getWidth()-18)/columns;
    const auto cellHeight=currentPage==aetherloom::Page::Matrix && editing ? 60:112;
    int index=0;
    for (auto& control:controls) {
        const auto visible=control->page==(editing ? currentPage:aetherloom::Page::Play);
        control->setVisible(visible);
        if (visible) { control->setBounds((index%columns)*cellWidth,(index/columns)*cellHeight,cellWidth-6,cellHeight-4); ++index; }
    }
    panel.setSize(viewport.getWidth()-18,std::max(viewport.getHeight(),((index+columns-1)/columns)*cellHeight));
    pad.setBounds(500,160,496,350); status.setBounds(24,662,972,24);
    keyboard.setBounds(24,700,972,60);
}
