# Dateiformate von Der Clou!

Diese Beschreibung ist aus dem Ladecode des Spiels abgeleitet, nicht geraten. Zu jedem Format steht
die Funktion, die es liest. Das Werkzeug `tools/clou-data.py` liest und schreibt alle hier
beschriebenen Formate verlustfrei (siehe README, Abschnitt „Datenwerkzeug“).

## Allgemeines

- **Byte-Reihenfolge:** Alle Binärdateien sind Little Endian. Der Code liest Zahlen mit `dskRead`
  direkt in den Speicher und ruft danach `EndianW` oder `EndianL` auf (`src/port/port.c`). Diese
  Funktionen tauschen die Bytes nur auf Big-Endian-Rechnern, die Datei ist also Little Endian.
- **Keine Ausrichtung:** Die Felder liegen ohne Füllbytes hintereinander. Der Code liest jedes Feld
  einzeln, die Strukturen im Speicher sind dafür bedeutungslos.
- **Textdateien:** Zeilenende ist CR LF (DOS). `dskGets` (`src/disk/disk.c`) entfernt CR und LF,
  Zahlen werden mit `atol` gelesen. Viele Dateien enden mit dem DOS-Endezeichen `0x1A` (Strg+Z),
  das das Spiel ignoriert, weil danach keine Zeile mehr erwartet wird.
- **Dateinamen:** `dskBuildPathName` setzt `Wurzel/Ordner/Name` zusammen und schreibt alles nach der
  Wurzel groß. Ein Name aus den Daten wie `BulCEta0` wird so zu `DATA/BULCETA0.DAT`.
- **MODFILES:** `dskOpen` versucht beim Lesen zuerst `MODFILES/<Ordner>/<Name>` im Spielordner und
  schreibt dann `Modfile: <Pfad>` ins Log. Jede Datei dieser Beschreibung lässt sich so ersetzen,
  ohne die Originaldaten zu ändern.
- **Objektnummern:** Datenbank `tcMain` belegt die Nummern 1 bis 10000, `tcBuild` 500001 bis
  510000. Die Objekte eines Stockwerks liegen ab `ul_ObjectBaseNr` des Stockwerks (zum Beispiel
  660000), die Nummern sind dort fortlaufend.

## Übersicht

| Datei | Format im Werkzeug | Gelesen von | Inhalt |
| --- | --- | --- | --- |
| `DATA/TCMAIN.DAT` | `objects` | `dbLoadAllObjects` (`src/data/database.c`) | Personen, Autos, Orte, Werkzeuge, Gegenstandsarten, Beute, Spieler |
| `DATA/TCMAIN.REL` | `relations` | `LoadRelations` (`src/data/relation.c`) | Beziehungen der Hauptdatenbank (kennt, hat, wohnt in, ...) |
| `DATA/TCBUILD.DAT` | `objects` | `dbLoadAllObjects` | Gebäude, Stockwerke (`LSArea`), Wächter (`Police`) |
| `DATA/TCBUILD.REL` | `relations` | `LoadRelations` | welches Gebäude aus welchen Stockwerken besteht und wer es bewacht |
| `DATA/<STOCKWERK>.DAT` | `objects` | `lsInitObjectDB` (`src/landscap/init.c`) | alle Objekte eines Stockwerks: Mauern, Türen, Safes, Alarmanlagen, Räume |
| `DATA/<STOCKWERK>.REL` | `relations` | `lsInitObjectDB` | Schlösser, Alarm, Strom, Beute und Räume eines Stockwerks |
| `DATA/<STOCKWERK>.LFD` | `floor` | `lsInitFloorSquares` | Bodenraster 20 × 8 Felder |
| `DATA/<GEBÄUDE>.GRE` | `relations` | `lsLoadGlobalData` | Stechuhren und Treppen über alle Stockwerke |
| `DATA/<GEBÄUDE>.SPO` | `spots` | `lsLoadSpots` (`src/landscap/spot.c`) | Scheinwerfer und ihre Bahnen |
| `DATA/<GEBÄUDE>.GUA` | `actions` | `grdInit`, `LoadHandler` (`src/planing/guards.c`, `system.c`) | Laufwege der Wächter |
| `DATADISK/<GEBÄUDE>.PLL` | `planlist` | `plOpen` (`src/planing/io.c`) | welche der Pläne 1 bis 4 gespeichert sind |
| `DATADISK/<GEBÄUDE><n>.PLN` | `actions` | `plLoad`, `LoadSystem`, `plLoadTools`, `LoadHandler` | gespeicherter Einbruchsplan |
| `DATA/TCSTORY.PC` | `story` | `PrepareStory`, `LoadSceneforStory` (`src/gameplay/gp.c`) | Szenen der Handlung |
| `TEXTS/*.TXT` | `text` | `txtLoad` (`src/text/text.c`) | alle Texte, auch die Objektnamen |
| `TEXTS/*.LST`, `DATADISK/*.LST` | `list` | `ReadList` (`src/list/list.c`) und eigene Leser | Bilder, Grafiksammlungen, Figuren, Orte, Spielstände |
| `TCSTORYn.DAT` (Spielstand) | `scenestate` | `tcLoadChangesInScenes` (`src/gameplay/loadsave.c`) | wie oft jede Szene schon geschehen ist |

