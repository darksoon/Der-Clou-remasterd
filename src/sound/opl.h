/*  _________  _______
   / ___/ __ \/ __/ _ \      Der Clou!
  / /__/ /_/ /\ \/ ___/ Open Source Project
  \___/\____/___/_/ http://cosp.sourceforge.net
   Based on the original by neo Software GmbH
*/
/* AdLib (YM3812) emulation for the HSC music: Nuked OPL3 (accurate) or MAME's fmopl (classic) */
#ifndef MODULE_OPL
#define MODULE_OPL

#define OPL_EMULATOR_MAME 0
#define OPL_EMULATOR_NUKED 1

extern void oplInit(void);
extern void oplShutdown(void);
extern void oplReset(void);
extern void oplWrite(int reg, int val);
extern void oplGenerate(short *buffer, int samples); /* mono, OPL_FREQUENCY */
extern void oplSelect(int emulator);                 /* call with the audio locked */

#endif
