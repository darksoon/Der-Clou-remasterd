/*  _________  _______
   / ___/ __ \/ __/ _ \      Der Clou!
  / /__/ /_/ /\ \/ ___/ Open Source Project
  \___/\____/___/_/ http://cosp.sourceforge.net
   Based on the original by neo Software GmbH
*/
/* Lua scripting for Game+.
   Scripts run in a sandbox: only the base, string, table, math, utf8 and coroutine libraries,
   without dofile, loadfile and load, so a script can neither read nor write files.
   A script can also not stop or freeze the game:
   - every access to the script runs in a protected call, an error is written to the log and the
     game goes on with its built-in values,
   - a count hook ends a script that runs too long ("script took too long"),
   - the state has its own allocator with a limit of a few megabytes,
   - the pattern functions of the string library only take short texts and simple patterns and
     setmetatable takes no finalizer (__gc), because the hook cannot interrupt these.
   A script file that fails is left out as a whole, the others still load. */
#include "script/script.h"

#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "base/base.h"
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"

#define SCRIPT_DIRECTORY "plus"
#define SCRIPT_BALANCE "balance.lua"
#define SCRIPT_NEWS "news.lua"
#define SCRIPT_WEATHER "weather.lua"

#define SCR_MEMORY_LIMIT ((size_t)4 * 1024 * 1024) /* bytes the script state may hold */
#define SCR_HOOK_STEP 1000                         /* instructions between two checks */
#define SCR_BUDGET 5000000L                        /* instructions per call into a script */
#define SCR_SECONDS 2                              /* processor time per call, for slow library functions */

/* limits of string.find, match, gmatch and gsub */
#define SCR_PATTERN_SUBJECT 4096       /* longest text */
#define SCR_PATTERN_SUBJECT_NESTED 512 /* longest text for a pattern with two repetitions */
#define SCR_PATTERN_LENGTH 128         /* longest pattern */
#define SCR_PATTERN_REPEATS 2          /* most repetitions (* + - ?) in a pattern */

static lua_State *L = NULL;
static int scrDisabled = 0; /* scripting was switched off after a script ran away */

static const char *scrFiles[] = {SCRIPT_BALANCE, SCRIPT_NEWS, SCRIPT_WEATHER};
#define SCR_FILE_COUNT ((int)(sizeof(scrFiles) / sizeof(scrFiles[0])))

/* --- memory limit --- */

static size_t scrMemoryUsed = 0;

static void *scrAlloc(void *ud, void *ptr, size_t osize, size_t nsize)
{
    void *block = NULL;

    (void)ud;
    if (!ptr) osize = 0; /* for a new block osize tells the kind of object */

    if (nsize == 0)
    {
        free(ptr);
        scrMemoryUsed -= osize;
        return NULL;
    }
    if ((nsize > osize) && (scrMemoryUsed - osize + nsize > SCR_MEMORY_LIMIT)) return NULL;
    if (!(block = realloc(ptr, nsize))) return NULL;
    scrMemoryUsed = scrMemoryUsed - osize + nsize;
    return block;
}

/* --- running time limit --- */

static long scrBudget = 0;
static int scrBudgetExhausted = 0;
static clock_t scrBudgetStart = 0;

/* Once the budget is used up, the hook runs before every single instruction and raises the error
   again, so a script cannot go on by catching the error with pcall or inside a coroutine. */
static void scrHook(lua_State *thread, lua_Debug *ar)
{
    (void)ar;
    if (!scrBudgetExhausted)
    {
        scrBudget -= SCR_HOOK_STEP;
        if ((scrBudget > 0) && ((clock() - scrBudgetStart) < (clock_t)SCR_SECONDS * CLOCKS_PER_SEC)) return;

        scrBudgetExhausted = 1;
        if (L) lua_sethook(L, scrHook, LUA_MASKCOUNT, 1);
        if (thread != L) lua_sethook(thread, scrHook, LUA_MASKCOUNT, 1);
    }
    luaL_error(thread, "script took too long");
}

static void scrStartBudget(void)
{
    scrBudget = SCR_BUDGET;
    scrBudgetExhausted = 0;
    scrBudgetStart = clock();
    lua_sethook(L, scrHook, LUA_MASKCOUNT, SCR_HOOK_STEP);
}

/* --- errors --- */

static char scrLastError[200];