Spielstände im Benutzerordner (`DATADISK/TCMAINn.DAT`, `TCMAINn.REL`, `TCBUILDn.DAT`,
`TCBUILDn.REL`) haben dieselben Formate wie die Originaldateien, sie werden mit `dbSaveAllObjects`
und `SaveRelations` geschrieben.

## Welche Dateien ein Gebäude beschreiben

Am Beispiel Bulstrode Chemistry Ltd (`Building_Bulstrode_Chemestry_Ltd`, Nummer 509108):

1. **`TCBUILD.DAT`** enthält das Gebäude (Typ `Building`: Fluchtweg, Polizeizeit, Wert,
   Position des Fluchtwagens), seine Stockwerke (Typ `LSArea`: Grafiksammlungen, Größe,
   `ul_ObjectBaseNr`, Startpunkte) und die Wächter (Typ `Police`).
2. **`TCBUILD.REL`** verbindet sie: Relation `consistsOf` (500002) vom Gebäude zu jedem Stockwerk,
   `startsWith` (500001) zum Stockwerk, in dem der Einbruch beginnt, `isGuardedby` (500003) vom
   Gebäude zu jedem Wächter, der Parameter ist das Stockwerk des Wächters.
3. **Der Name des Stockwerks** steht in `TEXTS/TCMAINOD.TXT` (`#508115` → `BulCEta0`). Aus ihm
   entstehen die Dateinamen: `BULCETA0.DAT`, `BULCETA0.REL`, `BULCETA0.LFD`. Ohne die letzte
   Ziffer ergibt er die Dateien des ganzen Gebäudes: `BULCETA.GRE`, `BULCETA.SPO`, `BULCETA.GUA`
   und im Ordner `DATADISK` `BULCETA.PLL` mit `BULCETA1.PLN` bis `BULCETA4.PLN`.
4. **`<STOCKWERK>.DAT`** enthält alles, was im Grundriss steht: Mauern, Mauerecken, Türen, Fenster,
   Möbel, Safes, Alarmanlagen, Steuerkästen, Scheinwerfer, Treppen (Typ `LSObject`, die Art steht in
   `Type` als Nummer eines `Item`-Objekts aus `TCMAIN.DAT`) und die Räume (Typ `LSRoom`).
5. **`<STOCKWERK>.REL`** sagt, welches Schloss eine Tür hat, welche Alarmanlage und welcher
   Steuerkasten ein Objekt sichern, welche Beute in einem Safe liegt (mit ihrem Wert) und welche
   Objekte in welchem Raum stehen.
