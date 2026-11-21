-- Der Clou! remastered: balance of the Game+ mode.
-- Percentages, 100 = like the original. Only used in Game+, the original mode ignores this file.
-- To change them, put a modified copy into MODFILES/PLUS/BALANCE.LUA.

balance = {
  easy = {
    police = 125, -- time until the police arrives after an alarm
    dealer = 115, -- what the dealers pay for the loot
  },
  normal = {
    police = 100,
    dealer = 100,
  },
  hard = {
    police = 80,
    dealer = 85,
  },
}

-- Skill a tool needs in Game+, in percent of the ability (the original values are shown
-- when looking at a tool at Mary Bolton's). Tools not listed keep the original value.
-- The stethoscope asks for 70 % safe cracking in the original, which almost no one who
-- can be hired reaches; the cutting torch only needs 35 %.
tools = {
  Stethoskop = 50,
}
