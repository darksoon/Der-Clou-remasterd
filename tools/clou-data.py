#!/usr/bin/env python3
"""Read and write the data files of Der Clou! losslessly.

Every format is derived from the loader code of the game (src/data/database.c,
src/data/relation.c, src/landscap/init.c, src/landscap/spot.c, src/planing/system.c,
src/planing/io.c, src/gameplay/gp.c, src/text/text.c, src/list/list.c). The layout of
each format is described in docs/DATEIFORMATE.md.

Exported JSON files contain the data plus hints. Every key that starts with an
underscore ("_name", "_hints", ...) is only a hint for the reader and is ignored by
"import". Everything else is written back exactly, so export followed by import gives
the original file byte for byte.

Only the Python 3 standard library is used.
"""

import argparse
import json
import os
import re
import struct
import sys
from collections import Counter

TOOL_VERSION = 1

# ---------------------------------------------------------------------------
# constants taken from the C sources
# ---------------------------------------------------------------------------

# object types (src/data/tcdata.h) and their record layout in the order in which
# dbLoadOrSaveObject() reads them (src/data/database.c). All values are little endian.
# B = unsigned char, H = unsigned short, I = uint32_t
OBJECT_TYPES = {
    9900: ("Person", [
        ("PictID", "H"), ("Job", "H"), ("Sex", "H"), ("Age", "B"), ("Health", "B"),
        ("Mood", "B"), ("Intelligence", "B"), ("Strength", "B"), ("Stamina", "B"),
        ("Loyality", "B"), ("Skill", "B"), ("Known", "B"), ("Popularity", "B"),
        ("Avarice", "B"), ("Panic", "B"), ("KnownToPolice", "B"), ("TalkBits", "I"),
        ("TalkFileID", "B"), ("OldHealth", "B")]),
    9901: ("Player", [
        ("Money", "I"), ("StolenMoney", "I"), ("MyStolenMoney", "I"),
        ("NrOfBurglaries", "B"), ("JobOfferCount", "B"), ("MattsPart", "B"),
        ("CurrScene", "I"), ("CurrDay", "I"), ("CurrMinute", "I"), ("CurrLocation", "I")]),
    9902: ("Car", [
        ("PictID", "H"), ("Land", "H"), ("Value", "I"), ("YearOfConstruction", "H"),
        ("ColorIndex", "H"), ("Strike", "B"), ("Capacity", "I"), ("PS", "B"),
        ("Speed", "B"), ("State", "B"), ("MotorState", "B"), ("BodyWorkState", "B"),
        ("TyreState", "B"), ("PlacesInCar", "B")]),
    9903: ("Location", [("LocationNr", "I"), ("OpenFromMinute", "H"), ("OpenToMinute", "H")]),
    9904: ("Ability", [("Name", "H"), ("Use", "I")]),
    9905: ("Item", [
        ("Type", "H"), ("OffsetFact", "H"), ("HExactXOffset", "H"), ("HExactYOffset", "H"),
        ("HExactWidth", "H"), ("HExactHeight", "H"), ("VExactXOffset", "H"),
        ("VExactYOffset", "H"), ("VExactWidth", "H"), ("VExactHeight", "H"),
        ("Size", "B"), ("ColorNr", "B")]),
    9906: ("Tool", [("PictID", "H"), ("Value", "I"), ("Danger", "B"), ("Volume", "B"),
                    ("Effect", "B")]),
    9907: ("Environment", [
        ("MattHasHotelRoom", "B"), ("MattHasIdentityCard", "B"), ("WithOrWithoutYou", "B"),
        ("MattIsInLove", "B"), ("SouthhamptonHappened", "B"), ("Present", "B"),
        ("FirstTimeInSouth", "B"), ("PostzugDone", "B")]),
    9908: ("London", [("Useless", "B")]),
    9909: ("Evidence", [
        ("pers", "I"), ("Recognition", "B"), ("WalkTrail", "B"), ("WaitTrail", "B"),
        ("WorkTrail", "B"), ("KillTrail", "B"), ("CallTrail", "B"), ("PaperTrail", "B"),
        ("FotoTrail", "B")]),
    9910: ("Loot", [("Type", "H"), ("Name", "H"), ("Volume", "I"), ("Weight", "H"),
                    ("PictID", "H")]),
    9911: ("CompleteLoot", [
        ("Bild", "I"), ("Gold", "I"), ("Geld", "I"), ("Juwelen", "I"), ("Delikates", "I"),
        ("Statue", "I"), ("Kuriositaet", "I"), ("HistKunst", "I"), ("GebrauchsArt", "I"),
        ("Vase", "I"), ("TotalWeight", "I"), ("TotalVolume", "I")]),
    9912: ("LSOLock", [("Type", "H")]),
    9913: ("LSObject", [
        ("us_OffsetFact", "H"), ("us_DestX", "H"), ("us_DestY", "H"), ("uch_ExactX", "B"),
        ("uch_ExactY", "B"), ("uch_ExactX1", "B"), ("uch_ExactY1", "B"), ("uch_Size", "B"),
        ("uch_Visible", "B"), ("uch_Chained", "B"), ("ul_Status", "I"), ("Type", "I")]),
    509990: ("Building", [
        ("LocationNr", "I"), ("PoliceTime", "H"), ("GTime", "H"), ("Exactlyness", "B"),
        ("GRate", "B"), ("Strike", "B"), ("Values", "I"), ("EscapeRoute", "H"),
        ("EscapeRouteLength", "B"), ("RadioGuarding", "B"), ("MaxVolume", "B"),
        ("GuardStrength", "B"), ("CarXPos", "H"), ("CarYPos", "H"), ("DiskId", "B")]),
    509991: ("Police", [("PictID", "H"), ("LivingID", "B")]),
    509992: ("LSArea", [
        ("us_Coll16ID", "H"), ("us_Coll32ID", "H"), ("us_Coll48ID", "H"),
        ("us_PlanColl16ID", "H"), ("us_PlanColl32ID", "H"), ("us_PlanColl48ID", "H"),
        ("us_FloorCollID", "H"), ("us_PlanFloorCollID", "H"), ("us_Width", "H"),
        ("us_Height", "H"), ("ul_ObjectBaseNr", "I"), ("uch_Darkness", "B"),
        ("us_StartX0", "H"), ("us_StartX1", "H"), ("us_StartX2", "H"), ("us_StartX3", "H"),
        ("us_StartX4", "H"), ("us_StartX5", "H"), ("us_StartY0", "H"), ("us_StartY1", "H"),
        ("us_StartY2", "H"), ("us_StartY3", "H"), ("us_StartY4", "H"), ("us_StartY5", "H")]),
    509993: ("LSRoom", [("us_LeftEdge", "H"), ("us_TopEdge", "H"), ("us_Width", "H"),
                        ("us_Height", "H")]),
}
# fields only present in the Profidisk version (bProfidisk); the header size tells which variant
PROFIDISK_ONLY = {9907: "PostzugDone", 509990: "DiskId"}
TYPE_BY_NAME = {name: nr for nr, (name, _) in OBJECT_TYPES.items()}

