# Licensing decision — 2026-10-22

New Aetherloom source is licensed under AGPL-3.0-only, copyright 2026 loveyydoll. The full licence is in LICENSE. This decision uses JUCE's open-source route, with no paid/commercial agreement accepted on the user's behalf.

The requested historical GPLv3 intention was checked against the referenced chat. The available history exposes the user's sound/interface goals, but assistant responses are unresolved content-reference markers; the historical licensing wording cannot be verified. No existing project code has been imported or relicensed.

Pinned JUCE 8.0.12 explicitly dual-licenses its framework modules under AGPLv3 or its commercial JUCE licence. Consequently this new project uses AGPLv3 rather than labelling the combined application GPLv3-only. GPL and AGPL are different licences; GPL-only must not be presented as a substitute for JUCE's AGPL obligations. Revisiting GPL-only would require a separately reviewed dependency/licensing strategy.

Primary references checked at initialization:
- https://github.com/juce-framework/JUCE/blob/29396c22c93392d6738e021b83196283d6e4d850/LICENSE.md
- https://juce.com/legal/juce-8-licence/
- https://www.gnu.org/licenses/agpl-3.0.html

Redistribution must include applicable copyright/licence notices and satisfy corresponding-source obligations. Review JUCE's bundled dependency notices, including AU SDK and VST3 SDK, when packaging binaries. CI artifacts are development builds, not signed public releases. There is no existing contributor code to relicense here; future contributions should use the project licence.
