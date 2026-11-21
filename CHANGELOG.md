# Änderungen

Die Versionsnummer folgt [Semantic Versioning](https://semver.org/lang/de/). Bis zur 1.0 kann sich das
Verhalten zwischen Versionen noch ändern.

## 0.21.0 (21.11.2026)

### Neu

- Spiel+: Der Einbruch spielt bei Nacht. Die Räume sind dunkel, jeder Einbrecher leuchtet mit einer
  Taschenlampe in Blickrichtung, die Wachen tragen eine Laterne, die Scheinwerfer des Gebäudes
  strahlen, durch die Fenster fällt kaltes Mondlicht schräg in die Räume, und die Schatten der Mauern
  sind tiefer, so dass die Wände Höhe bekommen. Licht scheint nicht durch Wände
- Spiel+: Wetter beim Einbruch: klare Nacht, bewölkt, Regen, Gewitter oder Nebel. Regen fällt auf die
  Straße und ist zu hören, Nebel zieht durchs Bild und verkürzt das Licht, bei Gewitter erhellt ein
  Doppelblitz Straße und Fenster, kurz darauf donnert es nah oder fern. Das Wetter steht in der
  Menüzeile zwischen Person und Zeit. Welches Wetter in welcher Nacht herrscht, bestimmt
  `PLUS/WEATHER.LUA` (Chancen und eine eigene Regel, nach Spieltag und Gebäude; ein geladenes Spiel
  hat dasselbe Wetter)
- Nacht und Wetter ändern nur Bild und Ton, nie den Ablauf: Wachen, Alarme, Zeiten und Zufallszahlen
  bleiben gleich. Abschaltbar oder fest einstellbar im Einstellungsmenü („Nacht und Wetter
  (Spiel+)“, `plusweather` in `cosp.cfg`). Funktioniert mit allen Bildfiltern und mit „Kanten glätten“
- Spiel+: Am Morgen nach dem Coup bringt die Zeitung eine Schlagzeile zum Einbruch, je nachdem, ob
  die Bande entkam, ob es Alarm gab und wie viel sie erbeutete. Die Texte stehen in
  `PLUS/NEWS.LUA` und lassen sich dort ändern
- Browser: Spielstände lassen sich als Datei sichern und wieder einspielen

### Behoben

- Spiel+: Die Hintergrundgeräusche richten sich nach dem Ort statt nach der Musik. Im Hotelzimmer und
  an der Rezeption lief bisher die Straße, weil diese Szenen die Straßenmusik behalten. Die Rezeption
  hat jetzt ein eigenes, gedämpftes Geräusch
- Planung: Der Weg per Rechtsklick konnte durch dünne Wände führen, etwa neben der Tür des Kiosks.
  Die Wegsuche prüft jetzt auch die Wand direkt vor der Person. Das Gehen mit den Pfeiltasten ist
  unverändert
- GitHub-Seite und Projektseite überarbeitet: Screenshots, Original und Spiel+ im Überblick, die
  Hinweise zum Bauen stehen jetzt in `docs/ENTWICKLUNG.md`

### Technik

- Neue Testpartie `burglary-weather.txt`, die einen Einbruch mit Gewitter und ohne Wetter spielt und
  dasselbe Ergebnis verlangt; die Testpartie in Spiel+ prüft auch die Zeitung
- Neue Testpartie `walk-to-kiosk.txt`: drei Rechtsklicks in den geschlossenen Kiosk bleiben draußen

## 0.20.0 (21.11.2026)

### Neu

- Spiel+: In der Planung zeigt der Grundriss die Laufwege der Wachen und den Bereich, den jede Wache
  gerade sieht (Taste W, in Spiel+ zu Beginn an). Er folgt der echten Regel des Spiels: Eine Wache
  entdeckt Einbrecher auf demselben Stockwerk, im selben Raum und auf der Seite, in die sie blickt,
  ohne Grenze für Winkel oder Entfernung. Beim Gehen blickt sie in Laufrichtung, beim Warten,
  Kontrollieren und Öffnen nach unten. Kleine Quadrate markieren, wo eine Wache stehen bleibt
- Spiel+: Taste T in den Planungsmenüs öffnet eine Zeitleiste mit einer Zeile pro Einbrecher (Gehen,
  Warten, Werkzeug, Nehmen/Ablegen, Öffnen/Schließen, Funk) und der Gesamtzeit. Dort macht Z (oder
  die Rücktaste) die letzte Aktion der aktuellen Person rückgängig, Y stellt sie wieder her
- Spiel+: Hintergrundgeräusche auch im Hotelzimmer (tickende Uhr, gedämpfter Verkehr) und auf der
  Polizeiwache (Schreibmaschine, Telefon)
- Neues Datenwerkzeug `tools/clou-data.py` (nur Python 3): liest und schreibt Objektdatenbank,
  Relationen, Gebäude (Bodenraster, Scheinwerfer, Wachenrouten), Einbruchspläne, Handlung,
  verschlüsselte Texte und Listen verlustfrei als lesbares JSON mit Objekt- und Relationsnamen.
  Grundlage für neue Einbrüche in Spiel+. Alle Formate sind aus dem Ladecode abgeleitet und in
  `docs/DATEIFORMATE.md` beschrieben, auch welche Dateien ein Gebäude ausmachen

### Technik

- Neuer Test `tests/run-data-test.sh`: Alle 256 unterstützten Spieldateien ergeben nach Export und
  Import wieder dieselben Bytes; eine per JSON geänderte Wachenroute und ein geänderter Beutewert
  laden über `MODFILES` in einer kompletten Testpartie
- Der Originalmodus bleibt unverändert, ein neuer Test prüft die Planung in Spiel+ und dass die neuen
  Tasten im Original nichts bewirken

## 0.19.0 (21.11.2026)

### Neu

- Spiel+: Hintergrundgeräusche. Zur Straßenmusik hört man den Verkehr und Schritte auf nassem
  Pflaster, in den Pubs Stimmengemurmel, Gläser und Dartpfeile. Lautstärke über „Geräusche“,
  abschaltbar im Einstellungsmenü („Hintergrundgeräusche (Spiel+)“, `plusambience` in `cosp.cfg`).
  Die Dateien liegen in `PLUS/AMBIENT` und lassen sich über MODFILES ersetzen
- Testskripte kennen `keydown` und `keyup`, um eine Taste zu halten

### Behoben

- Die Tastenwiederholung des Originals fehlte seit der Umstellung auf SDL. Die Pfeiltasten
  wiederholen sich jetzt mit Verzögerung und Tempo, die das Spiel vorgibt, wie auf der PC-Tastatur
  von 1994: In der Planung läuft eine Figur beim Halten nach einer Viertelsekunde zügig los, sonst
  gilt die übliche halbe Sekunde

### Dokumentation

- `ROADMAP.md` auf den aktuellen Stand gebracht

## 0.18.0 (21.11.2026)

### Neu

- Einstellungsmenü: „Kanten glätten“ rundet beim Vergrößern die Treppenstufen schräger Kanten ab
  (Scale2x, 3x oder 4x, je nach Fenstergröße). Lässt sich mit den Bildfiltern kombinieren, gilt
  sofort und auch im Browser. In `cosp.cfg` heißt die Option `scale2x`
- Spiel+: Bei „Werkzeuge beschreiben“ steht unter der Beschreibung, welche Fähigkeit das Werkzeug
  wie stark verlangt und wer aus der Bande damit umgehen kann
- Spiel+: Das Stethoskop verlangt 50 statt 70 von 100 beim Safeknacken; vorher erreichte das kaum
  ein Komplize, den man anwerben kann. Die Werte stehen in `PLUS/BALANCE.LUA` unter `tools` und
  lassen sich für jedes Werkzeug anpassen
- Spiel+: Vor jedem Einbruch wird automatisch gespeichert, in einem eigenen fünften Platz
  „Auto“. Die vier Plätze des Originals bleiben unberührt

## 0.17.0 (21.11.2026)

### Neu

- Spiel+: Neue Stimmen für die Figuren, die im Original stumm bleiben. 75 Sätze der Geschichte sind
  vertont, darunter Inspektor Gludo, Mr. Briggs, Sabien, Taxiunternehmer Dan Stanford, die
  Hotelrezeption, Hehler Pooly, Danner, Grull, der Pate und die Polizei. Matt behält seine
  Originalstimme, der Originalmodus bleibt unverändert. Die Dateien liegen in `PLUS/VOICE` und
  lassen sich über `MODFILES/PLUS/VOICE` ersetzen
- Einstellungsmenü: „Intro: an / aus“ und „Neue Stimmen (Spiel+): an / aus“
  (`nointro` und `plusvoices` in `cosp.cfg`)
- Im Browser liegen die neuen Stimmen in einer eigenen Datei `voices.data`, damit keine Datei die
  Größengrenze von GitHub Pages überschreitet

### Behoben

- Wurde das Intro übersprungen, bevor die Titelmusik einsetzt, blieb das Hauptmenü stumm. Ein neuer
  Test prüft das
- Die Ladeanzeige im Browser zählte über 100 %, wenn der Server die Daten komprimiert schickt. Der
  Text auf dem Startbildschirm wird bei kleinen Fenstern nicht mehr abgeschnitten

## 0.16.0 (21.11.2026)

### Neu

- Browser auf dem Handy und Tablet: Neben dem Bild liegen Tasten für die Pfeile, „OK“ (Enter),
  „Zurück“ (Esc), „Rechtsklick“ (der nächste Tipp zählt als rechte Maustaste) und „Vollbild“.
  Gehaltene Pfeile wiederholen sich wie auf der Tastatur. Im Querformat füllt das Bild den
  Bildschirm, Seitenkopf und Fußzeile treten zurück. Kein Zoomen oder Verrutschen beim Tippen
- Die Browser-Version lässt sich als App auf den Startbildschirm legen und startet dann im
  Vollbild und im Querformat (Web-App-Manifest mit Icon)

### Geändert

- Das Intro läuft wieder beim Start, wie 1994. Mit Esc oder „Zurück“ geht es direkt ins Menü.
  Abschalten lässt es sich weiter mit `nointro = 1` in `cosp.cfg`

### Behoben

- Wurde das Fenster während des Intros geschlossen, endete nur das Intro und das Spiel lief im
  Hauptmenü weiter. Jetzt beendet es das Spiel

## 0.15.1 (21.11.2026)

### Behoben

- Mit der genauen AdLib-Emulation (Standard seit 0.15.0, auch im Browser) verstummte die Musik nach
  wenigen Sekunden fast ganz. Der Musikplayer schaltet eine Note aus und sofort wieder an; eine echte
  AdLib-Karte ist langsam genug, um das mitzubekommen, die Emulation bekam beides gleichzeitig und
  schlug gehaltene Noten nicht neu an. Die Befehle an den Chip kommen jetzt wie bei der echten Karte
  mit kurzer Verzögerung an. Die Titelmusik läuft damit wie mit AdPlug und der klassischen Emulation

### Technik

- Neuer Test `tests/run-music-test.sh`: nimmt die Titelmusik über den Audiotreiber „disk“ auf und
  schlägt fehl, wenn sie verstummt. Die CI prüft beide Emulationen
- `ROADMAP.md` mit den geplanten Punkten, getrennt nach Original und Spiel+

## 0.15.0 (21.11.2026)

### Neu

- AdLib-Musik über Nuked OPL3 1.8, eine sehr genaue Nachbildung des Yamaha-Chips. Die Musik klingt damit
  näher an einer echten AdLib-Karte als mit der bisherigen MAME-Emulation. Beide sind gleich laut
- Einstellungsmenü: „AdLib-Musik: genau / klassisch“ schaltet im laufenden Spiel um, die Musik beginnt
  dann neu. In `cosp.cfg` heißt die Option `opl = nuked | mame`
- Nuked OPL3 liegt unverändert in `src/sound/nuked/` (LGPL 2.1 oder neuer), der Lizenztext ist in jedem Paket
- Bildfilter „Scanlines“ und „Röhrenmonitor“ (Scanlines und die Farbstreifen einer Streifenmaske), im
  Einstellungsmenü unter „Bild“ oder mit `displayfilter = off | scanlines | crt`. Der Filter wird beim
  Umwandeln der 256 Farben in das Fensterbild über vorbereitete Farbtabellen angewendet und kostet damit
  praktisch keine Rechenzeit, auch im Browser. Screenshots mit F12 zeigen das Bild mit Filter

### Behoben

- Beim Wechsel der Musik konnte der Haupt-Thread den AdLib-Chip zurücksetzen, während der Audio-Thread
  gerade Ton daraus erzeugte. Zurücksetzen und Registerzugriffe laufen jetzt unter der Audio-Sperre

## 0.14.0 (21.11.2026)

### Neu

- Lua 5.5.1 als Skriptsprache für Spiel+. Lua ist fest eingebaut (feste Version mit Prüfsumme) und läuft
  abgeschottet: nur Basis-, Text-, Tabellen-, Mathe-, UTF-8- und Coroutine-Bibliothek, ohne `dofile`,
  `loadfile` und `load`, also ohne Zugriff auf Dateien oder das System
- Erster Einsatz: Die Prozentwerte des Schwierigkeitsgrads (Polizeizeit, Hehlerpreise) stehen in
  `PLUS/BALANCE.LUA` und lassen sich über `MODFILES/PLUS/BALANCE.LUA` ändern. Ein fehlerhaftes Skript
  landet mit Zeilennummer in `clou.log`, das Spiel nimmt dann die eingebauten Werte
- Der Originalmodus startet Lua nie, das prüft die Referenzpartie jetzt mit. Neue Testprüfung `REJECT`
  für Zeilen, die nicht im Log stehen dürfen

## 0.13.0 (21.11.2026)

### Technik

- Umstieg von SDL 2 auf SDL 3 (3.4.20). Ist keine passende Version installiert, baut CMake genau diese
  Version mit (feste Prüfsumme), unter Linux und im Browser immer. Auf Linux ist SDL damit fest eingebaut
  und nutzt X11, Wayland, PulseAudio, PipeWire oder ALSA, je nachdem, was vorhanden ist
- SDL_image wird nicht mehr gebraucht: ILBM liest der eingebaute Decoder, PNG und BMP lädt SDL 3 selbst,
  Screenshots speichert SDL 3 als PNG
- Ton über die Audio-Streams von SDL 3 mit derselben Puffergröße wie bisher
- Mausposition wird über die Darstellung von SDL 3 umgerechnet, damit Klicks auch bei skaliertem Fenster,
  Vollbild und ganzzahliger Skalierung genau treffen
- Alle Testpartien liefern mit SDL 3 dieselben Bilder und Referenzwerte wie mit SDL 2

### Behoben

- Browser-Version: Die Einstellungen gingen nach einem Neuladen verloren, weil nur der Ordner der
  Spielstände in IndexedDB gespeichert wird. `settings.cfg` liegt im Browser jetzt dort
- Kleinere Aufräumarbeiten, damit auch Clang (Browser-Build) keine Warnungen mehr meldet

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
