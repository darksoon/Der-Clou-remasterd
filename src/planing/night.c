/*  _________  _______
   / ___/ __ \/ __/ _ \      Der Clou!
  / /__/ /_/ /\ \/ ___/ Open Source Project
  \___/\____/___/_/ http://cosp.sourceforge.net
   Based on the original by neo Software GmbH
*/
#include "planing/night.h"

#include <math.h>

#include "gameplay/gp.h"
#include "gfx/light.h"
#include "landscap/landscap_p.h"
#include "planing/planer.h"
#include "scenes/evidence.h"
#include "script/script.h"
#include "sound/newsound.h"

/* ------------------------------------------------------------------------
 * Game+ night and weather during the burglary
 *
 * Every frame a light map of the visible 320 x 128 map is built (one entry per game pixel)
 * and multiplied into the 32 bit picture by gfx/light.c:
 *   - an ambient light, darker inside the building than on the street,
 *   - a flashlight cone for every burglar in his view direction, a small warm lamp for
 *     every guard, a glow for the spotlights of the building (the game draws their circles),
 *   - moonlight falling through every window (Item_Fenster) into the room behind it,
 *   - the shadows the original draws below and beside the walls (colour 46/110) deeper and
 *     with a soft edge, so the walls read as having height,
 *   - rain streaks on the street, drifting fog, cloud shadows and lightning.
 *
 * Inside or outside: the floor square (32 x 32) decides. The street is the floor under the
 * escape car of the building, squares without floor count as outside too.
 * Light does not shine through walls: the visible picture is split into the regions the
 * walls enclose (the collision colours 0, 1, 64 and 65 the game checks in lsIsCollision),
 * and a light only reaches its own region. Doors and windows may let a little through,
 * small regions (wall interiors, furniture) take light from every side.
 *
 * Everything here only reads the game state. The weather of a night comes from a hash of the
 * game day and the building (PLUS/WEATHER.LUA decides with it), the lightning from an own
 * number generator: CalcRandomNr is never called.
 * ------------------------------------------------------------------------ */

#define NIGHT_W GFX_LIGHT_W
#define NIGHT_H GFX_LIGHT_H
#define NIGHT_AREAS 3
#define NIGHT_MAX_WINDOWS 48
#define NIGHT_MAX_COMPS 4000 /* regions of the visible picture */
#define NIGHT_SMALL_COMP 700 /* fewer pixels: wall interior or furniture, lit from every side */

#define NIGHT_ANY_COMP 0xffff  /* a light whose region is not known: inside/outside only */
#define NIGHT_NO_CLIP 0xfffe   /* a light that is not clipped at all (spotlights) */

#define NIGHT_INSIDE 0
#define NIGHT_OUTSIDE 1
#define NIGHT_VOID 2 /* no floor: the background around the building */

#define NIGHT_SHADOW_DARK 46    /* the original's wall shadow, dark half of the palette ... */
#define NIGHT_SHADOW_BRIGHT 110 /* ... and bright half */

enum
{
    NIGHT_CLEAR = 0,
    NIGHT_CLOUDY,
    NIGHT_RAIN,
    NIGHT_STORM,
    NIGHT_FOG,
    NIGHT_WEATHER_COUNT
};

struct nightWindow
{
    float cx, cy; /* middle of the opening at the inner face of the wall (area coordinates) */
    float nx, ny; /* into the room */
    float half;   /* half the width of the opening along the wall */
};

struct nightArea
{
    uint32_t id;
    int outdoorType; /* floor type of the street, -1 if this floor has none */
    struct LSFloorSquare *floor;
    int windowCount;
    struct nightWindow windows[NIGHT_MAX_WINDOWS];
};

/* which region a light may reach */
struct nightClip
{
    uword comp;
    ubyte where; /* NIGHT_INSIDE or NIGHT_OUTSIDE */
};

/* how a weather looks; light values are multipliers of the palette colour (1 = as drawn) */
struct nightLook
{
    float inside[3];  /* ambient inside the building */
    float outside[3]; /* ambient on the street */
    float moon[3];    /* light through the windows */
    float shaftLen;   /* how far it falls into the room */
    float reach;      /* length of the flashlight cones */
    float fog;        /* haze 0..1 */
    float clouds;     /* moving cloud shadows on the street 0..1 */
    int rain;         /* number of rain streaks */
    int lightning;
};

static const struct nightLook nightLooks[NIGHT_WEATHER_COUNT] = {
    /* clear: bright moon */
    {{0.29f, 0.31f, 0.42f}, {0.42f, 0.47f, 0.64f}, {0.62f, 0.72f, 1.00f}, 40.0f, 72.0f, 0.0f, 0.0f, 0, 0},
    /* cloudy */
    {{0.27f, 0.29f, 0.37f}, {0.36f, 0.38f, 0.47f}, {0.36f, 0.40f, 0.54f}, 30.0f, 68.0f, 0.0f, 0.55f, 0, 0},
    /* rain */
    {{0.26f, 0.28f, 0.36f}, {0.33f, 0.36f, 0.45f}, {0.30f, 0.35f, 0.48f}, 28.0f, 66.0f, 0.0f, 0.35f, 150, 0},
    /* thunderstorm */
    {{0.24f, 0.26f, 0.35f}, {0.30f, 0.32f, 0.42f}, {0.28f, 0.33f, 0.47f}, 28.0f, 66.0f, 0.0f, 0.45f, 210, 1},
    /* fog */
    {{0.30f, 0.31f, 0.35f}, {0.44f, 0.45f, 0.48f}, {0.30f, 0.33f, 0.38f}, 22.0f, 44.0f, 1.0f, 0.0f, 0, 0},
};

