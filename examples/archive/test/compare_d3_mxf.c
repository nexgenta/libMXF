/*
 * $Id: compare_d3_mxf.c,v 1.1 2007/09/11 13:24:47 stuart_hc Exp $
 *
 * 
 *
 * Copyright (C) 2007  Philip de Nier <philipn@users.sourceforge.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>



#define CHECK_ARGUMENT_PRESENT(arg) \
    if (cmdln + 1 >= argc) \
    { \
        usage(argv[0]); \
        fprintf(stderr, "Missing argument for %s\n", arg); \
        return 1; \
    }

#define CHECK(cmd) \
    if (!(cmd)) \
    { \
        fprintf(stderr, "'%s' failed in line %d\n", #cmd, __LINE__); \
        exit(1); \
    }

#define CHK_ORET(cmd) \
    if (!(cmd)) \
    { \
        fprintf(stderr, "'%s' failed in line %d\n", #cmd, __LINE__); \
        return 0; \
    }

#define CHK_OFAIL(cmd) \
    if (!(cmd)) \
    { \
        fprintf(stderr, "'%s' failed in line %d\n", #cmd, __LINE__); \
        goto fail; \
    }

#define SAFE_FREE(ppdata) \
    if ((*ppdata) != NULL) \
    { \
        free(*ppdata); \
        *ppdata = NULL; \
    }

    
typedef struct
{
    int hour;
    int min;
    int sec;
    int frame;
} Timecode;

typedef struct
{
    int64_t frameCount;
    int64_t positionA;
    int64_t positionB;
    
    int64_t vitcDiffCount;
    int64_t ltcDiffCount;
    int64_t videoDiffCount;
    int64_t audioDiffCount[4];
} Summary;

typedef struct
{
    uint8_t octet0;
    uint8_t octet1;
    uint8_t octet2;
    uint8_t octet3;
    uint8_t octet4;
    uint8_t octet5;
    uint8_t octet6;
    uint8_t octet7;
    uint8_t octet8;
    uint8_t octet9;
    uint8_t octet10;
    uint8_t octet11;
    uint8_t octet12;
    uint8_t octet13;
    uint8_t octet14;
    uint8_t octet15;
} mxfKey;


static const mxfKey g_PartitionPackKeyPrefix = 
    {0x06, 0x0e, 0x2b, 0x34, 0x02, 0x05, 0x01, 0x01, 0x0d, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0x00};

static const mxfKey g_SystemItemElementKey = 
    {0x06, 0x0e, 0x2b, 0x34, 0x02, 0x53, 0x01, 0x01 , 0x0d, 0x01, 0x03, 0x01, 0x14, 0x02, 0x01, 0x00};
    
static const mxfKey g_VideoItemElementKey = 
    {0x06, 0x0e, 0x2b, 0x34, 0x01, 0x02, 0x01, 0x01, 0x0d, 0x01, 0x03, 0x01, 0x15, 0x01, 0x02, 0x01};
    
static const mxfKey g_AudioItemElementKey[4] =
{
    {0x06, 0x0e, 0x2b, 0x34, 0x01, 0x02, 0x01, 0x01, 0x0d, 0x01, 0x03, 0x01, 0x16, 0x04, 0x01, 0x01},
    {0x06, 0x0e, 0x2b, 0x34, 0x01, 0x02, 0x01, 0x01, 0x0d, 0x01, 0x03, 0x01, 0x16, 0x04, 0x01, 0x02},
    {0x06, 0x0e, 0x2b, 0x34, 0x01, 0x02, 0x01, 0x01, 0x0d, 0x01, 0x03, 0x01, 0x16, 0x04, 0x01, 0x03},
    {0x06, 0x0e, 0x2b, 0x34, 0x01, 0x02, 0x01, 0x01, 0x0d, 0x01, 0x03, 0x01, 0x16, 0x04, 0x01, 0x04},
};
    
    

static int mxf_read_k(FILE* mxfFile, mxfKey* key)
{
    CHK_ORET(fread((uint8_t*)key, 1, 16, mxfFile) == 16);
    
    return 1;
}

static int mxf_read_l(FILE* mxfFile, uint8_t* llen, uint64_t* len)
{
    int i;
    int c;
    uint64_t length;
    uint8_t llength;
    
    CHK_ORET((c = fgetc(mxfFile)) != EOF); 

    length = 0;
    llength = 1;
    if (c < 0x80) 
    {
        length = c;
    }
    else 
    {
        int bytesToRead = c & 0x7f;
        CHK_ORET(bytesToRead <= 8); 
        for (i = 0; i < bytesToRead; i++) 
        {
            CHK_ORET((c = fgetc(mxfFile)) != EOF); 
            length = length << 8;
            length = length | c;
        }
        llength += bytesToRead;
    }
    
    *llen = llength;
    *len = length;
    
    return 1;
}

static int mxf_read_kl(FILE* mxfFile, mxfKey* key, uint8_t* llen, uint64_t *len)
{
    CHK_ORET(mxf_read_k(mxfFile, key)); 
    CHK_ORET(mxf_read_l(mxfFile, llen, len));
    
    return 1; 
}

static int mxf_skip(FILE* mxfFile, uint64_t len)
{
    CHK_ORET(fseeko(mxfFile, len, SEEK_CUR) == 0);
    
    return 1;
}

int mxf_read_uint16(FILE* mxfFile, uint16_t* value)
{
    uint8_t buffer[2];
    CHK_ORET(fread(buffer, 1, 2, mxfFile) == 2);
    
    *value = (buffer[0]<<8) | (buffer[1]);
    
    return 1;
}

static int mxf_equals_key(const mxfKey* keyA, const mxfKey* keyB)
{
    return memcmp((const void*)keyA, (const void*)keyB, sizeof(mxfKey)) == 0;
}

static int mxf_equals_key_prefix(const mxfKey* keyA, const mxfKey* keyB, size_t cmpLen)
{
    return memcmp((const void*)keyA, (const void*)keyB, cmpLen) == 0;
}

static void convert_12m_to_timecode(unsigned char* t12m, Timecode* t)
{
    t->frame = ((t12m[0] >> 4) & 0x03) * 10 + (t12m[0] & 0xf);
    t->sec = ((t12m[1] >> 4) & 0x07) * 10 + (t12m[1] & 0xf);
    t->min = ((t12m[2] >> 4) & 0x07) * 10 + (t12m[2] & 0xf);
    t->hour = ((t12m[3] >> 4) & 0x03) * 10 + (t12m[3] & 0xf);
}

static int read_timecode(FILE* mxfFile, Timecode* vitc, Timecode* ltc)
{
    unsigned char t12m[8];
    
    /* skip the local item tag and length, and array header */
    CHK_ORET(mxf_skip(mxfFile, 12));
    
    /* read the timecode */
    CHK_ORET(fread(t12m, 1, 8, mxfFile) == 8);
    convert_12m_to_timecode(t12m, vitc);
    CHK_ORET(fread(t12m, 1, 8, mxfFile) == 8);
    convert_12m_to_timecode(t12m, ltc);

    return 1;
}

