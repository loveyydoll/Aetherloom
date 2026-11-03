# Changelog

## 0.2.0 — 2026-11-03

- Target MacBook Air M5 / 16 GB / 1 TB with native arm64-only macOS AUv2 and Standalone defaults; keep VST3 opt-in on macOS.
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
