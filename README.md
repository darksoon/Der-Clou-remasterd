# Der Clou! remastered

Weiterentwicklung der Einbruchssimulation **Der Clou!** (1994) von neo Software.
Basierend auf der Originalversion von neo Software. Details darüber finden Sie unter www.neo.at.

Dieses Projekt führt die besten Teile der bisherigen Community-Ports zusammen:

| Herkunft | Was übernommen wurde |
| --- | --- |
| [Der Clou! Open Source Project (COSP) 0.8](https://sourceforge.net/projects/cosp) | SDL-Port, Sprachausgabe der CD von Festplatte, Modfiles |
| [iljabauer/DerClou](https://github.com/iljabauer/DerClou) | Basis dieses Repositories: CMake, SDL2-Port, Browser-Version (WebAssembly), PNG-Bilder, Spielstände im Browser, viele Fehlerkorrekturen |
| [vrza/derclou](https://github.com/vrza/derclou) | Versionserkennung, frei skalierbares Fenster, Vollbild-Taste, Joystick-Hotplug, Steuerkreuz |
| [vcosta/derclou](https://github.com/vcosta/derclou), [midzer/derclou](https://github.com/midzer/derclou) | Ursprünglicher SDL2-Port und Emscripten-Experimente, als Referenz |

**Direkt im Browser spielen:** https://darksoon.github.io/Der-Clou-remasterd/play/
**Downloads für Windows, Linux und macOS:** [Releases](https://github.com/darksoon/Der-Clou-remasterd/releases)

## Neu in dieser Version

- Eigener Name und eigene Versionsnummer (siehe `VERSION` und [CHANGELOG.md](CHANGELOG.md)), Lizenzhinweis von neo im Startmenü
- Erkennt automatisch Standard- oder Profidisk-Version, Diskette oder CD-ROM
- Fenster frei skalierbar, das Bild behält sein Seitenverhältnis, wählbarer Filter und ganzzahlige Skalierung
- Joysticks können im laufenden Spiel an- und abgesteckt werden (`usejoystick = auto`), das Steuerkreuz funktioniert
- Schließen des Fensters beendet das Spiel sauber
- Logdatei und Screenshots liegen im Benutzerordner, nicht mehr im Spielordner
- Browser-Version mit Startseite, Ton nach dem ersten Klick, Einbruchspläne bleiben nach einem Neuladen erhalten
- Linux als AppImage, macOS mit eingebetteten Bibliotheken, Windows mit allen DLLs
- Planung per Mausklick: Rechtsklick im Gehen-Modus, die Person nimmt den schnellsten Weg
- Zeitraffer beim Einbruch (Tab) und Polizei-Countdown nach einem lauten Alarm
- Zwei Spielarten, gewählt beim Start eines neuen Spiels und im Spielstand gespeichert: „Original“ spielt wie 1994 und bekommt nur Komfortfunktionen, „Spiel+“ enthält alles, was das Spiel oder seine Balance verändert
- Schwierigkeitsgrad leicht, normal oder schwer (nur in Spiel+), die Werte stehen im Lua-Skript `PLUS/BALANCE.LUA`
- Bildfilter „Scanlines“ und „Röhrenmonitor“, auch im Browser und ohne Mehraufwand gegenüber dem normalen Bild
- AdLib-Musik wahlweise über Nuked OPL3 (klingt wie der echte Chip, Standard) oder die bisherige MAME-Emulation, umschaltbar im laufenden Spiel
- Lua 5.5 als Skriptsprache für Spiel+, abgeschottet ohne Datei- und Systemzugriff. Der Originalmodus startet nie ein Skript
- Einstellungsmenü im Hauptmenü: Lautstärken, Vollbild, Skalierung, Schwierigkeit, Polizei-Countdown, Zeitraffer und Gamepad, gespeichert im Benutzerordner (auch im Browser)
- Spiel+: Die Planung zeigt Laufwege und Sichtbereiche der Wachen im Grundriss und eine Zeitleiste aller Einbrecher mit Rückgängig und Wiederholen
- Skriptmodus für automatische Testpartien, die bei jeder Änderung in der CI laufen, bis hin zu einem kompletten Einbruch

Die wichtigsten Optionen lassen sich im Hauptmenü unter „Einstellungen“ ändern. Dort gilt: Klick oder Eingabetaste schaltet weiter, die Pfeiltasten links und rechts verringern oder erhöhen, Rechtsklick oder Escape führt zurück. Die Auswahl landet als `settings.cfg` im Benutzerordner und wird nach `cosp.cfg` gelesen. Alle Optionen stehen kommentiert in `gamedata/cosp.cfg`.

Eigene Dateien ersetzen Originaldaten, ohne diese zu verändern: Eine Datei unter `MODFILES/` im Spielordner
wird statt der gleichnamigen Originaldatei geladen, zum Beispiel `MODFILES/PICTURES/MENU` statt
`PICTURES/MENU`. Ordner- und Dateinamen werden großgeschrieben. Bilder dürfen statt ILBM auch PNG mit
256-Farben-Palette sein, Name, Größe und Farbbereich müssen dem Original entsprechen. So lassen sich auch
die Lua-Skripte von Spiel+ anpassen, zum Beispiel die Werte des Schwierigkeitsgrads in
`MODFILES/PLUS/BALANCE.LUA`. Fehler in einem Skript stehen in `clou.log`, das Spiel nimmt dann die
eingebauten Werte. Was noch zu tun ist, steht in [AUDIT.md](AUDIT.md).

## Tasten

| Taste | Funktion |
| --- | --- |
| F11 oder Alt+Enter | Vollbild ein und aus |
| F12 | Screenshot als PNG im Benutzerordner unter `screenshots` |
| Rechtsklick (Planung, Gehen) | Person läuft auf dem schnellsten Weg zum angeklickten Punkt |
| Tab (Einbruch) | Zeitraffer ein und aus |
| W (Planung, Spiel+) | Laufwege und Sichtbereiche der Wachen ein und aus (in Spiel+ zu Beginn an) |
| T (Planungsmenüs, Spiel+) | Zeitleiste des Plans. Dort macht Z (oder die Rücktaste) die letzte Aktion der aktuellen Person rückgängig, Y stellt sie wieder her, T, Esc oder ein Klick schließt |
| Umschalt+F12 | Screenshot des internen Bildpuffers (BMP, zur Fehlersuche) |

Der Benutzerordner ist unter Linux `~/.local/share/DerClou/`, unter Windows `%APPDATA%\DerClou\` und unter macOS
`~/Library/Application Support/DerClou/`. Dort liegen Spielstände, Pläne, Screenshots, `settings.cfg` und `clou.log`.

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
weitere Pakete. Die Formate stehen in [docs/DATEIFORMATE.md](docs/DATEIFORMATE.md).

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

## Webseite und Vercel

Die CI veröffentlicht `web/site/` zusammen mit der Browser-Version unter `play/` in den Branch `gh-pages`.
Falls GitHub Pages noch nicht aktiv ist: in den Repository-Einstellungen unter Pages als Quelle den Branch
`gh-pages` wählen. Wird zusätzlich ein Secret `VERCEL_TOKEN` angelegt, veröffentlicht die CI dieselbe Seite
auch auf Vercel (Projekt `der-clou-remastered`).

## Spieldaten und Lizenz

Quellcode, Grafiken, Musik und die deutschen Spieldaten stehen unter der
Lizenz von neo Software (`PublicLicenceContract.txt`). Kurz gesagt: Weitergabe
und Veränderung sind erlaubt, aber **nur kostenlos und nichtkommerziell**,
der Name muss mit „Der Clou!“ beginnen, und Quellcode samt Daten müssen
öffentlich bleiben. Die englischen Spieldaten wurden von neo nie freigegeben
und sind deshalb nicht enthalten.

Wer eine eigene englische (oder französische, spanische) Version besitzt, kann deren Daten verwenden:
Den Inhalt der Ordner `TEXTS`, `DATA`, `PICTURES` und, falls vorhanden, `SAMPLES` der eigenen Version in
einen eigenen Spielordner kopieren und das Spiel mit `DERCLOU_ROOTDIR` auf diesen Ordner zeigen lassen.
Die Sprache wird an der Datei `TEXTS/TCMAINE?.TXT` erkannt (`E` Englisch, `D` Deutsch, `F` Französisch,
`S` Spanisch), Englisch hat Vorrang.

Der enthaltene OPL-Emulator (fmopl) stammt aus MAME und steht unter dessen
eigener, ebenfalls nichtkommerzieller Lizenz (`mame.txt`). Nuked OPL3 von Nuke.YKT liegt unverändert in
`src/sound/nuked/` und steht unter der GNU LGPL 2.1 oder neuer (`LICENSE-Nuked-OPL3.txt` in jedem Paket).

## Roadmap

Zusammengeführt aus der Roadmap von iljabauer/DerClou und eigenen Zielen.

1. Fertige Downloads
    - [x] Linux, Windows, macOS als automatisch gebaute Releases
    - [x] Browser-Version (WebAssembly)
        - [x] Spielstände bleiben erhalten
        - [x] Einbruchspläne bleiben erhalten
        - [x] Ton nach dem ersten Klick
        - [x] Kleinere Downloadgröße (Release ohne Debug-Infos)
2. Grafik
    - [x] Bilder ohne Zusatzbibliothek laden (ILBM eingebaut, PNG über SDL 3)
    - [x] Umstieg auf SDL 3
    - [x] Frei skalierbares Fenster, Vollbild-Taste, wählbarer Filter
    - [x] Bildfilter Scanlines und Röhrenmonitor
    - [ ] Bilder mit 640 Pixeln Breite, Basisauflösung 640 × 240
    - [ ] Mehr als 256 Farben (16 oder 32 Bit)
3. Ton und Musik
    - [x] Genauere AdLib-Emulation (Nuked OPL3), alte Emulation wählbar
    - [ ] Musik als OGG oder ähnlich
    - [ ] Vollständige Sprachausgabe aller Dialoge (Deutsch, Englisch)
4. Code
    - [ ] Verständlichere Namen für Funktionen und Variablen
    - [ ] Lesbarerer Code, Kommentare auf Englisch
    - [x] Logdatei im Benutzerverzeichnis statt im aktuellen Ordner
    - [x] Automatische Testpartien in der CI
5. Spiel
    - [x] Laufen per Mausklick mit Wegsuche im Planungsmodus
    - [x] Zeitraffer beim Einbruch, Polizei-Countdown
    - [x] Schwierigkeitsgrad
    - [x] Einstellungsmenü im Hauptmenü
    - [x] Spielarten „Original“ und „Spiel+“, Testpartie mit Referenzwerten sichert den Originalmodus ab
    - [ ] Bessere Übersicht im Planungsmodus (Sichtfelder der Wachen, Zeitleiste)
    - [x] Werkzeug zum Lesen und Schreiben der Gebäude- und Datenbankdateien, Grundlage für neue Gebäude
    - [x] Lua 5.5 für Spiel+ (erster Einsatz: Werte des Schwierigkeitsgrads)
    - [ ] Erweiterter Modus mit neuen Einbrüchen

## CREDITS
```
Idea & concept:  ...and avoid panic by
Programming:     Helmut Gaberschek, Kaweh Kazemi
Graphics:        Markus Hudolin
Story:           Karam Nada
Sound & music:   Hannes Seifert

SDL conversion:  Oliver Gantert (LucyG)
Contributions:   Baris Elmas (devkid82)
Ronny Thiemann (templer)
Thomas Trummer (LordSkaven)
Playtesting:     Baris Elmas (devkid82) - first to finish German version
Martin Banas (srandista) - first to finish Czech version
Neno - bug reporter
MvGulik - Linux+Wine bug reporter

IFF ANIM decoder C++ class by Markus Wolf (murkymind.de)
Ported to C by Oliver Gantert.

HSC music player by Thomas Trummer
The fmopl YM3812 emulator from MAME is under a no distribution for profit
license contained in the included 'mame.txt' file.
```