/* weather names in the game font: \x94 = ö */
static const char *nightName[NIGHT_WEATHER_COUNT][4] = {
    {"Clear night", "Klare Nacht", "Nuit claire", "Noche clara"},
    {"Cloudy", "Bew\x94lkt", "Nuageux", "Nublado"},
    {"Rain", "Regen", "Pluie", "Lluvia"},
    {"Thunderstorm", "Gewitter", "Orage", "Tormenta"},
    {"Fog", "Nebel", "Brouillard", "Niebla"},
};

static const char *nightLogName[NIGHT_WEATHER_COUNT] = {"clear", "cloudy", "rain", "storm", "fog"};

/* state */
static ubyte nightOn = 0;    /* light installed */
static ubyte nightSound = 0; /* rain loop started */
static int nightWeather = NIGHT_CLEAR;
static uint32_t nightRng = 1;   /* own numbers for lightning, never the game's */
static uint64_t nightStart = 0; /* SDL ticks when the playback started */
static uint64_t nightNextFlash = 0;
static uint64_t nightFlashStart = 0;
static float nightFlashPower = 0.0f;
static uint64_t nightThunderAt = 0;
static ubyte nightThunderClose = 0;
static float nightTorchAngle[PLANING_NR_PERSONS + PLANING_NR_GUARDS];
static ubyte nightTorchSet[PLANING_NR_PERSONS + PLANING_NR_GUARDS];

static struct nightArea nightAreas[NIGHT_AREAS];

/* per frame buffers of the visible map */
static uword nightComp[NIGHT_W * NIGHT_H]; /* region, 0 = wall line */
static int nightCompSize[NIGHT_MAX_COMPS];
static int nightStack[NIGHT_W * NIGHT_H];
static ubyte nightWhere[NIGHT_W * NIGHT_H]; /* NIGHT_INSIDE, NIGHT_OUTSIDE, NIGHT_VOID */
static float nightLight[NIGHT_W * NIGHT_H * 3];
static float nightRainAdd[NIGHT_W * NIGHT_H]; /* rain brightness, tinted when composed */
static float nightShade[NIGHT_W * NIGHT_H];
static int nightRegionsValid = 0; /* regions of the last frame, reused while nothing changed */
static int nightRegionsWx = 0, nightRegionsWy = 0;
static const struct nightArea *nightRegionsArea = NULL;

/* ------------------------------------------------------------------------
 * numbers
 * ------------------------------------------------------------------------ */

static uint32_t nightHash(uint32_t a)
{
    a ^= a >> 16;
    a *= 0x7feb352dU;
    a ^= a >> 15;
    a *= 0x846ca68bU;
    a ^= a >> 16;
    return a;
}

static uint32_t nightNext(void)
{
    nightRng ^= nightRng << 13;
    nightRng ^= nightRng >> 17;
    nightRng ^= nightRng << 5;
    return nightRng;
}

static int nightRange(int lo, int hi) { return lo + (int)(nightNext() % (uint32_t)(hi - lo + 1)); }

/* smooth value noise for fog and clouds, in area coordinates */
static float nightLattice(int x, int y)
{
    return (float)(nightHash((uint32_t)x * 73856093U ^ (uint32_t)y * 19349663U ^ 0x5bd1e995U) & 0xffff) / 65535.0f;
}

static float nightNoise(float x, float y)
{
    int ix = (int)floorf(x);
    int iy = (int)floorf(y);
    float fx = x - (float)ix;
    float fy = y - (float)iy;
    float a = nightLattice(ix, iy), b = nightLattice(ix + 1, iy);
    float c = nightLattice(ix, iy + 1), d = nightLattice(ix + 1, iy + 1);
    float top = 0.0f, bottom = 0.0f;

    fx = fx * fx * (3.0f - 2.0f * fx);
    fy = fy * fy * (3.0f - 2.0f * fy);
    top = a + (b - a) * fx;
    bottom = c + (d - c) * fx;
    return top + (bottom - top) * fy;
}

/* noise on a coarse grid of the visible picture (every 8 pixels), filled in between */
#define NIGHT_NOISE_STEP 8
#define NIGHT_NOISE_W (NIGHT_W / NIGHT_NOISE_STEP + 2)
#define NIGHT_NOISE_H (NIGHT_H / NIGHT_NOISE_STEP + 2)

static void nightNoiseGrid(float *grid, int wx, int wy, float dx, float dy, float scale)
{
    int gx = 0, gy = 0;

    for (gy = 0; gy < NIGHT_NOISE_H; gy++)
        for (gx = 0; gx < NIGHT_NOISE_W; gx++)
            grid[gy * NIGHT_NOISE_W + gx] = nightNoise(((float)(gx * NIGHT_NOISE_STEP + wx) + dx) / scale,
                                                       ((float)(gy * NIGHT_NOISE_STEP + wy) + dy) / scale);
}

static float nightNoiseAt(const float *grid, int x, int y)
{
    int gx = x / NIGHT_NOISE_STEP, gy = y / NIGHT_NOISE_STEP;
    float fx = (float)(x % NIGHT_NOISE_STEP) / NIGHT_NOISE_STEP, fy = (float)(y % NIGHT_NOISE_STEP) / NIGHT_NOISE_STEP;
    const float *g = grid + gy * NIGHT_NOISE_W + gx;
    float top = g[0] + (g[1] - g[0]) * fx;
    float bottom = g[NIGHT_NOISE_W] + (g[NIGHT_NOISE_W + 1] - g[NIGHT_NOISE_W]) * fx;

    return top + (bottom - top) * fy;
}

static float nightClamp01(float v) { return (v < 0.0f) ? 0.0f : (v > 1.0f) ? 1.0f : v; }

/* ------------------------------------------------------------------------
 * building data
 * ------------------------------------------------------------------------ */