6. **`<STOCKWERK>.LFD`** sagt, wo Boden ist; **`<GEBÄUDE>.GRE`** verbindet Treppen und Stechuhren;
   **`<GEBÄUDE>.SPO`** beschreibt die Scheinwerfer; **`<GEBÄUDE>.GUA`** die Runden der Wächter.
7. Die Grafik des Grundrisses kommt über die Sammlungsnummern in `LSArea` (`us_Coll16ID`,
   `us_FloorCollID` usw.) aus `TEXTS/COLL.LST` und dem Ordner `PICTURES`.

`clou-data.py building gamedata Bulstrode` sammelt alle diese Angaben zu einem Gebäude.

## Objektdatenbank (`*.DAT`)

Gelesen von `dbLoadAllObjects` und `dbLoadOrSaveObject` (`src/data/database.c`), geschrieben von
`dbSaveAllObjects`. Die Datei ist eine Folge von Einträgen bis zum Dateiende, ohne Dateikopf:

| Offset | Typ | Feld |
| --- | --- | --- |
| 0 | uint32 | `nr`, Objektnummer |
| 4 | uint32 | `type`, Objekttyp (Tabelle unten) |
| 8 | uint32 | `size`, Länge der Daten in Byte |
| 12 | `size` Byte | Felder des Typs |

- Ist eines der drei Kopffelder `0xFFFFFFFF`, liest das Spiel keine Daten dazu und macht mit dem
  nächsten Kopf weiter.
- **Wichtig:** Das Spiel wertet `size` nicht aus. Es liest die Felder, die zum Typ gehören. Ein
  unbekannter Typ oder eine abweichende Länge bringt den Leser aus dem Tritt. Das Werkzeug bewahrt
  solche Einträge als Rohdaten (`raw`), damit nichts verloren geht.
- **Namen:** In `TCMAIN.DAT` und `TCBUILD.DAT` bekommt jedes Objekt beim Laden den Namen aus dem
  nächsten Schlüssel von `TEXTS/TCMAINO?.TXT` (`txtGoKey(OBJECTS_TXT, NULL)`), und zwar der
  Reihe nach, nicht über die Nummer. Die Schlüssel dort tragen die Objektnummer (`#127`), und die
  Reihenfolge stimmt mit `TCMAIN.DAT` und danach `TCBUILD.DAT` überein (450 Objekte, geprüft).
  Wer ein Objekt hinzufügt, muss also auch den Namen an derselben Stelle der Textdatei einfügen.
  Die Objekte der Stockwerke werden ohne Namen geladen (`DB_LOAD_MODE_NO_NAME`).
- Die Reihenfolge der Einträge bleibt erhalten: `dbSaveAllObjects` schreibt in Ladereihenfolge.

Feldtypen: `B` = 1 Byte ohne Vorzeichen, `H` = 2 Byte ohne Vorzeichen, `I` = 4 Byte ohne
Vorzeichen. Aufzählungen (`TCenum`) sind 2 Byte groß, ihre Namen stehen in `TEXTS/TCMAINE?.TXT`
unter `#enum_<Name>E`.

