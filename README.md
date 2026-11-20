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

## Neu in dieser Version

- Erkennt automatisch Standard- oder Profidisk-Version, Diskette oder CD-ROM
- Fenster frei skalierbar, das Bild behält sein Seitenverhältnis
- **F11** oder **Alt+Enter** schaltet zwischen Fenster und Vollbild um, **F12** speichert einen Screenshot
- Joysticks können im laufenden Spiel an- und abgesteckt werden (`usejoystick = auto`), das Steuerkreuz funktioniert
- Schließen des Fensters beendet das Spiel sauber
- Browser-Version: gespeicherte Einbruchspläne bleiben nach einem Neuladen erhalten
- Linux-Build repariert

Alle Optionen stehen kommentiert in `gamedata/cosp.cfg`. Was noch zu tun ist, steht in [AUDIT.md](AUDIT.md).

## Bauen unter Linux (Debian/Ubuntu)

```
sudo apt install build-essential cmake libsdl2-dev libsdl2-image-dev
```

```
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j
```

```
DERCLOU_ROOTDIR=$PWD/gamedata/ ./build/build/x64/DerClou
```

## Spieldaten und Lizenz

Quellcode, Grafiken, Musik und die deutschen Spieldaten stehen unter der
Lizenz von neo Software (`PublicLicenceContract.txt`). Kurz gesagt: Weitergabe
und Veränderung sind erlaubt, aber **nur kostenlos und nichtkommerziell**,
der Name muss mit „Der Clou!“ beginnen, und Quellcode samt Daten müssen
öffentlich bleiben. Die englischen Spieldaten wurden von neo nie freigegeben
und sind deshalb nicht enthalten.

Der enthaltene OPL-Emulator (fmopl) stammt aus MAME und steht unter dessen
eigener, ebenfalls nichtkommerzieller Lizenz (`mame.txt`).

## Roadmap

Zusammengeführt aus der Roadmap von iljabauer/DerClou und eigenen Zielen.

1. Fertige Downloads
    - [ ] Linux, Windows, macOS als automatisch gebaute Releases
    - [ ] Browser-Version (WebAssembly)
        - [x] Spielstände bleiben erhalten
        - [x] Einbruchspläne bleiben erhalten
        - [ ] Ton direkt beim Start (Browser erlauben Ton erst nach einem Klick)
        - [ ] Kleinere Downloadgröße (Release ohne Debug-Infos, LTO)
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
    - [ ] Logdatei im Benutzerverzeichnis statt im aktuellen Ordner
5. Spiel
    - [ ] Bessere Übersicht im Planungsmodus

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