/* inside, outside or no floor at a point of an area */
static ubyte nightWhereAt(const struct nightArea *a, int x, int y)
{
    ubyte f = 0;

    if (!a || !a->floor || (x < 0) || (y < 0) || (x >= LS_MAX_AREA_WIDTH) || (y >= LS_MAX_AREA_HEIGHT))
        return NIGHT_VOID;

    f = a->floor[(y / LS_FLOOR_Y_SIZE) * LS_FLOORS_PER_LINE + (x / LS_FLOOR_X_SIZE)].uch_FloorType;
    if (LS_NO_FLOOR(f)) return NIGHT_VOID;
    return ((int)(f & 0xf) == a->outdoorType) ? NIGHT_OUTSIDE : NIGHT_INSIDE;
}

/* the windows of a floor: the light falls to the side that lies inside the building */
static void nightReadArea(struct nightArea *a, uint32_t areaId, LIST *objects, struct LSFloorSquare *floor,
                          int outdoorType)
{
    NODE *n = NULL;

    memset(a, 0, sizeof(*a));
    a->id = areaId;
    a->floor = floor;
    a->outdoorType = outdoorType;

    for (n = objects ? LIST_HEAD(objects) : NULL; n && NODE_SUCC(n) && (a->windowCount < NIGHT_MAX_WINDOWS);
         n = (NODE *)NODE_SUCC(n))
    {
        LSObject lso = (LSObject)OL_DATA(n);
        uword x0 = 0, y0 = 0, x1 = 0, y1 = 0;
        struct nightWindow *w = NULL;
        int vertical = 0;
        ubyte sideA = 0, sideB = 0;
        float cx = 0.0f, cy = 0.0f;

        if (lso->Type != Item_Fenster) continue;

        lsCalcExactSize(lso, &x0, &y0, &x1, &y1);
        vertical = (lso->ul_Status & (1 << Const_tcHORIZ_VERT_BIT)) ? 1 : 0;
        cx = (x0 + x1) * 0.5f;
        cy = (y0 + y1) * 0.5f;

        if (vertical)
        {
            sideA = nightWhereAt(a, (int)x0 - 8, (int)cy); /* left */
            sideB = nightWhereAt(a, (int)x1 + 8, (int)cy); /* right */
        }
        else
        {
            sideA = nightWhereAt(a, (int)cx, (int)y0 - 8); /* above */
            sideB = nightWhereAt(a, (int)cx, (int)y1 + 8); /* below */
        }

        /* only windows between inside and outside let the night in */
        if ((sideA == NIGHT_INSIDE) == (sideB == NIGHT_INSIDE)) continue;

        w = &a->windows[a->windowCount++];
        w->nx = vertical ? ((sideA == NIGHT_INSIDE) ? -1.0f : 1.0f) : 0.0f;
        w->ny = vertical ? 0.0f : ((sideA == NIGHT_INSIDE) ? -1.0f : 1.0f);
        w->half = vertical ? (y1 - y0) * 0.5f : (x1 - x0) * 0.5f;
        if (w->half < 3.0f) w->half = 3.0f;
        w->cx = vertical ? ((sideA == NIGHT_INSIDE) ? (float)x0 : (float)x1) : cx;
        w->cy = vertical ? cy : ((sideA == NIGHT_INSIDE) ? (float)y0 : (float)y1);
    }
}

static const struct nightArea *nightFindArea(uint32_t areaId)
{
    int i = 0;

    for (i = 0; i < NIGHT_AREAS; i++)
        if (nightAreas[i].id == areaId) return &nightAreas[i];
    return NULL;
}

/* ------------------------------------------------------------------------
 * regions of the visible picture
 * ------------------------------------------------------------------------ */

static int nightIsWall(uint8_t idx) { return (idx < 128) && ((idx & 63) <= 1); }

static void nightRegions(const uint8_t *pixels, int width, const struct nightArea *a, int wx, int wy)
{
    const uword unvisited = 0, wall = 0xffff;
    static uint8_t lastKind[NIGHT_W * NIGHT_H];
    static uint8_t kind[NIGHT_W * NIGHT_H];
    int comps = 1;
    int i = 0, x = 0, y = 0;

    /* the regions only change when the picture scrolls or a wall line changes (a door) */
    for (y = 0; y < NIGHT_H; y++)
    {
        const uint8_t *p = pixels + y * width;
        uint8_t *k = kind + y * NIGHT_W;

        for (x = 0; x < NIGHT_W; x++)
            k[x] = (uint8_t)(nightIsWall(p[x]) | ((((p[x] < 128) && ((p[x] & 63) == 0))) << 1));
    }
    if (nightRegionsValid && (wx == nightRegionsWx) && (wy == nightRegionsWy) && (a == nightRegionsArea) &&
        !memcmp(kind, lastKind, sizeof(kind)))
        return;
    memcpy(lastKind, kind, sizeof(kind));
    nightRegionsValid = 1;
    nightRegionsWx = wx;
    nightRegionsWy = wy;
    nightRegionsArea = a;

    nightCompSize[0] = 0;
    nightCompSize[NIGHT_MAX_COMPS - 1] = 0;

    for (y = 0; y < NIGHT_H; y++)
    {
        uword *comp = nightComp + y * NIGHT_W;
        ubyte *where = nightWhere + y * NIGHT_W;
        ubyte rowWhere[NIGHT_W / LS_FLOOR_X_SIZE + 2];
        int sq = 0;

        /* one look per floor square of this row */
        for (sq = 0; sq < NIGHT_W / LS_FLOOR_X_SIZE + 2; sq++)
            rowWhere[sq] = nightWhereAt(a, ((wx / LS_FLOOR_X_SIZE) + sq) * LS_FLOOR_X_SIZE, y + wy);

        for (x = 0; x < NIGHT_W; x++)
        {
            uint8_t k = kind[y * NIGHT_W + x];

            where[x] = (k & 2) ? NIGHT_VOID : rowWhere[(x + wx) / LS_FLOOR_X_SIZE - wx / LS_FLOOR_X_SIZE];
            comp[x] = (k & 1) ? wall : unvisited;
        }
    }

    /* flood fill every region that is not a wall line (4 neighbours) */
    for (i = 0; i < NIGHT_W * NIGHT_H; i++)
    {
        int top = 0;
        int size = 0;
        uword label = 0;

        if (nightComp[i] != unvisited) continue;

        if (comps < NIGHT_MAX_COMPS - 1)
        {
            label = (uword)comps++;
            nightCompSize[label] = 0;
        }
        else
            label = NIGHT_MAX_COMPS - 1; /* very many tiny regions share the last one */

        nightComp[i] = label;
        nightStack[top++] = i;

        while (top)
        {
            int j = nightStack[--top];
            int jx = j % NIGHT_W;

            size++;
            if ((jx > 0) && (nightComp[j - 1] == unvisited))
            {
                nightComp[j - 1] = label;
                nightStack[top++] = j - 1;
            }
            if ((jx < NIGHT_W - 1) && (nightComp[j + 1] == unvisited))
            {
                nightComp[j + 1] = label;
                nightStack[top++] = j + 1;
            }
            if ((j >= NIGHT_W) && (nightComp[j - NIGHT_W] == unvisited))
            {
                nightComp[j - NIGHT_W] = label;
                nightStack[top++] = j - NIGHT_W;
            }
            if ((j < NIGHT_W * (NIGHT_H - 1)) && (nightComp[j + NIGHT_W] == unvisited))
            {
                nightComp[j + NIGHT_W] = label;
                nightStack[top++] = j + NIGHT_W;
            }
        }
        nightCompSize[label] += size;
    }

    for (i = 0; i < NIGHT_W * NIGHT_H; i++)
        if (nightComp[i] == wall) nightComp[i] = 0;
}

