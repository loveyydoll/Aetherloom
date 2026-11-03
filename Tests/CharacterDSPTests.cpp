// SPDX-License-Identifier: AGPL-3.0-only
#include "../Source/CharacterDSP.h"
#include <iostream>
#include <stdexcept>
int main() {
    auto require = [](bool ok) { if (!ok) throw std::runtime_error("DSP invariant failed"); };
    aetherloom::SineTable table;
    for (int i = -50000; i < 50000; ++i) {
        const auto phase = static_cast<double>(i)/10000;
        require(std::abs(table(phase)-std::sin(phase*6.283185307179586)) < 0.000001);
        require(std::isfinite(aetherloom::saturate(table(phase)*10)));
    }
    const auto glow = aetherloom::morph(0,0), glass = aetherloom::morph(1,0);
    const auto haze = aetherloom::morph(0,1), fracture = aetherloom::morph(1,1);
    const auto middle = aetherloom::morph(0.5f,0.5f);
    require(std::abs(middle.fm-(glow.fm+glass.fm+haze.fm+fracture.fm)*0.25f) < 0.00001f);
    require(aetherloom::blend(glow, glass, 0).fm == glow.fm);
    require(aetherloom::blend(glow, glass, 1).fm == glass.fm);
    require(aetherloom::morph(-1,2).sub == haze.sub);
    std::cout << "Passed: sine table error bound, negative/wrapped phases, morph corners/weights, saturation\n";
}
