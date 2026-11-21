# Audit: Der Clou! remastered

Stand: 21.11.2026, Version 0.21.0. Dieser Bericht ersetzt den Audit zur Version 0.9.0. Geprüft wurden
der neue Code, die Trennung von Original und Spiel+, Lizenz und Pakete, CI, Browser-Version und
Webseite sowie die Übereinstimmung von Beschreibung und Spiel.

Priorität: **H** = als Nächstes, **M** = bald, **N** = bei Gelegenheit.
Aufwand: S = unter einer Stunde, M = einige Stunden, L = Tage.
„Bestätigt“ heißt: im laufenden Spiel oder mit einem Befehl nachgestellt. „Gelesen“ heißt: aus dem
Code abgeleitet.

## Stand mit Version 0.22.0

| ID | Stand |
| --- | --- |
| H1, H2, H3, H4, H6 | Erledigt |
| H5 | Offen: feste Zufallsfolge für Tests und zweite Referenzpartie fürs Original (Roadmap O5). Einen Einbruch mit Alarm und Festnahme gibt es jetzt als Test in Spiel+ |
| H7 | Erledigt in der README; die Bedingungen der Quelle der Tondateien prüft der Projektinhaber |
| M1, M2, M3, M4, M5, M7, M8, M9, M10, M11, M13, M14, M16 | Erledigt |
| M6 | Offen: Modfiles wirken weiter auch im Original |
| M12 | Offen: Größe der Browser-Daten und des Verlaufs |
| M15 | Offen: Werkzeug-Infos am Bildschirm nachweisen |
| M17 | Offen: Entscheidung zum Polizei-Countdown im Original |
| N2, N3, N5, N8, N10, N11, N12, N13 | Erledigt (N10: die Sprachlautstärke bleibt ohne CD-Daten verborgen; N12 ohne Fortschrittsbalken) |
| N14 | Erledigt bis auf die Versionsangabe lokaler Builds |
| N1, N4, N6, N7, N9, N15 | Offen |

Der Bericht zur Version 0.21.0 folgt unverändert.

## Kurzfassung

- Die Trennung der Spielarten hält. Lua, Schwierigkeit, neue Stimmen, Hintergrundgeräusche, Nacht und
  Wetter, Zeitung, automatisches Speichern und die Planungshilfen wirken nie im Original, auch nicht
  nach einem Wechsel der Spielart im laufenden Programm.
- Mit den mitgelieferten Daten wurde kein Absturz gefunden. Alle bestätigten Abstürze brauchen
  ausgetauschte Dateien (Mods) oder eine ungewöhnliche Umgebung (kein Tongerät, sehr langer Pfad).
- Die größte Lücke liegt bei den Tests: Die Referenzpartie für das Original prüft zu wenig, und drei
  Verbote in der CI konnten nie anschlagen.
- Bei der Lizenz fehlen zwei Lizenztexte in den Paketen und eine Angabe zu den neuen Ton-Dateien.

## Was geprüft wurde

| Bereich | Ergebnis |
| --- | --- |
| Neuer Code (Licht, Wetter, Planung, Lua, Ton, Spielstände, Eingabe, Bildfilter) | Sieben Fehler bestätigt (H1 bis H3, M1 bis M4), der Rest sauber: Lichtkarte, Wegsuche, Zeitleiste, Einlesen der Einstellungen, fünfter Speicherplatz |
| Abschottung von Lua | Hält: kein Zugriff auf Dateien, System, `require`, `load`, `debug`; Bytecode wird abgelehnt. Es fehlen Grenzen für Laufzeit und Speicher (H2) |
| Original gegen Spiel+ | Alle Schalter sitzen richtig. Zwei Nebenwege (Pläne, Modfiles) und das Lademenü sind nicht getrennt (M5 bis M7) |
| Spielstände | Die Spielart steht im Spielstand und wird zuverlässig gelesen, ältere Spielstände laden als Original |
| Lizenz und Pakete | Name, neo-Hinweis und Lizenzdateien von neo, MAME und Nuked OPL3 sind überall vorhanden, nur deutsche Spieldaten. Es fehlen Lua und libiconv (H6) |
| Repository | Keine Zugangsdaten, keine privaten Pfade, keine Build-Ausgaben im Verlauf. Texte deutsch, Commits und Kommentare englisch |
| CI | Filter für die Testpartien und Release-Ablauf arbeiten wie gedacht, mit den Schwächen H4, H5, M8, M9 |
| Browser und Webseite | Touch-Bedienung, Ton nach Klick, Ladereihenfolge, Speicher, Manifest und alle Links in Ordnung. Schwächen M10 bis M12 |
| Beschreibung gegen Spiel | Alle beschriebenen Optionen und Tasten gibt es. Abweichungen siehe H3, M13 bis M16 |

