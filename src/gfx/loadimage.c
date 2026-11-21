/*  _________  _______
   / ___/ __ \/ __/ _ \      Der Clou!
  / /__/ /_/ /\ \/ ___/ Open Source Project
  \___/\____/___/_/ https://cosp.sourceforge.net
   Based on the original by neo Software GmbH
*/
#include <stddef.h>
#include <string.h>
#include <SDL3/SDL.h>
#include "gfx/gfx.h"
#include "memory/memory.h"
// #include "base\base.h"
// #include "text\text.h"
#include "disk/disk.h"

typedef struct
{
    uword width;
    uword height;
    word xOrigin;
    word yOrigin;
    ubyte numPlanes;
    ubyte mask;
    ubyte compression;
    ubyte padding;
    uword transClr;
    ubyte xAspect;
    ubyte yAspect;
    uword pageWidth;
    uword pageHeight;
} ILBMHeader;

static const ubyte idFORM[] = {'F', 'O', 'R', 'M'};
static const ubyte idILBM[] = {'I', 'L', 'B', 'M'};
static const ubyte idBMHD[] = {'B', 'M', 'H', 'D'};
static const ubyte idBODY[] = {'B', 'O', 'D', 'Y'};
static const ubyte idCMAP[] = {'C', 'M', 'A', 'P'};

