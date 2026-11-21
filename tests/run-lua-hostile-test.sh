#!/bin/bash
# Game+ scripts must not be able to stop or freeze the game. Hostile replacements of
# PLUS/BALANCE.LUA are put into MODFILES, one after the other:
#  1. an error in a metamethod while the game reads a value
#  2. an endless loop that also catches the error which ends it
#  3. a loop that eats memory
#  4. an endless loop in a metamethod, which runs while the game reads a value
#  5. an endless loop in a finalizer, where Lua runs no hooks
# Every time the game has to play tests/scripts/gamemode.txt to its end, write a "Lua error" line
# to the log, go on with its built-in balance values and never reach the panic handler of Lua.
# Usage: tests/run-lua-hostile-test.sh <executable>
set -eu
REPO="$(cd "$(dirname "$0")/.." && pwd)"
EXE="$1"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

run() {
  local name="$1" error="$2"
  local shots="$REPO/tests/out/lua-hostile-$name"
  echo "== hostile BALANCE.LUA: $name"
  # the built-in values of difficulty hard are the same as in the script that comes with the game
  MODFILES="$WORK/$name" SHOTDIR="$shots" REJECT="PANIC" EXPECT="Balance: police 80 %, dealer 85 %" \
    "$REPO/tests/run-script-test.sh" "$EXE" "$REPO/tests/scripts/gamemode.txt" \
    "fullenv = 42" "storyoff = 42" "difficulty = hard"
  grep -q "Lua error: .*$error" "$shots/game.log" \
    || { echo "FAIL: no 'Lua error: ... $error' in the log ($name)"; exit 1; }
}

mkdir -p "$WORK/metamethod/PLUS" "$WORK/loop/PLUS" "$WORK/memory/PLUS" "$WORK/metaloop/PLUS" "$WORK/finalizer/PLUS"

cat > "$WORK/metamethod/PLUS/BALANCE.LUA" <<'EOF'
balance = setmetatable({}, {__index = function() error("boom") end})
EOF
run metamethod "boom"

# the error is caught again and again; the script still has to end
cat > "$WORK/loop/PLUS/BALANCE.LUA" <<'EOF'
balance = { hard = { police = 50, dealer = 50 } }
while true do pcall(function() while true do end end) end
EOF
run loop "script took too long"

cat > "$WORK/memory/PLUS/BALANCE.LUA" <<'EOF'
balance = { hard = { police = 50, dealer = 50 } }
local hoard = {}
while true do hoard[#hoard + 1] = string.rep("x", 1000000) .. #hoard end
EOF
run memory "not enough memory"

# a script that runs away during the game switches scripting off, so it cannot slow down every call
cat > "$WORK/metaloop/PLUS/BALANCE.LUA" <<'EOF'
balance = setmetatable({}, {__index = function() while true do end end})
EOF
run metaloop "script took too long"
grep -q "Lua: scripts switched off" "$REPO/tests/out/lua-hostile-metaloop/game.log" \
  || { echo "FAIL: scripting was not switched off (metaloop)"; exit 1; }

cat > "$WORK/finalizer/PLUS/BALANCE.LUA" <<'EOF'
setmetatable({}, {__gc = function() while true do end end})
collectgarbage()
EOF
run finalizer "__gc is not available"

echo "OK: hostile scripts"
