/*  _________  _______
   / ___/ __ \/ __/ _ \      Der Clou!
  / /__/ /_/ /\ \/ ___/ Open Source Project
  \___/\____/___/_/ http://cosp.sourceforge.net
   Based on the original by neo Software GmbH
*/
/****************************************************************************
  Portions copyright (c) 2005 Vasco Alexandre da Silva Costa
  Portions copyright (c) 2005 Jens Granseuer

  Please read the license terms which should be contained with this
  distribution.
 ****************************************************************************/
#include "sound/fx.h"

#include <SDL3/SDL.h>
#include "base/base.h"
#include "sound/mxr.h"

#define clamp(x, lower, upper) ((x) < (lower) ? (lower) : (x) > (upper) ? (upper) : (x))

MXR_Mixer *pAudioMixer = NULL;

void InitAudio(void)
{
    MXR_Format fmt;
    fmt.nSamplesPerSec = SND_FREQUENCY;
    fmt.nBitsPerChannel = 16;
    fmt.nNumChannels = 2;
    fmt.nSampleSize = 4;
    pAudioMixer = MXR_CreateMixer(&fmt);
    if (!pAudioMixer)
    {
        /* no audio device: the game runs without sound, all sound functions accept the missing mixer */
        Log("No audio output, the game runs silent");
    }

    MXR_SetOutputVolume(pAudioMixer, SND_MAX_VOLUME);

    currMusicVolume = Config.MusicVolume;
}

void RemoveAudio(void)
{
    MXR_DestroyMixer(pAudioMixer);
    pAudioMixer = NULL;
}

/* the sound effect channel needs no state of its own (the old "channel on" flag was never read) */
void sndInitFX(void) {}

void sndDoneFX(void) {}

void sndPlayFX(const char *name)
{
    char fileName[DSK_PATH_MAX];

    sndDoneFX();

    if (pAudioMixer)
    {
        dskBuildPathName(SAMPLES_DIRECTORY, name, fileName);
        MXR_SetInput(pAudioMixer, MXR_INPUT_FX, MXR_CreateInputVOC(fileName));
        MXR_SetInputVolume(pAudioMixer, MXR_INPUT_FX, Config.SfxVolume);  // 2018-09-25
    }
}
