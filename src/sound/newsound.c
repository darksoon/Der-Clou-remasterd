/*  _________  _______
   / ___/ __ \/ __/ _ \      Der Clou!
  / /__/ /_/ /\ \/ ___/ Open Source Project
  \___/\____/___/_/ http://cosp.sourceforge.net
   Based on the original by neo Software GmbH
*/
/****************************************************************************
  Portions copyright (c) 2005 Vasco Alexandre da Silva Costa

  Please read the license terms which should be contained with this
  distribution.
 ****************************************************************************/
#include "sound/newsound.h"

#include <SDL3/SDL.h>
#include "disk/disk.h"
#include "sound/fx.h"
#include "sound/mxr.h"

char currSoundName[256];

/* Game+: background sounds that go with the street and pub music */
static const char *currAmbience = NULL;
/* Game+: the rain of a burglary night replaces the background sound of the music */
static const char *weatherAmbience = NULL;

static int sndPlusSoundsOn(void)
{
    return (GameMode == GAME_MODE_PLUS) && Config.PlusAmbience && (Config.SfxVolume > 0);
}

static void sndSetAmbience(const char *file)
{
    char path[256];

    if (!pAudioMixer) return;
    if (!sndPlusSoundsOn()) file = NULL;
    if (file == currAmbience) return;

    currAmbience = file;
    if (file)
    {
        dskBuildPathName((file == weatherAmbience) ? PLUS_WEATHER_DIRECTORY : PLUS_AMBIENT_DIRECTORY, file, path);
        Log("Background sound: %s", path);
        MXR_SetInput(pAudioMixer, MXR_INPUT_AMBIENT, MXR_CreateInputWAVLoop(path));
        MXR_SetInputVolume(pAudioMixer, MXR_INPUT_AMBIENT, (unsigned char)Config.SfxVolume);
    }
    else
    {
        MXR_SetInput(pAudioMixer, MXR_INPUT_AMBIENT, NULL);
    }
}

/* the background sound for a piece of music: street, pub, hotel room, police */
static void sndAmbienceForMusic(const char *name)
{
    if (weatherAmbience)
        sndSetAmbience(weatherAmbience);
    else if (!strncasecmp(name, "street", 6))
        sndSetAmbience("STREET.WAV");
    else if (!strncasecmp(name, "bar", 3))
        sndSetAmbience("PUB.WAV");
    else if (!strcasecmp(name, "hotel.bk"))
        sndSetAmbience("HOTEL.WAV");
    else if (!strcasecmp(name, "gludo.bk"))
        sndSetAmbience("POLICE.WAV");
    else
        sndSetAmbience(NULL);
}

/* after a change in the settings or the game mode */
void sndUpdateAmbience(void)
{
    currAmbience = NULL;
    MXR_SetInput(pAudioMixer, MXR_INPUT_AMBIENT, NULL);
    sndAmbienceForMusic(currSoundName);
}

/* Game+: a looping weather sound (rain) for the burglary, NULL to end it */
void sndSetWeatherAmbience(const char *file)
{
    weatherAmbience = file;
    if (pAudioMixer) sndUpdateAmbience();
}

/* Game+: a single weather sound (thunder) on its own mixer input; volume in percent of the effects */
void sndPlayWeatherFX(const char *file, int percent)
{
    char path[256];
    int volume = 0;

    if (!pAudioMixer) return;
    if (!file || !sndPlusSoundsOn())
    {
        MXR_SetInput(pAudioMixer, MXR_INPUT_WEATHER, NULL);
        return;
    }

    volume = (Config.SfxVolume * percent) / 100;
    if (volume > SND_MAX_VOLUME) volume = SND_MAX_VOLUME;

    dskBuildPathName(PLUS_WEATHER_DIRECTORY, file, path);
    MXR_SetInput(pAudioMixer, MXR_INPUT_WEATHER, MXR_CreateInputWAV(path));
    MXR_SetInputVolume(pAudioMixer, MXR_INPUT_WEATHER, (unsigned char)volume);
}

int currMusicVolume = 0;
int targetMusicVolume = 0;

static char bMuted = 0;

void sndInit(void)
{
    currSoundName[0] = '\0';

    currMusicVolume = Config.MusicVolume;
    targetMusicVolume = Config.MusicVolume;
}

void sndDone(void) {}

void sndPlaySound(char *name, uint32_t mode)
{
    char path[256];

    if (pAudioMixer)
    {
        if (strcasecmp(currSoundName, name))
        {
            strcpy(currSoundName, name);

            dskBuildPathName(SOUND_DIRECTORY, name, path);

            MXR_SetInput(pAudioMixer, MXR_INPUT_MUSIC, MXR_CreateInputHSC(path));

            currMusicVolume = 0;
            targetMusicVolume = Config.MusicVolume;
            MXR_SetInputVolume(pAudioMixer, MXR_INPUT_MUSIC, currMusicVolume);  // 2018-09-25

            sndAmbienceForMusic(name);
        }
    }
}

char *sndGetCurrSoundName(void) { return currSoundName; }

/* play the current music again from the start, e.g. after the AdLib emulation changed */
void sndRestartMusic(void)
{
    char name[256];

    if (!currSoundName[0]) return;
    snprintf(name, sizeof(name), "%s", currSoundName);
    currSoundName[0] = '\0';
    sndPlaySound(name, 0);
}

// 2014-07-17 LucyG : called from inpDoPseudoMultiTasking
void sndDoFading(void)
{
    // 2018-09-25: volume can now be changed any time
    if (bMuted)
    {
        targetMusicVolume = Config.MusicVolume / 4;
    }
    else
    {
        targetMusicVolume = Config.MusicVolume;
    }

    if (currMusicVolume < targetMusicVolume)
    {
        currMusicVolume++;
    }
    else if (currMusicVolume > targetMusicVolume)
    {
        currMusicVolume--;
    }

    MXR_SetInputVolume(pAudioMixer, MXR_INPUT_MUSIC, currMusicVolume);
    MXR_SetInputVolume(pAudioMixer, MXR_INPUT_FX, Config.SfxVolume);
    MXR_SetInputVolume(pAudioMixer, MXR_INPUT_VOICE, Config.VoiceVolume);
}

void sndFading(short int targetVol)
{
    if (pAudioMixer)
    {
        /* 2014-07-17 LucyG : this is called from dialog.c (and intro.c)
           with targetVol = 16 before and 0 after voice playback */
        if (!targetVol)
        {
            bMuted = 0;
            targetMusicVolume = Config.MusicVolume;
        }
        else
        {
            bMuted = 1;
            targetMusicVolume = Config.MusicVolume / 4;
        }
    }
}

void sndStopSound(ubyte dummy)
{
    MXR_SetInput(pAudioMixer, MXR_INPUT_MUSIC, NULL);
    sndSetAmbience(NULL);
}
