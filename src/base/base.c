/*  _________  _______
   / ___/ __ \/ __/ _ \      Der Clou!
  / /__/ /_/ /\ \/ ___/ Open Source Project
  \___/\____/___/_/ http://cosp.sourceforge.net
   Based on the original by neo Software GmbH
*/
#include "base/base.h"

#include <libgen.h>
#include <string.h>

#include <SDL3/SDL.h>
#include "base/settings.h"
#include "script/script.h"
#include "intro/intro.h"

// cheat function
#include "base/fullenv.c"
#include "theclou.h"

Configuration_t Config;
unsigned char GameMode = GAME_MODE_ORIGINAL;

int32_t cfgScale(int32_t value, const char *what, int easyPerc, int hardPerc)
{
    int difficulty = CFG_DIFFICULTY_EFFECTIVE;
    int percent = 100;

    if (GameMode != GAME_MODE_PLUS) return value; /* the original mode is never changed */

    if (difficulty == CFG_DIFFICULTY_EASY)
        percent = easyPerc;
    else if (difficulty == CFG_DIFFICULTY_HARD)
        percent = hardPerc;

    percent = scrBalancePercent(what, difficulty, percent);
    return (value * percent) / 100;
}

void *StdBuffer1 = 0;

char prgname[255];

/************************************************/
char bProfidisk = 1;  // default = 1, overwritten by detectVersion()
char bCDRom = 1;      // overwritten by detectVersion()
/************************************************/

void tcClearStdBuffer(void *p_Buffer)
{
    int32_t i = 0;
    char *p = (char *)p_Buffer;
    for (i = 0; i < STD_BUFFER1_SIZE; i++) p[i] = 0;
}

static void tcDone(void)
{
    scrDone();
    plDone();
    sndDoneFX();  // Attention!
    sndDone();

    RemoveAudio();

    dbDone();
    CloseAnimHandler();
    txtDone();
    inpCloseAllInputDevs();
    gfxDone();

    if (StdBuffer1) MemFree(StdBuffer1, STD_BUFFER1_SIZE);

    if (MemGetAllocated())
    {
        Log("WARNING! Memory Leak (%d bytes)", MemGetAllocated());

        Log("Total memory used: %d bytes", MemGetMaxAllocated());
    }

    pcErrClose();
}

/* 2014-06-27 templer
Suggestion for automatic detection of the language of the game
*/
static ubyte detectLanguage(void)
{
    char languageMark[4] = {'E', 'D', 'F', 'S'};
    int count = 0;

    for (count = 0; count < sizeof(languageMark) / sizeof(languageMark[0]); count++)
    {
        char fileName[TXT_KEY_LENGTH];
        char fileWithPath[TXT_KEY_LENGTH];
        FILE *p_File = NULL;

        sprintf(fileName, "tcmaine%c.txt", languageMark[count]);
        dskBuildPathName(TEXT_DIRECTORY, fileName, fileWithPath);
        if ((p_File = fopen(fileWithPath, "r")))
        {
            fclose(p_File);
            return (ubyte)count;
        }
    }

    Log("couldn't detect language of the game!");
    NewErrorMsg(Internal_Error, __FILE__, __func__, 1);
    return 0;
}

/* 2026-11-20 quit when the window is closed (idea from vrza/derclou tcQuit) */
void tcQuitGame(void)
{
    Log("Window closed, quitting.");
    tcDone();
    exit(EXIT_SUCCESS); /* SDL_Quit() runs via atexit() */
}

/* 2026-11-20 detect game version from text keys (ported from vrza/derclou) */
static void detectVersion(void)
{
    if (txtKeyExists(THECLOU_TXT, "BITTE_WARTEN_PC_CD_ROM_PROFI"))
    {
        bProfidisk = 1;
        bCDRom = 1;
    }
    else if (txtKeyExists(THECLOU_TXT, "BITTE_WARTEN_PC_PROFI"))
    {
        bProfidisk = 1;
        bCDRom = 0;
    }
    else if (txtKeyExists(THECLOU_TXT, "BITTE_WARTEN_PC_CD_ROM"))
    {
        bProfidisk = 0;
        bCDRom = 1;
    }
    else if (txtKeyExists(THECLOU_TXT, "BITTE_WARTEN_PC"))
    {
        bProfidisk = 0;
        bCDRom = 0;
    }

    Log("Detected version: Der Clou!%s%s", bProfidisk ? " Profidisk" : "", bCDRom ? " CD-ROM" : "");
}

