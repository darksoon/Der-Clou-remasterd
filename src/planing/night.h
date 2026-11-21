/*  _________  _______
   / ___/ __ \/ __/ _ \      Der Clou!
  / /__/ /_/ /\ \/ ___/ Open Source Project
  \___/\____/___/_/ http://cosp.sourceforge.net
   Based on the original by neo Software GmbH
*/
#ifndef MODULE_PLANING_NIGHT
#define MODULE_PLANING_NIGHT

#include "theclou.h"

/* Game+: night light and weather while a burglary is played back.
   Only the picture and the sound change: the light is computed from the game state (read only)
   and applied to the 32 bit picture (gfx/light.h), the weather is chosen with a hash of the game
   day and the building, never with the game's random numbers. Inactive in the original mode. */

void plNightEnter(uint32_t buildingId); /* playback starts */
void plNightLeave(void);                /* playback ended: the picture is normal again */
void plNightStopSound(void);            /* the burglary is over: rain and thunder end */
void plNightShowWeather(void);          /* weather line in the menu, next to the burglary time */

#endif
