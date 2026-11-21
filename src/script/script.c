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
