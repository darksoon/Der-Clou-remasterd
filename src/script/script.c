/*  _________  _______
   / ___/ __ \/ __/ _ \      Der Clou!
  / /__/ /_/ /\ \/ ___/ Open Source Project
  \___/\____/___/_/ http://cosp.sourceforge.net
   Based on the original by neo Software GmbH
*/
/* Lua scripting for Game+.
   Scripts run in a sandbox: only the base, string, table, math, utf8 and coroutine libraries,
   without dofile, loadfile and load, so a script can neither read nor write files. */
#include "script/script.h"

#include <string.h>

#include "base/base.h"
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"

#define SCRIPT_DIRECTORY "plus"
#define SCRIPT_BALANCE "balance.lua"
#define SCRIPT_NEWS "news.lua"

static lua_State *L = NULL;

/* runs a script file from the game data (a replacement in MODFILES wins, see dskOpen) */
static int scrRunFile(const char *directory, const char *fileName)
{
    char path[FILENAME_MAX];
    int32_t size = 0;
    char *buffer = NULL;
    FILE *file = NULL;
    int ok = 0;

    dskBuildPathName(directory, fileName, path);
    size = dskFileLength(path);
    if (size <= 0)
    {
        Log("Lua: %s not found", path);
        return 0;
    }

    if (!(buffer = (char *)MemAlloc(size)))
    {
        return 0;
    }
    if ((file = dskOpen(path, "rb", 0)))
    {
        dskRead(file, buffer, size);
        dskClose(file);

        if ((luaL_loadbufferx(L, buffer, (size_t)size, fileName, "t") == LUA_OK) &&
            (lua_pcall(L, 0, 0, 0) == LUA_OK))
        {
            Log("Lua: %s loaded", fileName);
            ok = 1;
        }
        else
        {
            Log("Lua error: %s", lua_tostring(L, -1));
            lua_pop(L, 1);
        }
    }
    MemFree(buffer, size);
    return ok;
}

void scrInit(void)
{
    if (L) return;

    if (!(L = luaL_newstate()))
    {
        Log("Lua: cannot create state");
        return;
    }

    luaL_openselectedlibs(L, LUA_GLIBK | LUA_COLIBK | LUA_MATHLIBK | LUA_STRLIBK | LUA_TABLIBK | LUA_UTF8LIBK, 0);

    /* no file access from scripts */
    lua_pushnil(L);
    lua_setglobal(L, "dofile");
    lua_pushnil(L);
    lua_setglobal(L, "loadfile");
    lua_pushnil(L);
    lua_setglobal(L, "load");

    Log("Lua: %s", LUA_RELEASE);

    scrRunFile(SCRIPT_DIRECTORY, SCRIPT_BALANCE);
    scrRunFile(SCRIPT_DIRECTORY, SCRIPT_NEWS);
}

void scrDone(void)
{
    if (L)
    {
        lua_close(L);
        L = NULL;
    }
}

