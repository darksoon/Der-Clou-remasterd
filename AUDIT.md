# Audit: Der Clou! remastered

## Stand mit Version 0.9.0

| ID | Status |
| --- | --- |
| H1 | Erledigt: Name, Version und neo-Hinweis im Startmenü, auf der Webseite und in den Paketen |
| H2 | Entfällt: neo Software existiert nicht mehr, eine Adresse für die Mail gibt es nicht |
| H3 | Erledigt: im echten Browser (Chrome) getestet, Startbildschirm mit Klick, Ton läuft, Pläne bleiben erhalten |
| H4 | Erledigt: Release `v0.9.0` |
| M1, M2, M3 | Erledigt |
| M4 | Entschärft: ungültige Instrumentnummern werden ignoriert. Ein Hörvergleich mit dem DOS-Original steht aus |
| M5 | Erledigt: AppImage für Linux, macOS mit eingebetteten Bibliotheken |
| M6, M7 | Erledigt |
| M8 | Erledigt: Skriptmodus mit zwei Testpartien (Spielbeginn, Planung), auch unter ASan und UBSan in der CI |
| N2 | Erledigt: Build ohne Warnungen bei `-Wall` |
| N8 | Erledigt: in der README beschrieben |
| N1, N3 bis N7 | Offen |

Der ursprüngliche Bericht folgt unverändert.


Stand: 20.11.2026, Commit nach „Fix Windows build: link SDL2main to provide WinMain“.

Das Projekt baut auf allen vier Plattformen (Linux, Windows, macOS, Browser) und startet unter Linux
fehlerfrei bis ins Hauptmenü. Vor einem ersten Release sollten vor allem die Lizenzpflichten erfüllt,
die Browser-Version im echten Browser getestet und ein paar Pfadprobleme behoben werden. Die größte
offene Unbekannte ist das Spiel selbst: Spielabläufe jenseits von Intro und Menü wurden noch nicht
automatisch geprüft und brauchen echte Testpartien.

## Was geprüft wurde

| Prüfung | Ergebnis |
| --- | --- |
| Build über GitHub Actions (Linux, Windows/MSYS2, macOS, Emscripten) | Alle vier erfolgreich. Pakete: Linux 61 MB, Windows 76 MB, Web 63 MB |
| Starttest unter Linux ohne Bildschirm (`scripts/smoke-test.sh`, läuft in der CI) | Start bis Hauptmenü, Versionserkennung, sauberes Beenden beim Schließen: bestanden |
| Laufzeitprüfung mit AddressSanitizer und UndefinedBehaviorSanitizer (Intro, Musik, Menü) | Keine Speicherzugriffsfehler. Undefiniertes Verhalten im Musik-Player und im OPL-Emulator (siehe M4, N3) |
| Compiler-Warnungen (gcc, `-Wall -Wextra`) | 20 Warnungen, keine davon kritisch (siehe N2) |
| Lizenztext der neo-Lizenz gegen das Programm | Pflichthinweis fehlt (siehe H1) |
| Repository-Struktur und Build-Skripte | Mehrere veraltete, nur für macOS gedachte Dateien (siehe M7) |

Nicht geprüft werden konnte: das Spiel unter Windows und macOS zur Laufzeit, die Browser-Version im
echten Browser und alle Spielabläufe nach dem Hauptmenü (neues Spiel, Planung, Einbrüche, Laden und
Speichern).

## Befunde

Priorität: **H** = vor dem ersten Release, **M** = bald, **N** = bei Gelegenheit.
Aufwand: S = unter einer Stunde, M = einige Stunden, L = Tage.

### Hoch

