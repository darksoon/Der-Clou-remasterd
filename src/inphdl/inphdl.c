/*  _________  _______
   / ___/ __ \/ __/ _ \      Der Clou!
  / /__/ /_/ /\ \/ ___/ Open Source Project
  \___/\____/___/_/ http://cosp.sourceforge.net
   Based on the original by neo Software GmbH
*/
#include "inphdl/inphdl.h"
#include "base/base.h"

#include <SDL3/SDL.h>
#include <string.h>
#include <stdlib.h>
#include "inphdl/arrow1x_xpm.c"
#include "inphdl/arrow2x_xpm.c"
#include "sound/fx.h"
#include "sound/newsound.h"

static SDL_Cursor *cursor[2];

static ubyte cursorInit;
ubyte initMouseCursor(void);
void removeMouseCursor(void);

struct IHandler
{
    int32_t ul_XSensitivity;
    int32_t ul_YSensitivity;
    int32_t ul_WaitTicks;

    Uint16 us_MouseX;
    Uint16 us_MouseY;

    ubyte uch_EscStatus;
    ubyte uch_FunctionKeyStatus;
    ubyte uch_MouseStatus;
};

struct IHandler IHandler;

/* fast forward for the burglary playback, toggled with Tab */
static ubyte inpFastForwardOn = 0;
ubyte inpFastForward(void) { return inpFastForwardOn; }
void inpSetFastForward(ubyte on) { inpFastForwardOn = on; }

static SDL_Cursor *init_system_cursor(char *image[])
{
    int i = 0;
    int row = 0;
    int col = 0;
    Uint8 data[4 * 32];
    Uint8 mask[4 * 32];
    int hot_x = 0;
    int hot_y = 0;

    i = -1;
    for (row = 0; row < 32; ++row)
    {
        for (col = 0; col < 32; ++col)
        {
            if (col % 8)
            {
                data[i] <<= 1;
                mask[i] <<= 1;
            }
            else
            {
                ++i;
                data[i] = mask[i] = 0;
            }
            switch (image[4 + row][col])
            {
                case 'X':
                    data[i] |= 0x01;
                    mask[i] |= 0x01;
                    break;
                case '.':
                    mask[i] |= 0x01;
                    break;
                case ' ':
                    break;
            }
        }
    }
    sscanf(image[4 + row], "%d,%d", &hot_x, &hot_y);
    return SDL_CreateCursor(data, mask, 32, 32, hot_x, hot_y);
}

void setMouseCursor(ubyte fact)
{
    if (cursorInit)
    {
        switch (fact)
        {
            case 1:
            case 2:
                SDL_SetCursor(cursor[0]);
                break;

            case 3:
            case 4:
                SDL_SetCursor(cursor[1]);
                break;
        }
        SDL_ShowCursor();
    }
}

ubyte initMouseCursor(void)
{
    if (!cursorInit)
    {
        cursor[0] = init_system_cursor(arrow1x);

        if (!cursor[0])
        {
            Log("SDL_CreateCursor -> %s", SDL_GetError());
            return 0;
        }

        cursor[1] = init_system_cursor(arrow2x);

        if (!cursor[1])
        {
            Log("SDL_CreateCursor -> %s", SDL_GetError());
            return 0;
        }
    }
    return 1;
}

void removeMouseCursor(void)
{
    if (cursorInit)
    {
        SDL_HideCursor();
        SDL_DestroyCursor(cursor[0]);
        SDL_DestroyCursor(cursor[1]);
    }
}

/* 2014-01-10 LucyG: Joystick
   2026-11-20: hotplug support and "auto" mode (idea from vrza/derclou) */
#define INP_JOY_DEAD_ZONE 10000

static SDL_Joystick *inpJoystick = NULL;
static SDL_JoystickID inpJoystickId = 0; /* 0 = none in SDL 3 */
static ubyte inpJoySubsystem = 0;