Nicht geprüft: das Spiel unter Windows und macOS zur Laufzeit, die Browser-Version in einem echten
Durchlauf, Französisch, Spanisch und Englisch am Bildschirm (es liegen nur deutsche Daten vor), der
alte Code jenseits dessen, was der neue aufruft (Musik-Player, VOC, Intro-Animation, Datenbank mit
beschädigten Dateien).

## Befunde

### Hoch

| ID | Bereich | Befund | Empfehlung | Aufwand |
| --- | --- | --- | --- | --- |
| H1 | Ton | Bestätigt: Eine WAV-Datei mit mehr als 22 kHz, Stereo oder mehr als 16 Bit überschreibt den Puffer des Mischers (`mxr.c`, `mxr_wav.c`), das Spiel stürzt ab. Format und Rate werden nicht geprüft. Die mitgelieferten Dateien passen alle | Nur PCM mit 1 oder 2 Kanälen, 8 oder 16 Bit und sinnvoller Rate annehmen, die gelesene Menge auf den Puffer begrenzen | S |
| H2 | Lua | Bestätigt: Ein Skript kann das Spiel abbrechen (Fehler in einer Metamethode außerhalb eines geschützten Aufrufs) oder einfrieren (`while true do end`). Es gibt keine Grenze für Rechenschritte und Speicher | Zugriffe ohne Metamethoden oder geschützt ausführen, Zählhaken mit Budget vor jedem Aufruf, Speichergrenze, eigener Panic-Handler | M |
| H3 | Screenshots | Bestätigt: F12 speichert im normalen Bild ein PNG, dessen Farben alle durchsichtig sind. Viele Programme zeigen es leer. Mit Bildfilter oder Nachtlicht stimmt es | Palette vor dem Speichern deckend setzen oder als RGB speichern, im Test prüfen | S |
| H4 | Tests | Bestätigt: Drei Verbote der Testpartien schlagen nie an, weil `grep` ohne `-E` das Zeichen `\|` wörtlich nimmt: „Original startet nie Lua“, „Original nutzt keine neuen Stimmen und Geräusche“, „Wetter aus zeigt nichts“. Die Protokolle sind derzeit sauber | `grep -qE` in `tests/run-script-test.sh` | S |
| H5 | Tests | Bestätigt: Die Referenzpartie für das Original prüft drei Zeilen (Spielart, Dauer, Flucht) ohne Alarm. Sie besteht auch mit einem anderen Zufallsstartwert, und ihre Werte stammen aus diesem Projekt, nicht aus dem Spiel von 1994. Dazu kommt: Die Ortsanimationen ziehen Zufallszahlen nach Echtzeit, und `rand()` unterscheidet sich je System. Eine feste Zufallsfolge ist so nicht prüfbar | Für Testläufe einen eigenen Generator und einen getrennten Strom für Animationen. Zweite Referenzpartie mit Alarm, Polizei und Fahndung, einmal mit Tasten und ohne Zeitraffer. Werte gegen den Ausgangsstand oder DOSBox abgleichen (Roadmap O5) | L |
| H6 | Lizenz | Bestätigt: In keinem Paket liegt der Lizenztext von Lua (MIT verlangt ihn). Das Windows-Paket enthält `libiconv-2.dll` (LGPL) ohne Lizenztext | `LICENSE-Lua.txt` in alle Pakete, für libiconv den Text beilegen oder SDL unter Windows selbst bauen | S |
| H7 | Lizenz | Für die neuen Dateien unter `gamedata/PLUS` (Stimmen, Geräusche, Wetter, Skripte) steht nirgends, woher sie stammen und unter welchen Bedingungen sie weitergegeben werden dürfen. Die neo-Lizenz verlangt für neues Material dieselben Bedingungen | Absatz in der README, vorher die Bedingungen der Quelle prüfen | S |

