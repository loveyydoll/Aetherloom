// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace aetherloom {
// Shared immutable wavetable: constructed before rendering, ~16 KiB per processor.
class SineTable {
public:
    SineTable() {
        for (size_t i = 0; i <= size; ++i)
            data[i] = static_cast<float>(std::sin(6.283185307179586 * static_cast<double>(i) / size));
    }
    float operator()(double phase) const noexcept {
        phase -= std::floor(phase);
        const auto position = phase * size;
        // Tiny negative phases can round to exactly one after wrapping.
        const auto index = std::min(size - 1, static_cast<size_t>(position));
        const auto fraction = static_cast<float>(position - static_cast<double>(index));
        return data[index] + fraction * (data[index + 1] - data[index]);
    }
private:
    static constexpr size_t size = 4096;
    std::array<float, size + 1> data{};
};

struct Timbre { float colour, mix, sub, fm, motion; };
// Bilinear timbre interpolation, not Alchemy spectral/sample morphing.
inline Timbre morph(float x, float y) noexcept {
    constexpr std::array<Timbre, 4> corners{{
        {0.12f, 0.12f, 0.18f, 0.0f, 0.08f},  // Glow
        {0.7f, 0.8f, 0.05f, 0.55f, 0.18f},   // Glass
        {0.4f, 0.28f, 0.6f, 0.08f, 0.65f},   // Haze
        {0.95f, 0.65f, 0.25f, 0.9f, 0.9f}   // Fracture
    }};
    x = std::clamp(x, 0.0f, 1.0f); y = std::clamp(y, 0.0f, 1.0f);
    const std::array<float, 4> w{(1-x)*(1-y), x*(1-y), (1-x)*y, x*y};
    Timbre out{};
    for (size_t i = 0; i < w.size(); ++i) {
        out.colour += w[i]*corners[i].colour; out.mix += w[i]*corners[i].mix;
        out.sub += w[i]*corners[i].sub; out.fm += w[i]*corners[i].fm;
        out.motion += w[i]*corners[i].motion;
    }
    return out;
}
inline Timbre blend(Timbre a, Timbre b, float amount) noexcept {
    return {a.colour + amount*(b.colour-a.colour), a.mix + amount*(b.mix-a.mix),
        a.sub + amount*(b.sub-a.sub), a.fm + amount*(b.fm-a.fm),
        a.motion + amount*(b.motion-a.motion)};
}
inline float saturate(float x) noexcept { return x / (1.0f + std::abs(x)); }
}