# enum fields and the key of their names in the enum text file (TCMAINE?.TXT)
ENUM_FIELDS = {
    ("Person", "Job"): "enum_JobE", ("Person", "Sex"): "enum_SexE",
    ("Car", "Land"): "enum_LandE", ("Car", "ColorIndex"): "enum_ColorE",
    ("Ability", "Name"): "enum_AbilityE", ("Item", "Type"): "enum_ItemE",
    ("LSOLock", "Type"): "enum_LockE", ("Loot", "Type"): "enum_LootE",
    ("Loot", "Name"): "enum_LootNameE", ("Building", "EscapeRoute"): "enum_RouteE",
}
# fields that hold an object number
REF_FIELDS = {("LSObject", "Type"), ("Evidence", "pers")}

# bits of LSObject.ul_Status (Const_tc*_BIT in tcdata.h)
STATUS_BITS = {0: "horiz_vert", 1: "open_close", 2: "lock_unlock", 3: "take", 4: "access",
               5: "in_progress", 6: "on_off"}
CHAINED = {0: "none", 1: "alarm", 2: "power"}

# relation ids (Relation_* in tcdata.h)
RELATIONS = {
    1: "hasClock", 2: "ClockTimer", 3: "StairConnects", 4: "has", 5: "knows",
    6: "likes_to_be", 7: "join", 8: "joined_by", 9: "uses", 10: "livesIn", 11: "break",
    12: "hurt", 13: "sound", 14: "opens", 15: "toolRequires", 16: "taxi", 17: "learned",
    18: "remember", 19: "hasLootBag", 20: "PersonWorksHere",
    500001: "startsWith", 500002: "consistsOf", 500003: "isGuardedby",
}
NO_PARAMETER = 0xFFFFFFFF
# relations of one building area: ul_ObjectBaseNr of the area plus REL_*_OFFSET (src/landscap/landscap.h)
AREA_RELATIONS = {3: "consistsOf", 4: "hasLock", 5: "hasAlarm", 6: "hasPower", 7: "hasLoot",
                  10: "hasRoom"}


def relation_name(rid):
    if not isinstance(rid, int):
        return None
    if rid in RELATIONS:
        return RELATIONS[rid]
    if rid >= 600000 and rid % 10000 in AREA_RELATIONS:
        return "%s (area %d)" % (AREA_RELATIONS[rid % 10000], rid - rid % 10000)
    return None

# planning actions (src/planing/system.h) and the argument lines LoadHandler() reads
ACTIONS = {1: ("go", ["direction"]), 2: ("wait", []), 3: ("signal", ["item"]),
           4: ("wait_signal", ["item"]), 5: ("use", ["tool", "item"]),
           6: ("take", ["tool", "item"]), 7: ("drop", ["tool", "item"]),
           8: ("open", ["item"]), 9: ("close", ["item"]), 10: ("control", ["item"])}
ACTION_BY_NAME = {name: nr for nr, (name, _) in ACTIONS.items()}
DIRECTIONS = {0: "none", 1: "left", 2: "right", 4: "up", 8: "down"}
DIRECTION_BY_NAME = {v: k for k, v in DIRECTIONS.items()}

# floor squares (src/landscap/landscap.h): 640 / 32 by 256 / 32
FLOOR_COLUMNS = 20
FLOOR_ROWS = 8

TXT_XOR = 0x75  # TXT_XOR_VALUE in src/text/text_p.h

# files the tool does not handle, by directory or extension
UNSUPPORTED = [
    (lambda p: p.endswith(".WAV"), "Audio (RIFF WAVE), Standardformat"),
    (lambda p: "/AUDIO/" in p or "/SAMPLES/" in p or "/SOUNDS/" in p,
     "Musik und Klänge (HSC, VOC, Rohdaten), keine Spieldaten im Sinne dieses Werkzeugs"),
    (lambda p: "/PICTURES/" in p or "/INTROPIX/" in p,
     "Bilder und Animationen (IFF ILBM, IFF ANIM, PNG), mit Bildwerkzeugen bearbeiten"),
    (lambda p: p.endswith(".LUA"), "Lua-Skript von Spiel+, reiner Text"),
    (lambda p: p.endswith(".CFG"), "Einstellungsdatei, reiner Text"),
    (lambda p: p.endswith(".LOG"), "Logdatei des Spiels"),
]


class DataError(Exception):
    pass


# ---------------------------------------------------------------------------
# line based text files
# ---------------------------------------------------------------------------

def split_lines(text):
    """Split text into lines and a layout that restores the exact line endings."""
    parts = text.split("\n")
    rest = parts.pop()
    lines, eols = [], []
    for p in parts:
        if p.endswith("\r"):
            lines.append(p[:-1])
            eols.append("\r\n")
        else:
            lines.append(p)
            eols.append("\n")
    layout = {}
    if rest.endswith("\x1a"):  # DOS end of file mark
        layout["eof_mark"] = True
        rest = rest[:-1]
    if rest:
        lines.append(rest)
        layout["final_eol"] = False
    kinds = set(eols)
    if len(kinds) > 1:
        layout["eols"] = ["crlf" if e == "\r\n" else "lf" for e in eols]
    else:
        layout["eol"] = "lf" if kinds == {"\n"} else "crlf"
    return lines, layout


def join_lines(lines, layout):
    default = "\n" if layout.get("eol") == "lf" else "\r\n"
    eols = layout.get("eols")
    if eols is not None:
        eols = ["\n" if e == "lf" else "\r\n" for e in eols]
        if eols:
            default = Counter(eols).most_common(1)[0][0]
    out = []
    for i, line in enumerate(lines):
        if eols is not None and i < len(eols):
            eol = eols[i]
        else:
            eol = default
        if i == len(lines) - 1 and layout.get("final_eol") is False:
            eol = ""
        out.append(line + eol)
    if layout.get("eof_mark"):
        out.append("\x1a")
    return "".join(out)


def tok(s):
    """A number if the line is a plain decimal number, otherwise the text itself."""
    if re.fullmatch(r"-?[0-9]+", s) and str(int(s)) == s:
        return int(s)
    return s