static int tcInit(void)
{
    pcErrOpen(ERR_NO_OUTPUT, tcDone, NULL);

    StdBuffer1 = MemAlloc(STD_BUFFER1_SIZE);

    if (StdBuffer1)
    {
        gfxInit();
        inpOpenAllInputDevs();

        txtInit(detectLanguage()); /* 2014-06-27 templer */
        detectVersion();

        InitAnimHandler();

        dbInit();

        InitAudio();

        sndInit();

        if (!(GamePlayMode & GP_NO_SAMPLES)) sndInitFX();

        plInit();

        gfxCopyCollToXMS(128, &StdRP0InXMS);  // Menü nach StdRP0InXMS
        gfxCopyCollToXMS(129, &StdRP1InXMS);  // Bubbles etc nach StdRP1InXMS

        CurrentBackground = BGD_LONDON;

        return 1;
    }
    return 0;
}

static void InitData(void)
{
    char MainData[TXT_KEY_LENGTH] = {0};
    char BuildData[TXT_KEY_LENGTH] = {0};
    char MainRel[TXT_KEY_LENGTH] = {0};
    char BuildRel[TXT_KEY_LENGTH] = {0};
    ubyte result = 0;

    dskBuildPathName(DATA_DIRECTORY, MAIN_DATA_NAME GAME_DATA_EXT, MainData);
    dskBuildPathName(DATA_DIRECTORY, BUILD_DATA_NAME GAME_DATA_EXT, BuildData);

    dskBuildPathName(DATA_DIRECTORY, MAIN_DATA_NAME GAME_REL_EXT, MainRel);
    dskBuildPathName(DATA_DIRECTORY, BUILD_DATA_NAME GAME_REL_EXT, BuildRel);

    txtReset(OBJECTS_TXT);

    if (dbLoadAllObjects((char *)MainData, 0))
    {
        if (dbLoadAllObjects((char *)BuildData, 0))
        {
            if (LoadRelations((char *)MainRel, 0))
            {
                if (LoadRelations((char *)BuildRel, 0))
                {
                    InitTaxiLocations();
                    result = 1;
                }
            }
        }
    }

    if (!result) NewErrorMsg(Disk_Defect, __FILE__, __func__, 1);
}

static void CloseData(void)
{
    RemRelations(0L, 0L);
    dbDeleteAllObjects(0L, 0L);
}

void tcSetPermanentColors(void)
{
    ubyte colortable[256][3];

    memset(&colortable[0][0], 0, 256 * 3);

    // RGB8

    colortable[248][0] = 116;
    colortable[248][1] = 224;
    colortable[248][2] = 142;

    colortable[249][0] = 224;
    colortable[249][1] = 224;
    colortable[249][2] = 224;

    colortable[250][0] = 55;
    colortable[250][1] = 12;
    colortable[250][2] = 32;

    colortable[251][0] = 128;
    colortable[251][1] = 29;
    colortable[251][2] = 75;

    colortable[252][0] = 170;
    colortable[252][1] = 170;
    colortable[252][2] = 170;

    colortable[253][0] = 104;
    colortable[253][1] = 104;
    colortable[253][2] = 104;

    colortable[254][0] = 0;
    colortable[254][1] = 0;
    colortable[254][2] = 0;

    gfxSetColorRange(248, 254);

    gfxChangeColors(NULL, 0, GFX_BLEND_UP, &colortable[0][0]);
}

