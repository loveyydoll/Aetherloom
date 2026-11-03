# Aetherloom

A character synthesizer for **Logic Pro and GarageBand on Apple Silicon**. C++20 / JUCE 8 / AUv2, **0.3.0 alpha**. Primary hardware target: MacBook Air M5, 16 GB unified memory, 1 TB SSD.

## Make a sound, then go deeper

Start with one of **18 original factory sounds**, use the four-corner scene pad and performance macros, then open EDIT when you want the machinery. Search/filter the sound browser, mutate a timbre, save your own `.aetherpreset`, or use the host's project/preset system.

**PLAY** gives you Level, Brightness, Colour, Scene amount and Warmth / Air / Movement / Space. **EDIT** has six pages: Oscillators, Filters/Amp, Motion, Mod Matrix, Effects and Arp/Steps. Controls that require Hybrid dim when Classic is selected. RELOAD restores the factory sound; PANIC clears voices, arpeggiator ownership and effect tails on the audio thread.

### Hybrid engine

- **Three independent oscillators**: sine, mipmapped triangle, PolyBLEP saw/pulse and a four-frame spectral wavetable. Each has its own semitone/cents tuning, level, spectral position and pulse width. Up to **three unison voices per oscillator**, with detune and stereo spread; at most 144 oscillator lanes across 16 polyphonic voices.
- Oscillator 2 → oscillator 1 phase/FM modulation, ring modulation, noise and a sine sub. High-note FM depth is reduced; this is not a guarantee of alias-free FM.
- **Two resonant TPT state-variable filters per stereo voice**: low-pass, band-pass, high-pass or notch; serial/parallel routing and a blend between filter outputs. Drive, keyboard tracking and a separate bipolar-depth filter envelope.
- **Three ADSRs**: amplitude, filter and freely routable modulation. Two note-retriggered LFOs offer sine, triangle, ramp, square and sample & hold; choose Hz or tempo divisions.
- **Eight modulation routes**, each with source, destination and bipolar depth. Sources include LFOs, envelopes, velocity, key, mod wheel, channel/poly pressure, per-note random and the arpeggiator's step transpose. Destinations include global/per-oscillator pitch, both cutoffs, resonance, spectral scan, FM, pan, level and scene X/Y.
- The scene pad blends original Glow / Glass / Haze / Fracture timbres: character drive, oscillator balance, sub, FM and brightness motion. Envelope, cutoff and level stay separate. This is parameter interpolation, not Alchemy's sample/spectral resynthesis.

### Clock, effects and patches

- **Arpeggiator**: Up, Down, Up/Down and As Played; one to three octaves, six divisions, gate, swing and eight editable transpose steps. MIDI offsets and fractional step timing survive block boundaries. Sustain owns held keys; panic, engine/arp changes and transport stop/seek clear ownership. The first held note starts the pattern; this is not PPQ-quantised note input.
- **Stereo effects**: chorus, interpolated ping-pong delay with filtered cross-feedback, and algorithmic reverb. Delay/LFO divisions follow host BPM; without a host tempo they use 120 BPM. Delay times are capped at four seconds, including very slow synced divisions. Space raises delay/reverb mix.
- **18 patches**: six Classic sounds plus Copper Keys, Velvet Machine, Glass Elevator, Weather Archive, Orbit Choir, Magnetic Dawn, Basement Current, Rubber Voltage, Neon Thread, Fracture Bloom, Clockwork Garden and Broken Satellites.
- MUTATE changes oscillator/filter timbre within constrained musical ranges; it keeps the envelope, effects, modulation routes and clock. SAVE/LOAD use a versioned binary state container named `.aetherpreset`; native `.aupreset`/Logic patch management remains the host's responsibility.
- **114 host-automatable parameters**. Factory names indicate the last loaded sound; the edited marker and all parameter values are saved in sessions/user presets.

**Classic** keeps the earlier WARM / BLEND / GLITCH character engine. Existing parameter IDs, AU identity and factory indices are retained. New controls use AU version hint 3; missing controls in older sessions receive neutral defaults, including Classic engine and arp off. Classic remains playable with the new shared effects and arpeggiator.

## Build and install on the M5 Air