def untok(v):
    if isinstance(v, bool):
        raise DataError("boolean where a number or text is expected")
    if isinstance(v, int):
        return str(v)
    if isinstance(v, str):
        return v
    raise DataError("number or text expected, got %r" % (v,))


# ---------------------------------------------------------------------------
# names and hints
# ---------------------------------------------------------------------------

class Context:
    """Names of objects, enum values and local objects of a building, used for hints."""

    def __init__(self, texts_dir=None, hints=True):
        self.hints = hints
        self.names = {}     # nr -> (name, symbol)
        self.enums = {}     # enum key -> list of names
        self.local = {}     # object nr inside a building area -> item type nr
        self.texts_dir = texts_dir
        self._loaded_texts = None
        self._loaded_local = set()

    def find_texts(self, path):
        if self.texts_dir:
            return self.texts_dir
        d = os.path.dirname(os.path.abspath(path))
        for cand in (os.path.join(d, "TEXTS"), os.path.join(os.path.dirname(d), "TEXTS")):
            if os.path.isdir(cand):
                return cand
        return None

    def load_for(self, path):
        if not self.hints:
            return
        texts = self.find_texts(path)
        if texts and texts != self._loaded_texts:
            self._loaded_texts = texts
            self._load_texts(texts)
        self._load_local(path)

    def _text_file(self, texts, base):
        # same order of languages as the game: English first, then German, French, Spanish
        for lang in "EDFS":
            p = os.path.join(texts, base + lang + ".TXT")
            if os.path.exists(p):
                return p
        return None

    def _load_texts(self, texts):
        p = self._text_file(texts, "TCMAINO")
        if p:
            doc = TextFormat.decode(read_file(p), self)
            for b in doc["blocks"]:
                key = tok(b["key"])
                if isinstance(key, int):
                    sym = None
                    for c in b.get("comments", []):
                        sym = c.lstrip("; ").strip() or sym
                    self.names[key] = (b["lines"][0] if b["lines"] else "", sym)
        p = self._text_file(texts, "TCMAINE")
        if p:
            doc = TextFormat.decode(read_file(p), self)
            for b in doc["blocks"]:
                self.enums[b["key"]] = [l.strip() for l in b["lines"] if not l.startswith(";")]

    def _load_local(self, path):
        """Collect the item types of the objects of the areas next to a building file."""
        d = os.path.dirname(os.path.abspath(path))
        base = os.path.basename(path).upper().rsplit(".", 1)[0]
        stem = base[:-1] if base[-1:].isdigit() else base
        if (d, stem) in self._loaded_local or not os.path.isdir(d):
            return
        self._loaded_local.add((d, stem))
        for f in sorted(os.listdir(d)):
            fu = f.upper()
            if fu.endswith(".DAT") and fu.startswith(stem) and len(fu) == len(stem) + 5 \
                    and fu[len(stem)].isdigit():
                try:
                    for o in ObjectFormat.parse(read_file(os.path.join(d, f))):
                        if o.get("type") == 9913 and "fields" in o:
                            self.local[o["nr"]] = o["fields"]["Type"]
                        elif o.get("type") == 509993:
                            self.local[o["nr"]] = "room"
                except DataError:
                    pass

    def name(self, nr):
        if not isinstance(nr, int):
            return None
        if nr in self.names:
            return self.names[nr][0]
        if nr in self.local:
            if self.local[nr] == "room":
                return "LSRoom (local)"
            item = self.names.get(self.local[nr])
            return "%s (local)" % item[0] if item else "local object"
        return None

    def enum(self, key, idx):
        vals = self.enums.get(key)
        if vals and isinstance(idx, int) and 0 <= idx < len(vals):
            return vals[idx]
        return None


def read_file(path):
    with open(path, "rb") as f:
        return f.read()


# ---------------------------------------------------------------------------
# formats
# ---------------------------------------------------------------------------

class ObjectFormat:
    """Object database: *.DAT (TCMAIN, TCBUILD and one per building area)."""
    name = "objects"
    title = "Objektdatenbank (*.DAT)"

    @staticmethod
    def parse(data):
        objs = []
        p = 0
        n = len(data)
        while p < n:
            if n - p < 12:
                objs.append({"trailing": data[p:].hex()})
                break
            nr, typ, size = struct.unpack_from("<III", data, p)
            p += 12
            if 0xFFFFFFFF in (nr, typ, size):
                # dbLoadAllObjects skips such headers and reads no data after them
                objs.append({"marker": [nr, typ, size]})
                continue
            o = {"nr": nr, "type": typ}
            layout = OBJECT_TYPES.get(typ)
            fields = None
            if layout:
                fl = list(layout[1])
                if typ in PROFIDISK_ONLY and size == struct.calcsize("<" + "".join(t for _, t in fl)) - 1:
                    fl = [f for f in fl if f[0] != PROFIDISK_ONLY[typ]]
                fmt = "<" + "".join(t for _, t in fl)
                if struct.calcsize(fmt) == size and p + size <= n:
                    vals = struct.unpack_from(fmt, data, p)
                    fields = dict(zip([f for f, _ in fl], vals))
            if fields is None:
                # unknown type or unexpected size: keep the bytes as they are
                if p + size > n:
                    raise DataError("object %d: size %d beyond end of file" % (nr, size))
                o["size"] = size
                o["raw"] = data[p:p + size].hex()
            else:
                o["fields"] = fields
            p += size
            objs.append(o)
        return objs

    @staticmethod
    def decode(data, ctx):
        objs = ObjectFormat.parse(data)
        for o in objs:
            if "fields" not in o:
                continue
            tname = OBJECT_TYPES[o["type"]][0]
            o["type"] = tname
            if not ctx.hints:
                continue
            nm = ctx.names.get(o["nr"])
            if nm:
                o["_name"] = nm[0]
                if nm[1]:
                    o["_symbol"] = nm[1]
            hints = {}
            for f, v in o["fields"].items():
                key = ENUM_FIELDS.get((tname, f))
                if key:
                    e = ctx.enum(key, v)
                    if e:
                        hints[f] = e
                if (tname, f) in REF_FIELDS:
                    e = ctx.name(v)
                    if e:
                        hints[f] = e
            if tname == "LSObject":
                st = o["fields"]["ul_Status"]
                bits = [STATUS_BITS.get(b, "bit%d" % b) for b in range(32) if st & (1 << b)]
                if bits:
                    hints["ul_Status"] = bits
                ch = o["fields"]["uch_Chained"]
                hints["uch_Chained"] = [CHAINED[b] for b in (1, 2) if ch & b] or ["none"]
            if hints:
                o["_hints"] = hints
        # reorder keys for readability
        out = []
        for o in objs:
            r = {}
            for k in ("nr", "type", "_name", "_symbol", "size", "fields", "_hints", "raw",
                      "marker", "trailing"):
                if k in o:
                    r[k] = o[k]
            out.append(r)
        return {"objects": out}

    @staticmethod
    def encode(doc):
        out = bytearray()
        for o in doc["objects"]:
            if "trailing" in o:
                out += bytes.fromhex(o["trailing"])
                continue
            if "marker" in o:
                out += struct.pack("<III", *o["marker"])
                continue
            typ = o["type"]
            if isinstance(typ, str):
                if typ not in TYPE_BY_NAME:
                    raise DataError("unknown object type %r" % typ)
                typ = TYPE_BY_NAME[typ]
            if "raw" in o:
                raw = bytes.fromhex(o["raw"])
                out += struct.pack("<III", o["nr"], typ, o.get("size", len(raw)))
                out += raw
                continue
            if typ not in OBJECT_TYPES:
                raise DataError("object %s: unknown type %r without raw data" % (o.get("nr"), typ))
            fl = list(OBJECT_TYPES[typ][1])
            fields = o["fields"]
            if typ in PROFIDISK_ONLY and PROFIDISK_ONLY[typ] not in fields:
                fl = [f for f in fl if f[0] != PROFIDISK_ONLY[typ]]
            fmt = "<" + "".join(t for _, t in fl)
            vals = []
            for f, t in fl:
                if f not in fields:
                    raise DataError("object %s: field %s missing" % (o.get("nr"), f))
                vals.append(fields[f])
            unknown = set(fields) - {f for f, _ in fl}
            if unknown:
                raise DataError("object %s: unknown fields %s" % (o.get("nr"), sorted(unknown)))
            try:
                body = struct.pack(fmt, *vals)
            except struct.error as e:
                raise DataError("object %s: %s" % (o.get("nr"), e))
            out += struct.pack("<III", o["nr"], typ, len(body)) + body
        return bytes(out)

    @staticmethod
    def info(doc):
        c = Counter(o.get("type", "marker") if "trailing" not in o else "trailing"
                    for o in doc["objects"])
        return "%d Objekte: %s" % (len(doc["objects"]),
                                   ", ".join("%s %d" % (k, v) for k, v in c.most_common()))