### Mittel

| ID | Bereich | Befund | Empfehlung | Aufwand |
| --- | --- | --- | --- | --- |
| M1 | Bilder | Bestätigt: Ein ersetztes Bild, das kein Bild mit 256-Farben-Palette ist, lässt das Spiel beim Start abstürzen (`gfxPrepareColl`). Gelesen: Ein beschädigtes ILBM kann über die Bildfläche hinaus schreiben | Format prüfen, Fehler melden statt abstürzen, ILBM-Entpacker begrenzen | M |
| M2 | Modfiles | Bestätigt: `MODFILES` wirkt bei keiner Tondatei (WAV, VOC, HSC, Intro), obwohl README, Changelog und `DATEIFORMATE.md` das zusagen. Eine Stimme, die es nur in `MODFILES` gibt, bleibt stumm | Den Pfad an einer Stelle auflösen und in allen Ladern verwenden | S |
| M3 | Pfade | Bestätigt: Ein Spielordner mit etwa 250 Zeichen Pfadlänge bricht den Start ab (`disk.c`, fester Puffer mit `sprintf`) | Größe mitgeben, `snprintf`, Puffer in `FILENAME_MAX` | S |
| M4 | Ton | Bestätigt: Ohne Tongerät stürzt das Spiel beim Start ab (`fx.c`, Mischer ist `NULL`). Den Fehler gab es schon im Ausgangsstand | `MXR_Set*` bei fehlendem Mischer beenden | S |
| M5 | Original | Gelesen: Ein in Spiel+ erstellter Plan lädt und läuft im Original. Mit gelockerten Werkzeugwerten spielt das Original so eine Aktion ab, die die Regeln von 1994 verweigern | Werkzeugaktionen beim Laden im Original neu prüfen oder die Spielart im Plan vermerken | M |
| M6 | Original | Bestätigt: Ersetzte Dateien aus `MODFILES` wirken auch im Original, etwa eine geänderte Wachenroute | Im Original nur Darstellung zulassen oder deutlich warnen; Verbot in der Referenzpartie | M |
| M7 | Original | Bestätigt: Der Autosave-Platz aus Spiel+ erscheint im Lademenü eines Original-Spiels | Fünften Platz nur in Spiel+ und im Hauptmenü zeigen | S |
| M8 | CI | UBSan kann den Sanitizer-Job nicht scheitern lassen: Es fehlt `-fno-sanitize-recover`, geprüft wird nur auf AddressSanitizer | Option setzen oder auf `runtime error:` prüfen | S |
| M9 | CI | Release und Seite ohne Sperre: Der Tag `v0.21.0` zeigt auf den Commit nach „Release 0.21.0“, weil der erste Lauf scheiterte. Zwei Läufe kurz nacheinander können eine ältere Seite veröffentlichen. Jeder Job darf schreiben, `linuxdeploy` wird ohne Prüfsumme geladen und ausgeführt, Actions und Emscripten sind nicht festgenagelt | `concurrency`, Schreibrecht nur für Seite und Release, `linuxdeploy` und emsdk mit fester Version und Prüfsumme | M |
| M10 | Browser | Gelesen: Bricht der Download der Spieldaten ab, bleibt die Seite für immer auf „Lade Spieldaten“. Nach einem Release kann der Browser bis zu zehn Minuten neues Programm mit alten Daten mischen | Fehler anzeigen (`unhandledrejection`, `onAbort`), Version an die Dateinamen hängen | S |
| M11 | Webseite | Die Projektseite lädt eine Schrift von Google, der einzige Abruf bei Dritten | Schrift selbst ausliefern oder Systemschrift | S |
| M12 | Browser | `DerClou.data` hat 83 MB bei einer Grenze von 100 MB je Datei. Der Verlauf wächst mit jeder ersetzten WAV dauerhaft | Geräusche in das zweite Paket, vor weiteren Stimmen ein komprimiertes Format prüfen | M |
| M13 | Zeitung | Gelesen: Bei einer Festnahme erscheint die Zeitung nie, die Texte dafür sind toter Text. Sätze mit dem Gebäudenamen klingen falsch („bei Kiosk ein“), Französisch und Spanisch fallen auf Deutsch statt Englisch zurück, eine alte Schlagzeile überlebt das Laden eines anderen Spiels | Zeitung auch vor dem Gefängnis zeigen, Sätze umbauen, Merker beim Laden löschen | S |
| M14 | Bedienung | Gelesen: Tab schaltet den Zeitraffer überall um, auch im Menü, und er bleibt für den nächsten Einbruch an. Eine Anzeige gibt es nicht | Nur beim Abspielen annehmen, beim Start zurücksetzen, Anzeige in der Statuszeile | S |
| M15 | Werkzeuge | Die Werkzeug-Infos aus Spiel+ waren am Bildschirm nicht nachzuweisen: Im Testspiel führt die Händlerin nur Werkzeuge ohne Anforderung. Der Kommentar in `BALANCE.LUA` zur Anzeige stimmt nicht, der Schlüssel `Stethoskop` passt nur zu deutschen Daten | Testpartie mit einem Werkzeug, das Fähigkeiten verlangt; Kommentar und Schlüssel klären | M |
| M16 | Texte | Bestätigt: „keine Fähigkeiten“ steht in der Bandenübersicht immer in der ersten Spalte und überdeckt dort die Balken (alter Fehler). Die Zeitleiste sagt „wiederholen“ statt „wiederherstellen“. `cosp.cfg` beschreibt `plusvoices` und `plusambience` nicht. Neue Texte duzen, das Spiel siezt | Spalte korrigieren, Wörter angleichen, Optionen ergänzen | S |
| M17 | Original | Der Polizei-Countdown ist auch im Original an und zeigt einen Wert, den man 1994 nicht sah. Er ändert nichts am Ablauf und ist abschaltbar | Entscheidung: im Original aus als Vorgabe | S |

