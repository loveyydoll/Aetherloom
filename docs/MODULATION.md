# Modulation matrix

Eight independently automatable slots each select one source, one destination and a depth from -1 to +1. Source/destination Off disables a slot. Contributions to the same destination add, then clamp to [-2, +2]. Evaluation occurs every 32 voice samples; this is control-rate modulation.

| Source | Value |
| --- | --- |
| LFO 1 / LFO 2 | Bipolar [-1, +1]; reset on note-on |
| Filter Env / Mod Env | ADSR [0, 1] |
| Velocity | Note-on velocity [0, 1] |
| Key | (MIDI note - 60) / 60 |
| Mod Wheel | CC1 / 127, channel state inherited on new notes |
| Pressure | Channel pressure or latest poly-aftertouch for the note, / 127 |
| Random | One deterministic bipolar draw per voice/note start |
| Step | Arpeggiator transpose / 12, updated at generated step boundaries |

| Destination | Mapping at depth +1 and source value +1 |
| --- | --- |
| Pitch | +24 semitones to all three oscillators |
| Osc 1 / 2 / 3 Pitch | +12 semitones to that oscillator |
| Cutoff 1 / 2 | +4 octaves, with final coefficient cutoff clamped to [20 Hz, 0.4 × sample rate] |
| Resonance | +0.4 to both resonance settings, clamped to [0, 0.95] |
| Scan | +1 spectral position on all three spectral oscillators, clamped [0, 1] |
| FM | +1 phase-modulation amount, clamped [0, 1] |
| Pan | Rightward balance; final pan clamped [-0.95, +0.95] and smoothed |
| Level | Adds 1 to a base multiplier of 1; final range [0, 2] |
| Scene X / Y | +1 to the pad coordinate, clamped [0, 1] |

Negative depths reverse direction; bipolar LFOs move both ways around the base setting. Sub pitch follows keyboard and pitch bend; matrix pitch targets address the three main oscillators. Pulse width, effect controls and arp settings remain directly automatable, not matrix destinations in 0.3.

Filter envelope depth is separate from the matrix, in octaves. Keyboard tracking at 1 follows the keyboard by one cutoff octave per played octave around MIDI 60. Movement and character motion add LFO 1 cutoff movement. Warmth adds filter drive, Air raises cutoff, Space raises delay/reverb mix. These macros do not overwrite the underlying saved knob values.

The scene pad interpolates timbre controls, not recorded snapshots of arbitrary parameters. Hybrid uses scene colour for drive, mix for oscillator balance, sub for octave sine, FM for oscillator-1 phase modulation, and motion for cutoff movement. Classic preserves the earlier character implementation. Envelope, base brightness and level are independent.
