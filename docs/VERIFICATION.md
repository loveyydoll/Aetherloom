# Verification record

Initialization date: 2026-10-22.

- Local Windows: CMake configuration succeeded with Visual Studio 18 2026, MSVC 19.51 and Windows SDK 10.0.26100.0; pinned JUCE 8.0.12.
- Local Windows Release: Standalone, VST3 and AetherloomTests built successfully at code commit `a60c32d115b524e9fcb27aa17943e3d586138d26`.
- Local CTest: 1/1 passed (the processor suite), 0 failures. No compiler errors were reported. An initial sandbox MSBuild worker stall was resolved by running the same build outside the sandbox.
- macOS AUv2/Standalone/VST3 and Windows CI: first run was still compiling when local verification completed. Check [run 39838972415](https://github.com/loveyydoll/Aetherloom/actions/runs/39838972415) for the final result; no CI success is claimed here.
- GarageBand, Logic Pro, auval, external MIDI, physical audio output and GUI interaction: not tested.
- No signed/notarised release or installer exists.

Processor tests cover silent initialization, MIDI event offsets, multi-note contribution, cutoff response, audio generation in all modes, finite output, release to silence, parameter-state round trip and malformed-state rejection. They do not replace listening tests, UI checks or plug-in host validation.
