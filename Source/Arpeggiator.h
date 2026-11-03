// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "SynthParameters.h"
#include <juce_audio_basics/juce_audio_basics.h>

namespace aetherloom {
// Fixed-capacity note ownership and sample-offset MIDI generation; no audio thread containers grow here.
class Arpeggiator {
public:
    struct Settings { int mode=0,division=1,octaves=1; float gate=0.65f,swing=0; std::array<int,8> steps{}; };
    struct Marker { int offset; float value; };
    void reset() noexcept { count=0; remaining=gateRemaining=0; active=-1; patternStep=0; sustain.fill(false); markerCount=0; }
    void process(const juce::MidiBuffer& input,juce::MidiBuffer& output,int samples,double rate,double bpm,const Settings& settings) {
        output.clear(); markerCount=0;
        auto event=input.cbegin();
        const auto end=input.cend();
        for (int position=0;position<samples;++position) {
            while (event!=end && (*event).samplePosition<=position) { consume((*event).getMessage(),output,position); ++event; }
            if (active>=0 && gateRemaining<=0) release(output,position);
            if (count>0 && remaining<=0) {
                release(output,position);
                std::array<int,128> sorted{};
                for (int i=0;i<count;++i) sorted[static_cast<size_t>(i)]=i;
                if (settings.mode!=3) std::sort(sorted.begin(),sorted.begin()+count,[this](int a,int b) { return held[static_cast<size_t>(a)].pitch<held[static_cast<size_t>(b)].pitch; });
                const auto total=count*settings.octaves;
                auto ordinal=static_cast<int>(patternStep%static_cast<unsigned>(total));
                if (settings.mode==1) ordinal=total-1-ordinal;
                if (settings.mode==2 && total>1) {
                    ordinal=static_cast<int>(patternStep%static_cast<unsigned>(2*(total-1)));
                    if (ordinal>=total) ordinal=2*(total-1)-ordinal;
                }
                const auto& note=held[static_cast<size_t>(sorted[static_cast<size_t>(ordinal%count)])];
                const auto transpose=settings.steps[patternStep%8];
                active=juce::jlimit(0,127,note.pitch+12*(ordinal/count)+transpose); activeChannel=note.channel;
                output.addEvent(juce::MidiMessage::noteOn(activeChannel,active,note.velocity),position);
                if (markerCount<markers.size()) markers[markerCount++]={position,static_cast<float>(transpose)/12};
                const auto duration=rate*60/std::clamp(bpm,20.0,400.0)*beats[static_cast<size_t>(settings.division)]
                    *(patternStep%2 ? 1.0-static_cast<double>(settings.swing):1.0+static_cast<double>(settings.swing));
                remaining+=duration; gateRemaining=duration*settings.gate; ++patternStep;
            }
            if (count==0) { release(output,position); remaining=1; patternStep=0; }
            remaining-=1; gateRemaining-=1;
        }
    }
    const std::array<Marker,512>& getMarkers() const noexcept { return markers; }
    size_t getMarkerCount() const noexcept { return markerCount; }
private:
    struct Note { int pitch=0,channel=1; float velocity=0; bool down=false; };
    void release(juce::MidiBuffer& output,int position) {
        if (active>=0) { output.addEvent(juce::MidiMessage::noteOff(activeChannel,active),position); active=-1; }
    }
    void consume(const juce::MidiMessage& message,juce::MidiBuffer& output,int position) {
        const auto channel=message.getChannel();
        if (message.isNoteOn()) {
            for (int i=0;i<count;++i) if (held[static_cast<size_t>(i)].pitch==message.getNoteNumber() && held[static_cast<size_t>(i)].channel==channel) {
                held[static_cast<size_t>(i)].down=true; held[static_cast<size_t>(i)].velocity=message.getFloatVelocity(); return;
            }
            if (count<static_cast<int>(held.size())) held[static_cast<size_t>(count++)]={message.getNoteNumber(),channel,message.getFloatVelocity(),true};
        } else if (message.isNoteOff()) {
            for (int i=count-1;i>=0;--i) if (held[static_cast<size_t>(i)].pitch==message.getNoteNumber() && held[static_cast<size_t>(i)].channel==channel) {
                held[static_cast<size_t>(i)].down=false;
                if (!sustain[static_cast<size_t>(channel-1)]) erase(i);
            }
        } else if (message.isController() && message.getControllerNumber()==64) {
            sustain[static_cast<size_t>(channel-1)]=message.getControllerValue()>=64;
            if (message.getControllerValue()<64) for (int i=count-1;i>=0;--i)
                if (held[static_cast<size_t>(i)].channel==channel && !held[static_cast<size_t>(i)].down) erase(i);
        } else if (message.isAllNotesOff() || message.isAllSoundOff() || (message.isController() && message.getControllerNumber()==121)) {
            // Panic/reset must not leave latched notes on any channel.
            release(output,position);
            const auto previousMarkers=markerCount; reset(); markerCount=previousMarkers;
            output.addEvent(message,position);
        } else output.addEvent(message,position);
    }
    void erase(int index) noexcept { for (int i=index;i<count-1;++i) held[static_cast<size_t>(i)]=held[static_cast<size_t>(i+1)]; --count; }
    std::array<Note,128> held{};
    std::array<bool,16> sustain{};
    std::array<Marker,512> markers{};
    size_t markerCount=0;
    int count=0,active=-1,activeChannel=1;
    unsigned patternStep=0;
    double remaining=0,gateRemaining=0;
};
}