static ubyte StartupMenu(void)
{
    LIST *menu = txtGoKey(MENU_TXT, "STARTUP_MENU");
    uint32_t activ = 0;
    char line[TXT_KEY_LENGTH] = COSP_TITLE " v" COSP_VERSION;
    ubyte ret = 0;

    ShowMenuBackground();

    // 2014-07-01 templer
    if (!bProfidisk)
    {
        strcat(line, bCDRom ? " (Std. CD-ROM)" : " (Std.)");
    }
    else
    {
        strcat(line, bCDRom ? " (Prof. CD-ROM)" : " (Prof.)");
    }

    PrintStatus(line);

    /* notice required by the neo license (after start, clearly readable) */
    gfxSetDrMd(u_wrp, GFX_JAM_1);
    gfxSetRect(0, 320);
    gfxSetPens(u_wrp, 249, 254, GFX_SAME_PEN);
    gfxPrint(u_wrp, NEO_NOTICE_1, 100, GFX_PRINT_SHADOW | GFX_PRINT_CENTER);
    gfxPrint(u_wrp, NEO_NOTICE_2, 110, GFX_PRINT_SHADOW | GFX_PRINT_CENTER);

    /* new entry "settings" in front of "quit": new, load / settings, quit */
    AddNode(menu, CreateNode(NULL, 0, (char *)tcSettingsMenuEntry()), GetNthNode(menu, 1));

    inpTurnFunctionKey(0);
    inpTurnESC(0);

    activ = Menu(menu, 15L, 0, NULL, 0L);

    inpTurnESC(1);
    inpTurnFunctionKey(1);

    switch (activ)
    {
        case 0:  // New Game
            if (tcChooseGameMode())
            {
                InitData();
                ret = 1;
            }
            break;

        case 1:  // Load Game
            txtReset(OBJECTS_TXT);
            if (tcLoadTheClou())
            {
                film->StartScene = SceneArgs.ReturnValue;
                ret = 1;
            }
            break;

        case 2:  // Settings
            tcSettingsMenu();
            break;

        case 3:  // Quit Game
            ret = 2;
            break;
    }

    RemoveList(menu);
    return ret;
}

static void tcDo(void)
{
    uint32_t sceneId = SCENE_NEW_GAME;

    gfxChangeColors(l_wrp, 0, GFX_FADE_OUT, 0);
    gfxChangeColors(m_wrp, 0, GFX_FADE_OUT, 0);

    tcSetPermanentColors();

    // to fade in the menu colors once:
    gfxShow(CurrentBackground, GFX_ONE_STEP | GFX_NO_REFRESH | GFX_BLEND_UP, 0, -1, -1);

    // set mouse cursor to white - on spec set color 15 and 16 to white
    gfxSetRGB(NULL, 15, 255, 255, 255);
    gfxSetRGB(NULL, 16, 255, 255, 255);

    SetBubbleType(SPEAK_BUBBLE);

    ShowMenuBackground();

    while (sceneId == SCENE_NEW_GAME)
    {
        ubyte ret = 0;

        if (!(GamePlayMode & GP_DEMO))
            InitStory(STORY_DAT);
        else
            InitStory(STORY_DAT_DEMO);

        while (!ret) ret = StartupMenu();

        if (ret != 2)
        {
            if (GamePlayMode & GP_FULL_ENV) SetFullEnvironment();

            sceneId = PlayStory();
        }
        else
            sceneId = SCENE_THE_END;

        CloseData();
        CloseStory();
    }
}

// remove tabs and spaces
static char *trimstr(char *str)
{
    char *p = NULL;

    str += strspn(str, " \t");
    while ((p = strrchr(str, ' ')))
    {
        *p = '\0';
    }
    while ((p = strrchr(str, '\t')))
    {
        *p = '\0';
    }
    return str;
}