static void print_position_info(Summary* summary)
{
    printf("    count, pos A, pos B: %lld, %lld, %lld\n", summary->frameCount, summary->positionA, summary->positionB);
}

static int position_file(FILE* mxfFile, Timecode* startVITC, Timecode* startLTC, int64_t* position)
{
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    int haveStartEssence;
    int haveFoundStartTimecode;
    Timecode vitc;
    Timecode ltc;

    
    /* read and check header partition key */
    CHK_ORET(mxf_read_kl(mxfFile, &key, &llen, &len));
    CHK_ORET(mxf_equals_key_prefix(&key, &g_PartitionPackKeyPrefix, 13) && key.octet13 == 0x02);
    CHK_ORET(mxf_skip(mxfFile, len));
    
    
    /* move to start of essence */
    haveStartEssence = 0;
    while (!haveStartEssence)
    {
        CHK_ORET(mxf_read_kl(mxfFile, &key, &llen, &len));
        if (mxf_equals_key(&key, &g_SystemItemElementKey))
        {
            haveStartEssence = 1;
        }
        else
        {
            CHK_ORET(mxf_skip(mxfFile, len));
        }
    }
    CHK_ORET(haveStartEssence);

    /* seek back to before the system item key */
    CHK_ORET(fseeko(mxfFile, -(16 + llen), SEEK_CUR) == 0);

    
    /* position at given startVITC/LTC */
    
    if (startVITC->hour >= 0 || startLTC->hour >= 0)
    {
        haveFoundStartTimecode = 0;
        while (!haveFoundStartTimecode)
        {
            CHK_ORET(mxf_read_kl(mxfFile, &key, &llen, &len));
            if (mxf_equals_key(&key, &g_SystemItemElementKey))
            {
                CHK_ORET(len == 28);
                CHK_ORET(read_timecode(mxfFile, &vitc, &ltc));

                if (startVITC->hour >= 0 && memcmp(startVITC, &vitc, sizeof(Timecode)) == 0)
                {
                    haveFoundStartTimecode = 1;
                }
                else if (startLTC->hour >= 0 && memcmp(startLTC, &ltc, sizeof(Timecode)) == 0)
                {
                    haveFoundStartTimecode = 1;
                }
                else
                {
                    (*position)++;
                }
            }
            else
            {
                CHK_ORET(mxf_skip(mxfFile, len));
            }
        }
        CHK_ORET(haveFoundStartTimecode);
        
        /* seek back to before the system item */
        CHK_ORET(fseeko(mxfFile, -(16 + llen + 28), SEEK_CUR) == 0);
    }
    else
    {
        *position = 0;
    }
    
    return 1;
}

