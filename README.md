# Aetherloom

An original character synthesizer by loveyydoll. **0.2.0 alpha**, C++20 / JUCE 8.
Built for making music in **Logic Pro and GarageBand on Apple Silicon**. Primary target: MacBook Air M5, 16 GB unified memory, 1 TB SSD.

## Play first, edit when you want

- Six original factory patches: **Init, Velvet Steps, Glass Rooms, Slow Orbit, Paper Circuit, Low Lantern**. Load one, play the piano or MIDI controller, then shape it.
- **PLAY:** WARM / BLEND / GLITCH, Colour, Level, Scene amount and a four-corner pad: **Glow / Glass / Haze / Fracture**. Raise Scene amount to hear the pad. At zero, EDIT's timbre controls determine the sound.
- **EDIT:** character oscillator + second harmonic + octave-down sine, Body/Harmonic balance, Sub weight, Glass (FM), brightness, per-voice motion depth/speed and ADSR described as Fade in / Fall / Hold / Fade out. Tooltips explain each control; the scene pad also has X/Y sliders.
- Scene blending interpolates colour, oscillator balance, sub, FM and motion depth. Envelope, brightness, rate, mode and level stay independent. This is a small timbre interpolation engine, **not** Alchemy sample/spectral morphing.
- 16 voices, voice stealing, velocity, sustain, +/-2-semitone pitch wheel, mono/stereo output, automatable controls and session recall. Factory sounds are original; no Apple presets, samples or code are included.

Inspired by Alchemy's performance-first morphing and ES2's oscillator mixing/FM, with fewer controls at once. Granular/additive sample synthesis, sample import, a modulation matrix, vector envelopes, tempo sync and effects are future work, not implemented features.

## Native Apple Silicon AU

The macOS default is **AUv2 `Aetherloom.component` + Standalone**, arm64 only. `.component` is the plugin bundle extension used by Logic Pro and GarageBand; renaming a VST3 or using a Logic `.patch` file would not turn it into an AU. Existing manufacturer `Lvyd`, subtype `Aelm`, bundle ID and parameter IDs remain stable. New AU controls use version hint 2 so old automation addresses keep their ordering.

Install Xcode and its command-line tools, Git and CMake 3.22+. JUCE 8.0.12 is pinned at `29396c22c93392d6738e021b83196283d6e4d850` and fetched during configuration. From a native Terminal:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
bash scripts/install-au.sh
```

Quit the hosts before installation, then restart them. GarageBand: enable Audio Units in Settings > Audio/MIDI; on a software-instrument track open Smart Controls > Plug-ins > instrument > AU Instruments > loveyydoll > Aetherloom. Logic: instrument slot > AU Instruments > loveyydoll > Aetherloom; use Plug-in Manager to rescan if needed. The installer checks arm64, ad-hoc signs the local build, copies it into `~/Library/Audio/Plug-Ins/Components/` and runs `auval -v aumu Aelm Lvyd`.

These are local development builds, not signed/notarised distribution packages. CI's Apple Silicon component can be downloaded from the run's artifacts; preserve/reconstruct its bundle structure and pass its path to the install script. Native host testing is still required.

Optional VST3: add `-DAETHERLOOM_BUILD_VST3=ON` on macOS. Non-Apple builds retain Standalone/VST3 for development checks. Intel/universal macOS builds are intentionally rejected. Offline JUCE checkout: `-DFETCHCONTENT_SOURCE_DIR_JUCE=/absolute/path/to/JUCE`.

## M5 Air performance approach

One immutable interpolated sine table per processor (~16 KiB), cached atomic parameter pointers, no per-sample sine/tanh calls, bounded 16-voice work, control-rate filter coefficient updates, no render-time sample loading or disk I/O, denormal protection, 20 ms timbre/output smoothing and Release optimisation/LTO on macOS. Uses the arm64 baseline instead of an unverified M5-only compiler flag. The engine is small; the 1 TB SSD is for projects, not a required sample library. No GPU worker threads or large sample cache compete for unified memory.

Run `build/AetherloomBenchmark_artefacts/Release/AetherloomBenchmark` for median/p99/max render time of 16 voices at 48 kHz/128 samples. Compare against its printed block budget; it is a diagnostic, not a guarantee. Measure on the M5 Air after a long session to capture thermal behaviour. No M5 measurements have been made here.

## Alpha boundaries

FM and quantisation can alias, especially GLITCH; this is not an oversampled engine. Smoothing reduces timbre/level discontinuities but does not guarantee click-free voice stealing, pitch wheel or live envelope changes. WARM's saturation has changed, so old sessions load their controls but may sound different. Legacy states missing new controls receive Init defaults. Patch names indicate the last loaded patch; edited sounds are saved in the host project. Motion is free-running per note, not tempo synced.

See [host checks](docs/HOST_TESTING.md), [feature mapping](docs/SOUND_DESIGN.md) and [verification](docs/VERIFICATION.md). CI and auval do not prove Logic/GarageBand playback or physical audio output.

## Licence

AGPL-3.0-only; see [LICENSE](LICENSE) and [docs/LICENSING.md](docs/LICENSING.md). JUCE and bundled dependencies retain their licences.
