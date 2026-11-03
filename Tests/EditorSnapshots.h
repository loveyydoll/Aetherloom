// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "../Source/PluginEditor.h"
#include <cstdlib>

inline void renderEditorSnapshots(AetherloomProcessor& processor) {
    const auto* directory=std::getenv("AETHERLOOM_EDITOR_SNAPSHOTS");
    if (directory==nullptr) return;
    const juce::File folder(directory); folder.createDirectory();
    auto editor=std::unique_ptr<juce::AudioProcessorEditor>(processor.createEditor());
    auto render=[&](const char* name) {
        juce::Image image(juce::Image::ARGB,editor->getWidth(),editor->getHeight(),true);
        juce::Graphics graphics(image); editor->paintEntireComponent(graphics,true);
        auto stream=folder.getChildFile(juce::String(name)+".png").createOutputStream();
        if (stream) { stream->setPosition(0); stream->truncate(); }
        juce::PNGImageFormat png;
        if (!stream || !png.writeImageToStream(image,*stream)) throw std::runtime_error("Editor snapshot failed");
    };
    auto click=[&](const char* name) {
        for (auto* child:editor->getChildren()) if (auto* button=dynamic_cast<juce::Button*>(child))
            if (button->getButtonText()==name && button->onClick) { button->onClick(); return; }
        throw std::runtime_error("Missing editor navigation button");
    };
    render("play"); click("EDIT");
    constexpr std::array<const char*,6> tabs{"OSCILLATORS","FILTERS / AMP","MOTION","MOD MATRIX","EFFECTS","ARP / STEPS"};
    constexpr std::array<const char*,6> files{"oscillators","filters","motion","matrix","effects","arpeggiator"};
    for (size_t i=0;i<tabs.size();++i) { click(tabs[i]);render(files[i]); }
}
