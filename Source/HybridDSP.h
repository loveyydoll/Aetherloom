// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "CharacterDSP.h"
#include <cstdint>

namespace aetherloom {
inline float randomBipolar(uint32_t& state) noexcept {
    state ^= state << 13; state ^= state >> 17; state ^= state << 5;
    return static_cast<float>(state >> 8) * (2.0f/16777215.0f)-1.0f;
}
inline float polyBlep(double phase, double delta) noexcept {
    if (phase < delta) { const auto t = phase/delta; return static_cast<float>(2*t-t*t-1); }
    if (phase > 1-delta) { const auto t = (phase-1)/delta; return static_cast<float>(t*t+2*t+1); }
    return 0;
}
// Immutable mipmapped spectral frames, allocated once outside rendering (~256 KiB).
class WavetableBank {
public:
    explicit WavetableBank(const SineTable& sine) {
        for (size_t band=0; band<bands; ++band) for (size_t frame=0; frame<frames; ++frame) {
            auto& table = data[band][frame];
            float sum = 0;
            for (int h=1; h<= (1 << band); ++h) {
                float weight = h==1 ? 1.0f : 0.0f;
                if (frame==1) weight = h%2 ? (h%4==1 ? 1.0f : -1.0f)/static_cast<float>(h*h) : 0.0f;
                if (frame==2) weight = 1.0f/static_cast<float>(h);
                if (frame==3) weight = (h%3==0 ? 0.7f : 0.15f)/std::sqrt(static_cast<float>(h));
                sum += std::abs(weight);
                for (size_t i=0; i<size; ++i) table[i] += weight*sine(static_cast<double>(i*h)/size);
            }
            for (size_t i=0; i<size; ++i) table[i] /= std::max(1.0f,sum);
            table[size] = table[0];
        }
    }
    float read(double phase, double delta, float scan) const noexcept {
        int band=0;
        while (band<static_cast<int>(bands)-1 && (1 << (band+1))*delta < 0.45) ++band;
        const auto position = std::clamp(scan,0.0f,1.0f)*static_cast<float>(frames-1);
        const auto frame = std::min(frames-2,static_cast<size_t>(position));
        const auto blend = position-static_cast<float>(frame);
        const auto a = lookup(data[static_cast<size_t>(band)][frame],phase);
        return a + blend*(lookup(data[static_cast<size_t>(band)][frame+1],phase)-a);
    }
    float oscillator(int wave,double phase,double delta,float scan,float width,const SineTable& sine) const noexcept {
        phase -= std::floor(phase); delta=std::clamp(delta,0.000001,0.45);
        if (wave==0) return sine(phase);
        if (wave==1) return read(phase,delta,1.0f/3.0f);
        if (wave==2) return static_cast<float>(2*phase-1)-polyBlep(phase,delta);
        if (wave==3) {
            auto shifted = phase-width; shifted-=std::floor(shifted);
            return (phase<width ? 1.0f : -1.0f)+polyBlep(phase,delta)-polyBlep(shifted,delta)-(2*width-1);
        }
        return read(phase,delta,scan);
    }
private:
    static constexpr size_t size=2048, bands=8, frames=4;
    using Table = std::array<float,size+1>;
    static float lookup(const Table& dataIn,double phase) noexcept {
        phase-=std::floor(phase);
        const auto at=phase*size;
        const auto i=std::min(size-1,static_cast<size_t>(at));
        return dataIn[i]+static_cast<float>(at-static_cast<double>(i))*(dataIn[i+1]-dataIn[i]);
    }
    std::array<std::array<Table,frames>,bands> data{};
};
// Topology-preserving state-variable filter. Coefficients update at a bounded control rate.
class StateFilter {
public:
    void reset() noexcept { ic1=ic2=0; }
    void configure(double sampleRate,float cutoff,float resonance) noexcept {
        const auto g=std::tan(3.141592653589793*std::clamp(static_cast<double>(cutoff),20.0,sampleRate*0.4)/sampleRate);
        k=2.0f-1.9f*std::clamp(resonance,0.0f,0.95f);
        a1=static_cast<float>(1/(1+g*(g+k))); a2=static_cast<float>(g)*a1; a3=static_cast<float>(g)*a2;
    }
    float process(float input,int type) noexcept {
        const auto v3=input-ic2, v1=a1*ic1+a2*v3, v2=ic2+a2*ic1+a3*v3;
        ic1=2*v1-ic1; ic2=2*v2-ic2;
        const auto high=input-k*v1-v2;
        return type==0 ? v2 : type==1 ? v1 : type==2 ? high : high+v2;
    }
private:
    float a1=1,a2=0,a3=0,k=2,ic1=0,ic2=0;
};
class Lfo {
public:
    void reset() noexcept { phase=0; held=0; }
    float next(float hz,double sampleRate,int shape,const SineTable& sine,uint32_t& random) noexcept {
        const auto value = shape==0 ? sine(phase) : shape==1 ? static_cast<float>(1-4*std::abs(phase-0.5))
            : shape==2 ? static_cast<float>(2*phase-1) : shape==3 ? (phase<0.5 ? 1.0f : -1.0f) : held;
        phase += hz/sampleRate;
        if (phase>=1) { phase-=std::floor(phase); held=randomBipolar(random); }
        return value;
    }
private:
    double phase=0;
    float held=0;
};
}
