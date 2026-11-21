/*  _________  _______
   / ___/ __ \/ __/ _ \      Der Clou!
  / /__/ /_/ /\ \/ ___/ Open Source Project
  \___/\____/___/_/ http://cosp.sourceforge.net
   Based on the original by neo Software GmbH
*/
#include "planing/plus.h"

#include "planing/planer.h"
#include "present/interac.h"

/* ------------------------------------------------------------------------
 * Game+ planner additions
 *
 * Guards (key W, on by default): every guard of the visible floor gets its patrol route
 * (dashed line, small squares where it stops) and the area it watches right now.
 * The area follows the rule of tcGuardDetectsGuy (data/dataappl.c), checked during the
 * burglary every 12 ticks for each guard that is not knocked out:
 *   - the burglar is on the same floor (area) as the guard,
 *   - he is on the side the guard looks to (livIsPositionInViewDirection: a half plane,
 *     no angle and no distance limit),
 *   - and a room rectangle of that floor contains both reference points
 *     (x + 8, y) of guard and burglar (tcInsideSameRoom).
 * The guard looks in its walking direction; during every other action (waiting, control,
 * opening) the guard sprite plays ANM_WORK_CONTROL, which livCorrectViewDirection turns into
 * looking down. So the watched area is the part of the guard's room(s) on that side.
 *
 * Timeline (key T in the planner menus): one row per burglar with his actions over time,
 * the total time, undo (the original's plRemLastAction, as Esc while walking) and redo.
 *
 * Nothing here is drawn into a bitmap the game logic reads (collision and object picking
 * read l_wrp): the overlay is painted into the per frame copy made by gfxUpdateSDL.
 * ------------------------------------------------------------------------ */

#define PLUS_MAP_W 320
#define PLUS_MAP_H 128

/* colours of the planning palette (menu range 192-255, the same on every floor) */
#define PLUS_COL_VIEW 198     /* red: watched area */
#define PLUS_COL_ROUTE 208    /* pale yellow: patrol route */
#define PLUS_COL_BLACK 197    /* black outline */
#define PLUS_COL_TEXT 248     /* light green: the planner's text colour */
#define PLUS_COL_WHITE 249    /* highlighted text */
#define PLUS_COL_PANEL 238    /* dark green panel */
#define PLUS_COL_TRACK 241    /* bar background */
#define PLUS_COL_FRAME 248    /* panel frame */
#define PLUS_COL_GRID 244     /* time grid */

enum
{
    PLUS_CAT_WALK = 0,
    PLUS_CAT_WAIT,
    PLUS_CAT_TOOL,
    PLUS_CAT_LOOT,
    PLUS_CAT_DOOR,
    PLUS_CAT_RADIO,
    PLUS_CAT_COUNT
};

static const ubyte plusCatColour[PLUS_CAT_COUNT] = {252, 228, 198, 208, 201, 251};

/* texts in the game font (code page 437): \x81 = ü, \x84 = ä, \x99 = Ö, \xe1 = ß, \x82 = é, \x85 = à, \xa1 = í */
static const char *plusCatName[4][PLUS_CAT_COUNT] = {
    {"Walk", "Wait", "Tool", "Take/Drop", "Open/Close", "Radio"},
    {"Gehen", "Warten", "Werkzeug", "Nehmen/Ablegen", "\x99" "ffnen/Schlie\xe1" "en", "Funk"},
    {"Marcher", "Attendre", "Outil", "Prendre/Poser", "Ouvrir/Fermer", "Radio"},
    {"Andar", "Esperar", "Herramienta", "Coger/Dejar", "Abrir/Cerrar", "Radio"}};

static const char *plusTxtTitle[4] = {"Timeline", "Zeitleiste", "Chronologie", "Cronolog\xa1" "a"};
static const char *plusTxtTotal[4] = {"Total", "Gesamt", "Total", "Total"};
static const char *plusTxtEmpty[4] = {"Nothing planned yet", "Noch nichts geplant", "Rien de planifi\x82",
                                      "Nada planeado"};
static const char *plusTxtKeys[4] = {"Z: undo   Y: redo   T: close", "Z: r\x81" "ckg\x84" "ngig   Y: wiederherstellen   T: schlie\xe1" "en",
                                     "Z: annuler   Y: r\x82" "tablir   T: fermer", "Z: deshacer   Y: rehacer   T: cerrar"};
