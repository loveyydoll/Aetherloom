# Verification record

Initialization date: 2026-10-22.

- Local Windows: CMake configuration succeeded with Visual Studio 18 2026, MSVC 19.51 and Windows SDK 10.0.26100.0; pinned JUCE 8.0.12.
- Local Windows Release: Standalone, VST3 and AetherloomTests built successfully at code commit `a60c32d115b524e9fcb27aa17943e3d586138d26`.
- Local CTest: 1/1 passed (the processor suite), 0 failures. No compiler errors were reported. An initial sandbox MSBuild worker stall was resolved by running the same build outside the sandbox.
- macOS AUv2/Standalone/VST3 and Windows CI: first run was still compiling when local verification completed. Check [run 39838972415](https://github.com/loveyydoll/Aetherloom/actions/runs/39838972415) for the final result; no CI success is claimed here.
- GarageBand, Logic Pro, auval, external MIDI, physical audio output and GUI interaction: not tested.
- No signed/notarised release or installer exists.

Processor tests cover silent initialization, MIDI event offsets, multi-note contribution, cutoff response, audio generation in all modes, finite output, release to silence, parameter-state round trip and malformed-state rejection. They do not replace listening tests, UI checks or plug-in host validation.

## 0.2.0 — 2026-11-03

- Local Linux: independent CharacterDSPTests compiled with g++ 13, C++20, -O2 -Wall -Wextra -Werror and passed sine-table error bounds, phase wrapping, morph weight/corner and saturation checks.
- AddressSanitizer + UndefinedBehaviorSanitizer DSP run passed (leak detection disabled because this environment uses ptrace).
- JUCE 8.0.12 header-level syntax checks passed for processor, editor, processor tests and benchmark using g++ 13; this does not constitute a linked plugin build.
- `bash -n scripts/install-au.sh` and `git diff --check` passed.
- Expanded JUCE processor suite covers all factory patches, new control/state recall, legacy-state defaults, audible FM/morph/motion response and finite bounded output across 44.1/48/96 kHz, 1/17/64/512/1024 samples and voice stealing.
- Full JUCE/macOS/Windows build, processor tests, AU architecture/signing/auval and benchmark are configured in CI; results must be read from the PR run, not inferred from test source.
- M5 Air performance, Logic Pro/GarageBand loading, listening, Retina/keyboard interaction and long-session thermal behaviour remain untested on physical hardware.

## 0.3.0 development — 2026-11-03

- Previous 0.2 head e008dd6 passed macOS/Windows CI, including architecture/signature/auval and archive packaging: https://github.com/loveyydoll/Aetherloom/actions/runs/41664527999 . This does not validate 0.3.
- Independent HybridDSPTests passed locally with g++ 13, C++20, -O2 -Wall -Wextra -Werror: waveform bounds, PolyBLEP saw seam, high-note spectral mip selection, resonant filter stability/DC and parameter contracts.
- New processor, UI, arp and benchmark source passed JUCE 8.0.12 header syntax checks. Linked test/CI results must be recorded in the PR; test source alone is not a passing result.
- Added regression coverage for Hybrid tuning/routes/filters/effects/limits, all parameter/user-preset recall, Classic defaults, arpeggiator audio block equivalence and mono/oversized chorus buffers.
- Added standalone arpeggiator tests for offsets/cross-block timing, direction/octaves, swing, sustain ownership and panic. Optional editor snapshots exercise PLAY and all six EDIT pages.
- Physical Logic Pro/GarageBand playback, MIDI hardware, native UI/file panels, M5 load/thermal measurements, signing/notarisation and distribution acceptance remain separate unperformed checks.

### Local 0.3 checks

- Linked Linux/g++ 13 build of processor/editor, AetherloomTests, CharacterDSPTests, HybridDSPTests, ArpeggiatorTests and benchmark completed with pinned JUCE 8.0.12.
- CTest: 4/4 passed. Coverage includes conventional expression before note-on/reset, cross-block arpeggiated audio, feedback tail/panic, parameter/user-preset recall, mono and larger-than-prepared chorus buffers.
- HybridDSPTests passed AddressSanitizer + UndefinedBehaviorSanitizer with leak detection disabled for this ptrace-based environment.
- Software-rendered PLAY and all six EDIT pages inspected; fixed Unicode display, numeric formatting and matrix viewport fit. This is not native host/Retina/file-panel testing.
- Local shared x86 Linux benchmark, 16 held notes / 128 samples: Classic median 110.014 us; Hybrid stress (3 unison, 8 routes, chorus/delay/reverb) median 425.702 us at 48 kHz and 424.761 us at 96 kHz. Maxima occasionally exceeded block budget under shared scheduling; these measurements are not a realtime guarantee or M5 measurement.
- Native macOS/Windows CI and auval status is tracked on the PR for the actual head commit. The physical-machine/host limits above still apply.
