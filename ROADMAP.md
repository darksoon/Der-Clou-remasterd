# Roadmap: Der Clou! remastered

Stand: Version 0.15.0. Die Roadmap ist nach den beiden Spielarten getrennt. Im Original gilt:
Es spielt sich wie 1994. Erlaubt sind dort nur Fehlerkorrekturen, Technik und Komfort, der den
Spielablauf nicht verändert und abschaltbar ist. Alles, was Regeln, Balance oder Inhalte ändert,
gehört in Spiel+.

## Original

### Fehler und Treue zum DOS-Original

| Punkt | Inhalt | Aufwand |
| --- | --- | --- |
| O1 | HSC-Musik mit DOSBox und AdPlug vergleichen (Instrument mit Index über 127, AUDIT M4) und das Originalverhalten nachbilden | M |
| O2 | Bekannte Fehler der Erstversion prüfen, die die Profi-Disk behoben hat (zum Beispiel Türen, die sich hinter Wachen nicht schließen), und sicherstellen, dass sie hier behoben sind | M |
| O3 | Tastenwiederholung wie im Original (AUDIT N5) | S |
| O4 | Richtungswechsel bei Kollision in der Planung (AUDIT N7), nur wenn es im Original auch so gedacht war | M |
| O5 | Zweite Referenzpartie mit Alarm und Polizei, damit auch die Fluchtzeit gegen Änderungen abgesichert ist | M |
| O6 | Tippfehler und Textfehler in den deutschen Texten, ohne Inhalte zu ändern | S |

### Technik und Komfort (abschaltbar, ändert keine Regeln)

| Punkt | Inhalt | Aufwand |
| --- | --- | --- |
| O7 | Sichere Zeichenkettenfunktionen (AUDIT N1) | M |
| O8 | Tastaturkürzel überall vollständig nutzbar, dazu eine Übersicht der Tasten im Einstellungsmenü | S |
| O9 | Gamepad vollständig bedienbar, auch in der Planung | M |
| O10 | Hochauflösende Bildfilter mit GPU-Shadern auf dem Desktop (im Browser bleibt der bisherige Filter) | M |
| O11 | Barrierearmut: größere Mauszeiger, auf Wunsch langsamere Texte | S |

## Spiel+

### Planung

| Punkt | Inhalt | Aufwand |
| --- | --- | --- |
| P1 | Sichtkegel und Laufwege der Wachen im Grundriss | L |
| P2 | Zeitleiste aller Figuren mit Sprungmarken, Rückgängig und Wiederholen | L |
| P3 | Einbruchspläne kopieren und Varianten vergleichen | M |
| P4 | Werkzeuge und Gegenstände mit Erklärung (wozu dient der Ausweis, was braucht man für welchen Safe) | S |

### Balance und Inhalte

| Punkt | Inhalt | Aufwand |
| --- | --- | --- |
| P5 | Werkzeugschwellen prüfen: Das Stethoskop verlangt 70 Prozent Safe-Können, kaum ein anwerbbarer Komplize erreicht das | S |
| P6 | Fluchtwagen: Farbe und Auffälligkeit mit echter Wirkung, Aufkleber entfernbar gegen Geld | M |
| P7 | Weicherer Spielbeginn auf „Leicht“, wenn das Geld für das Hotel nicht reicht | S |
| P8 | Datenwerkzeug für Gebäude und Datenbank (verlustfrei lesen und schreiben, JSON-Export) | L |
| P9 | Neue Einbrüche, Ereignisse und Nebenhandlungen per Lua, erster Beweis ist ein neues Gebäude | L |
| P10 | Statistik nach jedem Einbruch: Zeit, Lärm, Spuren, Beute | M |
| P11 | Automatisches Speichern vor jedem Einbruch | S |

### Ton

| Punkt | Inhalt | Aufwand |
| --- | --- | --- |
| P12 | Optionales Musikpaket als OGG: Aufnahmen der Original-Musik von echter OPL3-Hardware oder Neuarrangements, mit stb_vorbis abgespielt | M |
| P13 | Sprachausgabe für alle Figuren; bisher spricht nur Matt (CD-Fassung) | L |
| P14 | Geräusche im Hintergrund (Straße, Bar, Einbruch) | M |

### Grafik

| Punkt | Inhalt | Aufwand |
| --- | --- | --- |
| P15 | HD-Ebene für einzelne Bilder (Hintergründe, Porträts) über MODFILES, die Spiellogik bleibt beim 8-Bit-Bild | L |
| P16 | Weichere Animationen und Überblendungen zwischen Szenen | M |

## Vorgeschlagene Reihenfolge

1. O1, O2, O5: das Original absichern, bevor Spiel+ wächst.
2. P8, dann P9: Datenwerkzeug und erster neuer Einbruch.
3. P1, P2, P4: die Planung, die man am meisten bemerkt.
4. P12 und P13, danach P15.
