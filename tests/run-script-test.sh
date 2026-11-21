#!/bin/bash
# Play an input script headless and check that the game survives it.
# Usage: tests/run-script-test.sh <executable> <script> [extra cosp.cfg lines...]
# TEST_HOME=<dir> uses that directory as home instead of a fresh one.
# Screenshots taken by the script land in $SHOTDIR (default: tests/out/<script name>).
# EXPECT=<grep pattern> additionally requires a line in the game log.
# EXPECT_FILE=<file> requires every line of that file (except # comments) literally in the log.
set -u
EXE="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
SCRIPT="$(cd "$(dirname "$2")" && pwd)/$(basename "$2")"
shift 2
REPO="$(cd "$(dirname "$0")/.." && pwd)"
NAME="$(basename "$SCRIPT" .txt)"
SHOTDIR="${SHOTDIR:-$REPO/tests/out/$NAME}"
ROOT="$(mktemp -d)"
mkdir -p "$SHOTDIR"

# game directory: original data via symlinks plus a test configuration
for d in "$REPO"/gamedata/*/; do ln -s "$d" "$ROOT/$(basename "$d")"; done
{ echo "screenwidth = 1280"; echo "screenheight = 960"; echo "nointro = 1"; for l in "$@"; do echo "$l"; done; } > "$ROOT/cosp.cfg"

# TEST_HOME keeps the user directory between runs (settings, saved games)
export SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy HOME="${TEST_HOME:-$(mktemp -d)}"
export DERCLOU_ROOTDIR="$ROOT" DERCLOU_SCRIPT="$SCRIPT" DERCLOU_SHOTDIR="$SHOTDIR"
# fixed random seed: test games play the same every time (DERCLOU_SEED= to override)
export DERCLOU_SEED="${DERCLOU_SEED-42}"
export ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0:verify_asan_link_order=0}"

timeout -s KILL 300 "$EXE" > "$SHOTDIR/game.log" 2>&1
STATUS=$?
cat "$SHOTDIR/game.log"

if grep -q "ERROR: AddressSanitizer" "$SHOTDIR/game.log"; then echo "FAIL: AddressSanitizer error"; exit 1; fi
if ! grep -q "Script mode: end of script" "$SHOTDIR/game.log"; then echo "FAIL: script did not finish (exit $STATUS)"; exit 1; fi
if ! grep -q "Window closed, quitting" "$SHOTDIR/game.log"; then echo "FAIL: no clean quit (exit $STATUS)"; exit 1; fi
if [ -n "${EXPECT:-}" ] && ! grep -q "$EXPECT" "$SHOTDIR/game.log"; then echo "FAIL: log does not contain '$EXPECT'"; exit 1; fi
if [ -n "${EXPECT_FILE:-}" ]; then
  while IFS= read -r line; do
    case "$line" in ''|'#'*) continue ;; esac
    if ! grep -qF -- "$line" "$SHOTDIR/game.log"; then echo "FAIL: log does not contain '$line' (from $EXPECT_FILE)"; exit 1; fi
  done < "$EXPECT_FILE"
fi
echo "OK: $NAME ($(ls "$SHOTDIR"/*.png 2>/dev/null | wc -l) screenshots in $SHOTDIR)"
