// SPDX-License-Identifier: AGPL-3.0-only
#include "../Source/PluginProcessor.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <vector>
int main() {
    juce::ScopedJuceInitialiser_GUI init;
    AetherloomProcessor processor;
    constexpr double rate = 48000;
    constexpr int block = 128;
    processor.setCurrentProgram(4);
    processor.prepareToPlay(rate, block);
    juce::AudioBuffer<float> buffer(2, block);
    juce::MidiBuffer midi;
    for (int note=48; note<64; ++note) midi.addEvent(juce::MidiMessage::noteOn(1,note,0.8f),0);
    // Warm instruction/data caches before timing; no allocation in timed scope.
    for (int i=0; i<100; ++i) processor.processBlock(buffer,midi);
    std::vector<double> times(4000);
    for (auto& elapsed : times) {
        const auto begin = std::chrono::steady_clock::now();
        processor.processBlock(buffer,midi);
        elapsed = std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-begin).count();
    }
    std::sort(times.begin(), times.end());
    const auto deadline = 1e6*block/rate;
    std::cout << "16 held voices / Paper Circuit / 48 kHz / 128 samples\n"
        << "median_us=" << times[times.size()/2] << " p99_us=" << times[times.size()*99/100]
        << " max_us=" << times.back() << " block_budget_us=" << deadline << '\n';
    return 0;
}
