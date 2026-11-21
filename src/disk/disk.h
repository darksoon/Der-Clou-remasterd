/*  _________  _______
   / ___/ __ \/ __/ _ \      Der Clou!
  / /__/ /_/ /\ \/ ___/ Open Source Project
  \___/\____/___/_/ http://cosp.sourceforge.net
   Based on the original by neo Software GmbH
*/
#ifndef MODULE_DISK
#define MODULE_DISK

#include "theclou.h"

#ifndef __STDIO_H
#include <stdio.h>
#endif

#ifndef __STDARG_H
#include <stdarg.h>
#endif

#ifndef __STRING_H
#include <string.h>
#endif

#include "theclou.h"

#ifndef MODULE_ERROR
#include "error/error.h"
#endif

extern char CDDrive[256];

#define DSK_PATHNAMELENGTH 255L
#define DSK_DUMMY_DISK_ID 0L /* to support functions with usefull parameter */

#define ERR_DISK_OPEN_FAILED 1
#define ERR_DISK_WRITE_FAILED 2
#define ERR_DISK_READ_FAILED 3

extern void dskSetRootPath(const char *newRootPath);
extern void dskInitUserDataPath(void);
extern void dskMakeDirectory(const char *path);
extern char *dskGetRootPathN(char *result, size_t size);
extern const char *dskGetUserPath(void); /* ends with a directory separator */

extern int32_t dskLoad(const char *puch_Pathname, void *p_MemDest);
extern void dskBuildPathNameN(const char *puch_Directory, const char *puch_Filename, char *puch_Result, size_t size);
extern void dskBuildPathNameUserN(const char *puch_Directory, const char *puch_Filename, char *puch_Result,
                                  size_t size);

/* The result must be a char array (not a pointer), its size limits what is written.
   Path buffers should have DSK_PATH_MAX bytes. */
#define DSK_PATH_MAX FILENAME_MAX
#define DSK_ARRAY_SIZE(a) (sizeof(a) + 0 * sizeof(char[(sizeof(a) > sizeof(char *)) ? 1 : -1]))
#define dskBuildPathName(dir, file, result) dskBuildPathNameN((dir), (file), (result), DSK_ARRAY_SIZE(result))
#define dskBuildPathNameUser(dir, file, result) dskBuildPathNameUserN((dir), (file), (result), DSK_ARRAY_SIZE(result))
#define dskGetRootPath(result) dskGetRootPathN((result), DSK_ARRAY_SIZE(result))

/* Path of the file that is really used: the replacement in MODFILES if there is one, else the
   original path. Returns 1 if the result is a replacement. */
extern int dskResolvePath(const char *puch_Pathname, char *puch_Result, size_t size);
extern void dskClose(FILE *p_File);
extern void dskWrite(FILE *p_File, void *p_MemSource, uint32_t ul_Size);
extern void dskRead(FILE *p_File, void *p_MemDest, uint32_t ul_Size);

extern int32_t dskFileLength(const char *puch_Pathname);

extern FILE *dskOpen(const char *puch_Pathname, const char *puch_Mode, uword us_DiskId);

extern int dskIsEOF(FILE *p_File);

extern char *dskGets(char *text, int n, FILE *file);

extern void dskSyncUserData(void);

#endif
