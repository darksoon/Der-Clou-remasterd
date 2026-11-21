# Roadmap: Der Clou! remastered

Stand: Version 0.20.0. Die Roadmap ist nach den beiden Spielarten getrennt. Im Original gilt:
Es spielt sich wie 1994. Erlaubt sind dort nur Fehlerkorrekturen, Technik und Komfort, der den
Spielablauf nicht verändert und abschaltbar ist. Alles, was Regeln, Balance oder Inhalte ändert,
gehört in Spiel+.

## Erledigt

- Einstellungsmenü, Spielarten Original und Spiel+ mit Referenzpartie, SDL 3, Lua 5.5 für Spiel+
- Genaue AdLib-Emulation (Nuked OPL3), Bildfilter Scanlines und Röhrenmonitor, Kanten glätten
- Bedienung im Browser auf Handy und Tablet, Web-App zum Installieren, Intro wieder an
- Tastenwiederholung wie auf der PC-Tastatur von 1994 (die Planung läuft beim Halten schneller)
- Spiel+: neue Stimmen für 75 Sätze der Geschichte, Hintergrundgeräusche auf der Straße und im Pub
- Spiel+: Werkzeug-Infos (wer aus der Bande es benutzen kann), Werkzeugwerte per Lua (Stethoskop),
  automatisches Speichern vor jedem Einbruch in einem eigenen fünften Platz
- Spiel+ Planung: Laufwege und Sichtbereich der Wachen (Taste W), Zeitleiste mit Rückgängig und
  Wiederherstellen (Taste T)
- Datenwerkzeug `tools/clou-data.py`: Datenbank, Gebäude, Wachenrouten, Pläne, Handlung und Texte
  verlustfrei als JSON lesen und schreiben, Formate in `docs/DATEIFORMATE.md`
- Musik: Das Titelstück fordert einmal ein Instrument an, das es nicht gibt. Das geschieht bei
  Lautstärke 60 von 63 Stufen Dämpfung, ist also praktisch unhörbar; das Spiel behält dort das
  bisherige Instrument

## Original

| Punkt | Inhalt | Aufwand |
| --- | --- | --- |
| O2 | Fehler der Erstversion prüfen, die die Profi-Disk behoben hat (zum Beispiel Türen, die sich hinter Wachen nicht schließen) | M |
| O4 | Richtungswechsel bei Kollision in der Planung (AUDIT N7), nur wenn es im Original auch so gedacht war | M |
| O5 | Zweite Referenzpartie mit Alarm und Polizei, damit auch die Fluchtzeit abgesichert ist | M |
| O6 | Tippfehler und Textfehler in den deutschen Texten, ohne Inhalte zu ändern | S |
| O7 | Sichere Zeichenkettenfunktionen (AUDIT N1) | M |
| O8 | Übersicht der Tasten im Einstellungsmenü | S |
| O9 | Gamepad vollständig bedienbar, auch in der Planung | M |
| O10 | GPU-Shader auf dem Desktop (im Browser bleibt der bisherige Filter) | M |

## Spiel+

| Punkt | Inhalt | Aufwand |
| --- | --- | --- |
| P2 | Zeitleiste: Sprungmarken zu einem Zeitpunkt, Wachen in der Zeitleiste | M |
| P3 | Einbruchspläne kopieren und Varianten vergleichen | M |
| P9 | Neue Einbrüche, Ereignisse und Nebenhandlungen per Lua, erster Beweis ist ein neues Gebäude | L |
| P13 | Stimmen für die freien Gespräche und die übrigen Figuren (Abt, Auftraggeber am Marx-Grab) | M |
| P14 | Weitere Hintergrundgeräusche (Bahnhof, Hotel, Polizei, Einbruch) | S |
| P12 | Musik neu arrangiert (Probe als Jazz liegt vor, vorerst zurückgestellt) | L |
| P15 | HD-Ebene für einzelne Bilder, erst als Test mit zwei, drei Orten | L |
| P16 | Weichere Überblendungen zwischen Szenen | M |
| P17 | Wetter (Regen, Nebel, klare Nacht) mit Wirkung auf Geräusche, Sicht der Wachen und Flucht, sichtbar und hörbar beim Einbruch | M |
| P18 | Zufällige neue Komplizen: Werte, Namen und Vorgeschichte per Lua, Porträts aus Einzelteilen | L |
| P19 | Komplizen trainieren: Kurse gegen Geld und Zeit, Fortschritt durch Einbrüche | M |
