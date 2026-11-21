/*  _________  _______
   / ___/ __ \/ __/ _ \      Der Clou!
  / /__/ /_/ /\ \/ ___/ Open Source Project
  \___/\____/___/_/ http://cosp.sourceforge.net
   Based on the original by neo Software GmbH
*/
/* Settings menu of the main menu. Every entry changes a value of Config at once,
   the result is stored in the user directory (CFG_USER_FILENAME) and read after cosp.cfg. */
#include "base/settings.h"

#include <string.h>

#include "base/base.h"
#include "gameplay/gp_app.h"
#include "sound/fx.h"
#include "sound/mxr.h"
#include "sound/opl.h"

enum
{
    SET_MUSIC,
    SET_SFX,
    SET_VOICE,
    SET_OPL,
    SET_FULLSCREEN,
    SET_FILTER,
    SET_DISPLAY_FILTER,
    SET_INTEGER_SCALE,
    SET_DIFFICULTY,
    SET_POLICE_TIMER,
    SET_FAST_FORWARD,
    SET_GAMEPAD,
    SET_INTRO,
    SET_PLUS_VOICES,
    SET_BACK,
    SET_COUNT
};

/* texts in the game font: \x81 = ü, \x84 = ä, \x82 = é, \x90 = É, \xa0 = á, \xa1 = í, \xa2 = ó, \x8a = è */
static const char *setLabel[SET_COUNT][4] = {
    {"Music", "Musik", "Musique", "M\xa3sica"},
    {"Sound effects", "Ger\x84usche", "Effets sonores", "Efectos"},
    {"Speech", "Sprachausgabe", "Voix", "Voces"},
    {"AdLib music", "AdLib-Musik", "Musique AdLib", "M\xa3sica AdLib"},
    {"Fullscreen", "Vollbild", "Plein \x82" "cran", "Pantalla completa"},
    {"Scaling", "Skalierung", "Lissage", "Escalado"},
    {"Picture", "Bild", "Image", "Imagen"},
    {"Whole pixels", "Ganze Pixel", "Pixels entiers", "P\xa1xeles enteros"},
    {"Difficulty (Game+)", "Schwierigkeit (Spiel+)", "Difficult\x82 (Jeu+)", "Dificultad (Juego+)"},
    {"Police countdown", "Polizei-Countdown", "Chrono police", "Aviso polic\xa1" "a"},
    {"Fast forward", "Zeitraffer", "Avance rapide", "Avance r\xa0pido"},
    {"Gamepad", "Gamepad", "Manette", "Mando"},
    {"Intro", "Intro", "Intro", "Intro"},
    {"New voices (Game+)", "Neue Stimmen (Spiel+)", "Nouvelles voix (Jeu+)", "Voces nuevas (Juego+)"},
    {"Back", "Zur\x81" "ck", "Retour", "Volver"},
};

static const char *setOnOff[2][4] = {
    {"off", "aus", "non", "no"},
    {"on", "an", "oui", "s\xa1"},
};

static const char *setFilter[2][4] = {
    {"sharp", "scharf", "net", "n\xa1tido"},
    {"smooth", "weich", "doux", "suave"},
};

static const char *setDifficulty[3][4] = {
    {"easy", "leicht", "facile", "f\xa0" "cil"},
    {"normal", "normal", "normal", "normal"},
    {"hard", "schwer", "difficile", "dif\xa1" "cil"},
};

static const char *setDisplayFilter[3][4] = {
    {"plain", "normal", "normale", "normal"},
    {"scanlines", "Scanlines", "lignes", "l\xa1neas"},
    {"CRT monitor", "R\x94hrenmonitor", "\x82" "cran CRT", "monitor CRT"},
};

static const char *setOpl[2][4] = {
    {"classic", "klassisch", "classique", "cl\xa0sica"},
    {"accurate", "genau", "fid\x8ale", "fiel"},
};

static const char *setAuto[4] = {"auto", "automatisch", "auto", "auto"};

static const char *setHint[4] = {"Click or arrow keys: change   Right click: back",
                                 "Klick oder Pfeiltasten: \x84ndern   Rechtsklick: zur\x81" "ck",
                                 "Clic ou fl\x8a" "ches: changer   Clic droit: retour",
                                 "Clic o flechas: cambiar   Clic derecho: volver"};

