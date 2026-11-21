/*  _________  _______
   / ___/ __ \/ __/ _ \      Der Clou!
  / /__/ /_/ /\ \/ ___/ Open Source Project
  \___/\____/___/_/ http://cosp.sourceforge.net
   Based on the original by neo Software GmbH
*/
/* Lua scripting for Game+. The original mode never runs any script. */
#ifndef MODULE_SCRIPT
#define MODULE_SCRIPT

#include <stdint.h>

extern void scrInit(void);
extern void scrDone(void);

/* percentage for a balance value of Game+ from PLUS/BALANCE.LUA, fallback if the script has none */
extern int scrBalancePercent(const char *what, int difficulty, int fallback);

#endif
