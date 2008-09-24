/*
 * $Id: main.c,v 1.6 2008/09/24 09:17:08 philipn Exp $
 *
 * Test writing video and audio to MXF files supported by Avid editing software
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

#include <write_avid_mxf.h>
#include <mxf/mxf.h>
#include <mxf/mxf_avid.h>


#define MAX_INPUTS      17

/* allow the last ntsc audio frame to be x samples less than the average */
#define MAX_AUDIO_VARIATION     5



typedef struct
{
    FILE* file;
    size_t dataOffset;
    size_t dataSize;
    
    uint16_t numAudioChannels;
    mxfRational audioSamplingRate;
    uint16_t nBlockAlign;
    uint16_t audioSampleBits;

    int bytesPerSample;

    size_t totalRead;
} WAVInput;

typedef struct
{
    AvidMJPEGResolution resolution;
    
    unsigned char* buffer;
    long bufferSize;
    long position;
    long prevPosition;
    long dataSize;
    
    int endOfField;
    int field2;
    long skipCount;
    int haveLenByte1;
    int haveLenByte2;
    // states
    // 0 = search for 0xFF
    // 1 = test for 0xD8 (start of image)
    // 2 = search for 0xFF - start of marker
    // 3 = test for 0xD9 (end of image), else skip
    // 4 = skip marker segment data
    //
    // transitions
    // 0 -> 1 (data == 0xFF)
    // 1 -> 0 (data != 0xD8 && data != 0xFF)
    // 1 -> 2 (data == 0xD8)
    // 2 -> 3 (data == 0xFF)
    // 3 -> 0 (data == 0xD9)
    // 3 -> 2 (data >= 0xD0 && data <= 0xD7 || data == 0x01 || data == 0x00)
    // 3 -> 4 (else and data != 0xFF)
    int markerState;
    
} MJPEGState;

typedef struct
{
    EssenceType essenceType;
    int isVideo;
    int trackNumber;
    uint32_t materialTrackID;
    EssenceInfo essenceInfo;
    const char* filename;
    FILE* file;  
    unsigned long frameSize;
    unsigned long frameSizeSeq[5]; /* PCM for NTSC */
    unsigned long minFrameSize;
    unsigned long availFrameSize;
    int seqIndex;
    unsigned char* buffer;
    int bufferOffset;

    /* used when writing MJPEG */    
    MJPEGState mjpegState;

    int isWAVFile;
    int channelIndex; /* Note: channel 0 will own the WAVInput */
    WAVInput wavInput;
    unsigned char* channelBuffer;
    int bytesPerSample;
} Input;



 
static const unsigned char RIFF_ID[4] = {'R', 'I', 'F', 'F'};
static const unsigned char WAVE_ID[4] = {'W', 'A', 'V', 'E'};
static const unsigned char FMT_ID[4] = {'f', 'm', 't', ' '};
static const unsigned char BEXT_ID[4] = {'b', 'e', 'x', 't'};
static const unsigned char DATA_ID[4] = {'d', 'a', 't', 'a'};
static const uint16_t WAVE_FORMAT_PCM = 0x0001;


/* TODO: have a problem if start-of-frame marker not found directly after end-of-frame */
static int read_next_mjpeg_image_data(FILE* file, MJPEGState* state, 
    unsigned char** dataOut, long* dataOutSize, int* haveImage)
{
    *haveImage = 0;
    
    if (state->position >= state->dataSize)
    {
        if (state->dataSize < state->bufferSize)
        {
            /* EOF if previous read was less than capacity of buffer */
            return 0;
        }
        
        if ((state->dataSize = (long)fread(state->buffer, 1, state->bufferSize, file)) == 0)
        {
            /* EOF if nothing was read */
            return 0;
        }
        state->prevPosition = 0;
        state->position = 0;
    }
    
    /* locate start and end of image */
    while (!(*haveImage) && state->position < state->dataSize)
    {
        switch (state->markerState)
        {
            case 0:
                if (state->buffer[state->position] == 0xFF)
                {
                    state->markerState = 1;
                }
                else
                {
                    fprintf(stderr, "Error: image start is non-0xFF byte\n");
                    return 0;
                }
                break;
            case 1:
                if (state->buffer[state->position] == 0xD8) /* start of frame */
                {
                    state->markerState = 2;
                }
                else if (state->buffer[state->position] != 0xFF) /* 0xFF is fill byte */
                {
                    state->markerState = 0;
                }
                break;
            case 2:
                if (state->buffer[state->position] == 0xFF)
                {
                    state->markerState = 3;
                }
                /* else wait here */
                break;
            case 3:
                if (state->buffer[state->position] == 0xD9) /* end of field */
                {
                    state->markerState = 0;
                    state->endOfField = 1;
                }
                /* 0xD0-0xD7 and 0x01 are empty markers and 0x00 is stuffed zero */
                else if ((state->buffer[state->position] >= 0xD0 && state->buffer[state->position] <= 0xD7) ||
                        state->buffer[state->position] == 0x01 || 
                        state->buffer[state->position] == 0x00)
                {
                    state->markerState = 2;
                }
                else if (state->buffer[state->position] != 0xFF) /* 0xFF is fill byte */
                {
                    state->markerState = 4;
                    /* initialise for state 4 */
                    state->haveLenByte1 = 0;
                    state->haveLenByte2 = 0;
                    state->skipCount = 0;
                }
                break;
            case 4:
                if (!state->haveLenByte1)
                {
                    state->haveLenByte1 = 1;
                    state->skipCount = state->buffer[state->position] << 8;
                }
                else if (!state->haveLenByte2)
                {
                    state->haveLenByte2 = 1;
                    state->skipCount += state->buffer[state->position];
                    state->skipCount -= 1; /* length includes the 2 length bytes, one subtracted here and one below */
                }

                if (state->haveLenByte1 && state->haveLenByte2)
                {
                    state->skipCount--;
                    if (state->skipCount == 0)
                    {
                        state->markerState = 2;
                    }
                }
                break;
            default:
                assert(0);
                return 0;
                break;
        }
        state->position++;
        
        if (state->endOfField)
        {
            /* mjpeg151s and mjpeg101m resolutions are single field; other mjpeg resolutions are 2 fields */
            if (state->resolution == Res151s || state->resolution == Res101m || state->field2)
            {
                *haveImage = 1;
            }
            state->endOfField = 0;
            state->field2 = !state->field2;
        }
    }
    
    *dataOut = &state->buffer[state->prevPosition];
    *dataOutSize = state->position - state->prevPosition;
    state->prevPosition = state->position;
    return 1;
}



static uint32_t get_uint32_le(unsigned char* buffer)
{
    return (buffer[3]<<24) | (buffer[2]<<16) | (buffer[1]<<8) | (buffer[0]);
}

static uint16_t get_uint16_le(unsigned char* buffer)
{
    return (buffer[1]<<8) | (buffer[0]);
}


