// SPDX-License-Identifier: AGPL-3.0-only
#include "../Source/PluginProcessor.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <vector>
int main() {
    juce::ScopedJuceInitialiser_GUI init;
    AetherloomProcessor processor;
    const auto run=[&](int program,double rate,bool extreme) {
        processor.releaseResources();processor.setCurrentProgram(program);
        if (extreme) {
            auto set=[&](const char* id,float value) { auto* p=processor.parameters.getParameter(id);p->setValueNotifyingHost(p->convertTo0to1(value)); };
            set("unison",2);set("osc1wave",4);set("osc2wave",4);set("osc3wave",4);
            set("chorus",0.3f);set("delayMix",0.3f);set("reverbMix",0.3f);
            for (int route=1;route<=8;++route) {
                const auto prefix="mod"+juce::String(route);
                set((prefix+"source").toRawUTF8(),static_cast<float>(route));
                set((prefix+"target").toRawUTF8(),static_cast<float>(route));
                set((prefix+"amount").toRawUTF8(),0.2f);
            }
        }
        constexpr int block=128;processor.prepareToPlay(rate,block);
        juce::AudioBuffer<float> buffer(2,block);juce::MidiBuffer midi;
        for (int note=48;note<64;++note) midi.addEvent(juce::MidiMessage::noteOn(1,note,0.8f),0);
        for (int i=0;i<100;++i) processor.processBlock(buffer,midi);
        std::vector<double> times(1200);
        for (auto& elapsed:times) {
            const auto begin=std::chrono::steady_clock::now();processor.processBlock(buffer,midi);
            elapsed=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-begin).count();
        }
        std::sort(times.begin(),times.end());const auto deadline=1e6*block/rate;
        std::cout<<processor.getProgramName(program)<<(extreme ? " / 3-unison + 8 routes + FX":" / default")
            <<" / 16 held voices / "<<rate<<" Hz / 128 samples\n"
            <<"median_us="<<times[times.size()/2]<<" p99_us="<<times[times.size()*99/100]
            <<" max_us="<<times.back()<<" block_budget_us="<<deadline<<'\n';
    };
    run(4,48000,false);run(9,48000,false);run(9,48000,true);run(9,96000,true);
}