static inline uint32_t PeekL_BE(ubyte *p)
{
    return (((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3]);
}

static inline uword PeekW_BE(ubyte *p) { return (((uword)p[0] << 8) | (uword)p[1]); }

static uint32_t IFFFindChunk(ubyte *pFileBuffer, int32_t sizeOfForm, const ubyte *chkID)
{
    uint32_t currPos = 12;
    while ((currPos + 4) <= sizeOfForm)
    {
        if (!memcmp(&pFileBuffer[currPos], chkID, 4))
        {
            currPos += 4;
            return currPos;
        }
        currPos += 2;
    }
    return 0;
}

static void MakeMCGA(ubyte b, ubyte *pic, ubyte PlSt, int c)
{
    if ((b & 0x80) && (c > 0)) pic[0] |= PlSt;
    if ((b & 0x40) && (c > 1)) pic[1] |= PlSt;
    if ((b & 0x20) && (c > 2)) pic[2] |= PlSt;
    if ((b & 0x10) && (c > 3)) pic[3] |= PlSt;
    if ((b & 0x08) && (c > 4)) pic[4] |= PlSt;
    if ((b & 0x04) && (c > 5)) pic[5] |= PlSt;
    if ((b & 0x02) && (c > 6)) pic[6] |= PlSt;
    if ((b & 0x01) && (c > 7)) pic[7] |= PlSt;
}

/* Both decoders read at most bodySize bytes and write only inside the surface. They return 0 if the
   BODY ends before the picture is complete (the rest of the picture stays empty). */
static int ILBMUncompressToSurface(ubyte *pBody, uint32_t bodySize, ILBMHeader *hdr, SDL_Surface *pSurface)
{
    ubyte *pBodyEnd = pBody + bodySize;
    ubyte *pPlane = NULL;
    ubyte *pLine = NULL;
    ubyte *pEnd = NULL;
    int x = 0;
    int y = 0;
    int plane = 0;
    int breite = 0;
    int room = 0;
    uword a = 0;
    ubyte o = 0;

    pLine = (ubyte *)pSurface->pixels;
    pEnd = pLine + (size_t)pSurface->pitch * (size_t)pSurface->h;
    for (y = 1; y <= hdr->height; y++)
    {
        for (plane = 0; plane < hdr->numPlanes; plane++)
        {
            pPlane = pLine;
            breite = (hdr->width + 15) & 0xFFF0;
            do
            {
                if (pBody >= pBodyEnd) return 0;
                a = *pBody++; /* Kommando (wiederholen oder uebernehmen */
                if (a > 128)
                { /* Zeichen wiederholen */
                    a = 257 - a;
                    if (pBody >= pBodyEnd) return 0;
                    o = *pBody++;
                    for (x = 1; x <= a; x++)
                    {
                        /* the last line has no room for the padding up to 16 pixels */
                        room = (pPlane < pEnd) ? (int)SDL_min((ptrdiff_t)breite, pEnd - pPlane) : 0;
                        MakeMCGA(o, pPlane, (1 << plane), room);
                        pPlane += 8;
                        breite -= 8;
                    }
                }
                else
                { /* Zeichen uebernehmen */
                    for (x = 0; x <= a; x++)
                    {
                        if (pBody >= pBodyEnd) return 0;
                        o = *pBody++;
                        room = (pPlane < pEnd) ? (int)SDL_min((ptrdiff_t)breite, pEnd - pPlane) : 0;
                        MakeMCGA(o, pPlane, (1 << plane), room);
                        pPlane += 8;
                        breite -= 8;
                    }
                }
            } while (breite > 0);
        }
        pLine += pSurface->pitch;
    }
    return 1;
}

static int ILBMCopyToSurface(ubyte *pBody, uint32_t bodySize, ILBMHeader *hdr, SDL_Surface *pSurface)
{
    ubyte *pBodyEnd = pBody + bodySize;
    ubyte *pPlane = NULL;
    ubyte *pLine = NULL;
    ubyte *pEnd = NULL;
    int y = 0;
    int plane = 0;
    int breite = 0;
    int room = 0;
    ubyte o = 0;

    pLine = (ubyte *)pSurface->pixels;
    pEnd = pLine + (size_t)pSurface->pitch * (size_t)pSurface->h;
    for (y = 1; y <= hdr->height; y++)
    {
        for (plane = 0; plane < hdr->numPlanes; plane++)
        {
            pPlane = pLine;
            breite = (hdr->width + 15) & 0xFFF0;
            do
            {
                if (pBody >= pBodyEnd) return 0;
                o = *pBody++;
                room = (pPlane < pEnd) ? (int)SDL_min((ptrdiff_t)breite, pEnd - pPlane) : 0;
                MakeMCGA(o, pPlane, (1 << plane), room);
                pPlane += 8;
                breite -= 8;
            } while (breite > 0);
        }
        pLine += pSurface->pitch;
    }
    return 1;
}

static SDL_Surface *gfxLoadILBM(ubyte *pFileBuffer, int32_t ulSizeOfFile, const char *puch_Pathname)
{
    if (ulSizeOfFile < 12 || memcmp(pFileBuffer, idFORM, 4) || memcmp(&pFileBuffer[8], idILBM, 4))
    {
        Log("Not an ILBM file: %s\n", puch_Pathname);
        return NULL;
    }
    uint32_t formLength = PeekL_BE(&pFileBuffer[4]);
    if (formLength > (uint32_t)ulSizeOfFile - 8)
    {
        // file truncated!
        Log("ILBM truncated: %s\n", puch_Pathname);
        return NULL;
    }
    int32_t sizeOfForm = (int32_t)formLength + 8;
    if (sizeOfForm > ulSizeOfFile)
    {
        // file truncated!
        Log("ILBM truncated: %s\n", puch_Pathname);
        return NULL;
    }
    uint32_t currPos = IFFFindChunk(pFileBuffer, sizeOfForm, idBMHD);
    if (!currPos)
    {
        Log("No BMHD found: %s\n", puch_Pathname);
        return NULL;
    }

    /* the chunk length and the 20 bytes of the header must lie inside the file */
    if (currPos + 4 + 20 > (uint32_t)sizeOfForm)
    {
        Log("Invalid BMHD: %s\n", puch_Pathname);
        return NULL;
    }
    int32_t chunkSize = PeekL_BE(&pFileBuffer[currPos]);
    currPos += 4;
    if (chunkSize < 20)
    {
        Log("Invalid BMHD: %s\n", puch_Pathname);
        return NULL;
    }

    ILBMHeader hdr;
    hdr.width = PeekW_BE(&pFileBuffer[currPos]);
    currPos += 2;
    hdr.height = PeekW_BE(&pFileBuffer[currPos]);
    currPos += 2;
    hdr.xOrigin = PeekW_BE(&pFileBuffer[currPos]);
    currPos += 2;
    hdr.yOrigin = PeekW_BE(&pFileBuffer[currPos]);
    currPos += 2;
    hdr.numPlanes = pFileBuffer[currPos];
    currPos++;
    hdr.mask = pFileBuffer[currPos];
    currPos++;
    hdr.compression = pFileBuffer[currPos];
    currPos++;
    currPos++;  // padding
    hdr.transClr = PeekW_BE(&pFileBuffer[currPos]);
    currPos += 2;
    hdr.xAspect = pFileBuffer[currPos];
    currPos++;
    hdr.yAspect = pFileBuffer[currPos];
    currPos++;
    hdr.pageWidth = PeekW_BE(&pFileBuffer[currPos]);
    currPos += 2;
    hdr.pageHeight = PeekW_BE(&pFileBuffer[currPos]);
    currPos += 2;

    if (hdr.compression > 1)
    {
        Log("Unsupported compression: %s\n", puch_Pathname);
        return NULL;
    }
    /* the pictures are stored as up to 8 bit planes for the 256 colour palette */
    if (!hdr.width || !hdr.height || !hdr.numPlanes || (hdr.numPlanes > 8))
    {
        Log("Invalid BMHD (%u x %u, %u planes): %s\n", hdr.width, hdr.height, hdr.numPlanes, puch_Pathname);
        return NULL;
    }

    currPos = IFFFindChunk(pFileBuffer, sizeOfForm, idBODY);
    if (!currPos)
    {
        Log("No BODY found: %s\n", puch_Pathname);
        return NULL;
    }
    if (currPos + 4 > (uint32_t)sizeOfForm)
    {
        Log("Invalid BODY: %s\n", puch_Pathname);
        return NULL;
    }
    /* the BODY ends with the file at the latest, whatever its length field says */
    uint32_t bodySize = PeekL_BE(&pFileBuffer[currPos]);
    currPos += 4;
    if (bodySize > (uint32_t)sizeOfForm - currPos) bodySize = (uint32_t)sizeOfForm - currPos;

    SDL_Surface *pSurface = gfxCreateIndexedSurface(hdr.width, hdr.height);
    if (!pSurface)
    {
        Log("Failed to create surface: %s\n", puch_Pathname);
        return NULL;
    }

    int complete = 0;
    if (hdr.compression == 1)
    {
        complete = ILBMUncompressToSurface(&pFileBuffer[currPos], bodySize, &hdr, pSurface);
    }
    else
    {
        complete = ILBMCopyToSurface(&pFileBuffer[currPos], bodySize, &hdr, pSurface);
    }
    if (!complete)
    {
        Log("ILBM BODY ends early: %s\n", puch_Pathname);
    }

    currPos = IFFFindChunk(pFileBuffer, sizeOfForm, idCMAP);
    if (currPos && (currPos + 4 <= (uint32_t)sizeOfForm))
    {
        uint32_t cmapSize = PeekL_BE(&pFileBuffer[currPos]);
        currPos += 4;
        /* only colours that lie inside the file */
        if (cmapSize > (uint32_t)sizeOfForm - currPos) cmapSize = (uint32_t)sizeOfForm - currPos;
        int numColors = (int)(cmapSize / 3);
        if (numColors > 0)
        {
            if (numColors > 256) numColors = 256;

            int c = 0;
            SDL_Color colors[256];
            for (c = 0; c < numColors; c++)
            {
                colors[c].r = pFileBuffer[currPos];
                currPos++;
                colors[c].g = pFileBuffer[currPos];
                currPos++;
                colors[c].b = pFileBuffer[currPos];
                currPos++;
                colors[c].a = SDL_ALPHA_OPAQUE;
            }
            SDL_SetPaletteColors(SDL_GetSurfacePalette(pSurface), colors, 0, numColors);
        }
    }
    return pSurface;
}

/* decodes one picture file; NULL (with a line in the log) unless it gives a 256 colour picture */
static SDL_Surface *gfxLoadImageFile(const char *puch_Pathname)
{
    FILE *p_File = NULL;
    long l_Size = 0;
    int32_t ul_SizeOfFile = 0;
    ubyte *p_FileBuffer = NULL;
    SDL_Surface *pSurface = NULL;

    p_File = fopen(puch_Pathname, "rb");
    if (!p_File)
    {
        Log("gfxLoadImage failed to open: %s", puch_Pathname);
        return NULL;
    }
    if (fseek(p_File, 0, SEEK_END) == 0) l_Size = ftell(p_File);
    if ((l_Size <= 0) || (l_Size > INT32_MAX) || (fseek(p_File, 0, SEEK_SET) != 0))
    {
        fclose(p_File);
        Log("gfxLoadImage empty file: %s", puch_Pathname);
        return NULL;
    }
    ul_SizeOfFile = (int32_t)l_Size;

    p_FileBuffer = MemAlloc(ul_SizeOfFile);
    if (!p_FileBuffer)
    {
        fclose(p_File);
        Log("gfxLoadImage out of memory: %s", puch_Pathname);
        return NULL;
    }
    if (fread(p_FileBuffer, 1, (size_t)ul_SizeOfFile, p_File) != (size_t)ul_SizeOfFile)
    {
        fclose(p_File);
        MemFree(p_FileBuffer, ul_SizeOfFile);
        Log("gfxLoadImage failed to read: %s", puch_Pathname);
        return NULL;
    }
    fclose(p_File);

    if ((ul_SizeOfFile >= 12) && !memcmp(p_FileBuffer, "FORM", 4) && !memcmp(p_FileBuffer + 8, "ILBM", 4))
    {
        pSurface = gfxLoadILBM(p_FileBuffer, ul_SizeOfFile, puch_Pathname);
    }
    else
    {
        /* replacements may be PNG (with a 256 colour palette) or BMP */
        pSurface = SDL_LoadSurface_IO(SDL_IOFromConstMem(p_FileBuffer, ul_SizeOfFile), true);
        if (!pSurface) Log("gfxLoadImage: %s: %s", puch_Pathname, SDL_GetError());
    }
    MemFree(p_FileBuffer, ul_SizeOfFile);

    /* the whole game draws with one palette of 256 colours, nothing else can be shown */
    if (pSurface && ((pSurface->format != SDL_PIXELFORMAT_INDEX8) || !SDL_GetSurfacePalette(pSurface)))
    {
        Log("gfxLoadImage: not a picture with a 256 colour palette (%s): %s",
            SDL_GetPixelFormatName(pSurface->format), puch_Pathname);
        SDL_DestroySurface(pSurface);
        pSurface = NULL;
    }
    return pSurface;
}

/* NULL if the picture cannot be used; callers must check */
SDL_Surface *gfxLoadImage(const char *puch_Pathname)
{
    char path[DSK_PATH_MAX];
    SDL_Surface *pSurface = NULL;

    /* a replacement in modfiles is used instead of the original */
    int replaced = dskResolvePath(puch_Pathname, path, sizeof(path));

    pSurface = gfxLoadImageFile(path);
    if (!pSurface && replaced)
    {
        Log("gfxLoadImage: replacement not usable, loading the original: %s", puch_Pathname);
        pSurface = gfxLoadImageFile(puch_Pathname);
    }
    if (!pSurface)
    {
        Log("gfxLoadImage pSurface = NULL: %s", puch_Pathname);
    }
    return pSurface;
}
