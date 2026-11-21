/*  _________  _______
   / ___/ __ \/ __/ _ \      Der Clou!
  / /__/ /_/ /\ \/ ___/ Open Source Project
  \___/\____/___/_/ http://cosp.sourceforge.net
   Based on the original by neo Software GmbH
*/
#include "disk/disk.h"

#include <SDL3/SDL.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

#include "disk/disk_e.h"

#if (defined(_WIN32) || defined(__WIN32__))
#include <direct.h> /* _mkdir */
#define mkdir(A, B) _mkdir(A)
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

static char RootPathName[FILENAME_MAX] = {0};
static char UserPathName[FILENAME_MAX] = {0};

static const char *USER_GAME_DATA_DIR = "DerClou";

void dskSetRootPath(const char *newRootPath) { snprintf(RootPathName, sizeof(RootPathName), "%s", newRootPath); }

const char *dskGetUserPath(void) { return UserPathName; }

void dskMakeDirectory(const char *path)
{
    int result = mkdir(path, 0755);
    if (result && errno != EEXIST)
    {
        Log("The directory \"%s\" could not be created.", path);
    }
}

void dskInitUserDataPath(void)
{
    if (UserPathName[0] == '\0')
    {
        char *path = SDL_GetPrefPath("", USER_GAME_DATA_DIR);
        if (!path)
        {
            NewErrorMsg(Disk_Defect, __FILE__, __func__, 1);
        }
        snprintf(UserPathName, sizeof(UserPathName), "%s", path);
        SDL_free(path);
        dskMakeDirectory(UserPathName);
        char result[FILENAME_MAX];
        dskBuildPathNameUser(DATADISK_DIRECTORY, "", result);
        dskMakeDirectory(result);

        #ifdef __EMSCRIPTEN__
        // Create UserDataPath and mount it to indexdb to persist saves on browser reload
        EM_ASM(
            const result = UTF8ToString($0);
            let infos = FS.analyzePath(result);
            if (!infos.exists) {
                console.debug("dskInitUserDataPath: Path does not exist", result);
                FS.mkdir(result);
            }
            else {
                console.debug("dskInitUserDataPath: Path does exist", result);
            }

            FS.mount(IDBFS, {}, result);
            Module.derclouSaveDir = result; /* the page can back up and restore this directory */
            Module.derclouUserDataReady = 0;
            FS.syncfs(true, function (err) {
                if (err) {console.error("dskInitUserDataPath: syncing failed", err)}
                Module.derclouUserDataReady = 1;
            });
        , result);

        /* the settings file is read right after this, so wait (at most 3 s) until IndexedDB is loaded */
        for (int i = 0; i < 300 && !EM_ASM_INT(return Module.derclouUserDataReady;); i++)
        {
            emscripten_sleep(10);
        }
        #endif
    }
}

char *dskGetRootPathN(char *result, size_t size)
{
    snprintf(result, size, "%s", RootPathName);
    return result;
}

/* 2019-11-22 LucyG  */
static int dskModfilesPathName(const char *pszPath, char *pszModfilesPath, size_t size)
{
    size_t rootPathLen = strlen(RootPathName);

    /* only files below the game directory can be replaced: root, separator, at least one character */
    if (rootPathLen && !strncmp(pszPath, RootPathName, rootPathLen) && pszPath[rootPathLen] &&
        pszPath[rootPathLen + 1])
    {
        /* also skip the separator */
        dskBuildPathNameN(MODFILES_DIRECTORY, &pszPath[rootPathLen + 1], pszModfilesPath, size);
        return 1;
    }
    return 0;
}

int dskResolvePath(const char *puch_Pathname, char *puch_Result, size_t size)
{
    char szModfilesPath[DSK_PATH_MAX];

    if (dskModfilesPathName(puch_Pathname, szModfilesPath, sizeof(szModfilesPath)))
    {
        FILE *file = fopen(szModfilesPath, "rb");
        if (file)
        {
            fclose(file);
            if (strlen(szModfilesPath) < size)
            {
                Log("Modfile: %s", szModfilesPath);
                snprintf(puch_Result, size, "%s", szModfilesPath);
                return 1;
            }
        }
    }
    snprintf(puch_Result, size, "%s", puch_Pathname);
    return 0;
}

FILE *dskOpen(const char *puch_Pathname, const char *puch_Mode, uword us_DiskId)
{
    FILE *p_File = NULL;

    /* 2019-11-22 LucyG : Modfiles support */
    if (puch_Mode[0] == 'r')
    {
        char szModfilesPath[DSK_PATH_MAX];
        if (dskModfilesPathName(puch_Pathname, szModfilesPath, sizeof(szModfilesPath)))
        {
            /* try to open modded file */
            p_File = fopen(szModfilesPath, puch_Mode);
            if (p_File) Log("Modfile: %s", szModfilesPath);
        }
    }

    if (!p_File)
    {
        /* open original file */
        p_File = fopen(puch_Pathname, puch_Mode);
    }

    if (!p_File)
    {
        Log("Could not open file %s\n", puch_Pathname);
        NewErrorMsg(Disk_Defect, __FILE__, __func__, ERR_DISK_OPEN_FAILED);
    }

    return p_File;
}

