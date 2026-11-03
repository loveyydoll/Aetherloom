// SPDX-License-Identifier: AGPL-3.0-only
#include "../Source/Arpeggiator.h"
#include <iostream>
#include <stdexcept>
#include <vector>

static void require(bool ok,const char* message) { if (!ok) throw std::runtime_error(message); }
int main() {
    try {
        aetherloom::Arpeggiator arp;
        aetherloom::Arpeggiator::Settings settings;
        juce::MidiBuffer input,output; output.ensureSize(65536);
        input.addEvent(juce::MidiMessage::noteOn(1,67,0.8f),100);
        input.addEvent(juce::MidiMessage::noteOn(1,60,0.8f),100);
        input.addEvent(juce::MidiMessage::noteOn(1,64,0.8f),100);
        std::vector<int> times,notes,off;
        for (int block=0;block<4;++block) {
            arp.process(input,output,5000,48000,120,settings); input.clear();
            for (const auto event:output) {
                const auto m=event.getMessage();
                if (m.isNoteOn()) { times.push_back(block*5000+event.samplePosition);notes.push_back(m.getNoteNumber()); }
                if (m.isNoteOff()) off.push_back(block*5000+event.samplePosition);
            }
        }
        require(times==std::vector<int>{100,6100,12100,18100},"Arp timing must preserve MIDI onset and carry phase across blocks");
        require(notes==std::vector<int>{60,64,67,60},"Up pattern must sort input pitches");
        require(off.size()>=3 && off[0]==4000,"Gate duration must be sample accurate");
        arp.reset(); times.clear(); settings.swing=0.2f;
        input.addEvent(juce::MidiMessage::noteOn(1,60,0.8f),0);
        arp.process(input,output,24001,48000,120,settings); input.clear();
        for (const auto event:output) if (event.getMessage().isNoteOn()) times.push_back(event.samplePosition);
        require(times.size()>=4 && times[0]==0 && std::abs(times[1]-7200)<=1 && std::abs(times[2]-12000)<=1,"Swing must alternate long/short steps without drifting pairs");
        arp.reset(); settings.swing=0; settings.mode=2; settings.octaves=2; notes.clear();
        input.addEvent(juce::MidiMessage::noteOn(2,60,0.8f),0);
        input.addEvent(juce::MidiMessage::noteOn(2,64,0.8f),0);
        arp.process(input,output,42001,48000,120,settings); input.clear();
        for (const auto event:output) if (event.getMessage().isNoteOn()) notes.push_back(event.getMessage().getNoteNumber());
        require(notes==std::vector<int>{60,64,72,76,72,64,60,64},"Up/down must avoid duplicated endpoints across octaves");
        arp.reset(); settings.mode=0; settings.octaves=1;
        input.addEvent(juce::MidiMessage::controllerEvent(2,64,127),0);
        input.addEvent(juce::MidiMessage::noteOn(2,60,0.8f),0);
        input.addEvent(juce::MidiMessage::noteOff(2,60),100);
        input.addEvent(juce::MidiMessage::controllerEvent(2,64,0),7000);
        arp.process(input,output,12001,48000,120,settings); input.clear();
        times.clear();
        for (const auto event:output) if (event.getMessage().isNoteOn()) times.push_back(event.samplePosition);
        require(times==std::vector<int>{0,6000},"Pedal must latch held keys and release ownership when lifted");
        arp.reset(); input.addEvent(juce::MidiMessage::noteOn(2,60,0.8f),0);
        input.addEvent(juce::MidiMessage::allNotesOff(1),100);
        arp.process(input,output,200,48000,120,settings);
        bool released=false;
        for (const auto event:output) if (event.getMessage().isNoteOff() && event.getMessage().getChannel()==2 && event.samplePosition==100) released=true;
        require(released,"Panic must release the generated voice even if the panic channel differs");
        std::cout<<"Passed: offsets, cross-block timing, sorting, swing, octave direction, sustain ownership and panic\n";
        return 0;
    } catch (const std::exception& error) { std::cerr<<error.what()<<'\n';return 1; }
}
