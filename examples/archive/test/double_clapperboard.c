/*
 * $Id: double_clapperboard.c,v 1.2 2008/05/07 15:22:04 philipn Exp $
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

#include "avsync_eval.h"

#ifdef __MINGW32__
#define fseeko(x,y,z)  fseeko64(x,y,z)
#define ftello(x)      ftello64(x)
#endif


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
    int64_t flashCount;
    int64_t clickCount[4];
    int64_t clickNoFlashCount;
    int64_t flashNoClickCount;
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
    
    
static unsigned char g_videoBuffer[829440];
static unsigned char g_audioBuffer[5760];



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


static int position_file(FILE* mxfFile)
{
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    int haveStartEssence;

    
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

    return 1;
}


static int skip_timecode(FILE* mxfFile)
{
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    
    CHK_ORET(mxf_read_kl(mxfFile, &key, &llen, &len));
    if (!mxf_equals_key(&key, &g_SystemItemElementKey))
    {
        return 0;
    }
    
    CHK_ORET(mxf_skip(mxfFile, len));
    
    return 1;
}

static int check_video(Summary* summary, FILE* mxfFile)
{
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    
    /* check element keys */
    CHK_ORET(mxf_read_kl(mxfFile, &key, &llen, &len));
    CHK_ORET(mxf_equals_key(&key, &g_VideoItemElementKey));
    CHK_ORET(len == 829440);

    /* read into buffer */
    CHK_ORET(fread(g_videoBuffer, len, 1, mxfFile) == 1);
    
    if (find_red_flash_uyvy(g_videoBuffer + 720 * 2 * 16, 720 * 2))
    {
        printf("%5"PRId64"  Red flash\n", summary->frameCount);
        summary->flashCount++;
        return 2;
    }
    
    return 1;
}

static int check_audio(Summary* summary, FILE* mxfFile, int num)
{
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    int click = 0;
    int offset = 0;
    
    /* check element keys */
    CHK_ORET(mxf_read_kl(mxfFile, &key, &llen, &len));
    CHK_ORET(mxf_equals_key(&key, &g_AudioItemElementKey[num]));
    CHK_ORET(len == 5760);

    
    /* read into buffer */
    CHK_ORET(fread(g_audioBuffer, len, 1, mxfFile) == 1);

    find_audio_click_mono(g_audioBuffer, 24, &click, &offset);
    
    if (click)
    {
        printf("%5"PRId64"  Click ch=%d, off=%d %.1fms\n", summary->frameCount, num, offset, offset / 1920.0 * 40);
        summary->clickCount[num]++; 
        return 2;
    }
    
    return 1;
}


void usage(const char* cmd)
{
    fprintf(stderr, "Usage: %s [OPTIONS] <filename>\n", cmd);
    fprintf(stderr, "\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -h, --help                   Show help\n");
    fprintf(stderr, "\n");
}

int main(int argc, const char** argv)
{
    int cmdln = 1;
    const char* filename;
    
    
    /* process command line parameters */
    
    while (cmdln < argc)
    {
        if ((strcmp(argv[cmdln], "-h") == 0) ||
            (strcmp(argv[cmdln], "--help") == 0))
        {
            usage(argv[0]);
            return 0;
        }
        else
        {
            break;
        }
    }
    
    if (cmdln >= argc)
    {
        usage(argv[0]);
        fprintf(stderr, "Missing filename\n");
        return 1;
    }
    
    filename = argv[cmdln++];
    
    
    /* print selected test information */
    
    printf("Double clapper board check of '%s'\n", filename);
    
    
    
    /* open files */
    
    FILE* file;
    
    if ((file = fopen(filename, "rb")) == NULL)
    {
        perror("fopen");
        exit(1);
    }
    
    CHECK(position_file(file));
    

    /* do check */
    
    Summary summary;
    memset(&summary, 0, sizeof(Summary));
    int videoResult;
    int audioResult[4];
    while (1)
    {
        if (!skip_timecode(file))
        {
            break;
        }
        
        CHECK(videoResult = check_video(&summary, file));
        CHECK(audioResult[0] = check_audio(&summary, file, 0));
        CHECK(audioResult[1] = check_audio(&summary, file, 1));
        CHECK(audioResult[2] = check_audio(&summary, file, 2));
        CHECK(audioResult[3] = check_audio(&summary, file, 3));
        
        if (videoResult == 2 && !(audioResult[0] == 2 || audioResult[1] == 2 || 
                audioResult[2] == 2 || audioResult[3] == 2))
        {
            printf("Red flash but no click\n");
            summary.flashNoClickCount++;
        }
        if ((audioResult[0] == 2 || audioResult[1] == 2 || audioResult[2] == 2 || audioResult[3] == 2) && 
            !(videoResult == 2))
        {
            printf("Click with no red flash\n");
            summary.clickNoFlashCount++;
        }
        
        fflush(stdout);
        
        summary.frameCount++;
    }
    
    /* print result summary */
    
    fprintf(stderr, "\nResults:\n");
    fprintf(stderr, "# frames = %"PRId64"\n", summary.frameCount);
    fprintf(stderr, "# red flashes = %"PRId64"\n", summary.flashCount);
    fprintf(stderr, "# clicks A1 = %"PRId64"\n", summary.clickCount[0]);
    fprintf(stderr, "# clicks A2 = %"PRId64"\n", summary.clickCount[1]);
    fprintf(stderr, "# clicks A3 = %"PRId64"\n", summary.clickCount[2]);
    fprintf(stderr, "# clicks A4 = %"PRId64"\n", summary.clickCount[3]);
    fprintf(stderr, "# flash with no click = %"PRId64"\n", summary.flashNoClickCount);
    fprintf(stderr, "# click with no flash = %"PRId64"\n", summary.clickNoFlashCount);
    
    
    fclose(file);
    
    return 0;
}