Install Xcode and its command-line tools, Git and CMake 3.22+. JUCE 8.0.12 is pinned at `29396c22c93392d6738e021b83196283d6e4d850`.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
bash scripts/install-au.sh
```

macOS builds are **arm64 only**, with AUv2 `Aetherloom.component` and Standalone by default. Add `-DAETHERLOOM_BUILD_VST3=ON` for VST3; Intel/universal macOS configurations are rejected. Non-Apple Standalone/VST3 builds remain development checks. For an existing JUCE checkout, add `-DFETCHCONTENT_SOURCE_DIR_JUCE=/absolute/path/to/JUCE`.

Quit audio hosts before installation. The script checks architecture, ad-hoc signs the local component, copies it to `~/Library/Audio/Plug-Ins/Components/`, refreshes the current user's AU registrar and runs `auval -v aumu Aelm Lvyd`. Restart the host afterwards.

- **GarageBand:** enable Audio Units in Settings > Audio/MIDI; on a software-instrument track open Smart Controls > Plug-ins > instrument > AU Instruments > loveyydoll > Aetherloom.
- **Logic Pro:** instrument slot > AU Instruments > loveyydoll > Aetherloom; rescan in Plug-in Manager if needed.

CI packages an inner `Aetherloom-0.3.0-arm64.zip`, preserving the complete component bundle and including `install-au.sh`. Extract it, then run `bash install-au.sh` in that folder. These are ad-hoc development builds, not Developer ID signed/notarised distribution packages. AUv2 `.component` remains the integration format; a Logic `.patch` file is not the plugin binary.

## Performance and validation

Fixed 16-voice polyphony and bounded unison, cached parameter pointers, immutable sine/spectral tables, 32-sample modulation/filter control updates and smoothed continuous controls. Voices use fixed arrays; delay/chorus storage is prepared before rendering. Classic/dry effects bypass avoids hidden effect processing when mixes have settled to zero. The scene pad redraws only on changes. macOS Release builds use optimisation/LTO and the native arm64 baseline, without fast-math or speculative M5-only flags.

The spectral bank costs about 256 KiB per instance; the stereo delay reserves four seconds (~1.5 MiB at 48 kHz, ~3 MiB at 96 kHz). No sample library, render-time disk streaming, GPU worker or background synthesis thread is required. Host/JUCE MIDI queues and host scheduling still matter; this is not a hard realtime certification.

```sh
build/AetherloomBenchmark_artefacts/Release/AetherloomBenchmark
```

The benchmark reports median/p99/max block time for Classic, Hybrid, and a 16-note/three-unison/eight-route/full-effects stress configuration at 48/96 kHz. Compare with the printed 128-sample budget. Shared CI timing does not establish M5 Air performance or sustained thermal behaviour.

The suites cover oscillator/filter invariants, all patches, session/user-preset recall, legacy defaults, modulation, effects/panic, arpeggiator ownership/timing, and audio equivalence across 64- versus 8192-sample blocks. Optional editor snapshots: set `AETHERLOOM_EDITOR_SNAPSHOTS` to an absolute output folder when running AetherloomTests.

See [modulation units](docs/MODULATION.md), [architecture](docs/ARCHITECTURE.md), [sound design](docs/SOUND_DESIGN.md), [host acceptance](docs/HOST_TESTING.md) and [verification](docs/VERIFICATION.md).

## Alpha boundaries

This is a substantial synthesis engine, not full Alchemy/ES2 parity. No audio import, granular/additive sample engine, user wavetable import, vector envelope editor, MPE, oversampling, undo history or notarised installer is implemented. Spectral frames are generated internally. PolyBLEP/mipmaps reduce particular alias sources; FM, ring modulation, saturation, quantisation and waveform/band changes can still alias or click. Voice stealing and live envelope changes can click. Modulation/filter updates run every 32 samples, not at audio rate; scene interpolation is not spectral morphing.

A soft safety ceiling acts above 0.95 output amplitude. Preset/engine/arp changes can interrupt held notes. Effect time changes glide and may alter pitch. Native builds/auval and offline tests do not prove Logic/GarageBand playback, Retina interaction or M5 thermal performance; record those separately.

## Licence

AGPL-3.0-only. See [LICENSE](LICENSE) and [docs/LICENSING.md](docs/LICENSING.md). No Apple preset assets, recordings or proprietary synth code are included; JUCE/dependency licences remain applicable.
