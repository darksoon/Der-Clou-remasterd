/*
** MXR - A Simple Audio Mixer
** Copyright (c) 2017 by Oliver Gantert.
**
** Permission is given by the author to use this version of MXR freely,
** provided that the message "Copyright (c) 2017 by Oliver Gantert" in this
** and associated files is not changed or removed.
**
** See "mxr.h" for details.
*/
#include "mxr.h"

#include "disk/disk.h"

/******************************************************************************/

typedef struct
{
    uint32_t dwChunkID;
    uint32_t dwChunkSize;
} WAVChunk;

typedef struct
{
    unsigned short wFormatTag;
    unsigned short wChannels;
    uint32_t dwSamplesPerSec;
    uint32_t dwAvgBytesPerSec;
    unsigned short wBlockAlign;
    unsigned short wBitsPerSample;
} WAVFmtChunk;

typedef struct
{
    FILE *pFile;
    WAVFmtChunk fmt;
    uint32_t dwNumSamples;
    uint32_t dwDataOffset;
    uint32_t dwDataSize;
    uint32_t dwSampleSize;
    uint32_t dwPosition;
} WAVFile;

#define makefourcc(ch0, ch1, ch2, ch3)                                                \
    ((unsigned int)(unsigned char)(ch0) | ((unsigned int)(unsigned char)(ch1) << 8) | \
     ((unsigned int)(unsigned char)(ch2) << 16) | ((unsigned int)(unsigned char)(ch3) << 24))

/******************************************************************************/

/* accepted sample rates; the mixer buffer is made for inputs up to twice the output rate */
#define WAV_MIN_RATE 4000
#define WAV_MAX_RATE 48000

static int32_t WAVOpen(WAVFile *wav, const char *pszFileName)
{
    WAVChunk chk;
    char szPath[DSK_PATH_MAX];
    const char *pszReason = "not a RIFF/WAVE file";
    long nFileSize = 0;
    long nPos = 0;
    int bHaveFmt = 0;

    /* a replacement in MODFILES wins, like for every other game file */
    dskResolvePath(pszFileName, szPath, sizeof(szPath));

    wav->pFile = fopen(szPath, "rb");
    if (!wav->pFile)
    {
        return 0;
    }

    if (fseek(wav->pFile, 0, SEEK_END) != 0) goto Lgetout;
    nFileSize = ftell(wav->pFile);
    if (fseek(wav->pFile, 0, SEEK_SET) != 0) goto Lgetout;

    if (fread(&chk, 1, sizeof(WAVChunk), wav->pFile) != sizeof(WAVChunk)) goto Lgetout;
    if (chk.dwChunkID != makefourcc('R', 'I', 'F', 'F')) goto Lgetout;
    if (fread(&chk.dwChunkID, 1, sizeof(int32_t), wav->pFile) != sizeof(int32_t)) goto Lgetout;
    if (chk.dwChunkID != makefourcc('W', 'A', 'V', 'E')) goto Lgetout;

    /* walk the chunks up to "data"; other chunks ("LIST", "fact", ...) are skipped */
    for (;;)
    {
        pszReason = bHaveFmt ? "no data chunk" : "no fmt chunk";
        if (fread(&chk, 1, sizeof(WAVChunk), wav->pFile) != sizeof(WAVChunk)) goto Lgetout;
        nPos = ftell(wav->pFile);
        if ((nPos < 0) || (nPos > nFileSize)) goto Lgetout;

        if (chk.dwChunkID == makefourcc('d', 'a', 't', 'a'))
        {
            if (!bHaveFmt) goto Lgetout;
            break;
        }

        pszReason = "truncated file";
        if (chk.dwChunkSize > (uint32_t)(nFileSize - nPos)) goto Lgetout;

        if (chk.dwChunkID == makefourcc('f', 'm', 't', ' '))
        {
            pszReason = "bad fmt chunk";
            if (chk.dwChunkSize < sizeof(WAVFmtChunk)) goto Lgetout;
            if (fread(&wav->fmt, 1, sizeof(WAVFmtChunk), wav->pFile) != sizeof(WAVFmtChunk)) goto Lgetout;
            bHaveFmt = 1;
        }

        /* chunks are word aligned */
        if (fseek(wav->pFile, nPos + (long)chk.dwChunkSize + (long)(chk.dwChunkSize & 1), SEEK_SET) != 0)
            goto Lgetout;
    }

    /* the mixer handles nothing else, see MXR_ProcessMixer */
    pszReason = "only uncompressed PCM is supported";
    if (wav->fmt.wFormatTag != 1) goto Lgetout;
    pszReason = "only mono and stereo are supported";
    if ((wav->fmt.wChannels != 1) && (wav->fmt.wChannels != 2)) goto Lgetout;
    pszReason = "only 8 and 16 bit are supported";
    if ((wav->fmt.wBitsPerSample != 8) && (wav->fmt.wBitsPerSample != 16)) goto Lgetout;
    pszReason = "sample rate must be 4000 to 48000 Hz";
    if ((wav->fmt.dwSamplesPerSec < WAV_MIN_RATE) || (wav->fmt.dwSamplesPerSec > WAV_MAX_RATE)) goto Lgetout;

    pszReason = "no sample data";
    wav->dwSampleSize = (wav->fmt.wChannels * wav->fmt.wBitsPerSample) >> 3;
    if (!wav->dwSampleSize) goto Lgetout;
    wav->dwDataSize = chk.dwChunkSize;
    /* a data chunk that claims more than the file holds ends with the file */
    if (wav->dwDataSize > (uint32_t)(nFileSize - nPos)) wav->dwDataSize = (uint32_t)(nFileSize - nPos);
    if (!wav->dwDataSize) goto Lgetout;
    wav->dwDataOffset = (uint32_t)nPos;
    wav->dwNumSamples = wav->dwDataSize / wav->dwSampleSize;
    if (!wav->dwNumSamples) goto Lgetout;
    wav->dwPosition = 0;

    return 1;

Lgetout:
    Log("WAV file rejected (%s): %s", pszReason, szPath);
    fclose(wav->pFile);
    wav->pFile = NULL;
    return 0;
}