| Typ | Name | Länge | Felder in Dateireihenfolge |
| --- | --- | --- | --- |
| 9900 | `Person` | 25 | PictID H, Job H (JobE), Sex H (SexE), Age B, Health B, Mood B, Intelligence B, Strength B, Stamina B, Loyality B, Skill B, Known B, Popularity B, Avarice B, Panic B, KnownToPolice B, TalkBits I, TalkFileID B, OldHealth B |
| 9901 | `Player` | 31 | Money I, StolenMoney I, MyStolenMoney I, NrOfBurglaries B, JobOfferCount B, MattsPart B, CurrScene I, CurrDay I, CurrMinute I, CurrLocation I |
| 9902 | `Car` | 24 | PictID H, Land H (LandE), Value I, YearOfConstruction H, ColorIndex H (ColorE), Strike B, Capacity I, PS B, Speed B, State B, MotorState B, BodyWorkState B, TyreState B, PlacesInCar B |
| 9903 | `Location` | 8 | LocationNr I, OpenFromMinute H, OpenToMinute H |
| 9904 | `Ability` | 6 | Name H (AbilityE), Use I |
| 9905 | `Item` | 22 | Type H (ItemE), OffsetFact H, HExactXOffset H, HExactYOffset H, HExactWidth H, HExactHeight H, VExactXOffset H, VExactYOffset H, VExactWidth H, VExactHeight H, Size B, ColorNr B |
| 9906 | `Tool` | 9 | PictID H, Value I, Danger B, Volume B, Effect B |
| 9907 | `Environment` | 7 oder 8 | MattHasHotelRoom B, MattHasIdentityCard B, WithOrWithoutYou B, MattIsInLove B, SouthhamptonHappened B, Present B, FirstTimeInSouth B, PostzugDone B (nur Profidisk) |
| 9908 | `London` | 1 | Useless B |
| 9909 | `Evidence` | 12 | pers I (Objektnummer), Recognition B, WalkTrail B, WaitTrail B, WorkTrail B, KillTrail B, CallTrail B, PaperTrail B, FotoTrail B |
| 9910 | `Loot` | 12 | Type H (LootE), Name H (LootNameE), Volume I, Weight H, PictID H |
| 9911 | `CompleteLoot` | 48 | Bild I, Gold I, Geld I, Juwelen I, Delikates I, Statue I, Kuriositaet I, HistKunst I, GebrauchsArt I, Vase I, TotalWeight I, TotalVolume I |
| 9912 | `LSOLock` | 2 | Type H (LockE) |
| 9913 | `LSObject` | 21 | us_OffsetFact H, us_DestX H, us_DestY H, uch_ExactX B, uch_ExactY B, uch_ExactX1 B, uch_ExactY1 B, uch_Size B, uch_Visible B, uch_Chained B, ul_Status I, Type I |
| 509990 | `Building` | 25 oder 26 | LocationNr I, PoliceTime H, GTime H, Exactlyness B, GRate B, Strike B, Values I, EscapeRoute H (RouteE), EscapeRouteLength B, RadioGuarding B, MaxVolume B, GuardStrength B, CarXPos H, CarYPos H, DiskId B (nur Profidisk) |
| 509991 | `Police` | 3 | PictID H, LivingID B |
| 509992 | `LSArea` | 49 | us_Coll16ID H, us_Coll32ID H, us_Coll48ID H, us_PlanColl16ID H, us_PlanColl32ID H, us_PlanColl48ID H, us_FloorCollID H, us_PlanFloorCollID H, us_Width H, us_Height H, ul_ObjectBaseNr I, uch_Darkness B, us_StartX0 bis us_StartX5 je H, us_StartY0 bis us_StartY5 je H |
| 509993 | `LSRoom` | 8 | us_LeftEdge H, us_TopEdge H, us_Width H, us_Height H |

Die Felder `PostzugDone` und `DiskId` liest das Spiel nur in der Profidisk-Fassung (`bProfidisk`).
Die mitgelieferten Daten sind Profidisk-Daten (`Building` 26, `Environment` 8 Byte). Das Werkzeug
erkennt die Fassung an `size`.

### Objekte im Grundriss (`LSObject`)

- `us_DestX`, `us_DestY`: Position in Pixeln im Stockwerk (bis 640 × 256).
- `uch_ExactX`, `uch_ExactY`, `uch_ExactX1`, `uch_ExactY1`: Kollisionsrechteck relativ zur Position.
- `uch_Size`: Kantenlänge der Grafik (16, 32 oder 48). Beim Laden setzt `lsPatchObjects` den Wert
  aus `Item.Size` der Objektart neu.
