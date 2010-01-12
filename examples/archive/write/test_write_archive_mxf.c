/*
 * $Id: test_write_archive_mxf.c,v 1.7 2010/01/12 17:17:50 john_f Exp $
 *
 * 
 *
 * Copyright (C) 2006  Philip de Nier <philipn@users.sourceforge.net>
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
#include <assert.h>

#include <write_archive_mxf.h>
#include <mxf/mxf_utils.h>
#include <mxf/mxf_page_file.h>


#define VIDEO_FRAME_WIDTH           720
#define VIDEO_FRAME_HEIGHT          576
#define VIDEO_FRAME_SIZE_8BIT       (VIDEO_FRAME_WIDTH * VIDEO_FRAME_HEIGHT * 2)
#define VIDEO_FRAME_SIZE_10BIT      ((VIDEO_FRAME_WIDTH + 5) / 6 * 16 * VIDEO_FRAME_HEIGHT)
#define AUDIO_FRAME_SIZE            5760

#define MXF_PAGE_SIZE               (2 * 60 * 25 * 852628LL)


// Represent the colour and position of a colour bar
typedef struct {
    double          position;
    unsigned short  colour[4];
} bar_colour_t;

// Routine to pack 12 unsigned shorts into 16 bytes of 10-bit output
static void pack4(unsigned short *pIn, unsigned char *pOut)
{
    int     i;

    for (i = 0; i < 4; i++)
    {
        *pOut++ = *pIn & 0xff;
        *pOut = (*pIn++ >> 8) & 0x03;
        *pOut++ += (*pIn & 0x3f) << 2;
        *pOut = (*pIn++ >> 6) & 0x0f;
        *pOut++ += (*pIn & 0x0f) << 4;
        *pOut++ = (*pIn++ >> 4) & 0x3f;
    }
}

// Generate a video buffer containing uncompressed UYVY video representing
// the familiar colour bars test signal
static void create_colour_bars(unsigned char *video_buffer, int depth_8Bit, int width, int height)
{
    int i,j,b;

    if (depth_8Bit)
    {
        bar_colour_t UYVY_table[] = {
            {52/720.0,  {0x80,0xEB,0x80,0xEB}}, // white
            {140/720.0, {0x10,0xD2,0x92,0xD2}}, // yellow
            {228/720.0, {0xA5,0xA9,0x10,0xA9}}, // cyan
            {316/720.0, {0x35,0x90,0x22,0x90}}, // green
            {404/720.0, {0xCA,0x6A,0xDD,0x6A}}, // magenta
            {492/720.0, {0x5A,0x51,0xF0,0x51}}, // red
            {580/720.0, {0xf0,0x29,0x6d,0x29}}, // blue
            {668/720.0, {0x80,0x10,0x80,0x10}}, // black
            {720/720.0, {0x80,0xEB,0x80,0xEB}}  // white
        };
                
        for (j = 0; j < height; j++)
        {
            for (i = 0; i < width; i+=2)
            {
                for (b = 0; b < 9; b++)
                {
                    if ((i / ((double)width)) < UYVY_table[b].position)
                    {
                        video_buffer[j*width*2 + i*2 + 0] = UYVY_table[b].colour[0];
                        video_buffer[j*width*2 + i*2 + 1] = UYVY_table[b].colour[1];
                        video_buffer[j*width*2 + i*2 + 2] = UYVY_table[b].colour[2];
                        video_buffer[j*width*2 + i*2 + 3] = UYVY_table[b].colour[3];

                        break;
                    }
                }
            }
        }
    }
    else
    {
        bar_colour_t UYVY_table[] = {
            {52/720.0,  {0x0200,0x03ac,0x0200,0x03ac}}, // white
            {140/720.0, {0x0040,0x0348,0x0248,0x0348}}, // yellow
            {228/720.0, {0x0294,0x02a4,0x0040,0x02a4}}, // cyan
            {316/720.0, {0x00d4,0x0240,0x0088,0x0240}}, // green
            {404/720.0, {0x0328,0x01a8,0x0374,0x01a8}}, // magenta
            {492/720.0, {0x0168,0x0144,0x03c0,0x0144}}, // red
            {580/720.0, {0x03c0,0x00a4,0x01b4,0x00a4}}, // blue
            {668/720.0, {0x0200,0x0040,0x0200,0x0040}}, // black
            {720/720.0, {0x0200,0x03ac,0x0200,0x03ac}}  // white
        };
                
        unsigned short uyvy_group[12];
        int line_stride = (width + 5) / 6 * 16;
        
        for (j = 0; j < height; j++)
        {
            for (i = 0; i < width; i+=6)
            {
                for (b = 0; b < 9; b++)
                {
                    if ((i / ((double)width)) < UYVY_table[b].position)
                    {
                        memcpy(uyvy_group, UYVY_table[b].colour, sizeof(unsigned short) * 4);
                        memcpy(&uyvy_group[4], UYVY_table[b].colour, sizeof(unsigned short) * 4);
                        memcpy(&uyvy_group[8], UYVY_table[b].colour, sizeof(unsigned short) * 4);
                        
                        pack4(uyvy_group, &video_buffer[j*line_stride + i/6*16]);
                        break;
                    }
                }
            }
        }
    }
}

static void create_tone(unsigned char* pcmBuffer, int bufferSize)
{
    int i;
    unsigned char* pcmBufferPtr = pcmBuffer;
    
    for (i = 0; i < bufferSize; i += 3)
    {
        (*pcmBufferPtr++) = 0; 
        (*pcmBufferPtr++) = (unsigned char)(64 + ((i / 3) % 10) / 10.0f * 128.0f); 
        (*pcmBufferPtr++) = 0; 
    }
}

static void increment_timecode(ArchiveTimecode* timecode)
{
    timecode->frame++;
    if (timecode->frame > 24)
    {
        timecode->frame = 0;
        timecode->sec++;
        if (timecode->sec > 59)
        {
            timecode->sec = 0;
            timecode->min++;
            if (timecode->min > 59)
            {
                timecode->min = 0;
                timecode->hour++;
                if (timecode->hour > 23)
                {
                    timecode->hour = 0;
                    timecode->frame++;
                }
            }
        }
    }
}

static void usage(const char* cmd)
{
    fprintf(stderr, "Usage: %s [--num-audio <val> --10bit --16by9 --no-lto-update --crc32] <num frames> <filename> \n", cmd);
}

int main(int argc, const char* argv[])
{
    const char* mxfFilename;
    long numFrames;
    ArchiveMXFWriter* output;
    unsigned char uncData[VIDEO_FRAME_SIZE_10BIT];
    unsigned char pcmData[AUDIO_FRAME_SIZE];
    long i;
    int j;
    int passed;
    ArchiveTimecode vitc;    
    ArchiveTimecode ltc;    
    VTRError* vtrErrors = NULL;
    long numVTRErrors = 0;
    PSEFailure* pseFailures = NULL;
    long numPSEFailures = 0;
    DigiBetaDropout* digiBetaDropouts = NULL;
    long numDigiBetaDropouts = 0;
    int numAudioTracks = 4;
    int ltoUpdate = 1;
    int depth8Bit = 1;
    uint32_t videoFrameSize = VIDEO_FRAME_SIZE_8BIT;
    mxfRational aspectRatio = {4, 3};
    uint32_t crc32[17];
    int numCRC32 = 0;
    int includeCRC32 = 0;
    int cmdlnIndex = 1;
    

    while (cmdlnIndex + 2 < argc)
    {
        if (strcmp(argv[cmdlnIndex], "--num-audio") == 0)
        {
            if (cmdlnIndex + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing value for argument '%s'\n", argv[cmdlnIndex]);
                return 1;
            }
            if (sscanf(argv[cmdlnIndex + 1], "%d", &numAudioTracks) != 1 ||
                numAudioTracks > MAX_ARCHIVE_AUDIO_TRACKS || numAudioTracks < 0)
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for argument '%s'\n", argv[cmdlnIndex + 1], argv[cmdlnIndex]);
                return 1;
            }
            cmdlnIndex += 2;
        }
        else if (strcmp(argv[cmdlnIndex], "--no-lto-update") == 0)
        {
            ltoUpdate = 0;
            cmdlnIndex++;
        }
        else if (strcmp(argv[cmdlnIndex], "--10bit") == 0)
        {
            depth8Bit = 0;
            videoFrameSize = VIDEO_FRAME_SIZE_10BIT;
            cmdlnIndex++;
        }
        else if (strcmp(argv[cmdlnIndex], "--16by9") == 0)
        {
            aspectRatio.numerator = 16;
            aspectRatio.denominator = 9;
            cmdlnIndex++;
        }
        else if (strcmp(argv[cmdlnIndex], "--crc32") == 0)
        {
            includeCRC32 = 1;
            cmdlnIndex++;
        }
        else
        {
            usage(argv[0]);
            fprintf(stderr, "Unknown argument '%s'\n", argv[cmdlnIndex]);
            return 1;
        }
    }
    if (cmdlnIndex + 2 != argc)
    {
        usage(argv[0]);
        return 1;
    }
    if (sscanf(argv[cmdlnIndex], "%ld", &numFrames) != 1)
    {
        usage(argv[0]);
        return 1;
    }        
    cmdlnIndex++;
    mxfFilename = argv[cmdlnIndex];
    
    
    if (strstr(mxfFilename, "%d") != NULL)
    {
        MXFPageFile* mxfPageFile;
        MXFFile* mxfFile;
        if (!mxf_page_file_open_new(mxfFilename, MXF_PAGE_SIZE, &mxfPageFile))
        {
            fprintf(stderr, "Failed to open page mxf file\n");
            return 1;
        }
        mxfFile = mxf_page_file_get_file(mxfPageFile);
        if (!prepare_archive_mxf_file_2(&mxfFile, mxfFilename, depth8Bit, &aspectRatio,
            numAudioTracks, includeCRC32, 0, 1, &output))
        {
            fprintf(stderr, "Failed to prepare file\n");
            if (mxfFile != NULL)
            {
                mxf_file_close(&mxfFile);
            }
            return 1;
        }
    }
    else
    {
        if (!prepare_archive_mxf_file(mxfFilename, depth8Bit, &aspectRatio, numAudioTracks,
            includeCRC32, 0, 1, &output))
        {
            fprintf(stderr, "Failed to prepare file\n");
            return 1;
        }
    }

    create_colour_bars(uncData, depth8Bit, VIDEO_FRAME_WIDTH, VIDEO_FRAME_HEIGHT);
    create_tone(pcmData, AUDIO_FRAME_SIZE);
    
    if (includeCRC32)
    {
        numCRC32 = 1 + numAudioTracks;
    }
    
    vtrErrors = (VTRError*)malloc(sizeof(VTRError) * numFrames);
    pseFailures = (PSEFailure*)malloc(sizeof(PSEFailure) * numFrames);
    digiBetaDropouts = (DigiBetaDropout*)malloc(sizeof(DigiBetaDropout) * numFrames);
    memset(&vitc, 0, sizeof(ArchiveTimecode));
    memset(&ltc, 0, sizeof(ArchiveTimecode));
    vitc.hour = 10;
    ltc.hour = 10;
    passed = 1;
    for (i = 0; i < numFrames; i++)
    {
        if (includeCRC32)
        {
            crc32[0] = calc_crc32(uncData, videoFrameSize);
            for (j = 0; j < numAudioTracks; j++)
            {
                crc32[j + 1] = calc_crc32(pcmData, AUDIO_FRAME_SIZE);
            }
        }
        
        if (!write_system_item(output, vitc, ltc, crc32, numCRC32))
        {
            passed = 0;
            fprintf(stderr, "Failed to write system item\n");
            break;
        }
        if (!write_video_frame(output, uncData, videoFrameSize))
        {
            passed = 0;
            fprintf(stderr, "Failed to write video\n");
            break;
        }
        for (j = 0; j < numAudioTracks; j++)
        {
            if (!write_audio_frame(output, pcmData, AUDIO_FRAME_SIZE))
            {
                passed = 0;
                fprintf(stderr, "Failed to write audio %d\n", j);
                break;
            }
        }
        
        if (i % 5 == 0)
        {
            vtrErrors[numVTRErrors].vitcTimecode = vitc;
            vtrErrors[numVTRErrors].ltcTimecode = ltc;
            vtrErrors[numVTRErrors].errorCode = (uint8_t)(1 + numVTRErrors % 0xfe);
            if (i % 10 == 0)
            {
                /* invalidate the VITC */
                vtrErrors[numVTRErrors].vitcTimecode.hour = INVALID_TIMECODE_HOUR;
            }
            numVTRErrors++;
    
        }

        if (i % 20 == 0)
        {
            /* reset */
            memset(&vitc, 0, sizeof(ArchiveTimecode));
            vitc.hour = 10;
            memset(&ltc, 0, sizeof(ArchiveTimecode));
            ltc.hour = 10;
        }
        if (i % 5 == 0)
        {
            /* out of sync */
            increment_timecode(&vitc);
        }
        else if (i % 10 == 0)
        {
            /* back in sync */
            increment_timecode(&ltc);
        }
        else
        {
            increment_timecode(&vitc);
            increment_timecode(&ltc);
        }
        

        if (i % 3 == 0)
        {
            memset(&pseFailures[numPSEFailures], 0, sizeof(PSEFailure));
            pseFailures[numPSEFailures].position = i;
            pseFailures[numPSEFailures].luminanceFlash = 3000;
            numPSEFailures++;
        }
        else if (i % 4 == 0)
        {
            memset(&pseFailures[numPSEFailures], 0, sizeof(PSEFailure));
            pseFailures[numPSEFailures].position = i;
            pseFailures[numPSEFailures].redFlash = 3400;
            numPSEFailures++;
        }
        else if (i % 5 == 0)
        {
            memset(&pseFailures[numPSEFailures], 0, sizeof(PSEFailure));
            pseFailures[numPSEFailures].position = i;
            pseFailures[numPSEFailures].extendedFailure = 1;
            numPSEFailures++;
        }
        else if (i % 6 == 0)
        {
            memset(&pseFailures[numPSEFailures], 0, sizeof(PSEFailure));
            pseFailures[numPSEFailures].position = i;
            pseFailures[numPSEFailures].spatialPattern = 1000;
            numPSEFailures++;
        }
        
        if (i % 15 == 0)
        {
            memset(&digiBetaDropouts[numDigiBetaDropouts], 0, sizeof(DigiBetaDropout));
            digiBetaDropouts[numDigiBetaDropouts].position = i;
            digiBetaDropouts[numDigiBetaDropouts].strength = 160;
            numDigiBetaDropouts++;
        }
    }
    
    if (!passed)
    {
        fprintf(stderr, "Failed to write D3 essence to MXF file\n");
        abort_archive_mxf_file(&output);
    }
    else
    {
        const char* d3InfaxDataString = 
            "D3|"
            "D3 preservation programme|"
            "|"
            "2006-02-02|"
            "|"
            "LME1306H|"
            "71|"
            "T|"
            "2006-01-01|"
            "PROGRAMME BACKING COPY|"
            "Bla bla bla|"
            "1732|"
            "DGN377505|"
            "DC193783|"
            "LONPROG|"
            "1";
        InfaxData d3InfaxData;
        parse_infax_data(d3InfaxDataString, &d3InfaxData, 1);
        
        printf("Completing\n");
        if (!complete_archive_mxf_file(&output, &d3InfaxData,
            pseFailures, numPSEFailures,
            vtrErrors, numVTRErrors,
            digiBetaDropouts, numDigiBetaDropouts))
        {
            fprintf(stderr, "Failed to complete writing archive MXF file\n");
            abort_archive_mxf_file(&output);
            passed = 0;
        }
        
        if (passed && ltoUpdate)
        {
            
            /* update with LTO Infax data and filename */
            const char* ltoInfaxDataString = 
                "LTO|"
                "D3 preservation programme|"
                "|"
                "2006-02-02|"
                "|"
                "LME1306H|"
                "71|"
                "M|"
                "|"
                "PROGRAMME (DUB OF DGN377505)|"
                "Bla bla bla|"
                "1732|"
                "LTA000001|"
                "|"
                "LONPROG|"
                "1";
            const char* newMXFFilename = "XYZ.mxf";

            InfaxData ltoInfaxData;
            parse_infax_data(ltoInfaxDataString, &ltoInfaxData, 1);

            printf("Updating\n");
            if (strstr(mxfFilename, "%d") != NULL)
            {
                MXFPageFile* mxfPageFile;
                MXFFile* mxfFile = NULL;
                if (!mxf_page_file_open_modify(mxfFilename, MXF_PAGE_SIZE, &mxfPageFile))
                {
                    fprintf(stderr, "Failed to open page mxf file\n");
                    return 1;
                }
                mxfFile = mxf_page_file_get_file(mxfPageFile);
                if (!update_archive_mxf_file_2(&mxfFile, newMXFFilename, &ltoInfaxData))
                {
                    fprintf(stderr, "Failed to update file with LTO Infax data and new filename\n");
                    passed = 0;
                    if (mxfFile != NULL)
                    {
                        mxf_file_close(&mxfFile);
                    }
                }
            }
            else
            {
                if (!update_archive_mxf_file(mxfFilename, newMXFFilename, &ltoInfaxData))
                {
                    fprintf(stderr, "Failed to update file with LTO Infax data and new filename\n");
                    passed = 0;
                }
            }

#if 1            
            {            
                const char* ltoInfaxDataString = 
                    "LTO|"
                    "D3 preservation programme|"
                    "|"
                    "2006-02-02|"
                    "|"
                    "LME1306H       |" /* will result in error message */
                    "71|"
                    "T|"
                    "|"
                    "D3 PRESERVATION COPY|"
                    "|"
                    "1732|"
                    "XYZ1234|"
                    "|"
                    "LONPROG|"
                    "1";
                const char* newMXFFilename = "ABC.mxf";
                
                InfaxData ltoInfaxData;
                parse_infax_data(ltoInfaxDataString, &ltoInfaxData, 1);

                printf("Updating again (NOTE: you should see an error message complaining about the prog no size)\n");
                if (!update_archive_mxf_file(mxfFilename, newMXFFilename, &ltoInfaxData))
                {
                    fprintf(stderr, "Failed to update 2nd time\n");
                    passed = 0;
                }
            }
#endif            
            
        }
    }
    
    if (vtrErrors != NULL)
    {
        free(vtrErrors);
    }
    if (pseFailures != NULL)
    {
        free(pseFailures);
    }
    return 0;
}