int scrBalancePercent(const char *what, int difficulty, int fallback)
{
    static const char *level[] = {"easy", "normal", "hard"};
    int percent = fallback;

    if (!L) scrInit(); /* started on first use, so the original mode never runs a script */
    if (!L || (difficulty < 0) || (difficulty > 2)) return fallback;

    /* balance[level][what] */
    if (lua_getglobal(L, "balance") == LUA_TTABLE)
    {
        if (lua_getfield(L, -1, level[difficulty]) == LUA_TTABLE)
        {
            if (lua_getfield(L, -1, what) == LUA_TNUMBER)
            {
                lua_Integer v = lua_tointeger(L, -1);
                if ((v >= 10) && (v <= 1000)) percent = (int)v; /* keep values sane */
            }
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    return percent;
}

int scrToolSkillPercent(const char *toolName)
{
    int percent = -1;

    if (!L) scrInit();
    if (!L || !toolName) return -1;

    /* tools[name] */
    if (lua_getglobal(L, "tools") == LUA_TTABLE)
    {
        if (lua_getfield(L, -1, toolName) == LUA_TNUMBER)
        {
            lua_Integer v = lua_tointeger(L, -1);
            if ((v >= 0) && (v <= 100)) percent = (int)v;
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    return percent;
}

/* scripts are UTF-8, the game font uses code page 850: convert the letters German texts need */
static const struct
{
    const char *utf8;
    char cp850;
} scrLetters[] = {{"\xc3\xa4", '\x84'}, {"\xc3\xb6", '\x94'}, {"\xc3\xbc", '\x81'}, {"\xc3\x84", '\x8e'},
                  {"\xc3\x96", '\x99'}, {"\xc3\x9c", '\x9a'}, {"\xc3\x9f", '\xe1'}, {"\xc3\xa9", '\x82'},
                  {"\xc3\xa8", '\x8a'}, {"\xc3\xa0", '\x85'}, {"\xc3\xa1", '\xa0'}, {"\xc3\xad", '\xa1'},
                  {"\xc3\xb3", '\xa2'}, {"\xc3\xba", '\xa3'}, {"\xc3\xb1", '\xa4'}, {"\xc3\xa7", '\x87'}};

static void scrToGame(const char *src, char *dst, int size)
{
    int n = 0;
    while (*src && (n < size - 1))
    {
        size_t i = 0;
        int done = 0;
        for (i = 0; i < sizeof(scrLetters) / sizeof(scrLetters[0]); i++)
        {
            if (!strncmp(src, scrLetters[i].utf8, 2))
            {
                dst[n++] = scrLetters[i].cp850;
                src += 2;
                done = 1;
                break;
            }
        }
        if (done) continue;
        if ((unsigned char)*src < 0x80)
            dst[n++] = *src++;
        else
            src++; /* other UTF-8 bytes are left out */
    }
    dst[n] = '\0';
}

static void scrFromGame(const char *src, char *dst, int size)
{
    int n = 0;
    while (*src && (n < size - 2))
    {
        size_t i = 0;
        int done = 0;
        for (i = 0; i < sizeof(scrLetters) / sizeof(scrLetters[0]); i++)
        {
            if (*src == scrLetters[i].cp850)
            {
                dst[n++] = scrLetters[i].utf8[0];
                dst[n++] = scrLetters[i].utf8[1];
                done = 1;
                break;
            }
        }
        if (!done && ((unsigned char)*src < 0x80)) dst[n++] = *src;
        src++;
    }
    dst[n] = '\0';
}

int scrNews(const ScrNewsInfo *info, char *lines, int maxLines, int lineSize)
{
    char building[256];
    int count = 0;

    if (!L) scrInit();
    if (!L || !info) return 0;

    if (lua_getglobal(L, "news") != LUA_TFUNCTION)
    {
        lua_pop(L, 1);
        return 0;
    }

    scrFromGame(info->building ? info->building : "", building, sizeof(building));
    lua_createtable(L, 0, 7);
    lua_pushstring(L, building);
    lua_setfield(L, -2, "building");
    lua_pushboolean(L, info->escaped);
    lua_setfield(L, -2, "escaped");
    lua_pushstring(L, (info->alarm == 2) ? "loud" : (info->alarm == 1) ? "silent" : "none");
    lua_setfield(L, -2, "alarm");
    lua_pushinteger(L, info->loot);
    lua_setfield(L, -2, "loot");
    lua_pushinteger(L, info->knockedOut);
    lua_setfield(L, -2, "knocked_out");
    lua_pushinteger(L, info->day);
    lua_setfield(L, -2, "day");
    lua_pushstring(L, (info->lang == 0) ? "en" : (info->lang == 2) ? "fr" : (info->lang == 3) ? "es" : "de");
    lua_setfield(L, -2, "lang");

    if (lua_pcall(L, 1, 1, 0) != LUA_OK)
    {
        Log("Lua error: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
        return 0;
    }

    if (lua_istable(L, -1))
    {
        int i = 0;
        for (i = 1; (i <= maxLines); i++)
        {
            if (lua_geti(L, -1, i) == LUA_TSTRING)
            {
                scrToGame(lua_tostring(L, -1), lines + count * lineSize, lineSize);
                count++;
                lua_pop(L, 1);
            }
            else
            {
                lua_pop(L, 1);
                break;
            }
        }
    }
    lua_pop(L, 1);
    return count;
}
