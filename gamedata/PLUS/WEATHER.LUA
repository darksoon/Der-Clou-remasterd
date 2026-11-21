-- Der Clou! remastered: weather of the burglary nights in Game+.
-- The weather only changes the picture and the sound of the burglary: rooms lit by flashlights,
-- moonlight through the windows, rain, fog and thunderstorms. Guards, alarms and times stay the same.
-- The original mode ignores this file. To change it, put a modified copy into MODFILES/PLUS/WEATHER.LUA.

-- Chances of each weather per night, as weights (they need not add up to 100).
weather = {
  clear = 30,  -- clear night, bright moonlight through the windows
  cloudy = 25, -- clouds, less moonlight
  rain = 20,
  storm = 10,  -- rain with lightning and thunder
  fog = 15,    -- haze, flashlights reach less far
}

-- Optional: decide the weather of a night yourself.
-- day: game day, building: number of the building, roll: a number from 0 to 9999 that is the same
-- for the same night and building (a reloaded game gets the same weather).
-- Return "clear", "cloudy", "rain", "storm" or "fog", or nil for the chances above.
function night_weather(day, building, roll)
  -- London: fog is more likely in the cold months (late autumn and winter)
  local d = day % 365
  if (d >= 300 or d < 60) and roll < 1500 then
    return "fog"
  end
  return nil
end
