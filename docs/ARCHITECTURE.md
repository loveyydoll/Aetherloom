# Aetherloom 0.3 architecture

Audio path: host/keyboard MIDI → optional arpeggiator → Classic or Hybrid voice pool → chorus → cross-feedback delay → reverb → smoothed level → soft output ceiling.

## Ownership and bounds

- AudioProcessorValueTreeState owns parameter/state storage. SynthParameters.h describes the 98 new controls (IDs, defaults, ranges, page and choices). The existing 16 IDs remain in the processor; hints 1/2 retain their previous ordering, new IDs use hint 3.
- Processor owns one sine table and one heap-allocated WavetableBank. Table generation happens in construction, not rendering. Each bank contains four spectral frames at eight harmonic ceilings (1 through 128), with linear phase/frame interpolation. Selection drops upper partials as frequency rises.
- Classic and Hybrid each have a pre-created pool of 16 JUCE voices. Only the selected pool renders; engine changes kill both pools. Hybrid has three oscillator sources × up to three unison lanes, two stereo TPT filters, amplitude/filter/mod envelopes, two LFOs and eight modulation slots per voice.
- ExpressionSynth records per-channel mod wheel/channel pressure before forwarding MIDI to active voices. A newly started Hybrid voice inherits the channel's expression. Poly pressure changes that note's pressure source. This is conventional MIDI expression, not MPE.
- Voice parameter pointers are cached. Continuous controls approach their targets on the 32-sample control clock; choices switch directly. Envelope samples run at audio rate. Route sums are bounded before pitch/cutoff/scan/FM/pan/level mappings. Tuning, wavetable mip selection, filter coefficients and unison panning update at control rate. Stereo filters share coefficients while keeping separate states. ADSR parameters are reapplied only when changed (or a voice starts), preserving note-off release rates.
- Effects prepare all scratch/delay storage outside rendering. Chorus uses bounded chunks, including a preallocated stereo scratch for mono layouts; larger host buffers do not require reallocation. Delay uses interpolation, smoothed time/mix/feedback, filtered cross-feedback and a four-second cap. Zero settled mixes enter a dry bypass and clear stale history once.
- JUCE owns its internal synthesis/controller locking and keyboard MIDI queue. Preallocated arp/segment MIDI buffers reserve 64 KiB each, but pathological incoming event bursts can exceed that reservation. No hard realtime/no-allocation claim is made for the complete JUCE callback.

## Arpeggiator scheduling

128 held note/channel entries preserve insertion order, velocity and key-down status; sustain latches released keys until pedal-up. Up/down patterns sort fixed-capacity indices and add octave/transpose offsets. Fractional sample counters carry across blocks; swing changes alternating durations with complementary double-precision factors. Gate produces note-offs before subsequent note-ons.

Output step markers carry the transpose modulation value into synthesis at its MIDI offset. The processor slices MIDI into each render interval: JUCE Synthesiser consumes remaining MIDI after rendering a requested range, so passing the full MIDI buffer repeatedly would trigger future notes early. Only each interval's events are supplied. Both pools use a one-sample minimum MIDI subdivision. Up to 512 step markers are retained per host block; pathological giant blocks beyond this keep generating MIDI but hold the last available modulation marker.

Transport stop/seek, engine or arp toggles, controller reset/panic and UI PANIC reset note ownership. PANIC runs on the next audio block, not on the UI thread. The first new held note starts an immediate pattern; tempo division is BPM-synchronised, not input-note PPQ quantisation. Absent host tempo defaults to 120 BPM. Synced delay times above four seconds are capped.

## State and user actions

Factories initialize all 114 controls, avoiding stale settings from the previously loaded sound. Old factory indices 0–5 retain their names/order; Hybrid sounds append at 6–17. Session XML adds schema/program/edited metadata inside JUCE's binary state envelope. Missing parameters merge from the initial state before replacement. Files use .aetherpreset and are limited to 4 MiB; file choice and file I/O run from explicit GUI actions.

Parameter listeners only update an atomic dirty flag. File callbacks hold a SafePointer to the editor. Patch changes use host-notifying parameter updates; user sliders/pad provide their parameter gestures. MUTATE limits tuning to selected musical intervals and keeps envelope, routing and effect choices. It is a timbre mutation, not an undoable whole-patch generator.

## Performance limits

Control-rate modulation and bounded oscillators are practical CPU choices. They are not equivalent to audio-rate modulation, oversampling or a CPU governor. The output ceiling protects the final signal; it does not make every internal nonlinear process alias-free. Tables/PolyBLEP reduce specific oscillator alias sources, while FM/ring/distortion/quantisation remain possible alias sources. Measure worst-case presets on the actual M5 Air under sustained load.