- `uch_Visible`: 1 sichtbar, 0 unsichtbar.
- `uch_Chained`: Bit 0 an Alarmanlage, Bit 1 an Stromkreis (`Const_tcCHAINED_TO_ALARM`, `..._POWER`).
- `ul_Status`: Bits nach `Const_tc*_BIT`: 0 waagrecht/senkrecht, 1 offen/zu, 2 abgeschlossen,
  3 mitnehmbar, 4 zugänglich, 5 in Arbeit, 6 an/aus. Höhere Bits (zum Beispiel Bit 16) kommen in den
  Daten vor, das Spiel wertet sie nicht aus.
- `Type`: Nummer des `Item`-Objekts in `TCMAIN.DAT`, zum Beispiel 9208 `Mauer`, 9201 `Holztür`.
- `us_OffsetFact`: Nummer des Bildausschnitts; bei den Beutesäcken (9701 bis 9708) die Bob-Nummer.

### Stockwerk (`LSArea`)

- `us_Coll16ID`, `us_Coll32ID`, `us_Coll48ID`, `us_FloorCollID`: Grafiksammlungen (Zeilen in
  `TEXTS/COLL.LST`) für 16er, 32er, 48er Objekte und den Boden, `us_PlanColl*`: dasselbe für die
  Planung.
- `us_StartX0..3`, `us_StartY0..3`: Startpunkte der bis zu vier Einbrecher (`src/planing/prepare.c`),
  `us_StartX4/5`, `us_StartY4/5`: Startpunkte der Wächter.
- `uch_Darkness`: Dunkelheit des Stockwerks.

## Relationen (`*.REL`, `*.GRE`)

Gelesen von `LoadRelations`, geschrieben von `SaveRelations` (`src/data/relation.c`). Textdatei,
eine Angabe je Zeile:

```
RELF                 Dateikennung (nur die ersten 4 Zeichen werden verglichen)
RTAB                 Beginn einer Tabelle
<Relationsnummer>
<linkes Objekt>      je Eintrag drei Zeilen
<rechtes Objekt>
<Parameter>
...
RTAB                 nächste Tabelle
...
```

- Eine Tabelle endet, sobald an der Stelle eines linken Objekts `RTAB` steht, oder am Dateiende.
  Eine Tabelle ohne Einträge ist erlaubt.
- Objekte stehen als Dezimalzahl der Objektnummer und müssen beim Laden schon existieren
  (`dbEncode`), sonst wird der Eintrag mit einem leeren Schlüssel angelegt und ins Log geschrieben.
- Der Parameter `-1` oder `4294967295` bedeutet „kein Parameter“ (`NO_PARAMETER`).
- Das Spiel hängt jeden geladenen Eintrag vorn an die Liste an (`SetP`). Beim Speichern eines
  Spielstands kehrt sich die Reihenfolge deshalb um, für das Spiel ist sie ohne Bedeutung.

Feste Relationen (aus `src/data/tcdata.h`):

| Nummer | Name | Nummer | Name |
| --- | --- | --- | --- |
| 1 | hasClock | 12 | hurt |
| 2 | ClockTimer | 13 | sound |
| 3 | StairConnects | 14 | opens |
| 4 | has | 15 | toolRequires |
| 5 | knows | 16 | taxi |
| 6 | likes_to_be | 17 | learned |
| 7 | join | 18 | remember |
| 8 | joined_by | 19 | hasLootBag |
| 9 | uses | 20 | PersonWorksHere |
| 10 | livesIn | 500001 | startsWith |
| 11 | break | 500002 | consistsOf |
| | | 500003 | isGuardedby |

Ein Stockwerk hat eigene Relationen mit den Nummern `ul_ObjectBaseNr` plus Versatz
(`REL_*_OFFSET` in `src/landscap/landscap.h`): +3 `consistsOf`, +4 `hasLock`, +5 `hasAlarm`,
+6 `hasPower`, +7 `hasLoot`, +10 `hasRoom`. In `hasLoot` steht als Parameter der Wert der Beute
(`src/present/presenta.c`). Die `.GRE`-Datei des Gebäudes enthält die Relationen 1 bis 3
(Stechuhren und Treppen), sie werden beim Verlassen wieder gelöscht (`lsDoneObjectDB`).

