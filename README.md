# Aetherloom

An original cross-DAW character synthesizer by loveyydoll. Early alpha, C++20 / JUCE 8 / CMake.

## Implemented

- 16 polyphonic voices with voice stealing; MIDI note on/off, velocity, sustain pedal through JUCE Synthesiser, all-notes-off, and +/-2-semitone pitch wheel.
- Per-voice ADSR and one-pole low-pass filter; stereo or mono output.
- WARM: softly saturated sine with second harmonic. BLEND: colour-controlled mix of warm and quantised waves. GLITCH: quantised harmonic wave. These are intentionally small starter algorithms, not a finished signature sound.
- PLAY view: character, colour, level and clickable piano. EDIT: same performance path plus ADSR and cutoff. Host-automatable parameters and XML session-state recall.
- AUv2 instrument on macOS (`AU` in JUCE), Standalone, and VST3 build targets. Windows builds Standalone/VST3 as an additional development check.

## Planned

Signature patches, modulation routing, drift, richer glitch processing, preset browser, polished visual identity, smoothing/antialiasing, installers, signing/notarisation and release packaging. There are no copied artist patches or recordings. The simple quantisation can alias. Rapid automation can click. Do not treat this as a production-ready audio engine.

## Build and test

Requires Git, CMake 3.22+, C++20 compiler and internet for the pinned JUCE revision. On macOS install Xcode with its command-line tools; on Windows install Visual Studio with Desktop development with C++. JUCE is fetched at 8.0.12 commit `29396c22c93392d6738e021b83196283d6e4d850`.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
```

For a universal macOS build, add `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0` at configure time. CI uses the runner's native architecture. For an offline JUCE checkout add `-DFETCHCONTENT_SOURCE_DIR_JUCE=/absolute/path/to/JUCE`.

Artifacts are in `build/Aetherloom_artefacts/Release/` (or the corresponding configuration). Run the Standalone application and choose audio/MIDI devices in its settings; use the clickable keyboard or a MIDI controller. PLAY selects a view; it is not a transport button.

For macOS host testing, manually copy the built `AU/Aetherloom.component` into `~/Library/Audio/Plug-Ins/Components/`. VST3 hosts use `~/Library/Audio/Plug-Ins/VST3/`. Copying is deliberately not automatic. Restart/rescan your host. Run `auval -v aumu Aelm Lvyd`, then use the checklist in [docs/HOST_TESTING.md](docs/HOST_TESTING.md).

## Verification boundaries

AUv2 is the intended integration route for GarageBand and Logic Pro. Target configuration and compilation do **not** prove compatibility. No GarageBand or Logic host tests have been performed at project creation. See [docs/VERIFICATION.md](docs/VERIFICATION.md) for the recorded checks. CI builds and offline processor tests do not verify speakers, MIDI devices, DAW scanning or UI operation.

## Licence

AGPL-3.0-only; see [LICENSE](LICENSE) and [docs/LICENSING.md](docs/LICENSING.md). JUCE and its bundled dependencies retain their own licences. No commercial JUCE entitlement is assumed.
