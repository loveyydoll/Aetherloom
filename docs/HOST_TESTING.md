# macOS acceptance checklist

Not yet performed. Record OS, architecture, Xcode, host version, commit and results for each host separately.

1. Build native arm64 AUv2/Standalone (optional VST3). Run `bash scripts/install-au.sh`; verify architecture, signing and auval. Intel/universal builds must fail configuration.
2. GarageBand: scan instrument, insert on software-instrument track, play chords, sustain and pitch wheel. Verify audio output and note release after stop/seek.
3. Logic Pro: repeat scanning and playing; test mono/stereo, offline bounce, automation, project save/reopen and multiple instances.
4. Switch PLAY/EDIT and all character modes during held notes. Exercise on-screen keyboard and external MIDI. Check focus and resizing on Retina displays.
5. Test 44.1/48/96 kHz, different buffer sizes, device changes, panic/all-notes-off, voice stealing, and long sessions. Check CPU and audible discontinuities.
6. Test Standalone device setup and VST3 in a named VST3 host independently.

Passing CI, auval or one host does not imply all hosts passed. Signing, notarisation and installer acceptance remain separate release tasks.

## 0.2.0 / M5 Air 16 GB, 1 TB acceptance

- Check every patch, move the scene pad at Scene amount 0 and 1, and automate X/Y, FM, Sub, Motion and Level during held chords. Compare pad and EDIT sliders.
- Save/reopen a project after modifying a factory patch, duplicate tracks, bounce offline and compare audio/state. Load a 0.1 session after a 0.2 patch; missing controls must reset to Init values. WARM saturation intentionally differs from 0.1.
- Test minimum 1/17-sample processor blocks separately from host buffers 64/128/256/512 at 44.1/48/96 kHz. Exercise high notes, sustain and >16 notes.
- Run the render benchmark in Release. Record median/p99/max, macOS/host version, hardware, sample rate, buffer size and number of instances.
- Repeat after a 30-minute music session on battery and mains. Listen for dropouts and inspect memory/CPU; passing a short benchmark is not thermal validation.
- Verify keyboard navigation, tooltips, PLAY/EDIT label visibility and Retina layout. Confirm the preset menu follows host program changes; name denotes last loaded patch, not an unedited guarantee.