class LineFormat:
    """Base class for line based files: structured parse with a raw fallback."""
    encoding = "latin-1"
    xor = None

    @classmethod
    def decode(cls, data, ctx):
        if cls.xor is not None:
            data = bytes(b ^ cls.xor for b in data)
        lines, layout = split_lines(data.decode(cls.encoding))
        doc = {"layout": layout}
        try:
            body = cls.parse_lines(lines, ctx)
            if cls.unparse_lines(json.loads(json.dumps(body))) != lines:
                raise DataError("structure does not reproduce the file")
            doc.update(body)
        except (DataError, ValueError, IndexError, KeyError, TypeError):
            doc["raw_lines"] = lines
        return doc

    @classmethod
    def encode(cls, doc):
        if "raw_lines" in doc:
            lines = doc["raw_lines"]
        else:
            lines = cls.unparse_lines(doc)
        data = join_lines(lines, doc.get("layout", {})).encode(cls.encoding)
        if cls.xor is not None:
            data = bytes(b ^ cls.xor for b in data)
        return data

    @classmethod
    def info(cls, doc):
        if "raw_lines" in doc:
            return "%d Zeilen (nicht strukturiert, als Rohzeilen erhalten)" % len(doc["raw_lines"])
        return cls.summary(doc)


class RelationFormat(LineFormat):
    """Relations: *.REL and *.GRE (text, LoadRelations in src/data/relation.c)."""
    name = "relations"
    title = "Relationen (*.REL, *.GRE)"

    @staticmethod
    def parse_lines(lines, ctx):
        n = len(lines)
        doc = {"header": lines[0] if lines else None}
        i = 1
        pre = []
        while i < n and lines[i] != "RTAB":
            pre.append(lines[i])
            i += 1
        if pre:
            doc["preamble"] = pre
        tables = []
        while i < n:
            i += 1  # RTAB
            t = {}
            if i >= n:
                t["id"] = None
                tables.append(t)
                break
            t["id"] = tok(lines[i])
            i += 1
            if ctx and ctx.hints and relation_name(t["id"]):
                t["_name"] = relation_name(t["id"])
            entries = []
            while i < n and lines[i] != "RTAB":
                if n - i < 3:
                    t["rest"] = lines[i:]
                    i = n
                    break
                e = {"left": tok(lines[i]), "right": tok(lines[i + 1]), "param": tok(lines[i + 2])}
                if ctx and ctx.hints:
                    for k in ("left", "right"):
                        nm = ctx.name(e[k])
                        if nm:
                            e["_" + k] = nm
                    if e["param"] in (NO_PARAMETER, -1):
                        e["_param"] = "NO_PARAMETER"
                    elif ctx.name(e["param"]) and t["id"] == 500003:
                        e["_param"] = ctx.name(e["param"])
                entries.append(e)
                i += 3
            t["entries"] = entries
            tables.append(t)
        doc["tables"] = tables
        return doc

    @staticmethod
    def unparse_lines(doc):
        lines = []
        if doc.get("header") is not None:
            lines.append(doc["header"])
        lines += doc.get("preamble", [])
        for t in doc["tables"]:
            lines.append("RTAB")
            if t.get("id") is None:
                continue
            lines.append(untok(t["id"]))
            for e in t.get("entries", []):
                lines += [untok(e["left"]), untok(e["right"]), untok(e["param"])]
            lines += t.get("rest", [])
        return lines

    @staticmethod
    def summary(doc):
        return "%d Tabellen, %d Einträge" % (
            len(doc["tables"]), sum(len(t.get("entries", [])) for t in doc["tables"]))


def action_to_json(typ, time, args, ctx):
    a = {}
    if isinstance(typ, int) and typ in ACTIONS:
        name, argn = ACTIONS[typ]
        a["action"] = name
        a["time"] = time
        for k, v in zip(argn, args):
            if k == "direction" and v in DIRECTIONS:
                v = DIRECTIONS[v]
            a[k] = v
            if ctx and ctx.hints and k in ("tool", "item"):
                nm = ctx.name(v)
                if nm:
                    a["_" + k] = nm
    else:
        a["action"] = typ
        a["time"] = time
    return a