/* the region a light at (x, y) of the picture belongs to */
static struct nightClip nightClipAt(const struct nightArea *a, float fx, float fy, int wx, int wy)
{
    struct nightClip c;
    int x = (int)fx, y = (int)fy;
    int r = 0, dx = 0, dy = 0;

    c.where = nightWhereAt(a, x + wx, y + wy);
    if (c.where == NIGHT_VOID) c.where = NIGHT_OUTSIDE;
    c.comp = NIGHT_ANY_COMP;

    if ((x < 0) || (y < 0) || (x >= NIGHT_W) || (y >= NIGHT_H)) return c;

    /* the nearest pixel of a large region (the person stands on it) */
    for (r = 0; r <= 4; r++)
    {
        for (dy = -r; dy <= r; dy++)
        {
            for (dx = -r; dx <= r; dx++)
            {
                int px = x + dx, py = y + dy;
                uword comp = 0;

                if ((px < 0) || (py < 0) || (px >= NIGHT_W) || (py >= NIGHT_H)) continue;
                comp = nightComp[py * NIGHT_W + px];
                if (comp && (nightCompSize[comp] >= NIGHT_SMALL_COMP))
                {
                    c.comp = comp;
                    return c;
                }
            }
        }
    }
    return c;
}

static int nightVisible(int i, struct nightClip clip)
{
    uword comp = 0;

    if (nightWhere[i] == NIGHT_VOID) return 0;
    if (clip.comp == NIGHT_NO_CLIP) return 1;

    comp = nightComp[i];
    if (!comp) return 1; /* the wall line itself */
    if (nightWhere[i] != clip.where) return 0;
    if ((clip.comp == NIGHT_ANY_COMP) || (nightCompSize[comp] < NIGHT_SMALL_COMP)) return 1;
    return comp == clip.comp;
}

/* ------------------------------------------------------------------------
 * lights
 * ------------------------------------------------------------------------ */

static void nightAddAt(int i, const float *col, float k)
{
    nightLight[i * 3 + 0] += col[0] * k;
    nightLight[i * 3 + 1] += col[1] * k;
    nightLight[i * 3 + 2] += col[2] * k;
}

static int nightBox(float x, float y, float r, int *x0, int *y0, int *x1, int *y1)
{
    *x0 = (int)(x - r);
    *x1 = (int)(x + r) + 1;
    *y0 = (int)(y - r);
    *y1 = (int)(y + r) + 1;
    if ((*x1 < 0) || (*y1 < 0) || (*x0 >= NIGHT_W) || (*y0 >= NIGHT_H)) return 0;
    if (*x0 < 0) *x0 = 0;
    if (*y0 < 0) *y0 = 0;
    if (*x1 >= NIGHT_W) *x1 = NIGHT_W - 1;
    if (*y1 >= NIGHT_H) *y1 = NIGHT_H - 1;
    return 1;
}

/* round light: full up to inner, fading to nothing at outer (picture coordinates) */
static void nightLamp(float x, float y, float inner, float outer, const float *col, float k, struct nightClip clip)
{
    int x0 = 0, y0 = 0, x1 = 0, y1 = 0, px = 0, py = 0;

    if (!nightBox(x, y, outer, &x0, &y0, &x1, &y1)) return;

    for (py = y0; py <= y1; py++)
    {
        for (px = x0; px <= x1; px++)
        {
            int i = py * NIGHT_W + px;
            float dx = (float)px + 0.5f - x, dy = (float)py + 0.5f - y;
            float d = sqrtf(dx * dx + dy * dy);
            float f = 0.0f;

            if (d >= outer) continue;
            if (!nightVisible(i, clip)) continue;

            f = (d <= inner) ? 1.0f : 1.0f - (d - inner) / (outer - inner);
            nightAddAt(i, col, k * f * f);
        }
    }
}

