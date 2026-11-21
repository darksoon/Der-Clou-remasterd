/*  _________  _______
   / ___/ __ \/ __/ _ \      Der Clou!
  / /__/ /_/ /\ \/ ___/ Open Source Project
  \___/\____/___/_/ http://cosp.sourceforge.net
   Based on the original by neo Software GmbH
*/
#ifndef INPHDL_MODULE
#define INPHDL_MODULE

#include "theclou.h"

#ifndef MODULE_GFX
#include "gfx/gfx.h"
#endif

#ifndef MODULE_ANIM
#include "anim/sysanim.h"
#endif

#define INP_TICKS_PER_SECOND 60  // 2014-07-04
#define INP_TICKS_TO_MS(t) (((t)*1000) / INP_TICKS_PER_SECOND)

#define INP_AS_FAST_AS_POSSIBLE 2

#define INP_UP (1L << 0)
#define INP_DOWN (1L << 1)
#define INP_LEFT (1L << 2)
#define INP_RIGHT (1L << 3)
#define INP_ESC (1L << 4) /* wird standardmaessig gesetzt */

// these two defines have been exchanged by kaweh
#define INP_LBUTTONP (1L << 5) /* left button pressed */
#define INP_LBUTTONR (1L << 6) /* left button released */

#define INP_RBUTTONP (1L << 7) /* right button pressed */
#define INP_RBUTTONR (1L << 8) /* right button released */
#define INP_NO_ESC (1L << 10)  /* Esc Taste wird in WaitFor gesperrt */
#define INP_TIME (1L << 11)
#define INP_KEYBOARD (1L << 12) /* look at IH.keyCode for further information */
#define INP_FUNCTION_KEY (1L << 13)
#define INP_SPACE (1L << 14)
#define INP_MOUSE (1L << 15)
#define INP_MOUSEWHEEL (1L << 16)
#define INP_QUIT (1L << 17)

#define INP_BUTTON (INP_LBUTTONP | INP_RBUTTONP)
#define INP_MOVEMENT (INP_UP | INP_DOWN | INP_LEFT | INP_RIGHT)
#define INP_ALL_MODES                                                                                                 \
    (INP_UP | INP_DOWN | INP_LEFT | INP_RIGHT | INP_ESC | INP_LBUTTONP | INP_LBUTTONR | INP_RBUTTONP | INP_RBUTTONR | \
     INP_TIME | INP_MOUSE | INP_KEYBOARD | INP_SPACE)

/* global functions */

extern void inpOpenAllInputDevs(void);
extern void inpApplyJoystickConfig(void);
extern void inpCloseAllInputDevs(void);

extern void inpGetMouseXY(struct RastPort *p_RP, uword *p_X, uword *p_Y);
extern uword inpGetMouseY(struct RastPort *p_RP);

extern int32_t inpWaitFor(int32_t l_Mask); /* retourniert was passiert ist
                                            * (siehe defines) */
extern void inpSetWaitTicks(int32_t l_Ticks);

extern void inpTurnESC(uword us_NewStatus);         /* 0 means off, 1 means on */
extern void inpTurnFunctionKey(uword us_NewStatus); /* 0 means off, 1 means on */
extern void inpTurnMouse(uword us_NewStatus);       // 0 means off, 1 means on

extern void inpDelay(int32_t l_Ticks);
extern void inpSetKeyRepeat(unsigned char rate);
extern ubyte inpFastForward(void);
extern void inpSetFastForward(ubyte on);
extern void inpAllowFastForward(ubyte allowed); /* Tab only counts while the burglary playback runs */

extern void setMouseCursor(ubyte fact);

/* extra keys: the hook gets every key press (not repeats) before the game does and returns
   INP_HOOK_UNUSED, INP_HOOK_USED (key swallowed) or INP_HOOK_RETURN (key swallowed and
   inpWaitFor returns INP_KEYBOARD, if that was asked for) */
#define INP_HOOK_UNUSED 0
#define INP_HOOK_USED 1
#define INP_HOOK_RETURN 2
typedef int (*inpKeyHookFunc)(int32_t key);
extern void inpSetKeyHook(inpKeyHookFunc hook);

#endif
