# macOS acceptance checklist

Not yet performed. Record OS, architecture, Xcode, host version, commit and results for each host separately.

1. Build native and universal AUv2/Standalone/VST3; verify bundle architectures. Install AU manually and run `auval -v aumu Aelm Lvyd`.
2. GarageBand: scan instrument, insert on software-instrument track, play chords, sustain and pitch wheel. Verify audio output and note release after stop/seek.
3. Logic Pro: repeat scanning and playing; test mono/stereo, offline bounce, automation, project save/reopen and multiple instances.
4. Switch PLAY/EDIT and all character modes during held notes. Exercise on-screen keyboard and external MIDI. Check focus and resizing on Retina displays.
5. Test 44.1/48/96 kHz, different buffer sizes, device changes, panic/all-notes-off, voice stealing, and long sessions. Check CPU and audible discontinuities.
6. Test Standalone device setup and VST3 in a named VST3 host independently.

Passing CI, auval or one host does not imply all hosts passed. Signing, notarisation and installer acceptance remain separate release tasks.
