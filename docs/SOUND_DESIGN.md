# Sound design in 0.3

## Starting points

| Family | Original patches | Try first |
| --- | --- | --- |
| Classic | Init, Velvet Steps, Glass Rooms, Slow Orbit, Paper Circuit, Low Lantern | Colour and WARM/BLEND/GLITCH; add the shared effects or arp |
| Keys | Copper Keys, Velvet Machine, Glass Elevator | Amp decay, filter envelope depth, oscillator-2 tuning and FM |
| Pads | Weather Archive, Orbit Choir, Magnetic Dawn | Spectral scan, unison spread, slow LFO/envelope routes and reverb |
| Bass | Basement Current, Rubber Voltage | Pulse width, envelope decay, resonance and drive |
| Leads | Neon Thread | Detune, mod-wheel pitch route and synced delay |
| Textures | Fracture Bloom | Ring modulation, independent filter types and random-to-scan |
| Sequences | Clockwork Garden, Broken Satellites | Hold a chord, change arp direction/octaves and eight step transposes |

## Build your own

1. Select Hybrid and RELOAD a starting patch. Set effects/arp off if you want to hear the raw voice.
2. Mix three oscillators at different tunings. Spectral scans four internal harmonic frames; it does not load recordings. Pulse width only changes Pulse.
3. Start with one unison voice, then add two or three and adjust detune/spread. This multiplies oscillator cost per held note.
4. Shape the amplifier and the separate filter envelope. Try filter 1 low-pass and filter 2 band-pass/high-pass; compare series and parallel with Filter blend.
5. Add a route: LFO 1 → Scan, Mod Env → Cutoff 1, Velocity → FM or Mod Wheel → Pitch. Depth has explicit units in MODULATION.md.
6. Add chorus, delay and reverb. Synced choices use host BPM; free delay changes glide and can bend pitch.
7. Explore the scene pad with Scene amount raised. It blends a defined timbre palette, not every parameter on the page.
8. SAVE your edited sound. MUTATE keeps timing/envelopes/effects while varying oscillator/filter timbre; RELOAD discards the edits and returns to the factory sound.

Alchemy-inspired ideas: a performance pad, spectral timbre controls, macros and deeper editing. ES2-inspired ideas: independently tuned oscillator mixing, FM/ring textures, dual resonant filters and a route matrix. All algorithms/preset settings here are original project code/settings; no Apple assets or recordings are included. This is not a replica or a full feature-equivalent implementation of either instrument.
