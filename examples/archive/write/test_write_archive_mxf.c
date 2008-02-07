/*
 * $Id: test_write_archive_mxf.c,v 1.1 2007/09/11 13:24:47 stuart_hc Exp $
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

#define VIDEO_FRAME_WIDTH       720
#define VIDEO_FRAME_HEIGHT      576
#define VIDEO_FRAME_SIZE        (VIDEO_FRAME_WIDTH * VIDEO_FRAME_HEIGHT * 2)
#define AUDIO_FRAME_SIZE        5760


// Represent the colour and position of a colour bar
typedef struct {
	double			position;
	unsigned char	colour[4];
} bar_colour_t;

// Generate a video buffer containing uncompressed UYVY video representing
// the familiar colour bars test signal (or YUY2 video if specified).
static void create_colour_bars(unsigned char *video_buffer, int width, int height, int convert_to_YUY2)
{
	int				i,j,b;
	bar_colour_t	UYVY_table[] = {
				{52/720.0,	{0x80,0xEB,0x80,0xEB}},	// white
				{140/720.0,	{0x10,0xD2,0x92,0xD2}},	// yellow
				{228/720.0,	{0xA5,0xA9,0x10,0xA9}},	// cyan
				{316/720.0,	{0x35,0x90,0x22,0x90}},	// green
				{404/720.0,	{0xCA,0x6A,0xDD,0x6A}},	// magenta
				{492/720.0,	{0x5A,0x51,0xF0,0x51}},	// red
				{580/720.0,	{0xf0,0x29,0x6d,0x29}},	// blue
				{668/720.0,	{0x80,0x10,0x80,0x10}},	// black
				{720/720.0,	{0x80,0xEB,0x80,0xEB}}	// white
			};

	for (j = 0; j < height; j++)
	{
		for (i = 0; i < width; i+=2)
		{
			for (b = 0; b < 9; b++)
			{
				if ((i / ((double)width)) < UYVY_table[b].position)
				{
					if (convert_to_YUY2) {
						// YUY2 packing
						video_buffer[j*width*2 + i*2 + 1] = UYVY_table[b].colour[0];
						video_buffer[j*width*2 + i*2 + 0] = UYVY_table[b].colour[1];
						video_buffer[j*width*2 + i*2 + 3] = UYVY_table[b].colour[2];
						video_buffer[j*width*2 + i*2 + 2] = UYVY_table[b].colour[3];
					}
					else {
						// UYVY packing
						video_buffer[j*width*2 + i*2 + 0] = UYVY_table[b].colour[0];
						video_buffer[j*width*2 + i*2 + 1] = UYVY_table[b].colour[1];
						video_buffer[j*width*2 + i*2 + 2] = UYVY_table[b].colour[2];
						video_buffer[j*width*2 + i*2 + 3] = UYVY_table[b].colour[3];
					}
					break;
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
        (*pcmBufferPtr++) = 64 + ((i / 3) % 10) / 10.0f * 128.0f; 
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
    fprintf(stderr, "Usage: %s <num frames> <filename> \n", cmd);
}

int main(int argc, const char* argv[])
{
    const char* mxfFilename;
    long numFrames;
    ArchiveMXFWriter* output;
    unsigned char uncData[VIDEO_FRAME_SIZE];
    unsigned char pcmDataC1[AUDIO_FRAME_SIZE];
    unsigned char pcmData[AUDIO_FRAME_SIZE];
    long i;
    int passed;
    ArchiveTimecode vitc;    
    ArchiveTimecode ltc;    
    VTRError* vtrErrors = NULL;
    long numVTRErrors = 0;
    PSEFailure* pseFailures = NULL;
    long numPSEFailures = 0;

    if (argc != 3)
    {
        usage(argv[0]);
        return 1;
    }
    if (sscanf(argv[1], "%ld", &numFrames) != 1)
    {
        usage(argv[0]);
        return 1;
    }        
    mxfFilename = argv[2];
    
    if (!prepare_archive_mxf_file(mxfFilename, 4, 0, 1, &output))
    {
        fprintf(stderr, "Failed to prepare file\n");
        return 1;
    }

    create_colour_bars(uncData, VIDEO_FRAME_WIDTH, VIDEO_FRAME_HEIGHT, 0);
    create_tone(pcmDataC1, AUDIO_FRAME_SIZE);
    
    memset(pcmData, 0, AUDIO_FRAME_SIZE);
    //for (i = 0; i < AUDIO_FRAME_SIZE; i++)
    //{
        //if (i % (3 * 5) == 0)
        //{
            //pcmData[i] |= 0xf0;
        //}
    //}
    
    vtrErrors = (VTRError*)malloc(sizeof(VTRError) * numFrames);
    pseFailures = (PSEFailure*)malloc(sizeof(PSEFailure) * numFrames);
    memset(&vitc, 0, sizeof(ArchiveTimecode));
    memset(&ltc, 0, sizeof(ArchiveTimecode));
    vitc.hour = 10;
    ltc.hour = 10;
    passed = 1;
    for (i = 0; i < numFrames; i++)
    {
        if (!write_timecode(output, vitc, ltc))
        {
            passed = 0;
            fprintf(stderr, "Failed to write timecode\n");
            break;
        }
        if (!write_video_frame(output, uncData, VIDEO_FRAME_SIZE))
        {
            passed = 0;
            fprintf(stderr, "Failed to write video\n");
            break;
        }
        if (!write_audio_frame(output, pcmDataC1, AUDIO_FRAME_SIZE))
        {
            passed = 0;
            fprintf(stderr, "Failed to write audio\n");
            break;
        }
        if (!write_audio_frame(output, pcmData, AUDIO_FRAME_SIZE))
        {
            passed = 0;
            fprintf(stderr, "Failed to write audio\n");
            break;
        }
        if (!write_audio_frame(output, pcmData, AUDIO_FRAME_SIZE))
        {
            passed = 0;
            fprintf(stderr, "Failed to write audio\n");
            break;
        }
        if (!write_audio_frame(output, pcmData, AUDIO_FRAME_SIZE))
        {
            passed = 0;
            fprintf(stderr, "Failed to write audio\n");
            break;
        }
        
        if (i % 5 == 0)
        {
            vtrErrors[numVTRErrors].vitcTimecode = vitc;
            vtrErrors[numVTRErrors].ltcTimecode = ltc;
            vtrErrors[numVTRErrors].errorCode = 1 + numVTRErrors % 0xfe;
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
            "T|"
            "2006-01-01|"
            "PROGRAMME BACKING COPY|"
            "Bla bla bla|"
            "1732|"
            "DGN377505|"
            "DC193783|"
            "LONPROG";
        
        printf("Completing\n");
        if (!complete_archive_mxf_file(&output, d3InfaxDataString, pseFailures, numPSEFailures, vtrErrors, numVTRErrors))
        {
            fprintf(stderr, "Failed to complete writing D3 MXF file\n");
            abort_archive_mxf_file(&output);
            passed = 0;
        }
        
        if (passed)
        {
            
            /* update with LTO Infax data and filename */
		const char* ltoInfaxDataString = 
			"LTO|THE BROTHERS|5:THE PARTY|1972-04-07|"
			"|LDL9114J|S|1993-12-02|PROGRAMME (DUB OF 64045)|NO ISSUE WITHOUT REF TO ARC."
			"  SEL,TVLA MAN.OR ENQ.SERV.LIB|2875|DA 0010"
			"46|DA1046|LONPROG";
