#!/bin/bash
# Smoke test: start the game headless, wait, close the window, check the log.
# Usage: smoke-test.sh <path-to-executable-inside-game-dir> [seconds]
set -u
EXE="$1"
WAIT="${2:-10}"
DIR="$(cd "$(dirname "$EXE")" && pwd)"
LOG="$DIR/clou.log"
rm -f "$LOG"
export SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy
export HOME="$(mktemp -d)"
cd "$DIR"
"./$(basename "$EXE")" > smoke-stdout.log 2>&1 &
PID=$!
sleep "$WAIT"
if ! kill -0 "$PID" 2>/dev/null; then
    echo "FAIL: game exited/crashed during startup"; cat smoke-stdout.log; exit 1
fi
kill -TERM "$PID"   # SDL turns SIGTERM into a quit event (= closing the window)
for i in $(seq 1 10); do kill -0 "$PID" 2>/dev/null || break; sleep 1; done
if kill -0 "$PID" 2>/dev/null; then
    echo "FAIL: game did not quit after the window was closed"; kill -9 "$PID"; exit 1
fi
cat smoke-stdout.log
grep -q "Detected version" smoke-stdout.log || { echo "FAIL: version not detected"; exit 1; }
grep -q "Window closed, quitting" smoke-stdout.log || { echo "FAIL: no clean quit"; exit 1; }
rm -f smoke-stdout.log
echo "OK: smoke test passed"
