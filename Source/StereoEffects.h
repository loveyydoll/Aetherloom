// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "SynthParameters.h"
#include <juce_dsp/juce_dsp.h>

namespace aetherloom {
class StereoEffects {
public:
    explicit StereoEffects(juce::AudioProcessorValueTreeState& state) {
        for (size_t i=0;i<specs.size();++i) params[i]=state.getRawParameterValue(specs[i].id);
    }
    void prepare(double sampleRate,int maximumBlock) {
        rate=sampleRate; chunkSize=std::max(1,maximumBlock); monoScratch.setSize(2,chunkSize);
        const auto length=static_cast<size_t>(std::ceil(rate*4))+2;
        for (auto& channel:delay) channel.assign(length,0);
        chorus.prepare({rate,static_cast<juce::uint32>(std::max(1,maximumBlock)),2});
        chorus.setRate(0.3f); chorus.setDepth(0.35f); chorus.setCentreDelay(12); chorus.setFeedback(0.08f);
        reverb.setSampleRate(rate);
        for (auto& smooth:smoothing) { smooth.reset(rate,0.04); smooth.setCurrentAndTargetValue(0); }
        delaySamples.reset(rate,0.08); delaySamples.setCurrentAndTargetValue(rate*0.3);
        reset();
    }
    void reset() {
        for (auto& channel:delay) std::fill(channel.begin(),channel.end(),0);
        write=0; feedbackLow.fill(0); chorus.reset(); reverb.reset();
    }
    void process(juce::AudioBuffer<float>& buffer,float bpm) {
        const auto space=value(Space);
        const auto chorusMix=value(Chorus), delayMix=std::min(0.7f,value(DelayMix)+space*0.25f), reverbMix=std::min(0.7f,value(Reverb)+space*0.3f);
        if (chorusMix==0 && delayMix==0 && reverbMix==0 && smoothing[0].getCurrentValue()==0 && smoothing[2].getCurrentValue()==0) {
            if (!bypassed) reset();
            bypassed=true; return;
        }
        bypassed=false;
        // Chorus is bypassed exactly at zero; no dry-path coloration or allocation.
        chorus.setMix(chorusMix);
        if (chorusMix>0) {
            for (int start=0;start<buffer.getNumSamples();start+=chunkSize) {
                const auto count=std::min(chunkSize,buffer.getNumSamples()-start);
                if (buffer.getNumChannels()==1) {
                    monoScratch.copyFrom(0,0,buffer,0,start,count); monoScratch.copyFrom(1,0,buffer,0,start,count);
                    auto block=juce::dsp::AudioBlock<float>(monoScratch).getSubBlock(0,static_cast<size_t>(count));
                    chorus.process(juce::dsp::ProcessContextReplacing<float>(block));
                    for (int i=0;i<count;++i) buffer.setSample(0,start+i,(monoScratch.getSample(0,i)+monoScratch.getSample(1,i))*0.5f);
                } else {
                    auto block=juce::dsp::AudioBlock<float>(buffer).getSubBlock(static_cast<size_t>(start),static_cast<size_t>(count));
                    chorus.process(juce::dsp::ProcessContextReplacing<float>(block));
                }
            }
        }
        juce::Reverb::Parameters settings;
        settings.roomSize=value(Size); settings.damping=0.55f; settings.wetLevel=1; settings.dryLevel=0; settings.width=1; settings.freezeMode=0;
        reverb.setParameters(settings);
        smoothing[0].setTargetValue(delayMix); smoothing[1].setTargetValue(value(Feedback)); smoothing[2].setTargetValue(reverbMix);
        const auto sync=static_cast<int>(value(DelaySync));
        const auto seconds=sync==0 ? value(DelayTime) : 60/std::clamp(bpm,20.0f,400.0f)*beats[static_cast<size_t>(sync-1)];
        delaySamples.setTargetValue(std::clamp(static_cast<double>(seconds)*rate,1.0,static_cast<double>(delay[0].size()-2)));
        for (int i=0;i<buffer.getNumSamples();++i) {
            auto left=buffer.getSample(0,i), right=buffer.getNumChannels()>1 ? buffer.getSample(1,i):left;
            const auto dm=smoothing[0].getNextValue(), feedback=smoothing[1].getNextValue(), rm=smoothing[2].getNextValue();
            const auto offset=delaySamples.getNextValue();
            auto read=static_cast<double>(write)-offset;
            if (read<0) read+=static_cast<double>(delay[0].size());
            const auto a=static_cast<size_t>(read), b=(a+1)%delay[0].size(); const auto fraction=static_cast<float>(read-static_cast<double>(a));
            std::array<float,2> echo{};
            for (size_t ch=0;ch<2;++ch) {
                echo[ch]=delay[ch][a]+fraction*(delay[ch][b]-delay[ch][a]);
                feedbackLow[ch]+=0.25f*(echo[ch]-feedbackLow[ch]);
            }
            delay[0][write]=left+feedbackLow[1]*feedback; delay[1][write]=right+feedbackLow[0]*feedback;
            write=(write+1)%delay[0].size(); left+=echo[0]*dm; right+=echo[1]*dm;
            auto wetLeft=left,wetRight=right;
            reverb.processStereo(&wetLeft,&wetRight,1);
            left=left*(1-rm)+wetLeft*rm; right=right*(1-rm)+wetRight*rm;
            if (buffer.getNumChannels()==1) buffer.setSample(0,i,(left+right)*0.5f);
            else { buffer.setSample(0,i,left); buffer.setSample(1,i,right); }
        }
    }
private:
    float value(Key key) const { return params[static_cast<size_t>(key)]->load(std::memory_order_relaxed); }
    std::array<std::atomic<float>*,ParameterCount> params{};
    std::array<std::vector<float>,2> delay;
    std::array<juce::SmoothedValue<float>,3> smoothing;
    juce::SmoothedValue<double> delaySamples;
    std::array<float,2> feedbackLow{};
    juce::dsp::Chorus<float> chorus;
    juce::Reverb reverb;
    juce::AudioBuffer<float> monoScratch;
    int chunkSize=512;
    size_t write=0;
    double rate=48000;
    bool bypassed=true;
};
}