/* flashlight: a soft cone from (x, y) in direction angle */
static void nightCone(float x, float y, float angle, float len, const float *col, float k, struct nightClip clip)
{
    const float cosIn = 0.95f;  /* about 18 degrees: full light */
    const float cosOut = 0.83f; /* about 34 degrees: no light */
    const float spread = 0.59f; /* acos(cosOut) */
    float dx = cosf(angle), dy = sinf(angle);
    float ex[3], ey[3];
    float minX = x, maxX = x, minY = y, maxY = y;
    int x0 = 0, y0 = 0, x1 = 0, y1 = 0, px = 0, py = 0, e = 0;

    /* only the box around the beam: the lamp and the far ends of both edges and the middle */
    ex[0] = x + len * cosf(angle - spread), ey[0] = y + len * sinf(angle - spread);
    ex[1] = x + len * cosf(angle + spread), ey[1] = y + len * sinf(angle + spread);
    ex[2] = x + len * dx, ey[2] = y + len * dy;
    for (e = 0; e < 3; e++)
    {
        if (ex[e] < minX) minX = ex[e];
        if (ex[e] > maxX) maxX = ex[e];
        if (ey[e] < minY) minY = ey[e];
        if (ey[e] > maxY) maxY = ey[e];
    }
    x0 = (int)minX - 1, x1 = (int)maxX + 1, y0 = (int)minY - 1, y1 = (int)maxY + 1;
    if ((x1 < 0) || (y1 < 0) || (x0 >= NIGHT_W) || (y0 >= NIGHT_H)) return;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= NIGHT_W) x1 = NIGHT_W - 1;
    if (y1 >= NIGHT_H) y1 = NIGHT_H - 1;

    for (py = y0; py <= y1; py++)
    {
        for (px = x0; px <= x1; px++)
        {
            int i = py * NIGHT_W + px;
            float vx = (float)px + 0.5f - x, vy = (float)py + 0.5f - y;
            float d = sqrtf(vx * vx + vy * vy);
            float c = 0.0f, f = 0.0f, edge = 0.0f;

            if ((d >= len) || (d < 1.0f)) continue;
            c = (vx * dx + vy * dy) / d;
            if (c <= cosOut) continue;
            if (!nightVisible(i, clip)) continue;

            edge = (c >= cosIn) ? 1.0f : (c - cosOut) / (cosIn - cosOut);
            f = 1.0f - d / len;
            /* brighter in the middle of the beam, soft start at the lamp */
            nightAddAt(i, col, k * edge * edge * f * (0.55f + 0.45f * f) * nightClamp01(d / 6.0f));
        }
    }
}

/* moonlight through a window: a slanting shaft into the room */
static void nightShaft(const struct nightWindow *w, const struct nightArea *a, int wx, int wy, const float *col,
                       float k, float len)
{
    const float skew = 0.45f;   /* the moon stands high and a little to the side */
    const float spread = 0.25f; /* the shaft widens */
    float tx = -w->ny, ty = w->nx; /* along the wall */
    float ox = w->cx - (float)wx, oy = w->cy - (float)wy;
    struct nightClip clip = nightClipAt(a, ox + w->nx * 5.0f, oy + w->ny * 5.0f, wx, wy);
    float reach = len + w->half * 2.0f + 8.0f;
    int x0 = 0, y0 = 0, x1 = 0, y1 = 0, px = 0, py = 0;

    clip.where = NIGHT_INSIDE;
    if (!nightBox(ox, oy, reach, &x0, &y0, &x1, &y1)) return;

    for (py = y0; py <= y1; py++)
    {
        for (px = x0; px <= x1; px++)
        {
            int i = py * NIGHT_W + px;
            float vx = (float)px + 0.5f - ox, vy = (float)py + 0.5f - oy;
            float u = vx * w->nx + vy * w->ny; /* into the room */
            float v = vx * tx + vy * ty;       /* along the wall */
            float edge = 0.0f, f = 0.0f, h = 0.0f;

            if ((u < -7.0f) || (u >= len)) continue; /* from the glass in the wall on */
            if (!nightVisible(i, clip)) continue;

            v -= u * skew;
            h = w->half + u * spread;
            edge = h + 2.5f - fabsf(v); /* soft sides */
            if (edge <= 0.0f) continue;
            edge = nightClamp01(edge / 3.0f);

            if (u < 0.0f)
            {
                /* the window itself shines */
                nightAddAt(i, col, k * edge * 0.9f);
                continue;
            }
            f = 1.0f - nightClamp01(u / len);
            nightAddAt(i, col, k * edge * f * sqrtf(f) * ((u < 2.0f) ? 1.3f : 1.0f));
        }
    }
}

/* ------------------------------------------------------------------------
 * lightning
 * ------------------------------------------------------------------------ */

static void nightPlanFlash(uint64_t now)
{
    if (Config.PlusFlashMs > 0)
        nightNextFlash = now + (uint64_t)Config.PlusFlashMs;
    else
        nightNextFlash = now + (uint64_t)nightRange(5000, 16000);
}

/* brightness of the lightning: a quick double flash, then fading; thunder follows */
static float nightFlash(uint64_t now)
{
    float t = 0.0f;

    if (!nightLooks[nightWeather].lightning) return 0.0f;

    if (now >= nightNextFlash)
    {
        int delay = nightRange(300, 2000);

        nightFlashStart = now;
        nightThunderClose = (delay < 900) ? 1 : 0;
        nightFlashPower = nightThunderClose ? 1.0f : 0.55f + (float)nightRange(0, 25) / 100.0f;
        nightThunderAt = now + (uint64_t)delay;
        nightPlanFlash(now);
        Log("Lightning, thunder in %d ms", delay);
    }

    if (nightThunderAt && (now >= nightThunderAt))
    {
        nightThunderAt = 0;
        sndPlayWeatherFX(nightThunderClose ? "THUNDER1.WAV" : "THUNDER2.WAV", nightThunderClose ? 100 : 70);
        Log("Thunder (%s)", nightThunderClose ? "close" : "far");
    }

    if (!nightFlashStart) return 0.0f;

    t = (float)(now - nightFlashStart);
    if (t < 70.0f) return nightFlashPower;
    if (t < 140.0f) return nightFlashPower * 0.12f;
    if (t < 230.0f) return nightFlashPower * 0.85f;
    if (t < 1600.0f) return nightFlashPower * 0.85f * expf(-(t - 230.0f) / 230.0f);
    nightFlashStart = 0;
    return 0.0f;
}

