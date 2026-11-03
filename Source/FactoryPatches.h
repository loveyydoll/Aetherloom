// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "SynthParameters.h"
#include <vector>
#include <utility>
#include <initializer_list>

namespace aetherloom {
struct FactoryPatch {
    const char* name;
    const char* category;
    std::array<float,16> classic;
    std::array<float,ParameterCount> values{};
};
inline const std::vector<FactoryPatch>& hybridPatches() {
    static const auto patches=[] {
        std::vector<FactoryPatch> result;
        auto add=[&](const char* name,const char* category,std::array<float,16> classic,std::initializer_list<std::pair<Key,float>> overrides) {
            FactoryPatch patch{name,category,classic,{}};
            for (size_t i=0;i<specs.size();++i) patch.values[i]=specs[i].initial;
            patch.values[Engine]=1;
            for (const auto& [key,value]:overrides) patch.values[static_cast<size_t>(key)]=value;
            result.push_back(patch);
        };
        // Old controls: colour, gain, cutoff, amp ADSR, mix, sub, FM, motion, rate, scene X/Y/amount, mode.
        const std::array<float,16> keys{0.35f,0.7f,5000,0.005f,0.6f,0.35f,0.4f,0,0,0,0,1,0,0,0,0};
        const std::array<float,16> pad{0.4f,0.6f,2500,0.8f,1.2f,0.8f,2,0,0.1f,0,0.1f,1,0,0,0,0};
        const std::array<float,16> bass{0.2f,0.7f,1200,0.003f,0.2f,0.55f,0.12f,0,0.1f,0,0,1,0,0,0,0};
        add("Copper Keys","Keys",keys,{{Wave1,1},{Wave2,0},{Semi2,12},{FM,0.28f},{EnvDepth,2},{EnvDecay,0.4f},{Chorus,0.12f},{Reverb,0.08f}});
        add("Velvet Machine","Keys",keys,{{Wave1,3},{Width1,0.32f},{Wave2,1},{Fine2,7},{Semi2,0},{Filter1,0},{Drive,0.3f},{EnvDepth,2.5f},{EnvDecay,0.3f},{Warmth,0.25f},{Chorus,0.15f}});
        add("Glass Elevator","Keys",keys,{{Wave1,0},{Wave2,4},{Scan2,1},{Semi2,19},{FM,0.7f},{Level2,0.08f},{Level3,0},{DelayMix,0.2f},{DelaySync,3},{Reverb,0.15f}});
        add("Weather Archive","Pads",pad,{{Wave1,4},{Scan1,0.7f},{Wave2,4},{Scan2,0.35f},{Semi2,7},{Unison,2},{Detune,12},{Chorus,0.25f},{Reverb,0.35f},{Source1,1},{Target1,8},{Amount1,0.4f},{LfoRate1,0.12f},{EnvDepth,0.5f}});
        add("Orbit Choir","Pads",pad,{{Wave1,4},{Scan1,1},{Wave2,1},{Semi2,7},{Wave3,0},{Semi3,12},{Level3,0.3f},{Filter2,1},{FilterMix,0.35f},{Routing,1},{Cutoff2,1600},{Reverb,0.4f},{Source1,2},{Target1,10},{Amount1,0.5f},{LfoRate2,0.13f}});
        add("Magnetic Dawn","Pads",pad,{{Wave1,2},{Wave2,2},{Semi2,0},{Fine2,-9},{Wave3,3},{Semi3,-12},{Unison,2},{Detune,8},{Resonance1,0.45f},{Source1,4},{Target1,5},{Amount1,0.45f},{ModAttack,2},{ModDecay,3},{ModSustain,0.2f},{Reverb,0.25f}});
        add("Basement Current","Bass",bass,{{Wave1,2},{Wave2,3},{Semi2,-12},{Level2,0.4f},{Level3,0},{EnvDepth,2.5f},{EnvDecay,0.18f},{EnvSustain,0},{Drive,0.5f},{Resonance1,0.35f}});
        add("Rubber Voltage","Bass",bass,{{Wave1,3},{Width1,0.2f},{Wave2,0},{Semi2,12},{FM,0.4f},{EnvDepth,3},{EnvDecay,0.12f},{EnvSustain,0},{Resonance1,0.6f},{FilterMix,0.2f}});
        add("Neon Thread","Leads",keys,{{Wave1,2},{Wave2,3},{Semi2,0},{Unison,1},{Detune,14},{EnvDepth,0.7f},{DelayMix,0.15f},{DelaySync,3},{Source1,7},{Target1,1},{Amount1,0.08f}});
        add("Fracture Bloom","Textures",pad,{{Wave1,4},{Scan1,1},{Wave2,4},{Scan2,1},{Semi2,7},{Ring,0.65f},{FM,0.5f},{Noise,0.07f},{Filter1,1},{Resonance1,0.5f},{Filter2,2},{Routing,1},{Chorus,0.2f},{Reverb,0.35f},{Source1,9},{Target1,8},{Amount1,0.5f}});
        add("Clockwork Garden","Sequences",keys,{{ArpOn,1},{ArpMode,2},{ArpOctaves,1},{ArpDivision,1},{ArpSwing,0.12f},{Step3,7},{Step7,12},{Wave1,3},{Width1,0.3f},{EnvDepth,3},{EnvDecay,0.15f},{EnvSustain,0},{DelayMix,0.25f},{DelaySync,3},{Source1,10},{Target1,5},{Amount1,0.3f}});
        add("Broken Satellites","Sequences",keys,{{ArpOn,1},{ArpMode,3},{Step2,7},{Step4,-12},{Step6,12},{Step8,-5},{Wave1,4},{Scan1,0.8f},{FM,0.35f},{LfoShape2,4},{LfoSync2,2},{Source1,2},{Target1,8},{Amount1,0.6f},{DelayMix,0.3f},{DelaySync,2},{Feedback,0.55f},{Reverb,0.2f}});
        return result;
    }();
    return patches;
}
}
