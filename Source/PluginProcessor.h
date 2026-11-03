// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "CharacterDSP.h"

class AetherloomProcessor final : public juce::AudioProcessor
{
public:
    AetherloomProcessor();
    void prepareToPlay(double, int) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Aetherloom"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 5.0; }
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
    juce::Synthesiser synth;
    juce::SmoothedValue<float> outputGain;
    std::atomic<int> currentProgram{0};
    juce::ValueTree initialState;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AetherloomProcessor)
};