int32_t dskLoad(const char *puch_Pathname, void *p_MemDest)
{
    FILE *p_File = NULL;
    int32_t ul_SizeOfFile = 0;

    if ((ul_SizeOfFile = dskFileLength(puch_Pathname)) > 0)
    {
        if ((p_File = dskOpen(puch_Pathname, "rb", 0)))
        {
            dskRead(p_File, p_MemDest, ul_SizeOfFile);
            dskClose(p_File);
            return ul_SizeOfFile;
        }
    }
    return 0;
}

static void dskBuildPathNameGeneric(const char *puch_Directory, const char *puch_Filename, char *puch_Result,
                                    size_t size, const char *rootPath)
{
    int len = 0;

    if (!size) return;
    len = snprintf(puch_Result, size, "%s" DIR_SEPARATOR "%s" DIR_SEPARATOR "%s", rootPath, puch_Directory,
                   puch_Filename);
    if ((len < 0) || ((size_t)len >= size))
    {
        /* the truncated name does not exist, so opening it fails with the usual error */
        Log("Path too long (limit %u): %s" DIR_SEPARATOR "%s" DIR_SEPARATOR "%s", (unsigned)size, rootPath,
            puch_Directory, puch_Filename);
        if (len < 0) puch_Result[0] = '\0';
    }
    // convert all chars after rootPath to upper case as all data file names are upper case
    size_t rootLen = strlen(rootPath);
    if (rootLen > strlen(puch_Result)) return;
    char *cur = &puch_Result[rootLen];
    while (*cur != '\0')
    {
        *cur = (char)toupper((unsigned char)*cur);
        ++cur;
    }
}

void dskBuildPathNameN(const char *puch_Directory, const char *puch_Filename, char *puch_Result, size_t size)
{
    dskBuildPathNameGeneric(puch_Directory, puch_Filename, puch_Result, size, RootPathName);
}

void dskBuildPathNameUserN(const char *puch_Directory, const char *puch_Filename, char *puch_Result, size_t size)
{
    dskBuildPathNameGeneric(puch_Directory, puch_Filename, puch_Result, size, UserPathName);
}

/* never negative: 0 for a file that is missing, empty or cannot be measured */
int32_t dskFileLength(const char *puch_Pathname)
{
    long l_Size = 0;
    FILE *file = NULL;
    char szModfilesPath[DSK_PATH_MAX];

    /* same lookup as dskOpen: a file in modfiles replaces the original, so its size counts */
    if (dskModfilesPathName(puch_Pathname, szModfilesPath, sizeof(szModfilesPath)))
        file = fopen(szModfilesPath, "rb");
    if (!file) file = fopen(puch_Pathname, "rb");

    if (file)
    {
        if (fseek(file, 0, SEEK_END) == 0) l_Size = ftell(file);
        fclose(file);
    }
    if ((l_Size < 0) || (l_Size > INT32_MAX)) l_Size = 0;

    return (int32_t)l_Size;
}

void dskClose(FILE *p_File)
{
    if (p_File) fclose(p_File);
}

void dskWrite(FILE *p_File, void *p_MemSource, uint32_t ul_Size)
{
    if (ul_Size != fwrite(p_MemSource, 1, ul_Size, p_File))
    {
        NewErrorMsg(Disk_Defect, __FILE__, __func__, ERR_DISK_WRITE_FAILED);
    }
}

void dskRead(FILE *p_File, void *p_MemDest, uint32_t ul_Size)
{
    uint32_t ul_Read = 0;
    ul_Read = fread(p_MemDest, 1, ul_Size, p_File);
    if (ul_Read != ul_Size)
    {
        NewErrorMsg(Disk_Defect, __FILE__, __func__, ERR_DISK_READ_FAILED);
    }
}

// feof() didn't seem to work
// as expected...
int dskIsEOF(FILE *p_File)
{
    if (fgetc(p_File) == EOF) return (-1);
    fseek(p_File, -1, SEEK_CUR);
    return 0;
}

// fgets() has this CR/LF problem
char *dskGets(char *text, int n, FILE *file)
{
    char *p = NULL;
    if (fgets(text, n, file))
    {
        if ((p = strrchr(text, 13)))
        {
            *p = '\0';
        }
        if ((p = strrchr(text, 10)))
        {
            *p = '\0';
        }
        return text;
    }
    return NULL;
}

/* 2026-11-20 persist the user data directory (IDBFS) in the browser build.
   Without this, files written outside of tcSaveTheClou (e.g. burglary plans)
   are lost on page reload. No-op on native builds. */
void dskSyncUserData(void)
{
#ifdef __EMSCRIPTEN__
    EM_ASM(
        FS.syncfs(false, function (err) {if (err) {console.error("dskSyncUserData: syncing failed", err)}});
    );
#endif
}