/* writes a script error to the log; the same error in a row is written only once */
static void scrLogError(const char *text)
{
    if (scrLastError[0] && !strncmp(scrLastError, text, sizeof(scrLastError) - 1)) return;
    snprintf(scrLastError, sizeof(scrLastError), "%s", text);
    Log("Lua error: %s", text);
}

/* text of the error value on top of the stack; nothing is converted here, a conversion could
   raise another error outside a protected call */
static const char *scrErrorText(lua_State *state)
{
    if (lua_type(state, -1) == LUA_TSTRING) return lua_tostring(state, -1);
    return "(the error value is not a text)";
}

static void scrClose(void)
{
    if (L)
    {
        lua_State *state = L;

        lua_sethook(state, NULL, 0, 0);
        L = NULL;
        lua_close(state);
    }
}

static void scrDisable(const char *why)
{
    scrClose();
    scrDisabled = 1;
    Log("Lua: scripts switched off (%s), the game uses its built-in values", why);
}

/* Last resort: an error outside a protected call. All calls below are protected, so this should
   never run; if it does, the state is given up instead of ending the game. */
static jmp_buf scrPanicJump;
static int scrPanicArmed = 0;

static int scrPanic(lua_State *state)
{
    scrLogError(scrErrorText(state));
    L = NULL; /* the state is in an unknown condition: do not use or close it any more */
    scrDisabled = 1;
    Log("Lua: scripts switched off (unprotected error), the game uses its built-in values");
    if (scrPanicArmed)
    {
        scrPanicArmed = 0;
        longjmp(scrPanicJump, 1);
    }
    return 0; /* Lua ends the program */
}

/* Protected call of the function below its nargs arguments on the stack, without results.
   Returns 1 if it ran through. An error is logged. A script that runs out of time or memory
   during the game (strict) switches scripting off, because it would do so on every call. */
static int scrCall(int nargs, int strict)
{
    int status = LUA_OK;

    if (setjmp(scrPanicJump)) return 0;
    scrPanicArmed = 1;
    scrStartBudget();
    status = lua_pcall(L, nargs, 0, 0);
    scrPanicArmed = 0;

    if (status == LUA_OK) return 1;

    scrLogError(scrErrorText(L));
    lua_pop(L, 1);
    if (strict && (scrBudgetExhausted || (status == LUA_ERRMEM)))
        scrDisable(scrBudgetExhausted ? "a script took too long" : "a script used too much memory");
    return 0;
}

/* runs a C function that works on the script state in a protected call */
static int scrProtected(lua_CFunction function, void *arg, int strict)
{
    lua_pushcfunction(L, function);
    lua_pushlightuserdata(L, arg);
    return scrCall(1, strict);
}

/* --- sandbox --- */

/* string.find, match, gmatch and gsub with limits: matching can take very long for long texts
   and patterns with several repetitions, and the count hook cannot interrupt it */
static int scrPatternGuard(lua_State *state)
{
    size_t subjectLength = 0;
    size_t patternLength = 0;
    const char *pattern = NULL;
    size_t i = 0;
    int repeats = 0;
    int n = lua_gettop(state);

    luaL_checklstring(state, 1, &subjectLength);
    pattern = luaL_checklstring(state, 2, &patternLength);
    for (i = 0; i < patternLength; i++)
        if ((pattern[i] == '*') || (pattern[i] == '+') || (pattern[i] == '-') || (pattern[i] == '?')) repeats++;

    if ((subjectLength > SCR_PATTERN_SUBJECT) || (patternLength > SCR_PATTERN_LENGTH) ||
        (repeats > SCR_PATTERN_REPEATS) || ((repeats > 1) && (subjectLength > SCR_PATTERN_SUBJECT_NESTED)))
        return luaL_error(state, "text or pattern too long or too complex for a game script");

    lua_pushvalue(state, lua_upvalueindex(1));
    lua_insert(state, 1);
    lua_call(state, n, LUA_MULTRET);
    return lua_gettop(state);
}

/* setmetatable without finalizers: Lua runs __gc with hooks switched off, so the running time
   limit could not end an endless loop there. A table only gets a finalizer if __gc is in the
   metatable at this moment, so this check is enough. */