static const char *plusTxtUndone[4] = {"Undone", "R\x81" "ckg\x84" "ngig", "Annul\x82", "Deshecho"};
static const char *plusTxtRedone[4] = {"Redone", "Wiederhergestellt", "R\x82" "tabli", "Rehecho"};
static const char *plusTxtNoUndo[4] = {"Nothing to undo", "Nichts zum R\x81" "ckg\x84" "ngigmachen",
                                       "Rien \x85 annuler", "Nada que deshacer"};
static const char *plusTxtNoRedo[4] = {"Nothing to redo", "Nichts wiederherzustellen", "Rien \x85 r\x82" "tablir",
                                       "Nada que rehacer"};

/* state */
static ubyte plusActive = 0;       /* hooks installed (Game+ planner) */
static ubyte plusGuardsOn = 1;     /* key W, kept between planner visits */
static ubyte plusInMenu = 0;       /* a planner menu waits: the timeline may open */
static ubyte plusTimelineOpen = 0; /* timeline shown */
static int plusCommand = 0;        /* key pressed inside the timeline */
static char plusStatus[80];        /* last undo or redo */

enum
{
    PLUS_CMD_NONE = 0,
    PLUS_CMD_CLOSE,
    PLUS_CMD_UNDO,
    PLUS_CMD_REDO
};

/* redo stack: actions removed by undo, newest last */
#define PLUS_REDO_MAX 128

struct plusRedo
{
    uword type;
    uword time;
    uint32_t data1;
    uint32_t data2;
};

static struct plusRedo plusRedoStack[PLUS_REDO_MAX];
static int plusRedoCount = 0;
static uint32_t plusRedoPerson = 0;  /* the stack belongs to this person ... */
static uint32_t plusRedoActions = 0; /* ... and to this plan state (number of actions, length) */
static uint32_t plusRedoLength = 0;

static ubyte plusLang(void)
{
    ubyte lang = txtGetLanguage();
    return (lang <= 3) ? lang : 1;
}

/* ------------------------------------------------------------------------
 * drawing into the composed picture with the game's own primitives
 * ------------------------------------------------------------------------ */

static struct RastPort plusRP;

static void plusInitRP(ubyte *pixels, int width, int height)
{
    memset(&plusRP, 0, sizeof(plusRP));
    plusRP.us_Width = (uword)width;
    plusRP.us_Height = (uword)height;
    plusRP.uch_VideoMode = GFX_VIDEO_MCGA;
    plusRP.uch_ColorEnd = 255;
    plusRP.us_DrawMode = GFX_JAM_1;
    plusRP.p_BitMap = pixels;
    plusRP.p_Font = bubbleFont;
}

static void plusPixel(int x, int y, ubyte col)
{
    if ((x >= 0) && (y >= 0) && (x < PLUS_MAP_W) && (y < PLUS_MAP_H)) ((ubyte *)plusRP.p_BitMap)[y * plusRP.us_Width + x] = col;
}

/* filled rectangle, clipped to the map */
static void plusRect(int x0, int y0, int x1, int y1, ubyte col)
{
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= PLUS_MAP_W) x1 = PLUS_MAP_W - 1;
    if (y1 >= PLUS_MAP_H) y1 = PLUS_MAP_H - 1;
    if ((x1 < x0) || (y1 < y0)) return;

    gfxSetPens(&plusRP, col, col, col);
    gfxRectFill(&plusRP, (uword)x0, (uword)y0, (uword)x1, (uword)y1);
}

/* horizontal or vertical line, clipped to the map */
static void plusLine(int x0, int y0, int x1, int y1, ubyte col)
{
    int t = 0;

    if (x0 > x1) t = x0, x0 = x1, x1 = t;
    if (y0 > y1) t = y0, y0 = y1, y1 = t;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= PLUS_MAP_W) x1 = PLUS_MAP_W - 1;
    if (y1 >= PLUS_MAP_H) y1 = PLUS_MAP_H - 1;
    if ((x1 < x0) || (y1 < y0)) return;

    gfxSetPens(&plusRP, col, GFX_SAME_PEN, GFX_SAME_PEN);
    gfxMoveCursor(&plusRP, (uword)x0, (uword)y0);
    gfxDraw(&plusRP, (uword)x1, (uword)y1);
}

