// SPDX-License-Identifier: AGPL-3.0-only
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "FactoryPatches.h"
#include <cmath>

namespace {
// Append new IDs; never rename existing automation IDs or change AU identity.
constexpr std::array<const char*, 16> ids{"colour", "gain", "cutoff", "attack", "decay", "sustain", "release",
    "mix", "sub", "fm", "motion", "rate", "morphX", "morphY", "morph", "mode"};
constexpr std::array<size_t, 11> smoothedIndices{0,2,7,8,9,10,11,12,13,14,15};
struct Patch { const char* name; std::array<float, 16> values; };
constexpr std::array<Patch, 6> patches{{
    {"Init", {0.35f,0.7f,3500,0.015f,0.3f,0.7f,0.5f, 0,0,0,0,1,0,0,0,0}},
    {"Velvet Steps", {0.3f,0.7f,2200,0.008f,0.45f,0.35f,0.25f, 0.2f,0.3f,0.04f,0.18f,1.5f,0,0,0,0}},
    {"Glass Rooms", {0.5f,0.6f,6500,0.012f,0.8f,0.45f,1.2f, 0.65f,0.05f,0.45f,0.2f,0.4f,1,0,0.6f,1}},
    {"Slow Orbit", {0.4f,0.65f,1800,0.8f,1.2f,0.8f,2.5f, 0.35f,0.3f,0.1f,0.65f,0.15f,0,1,0.7f,0}},
    {"Paper Circuit", {0.7f,0.5f,4200,0.003f,0.18f,0.2f,0.12f, 0.5f,0.1f,0.3f,0.6f,4,1,1,0.5f,2}},
    {"Low Lantern", {0.2f,0.7f,900,0.01f,0.3f,0.6f,0.2f, 0.1f,0.65f,0,0.12f,0.7f,0,0,0.3f,0}}
}};
struct Sound final : juce::SynthesiserSound {
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};
class Voice final : public juce::SynthesiserVoice {
public:
    Voice(juce::AudioProcessorValueTreeState& state, const aetherloom::SineTable& table) : sine(table) {
        for (size_t i = 0; i < ids.size(); ++i) params[i] = state.getRawParameterValue(ids[i]);
    }
    bool canPlaySound(juce::SynthesiserSound* s) override { return dynamic_cast<Sound*>(s) != nullptr; }
    void startNote(int note, float velocity, juce::SynthesiserSound*, int wheel) override {
        frequency = juce::MidiMessage::getMidiNoteInHertz(note);
        level = velocity * 0.08f;
        phase = subPhase = lfoPhase = 0.0;
        filter = coefficient = 0.0f; controlCounter = 0;
        pitchWheelMoved(wheel);
        envelope.setSampleRate(getSampleRate());
        envelope.reset(); updateEnvelope(); envelope.noteOn();
        for (const auto i : smoothedIndices) {
            smooth[i].reset(getSampleRate(), 0.02);
            smooth[i].setCurrentAndTargetValue(value(i));
        }
    }
    void stopNote(float, bool tail) override {
        if (tail) envelope.noteOff(); else { envelope.reset(); clearCurrentNote(); }
    }
    void pitchWheelMoved(int valueIn) override { bend = std::pow(2.0, (valueIn - 8192) / 8192.0 * 2.0 / 12.0); }
    void controllerMoved(int, int) override {}
    void renderNextBlock(juce::AudioBuffer<float>& buffer, int start, int count) override {
        updateEnvelope();
        for (const auto i : smoothedIndices) smooth[i].setTargetValue(value(i));
        const auto rate = getSampleRate();
        const auto increment = std::min(0.45, frequency * bend / rate);
        for (int i = 0; i < count; ++i) {
            std::array<float, 16> v{};
            for (const auto j : smoothedIndices) v[j] = smooth[j].getNextValue();
            const auto tone = aetherloom::blend({v[0],v[7],v[8],v[9],v[10]}, aetherloom::morph(v[12],v[13]), v[14]);
            const auto lfo = sine(lfoPhase);
            // Cap FM depth at high notes; this is not an oversampled/alias-free engine.
            const auto depth = tone.fm * std::min(1.0, 0.08 / increment);
            const auto carrier = sine(phase + depth * 0.3 * sine(phase * 2));
            const auto harmonic = increment < 0.225 ? sine(phase * 2) : 0.0f;
            const auto warm = aetherloom::saturate((carrier + 0.25f*tone.colour*harmonic)*1.4f);
            const auto glitch = std::round((carrier + tone.colour*harmonic)*8.0f)/8.0f;
            const auto mode = v[15];
            const auto blended = warm*(1-tone.colour)+glitch*tone.colour;
            const auto wave = mode <= 1 ? warm + mode*(blended-warm) : blended + (mode-1)*(glitch-blended);
            const auto body = wave*(1-tone.mix) + harmonic*tone.mix;
            const auto mixed = body*(1-tone.sub) + sine(subPhase)*tone.sub;
            // Exp only at a 32-sample control rate; smooth coefficients at audio rate.
            if (controlCounter++ % 32 == 0) {
                const auto hz = juce::jlimit(20.0, rate*0.45, static_cast<double>(v[2]) * std::pow(2.0, lfo*tone.motion*2.0));
                coefficientTarget = static_cast<float>(1.0 - std::exp(-juce::MathConstants<double>::twoPi*hz/rate));
            }
            coefficient += 0.02f*(coefficientTarget-coefficient);
            filter += coefficient*(mixed-filter);
            const auto sample = filter*envelope.getNextSample()*level;
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch) buffer.addSample(ch, start+i, sample);
            phase += increment; phase -= std::floor(phase);
            subPhase += increment*0.5; subPhase -= std::floor(subPhase);
            lfoPhase += v[11]/rate; lfoPhase -= std::floor(lfoPhase);
            if (!envelope.isActive()) { clearCurrentNote(); break; }
        }
    }
private:
    float value(size_t i) const { return params[i]->load(std::memory_order_relaxed); }
    void updateEnvelope() { envelope.setParameters({value(3),value(4),value(5),value(6)}); }
    const aetherloom::SineTable& sine;
    std::array<std::atomic<float>*, 16> params{};
    std::array<juce::SmoothedValue<float>, 16> smooth;
    juce::ADSR envelope;
    double frequency = 440, phase = 0, subPhase = 0, lfoPhase = 0, bend = 1;
    float level = 0, filter = 0, coefficient = 0, coefficientTarget = 0;
    unsigned controlCounter = 0;
};
}