/* reads one configuration file; later files override earlier ones */
static int readConfigFile(const char *config_file, int optional)
{
    char line[256];
    char *pParam = NULL;
    char *pValue = NULL;
    char *p = NULL;
    FILE *file = NULL;

    /* optional files are opened directly, dskOpen would report a missing one as an error */
    file = optional ? fopen(config_file, "rb") : dskOpen(config_file, "rb", 0);
    if (!file)
    {
        return 0;
    }

    while (dskGets(line, sizeof(line) - 1, file))
    {
        line[sizeof(line) - 1] = '\0';  // just to be safe

        if ((p = strchr(line, ';')))
        {
            *p = '\0';
        }
        if ((p = strchr(line, '#')))
        {
            *p = '\0';
        }

        if ((pValue = strchr(line, '=')))
        {
            *pValue++ = '\0';
            pValue = trimstr(pValue);
            if (*pValue == '\0')
            {  // no value after =
                continue;
            }
            pParam = trimstr(line);

            if (!strcasecmp(pParam, "screenwidth"))
            {
                Config.gfxScreenWidth = atoi(pValue);
            }
            else if (!strcasecmp(pParam, "screenheight"))
            {
                Config.gfxScreenHeight = atoi(pValue);
            }
            else if (!strcasecmp(pParam, "fullscreen"))
            {
                Config.gfxFullScreen = atoi(pValue) ? 1 : 0;
            }
            else if (!strcasecmp(pParam, "scale2x"))
            {
                Config.gfxScaleNx = atoi(pValue) ? 1 : 0;
            }
            else if (!strcasecmp(pParam, "soundvolume"))
            {
                Config.SfxVolume = atoi(pValue);
            }
            else if (!strcasecmp(pParam, "musicvolume"))
            {
                Config.MusicVolume = atoi(pValue);
            }
            else if (!strcasecmp(pParam, "voicevolume"))
            {
                Config.VoiceVolume = atoi(pValue);
            }
            else if (!strcasecmp(pParam, "nofontshadow"))
            {
                Config.gfxNoFontShadow = atoi(pValue) ? 1 : 0;
            }
            else if (!strcasecmp(pParam, "nointro"))
            {
                Config.NoIntro = atoi(pValue) ? 1 : 0;
            }
            else if (!strcasecmp(pParam, "resizable"))
            {
                Config.gfxResizable = atoi(pValue) ? 1 : 0;
            }
            else if (!strcasecmp(pParam, "scalequality"))
            {
                Config.gfxLinearFilter = (!strcasecmp(pValue, "linear") || !strcmp(pValue, "1")) ? 1 : 0;
            }
            else if (!strcasecmp(pParam, "integerscale"))
            {
                Config.gfxIntegerScale = atoi(pValue) ? 1 : 0;
            }
            else if (!strcasecmp(pParam, "difficulty"))
            {
                if (!strcasecmp(pValue, "easy") || !strcmp(pValue, "0"))
                    Config.Difficulty = CFG_DIFFICULTY_EASY;
                else if (!strcasecmp(pValue, "hard") || !strcmp(pValue, "2"))
                    Config.Difficulty = CFG_DIFFICULTY_HARD;
                else
                    Config.Difficulty = CFG_DIFFICULTY_NORMAL;
            }
            else if (!strcasecmp(pParam, "policetimer"))
            {
                Config.ShowPoliceTimer = atoi(pValue) ? 1 : 0;
            }
            else if (!strcasecmp(pParam, "plusambience"))
            {
                Config.PlusAmbience = atoi(pValue) ? 1 : 0;
            }
            else if (!strcasecmp(pParam, "plusweather"))
            {
                static const char *names[CFG_WEATHER_COUNT] = {"off", "auto", "clear", "cloudy", "rain", "storm", "fog"};
                int i = 0;

                Config.PlusWeather = CFG_WEATHER_AUTO;
                for (i = 0; i < CFG_WEATHER_COUNT; i++)
                    if (!strcasecmp(pValue, names[i])) Config.PlusWeather = (char)i;
                if (!strcmp(pValue, "0")) Config.PlusWeather = CFG_WEATHER_OFF;
            }
            else if (!strcasecmp(pParam, "plusflash"))
            {
                Config.PlusFlashMs = atoi(pValue);
                if (Config.PlusFlashMs < 0) Config.PlusFlashMs = 0;
            }
            else if (!strcasecmp(pParam, "plusvoices"))
            {
                Config.PlusVoices = atoi(pValue) ? 1 : 0;
            }
            else if (!strcasecmp(pParam, "displayfilter"))
            {
                Config.gfxDisplayFilter = !strcasecmp(pValue, "crt") || !strcmp(pValue, "2")         ? 2
                                          : !strcasecmp(pValue, "scanlines") || !strcmp(pValue, "1") ? 1
                                                                                                      : 0;
            }
            else if (!strcasecmp(pParam, "opl"))
            {
                Config.OplEmulator = (!strcasecmp(pValue, "mame") || !strcmp(pValue, "0")) ? 0 : 1;
            }
            else if (!strcasecmp(pParam, "fastforward"))
            {
                Config.FastForward = atoi(pValue);
            }
            else if (!strcasecmp(pParam, "usejoystick"))
            {
                Config.UseJoystick = (!strcasecmp(pValue, "auto")) ? CFG_JOYSTICK_AUTO : atoi(pValue);
            }
            else if (!strcasecmp(pParam, "demo") && (atoi(pValue) == 42))
            {
                GamePlayMode |= GP_DEMO | GP_STORY_OFF;
            }
            else if (!strcasecmp(pParam, "storyoff") && (atoi(pValue) == 42))
            {
                GamePlayMode |= GP_STORY_OFF;
            }
            else if (!strcasecmp(pParam, "fullenv") && (atoi(pValue) == 42))
            {
                GamePlayMode |= GP_FULL_ENV;
            }
            else if (!strcasecmp(pParam, "leveldesign") && (atoi(pValue) == 42))
            {
                GamePlayMode |= GP_LEVEL_DESIGN;
            }
            else if (!strcasecmp(pParam, "guarddesign") && (atoi(pValue) == 42))
            {
                GamePlayMode |= GP_GUARD_DESIGN;
            }
            else if (!strcasecmp(pParam, "nosamples") && (atoi(pValue) == 42))
            {
                GamePlayMode |= GP_NO_SAMPLES;
            }
            else if (!strcasecmp(pParam, "nocollision") && (atoi(pValue) == 42))
            {
                GamePlayMode |= GP_COLLISION_CHECKING_OFF;
            }
            else if (!strcasecmp(pParam, "showrooms") && (atoi(pValue) == 42))
            {
                GamePlayMode |= GP_SHOW_ROOMS;
            }
            else if (!strcasecmp(pParam, "moremoney") && (atoi(pValue) == 42))
            {
                GamePlayMode |= GP_MORE_MONEY;
            }
        }
    }

    dskClose(file);
    return 1;
}

