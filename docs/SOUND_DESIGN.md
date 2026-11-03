# Sound design in 0.2.0

| Reference idea | Aetherloom implementation | How to use it |
| --- | --- | --- |
| Alchemy performance controls | Four original timbre corners, bilinear parameter blending | PLAY: raise Scene amount, drag the pad; EDIT has equivalent X/Y sliders |
| ES2 three-oscillator balance | Character oscillator, second harmonic, octave-down sine | EDIT: Body/Harmonic blends the first two; Sub weight blends in the third |
| ES2 oscillator FM | Second-harmonic sine phase-modulates the character carrier, with high-note depth reduction | Raise Glass (FM); Brightness softens the result |
| Synth modulation | Per-note sine LFO modulates filter brightness over +/-2 octaves at full depth | Motion depth + Motion speed in Hz |
| Envelopes | Per-voice ADSR | Fade in / Fall / Hold / Fade out |
| Patch performance | Six original factory programs, saved edited state, host automation | Select a sound and then edit; save the host project |

The scene pad controls colour, oscillator balance, sub, FM and motion depth together. Scene amount crossfades between the editable base timbre (0) and scene timbre (1). At 1, those five base controls no longer affect the result; lower Scene amount to mix them back in. Level, envelope, brightness, mode and motion speed remain independent.

These are implemented concepts, not replicas of Apple's algorithms or patch assets. Alchemy's additive/spectral/granular/sample engine, ES2's full router/vector envelope/dual filters, and effects are not present. Oscillator 2 is a harmonic source, not an independently tuned wavetable oscillator. No artist patch is claimed to be reproduced.

Reference documentation:
- https://support.apple.com/guide/logicpro/alchemy-overview-lgsi2618652a/mac
- https://support.apple.com/guide/logicpro/alchemy-morph-use-and-tips-lgsi6215646f/mac
- https://help.apple.com/logicpro/mac/9.1.6/en/logicpro/instruments/chapter_5_section_3.html

## First experiments

- Velvet Steps: short chords; adjust Sub weight and Fall for a soft rhythmic patch.
- Glass Rooms: reduce Scene amount to make manual FM more prominent, then change Brightness.
- Slow Orbit: hold a chord, move between Glow and Haze, adjust Motion speed.
- Paper Circuit: quantised texture; use a lower Brightness if high notes are harsh.

All names and settings are original. No extra sample downloads are required.