| ID | Bereich | Befund | Empfehlung | Aufwand |
| --- | --- | --- | --- | --- |
| H1 | Lizenz | Die neo-Lizenz verlangt, dass „basierend auf der Originalversion von neo Software“ und „Details darüber finden Sie unter www.neo.at“ deutlich lesbar nach dem Start am Bildschirm und im Installationsteil erscheinen. Das Programm zeigt das nicht. Titel und Version lauten noch „Der Clou! Open Source Project v0.8“. | Titel auf „Der Clou! remastered“ und eigene Version umstellen, Hinweis im Startmenü und in der README bzw. im Paket anzeigen. | S |
| H2 | Lizenz | Die Lizenz verlangt binnen 14 Tagen nach Fertigstellung eine Kopie aller Änderungen an office@neo.at. neo Software existiert in dieser Form seit 2006 nicht mehr (Rockstar Vienna, geschlossen). | Vor dem ersten Release die Mail trotzdem schicken und das Ergebnis dokumentieren. Optional bei Take-Two nachfragen, wer die Rechte hält. | S |
| H3 | Browser | Zwei Änderungen sind ungetestet: das dauerhafte Speichern der Einbruchspläne und das Verhalten von Ton und skalierbarem Fenster im Browser. Laut alter Roadmap startet der Ton nicht (Browser erlauben Ton erst nach einer Nutzeraktion). | Web-Paket aus der CI im Browser testen, Startbildschirm „Klicken zum Starten“ einbauen, Version über GitHub Pages veröffentlichen. | M |
| H4 | Release | Es gibt noch kein Release. Die CI erzeugt bei einem Tag `v*` automatisch eines mit allen Paketen. | Nach H1 bis H3 einen Tag `v0.9.0` setzen. | S |

### Mittel

| ID | Bereich | Befund | Empfehlung | Aufwand |
| --- | --- | --- | --- | --- |
| M1 | Start | Der Spielordner wird aus `dirname(argv[0])` bestimmt. Wird das Spiel über den PATH, einen Symlink oder eine .desktop-Datei gestartet, zeigt das auf das aktuelle Verzeichnis, und die Daten werden nicht gefunden. | `SDL_GetBasePath()` verwenden, `DERCLOU_ROOTDIR` als Vorrang behalten. | S |
| M2 | Dateien | `clou.log` landet im aktuellen Verzeichnis. In einem schreibgeschützten Installationsordner geht das nicht, und es vermüllt den Spielordner. | Logdatei in den Benutzerordner (`SDL_GetPrefPath`) schreiben, wo auch die Spielstände liegen. | S |
| M3 | Dateien | Screenshots (F12) beginnen bei jedem Start wieder bei `screenshot01.bmp` und überschreiben damit ältere. Sie landen außerdem im aktuellen Verzeichnis. | Dateiname mit Datum und Uhrzeit, Ablage im Benutzerordner, PNG statt BMP. | S |
| M4 | Musik | Im HSC-Musik-Player wird ein Instrument mit Index 207 angesprochen, obwohl es nur 128 gibt (UBSan, `mxr_hsc.c`). Gelesen werden dabei andere Daten aus der Musikdatei. Weil die Datenstruktur nicht genau dem Dateiformat entspricht, klingt das betroffene Instrument womöglich anders als im DOS-Original. | Mit dem Original (DOSBox) und mit AdPlug vergleichen, dann entweder das Originalverhalten nachbilden oder den Index begrenzen. | M |
| M5 | Pakete | Das Linux-Paket braucht SDL2 und SDL2_image aus dem System. macOS wird nur gebaut, nicht verpackt. | Linux als AppImage oder Flatpak, macOS als .app mit eingebetteten Bibliotheken. | M |
| M6 | Browser | Der Web-Build läuft mit `-g` und `-O1`, also mit Debug-Informationen und wenig Optimierung. Die Linker-Optionen `ASYNCIFY` und `LEGACY_GL_EMULATION` stehen zusätzlich bei den Compiler-Optionen und erzeugen dort Warnungen. | Release ohne `-g`, `-O2` oder `-Os` testen, Optionen sauber zwischen Compiler und Linker trennen. | S |
| M7 | Repository | `build-pc.sh` und `build-web.sh` funktionieren nur auf macOS mit Conan. `CMakeUserPresets.json` verweist auf einen nicht vorhandenen Ordner, `devbox.*` und die Conan-Profile werden nicht mehr gebraucht. | Aufräumen und den Build einheitlich über CMake beschreiben. | S |
| M8 | Tests | Außer dem Starttest gibt es keine automatischen Tests. Spielabläufe werden nicht abgedeckt. | Ein Testmodus, der vorbereitete Tastatur- und Mauseingaben abspielt, damit die CI zum Beispiel „neues Spiel, ersten Ort betreten, speichern, laden“ prüfen kann. | L |

