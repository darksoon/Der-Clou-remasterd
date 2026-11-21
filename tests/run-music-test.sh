#!/bin/bash
# Record the title music with the SDL disk audio driver and check that it keeps playing.
# Usage: tests/run-music-test.sh <executable> [extra cosp.cfg lines...]
set -u
EXE="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
shift
REPO="$(cd "$(dirname "$0")/.." && pwd)"
ROOT="$(mktemp -d)"
RAW="$ROOT/music.raw"

for d in "$REPO"/gamedata/*/; do ln -s "$d" "$ROOT/$(basename "$d")"; done
{ echo "nointro = 1"; for l in "$@"; do echo "$l"; done; } > "$ROOT/cosp.cfg"

export SDL_VIDEODRIVER=dummy SDL_AUDIO_DRIVER=disk SDL_AUDIO_DISK_OUTPUT_FILE="$RAW" HOME="$(mktemp -d)"
export DERCLOU_ROOTDIR="$ROOT" DERCLOU_SCRIPT="$REPO/tests/scripts/music.txt" DERCLOU_SHOTDIR="$ROOT"
export ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0:verify_asan_link_order=0}"

timeout -s KILL 180 "$EXE" > "$ROOT/game.log" 2>&1
grep "AdLib emulation" "$ROOT/game.log"
if grep -q "ERROR: AddressSanitizer" "$ROOT/game.log"; then cat "$ROOT/game.log"; echo "FAIL: AddressSanitizer error"; exit 1; fi
if [ ! -s "$RAW" ]; then cat "$ROOT/game.log"; echo "FAIL: no audio recorded"; exit 1; fi

# 16 bit stereo, 44100 Hz: every second from 8 to 13 must carry music
python3 - "$RAW" <<'PY'
import array, math, sys
a = array.array('h')
with open(sys.argv[1], 'rb') as f:
    data = f.read()
a.frombytes(data[:len(data) // 2 * 2])
left = a[::2]
rate = 44100
levels = [int(math.sqrt(sum(x * x for x in left[s * rate:(s + 1) * rate]) / rate)) for s in range(len(left) // rate)]
print("level per second:", levels)
if len(levels) < 14:
    sys.exit("FAIL: recording too short")
quiet = [s for s in range(8, 14) if levels[s] < 300]
if quiet:
    sys.exit("FAIL: music is silent in seconds %s" % quiet)
print("OK: music")
PY