def action_to_lines(a):
    typ = a["action"]
    argn = []
    if isinstance(typ, str) and typ in ACTION_BY_NAME:
        typ = ACTION_BY_NAME[typ]
        argn = ACTIONS[typ][1]
    lines = ["ACTI", untok(typ), untok(a["time"])]
    for k in argn:
        v = a[k]
        if k == "direction" and isinstance(v, str) and v in DIRECTION_BY_NAME:
            v = DIRECTION_BY_NAME[v]
        lines.append(untok(v))
    return lines


class ActionFormat(LineFormat):
    """Guard routes (*.GUA) and burglary plans (*.PLN): SaveHandler in src/planing/system.c."""
    name = "actions"
    title = "Wachenrouten und Einbruchspläne (*.GUA, *.PLN)"

    @staticmethod
    def parse_lines(lines, ctx):
        n = len(lines)
        items = []
        i = 0

        def raw(line):
            if items and "raw" in items[-1]:
                items[-1]["raw"].append(line)
            else:
                items.append({"raw": [line]})

        while i < n:
            l = lines[i]
            if l == "SYS ":
                i += 1
                hs = []
                while i + 1 < n and lines[i] == "HAND":
                    hs.append(tok(lines[i + 1]))
                    i += 2
                items.append({"system": hs})
            elif l == "TOOB":
                i += 1
                tools = []
                while i < n and lines[i] != "TOOE":
                    tools.append(tok(lines[i]))
                    i += 1
                it = {"tools": tools}
                if i < n:
                    i += 1
                else:
                    it["unterminated"] = True
                items.append(it)
            elif l == "ACLI" and i + 1 < n:
                hid = tok(lines[i + 1])
                i += 2
                acts = []
                while i + 2 < n and lines[i] == "ACTI":
                    typ = tok(lines[i + 1])
                    time = tok(lines[i + 2])
                    argn = ACTIONS[typ][1] if isinstance(typ, int) and typ in ACTIONS else []
                    if i + 3 + len(argn) > n:
                        break
                    args = [tok(x) for x in lines[i + 3:i + 3 + len(argn)]]
                    acts.append(action_to_json(typ, time, args, ctx))
                    i += 3 + len(argn)
                h = {"handler": hid}
                if ctx and ctx.hints and ctx.name(hid):
                    h["_name"] = ctx.name(hid)
                h["actions"] = acts
                if ctx and ctx.hints:
                    h["_total_time"] = sum(a["time"] for a in acts if isinstance(a["time"], int))
                items.append(h)
            else:
                raw(l)
                i += 1
        return {"items": items}

    @staticmethod
    def unparse_lines(doc):
        lines = []
        for it in doc["items"]:
            if "raw" in it:
                lines += it["raw"]
            elif "system" in it:
                lines.append("SYS ")
                for h in it["system"]:
                    lines += ["HAND", untok(h)]
            elif "tools" in it:
                lines.append("TOOB")
                lines += [untok(t) for t in it["tools"]]
                if not it.get("unterminated"):
                    lines.append("TOOE")
            elif "handler" in it:
                lines += ["ACLI", untok(it["handler"])]
                for a in it["actions"]:
                    lines += action_to_lines(a)
            else:
                raise DataError("unknown item %r" % (it,))
        return lines

    @staticmethod
    def summary(doc):
        hs = [it for it in doc["items"] if "handler" in it]
        return "%d Aktionslisten mit %d Aktionen" % (len(hs), sum(len(h["actions"]) for h in hs))


class SpotFormat(LineFormat):
    """Spot lights and cameras: *.SPO (lsLoadSpots in src/landscap/spot.c)."""
    name = "spots"
    title = "Scheinwerfer und Kameras (*.SPO)"

    @staticmethod
    def parse_lines(lines, ctx):
        count = int(lines[0])
        i = 1
        spots = []
        for _ in range(count):
            s = {"size": tok(lines[i]), "speed": tok(lines[i + 1]), "control": tok(lines[i + 2])}
            pc = int(lines[i + 3])
            i += 4
            if ctx and ctx.hints and ctx.name(s["control"]):
                s["_control"] = ctx.name(s["control"])
            s["positions"] = [[tok(lines[i + 2 * k]), tok(lines[i + 2 * k + 1])] for k in range(pc)]
            if len(lines) < i + 2 * pc:
                raise DataError("spot positions beyond end of file")
            i += 2 * pc
            spots.append(s)
        doc = {"spots": spots}
        if i < len(lines):
            doc["rest"] = lines[i:]
        return doc

    @staticmethod
    def unparse_lines(doc):
        lines = [str(len(doc["spots"]))]
        for s in doc["spots"]:
            lines += [untok(s["size"]), untok(s["speed"]), untok(s["control"]),
                      str(len(s["positions"]))]
            for x, y in s["positions"]:
                lines += [untok(x), untok(y)]
        return lines + doc.get("rest", [])

    @staticmethod
    def summary(doc):
        return "%d Spots" % len(doc["spots"])


class PlanListFormat(LineFormat):
    """List of saved plans: *.PLL (plOpen in src/planing/io.c)."""
    name = "planlist"
    title = "Planliste (*.PLL)"

    @staticmethod
    def parse_lines(lines, ctx):
        mask = tok(lines[0])
        doc = {"plans": mask}
        if ctx and ctx.hints and isinstance(mask, int):
            doc["_saved_plans"] = [b + 1 for b in range(32) if mask & (1 << b)]
        if len(lines) > 1:
            doc["rest"] = lines[1:]
        return doc

    @staticmethod
    def unparse_lines(doc):
        return [untok(doc["plans"])] + doc.get("rest", [])

    @staticmethod
    def summary(doc):
        m = doc["plans"]
        return "Pläne gespeichert: %s" % ([b + 1 for b in range(32) if isinstance(m, int) and m & (1 << b)] or "keine")


class ListFormat(LineFormat):
    """Plain lists: *.LST (ReadList in src/list/list.c and the special readers)."""
    name = "list"
    title = "Listen (*.LST)"
    encoding = "cp850"

    @staticmethod
    def parse_lines(lines, ctx):
        return {"lines": lines}

    @staticmethod
    def unparse_lines(doc):
        return list(doc["lines"])

    @staticmethod
    def summary(doc):
        return "%d Zeilen, davon %d Einträge" % (
            len(doc["lines"]), sum(1 for l in doc["lines"] if l and not l.startswith(";")))


