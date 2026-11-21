# Roadmap: Der Clou! remastered

Stand: Version 0.22.2. Die Roadmap ist nach den beiden Spielarten getrennt. Im Original gilt:
Es spielt sich wie 1994. Erlaubt sind dort nur Fehlerkorrekturen, Technik und Komfort, der den
Spielablauf nicht verändert und abschaltbar ist. Alles, was Regeln, Balance oder Inhalte ändert,
gehört in Spiel+.

## Erledigt

- Einstellungsmenü, Spielarten Original und Spiel+ mit Referenzpartie, SDL 3, Lua 5.5 für Spiel+
- Genaue AdLib-Emulation (Nuked OPL3), Bildfilter Scanlines und Röhrenmonitor, Kanten glätten
- Bedienung im Browser auf Handy und Tablet, Web-App zum Installieren, Intro wieder an
- Tastenwiederholung wie auf der PC-Tastatur von 1994 (die Planung läuft beim Halten schneller)
- Spiel+: neue Stimmen für 75 Sätze der Geschichte, Hintergrundgeräusche je nach Ort (Straße, Park, Pub, Hotel, Polizei, Laden, Werkstatt, Bahnhof)
- Spiel+: Werkzeug-Infos (wer aus der Bande es benutzen kann), Werkzeugwerte per Lua (Stethoskop),
  automatisches Speichern vor jedem Einbruch in einem eigenen fünften Platz
- Spiel+ Planung: Laufwege und Sichtbereich der Wachen (Taste W), Zeitleiste mit Rückgängig und
  Wiederherstellen (Taste T)
- Spiel+: Zeitung am Morgen nach dem Coup mit Schlagzeile per Lua (`PLUS/NEWS.LUA`)
- Browser: Spielstände als Datei sichern und wieder einspielen
- Spiel+ Einbruch bei Nacht: Taschenlampen, Laternen der Wachen, Mondlicht durch die Fenster,
  Wetter (klar, bewölkt, Regen, Gewitter, Nebel) aus `PLUS/WEATHER.LUA`, nur Bild und Ton
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
| O11 | „Kanten glätten“ überarbeiten: Scale2x wirkt bei diesen Bildern seltsam, ein anderes Verfahren suchen (zum Beispiel xBR oder weiches Hochskalieren mit Schärfe) | M |

## Spiel+

| Punkt | Inhalt | Aufwand |
| --- | --- | --- |
| P2 | Zeitleiste: Sprungmarken zu einem Zeitpunkt, Wachen in der Zeitleiste | M |
| P3 | Einbruchspläne kopieren und Varianten vergleichen | M |
| P9 | Neue Einbrüche, Ereignisse und Nebenhandlungen per Lua, erster Beweis ist ein neues Gebäude | L |
| P13 | Stimmen für die freien Gespräche und die übrigen Figuren (Abt, Auftraggeber am Marx-Grab) | M |
| P14 | Weitere Hintergrundgeräusche (Museen, Hafen, Gefängnis, Einbruch) | S |
| P12 | Musik neu arrangiert (Probe als Jazz liegt vor, vorerst zurückgestellt) | L |
| P15 | HD-Ebene für einzelne Bilder, erst als Test mit zwei, drei Orten | L |
| P16 | Weichere Überblendungen zwischen Szenen | M |
| P20 | Gebäude beobachten: Wachenwege und Sichtbereiche erst nach einer Nacht Auskundschaften (Der Clou! 2) | M |
| P21 | Lärmbalken während des Einbruchs | S |
| P22 | Ruf der Bande: bessere Hehlerpreise und Komplizen nach erfolgreichen Coups | M |
| P23 | Tragkraft je Komplize, Lichtschranken und Kameras in neuen Gebäuden | L |
| P24 | Licht mit Wirkung: dunkle Ecken verbergen, laute Böden verraten (wie Thief) | M |
| P25 | Steigende Alarmstufe im Gebäude, Polizeidruck pro Stadtteil, Untertauchen | M |
| P26 | Karriereübersicht mit Statistik und Erfolgen, Herausforderungen, Ironman | M |
| P27 | Wiederholung des Einbruchs als Film, als Video speichern | M |
| P28 | Mod-Auswahl im Menü, Farbschema für Farbenblinde, größere Schrift | M |
| P29 | Zufällig erzeugte Gebäude aus Bausteinen | L |
| P17 | Wirkung des Wetters auf den Ablauf: Regen übertönt Geräusche, Nebel verkürzt die Sicht der Wachen, nasse Straßen verlängern die Flucht (Bild und Ton gibt es schon) | M |
| P18 | Zufällige neue Komplizen: Werte, Namen und Vorgeschichte per Lua, Porträts aus Einzelteilen | L |
| P19 | Komplizen trainieren: Kurse gegen Geld und Zeit, Fortschritt durch Einbrüche | M |