static void plusFrame(int x0, int y0, int x1, int y1, ubyte col)
{
    plusLine(x0, y0, x1, y0, col);
    plusLine(x0, y1, x1, y1, col);
    plusLine(x0, y0, x0, y1, col);
    plusLine(x1, y0, x1, y1, col);
}

/* text in the bubble font with the usual shadow; cut at the right edge of the map */
static int plusText(int x, int y, const char *text, ubyte col)
{
    char buf[96];
    int maxChars = 0;
    int charW = bubbleFont->us_Width;

    if ((y < 0) || (y + bubbleFont->us_Height > PLUS_MAP_H - 1) || (x < 0) || (x >= PLUS_MAP_W - 1)) return x;

    maxChars = (PLUS_MAP_W - 1 - x) / charW;
    snprintf(buf, sizeof(buf), "%s", text);
    if ((int)strlen(buf) > maxChars) buf[maxChars] = '\0';

    gfxSetPens(&plusRP, PLUS_COL_BLACK, GFX_SAME_PEN, GFX_SAME_PEN);
    gfxPrintExact(&plusRP, buf, (uword)(x + 1), (uword)(y + 1));
    gfxSetPens(&plusRP, col, GFX_SAME_PEN, GFX_SAME_PEN);
    gfxPrintExact(&plusRP, buf, (uword)x, (uword)y);

    return x + (int)strlen(buf) * charW;
}

static int plusTextWidth(const char *text) { return (int)strlen(text) * bubbleFont->us_Width; }

/* ------------------------------------------------------------------------
 * guards
 * ------------------------------------------------------------------------ */

/* start position of a guard sprite, exactly as plPrepareSprite places it */
static void plusGuardStart(ubyte livNr, uint32_t areaId, int *x, int *y)
{
    LSArea area = (LSArea)dbGetObject(areaId);
    int slot = livNr + (PLANING_NR_PERSONS - BurglarsNr);

    if ((slot == 5) || (slot == 7))
    {
        *x = area->us_StartX5 - 4 + 9;
        *y = area->us_StartY5 + 11 - 18;
    }
    else
    {
        *x = area->us_StartX4 - 4 + 9;
        *y = area->us_StartY4 + 11 - 18;
    }
}

/* the direction a guard looks to during its current action (see the comment at the top) */
static ubyte plusGuardView(struct Handler *h)
{
    struct Action *a = h ? (struct Action *)h->CurrentAction : NULL;

    if (a && (a->Type == ACTION_GO))
    {
        switch (ActionData(a, struct ActionGo *)->Direction)
        {
            case DIRECTION_LEFT:
                return ANM_MOVE_LEFT;
            case DIRECTION_RIGHT:
                return ANM_MOVE_RIGHT;
            case DIRECTION_UP:
                return ANM_MOVE_UP;
            default:
                return ANM_MOVE_DOWN;
        }
    }

    return ANM_MOVE_DOWN;
}

/* watched areas of all guards, as a mask one pixel larger than the map on every side, so the
   outline is drawn only where an area really ends and not at the edge of the window */
#define PLUS_MASK_W (PLUS_MAP_W + 2)
#define PLUS_MASK_H (PLUS_MAP_H + 2)
static ubyte plusMask[PLUS_MASK_W * PLUS_MASK_H];

static void plusMaskRect(int x0, int y0, int x1, int y1)
{
    int x = 0;
    int y = 0;

    /* map coordinates to mask coordinates */
    x0++, y0++, x1++, y1++;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= PLUS_MASK_W) x1 = PLUS_MASK_W - 1;
    if (y1 >= PLUS_MASK_H) y1 = PLUS_MASK_H - 1;

    for (y = y0; y <= y1; y++)
        for (x = x0; x <= x1; x++) plusMask[y * PLUS_MASK_W + x] = 1;
}

/* sparse dots (every 2nd pixel of every 2nd line, offset per line pair, anchored to the floor
   so they do not swim when scrolling) and a solid outline */
