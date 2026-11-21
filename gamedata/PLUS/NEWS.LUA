-- Der Clou! remastered: the newspaper after a burglary in Game+.
-- news(info) returns the lines of the article (UTF-8, at most 49 characters per line,
-- that is the width of the bubble; the longest building name has 24 characters).
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

-- 1234567 -> "1.234.567" (German) or "1,234,567" (English)
local function amount(value, separator)
  local s = tostring(math.floor(value))
  local out = s:reverse():gsub("(%d%d%d)", "%1" .. separator):reverse()
  if out:sub(1, 1) == separator then out = out:sub(2) end
  return out
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

-- The name of the building always stands alone ("Tatort: Kiosk."), so the sentences read right
-- for every name.
function news(info)
  local german = (info.lang == "de") -- French and Spanish read the English paper
  local t = german and de or en
  local lines = {}

  if not info.escaped then
    lines[1] = pick(t.caught, info, 1)
    if german then
      lines[2] = "In der vergangenen Nacht nahm die Polizei"
      lines[3] = "eine Einbrecherbande auf frischer Tat fest."
      lines[4] = "Tatort: " .. info.building .. "."
      lines[5] = "Inspektor Gludo spricht von einem"
      lines[6] = "Meisterstück der Ermittlungsarbeit."
    else
      lines[2] = "Last night the police arrested a gang of"
      lines[3] = "burglars in the act."
      lines[4] = "Scene of the crime: " .. info.building .. "."
      lines[5] = "Inspector Gludo calls it a masterpiece"
      lines[6] = "of detective work."
    end
    return lines
  end

  local kind = (info.alarm == "loud") and "loud" or (info.alarm == "silent") and "silent" or "clean"
  lines[1] = pick(t[kind], info, 3)

  if german then
    lines[2] = "In der Nacht schlugen unbekannte Einbrecher zu."
    lines[3] = "Tatort: " .. info.building .. "."
    if info.loot > 0 then
      lines[4] = "Der Schaden soll bei " .. amount(info.loot, ".") .. " Pfund liegen."
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

  lines[2] = "Unknown burglars struck last night."
  lines[3] = "Scene of the crime: " .. info.building .. "."
  if info.loot > 0 then
    lines[4] = "The loot is said to be worth " .. amount(info.loot, ",") .. " pounds."
  else
    lines[4] = "Apparently nothing was stolen."
  end
  if info.knocked_out > 0 then
    lines[#lines + 1] = "A guard was overpowered."
  end
  if kind == "clean" then
    lines[#lines + 1] = "Scotland Yard is groping in the dark."
  else
    lines[#lines + 1] = "The police are asking for information."
  end
  return lines
end