static const char *setMenuEntry[4] = {"Settings", "Einstellungen", "Options", "Opciones"};

static const int setFastForwardSteps[] = {2, 4, 8};
#define SET_FAST_FORWARD_COUNT ((int)(sizeof(setFastForwardSteps) / sizeof(setFastForwardSteps[0])))

static ubyte setLang(void)
{
    ubyte lang = txtGetLanguage();
    return (lang <= 3) ? lang : 1;
}

const char *tcSettingsMenuEntry(void) { return setMenuEntry[setLang()]; }

/* volume 0..SND_MAX_VOLUME shown and changed in ten steps (the game font has no readable %) */
static int setVolumeStep(int volume) { return (volume * 10 + SND_MAX_VOLUME / 2) / SND_MAX_VOLUME; }

static int setStepVolume(int volume, int dir)
{
    int step = setVolumeStep(volume) + dir;

    if (step < 0) step = 0;
    if (step > 10) step = 10;

    return (step * SND_MAX_VOLUME) / 10;
}

static int setStepFastForward(int steps, int dir)
{
    int i = 0;

    if (dir > 0)
    {
        for (i = 0; i < SET_FAST_FORWARD_COUNT; i++)
            if (setFastForwardSteps[i] > steps) return setFastForwardSteps[i];
        return setFastForwardSteps[0];
    }

    for (i = SET_FAST_FORWARD_COUNT - 1; i >= 0; i--)
        if (setFastForwardSteps[i] < steps) return setFastForwardSteps[i];
    return setFastForwardSteps[SET_FAST_FORWARD_COUNT - 1];
}

static void setValueText(int entry, char *value, size_t size)
{
    ubyte lang = setLang();

    switch (entry)
    {
        case SET_MUSIC:
            snprintf(value, size, "%d/10", setVolumeStep(Config.MusicVolume));
            break;
        case SET_SFX:
            snprintf(value, size, "%d/10", setVolumeStep(Config.SfxVolume));
            break;
        case SET_VOICE:
            snprintf(value, size, "%d/10", setVolumeStep(Config.VoiceVolume));
            break;
        case SET_OPL:
            snprintf(value, size, "%s", setOpl[Config.OplEmulator ? 1 : 0][lang]);
            break;
        case SET_FULLSCREEN:
            snprintf(value, size, "%s", setOnOff[Config.gfxFullScreen ? 1 : 0][lang]);
            break;
        case SET_FILTER:
            snprintf(value, size, "%s", setFilter[Config.gfxLinearFilter ? 1 : 0][lang]);
            break;
        case SET_DISPLAY_FILTER:
            snprintf(value, size, "%s", setDisplayFilter[(int)Config.gfxDisplayFilter % 3][lang]);
            break;
        case SET_INTEGER_SCALE:
            snprintf(value, size, "%s", setOnOff[Config.gfxIntegerScale ? 1 : 0][lang]);
            break;
        case SET_DIFFICULTY:
            snprintf(value, size, "%s", setDifficulty[(int)Config.Difficulty][lang]);
            break;
        case SET_POLICE_TIMER:
            snprintf(value, size, "%s", setOnOff[Config.ShowPoliceTimer ? 1 : 0][lang]);
            break;
        case SET_FAST_FORWARD:
            snprintf(value, size, "%dx", Config.FastForward);
            break;
        case SET_INTRO:
            snprintf(value, size, "%s", setOnOff[Config.NoIntro ? 0 : 1][lang]);
            break;
        case SET_PLUS_VOICES:
            snprintf(value, size, "%s", setOnOff[Config.PlusVoices ? 1 : 0][lang]);
            break;
        case SET_GAMEPAD:
            if (!Config.UseJoystick)
                snprintf(value, size, "%s", setOnOff[0][lang]);
            else if (Config.UseJoystick == CFG_JOYSTICK_AUTO)
                snprintf(value, size, "%s", setAuto[lang]);
            else
                snprintf(value, size, "#%d", Config.UseJoystick);
            break;
        default:
            value[0] = EOS;
            break;
    }
}