static void plusDrawMask(int wx, int wy, ubyte col)
{
    int x = 0;
    int y = 0;

    for (y = 0; y < PLUS_MAP_H; y++)
    {
        for (x = 0; x < PLUS_MAP_W; x++)
        {
            const ubyte *m = &plusMask[(y + 1) * PLUS_MASK_W + (x + 1)];

            if (!*m) continue;

            if (!m[-1] || !m[1] || !m[-PLUS_MASK_W] || !m[PLUS_MASK_W])
                plusPixel(x, y, col);
            else if (!((y + wy) & 1) && !((x + wx + ((y + wy) & 2)) & 3))
                plusPixel(x, y, col);
        }
    }
}

/* the area the guard watches: each room containing the guard's reference point, cut at the
   line through that point, on the side it looks to (coordinates of the reference point) */
static void plusMarkGuardView(ubyte i, int wx, int wy)
{
    int gx = livGetXPos(Planing_Name[i]) + 8; /* tcX_HOTSPOT */
    int gy = livGetYPos(Planing_Name[i]);
    struct Handler *h = FindHandler(plSys, OL_NR(GetNthNode(PersonsList, i)));
    ubyte view = plusGuardView(h);
    LIST *rooms = Planing_GuardRoomList[i - BurglarsNr]; /* the list tcGuardDetectsGuy gets */
    NODE *n = NULL;

    if (!rooms) return;

    for (n = LIST_HEAD(rooms); NODE_SUCC(n); n = (NODE *)NODE_SUCC(n))
    {
        LSRoom r = (LSRoom)OL_DATA(n);
        int x0 = r->us_LeftEdge;
        int y0 = r->us_TopEdge;
        int x1 = r->us_LeftEdge + r->us_Width;
        int y1 = r->us_TopEdge + r->us_Height;

        if ((gx < x0) || (gx > x1) || (gy < y0) || (gy > y1)) continue;

        switch (view)
        {
            case ANM_MOVE_LEFT:
                x1 = gx;
                break;
            case ANM_MOVE_RIGHT:
                x0 = gx;
                break;
            case ANM_MOVE_UP:
                y1 = gy;
                break;
            default:
                y0 = gy;
                break;
        }

        plusMaskRect(x0 - wx, y0 - wy, x1 - wx, y1 - wy);
    }
}

/* dashed line between two points of the route (they differ in x or in y only) */
static void plusDash(int x0, int y0, int x1, int y1, int wx, int wy)
{
    int steps = abs(x1 - x0) + abs(y1 - y0);
    int dx = (x1 > x0) - (x1 < x0);
    int dy = (y1 > y0) - (y1 < y0);
    int k = 0;

    for (k = 0; k <= steps; k++)
    {
        int ax = x0 + dx * k;
        int ay = y0 + dy * k;

        if (((ax + ay) & 3) < 2) plusPixel(ax - wx, ay - wy, PLUS_COL_ROUTE);
    }
}

/* where the guard stands still (waits, checks or opens something): a small open square */
static void plusStop(int ax, int ay, int wx, int wy)
{
    int x = ax - wx;
    int y = ay - wy;

    plusFrame(x - 3, y - 3, x + 3, y + 3, PLUS_COL_BLACK);
    plusFrame(x - 2, y - 2, x + 2, y + 2, PLUS_COL_ROUTE);
}

/* patrol route: one round of the guard's action list from its start position. The list
   repeats (SHF_AUTOREVERS), so the route of the data files is closed. */
static void plusDrawGuardRoute(ubyte i, int wx, int wy)
{
    struct Handler *h = FindHandler(plSys, OL_NR(GetNthNode(PersonsList, i)));
    struct Action *a = NULL;
    int x = 0;
    int y = 0;
    int cx = LS_PERSON_X_SIZE / 2; /* centre of the 14 x 14 sprite */
    int cy = LS_PERSON_Y_SIZE / 2;
    int pass = 0;

    if (!h || LIST_EMPTY(h->Actions)) return;

    /* first the lines, then the stops on top */
    for (pass = 0; pass < 2; pass++)
    {
        plusGuardStart(i, livWhereIs(Planing_Name[i]), &x, &y);

        for (a = (struct Action *)LIST_HEAD(h->Actions); NODE_SUCC(a); a = (struct Action *)NODE_SUCC(a))
        {
            if (a->Type == ACTION_GO)
            {
                int nx = x;
                int ny = y;
                int d = a->TimeNeeded * LS_STD_SCROLL_SPEED;

                switch (ActionData(a, struct ActionGo *)->Direction)
                {
                    case DIRECTION_LEFT:
                        nx -= d;
                        break;
                    case DIRECTION_RIGHT:
                        nx += d;
                        break;
                    case DIRECTION_UP:
                        ny -= d;
                        break;
                    case DIRECTION_DOWN:
                        ny += d;
                        break;
                }

                if (!pass) plusDash(x + cx, y + cy, nx + cx, ny + cy, wx, wy);

                x = nx;
                y = ny;
            }
            else if (pass)
                plusStop(x + cx, y + cy, wx, wy);
        }
    }
}