static int scrSetMetatable(lua_State *state)
{
    int n = lua_gettop(state);

    if (lua_istable(state, 2))
    {
        lua_pushliteral(state, "__gc");
        if (lua_rawget(state, 2) != LUA_TNIL) return luaL_error(state, "__gc is not available in a game script");
        lua_pop(state, 1);
    }
    lua_pushvalue(state, lua_upvalueindex(1));
    lua_insert(state, 1);
    lua_call(state, n, LUA_MULTRET);
    return lua_gettop(state);
}

static int scrOpenLibraries(lua_State *state)
{
    static const char *removed[] = {"dofile", "loadfile", "load"}; /* no file access, no other code */
    static const char *guarded[] = {"find", "match", "gmatch", "gsub"};
    size_t i = 0;

    luaL_openselectedlibs(state, LUA_GLIBK | LUA_COLIBK | LUA_MATHLIBK | LUA_STRLIBK | LUA_TABLIBK | LUA_UTF8LIBK,
                          0);

    for (i = 0; i < sizeof(removed) / sizeof(removed[0]); i++)
    {
        lua_pushnil(state);
        lua_setglobal(state, removed[i]);
    }

    lua_getglobal(state, "setmetatable");
    lua_pushcclosure(state, scrSetMetatable, 1);
    lua_setglobal(state, "setmetatable");

    lua_getglobal(state, "string");
    for (i = 0; i < sizeof(guarded) / sizeof(guarded[0]); i++)
    {
        lua_getfield(state, -1, guarded[i]);
        lua_pushcclosure(state, scrPatternGuard, 1);
        lua_setfield(state, -2, guarded[i]);
    }
    lua_pop(state, 1);
    return 0;
}

/* Runs a script file from the game data (a replacement in MODFILES wins, see dskOpen).
   Returns 0 only if the script itself failed. */
static int scrRunFile(const char *directory, const char *fileName)
{
    char path[FILENAME_MAX];
    int32_t size = 0;
    char *buffer = NULL;
    FILE *file = NULL;
    int ok = 1;

    dskBuildPathName(directory, fileName, path);
    size = dskFileLength(path);
    if (size <= 0)
    {
        Log("Lua: %s not found", path);
        return 1;
    }

    if (!(buffer = (char *)MemAlloc(size)))
    {
        return 1;
    }
    if ((file = dskOpen(path, "rb", 0)))
    {
        dskRead(file, buffer, size);
        dskClose(file);

        /* the loader is protected by itself; text only, no precompiled code */
        if (luaL_loadbufferx(L, buffer, (size_t)size, fileName, "t") == LUA_OK)
        {
            ok = scrCall(0, 0);
        }
        else
        {
            scrLogError(scrErrorText(L));
            lua_pop(L, 1);
            ok = 0;
        }
        if (ok) Log("Lua: %s loaded", fileName);
    }
    MemFree(buffer, size);
    return ok;
}

/* a new state with the sandbox, without any script */
static int scrOpen(void)
{
    scrMemoryUsed = 0;
    if (!(L = lua_newstate(scrAlloc, NULL, 0)))
    {
        Log("Lua: cannot create state");
        return 0;
    }
    lua_atpanic(L, scrPanic);

    if (!scrProtected(scrOpenLibraries, NULL, 0))
    {
        scrClose();
        return 0;
    }
    return 1;
}

void scrInit(void)
{
    int failed[SCR_FILE_COUNT] = {0};
    int again = 1;
    int i = 0;

    if (L || scrDisabled) return;

    Log("Lua: %s", LUA_RELEASE);

    /* A script that fails may have run halfway. Then everything starts over without it, so the
       game uses its built-in values for all of that file. */
    while (again)
    {
        again = 0;
        if (!scrOpen()) return;

        for (i = 0; (i < SCR_FILE_COUNT) && !again; i++)
        {
            if (failed[i]) continue;
            if (!scrRunFile(SCRIPT_DIRECTORY, scrFiles[i]))
            {
                if (!L) return; /* given up by the panic handler */
                Log("Lua: %s is not used", scrFiles[i]);
                failed[i] = 1;
                again = 1;
                scrClose();
            }
        }
    }
}

void scrDone(void)
{
    scrClose();
    scrDisabled = 0;
    scrLastError[0] = '\0';
}

/* started on first use, so the original mode never runs a script */
static int scrReady(void)
{
    if (!L) scrInit();
    return L != NULL;
}

