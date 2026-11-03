// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "SynthParameters.h"
#include "HybridDSP.h"
#include <juce_audio_utils/juce_audio_utils.h>

namespace aetherloom {
struct ExpressionState { std::array<float,16> wheel{},pressure{}; };
class ExpressionSynth final : public juce::Synthesiser {
public:
    ExpressionState expression;
    void handleController(int channel,int controller,int value) override {
        if (channel>=1 && channel<=16) {
            if (controller==1) expression.wheel[static_cast<size_t>(channel-1)]=static_cast<float>(value)/127;
            if (controller==121) {
                expression.wheel[static_cast<size_t>(channel-1)]=0;expression.pressure[static_cast<size_t>(channel-1)]=0;
                handleSustainPedal(channel,false);handleSostenutoPedal(channel,false);
            }
        }
        juce::Synthesiser::handleController(channel,controller,value);
    }
    void handleChannelPressure(int channel,int value) override {
        if (channel>=1 && channel<=16) expression.pressure[static_cast<size_t>(channel-1)]=static_cast<float>(value)/127;
        juce::Synthesiser::handleChannelPressure(channel,value);
    }
};
class HybridVoice final : public juce::SynthesiserVoice {
public:
    HybridVoice(juce::AudioProcessorValueTreeState& state,const SineTable& sineIn,const WavetableBank& bankIn,
                const std::atomic<float>& tempoIn,const std::atomic<float>& stepIn,const ExpressionState& expressionIn,int seed)
        : sine(sineIn),bank(bankIn),tempo(tempoIn),step(stepIn),expression(expressionIn),seedValue(static_cast<uint32_t>(seed+1)*7919u) {
        for (size_t i=0;i<specs.size();++i) raw[i]=state.getRawParameterValue(specs[i].id);
        constexpr std::array<const char*,12> oldIds{"attack","decay","sustain","release","cutoff","colour","mix","sub","fm","motion","morphX","morphY"};
        for (size_t i=0;i<oldIds.size();++i) old[i]=state.getRawParameterValue(oldIds[i]);
        sceneAmount=state.getRawParameterValue("morph");
    }
    bool canPlaySound(juce::SynthesiserSound* sound) override { return sound!=nullptr; }
    void startNote(int note,float velocityIn,juce::SynthesiserSound*,int wheel) override {
        noteNumber=note; velocity=velocityIn; frequency=juce::MidiMessage::getMidiNoteInHertz(note);
        rng=seedValue+static_cast<uint32_t>(note)*101u; noteRandom=randomBipolar(rng);
        for (auto& osc:phase) for (auto& p:osc) p=(randomBipolar(rng)+1)*0.5;
        subPhase=0; counter=0; modWheel=pressure=0; pan=0;
        for (int channel=1;channel<=16;++channel) if (isPlayingChannel(channel)) {
            modWheel=expression.wheel[static_cast<size_t>(channel-1)]; pressure=expression.pressure[static_cast<size_t>(channel-1)]; break;
        }
        for (size_t i=0;i<raw.size();++i) current[i]=target[i]=raw[i]->load(std::memory_order_relaxed);
        for (auto& filters:filter) for (auto& f:filters) f.reset();
        for (auto& l:lfo) l.reset();
        for (auto* e:{&amp,&filterEnv,&modEnv}) { e->setSampleRate(getSampleRate()); e->reset(); }
        updateEnvelopes(true); amp.noteOn(); filterEnv.noteOn(); modEnv.noteOn(); pitchWheelMoved(wheel);
    }
    void stopNote(float,bool tail) override {
        if (tail) { amp.noteOff(); filterEnv.noteOff(); modEnv.noteOff(); }
        else { amp.reset(); filterEnv.reset(); modEnv.reset(); clearCurrentNote(); }
    }
    void pitchWheelMoved(int wheel) override { bend=static_cast<float>(wheel-8192)/8192.0f*2; }
    void controllerMoved(int controller,int value) override {
        if (controller==1) modWheel=static_cast<float>(value)/127;
        if (controller==121) modWheel=pressure=0;
    }
    void aftertouchChanged(int value) override { pressure=static_cast<float>(value)/127; }
    void channelPressureChanged(int value) override { pressure=static_cast<float>(value)/127; }
    void renderNextBlock(juce::AudioBuffer<float>& buffer,int start,int count) override {
        for (size_t i=0;i<raw.size();++i) target[i]=raw[i]->load(std::memory_order_relaxed);
        updateEnvelopes();
        const auto sr=getSampleRate();
        for (int sampleIndex=0;sampleIndex<count;++sampleIndex) {
            const auto env=filterEnv.getNextSample(), mod=modEnv.getNextSample();
            const auto l1=lfo[0].next(lfoHz[0],sr,static_cast<int>(current[LfoShape1]),sine,rng);
            const auto l2=lfo[1].next(lfoHz[1],sr,static_cast<int>(current[LfoShape2]),sine,rng);
            if ((counter++ & 31u)==0) control(env,mod,l1,l2);
            float left=0,right=0;
            const auto voices=1+static_cast<int>(current[Unison]);
            for (int u=0;u<voices;++u) {
                std::array<float,3> wave{};
                // Render oscillator 2 first: its actual independent waveform drives FM.
                for (const int osc:{1,2,0}) {
                    const auto o=static_cast<size_t>(osc), un=static_cast<size_t>(u);
                    auto p=phase[o][un];
                    if (osc==0) p+=wave[1]*fmDepth*std::min(1.0,0.06/increments[o][un]);
                    const auto key=static_cast<size_t>(Wave1+osc*6);
                    wave[o]=bank.oscillator(static_cast<int>(current[key]),p,increments[o][un],scan[o],current[key+5],sine);
                    phase[o][un]+=increments[o][un]; phase[o][un]-=std::floor(phase[o][un]);
                }
                const auto main=wave[0]*(1-current[Ring])+wave[0]*wave[1]*current[Ring];
                const auto mixed=main*current[Level1]*(1-0.35f*sceneMix)+wave[1]*(current[Level2]+0.35f*sceneMix)+wave[2]*current[Level3];
                left+=mixed*unisonPan[static_cast<size_t>(u)][0]/static_cast<float>(voices);
                right+=mixed*unisonPan[static_cast<size_t>(u)][1]/static_cast<float>(voices);
            }
            const auto extra=sine(subPhase)*subAmount+randomBipolar(rng)*current[Noise];
            subPhase+=subIncrement; subPhase-=std::floor(subPhase);
            const auto amplitude=amp.getNextSample()*velocity*0.075f*levelMod;
            pan+=0.003f*(panTarget-pan);
            std::array<float,2> signal{left+extra,right+extra};
            for (size_t ch=0;ch<2;++ch) {
                auto x=signal[ch];
                x=(1-driveAmount)*x+driveAmount*2*aetherloom::saturate(x*2);
                const auto first=filter[ch][0].process(x,static_cast<int>(current[Filter1]));
                const auto second=filter[ch][1].process(current[Routing]<0.5f ? first:x,static_cast<int>(current[Filter2]));
                signal[ch]=(first*(1-current[FilterMix])+second*current[FilterMix])*amplitude*(ch==0 ? 1-pan:1+pan);
            }
            if (buffer.getNumChannels()==1) buffer.addSample(0,start+sampleIndex,(signal[0]+signal[1])*0.5f);
            else { buffer.addSample(0,start+sampleIndex,signal[0]); buffer.addSample(1,start+sampleIndex,signal[1]); }
            if (!amp.isActive()) { clearCurrentNote(); break; }
        }
    }
private:
    static void updateEnvelope(juce::ADSR& envelope,const juce::ADSR::Parameters& next,bool force) {
        const auto& previous=envelope.getParameters();
        if (force || next.attack!=previous.attack || next.decay!=previous.decay || next.sustain!=previous.sustain || next.release!=previous.release)
            envelope.setParameters(next);
    }
    void updateEnvelopes(bool force=false) {
        updateEnvelope(amp,{old[0]->load(),old[1]->load(),old[2]->load(),old[3]->load()},force);
        updateEnvelope(filterEnv,{target[EnvAttack],target[EnvDecay],target[EnvSustain],target[EnvRelease]},force);
        updateEnvelope(modEnv,{target[ModAttack],target[ModDecay],target[ModSustain],target[ModRelease]},force);
    }
    void control(float env,float mod,float l1,float l2) {
        for (size_t i=0;i<current.size();++i)
            current[i]=specs[i].choices ? target[i] : current[i]+0.15f*(target[i]-current[i]);
        std::array<float,14> destinations{};
        const std::array<float,11> source{0,l1,l2,env,mod,velocity,(static_cast<float>(noteNumber)-60)/60,modWheel,pressure,noteRandom,step.load(std::memory_order_relaxed)};
        for (int route=0;route<8;++route) {
            const auto index=static_cast<size_t>(Source1+route*3);
            const auto src=static_cast<int>(current[index]), dst=static_cast<int>(current[index+1]);
            if (src>0 && src<11 && dst>0 && dst<14) destinations[static_cast<size_t>(dst)]+=source[static_cast<size_t>(src)]*current[index+2];
        }
        for (auto& value:destinations) value=std::clamp(value,-2.0f,2.0f);
        const auto x=std::clamp(old[10]->load()+destinations[12],0.0f,1.0f), y=std::clamp(old[11]->load()+destinations[13],0.0f,1.0f);
        const auto tone=blend({old[5]->load(),old[6]->load(),old[7]->load(),old[8]->load(),old[9]->load()},morph(x,y),sceneAmount->load());
        fmDepth=std::clamp(current[FM]+tone.fm+destinations[9],0.0f,1.0f);
        subAmount=tone.sub; driveAmount=std::clamp(current[Drive]+current[Warmth]*0.5f+tone.colour*0.2f,0.0f,1.0f); sceneMix=tone.mix;
        const auto bpm=tempo.load(std::memory_order_relaxed);
        for (int i=0;i<2;++i) {
            const auto sync=static_cast<int>(current[static_cast<size_t>(LfoSync1+i*3)]);
            lfoHz[static_cast<size_t>(i)]=sync==0 ? current[static_cast<size_t>(LfoRate1+i*3)] : bpm/(60*beats[static_cast<size_t>(sync-1)]);
        }
        const auto motion=(tone.motion+current[Movement])*l1*2;
        const auto keyboard=(static_cast<float>(noteNumber)-60)/12*current[KeyTrack];
        const auto cutoff1=old[4]->load()*std::exp2(env*current[EnvDepth]+keyboard+destinations[5]*4+motion+current[Air]*2);
        const auto cutoff2=current[Cutoff2]*std::exp2(destinations[6]*4+keyboard);
        for (auto& f:filter) {
            f[0].configure(getSampleRate(),cutoff1,std::clamp(current[Resonance1]+destinations[7]*0.4f,0.0f,0.95f));
            f[1].configure(getSampleRate(),cutoff2,std::clamp(current[Resonance2]+destinations[7]*0.4f,0.0f,0.95f));
        }
        const auto voices=1+static_cast<int>(current[Unison]);
        for (int o=0;o<3;++o) {
            const auto key=static_cast<size_t>(Wave1+o*6);
            scan[static_cast<size_t>(o)]=std::clamp(current[key+4]+destinations[8],0.0f,1.0f);
            for (int u=0;u<voices;++u) {
                const auto detune=voices==1 ? 0.0f : (static_cast<float>(u)*2/static_cast<float>(voices-1)-1);
                const auto semitones=current[key+1]+current[key+2]*0.01f+detune*current[Detune]*0.01f+bend+destinations[1]*24+destinations[static_cast<size_t>(2+o)]*12;
                increments[static_cast<size_t>(o)][static_cast<size_t>(u)]=std::clamp(frequency*std::exp2(semitones/12)/getSampleRate(),0.000001,0.45);
                const auto position=detune*current[Spread];
                unisonPan[static_cast<size_t>(u)]={std::sqrt((1-position)*0.5f),std::sqrt((1+position)*0.5f)};
            }
        }
        subIncrement=std::min(0.45,frequency*0.5*std::exp2(bend/12)/getSampleRate());
        panTarget=std::clamp(destinations[10],-0.95f,0.95f); levelMod=std::clamp(1+destinations[11],0.0f,2.0f);
    }
    const SineTable& sine;
    const WavetableBank& bank;
    const std::atomic<float>& tempo;
    const std::atomic<float>& step;
    const ExpressionState& expression;
    std::array<std::atomic<float>*,ParameterCount> raw{};
    std::array<std::atomic<float>*,12> old{};
    std::atomic<float>* sceneAmount=nullptr;
    std::array<float,ParameterCount> current{},target{};
    std::array<std::array<double,3>,3> phase{},increments{};
    std::array<std::array<float,2>,3> unisonPan{};
    std::array<float,3> scan{};
    std::array<std::array<StateFilter,2>,2> filter;
    std::array<Lfo,2> lfo;
    std::array<float,2> lfoHz{0.5f,2};
    juce::ADSR amp,filterEnv,modEnv;
    uint32_t seedValue=1,rng=1;
    unsigned counter=0;
    int noteNumber=60;
    double frequency=440,subPhase=0,subIncrement=0;
    float velocity=0,bend=0,modWheel=0,pressure=0,noteRandom=0,subAmount=0,fmDepth=0,driveAmount=0,pan=0,panTarget=0,levelMod=1,sceneMix=0;
};
}