static void plusDrawGuards(int wx, int wy)
{
    ubyte i = 0;
    uint32_t areaId = lsGetActivAreaID();

    memset(plusMask, 0, sizeof(plusMask));

    for (i = BurglarsNr; i < PersonsNr; i++)
    {
        if (livWhereIs(Planing_Name[i]) != areaId) continue;

        /* a knocked out (or just attacked) guard sees nothing */
        if (!Planing_Guard[i - BurglarsNr]) plusMarkGuardView(i, wx, wy);
    }

    plusDrawMask(wx, wy, PLUS_COL_VIEW);

    for (i = BurglarsNr; i < PersonsNr; i++)
    {
        if (livWhereIs(Planing_Name[i]) == areaId) plusDrawGuardRoute(i, wx, wy);
    }
}

/* ------------------------------------------------------------------------
 * timeline
 * ------------------------------------------------------------------------ */

static int plusCategory(uword type)
{
    switch (type)
    {
        case ACTION_GO:
            return PLUS_CAT_WALK;
        case ACTION_USE:
            return PLUS_CAT_TOOL;
        case ACTION_TAKE:
        case ACTION_DROP:
            return PLUS_CAT_LOOT;
        case ACTION_OPEN:
        case ACTION_CLOSE:
            return PLUS_CAT_DOOR;
        case ACTION_SIGNAL:
            return PLUS_CAT_RADIO;
        default:
            return PLUS_CAT_WAIT;
    }
}

static struct Handler *plusHandler(ubyte person) { return FindHandler(plSys, OL_NR(GetNthNode(PersonsList, person))); }

static uint32_t plusLength(struct Handler *h)
{
    struct Action *a = NULL;
    uint32_t t = 0;

    if (!h) return 0;
    for (a = (struct Action *)LIST_HEAD(h->Actions); NODE_SUCC(a); a = (struct Action *)NODE_SUCC(a)) t += a->TimeNeeded;
    return t;
}

/* planning time (1/3 s) as m:ss */
static void plusTime(char *buf, size_t size, uint32_t ticks)
{
    uint32_t sec = ticks / PLANING_CORRECT_TIME;
    snprintf(buf, size, "%u:%02u", (unsigned)(sec / 60), (unsigned)(sec % 60));
}

