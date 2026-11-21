/*  _________  _______
   / ___/ __ \/ __/ _ \      Der Clou!
  / /__/ /_/ /\ \/ ___/ Open Source Project
  \___/\____/___/_/ http://cosp.sourceforge.net
   Based on the original by neo Software GmbH
*/
/* optional display filters, computed on the CPU so they work everywhere (also in the browser):
   scanlines, or a CRT look with scanlines and aperture grille.
   The filter is applied while the 8 bit picture is converted to 32 bit: for every kind of screen line
   and column a ready made copy of the palette is prepared, so each pixel is a single table lookup. */
#ifndef MODULE_CRT
#define MODULE_CRT

#include <stdint.h>

#define GFX_DISPLAY_FILTER_OFF 0
#define GFX_DISPLAY_FILTER_SCANLINES 1
#define GFX_DISPLAY_FILTER_CRT 2
#define GFX_DISPLAY_FILTER_COUNT 3

/* src: 8 bit pixels, palette: 256 x RGBA (SDL_Color), dst: ARGB8888.
   factor: screen pixels per game pixel. Returns 0 if out of memory. */
extern int gfxConvertWithDisplayFilter(const uint8_t *src, int srcPitch, const uint8_t *palette, uint32_t *dst,
                                       int dstPitchPixels, int width, int height, int factor, int mode);

#endif