static int calc_audio_shift(int maxAudioFrameShift, FILE* mxfFileA, FILE* mxfFileB, int* audioSampleShift)
{
    unsigned char* bufferA = NULL;
    unsigned char* bufferB = NULL;
    int frameCount = 0;
    mxfKey keyA;
    uint8_t llenA;
    uint64_t lenA;
    mxfKey keyB;
    uint8_t llenB;
    uint64_t lenB;
    int64_t filePosA;
    int64_t filePosB;
    int shift;
    int firstShift;
    
    
    /* store the file positions for restoring later */
    CHK_ORET((filePosA = ftello(mxfFileA)) >= 0);
    CHK_ORET((filePosB = ftello(mxfFileB)) >= 0);
    
    /* allocate buffers */
    CHK_OFAIL((bufferA = (unsigned char*)malloc(5760)) != NULL);
    CHK_OFAIL((bufferB = (unsigned char*)malloc((maxAudioFrameShift * 2 + 1) * 5760)) != NULL);

    
    /* read in audio data from stream 1 */ 
    while (frameCount < (maxAudioFrameShift * 2 + 1))
    {
        CHK_OFAIL(mxf_read_kl(mxfFileA, &keyA, &llenA, &lenA));
        CHK_OFAIL(mxf_read_kl(mxfFileB, &keyB, &llenB, &lenB));
        CHK_OFAIL(mxf_equals_key(&keyA, &keyB));
        CHK_OFAIL(lenA == lenB);
        
        if (mxf_equals_key(&keyA, &g_AudioItemElementKey[0]))
        {
            CHK_OFAIL(lenA == 5760);
            if (frameCount == maxAudioFrameShift)
            {
                CHK_OFAIL(fread(bufferA, lenA, 1, mxfFileA) == 1);
            }
            else
            {
                CHK_OFAIL(mxf_skip(mxfFileA, lenA));
            }
            CHK_OFAIL(fread(&bufferB[frameCount * lenB], lenB, 1, mxfFileB) == 1);
            frameCount++;
        }
        else
        {
            CHK_OFAIL(mxf_skip(mxfFileA, lenA));
            CHK_OFAIL(mxf_skip(mxfFileB, lenB));
        }
    }
    
    /* compare with shifts */
    firstShift = -1;
    for (shift = -(maxAudioFrameShift * 1920); shift < maxAudioFrameShift * 1920; shift++)
    {
        if (memcmp(&bufferB[(shift + maxAudioFrameShift * 1920) * 3], bufferA, 1920 * 3) == 0)
        {
            printf("Audio equal for shift of %d samples\n", shift);
            if (firstShift < 0)
            {
                firstShift = shift;
            }
        }
    }

    
    /* restore the file positions */
    CHK_OFAIL(fseeko(mxfFileA, filePosA, SEEK_SET) == 0);
    CHK_OFAIL(fseeko(mxfFileB, filePosB, SEEK_SET) == 0);
    
    SAFE_FREE(&bufferA);
    SAFE_FREE(&bufferB);
    
    *audioSampleShift = firstShift;
    return 1;
    
fail:
    fseeko(mxfFileA, filePosA, SEEK_SET);
    fseeko(mxfFileB, filePosB, SEEK_SET);
    SAFE_FREE(&bufferA);
    SAFE_FREE(&bufferB);
    return 0;
}

