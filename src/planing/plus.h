/*  _________  _______
   / ___/ __ \/ __/ _ \      Der Clou!
  / /__/ /_/ /\ \/ ___/ Open Source Project
  \___/\____/___/_/ http://cosp.sourceforge.net
   Based on the original by neo Software GmbH
*/
#ifndef MODULE_PLANING_PLUS
#define MODULE_PLANING_PLUS

#include "theclou.h"

/* Game+ additions to the planner: guard routes and view areas on the floor plan (key W)
   and a timeline of the plan with undo and redo (key T). Everything here is inactive in
   the original game mode: plPlusEnter only installs its hooks in Game+. */

void plPlusEnter(void);              /* planner opened */
void plPlusLeave(void);              /* planner closed */
void plPlusMenuContext(ubyte inMenu); /* 1 while a planner menu waits for input (timeline allowed) */

#endif
