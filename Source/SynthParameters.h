// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <array>

namespace aetherloom {
enum class Page { Play, Oscillators, Filters, Motion, Matrix, Effects, Arp };
enum Key {
    Engine, Warmth, Air, Movement, Space,
    Wave1, Semi1, Fine1, Level1, Scan1, Width1,
    Wave2, Semi2, Fine2, Level2, Scan2, Width2,
    Wave3, Semi3, Fine3, Level3, Scan3, Width3,
    Unison, Detune, Spread, FM, Ring, Noise,
    Filter1, Filter2, Cutoff2, Resonance1, Resonance2, Routing, FilterMix, Drive, KeyTrack, EnvDepth,
    EnvAttack, EnvDecay, EnvSustain, EnvRelease,
    LfoShape1, LfoRate1, LfoSync1, LfoShape2, LfoRate2, LfoSync2,
    ModAttack, ModDecay, ModSustain, ModRelease,
    Chorus, DelayMix, DelayTime, DelaySync, Feedback, Reverb, Size,
    ArpOn, ArpMode, ArpDivision, ArpOctaves, ArpGate, ArpSwing,
    Step1, Step2, Step3, Step4, Step5, Step6, Step7, Step8,
    Source1, Target1, Amount1, Source2, Target2, Amount2, Source3, Target3, Amount3, Source4, Target4, Amount4,
    Source5, Target5, Amount5, Source6, Target6, Amount6, Source7, Target7, Amount7, Source8, Target8, Amount8,
    ParameterCount
};
struct ParameterSpec {
    const char* id;
    const char* name;
    float low, high, initial, skew;
    Page page;
    const char* choices = nullptr;
};
inline constexpr const char* waves = "Sine|Triangle|Saw|Pulse|Spectral";
inline constexpr const char* shapes = "Sine|Triangle|Ramp|Square|Sample & Hold";
inline constexpr const char* divisions = "1/32|1/16|1/8|1/4|1/2|1 bar";
inline constexpr const char* sources = "Off|LFO 1|LFO 2|Filter Env|Mod Env|Velocity|Key|Mod Wheel|Pressure|Random|Step";
inline constexpr const char* targets = "Off|Pitch|Osc 1 Pitch|Osc 2 Pitch|Osc 3 Pitch|Cutoff 1|Cutoff 2|Resonance|Scan|FM|Pan|Level|Scene X|Scene Y";
inline constexpr std::array<ParameterSpec, ParameterCount> specs{{
    {"engine","Engine",0,1,0,1,Page::Play,"Classic|Hybrid"},
    {"warmth","Warmth",0,1,0,1,Page::Play}, {"air","Air",0,1,0,1,Page::Play},
    {"movement","Movement",0,1,0,1,Page::Play}, {"space","Space",0,1,0,1,Page::Play},
    {"osc1wave","Osc 1 wave",0,4,2,1,Page::Oscillators,waves},
    {"osc1semi","Osc 1 semitones",-24,24,0,1,Page::Oscillators}, {"osc1fine","Osc 1 cents",-50,50,0,1,Page::Oscillators},
    {"osc1level","Osc 1 level",0,1,0.7f,1,Page::Oscillators}, {"osc1scan","Osc 1 spectrum",0,1,0,1,Page::Oscillators}, {"osc1width","Osc 1 pulse width",0.05f,0.95f,0.5f,1,Page::Oscillators},
    {"osc2wave","Osc 2 wave",0,4,0,1,Page::Oscillators,waves},
    {"osc2semi","Osc 2 semitones",-24,24,12,1,Page::Oscillators}, {"osc2fine","Osc 2 cents",-50,50,4,1,Page::Oscillators},
    {"osc2level","Osc 2 level",0,1,0.25f,1,Page::Oscillators}, {"osc2scan","Osc 2 spectrum",0,1,0.5f,1,Page::Oscillators}, {"osc2width","Osc 2 pulse width",0.05f,0.95f,0.5f,1,Page::Oscillators},
    {"osc3wave","Osc 3 wave",0,4,1,1,Page::Oscillators,waves},
    {"osc3semi","Osc 3 semitones",-24,24,-12,1,Page::Oscillators}, {"osc3fine","Osc 3 cents",-50,50,-4,1,Page::Oscillators},
    {"osc3level","Osc 3 level",0,1,0.2f,1,Page::Oscillators}, {"osc3scan","Osc 3 spectrum",0,1,0.2f,1,Page::Oscillators}, {"osc3width","Osc 3 pulse width",0.05f,0.95f,0.5f,1,Page::Oscillators},
    {"unison","Unison",0,2,0,1,Page::Oscillators,"1 voice|2 voices|3 voices"},
    {"detune","Unison detune (cents)",0,35,9,1,Page::Oscillators}, {"spread","Unison width",0,1,0.6f,1,Page::Oscillators},
    {"hybridFm","Osc 2 > Osc 1 FM",0,1,0,1,Page::Oscillators}, {"ring","Ring modulation",0,1,0,1,Page::Oscillators}, {"noise","Noise",0,0.5f,0,1,Page::Oscillators},
    {"filter1type","Filter 1",0,3,0,1,Page::Filters,"Low-pass|Band-pass|High-pass|Notch"},
    {"filter2type","Filter 2",0,3,0,1,Page::Filters,"Low-pass|Band-pass|High-pass|Notch"},
    {"cutoff2","Filter 2 cutoff (Hz)",20,18000,12000,0.25f,Page::Filters},
    {"resonance1","Filter 1 resonance",0,0.95f,0.15f,1,Page::Filters}, {"resonance2","Filter 2 resonance",0,0.95f,0.1f,1,Page::Filters},
    {"filterRouting","Routing",0,1,0,1,Page::Filters,"Serial|Parallel"}, {"filterMix","Filter blend",0,1,0.5f,1,Page::Filters},
    {"drive","Filter drive",0,1,0.1f,1,Page::Filters}, {"keyTrack","Keyboard tracking",0,1,0,1,Page::Filters}, {"filterEnvDepth","Envelope depth (oct)",-5,5,1.5f,1,Page::Filters},
    {"filterAttack","Filter attack (s)",0.001f,5,0.01f,0.35f,Page::Filters}, {"filterDecay","Filter decay (s)",0.001f,5,0.6f,0.35f,Page::Filters},
    {"filterSustain","Filter sustain",0,1,0.3f,1,Page::Filters}, {"filterRelease","Filter release (s)",0.001f,5,0.5f,0.35f,Page::Filters},
    {"lfo1shape","LFO 1 shape",0,4,0,1,Page::Motion,shapes}, {"lfo1rate","LFO 1 speed (Hz)",0.02f,20,0.5f,0.35f,Page::Motion},
    {"lfo1sync","LFO 1 clock",0,6,0,1,Page::Motion,"Free|1/32|1/16|1/8|1/4|1/2|1 bar"},
    {"lfo2shape","LFO 2 shape",0,4,1,1,Page::Motion,shapes}, {"lfo2rate","LFO 2 speed (Hz)",0.02f,20,2,0.35f,Page::Motion},
    {"lfo2sync","LFO 2 clock",0,6,0,1,Page::Motion,"Free|1/32|1/16|1/8|1/4|1/2|1 bar"},
    {"modAttack","Mod attack (s)",0.001f,5,0.2f,0.35f,Page::Motion}, {"modDecay","Mod decay (s)",0.001f,5,0.8f,0.35f,Page::Motion},
    {"modSustain","Mod sustain",0,1,0.5f,1,Page::Motion}, {"modRelease","Mod release (s)",0.001f,5,0.6f,0.35f,Page::Motion},
    {"chorus","Chorus mix",0,1,0,1,Page::Effects}, {"delayMix","Delay mix",0,0.7f,0,1,Page::Effects},
    {"delayTime","Delay time (s)",0.02f,2,0.3f,0.4f,Page::Effects}, {"delaySync","Delay clock",0,6,0,1,Page::Effects,"Free|1/32|1/16|1/8|1/4|1/2|1 bar"},
    {"delayFeedback","Delay feedback",0,0.85f,0.35f,1,Page::Effects}, {"reverbMix","Reverb mix",0,0.7f,0,1,Page::Effects}, {"reverbSize","Reverb size",0,1,0.5f,1,Page::Effects},
    {"arpOn","Arpeggiator",0,1,0,1,Page::Arp,"Off|On"}, {"arpMode","Pattern",0,3,0,1,Page::Arp,"Up|Down|Up / Down|As played"},
    {"arpDivision","Step length",0,5,1,1,Page::Arp,divisions}, {"arpOctaves","Octaves",0,2,0,1,Page::Arp,"1|2|3"},
    {"arpGate","Gate",0.05f,0.95f,0.65f,1,Page::Arp}, {"arpSwing","Swing",0,0.45f,0,1,Page::Arp},
    {"step1","Step 1 transpose",-12,12,0,1,Page::Arp}, {"step2","Step 2 transpose",-12,12,0,1,Page::Arp},
    {"step3","Step 3 transpose",-12,12,0,1,Page::Arp}, {"step4","Step 4 transpose",-12,12,0,1,Page::Arp},
    {"step5","Step 5 transpose",-12,12,0,1,Page::Arp}, {"step6","Step 6 transpose",-12,12,0,1,Page::Arp},
    {"step7","Step 7 transpose",-12,12,0,1,Page::Arp}, {"step8","Step 8 transpose",-12,12,0,1,Page::Arp},
#define AETHERLOOM_ROUTE(n) \
    {"mod" #n "source","Route " #n " source",0,10,0,1,Page::Matrix,sources}, \
    {"mod" #n "target","Route " #n " target",0,13,0,1,Page::Matrix,targets}, \
    {"mod" #n "amount","Route " #n " depth",-1,1,0,1,Page::Matrix}
    AETHERLOOM_ROUTE(1), AETHERLOOM_ROUTE(2), AETHERLOOM_ROUTE(3), AETHERLOOM_ROUTE(4),
    AETHERLOOM_ROUTE(5), AETHERLOOM_ROUTE(6), AETHERLOOM_ROUTE(7), AETHERLOOM_ROUTE(8)
#undef AETHERLOOM_ROUTE
}};
inline constexpr std::array<float,6> beats{0.125f,0.25f,0.5f,1,2,4};
}
