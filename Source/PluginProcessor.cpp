// SPDX-License-Identifier: AGPL-3.0-only
#include "PluginProcessor.h"
#include "PluginEditor.h"
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
    return result;
}
AetherloomProcessor::AetherloomProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "AetherloomState", layout()) {
    gainParameter = parameters.getRawParameterValue("gain");
    initialState = parameters.copyState();
    for (int i = 0; i < 16; ++i) synth.addVoice(new Voice(parameters, sine));
    synth.addSound(new Sound()); synth.setNoteStealingEnabled(true);
}
int AetherloomProcessor::getNumPrograms() { return static_cast<int>(patches.size()); }
const juce::String AetherloomProcessor::getProgramName(int index) {
    return juce::isPositiveAndBelow(index, getNumPrograms()) ? patches[static_cast<size_t>(index)].name : juce::String{};
}
void AetherloomProcessor::setCurrentProgram(int index) {
    if (!juce::isPositiveAndBelow(index, getNumPrograms())) return;
    for (size_t i = 0; i < ids.size(); ++i) {
        auto* parameter = parameters.getParameter(ids[i]);
        parameter->setValueNotifyingHost(parameter->convertTo0to1(patches[static_cast<size_t>(index)].values[i]));
    }
    currentProgram.store(index);
}
void AetherloomProcessor::prepareToPlay(double rate, int) {
    synth.allNotesOff(0, false); synth.setCurrentPlaybackSampleRate(rate); keyboard.reset();
    outputGain.reset(rate, 0.02); outputGain.setCurrentAndTargetValue(gainParameter->load(std::memory_order_relaxed));
}
void AetherloomProcessor::releaseResources() { synth.allNotesOff(0, false); keyboard.reset(); }
bool AetherloomProcessor::isBusesLayoutSupported(const BusesLayout& buses) const {
    return buses.getMainInputChannelSet().isDisabled() && (buses.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        || buses.getMainOutputChannelSet() == juce::AudioChannelSet::mono());
}
void AetherloomProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;
    buffer.clear(); keyboard.processNextMidiBuffer(midi, 0, buffer.getNumSamples(), true);
    synth.renderNextBlock(buffer, midi, 0, buffer.getNumSamples());
    outputGain.setTargetValue(gainParameter->load(std::memory_order_relaxed));
    outputGain.applyGain(buffer, buffer.getNumSamples()); midi.clear();
}
void AetherloomProcessor::getStateInformation(juce::MemoryBlock& dest) {
    auto state = parameters.copyState(); state.setProperty("program", currentProgram.load(), nullptr);
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
    }
}
juce::AudioProcessorEditor* AetherloomProcessor::createEditor() { return new AetherloomEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AetherloomProcessor(); }