## Bodenraster (`*.LFD`)

Gelesen von `lsInitFloorSquares` mit `dskLoad` in einen Puffer von genau 160 Byte. Eine größere
Datei würde den Puffer überschreiben, die Datei muss also genau 160 Byte lang sein.

- 20 Felder je Zeile (640 / 32), 8 Zeilen (256 / 32), ein Byte je Feld von 32 × 32 Pixeln,
  Index = Zeile × 20 + Spalte.
- Bits 0 bis 3: Nummer der Bodenkachel in der Sammlung `us_FloorCollID` (`src/landscap/hardware.c`).
- Bit 5: kein Boden (`LS_NO_FLOOR`).
- Bit 6: Mikrofon auf dem Feld, Bit 7: Objekt auf dem Feld. Beide setzt das Spiel zur Laufzeit.

## Scheinwerfer (`*.SPO`)

Gelesen von `lsLoadSpots` (`src/landscap/spot.c`), eine Zahl je Zeile:

```
<Anzahl Spots>
  <Größe>            16, 32 oder 48 Pixel
  <Geschwindigkeit>  Sekunden je Bewegung
  <Steuerkasten>     Objektnummer, schaltet den Spot
  <Anzahl Positionen>
    <X>              je Position zwei Zeilen
    <Y>
```

## Wachenrouten und Pläne (`*.GUA`, `*.PLN`)

Beide bestehen aus Aktionslisten, geschrieben von `SaveHandler`, gelesen von `LoadHandler`
(`src/planing/system.c`). Eine Aktionsliste:

```
ACLI                 Beginn einer Aktionsliste
<Figur>              Objektnummer des Wächters oder Einbrechers
ACTI                 je Aktion: Kennung, Art, Dauer, Argumente
<Art>
<Dauer>
<Argumente>
```

| Art | Name | Argumente |
| --- | --- | --- |
| 1 | go | Richtung (1 links, 2 rechts, 4 hoch, 8 runter) |
| 2 | wait | keine |
| 3 | signal | Empfänger |
| 4 | wait_signal | Sender |
| 5 | use | Werkzeug, Objekt |
| 6 | take | Werkzeug, Objekt |
| 7 | drop | Werkzeug, Objekt |
| 8 | open | Objekt |
| 9 | close | Objekt |
| 10 | control | Objekt |

- Die Dauer zählt in Zeiteinheiten der Planung (laut Kommentar „seconds/3“). Bei `go` ist sie
  zugleich die Weglänge: `grdDraw` zieht je Einheit einen Pixel im halb so großen Grundriss.
- `LoadHandler` sucht die Liste einer Figur jedes Mal vom Dateianfang, die Reihenfolge der Listen
  ist also frei. Art 0 bricht das Laden ab.
- Eine `.GUA`-Datei enthält nur Aktionslisten, je Wächter eine (`grdDo`). Wächter laufen ihre
  Runde immer wieder (`SHF_AUTOREVERS`). Sie beginnen am Startpunkt `us_StartX4/5` des Stockwerks.
- Eine `.PLN`-Datei beginnt mit der Liste der Beteiligten und der Werkzeuge (`SaveSystem`,
  `plSaveTools` in `src/planing/io.c`), danach folgt je Einbrecher eine Aktionsliste:

```
SYS                  (mit Leerzeichen am Ende)
HAND                 je Beteiligter zwei Zeilen
<Objektnummer>
TOOB                 Beginn der Werkzeugliste
<Werkzeug>           je Zeile eine Objektnummer
TOOE                 Ende der Werkzeugliste
ACLI ...             Aktionslisten wie oben
```

## Planliste (`*.PLL`)

Gelesen und geschrieben von `plOpen` (`src/planing/io.c`). Die erste Zeile ist eine Bitmaske der
gespeicherten Pläne: Bit 0 steht für `<GEBÄUDE>1.PLN`, Bit 3 für `<GEBÄUDE>4.PLN`. Das Spiel
schreibt die Zahl ohne Zeilenende. Weitere Zeilen werden nicht gelesen. Eigene Pläne landen im
Benutzerordner unter `DATADISK`, die Originaldateien dienen nur als Vorgabe.