static void loadConfig(const char *rootPath)
{
    char config_file[FILENAME_MAX];

    // set defaults
    Config.gfxScreenWidth = 320;
    Config.gfxScreenHeight = 200;
    Config.gfxFullScreen = 0;
    Config.gfxNoFontShadow = 0;
    Config.NoIntro = 0;
    Config.MusicVolume = SND_MAX_VOLUME;
    Config.SfxVolume = SND_MAX_VOLUME;
    Config.VoiceVolume = SND_MAX_VOLUME;
    Config.UseJoystick = 0;
#ifdef __EMSCRIPTEN__
    Config.gfxResizable = 0; /* the web page scales the canvas itself */
#else
    Config.gfxResizable = 1;
#endif
    Config.gfxLinearFilter = 0;
    Config.gfxIntegerScale = 0;
    Config.Difficulty = CFG_DIFFICULTY_NORMAL;
    Config.ShowPoliceTimer = 1;
    Config.PlusVoices = 1;
    Config.PlusAmbience = 1;
    Config.PlusWeather = CFG_WEATHER_AUTO;
    Config.PlusFlashMs = 0;
    Config.FastForward = CFG_FAST_FORWARD_DEFAULT;
    Config.OplEmulator = 1;
    Config.gfxDisplayFilter = 0;

    snprintf(config_file, sizeof(config_file), "%s" DIR_SEPARATOR "%s", rootPath, "cosp.cfg");
    if (!readConfigFile(config_file, 0))
    {
        Log("Failed to open cosp.cfg");
    }

    /* settings chosen in the settings menu, stored in the user directory (also works in the browser) */
    tcSettingsPath(config_file, sizeof(config_file));
    if (readConfigFile(config_file, 1))
    {
        Log("Settings: %s", config_file);
    }
}