class TextFormat(LineFormat):
    """Game texts: TEXTS/*.TXT, XOR 0x75 and code page 850 (txtLoad in src/text/text.c)."""
    name = "text"
    title = "Spieltexte (TEXTS/*.TXT)"
    encoding = "cp850"
    xor = TXT_XOR

    @staticmethod
    def parse_lines(lines, ctx):
        pre = []
        blocks = []
        pending = []
        end = None
        for idx, l in enumerate(lines):
            if l.startswith("^"):
                end = lines[idx:]
                break
            if l.startswith("#"):
                j = len(pending)
                while j > 0 and pending[j - 1].startswith(";"):
                    j -= 1
                if blocks:
                    blocks[-1]["lines"] = pending[:j]
                else:
                    pre = pending[:j]
                b = {}
                if pending[j:]:
                    b["comments"] = pending[j:]
                b["key"] = l[1:]
                b["lines"] = []
                blocks.append(b)
                pending = []
            else:
                pending.append(l)
        if blocks:
            blocks[-1]["lines"] = pending
        else:
            pre = pending
        doc = {}
        if pre:
            doc["preamble"] = pre
        doc["blocks"] = blocks
        if end is not None:
            doc["end"] = end
        return doc

    @staticmethod
    def unparse_lines(doc):
        lines = list(doc.get("preamble", []))
        for b in doc["blocks"]:
            lines += b.get("comments", [])
            lines.append("#" + b["key"])
            lines += b["lines"]
        lines += doc.get("end", [])
        return lines

    @staticmethod
    def summary(doc):
        return "%d Schlüssel" % len(doc["blocks"])


class SceneStateFormat(LineFormat):
    """Scene state of a saved game: TCSTORYn.DAT (tcSaveChangesInScenes in src/gameplay/loadsave.c)."""
    name = "scenestate"
    title = "Szenenstand eines Spielstands (TCSTORYn.DAT)"

    @staticmethod
    def parse_lines(lines, ctx):
        doc = {"EnabledChoices": tok(lines[0]), "scenes": []}
        i = 1
        while i + 1 < len(lines) and not lines[i].startswith("gamemode"):
            doc["scenes"].append({"EventNr": tok(lines[i]), "Geschehen": tok(lines[i + 1])})
            i += 2
        if i < len(lines):
            doc["rest"] = lines[i:]
        return doc

    @staticmethod
    def unparse_lines(doc):
        lines = [untok(doc["EnabledChoices"])]
        for s in doc["scenes"]:
            lines += [untok(s["EventNr"]), untok(s["Geschehen"])]
        return lines + doc.get("rest", [])

    @staticmethod
    def summary(doc):
        return "%d Szenen" % len(doc["scenes"])


class FloorFormat:
    """Floor squares of an area: *.LFD (lsInitFloorSquares in src/landscap/init.c)."""
    name = "floor"
    title = "Bodenraster (*.LFD)"

    @staticmethod
    def decode(data, ctx):
        w = FLOOR_COLUMNS
        rows = [list(data[i:i + w]) for i in range(0, len(data) - len(data) % w, w)]
        doc = {"columns": w, "rows": rows}
        if len(data) % w:
            doc["trailing"] = data[len(data) - len(data) % w:].hex()
        if ctx.hints:
            def cell(v):
                if v & 0x20:
                    return "."
                return "%x%s" % (v & 0x0F, "o" if v & 0x80 else ("m" if v & 0x40 else " "))
            doc["_map"] = [" ".join(cell(v) for v in r) for r in rows]
            doc["_legend"] = ". = kein Boden (Bit 5), Ziffer = Bodenkachel (Bits 0-3), " \
                             "o = Objekt (Bit 7), m = Mikrofon (Bit 6)"
        return doc

    @staticmethod
    def encode(doc):
        out = bytearray()
        for r in doc["rows"]:
            out += bytes(r)
        if "trailing" in doc:
            out += bytes.fromhex(doc["trailing"])
        return bytes(out)

    @staticmethod
    def info(doc):
        n = sum(len(r) for r in doc["rows"])
        empty = sum(1 for r in doc["rows"] for v in r if v & 0x20)
        return "%d Felder (%d x %d), %d ohne Boden" % (n, doc["columns"], len(doc["rows"]), empty)


STORY_HEADER = [("EventCount", "I"), ("SceneCount", "I"), ("AmountOfScenes", "I"),
                ("AmountOfEvents", "I"), ("StartZeit", "I"), ("StartOrt", "I"), ("StartSzene", "I")]
SCENE_A = [("Tag", "i"), ("MinZeitPunkt", "i"), ("MaxZeitPunkt", "i"), ("Ort", "i")]
SCENE_B = [("Moeglichkeiten", "I"), ("Dauer", "I"), ("Anzahl", "H"), ("Geschehen", "H"),
           ("Possibility", "B"), ("Sample", "I"), ("Anim", "I"), ("NewOrt", "i")]


def fixed_str_to_json(raw):
    s, _, pad = raw.partition(b"\0")
    r = {"text": s.decode("latin-1")}
    if raw.find(b"\0") >= 0 and any(pad):
        r["pad"] = pad.hex()
    if raw.find(b"\0") < 0:
        r["unterminated"] = True
    return r


def fixed_str_from_json(v, size):
    b = v["text"].encode("latin-1")
    if not v.get("unterminated"):
        b += b"\0" + bytes.fromhex(v.get("pad", ""))
    if len(b) > size:
        raise DataError("text %r longer than %d bytes" % (v["text"], size))
    return b.ljust(size, b"\0")