/******************************************************************************/

typedef struct
{
    MXR_Input mxrInput;
    WAVFile wavFile;
    int bLoop;
} MXR_InputWAV;

static uint32_t MXR_ProcessInputWAV(MXR_InputWAV *pInput, void *pStream, uint32_t nNumSamples)
{
    int32_t nSamplesRemain = 0;
    int32_t nSamplesRead = 0;
    nSamplesRemain = (pInput->wavFile.dwDataSize - pInput->wavFile.dwPosition) / pInput->mxrInput.fmt.nSampleSize;
    if (nNumSamples > nSamplesRemain)
    {
        nNumSamples = nSamplesRemain;
    }
    if (nNumSamples > 0)
    {
        nSamplesRead = fread(pStream, pInput->mxrInput.fmt.nSampleSize, nNumSamples, pInput->wavFile.pFile);
        if (nSamplesRead > 0)
        {
            pInput->wavFile.dwPosition += (nSamplesRead * pInput->mxrInput.fmt.nSampleSize);
            return nSamplesRead;
        }
    }
    return 0;  // MXR will destroy this input
}

/* looping input: starts again at the beginning of the data when the end is reached */
static uint32_t MXR_ProcessInputWAVLoop(MXR_InputWAV *pInput, void *pStream, uint32_t nNumSamples)
{
    uint32_t nDone = 0;
    int nRestarts = 0;

    while ((nDone < nNumSamples) && (nRestarts < 2))
    {
        uint32_t n = MXR_ProcessInputWAV(pInput, (unsigned char *)pStream + nDone * pInput->mxrInput.fmt.nSampleSize,
                                         nNumSamples - nDone);
        if (n)
        {
            nDone += n;
            nRestarts = 0;
        }
        else
        {
            fseek(pInput->wavFile.pFile, (long)pInput->wavFile.dwDataOffset, SEEK_SET);
            pInput->wavFile.dwPosition = 0;
            nRestarts++;
        }
    }
    return nDone;
}

static void MXR_DestroyInputWAV(MXR_InputWAV *pInput)
{
    if (pInput->wavFile.pFile)
    {
        fclose(pInput->wavFile.pFile);
    }
    MXR_MemFree(pInput, sizeof(MXR_InputWAV));
}

MXR_Input *MXR_CreateInputWAV(const char *pszFileName)
{
    MXR_InputWAV *pInput = NULL;
    pInput = (MXR_InputWAV *)MXR_MemAlloc(sizeof(MXR_InputWAV));
    if (!pInput)
    {
        return NULL;
    }
    pInput->mxrInput.pProcess = (MXR_ProcessInputFunc)MXR_ProcessInputWAV;
    pInput->mxrInput.pDestroy = (MXR_DestroyInputFunc)MXR_DestroyInputWAV;
    pInput->mxrInput.nVolume = 128;
    pInput->bLoop = 0;

    if (!WAVOpen(&pInput->wavFile, pszFileName))
    {
        MXR_MemFree(pInput, sizeof(MXR_InputWAV));
        return NULL;
    }

    pInput->mxrInput.fmt.nSamplesPerSec = pInput->wavFile.fmt.dwSamplesPerSec;
    pInput->mxrInput.fmt.nBitsPerChannel = pInput->wavFile.fmt.wBitsPerSample;
    pInput->mxrInput.fmt.nNumChannels = pInput->wavFile.fmt.wChannels;
    pInput->mxrInput.fmt.nSampleSize = (pInput->mxrInput.fmt.nNumChannels * pInput->mxrInput.fmt.nBitsPerChannel) >> 3;

    return ((MXR_Input *)pInput);
}

/******************************************************************************/

MXR_Input *MXR_CreateInputWAVLoop(const char *pszFileName)
{
    MXR_InputWAV *pInput = (MXR_InputWAV *)MXR_CreateInputWAV(pszFileName);

    if (pInput)
    {
        pInput->bLoop = 1;
        pInput->mxrInput.pProcess = (MXR_ProcessInputFunc)MXR_ProcessInputWAVLoop;
    }
    return (MXR_Input *)pInput;
}