### Niedrig

| ID | Bereich | Befund | Empfehlung | Aufwand |
| --- | --- | --- | --- | --- |
| N1 | Code | Weiter offen: 185 Aufrufe von `strcpy`, `strcat` und `sprintf` gegenüber 49 begrenzten (Roadmap O7) | Schrittweise umstellen, zuerst Pfade (siehe M3) | M |
| N2 | Ton | WAV mit einem Block zwischen `fmt` und `data`, wie ihn Audacity und ffmpeg schreiben, wird stumm abgelehnt | Unbekannte Blöcke überspringen, Ablehnung ins Protokoll | S |
| N3 | Ton | Gelesen: `MXR_SetInputVolume` prüft den Eingang außerhalb der Sperre, der Ton-Thread kann ihn dazwischen freigeben | Sperre vor der Prüfung | S |
| N4 | Speicher | Weiter offen und größer: Beim Beenden meldet das Spiel 28 bis 115 KB nicht freigegebenen Speicher | Beim Aufräumen mitnehmen | M |
| N5 | Planung | Gelesen: Nach Rückgängig, einer anderen Aktion gleicher Dauer und Wiederherstellen wird die alte Aktion von der falschen Stelle aus angewendet | Wiederherstellen-Liste bei jeder anderen Planänderung leeren | S |
| N6 | Code | Weiter offen: Kommentare gemischt deutsch und englisch, Namensreste aus der Amiga-Zeit | Bei Änderungen angleichen | L |
| N7 | Planung | Weiter offen: kein Richtungswechsel bei Kollision ohne Schritt (Roadmap O4) | Nur wenn es im Original so gedacht war | M |
| N8 | Spielstand | Die Zeile mit der Spielart wird über ihre Position gefunden. Bekommt Spiel+ neue Szenen, lädt der Spielstand still als Original | Nach Schlüssel suchen | S |
| N9 | Original | Die Fluchtanimation nach einem Einbruch ohne Alarm ist seit einer Korrektur immer die halbe Strecke; 1994 hing das am Überlauf. Rechtsklick-Laufen, Zeitraffer und Tastenwiederholung lassen sich nicht abschalten | Abweichung dokumentieren, Schalter erwägen | S |
| N10 | Einstellungen | „Neue Stimmen“ ist ohne CD-Daten verborgen, die neuen Stimmen spielen trotzdem. Vollbild per F11 und die Lautstärketasten werden nicht gespeichert. Die Reihenfolge mischt Spiel+ und Allgemeines, das Menü gibt es nur im Hauptmenü | Eintrag immer zeigen, speichern, ordnen | S |
| N11 | CI | Kein Zeitlimit für Jobs, Artefakte bleiben 90 Tage, leere Release-Notiz fällt nicht auf, ein unbekanntes Testprofil läuft still mit weniger Tests | Kleine Absicherungen | S |
| N12 | Browser | Import der Spielstände prüft erst beim Schreiben, nimmt `.` und `..` an und sagt „ersetzt“, obwohl er zusammenführt. Fortschrittsbalken steht früh auf 100 %. Keine `og:`-Angaben mit voller Adresse, Spielfläche ohne Beschriftung für Vorleseprogramme | Erst prüfen, dann schreiben; Angaben ergänzen | S |
| N13 | Webseite | „Matt Stuvysant“ statt „Stuvysunt“, „rund 80 MB“ statt etwa 95 MB, macOS-Paket läuft nur auf Apple-Chips | Texte korrigieren | S |
| N14 | Doku | Roadmap P14 nennt Hotel und Polizei noch als offen. `DATEIFORMATE.md` kennt die neuen Dateien unter `PLUS` und den fünften Speicherplatz nicht. Die Versionsangabe im Menü eines lokalen Builds kann veraltet sein | Nachziehen, Version beim Bauen aus `VERSION` | S |
| N15 | Texte | „Policia“ ohne Akzent, französische Wetterwörter mit gemischtem Geschlecht, ein französischer Satz ein Zeichen zu lang. Fest eingebaut unabhängig von der Sprache: „THE TIMES“, „Auto“. Die Umwandlung der Lua-Texte kennt einige französische und spanische Zeichen nicht | Bei der nächsten Textrunde | S |