#if 0            
            const char* ltoInfaxDataString = 
                "LTO|"
                "D3 preservation programme|"
                "|"
                "2006-02-02|"
                "|"
                "LME1306H|"
                "T|"
                "|"
                "D3 PRESERVATION COPY|"
                "|"
                "1732|"
                "XYZ1234|"
                "|"
                "LONPROG";
#endif
            const char* newMXFFilename = "XYZ.mxf";

            printf("Updating\n");
            if (!update_archive_mxf_file(mxfFilename, newMXFFilename, ltoInfaxDataString, 1))
            {
                fprintf(stderr, "Failed to update file with LTO Infax data and new filename\n");
                passed = 0;
            }

#if 0            
            {            
                const char* ltoInfaxDataString = 
                    "LTO|"
                    "D3 preservation programme|"
                    "|"
                    "2006-02-02|"
                    "|"
                    "LME1306H       |" /* will result in error message */
                    "T|"
                    "|"
                    "D3 PRESERVATION COPY|"
                    "|"
                    "1732|"
                    "XYZ1234|"
                    "|"
                    "LONPROG";
                const char* newMXFFilename = "ABC.mxf";
                
                printf("Updating again (NOTE: you should see an error message complaining about the prog no size)\n");
                if (!update_archive_mxf_file(mxfFilename, newMXFFilename, ltoInfaxDataString, 0))
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
    return 0;
}
