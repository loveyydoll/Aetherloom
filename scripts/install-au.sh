#!/bin/bash
# Install a local development build for Logic Pro / GarageBand (no sudo).
set -euo pipefail
component="${1:-Aetherloom.component}"
if [[ $# -eq 0 && ! -d "$component" ]]; then
    component="build/Aetherloom_artefacts/Release/AU/Aetherloom.component"
fi
if [[ "$(uname -s)" != Darwin || "$(uname -m)" != arm64 ]]; then
    echo 'Run this from a native Terminal on an Apple Silicon Mac.' >&2
    exit 1
fi
binary="$component/Contents/MacOS/Aetherloom"
[[ -f "$binary" ]] || { echo "Missing AU binary: $binary" >&2; exit 1; }
[[ "$(lipo -archs "$binary")" == arm64 ]] || { echo 'Expected an arm64-only AU.' >&2; exit 1; }
# Ad-hoc signing is for local development, not Developer ID distribution.
codesign --force --sign - "$component"
destination="$HOME/Library/Audio/Plug-Ins/Components"
mkdir -p "$destination"
ditto "$component" "$destination/Aetherloom.component"
codesign --verify --strict "$destination/Aetherloom.component"
# AUv2 discovery is cached by a per-user service. Refresh only this user's
# registrar after installing; quit audio hosts before running this script.
killall -u "$(id -un)" AudioComponentRegistrar 2>/dev/null || true
for attempt in 1 2 3 4 5; do
    if auval -a 2>/dev/null | awk '$1 == "aumu" && $2 == "Aelm" && $3 == "Lvyd" { found = 1 } END { exit !found }'; then
        break
    fi
    sleep 1
done
# Discovery retries do not mask validation errors: this command must succeed.
auval -v aumu Aelm Lvyd
printf '%s\n' 'Installed Aetherloom.component. Restart Logic Pro / GarageBand before scanning.'