typedef struct
{
    const char *table;
    const char *level; /* NULL: table[name], otherwise table[level][name] */
    const char *name;
    lua_Integer min;
    lua_Integer max;
    int value;
} ScrNumberCall;

/* number from table[name] or table[level][name]; the value stays if it is missing or out of range */
static int scrNumberCall(lua_State *state)
{
    ScrNumberCall *call = (ScrNumberCall *)lua_touserdata(state, 1);

    if (lua_getglobal(state, call->table) != LUA_TTABLE) return 0;
    if (call->level && (lua_getfield(state, -1, call->level) != LUA_TTABLE)) return 0;
    if (lua_getfield(state, -1, call->name) == LUA_TNUMBER)
    {
        lua_Integer v = lua_tointeger(state, -1);
        if ((v >= call->min) && (v <= call->max)) call->value = (int)v; /* keep values sane */
    }
    return 0;
}

int scrBalancePercent(const char *what, int difficulty, int fallback)
{
    static const char *level[] = {"easy", "normal", "hard"};
    ScrNumberCall call = {"balance", NULL, what, 10, 1000, fallback};

    if ((difficulty < 0) || (difficulty > 2) || !what || !scrReady()) return fallback;

    /* balance[level][what] */
    call.level = level[difficulty];
    if (!scrProtected(scrNumberCall, &call, 1)) return fallback;
    return call.value;
}

int scrToolSkillPercent(const char *toolName)
{
    ScrNumberCall call = {"tools", NULL, toolName, 0, 100, -1};

    if (!toolName || !scrReady()) return -1;

    /* tools[name] */
    if (!scrProtected(scrNumberCall, &call, 1)) return -1;
    return call.value;
}

/* scripts are UTF-8, the game font uses code page 850: convert the letters the texts of the
   four languages need */
static const struct
{
    const char *utf8;
    char cp850;
} scrLetters[] = {{"\xc3\xa4", '\x84'}, {"\xc3\xb6", '\x94'}, {"\xc3\xbc", '\x81'}, {"\xc3\x84", '\x8e'},
                  {"\xc3\x96", '\x99'}, {"\xc3\x9c", '\x9a'}, {"\xc3\x9f", '\xe1'}, {"\xc3\xa9", '\x82'},
                  {"\xc3\xa8", '\x8a'}, {"\xc3\xa0", '\x85'}, {"\xc3\xa1", '\xa0'}, {"\xc3\xad", '\xa1'},
                  {"\xc3\xb3", '\xa2'}, {"\xc3\xba", '\xa3'}, {"\xc3\xb1", '\xa4'}, {"\xc3\xa7", '\x87'},
                  /* e a o i u with circumflex, u with grave, i e with diaeresis, E with acute */
                  {"\xc3\xaa", '\x88'}, {"\xc3\xa2", '\x83'}, {"\xc3\xb4", '\x93'}, {"\xc3\xae", '\x8c'},
                  {"\xc3\xbb", '\x96'}, {"\xc3\xb9", '\x97'}, {"\xc3\xaf", '\x8b'}, {"\xc3\xab", '\x89'},
                  {"\xc3\x89", '\x90'},
                  /* inverted question and exclamation mark, angle quotation marks */
                  {"\xc2\xbf", '\xa8'}, {"\xc2\xa1", '\xad'}, {"\xc2\xab", '\xae'}, {"\xc2\xbb", '\xaf'}};

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

typedef struct
{
    const ScrNewsInfo *info;
    char *lines;
    int maxLines;
    int lineSize;
    int count;
} ScrNewsCall;