static void plusDrawTimeline(void)
{
    ubyte lang = plusLang();
    const int left = 72;   /* bars from here ... */
    const int right = 290; /* ... to here */
    const int rowTop = 26;
    const int rowH = 13;
    char buf[96];
    char t[16];
    uint32_t total = 0;
    uint32_t now = 0;
    ubyte i = 0;
    int x = 0;
    int y = 0;

    plusRect(0, 0, PLUS_MAP_W - 1, PLUS_MAP_H - 1, PLUS_COL_PANEL);
    plusFrame(0, 0, PLUS_MAP_W - 1, PLUS_MAP_H - 1, PLUS_COL_FRAME);

    for (i = 0; i < BurglarsNr; i++)
    {
        uint32_t len = plusLength(plusHandler(i));
        if (len > total) total = len;
    }

    {
        struct Handler *h = plusHandler((ubyte)CurrentPerson);
        now = h ? h->Timer : 0;
    }

    plusText(9, 4, plusTxtTitle[lang], PLUS_COL_WHITE);
    {
        uint32_t sec = total / PLANING_CORRECT_TIME;
        snprintf(buf, sizeof(buf), "%s %02u:%02u:%02u", plusTxtTotal[lang], (unsigned)(sec / 3600),
                 (unsigned)((sec / 60) % 60), (unsigned)(sec % 60));
        plusText(PLUS_MAP_W - 9 - plusTextWidth(buf), 4, buf, PLUS_COL_WHITE);
    }
    plusLine(4, 14, PLUS_MAP_W - 5, 14, PLUS_COL_GRID);

    /* time grid with labels */
    if (total)
    {
        static const uint32_t steps[] = {5, 10, 15, 30, 60, 120, 300, 600, 900, 1800, 3600};
        uint32_t totalSec = (total + PLANING_CORRECT_TIME - 1) / PLANING_CORRECT_TIME;
        uint32_t step = steps[0];
        uint32_t s = 0;
        int k = 0;

        for (k = 0; k < (int)(sizeof(steps) / sizeof(steps[0])); k++)
        {
            step = steps[k];
            if (totalSec / step <= 5) break;
        }

        for (s = 0; s <= totalSec; s += step)
        {
            x = left + (int)((uint64_t)s * PLANING_CORRECT_TIME * (right - left) / total);
            if (x > right) break;
            for (y = rowTop - 2; y < rowTop + BurglarsNr * rowH; y += 2) plusPixel(x, y, PLUS_COL_GRID);
            plusTime(t, sizeof(t), s * PLANING_CORRECT_TIME);
            {
                int tx = x - plusTextWidth(t) / 2;
                if (tx + plusTextWidth(t) > PLUS_MAP_W - 4) tx = PLUS_MAP_W - 4 - plusTextWidth(t);
                plusText(tx, 16, t, PLUS_COL_TEXT);
            }
        }
    }

    /* one row per burglar */
    for (i = 0; i < BurglarsNr; i++)
    {
        struct Handler *h = plusHandler(i);
        struct Action *a = NULL;
        uint32_t t0 = 0;
        ubyte current = (i == CurrentPerson);
        ubyte col = current ? PLUS_COL_WHITE : PLUS_COL_TEXT;

        y = rowTop + i * rowH;

        dbGetObjectName(OL_NR(GetNthNode(PersonsList, i)), buf);
        buf[15] = '\0';
        if (current) plusText(3, y, ">", col);
        plusText(9, y, buf, col);

        plusRect(left, y, right, y + 6, PLUS_COL_TRACK);

        if (h && total)
        {
            for (a = (struct Action *)LIST_HEAD(h->Actions); NODE_SUCC(a); a = (struct Action *)NODE_SUCC(a))
            {
                uint32_t t1 = t0 + a->TimeNeeded;
                int cat = plusCategory(a->Type);
                int x0 = left + (int)((uint64_t)t0 * (right - left) / total);
                int x1 = left + (int)((uint64_t)t1 * (right - left) / total) - 1;
                struct Action *prev = (struct Action *)NODE_PRED(a);

                if (x1 < x0) x1 = x0;
                plusRect(x0, y, x1, y + 6, plusCatColour[cat]);

                /* a thin gap between two actions of the same kind (each one is a step of undo) */
                if (NODE_PRED(prev) && (plusCategory(prev->Type) == cat) && (x1 > x0 + 2))
                    plusLine(x0, y, x0, y + 6, PLUS_COL_TRACK);

                t0 = t1;
            }
        }

        plusTime(t, sizeof(t), plusLength(h));
        plusText(right + 4, y, t, col);
    }

    /* planning time of the current person */
    if (total)
    {
        x = left + (int)((uint64_t)(now > total ? total : now) * (right - left) / total);
        plusLine(x, rowTop - 3, x, rowTop + BurglarsNr * rowH - 4, PLUS_COL_WHITE);
    }
    else
        plusText(left, rowTop + BurglarsNr * rowH, plusTxtEmpty[lang], PLUS_COL_TEXT);

    /* legend: two rows of three */
    for (i = 0; i < PLUS_CAT_COUNT; i++)
    {
        x = 9 + (i % 3) * 104;
        y = 84 + (i / 3) * 10;
        plusRect(x, y + 1, x + 6, y + 6, plusCatColour[i]);
        plusText(x + 10, y, plusCatName[lang][i], PLUS_COL_TEXT);
    }

    plusLine(4, 105, PLUS_MAP_W - 5, 105, PLUS_COL_GRID);
    if (plusStatus[0]) plusText(9, 107, plusStatus, PLUS_COL_WHITE);
    plusText(9, 117, plusTxtKeys[lang], PLUS_COL_TEXT);
}

