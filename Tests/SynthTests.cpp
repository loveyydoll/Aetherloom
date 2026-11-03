// SPDX-License-Identifier: AGPL-3.0-only
#include "../Source/PluginProcessor.h"
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    try {
        AetherloomProcessor p;
        p.prepareToPlay(48000, 512);
        juce::AudioBuffer<float> buffer(2, 512);
        juce::MidiBuffer midi;
        p.processBlock(buffer, midi);
        require(buffer.getMagnitude(0, 512) == 0, "Init must be silent");
        auto set = [&p](const char* id, float value) {
            auto* param = p.parameters.getParameter(id);
            param->setValueNotifyingHost(param->convertTo0to1(value));
        };
        // Deterministic voice phases let us compare one note with two distinct notes.
        auto renderChord = [&p, &buffer, &midi](bool second) {
            p.releaseResources(); p.prepareToPlay(48000, 512);
            midi.addEvent(juce::MidiMessage::noteOn(1, 60, 0.8f), 0);
            if (second) midi.addEvent(juce::MidiMessage::noteOn(1, 67, 0.8f), 0);
            p.processBlock(buffer, midi);
            return buffer.getSample(0, 400);
        };
        const auto single = renderChord(false);
        const auto chord = renderChord(true);
        require(std::abs(single - chord) > 0.0001f, "Second voice must contribute");
        set("cutoff", 20);
        const auto dark = renderChord(false);
        set("cutoff", 18000);
        const auto bright = renderChord(false);
        require(std::abs(dark - bright) > 0.0001f, "Cutoff must affect output");
        set("cutoff", 3500);
        for (int mode = 0; mode < 3; ++mode) {
            p.releaseResources(); p.prepareToPlay(48000, 512);
            auto* param = p.parameters.getParameter("mode");
            param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(mode)));
            midi.addEvent(juce::MidiMessage::noteOn(1, 60, 0.8f), 128);
            midi.addEvent(juce::MidiMessage::noteOn(1, 67, 0.8f), 128);
            p.processBlock(buffer, midi);
            require(buffer.getMagnitude(0, 128) == 0, "MIDI offset must be respected");
            require(buffer.getMagnitude(128, 384) > 0.001f, "Chord must produce audio in each mode");
            for (int ch = 0; ch < 2; ++ch) for (int i = 0; i < 512; ++i)
                require(std::isfinite(buffer.getSample(ch, i)), "Audio must be finite");
            midi.addEvent(juce::MidiMessage::allNotesOff(1), 0);
            for (int i = 0; i < 100; ++i) p.processBlock(buffer, midi);
            require(buffer.getMagnitude(0, 512) < 0.000001f, "Release must return to silence");
        }
        p.parameters.getParameter("colour")->setValueNotifyingHost(0.83f);
        juce::MemoryBlock state; p.getStateInformation(state);
        AetherloomProcessor restored; restored.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
        require(std::abs(restored.parameters.getRawParameterValue("colour")->load() - 0.83f) < 0.001f, "State must round-trip");
        const char invalid[] = "broken"; restored.setStateInformation(invalid, sizeof(invalid));
        require(std::abs(restored.parameters.getRawParameterValue("colour")->load() - 0.83f) < 0.001f, "Invalid state must be ignored");
        for (auto id : {"mode","colour","gain","cutoff","attack","decay","sustain","release"})
            require(p.parameters.getParameter(id)->getVersionHint() == 1, "Old AU hints must stay stable");
        for (auto id : {"mix","sub","fm","motion","rate","morphX","morphY","morph"})
            require(p.parameters.getParameter(id)->getVersionHint() == 2, "New AU parameters must append");
        // Every factory patch must be audible, distinct and recallable.
        float previous = 0;
        for (int patch = 0; patch < p.getNumPrograms(); ++patch) {
            p.setCurrentProgram(patch);
            const auto sample = renderChord(true);
            require(std::abs(sample-previous) > 0.000001f, "Factory patches must differ");
            require(buffer.getMagnitude(0,512) > 0.00001f, "Patch must be audible");
            previous = sample;
            juce::MemoryBlock saved; p.getStateInformation(saved);
            restored.setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
            require(restored.getCurrentProgram() == patch, "Patch label must recall");
            for (auto id : {"mix","sub","fm","motion","rate","morphX","morphY","morph"})
                require(std::abs(p.parameters.getRawParameterValue(id)->load()-restored.parameters.getRawParameterValue(id)->load()) < 0.0001f,
                        "New parameter must round-trip");
        }
        // Restoring an old session after a new patch must reset absent new controls.
        auto old = p.parameters.copyState();
        for (auto id : {"mix","sub","fm","motion","rate","morphX","morphY","morph"})
            old.removeChild(old.getChildWithProperty("id", id), nullptr);
        juce::MemoryBlock legacy;
        if (auto xml = old.createXml()) juce::AudioProcessor::copyXmlToBinary(*xml, legacy);
        restored.setCurrentProgram(4);
        restored.setStateInformation(legacy.getData(), static_cast<int>(legacy.getSize()));
        require(restored.parameters.getRawParameterValue("fm")->load() == 0, "Legacy sessions must default new controls");
        p.setCurrentProgram(0); set("mix",0.5f); set("sub",0.2f);
        const auto plain = renderChord(false); set("fm",0.8f);
        require(std::abs(renderChord(false)-plain) > 0.00001f, "FM must affect output");
        set("morph",1); set("morphX",0); set("morphY",0);
        const auto glow = renderChord(false); set("morphX",1); set("morphY",1);
        require(std::abs(renderChord(false)-glow) > 0.00001f, "Scene pad must affect output");
        set("morph",0); set("motion",0);
        const auto still = renderChord(false); set("motion",1); set("rate",12);
        require(std::abs(renderChord(false)-still) > 0.00001f, "Motion must affect output");
        // Bound output for high notes, maximum modulation, voice stealing and odd block sizes.
        for (double rate : {44100.0,48000.0,96000.0}) for (int block : {1,17,64,512,1024}) {
            p.releaseResources(); p.prepareToPlay(rate,block);
            buffer.setSize(2,block); p.setCurrentProgram(4);
            set("fm",1); set("motion",1); set("cutoff",18000); set("morph",1);
            for (int note = 90; note < 114; ++note) midi.addEvent(juce::MidiMessage::noteOn(1,note,1.0f),0);
            for (int n = 0; n < 30; ++n) {
                set("morphX", n%2 ? 1.0f : 0.0f); set("gain", n%2 ? 0.0f : 1.0f);
                p.processBlock(buffer,midi);
                for (int ch=0; ch<2; ++ch) for (int j=0; j<block; ++j)
                    require(std::isfinite(buffer.getSample(ch,j)) && std::abs(buffer.getSample(ch,j)) < 3.0f,
                            "Stress audio must stay finite and bounded");
            }
        }
        std::cout << "Passed: silence, timed MIDI chords, modes, finite output, release, state\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
