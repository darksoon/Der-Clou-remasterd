#!/bin/bash
# Check that unusual replacement files and an unusual environment do not crash the game:
#  1. sound files in MODFILES: a 48 kHz stereo WAV as street ambience, a WAV with a LIST chunk
#     before the sample data as lobby ambience (it must play), a replaced Game+ voice
#  2. a WAV the mixer cannot play (96 kHz) is refused with a line in the log
#  3. a truecolour PNG as MODFILES/PICTURES/MENU: the game falls back to the original picture
#  4. no audio device (unknown SDL audio driver): the game runs silent
#  5. a game directory with a very long path
# Usage: tests/run-robustness-test.sh <executable>
# The test files are generated in a temporary directory with the Python standard library.
set -eu
EXE="$1"
REPO="$(cd "$(dirname "$0")/.." && pwd)"
T="$REPO/tests/run-script-test.sh"
PY="${PYTHON:-python3}"
OUT="$REPO/tests/out/robustness"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

"$PY" - "$WORK" <<'EOF'
import math, os, struct, sys, zlib
work = sys.argv[1]

def wav(path, rate, channels, bits, seconds=1.0, extra=b""):
    """PCM sine wave; extra is put between the fmt and the data chunk"""
    frames = int(rate * seconds)
    data = bytearray()
    for i in range(frames):
        v = math.sin(i * 2 * math.pi * 440 / rate) * 0.3
        for _ in range(channels):
            data += struct.pack("<h", int(v * 32767)) if bits == 16 else struct.pack("B", int(128 + v * 127))
    block = channels * bits // 8
    fmt = struct.pack("<HHIIHH", 1, channels, rate, rate * block, block, bits)
    body = b"WAVE" + b"fmt " + struct.pack("<I", len(fmt)) + fmt + extra + b"data" + struct.pack("<I", len(data)) + data
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as f:
        f.write(b"RIFF" + struct.pack("<I", len(body)) + body)

def chunk(name, payload):
    return name + struct.pack("<I", len(payload)) + payload + (b"\0" if len(payload) & 1 else b"")

def png_rgb(path, width, height):
    """truecolour PNG, 8 bit per channel, no palette"""
    def part(name, payload):
        return struct.pack(">I", len(payload)) + name + payload + struct.pack(">I", zlib.crc32(name + payload))
    rows = b"".join(b"\0" + bytes(v for x in range(width) for v in (x & 255, y & 255, 128))
                    for y in range(height))
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n" + part(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
                + part(b"IDAT", zlib.compress(rows)) + part(b"IEND", b""))

# 1. sounds the old mixer could not take
info = chunk(b"LIST", b"INFO" + chunk(b"ISFT", b"robustness test"))  # odd payload: tests the padding
wav(work + "/sound/PLUS/AMBIENT/STREET.WAV", 48000, 2, 16)
wav(work + "/sound/PLUS/AMBIENT/LOBBY.WAV", 22050, 1, 16, extra=info)
wav(work + "/sound/PLUS/VOICE/HOTEL_ANT_11.WAV", 44100, 2, 8, extra=chunk(b"fact", struct.pack("<I", 44100)))
# 2. outside of what the mixer accepts
wav(work + "/badsound/PLUS/AMBIENT/LOBBY.WAV", 96000, 2, 16, seconds=0.25)
# 3. not a picture with a 256 colour palette
png_rgb(work + "/picture/PICTURES/MENU", 320, 200)
EOF

echo "== replaced sounds: 48 kHz stereo, LIST chunk, 8 bit stereo voice"
MODFILES="$WORK/sound" SHOTDIR="$OUT/sound" REJECT="WAV file rejected" EXPECT="Modfile: .*PLUS/AMBIENT/LOBBY.WAV" \
  "$T" "$EXE" "$REPO/tests/scripts/story-start-plus.txt"
for f in PLUS/AMBIENT/STREET.WAV PLUS/VOICE/HOTEL_ANT_11.WAV; do
  grep -q "Modfile: .*$f" "$OUT/sound/game.log" || { echo "FAIL: $f was not loaded from MODFILES"; exit 1; }
done

echo "== WAV with an unsupported sample rate"
MODFILES="$WORK/badsound" SHOTDIR="$OUT/badsound" EXPECT="WAV file rejected \\(sample rate.*LOBBY.WAV" \
  "$T" "$EXE" "$REPO/tests/scripts/story-start-plus.txt"

echo "== truecolour PNG as replacement picture"
MODFILES="$WORK/picture" SHOTDIR="$OUT/picture" EXPECT="replacement not usable, loading the original: .*PICTURES/MENU" \
  "$T" "$EXE" "$REPO/tests/scripts/settings.txt"

echo "== no audio device"
TEST_AUDIODRIVER=nonexistent SHOTDIR="$OUT/noaudio" EXPECT="No audio output" \
  "$T" "$EXE" "$REPO/tests/scripts/story-start-plus.txt"

echo "== long path of the game directory"
LONG="$WORK"
for i in 1 2 3 4 5 6; do LONG="$LONG/a_very_long_directory_name_for_the_robustness_test_$i"; done
mkdir -p "$LONG"
[ "${#LONG}" -ge 300 ] || { echo "FAIL: test path too short (${#LONG})"; exit 1; }
TEST_ROOT="$LONG" SHOTDIR="$OUT/longpath" REJECT="Path too long" \
  "$T" "$EXE" "$REPO/tests/scripts/story-start-plus.txt"

echo "OK: robustness"