class StoryFormat:
    """Story: DATA/TCSTORY.PC (PrepareStory and LoadSceneforStory in src/gameplay/gp.c)."""
    name = "story"
    title = "Handlung (TCSTORY.PC)"

    @staticmethod
    def decode(data, ctx):
        if len(data) < 48:
            raise DataError("story header too short")
        doc = {"StoryName": fixed_str_to_json(data[:20])}
        vals = struct.unpack_from("<7I", data, 20)
        hdr = dict(zip([f for f, _ in STORY_HEADER], vals))
        amount = hdr.pop("AmountOfScenes")
        doc.update(hdr)
        p = 48
        scenes = []
        for _ in range(amount):
            if p + 89 > len(data):
                raise DataError("scene beyond end of file")
            s = {"EventNr": struct.unpack_from("<I", data, p)[0],
                 "SceneName": fixed_str_to_json(data[p + 4:p + 24])}
            p += 24
            s.update(zip([f for f, _ in SCENE_A], struct.unpack_from("<4i", data, p)))
            p += 16
            ne, nn, t1, t2, nf, t3 = struct.unpack_from("<6I", data, p)
            p += 24
            s.update(zip([f for f, _ in SCENE_B], struct.unpack_from("<IIHHBIIi", data, p)))
            p += 25
            lists = []
            for cnt in (ne, nn, nf):
                if p + 4 * cnt > len(data):
                    raise DataError("event list beyond end of file")
                lists.append(list(struct.unpack_from("<%dI" % cnt, data, p)))
                p += 4 * cnt
            s["events"], s["n_events"], s["nachfolger"] = lists
            # slots of the pointers events, n_events and nachfolger; the game reads and ignores them
            s["pointer_slots"] = [t1, t2, t3]
            scenes.append(s)
        doc["scenes"] = scenes
        if p < len(data):
            doc["trailing"] = data[p:].hex()
        return doc

    @staticmethod
    def encode(doc):
        hdr = dict(doc)
        hdr["AmountOfScenes"] = len(doc["scenes"])
        out = bytearray(fixed_str_from_json(doc["StoryName"], 20))
        out += struct.pack("<7I", *[hdr[f] for f, _ in STORY_HEADER])
        for s in doc["scenes"]:
            t1, t2, t3 = s.get("pointer_slots", [0, 0, 0])
            out += struct.pack("<I", s["EventNr"]) + fixed_str_from_json(s["SceneName"], 20)
            out += struct.pack("<4i", *[s[f] for f, _ in SCENE_A])
            out += struct.pack("<6I", len(s["events"]), len(s["n_events"]), t1, t2,
                               len(s["nachfolger"]), t3)
            out += struct.pack("<IIHHBIIi", *[s[f] for f, _ in SCENE_B])
            for lst in (s["events"], s["n_events"], s["nachfolger"]):
                out += struct.pack("<%dI" % len(lst), *lst)
        if "trailing" in doc:
            out += bytes.fromhex(doc["trailing"])
        return bytes(out)

    @staticmethod
    def info(doc):
        return "%d Szenen, Start: Tag %d, Ort %d, Szene %d" % (
            len(doc["scenes"]), doc["StartZeit"], doc["StartOrt"], doc["StartSzene"])


FORMATS = {f.name: f for f in (ObjectFormat, RelationFormat, ActionFormat, SpotFormat,
                               PlanListFormat, ListFormat, TextFormat, FloorFormat, StoryFormat,
                               SceneStateFormat)}

EXTENSIONS = {".DAT": "objects", ".REL": "relations", ".GRE": "relations", ".GUA": "actions",
              ".PLN": "actions", ".SPO": "spots", ".PLL": "planlist", ".LST": "list",
              ".TXT": "text", ".LFD": "floor", ".PC": "story"}


def detect(path):
    if re.fullmatch(r"TCSTORY[0-9]+\.DAT", os.path.basename(path).upper()):
        return SceneStateFormat
    ext = os.path.splitext(path)[1].upper()
    name = EXTENSIONS.get(ext)
    return FORMATS[name] if name else None


def unsupported_reason(path):
    p = path.replace(os.sep, "/").upper()
    for test, reason in UNSUPPORTED:
        if test(p):
            return reason
    return "unbekanntes Format"


# ---------------------------------------------------------------------------
# commands
# ---------------------------------------------------------------------------

def export_doc(path, ctx):
    fmt = detect(path)
    if not fmt:
        raise DataError("%s: format not supported (%s)" % (path, unsupported_reason(path)))
    ctx.load_for(path)
    doc = {"format": fmt.name, "tool_version": TOOL_VERSION,
           "_file": os.path.basename(path)}
    doc.update(fmt.decode(read_file(path), ctx))
    return doc


def strip_hints(v):
    if isinstance(v, dict):
        return {k: strip_hints(x) for k, x in v.items() if not k.startswith("_")}
    if isinstance(v, list):
        return [strip_hints(x) for x in v]
    return v


def import_doc(doc):
    if not isinstance(doc, dict) or doc.get("format") not in FORMATS:
        raise DataError("not an exported data file (missing or unknown \"format\")")
    return FORMATS[doc["format"]].encode(strip_hints(doc))


def dump_json(doc):
    return json.dumps(doc, ensure_ascii=False, indent=1) + "\n"


def roundtrip_file(path, ctx):
    original = read_file(path)
    text = dump_json(export_doc(path, ctx))
    rebuilt = import_doc(json.loads(text))
    if rebuilt == original:
        return True, ""
    for i, (a, b) in enumerate(zip(original, rebuilt)):
        if a != b:
            return False, "first difference at byte %d" % i
    return False, "length %d instead of %d" % (len(rebuilt), len(original))


def cmd_info(args):
    ctx = Context(args.texts)
    for path in args.files:
        doc = export_doc(path, ctx)
        fmt = FORMATS[doc["format"]]
        print("%s: %s, %d Bytes, %s" % (path, fmt.title, os.path.getsize(path), fmt.info(doc)))
    return 0


def cmd_export(args):
    ctx = Context(args.texts, hints=not args.no_hints)
    text = dump_json(export_doc(args.file, ctx))
    if args.output and args.output != "-":
        with open(args.output, "w", encoding="utf-8") as f:
            f.write(text)
    else:
        sys.stdout.write(text)
    return 0


def cmd_import(args):
    if args.file == "-":
        doc = json.load(sys.stdin)
    else:
        with open(args.file, encoding="utf-8") as f:
            doc = json.load(f)
    data = import_doc(doc)
    with open(args.output, "wb") as f:
        f.write(data)
    print("%s: %d Bytes geschrieben" % (args.output, len(data)))
    return 0


def cmd_roundtrip(args):
    ctx = Context(args.texts)
    bad = 0
    for path in args.files:
        ok, why = roundtrip_file(path, ctx)
        print("%s %s %s" % ("OK  " if ok else "FAIL", path, why))
        bad += not ok
    return 1 if bad else 0


def cmd_roundtrip_all(args):
    ctx = Context(args.texts)
    results = Counter()
    raw = []
    failed = []
    unsupported = Counter()
    for root, dirs, files in os.walk(args.dir):
        dirs.sort()
        for f in sorted(files):
            path = os.path.join(root, f)
            fmt = detect(path)
            if not fmt:
                unsupported[unsupported_reason(path)] += 1
                if args.verbose:
                    print("SKIP %s (%s)" % (path, unsupported_reason(path)))
                continue
            try:
                ok, why = roundtrip_file(path, ctx)
                doc = export_doc(path, ctx)
                if "raw_lines" in doc:
                    raw.append(path)
            except (DataError, ValueError, KeyError, TypeError) as e:
                ok, why = False, str(e)
            results[(fmt.name, ok)] += 1
            if not ok:
                failed.append((path, why))
                print("FAIL %s: %s" % (path, why))
            elif args.verbose:
                print("OK   %s" % path)
    total_ok = sum(v for (_, ok), v in results.items() if ok)
    print("Round trip: %d Dateien byte-identisch, %d Fehler" % (total_ok, len(failed)))
    for name in sorted({n for n, _ in results}):
        print("  %-10s %4d OK %4d Fehler" % (name, results[(name, True)], results[(name, False)]))
    if raw:
        print("Nur als Rohzeilen erhalten (Struktur nicht erkannt): %d" % len(raw))
        for p in raw:
            print("  " + p)
    if unsupported:
        print("Nicht unterstützt: %d Dateien" % sum(unsupported.values()))
        for reason, n in unsupported.most_common():
            print("  %4d  %s" % (n, reason))
    return 1 if failed else 0


