#!/bin/bash
# Check the data tool (tools/clou-data.py):
#  1. every supported file in gamedata survives export + import byte for byte
#  2. an edit through JSON lands exactly where it should: the value of the loot in the safe of
#     Bulstrode Chemistry Ltd (BULCETA1.REL, relation hasLoot) and the first wait of guard 1
#     (BULCETA.GUA) are changed, rebuilt and exported again
#  3. with an executable: the game plays tests/scripts/burglary.txt (Bulstrode Chemistry Ltd)
#     with the rebuilt files in MODFILES and must load them without crashing
# Usage: tests/run-data-test.sh [executable]
# Generated files go to a temporary directory; no game data is copied into the repository.
set -eu
REPO="$(cd "$(dirname "$0")/.." && pwd)"
TOOL="$REPO/tools/clou-data.py"
PY="${PYTHON:-python3}"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

echo "== round trip of all data files"
"$PY" "$TOOL" roundtrip-all "$REPO/gamedata"

echo "== edit through JSON"
"$PY" "$TOOL" export "$REPO/gamedata/DATA/BULCETA1.REL" -o "$WORK/rel.json"
"$PY" "$TOOL" export "$REPO/gamedata/DATA/BULCETA.GUA" -o "$WORK/gua.json"
"$PY" - "$WORK" <<'EOF'
import json, sys
work = sys.argv[1]
rel = json.load(open(work + "/rel.json", encoding="utf-8"))
loot = [e for t in rel["tables"] if t.get("_name", "").startswith("hasLoot") for e in t["entries"]]
assert loot and loot[0]["param"] == 100000, loot
loot[0]["param"] = 123456
json.dump(rel, open(work + "/rel.json", "w", encoding="utf-8"), ensure_ascii=False, indent=1)
gua = json.load(open(work + "/gua.json", encoding="utf-8"))
first = gua["items"][0]
assert first["handler"] == 508500 and first["actions"][0] == {"action": "wait", "time": 180}, first
first["actions"][0]["time"] = 120
json.dump(gua, open(work + "/gua.json", "w", encoding="utf-8"), ensure_ascii=False, indent=1)
EOF
mkdir -p "$WORK/mods/DATA"
"$PY" "$TOOL" import "$WORK/rel.json" -o "$WORK/mods/DATA/BULCETA1.REL"
"$PY" "$TOOL" import "$WORK/gua.json" -o "$WORK/mods/DATA/BULCETA.GUA"
"$PY" - "$REPO" "$WORK" <<'EOF'
import sys
repo, work = sys.argv[1:]
def same_except(a, b, old, new):
    a = open(a, "rb").read()
    b = open(b, "rb").read()
    assert a.count(old) == 1 and a.replace(old, new) == b, "unexpected difference"
same_except(repo + "/gamedata/DATA/BULCETA1.REL", work + "/mods/DATA/BULCETA1.REL",
            b"\r\n100000\r\n", b"\r\n123456\r\n")
same_except(repo + "/gamedata/DATA/BULCETA.GUA", work + "/mods/DATA/BULCETA.GUA",
            b"ACLI\r\n508500\r\nACTI\r\n2\r\n180\r\n", b"ACLI\r\n508500\r\nACTI\r\n2\r\n120\r\n")
print("OK: rebuilt files differ from the originals only in the edited values")
EOF
"$PY" "$TOOL" roundtrip "$WORK/mods/DATA/BULCETA1.REL" "$WORK/mods/DATA/BULCETA.GUA"

if [ $# -ge 1 ]; then
  echo "== game loads the edited files from MODFILES"
  MODFILES="$WORK/mods" EXPECT="Modfile: .*DATA/BULCETA.GUA" SHOTDIR="${SHOTDIR:-$REPO/tests/out/data-mod}" \
    "$REPO/tests/run-script-test.sh" "$1" "$REPO/tests/scripts/burglary.txt" "fullenv = 42" "storyoff = 42"
  grep -q "Modfile: .*DATA/BULCETA1.REL" "${SHOTDIR:-$REPO/tests/out/data-mod}/game.log" \
    || { echo "FAIL: BULCETA1.REL was not loaded from MODFILES"; exit 1; }
fi
echo "OK: data tool"
