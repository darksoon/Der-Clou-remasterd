-- Der Clou! remastered: the newspaper after a burglary in Game+.
-- news(info) returns the lines of the article (UTF-8, at most about 44 characters per line).
-- info.building  name of the building
-- info.escaped   true if the gang got away
-- info.alarm     "none", "silent" or "loud"
-- info.loot      value of what was taken, in pounds
-- info.knocked_out  how many burglars had to overpower someone
-- info.day       game day (use it to pick variants, so a reloaded game reads the same)
-- info.lang      "de", "en", "fr" or "es"
-- To change the articles, put a modified copy into MODFILES/PLUS/NEWS.LUA.

local function pick(list, info, salt)
  return list[((info.day * 7 + salt) % #list) + 1]
end

local function pounds(value)
  local s = tostring(value)
  local out = s:reverse():gsub("(%d%d%d)", "%1."):reverse()
  return (out:gsub("^%.", "")) .. " Pfund"
end

local de = {
  caught = {
    "BANDE AUF FRISCHER TAT GEFASST",
    "EINBRECHER LAUFEN POLIZEI IN DIE ARME",
    "SCOTLAND YARD SCHNAPPT DIE GANOVEN",
  },
  loud = {
    "ALARM! DIEBE ENTKOMMEN IN LETZTER MINUTE",
    "SIRENEN HEULTEN, DOCH DIE BANDE WAR SCHNELLER",
  },
  silent = {
    "STILLER ALARM KAM ZU SPÄT",
    "POLIZEI TRAF NUR NOCH LEERE RÄUME AN",
  },
  clean = {
    "MEISTERCOUP: NIEMAND HAT ETWAS BEMERKT",
    "SPURLOS VERSCHWUNDEN",
    "DREISTER EINBRUCH, POLIZEI RATLOS",
  },
}

local en = {
  caught = { "GANG CAUGHT RED-HANDED", "BURGLARS RUN INTO POLICE ARMS" },
  loud = { "ALARM! THIEVES ESCAPE AT THE LAST MINUTE" },
  silent = { "SILENT ALARM CAME TOO LATE" },
  clean = { "MASTER COUP: NOBODY NOTICED A THING", "VANISHED WITHOUT A TRACE" },
}

function news(info)
  local english = (info.lang == "en")
  local t = english and en or de
  local lines = {}

  if not info.escaped then
    lines[1] = pick(t.caught, info, 1)
    if english then
      lines[2] = "Last night the police arrested burglars"
      lines[3] = "at " .. info.building .. "."
    else
      lines[2] = "In der vergangenen Nacht nahm die Polizei"
      lines[3] = "Einbrecher bei " .. info.building .. " fest."
      lines[4] = "Inspektor Gludo spricht von einem"
      lines[5] = "Meisterstück der Ermittlungsarbeit."
    end
    return lines
  end

  local kind = (info.alarm == "loud") and "loud" or (info.alarm == "silent") and "silent" or "clean"
  lines[1] = pick(t[kind], info, 3)
  if english then
    lines[2] = "Burglars struck " .. info.building .. " last night."
    if info.loot > 0 then lines[3] = "The loot is said to be worth " .. info.loot .. " pounds." end
    return lines
  end

  lines[2] = "Unbekannte drangen letzte Nacht bei"
  lines[3] = info.building .. " ein."
  if info.loot > 0 then
    lines[4] = "Der Schaden soll bei " .. pounds(info.loot) .. " liegen."
  else
    lines[4] = "Gestohlen wurde offenbar nichts."
  end
  if info.knocked_out > 0 then
    lines[#lines + 1] = "Ein Wachmann wurde überwältigt."
  end
  if kind == "clean" then
    lines[#lines + 1] = "Scotland Yard tappt im Dunkeln."
  else
    lines[#lines + 1] = "Die Polizei bittet um Hinweise."
  end
  return lines
end
