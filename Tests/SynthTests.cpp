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
        std::cout << "Passed: silence, timed MIDI chords, modes, finite output, release, state\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