int tcStartGame(int argc, char **argv)
{
    int32_t res = 0;
#ifdef THECLOU_DEBUG_ALLOC
    MemInit();
#endif

    strcpy(prgname, argv[0]);

    rndInit();

    /* command line: --script <file> and --shotdir <dir> (same as the environment variables) */
    for (int i = 1; i + 1 < argc; i++)
    {
#ifdef _WIN32
        if (!strcmp(argv[i], "--script")) _putenv_s("DERCLOU_SCRIPT", argv[++i]);
        else if (!strcmp(argv[i], "--shotdir")) _putenv_s("DERCLOU_SHOTDIR", argv[++i]);
#else
        if (!strcmp(argv[i], "--script")) setenv("DERCLOU_SCRIPT", argv[++i], 1);
        else if (!strcmp(argv[i], "--shotdir")) setenv("DERCLOU_SHOTDIR", argv[++i], 1);
#endif
    }

    /* game data directory: DERCLOU_ROOTDIR, else the directory of the executable.
       SDL_GetBasePath() also works when started via PATH, a symlink or a desktop file. */
    static char rootPathBuf[FILENAME_MAX];
    char *rootPath = getenv("DERCLOU_ROOTDIR");
#ifdef __EMSCRIPTEN__
    if (!rootPath) rootPath = "/gamedata"; /* --preload-file target, see CMakeLists.txt */
#endif
    if (!rootPath)
    {
#ifndef __EMSCRIPTEN__
        const char *basePath = SDL_GetBasePath(); /* owned by SDL, must not be freed */
        if (basePath)
        {
            snprintf(rootPathBuf, sizeof(rootPathBuf), "%s", basePath);
            /* strip the trailing separator, the path builders add their own */
            size_t len = strlen(rootPathBuf);
            if (len > 1 && (rootPathBuf[len - 1] == '/' || rootPathBuf[len - 1] == '\\')) rootPathBuf[len - 1] = '\0';
            rootPath = rootPathBuf;
        }
        else
#endif
        {
            snprintf(rootPathBuf, sizeof(rootPathBuf), "%s", argv[0]);
            rootPath = dirname(rootPathBuf);
        }
    }

    dskSetRootPath(rootPath);
    // und den Pfad für BuildPathName setzen!
    dskInitUserDataPath();

    {
        char logPath[FILENAME_MAX];
        snprintf(logPath, sizeof(logPath), "%s%s", dskGetUserPath(), LOG_FILENAME);
        errSetLogFile(logPath);
        Log("%s %s", COSP_TITLE, COSP_VERSION);
        Log("Game data: %s", rootPath);
        Log("User data: %s", dskGetUserPath());
    }

    loadConfig(rootPath);
    Log("Difficulty: %s", (Config.Difficulty == CFG_DIFFICULTY_EASY)   ? "easy"
                          : (Config.Difficulty == CFG_DIFFICULTY_HARD) ? "hard"
                                                                        : "normal");

    if (Config.FastForward < 1) Config.FastForward = 1;
    if (Config.FastForward > 16) Config.FastForward = 16;

    if (Config.gfxScreenWidth < 320)
    {
        Config.gfxScreenWidth = 320;
    }
    if (Config.gfxScreenHeight < 200)
    {
        Config.gfxScreenHeight = 200;
    }
    if (Config.MusicVolume < 0)
    {
        Config.MusicVolume = 0;
    }
    else if (Config.MusicVolume > SND_MAX_VOLUME)
    {
        Config.MusicVolume = SND_MAX_VOLUME;
    }
    if (Config.SfxVolume < 0)
    {
        Config.SfxVolume = 0;
    }
    else if (Config.SfxVolume > SND_MAX_VOLUME)
    {
        Config.SfxVolume = SND_MAX_VOLUME;
    }
    if (Config.VoiceVolume < 0)
    {
        Config.VoiceVolume = 0;
    }
    else if (Config.VoiceVolume > SND_MAX_VOLUME)
    {
        Config.VoiceVolume = SND_MAX_VOLUME;
    }

    if ((res = tcInit()))
    {
        if (!Config.NoIntro)
        {
            tcIntro();  // 2014-06-29 LucyG
            /* the intro starts the title music with its third animation; skipped earlier, the
               menu would stay silent */
            sndPlaySound("title.bk", 0);
        }
        else
        {
            sndPlaySound("title.bk", 0);
        }
        tcDo();
    }

    tcDone();

    SDL_Quit();

    if (!res)
    {
        Log("Error: %s|%s|tcInit", __FILE__, __func__);
    }

#ifdef THECLOU_DEBUG_ALLOC
    MemQuit();
#endif
    return 0;
}
