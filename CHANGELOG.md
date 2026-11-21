# Änderungen

Die Versionsnummer folgt [Semantic Versioning](https://semver.org/lang/de/). Bis zur 1.0 kann sich das
Verhalten zwischen Versionen noch ändern.

## 0.12.1 (21.11.2026)

### Behoben

- Ersatzdateien im Ordner `MODFILES` wurden bei Bildern ignoriert, wenn SDL_image das ILBM-Format lesen
  konnte, und ihre Dateigröße wurde vom Original genommen. Jetzt werden sie für Bilder und alle anderen
  Daten verwendet, Ersatzbilder dürfen auch PNG mit Palette sein. Das Log nennt jede benutzte Ersatzdatei

## 0.12.0 (21.11.2026)

### Neu

- Spielarten „Original“ und „Spiel+“: Nach „Neues Spiel starten“ fragt das Spiel, wie gespielt werden soll.
  Die Wahl steht im Spielstand. Spielstände älterer Versionen laden als „Original“, und ältere Versionen
  können neue Spielstände weiterhin laden
- „Original“ spielt wie 1994 und bekommt nur Komfortfunktionen: Laufen per Rechtsklick, Zeitraffer,
  Polizei-Countdown und das Einstellungsmenü. Sie ändern keine Spielwerte, die Wegsuche plant genau die Zeit
  des kürzesten Wegs ein, den man auch von Hand gehen könnte
- „Spiel+“ enthält alles, was das Spiel oder seine Balance verändert. Bisher ist das der Schwierigkeitsgrad:
  Er wirkt nur noch in Spiel+, im Originalmodus gilt immer „normal“
- Testpartien laufen mit festem Zufallsstartwert (`DERCLOU_SEED`) und sind damit wiederholbar. Die
  Einbruchspartie im Originalmodus muss genau die Werte aus `tests/expected/burglary-original.txt` liefern,
  auch wenn `difficulty = hard` gesetzt ist. Eine weitere Testpartie speichert ein Spiel in Spiel+ und lädt
  es wieder

## 0.11.0 (21.11.2026)

### Neu

- Einstellungsmenü im Hauptmenü („Einstellungen“) im Stil der Auswahlblasen des Spiels: Lautstärke von Musik,
  Geräuschen und Sprachausgabe (nur CD-ROM) in zehn Stufen, Vollbild, Skalierfilter, ganze Pixel,
  Schwierigkeit, Polizei-Countdown, Zeitraffer-Tempo und Gamepad. Jede Änderung wirkt sofort. Bedienung mit
  Maus, Tastatur oder Gamepad: Klick schaltet weiter, links und rechts verringern oder erhöhen
- Die Auswahl wird als `settings.cfg` im Benutzerordner gespeichert und nach `cosp.cfg` gelesen, in der
  Browser-Version bleibt sie über IndexedDB erhalten
- Zeitraffer-Tempo einstellbar (`fastforward = 2 | 4 | 8`, Standard 4)
- Testpartien für das Einstellungsmenü und das erneute Laden der gespeicherten Einstellungen

### Behoben

- Die Logdatei nannte immer den Schwierigkeitsgrad „normal“, weil sie vor dem Lesen der Konfiguration
  geschrieben wurde
- Browser-Version: Der Benutzerordner wird vollständig aus IndexedDB geladen, bevor das Spiel ihn liest

## 0.10.0 (21.11.2026)

### Neu

- Planung: Im Gehen-Modus läuft die Person per Rechtsklick auf den schnellsten Weg zum angeklickten Punkt
  (Wegsuche über die Kollisionskarte). Das spart beim Planen hunderte Tastendrücke, die geplante Zeit
  entspricht genau dem kürzesten Weg. Ein Hinweis unter der Frage „Wohin soll ich gehen?“ erklärt die Bedienung
- Einbruch: Tab schaltet den Zeitraffer ein und aus (vierfache Geschwindigkeit)
- Einbruch: Sobald der Alarm laut ist, zeigt die Statuszeile, wie lange es noch bis zur Polizei dauert
  (`policetimer = 0` blendet das aus)
- Schwierigkeitsgrad `difficulty = easy | normal | hard` in `cosp.cfg`. Normal entspricht genau dem Original.
  Leicht: Die Polizei braucht nach einem Alarm 25 % länger, Hehler zahlen 15 % mehr. Schwer: Die Polizei
  ist 20 % schneller, Hehler zahlen 15 % weniger
- Neuer automatischer Test, der einen kompletten Einbruch plant, speichert, ausführt und auswertet

### Behoben

- Der Vorsprung bei der Flucht wurde ohne Alarm mit einem Überlauf berechnet (undefiniertes Verhalten)

## 0.9.0 (20.11.2026)

Erste Version von Der Clou! remastered. Grundlage ist iljabauer/DerClou, ergänzt um die Verbesserungen aus
vrza/derclou und eigene Arbeit.

### Neu

- Name „Der Clou! remastered“, eigene Versionsnummer aus `VERSION` und `git describe`
- Lizenzhinweis von neo Software im Startmenü, auf der Webseite und in jedem Paket
- Versionserkennung für Standard- und Profidisk, Diskette und CD-ROM (aus vrza/derclou)
- Frei skalierbares Fenster mit festem Seitenverhältnis, Optionen `resizable`, `scalequality` und `integerscale`
- Vollbild mit F11 oder Alt+Enter, Screenshots mit F12 als PNG mit Datum im Namen
- Joystick-Hotplug, Steuerkreuz, automatische Auswahl mit `usejoystick = auto`
- Skriptmodus (`--script`, `--shotdir`) für automatische Testpartien
- Browser-Version mit eigener Startseite, Fortschrittsanzeige und Ton nach dem ersten Klick
- Projektseite auf GitHub Pages mit spielbarer Browser-Version, optional auch auf Vercel
- Pakete: Linux als AppImage und tar.gz, Windows als zip mit allen DLLs, macOS als zip mit eingebetteten
  Bibliotheken, Browser als zip
- Fenstersymbol und Programmsymbol

### Behoben

- Schließen des Fensters und SIGTERM beenden das Spiel sauber, statt ignoriert zu werden
- Spieldaten werden über `SDL_GetBasePath()` gefunden, auch beim Start über PATH, Symlink oder Startmenü
- `clou.log` landet im Benutzerordner statt im aktuellen Verzeichnis
- Browser-Version findet ihre Spieldaten, Einbruchspläne bleiben nach einem Neuladen erhalten
- Titelmusik: Instrumentnummern außerhalb des gültigen Bereichs in HSC-Dateien werden ignoriert statt
  fremden Speicher zu lesen
- Ungeprüfte Lesezugriffe im Intro- und Musikcode, Build ohne Compiler-Warnungen bei `-Wall`
- Linux- und Windows-Build (libm, SDL2main)

### Technik

- CI für Linux, Windows, macOS und Browser mit Starttest, Testpartien und Sanitizer-Lauf
- Browser-Build mit `-O2` ohne Debug-Informationen, Compiler- und Linker-Optionen getrennt
- Veraltete Conan-, Devbox- und macOS-Buildskripte entfernt
