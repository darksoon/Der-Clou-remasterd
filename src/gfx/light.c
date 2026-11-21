/*  _________  _______
   / ___/ __ \/ __/ _ \      Der Clou!
  / /__/ /_/ /\ \/ ___/ Open Source Project
  \___/\____/___/_/ http://cosp.sourceforge.net
   Based on the original by neo Software GmbH
*/
#include "gfx/light.h"

#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL.h>
#include "base/base.h"

static gfxLightFunc lightFunc = NULL;
static int lightAnimated = 0;
static int lightActive = 0; /* the map is valid for the current frame */
static struct gfxLightMap *lightMap = NULL;

/* time spent per frame, logged when the light is switched off */
static uint64_t lightMapTicks = 0, lightApplyTicks = 0;
static uint32_t lightMapFrames = 0, lightApplyFrames = 0;

static void lightLogTimes(void)
{
    double f = (double)SDL_GetPerformanceFrequency() / 1000.0;

    if (!lightMapFrames || !lightApplyFrames) return;
    Log("Light: %u frames, light map %.2f ms, picture %.2f ms per frame", (unsigned)lightApplyFrames,
        (double)lightMapTicks / f / lightMapFrames, (double)lightApplyTicks / f / lightApplyFrames);
    lightMapTicks = lightApplyTicks = 0;
    lightMapFrames = lightApplyFrames = 0;
}

void gfxSetLight(gfxLightFunc func, int animated)
{
    if (!func) lightLogTimes();
    lightFunc = func;
    lightAnimated = func ? animated : 0;
    lightActive = 0;

    if (func && !lightMap) lightMap = (struct gfxLightMap *)calloc(1, sizeof(struct gfxLightMap));
    if (!lightMap) lightFunc = NULL;
}

int gfxLightIsOn(void) { return lightFunc != NULL; }

int gfxLightPrepare(const uint8_t *pixels, int width, int height)
{
    lightActive = 0;
    if (!lightFunc || !lightMap || (width < GFX_LIGHT_W) || (height < GFX_LIGHT_H)) return 0;

    {
        uint64_t t0 = SDL_GetPerformanceCounter();
        lightActive = lightFunc(pixels, width, height, lightMap) ? 1 : 0;
        lightMapTicks += SDL_GetPerformanceCounter() - t0;
        lightMapFrames++;
    }
    return lightActive;
}

int gfxLightActive(void) { return lightFunc && lightActive; }

int gfxLightAnimated(void) { return lightFunc && lightAnimated; }

void gfxLightDone(void)
{
    lightLogTimes();
    lightFunc = NULL;
    lightActive = 0;
    free(lightMap);
    lightMap = NULL;
}

static inline uint32_t lightChannel(uint32_t c, uint32_t m, uint32_t shift, uint32_t a)
{
    uint32_t v = ((c * m) >> shift) + a;
    return (v > 255) ? 255 : v;
}

static inline uint32_t lightPixel(uint32_t c, const uint16_t *m, const uint8_t *a, uint32_t shift)
{
    return (c & 0xff000000u) | (lightChannel((c >> 16) & 255, m[0], shift, a[0]) << 16) |
           (lightChannel((c >> 8) & 255, m[1], shift, a[1]) << 8) | lightChannel(c & 255, m[2], shift, a[2]);
}

/* src: the 8 bit screen (already scaled), dst: the same picture in ARGB. Both include the
   letterbox offsets; the map area starts at (offX, offY) and is 320 x 128 game pixels large. */
void gfxLightApply(const uint8_t *src, int srcPitch, uint32_t *dst, int dstPitchPixels, int factor, int offX,
                   int offY)
{
    int gy = 0;
    int k = 0;
    uint8_t darkLimit = 0;
    int rowWidth = 0;
    static uint32_t lightRowIn[GFX_LIGHT_W * 4]; /* first line of a game pixel row, unlit */
    uint64_t t0 = 0;

    if (!gfxLightActive() || (factor < 1)) return;

    t0 = SDL_GetPerformanceCounter();
    rowWidth = GFX_LIGHT_W * factor;
    darkLimit = lightMap->darkHalf ? 64 : 0;

    for (gy = 0; gy < GFX_LIGHT_H; gy++)
    {
        const uint16_t *mul = lightMap->mul + (size_t)gy * GFX_LIGHT_W * 4;
        const uint8_t *add = lightMap->add + (size_t)gy * GFX_LIGHT_W * 4;

        for (k = 0; k < factor; k++)
        {
            int sy = offY + gy * factor + k;
            const uint8_t *s = src + (size_t)sy * srcPitch + offX;
            uint32_t *d = dst + (size_t)sy * dstPitchPixels + offX;
            int gx = 0;

            /* without a display filter the lines of one game pixel are mostly the same: copy the lit line */
            if (k && rowWidth <= (int)(sizeof(lightRowIn) / sizeof(lightRowIn[0])))
            {
                const uint8_t *s0 = src + (size_t)(offY + gy * factor) * srcPitch + offX;
                uint32_t *d0 = dst + (size_t)(offY + gy * factor) * dstPitchPixels + offX;

                if (!memcmp(s, s0, (size_t)rowWidth) && !memcmp(d, lightRowIn, (size_t)rowWidth * 4))
                {
                    memcpy(d, d0, (size_t)rowWidth * 4);
                    continue;
                }
            }
            else if (!k && rowWidth <= (int)(sizeof(lightRowIn) / sizeof(lightRowIn[0])))
                memcpy(lightRowIn, d, (size_t)rowWidth * 4); /* this line before the light */

            for (gx = 0; gx < GFX_LIGHT_W; gx++, s += factor, d += factor)
            {
                const uint16_t *m = mul + gx * 4;
                const uint8_t *a = add + gx * 4;
                int kx = 0;

                /* the screen pixels of one game pixel mostly have the same colour: compute once */
                uint32_t lastIn = 0, lastOut = 0;
                int lastShift = -1;

                for (kx = 0; kx < factor; kx++)
                {
                    uint8_t idx = s[kx];
                    uint32_t c = 0;
                    int shift = 8;

                    if (idx >= GFX_LIGHT_FIRST_FIXED_COLOUR) continue;
                    if (idx < darkLimit) shift = 7; /* halved colour of a dark area: full colour times light */

                    c = d[kx];
                    if ((c != lastIn) || (shift != lastShift))
                    {
                        lastIn = c;
                        lastShift = shift;
                        lastOut = lightPixel(c, m, a, (uint32_t)shift);
                    }
                    d[kx] = lastOut;
                }
            }
        }
    }

    lightApplyTicks += SDL_GetPerformanceCounter() - t0;
    lightApplyFrames++;
}
