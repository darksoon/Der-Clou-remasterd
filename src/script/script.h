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

/* Game+: skill a tool needs, in percent (0..100), from tools[name] in PLUS/BALANCE.LUA;
   returns -1 if the script does not change this tool */
extern int scrToolSkillPercent(const char *toolName);

/* Game+: what the newspaper writes after a burglary, from news(info) in PLUS/NEWS.LUA */
typedef struct
{
    const char *building; /* name in the game's code page */
    int escaped;          /* 1 escaped, 0 caught */
    int alarm;            /* 0 none, 1 silent, 2 loud */
    int loot;             /* value of the loot taken, in pounds */
    int knockedOut;       /* guards that were knocked out */
    int day;              /* game day, also a stable seed for the script */
    int lang;             /* 0 English, 1 German, 2 French, 3 Spanish */
} ScrNewsInfo;

/* fills up to maxLines lines (game code page, at most lineSize - 1 bytes each); returns the count */
extern int scrNews(const ScrNewsInfo *info, char *lines, int maxLines, int lineSize);

#endif