juce::AudioProcessorValueTreeState::ParameterLayout AetherloomProcessor::layout() {
    juce::AudioProcessorValueTreeState::ParameterLayout result;
    int versionHint = 1;
    auto add = [&result, &versionHint](const char* id, const char* name, float low, float high, float initial, float skew = 1.0f) {
        result.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id, versionHint}, name,
            juce::NormalisableRange<float>(low, high, 0.0f, skew), initial));
    };
    result.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"mode", 1}, "Mode", juce::StringArray{"WARM", "BLEND", "GLITCH"}, 0));
    add("colour", "Colour", 0, 1, 0.35f); add("gain", "Level", 0, 1, 0.7f);
    add("cutoff", "Cutoff (Hz)", 20, 18000, 3500, 0.25f);
    add("attack", "Attack (s)", 0.001f, 3, 0.015f, 0.35f); add("decay", "Decay (s)", 0.001f, 3, 0.3f, 0.35f);
    add("sustain", "Sustain", 0, 1, 0.7f); add("release", "Release (s)", 0.001f, 5, 0.5f, 0.35f);
    versionHint = 2; // New AU parameters sort after the 0.1 automation addresses.
    add("mix", "Body / Harmonic", 0, 1, 0); add("sub", "Sub", 0, 1, 0); add("fm", "FM / Glass", 0, 1, 0);
    add("motion", "Motion Depth", 0, 1, 0); add("rate", "Motion Rate (Hz)", 0.05f, 12, 1, 0.4f);
    add("morphX", "Glow / Glass", 0, 1, 0); add("morphY", "Still / Fracture", 0, 1, 0); add("morph", "Scene Amount", 0, 1, 0);
    for (const auto& spec:aetherloom::specs) {
        if (spec.choices) {
            result.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{spec.id,3},spec.name,
                juce::StringArray::fromTokens(spec.choices,"|",""),static_cast<int>(spec.initial)));
        } else {
            result.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{spec.id,3},spec.name,
                juce::NormalisableRange<float>(spec.low,spec.high,0.0f,spec.skew),spec.initial));
        }
    }
    return result;
}
AetherloomProcessor::AetherloomProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "AetherloomState", layout()),
      bank(std::make_unique<aetherloom::WavetableBank>(sine)),effects(parameters) {
    (void)aetherloom::hybridPatches(); // Construct the factory bank before any host/audio callbacks.
    gainParameter = parameters.getRawParameterValue("gain");
    initialState = parameters.copyState();
    for (int i = 0; i < 16; ++i) synth.addVoice(new Voice(parameters, sine));
    synth.addSound(new Sound()); synth.setNoteStealingEnabled(true);
    for (int i=0;i<16;++i) hybridSynth.addVoice(new aetherloom::HybridVoice(parameters,sine,*bank,tempo,stepValue,hybridSynth.expression,i));
    hybridSynth.addSound(new Sound()); hybridSynth.setNoteStealingEnabled(true);
    synth.setMinimumRenderingSubdivisionSize(1,true); hybridSynth.setMinimumRenderingSubdivisionSize(1,true);
    for (size_t i=0;i<aetherloom::specs.size();++i) advanced[i]=parameters.getRawParameterValue(aetherloom::specs[i].id);
    for (const auto* id:ids) parameters.addParameterListener(id,this);
    for (const auto& spec:aetherloom::specs) parameters.addParameterListener(spec.id,this);
}
AetherloomProcessor::~AetherloomProcessor() {
    for (const auto* id:ids) parameters.removeParameterListener(id,this);
    for (const auto& spec:aetherloom::specs) parameters.removeParameterListener(spec.id,this);
}
int AetherloomProcessor::getNumPrograms() { return static_cast<int>(patches.size()+aetherloom::hybridPatches().size()); }
const juce::String AetherloomProcessor::getProgramName(int index) {
    if (!juce::isPositiveAndBelow(index,getNumPrograms())) return {};
    return index<static_cast<int>(patches.size()) ? patches[static_cast<size_t>(index)].name
        : aetherloom::hybridPatches()[static_cast<size_t>(index)-patches.size()].name;
}
void AetherloomProcessor::setCurrentProgram(int index) {
    if (!juce::isPositiveAndBelow(index,getNumPrograms())) return;
    const auto modern=index>=static_cast<int>(patches.size());
    const auto& classic=modern ? aetherloom::hybridPatches()[static_cast<size_t>(index)-patches.size()].classic : patches[static_cast<size_t>(index)].values;
    for (size_t i=0;i<ids.size();++i) {
        auto* p=parameters.getParameter(ids[i]); p->setValueNotifyingHost(p->convertTo0to1(classic[i]));
    }
    for (size_t i=0;i<aetherloom::specs.size();++i) {
        auto* p=parameters.getParameter(aetherloom::specs[i].id);
        const auto value=modern ? aetherloom::hybridPatches()[static_cast<size_t>(index)-patches.size()].values[i] : aetherloom::specs[i].initial;
        p->setValueNotifyingHost(p->convertTo0to1(value));
    }
    currentProgram.store(index); patchEdited.store(false);
}
void AetherloomProcessor::prepareToPlay(double rate, int block) {
    synth.allNotesOff(0,false); hybridSynth.allNotesOff(0,false);
    synth.setCurrentPlaybackSampleRate(rate); hybridSynth.setCurrentPlaybackSampleRate(rate); hybridSynth.expression={}; keyboard.reset();
    arpeggiator.reset(); arpMidi.ensureSize(65536); segmentMidi.ensureSize(65536); effects.prepare(rate,block);
    lastHybrid=advanced[aetherloom::Engine]->load()>0.5f; lastArp=advanced[aetherloom::ArpOn]->load()>0.5f;
    wasPlaying=hadPosition=false; stepValue.store(0); tempo.store(120); outputPeak.store(0);
    outputGain.reset(rate,0.02); outputGain.setCurrentAndTargetValue(gainParameter->load());
}
void AetherloomProcessor::releaseResources() {
    synth.allNotesOff(0,false); hybridSynth.allNotesOff(0,false); keyboard.reset(); arpeggiator.reset(); effects.reset();
}
bool AetherloomProcessor::isBusesLayoutSupported(const BusesLayout& buses) const {
    return buses.getMainInputChannelSet().isDisabled() && (buses.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        || buses.getMainOutputChannelSet() == juce::AudioChannelSet::mono());
}
void AetherloomProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    if (panicRequested.exchange(false)) { synth.allNotesOff(0,false); hybridSynth.allNotesOff(0,false); arpeggiator.reset(); effects.reset(); keyboard.reset(); }
    if (buffer.getNumSamples()==0) { midi.clear(); return; }
    keyboard.processNextMidiBuffer(midi,0,buffer.getNumSamples(),true);
    bool playing=false,positionAvailable=false;
    juce::int64 samplePosition=0;
    if (auto* playhead=getPlayHead()) if (auto position=playhead->getPosition()) {
        playing=position->getIsPlaying();
        if (auto bpm=position->getBpm(); bpm && std::isfinite(*bpm)) tempo.store(static_cast<float>(juce::jlimit(20.0,400.0,*bpm)));
        if (auto at=position->getTimeInSamples()) { samplePosition=*at; positionAvailable=true; }
    }
    const auto hybrid=advanced[aetherloom::Engine]->load()>0.5f, arp=advanced[aetherloom::ArpOn]->load()>0.5f;
    const auto seek=playing && wasPlaying && hadPosition && positionAvailable && samplePosition!=lastSamplePosition+lastBlockSize;
    if (hybrid!=lastHybrid || arp!=lastArp || (wasPlaying && !playing) || seek) {
        synth.allNotesOff(0,false); hybridSynth.allNotesOff(0,false); arpeggiator.reset(); stepValue.store(0);
    }
    lastHybrid=hybrid; lastArp=arp; wasPlaying=playing; hadPosition=positionAvailable;
    lastSamplePosition=samplePosition; lastBlockSize=buffer.getNumSamples();
    auto& engine=hybrid ? hybridSynth:synth;
    if (arp) {
        aetherloom::Arpeggiator::Settings settings;
        settings.mode=static_cast<int>(advanced[aetherloom::ArpMode]->load()); settings.division=static_cast<int>(advanced[aetherloom::ArpDivision]->load());
        settings.octaves=1+static_cast<int>(advanced[aetherloom::ArpOctaves]->load());
        settings.gate=advanced[aetherloom::ArpGate]->load(); settings.swing=advanced[aetherloom::ArpSwing]->load();
        for (size_t i=0;i<8;++i) settings.steps[i]=juce::roundToInt(advanced[aetherloom::Step1+i]->load());
        arpeggiator.process(midi,arpMidi,buffer.getNumSamples(),getSampleRate(),tempo.load(),settings);
        const auto renderRange=[&](int offset,int length) {
            // JUCE consumes events beyond the requested range; isolate each segment.
            segmentMidi.clear(); segmentMidi.addEvents(arpMidi,offset,length,0);
            engine.renderNextBlock(buffer,segmentMidi,offset,length);
        };
        int start=0;
        for (size_t i=0;i<arpeggiator.getMarkerCount();++i) {
            const auto marker=arpeggiator.getMarkers()[i];
            if (marker.offset>start) renderRange(start,marker.offset-start);
            stepValue.store(marker.value); start=marker.offset;
        }
        if (start<buffer.getNumSamples()) renderRange(start,buffer.getNumSamples()-start);
    } else engine.renderNextBlock(buffer,midi,0,buffer.getNumSamples());
    effects.process(buffer,tempo.load());
    outputGain.setTargetValue(gainParameter->load()); outputGain.applyGain(buffer,buffer.getNumSamples());
    // Soft safety ceiling for resonant/ring/feedback combinations, preserving normal dry levels.
    float peak=0;
    for (int ch=0;ch<buffer.getNumChannels();++ch) for (int i=0;i<buffer.getNumSamples();++i) {
        auto x=buffer.getSample(ch,i); const auto magnitude=std::abs(x);
        if (magnitude>0.95f) x=std::copysign(0.95f+0.05f*(magnitude-0.95f)/(magnitude-0.9f),x);
        x=std::isfinite(x) ? x:0.0f; peak=std::max(peak,std::abs(x)); buffer.setSample(ch,i,x);
    }
    outputPeak.store(peak); midi.clear();
}
void AetherloomProcessor::getStateInformation(juce::MemoryBlock& dest) {
    auto state = parameters.copyState(); state.setProperty("program", currentProgram.load(), nullptr);
    state.setProperty("schema",3,nullptr); state.setProperty("edited",patchEdited.load(),nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary(*xml, dest);
}
void AetherloomProcessor::setStateInformation(const void* data, int size) {
    if (auto xml = getXmlFromBinary(data, size); xml && xml->hasTagName(parameters.state.getType())) {
        auto state = juce::ValueTree::fromXml(*xml);
        // Old sessions get new controls at Init values, even after another patch was loaded.
        for (auto child : initialState)
            if (!state.getChildWithProperty("id", child["id"]).isValid()) state.addChild(child.createCopy(), -1, nullptr);
        currentProgram.store(juce::jlimit(0, getNumPrograms()-1, static_cast<int>(state.getProperty("program", 0))));
        parameters.replaceState(state);
        patchEdited.store(static_cast<bool>(state.getProperty("edited",false)));
    }
}
double AetherloomProcessor::getTailLengthSeconds() const {
    const auto value=[this](aetherloom::Key key) { return advanced[static_cast<size_t>(key)]->load(); };
    double tail=5;
    if (value(aetherloom::DelayMix)>0 || value(aetherloom::Space)>0) {
        const auto sync=static_cast<int>(value(aetherloom::DelaySync));
        const auto seconds=sync==0 ? value(aetherloom::DelayTime):60/tempo.load()*aetherloom::beats[static_cast<size_t>(sync-1)];
        const auto repeats=std::ceil(std::log(0.00001)/std::log(std::clamp(static_cast<double>(value(aetherloom::Feedback)),0.01,0.85)));
        tail+=std::min(4.0,static_cast<double>(seconds))*repeats;
    }
    if (value(aetherloom::Reverb)>0 || value(aetherloom::Space)>0) tail+=3+27*value(aetherloom::Size);
    return tail;
}
bool AetherloomProcessor::savePreset(const juce::File& file) {
    juce::MemoryBlock state; getStateInformation(state); return file.replaceWithData(state.getData(),state.getSize());
}
bool AetherloomProcessor::loadPreset(const juce::File& file) {
    if (file.getSize()<=0 || file.getSize()>4*1024*1024) return false;
    juce::MemoryBlock data; if (!file.loadFileAsData(data)) return false;
    auto xml=getXmlFromBinary(data.getData(),static_cast<int>(data.getSize()));
    if (!xml || !xml->hasTagName(parameters.state.getType())) return false;
    setStateInformation(data.getData(),static_cast<int>(data.getSize())); return true;
}
void AetherloomProcessor::randomiseTimbre() {
    juce::Random random;
    auto set=[this](const char* id,float value) {
        auto* p=parameters.getParameter(id); p->beginChangeGesture(); p->setValueNotifyingHost(p->convertTo0to1(value)); p->endChangeGesture();
    };
    set("engine",1);
    constexpr std::array<int,7> intervals{-12,-7,0,0,7,12,19};
    for (int osc=0;osc<3;++osc) {
        const auto key=static_cast<size_t>(aetherloom::Wave1+osc*6);
        set(aetherloom::specs[key].id,static_cast<float>(random.nextInt(5)));
        set(aetherloom::specs[key+1].id,static_cast<float>(osc==0 ? 0:intervals[static_cast<size_t>(random.nextInt(7))]));
        set(aetherloom::specs[key+2].id,random.nextFloat()*16-8);
        set(aetherloom::specs[key+3].id,osc==0 ? 0.65f:random.nextFloat()*0.35f);
        set(aetherloom::specs[key+4].id,random.nextFloat()); set(aetherloom::specs[key+5].id,0.15f+random.nextFloat()*0.7f);
    }
    set("hybridFm",random.nextFloat()*0.45f); set("ring",random.nextFloat()*0.3f);
    set("cutoff",700+random.nextFloat()*6000); set("resonance1",random.nextFloat()*0.6f);
    patchEdited.store(true);
}
juce::AudioProcessorEditor* AetherloomProcessor::createEditor() { return new AetherloomEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AetherloomProcessor(); }
