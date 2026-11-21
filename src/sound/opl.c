/*  _________  _______
   / ___/ __ \/ __/ _ \      Der Clou!
  / /__/ /_/ /\ \/ ___/ Open Source Project
  \___/\____/___/_/ http://cosp.sourceforge.net
   Based on the original by neo Software GmbH
*/
#include "sound/opl.h"

#include <stdint.h>
#include <string.h>

#include "base/base.h"
#include "sound/fmopl.h"
#include "sound/mxr.h"
#include "sound/nuked/opl3.h"

static int oplEmulator = -1;
static opl3_chip oplNuked;

static void oplStart(int emulator)
{
    oplEmulator = emulator;
    if (oplEmulator == OPL_EMULATOR_NUKED)
    {
        OPL3_Reset(&oplNuked, OPL_FREQUENCY);
    }
    else
    {
        YM3812Init(OPL_NUM_CHIPS, OPL_INTERNAL_FREQ, OPL_FREQUENCY);
    }
    Log("AdLib emulation: %s", (oplEmulator == OPL_EMULATOR_NUKED) ? "Nuked OPL3" : "MAME fmopl");
}

static void oplStop(void)
{
    if (oplEmulator == OPL_EMULATOR_MAME) YM3812Shutdown();
    oplEmulator = -1;
}

void oplInit(void) { oplStart(Config.OplEmulator ? OPL_EMULATOR_NUKED : OPL_EMULATOR_MAME); }

void oplShutdown(void) { oplStop(); }

void oplSelect(int emulator)
{
    if (emulator == oplEmulator) return;
    oplStop();
    oplStart(emulator);
}

/* reset and writes also come from the main thread (a new song is opened while the old one plays),
   so they take the audio lock; it is recursive, calls from the audio callback itself are fine */
void oplReset(void)
{
    MXR_LockAudio();
    if (oplEmulator == OPL_EMULATOR_NUKED)
        OPL3_Reset(&oplNuked, OPL_FREQUENCY);
    else if (oplEmulator == OPL_EMULATOR_MAME)
        YM3812ResetChip(OPL_CHIP0);
    MXR_UnlockAudio();
}

void oplWrite(int reg, int val)
{
    MXR_LockAudio();
    if (oplEmulator == OPL_EMULATOR_NUKED)
    {
        /* Buffered: every write takes effect two samples after the previous one, like the slow
           port access of a real card. The player switches a note off and on again without a
           pause; written at once the chip would never see the key go off and held notes would
           not be struck again. */
        OPL3_WriteRegBuffered(&oplNuked, (uint16_t)reg, (uint8_t)val);
    }
    else if (oplEmulator == OPL_EMULATOR_MAME)
    {
        YM3812Write(OPL_CHIP0, 0x0388, reg);
        YM3812Write(OPL_CHIP0, 0x0389, val);
    }
    MXR_UnlockAudio();
}

void oplGenerate(short *buffer, int samples)
{
    if (oplEmulator == OPL_EMULATOR_NUKED)
    {
        int16_t stereo[2 * 256];

        /* in OPL2 mode both channels carry the same signal */
        while (samples > 0)
        {
            int n = (samples < 256) ? samples : 256;
            int i = 0;

            OPL3_GenerateStream(&oplNuked, stereo, (uint32_t)n);
            for (i = 0; i < n; i++) buffer[i] = (short)(((int)stereo[2 * i] + (int)stereo[2 * i + 1]) / 2);
            buffer += n;
            samples -= n;
        }
    }
    else if (oplEmulator == OPL_EMULATOR_MAME)
    {
        YM3812UpdateOne(OPL_CHIP0, buffer, samples);
    }
    else
    {
        memset(buffer, 0, samples * sizeof(short));
    }
}