## Handlung (`TCSTORY.PC`)

Gelesen von `PrepareStory` und `LoadSceneforStory` (`src/gameplay/gp.c`). Kopf mit 48 Byte:

| Offset | Typ | Feld |
| --- | --- | --- |
| 0 | 20 Byte | StoryName, mit Null aufgefüllt |
| 20 | uint32 | EventCount |
| 24 | uint32 | SceneCount |
| 28 | uint32 | AmountOfScenes, Anzahl der folgenden Szenen |
| 32 | uint32 | AmountOfEvents |
| 36 | uint32 | StartZeit (Starttag) |
| 40 | uint32 | StartOrt |
| 44 | uint32 | StartSzene |

Danach `AmountOfScenes` Szenen zu je 89 Byte plus Listen:

| Offset | Typ | Feld |
| --- | --- | --- |
| 0 | uint32 | EventNr |
| 4 | 20 Byte | SceneName |
| 24 | int32 | Tag |
| 28 | int32 | MinZeitPunkt |
| 32 | int32 | MaxZeitPunkt |
| 36 | int32 | Ort, Bedingung |
| 40 | uint32 | Anzahl `events` (müssen geschehen sein) |
| 44 | uint32 | Anzahl `n_events` (dürfen nicht geschehen sein) |
| 48 | 2 × uint32 | Platz der Zeiger `events` und `n_events`, wird gelesen und verworfen |
| 56 | uint32 | Anzahl `nachfolger` |
| 60 | uint32 | Platz des Zeigers `nachfolger`, wird verworfen |
| 64 | uint32 | Moeglichkeiten (Menüpunkte) |
| 68 | uint32 | Dauer in Minuten |
| 72 | uint16 | Anzahl, wie oft die Szene geschehen kann |
| 74 | uint16 | Geschehen, wie oft schon |
| 76 | uint8 | Possibility, Wahrscheinlichkeit 0 bis 255 |
| 77 | uint32 | Sample |
| 81 | uint32 | Anim |
| 85 | int32 | NewOrt |
| 89 | uint32 je Eintrag | erst `events`, dann `n_events`, dann `nachfolger` |

Die Struktur `NewScene` im Speicher hat nach `Possibility` drei Füllbytes, die Datei nicht.
Die Namen (`StoryName`, `SceneName`) sind ISO 8859-1 wie auf dem Amiga, hinter der Null stehen
teils Reste älterer Namen; das Spiel liest die Namen ein, verwendet sie aber nicht. Nach
dem Laden ändert `PatchStory` einige Szenen im Code, das steht nicht in der Datei.

## Spieltexte (`TEXTS/*.TXT`)

Gelesen von `txtLoad` (`src/text/text.c`). Welche Dateien es gibt, steht in `TEXTS/TEXTS.LST`;
an den Namen hängt das Spiel den Sprachbuchstaben (`E`, `D`, `F`, `S`) und `.TXT` an.

- Jedes Byte ist mit `0x75` verknüpft (XOR). Nach dem Entschlüsseln ist der Text in Codepage 850
  (DOS), zum Beispiel `0x81` = „ü“.
- `txtLoad` ersetzt CR und LF durch Nullbytes, eine Zeile endet also an CR LF.
- `#Schlüssel` am Zeilenanfang beginnt einen Eintrag, `txtGoKey` vergleicht den ganzen Rest der
  Zeile. Die folgenden Zeilen bis zum nächsten `#` oder bis `^` sind der Text des Eintrags.
- Zeilen mit `;` am Anfang sind Kommentare und werden beim Lesen übersprungen.
- `^` am Zeilenanfang beendet die Datei.
- `TCMAINO?.TXT` enthält die Objektnamen, der Kommentar vor jedem Schlüssel nennt das Symbol aus
  `tcdata.h`. `TCMAINE?.TXT` enthält die Namen der Aufzählungen.