def load_db(gamedata, name, ctx):
    path = os.path.join(gamedata, "DATA", name)
    ctx.load_for(path)
    return ObjectFormat.decode(read_file(path), ctx)["objects"]


def cmd_building(args):
    """Collect everything that describes one building from the files of a game directory."""
    g = args.gamedata
    ctx = Context(args.texts or os.path.join(g, "TEXTS"))
    build = {o["nr"]: o for o in load_db(g, "TCBUILD.DAT", ctx) if "nr" in o}
    rel = RelationFormat.decode(read_file(os.path.join(g, "DATA", "TCBUILD.REL")), ctx)
    tables = {t["id"]: t.get("entries", []) for t in rel.get("tables", [])}
    buildings = [o for o in build.values() if o["type"] == "Building"]
    if not args.name:
        for b in buildings:
            areas = [ctx.name(e["right"]) for e in tables.get(500002, []) if e["left"] == b["nr"]]
            print("%d  %-32s %s" % (b["nr"], b.get("_name", "?"), ", ".join(a or "?" for a in areas)))
        return 0
    sel = [b for b in buildings if str(b["nr"]) == args.name or
           args.name.lower() in b.get("_name", "").lower()]
    if len(sel) != 1:
        raise DataError("%d buildings match %r" % (len(sel), args.name))
    b = sel[0]
    out = {"building": b, "areas": [], "guards": []}
    for e in tables.get(500003, []):
        if e["left"] == b["nr"]:
            out["guards"].append({"police": e["right"], "_name": ctx.name(e["right"]),
                                  "area": e["param"], "police_object": build.get(e["right"])})
    stem = None
    for e in tables.get(500002, []):
        if e["left"] != b["nr"]:
            continue
        area = build.get(e["right"], {"nr": e["right"]})
        aname = (area.get("_name") or "").upper()
        stem = aname[:-1]
        files = {}
        for ext in (".DAT", ".REL", ".LFD"):
            p = os.path.join(g, "DATA", aname + ext)
            if os.path.exists(p):
                files[ext[1:]] = os.path.relpath(p, g)
        a = {"area": area, "files": files}
        if "DAT" in files:
            objs = ObjectFormat.decode(read_file(os.path.join(g, files["DAT"])), ctx)["objects"]
            kinds = Counter()
            for o in objs:
                if o.get("type") == "LSObject":
                    kinds[o.get("_hints", {}).get("Type", str(o["fields"]["Type"]))] += 1
            a["objects"] = dict(kinds.most_common())
            a["rooms"] = [o["fields"] for o in objs if o.get("type") == "LSRoom"]
        if "LFD" in files:
            a["floor_map"] = FloorFormat.decode(read_file(os.path.join(g, files["LFD"])), ctx)["_map"]
        out["areas"].append(a)
    if stem:
        files = {}
        for d, ext in (("DATA", ".GRE"), ("DATA", ".SPO"), ("DATA", ".GUA"), ("DATADISK", ".PLL")):
            p = os.path.join(g, d, stem + ext)
            if os.path.exists(p):
                files[ext[1:]] = os.path.relpath(p, g)
        plans = sorted(f for f in os.listdir(os.path.join(g, "DATADISK"))
                       if re.fullmatch(re.escape(stem) + r"[0-9]+\.PLN", f.upper()))
        if plans:
            files["PLN"] = [os.path.join("DATADISK", f) for f in plans]
        out["files"] = files
        if "GUA" in files:
            gua = ActionFormat.decode(read_file(os.path.join(g, files["GUA"])), ctx)
            out["guard_routes"] = [{"handler": it["handler"], "actions": len(it["actions"]),
                                    "total_time": it.get("_total_time")}
                                   for it in gua.get("items", []) if "handler" in it]
        if "SPO" in files:
            spo = SpotFormat.decode(read_file(os.path.join(g, files["SPO"])), ctx)
            out["spots"] = len(spo.get("spots", []))
    sys.stdout.write(dump_json(out))
    return 0


def cmd_formats(args):
    for name, f in FORMATS.items():
        exts = [e for e, n in EXTENSIONS.items() if n == name] or ["TCSTORYn.DAT"]
        print("%-10s %-48s %s" % (name, f.title, " ".join(exts)))
    return 0


def main(argv=None):
    ap = argparse.ArgumentParser(
        prog="clou-data.py",
        description="Read and write Der Clou! data files losslessly (JSON export and import).")
    ap.add_argument("--texts", help="TEXTS directory used for names (default: next to the file)")
    sub = ap.add_subparsers(dest="cmd", required=True)

    p = sub.add_parser("info", help="short summary of data files")
    p.add_argument("files", nargs="+")
    p.set_defaults(func=cmd_info)

    p = sub.add_parser("export", help="write a data file as JSON")
    p.add_argument("file")
    p.add_argument("-o", "--output", help="JSON file (default: standard output)")
    p.add_argument("--no-hints", action="store_true", help="leave out names and other hints")
    p.set_defaults(func=cmd_export)

    p = sub.add_parser("import", help="build a data file from JSON")
    p.add_argument("file", help="JSON file written by export ('-' for standard input)")
    p.add_argument("-o", "--output", required=True, help="data file to write")
    p.set_defaults(func=cmd_import)

    p = sub.add_parser("roundtrip", help="export and import, check that the bytes are identical")
    p.add_argument("files", nargs="+")
    p.set_defaults(func=cmd_roundtrip)

    p = sub.add_parser("roundtrip-all", help="round trip every supported file below a directory")
    p.add_argument("dir")
    p.add_argument("-v", "--verbose", action="store_true")
    p.set_defaults(func=cmd_roundtrip_all)

    p = sub.add_parser("building", help="list buildings or show all data of one building")
    p.add_argument("gamedata", help="game directory (with DATA, DATADISK and TEXTS)")
    p.add_argument("name", nargs="?", help="number or part of the name of the building")
    p.set_defaults(func=cmd_building)

    p = sub.add_parser("formats", help="list the supported formats")
    p.set_defaults(func=cmd_formats)

    args = ap.parse_args(argv)
    try:
        return args.func(args)
    except (DataError, OSError, json.JSONDecodeError) as e:
        print("clou-data.py: %s" % e, file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
