#!/bin/bash
# Play an input script headless and check that the game survives it.
# Usage: tests/run-script-test.sh <executable> <script> [extra cosp.cfg lines...]
# MODFILES=<dir> provides replacement files (MODFILES directory).
# TEST_HOME=<dir> uses that directory as home instead of a fresh one.
# TEST_ROOT=<dir> uses that (empty) directory as game directory instead of a fresh one.
# TEST_AUDIODRIVER=<name> sets the SDL audio driver (default: dummy).
# Screenshots taken by the script land in $SHOTDIR (default: tests/out/<script name>).
# EXPECT=<extended grep pattern> additionally requires a line in the game log.
# REJECT=<grep pattern> fails the test if the game log contains it.
# EXPECT_FILE=<file> requires every line of that file (except # comments) literally in the log.
set -u
EXE="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
SCRIPT="$(cd "$(dirname "$2")" && pwd)/$(basename "$2")"
shift 2
REPO="$(cd "$(dirname "$0")/.." && pwd)"
NAME="$(basename "$SCRIPT" .txt)"
SHOTDIR="${SHOTDIR:-$REPO/tests/out/$NAME}"
ROOT="${TEST_ROOT:-$(mktemp -d)}"
mkdir -p "$SHOTDIR"

# game directory: original data via symlinks plus a test configuration
for d in "$REPO"/gamedata/*/; do ln -s "$d" "$ROOT/$(basename "$d")"; done
# MODFILES=<dir>: replacement files, used as the MODFILES directory of the game (names in upper case)
if [ -n "${MODFILES:-}" ]; then ln -s "$(cd "$MODFILES" && pwd)" "$ROOT/MODFILES"; fi
{ echo "screenwidth = 1280"; echo "screenheight = 960"; echo "nointro = 1"; for l in "$@"; do echo "$l"; done; } > "$ROOT/cosp.cfg"

# TEST_HOME keeps the user directory between runs (settings, saved games)
export SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER="${TEST_AUDIODRIVER:-dummy}" HOME="${TEST_HOME:-$(mktemp -d)}"
export DERCLOU_ROOTDIR="$ROOT" DERCLOU_SCRIPT="$SCRIPT" DERCLOU_SHOTDIR="$SHOTDIR"
# fixed random seed: test games play the same every time (DERCLOU_SEED= to override)
export DERCLOU_SEED="${DERCLOU_SEED-42}"
export ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0:verify_asan_link_order=0}"

timeout -s KILL 300 "$EXE" > "$SHOTDIR/game.log" 2>&1
STATUS=$?
cat "$SHOTDIR/game.log"

if grep -q "ERROR: AddressSanitizer" "$SHOTDIR/game.log"; then echo "FAIL: AddressSanitizer error"; exit 1; fi
if grep -q "runtime error:" "$SHOTDIR/game.log"; then echo "FAIL: undefined behaviour (UBSan)"; grep -m5 "runtime error:" "$SHOTDIR/game.log"; exit 1; fi
if ! grep -q "Script mode: end of script" "$SHOTDIR/game.log"; then echo "FAIL: script did not finish (exit $STATUS)"; exit 1; fi
if ! grep -q "Window closed, quitting" "$SHOTDIR/game.log"; then echo "FAIL: no clean quit (exit $STATUS)"; exit 1; fi
if [ -n "${EXPECT:-}" ] && ! grep -qE "$EXPECT" "$SHOTDIR/game.log"; then echo "FAIL: log does not contain '$EXPECT'"; exit 1; fi
if [ -n "${REJECT:-}" ] && grep -qE "$REJECT" "$SHOTDIR/game.log"; then echo "FAIL: log contains '$REJECT'"; exit 1; fi
if [ -n "${EXPECT_FILE:-}" ]; then
  while IFS= read -r line; do
    case "$line" in ''|'#'*) continue ;; esac
    if ! grep -qF -- "$line" "$SHOTDIR/game.log"; then echo "FAIL: log does not contain '$line' (from $EXPECT_FILE)"; exit 1; fi
  done < "$EXPECT_FILE"
fi
echo "OK: $NAME ($(ls "$SHOTDIR"/*.png 2>/dev/null | wc -l) screenshots in $SHOTDIR)"
