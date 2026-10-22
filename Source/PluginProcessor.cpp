// SPDX-License-Identifier: AGPL-3.0-only
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace {
struct Sound final : juce::SynthesiserSound
{
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

class Voice final : public juce::SynthesiserVoice
{
public:
    explicit Voice(juce::AudioProcessorValueTreeState& state) : params(state) {}
    bool canPlaySound(juce::SynthesiserSound* s) override { return dynamic_cast<Sound*>(s) != nullptr; }
    void startNote(int note, float velocity, juce::SynthesiserSound*, int wheel) override
    {
        frequency = juce::MidiMessage::getMidiNoteInHertz(note);
        level = velocity * 0.08f;
        phase = 0.0;
        filter = 0.0f;
        pitchWheelMoved(wheel);
        envelope.setSampleRate(getSampleRate());
        updateEnvelope();
        envelope.noteOn();
    }
    void stopNote(float, bool tail) override
    {
        if (tail) envelope.noteOff();
        else { envelope.reset(); clearCurrentNote(); }
    }
    void pitchWheelMoved(int value) override { bend = std::pow(2.0, (value - 8192) / 8192.0 * 2.0 / 12.0); }
    void controllerMoved(int, int) override {}
    void renderNextBlock(juce::AudioBuffer<float>& buffer, int start, int count) override
    {
        updateEnvelope();
        const auto mode = static_cast<int>(value("mode"));
        const auto colour = value("colour");
        const auto cutoff = juce::jlimit(20.0f, static_cast<float>(getSampleRate() * 0.45), value("cutoff"));
        const auto coefficient = 1.0f - std::exp(-juce::MathConstants<float>::twoPi * cutoff / static_cast<float>(getSampleRate()));
        const auto increment = frequency * bend / getSampleRate();
        for (int i = 0; i < count; ++i)
        {
            const auto sine = static_cast<float>(std::sin(phase * juce::MathConstants<double>::twoPi));
            const auto harmonic = static_cast<float>(std::sin(phase * juce::MathConstants<double>::twoPi * 2.0));
            const auto warm = std::tanh((sine + 0.25f * colour * harmonic) * 1.4f) / 1.4f;
            const auto glitch = std::round((sine + harmonic * colour) * 8.0f) / 8.0f;
            const auto wave = mode == 0 ? warm : (mode == 2 ? glitch : warm * (1.0f - colour) + glitch * colour);
            filter += coefficient * (wave - filter);
            const auto sample = filter * envelope.getNextSample() * level;
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch) buffer.addSample(ch, start + i, sample);
            phase += increment;
            phase -= std::floor(phase);
            if (!envelope.isActive()) { clearCurrentNote(); break; }
        }
    }
private:
    float value(const char* id) const { return params.getRawParameterValue(id)->load(); }
    void updateEnvelope() { envelope.setParameters({value("attack"), value("decay"), value("sustain"), value("release")}); }
    juce::AudioProcessorValueTreeState& params;
    juce::ADSR envelope;
    double frequency = 440.0, phase = 0.0, bend = 1.0;
    float level = 0.0f, filter = 0.0f;
};
}

juce::AudioProcessorValueTreeState::ParameterLayout AetherloomProcessor::layout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout result;
    auto add = [&result](const char* id, const char* name, float low, float high, float initial, float skew = 1.0f)
    {
        result.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id, 1}, name,
            juce::NormalisableRange<float>(low, high, 0.0f, skew), initial));
    };
    result.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"mode", 1}, "Mode", juce::StringArray{"WARM", "BLEND", "GLITCH"}, 0));
    add("colour", "Colour", 0, 1, 0.35f);
    add("gain", "Level", 0, 1, 0.7f);
    add("cutoff", "Cutoff (Hz)", 20, 18000, 3500, 0.25f);
    add("attack", "Attack (s)", 0.001f, 3, 0.015f, 0.35f);
    add("decay", "Decay (s)", 0.001f, 3, 0.3f, 0.35f);
    add("sustain", "Sustain", 0, 1, 0.7f);
    add("release", "Release (s)", 0.001f, 5, 0.5f, 0.35f);
    return result;
}
AetherloomProcessor::AetherloomProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "AetherloomState", layout())
{
    for (int i = 0; i < 16; ++i) synth.addVoice(new Voice(parameters));
    synth.addSound(new Sound());
    synth.setNoteStealingEnabled(true);
}
void AetherloomProcessor::prepareToPlay(double rate, int)
{
    synth.allNotesOff(0, false);
    synth.setCurrentPlaybackSampleRate(rate);
    keyboard.reset();
}
void AetherloomProcessor::releaseResources() { synth.allNotesOff(0, false); keyboard.reset(); }
bool AetherloomProcessor::isBusesLayoutSupported(const BusesLayout& buses) const
{
    return buses.getMainInputChannelSet().isDisabled()
        && (buses.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
            || buses.getMainOutputChannelSet() == juce::AudioChannelSet::mono());
}
void AetherloomProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    keyboard.processNextMidiBuffer(midi, 0, buffer.getNumSamples(), true);
    synth.renderNextBlock(buffer, midi, 0, buffer.getNumSamples());
    buffer.applyGain(parameters.getRawParameterValue("gain")->load());
    midi.clear();
}
void AetherloomProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    if (auto xml = parameters.copyState().createXml()) copyXmlToBinary(*xml, dest);
}
void AetherloomProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size); xml && xml->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
}
juce::AudioProcessorEditor* AetherloomProcessor::createEditor() { return new AetherloomEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AetherloomProcessor(); }
