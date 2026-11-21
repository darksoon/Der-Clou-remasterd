# Der Clou! remastered

Weiterentwicklung der Einbruchssimulation **Der Clou!** (1994) von neo Software.
Basierend auf der Originalversion von neo Software. Details darüber finden Sie unter www.neo.at.

Dieses Projekt führt die besten Teile der bisherigen Community-Ports zusammen:

| Herkunft | Was übernommen wurde |
| --- | --- |
| [Der Clou! Open Source Project (COSP) 0.8](https://sourceforge.net/projects/cosp) | SDL-Port, Sprachausgabe der CD von Festplatte, Modfiles |
| [iljabauer/DerClou](https://github.com/iljabauer/DerClou) | Basis dieses Repositories: CMake, SDL2, Browser-Version (WebAssembly), PNG-Bilder, Spielstände im Browser, viele Fehlerkorrekturen |
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
- Schwierigkeitsgrad leicht, normal (wie das Original) oder schwer
- Skriptmodus für automatische Testpartien, die bei jeder Änderung in der CI laufen, bis hin zu einem kompletten Einbruch

Alle Optionen stehen kommentiert in `gamedata/cosp.cfg`. Was noch zu tun ist, steht in [AUDIT.md](AUDIT.md).

## Tasten

| Taste | Funktion |
| --- | --- |
| F11 oder Alt+Enter | Vollbild ein und aus |
| F12 | Screenshot als PNG im Benutzerordner unter `screenshots` |
| Rechtsklick (Planung, Gehen) | Person läuft auf dem schnellsten Weg zum angeklickten Punkt |
| Tab (Einbruch) | Zeitraffer ein und aus |
| Umschalt+F12 | Screenshot des internen Bildpuffers (BMP, zur Fehlersuche) |

Der Benutzerordner ist unter Linux `~/.local/share/DerClou/`, unter Windows `%APPDATA%\DerClou\` und unter macOS
`~/Library/Application Support/DerClou/`. Dort liegen Spielstände, Pläne, Screenshots und `clou.log`.

## Bauen unter Linux (Debian/Ubuntu)

```
sudo apt install build-essential cmake libsdl2-dev libsdl2-image-dev
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
eigener, ebenfalls nichtkommerzieller Lizenz (`mame.txt`).

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
    - [x] Bilder per SDL_image laden, PNG möglich
    - [x] Frei skalierbares Fenster, Vollbild-Taste, wählbarer Filter
    - [ ] Bilder mit 640 Pixeln Breite, Basisauflösung 640 × 240
    - [ ] Mehr als 256 Farben (16 oder 32 Bit)
3. Ton und Musik
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
    - [ ] Bessere Übersicht im Planungsmodus (Sichtfelder der Wachen, Zeitleiste)
    - [ ] Werkzeug zum Lesen und Schreiben der Gebäude- und Datenbankdateien, Grundlage für neue Gebäude
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