static int diff_timecode(Summary* summary, int quiet, FILE* mxfFileA, FILE* mxfFileB)
{
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    int endOfEssenceA = 0;
    int endOfEssenceB = 0;
    Timecode vitcA;
    Timecode vitcB;
    Timecode ltcA;
    Timecode ltcB;
    
    /* check element keys, and also check for end of essence */
    CHK_ORET(mxf_read_kl(mxfFileA, &key, &llen, &len));
    if (!mxf_equals_key(&key, &g_SystemItemElementKey))
    {
        CHK_ORET(mxf_equals_key_prefix(&key, &g_PartitionPackKeyPrefix, 13) && key.octet13 == 0x04);
        endOfEssenceA = 1;
    }
    else
    {
        CHK_ORET(len == 28);
    }
    CHK_ORET(mxf_read_kl(mxfFileB, &key, &llen, &len));
    if (!mxf_equals_key(&key, &g_SystemItemElementKey))
    {
        CHK_ORET(mxf_equals_key_prefix(&key, &g_PartitionPackKeyPrefix, 13) && key.octet13 == 0x04);
        endOfEssenceB = 1;
    }
    else
    {
        CHK_ORET(len == 28);
    }
    
    if (endOfEssenceA || endOfEssenceB)
    {
        if (endOfEssenceA)
        {
            printf("Reached end of essence for file A\n");
        }
        else
        {
            printf("Not reached end of essence for file A\n");
        }
        if (endOfEssenceB)
        {
            printf("Reached end of essence for file B\n");
        }
        else
        {
            printf("Not reached end of essence for file B\n");
        }
        return 0;
    }

    /* read into buffer */
    CHK_ORET(read_timecode(mxfFileA, &vitcA, &ltcA));
    CHK_ORET(read_timecode(mxfFileB, &vitcB, &ltcB));
    
    /* compare */
    if (memcmp(&vitcA, &vitcB, sizeof(Timecode)) != 0)
    {
        if (!quiet)
        {
            printf("VITC differs\n");
            print_position_info(summary);
            printf("    VITC-A: %02d:%02d:%02d:%02d\n", vitcA.hour, vitcA.min, vitcA.sec, vitcA.frame);
            printf("    VITC-B: %02d:%02d:%02d:%02d\n", vitcA.hour, vitcA.min, vitcA.sec, vitcA.frame);
        }
        summary->vitcDiffCount++;
    }
    if (memcmp(&ltcA, &ltcB, sizeof(Timecode)) != 0)
    {
        if (!quiet)
        {
            printf("LTC differs\n");
            print_position_info(summary);
            printf("    LTC-A:  %02d:%02d:%02d:%02d\n", ltcA.hour, ltcA.min, ltcA.sec, ltcA.frame);
            printf("    LTC-B:  %02d:%02d:%02d:%02d\n", ltcB.hour, ltcB.min, ltcB.sec, ltcB.frame);
        }
        summary->ltcDiffCount++;
    }
    
    return 1;
}