### Niedrig

| ID | Bereich | Befund | Empfehlung | Aufwand |
| --- | --- | --- | --- | --- |
| N1 | Code | 188 Aufrufe von `strcpy`, `strcat` und `sprintf` stehen nur 3 sicheren Varianten gegenüber. Lange Pfade oder Namen könnten Puffer überlaufen lassen. | Schrittweise auf `snprintf` und längenbegrenzte Funktionen umstellen, zuerst bei Pfaden und Dateinamen. | M |
| N2 | Code | Compiler-Warnungen: 11 ungeprüfte `fread`-Rückgaben (Intro, Musik), 4 mögliche Formatüberläufe, 2 Vergleiche, die immer wahr sind (`gfx.c`), sowie je eine Warnung zu `strncpy`, einer möglicherweise nicht initialisierten Variable und einer unbenutzten Funktion. Die stichprobenhaft geprüften sind harmlos. | Bei der Gelegenheit bereinigen, Ziel: Build ohne Warnungen. | S |
| N3 | Ton | UBSan meldet im OPL-Emulator aus MAME (`fmopl.c`) Linksverschiebungen negativer Werte. In der Praxis harmlos, typischer Altcode. | Nur bei einem Austausch des Emulators angehen. | S |
| N4 | Speicher | Beim Beenden aus dem Hauptmenü meldet das Spiel 28 KB nicht freigegebenen Speicher. | Harmlos, beim Aufräumen des Menücodes mitnehmen. | S |
| N5 | Eingabe | `inpSetKeyRepeat` ist seit der Umstellung auf SDL2 leer (TODO im Code). Die Tastenwiederholung des Originals fehlt. | Prüfen, ob das beim Spielen auffällt, sonst weglassen. | S |
| N6 | Code | Kommentare gemischt deutsch und englisch, Namensreste aus der Amiga-Zeit (`us_`, `ul_`, RastPort). | Bei Änderungen nach und nach vereinheitlichen. | L |
| N7 | Planung | TODO in `planing/planer.c`: Eine Figur kann bei einer Kollision nicht die Richtung wechseln, ohne zu laufen. | Beim Spielen beobachten, ob das stört. | M |
| N8 | Daten | Frei weitergegeben werden dürfen nur die deutschen Spieldaten. Die Spracherkennung für Englisch, Französisch und Spanisch ist vorhanden, aber nicht dokumentiert. | In der README beschreiben, wie man die Dateien einer eigenen englischen Version einbindet. | S |

## Funktionen aus der Roadmap

Größere Vorhaben, die keine Fehler sind, stehen in der Roadmap in der README: Bilder mit 640 Pixeln
Breite, mehr als 256 Farben, Musik als OGG, vollständige Sprachausgabe und eine bessere Übersicht im
Planungsmodus.

## Vorgeschlagene Reihenfolge

1. H1, M1, M2, M3, M7: kleine Korrekturen, zusammen ein Abend.
2. H3 und M6: Browser-Version testen und verbessern, GitHub Pages einrichten.
3. H2, dann H4: Mail an neo, erstes Release `v0.9.0`.
4. Testpartien spielen und Fehler als Issues sammeln, parallel M4 und M5.
5. Danach M8 und die Roadmap-Funktionen.
