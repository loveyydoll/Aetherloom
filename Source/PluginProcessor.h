// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "CharacterDSP.h"
#include "HybridVoice.h"
#include "Arpeggiator.h"
#include "StereoEffects.h"

class AetherloomProcessor final : public juce::AudioProcessor, private juce::AudioProcessorValueTreeState::Listener
{
public:
    AetherloomProcessor();
    ~AetherloomProcessor() override;
    void requestPanic() { panicRequested.store(true); }
    float getOutputPeak() const { return outputPeak.load(); }
    bool isPatchEdited() const { return patchEdited.load(); }
    void randomiseTimbre();
    bool savePreset(const juce::File&);
    bool loadPreset(const juce::File&);
    using AudioProcessor::processBlock;
    void prepareToPlay(double, int) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Aetherloom"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram.load(); }
    void setCurrentProgram(int) override;
    const juce::String getProgramName(int) override;
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    juce::AudioProcessorValueTreeState parameters;
    juce::MidiKeyboardState keyboard;
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    aetherloom::SineTable sine;
    std::unique_ptr<aetherloom::WavetableBank> bank;
    juce::Synthesiser synth;
    aetherloom::ExpressionSynth hybridSynth;
    aetherloom::StereoEffects effects;
    aetherloom::Arpeggiator arpeggiator;
    juce::MidiBuffer arpMidi,segmentMidi;
    std::array<std::atomic<float>*,aetherloom::ParameterCount> advanced{};
    std::atomic<float> tempo{120},stepValue{0};
    std::atomic<bool> patchEdited{false},panicRequested{false};
    std::atomic<float> outputPeak{0};
    bool lastHybrid=false,lastArp=false,wasPlaying=false,hadPosition=false;
    int lastBlockSize=0;
    juce::int64 lastSamplePosition=0;
    void parameterChanged(const juce::String&,float) override { patchEdited.store(true); }
    juce::SmoothedValue<float> outputGain;
    std::atomic<float>* gainParameter = nullptr;
    std::atomic<int> currentProgram{0};
    juce::ValueTree initialState;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AetherloomProcessor)
};