static void inpOpenJoystickIfNeeded(void)
{
    int numJoys = 0;
    int i = 0;
    int first = 0;
    int last = 0;
    SDL_JoystickID *joys = NULL;

    if (!inpJoySubsystem || inpJoystick) return;

    joys = SDL_GetJoysticks(&numJoys);
    if (!joys) numJoys = 0;

    if (Config.UseJoystick == CFG_JOYSTICK_AUTO)
    {
        first = 0;
        last = numJoys - 1;
    }
    else
    {
        first = last = Config.UseJoystick - 1;
    }

    for (i = first; (i <= last) && (i < numJoys); i++)
    {
        inpJoystick = SDL_OpenJoystick(joys[i]);
        if (inpJoystick)
        {
            inpJoystickId = SDL_GetJoystickID(inpJoystick);
            Log("Using joystick #%d = \"%s\"", i + 1, SDL_GetJoystickName(inpJoystick));
            SDL_free(joys);
            return;
        }
    }

    if ((Config.UseJoystick != CFG_JOYSTICK_AUTO) && (numJoys > 0))
    {
        Log("Joystick #%d not available. Connected joysticks:", Config.UseJoystick);
        for (i = 0; i < numJoys; i++)
        {
            Log("#%d = \"%s\"", i + 1, SDL_GetJoystickNameForID(joys[i]));
        }
    }
    SDL_free(joys);
}

static void inpCloseJoystick(void)
{
    if (inpJoystick)
    {
        SDL_CloseJoystick(inpJoystick);
        inpJoystick = NULL;
        inpJoystickId = 0;
    }
}

static void inpInitJoystick(void)
{
    inpJoystick = NULL;
    inpJoystickId = 0;

    if (!Config.UseJoystick)
    {
        return;
    }

    if (!SDL_InitSubSystem(SDL_INIT_JOYSTICK))
    {
        Log("Failed to init joystick subsystem.");
        Config.UseJoystick = 0;
        return;
    }

    inpJoySubsystem = 1;
    SDL_SetJoystickEventsEnabled(true);

    /* a missing joystick is fine: it is opened as soon as it gets plugged in */
    inpOpenJoystickIfNeeded();
    if (!inpJoystick) Log("No joystick connected yet.");
}

static void inpQuitJoystick(void)
{
    if (inpJoySubsystem)
    {
        SDL_SetJoystickEventsEnabled(false);
        inpCloseJoystick();
        SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
        inpJoySubsystem = 0;
    }
}

/* ------------------------------------------------------------------------
 * Script mode (2026-11-21): plays input from a text file, for automated
 * tests and for playing without a screen. Enabled with DERCLOU_SCRIPT=file.
 *
 *   wait <ms>          pause before the next command
 *   key <name>         press and release a key (SDL key name: Return, Escape,
 *                      Up, Down, Left, Right, Space, F1 ... or a letter)
 *   click <x> <y>      left click at game coordinates (320 x 200)
 *   rclick <x> <y>     right click
 *   move <x> <y>       move the mouse
 *   shot <name>        save a screenshot as <name>.png in DERCLOU_SHOTDIR
 *   log <text>         write a line to the log
 *   quit               close the game
 *   # ...              comment
 * ------------------------------------------------------------------------ */
static FILE *inpScriptFile = NULL;
static uint32_t inpScriptNext = 0;
static uint32_t inpScriptUpAt = 0;
static Uint8 inpScriptUpButton = 0;
static int inpScriptUpX = 0;
static int inpScriptUpY = 0;
static char inpScriptShotDir[FILENAME_MAX] = ".";

static void inpScriptInit(void)
{
    const char *path = getenv("DERCLOU_SCRIPT");
    const char *dir = getenv("DERCLOU_SHOTDIR");

    if (dir) snprintf(inpScriptShotDir, sizeof(inpScriptShotDir), "%s", dir);
    if (!path) return;

    inpScriptFile = fopen(path, "r");
    if (inpScriptFile)
    {
        Log("Script mode: playing %s", path);
        inpScriptNext = SDL_GetTicks();
    }
    else
    {
        Log("Script mode: cannot open %s", path);
    }
}

