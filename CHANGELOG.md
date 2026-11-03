# Changelog

## 0.3.0 — 2026-11-03

- Add a separate 16-voice Hybrid engine: three independently tuned five-wave oscillators, four-frame/eight-band spectral bank, PolyBLEP saw/pulse, up to three unison lanes, detune/stereo spread, FM, ring, noise and sub.
- Add dual stereo TPT resonant filters (LP/BP/HP/notch, serial/parallel), drive, keyboard tracking, independent filter/mod ADSRs and two five-shape tempo/free LFOs.
- Add eight bipolar modulation slots, conventional MIDI expression inheritance and explicit target units. Add four performance macros; retain scene blending and Classic compatibility.
- Add a sample-offset arpeggiator with direction/octaves/gate/swing, eight transpose steps, sustain ownership, stop/seek/panic resets and MIDI-isolated render intervals.
- Add stereo chorus, smoothed interpolated ping-pong delay and reverb; mono/oversized-block chorus scratch, prepared delay storage, dry bypass, output ceiling and conservative effect-aware tail reporting.
- Add 12 Hybrid factory sounds (18 total), 98 new controls (114 total), appended AU version hint 3, full missing-control defaults and edited-state recall.
- Replace EDIT with six scrolling pages; add search/category browser, constrained MUTATE, RELOAD, user preset SAVE/LOAD and audio-thread PANIC.
- Avoid resetting ADSR release rates on unchanged block updates; format UI values in Hz/ms/percent and show all eight routes together.
- Add DSP/filter/parameter, arp clock/ownership, modulation/effects, preset, block-partition and mono/large-buffer tests; extend the render benchmark and optional software editor snapshots.
- Keep native arm64 AU/Standalone builds and development packaging. No full Alchemy/ES2 parity, user sample/wavetable import, MPE, oversampling or physical M5/DAW validation is claimed.

## 0.2.0 — 2026-11-03

- Target MacBook Air M5 / 16 GB / 1 TB with native arm64-only, sandbox-safe macOS AUv2 and Standalone defaults; keep VST3 opt-in on macOS.
- Add six original factory patches, host program/session recall and backward defaults for new controls.
- Add a PLAY scene pad and EDIT X/Y sliders, three-source oscillator mix, sub, FM and filter motion with explanatory labels/tooltips.
- Cache parameter pointers, share a sine lookup table, remove per-sample sine/tanh work, reduce coefficient calculations, smooth timbre/mode/output and enable macOS Release optimisation/LTO.
- Preserve AU identity and existing parameter IDs/version hints; append controls with version hint 2.
- Add local AU install/ad-hoc signing/auval script, native macOS CI, component artifact, DSP/processor regression tests and a render benchmark.
- Document aliasing, changed WARM saturation, free-running motion and unverified M5/host performance. No full Alchemy/ES2 parity is claimed.

## 0.1.0 — 2026-10-22

- Initialize C++20/JUCE/CMake AUv2, Standalone and VST3 targets.
- Add playable 16-voice synth, MIDI, ADSR, low-pass filter and starter WARM/BLEND/GLITCH engines.
- Add PLAY/EDIT editor, piano, automatable controls and session state.
- Add processor smoke tests and macOS/Windows build CI.
- Select AGPL-3.0-only based on JUCE 8's open-source licensing; document historical GPL uncertainty.
- GarageBand/Logic compatibility remains unverified pending real host tests.