/* ------------------------------------------------------------------------
 * undo and redo (the current person's last action)
 * ------------------------------------------------------------------------ */

static void plusRedoMark(void)
{
    struct Handler *h = plusHandler((ubyte)CurrentPerson);

    plusRedoPerson = CurrentPerson;
    plusRedoActions = h ? GetNrOfNodes(h->Actions) : 0;
    plusRedoLength = plusLength(h);

    /* every other change of the plan sets PlanChanged back to 1 (saving or loading to 0) */
    if (PlanChanged) PlanChanged = PLAN_CHANGED_BY_TIMELINE;
}

/* Forget the redo stack when the plan was changed some other way since the last undo or redo.
   Person, number of actions and length alone would miss an action that was replaced by another
   one of the same duration: redo would then put the old action back at the wrong place. */
static void plusRedoCheck(void)
{
    struct Handler *h = plusHandler((ubyte)CurrentPerson);

    if ((PlanChanged != PLAN_CHANGED_BY_TIMELINE) || (plusRedoPerson != CurrentPerson) ||
        (plusRedoActions != (h ? GetNrOfNodes(h->Actions) : 0)) || (plusRedoLength != plusLength(h)))
        plusRedoCount = 0;
}

static void plusSetStatus(const char *what, uword type, uword time)
{
    ubyte lang = plusLang();
    snprintf(plusStatus, sizeof(plusStatus), "%s: %s, %u s", what, plusCatName[lang][plusCategory(type)],
             (unsigned)(time / PLANING_CORRECT_TIME));
}

static void plusUndo(void)
{
    struct Handler *h = NULL;
    struct Action *a = NULL;
    struct plusRedo r;

    plusRedoCheck();

    SetActivHandler(plSys, OL_NR(GetNthNode(PersonsList, CurrentPerson)));
    h = (struct Handler *)plSys->ActivHandler;

    if (!h || LIST_EMPTY(h->Actions))
    {
        snprintf(plusStatus, sizeof(plusStatus), "%s", plusTxtNoUndo[plusLang()]);
        return;
    }

    a = (struct Action *)LIST_TPRED(h->Actions);
    memset(&r, 0, sizeof(r));
    r.type = a->Type;
    r.time = a->TimeNeeded;

    switch (a->Type)
    {
        case ACTION_GO:
            r.data1 = ActionData(a, struct ActionGo *)->Direction;
            break;
        case ACTION_USE:
        case ACTION_TAKE:
        case ACTION_DROP:
            r.data1 = ActionData(a, struct ActionUse *)->ItemId;
            r.data2 = ActionData(a, struct ActionUse *)->ToolId;
            break;
        case ACTION_SIGNAL:
        case ACTION_WAIT_SIGNAL:
        case ACTION_OPEN:
        case ACTION_CLOSE:
        case ACTION_CONTROL:
            r.data1 = ActionData(a, struct ActionOpen *)->ItemId;
            break;
    }

    if (plUndoLastAction())
    {
        if (plusRedoCount == PLUS_REDO_MAX)
        {
            memmove(&plusRedoStack[0], &plusRedoStack[1], sizeof(plusRedoStack[0]) * (PLUS_REDO_MAX - 1));
            plusRedoCount--;
        }
        plusRedoStack[plusRedoCount++] = r;
        plusSetStatus(plusTxtUndone[plusLang()], r.type, r.time);
        Log("Planner: undo action %u (%u ticks)", (unsigned)r.type, (unsigned)r.time);
    }

    plusRedoMark();
}

/* the action is added again and played forward by plSync, the same way a loaded plan is
   rebuilt, so all its effects (loot, doors, stairs, guards) come back */
