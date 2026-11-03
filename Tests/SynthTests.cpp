// SPDX-License-Identifier: AGPL-3.0-only
#include "../Source/PluginProcessor.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include "EditorSnapshots.h"

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
        for (const auto& spec:aetherloom::specs) old.removeChild(old.getChildWithProperty("id",spec.id),nullptr);
        legacy.reset();
        if (auto xml=old.createXml()) juce::AudioProcessor::copyXmlToBinary(*xml,legacy);
        restored.setCurrentProgram(17);
        restored.setStateInformation(legacy.getData(), static_cast<int>(legacy.getSize()));
        require(restored.parameters.getRawParameterValue("fm")->load() == 0, "Legacy sessions must default new controls");
        require(restored.parameters.getRawParameterValue("engine")->load()==0,"Legacy session must use Classic engine");
        require(restored.parameters.getRawParameterValue("arpOn")->load()==0,"Legacy session must disable arp");
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
        // Hybrid audio paths: independent tuning, all waves, routes, filters and effects.
        buffer.setSize(2,512); p.setCurrentProgram(6); set("reverbMix",0); set("chorus",0);
        set("hybridFm",0); set("filterEnvDepth",0); set("osc1wave",2);
        const auto base=renderChord(false); set("osc1semi",7);
        require(std::abs(renderChord(false)-base)>0.000001f,"Independent oscillator tuning must change audio");
        set("osc1semi",0); set("mod1source",5); set("mod1target",5); set("mod1amount",-0.7f);
        require(std::abs(renderChord(false)-base)>0.000001f,"Velocity-to-cutoff route must change audio");
        set("mod1source",0);
        const auto series=renderChord(false); set("filterRouting",1); set("filter2type",2); set("cutoff2",1800);
        require(std::abs(renderChord(false)-series)>0.000001f,"Dual filter routing/type must change audio");
        for (int wave=0;wave<5;++wave) for (double rate:{44100.0,48000.0,96000.0}) {
            p.setCurrentProgram(6); set("osc1wave",static_cast<float>(wave)); set("osc2wave",static_cast<float>(wave));
            set("unison",2); set("hybridFm",0.9f); set("ring",0.9f); set("resonance1",0.95f); set("resonance2",0.95f);
            set("cutoff",18000); set("cutoff2",18000); set("mod1source",2); set("mod1target",1); set("mod1amount",0.8f);
            p.releaseResources();p.prepareToPlay(rate,512);
            for (int note=70;note<90;++note) midi.addEvent(juce::MidiMessage::noteOn(1,note,1.0f),0);
            for (int n=0;n<12;++n) {
                p.processBlock(buffer,midi);
                for (int ch=0;ch<2;++ch) for (int i=0;i<512;++i)
                    require(std::isfinite(buffer.getSample(ch,i)) && std::abs(buffer.getSample(ch,i))<=1.001f,"Hybrid stress output must be finite/limited");
            }
        }
        p.setCurrentProgram(6);set("attack",0.001f);set("release",0.001f);set("chorus",0);set("reverbMix",0);
        set("delayMix",0.6f);set("delayTime",0.02f);set("delayFeedback",0.5f);
        p.releaseResources();p.prepareToPlay(48000,512);
        midi.addEvent(juce::MidiMessage::noteOn(1,60,0.8f),0); midi.addEvent(juce::MidiMessage::noteOff(1,60),128);
        p.processBlock(buffer,midi);
        double tail=0;
        for (int n=0;n<12;++n) { p.processBlock(buffer,midi);tail+=buffer.getMagnitude(0,512); }
        require(tail>0.0001,"Delay must produce an audible tail after voice release");
        p.requestPanic();p.processBlock(buffer,midi);
        require(buffer.getMagnitude(0,512)<0.000001f,"Panic must clear voices and effect tails");
        // New controls persist with the sound; malformed and oversized files are rejected.
        set("mod8source",8);set("mod8target",10);set("mod8amount",-0.4f);set("osc3semi",7);
        juce::MemoryBlock full;p.getStateInformation(full);restored.setStateInformation(full.getData(),static_cast<int>(full.getSize()));
        for (const auto& spec:aetherloom::specs)
            require(std::abs(restored.parameters.getRawParameterValue(spec.id)->load()-p.parameters.getRawParameterValue(spec.id)->load())<0.0001f,"Every Hybrid parameter must recall");
        require(restored.isPatchEdited(),"Edited marker must recall");
        const auto file=juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile("Aetherloom-test",".aetherpreset");
        require(p.savePreset(file) && restored.loadPreset(file),"User preset must save/load"); file.replaceWithText("invalid");
        require(!restored.loadPreset(file),"Malformed user preset must be rejected");file.deleteFile();
        const auto expressionAudio=[&](int source,int value,bool reset) {
            p.releaseResources();p.setCurrentProgram(6);set("chorus",0);set("delayMix",0);set("reverbMix",0);
            set("filterEnvDepth",0);set("mod1source",static_cast<float>(source));set("mod1target",5);set("mod1amount",-0.7f);
            p.prepareToPlay(48000,512);buffer.setSize(2,512);midi.clear();
            midi.addEvent(source==7 ? juce::MidiMessage::controllerEvent(1,1,value):juce::MidiMessage::channelPressureChange(1,value),0);
            if (reset) midi.addEvent(juce::MidiMessage::controllerEvent(1,121,0),0);
            midi.addEvent(juce::MidiMessage::noteOn(1,60,0.8f),0);p.processBlock(buffer,midi);
            return buffer.getSample(0,400);
        };
        const auto noWheel=expressionAudio(7,0,false);
        require(std::abs(expressionAudio(7,127,false)-noWheel)>0.000001f,"New notes must inherit previously sent mod wheel");
        require(std::abs(expressionAudio(8,127,false)-noWheel)>0.000001f,"New notes must inherit channel pressure");
        require(std::abs(expressionAudio(7,127,true)-noWheel)<0.000001f,"Reset controllers must clear inherited expression");
        // Arpeggiated audio must be independent of the host's block partition.
        auto arpAudio=[&](int blockSize,bool modern) {
            p.releaseResources();p.setCurrentProgram(0);set("engine",modern ? 1.0f:0.0f);set("arpOn",1);set("step3",7);
            p.prepareToPlay(48000,blockSize);
            constexpr int length=25000;std::vector<float> result(static_cast<size_t>(length));
            juce::AudioBuffer<float> chunk(2,blockSize);juce::MidiBuffer events;
            for (int start=0;start<length;start+=blockSize) {
                const auto count=std::min(blockSize,length-start);chunk.setSize(2,count,false,false,true);events.clear();
                for (int note:{60,64,67}) {
                    if (100>=start && 100<start+count) events.addEvent(juce::MidiMessage::noteOn(1,note,0.8f),100-start);
                    if (22000>=start && 22000<start+count) events.addEvent(juce::MidiMessage::noteOff(1,note),22000-start);
                }
                p.processBlock(chunk,events);
                for (int i=0;i<count;++i) result[static_cast<size_t>(start+i)]=chunk.getSample(0,i);
            }
            return result;
        };
        for (bool modern:{false,true}) {
            const auto shortBlocks=arpAudio(64,modern),longBlocks=arpAudio(8192,modern);
            for (size_t i=0;i<shortBlocks.size();++i)
                require(std::abs(shortBlocks[i]-longBlocks[i])<0.00001f,"Arpeggiated audio must be block invariant");
        }
        // Chorus handles mono and larger-than-advertised buffers through prepared chunks.
        p.releaseResources();p.setCurrentProgram(6);set("chorus",0.4f);p.prepareToPlay(48000,64);
        buffer.setSize(1,1024);midi.addEvent(juce::MidiMessage::noteOn(1,60,0.8f),0);p.processBlock(buffer,midi);
        for (int i=0;i<1024;++i) require(std::isfinite(buffer.getSample(0,i)),"Mono/oversized chorus buffer must be safe");
        p.setCurrentProgram(9);
        renderEditorSnapshots(p);
        std::cout << "Passed: silence, timed MIDI chords, modes, finite output, release, state\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
