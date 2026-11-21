/*  _________  _______
   / ___/ __ \/ __/ _ \      Der Clou!
  / /__/ /_/ /\ \/ ___/ Open Source Project
  \___/\____/___/_/ http://cosp.sourceforge.net
   Based on the original by neo Software GmbH
*/
#include "random/random.h"

void rndInit(void)
{
#ifndef __COSP__
    randomize(); /* Zufallszahl über Timer initialisieren */
#else
    /* DERCLOU_SEED makes automated test games repeatable */
    const char *seed = getenv("DERCLOU_SEED");
    srand(seed ? (unsigned int)strtoul(seed, NULL, 10) : (unsigned int)time(NULL));
#endif
}

uint32_t CalcRandomNr(uint32_t l_limit, uint32_t u_limit)
{
    uint32_t x = l_limit + (rand() % (u_limit - l_limit));

    return x;
}
