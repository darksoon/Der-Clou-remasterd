/*  _________  _______
   / ___/ __ \/ __/ _ \      Der Clou!
  / /__/ /_/ /\ \/ ___/ Open Source Project
  \___/\____/___/_/ http://cosp.sourceforge.net
   Based on the original by neo Software GmbH
*/
#include "gfx/crt.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define CRT_GRILLE 3  /* red, green and blue stripes of the aperture grille */
#define CRT_PHASES 4  /* screen lines per game pixel at most (scale 4) */

/* palettes[scanline phase][grille phase][256] */
static uint32_t *crtPalettes = NULL;
static int crtWidth = 0, crtHeight = 0, crtFactor = 0, crtMode = -1;
static uint8_t crtPalette[256 * 4];

/* how bright each line inside one game pixel is; the last one is the dark gap between scanlines */
static double crtScanline(int line, int factor, int mode)
{
    static const double s2[] = {1.08, 0.62};
    static const double s3[] = {1.10, 1.10, 0.58};
    static const double s4[] = {1.02, 1.14, 1.14, 0.52};
    const double *s = (factor == 2) ? s2 : (factor == 3) ? s3 : s4;
    double v = 0.0;

    if ((factor < 2) || (factor > 4)) return 1.0; /* no room for scanlines at the original size */

    v = s[line];
    if ((line == factor - 1) && (mode != GFX_DISPLAY_FILTER_CRT)) v = 1.0 - (1.0 - v) * 0.85; /* softer */
    return v;
}

static double crtGrille(int phase, int colour, int factor, int mode)
{
    if ((mode != GFX_DISPLAY_FILTER_CRT) || (factor < 3)) return 1.0; /* needs 3 screen pixels per pixel */
    return (phase == colour) ? 1.10 : 0.86;
}

static int crtPrepare(const uint8_t *palette, int width, int height, int factor, int mode)
{
    int phase = 0, g = 0, i = 0;
    int phases = (factor < 1) ? 1 : (factor > CRT_PHASES) ? CRT_PHASES : factor;
    uint32_t *p = NULL;

    if (!crtPalettes)
    {
        crtPalettes = (uint32_t *)malloc(sizeof(uint32_t) * CRT_PHASES * CRT_GRILLE * 256);
        if (!crtPalettes) return 0;
    }

    if ((width == crtWidth) && (height == crtHeight) && (factor == crtFactor) && (mode == crtMode) &&
        !memcmp(palette, crtPalette, sizeof(crtPalette)))
    {
        return 1; /* nothing changed */
    }

    p = crtPalettes;
    for (phase = 0; phase < CRT_PHASES; phase++)
    {
        double s = crtScanline(phase % phases, factor, mode);
        for (g = 0; g < CRT_GRILLE; g++)
        {
            int fr = (int)lround(s * crtGrille(g, 0, factor, mode) * 256.0);
            int fg = (int)lround(s * crtGrille(g, 1, factor, mode) * 256.0);
            int fb = (int)lround(s * crtGrille(g, 2, factor, mode) * 256.0);
            for (i = 0; i < 256; i++)
            {
                int r = (palette[i * 4 + 0] * fr) >> 8;
                int gr = (palette[i * 4 + 1] * fg) >> 8;
                int b = (palette[i * 4 + 2] * fb) >> 8;
                *p++ = 0xff000000u | ((uint32_t)(r > 255 ? 255 : r) << 16) | ((uint32_t)(gr > 255 ? 255 : gr) << 8) |
                       (uint32_t)(b > 255 ? 255 : b);
            }
        }
    }

    memcpy(crtPalette, palette, sizeof(crtPalette));
    crtWidth = width;
    crtHeight = height;
    crtFactor = factor;
    crtMode = mode;
    return 1;
}

int gfxConvertWithDisplayFilter(const uint8_t *src, int srcPitch, const uint8_t *palette, uint32_t *dst,
                                int dstPitchPixels, int width, int height, int factor, int mode)
{
    int x = 0, y = 0;
    int phases = (factor < 1) ? 1 : (factor > CRT_PHASES) ? CRT_PHASES : factor;

    if (!crtPrepare(palette, width, height, factor, mode)) return 0;

    for (y = 0; y < height; y++)
    {
        const uint8_t *s = src + (size_t)y * srcPitch;
        uint32_t *d = dst + (size_t)y * dstPitchPixels;
        const uint32_t *row = crtPalettes + (size_t)(y % phases) * CRT_GRILLE * 256;

        const uint32_t *red = row, *green = row + 256, *blue = row + 512; /* grille phases */

        for (x = 0; x + 2 < width; x += 3)
        {
            d[x] = red[s[x]];
            d[x + 1] = green[s[x + 1]];
            d[x + 2] = blue[s[x + 2]];
        }
        for (; x < width; x++) d[x] = row[(x % CRT_GRILLE) * 256 + s[x]];
    }
    return 1;
}