static void inpScriptToWindow(int x, int y, int *wx, int *wy)
{
    *wx = gfxScalingOffsetX + x * gfxScalingFactor + gfxScalingFactor / 2;
    *wy = gfxScalingOffsetY + y * gfxScalingFactor + gfxScalingFactor / 2;
}

static void inpScriptPushMouse(Uint32 type, Uint8 button, int wx, int wy)
{
    SDL_Event ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = type;
    if (type == SDL_EVENT_MOUSE_MOTION)
    {
        ev.motion.x = (float)wx;
        ev.motion.y = (float)wy;
    }
    else
    {
        ev.button.button = button;
        ev.button.down = (type == SDL_EVENT_MOUSE_BUTTON_DOWN);
        ev.button.clicks = 1;
        ev.button.x = (float)wx;
        ev.button.y = (float)wy;
    }
    SDL_PushEvent(&ev);
}

static void inpScriptPushKey(SDL_Keycode sym)
{
    SDL_Event ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = SDL_EVENT_KEY_DOWN;
    ev.key.down = true;
    ev.key.key = sym;
    ev.key.scancode = SDL_GetScancodeFromKey(sym, NULL);
    SDL_PushEvent(&ev);
    ev.type = SDL_EVENT_KEY_UP;
    ev.key.down = false;
    SDL_PushEvent(&ev);
}

static void inpScriptPump(void)
{
    char line[512];
    uint32_t now = SDL_GetTicks();

    if (inpScriptUpButton && now >= inpScriptUpAt)
    {
        inpScriptPushMouse(SDL_EVENT_MOUSE_BUTTON_UP, inpScriptUpButton, inpScriptUpX, inpScriptUpY);
        inpScriptUpButton = 0;
    }

    while (inpScriptFile && !inpScriptUpButton && now >= inpScriptNext)
    {
        char cmd[32] = {0};
        char arg[480] = {0};
        int x = 0, y = 0, wx = 0, wy = 0;

        if (!fgets(line, sizeof(line), inpScriptFile))
        {
            Log("Script mode: end of script");
            fclose(inpScriptFile);
            inpScriptFile = NULL;
            return;
        }
        line[strcspn(line, "\r\n")] = '\0';
        if (sscanf(line, "%31s %479[^\n]", cmd, arg) < 1 || cmd[0] == '#') continue;

        if (!strcmp(cmd, "wait"))
        {
            inpScriptNext = now + (uint32_t)atoi(arg);
        }
        else if (!strcmp(cmd, "key"))
        {
            SDL_Keycode sym = SDL_GetKeyFromName(arg);
            if (sym == SDLK_UNKNOWN)
                Log("Script mode: unknown key '%s'", arg);
            else
                inpScriptPushKey(sym);
        }
        else if ((!strcmp(cmd, "click") || !strcmp(cmd, "rclick")) && sscanf(arg, "%d %d", &x, &y) == 2)
        {
            Uint8 button = (cmd[0] == 'r') ? SDL_BUTTON_RIGHT : SDL_BUTTON_LEFT;
            inpScriptToWindow(x, y, &wx, &wy);
            inpScriptPushMouse(SDL_EVENT_MOUSE_MOTION, 0, wx, wy);
            inpScriptPushMouse(SDL_EVENT_MOUSE_BUTTON_DOWN, button, wx, wy);
            /* release a little later, like a real click */
            inpScriptUpButton = button;
            inpScriptUpX = wx;
            inpScriptUpY = wy;
            inpScriptUpAt = now + 80;
        }
        else if (!strcmp(cmd, "move") && sscanf(arg, "%d %d", &x, &y) == 2)
        {
            inpScriptToWindow(x, y, &wx, &wy);
            inpScriptPushMouse(SDL_EVENT_MOUSE_MOTION, 0, wx, wy);
        }
        else if (!strcmp(cmd, "shot"))
        {
            char fileName[FILENAME_MAX + 512];
            snprintf(fileName, sizeof(fileName), "%s" DIR_SEPARATOR "%s.png", inpScriptShotDir, arg);
            gfxSaveScreenshotTo(fileName);
            Log("Script mode: screenshot %s", fileName);
        }
        else if (!strcmp(cmd, "log"))
        {
            Log("Script: %s", arg);
        }
        else if (!strcmp(cmd, "quit"))
        {
            SDL_Event ev;
            memset(&ev, 0, sizeof(ev));
            ev.type = SDL_EVENT_QUIT;
            SDL_PushEvent(&ev);
        }
        else
        {
            Log("Script mode: cannot parse '%s'", line);
        }
    }
}

