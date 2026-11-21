#!/usr/bin/env bash
# Runs the scripted play tests of the CI in four parallel lanes.
# Usage: tests/ci-play-tests.sh <executable> [full|sanitizer]
# Tests that share a screenshot folder or a user folder stay in the same lane.
# Every lane writes its output to tests/out/lane-<name>.log, which is printed at the end.
set -u
EXE="$1"
PROFILE="${2:-full}"
case "$PROFILE" in full|sanitizer) ;; *) echo "unknown profile: $PROFILE (full or sanitizer)"; exit 2 ;; esac
REPO="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO"
T=tests/run-script-test.sh
ENV42=("fullenv = 42" "storyoff = 42")
TMP="${RUNNER_TEMP:-$(mktemp -d)}"

lane_story() {
    if [ "$PROFILE" = full ]; then
        # the original never uses the new Game+ voices or background sounds; Game+ does
        REJECT="PLUS/VOICE|Background sound" $T "$EXE" tests/scripts/story-start.txt
        EXPECT="Voice: .*PLUS/VOICE/HOTEL_ANT_11" $T "$EXE" tests/scripts/story-start-plus.txt
        # Game+ background sounds follow the place (the hotel reception)
        EXPECT="Background sound: .*LOBBY" $T "$EXE" tests/scripts/story-start-plus.txt
        # the title music must keep playing with both AdLib emulations
        tests/run-music-test.sh "$EXE" "opl = nuked"
        tests/run-music-test.sh "$EXE" "opl = mame"
        # skipping the intro right away must still start the title music in the menu
        MUSIC_SCRIPT=music-skip-intro tests/run-music-test.sh "$EXE" "nointro = 0"
        # held arrow key repeats with the game's own timing
        $T "$EXE" tests/scripts/key-repeat.txt
    else
        $T "$EXE" tests/scripts/story-start.txt
        tests/run-music-test.sh "$EXE" "opl = nuked"
    fi
    SHOTDIR=tests/out/story-start-crt $T "$EXE" tests/scripts/story-start.txt "displayfilter = crt"
    # smooth edges (Scale4x) with the CRT picture
    SHOTDIR=tests/out/story-smooth $T "$EXE" tests/scripts/story-start.txt "scale2x = 1" "displayfilter = crt"
}

lane_planning() {
    REJECT="Planner: " $T "$EXE" tests/scripts/planning.txt "${ENV42[@]}"
    # Game+ planner: guard routes and view areas, timeline with undo and redo
    EXPECT="Planner: redo action" $T "$EXE" tests/scripts/planning-plus.txt "${ENV42[@]}"
    $T "$EXE" tests/scripts/walk-to.txt "${ENV42[@]}"
    # path finding must not walk through the closed shop front of the Kiosk
    EXPECT_FILE=tests/expected/walk-to-kiosk.txt $T "$EXE" tests/scripts/walk-to-kiosk.txt "${ENV42[@]}"
    # Game+: balance from PLUS/BALANCE.LUA, mode restored from the saved game
    EXPECT_FILE=tests/expected/gamemode-plus.txt $T "$EXE" tests/scripts/gamemode.txt "${ENV42[@]}" "difficulty = hard"
    # a script in MODFILES replaces the built-in one
    mkdir -p "$TMP/luamods/PLUS" && sed 's/police = 80,/police = 50,/' gamedata/PLUS/BALANCE.LUA > "$TMP/luamods/PLUS/BALANCE.LUA"
    MODFILES="$TMP/luamods" EXPECT="Balance: police 50 %" $T "$EXE" tests/scripts/gamemode.txt "${ENV42[@]}" "difficulty = hard"
    # replacement files in MODFILES are used instead of the original data
    mkdir -p "$TMP/mods/PICTURES" && cp gamedata/PICTURES/MENU "$TMP/mods/PICTURES/"
    MODFILES="$TMP/mods" EXPECT="Modfile: .*PICTURES/MENU" $T "$EXE" tests/scripts/settings.txt
    # data files edited through JSON and rebuilt by tools/clou-data.py load from MODFILES
    if [ "$PROFILE" = full ]; then tests/run-data-test.sh "$EXE"; fi
    (
        export TEST_HOME="$(mktemp -d)"
        EXPECT="Settings saved" $T "$EXE" tests/scripts/settings.txt
        EXPECT_FILE=tests/expected/settings-reload.txt $T "$EXE" tests/scripts/settings-reload.txt
    )
}

lane_burglary() {
    # original mode: results must match the reference, the difficulty must not matter
    # (and it must never start Lua)
    REJECT="Lua|Autosave" EXPECT_FILE=tests/expected/burglary-original.txt $T "$EXE" tests/scripts/burglary.txt "${ENV42[@]}" "difficulty = hard"
    # Game+: autosave in the extra fifth slot before the burglary, it loads again as Game+
    (
        export TEST_HOME="$(mktemp -d)"
        EXPECT_FILE=tests/expected/burglary-plus.txt $T "$EXE" tests/scripts/burglary-plus.txt "${ENV42[@]}"
        EXPECT="Game mode of the saved game: Game\\+" $T "$EXE" tests/scripts/burglary-plus-load.txt
    )
}

lane_weather() {
    if [ "$PROFILE" = full ]; then
        # Game+ night and weather: thunderstorm with lightning every 1.5 s, the burglary plays the same
        EXPECT="Thunder \\(close\\)" EXPECT_FILE=tests/expected/burglary-weather.txt $T "$EXE" tests/scripts/burglary-weather.txt "${ENV42[@]}" "plusweather = storm" "plusflash = 1500"
        EXPECT_FILE=tests/expected/burglary-weather.txt REJECT="Weather|Light:" SHOTDIR=tests/out/burglary-weather-off $T "$EXE" tests/scripts/burglary-weather.txt "${ENV42[@]}" "plusweather = off"
    else
        # thunderstorm with the CRT picture and smooth edges
        EXPECT_FILE=tests/expected/burglary-weather.txt $T "$EXE" tests/scripts/burglary-weather.txt "${ENV42[@]}" "plusweather = storm" "plusflash = 1500" "displayfilter = crt" "scale2x = 1"
    fi
}

mkdir -p tests/out
LANES=(story planning burglary weather)
PIDS=()
for l in "${LANES[@]}"; do
    (set -e; "lane_$l") > "tests/out/lane-$l.log" 2>&1 &
    PIDS+=($!)
done

FAILED=""
for i in "${!LANES[@]}"; do
    if ! wait "${PIDS[$i]}"; then FAILED="$FAILED ${LANES[$i]}"; fi
done

for l in "${LANES[@]}"; do
    echo "::group::lane $l"
    cat "tests/out/lane-$l.log"
    echo "::endgroup::"
done

if [ -n "$FAILED" ]; then
    for l in $FAILED; do
        echo "FAILED lane: $l"
        grep -E "^(FAIL|OK)" "tests/out/lane-$l.log" | tail -3
    done
    exit 1
fi
echo "All play test lanes passed"
