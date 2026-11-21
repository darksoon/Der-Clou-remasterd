/*  _________  _______
   / ___/ __ \/ __/ _ \      Der Clou!
  / /__/ /_/ /\ \/ ___/ Open Source Project
  \___/\____/___/_/ http://cosp.sourceforge.net
   Based on the original by neo Software GmbH
*/
/* Light pass for the 32 bit picture (Game+ night and weather).
   The game draws 8 bit palette pixels; lighting those would only give muddy colours, so the light
   is applied after the conversion to ARGB, in every display path (plain, Scale2x/3x/4x, scanlines,
   CRT). A light map holds one entry per game pixel of the 320 x 128 map area: a multiplier per
   colour channel (8.8 fixed point) and a value that is added afterwards (rain, haze, flash).
   Only pixels of the map whose palette index is below GFX_LIGHT_FIRST_FIXED_COLOUR are touched,
   so bubbles, portraits, the planner overlay and the menu below always stay as they are.
   Nothing here ever writes into a bitmap the game reads. */
#ifndef MODULE_GFX_LIGHT
#define MODULE_GFX_LIGHT

#include <stdint.h>

#define GFX_LIGHT_W 320
#define GFX_LIGHT_H 128
#define GFX_LIGHT_FIRST_FIXED_COLOUR 192 /* menu, bubble and text colours from here on */
#define GFX_LIGHT_ONE 256                /* multiplier 1.0 */

struct gfxLightMap
{
    uint16_t mul[GFX_LIGHT_W * GFX_LIGHT_H * 4]; /* r, g, b, unused; 256 = 1.0 */
    uint8_t add[GFX_LIGHT_W * GFX_LIGHT_H * 4];  /* r, g, b, unused */
    int darkHalf; /* palette 0..63 holds the halved colours of a dark area: count them twice as bright */
};

/* fills the map for the next frame from the composed 8 bit picture (320 x 200);
   returns 0 if the picture stays unlit this frame */
typedef int (*gfxLightFunc)(const uint8_t *pixels, int width, int height, struct gfxLightMap *map);

/* animated: the picture changes by itself (rain, lightning), so it is redrawn regularly */
extern void gfxSetLight(gfxLightFunc func, int animated);
extern int gfxLightIsOn(void);

/* internal to the gfx module */
extern int gfxLightPrepare(const uint8_t *pixels, int width, int height);
extern int gfxLightActive(void);
extern int gfxLightAnimated(void);
extern void gfxLightApply(const uint8_t *src, int srcPitch, uint32_t *dst, int dstPitchPixels, int factor, int offX,
                          int offY);
extern void gfxLightDone(void);

#endif