static int prepare_wave_file(const char* filename, WAVInput* input)
{
    size_t size = 0;
    int haveFormatData = 0;
    int haveWAVEData = 0;
    unsigned char buffer[512];

    
    memset(input, 0, sizeof(WAVInput));
    
    
    if ((input->file = fopen(filename, "rb")) == NULL)
    {
        fprintf(stderr, "Failed to open WAV file '%s'\n", filename);
        return 0;
    }
    
    /* 'RIFF'(4) + size (4) + 'WAVE' (4) */     
    if (fread(buffer, 1, 12, input->file) < 12)
    {
        fprintf(stderr, "Failed to read wav RIFF format specifier\n");
        return 0;
    }
    if (memcmp(buffer, RIFF_ID, 4) != 0 || memcmp(&buffer[8], WAVE_ID, 4) != 0)
    {
        fprintf(stderr, "Not a RIFF WAVE file\n");
        return 0;
    }

    /* get the fmt data */
    while (1)
    {
        /* read chunk id (4) plus chunk data size (4) */
        if (fread(buffer, 1, 8, input->file) < 8)
        {
            if (feof(input->file) != 0)
            {
                break;
            }
            fprintf(stderr, "Failed to read next wav chunk name and size\n");
            return 0;
        }
        size = get_uint32_le(&buffer[4]);

        if (memcmp(buffer, FMT_ID, 4) == 0)
        {
            /* read the common fmt data */
            
            if (fread(buffer, 1, 14, input->file) < 14)
            {
                fprintf(stderr, "Failed to read the wav format chunk (common part)\n");
                return 0;
            }
            if (get_uint16_le(buffer) != WAVE_FORMAT_PCM)
            {
                fprintf(stderr, "Unexpected wav format - expecting WAVE_FORMAT_PCM (0x0001)\n");
                return 0;
            }
            input->numAudioChannels = get_uint16_le(&buffer[2]);
            if (input->numAudioChannels == 0)
            {
                fprintf(stderr, "Number wav audio channels is zero\n");
                return 0;
            }
            input->audioSamplingRate.numerator = get_uint32_le(&buffer[4]);
            input->audioSamplingRate.denominator = 1;
            input->nBlockAlign = get_uint16_le(&buffer[12]);
            
            if (fread(buffer, 1, 2, input->file) < 2)
            {
                fprintf(stderr, "Failed to read the wav PCM sample size\n");
                return 0;
            }
            input->audioSampleBits = get_uint16_le(buffer);
            input->bytesPerSample = (input->audioSampleBits + 7) / 8; 
            if (input->numAudioChannels * input->bytesPerSample != input->nBlockAlign)
            {
                fprintf(stderr, "WARNING: Block alignment in file, %d, is incorrect. "
                    "Assuming value is %d\n", input->nBlockAlign, input->numAudioChannels * input->bytesPerSample);
                input->nBlockAlign = input->numAudioChannels * input->bytesPerSample;                     
            }
            
            
            if (fseek(input->file, (long)(size - 14 - 2), SEEK_CUR) < 0)
            {
                fprintf(stderr, "Failed to seek to end of wav chunk\n");
                return 0;
            }
            
            haveFormatData = 1;
        }
        else if (memcmp(buffer, DATA_ID, 4) == 0)
        {
            /* get the wave data offset and size */
            
            input->dataOffset = ftell(input->file);
            input->dataSize = size;
            if (fseek(input->file, (long)size, SEEK_CUR) < 0)
            {
                fprintf(stderr, "Failed to seek to end of wav chunk\n");
                return 0;
            }
            haveWAVEData = 1;
        }
        else
        {
            if (fseek(input->file, (long)size, SEEK_CUR) < 0)
            {
                fprintf(stderr, "Failed to seek to end of wav chunk\n");
                return 0;
            }
        }
    }

    /* position at wave data */
    if (fseek(input->file, (long)input->dataOffset, SEEK_SET) < 0)
    {
        fprintf(stderr, "Failed to seek to start of wav data chunk\n");
        return 0;
    }
    
    return 1;
}

static int get_wave_data(WAVInput* input, unsigned char* buffer, size_t dataSize, size_t* numRead)
{
    size_t numToRead;
    size_t actualRead;
    
    if (input->totalRead >= input->dataSize)
    {
        *numRead = 0;
        return 1;
    }
    
    if (dataSize > input->dataSize - input->totalRead)
    {
        numToRead = input->dataSize - input->totalRead;
    }
    else
    {
        numToRead = dataSize;
    }
    
    if ((actualRead = fread(buffer, 1, numToRead, input->file)) != numToRead)
    {
        fprintf(stderr, "Failed to read %"PFszt" bytes of wave data. Actual read was %"PFszt"\n", numToRead, actualRead);
        return 0;
    }
    
    *numRead = actualRead;
    input->totalRead += actualRead;
    
    return 1;
}

static void get_wave_channel(WAVInput* input, size_t dataSize, unsigned char* buffer, 
    int channelIndex, unsigned char* channelBuffer)
{
    size_t i;
    int j;
    int channelOffset = channelIndex * input->bytesPerSample;
    size_t numSamples = dataSize / input->nBlockAlign;
    
    for (i = 0; i < numSamples; i++)
    {
        for (j = 0; j < input->bytesPerSample; j++)
        {
            channelBuffer[i * input->bytesPerSample + j] = buffer[i * input->nBlockAlign + channelOffset + j];
        }
    }
}


static void get_filename(const char* filenamePrefix, int isVideo, int typeTrackNum, char* filename)
{
    char suffix[16];
    
    if (isVideo)
    {
        sprintf(suffix, "_v%d.mxf", typeTrackNum);
    }
    else
    {
        sprintf(suffix, "_a%d.mxf", typeTrackNum);
    }
    
    strcpy(filename, filenamePrefix);
    strcat(filename, suffix);
}
 
static void get_track_name(int isVideo, int typeTrackNum, char* trackName)
{
    if (isVideo)
    {
        sprintf(trackName, "V%d", typeTrackNum);
    }
    else
    {
        sprintf(trackName, "A%d", typeTrackNum);
    }
}
 

static int parse_timestamp(const char* timestampStr, mxfTimestamp* ts)
{
    int data[7];
    int result;
    
    result = sscanf(timestampStr, "%u-%u-%uT%u:%u:%u:%u", &data[0], &data[1],
        &data[2], &data[3], &data[4], &data[5], &data[6]);
    if (result != 7)
    {
        return 0;
    }
    
    ts->year = data[0];
    ts->month = data[1];
    ts->day = data[2];
    ts->hour = data[3];
    ts->min = data[4];
    ts->sec = data[5];
    ts->qmsec = data[6];
    
    return 1;
}

static int parse_umid(const char* umidStr, mxfUMID* umid)
{
    int bytes[32];
    int result;
    
    result = sscanf(umidStr, 
        "%02x%02x%02x%02x%02x%02x%02x%02x"
        "%02x%02x%02x%02x%02x%02x%02x%02x"
        "%02x%02x%02x%02x%02x%02x%02x%02x"
        "%02x%02x%02x%02x%02x%02x%02x%02x",
        &bytes[0], &bytes[1], &bytes[2], &bytes[3], &bytes[4], &bytes[5], &bytes[6], &bytes[7], 
        &bytes[8], &bytes[9], &bytes[10], &bytes[11], &bytes[12], &bytes[13], &bytes[14], &bytes[15], 
        &bytes[16], &bytes[17], &bytes[18], &bytes[19], &bytes[20], &bytes[21], &bytes[22], &bytes[23], 
        &bytes[24], &bytes[25], &bytes[26], &bytes[27], &bytes[28], &bytes[29], &bytes[30], &bytes[31]);
    if (result != 32)
    {
        return 0;
    }
    
    umid->octet0 = bytes[0] & 0xff;
    umid->octet1 = bytes[1] & 0xff;
    umid->octet2 = bytes[2] & 0xff;
    umid->octet3 = bytes[3] & 0xff;
    umid->octet4 = bytes[4] & 0xff;
    umid->octet5 = bytes[5] & 0xff;
    umid->octet6 = bytes[6] & 0xff;
    umid->octet7 = bytes[7] & 0xff;
    umid->octet8 = bytes[8] & 0xff;
    umid->octet9 = bytes[9] & 0xff;
    umid->octet10 = bytes[10] & 0xff;
    umid->octet11 = bytes[11] & 0xff;
    umid->octet12 = bytes[12] & 0xff;
    umid->octet13 = bytes[13] & 0xff;
    umid->octet14 = bytes[14] & 0xff;
    umid->octet15 = bytes[15] & 0xff;
    umid->octet16 = bytes[16] & 0xff;
    umid->octet17 = bytes[17] & 0xff;
    umid->octet18 = bytes[18] & 0xff;
    umid->octet19 = bytes[19] & 0xff;
    umid->octet20 = bytes[20] & 0xff;
    umid->octet21 = bytes[21] & 0xff;
    umid->octet22 = bytes[22] & 0xff;
    umid->octet23 = bytes[23] & 0xff;
    umid->octet24 = bytes[24] & 0xff;
    umid->octet25 = bytes[25] & 0xff;
    umid->octet26 = bytes[26] & 0xff;
    umid->octet27 = bytes[27] & 0xff;
    umid->octet28 = bytes[28] & 0xff;
    umid->octet29 = bytes[29] & 0xff;
    umid->octet30 = bytes[30] & 0xff;
    umid->octet31 = bytes[31] & 0xff;
    
    return 1;
}