## Listen (`*.LST`)

Klartext ohne Verschlüsselung, Codepage 850. `ReadList` überspringt Zeilen mit `;` am Anfang und
leere Zeilen. Die Felder einer Zeile trennt ein Komma, `txtGetKey` holt das n-te Feld.

| Datei | Felder |
| --- | --- |
| `TEXTS/TEXTS.LST` | Name der Textdatei ohne Sprachbuchstaben, die Zeilennummer ist die Textnummer (`MENU_TXT` = 0, `OBJECTS_TXT` = 1, ...) |
| `TEXTS/COLL.LST` | Sammlungsnummer, Dateiname in `PICTURES`, Breite, Höhe, erste Farbe, letzte Farbe, Diskette (Profidisk) |
| `TEXTS/PICT.LST` | Bildnummer, Sammlungsnummer, X, Y, Breite, Höhe, Ziel-X, Ziel-Y |
| `TEXTS/TEMPLATE.LST` | Name der Figurenvorlage, Breite, Höhe, Nummer des ersten Bildes (`livLoadTemplates`, das fünfte Feld liest das Spiel nicht) |
| `TEXTS/LIVINGS.LST` | Name der Figur, Vorlage, Breite, Höhe, X-Geschwindigkeit, Y-Geschwindigkeit (`livAdd`) |
| `TEXTS/LOCATION.LST` | Name eines Ortes je Zeile, die Zeilennummer ist die Ortsnummer |
| `DATADISK/GAMES.LST`, `ORIGIN.LST` | Namen der Spielstände |

## Szenenstand eines Spielstands (`TCSTORYn.DAT`)

Geschrieben von `tcSaveChangesInScenes`, gelesen von `tcLoadChangesInScenes`
(`src/gameplay/loadsave.c`): erste Zeile `EnabledChoices`, dann je Szene zwei Zeilen `EventNr`
und `Geschehen`, zuletzt `gamemode <n>` (0 Original, 1 Spiel+).

## JSON des Werkzeugs

- `"format"` nennt das Format, `import` wählt danach den Schreiber.
- Schlüssel, die mit `_` beginnen (`_name`, `_symbol`, `_hints`, `_left`, `_map`, ...), sind nur
  Hinweise zum Lesen. `import` übergeht sie, sie dürfen fehlen oder falsch sein.
- `"layout"` hält bei Textdateien fest, wie die Zeilen enden (`crlf` oder `lf`), ob die letzte Zeile
  ein Zeilenende hat (`final_eol`) und ob ein Strg+Z folgt (`eof_mark`).
- Objekttypen stehen als Name (`"LSObject"`), Zahlen sind auch erlaubt. Zählerfelder (Anzahl der
  Spots, Positionen, Szenen, Ereignisse) werden beim Import aus den Listen berechnet.
- Was das Werkzeug nicht deuten kann, bleibt als `raw` (Bytes als Hex) oder `raw_lines` erhalten.

## Nicht unterstützt

| Dateien | Grund |
| --- | --- |
| `AUDIO/*.WAV`, `PLUS/VOICE/*.WAV`, `PLUS/AMBIENT/*.WAV`, `PLUS/WEATHER/*.WAV` | Sprache und Geräusche als RIFF WAVE (PCM, 8 oder 16 Bit, Mono oder Stereo) |
| `SOUNDS/*`, `SAMPLES/*` | Musik (HSC) und Klänge, keine Spieldaten im Sinne dieses Werkzeugs |
| `PICTURES/*`, `INTROPIX/*` | Bilder und Animationen (IFF ILBM, IFF ANIM, PNG), dafür gibt es Bildwerkzeuge |
| `PLUS/BALANCE.LUA`, `PLUS/NEWS.LUA`, `PLUS/WEATHER.LUA` | Lua-Skripte von Spiel+, reiner Text |
| `cosp.cfg` | Einstellungen, reiner Text |