static void setChange(int entry, int dir)
{
    switch (entry)
    {
        case SET_MUSIC:
            Config.MusicVolume = setStepVolume(Config.MusicVolume, dir);
            break;
        case SET_SFX:
            Config.SfxVolume = setStepVolume(Config.SfxVolume, dir);
            break;
        case SET_VOICE:
            Config.VoiceVolume = setStepVolume(Config.VoiceVolume, dir);
            break;
        case SET_OPL:
            Config.OplEmulator = !Config.OplEmulator;
            MXR_LockAudio();
            oplSelect(Config.OplEmulator ? OPL_EMULATOR_NUKED : OPL_EMULATOR_MAME);
            MXR_UnlockAudio();
            sndRestartMusic(); /* the new chip starts empty */
            break;
        case SET_FULLSCREEN:
            Config.gfxFullScreen = !Config.gfxFullScreen;
            gfxApplyDisplayConfig();
            break;
        case SET_FILTER:
            Config.gfxLinearFilter = !Config.gfxLinearFilter;
            gfxApplyDisplayConfig();
            break;
        case SET_DISPLAY_FILTER:
            Config.gfxDisplayFilter = (char)((Config.gfxDisplayFilter + 3 + dir) % 3);
            gfxApplyDisplayConfig();
            break;
        case SET_INTEGER_SCALE:
            Config.gfxIntegerScale = !Config.gfxIntegerScale;
            gfxApplyDisplayConfig();
            break;
        case SET_DIFFICULTY:
            Config.Difficulty = (char)((Config.Difficulty + 3 + dir) % 3);
            break;
        case SET_POLICE_TIMER:
            Config.ShowPoliceTimer = !Config.ShowPoliceTimer;
            break;
        case SET_FAST_FORWARD:
            Config.FastForward = (char)setStepFastForward(Config.FastForward, dir);
            break;
        case SET_INTRO:
            Config.NoIntro = !Config.NoIntro;
            break;
        case SET_PLUS_VOICES:
            Config.PlusVoices = !Config.PlusVoices;
            break;
        case SET_GAMEPAD:
            /* off and auto; a fixed joystick number from cosp.cfg is kept until it is changed */
            Config.UseJoystick = Config.UseJoystick ? 0 : CFG_JOYSTICK_AUTO;
            inpApplyJoystickConfig();
            break;
        default:
            break;
    }
}

/* where the settings menu stores its file. In the browser only the saved games directory is kept in
   IndexedDB, so the file goes there; moving that mount would lose the saved games of older versions. */
void tcSettingsPath(char *path, size_t size)
{
#ifdef __EMSCRIPTEN__
    char result[FILENAME_MAX];
    dskBuildPathNameUser(DATADISK_DIRECTORY, CFG_USER_FILENAME, result);
    snprintf(path, size, "%s", result);
#else
    snprintf(path, size, "%s%s", dskGetUserPath(), CFG_USER_FILENAME);
#endif
}

void tcSaveSettings(void)
{
    char path[FILENAME_MAX];
    FILE *file = NULL;

    tcSettingsPath(path, sizeof(path));

    if (!(file = fopen(path, "w")))
    {
        Log("Settings: cannot write %s", path);
        return;
    }

    fprintf(file, "# written by the settings menu, read after cosp.cfg\n");
    fprintf(file, "musicvolume = %d\n", Config.MusicVolume);
    fprintf(file, "soundvolume = %d\n", Config.SfxVolume);
    fprintf(file, "voicevolume = %d\n", Config.VoiceVolume);
    fprintf(file, "opl = %s\n", Config.OplEmulator ? "nuked" : "mame");
    fprintf(file, "fullscreen = %d\n", Config.gfxFullScreen ? 1 : 0);
    fprintf(file, "scalequality = %s\n", Config.gfxLinearFilter ? "linear" : "nearest");
    fprintf(file, "displayfilter = %s\n", (Config.gfxDisplayFilter == 2)   ? "crt"
                                          : (Config.gfxDisplayFilter == 1) ? "scanlines"
                                                                            : "off");
    fprintf(file, "integerscale = %d\n", Config.gfxIntegerScale ? 1 : 0);
    fprintf(file, "difficulty = %s\n", (Config.Difficulty == CFG_DIFFICULTY_EASY)   ? "easy"
                                       : (Config.Difficulty == CFG_DIFFICULTY_HARD) ? "hard"
                                                                                     : "normal");
    fprintf(file, "policetimer = %d\n", Config.ShowPoliceTimer ? 1 : 0);
    fprintf(file, "fastforward = %d\n", Config.FastForward);
    fprintf(file, "nointro = %d\n", Config.NoIntro ? 1 : 0);
    fprintf(file, "plusvoices = %d\n", Config.PlusVoices ? 1 : 0);
    if (Config.UseJoystick == CFG_JOYSTICK_AUTO)
        fprintf(file, "usejoystick = auto\n");
    else
        fprintf(file, "usejoystick = %d\n", Config.UseJoystick);

    fclose(file);
    dskSyncUserData();

    Log("Settings saved: %s", path);
}