static void usage(const char* cmd)
{
    fprintf(stderr, "Usage: %s <<options>> <<inputs>>\n", cmd);
    fprintf(stderr, "\n");
    fprintf(stderr, "Options: (options marked with * are required)\n");
    fprintf(stderr, "  -h, --help                 display this usage message\n");
    fprintf(stderr, "* --prefix <filename>        output filename prefix\n");
    fprintf(stderr, "  --clip <name>              clip (MaterialPackage) name.\n");
    fprintf(stderr, "  --project <name>           Avid project name.\n");
    fprintf(stderr, "  --tape <name>              tape name.\n");
    fprintf(stderr, "  --ntsc                     NTSC. Default is PAL\n");
    fprintf(stderr, "  --nolegacy                 don't use the legacy definitions\n");
    fprintf(stderr, "  --aspect <ratio>           video aspect ratio x:y. Default is 4:3\n");
    fprintf(stderr, "  --comment <string>         add 'Comments' user comment to material package\n");
    fprintf(stderr, "  --desc <string>            add 'Descript' user comment to material package\n");
    fprintf(stderr, "  --start-tc <count>         set the start timecode to given count of video frames\n");
    fprintf(stderr, "  --mp-uid <umid>            set the Material Package UMID. Default is autogen\n");
    fprintf(stderr, "  --mp-created <timestamp>   set the Material Package creation date. Default is now\n");
    fprintf(stderr, "  --tp-uid <umid>            set the tape Source Package UMID. Default is autogen\n");
    fprintf(stderr, "  --tp-created <timestamp>   set the tape Source Package creation date. Default is now\n");
    fprintf(stderr, "Inputs:\n");
    fprintf(stderr, "  --mjpeg <filename>         Avid MJPEG\n");
    fprintf(stderr, "       --res <resolution>    Resolution '2:1' (default), '3:1', '10:1', '10:1m', '15:1s' or '20:1'\n");
    fprintf(stderr, "  --dv25 <filename>          DV-based 25 Mbps\n");
    fprintf(stderr, "  --dv50 <filename>          DV-based 50 Mbps\n");
    fprintf(stderr, "  --dv1080i50 <filename>     DV 100 Mbps 1080i50 (SMPTE 370M)\n");
    fprintf(stderr, "  --dv720p50 <filename>      DV 100 Mbps 720p50 (not specified in SMPTE 370M)\n");
    fprintf(stderr, "  --IMX30 <filename>         IMX 30 Mbps MPEG-2 video (D-10, SMPTE 356M)\n");
    fprintf(stderr, "  --IMX40 <filename>         IMX 40 Mbps MPEG-2 video (D-10, SMPTE 356M)\n");
    fprintf(stderr, "  --IMX50 <filename>         IMX 50 Mbps MPEG-2 video (D-10, SMPTE 356M)\n");
    fprintf(stderr, "  --DNxHD720p120 <filename>  DNxHD 1280x720p50 120 Mbps\n");
    fprintf(stderr, "  --DNxHD720p185 <filename>  DNxHD 1280x720p50 185 Mbps\n");
    fprintf(stderr, "  --DNxHD1080i120 <filename> DNxHD 1920x1080i50 120 Mbps\n");
    fprintf(stderr, "  --DNxHD1080i185 <filename> DNxHD 1920x1080i50 185 Mbps\n");
    fprintf(stderr, "  --DNxHD1080p36 <filename>  DNxHD 1920x1080p25 36 Mbps\n");
    fprintf(stderr, "  --DNxHD1080p120 <filename> DNxHD 1920x1080p25 120 Mbps\n");
    fprintf(stderr, "  --DNxHD1080p185 <filename> DNxHD 1920x1080p25 185 Mbps\n");
    fprintf(stderr, "  --unc <filename>           Uncompressed 8-bit UYVY SD\n");
    fprintf(stderr, "  --unc1080i <filename>      Uncompressed 8-bit UYVY HD 1920x1080i\n");
    fprintf(stderr, "  --pcm <filename>           raw 48kHz PCM audio\n");
    fprintf(stderr, "       --bps <bits per sample>    # bits per sample. Default is 16\n");
    fprintf(stderr, "  --wavpcm <filename>        raw 48kHz PCM audio contained in a WAV file\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "<umid> format is [0-9a-fA-F]{64}, a sequence of 32 hexadecimal bytes\n");
    fprintf(stderr, "<timestamp> format is YYYY-MM-DDThh:mm:ss:qm where qm is in units of 1/250th second\n");
}


int main(int argc, const char* argv[])
{
    AvidClipWriter* clipWriter = NULL;
    const char* filenamePrefix = NULL;
    const char* projectName = NULL;
    const char* clipName = NULL;
    const char* tapeName = NULL;
    int isPAL = 1;
    Input inputs[MAX_INPUTS];
    int inputIndex = 0;
    int cmdlnIndex = 1;
    mxfRational imageAspectRatio = {4, 3};
    mxfRational videoSampleRate = {25, 1};
    mxfRational audioSampleRate = {48000, 1};
    int numAudioTracks = 0;
    int i;
    char filename[FILENAME_MAX];
    int audioTrackNumber = 0;
    int videoTrackNumber = 0;
    int done = 0;
    int useLegacy = 1;
    size_t numRead;
    uint16_t numAudioChannels;
    int haveImage;
    unsigned char* data = NULL;
    long dataSize;
    PackageDefinitions* packageDefinitions = NULL;
    mxfUMID materialPackageUID;
    mxfUMID filePackageUID;
    mxfUMID tapePackageUID;
    mxfTimestamp materialPackageCreated;
    mxfTimestamp filePackageCreated;
    mxfTimestamp tapePackageCreated;
    char trackName[4];
    const char* comment = NULL;
    const char* desc = NULL;
    int64_t videoStartPosition = 0; 
    mxfRational projectEditRate;
    int64_t tapeLen;
    
    memset(inputs, 0, sizeof(Input) * MAX_INPUTS);
    mxf_get_timestamp_now(&materialPackageCreated);
    memcpy(&filePackageCreated, &materialPackageCreated, sizeof(filePackageCreated));
    memcpy(&tapePackageCreated, &materialPackageCreated, sizeof(tapePackageCreated));
    mxf_generate_old_aafsdk_umid(&materialPackageUID);
    mxf_generate_old_aafsdk_umid(&tapePackageUID);


    while (cmdlnIndex < argc)
    {
        if (strcmp(argv[cmdlnIndex], "-h") == 0 ||
            strcmp(argv[cmdlnIndex], "--help") == 0)
        {
            usage(argv[0]);
            return 0;
        }
        else if (strcmp(argv[cmdlnIndex], "--prefix") == 0)
        {
            if (cmdlnIndex + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for %s\n", argv[cmdlnIndex]);
                return 1;
            }
            filenamePrefix = argv[cmdlnIndex + 1];
            cmdlnIndex += 2;
        }
        else if (strcmp(argv[cmdlnIndex], "--clip") == 0)
        {
            if (cmdlnIndex + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for %s\n", argv[cmdlnIndex]);
                return 1;
            }
            clipName = argv[cmdlnIndex + 1];
            cmdlnIndex += 2;
        }
        else if (strcmp(argv[cmdlnIndex], "--tape") == 0)
        {
            if (cmdlnIndex + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for %s\n", argv[cmdlnIndex]);
                return 1;
            }
            tapeName = argv[cmdlnIndex + 1];
            cmdlnIndex += 2;
        }
        else if (strcmp(argv[cmdlnIndex], "--project") == 0)
        {
            if (cmdlnIndex + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for %s\n", argv[cmdlnIndex]);
                return 1;
            }
            projectName = argv[cmdlnIndex + 1];
            cmdlnIndex += 2;
        }
        else if (strcmp(argv[cmdlnIndex], "--ntsc") == 0)
        {
            isPAL = 0;
            videoSampleRate.numerator = 30000;
            videoSampleRate.denominator = 1001;
            cmdlnIndex++;
        }
        else if (strcmp(argv[cmdlnIndex], "--nolegacy") == 0)
        {
            useLegacy = 0;
            cmdlnIndex++;
        }
        else if (strcmp(argv[cmdlnIndex], "--aspect") == 0)
        {
            int result;
            if (cmdlnIndex + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for %s\n", argv[cmdlnIndex]);
                return 1;
            }
            if ((result = sscanf(argv[cmdlnIndex + 1], "%d:%d", &imageAspectRatio.numerator, 
                    &imageAspectRatio.denominator)) != 2)
            {
                usage(argv[0]);
                fprintf(stderr, "Failed to read --aspect value '%s'\n", argv[cmdlnIndex + 1]);
                return 1;
            }
            cmdlnIndex += 2;
        }
        else if (strcmp(argv[cmdlnIndex], "--comment") == 0)
        {
            if (cmdlnIndex + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for %s\n", argv[cmdlnIndex]);
                return 1;
            }
            comment = argv[cmdlnIndex + 1];
            cmdlnIndex += 2;
        }
        else if (strcmp(argv[cmdlnIndex], "--desc") == 0)
        {
            if (cmdlnIndex + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for %s\n", argv[cmdlnIndex]);
                return 1;
            }
            desc = argv[cmdlnIndex + 1];
            cmdlnIndex += 2;
        }
        else if (strcmp(argv[cmdlnIndex], "--start-tc") == 0)
        {
            int result;
            if (cmdlnIndex + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for %s\n", argv[cmdlnIndex]);
                return 1;
            }
            if ((result = sscanf(argv[cmdlnIndex + 1], "%"PFi64"", &videoStartPosition)) != 1 ||
                videoStartPosition < 0)
            {
                usage(argv[0]);
                fprintf(stderr, "Failed to accept --start-tc integer value '%s'\n", argv[cmdlnIndex + 1]);
                return 1;
            }
            cmdlnIndex += 2;
        }
        else if (strcmp(argv[cmdlnIndex], "--mp-uid") == 0)
        {
            if (cmdlnIndex + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for %s\n", argv[cmdlnIndex]);
                return 1;
            }
            if (!parse_umid(argv[cmdlnIndex + 1], &materialPackageUID))
            {
                usage(argv[0]);
                fprintf(stderr, "Failed to accept --mp-uid umid value '%s'\n", argv[cmdlnIndex + 1]);
                return 1;
            }
            cmdlnIndex += 2;
        }
        else if (strcmp(argv[cmdlnIndex], "--mp-created") == 0)
        {
            if (cmdlnIndex + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for %s\n", argv[cmdlnIndex]);
                return 1;
            }
            if (!parse_timestamp(argv[cmdlnIndex + 1], &materialPackageCreated))
            {
                usage(argv[0]);
                fprintf(stderr, "Failed to accept --mp-created timestamp value '%s'\n", argv[cmdlnIndex + 1]);
                return 1;
            }
            cmdlnIndex += 2;
        }
        else if (strcmp(argv[cmdlnIndex], "--tp-uid") == 0)
        {
            if (cmdlnIndex + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for %s\n", argv[cmdlnIndex]);
                return 1;
            }
            if (!parse_umid(argv[cmdlnIndex + 1], &tapePackageUID))
            {
                usage(argv[0]);
                fprintf(stderr, "Failed to accept --tp-uid umid value '%s'\n", argv[cmdlnIndex + 1]);
                return 1;
            }
            cmdlnIndex += 2;
        }
        else if (strcmp(argv[cmdlnIndex], "--tp-created") == 0)
        {
            if (cmdlnIndex + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for %s\n", argv[cmdlnIndex]);
                return 1;
            }
            if (!parse_timestamp(argv[cmdlnIndex + 1], &tapePackageCreated))
            {
                usage(argv[0]);
                fprintf(stderr, "Failed to accept --tp-created timestamp value '%s'\n", argv[cmdlnIndex + 1]);
                return 1;
            }
            cmdlnIndex += 2;
        }
        else if (strcmp(argv[cmdlnIndex], "--mjpeg") == 0)
        {
            if (cmdlnIndex + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for %s\n", argv[cmdlnIndex]);
                return 1;
            }
            inputs[inputIndex].isVideo = 1;
            inputs[inputIndex].essenceType = AvidMJPEG;
            inputs[inputIndex].filename = argv[cmdlnIndex + 1];
            inputs[inputIndex].essenceInfo.avidMJPEGInfo.resolution = Res21;
            inputs[inputIndex].trackNumber = ++videoTrackNumber;
            inputIndex++;
            cmdlnIndex += 2;
        }
        else if (strcmp(argv[cmdlnIndex], "--res") == 0)
        {
            if (cmdlnIndex + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for %s\n", argv[cmdlnIndex]);
                return 1;
            }
            if (inputIndex == 0 || inputs[inputIndex - 1].essenceType != AvidMJPEG)
            {
                usage(argv[0]);
                fprintf(stderr, "The --res must follow a --mjpeg input\n");
                return 1;
            }
            if (strcmp(argv[cmdlnIndex + 1], "2:1") == 0)
            {
                inputs[inputIndex - 1].essenceInfo.avidMJPEGInfo.resolution = Res21;
            }
            else if (strcmp(argv[cmdlnIndex + 1], "3:1") == 0)
            {
                inputs[inputIndex - 1].essenceInfo.avidMJPEGInfo.resolution = Res31;
            }
            else if (strcmp(argv[cmdlnIndex + 1], "10:1") == 0)
            {
                inputs[inputIndex - 1].essenceInfo.avidMJPEGInfo.resolution = Res101;
            }
            else if (strcmp(argv[cmdlnIndex + 1], "10:1m") == 0)
            {
                inputs[inputIndex - 1].essenceInfo.avidMJPEGInfo.resolution = Res101m;
            }
            else if (strcmp(argv[cmdlnIndex + 1], "15:1s") == 0)
            {
                inputs[inputIndex - 1].essenceInfo.avidMJPEGInfo.resolution = Res151s;
            }
            else if (strcmp(argv[cmdlnIndex + 1], "20:1") == 0)
            {
                inputs[inputIndex - 1].essenceInfo.avidMJPEGInfo.resolution = Res201;
            }
            else
            {
                usage(argv[0]);
                fprintf(stderr, "Unknown Avid MJPEG resolution '%s'\n", argv[cmdlnIndex + 1]);
                return 1;
            }
            cmdlnIndex += 2;
        }
        else if (strcmp(argv[cmdlnIndex], "--dv25") == 0)
        {
            if (cmdlnIndex + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for %s\n", argv[cmdlnIndex]);
                return 1;
            }
            inputs[inputIndex].isVideo = 1;
            inputs[inputIndex].essenceType = DVBased25;
            inputs[inputIndex].filename = argv[cmdlnIndex + 1];
            inputs[inputIndex].trackNumber = ++videoTrackNumber;
            inputIndex++;
            cmdlnIndex += 2;
        }
        else if (strcmp(argv[cmdlnIndex], "--dv50") == 0)
        {
            if (cmdlnIndex + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for %s\n", argv[cmdlnIndex]);
                return 1;
            }
            inputs[inputIndex].isVideo = 1;
            inputs[inputIndex].essenceType = DVBased50;
            inputs[inputIndex].filename = argv[cmdlnIndex + 1];
            inputs[inputIndex].trackNumber = ++videoTrackNumber;
            inputIndex++;
            cmdlnIndex += 2;
        }
        else if (strcmp(argv[cmdlnIndex], "--dv1080i50") == 0)
        {
            if (cmdlnIndex + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for %s\n", argv[cmdlnIndex]);
                return 1;
            }
            inputs[inputIndex].isVideo = 1;
            inputs[inputIndex].essenceType = DV1080i50;
            inputs[inputIndex].filename = argv[cmdlnIndex + 1];
            inputs[inputIndex].trackNumber = ++videoTrackNumber;
            inputIndex++;
            cmdlnIndex += 2;
        }
        else if (strcmp(argv[cmdlnIndex], "--dv720p50") == 0)
        {
            if (cmdlnIndex + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for %s\n", argv[cmdlnIndex]);
                return 1;
            }
            inputs[inputIndex].isVideo = 1;
            inputs[inputIndex].essenceType = DV720p50;
            inputs[inputIndex].filename = argv[cmdlnIndex + 1];
            inputs[inputIndex].trackNumber = ++videoTrackNumber;
            inputIndex++;
            cmdlnIndex += 2;
        }
        else if (strcmp(argv[cmdlnIndex], "--IMX30") == 0 ||
                 strcmp(argv[cmdlnIndex], "--IMX40") == 0 ||
                 strcmp(argv[cmdlnIndex], "--IMX50") == 0)
        {
            if (cmdlnIndex + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for %s\n", argv[cmdlnIndex]);
                return 1;
            }
            inputs[inputIndex].isVideo = 1;
            inputs[inputIndex].essenceType = IMX30;
            if (argv[cmdlnIndex][5] == '4')
                inputs[inputIndex].essenceType = IMX40;
            if (argv[cmdlnIndex][5] == '5')
                inputs[inputIndex].essenceType = IMX50;
            inputs[inputIndex].filename = argv[cmdlnIndex + 1];
            inputs[inputIndex].filename = argv[cmdlnIndex + 1];
            inputs[inputIndex].filename = argv[cmdlnIndex + 1];
            inputs[inputIndex].trackNumber = ++videoTrackNumber;
            inputIndex++;
            cmdlnIndex += 2;
        }
        else if (strcmp(argv[cmdlnIndex], "--DNxHD720p120") == 0)
        {
            if (cmdlnIndex + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for %s\n", argv[cmdlnIndex]);
                return 1;
            }
            imageAspectRatio.numerator = 16;
            imageAspectRatio.denominator = 9;
            inputs[inputIndex].isVideo = 1;
            inputs[inputIndex].essenceType = DNxHD720p120;
            inputs[inputIndex].filename = argv[cmdlnIndex + 1];
            inputs[inputIndex].trackNumber = ++videoTrackNumber;
            inputIndex++;
            cmdlnIndex += 2;
        }
        else if (strcmp(argv[cmdlnIndex], "--DNxHD720p185") == 0)
        {
            if (cmdlnIndex + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for %s\n", argv[cmdlnIndex]);
                return 1;
            }
            imageAspectRatio.numerator = 16;
            imageAspectRatio.denominator = 9;
            inputs[inputIndex].isVideo = 1;
            inputs[inputIndex].essenceType = DNxHD720p185;
            inputs[inputIndex].filename = argv[cmdlnIndex + 1];
            inputs[inputIndex].trackNumber = ++videoTrackNumber;
            inputIndex++;
            cmdlnIndex += 2;
        }
        else if (strcmp(argv[cmdlnIndex], "--DNxHD1080i120") == 0)
        {
            if (cmdlnIndex + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for %s\n", argv[cmdlnIndex]);
                return 1;
            }
            imageAspectRatio.numerator = 16;
            imageAspectRatio.denominator = 9;
            inputs[inputIndex].isVideo = 1;
            inputs[inputIndex].essenceType = DNxHD1080i120;
            inputs[inputIndex].filename = argv[cmdlnIndex + 1];
            inputs[inputIndex].trackNumber = ++videoTrackNumber;
            inputIndex++;
            cmdlnIndex += 2;
        }
        else if (strcmp(argv[cmdlnIndex], "--DNxHD1080i185") == 0)
        {
            if (cmdlnIndex + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for %s\n", argv[cmdlnIndex]);
                return 1;
            }
            imageAspectRatio.numerator = 16;
            imageAspectRatio.denominator = 9;
            inputs[inputIndex].isVideo = 1;
            inputs[inputIndex].essenceType = DNxHD1080i185;
            inputs[inputIndex].filename = argv[cmdlnIndex + 1];
            inputs[inputIndex].trackNumber = ++videoTrackNumber;
            inputIndex++;
            cmdlnIndex += 2;
        }
        else if (strcmp(argv[cmdlnIndex], "--DNxHD1080p36") == 0)
        {
            if (cmdlnIndex + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for %s\n", argv[cmdlnIndex]);
                return 1;
            }
            imageAspectRatio.numerator = 16;
            imageAspectRatio.denominator = 9;
            inputs[inputIndex].isVideo = 1;
            inputs[inputIndex].essenceType = DNxHD1080p36;
            inputs[inputIndex].filename = argv[cmdlnIndex + 1];
            inputs[inputIndex].trackNumber = ++videoTrackNumber;
            inputIndex++;
            cmdlnIndex += 2;
        }
        else if (strcmp(argv[cmdlnIndex], "--DNxHD1080p120") == 0)
        {
            if (cmdlnIndex + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for %s\n", argv[cmdlnIndex]);
                return 1;
            }
            imageAspectRatio.numerator = 16;
            imageAspectRatio.denominator = 9;
            inputs[inputIndex].isVideo = 1;
            inputs[inputIndex].essenceType = DNxHD1080p120;
            inputs[inputIndex].filename = argv[cmdlnIndex + 1];
            inputs[inputIndex].trackNumber = ++videoTrackNumber;
            inputIndex++;
            cmdlnIndex += 2;
        }
        else if (strcmp(argv[cmdlnIndex], "--DNxHD1080p185") == 0)
        {
            if (cmdlnIndex + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for %s\n", argv[cmdlnIndex]);
                return 1;
            }
            imageAspectRatio.numerator = 16;
            imageAspectRatio.denominator = 9;
            inputs[inputIndex].isVideo = 1;
            inputs[inputIndex].essenceType = DNxHD1080p185;
            inputs[inputIndex].filename = argv[cmdlnIndex + 1];
            inputs[inputIndex].trackNumber = ++videoTrackNumber;
            inputIndex++;
            cmdlnIndex += 2;
        }
        else if (strcmp(argv[cmdlnIndex], "--unc") == 0)
        {
            if (cmdlnIndex + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for %s\n", argv[cmdlnIndex]);
                return 1;
            }
            inputs[inputIndex].isVideo = 1;
            inputs[inputIndex].essenceType = UncUYVY;
            inputs[inputIndex].filename = argv[cmdlnIndex + 1];
            inputs[inputIndex].trackNumber = ++videoTrackNumber;
            inputIndex++;
            cmdlnIndex += 2;
        }
        else if (strcmp(argv[cmdlnIndex], "--unc1080i") == 0)
        {
            if (cmdlnIndex + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for %s\n", argv[cmdlnIndex]);
                return 1;
            }
            inputs[inputIndex].isVideo = 1;
            inputs[inputIndex].essenceType = Unc1080iUYVY;
            inputs[inputIndex].filename = argv[cmdlnIndex + 1];
            inputs[inputIndex].trackNumber = ++videoTrackNumber;
            inputIndex++;
            cmdlnIndex += 2;
        }
        else if (strcmp(argv[cmdlnIndex], "--pcm") == 0)
        {
            if (cmdlnIndex + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for %s\n", argv[cmdlnIndex]);
                return 1;
            }
            inputs[inputIndex].isVideo = 0;
            inputs[inputIndex].essenceType = PCM;
            inputs[inputIndex].filename = argv[cmdlnIndex + 1];
            inputs[inputIndex].essenceInfo.pcmInfo.bitsPerSample = 16;
            inputs[inputIndex].bytesPerSample = 2;
            inputs[inputIndex].trackNumber = ++audioTrackNumber;
            inputIndex++;
            numAudioTracks++;
            cmdlnIndex += 2;
        }
        else if (strcmp(argv[cmdlnIndex], "--wavpcm") == 0)
        {
            if (cmdlnIndex + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for %s\n", argv[cmdlnIndex]);
                return 1;
            }
            if (!prepare_wave_file(argv[cmdlnIndex + 1], &inputs[inputIndex].wavInput))
            {
                fprintf(stderr, "Failed to prepare Wave input file\n");
                return 1;
            }
            if (inputs[inputIndex].wavInput.audioSamplingRate.numerator != 48000)
            {
                fprintf(stderr, "Only 48kHz audio sampling rate supported\n");
                return 1;
            }
            numAudioChannels = inputs[inputIndex].wavInput.numAudioChannels;
            for (i = 0; i < numAudioChannels; i++)
            {
                inputs[inputIndex].isVideo = 0;
                inputs[inputIndex].essenceType = PCM;
                inputs[inputIndex].isWAVFile = 1;
                inputs[inputIndex].channelIndex = i;
                inputs[inputIndex].wavInput = inputs[inputIndex - i].wavInput;
                inputs[inputIndex].essenceInfo.pcmInfo.bitsPerSample = 
                    inputs[inputIndex].wavInput.audioSampleBits;
                inputs[inputIndex].bytesPerSample = inputs[inputIndex].wavInput.bytesPerSample;
                inputs[inputIndex].trackNumber = ++audioTrackNumber;
                inputIndex++;
                numAudioTracks++;
            }
            cmdlnIndex += 2;
        }
        else if (strcmp(argv[cmdlnIndex], "--bps") == 0)
        {
            int result;
            int bitsPerSample;
            if (cmdlnIndex + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for %s\n", argv[cmdlnIndex]);
                return 1;
            }
            if (inputIndex == 0 || inputs[inputIndex - 1].essenceType != PCM)
            {
                usage(argv[0]);
                fprintf(stderr, "The --bps must follow a --pcm input\n");
                return 1;
            }
            if ((result = sscanf(argv[cmdlnIndex + 1], "%d", &bitsPerSample)) != 1)
            {
                usage(argv[0]);
                fprintf(stderr, "Failed to read --bps integer value '%s'\n", argv[cmdlnIndex + 1]);
                return 1;
            }
            if (bitsPerSample < 1 || bitsPerSample > 32)
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid --bps value '%s'\n", argv[cmdlnIndex + 1]);
                return 1;
            }
                
            inputs[inputIndex - 1].essenceInfo.pcmInfo.bitsPerSample = bitsPerSample;
            cmdlnIndex += 2;
        }
        else
        {
            usage(argv[0]);
            fprintf(stderr, "Unknown argument '%s'\n", argv[cmdlnIndex]);
            return 1;
        }
    }

    /* Check for required arguments */
    if (filenamePrefix == NULL)
    {
        usage(argv[0]);
        fprintf(stderr, "--prefix is required\n");
        return 1;
    }
    
    if (inputIndex == 0)
    {
        usage(argv[0]);
        fprintf(stderr, "No inputs\n");
        return 1;
    }

    /* default the clip name to prefix if clip name not specified */
    if (clipName == NULL)
    {
        clipName = filenamePrefix;
    }

    /* set essence parameters etc. */
    for (i = 0; i < inputIndex; i++)
    {
        if (inputs[i].essenceType == AvidMJPEG)
        {
            inputs[i].frameSize = 0; /* variable frame size */
            memset(&inputs[i].mjpegState, 0, sizeof(MJPEGState));
            CHK_MALLOC_ARRAY_OFAIL(inputs[i].mjpegState.buffer, unsigned char, 8192);
            inputs[i].mjpegState.bufferSize = 8192;
            inputs[i].mjpegState.dataSize = inputs[i].mjpegState.bufferSize;
            inputs[i].mjpegState.position = inputs[i].mjpegState.bufferSize;
            inputs[i].mjpegState.prevPosition = inputs[i].mjpegState.bufferSize;
            inputs[i].mjpegState.resolution = inputs[i].essenceInfo.avidMJPEGInfo.resolution;
        }
        else if (inputs[i].essenceType == DVBased25)
        {
            if (isPAL)
            {
                inputs[i].frameSize = 144000;
            }
            else
            {
                inputs[i].frameSize = 120000;
            }
            CHK_MALLOC_ARRAY_OFAIL(inputs[i].buffer, unsigned char, inputs[i].frameSize);
        }
        else if (inputs[i].essenceType == DVBased50)
        {
            if (isPAL)
            {
                inputs[i].frameSize = 288000;
            }
            else
            {
                inputs[i].frameSize = 240000;
            }
            CHK_MALLOC_ARRAY_OFAIL(inputs[i].buffer, unsigned char, inputs[i].frameSize);
        }
        else if (inputs[i].essenceType == DV1080i50)
        {
            inputs[i].frameSize = 576000;
            CHK_MALLOC_ARRAY_OFAIL(inputs[i].buffer, unsigned char, inputs[i].frameSize);
        }
        else if (inputs[i].essenceType == DV720p50)
        {
            inputs[i].frameSize = 576000;
            CHK_MALLOC_ARRAY_OFAIL(inputs[i].buffer, unsigned char, inputs[i].frameSize);
        }
        else if (inputs[i].essenceType == IMX30)
        {
            inputs[i].frameSize = 150000;
            CHK_MALLOC_ARRAY_OFAIL(inputs[i].buffer, unsigned char, inputs[i].frameSize);
        }
        else if (inputs[i].essenceType == IMX40)
        {
            inputs[i].frameSize = 200000;
            CHK_MALLOC_ARRAY_OFAIL(inputs[i].buffer, unsigned char, inputs[i].frameSize);
        }
        else if (inputs[i].essenceType == IMX50)
        {
            inputs[i].frameSize = 250000;
            CHK_MALLOC_ARRAY_OFAIL(inputs[i].buffer, unsigned char, inputs[i].frameSize);
        }
        else if (inputs[i].essenceType == DNxHD720p120)
        {
            videoSampleRate.numerator = 50;
            videoSampleRate.denominator = 1;
            inputs[i].frameSize = 303104;
            CHK_MALLOC_ARRAY_OFAIL(inputs[i].buffer, unsigned char, inputs[i].frameSize);
        }
        else if (inputs[i].essenceType == DNxHD720p185)
        {
            videoSampleRate.numerator = 50;
            videoSampleRate.denominator = 1;
            inputs[i].frameSize = 458752;
            CHK_MALLOC_ARRAY_OFAIL(inputs[i].buffer, unsigned char, inputs[i].frameSize);
        }
        else if (inputs[i].essenceType == DNxHD1080p36)
        {
            inputs[i].frameSize = 188416;
            CHK_MALLOC_ARRAY_OFAIL(inputs[i].buffer, unsigned char, inputs[i].frameSize);
        }
        else if (inputs[i].essenceType == DNxHD1080i120 || inputs[i].essenceType == DNxHD1080p120)
        {
            inputs[i].frameSize = 606208;
            CHK_MALLOC_ARRAY_OFAIL(inputs[i].buffer, unsigned char, inputs[i].frameSize);
        }
        else if (inputs[i].essenceType == DNxHD1080i185 || inputs[i].essenceType == DNxHD1080p185)
        {
            inputs[i].frameSize = 917504;
            CHK_MALLOC_ARRAY_OFAIL(inputs[i].buffer, unsigned char, inputs[i].frameSize);
        }
        else if (inputs[i].essenceType == UncUYVY)
        {
            if (isPAL)
            {
                inputs[i].frameSize = 720 * 576 * 2;
            }
            else
            {
                fprintf(stderr, "Uncompressed NTSC not yet implemented\n");
                return 1;
            }
            CHK_MALLOC_ARRAY_OFAIL(inputs[i].buffer, unsigned char, inputs[i].frameSize);
        }
        else if (inputs[i].essenceType == Unc1080iUYVY)
        {
            if (isPAL)
            {
                inputs[i].frameSize = 1920 * 1080 * 2;
            }
            else
            {
                fprintf(stderr, "Uncompressed 1080i NTSC not yet implemented\n");
                return 1;
            }
            CHK_MALLOC_ARRAY_OFAIL(inputs[i].buffer, unsigned char, inputs[i].frameSize);
        }
        else if (inputs[i].essenceType == PCM && !inputs[i].isWAVFile)
        {
            if (isPAL)
            {
                inputs[i].frameSize = 1920 * inputs[i].bytesPerSample;
             }
            else
            {
                /* frame size is 1601.6 samples on average and a maximum of 1602 + MAX_AUDIO_VARIATION */
                inputs[i].frameSize = (1602 + MAX_AUDIO_VARIATION) * inputs[i].bytesPerSample;
                inputs[i].minFrameSize = (1602 - MAX_AUDIO_VARIATION) * inputs[i].bytesPerSample;
                inputs[i].frameSizeSeq[0] = 1602 * inputs[i].bytesPerSample;
                inputs[i].frameSizeSeq[1] = 1601 * inputs[i].bytesPerSample;
                inputs[i].frameSizeSeq[2] = 1602 * inputs[i].bytesPerSample;
                inputs[i].frameSizeSeq[3] = 1601 * inputs[i].bytesPerSample;
                inputs[i].frameSizeSeq[4] = 1602 * inputs[i].bytesPerSample;
            }
            CHK_MALLOC_ARRAY_OFAIL(inputs[i].buffer, unsigned char, inputs[i].frameSize);
        }
        else if (inputs[i].essenceType == PCM && inputs[i].isWAVFile)
        {
            if (isPAL)
            {
                inputs[i].frameSize = 1920 * inputs[i].bytesPerSample;
            }
            else
            {
                /* frame size is 1601.6 samples on average and a maximum of 1602 + MAX_AUDIO_VARIATION */
                inputs[i].frameSize = (1602 + MAX_AUDIO_VARIATION) * inputs[i].bytesPerSample;
                inputs[i].minFrameSize = (1602 - MAX_AUDIO_VARIATION) * inputs[i].bytesPerSample;
                inputs[i].frameSizeSeq[0] = 1602 * inputs[i].bytesPerSample;
                inputs[i].frameSizeSeq[1] = 1601 * inputs[i].bytesPerSample;
                inputs[i].frameSizeSeq[2] = 1602 * inputs[i].bytesPerSample;
                inputs[i].frameSizeSeq[3] = 1601 * inputs[i].bytesPerSample;
                inputs[i].frameSizeSeq[4] = 1602 * inputs[i].bytesPerSample;
            }
            if (inputs[i].channelIndex == 0)
            {
                /* allocate buffer for multi-channel audio data */
                CHK_MALLOC_ARRAY_OFAIL(inputs[i].buffer, unsigned char, inputs[i].wavInput.numAudioChannels * inputs[i].frameSize);
            }
            CHK_MALLOC_ARRAY_OFAIL(inputs[i].channelBuffer, unsigned char, inputs[i].frameSize);
        }
        else
        {
            /* unknown essence type - code fix needed */
            assert(0);
        }
    }    



    /* create the package definitions */
    
    CHK_OFAIL(create_package_definitions(&packageDefinitions));

    /* project edit rate is the video sample rate */
    projectEditRate.numerator = videoSampleRate.numerator;
    projectEditRate.denominator = videoSampleRate.denominator;

    /* set the tape length to 120 hours */
    tapeLen = (int64_t)(120 * 60 * 60 * 
        videoSampleRate.numerator / (double)videoSampleRate.denominator + 0.5);
    
    /* material package with user comments */    
    CHK_OFAIL(create_material_package(packageDefinitions, &materialPackageUID, clipName, &materialPackageCreated));
    if (comment != NULL)
    {
        CHK_OFAIL(set_user_comment(packageDefinitions, "Comments", comment));
    }
    if (desc != NULL)
    {
        CHK_OFAIL(set_user_comment(packageDefinitions, "Descript", desc));
    }

    /* tape source package */
    CHK_OFAIL(create_tape_source_package(packageDefinitions, &tapePackageUID, tapeName, &tapePackageCreated));

    /* create a file source package for each input and add a track to each package */
    for (i = 0; i < inputIndex; i++)
    {
        Package* filePackage;
        Track* tapeTrack;
        Track* fileTrack;
        Track* materialTrack;
        mxfRational editRate;
        int64_t startPosition;
        
        if (inputs[i].isVideo)
        {
            startPosition = videoStartPosition;
            
            editRate.numerator = videoSampleRate.numerator;
            editRate.denominator = videoSampleRate.denominator;
        }
        else
        {
            startPosition = (int64_t)(videoStartPosition * 
                (audioSampleRate.numerator * videoSampleRate.denominator) / 
                    (double)(audioSampleRate.denominator * videoSampleRate.numerator) + 0.5);

            editRate.numerator = audioSampleRate.numerator;
            editRate.denominator = audioSampleRate.denominator;
        }
        
        get_filename(filenamePrefix, inputs[i].isVideo, inputs[i].trackNumber, filename);
        get_track_name(inputs[i].isVideo, inputs[i].trackNumber, trackName);

        
        /* create file package */
        mxf_generate_old_aafsdk_umid(&filePackageUID);
        CHK_OFAIL(create_file_source_package(packageDefinitions, &filePackageUID, NULL, &filePackageCreated,
            filename, inputs[i].essenceType, &inputs[i].essenceInfo, &filePackage));
            
            
        /* track in tape source package */
        CHK_OFAIL(create_track(packageDefinitions->tapeSourcePackage, i + 1, 0, trackName, inputs[i].isVideo,
            &projectEditRate, &g_Null_UMID, 0, 0, tapeLen, &tapeTrack));
            
        /* track in file source package */
        CHK_OFAIL(create_track(filePackage, 1, 0, trackName, inputs[i].isVideo,
            &editRate, &packageDefinitions->tapeSourcePackage->uid, tapeTrack->id, startPosition, 0, 
            &fileTrack));
            
        /* track in material package */
        CHK_OFAIL(create_track(packageDefinitions->materialPackage, i + 1, inputs[i].trackNumber, trackName, 
            inputs[i].isVideo, &editRate, &filePackage->uid, fileTrack->id, 0, fileTrack->length, 
            &materialTrack));
            
            
        /* the material track ID is the unique ID for the essence data from this input */
        inputs[i].materialTrackID = materialTrack->id;
    }
    
    
    
    /* open the input files */
    
    for (i = 0; i < inputIndex; i++)
    {
        if (!inputs[i].isWAVFile) /* WAVE file already open */
        {
            if ((inputs[i].file = fopen(inputs[i].filename, "rb")) == NULL)
            {
                fprintf(stderr, "Failed to open file '%s'\n", inputs[i].filename);
                goto fail;
            }
        }
    }   

    
    
    /* create the clip writer */
    
    if (!create_clip_writer(projectName, isPAL ? PAL_25i : NTSC_30i,
        imageAspectRatio, videoSampleRate, 0, useLegacy, packageDefinitions, &clipWriter))
    {
        fprintf(stderr, "Failed to create Avid MXF clip writer\n");
        goto fail;
    }
    
    
    
    /* write the essence data */
    
    done = 0;
    while (!done)
    {
        for (i = 0; i < inputIndex; i++)
        {
            if (inputs[i].essenceType == AvidMJPEG)
            {
                if (!start_write_samples(clipWriter, inputs[i].materialTrackID))
                {
                    fprintf(stderr, "Failed to start writing MJPEG frame\n");
                    goto fail;
                }
                haveImage = 0;
                while (!haveImage)
                {
                    if (!read_next_mjpeg_image_data(inputs[i].file, &inputs[i].mjpegState, 
                        &data, &dataSize, &haveImage))
                    {
                        done = 1;
                        break;
                    }
                    if (!write_sample_data(clipWriter, inputs[i].materialTrackID, 
                        data, dataSize))
                    {
                        fprintf(stderr, "Failed to write MJPEG frame data\n");
                        goto fail;
                    }
                }
                if (done)
                {
                    break;
                }
                if (!end_write_samples(clipWriter, inputs[i].materialTrackID, 1))
                {
                    fprintf(stderr, "Failed to end writing MJPEG frame\n");
                    goto fail;
                }
            }
            else if (inputs[i].essenceType == DVBased25)
            {
                if (fread(inputs[i].buffer, 1, inputs[i].frameSize, inputs[i].file) != inputs[i].frameSize)
                {
                    done = 1;
                    break;
                }
                if (!write_samples(clipWriter, inputs[i].materialTrackID, 1, inputs[i].buffer, inputs[i].frameSize))
                {
                    fprintf(stderr, "Failed to write DVBased25 frame\n");
                    goto fail;
                }
            }
            else if (inputs[i].essenceType == DVBased50)
            {
                if (fread(inputs[i].buffer, 1, inputs[i].frameSize, inputs[i].file) != inputs[i].frameSize)
                {
                    done = 1;
                    break;
                }
                if (!write_samples(clipWriter, inputs[i].materialTrackID, 1, inputs[i].buffer, inputs[i].frameSize))
                {
                    fprintf(stderr, "Failed to write DVBased50 frame\n");
                    goto fail;
                }
            }
            else if (inputs[i].essenceType == DV1080i50 || inputs[i].essenceType == DV720p50)
            {
                if (fread(inputs[i].buffer, 1, inputs[i].frameSize, inputs[i].file) != inputs[i].frameSize)
                {
                    done = 1;
                    break;
                }
                if (!write_samples(clipWriter, inputs[i].materialTrackID, 1, inputs[i].buffer, inputs[i].frameSize))
                {
                    fprintf(stderr, "Failed to write DV100 frame\n");
                    goto fail;
                }
            }
            else if (inputs[i].essenceType == IMX30 ||
                     inputs[i].essenceType == IMX40 ||
                     inputs[i].essenceType == IMX50)
            {
                if (fread(inputs[i].buffer, 1, inputs[i].frameSize, inputs[i].file) != inputs[i].frameSize)
                {
                    done = 1;
                    break;
                }
                if (!write_samples(clipWriter, inputs[i].materialTrackID, 1, inputs[i].buffer, inputs[i].frameSize))
                {
                    fprintf(stderr, "Failed to write IMX frame\n");
                    goto fail;
                }
            }
            else if (inputs[i].essenceType == DNxHD720p120 || inputs[i].essenceType == DNxHD720p185 ||
                     inputs[i].essenceType == DNxHD1080i120 || inputs[i].essenceType == DNxHD1080i185 ||
                     inputs[i].essenceType == DNxHD1080p120 || inputs[i].essenceType == DNxHD1080p185 ||
                     inputs[i].essenceType == DNxHD1080p36)
            {
                if (fread(inputs[i].buffer, 1, inputs[i].frameSize, inputs[i].file) != inputs[i].frameSize)
                {
                    done = 1;
                    break;
                }
                if (!write_samples(clipWriter, inputs[i].materialTrackID, 1, inputs[i].buffer, inputs[i].frameSize))
                {
                    fprintf(stderr, "Failed to write DNxHD frame\n");
                    goto fail;
                }
            }
            else if (inputs[i].essenceType == UncUYVY || inputs[i].essenceType == Unc1080iUYVY)
            {
                if (fread(inputs[i].buffer, 1, inputs[i].frameSize, inputs[i].file) != inputs[i].frameSize)
                {
                    done = 1;
                    break;
                }
                if (!write_samples(clipWriter, inputs[i].materialTrackID, 1, inputs[i].buffer, inputs[i].frameSize))
                {
                    fprintf(stderr, "Failed to write Uncompressed frame\n");
                    goto fail;
                }
            }
            else if (inputs[i].essenceType == PCM && !inputs[i].isWAVFile)
            {
                if (isPAL)
                {
                    uint32_t numSamples = inputs[i].frameSize / inputs[i].bytesPerSample;
                    if (fread(inputs[i].buffer, 1, inputs[i].frameSize, inputs[i].file) != inputs[i].frameSize)
                    {
                        done = 1;
                        break;
                    }
                    if (!write_samples(clipWriter, inputs[i].materialTrackID, numSamples, inputs[i].buffer, inputs[i].frameSize))
                    {
                        fprintf(stderr, "Failed to write PCM frame\n");
                        goto fail;
                    }
                }
                else
                {
                    /* For NTSC we read frame sizes corresponding to the sequence set in
                    inputs[i].frameSizeSeq. The last frame can be MAX_AUDIO_VARIATION samples
                    more or less than the average. */ 
                    
                    unsigned long availFrameSize;
                    uint32_t numSamples;
                    
                    /* read to get upto the maximum frame size in buffer */
                    numRead = fread(inputs[i].buffer + inputs[i].bufferOffset, 1, inputs[i].frameSize - inputs[i].bufferOffset, inputs[i].file);
                    if (inputs[i].bufferOffset + numRead < inputs[i].frameSize)
                    {
                        /* last or incomplete frame */
                        done = 1;
                        
                        /* frame is incomplete if it is less than the minimum allowed */ 
                        if (inputs[i].bufferOffset + numRead < inputs[i].minFrameSize)
                        {
                            break;
                        }
                        
                        /* frame size is the remaining samples */
                        availFrameSize = ((inputs[i].bufferOffset + (uint32_t)numRead) / inputs[i].bytesPerSample) * inputs[i].bytesPerSample;
                    }
                    else
                    {
                        availFrameSize = inputs[i].frameSizeSeq[inputs[i].seqIndex];
                    }

                    numSamples = availFrameSize / inputs[i].bytesPerSample;
                    if (!write_samples(clipWriter, inputs[i].materialTrackID, numSamples, inputs[i].buffer, availFrameSize))
                    {
                        fprintf(stderr, "Failed to write PCM frame\n");
                        goto fail;
                    }
                    
                    /* copy the remaining samples for the next frame */
                    inputs[i].bufferOffset = inputs[i].frameSize - availFrameSize;
                    memcpy(inputs[i].buffer, inputs[i].buffer + availFrameSize, inputs[i].bufferOffset);
                    
                    /* set the next frame size sequence index */
                    inputs[i].seqIndex = (inputs[i].seqIndex + 1) % 5;
                }
                
            }
            else if (inputs[i].essenceType == PCM && inputs[i].isWAVFile)
            {
                if (isPAL)
                {
                    uint32_t numSamples;
                    int channelZeroInput = i - inputs[i].channelIndex;
                    
                    if (inputs[i].channelIndex == 0)
                    {
                        /* read a frame of multi-channel audio data */
                        if (!get_wave_data(&inputs[i].wavInput, inputs[i].buffer, 
                            inputs[i].frameSize * inputs[i].wavInput.numAudioChannels, &numRead))
                        {
                            fprintf(stderr, "Failed to read PCM frame\n");
                            goto fail;
                        }
                        if (numRead != inputs[i].frameSize * inputs[i].wavInput.numAudioChannels)
                        {
                            done = 1;
                            break;
                        }
                    }
                    else
                    {
                        numRead = inputs[i].frameSize * inputs[i].wavInput.numAudioChannels;
                    }

                    numSamples = (uint32_t)numRead / (inputs[i].wavInput.numAudioChannels * inputs[i].bytesPerSample);
                        
                    /* extract a single channel audio data and write */
                    get_wave_channel(&inputs[i].wavInput, numRead, inputs[channelZeroInput].buffer, 
                        inputs[i].channelIndex, inputs[i].channelBuffer);
                    if (!write_samples(clipWriter, inputs[i].materialTrackID, numSamples, 
                        inputs[i].channelBuffer, inputs[i].frameSize))
                    {
                        fprintf(stderr, "Failed to write PCM frame\n");
                        goto fail;
                    }
                }
                else
                {
                    /* For NTSC we read frame sizes corresponding to the sequence set in
                    inputs[i].frameSizeSeq. The last frame can be MAX_AUDIO_VARIATION samples
                    more or less than the average. */ 
                    
                    uint32_t numSamples;
                    int channelZeroInput = i - inputs[i].channelIndex;
                    
                    if (inputs[i].channelIndex == 0)
                    {
                        /* read a frame of multi-channel audio data */
                        if (!get_wave_data(&inputs[i].wavInput, inputs[i].buffer + inputs[i].bufferOffset, 
                            inputs[i].frameSize * inputs[i].wavInput.numAudioChannels - inputs[i].bufferOffset, &numRead))
                        {
                            fprintf(stderr, "Failed to read Wave PCM frame\n");
                            goto fail;
                        }
                        
                        if (inputs[i].bufferOffset + numRead < inputs[i].frameSize * inputs[i].wavInput.numAudioChannels)
                        {
                            /* last or incomplete frame */
                            done = 1;
                            
                            /* frame is incomplete if it is less than the minimum allowed */ 
                            if (inputs[i].bufferOffset + numRead < inputs[i].minFrameSize * inputs[i].wavInput.numAudioChannels)
                            {
                                break;
                            }

                            /* frame size is the remaining samples */
                            inputs[i].availFrameSize = ((inputs[i].bufferOffset + (uint32_t)numRead) / 
                                (inputs[i].wavInput.numAudioChannels * inputs[i].bytesPerSample)) * 
                                inputs[i].bytesPerSample;
                        }
                        else
                        {
                            inputs[i].availFrameSize = inputs[i].frameSizeSeq[inputs[i].seqIndex];
                        }
                    }
                    else
                    {
                        inputs[i].availFrameSize = inputs[channelZeroInput].availFrameSize;
                    }

                    numSamples = inputs[i].availFrameSize / inputs[i].bytesPerSample;
                        
                    /* extract a single channel audio data and write */
                    get_wave_channel(&inputs[i].wavInput, inputs[i].availFrameSize * inputs[i].wavInput.numAudioChannels, 
                        inputs[channelZeroInput].buffer, inputs[i].channelIndex, inputs[i].channelBuffer);
                    if (!write_samples(clipWriter, inputs[i].materialTrackID, numSamples, inputs[i].channelBuffer, 
                        inputs[i].availFrameSize))
                    {
                        fprintf(stderr, "Failed to write Wave PCM frame\n");
                        goto fail;
                    }

                    /* prepare for the next frame if this is the last channel */
                    if (inputs[i].channelIndex + 1 >= inputs[i].wavInput.numAudioChannels)
                    {
                        /* copy the remaining samples for the next frame */
                        inputs[channelZeroInput].bufferOffset = (inputs[channelZeroInput].frameSize - inputs[channelZeroInput].availFrameSize) * 
                            inputs[channelZeroInput].wavInput.numAudioChannels;
                        memcpy(inputs[channelZeroInput].buffer, inputs[channelZeroInput].buffer + inputs[channelZeroInput].availFrameSize * inputs[channelZeroInput].wavInput.numAudioChannels, 
                            inputs[channelZeroInput].bufferOffset);
                        
                        /* set the next frame size sequence index */
                        inputs[channelZeroInput].seqIndex = (inputs[channelZeroInput].seqIndex + 1) % 5;
                    }
                }
            }
            else
            {
                /* unknown essence type - code fix needed */
                assert(0);
            }
        }
    }


    /* complete writing */
    
    if (!complete_writing(&clipWriter))
    {
        fprintf(stderr, "Failed to complete writing\n");
        goto fail;
    }

    
    
    /* free and close */
    
    free_package_definitions(&packageDefinitions);
    
    for (i = 0; i < inputIndex; i++)
    {
        if (inputs[i].isWAVFile)
        {
            if (inputs[i].channelIndex == 0)
            {
                fclose(inputs[i].wavInput.file);
            }
        }
        else
        {
            fclose(inputs[i].file);
        }
        
        if (inputs[i].buffer != NULL)
        {
            free(inputs[i].buffer);
        }
        
        if (inputs[i].essenceType == AvidMJPEG)
        {
            if (inputs[i].mjpegState.buffer != NULL)
            {
                free(inputs[i].mjpegState.buffer);
            }
        }
    }    


    return 0;

    
fail:
    /* abort, free and close and return 1 */
    
    if (clipWriter != NULL)
    {
        abort_writing(&clipWriter, 1);
    }

    if (packageDefinitions != NULL)
    {
        free_package_definitions(&packageDefinitions);
    }
    
    for (i = 0; i < inputIndex; i++)
    {
        if (inputs[i].isWAVFile)
        {
            if (inputs[i].channelIndex == 0)
            {
                if (inputs[i].wavInput.file != NULL)
                {
                    fclose(inputs[i].wavInput.file);
                }
            }
        }
        else
        {
            if (inputs[i].file != NULL)
            {
                fclose(inputs[i].file);
            }
        }

        if (inputs[i].buffer != NULL)
        {
            free(inputs[i].buffer);
        }
        
        if (inputs[i].essenceType == AvidMJPEG)
        {
            if (inputs[i].mjpegState.buffer != NULL)
            {
                free(inputs[i].mjpegState.buffer);
            }
        }
    }    

    return 1;
}