## Aus dem Audit zur Version 0.9.0

| ID | Stand |
| --- | --- |
| H1, H3, M1, M2, M3, M5, M6, M7, M8, N2, N8 | Erledigt (zu M3 siehe neues H3) |
| H2 | Entfällt: neo Software existiert nicht mehr, eine Adresse für die Mail gibt es nicht |
| H4 | Erledigt, aktuell ist Version 0.21.0 |
| M4 | Erledigt: Nuked OPL3 ist Vorgabe, das fehlende Instrument liegt bei kaum hörbarer Lautstärke |
| N3 | Weitgehend gegenstandslos: der MAME-Emulator ist nur noch wählbar |
| N5 (Tastenwiederholung) | Erledigt in 0.19.0 |
| N1, N4, N6, N7 | Offen, oben weitergeführt |

## Vorgeschlagene Reihenfolge

1. Tests scharf stellen: H4 und M8, danach laufen lassen und sehen, ob etwas auffällt.
2. Abstürze mit fremden Dateien und Umgebungen: H1, H2, M1 bis M4, N2.
3. Lizenz: H6 und H7.
4. Original sauber halten: M5 bis M7, Entscheidung zu M17.
5. Spürbares für Spieler: H3, M13, M14, M16, M10, M11.
6. CI absichern: M9, N11.
7. Die große Aufgabe: H5 mit fester Zufallsfolge und zweiter Referenzpartie.