static int scrNewsCall(lua_State *state)
{
    ScrNewsCall *call = (ScrNewsCall *)lua_touserdata(state, 1);
    const ScrNewsInfo *info = call->info;
    char building[256];
    int i = 0;

    if (lua_getglobal(state, "news") != LUA_TFUNCTION) return 0;

    scrFromGame(info->building ? info->building : "", building, sizeof(building));
    lua_createtable(state, 0, 7);
    lua_pushstring(state, building);
    lua_setfield(state, -2, "building");
    lua_pushboolean(state, info->escaped);
    lua_setfield(state, -2, "escaped");
    lua_pushstring(state, (info->alarm == 2) ? "loud" : (info->alarm == 1) ? "silent" : "none");
    lua_setfield(state, -2, "alarm");
    lua_pushinteger(state, info->loot);
    lua_setfield(state, -2, "loot");
    lua_pushinteger(state, info->knockedOut);
    lua_setfield(state, -2, "knocked_out");
    lua_pushinteger(state, info->day);
    lua_setfield(state, -2, "day");
    lua_pushstring(state, (info->lang == 0) ? "en" : (info->lang == 2) ? "fr" : (info->lang == 3) ? "es" : "de");
    lua_setfield(state, -2, "lang");

    lua_call(state, 1, 1);

    if (!lua_istable(state, -1)) return 0;
    for (i = 1; i <= call->maxLines; i++)
    {
        if (lua_geti(state, -1, i) != LUA_TSTRING) break;
        scrToGame(lua_tostring(state, -1), call->lines + call->count * call->lineSize, call->lineSize);
        call->count++;
        lua_pop(state, 1);
    }
    return 0;
}

int scrNews(const ScrNewsInfo *info, char *lines, int maxLines, int lineSize)
{
    ScrNewsCall call = {info, lines, maxLines, lineSize, 0};

    if (!info || !scrReady()) return 0;
    if (!scrProtected(scrNewsCall, &call, 1)) return 0; /* no half article */
    return call.count;
}

/* the weather names of PLUS/WEATHER.LUA, in the order of CFG_WEATHER_CLEAR .. CFG_WEATHER_FOG */
static const char *scrWeatherName[] = {"clear", "cloudy", "rain", "storm", "fog"};
#define SCR_WEATHER_COUNT ((int)(sizeof(scrWeatherName) / sizeof(scrWeatherName[0])))

static int scrWeatherIndex(const char *name)
{
    int i = 0;

    for (i = 0; name && (i < SCR_WEATHER_COUNT); i++)
        if (!strcmp(name, scrWeatherName[i])) return i;
    return -1;
}

typedef struct
{
    int32_t day;
    uint32_t building;
    uint32_t roll;
    int result;
    lua_Integer weight[SCR_WEATHER_COUNT];
} ScrWeatherCall;

/* night_weather(day, building, roll) may decide by itself */
static int scrWeatherFunctionCall(lua_State *state)
{
    ScrWeatherCall *call = (ScrWeatherCall *)lua_touserdata(state, 1);

    if (lua_getglobal(state, "night_weather") != LUA_TFUNCTION) return 0;
    lua_pushinteger(state, call->day);
    lua_pushinteger(state, call->building);
    lua_pushinteger(state, call->roll);
    lua_call(state, 3, 1);
    if (lua_type(state, -1) == LUA_TSTRING) call->result = scrWeatherIndex(lua_tostring(state, -1));
    return 0;
}

/* the chances in weather[name] */
static int scrWeatherTableCall(lua_State *state)
{
    ScrWeatherCall *call = (ScrWeatherCall *)lua_touserdata(state, 1);
    int i = 0;

    if (lua_getglobal(state, "weather") != LUA_TTABLE) return 0;
    for (i = 0; i < SCR_WEATHER_COUNT; i++)
    {
        if (lua_getfield(state, -1, scrWeatherName[i]) == LUA_TNUMBER)
        {
            lua_Integer v = lua_tointeger(state, -1);
            if ((v > 0) && (v <= 1000)) call->weight[i] = v;
        }
        lua_pop(state, 1);
    }
    return 0;
}

int scrWeather(int32_t day, uint32_t building, uint32_t roll, uint32_t pick)
{
    ScrWeatherCall call = {day, building, roll, -1, {0}};
    lua_Integer total = 0;
    int i = 0;

    if (!scrReady()) return -1;

    scrProtected(scrWeatherFunctionCall, &call, 1);
    if (call.result >= 0) return call.result;

    /* otherwise the chances; an error in night_weather() does not keep the game from using them */
    if (!L || !scrProtected(scrWeatherTableCall, &call, 1)) return -1;
    for (i = 0; i < SCR_WEATHER_COUNT; i++) total += call.weight[i];

    if (total <= 0) return -1;

    pick %= (uint32_t)total;
    for (i = 0; i < SCR_WEATHER_COUNT; i++)
    {
        if (pick < (uint32_t)call.weight[i]) return i;
        pick -= (uint32_t)call.weight[i];
    }
    return -1;
}