/* settings menu: open or close the joystick after Config.UseJoystick changed */
void inpApplyJoystickConfig(void)
{
    if (!Config.UseJoystick)
    {
        inpQuitJoystick();
        return;
    }

    if (!inpJoySubsystem)
    {
        inpInitJoystick();
        return;
    }

    inpCloseJoystick();
    inpOpenJoystickIfNeeded();
}

void inpOpenAllInputDevs(void)
{
    IHandler.uch_EscStatus = 1;
    IHandler.uch_FunctionKeyStatus = 1;
    IHandler.uch_MouseStatus = 1;

    // 2014-07
    cursorInit = initMouseCursor();
    setMouseCursor(gfxScalingFactor);

    if (Config.UseJoystick)
    {
        inpInitJoystick();
    }

    inpScriptInit();
}

static void inpDoPseudoMultiTasking(void)
{
    // 2014-06-30 LucyG : rewritten
    static uint32_t timePrev = 0;
    uint32_t timeNow = 0;
    uint32_t timePassed = 0;
    timeNow = SDL_GetTicks();
    if (!timePrev)
    {
        timePrev = timeNow;
    }
    timePassed = timeNow - timePrev;
    if (timePassed >= INP_TICKS_TO_MS(1))
    {
        timePrev = timeNow;

        sndDoFading();  // 2014-07-17 LucyG

        animator();
    }
}

void inpCloseAllInputDevs(void)
{
    // remove mouse cursor
    removeMouseCursor();

    if (Config.UseJoystick)
    {
        inpQuitJoystick();
    }
}

void inpDelay(int32_t l_Ticks)
{
    // 2014-07-03 LucyG : rewritten
    uint32_t timePrev = 0;
    timePrev = SDL_GetTicks();
    l_Ticks = INP_TICKS_TO_MS(l_Ticks);
    while ((SDL_GetTicks() - timePrev) < l_Ticks)
    {
        wfr();
        inpDoPseudoMultiTasking();
    }
}

void inpGetMouseXY(struct RastPort *p_RP, uword *p_X, uword *p_Y)
{
    *p_X = IHandler.us_MouseX - p_RP->us_LeftEdge;
    *p_Y = IHandler.us_MouseY - p_RP->us_TopEdge;
}

uword inpGetMouseY(struct RastPort *p_RP) { return (IHandler.us_MouseY) - p_RP->us_TopEdge; }

void inpSetWaitTicks(int32_t l_Ticks)
{
    // 2018-09-26 : timing fixes
    if (l_Ticks < INP_AS_FAST_AS_POSSIBLE)
    {
        l_Ticks = INP_AS_FAST_AS_POSSIBLE;
    }
    IHandler.ul_WaitTicks = l_Ticks;
}

void inpTurnESC(uword us_NewStatus) { IHandler.uch_EscStatus = (ubyte)us_NewStatus; }

void inpTurnFunctionKey(uword us_NewStatus) { IHandler.uch_FunctionKeyStatus = (ubyte)us_NewStatus; }

void inpTurnMouse(uword us_NewStatus) { IHandler.uch_MouseStatus = (ubyte)us_NewStatus; }