/* ------------------------------------------------------------------------
 * the light map of one frame
 * ------------------------------------------------------------------------ */

/* the original draws a shadow (colour 46/110) below and beside the walls: make it deeper with a
   soft edge, so the walls read as having height */
static void nightShadows(const uint8_t *pixels, int width)
{
    static uint8_t rowSum[NIGHT_W * NIGHT_H];
    float shade[26][2];
    int x = 0, y = 0, k = 0;

    for (k = 0; k <= 25; k++)
    {
        shade[k][0] = 1.0f - 0.30f * (float)k / 25.0f;
        shade[k][1] = shade[k][0] - 0.22f;
    }

    /* how many of the 5 x 5 pixels around are shadow: rows first, then columns */
    for (y = 0; y < NIGHT_H; y++)
    {
        const uint8_t *p = pixels + y * width;
        uint8_t mask[NIGHT_W + 4];
        uint8_t *sum = rowSum + y * NIGHT_W;

        mask[0] = mask[1] = mask[NIGHT_W + 2] = mask[NIGHT_W + 3] = 0;
        for (x = 0; x < NIGHT_W; x++) mask[x + 2] = (p[x] == NIGHT_SHADOW_DARK) || (p[x] == NIGHT_SHADOW_BRIGHT);
        for (x = 0; x < NIGHT_W; x++) sum[x] = mask[x] + mask[x + 1] + mask[x + 2] + mask[x + 3] + mask[x + 4];
    }

    for (y = 0; y < NIGHT_H; y++)
    {
        const uint8_t *p = pixels + y * width;
        const uint8_t *r[5];

        for (k = 0; k < 5; k++)
        {
            int yy = y + k - 2;
            r[k] = ((yy >= 0) && (yy < NIGHT_H)) ? rowSum + yy * NIGHT_W : NULL;
        }

        for (x = 0; x < NIGHT_W; x++)
        {
            int s = 0;
            int self = (p[x] == NIGHT_SHADOW_DARK) || (p[x] == NIGHT_SHADOW_BRIGHT);

            for (k = 0; k < 5; k++)
                if (r[k]) s += r[k][x];
            nightShade[y * NIGHT_W + x] = shade[s][self];
        }
    }
}

/* rain streaks on the street (anchored to the screen, falling and slanting a little) */
static void nightRain(uint64_t now, int drops)
{
    const int len = 6;
    const float speed = 0.30f; /* pixels per ms */
    int d = 0;

    for (d = 0; d < drops; d++)
    {
        uint32_t h = nightHash((uint32_t)d * 2654435761U + 17U);
        float period = (float)(NIGHT_H + 40) / speed * (0.85f + (float)(h & 255) / 900.0f);
        float phase = (float)((h >> 8) & 0xffff) / 65535.0f;
        float pos = (float)now / period + phase;
        uint32_t cycle = (uint32_t)pos;
        float t = pos - (float)cycle;
        uint32_t hx = nightHash(h ^ (cycle * 0x9e3779b9U));
        float y = t * (float)(NIGHT_H + 40) - 20.0f;
        float x = (float)(hx % (NIGHT_W + 60)) - y * 0.35f;
        int k = 0;

        for (k = 0; k < len; k++)
        {
            int px = (int)(x + (float)k * 0.35f);
            int py = (int)(y - (float)k);
            int i = 0;
            float f = 1.0f - (float)k / (float)len;

            if ((px < 0) || (py < 0) || (px >= NIGHT_W) || (py >= NIGHT_H)) continue;
            i = py * NIGHT_W + px;
            if (nightWhere[i] != NIGHT_OUTSIDE) continue; /* rain falls on the street, not into the rooms */

            nightRainAdd[i] += 44.0f * f;
        }

        /* a splash where some of the drops land */
        if (!(hx & 7) && (t > 0.9f))
        {
            int px = (int)x, py = (int)y;
            if ((px >= 0) && (py >= 0) && (px < NIGHT_W) && (py < NIGHT_H) &&
                (nightWhere[py * NIGHT_W + px] == NIGHT_OUTSIDE))
            {
                int i = py * NIGHT_W + px;
                nightRainAdd[i] += 30.0f;
            }
        }
    }
}

static float nightTargetAngle(ubyte view)
{
    switch (view)
    {
        case ANM_MOVE_UP:
            return -1.5707963f;
        case ANM_MOVE_LEFT:
            return 3.1415927f;
        case ANM_MOVE_RIGHT:
            return 0.0f;
        default:
            return 1.5707963f;
    }
}

/* turn the beam smoothly instead of jumping */
static float nightTurn(int person, float target)
{
    float a = nightTorchAngle[person];
    float d = 0.0f;

    if (!nightTorchSet[person])
    {
        nightTorchSet[person] = 1;
        nightTorchAngle[person] = target;
        return target;
    }

    d = target - a;
    while (d > 3.1415927f) d -= 6.2831853f;
    while (d < -3.1415927f) d += 6.2831853f;
    a += d * 0.35f;
    nightTorchAngle[person] = a;
    return a;
}

