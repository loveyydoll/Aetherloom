# Verification record

Initialization date: 2026-10-22.

- Local Windows: CMake configuration succeeded with Visual Studio 18 2026, MSVC 19.51 and Windows SDK 10.0.26100.0; pinned JUCE 8.0.12.
- Build and processor test results: pending first validation; see GitHub Actions for commit-specific results.
- macOS AUv2/Standalone/VST3: build job supplied; results must be checked per commit.
- GarageBand, Logic Pro, auval, external MIDI, physical audio output and GUI interaction: not tested.
- No signed/notarised release or installer exists.

Processor tests cover silent initialization, MIDI event offsets, multi-note contribution, cutoff response, audio generation in all modes, finite output, release to silence, parameter-state round trip and malformed-state rejection. They do not replace listening tests, UI checks or plug-in host validation.