static int diff_video(Summary* summary, int quiet, FILE* mxfFileA, FILE* mxfFileB)
{
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    unsigned char bufferA[829440];
    unsigned char bufferB[829440];
    
    /* check element keys */
    CHK_ORET(mxf_read_kl(mxfFileA, &key, &llen, &len));
    CHK_ORET(mxf_equals_key(&key, &g_VideoItemElementKey));
    CHK_ORET(len == 829440);
    CHK_ORET(mxf_read_kl(mxfFileB, &key, &llen, &len));
    CHK_ORET(mxf_equals_key(&key, &g_VideoItemElementKey));
    CHK_ORET(len == 829440);

    /* read into buffer */
    CHK_ORET(fread(bufferA, len, 1, mxfFileA) == 1);
    CHK_ORET(fread(bufferB, len, 1, mxfFileB) == 1);
    
    /* compare */
    if (memcmp(bufferA, bufferB, len) != 0)
    {
        if (!quiet)
        {
            printf("Video differs\n");
            print_position_info(summary);
        }
        summary->videoDiffCount++;
    }
    
    return 1;
}

static int diff_audio(Summary* summary, int quiet, unsigned char* bufferA, unsigned char* bufferB, 
    int maxFrameShift, int audioSampleShift, FILE* mxfFileA, FILE* mxfFileB, int num)
{
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    int bufferAReadOffset;
    int bufferBReadOffset;
    int bufferACmpOffset;
    int bufferBCmpOffset;
    int bufferAMove;
    int bufferBMove;
    
    /* inits */
    if (audioSampleShift > 0)
    {
        bufferAReadOffset = maxFrameShift * 1920 * 3 + audioSampleShift * 3;
        bufferBReadOffset = 0;
        bufferACmpOffset = maxFrameShift * 1920 * 3;
        bufferBCmpOffset = 0;
        bufferAMove = 1;
        bufferBMove = 0;
    }
    else if (audioSampleShift < 0)
    {
        bufferAReadOffset = 0;
        bufferBReadOffset = maxFrameShift * 1920 * 3 - audioSampleShift * 3;
        bufferACmpOffset = 0;
        bufferBCmpOffset = maxFrameShift * 1920 * 3;
        bufferAMove = 0;
        bufferBMove = 1;
    }
    else
    {
        bufferAReadOffset = 0;
        bufferBReadOffset = 0;
        bufferACmpOffset = 0;
        bufferBCmpOffset = 0;
        bufferAMove = 0;
        bufferBMove = 0;
    }

    /* check element keys */
    CHK_ORET(mxf_read_kl(mxfFileA, &key, &llen, &len));
    CHK_ORET(mxf_equals_key(&key, &g_AudioItemElementKey[num]));
    CHK_ORET(len == 5760);
    CHK_ORET(mxf_read_kl(mxfFileB, &key, &llen, &len));
    CHK_ORET(mxf_equals_key(&key, &g_AudioItemElementKey[num]));
    CHK_ORET(len == 5760);

    
    /* read into buffer */
    CHK_ORET(fread(&bufferA[bufferAReadOffset], len, 1, mxfFileA) == 1);
    CHK_ORET(fread(&bufferB[bufferBReadOffset], len, 1, mxfFileB) == 1);
    
    /* compare */
    if (audioSampleShift == 0 || summary->frameCount >= (abs(audioSampleShift) + 1919) / 1920)
    {
        if (memcmp(&bufferA[bufferACmpOffset], &bufferB[bufferBCmpOffset], len) != 0)
        {
            if (!quiet)
            {
                printf("Audio %d differs\n", num + 1);
                print_position_info(summary);
            }
            summary->audioDiffCount[num]++;
        }
    }
    else
    {
        printf("Skipping audio comparison for frame %lld\n", summary->frameCount);
    }
    
    /* shift the buffers */
    if (bufferAMove > 0)
    {
        CHK_ORET(memmove(bufferA, &bufferA[1920 * 3], maxFrameShift * 1920 * 3 * 2) != NULL);
    }
    else if (bufferBMove > 0)
    {
        CHK_ORET(memmove(bufferB, &bufferB[1920 * 3], maxFrameShift * 1920 * 3 * 2) != NULL);
    }
    
    return 1;
}