void tcSettingsMenu(void)
{
    int entries[SET_COUNT];
    int count = 0;
    int i = 0;
    ubyte activ = 0;
    ubyte changed = 0;

    for (i = 0; i < SET_COUNT; i++)
    {
        if (((i == SET_VOICE) || (i == SET_PLUS_VOICES)) && !bCDRom) continue; /* only the CD-ROM version talks */
        entries[count++] = i;
    }

    ShowMenuBackground();
    PrintStatus((char *)setHint[setLang()]);

    SetBubbleLeftRightExit(1);

    while (1)
    {
        LIST *bubble = CreateList(0);
        ubyte choice = 0;
        ubyte info = 0;
        int dir = 1;

        for (i = 0; i < count; i++)
        {
            char line[TXT_KEY_LENGTH];
            char value[32];

            setValueText(entries[i], value, sizeof(value));
            if (value[0])
                snprintf(line, sizeof(line), "*%s: %s", setLabel[entries[i]][setLang()], value);
            else
                snprintf(line, sizeof(line), "*%s", setLabel[entries[i]][setLang()]);
            CreateNode(bubble, 0, line);
        }

        SetPictID((uword)-1);
        choice = Bubble(bubble, activ, NULL, 0);
        info = GetExtBubbleActionInfo();
        RemoveList(bubble);

        if ((choice == GET_OUT) || (choice >= count) || (entries[choice] == SET_BACK)) break;

        if ((info & INP_LEFT) && !(info & INP_LBUTTONP)) dir = -1;

        activ = choice;
        setChange(entries[choice], dir);
        changed = 1;
    }

    SetBubbleLeftRightExit(0);

    if (changed) tcSaveSettings();
}

static const char *modeChoice[2][4] = {
    {"*Original: plays like 1994", "*Original: spielt wie 1994", "*Original: comme en 1994", "*Original: como en 1994"},
    {"*Game+: with new features", "*Spiel+: mit Neuerungen", "*Jeu+: avec nouveaut\x82s", "*Juego+: con novedades"},
};

static const char *modeQuestion[4] = {"Which way do you want to play?", "Wie m\x94" "chtest du spielen?",
                                      "Comment veux-tu jouer ?", "\xa8" "C\xa2mo quieres jugar?"};

ubyte tcChooseGameMode(void)
{
    LIST *bubble = CreateList(0);
    ubyte choice = 0;

    CreateNode(bubble, 0, (char *)modeChoice[GAME_MODE_ORIGINAL][setLang()]);
    CreateNode(bubble, 0, (char *)modeChoice[GAME_MODE_PLUS][setLang()]);

    ShowMenuBackground();
    PrintStatus((char *)modeQuestion[setLang()]);

    SetPictID((uword)-1);
    choice = Bubble(bubble, 0, NULL, 0);
    RemoveList(bubble);

    if (choice == GET_OUT) return 0;

    GameMode = (choice == 1) ? GAME_MODE_PLUS : GAME_MODE_ORIGINAL;
    Log("Game mode: %s, difficulty %s", (GameMode == GAME_MODE_PLUS) ? "Game+" : "original",
        (CFG_DIFFICULTY_EFFECTIVE == CFG_DIFFICULTY_EASY)   ? "easy"
        : (CFG_DIFFICULTY_EFFECTIVE == CFG_DIFFICULTY_HARD) ? "hard"
                                                            : "normal");
    Log("Balance: police %d %%, dealer %d %%", (int)CFG_SCALE(100, "police", 125, 80),
        (int)CFG_SCALE(100, "dealer", 115, 85));
    return 1;
}
