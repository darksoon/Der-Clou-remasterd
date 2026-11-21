# Entwicklung

Bauen, Tests und Werkzeuge von Der Clou! remastered.

## Bauen unter Linux (Debian/Ubuntu)

Das Spiel nutzt SDL 3 (mindestens 3.4.20). Ist keine passende Version installiert, lädt CMake genau diese
Version (mit Prüfsumme) und baut sie mit. Dafür braucht SDL die Entwicklerdateien seiner Linux-Treiber:

```
sudo apt install build-essential cmake ninja-build libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxfixes-dev libxi-dev libxss-dev libxtst-dev libxkbcommon-dev libdrm-dev libgbm-dev libgl1-mesa-dev libgles2-mesa-dev libegl1-mesa-dev libdbus-1-dev libibus-1.0-dev libudev-dev libwayland-dev wayland-protocols libdecor-0-dev libasound2-dev libpulse-dev libpipewire-0.3-dev
```

```
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j
```

```
./build/build/x64/DerClou
```

Das Spiel sucht seine Daten zuerst in `DERCLOU_ROOTDIR`, dann neben der ausführbaren Datei. Die
Browser-Version baut man mit Emscripten:

```
emcmake cmake -B build-web -DCMAKE_BUILD_TYPE=Release && cmake --build build-web -j
```

Das Ergebnis liegt in `build-web/build/web/` und braucht einen Webserver, zum Beispiel:

```
python3 -m http.server -d build-web/build/web 8000
```

## Versionen

Die Versionsnummer steht in der Datei `VERSION`. Beim Bauen ergänzt CMake sie über `git describe` um den
Commit, so dass jedes Paket genau zuzuordnen ist. Für ein neues Release genügt es, die Nummer in `VERSION`
zu erhöhen und einen Abschnitt in `CHANGELOG.md` zu ergänzen: Sobald das auf `main` landet, legt die CI den
Tag an und veröffentlicht ein Release mit allen Paketen und dem Changelog-Abschnitt als Beschreibung.
Ein von Hand gepushter Tag `v*` funktioniert ebenso. Jeder Push auf `main` aktualisiert die Webseite.

## Skriptmodus und Tests

Das Spiel kann vorbereitete Eingaben abspielen. Ein Skript ist eine Textdatei mit einem Befehl pro Zeile:

| Befehl | Wirkung |
| --- | --- |
| `wait 500` | 500 Millisekunden warten |
| `key Return` | Taste drücken (SDL-Tastenname) |
| `keydown Down`, `keyup Down` | Taste halten und loslassen (Tastenwiederholung) |
| `click 160 100`, `rclick 160 100` | Linke oder rechte Maustaste, Koordinaten im Spielbild 320 × 200 |
| `move 160 100` | Maus bewegen |
| `shot name` | Screenshot `name.png` im Screenshot-Ordner |
| `log Text` | Text ins Log schreiben |
| `quit` | Spiel beenden |

Gestartet wird mit `--script datei.txt` und optional `--shotdir ordner` (oder über die Umgebungsvariablen
`DERCLOU_SCRIPT` und `DERCLOU_SHOTDIR`). Die Testpartien in `tests/scripts/` laufen ohne Bildschirm:

```
tests/run-script-test.sh build/build/x64/DerClou tests/scripts/story-start.txt
```

```
tests/run-script-test.sh build/build/x64/DerClou tests/scripts/planning.txt "fullenv = 42" "storyoff = 42"
```

Die CI führt sie bei jedem Push normal und mit AddressSanitizer und UndefinedBehaviorSanitizer aus und
lädt die Screenshots als Artefakt hoch.

## Datenwerkzeug

`tools/clou-data.py` liest und schreibt die Daten des Spiels verlustfrei: die Objektdatenbank
(`*.DAT`), die Relationen (`*.REL`, `*.GRE`), die Gebäude mit Bodenraster (`*.LFD`), Scheinwerfern
(`*.SPO`), Wachenrouten (`*.GUA`) und Plänen (`*.PLN`, `*.PLL`), die Handlung (`TCSTORY.PC`), die
verschlüsselten Texte (`TEXTS/*.TXT`) und die Listen (`*.LST`). Es braucht nur Python 3 ohne
weitere Pakete. Die Formate stehen in [DATEIFORMATE.md](DATEIFORMATE.md).

```
python3 tools/clou-data.py info gamedata/DATA/BULCETA0.DAT
python3 tools/clou-data.py export gamedata/DATA/BULCETA.GUA -o wachen.json
python3 tools/clou-data.py import wachen.json -o MODFILES/DATA/BULCETA.GUA
python3 tools/clou-data.py roundtrip-all gamedata
python3 tools/clou-data.py building gamedata Bulstrode
```

Das JSON enthält Objektnamen, Typen, Relationsnamen und Hinweise wie die Art eines Objekts im
Grundriss. Alle Schlüssel mit `_` am Anfang sind nur Hinweise und werden beim Import übergangen.
Export und Import ergeben die Originaldatei Byte für Byte, das prüft `tests/run-data-test.sh` für
alle unterstützten Dateien in `gamedata`. Eine geänderte Datei legt man unter `MODFILES/` ab, die
Originaldaten bleiben unberührt. `building` ohne Namen listet alle Gebäude, mit Namen zeigt es alle
Dateien und Angaben eines Gebäudes.

## Webseite

Die CI veröffentlicht `web/site/` zusammen mit der Browser-Version unter `play/` in den Branch `gh-pages`.
Die Testpartien laufen nur, wenn sich am Spiel, an den Daten, den Tests oder der CI etwas geändert hat.