void usage(const char* cmd)
{
    fprintf(stderr, "Usage: %s [OPTIONS] <filename a> <filename b>\n", cmd);
    fprintf(stderr, "\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -h, --help                   Show help\n");
    fprintf(stderr, "  -q, --quiet                  Don't report differences frame by frame\n");
    fprintf(stderr, "  --start-vitc <timecode>      Start comparing at VITC timecode\n");
    fprintf(stderr, "  --start-ltc <timecode>       Start comparing at LTC timecode\n");
    fprintf(stderr, "  --duration <count>           Compare count number of frames\n");
    fprintf(stderr, "  --max-audio-shift <num>      Check for audio shift up to given maximum number of frames\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Timecode format is 'hh:mm:ss:ff'\n");
}

int main(int argc, const char** argv)
{
    int cmdln = 1;
    Timecode startVITC = {-1, -1, -1, -1};
    Timecode startLTC = {-1, -1, -1, -1};
    int64_t duration = -1;
    const char* filenameA;
    const char* filenameB;
    int quiet = 0;
    int maxAudioFrameShift = 0;
    int audioSampleShift = 0;
    unsigned char* bufferA0;
    unsigned char* bufferA1;
    unsigned char* bufferA2;
    unsigned char* bufferA3;
    unsigned char* bufferB0;
    unsigned char* bufferB1;
    unsigned char* bufferB2;
    unsigned char* bufferB3;
    
    
    /* process command line parameters */
    
    while (cmdln < argc)
    {
        if ((strcmp(argv[cmdln], "-h") == 0) ||
            (strcmp(argv[cmdln], "--help") == 0))
        {
            usage(argv[0]);
            return 0;
        }
        else if ((strcmp(argv[cmdln], "-q") == 0) ||
            (strcmp(argv[cmdln], "--quiet") == 0))
        {
            quiet = 1;
            cmdln++;
        }
        else if (strcmp(argv[cmdln], "--start-vitc") == 0)
        {
            CHECK_ARGUMENT_PRESENT("--start-vitc");
            if (sscanf(argv[cmdln + 1], "%d:%d:%d:%d", &startVITC.hour, &startVITC.min, 
                &startVITC.sec, &startVITC.frame) != 4)
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid timecode format '%s'\n", argv[cmdln + 1]);
                return 0;
            }
            cmdln += 2;
        }
        else if (strcmp(argv[cmdln], "--start-ltc") == 0)
        {
            CHECK_ARGUMENT_PRESENT("--start-ltc");
            if (sscanf(argv[cmdln + 1], "%d:%d:%d:%d", &startLTC.hour, &startLTC.min, 
                &startLTC.sec, &startLTC.frame) != 4)
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid timecode format '%s'\n", argv[cmdln + 1]);
                return 0;
            }
            cmdln += 2;
        }
        else if (strcmp(argv[cmdln], "--duration") == 0)
        {
            CHECK_ARGUMENT_PRESENT("--duration");
            if (sscanf(argv[cmdln + 1], "%lld", &duration) != 1)
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid frame count value '%s'\n", argv[cmdln + 1]);
                return 0;
            }
            cmdln += 2;
        }
        else if (strcmp(argv[cmdln], "--audio-shift") == 0)
        {
            CHECK_ARGUMENT_PRESENT("--audio-shift");
            if (sscanf(argv[cmdln + 1], "%d", &maxAudioFrameShift) != 1)
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid audio shift  value '%s'\n", argv[cmdln + 1]);
                return 0;
            }
            cmdln += 2;
        }
        else
        {
            break;
        }
    }
    
    if (cmdln + 2 < argc - 1)
    {
        usage(argv[0]);
        fprintf(stderr, "Unknown argument %s\n", argv[cmdln]);
        return 1;
    }
    else if (cmdln >= argc)
    {
        usage(argv[0]);
        fprintf(stderr, "Missing filename a and filename b\n");
        return 1;
    }
    else if (cmdln + 1 >= argc)
    {
        usage(argv[0]);
        fprintf(stderr, "Missing filename b\n");
        return 1;
    }
    
    filenameA = argv[cmdln++];
    filenameB = argv[cmdln++];
    
    
    /* print selected test information */
    
    printf("Comparing '%s' and '%s'", filenameA, filenameB);
    if (startVITC.hour >= 0)
    {
        printf(", starting at VITC timcode %02d:%02d:%02d:%02d", startVITC.hour, startVITC.min,
            startVITC.sec, startVITC.frame);
    }
    else if (startLTC.hour >= 0)
    {
        printf(", starting at LTC timcode %02d:%02d:%02d:%02d", startLTC.hour, startLTC.min,
            startLTC.sec, startLTC.frame);
    }
    if (duration >= 0)
    {
        printf(", for duration %lld", duration);
    }
    printf("\n");
    
    
    
    /* open files */
    
    FILE* fileA;
    FILE* fileB;
    
    if ((fileA = fopen(filenameA, "rb")) == NULL)
    {
        perror("fopen");
        exit(1);
    }
    if ((fileB = fopen(filenameB, "rb")) == NULL)
    {
        perror("fopen");
        exit(1);
    }
    

    /* position files at start or at given start VITC/LTC */
    
    int64_t startPositionA = 0;
    int64_t startPositionB = 0;
    
    if (startVITC.hour >= 0)
    {
        printf("Positioning file A at VITC timecode %02d:%02d:%02d:%02d\n", startVITC.hour, startVITC.min,
            startVITC.sec, startVITC.frame);
    }
    else if (startVITC.hour >= 0)
    {
        printf("Positioning file A at LTC timecode %02d:%02d:%02d:%02d\n", startVITC.hour, startVITC.min,
            startVITC.sec, startVITC.frame);
    }
    else
    {
        printf("Positioning file A at start of essence data\n");
    }
    if (!position_file(fileA, &startVITC, &startLTC, &startPositionA))
    {
        fprintf(stderr, "Failed to position file A\n");
        exit(1);
    }
    
    if (startVITC.hour >= 0)
    {
        printf("Positioning file B at VITC timecode %02d:%02d:%02d:%02d\n", startVITC.hour, startVITC.min,
            startVITC.sec, startVITC.frame);
    }
    else if (startVITC.hour >= 0)
    {
        printf("Positioning file B at LTC timecode %02d:%02d:%02d:%02d\n", startVITC.hour, startVITC.min,
            startVITC.sec, startVITC.frame);
    }
    else
    {
        printf("Positioning file B at start of essence data\n");
    }
    if (!position_file(fileB, &startVITC, &startLTC, &startPositionB))
    {
        fprintf(stderr, "Failed to position file B\n");
        exit(1);
    }

    /* check for audio shift */
    
    if (maxAudioFrameShift > 0)
    {
        printf("Calculating audio shift\n");
        CHECK(calc_audio_shift(maxAudioFrameShift, fileA, fileB, &audioSampleShift));
        printf("Audio shift is %d samples\n", audioSampleShift);
    }
    
    /* allocate audio buffer */
    
    if (audioSampleShift > 0)
    {
        CHECK((bufferA0 = malloc((maxAudioFrameShift * 2 + 1) * 1920 * 3)) != NULL);
        CHECK((bufferA1 = malloc((maxAudioFrameShift * 2 + 1) * 1920 * 3)) != NULL);
        CHECK((bufferA2 = malloc((maxAudioFrameShift * 2 + 1) * 1920 * 3)) != NULL);
        CHECK((bufferA3 = malloc((maxAudioFrameShift * 2 + 1) * 1920 * 3)) != NULL);
        CHECK((bufferB0 = malloc(1920 * 3)) != NULL);
        CHECK((bufferB1 = malloc(1920 * 3)) != NULL);
        CHECK((bufferB2 = malloc(1920 * 3)) != NULL);
        CHECK((bufferB3 = malloc(1920 * 3)) != NULL);
    }
    else if (audioSampleShift < 0)
    {
        CHECK((bufferA0 = malloc(1920 * 3)) != NULL);
        CHECK((bufferA1 = malloc(1920 * 3)) != NULL);
        CHECK((bufferA2 = malloc(1920 * 3)) != NULL);
        CHECK((bufferA3 = malloc(1920 * 3)) != NULL);
        CHECK((bufferB0 = malloc((maxAudioFrameShift * 2 + 1) * 1920 * 3)) != NULL);
        CHECK((bufferB1 = malloc((maxAudioFrameShift * 2 + 1) * 1920 * 3)) != NULL);
        CHECK((bufferB2 = malloc((maxAudioFrameShift * 2 + 1) * 1920 * 3)) != NULL);
        CHECK((bufferB3 = malloc((maxAudioFrameShift * 2 + 1) * 1920 * 3)) != NULL);
    }
    else
    {
        CHECK((bufferA0 = malloc(1920 * 3)) != NULL);
        CHECK((bufferA1 = malloc(1920 * 3)) != NULL);
        CHECK((bufferA2 = malloc(1920 * 3)) != NULL);
        CHECK((bufferA3 = malloc(1920 * 3)) != NULL);
        CHECK((bufferB0 = malloc(1920 * 3)) != NULL);
        CHECK((bufferB1 = malloc(1920 * 3)) != NULL);
        CHECK((bufferB2 = malloc(1920 * 3)) != NULL);
        CHECK((bufferB3 = malloc(1920 * 3)) != NULL);
    }
         

    
    /* do diff */
    
    Summary summary;
    memset(&summary, 0, sizeof(Summary));
    summary.positionA = startPositionA;
    summary.positionB = startPositionB;
    
    while (1)
    {
        if (duration >= 0 && summary.frameCount >= duration)
        {
            break;
        }
        
        if (!diff_timecode(&summary, quiet, fileA, fileB))
        {
            break;
        }
        
        CHECK(diff_video(&summary, quiet, fileA, fileB));
        CHECK(diff_audio(&summary, quiet, bufferA0, bufferB0, maxAudioFrameShift, audioSampleShift, fileA, fileB, 0));
        CHECK(diff_audio(&summary, quiet, bufferA1, bufferB1, maxAudioFrameShift, audioSampleShift, fileA, fileB, 1));
        CHECK(diff_audio(&summary, quiet, bufferA2, bufferB2, maxAudioFrameShift, audioSampleShift, fileA, fileB, 2));
        CHECK(diff_audio(&summary, quiet, bufferA3, bufferB3, maxAudioFrameShift, audioSampleShift, fileA, fileB, 3));
        
        summary.frameCount++;
        summary.positionA++;
        summary.positionB++;
    }
    
    /* print result summary */
    
    fprintf(stderr, "\nResults:\n");
    fprintf(stderr, "Compared %lld frames\n", summary.frameCount);
    fprintf(stderr, "Started from position %lld in file A and position %lld in file B\n", 
        startPositionA, startPositionB);
    
    if (audioSampleShift != 0)
    {
        printf("Audio was shifted %d samples when comparing\n", audioSampleShift);
    }
    
    if (summary.vitcDiffCount > 0 ||
        summary.ltcDiffCount > 0 ||
        summary.videoDiffCount > 0 ||
        summary.audioDiffCount[0] > 0 ||
        summary.audioDiffCount[1] > 0 ||
        summary.audioDiffCount[2] > 0 ||
        summary.audioDiffCount[3] > 0)
    {
        fprintf(stderr, "The essence data differs:\n");
        fprintf(stderr, "    VITC   : %lld\n", summary.vitcDiffCount);
        fprintf(stderr, "    LTC    : %lld\n", summary.ltcDiffCount);
        fprintf(stderr, "    Video  : %lld\n", summary.videoDiffCount);
        fprintf(stderr, "    Audio 1: %lld\n", summary.audioDiffCount[0]);
        fprintf(stderr, "    Audio 2: %lld\n", summary.audioDiffCount[1]);
        fprintf(stderr, "    Audio 3: %lld\n", summary.audioDiffCount[2]);
        fprintf(stderr, "    Audio 4: %lld\n", summary.audioDiffCount[3]);
    }
    else
    {
        fprintf(stderr, "No differences found\n");
    }
    
    fclose(fileA);
    fclose(fileB);
    
    return 0;
}