static void plusRedo(void)
{
    struct Handler *h = NULL;
    struct Action *a = NULL;
    struct plusRedo r;

    plusRedoCheck();

    if (!plusRedoCount)
    {
        snprintf(plusStatus, sizeof(plusStatus), "%s", plusTxtNoRedo[plusLang()]);
        return;
    }

    r = plusRedoStack[plusRedoCount - 1];

    SetActivHandler(plSys, OL_NR(GetNthNode(PersonsList, CurrentPerson)));
    h = (struct Handler *)plSys->ActivHandler;

    if (!h || !(a = InitAction(plSys, r.type, r.data1, r.data2, r.time)))
    {
        plSay("PLANING_END", CurrentPerson);
        plusRedoMark();
        return;
    }

    plusRedoCount--;

    /* rewind to the start of the new action, then play it */
    a->Timer = 0;
    h->Timer -= r.time;

    plSync(PLANING_ANIMATE_NO, GetMaxTimer(plSys), r.time, 1);
    lsSetActivLiving(Planing_Name[CurrentPerson], (uword)-1, (uword)-1);
    livRefreshAll();

    PlanChanged = 1;

    plDisplayTimer(0, 1);
    plDisplayInfo();

    plusSetStatus(plusTxtRedone[plusLang()], r.type, r.time);
    Log("Planner: redo action %u (%u ticks)", (unsigned)r.type, (unsigned)r.time);

    plusRedoMark();
}

/* ------------------------------------------------------------------------
 * hooks
 * ------------------------------------------------------------------------ */

static void plusOverlay(ubyte *pixels, int width, int height)
{
    uword wx = 0;
    uword wy = 0;

    if (!plusActive || (height < PLUS_MAP_H) || IsBubbleOpen()) return;

    plusInitRP(pixels, width, height);

    if (plusTimelineOpen)
    {
        plusDrawTimeline();
        return;
    }

    if (plusGuardsOn)
    {
        lsGetWindowPos(&wx, &wy);
        plusDrawGuards(wx, wy);
    }
}

static void plusTimeline(void)
{
    plusTimelineOpen = 1;
    plusStatus[0] = '\0';
    gfxInvalidate();
    Log("Planner: timeline opened");

    while (plusTimelineOpen)
    {
        int32_t action = 0;

        plusCommand = PLUS_CMD_NONE;
        action = inpWaitFor(INP_KEYBOARD | INP_LBUTTONP | INP_RBUTTONP | INP_NO_ESC);

        switch (plusCommand)
        {
            case PLUS_CMD_CLOSE:
                plusTimelineOpen = 0;
                break;
            case PLUS_CMD_UNDO:
                plusUndo();
                break;
            case PLUS_CMD_REDO:
                plusRedo();
                break;
            default:
                if (action & (INP_LBUTTONP | INP_RBUTTONP)) plusTimelineOpen = 0;
                break;
        }

        gfxInvalidate();
    }

    Log("Planner: timeline closed");
}

static int plusKey(int32_t key)
{
    if (plusTimelineOpen)
    {
        switch (key)
        {
            case SDLK_T:
            case SDLK_ESCAPE:
                plusCommand = PLUS_CMD_CLOSE;
                return INP_HOOK_RETURN;
            case SDLK_Z:
            case SDLK_BACKSPACE:
                plusCommand = PLUS_CMD_UNDO;
                return INP_HOOK_RETURN;
            case SDLK_Y:
                plusCommand = PLUS_CMD_REDO;
                return INP_HOOK_RETURN;
        }
        return INP_HOOK_UNUSED;
    }

    switch (key)
    {
        case SDLK_W:
            plusGuardsOn = !plusGuardsOn;
            Log("Planner: guards %s", plusGuardsOn ? "shown" : "hidden");
            gfxInvalidate();
            return INP_HOOK_USED;

        case SDLK_T:
            if (!plusInMenu || IsBubbleOpen()) return INP_HOOK_UNUSED;
            plusTimeline();
            return INP_HOOK_USED;
    }

    return INP_HOOK_UNUSED;
}

void plPlusEnter(void)
{
    if (GameMode != GAME_MODE_PLUS) return;

    plusActive = 1;
    plusInMenu = 0;
    plusTimelineOpen = 0;
    plusRedoCount = 0;
    plusStatus[0] = '\0';

    inpSetKeyHook(plusKey);
    gfxSetOverlay(plusOverlay);
}

void plPlusLeave(void)
{
    if (!plusActive) return;

    plusActive = 0;
    plusInMenu = 0;
    plusRedoCount = 0;

    inpSetKeyHook(NULL);
    gfxSetOverlay(NULL);
}

void plPlusMenuContext(ubyte inMenu)
{
    if (plusActive) plusInMenu = inMenu;
}