int32_t inpWaitFor(int32_t l_Mask)
{
    int32_t action = 0;
    uint32_t WaitTime = 0;

    uint32_t timePrev = SDL_GetTicks();

    SDL_Event event;
    SDL_Keycode sym = (SDL_Keycode)0;

    if (IHandler.uch_EscStatus && !(l_Mask & INP_NO_ESC)) l_Mask |= INP_ESC;
    if (IHandler.uch_FunctionKeyStatus) l_Mask |= INP_FUNCTION_KEY;

    while (!action)
    {
        inpScriptPump();

        while (SDL_PollEvent(&event))
        {
            /* window to game screen coordinates; script events (window 0) already are */
            gfxConvertEventCoordinates(&event);

            switch (event.type)
            {
                case SDL_EVENT_KEY_DOWN:
                    sym = event.key.key;
                    if ((sym == SDLK_TAB) && !event.key.repeat)
                    {
                        inpFastForwardOn = !inpFastForwardOn;
                        break;
                    }
                    /* Alt+Enter: toggle fullscreen (handled here so Enter does not also confirm) */
                    if (((sym == SDLK_RETURN) || (sym == SDLK_KP_ENTER)) && (event.key.mod & SDL_KMOD_ALT))
                    {
                        if (!event.key.repeat) gfxToggleFullscreen();
                        break;
                    }
                    if ((l_Mask & INP_LEFT) && (sym == SDLK_LEFT))
                    {
                        action |= INP_KEYBOARD | INP_LEFT;
                    }
                    else if ((l_Mask & INP_RIGHT) && (sym == SDLK_RIGHT))
                    {
                        action |= INP_KEYBOARD | INP_RIGHT;
                    }
                    if ((l_Mask & INP_UP) && (sym == SDLK_UP))
                    {
                        action |= INP_KEYBOARD | INP_UP;
                    }
                    else if ((l_Mask & INP_DOWN) && (sym == SDLK_DOWN))
                    {
                        action |= INP_KEYBOARD | INP_DOWN;
                    }
                    break;
                case SDL_EVENT_KEY_UP:
                    sym = event.key.key;
                    if ((l_Mask & (INP_LBUTTONP | INP_LBUTTONR)) &&
                        ((sym == SDLK_SPACE) || (sym == SDLK_RETURN) || (sym == SDLK_KP_ENTER)))
                    {
                        action |= INP_KEYBOARD | INP_LBUTTONP;
                    }
                    if (IHandler.uch_EscStatus && (l_Mask & INP_ESC) && (sym == SDLK_ESCAPE))
                    {
                        action |= INP_KEYBOARD | INP_ESC;
                    }
                    if (IHandler.uch_FunctionKeyStatus && (l_Mask & INP_FUNCTION_KEY) &&
                        ((sym >= SDLK_F1) && (sym <= SDLK_F10)))
                    {
                        action |= INP_KEYBOARD | INP_FUNCTION_KEY;
                    }
                    switch (sym)
                    {
                        case SDLK_F11:
                        {
                            if (!event.key.repeat) gfxToggleFullscreen();
                        }
                        break;
                        case SDLK_F12:
                        {
                            if (event.key.mod & SDL_KMOD_SHIFT)
                                gfxScreenshotShadow();  // Shift+F12: save shadow surface (debug)
                            else
                                gfxScreenshot();  // 2014-07-13 LucyG : guess what F12 does
                        }
                        break;
                        case SDLK_INSERT:
                        {
                            Config.MusicVolume += 25;
                            if (Config.MusicVolume > SND_MAX_VOLUME)
                            {
                                Config.MusicVolume = SND_MAX_VOLUME;
                            }
                        }
                        break;
                        case SDLK_DELETE:
                        {
                            Config.MusicVolume -= 25;
                            if (Config.MusicVolume < 0)
                            {
                                Config.MusicVolume = 0;
                            }
                        }
                        break;
                        case SDLK_HOME:
                        {
                            Config.SfxVolume += 25;
                            if (Config.SfxVolume > SND_MAX_VOLUME)
                            {
                                Config.SfxVolume = SND_MAX_VOLUME;
                            }
                        }
                        break;
                        case SDLK_END:
                        {
                            Config.SfxVolume -= 25;
                            if (Config.SfxVolume < 0)
                            {
                                Config.SfxVolume = 0;
                            }
                        }
                        break;
                        case SDLK_PAGEUP:
                        {
                            Config.VoiceVolume += 25;
                            if (Config.VoiceVolume > SND_MAX_VOLUME)
                            {
                                Config.VoiceVolume = SND_MAX_VOLUME;
                            }
                        }
                        break;
                        case SDLK_PAGEDOWN:
                        {
                            Config.VoiceVolume -= 25;
                            if (Config.VoiceVolume < 0)
                            {
                                Config.VoiceVolume = 0;
                            }
                        }
                        break;
                        default:
                            // nothing to do
                            break;
                    }
                    break;
                case SDL_EVENT_MOUSE_MOTION:
                {
                    int mx = ((int)event.motion.x - gfxScalingOffsetX) / gfxScalingFactor;
                    int my = ((int)event.motion.y - gfxScalingOffsetY) / gfxScalingFactor;
                    if ((l_Mask & INP_LEFT) && (mx < IHandler.us_MouseX))
                    {
                        action |= INP_MOUSE | INP_LEFT;
                    }
                    else if ((l_Mask & INP_RIGHT) && (mx > IHandler.us_MouseX))
                    {
                        action |= INP_MOUSE | INP_RIGHT;
                    }
                    if ((l_Mask & INP_UP) && (my < IHandler.us_MouseY))
                    {
                        action |= INP_MOUSE | INP_UP;
                    }
                    else if ((l_Mask & INP_DOWN) && (my > IHandler.us_MouseY))
                    {
                        action |= INP_MOUSE | INP_DOWN;
                    }
                    IHandler.us_MouseX = mx;
                    IHandler.us_MouseY = my;
                    break;
                }
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    if ((l_Mask & INP_LBUTTONP) && (event.button.button == SDL_BUTTON_LEFT))
                    {
                        action |= INP_MOUSE | INP_LBUTTONP;
                    }
                    if ((l_Mask & INP_RBUTTONP) && (event.button.button == SDL_BUTTON_RIGHT))
                    {
                        action |= INP_MOUSE | INP_RBUTTONP;
                    }
                    IHandler.us_MouseX = ((int)event.button.x - gfxScalingOffsetX) / gfxScalingFactor;
                    IHandler.us_MouseY = ((int)event.button.y - gfxScalingOffsetY) / gfxScalingFactor;
                    break;
                case SDL_EVENT_MOUSE_BUTTON_UP:
                    if ((l_Mask & INP_LBUTTONR) && (event.button.button == SDL_BUTTON_LEFT))
                    {
                        action |= INP_MOUSE | INP_LBUTTONR;
                    }
                    if ((l_Mask & INP_RBUTTONR) && (event.button.button == SDL_BUTTON_RIGHT))
                    {
                        action |= INP_MOUSE | INP_RBUTTONR;
                    }
                    IHandler.us_MouseX = ((int)event.button.x - gfxScalingOffsetX) / gfxScalingFactor;
                    IHandler.us_MouseY = ((int)event.button.y - gfxScalingOffsetY) / gfxScalingFactor;
                    break;
                case SDL_EVENT_MOUSE_WHEEL:
                    if (event.wheel.y >= 0)
                    {
                        action |= INP_MOUSEWHEEL | INP_UP;
                    }
                    else
                    {
                        action |= INP_MOUSEWHEEL | INP_DOWN;
                    }
                    break;
                case SDL_EVENT_JOYSTICK_ADDED:
                    inpOpenJoystickIfNeeded();
                    break;
                case SDL_EVENT_JOYSTICK_REMOVED:
                    if (inpJoystick && (event.jdevice.which == inpJoystickId))
                    {
                        Log("Joystick disconnected.");
                        inpCloseJoystick();
                        inpOpenJoystickIfNeeded(); /* auto mode: switch to another one if present */
                    }
                    break;
                case SDL_EVENT_JOYSTICK_HAT_MOTION: /* d-pad: emulate cursor keys */
                    if (event.jhat.which != inpJoystickId) break;
                    if ((l_Mask & INP_LEFT) && (event.jhat.value & SDL_HAT_LEFT))
                    {
                        action |= INP_KEYBOARD | INP_LEFT;
                    }
                    else if ((l_Mask & INP_RIGHT) && (event.jhat.value & SDL_HAT_RIGHT))
                    {
                        action |= INP_KEYBOARD | INP_RIGHT;
                    }
                    if ((l_Mask & INP_UP) && (event.jhat.value & SDL_HAT_UP))
                    {
                        action |= INP_KEYBOARD | INP_UP;
                    }
                    else if ((l_Mask & INP_DOWN) && (event.jhat.value & SDL_HAT_DOWN))
                    {
                        action |= INP_KEYBOARD | INP_DOWN;
                    }
                    break;
                case SDL_EVENT_JOYSTICK_AXIS_MOTION: /* 2015-01-10 LucyG: emulate cursor keys with joystick */
                    if (event.jaxis.which != inpJoystickId) break;
                    if (event.jaxis.axis == 0)
                    {  // X axis
                        if ((l_Mask & INP_LEFT) && (event.jaxis.value < -INP_JOY_DEAD_ZONE))
                        {
                            action |= INP_KEYBOARD | INP_LEFT;
                        }
                        else if ((l_Mask & INP_RIGHT) && (event.jaxis.value > INP_JOY_DEAD_ZONE))
                        {
                            action |= INP_KEYBOARD | INP_RIGHT;
                        }
                    }
                    else if (event.jaxis.axis == 1)
                    {  // Y axis
                        if ((l_Mask & INP_UP) && (event.jaxis.value < -INP_JOY_DEAD_ZONE))
                        {
                            action |= INP_KEYBOARD | INP_UP;
                        }
                        else if ((l_Mask & INP_DOWN) && (event.jaxis.value > INP_JOY_DEAD_ZONE))
                        {
                            action |= INP_KEYBOARD | INP_DOWN;
                        }
                    }
                    break;
                case SDL_EVENT_JOYSTICK_BUTTON_UP: /* 2015-01-10 LucyG: emulate RETURN/SPACE with (any) joystick button */
                    if (event.jbutton.which != inpJoystickId) break;
                    if (l_Mask & (INP_LBUTTONP | INP_LBUTTONR))
                    {
                        action |= INP_KEYBOARD | INP_LBUTTONP;
                    }
                    break;
                case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                case SDL_EVENT_WINDOW_EXPOSED:
                    gfxRepresentSDL();
                    break;
                case SDL_EVENT_QUIT:
                    if (l_Mask & INP_QUIT)
                    {
                        action |= INP_QUIT;
                    }
                    else
                    {
                        /* window closed outside of a quit prompt: leave cleanly instead of ignoring it */
                        tcQuitGame();
                    }
                    break;
            }
        }
        inpDoPseudoMultiTasking();
        wfd();

        WaitTime = SDL_GetTicks() - timePrev;
        if ((l_Mask & INP_TIME) && (WaitTime >= INP_TICKS_TO_MS(IHandler.ul_WaitTicks))) action |= INP_TIME;
    }

    return action;
}

/*
 * rate = (x << 5) | y
 * x : Delay (0-3)
 * y : Repeat (0-31)
 */
void inpSetKeyRepeat(unsigned char rate)
{
    /* needs some experimenting... */
    // int x = (((rate & 0xe0) >> 5) + 1) << 7;
    // int y = ((rate & 0x1f) + 1) << 1;
    // TODO SDL2 int x = (((rate & 0xe0) >> 5) + 1) << 8;
    // TODO SDL2 int y = ((rate & 0x1f) + 1) << 4;
    // TODO SDL2 SDL_EnableKeyRepeat(x, y);
}