static int nightLightMap(const uint8_t *pixels, int width, int height, struct gfxLightMap *map)
{
    const struct nightLook *look = &nightLooks[nightWeather];
    uint32_t areaId = 0;
    const struct nightArea *area = NULL;
    LSArea lsArea = NULL;
    uint64_t now = SDL_GetTicks();
    float flash = 0.0f;
    float inside[3], outside[3], moon[3], voidAmb[3];
    static float cloudGrid[NIGHT_NOISE_W * NIGHT_NOISE_H], fogGrid[NIGHT_NOISE_W * NIGHT_NOISE_H];
    static const float rainTint[3] = {0.68f, 0.77f, 1.0f};
    float t = (float)(now - nightStart) / 1000.0f;
    float litBoost = 1.0f;
    uword wx = 0, wy = 0;
    int i = 0, c = 0;
    ubyte p = 0;

    (void)height;
    if (!nightOn || !ls) return 0;

    areaId = lsGetActivAreaID();
    area = nightFindArea(areaId);
    lsArea = (LSArea)dbGetObject(areaId);
    if (!lsArea) return 0;

    flash = nightFlash(now);
    lsGetWindowPos(&wx, &wy);

    /* a building with its lights on (no darkness in the original) stays a little brighter */
    if (!lsArea->uch_Darkness) litBoost = 1.5f;

    for (c = 0; c < 3; c++)
    {
        inside[c] = look->inside[c] * litBoost + flash * 0.26f;
        outside[c] = look->outside[c] + flash * (0.85f + 0.1f * (float)c);
        moon[c] = look->moon[c] * (1.0f + flash * 3.2f);
        voidAmb[c] = look->outside[c] * 0.6f; /* the background around the building, no lightning */
    }

    nightRegions(pixels, width, area, wx, wy);
    nightShadows(pixels, width);
    memset(nightLight, 0, sizeof(nightLight));
    if (look->rain) memset(nightRainAdd, 0, sizeof(nightRainAdd));

    /* moonlight through the windows */
    for (i = 0; area && (i < area->windowCount); i++)
        nightShaft(&area->windows[i], area, wx, wy, moon, 1.1f, look->shaftLen * (1.0f + flash * 0.5f));

    /* the spotlights of the building (the game draws their circles itself) */
    {
        LIST *spots = lsGetSpotList();
        struct Spot *s = NULL;
        static const float spotCol[3] = {1.0f, 0.97f, 0.88f};
        struct nightClip none;

        none.comp = NIGHT_NO_CLIP;
        none.where = NIGHT_INSIDE;

        for (s = spots ? (struct Spot *)LIST_HEAD(spots) : NULL; s && NODE_SUCC(s); s = (struct Spot *)NODE_SUCC(s))
        {
            float r = 0.0f;

            if (!(s->uch_Status & LS_SPOT_ON) || !s->p_CurrPos || (s->ul_AreaId != areaId)) continue;
            r = (float)s->us_Size * 0.5f;
            nightLamp((float)s->p_CurrPos->us_XPos + r - (float)wx, (float)s->p_CurrPos->us_YPos + r - (float)wy,
                      r * 0.8f, r * 1.45f, spotCol, 0.75f, none);
        }
    }

    /* burglars: flashlights; guards: lamps */
    for (p = 0; p < PersonsNr; p++)
    {
        static const float torch[3] = {1.00f, 0.93f, 0.76f};
        static const float lamp[3] = {1.00f, 0.74f, 0.42f};
        char *name = Planing_Name[p];
        struct nightClip clip;
        float x = 0.0f, y = 0.0f;

        if (livWhereIs(name) != areaId) continue;

        x = (float)livGetXPos(name) + 7.0f - (float)wx;
        y = (float)livGetYPos(name) + 7.0f - (float)wy;
        if ((x < -80.0f) || (y < -80.0f) || (x > NIGHT_W + 80.0f) || (y > NIGHT_H + 80.0f)) continue;

        clip = nightClipAt(area, x, y, wx, wy);

        if (p < BurglarsNr)
        {
            float a = nightTurn(p, nightTargetAngle(livGetViewDirection(name)));
            nightCone(x + cosf(a) * 3.0f, y + sinf(a) * 3.0f, a, look->reach, torch, 1.35f, clip);
            nightLamp(x, y, 3.0f, 12.0f, torch, 0.45f, clip); /* so the burglar himself can be seen */
        }
        else
        {
            float k = Planing_Guard[p - BurglarsNr] ? 0.35f : 0.8f; /* a knocked out guard dropped his lamp */
            nightLamp(x, y, 4.0f, (look->fog > 0.0f) ? 20.0f : 26.0f, lamp, k, clip);
        }
    }

    if (look->rain) nightRain(now, look->rain);

    if (look->clouds > 0.0f) nightNoiseGrid(cloudGrid, wx, wy, t * 9.0f, t * 4.0f, 70.0f);
    if (look->fog > 0.0f) nightNoiseGrid(fogGrid, wx, wy, t * 6.0f, -t * 2.0f, 48.0f);

    /* compose: ambient plus lights, times the wall shadows; fog and clouds */
    {
        const float *ambient[3];
        int extra = (look->fog > 0.0f) || (look->rain > 0);

        ambient[NIGHT_INSIDE] = inside;
        ambient[NIGHT_OUTSIDE] = outside;
        ambient[NIGHT_VOID] = voidAmb;
        if (!extra) memset(map->add, 0, sizeof(map->add));

        for (i = 0; i < NIGHT_W * NIGHT_H; i++)
        {
            ubyte where = nightWhere[i];
            const float *amb = ambient[where];
            const float *light = nightLight + i * 3;
            float shade = nightShade[i] * (float)GFX_LIGHT_ONE;
            float cloud = 1.0f;

            if ((look->clouds > 0.0f) && (where == NIGHT_OUTSIDE))
                cloud = 1.0f - look->clouds * 0.35f * nightNoiseAt(cloudGrid, i % NIGHT_W, i / NIGHT_W);

            for (c = 0; c < 3; c++)
            {
                int m = (int)((amb[c] * cloud + light[c]) * shade);
                map->mul[i * 4 + c] = (uint16_t)((m > 3 * GFX_LIGHT_ONE) ? 3 * GFX_LIGHT_ONE : m);
            }

            if (extra)
            {
                float haze = 0.0f;
                float rain = look->rain ? nightRainAdd[i] : 0.0f;

                if ((look->fog > 0.0f) && (where != NIGHT_VOID))
                {
                    float n = nightNoiseAt(fogGrid, i % NIGHT_W, i / NIGHT_W);
                    float glow = (light[0] + light[1] + light[2]) / 3.0f;
                    haze = look->fog * (((where == NIGHT_INSIDE) ? 10.0f : 22.0f) + 22.0f * n + 26.0f * glow);
                }

                for (c = 0; c < 3; c++)
                {
                    float a = haze * ((c == 2) ? 1.08f : 1.0f) + rain * rainTint[c];
                    map->add[i * 4 + c] = (uint8_t)((a > 255.0f) ? 255.0f : a);
                }
            }
        }
    }

    map->darkHalf = lsArea->uch_Darkness ? 1 : 0;
    return 1;
}

/* ------------------------------------------------------------------------
 * interface
 * ------------------------------------------------------------------------ */

static int nightChooseWeather(uint32_t buildingId)
{
    int32_t day = film ? (int32_t)GetDay : 0;
    uint32_t h = nightHash((uint32_t)day * 7919U ^ nightHash(buildingId ^ 0x4c6f6e64U));
    int w = -1;

    nightRng = nightHash(h ^ 0x52616e64U) | 1u; /* lightning and thunder of this night */

    if ((Config.PlusWeather >= CFG_WEATHER_CLEAR) && (Config.PlusWeather <= CFG_WEATHER_FOG))
    {
        w = Config.PlusWeather - CFG_WEATHER_CLEAR;
        Log("Weather: %s (fixed in the settings)", nightLogName[w]);
        return w;
    }

    w = scrWeather(day, buildingId, h % 10000U, nightHash(h + 1U));
    if ((w < 0) || (w >= NIGHT_WEATHER_COUNT))
    {
        /* no script: the chances of the original PLUS/WEATHER.LUA */
        static const int chance[NIGHT_WEATHER_COUNT] = {30, 25, 20, 10, 15};
        uint32_t pick = nightHash(h + 1U) % 100U;

        for (w = 0; w < NIGHT_WEATHER_COUNT - 1; w++)
        {
            if (pick < (uint32_t)chance[w]) break;
            pick -= (uint32_t)chance[w];
        }
    }
    Log("Weather: %s on day %d", nightLogName[w], (int)day);
    return w;
}

void plNightEnter(uint32_t buildingId)
{
    Building b = (Building)dbGetObject(buildingId);
    uint32_t startArea = 0;
    int outdoorType = -1;
    int i = 0, k = 0;

    if ((GameMode != GAME_MODE_PLUS) || (Config.PlusWeather == CFG_WEATHER_OFF) || !ls) return;

    nightWeather = nightChooseWeather(buildingId);

    /* the burglary starts at the escape car: the floor under it is the street */
    startArea = lsGetActivAreaID();
    for (k = 0; k < NIGHT_AREAS; k++)
    {
        if ((ls->ul_FloorAreaId[k] == startArea) && ls->p_AllFloors[k] && b)
        {
            ubyte f = ls->p_AllFloors[k][lsGetFloorIndex(b->CarXPos, b->CarYPos)].uch_FloorType;
            if (!LS_NO_FLOOR(f)) outdoorType = f & 0xf;
        }
    }

    memset(nightAreas, 0, sizeof(nightAreas));
    for (i = 0; i < NIGHT_AREAS; i++)
    {
        uint32_t id = ls->ul_FloorAreaId[i];
        LIST *objects = NULL;

        if (!id) continue;
        for (k = 0; k < NIGHT_AREAS; k++)
            if (ls->ul_ObjectRetrievalAreaId[k] == id) objects = ls->p_ObjectRetrievalLists[k];

        nightReadArea(&nightAreas[i], id, objects, ls->p_AllFloors[i], (id == startArea) ? outdoorType : -1);
        Log("Night: floor %u, %d windows", (unsigned)id, nightAreas[i].windowCount);
    }

    memset(nightTorchSet, 0, sizeof(nightTorchSet));
    nightRegionsValid = 0;
    nightStart = SDL_GetTicks();
    nightFlashStart = 0;
    nightThunderAt = 0;
    if (nightLooks[nightWeather].lightning)
    {
        if (Config.PlusFlashMs > 0)
            nightPlanFlash(nightStart);
        else
            nightNextFlash = nightStart + (uint64_t)nightRange(2500, 7000);
    }

    nightOn = 1;
    gfxSetLight(nightLightMap, 1);

    if ((nightWeather == NIGHT_RAIN) || (nightWeather == NIGHT_STORM))
    {
        nightSound = 1;
        sndSetWeatherAmbience("RAIN.WAV");
    }

    plNightShowWeather();
    gfxInvalidate();
}

void plNightLeave(void)
{
    if (!nightOn) return;

    nightOn = 0;
    gfxSetLight(NULL, 0);
    gfxInvalidate();
}

void plNightStopSound(void)
{
    if (!nightSound) return;

    nightSound = 0;
    sndPlayWeatherFX(NULL, 0);
    sndSetWeatherAmbience(NULL);
}

void plNightShowWeather(void)
{
    ubyte lang = txtGetLanguage();

    if (!nightOn) return;

    /* the police countdown uses the same place */
    if (Config.ShowPoliceTimer && (Search.EscapeBits & FAHN_ALARM) && Search.TimeOfAlarm) return;

    gfxSetPens(m_wrp, 0, 0, 0);
    gfxRectFill(m_wrp, 122, 0, 218, 10);
    gfxSetRect(122, 96);
    gfxSetPens(m_wrp, 248, GFX_SAME_PEN, GFX_SAME_PEN);
    gfxPrint(m_wrp, (char *)nightName[nightWeather][(lang <= 3) ? lang : 1], 2, GFX_PRINT_CENTER);
}
