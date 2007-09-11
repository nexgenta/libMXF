/*
 * $Id: write_archive_mxf.c,v 1.1 2007/09/11 13:24:47 stuart_hc Exp $
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
#include <stdarg.h>
#include <ctype.h>
#include <assert.h>

#include <mxf/mxf.h>
#include <mxf/mxf_uu_metadata.h>
#include <write_archive_mxf.h>
#include <timecode_index.h>


/* declare the BBC D3 extensions */

#define MXF_LABEL(d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15) \
    {d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15}

#define MXF_SET_DEFINITION(parentName, name, label) \
    static const mxfUL MXF_SET_K(name) = label;
    
#define MXF_ITEM_DEFINITION(setName, name, label, localTag, typeId, isRequired) \
    static const mxfUL MXF_ITEM_K(setName, name) = label;

#include <../bbc_d3_extensions_data_model.h>


/* minimum llen to use for sets */
#define MIN_LLEN                        4

/* (2^16 - 8) / 16*/
#define MAX_STRONG_REF_ARRAY_COUNT      4095

/* the size of the System Item in the essence data */
#define SYSTEM_ITEM_SIZE                28    

#define MAX_AUDIO_TRACKS                4    

    
/* the maximum number of D3 VTR errors in which a check is made that a timeode can be
   converted to a position. If not, then no errors are stored and a warning is given */
#define MAXIMUM_ERROR_CHECK             20

/* we expect the URL to be the filename used on the LTO tape which is around 16 characters */
/* the value is much larger here to allow for unforseen changes or for testing purposes */
/* note that the filename will always be truncated if it exceeds this length (-1) */
#define NETWORK_LOCATOR_URL_SIZE        256


/* values used to identify fields that are not present */
#define INVALID_MONTH_VALUE             99
#define INVALID_DURATION_VALUE          (-1)



typedef struct
{
    int haveTimecode;
    int haveVideo;
    int audioNum;
} EssWriteState;

struct _ArchiveMXFWriter 
{
    int numAudioTracks;
    int beStrict;
    
    MXFFile* mxfFile;
    
    MXFList pseFailures;
    
    mxfUTF16Char* tempString;
    
    TimecodeIndex vitcIndex;
    TimecodeIndex ltcIndex;
    
    mxfLength duration;

    EssWriteState essWriteState;
    
    uint64_t headerMetadataFilePos;
    uint64_t bodyFilePos;
    mxfTimestamp now;
    
    MXFDataModel* dataModel;
    MXFList* partitions;
    MXFHeaderMetadata* headerMetadata;
    MXFIndexTableSegment* indexSegment;
    
    /* these are references and should not be free'd */
    MXFPartition* headerPartition;
    MXFPartition* footerPartition;
    MXFMetadataSet* prefaceSet;
    MXFMetadataSet* identSet;
    MXFMetadataSet* contentStorageSet;
    MXFMetadataSet* materialPackageSet;
    MXFMetadataSet* sourcePackageSet;
    MXFMetadataSet* tapeSourcePackageSet;
    MXFMetadataSet* sourcePackageTrackSet;
    MXFMetadataSet* materialPackageTrackSet;
    MXFMetadataSet* sequenceSet;
    MXFMetadataSet* sourceClipSet;
    MXFMetadataSet* dmSet;
    MXFMetadataSet* fileDMFrameworkSet;
    MXFMetadataSet* tapeDMFrameworkSet;
    MXFMetadataSet* dmFrameworkSet;
    MXFMetadataSet* timecodeComponentSet;    
    MXFMetadataSet* essContainerDataSet;
    MXFMetadataSet* multipleDescriptorSet;
    MXFMetadataSet* descriptorSet;
    MXFMetadataSet* cdciDescriptorSet;
    MXFMetadataSet* bwfDescriptorSet;
    MXFMetadataSet* tapeDescriptorSet;
    MXFMetadataSet* videoMaterialPackageTrackSet;
    MXFMetadataSet* videoSequenceSet;
    MXFMetadataSet* networkLocatorSet;
    
    /* duration items that are updated when writing is completed */
    MXFMetadataItem* durationItems[2 + (MAX_AUDIO_TRACKS + 1) * 4];
    int numDurationItems;
    /* container duration items that are created when writing is completed */
    MXFMetadataSet* descriptorSets[MAX_AUDIO_TRACKS + 2];
    int numDescriptorSets;
    
    MXFList d3VTRErrorTrackSets;
    MXFList pseFailureTrackSets;
};
    

static const uint64_t g_fixedBodyOffset = 0x8000;


static const mxfUUID g_mxfIdentProductUID = 
    {0x9e, 0x26, 0x08, 0xb1, 0xc9, 0xfe, 0x44, 0x48, 0x88, 0xdf, 0x26, 0x94, 0xcf, 0x77, 0x59, 0x9a};
static const mxfUTF16Char* g_mxfIdentCompanyName = L"BBC";
static const mxfUTF16Char* g_mxfIdentProductName = L"D3 Preservation MXF Writer";
static const mxfUTF16Char* g_mxfIdentVersionString = L"Version March 2007";


static const mxfUL MXF_DM_L(D3P_D3PreservationDescriptiveScheme) = 
    {0x06, 0x0E, 0x2B, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0D, 0x04, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00};

static const mxfKey g_UncBaseElementKey = MXF_UNC_EE_K(0x00, 0x00, 0x00);
static const mxfKey g_WavBaseElementKey = MXF_AES3BWF_EE_K(0x00, 0x00, 0x00);
static const mxfKey g_TimecodeSysItemElementKey = MXF_SS1_ELEMENT_KEY(0x01, 0x00);

static const uint32_t g_bodySID = 1;
static const uint32_t g_indexSID = 2;

static const mxfRational g_audioSampleRate = {48000, 1};
static const mxfRational g_audioEditRate = {25, 1};
static const uint32_t g_audioQuantBits = 20;
static const uint16_t g_audioBlockAlign = 3;
static const uint32_t g_audioAvgBps = 144000; /* 48000*3 */
static const uint16_t g_audioSamplesPerFrame = 1920; /* 48000/25 */ 
static const uint32_t g_audioFrameSize = 1920 * 3;

static const mxfRational g_videoSampleRate = {25, 1};
static const mxfRational g_videoEditRate = {25, 1};
static const uint8_t g_videoFrameLayout = 0x03;
static const uint32_t g_videoStoredHeight = 576; /* for mixed fields this is the frame height */
static const uint32_t g_videoStoredWidth = 720;
static const int32_t g_videoLineMap[2] = {23, 336};
static const mxfRational g_videoAspectRatio = {4, 3};
static const uint32_t g_videoComponentDepth = 8;
static const uint32_t g_videoHorizontalSubSampling = 2;
static const uint32_t g_videoVerticalSubSampling = 1;
static const uint32_t g_videoFrameSize = 720 * 576 * 2; /* W x H x (Y + Cr/2 + Cb/2) */

static const int64_t g_tapeLen = 120 * 25 * 60 * 60;
static const int g_numTapeAudioTracks = MAX_AUDIO_TRACKS;

static const mxfUTF16Char* g_vtrErrorsTrackName = L"D3 VTR Errors";
static const mxfUTF16Char* g_pseFailuresTrackName = L"PSE Failures";

static const mxfUTF16Char* g_D3FormatString = L"D3";

static const int g_infaxDataStringSeparator = '|';

/* size of infax data set (including InstanceUID) with all fields + the minimum KLV fill + 32 (error margin) */
static const uint64_t g_fixedInfaxSetAllocationSize = 
    (16 + MIN_LLEN + 4 + 16 + COMPLETE_INFAX_EXTERNAL_SIZE + 16 + MIN_LLEN + 32);


/* functions for loading the BBC D3 extensions */

#define MXF_LABEL(d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15) \
    {d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15}

#define MXF_SET_DEFINITION(parentName, name, label) \
    CHK_ORET(mxf_register_set_def(dataModel, #name, &MXF_SET_K(parentName), &MXF_SET_K(name)));
    
#define MXF_ITEM_DEFINITION(setName, name, label, tag, typeId, isRequired) \
    CHK_ORET(mxf_register_item_def(dataModel, #name, &MXF_SET_K(setName), &MXF_ITEM_K(setName, name), tag, typeId, isRequired));
    
static int load_bbc_d3_extensions(MXFDataModel* dataModel)
{
#include <../bbc_d3_extensions_data_model.h>

    return 1;
}


static void set_null_infax_data(InfaxData* infaxData)
{
    memset(infaxData, 0, sizeof(InfaxData));
    infaxData->txDate.month = INVALID_MONTH_VALUE;
    infaxData->stockDate.month = INVALID_MONTH_VALUE;
    infaxData->duration = INVALID_DURATION_VALUE;
}

static int convert_string(const char* input, mxfUTF16Char** tempString)
{
    size_t len = strlen(input);
    SAFE_FREE(tempString);
    CHK_MALLOC_ARRAY_ORET((*tempString), mxfUTF16Char, len + 1);
    memset((*tempString), 0, sizeof(mxfUTF16Char) * (len + 1));
    CHK_ORET(mbstowcs((*tempString), input, len + 1) != (size_t)(-1));
    
    return 1;
}

static void free_d3_mxf_file(ArchiveMXFWriter** output)
{
    if (*output == NULL)
    {
        return;
    }
    
    SAFE_FREE(&(*output)->tempString);

    clear_timecode_index(&(*output)->vitcIndex);
    clear_timecode_index(&(*output)->ltcIndex);
    
    mxf_free_index_table_segment(&(*output)->indexSegment);
    mxf_free_file_partitions(&(*output)->partitions);
    mxf_free_header_metadata(&(*output)->headerMetadata);
    mxf_free_data_model(&(*output)->dataModel);
    
    mxf_clear_list(&(*output)->d3VTRErrorTrackSets);
    mxf_clear_list(&(*output)->pseFailureTrackSets);
    
    mxf_file_close(&(*output)->mxfFile);
    
    SAFE_FREE(output);
}


static int verify_essence_write_state(ArchiveMXFWriter* output, int writeTimecode, int writeVideo, int writeAudio)
{
    assert(writeTimecode || writeVideo || writeAudio);
    
    if (writeTimecode)
    {
        if (output->essWriteState.haveTimecode)
        {
            mxf_log(MXF_ELOG, "Timecode already written" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
            return 0;
        }
    }
    else if (writeVideo)
    {
        if (!output->essWriteState.haveTimecode)
        {
            mxf_log(MXF_ELOG, "Must first write timecode before video frame" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
            return 0;
        }
        if (output->essWriteState.haveVideo)
        {
            mxf_log(MXF_ELOG, "Video frame already written" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
            return 0;
        }
    }
    else // writeAudio
    {
        if (!output->essWriteState.haveVideo)
        {
            mxf_log(MXF_ELOG, "Must write video frame before audio frames" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
            return 0;
        }
    }
    
    return 1;
}

static void update_essence_write_state(ArchiveMXFWriter* output, int writeTimecode, int writeVideo, int writeAudio)
{
    assert(writeTimecode || writeVideo || writeAudio);
    
    if (writeTimecode)
    {
        output->essWriteState.haveTimecode = 1;
    }
    else if (writeVideo)
    {
        output->essWriteState.haveVideo = 1;
        if (output->numAudioTracks == 0)
        {
            output->essWriteState.haveTimecode = 0;
            output->essWriteState.haveVideo = 0;
            output->essWriteState.audioNum = 0;
        }
    }
    else // writeAudio
    {
        output->essWriteState.audioNum++;
        if (output->essWriteState.audioNum >= output->numAudioTracks)
        {
            output->essWriteState.haveTimecode = 0;
            output->essWriteState.haveVideo = 0;
            output->essWriteState.audioNum = 0;
        }
    }
}

static void convert_timecode_to_12m(ArchiveTimecode* t, uint8_t* t12m)
{
    /* the format follows the specification of the TimecodeArray property
    defined in SMPTE 405M, table 2, which follows section 8.2 of SMPTE 331M 
    The Binary Group Data is not used and is set to 0*/
    
    memset(t12m, 0, 8);
    t12m[0] = ((t->frame % 10) & 0x0f) | (((t->frame / 10) & 0x3) << 4);
    t12m[1] = ((t->sec % 10) & 0x0f) | (((t->sec / 10) & 0x7) << 4);
    t12m[2] = ((t->min % 10) & 0x0f) | (((t->min / 10) & 0x7) << 4);
    t12m[3] = ((t->hour % 10) & 0x0f) | (((t->hour / 10) & 0x3) << 4);
    
}

static int64_t getPosition(int64_t videoPosition, const mxfRational* targetEditRate)
{
    if (memcmp(targetEditRate, &g_videoEditRate, sizeof(mxfRational)) == 0)
    {
        return videoPosition;
    }

    double factor = targetEditRate->numerator * g_videoEditRate.denominator / 
        (double)(targetEditRate->denominator * g_videoEditRate.numerator);
    return (int64_t)(videoPosition * factor + 0.5);        
}

static int is_empty_string(const char* str, int includingSpace)
{
    const char* strPtr = str;
    
    if (str == NULL)
    {
        return 1;
    }
    
    while (*strPtr != '\0')
    {
        if (!includingSpace || !isspace(*strPtr))
        {
            return 0;
        }
        strPtr++;
    }
    
    return 1;
}

static int set_infax_data(MXFMetadataSet* dmFrameworkSet, InfaxData* infaxData)
{
    mxfTimestamp dateOnly;
    mxfUTF16Char* tempString = NULL;
    MXFMetadataItem* item;
    
    if (!is_empty_string(infaxData->format, 1))
    {
        CHK_OFAIL(convert_string(infaxData->format, &tempString));
        CHK_OFAIL(mxf_set_fixed_size_utf16string_item(dmFrameworkSet, &MXF_ITEM_K(D3P_InfaxFramework, D3P_Format), tempString, FORMAT_SIZE));
    }
    else if (mxf_have_item(dmFrameworkSet, &MXF_ITEM_K(D3P_InfaxFramework, D3P_Format)))
    {
        CHK_OFAIL(mxf_remove_item(dmFrameworkSet, &MXF_ITEM_K(D3P_InfaxFramework, D3P_Format), &item));
        mxf_free_item(&item);
    }
    
    if (!is_empty_string(infaxData->progTitle, 0))
    {
        CHK_OFAIL(convert_string(infaxData->progTitle, &tempString));
        CHK_OFAIL(mxf_set_fixed_size_utf16string_item(dmFrameworkSet, &MXF_ITEM_K(D3P_InfaxFramework, D3P_ProgrammeTitle), tempString, PROGTITLE_SIZE));
    }
    else if (mxf_have_item(dmFrameworkSet, &MXF_ITEM_K(D3P_InfaxFramework, D3P_ProgrammeTitle)))
    {
        CHK_OFAIL(mxf_remove_item(dmFrameworkSet, &MXF_ITEM_K(D3P_InfaxFramework, D3P_ProgrammeTitle), &item));
        mxf_free_item(&item);
    }
    
    if (!is_empty_string(infaxData->epTitle, 0))
    {
        CHK_OFAIL(convert_string(infaxData->epTitle, &tempString));
        CHK_OFAIL(mxf_set_fixed_size_utf16string_item(dmFrameworkSet, &MXF_ITEM_K(D3P_InfaxFramework, D3P_EpisodeTitle), tempString, EPTITLE_SIZE));
    }
    else if (mxf_have_item(dmFrameworkSet, &MXF_ITEM_K(D3P_InfaxFramework, D3P_EpisodeTitle)))
    {
        CHK_OFAIL(mxf_remove_item(dmFrameworkSet, &MXF_ITEM_K(D3P_InfaxFramework, D3P_EpisodeTitle), &item));
        mxf_free_item(&item);
    }
    
    if (infaxData->txDate.month != INVALID_MONTH_VALUE)
    {
        /* only the date part is relevant */
        dateOnly = infaxData->txDate;
        dateOnly.hour = 0;
        dateOnly.min = 0;
        dateOnly.sec = 0;
        dateOnly.qmsec = 0;
        CHK_OFAIL(mxf_set_timestamp_item(dmFrameworkSet, &MXF_ITEM_K(D3P_InfaxFramework, D3P_TransmissionDate), &dateOnly));
    }
    
    if (!is_empty_string(infaxData->magPrefix, 1))
    {
        CHK_OFAIL(convert_string(infaxData->magPrefix, &tempString));
        CHK_OFAIL(mxf_set_fixed_size_utf16string_item(dmFrameworkSet, &MXF_ITEM_K(D3P_InfaxFramework, D3P_MagazinePrefix), tempString, MAGPREFIX_SIZE));
    }
    else if (mxf_have_item(dmFrameworkSet, &MXF_ITEM_K(D3P_InfaxFramework, D3P_MagazinePrefix)))
    {
        CHK_OFAIL(mxf_remove_item(dmFrameworkSet, &MXF_ITEM_K(D3P_InfaxFramework, D3P_MagazinePrefix), &item));
        mxf_free_item(&item);
    }
    
    if (!is_empty_string(infaxData->progNo, 1))
    {
        CHK_OFAIL(convert_string(infaxData->progNo, &tempString));
        CHK_OFAIL(mxf_set_fixed_size_utf16string_item(dmFrameworkSet, &MXF_ITEM_K(D3P_InfaxFramework, D3P_ProgrammeNumber), tempString, PROGNO_SIZE));
    }
    else if (mxf_have_item(dmFrameworkSet, &MXF_ITEM_K(D3P_InfaxFramework, D3P_ProgrammeNumber)))
    {
        CHK_OFAIL(mxf_remove_item(dmFrameworkSet, &MXF_ITEM_K(D3P_InfaxFramework, D3P_ProgrammeNumber), &item));
        mxf_free_item(&item);
    }
    
    if (!is_empty_string(infaxData->spoolStatus, 1))
    {
        CHK_OFAIL(convert_string(infaxData->spoolStatus, &tempString));
        CHK_OFAIL(mxf_set_fixed_size_utf16string_item(dmFrameworkSet, &MXF_ITEM_K(D3P_InfaxFramework, D3P_SpoolStatus), tempString, SPOOLSTATUS_SIZE));
    }
    else if (mxf_have_item(dmFrameworkSet, &MXF_ITEM_K(D3P_InfaxFramework, D3P_SpoolStatus)))
    {
        CHK_OFAIL(mxf_remove_item(dmFrameworkSet, &MXF_ITEM_K(D3P_InfaxFramework, D3P_SpoolStatus), &item));
        mxf_free_item(&item);
    }
    
    if (infaxData->stockDate.month != INVALID_MONTH_VALUE)
    {
        /* only the date part is relevant */
        dateOnly = infaxData->stockDate;
        dateOnly.hour = 0;
        dateOnly.min = 0;
        dateOnly.sec = 0;
        dateOnly.qmsec = 0;
        CHK_OFAIL(mxf_set_timestamp_item(dmFrameworkSet, &MXF_ITEM_K(D3P_InfaxFramework, D3P_StockDate), &dateOnly));
    }
    
    if (!is_empty_string(infaxData->spoolDesc, 0))
    {
        CHK_OFAIL(convert_string(infaxData->spoolDesc, &tempString));
        CHK_OFAIL(mxf_set_fixed_size_utf16string_item(dmFrameworkSet, &MXF_ITEM_K(D3P_InfaxFramework, D3P_SpoolDescriptor), tempString, SPOOLDESC_SIZE));
    }
    else if (mxf_have_item(dmFrameworkSet, &MXF_ITEM_K(D3P_InfaxFramework, D3P_SpoolDescriptor)))
    {
        CHK_OFAIL(mxf_remove_item(dmFrameworkSet, &MXF_ITEM_K(D3P_InfaxFramework, D3P_SpoolDescriptor), &item));
        mxf_free_item(&item);
    }
    
    if (!is_empty_string(infaxData->memo, 0))
    {
        CHK_OFAIL(convert_string(infaxData->memo, &tempString));
        CHK_OFAIL(mxf_set_fixed_size_utf16string_item(dmFrameworkSet, &MXF_ITEM_K(D3P_InfaxFramework, D3P_Memo), tempString, MEMO_SIZE));
    }
    else if (mxf_have_item(dmFrameworkSet, &MXF_ITEM_K(D3P_InfaxFramework, D3P_Memo)))
    {
        CHK_OFAIL(mxf_remove_item(dmFrameworkSet, &MXF_ITEM_K(D3P_InfaxFramework, D3P_Memo), &item));
        mxf_free_item(&item);
    }
    
    if (infaxData->duration != INVALID_DURATION_VALUE)
    {
        CHK_OFAIL(mxf_set_int64_item(dmFrameworkSet, &MXF_ITEM_K(D3P_InfaxFramework, D3P_Duration), infaxData->duration));
    }
    
    if (!is_empty_string(infaxData->spoolNo, 1))
    {
        CHK_OFAIL(convert_string(infaxData->spoolNo, &tempString));
        CHK_OFAIL(mxf_set_fixed_size_utf16string_item(dmFrameworkSet, &MXF_ITEM_K(D3P_InfaxFramework, D3P_SpoolNumber), tempString, SPOOLNO_SIZE));
    }
    else if (mxf_have_item(dmFrameworkSet, &MXF_ITEM_K(D3P_InfaxFramework, D3P_SpoolNumber)))
    {
        CHK_OFAIL(mxf_remove_item(dmFrameworkSet, &MXF_ITEM_K(D3P_InfaxFramework, D3P_SpoolNumber), &item));
        mxf_free_item(&item);
    }
    
    if (!is_empty_string(infaxData->accNo, 1))
    {
        CHK_OFAIL(convert_string(infaxData->accNo, &tempString));
        CHK_OFAIL(mxf_set_fixed_size_utf16string_item(dmFrameworkSet, &MXF_ITEM_K(D3P_InfaxFramework, D3P_AccessionNumber), tempString, ACCNO_SIZE));
    }
    else if (mxf_have_item(dmFrameworkSet, &MXF_ITEM_K(D3P_InfaxFramework, D3P_AccessionNumber)))
    {
        CHK_OFAIL(mxf_remove_item(dmFrameworkSet, &MXF_ITEM_K(D3P_InfaxFramework, D3P_AccessionNumber), &item));
        mxf_free_item(&item);
    }
    
    if (!is_empty_string(infaxData->catDetail, 1))
    {
        CHK_OFAIL(convert_string(infaxData->catDetail, &tempString));
        CHK_OFAIL(mxf_set_fixed_size_utf16string_item(dmFrameworkSet, &MXF_ITEM_K(D3P_InfaxFramework, D3P_CatalogueDetail), tempString, CATDETAIL_SIZE));
    }
    else if (mxf_have_item(dmFrameworkSet, &MXF_ITEM_K(D3P_InfaxFramework, D3P_CatalogueDetail)))
    {
        CHK_OFAIL(mxf_remove_item(dmFrameworkSet, &MXF_ITEM_K(D3P_InfaxFramework, D3P_CatalogueDetail), &item));
        mxf_free_item(&item);
    }
    
    SAFE_FREE(&tempString);
    return 1;
    
fail:
    SAFE_FREE(&tempString);
    return 0;
}


#define PARSE_STRING(member, size, beStrict) \
{ \
    int cpySize = endField - startField; \
    if (cpySize < 0) \
    { \
        mxf_log(MXF_ELOG, "invalid infax string field" LOG_LOC_FORMAT, LOG_LOC_PARAMS); \
        return 0; \
    } \
    else if (cpySize >= size) \
    { \
        if (beStrict) \
        { \
            mxf_log(MXF_ELOG, "Infax string size (%d) exceeds limit (%d)" \
                LOG_LOC_FORMAT, endField - startField, size, LOG_LOC_PARAMS); \
            return 0; \
        } \
        else \
        { \
            mxf_log(MXF_WLOG, "Infax string size (%d) exceeds limit (%d) - string will be truncated" \
                LOG_LOC_FORMAT, endField - startField, size, LOG_LOC_PARAMS); \
            cpySize = size - 1; \
        } \
    } \
    \
    if (cpySize > 0) \
    { \
        strncpy(member, startField, cpySize); \
    } \
    member[cpySize] = '\0'; \
}


#define PARSE_DATE(member, beStrict) \
{ \
    if (endField - startField > 0) \
    { \
        int year, month, day; \
        CHK_ORET(sscanf(startField, "%d-%u-%u", &year, &month, &day) == 3); \
        member.year = (int16_t)year; \
        member.month = (uint8_t)month; \
        member.day = (uint8_t)day; \
    } \
    else \
    { \
        member.month = INVALID_MONTH_VALUE; \
    } \
}
#define PARSE_INT64(member, invalid, beStrict) \
{ \
    if (endField - startField > 0) \
    { \
        CHK_ORET(sscanf(startField, "%"PRIi64"", &member) == 1); \
    } \
    else \
    { \
        member = invalid; \
    } \
}

static int parse_infax_data(const char* infaxDataString, InfaxData* infaxData, int beStrict)
{
    const char* startField = infaxDataString;
    const char* endField = NULL;
    int fieldIndex = 0;
    int done = 0;
    
    CHK_ORET(infaxDataString != NULL);
    
    set_null_infax_data(infaxData);
    
    do
    {
        endField = strchr(startField, g_infaxDataStringSeparator);
        if (endField == NULL)
        {
            done = 1;
            endField = startField + strlen(startField);
        }
        
        switch (fieldIndex)
        {
            case 0:
                PARSE_STRING(infaxData->format, FORMAT_SIZE, beStrict);
                break;
            case 1:
                PARSE_STRING(infaxData->progTitle, PROGTITLE_SIZE, beStrict);
                break;
            case 2:
                PARSE_STRING(infaxData->epTitle, EPTITLE_SIZE, beStrict);
                break;
            case 3:
                PARSE_DATE(infaxData->txDate, beStrict);
                break;
            case 4:
                PARSE_STRING(infaxData->magPrefix, MAGPREFIX_SIZE, beStrict);
                break;
            case 5:
                PARSE_STRING(infaxData->progNo, PROGNO_SIZE, beStrict);
                break;
            case 6:
                PARSE_STRING(infaxData->spoolStatus, SPOOLSTATUS_SIZE, beStrict);
                break;
            case 7:
                PARSE_DATE(infaxData->stockDate, beStrict);
                break;
            case 8:
                PARSE_STRING(infaxData->spoolDesc, SPOOLDESC_SIZE, beStrict);
                break;
            case 9:
                PARSE_STRING(infaxData->memo, MEMO_SIZE, beStrict);
                break;
            case 10:
                PARSE_INT64(infaxData->duration, INVALID_DURATION_VALUE, beStrict);
                break;
            case 11:
                PARSE_STRING(infaxData->spoolNo, SPOOLNO_SIZE, beStrict);
                break;
            case 12:
                PARSE_STRING(infaxData->accNo, ACCNO_SIZE, beStrict);
                break;
            case 13:
                PARSE_STRING(infaxData->catDetail, CATDETAIL_SIZE, beStrict);
                break;
            default:
                mxf_log(MXF_ELOG, "Invalid Infax data string ('%s')" LOG_LOC_FORMAT, infaxDataString, LOG_LOC_PARAMS);
                return 0;            
        }
        
        if (!done)
        {
            startField = endField + 1;
            fieldIndex++;
        }
    }
    while (!done);
    
    CHK_ORET(fieldIndex == 13); 
    
    return 1;
}

int prepare_archive_mxf_file(const char* filename, int numAudioTracks, int64_t startPosition, int beStrict, ArchiveMXFWriter** output)
{
    ArchiveMXFWriter* newOutput;
    int64_t filePos;
    mxfUUID uuid;
    int i;
    mxfUMID tapeSourcePackageUID;
    mxfUMID fileSourcePackageUID;
    mxfUMID materialPackageUID;
    uint32_t videoTrackNum = MXF_UNC_TRACK_NUM(0x00, 0x00, 0x00);
    uint32_t audioTrackNum = MXF_AES3BWF_TRACK_NUM(0x00, 0x00, 0x00);
    uint32_t deltaOffset;
#define NAME_BUFFER_SIZE 256
    char cNameBuffer[NAME_BUFFER_SIZE];
    mxfUTF16Char wNameBuffer[NAME_BUFFER_SIZE];
    uint8_t* arrayElement;
    InfaxData nullInfaxData;
    mxfLocalTag assignedTag;

    CHK_ORET(numAudioTracks <= MAX_AUDIO_TRACKS);
    
    mxf_generate_umid(&tapeSourcePackageUID);
    mxf_generate_umid(&fileSourcePackageUID);
    mxf_generate_umid(&materialPackageUID);

    set_null_infax_data(&nullInfaxData);

    CHK_MALLOC_ORET(newOutput, ArchiveMXFWriter);
    memset(newOutput, 0, sizeof(ArchiveMXFWriter));
    initialise_timecode_index(&newOutput->vitcIndex, 512);
    initialise_timecode_index(&newOutput->ltcIndex, 512);
    mxf_initialise_list(&newOutput->d3VTRErrorTrackSets, NULL);
    mxf_initialise_list(&newOutput->pseFailureTrackSets, NULL);
            
    
    newOutput->numAudioTracks = numAudioTracks;
    newOutput->beStrict = beStrict;
    mxf_get_timestamp_now(&newOutput->now);


    CHK_OFAIL(mxf_create_file_partitions(&newOutput->partitions));
    
    CHK_OFAIL(mxf_disk_file_open_new(filename, &newOutput->mxfFile));
    
    
    /* minimum llen is 4 */
    
    mxf_file_set_min_llen(newOutput->mxfFile, MIN_LLEN);

    
    /*
     * Write the Header Partition Pack
     */
     
    CHK_OFAIL(mxf_append_new_partition(newOutput->partitions, &newOutput->headerPartition));
    /* partition is open because the LTO and D3 Infax data needs to be filled in and is incomplete
       because the durations are not yet known */
    newOutput->headerPartition->key = MXF_PP_K(OpenIncomplete, Header);
    newOutput->headerPartition->bodySID = g_bodySID;
    newOutput->headerPartition->indexSID = g_indexSID;
    newOutput->headerPartition->operationalPattern = MXF_OP_L(1a, qq09);            
    CHK_OFAIL(mxf_append_partition_esscont_label(newOutput->headerPartition, 
        &MXF_EC_L(MultipleWrappings)));
    CHK_OFAIL(mxf_append_partition_esscont_label(newOutput->headerPartition, 
        &MXF_EC_L(SD_Unc_625_50i_422_135_FrameWrapped)));
    if (numAudioTracks > 0)
    {
        CHK_OFAIL(mxf_append_partition_esscont_label(newOutput->headerPartition, 
            &MXF_EC_L(BWFFrameWrapped)));
    }
    CHK_OFAIL(mxf_write_partition(newOutput->mxfFile, newOutput->headerPartition));

    
    CHK_OFAIL((filePos = mxf_file_tell(newOutput->mxfFile)) >= 0);
    newOutput->headerMetadataFilePos = (uint64_t)filePos;


    /*
     * Create the header metadata
     */
     
    /* load the baseline data model and BBC extensions */
    CHK_OFAIL(mxf_load_data_model(&newOutput->dataModel));
    CHK_OFAIL(load_bbc_d3_extensions(newOutput->dataModel));
    CHK_OFAIL(mxf_finalise_data_model(newOutput->dataModel));
    
    
    /* create the header metadata */
    CHK_OFAIL(mxf_create_header_metadata(&newOutput->headerMetadata, newOutput->dataModel));
    newOutput->numDurationItems = 0;
    newOutput->numDescriptorSets = 0;
    
    
    /* register the Infax data set items in the primer pack, so that addition of new items in the
    update_archive_mxf_file() function have their local tag registered in the primer pack because
    the primer pack is not extendable in update_archive_mxf_file() */
    CHK_OFAIL(mxf_register_primer_entry(newOutput->headerMetadata->primerPack, &MXF_ITEM_K(D3P_InfaxFramework, D3P_Format), 
        g_Null_LocalTag, &assignedTag));
    CHK_OFAIL(mxf_register_primer_entry(newOutput->headerMetadata->primerPack, &MXF_ITEM_K(D3P_InfaxFramework, D3P_ProgrammeTitle), 
        g_Null_LocalTag, &assignedTag));
    CHK_OFAIL(mxf_register_primer_entry(newOutput->headerMetadata->primerPack, &MXF_ITEM_K(D3P_InfaxFramework, D3P_EpisodeTitle), 
        g_Null_LocalTag, &assignedTag));
    CHK_OFAIL(mxf_register_primer_entry(newOutput->headerMetadata->primerPack, &MXF_ITEM_K(D3P_InfaxFramework, D3P_TransmissionDate), 
        g_Null_LocalTag, &assignedTag));
    CHK_OFAIL(mxf_register_primer_entry(newOutput->headerMetadata->primerPack, &MXF_ITEM_K(D3P_InfaxFramework, D3P_MagazinePrefix), 
        g_Null_LocalTag, &assignedTag));
    CHK_OFAIL(mxf_register_primer_entry(newOutput->headerMetadata->primerPack, &MXF_ITEM_K(D3P_InfaxFramework, D3P_ProgrammeNumber), 
        g_Null_LocalTag, &assignedTag));
    CHK_OFAIL(mxf_register_primer_entry(newOutput->headerMetadata->primerPack, &MXF_ITEM_K(D3P_InfaxFramework, D3P_SpoolStatus), 
        g_Null_LocalTag, &assignedTag));
    CHK_OFAIL(mxf_register_primer_entry(newOutput->headerMetadata->primerPack, &MXF_ITEM_K(D3P_InfaxFramework, D3P_StockDate), 
        g_Null_LocalTag, &assignedTag));
    CHK_OFAIL(mxf_register_primer_entry(newOutput->headerMetadata->primerPack, &MXF_ITEM_K(D3P_InfaxFramework, D3P_SpoolDescriptor), 
        g_Null_LocalTag, &assignedTag));
    CHK_OFAIL(mxf_register_primer_entry(newOutput->headerMetadata->primerPack, &MXF_ITEM_K(D3P_InfaxFramework, D3P_Memo), 
        g_Null_LocalTag, &assignedTag));
    CHK_OFAIL(mxf_register_primer_entry(newOutput->headerMetadata->primerPack, &MXF_ITEM_K(D3P_InfaxFramework, D3P_Duration), 
        g_Null_LocalTag, &assignedTag));
    CHK_OFAIL(mxf_register_primer_entry(newOutput->headerMetadata->primerPack, &MXF_ITEM_K(D3P_InfaxFramework, D3P_SpoolNumber), 
        g_Null_LocalTag, &assignedTag));
    CHK_OFAIL(mxf_register_primer_entry(newOutput->headerMetadata->primerPack, &MXF_ITEM_K(D3P_InfaxFramework, D3P_AccessionNumber), 
        g_Null_LocalTag, &assignedTag));
    CHK_OFAIL(mxf_register_primer_entry(newOutput->headerMetadata->primerPack, &MXF_ITEM_K(D3P_InfaxFramework, D3P_CatalogueDetail), 
        g_Null_LocalTag, &assignedTag));
    
    
    /* Preface */
    CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(Preface), &newOutput->prefaceSet));
    CHK_OFAIL(mxf_set_timestamp_item(newOutput->prefaceSet, &MXF_ITEM_K(Preface, LastModifiedDate), &newOutput->now));
    CHK_OFAIL(mxf_set_version_type_item(newOutput->prefaceSet, &MXF_ITEM_K(Preface, Version), 0x0102));
    CHK_OFAIL(mxf_set_ul_item(newOutput->prefaceSet, &MXF_ITEM_K(Preface, OperationalPattern), &MXF_OP_L(1a, qq09)));
    if (newOutput->numAudioTracks > 0)
    {
        CHK_OFAIL(mxf_alloc_array_item_elements(newOutput->prefaceSet, &MXF_ITEM_K(Preface, EssenceContainers), mxfUL_extlen, 3, &arrayElement));
        mxf_set_ul(&MXF_EC_L(MultipleWrappings), arrayElement);
        mxf_set_ul(&MXF_EC_L(SD_Unc_625_50i_422_135_FrameWrapped), &arrayElement[mxfUL_extlen]);
        mxf_set_ul(&MXF_EC_L(BWFFrameWrapped), &arrayElement[mxfUL_extlen * 2]);
    }
    else
    {
        CHK_OFAIL(mxf_alloc_array_item_elements(newOutput->prefaceSet, &MXF_ITEM_K(Preface, EssenceContainers), mxfUL_extlen, 1, &arrayElement));
        mxf_set_ul(&MXF_EC_L(SD_Unc_625_50i_422_135_FrameWrapped), arrayElement);
    }
    CHK_OFAIL(mxf_alloc_array_item_elements(newOutput->prefaceSet, &MXF_ITEM_K(Preface, DMSchemes), mxfUL_extlen, 1, &arrayElement));
    mxf_set_ul(&MXF_DM_L(D3P_D3PreservationDescriptiveScheme), arrayElement);
    CHK_OFAIL(mxf_set_uint32_item(newOutput->prefaceSet, &MXF_ITEM_K(Preface, D3P_D3ErrorCount), 0));
    CHK_OFAIL(mxf_set_uint32_item(newOutput->prefaceSet, &MXF_ITEM_K(Preface, D3P_PSEFailureCount), 0));

    
    /* Preface - Identification */
    mxf_generate_uuid(&uuid);
    CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(Identification), &newOutput->identSet));
    CHK_OFAIL(mxf_add_array_item_strongref(newOutput->prefaceSet, &MXF_ITEM_K(Preface, Identifications), newOutput->identSet));
    CHK_OFAIL(mxf_set_uuid_item(newOutput->identSet, &MXF_ITEM_K(Identification, ThisGenerationUID), &uuid));
    CHK_OFAIL(mxf_set_utf16string_item(newOutput->identSet, &MXF_ITEM_K(Identification, CompanyName), g_mxfIdentCompanyName));
    CHK_OFAIL(mxf_set_utf16string_item(newOutput->identSet, &MXF_ITEM_K(Identification, ProductName), g_mxfIdentProductName));
    CHK_OFAIL(mxf_set_utf16string_item(newOutput->identSet, &MXF_ITEM_K(Identification, VersionString), g_mxfIdentVersionString));
    CHK_OFAIL(mxf_set_uuid_item(newOutput->identSet, &MXF_ITEM_K(Identification, ProductUID), &g_mxfIdentProductUID));
    CHK_OFAIL(mxf_set_timestamp_item(newOutput->identSet, &MXF_ITEM_K(Identification, ModificationDate), &newOutput->now));
    CHK_OFAIL(mxf_set_product_version_item(newOutput->identSet, &MXF_ITEM_K(Identification, ToolkitVersion), mxf_get_version()));
    CHK_OFAIL(mxf_set_utf16string_item(newOutput->identSet, &MXF_ITEM_K(Identification, Platform), mxf_get_platform_wstring()));

    /* Preface - ContentStorage */
    CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(ContentStorage), &newOutput->contentStorageSet));
    CHK_OFAIL(mxf_set_strongref_item(newOutput->prefaceSet, &MXF_ITEM_K(Preface, ContentStorage), newOutput->contentStorageSet));

    
    /* Preface - ContentStorage - EssenceContainerData */    
    CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(EssenceContainerData), &newOutput->essContainerDataSet));
    CHK_OFAIL(mxf_add_array_item_strongref(newOutput->contentStorageSet, &MXF_ITEM_K(ContentStorage, EssenceContainerData), newOutput->essContainerDataSet));
    CHK_OFAIL(mxf_set_umid_item(newOutput->essContainerDataSet, &MXF_ITEM_K(EssenceContainerData, LinkedPackageUID), &fileSourcePackageUID));
    CHK_OFAIL(mxf_set_uint32_item(newOutput->essContainerDataSet, &MXF_ITEM_K(EssenceContainerData, IndexSID), g_indexSID));
    CHK_OFAIL(mxf_set_uint32_item(newOutput->essContainerDataSet, &MXF_ITEM_K(EssenceContainerData, BodySID), g_bodySID));

    
    /* Preface - ContentStorage - MaterialPackage */
    CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(MaterialPackage), &newOutput->materialPackageSet));
    CHK_OFAIL(mxf_add_array_item_strongref(newOutput->contentStorageSet, &MXF_ITEM_K(ContentStorage, Packages), newOutput->materialPackageSet));
    CHK_OFAIL(mxf_set_umid_item(newOutput->materialPackageSet, &MXF_ITEM_K(GenericPackage, PackageUID), &materialPackageUID));
    CHK_OFAIL(mxf_set_timestamp_item(newOutput->materialPackageSet, &MXF_ITEM_K(GenericPackage, PackageCreationDate), &newOutput->now));
    CHK_OFAIL(mxf_set_timestamp_item(newOutput->materialPackageSet, &MXF_ITEM_K(GenericPackage, PackageModifiedDate), &newOutput->now));
    CHK_OFAIL(mxf_set_utf16string_item(newOutput->materialPackageSet, &MXF_ITEM_K(GenericPackage, Name), L"D3 ingested material"));
    /* Name will be updated when complete_archive_mxf_file() is called */


    /* Preface - ContentStorage - MaterialPackage - Timecode Track */    
    CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(Track), &newOutput->materialPackageTrackSet));
    CHK_OFAIL(mxf_add_array_item_strongref(newOutput->materialPackageSet, &MXF_ITEM_K(GenericPackage, Tracks), newOutput->materialPackageTrackSet));
    sprintf(cNameBuffer, "TC%d", 1);
    mbstowcs(wNameBuffer, cNameBuffer, NAME_BUFFER_SIZE);
    CHK_OFAIL(mxf_set_utf16string_item(newOutput->materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackName), wNameBuffer));
    CHK_OFAIL(mxf_set_uint32_item(newOutput->materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackID), 1));
    CHK_OFAIL(mxf_set_uint32_item(newOutput->materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), 0));
    CHK_OFAIL(mxf_set_rational_item(newOutput->materialPackageTrackSet, &MXF_ITEM_K(Track, EditRate), &g_videoEditRate));
    CHK_OFAIL(mxf_set_position_item(newOutput->materialPackageTrackSet, &MXF_ITEM_K(Track, Origin), 0));

    /* Preface - ContentStorage - MaterialPackage - Timecode Track - TimecodeComponent */    
    CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(TimecodeComponent), &newOutput->timecodeComponentSet));
    CHK_OFAIL(mxf_set_strongref_item(newOutput->materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), newOutput->timecodeComponentSet));
    CHK_OFAIL(mxf_set_ul_item(newOutput->timecodeComponentSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(Timecode)));
    CHK_OFAIL(mxf_set_length_item(newOutput->timecodeComponentSet, &MXF_ITEM_K(StructuralComponent, Duration), -1)); /* updated when writing completed */
    CHK_OFAIL(mxf_set_uint16_item(newOutput->timecodeComponentSet, &MXF_ITEM_K(TimecodeComponent, RoundedTimecodeBase), 25));
    CHK_OFAIL(mxf_set_boolean_item(newOutput->timecodeComponentSet, &MXF_ITEM_K(TimecodeComponent, DropFrame), 0));
    CHK_OFAIL(mxf_set_position_item(newOutput->timecodeComponentSet, &MXF_ITEM_K(TimecodeComponent, StartTimecode), 0));
                
    CHK_OFAIL(mxf_get_item(newOutput->timecodeComponentSet, &MXF_ITEM_K(StructuralComponent, Duration), &newOutput->durationItems[newOutput->numDurationItems++]));

    
    /* Preface - ContentStorage - MaterialPackage - Timeline Tracks */    
    for (i = 0; i < newOutput->numAudioTracks + 1; i++)
    {
        int isPicture = (i == 0);
        
        /* Preface - ContentStorage - MaterialPackage - Timeline Track */    
        CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(Track), &newOutput->materialPackageTrackSet));
        CHK_OFAIL(mxf_add_array_item_strongref(newOutput->materialPackageSet, &MXF_ITEM_K(GenericPackage, Tracks), newOutput->materialPackageTrackSet));
        if (isPicture)
        {
            sprintf(cNameBuffer, "V%d", 1);
            mbstowcs(wNameBuffer, cNameBuffer, NAME_BUFFER_SIZE);
            CHK_OFAIL(mxf_set_utf16string_item(newOutput->materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackName), wNameBuffer));
        }
        else
        {
            sprintf(cNameBuffer, "A%d", i);
            mbstowcs(wNameBuffer, cNameBuffer, NAME_BUFFER_SIZE);
            CHK_OFAIL(mxf_set_utf16string_item(newOutput->materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackName), wNameBuffer));
        }
        CHK_OFAIL(mxf_set_uint32_item(newOutput->materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackID), i + 2));
        CHK_OFAIL(mxf_set_uint32_item(newOutput->materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), 0));
        if (isPicture)
        {
            CHK_OFAIL(mxf_set_rational_item(newOutput->materialPackageTrackSet, &MXF_ITEM_K(Track, EditRate), &g_videoEditRate));
        }
        else
        {
            CHK_OFAIL(mxf_set_rational_item(newOutput->materialPackageTrackSet, &MXF_ITEM_K(Track, EditRate), &g_audioEditRate));
        }
        CHK_OFAIL(mxf_set_position_item(newOutput->materialPackageTrackSet, &MXF_ITEM_K(Track, Origin), 0));
    
        /* Preface - ContentStorage - MaterialPackage - Timeline Track - Sequence */    
        CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(Sequence), &newOutput->sequenceSet));
        CHK_OFAIL(mxf_set_strongref_item(newOutput->materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), newOutput->sequenceSet));
        if (isPicture)
        {
            CHK_OFAIL(mxf_set_ul_item(newOutput->sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(Picture)));
            CHK_OFAIL(mxf_set_length_item(newOutput->sequenceSet, &MXF_ITEM_K(StructuralComponent, Duration), -1)); /* updated when writing completed */
        }
        else
        {
            CHK_OFAIL(mxf_set_ul_item(newOutput->sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(Sound)));
            CHK_OFAIL(mxf_set_length_item(newOutput->sequenceSet, &MXF_ITEM_K(StructuralComponent, Duration), -1)); /* updated when writing completed */
        }

        CHK_OFAIL(mxf_get_item(newOutput->sequenceSet, &MXF_ITEM_K(StructuralComponent, Duration), &newOutput->durationItems[newOutput->numDurationItems++]));
        
        /* Preface - ContentStorage - MaterialPackage - Timeline Track - Sequence - SourceClip */    
        CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(SourceClip), &newOutput->sourceClipSet));
        CHK_OFAIL(mxf_add_array_item_strongref(newOutput->sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), newOutput->sourceClipSet));
        if (isPicture)
        {
            CHK_OFAIL(mxf_set_ul_item(newOutput->sourceClipSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(Picture)));
            CHK_OFAIL(mxf_set_length_item(newOutput->sourceClipSet, &MXF_ITEM_K(StructuralComponent, Duration), -1)); /* updated when writing completed */
        }
        else
        {
            CHK_OFAIL(mxf_set_ul_item(newOutput->sourceClipSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(Sound)));
            CHK_OFAIL(mxf_set_length_item(newOutput->sourceClipSet, &MXF_ITEM_K(StructuralComponent, Duration), -1)); /* updated when writing completed */
        }
        CHK_OFAIL(mxf_set_position_item(newOutput->sourceClipSet, &MXF_ITEM_K(SourceClip, StartPosition), 0));
        CHK_OFAIL(mxf_set_uint32_item(newOutput->sourceClipSet, &MXF_ITEM_K(SourceClip, SourceTrackID), i + 1));
        CHK_OFAIL(mxf_set_umid_item(newOutput->sourceClipSet, &MXF_ITEM_K(SourceClip, SourcePackageID), &fileSourcePackageUID));

        CHK_OFAIL(mxf_get_item(newOutput->sourceClipSet, &MXF_ITEM_K(StructuralComponent, Duration), &newOutput->durationItems[newOutput->numDurationItems++]));
    }

    
    
    /* Preface - ContentStorage - SourcePackage */
    CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(SourcePackage), &newOutput->sourcePackageSet));
    CHK_OFAIL(mxf_add_array_item_strongref(newOutput->contentStorageSet, &MXF_ITEM_K(ContentStorage, Packages), newOutput->sourcePackageSet));
    CHK_OFAIL(mxf_set_umid_item(newOutput->sourcePackageSet, &MXF_ITEM_K(GenericPackage, PackageUID), &fileSourcePackageUID));
    CHK_OFAIL(mxf_set_timestamp_item(newOutput->sourcePackageSet, &MXF_ITEM_K(GenericPackage, PackageCreationDate), &newOutput->now));
    CHK_OFAIL(mxf_set_timestamp_item(newOutput->sourcePackageSet, &MXF_ITEM_K(GenericPackage, PackageModifiedDate), &newOutput->now));


    /* Preface - ContentStorage - SourcePackage - Timeline Tracks */    
    for (i = 0; i < newOutput->numAudioTracks + 1; i++)
    {
        int isPicture = (i == 0);
        
        /* Preface - ContentStorage - SourcePackage - Timeline Track */    
        CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(Track), &newOutput->sourcePackageTrackSet));
        CHK_OFAIL(mxf_add_array_item_strongref(newOutput->sourcePackageSet, &MXF_ITEM_K(GenericPackage, Tracks), newOutput->sourcePackageTrackSet));
        if (isPicture)
        {
            sprintf(cNameBuffer, "V%d", 1);
            mbstowcs(wNameBuffer, cNameBuffer, NAME_BUFFER_SIZE);
            CHK_OFAIL(mxf_set_utf16string_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackName), wNameBuffer));
        }
        else
        {
            sprintf(cNameBuffer, "A%d", i);
            mbstowcs(wNameBuffer, cNameBuffer, NAME_BUFFER_SIZE);
            CHK_OFAIL(mxf_set_utf16string_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackName), wNameBuffer));
        }
        CHK_OFAIL(mxf_set_uint32_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackID), i + 1));
        if (isPicture)
        {
            mxf_complete_essence_element_track_num(&videoTrackNum, 1, MXF_UNC_FRAME_WRAPPED_EE_TYPE, 1);
            CHK_OFAIL(mxf_set_uint32_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), videoTrackNum));
            CHK_OFAIL(mxf_set_rational_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(Track, EditRate), &g_videoEditRate));
        }
        else
        {
            mxf_complete_essence_element_track_num(&audioTrackNum, newOutput->numAudioTracks, MXF_BWF_FRAME_WRAPPED_EE_TYPE, i);
            CHK_OFAIL(mxf_set_uint32_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), audioTrackNum));
            CHK_OFAIL(mxf_set_rational_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(Track, EditRate), &g_audioEditRate));
        }
        CHK_OFAIL(mxf_set_position_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(Track, Origin), 0));
    
        /* Preface - ContentStorage - SourcePackage - Timeline Track - Sequence */    
        CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(Sequence), &newOutput->sequenceSet));
        CHK_OFAIL(mxf_set_strongref_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), newOutput->sequenceSet));
        if (isPicture)
        {
            CHK_OFAIL(mxf_set_ul_item(newOutput->sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(Picture)));
            CHK_OFAIL(mxf_set_length_item(newOutput->sequenceSet, &MXF_ITEM_K(StructuralComponent, Duration), -1)); /* updated when writing completed */
        }
        else
        {
            CHK_OFAIL(mxf_set_ul_item(newOutput->sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(Sound)));
            CHK_OFAIL(mxf_set_length_item(newOutput->sequenceSet, &MXF_ITEM_K(StructuralComponent, Duration), -1)); /* updated when writing completed */
        }

        CHK_OFAIL(mxf_get_item(newOutput->sequenceSet, &MXF_ITEM_K(StructuralComponent, Duration), &newOutput->durationItems[newOutput->numDurationItems++]));
        
        /* Preface - ContentStorage - SourcePackage - Timeline Track - Sequence - SourceClip */    
        CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(SourceClip), &newOutput->sourceClipSet));
        CHK_OFAIL(mxf_add_array_item_strongref(newOutput->sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), newOutput->sourceClipSet));
        if (isPicture)
        {
            CHK_OFAIL(mxf_set_ul_item(newOutput->sourceClipSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(Picture)));
            CHK_OFAIL(mxf_set_length_item(newOutput->sourceClipSet, &MXF_ITEM_K(StructuralComponent, Duration), -1)); /* updated when writing completed */
            CHK_OFAIL(mxf_set_uint32_item(newOutput->sourceClipSet, &MXF_ITEM_K(SourceClip, SourceTrackID), 1));
            CHK_OFAIL(mxf_set_position_item(newOutput->sourceClipSet, &MXF_ITEM_K(SourceClip, StartPosition), getPosition(startPosition, &g_videoEditRate)));
        }
        else
        {
            CHK_OFAIL(mxf_set_ul_item(newOutput->sourceClipSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(Sound)));
            CHK_OFAIL(mxf_set_length_item(newOutput->sourceClipSet, &MXF_ITEM_K(StructuralComponent, Duration), -1)); /* updated when writing completed */
            CHK_OFAIL(mxf_set_uint32_item(newOutput->sourceClipSet, &MXF_ITEM_K(SourceClip, SourceTrackID), i + 1));
            CHK_OFAIL(mxf_set_position_item(newOutput->sourceClipSet, &MXF_ITEM_K(SourceClip, StartPosition), getPosition(startPosition, &g_audioEditRate)));
        }
        CHK_OFAIL(mxf_set_umid_item(newOutput->sourceClipSet, &MXF_ITEM_K(SourceClip, SourcePackageID), &tapeSourcePackageUID));

        CHK_OFAIL(mxf_get_item(newOutput->sourceClipSet, &MXF_ITEM_K(StructuralComponent, Duration), &newOutput->durationItems[newOutput->numDurationItems++]));
    }

    /* Preface - ContentStorage - SourcePackage - MultipleDescriptor */    
    CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(MultipleDescriptor), &newOutput->multipleDescriptorSet));
    CHK_OFAIL(mxf_set_strongref_item(newOutput->sourcePackageSet, &MXF_ITEM_K(SourcePackage, Descriptor), newOutput->multipleDescriptorSet));
    CHK_OFAIL(mxf_set_rational_item(newOutput->multipleDescriptorSet, &MXF_ITEM_K(FileDescriptor, SampleRate), &g_videoSampleRate));
    CHK_OFAIL(mxf_set_ul_item(newOutput->multipleDescriptorSet, &MXF_ITEM_K(FileDescriptor, EssenceContainer), &MXF_EC_L(MultipleWrappings)));
    
    newOutput->descriptorSets[newOutput->numDescriptorSets++] = newOutput->multipleDescriptorSet;  /* ContainerDuration updated when writing completed */

    
    /* Preface - ContentStorage - SourcePackage - MultipleDescriptor - NetworkLocator */    
    CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(NetworkLocator), &newOutput->networkLocatorSet));
    CHK_OFAIL(mxf_add_array_item_strongref(newOutput->multipleDescriptorSet, &MXF_ITEM_K(GenericDescriptor, Locators), newOutput->networkLocatorSet));
    CHK_OFAIL(convert_string(filename, &newOutput->tempString));
    CHK_OFAIL(mxf_set_fixed_size_utf16string_item(newOutput->networkLocatorSet, &MXF_ITEM_K(NetworkLocator, URLString), newOutput->tempString, NETWORK_LOCATOR_URL_SIZE));

    
    /* Preface - ContentStorage - SourcePackage - MultipleDescriptor - CDCIEssenceDescriptor */    
    CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(CDCIEssenceDescriptor), &newOutput->cdciDescriptorSet));
    CHK_OFAIL(mxf_add_array_item_strongref(newOutput->multipleDescriptorSet, &MXF_ITEM_K(MultipleDescriptor, SubDescriptorUIDs), newOutput->cdciDescriptorSet));
    CHK_OFAIL(mxf_set_uint32_item(newOutput->cdciDescriptorSet, &MXF_ITEM_K(FileDescriptor, LinkedTrackID), 1));
    CHK_OFAIL(mxf_set_rational_item(newOutput->cdciDescriptorSet, &MXF_ITEM_K(FileDescriptor, SampleRate), &g_videoSampleRate));
    CHK_OFAIL(mxf_set_ul_item(newOutput->cdciDescriptorSet, &MXF_ITEM_K(FileDescriptor, EssenceContainer), &MXF_EC_L(SD_Unc_625_50i_422_135_FrameWrapped)));
    CHK_OFAIL(mxf_set_uint8_item(newOutput->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameLayout), g_videoFrameLayout));
    CHK_OFAIL(mxf_set_uint32_item(newOutput->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, StoredHeight), g_videoStoredHeight));
    CHK_OFAIL(mxf_set_uint32_item(newOutput->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, StoredWidth), g_videoStoredWidth));
    CHK_OFAIL(mxf_alloc_array_item_elements(newOutput->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, VideoLineMap), 4, 2, &arrayElement));
    mxf_set_int32(g_videoLineMap[0], arrayElement);
    mxf_set_int32(g_videoLineMap[1], &arrayElement[4]);
    CHK_OFAIL(mxf_set_rational_item(newOutput->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, AspectRatio), &g_videoAspectRatio));
    CHK_OFAIL(mxf_set_uint32_item(newOutput->cdciDescriptorSet, &MXF_ITEM_K(CDCIEssenceDescriptor, ComponentDepth), g_videoComponentDepth));
    CHK_OFAIL(mxf_set_uint32_item(newOutput->cdciDescriptorSet, &MXF_ITEM_K(CDCIEssenceDescriptor, HorizontalSubsampling), g_videoHorizontalSubSampling));
    CHK_OFAIL(mxf_set_uint32_item(newOutput->cdciDescriptorSet, &MXF_ITEM_K(CDCIEssenceDescriptor, VerticalSubsampling), g_videoVerticalSubSampling));

    newOutput->descriptorSets[newOutput->numDescriptorSets++] = newOutput->cdciDescriptorSet;  /* ContainerDuration updated when writing completed */

    
    
    for (i = 0; i < newOutput->numAudioTracks; i++)
    {
        /* Preface - ContentStorage - SourcePackage - MultipleDescriptor - WAVEssenceDescriptor */    
        CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(WaveAudioDescriptor), &newOutput->bwfDescriptorSet));
        CHK_OFAIL(mxf_add_array_item_strongref(newOutput->multipleDescriptorSet, &MXF_ITEM_K(MultipleDescriptor, SubDescriptorUIDs), newOutput->bwfDescriptorSet));
        CHK_OFAIL(mxf_set_uint32_item(newOutput->bwfDescriptorSet, &MXF_ITEM_K(FileDescriptor, LinkedTrackID), i + 2));
        CHK_OFAIL(mxf_set_rational_item(newOutput->bwfDescriptorSet, &MXF_ITEM_K(FileDescriptor, SampleRate), &g_audioEditRate)); 
        CHK_OFAIL(mxf_set_ul_item(newOutput->bwfDescriptorSet, &MXF_ITEM_K(FileDescriptor, EssenceContainer), &MXF_EC_L(BWFFrameWrapped)));
        CHK_OFAIL(mxf_set_rational_item(newOutput->bwfDescriptorSet, &MXF_ITEM_K(GenericSoundEssenceDescriptor, AudioSamplingRate), &g_audioSampleRate));
        CHK_OFAIL(mxf_set_boolean_item(newOutput->bwfDescriptorSet, &MXF_ITEM_K(GenericSoundEssenceDescriptor, Locked), 1));
        CHK_OFAIL(mxf_set_uint32_item(newOutput->bwfDescriptorSet, &MXF_ITEM_K(GenericSoundEssenceDescriptor, ChannelCount), 1));
        CHK_OFAIL(mxf_set_uint32_item(newOutput->bwfDescriptorSet, &MXF_ITEM_K(GenericSoundEssenceDescriptor, QuantizationBits), g_audioQuantBits));
        CHK_OFAIL(mxf_set_uint16_item(newOutput->bwfDescriptorSet, &MXF_ITEM_K(WaveAudioDescriptor, BlockAlign), g_audioBlockAlign));
        CHK_OFAIL(mxf_set_uint32_item(newOutput->bwfDescriptorSet, &MXF_ITEM_K(WaveAudioDescriptor, AvgBps), g_audioAvgBps));

        newOutput->descriptorSets[newOutput->numDescriptorSets++] = newOutput->bwfDescriptorSet;  /* ContainerDuration updated when writing completed */
    }

    /* Preface - ContentStorage - SourcePackage - DM Track */    
    CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(StaticTrack), &newOutput->sourcePackageTrackSet));
    CHK_OFAIL(mxf_add_array_item_strongref(newOutput->sourcePackageSet, &MXF_ITEM_K(GenericPackage, Tracks), newOutput->sourcePackageTrackSet));
    sprintf(cNameBuffer, "DM%d", 1);
    mbstowcs(wNameBuffer, cNameBuffer, NAME_BUFFER_SIZE);
    CHK_OFAIL(mxf_set_utf16string_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackName), wNameBuffer));
    CHK_OFAIL(mxf_set_uint32_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackID), newOutput->numAudioTracks + 2));
    CHK_OFAIL(mxf_set_uint32_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), 0));

    /* Preface - ContentStorage - SourcePackage - DM Track - Sequence */    
    CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(Sequence), &newOutput->sequenceSet));
    CHK_OFAIL(mxf_set_strongref_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), newOutput->sequenceSet));
    CHK_OFAIL(mxf_set_ul_item(newOutput->sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(DescriptiveMetadata)));

    /* Preface - ContentStorage - SourcePackage - DM Track - Sequence - DMSegment */    
    CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(DMSegment), &newOutput->dmSet));
    CHK_OFAIL(mxf_add_array_item_strongref(newOutput->sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), newOutput->dmSet));
    CHK_OFAIL(mxf_set_ul_item(newOutput->dmSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(DescriptiveMetadata)));
    
    /* Preface - ContentStorage - SourcePackage - DM Track - Sequence - DMSegment - DMFramework */    
    CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(D3P_InfaxFramework), &newOutput->fileDMFrameworkSet));
    CHK_OFAIL(mxf_set_strongref_item(newOutput->dmSet, &MXF_ITEM_K(DMSegment, DMFramework), newOutput->fileDMFrameworkSet));
    mxf_set_fixed_set_space_allocation(newOutput->fileDMFrameworkSet, g_fixedInfaxSetAllocationSize);
    CHK_OFAIL(set_infax_data(newOutput->fileDMFrameworkSet, &nullInfaxData));
    /* D3P_InfaxFramework will be filled in when update_mxf_file() is called */    

    
    /* Preface - ContentStorage - tape SourcePackage */
    CHK_ORET(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(SourcePackage), &newOutput->tapeSourcePackageSet));
    CHK_ORET(mxf_add_array_item_strongref(newOutput->contentStorageSet, &MXF_ITEM_K(ContentStorage, Packages), newOutput->tapeSourcePackageSet));
    CHK_ORET(mxf_set_umid_item(newOutput->tapeSourcePackageSet, &MXF_ITEM_K(GenericPackage, PackageUID), &tapeSourcePackageUID));
    CHK_ORET(mxf_set_timestamp_item(newOutput->tapeSourcePackageSet, &MXF_ITEM_K(GenericPackage, PackageCreationDate), &newOutput->now));
    CHK_ORET(mxf_set_timestamp_item(newOutput->tapeSourcePackageSet, &MXF_ITEM_K(GenericPackage, PackageModifiedDate), &newOutput->now));
    CHK_ORET(mxf_set_utf16string_item(newOutput->tapeSourcePackageSet, &MXF_ITEM_K(GenericPackage, Name), L"D3 tape"));
    /* Name will be updated when complete_archive_mxf_file() is called */

    /* Preface - ContentStorage - tape SourcePackage - video Timeline Track */    
    CHK_ORET(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(Track), &newOutput->sourcePackageTrackSet));
    CHK_ORET(mxf_add_array_item_strongref(newOutput->tapeSourcePackageSet, &MXF_ITEM_K(GenericPackage, Tracks), newOutput->sourcePackageTrackSet));
    CHK_ORET(mxf_set_uint32_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackID), 1));
    sprintf(cNameBuffer, "V%d", 1);
    mbstowcs(wNameBuffer, cNameBuffer, NAME_BUFFER_SIZE);
    CHK_ORET(mxf_set_utf16string_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackName), wNameBuffer));
    CHK_ORET(mxf_set_uint32_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), 1));
    CHK_ORET(mxf_set_rational_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(Track, EditRate), &g_videoEditRate));
    CHK_ORET(mxf_set_position_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(Track, Origin), 0));

    /* Preface - ContentStorage - tape SourcePackage - video Timeline Track - Sequence */    
    CHK_ORET(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(Sequence), &newOutput->sequenceSet));
    CHK_ORET(mxf_set_strongref_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), newOutput->sequenceSet));
    CHK_ORET(mxf_set_ul_item(newOutput->sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(Picture)));
    CHK_ORET(mxf_set_length_item(newOutput->sequenceSet, &MXF_ITEM_K(StructuralComponent, Duration), g_tapeLen));

    /* Preface - ContentStorage - tape SourcePackage - video Timeline Track - Sequence - SourceClip */    
    CHK_ORET(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(SourceClip), &newOutput->sourceClipSet));
    CHK_ORET(mxf_add_array_item_strongref(newOutput->sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), newOutput->sourceClipSet));
    CHK_ORET(mxf_set_ul_item(newOutput->sourceClipSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(Picture)));
    CHK_ORET(mxf_set_length_item(newOutput->sourceClipSet, &MXF_ITEM_K(StructuralComponent, Duration), g_tapeLen));
    CHK_ORET(mxf_set_position_item(newOutput->sourceClipSet, &MXF_ITEM_K(SourceClip, StartPosition), 0));
    CHK_ORET(mxf_set_umid_item(newOutput->sourceClipSet, &MXF_ITEM_K(SourceClip, SourcePackageID), &g_Null_UMID));
    CHK_ORET(mxf_set_uint32_item(newOutput->sourceClipSet, &MXF_ITEM_K(SourceClip, SourceTrackID), 0));

    
    for (i = 0; i < g_numTapeAudioTracks; i++)
    {
        /* Preface - ContentStorage - tape SourcePackage - audio Timeline Track */    
        CHK_ORET(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(Track), &newOutput->sourcePackageTrackSet));
        CHK_ORET(mxf_add_array_item_strongref(newOutput->tapeSourcePackageSet, &MXF_ITEM_K(GenericPackage, Tracks), newOutput->sourcePackageTrackSet));
        CHK_ORET(mxf_set_uint32_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackID), i + 2));
        sprintf(cNameBuffer, "A%d", i + 1);
        mbstowcs(wNameBuffer, cNameBuffer, NAME_BUFFER_SIZE);
        CHK_ORET(mxf_set_utf16string_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackName), wNameBuffer));
        CHK_ORET(mxf_set_uint32_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), i + 1));
        CHK_ORET(mxf_set_rational_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(Track, EditRate), &g_videoEditRate));
        CHK_ORET(mxf_set_position_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(Track, Origin), 0));
    
        /* Preface - ContentStorage - tape SourcePackage - audio Timeline Track - Sequence */    
        CHK_ORET(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(Sequence), &newOutput->sequenceSet));
        CHK_ORET(mxf_set_strongref_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), newOutput->sequenceSet));
        CHK_ORET(mxf_set_ul_item(newOutput->sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(Sound)));
        CHK_ORET(mxf_set_length_item(newOutput->sequenceSet, &MXF_ITEM_K(StructuralComponent, Duration), g_tapeLen));
    
        /* Preface - ContentStorage - tape SourcePackage - audio Timeline Track - Sequence - SourceClip */    
        CHK_ORET(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(SourceClip), &newOutput->sourceClipSet));
        CHK_ORET(mxf_add_array_item_strongref(newOutput->sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), newOutput->sourceClipSet));
        CHK_ORET(mxf_set_ul_item(newOutput->sourceClipSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(Sound)));
        CHK_ORET(mxf_set_length_item(newOutput->sourceClipSet, &MXF_ITEM_K(StructuralComponent, Duration), g_tapeLen));
        CHK_ORET(mxf_set_position_item(newOutput->sourceClipSet, &MXF_ITEM_K(SourceClip, StartPosition), 0));
        CHK_ORET(mxf_set_umid_item(newOutput->sourceClipSet, &MXF_ITEM_K(SourceClip, SourcePackageID), &g_Null_UMID));
        CHK_ORET(mxf_set_uint32_item(newOutput->sourceClipSet, &MXF_ITEM_K(SourceClip, SourceTrackID), 0));
    }

    /* Preface - ContentStorage - tape SourcePackage - timecode Timeline Track */    
    CHK_ORET(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(Track), &newOutput->sourcePackageTrackSet));
    CHK_ORET(mxf_add_array_item_strongref(newOutput->tapeSourcePackageSet, &MXF_ITEM_K(GenericPackage, Tracks), newOutput->sourcePackageTrackSet));
    CHK_ORET(mxf_set_uint32_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackID), g_numTapeAudioTracks + 2));
    CHK_ORET(mxf_set_uint32_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), 0));
    sprintf(cNameBuffer, "T%d", 1);
    mbstowcs(wNameBuffer, cNameBuffer, NAME_BUFFER_SIZE);
    CHK_ORET(mxf_set_utf16string_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackName), wNameBuffer));
    CHK_ORET(mxf_set_rational_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(Track, EditRate), &g_videoEditRate));
    CHK_ORET(mxf_set_position_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(Track, Origin), 0));

    /* Preface - ContentStorage - tape SourcePackage - timecode Timeline Track - Sequence */    
    CHK_ORET(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(Sequence), &newOutput->sequenceSet));
    CHK_ORET(mxf_set_strongref_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), newOutput->sequenceSet));
    CHK_ORET(mxf_set_ul_item(newOutput->sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(Timecode)));
    CHK_ORET(mxf_set_length_item(newOutput->sequenceSet, &MXF_ITEM_K(StructuralComponent, Duration), g_tapeLen));

    /* Preface - ContentStorage - tape SourcePackage - Timecode Track - Sequence - TimecodeComponent */    
    CHK_ORET(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(TimecodeComponent), &newOutput->timecodeComponentSet));
    CHK_ORET(mxf_add_array_item_strongref(newOutput->sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), newOutput->timecodeComponentSet));
    CHK_ORET(mxf_set_ul_item(newOutput->timecodeComponentSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(Timecode)));
    CHK_ORET(mxf_set_length_item(newOutput->timecodeComponentSet, &MXF_ITEM_K(StructuralComponent, Duration), g_tapeLen));
    CHK_ORET(mxf_set_uint16_item(newOutput->timecodeComponentSet, &MXF_ITEM_K(TimecodeComponent, RoundedTimecodeBase), 25));
    CHK_ORET(mxf_set_boolean_item(newOutput->timecodeComponentSet, &MXF_ITEM_K(TimecodeComponent, DropFrame), 0));
    CHK_ORET(mxf_set_position_item(newOutput->timecodeComponentSet, &MXF_ITEM_K(TimecodeComponent, StartTimecode), 0));

    
    /* Preface - ContentStorage - tape SourcePackage - DM Track */    
    CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(StaticTrack), &newOutput->sourcePackageTrackSet));
    CHK_OFAIL(mxf_add_array_item_strongref(newOutput->tapeSourcePackageSet, &MXF_ITEM_K(GenericPackage, Tracks), newOutput->sourcePackageTrackSet));
    sprintf(cNameBuffer, "DM%d", 1);
    mbstowcs(wNameBuffer, cNameBuffer, NAME_BUFFER_SIZE);
    CHK_OFAIL(mxf_set_utf16string_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackName), wNameBuffer));
    CHK_OFAIL(mxf_set_uint32_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackID), g_numTapeAudioTracks + 3));
    CHK_OFAIL(mxf_set_uint32_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), 0));

    /* Preface - ContentStorage - tape SourcePackage - DM Track - Sequence */    
    CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(Sequence), &newOutput->sequenceSet));
    CHK_OFAIL(mxf_set_strongref_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), newOutput->sequenceSet));
    CHK_OFAIL(mxf_set_ul_item(newOutput->sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(DescriptiveMetadata)));

    /* Preface - ContentStorage - tape SourcePackage - DM Track - Sequence - DMSegment */    
    CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(DMSegment), &newOutput->dmSet));
    CHK_OFAIL(mxf_add_array_item_strongref(newOutput->sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), newOutput->dmSet));
    CHK_OFAIL(mxf_set_ul_item(newOutput->dmSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(DescriptiveMetadata)));
    
    /* Preface - ContentStorage - tape SourcePackage - DM Track - Sequence - DMSegment - DMFramework */    
    CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(D3P_InfaxFramework), &newOutput->tapeDMFrameworkSet));
    CHK_OFAIL(mxf_set_strongref_item(newOutput->dmSet, &MXF_ITEM_K(DMSegment, DMFramework), newOutput->tapeDMFrameworkSet));
    mxf_set_fixed_set_space_allocation(newOutput->tapeDMFrameworkSet, g_fixedInfaxSetAllocationSize);
    /* D3P_InfaxFramework will be completed in when complete_archive_mxf_file() is called */    
    

    /* Preface - ContentStorage - tape SourcePackage - TapeDescriptor */
    CHK_ORET(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(TapeDescriptor), &newOutput->tapeDescriptorSet));
    CHK_ORET(mxf_set_strongref_item(newOutput->tapeSourcePackageSet, &MXF_ITEM_K(SourcePackage, Descriptor), newOutput->tapeDescriptorSet));
    
    
        
    /*
     * Write the Header Metadata
     */
    
    CHK_OFAIL(mxf_mark_header_start(newOutput->mxfFile, newOutput->headerPartition));
    CHK_OFAIL(mxf_write_header_metadata(newOutput->mxfFile, newOutput->headerMetadata));
    CHK_OFAIL(mxf_mark_header_end(newOutput->mxfFile, newOutput->headerPartition));
    

    /*
     * Write the index table segment
     */
    
    CHK_OFAIL(mxf_mark_index_start(newOutput->mxfFile, newOutput->headerPartition));
    
    CHK_OFAIL(mxf_create_index_table_segment(&newOutput->indexSegment)); 
    mxf_generate_uuid(&newOutput->indexSegment->instanceUID);
    newOutput->indexSegment->indexEditRate = g_videoEditRate;
    newOutput->indexSegment->indexDuration = 0; /* although valid, will be updated when writing completed */
    newOutput->indexSegment->editUnitByteCount = mxfKey_extlen + 4 + SYSTEM_ITEM_SIZE +
        mxfKey_extlen + 4 + g_videoFrameSize +  
        newOutput->numAudioTracks * (mxfKey_extlen + 4 + g_audioFrameSize);
    newOutput->indexSegment->indexSID = g_indexSID;
    newOutput->indexSegment->bodySID = g_bodySID;
    deltaOffset = 0;
    CHK_OFAIL(mxf_add_delta_entry(newOutput->indexSegment, 0, 0, deltaOffset));
    deltaOffset += mxfKey_extlen + 4 + SYSTEM_ITEM_SIZE;
    CHK_OFAIL(mxf_add_delta_entry(newOutput->indexSegment, 0, 0, deltaOffset));
    deltaOffset += mxfKey_extlen + 4 + g_videoFrameSize;
    for (i = 0; i < newOutput->numAudioTracks; i++)
    {
        CHK_OFAIL(mxf_add_delta_entry(newOutput->indexSegment, 0, 0, deltaOffset));
        deltaOffset += mxfKey_extlen + 4 + g_audioFrameSize;
    }

    CHK_OFAIL(mxf_write_index_table_segment(newOutput->mxfFile, newOutput->indexSegment));
    
    /* allocate space for any future header metadata extensions */
    CHK_OFAIL((filePos = mxf_file_tell(newOutput->mxfFile)) >= 0);
    CHK_OFAIL((uint64_t)filePos < g_fixedBodyOffset - 17); /* min fill is 17 */
    CHK_OFAIL(mxf_fill_to_position(newOutput->mxfFile, g_fixedBodyOffset));

    CHK_OFAIL(mxf_mark_index_end(newOutput->mxfFile, newOutput->headerPartition));

    
    /*
     * Update the header partition pack
     */
    
    CHK_ORET(mxf_update_partitions(newOutput->mxfFile, newOutput->partitions));
    
    
    *output = newOutput;
    return 1;
    
fail:
    free_d3_mxf_file(&newOutput);
    return 0;
}


int write_timecode(ArchiveMXFWriter* output, ArchiveTimecode vitc, ArchiveTimecode ltc)
{
    uint8_t t12m[8];
    
    CHK_ORET(verify_essence_write_state(output, 1, 0, 0));
    
    CHK_ORET(add_timecode(&output->vitcIndex, &vitc));
    CHK_ORET(add_timecode(&output->ltcIndex, &ltc));

    CHK_ORET(mxf_write_fixed_kl(output->mxfFile, &g_TimecodeSysItemElementKey, 4, SYSTEM_ITEM_SIZE));
    
    CHK_ORET(mxf_write_uint16(output->mxfFile, 0x0102)); /* local tag */
    CHK_ORET(mxf_write_uint16(output->mxfFile, SYSTEM_ITEM_SIZE - 4)); /* len */
    CHK_ORET(mxf_write_array_header(output->mxfFile, 2, 8)); /* VITC and LTC SMPTE-12M timecodes */

    convert_timecode_to_12m(&vitc, t12m);
    CHK_ORET(mxf_file_write(output->mxfFile, t12m, 8) == 8);
    convert_timecode_to_12m(&ltc, t12m);
    CHK_ORET(mxf_file_write(output->mxfFile, t12m, 8) == 8);
        
    update_essence_write_state(output, 1, 0, 0);
    
    return 1;
}

int write_video_frame(ArchiveMXFWriter* output, uint8_t* data, uint32_t size)
{
    mxfKey eeKey = g_UncBaseElementKey;

    if (size != g_videoFrameSize)
    {
        mxf_log(MXF_ELOG, "Invalid video frame size %ld; expecting %ld" LOG_LOC_FORMAT, size, g_videoFrameSize, LOG_LOC_PARAMS);
        return 0;
    }
    
    CHK_ORET(verify_essence_write_state(output, 0, 1, 0));
    
    mxf_complete_essence_element_key(&eeKey, 1, MXF_UNC_FRAME_WRAPPED_EE_TYPE, 1);

    CHK_ORET(mxf_write_fixed_kl(output->mxfFile, &eeKey, 4, size));
    CHK_ORET(mxf_file_write(output->mxfFile, data, size) == size);

    output->duration++;
    update_essence_write_state(output, 0, 1, 0);
    
    return 1;
}

int write_audio_frame(ArchiveMXFWriter* output, uint8_t* data, uint32_t size)
{
    mxfKey eeKey = g_WavBaseElementKey;

    if (size != g_audioFrameSize)
    {
        mxf_log(MXF_ELOG, "Invalid audio frame size %ld; expecting %ld" LOG_LOC_FORMAT, size, g_audioFrameSize, LOG_LOC_PARAMS);
        return 0;
    }
    
    CHK_ORET(verify_essence_write_state(output, 0, 0, 1));
    
    mxf_complete_essence_element_key(&eeKey, output->numAudioTracks, MXF_BWF_FRAME_WRAPPED_EE_TYPE, 
        output->essWriteState.audioNum + 1);
    
    CHK_ORET(mxf_write_fixed_kl(output->mxfFile, &eeKey, 4, size));
    CHK_ORET(mxf_file_write(output->mxfFile, data, size) == size);
    
    update_essence_write_state(output, 0, 0, 1);

    return 1;
}

int abort_archive_mxf_file(ArchiveMXFWriter** output)
{
    free_d3_mxf_file(output);
    return 1;
}

int complete_archive_mxf_file(ArchiveMXFWriter** outputRef, const char* d3InfaxDataString,
    const PSEFailure* pseFailures, long numPSEFailures,
    const VTRError* vtrErrors, long numVTRErrors)
{
    ArchiveMXFWriter* output = *outputRef;
    int i;
    int64_t filePos;
    long j;
    MXFListIterator iter;
    const PSEFailure* pseFailure;
    const VTRError* vtrError;
    uint32_t nextTrackID;
    int numTracks;
    TimecodeIndexSearcher vitcIndexSearcher;
    TimecodeIndexSearcher ltcIndexSearcher;
    int64_t errorPosition;
    int locatedAtLeastOneVTRError;
    long errorIndex;
    long failureIndex;
    char mpName[64];
    InfaxData d3InfaxData;
    

    /* update the PSE failure and D3 VTR error counts */
    
    CHK_ORET(mxf_set_uint32_item(output->prefaceSet, &MXF_ITEM_K(Preface, D3P_D3ErrorCount), numVTRErrors));
    CHK_ORET(mxf_set_uint32_item(output->prefaceSet, &MXF_ITEM_K(Preface, D3P_PSEFailureCount), numPSEFailures));
    
    
    /* update the header metadata durations */
    
    for (i = 0; i < output->numDurationItems; i++)
    {
        CHK_ORET(mxf_set_length_item(output->durationItems[i]->set, &output->durationItems[i]->key, output->duration));
    }
    for (i = 0; i < output->numDescriptorSets; i++)
    {
        CHK_ORET(mxf_set_length_item(output->descriptorSets[i], &MXF_ITEM_K(FileDescriptor, ContainerDuration), output->duration));
    }
    
    /* add the D3 tape Infax items to the set */

    CHK_ORET(parse_infax_data(d3InfaxDataString, &d3InfaxData, output->beStrict));
    CHK_ORET(set_infax_data(output->tapeDMFrameworkSet, &d3InfaxData));

    
    /* update the header metadata MaterialPackage and tape SourcePackage Names */

    CHK_ORET(convert_string(d3InfaxData.spoolNo, &output->tempString));
    CHK_ORET(mxf_set_utf16string_item(output->tapeSourcePackageSet, &MXF_ITEM_K(GenericPackage, Name), output->tempString));
    
    strcpy(mpName, "D3 preservation ");
    strcat(mpName, d3InfaxData.spoolNo);
    CHK_ORET(convert_string(mpName, &output->tempString));
    CHK_ORET(mxf_set_utf16string_item(output->materialPackageSet, &MXF_ITEM_K(GenericPackage, Name), output->tempString));
    

    /* re-write the header metadata */
    
    CHK_ORET(mxf_file_seek(output->mxfFile, output->headerMetadataFilePos, SEEK_SET));
    CHK_ORET(mxf_mark_header_start(output->mxfFile, output->headerPartition));
    CHK_ORET(mxf_write_header_metadata(output->mxfFile, output->headerMetadata));
    CHK_ORET(mxf_mark_header_end(output->mxfFile, output->headerPartition));
    

    /* re-write the header index table segment */
    
    CHK_ORET(mxf_mark_index_start(output->mxfFile, output->headerPartition));

    CHK_ORET(mxf_write_index_table_segment(output->mxfFile, output->indexSegment));

    /* fill space to body position */
    CHK_ORET((filePos = mxf_file_tell(output->mxfFile)) >= 0);
    CHK_ORET((uint64_t)filePos < g_fixedBodyOffset - 17); /* min fill is 17 */
    CHK_ORET(mxf_fill_to_position(output->mxfFile, g_fixedBodyOffset));

    CHK_ORET(mxf_mark_index_end(output->mxfFile, output->headerPartition));



    /* Write the Footer Partition Pack */
    
    CHK_ORET(mxf_file_seek(output->mxfFile, 0, SEEK_END));
    CHK_ORET(mxf_append_new_from_partition(output->partitions, output->headerPartition, 
        &output->footerPartition));
    /* partition is open because the LTO Infax data needs to be filled in */
    output->footerPartition->key = MXF_PP_K(OpenComplete, Footer);
    output->footerPartition->indexSID = g_indexSID;

    CHK_ORET(mxf_write_partition(output->mxfFile, output->footerPartition));



    
    /* update the metadata with just the references to the Tracks for the D3 VTR errors and 
       PSE failures */
    /* the individual D3 VTR errors and PSE failures will be written straight to file
       at the end of the header metadata. This is done to avoid the memory hit that results
       from storing the (potentially large) errors and failures metadata sets in memory */
    
    nextTrackID = output->numAudioTracks + 3;

    /* PSE failures Track(s) */
    
    if (numPSEFailures > 0)
    {
        /* if num PSE failures exceeds MAX_STRONG_REF_ARRAY_COUNT then the failures are
           written in > 1 tracks */
        numTracks = 1 + numPSEFailures / MAX_STRONG_REF_ARRAY_COUNT;
        for (i = 0; i < numTracks; i++)
        {
            /* Preface - ContentStorage - SourcePackage - DM Event Track */    
            CHK_ORET(mxf_create_set(output->headerMetadata, &MXF_SET_K(EventTrack), &output->sourcePackageTrackSet));
            CHK_ORET(mxf_add_array_item_strongref(output->sourcePackageSet, &MXF_ITEM_K(GenericPackage, Tracks), output->sourcePackageTrackSet));
            CHK_ORET(mxf_set_utf16string_item(output->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackName), g_pseFailuresTrackName));
            CHK_ORET(mxf_set_uint32_item(output->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackID), nextTrackID++));
            CHK_ORET(mxf_set_uint32_item(output->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), 0));
            CHK_ORET(mxf_set_rational_item(output->sourcePackageTrackSet, &MXF_ITEM_K(EventTrack, EventEditRate), &g_videoEditRate));
            CHK_ORET(mxf_set_position_item(output->sourcePackageTrackSet, &MXF_ITEM_K(EventTrack, EventOrigin), 0));
        
            /* remove Track set from header metadata, leaving just the reference in the sourcePackageSet
               The Track will be written when the failures are written */
            CHK_ORET(mxf_remove_set(output->headerMetadata, output->sourcePackageTrackSet));
            CHK_ORET(mxf_append_list_element(&output->pseFailureTrackSets, output->sourcePackageTrackSet));
        }
    }
    
    /* D3 VTR errors Track(s) */
    
    if (numVTRErrors > 0)
    {
        initialise_timecode_index_searcher(&output->vitcIndex, &vitcIndexSearcher);
        initialise_timecode_index_searcher(&output->ltcIndex, &ltcIndexSearcher);
        
        /* check we can at least find one D3 VTR error position in the first MAXIMUM_ERROR_CHECK */
        /* if we can't then somethng is badly wrong with the rs-232 or SDI links 
           and we decide to store nothing */
        locatedAtLeastOneVTRError = 0;
        for (j = 0; j < MAXIMUM_ERROR_CHECK && j < numVTRErrors; j++)
        {
            if (find_position_at_dual_timecode(&vitcIndexSearcher, &vtrErrors[j].vitcTimecode, 
                &ltcIndexSearcher, &vtrErrors[j].ltcTimecode, &errorPosition))
            {
                locatedAtLeastOneVTRError = 1;
                break;
            }
        }
        
        if (!locatedAtLeastOneVTRError)
        {
            mxf_log(MXF_WLOG, "Failed to find the position of at least one D3 VTR error in first %d "
                "- not recording any errors" LOG_LOC_FORMAT, MAXIMUM_ERROR_CHECK, LOG_LOC_PARAMS);
        }
        else
        {
            /* if numVTRErrors exceeds MAX_STRONG_REF_ARRAY_COUNT then the errors are
               written in > 1 tracks */
            numTracks = 1 + numVTRErrors / MAX_STRONG_REF_ARRAY_COUNT;
            for (j = 0; j < numTracks; j++)
            {
                /* Preface - ContentStorage - SourcePackage - DM Event Track */    
                CHK_ORET(mxf_create_set(output->headerMetadata, &MXF_SET_K(EventTrack), &output->sourcePackageTrackSet));
                CHK_ORET(mxf_add_array_item_strongref(output->sourcePackageSet, &MXF_ITEM_K(GenericPackage, Tracks), output->sourcePackageTrackSet));
                CHK_ORET(mxf_set_utf16string_item(output->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackName), g_vtrErrorsTrackName));
                CHK_ORET(mxf_set_uint32_item(output->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackID), nextTrackID++));
                CHK_ORET(mxf_set_uint32_item(output->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), 0));
                CHK_ORET(mxf_set_rational_item(output->sourcePackageTrackSet, &MXF_ITEM_K(EventTrack, EventEditRate), &g_videoEditRate));
                CHK_ORET(mxf_set_position_item(output->sourcePackageTrackSet, &MXF_ITEM_K(EventTrack, EventOrigin), 0));
                
                /* remove Track set from header metadata, leaving just the reference in the sourcePackageSet
                   The Track will be written when the errors are written */
                CHK_ORET(mxf_remove_set(output->headerMetadata, output->sourcePackageTrackSet));
                CHK_ORET(mxf_append_list_element(&output->d3VTRErrorTrackSets, output->sourcePackageTrackSet));
            }            
            
        }
        
    }


    /* register the PSE failure and D3 VTR error items that will be written later 
       This must be done so that the local tags are registered in the Primer */
    CHK_ORET(mxf_register_set_items(output->headerMetadata, &MXF_SET_K(DMSegment)));
    CHK_ORET(mxf_register_set_items(output->headerMetadata, &MXF_SET_K(D3P_D3ReplayErrorFramework)));
    CHK_ORET(mxf_register_set_items(output->headerMetadata, &MXF_SET_K(D3P_PSEAnalysisFramework)));

    
    /* write the footer header metadata created thus far */
    
    CHK_ORET(mxf_mark_header_start(output->mxfFile, output->footerPartition));
    CHK_ORET(mxf_write_header_metadata(output->mxfFile, output->headerMetadata));
    
    
    /* write the PSE failures Track(s) and child items */
    
    if (mxf_get_list_length(&output->pseFailureTrackSets) > 0)
    {
        failureIndex = 0;
        mxf_initialise_list_iter(&iter, &output->pseFailureTrackSets);
        while (mxf_next_list_iter_element(&iter))
        {
            output->sourcePackageTrackSet = (MXFMetadataSet*)mxf_get_iter_element(&iter);
            CHK_ORET(mxf_add_set(output->headerMetadata, output->sourcePackageTrackSet));
            
            /* Preface - ContentStorage - SourcePackage - DM Event Track - Sequence */    
            CHK_ORET(mxf_create_set(output->headerMetadata, &MXF_SET_K(Sequence), &output->sequenceSet));
            CHK_ORET(mxf_set_strongref_item(output->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), output->sequenceSet));
            CHK_ORET(mxf_set_ul_item(output->sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(DescriptiveMetadata)));
        
            for (j = 0; j < MAX_STRONG_REF_ARRAY_COUNT && failureIndex < numPSEFailures; j++, failureIndex++)
            {
                pseFailure = (const PSEFailure*)&pseFailures[failureIndex];
                
                /* Preface - ContentStorage - SourcePackage - DM Event Track - Sequence - DMSegment */    
                CHK_ORET(mxf_create_set(output->headerMetadata, &MXF_SET_K(DMSegment), &output->dmSet));
                CHK_ORET(mxf_add_array_item_strongref(output->sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), output->dmSet));
                CHK_ORET(mxf_set_ul_item(output->dmSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(DescriptiveMetadata)));
                CHK_ORET(mxf_set_position_item(output->dmSet, &MXF_ITEM_K(DMSegment, EventStartPosition), pseFailure->position));
                CHK_ORET(mxf_set_length_item(output->dmSet, &MXF_ITEM_K(StructuralComponent, Duration), 1));
    
                /* Preface - ContentStorage - SourcePackage - DM Event Track - Sequence - DMSegment - DMFramework */    
                CHK_ORET(mxf_create_set(output->headerMetadata, &MXF_SET_K(D3P_PSEAnalysisFramework), &output->dmFrameworkSet));
                CHK_ORET(mxf_set_strongref_item(output->dmSet, &MXF_ITEM_K(DMSegment, DMFramework), output->dmFrameworkSet));
                CHK_ORET(mxf_set_int16_item(output->dmFrameworkSet, &MXF_ITEM_K(D3P_PSEAnalysisFramework, D3P_RedFlash), pseFailure->redFlash));
                CHK_ORET(mxf_set_int16_item(output->dmFrameworkSet, &MXF_ITEM_K(D3P_PSEAnalysisFramework, D3P_SpatialPattern), pseFailure->spatialPattern));
                CHK_ORET(mxf_set_int16_item(output->dmFrameworkSet, &MXF_ITEM_K(D3P_PSEAnalysisFramework, D3P_LuminanceFlash), pseFailure->luminanceFlash));
                CHK_ORET(mxf_set_boolean_item(output->dmFrameworkSet, &MXF_ITEM_K(D3P_PSEAnalysisFramework, D3P_ExtendedFailure), pseFailure->extendedFailure));
                
                
                /* write the DMSegment and DMFramework to file */
                CHK_ORET(mxf_write_set(output->mxfFile, output->dmSet));
                CHK_ORET(mxf_write_set(output->mxfFile, output->dmFrameworkSet));
                
                /* remove and free the sets to limit memory usage */
                CHK_ORET(mxf_remove_set(output->headerMetadata, output->dmSet));
                mxf_free_set(&output->dmSet);
                CHK_ORET(mxf_remove_set(output->headerMetadata, output->dmFrameworkSet));
                mxf_free_set(&output->dmFrameworkSet);
            }

            /* write the Track and Sequence to file */
            CHK_ORET(mxf_write_set(output->mxfFile, output->sourcePackageTrackSet));
            CHK_ORET(mxf_write_set(output->mxfFile, output->sequenceSet));

            /* remove and free the sets to limit memory usage */
            CHK_ORET(mxf_remove_set(output->headerMetadata, output->sourcePackageTrackSet));
            mxf_free_set(&output->sourcePackageTrackSet);
            CHK_ORET(mxf_remove_set(output->headerMetadata, output->sequenceSet));
            mxf_free_set(&output->sequenceSet);
        }
    }

    
    
    /* write the D3 VTR errors Track(s) and child items */
    
    if (mxf_get_list_length(&output->d3VTRErrorTrackSets) > 0)
    {
        initialise_timecode_index_searcher(&output->vitcIndex, &vitcIndexSearcher);
        initialise_timecode_index_searcher(&output->ltcIndex, &ltcIndexSearcher);
        
        errorIndex = 0;
        mxf_initialise_list_iter(&iter, &output->d3VTRErrorTrackSets);
        while (mxf_next_list_iter_element(&iter))
        {
            output->sourcePackageTrackSet = (MXFMetadataSet*)mxf_get_iter_element(&iter);
            CHK_ORET(mxf_add_set(output->headerMetadata, output->sourcePackageTrackSet));
            
            /* Preface - ContentStorage - SourcePackage - DM Event Track - Sequence */    
            CHK_ORET(mxf_create_set(output->headerMetadata, &MXF_SET_K(Sequence), &output->sequenceSet));
            CHK_ORET(mxf_set_strongref_item(output->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), output->sequenceSet));
            CHK_ORET(mxf_set_ul_item(output->sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(DescriptiveMetadata)));
        
            for (j = 0; j < MAX_STRONG_REF_ARRAY_COUNT && errorIndex < numVTRErrors; j++, errorIndex++)
            {
                vtrError = (const VTRError*)&vtrErrors[errorIndex];
                
                /* find the position associated with the VITC and LTC timecode */
                if (!find_position_at_dual_timecode(&vitcIndexSearcher, &vtrError->vitcTimecode, 
                    &ltcIndexSearcher, &vtrError->ltcTimecode, &errorPosition))
                {
                    mxf_log(MXF_WLOG, "Failed to find the position of the D3 VTR error %ld" LOG_LOC_FORMAT, errorIndex, LOG_LOC_PARAMS);
                    continue;
                }
                
                /* Preface - ContentStorage - SourcePackage - DM Event Track - Sequence - DMSegment */    
                CHK_ORET(mxf_create_set(output->headerMetadata, &MXF_SET_K(DMSegment), &output->dmSet));
                CHK_ORET(mxf_add_array_item_strongref(output->sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), output->dmSet));
                CHK_ORET(mxf_set_ul_item(output->dmSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(DescriptiveMetadata)));
                CHK_ORET(mxf_set_position_item(output->dmSet, &MXF_ITEM_K(DMSegment, EventStartPosition), errorPosition));
                CHK_ORET(mxf_set_length_item(output->dmSet, &MXF_ITEM_K(StructuralComponent, Duration), 1));
                
                /* Preface - ContentStorage - SourcePackage - DM Event Track - Sequence - DMSegment - DMFramework */    
                CHK_ORET(mxf_create_set(output->headerMetadata, &MXF_SET_K(D3P_D3ReplayErrorFramework), &output->dmFrameworkSet));
                CHK_ORET(mxf_set_strongref_item(output->dmSet, &MXF_ITEM_K(DMSegment, DMFramework), output->dmFrameworkSet));
                CHK_ORET(mxf_set_uint8_item(output->dmFrameworkSet, &MXF_ITEM_K(D3P_D3ReplayErrorFramework, D3P_D3ErrorCode), vtrError->errorCode));
                
                
                /* write the DMSegment and DMFramework to file */
                CHK_ORET(mxf_write_set(output->mxfFile, output->dmSet));
                CHK_ORET(mxf_write_set(output->mxfFile, output->dmFrameworkSet));

                /* remove and free the sets to limit memory usage */
                CHK_ORET(mxf_remove_set(output->headerMetadata, output->dmSet));
                mxf_free_set(&output->dmSet);
                CHK_ORET(mxf_remove_set(output->headerMetadata, output->dmFrameworkSet));
                mxf_free_set(&output->dmFrameworkSet);
            }
            
            /* write the Track and Sequence to file */
            CHK_ORET(mxf_write_set(output->mxfFile, output->sourcePackageTrackSet));
            CHK_ORET(mxf_write_set(output->mxfFile, output->sequenceSet));

            /* remove and free the sets to limit memory usage */
            CHK_ORET(mxf_remove_set(output->headerMetadata, output->sourcePackageTrackSet));
            mxf_free_set(&output->sourcePackageTrackSet);
            CHK_ORET(mxf_remove_set(output->headerMetadata, output->sequenceSet));
            mxf_free_set(&output->sequenceSet);
        }
    }
    
    
    /* indicate end of header metadata writing */
    CHK_ORET(mxf_mark_header_end(output->mxfFile, output->footerPartition));
    

    
    /* Write the Footer Index Table Segment */

    CHK_ORET(mxf_mark_index_start(output->mxfFile, output->footerPartition));
    mxf_generate_uuid(&output->indexSegment->instanceUID);
    output->indexSegment->indexDuration = output->duration;
    CHK_ORET(mxf_write_index_table_segment(output->mxfFile, output->indexSegment));
    CHK_ORET(mxf_mark_index_end(output->mxfFile, output->footerPartition));
    

    /* write the random index pack */
    
    CHK_ORET(mxf_write_rip(output->mxfFile, output->partitions));
    
   
    
    /* Update the Partition Packs and re-write them */

    /* header partition is open because the LTO Infax data needs to be filled in 
       and it doesn't include the PSE failures and D3 VTR errors */
    output->headerPartition->key = MXF_PP_K(OpenComplete, Header);
    CHK_ORET(mxf_update_partitions(output->mxfFile, output->partitions));
     
    
    
    free_d3_mxf_file(outputRef);
    return 1;
}


static int update_header_metadata(MXFFile* mxfFile, uint64_t headerByteCount, InfaxData* infaxData, 
    const char* newFilename)
{
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    MXFDataModel* dataModel = NULL;
    MXFHeaderMetadata* headerMetadata = NULL;
    uint64_t count;
    MXFMetadataSet* frameworkSet;
    mxfUTF16Char formatString[FORMAT_SIZE];
    int ltoInfaxSetFoundAndUpdated;
    int networkLocatorSetFoundAndUpdated;
    mxfUTF16Char* tempString = NULL;
    MXFMetadataSet* networkLocatorSet;
    int ltoInfaxSetFound;

    
    /* load the data model */
    
    CHK_OFAIL(mxf_load_data_model(&dataModel));
    CHK_OFAIL(load_bbc_d3_extensions(dataModel));
    CHK_OFAIL(mxf_finalise_data_model(dataModel));
    
    
    /* find the LTO Infax metadata set and update and rewrite it */
    
    CHK_OFAIL(mxf_read_next_nonfiller_kl(mxfFile, &key, &llen, &len));
    CHK_OFAIL(mxf_is_header_metadata(&key));
    
    CHK_OFAIL(mxf_create_header_metadata(&headerMetadata, dataModel));
    mxf_free_primer_pack(&headerMetadata->primerPack);
    CHK_OFAIL(mxf_read_primer_pack(mxfFile, &headerMetadata->primerPack));

    count = mxfKey_extlen + llen + len;
    
    ltoInfaxSetFoundAndUpdated = 0;
    networkLocatorSetFoundAndUpdated = 0;
    while (count < headerByteCount &&
        (!ltoInfaxSetFoundAndUpdated || !networkLocatorSetFoundAndUpdated))
    {
        CHK_OFAIL(mxf_read_kl(mxfFile, &key, &llen, &len));
        count += mxfKey_extlen + llen;
        
        if (mxf_is_filler(&key))
        {
            CHK_OFAIL(mxf_skip(mxfFile, len));
        }
        else
        {
            /* the LTO Infax set is identifiable by a missing format string, or
               if the file has already been updated for some reason, then the 
               format string != g_D3FormatString */
            if (mxf_equals_key(&key, &MXF_SET_K(D3P_InfaxFramework)))
            {
                ltoInfaxSetFound = 0;
                CHK_OFAIL(mxf_read_and_return_set(mxfFile, &key, len, headerMetadata, 1, &frameworkSet));
                if (mxf_have_item(frameworkSet, &MXF_ITEM_K(D3P_InfaxFramework, D3P_Format)))
                {
                    CHK_OFAIL(mxf_get_utf16string_item(frameworkSet, &MXF_ITEM_K(D3P_InfaxFramework, D3P_Format), formatString));
                    if (wcslen(formatString) == 0 || wcscmp(formatString, g_D3FormatString) != 0)
                    {
                        ltoInfaxSetFound = 1;
                    }
                }
                else
                {
                    ltoInfaxSetFound = 1;
                }
                
                if (ltoInfaxSetFound)
                {
                    /* update set and allocate the same fixed space */
                    mxf_set_fixed_set_space_allocation(frameworkSet, g_fixedInfaxSetAllocationSize);
                    CHK_OFAIL(set_infax_data(frameworkSet, infaxData));
                    
                    /* re-write set */
                    CHK_OFAIL(mxf_file_seek(mxfFile, - mxfKey_extlen - llen - len, SEEK_CUR));
                    CHK_OFAIL(mxf_write_set(mxfFile, frameworkSet));
                    ltoInfaxSetFoundAndUpdated = 1;
                }
            }
            
            /* update the NetworkLocator URL which holds the filename of 'this' file */ 
            else if (mxf_equals_key(&key, &MXF_SET_K(NetworkLocator)))
            {
                /* we are assuming there is only 1 NetworkLocator object in the header metadata */ 
                CHK_OFAIL(!networkLocatorSetFoundAndUpdated);
                
                /* update set (and debug check the length hasn't changed) */
                CHK_OFAIL(mxf_read_and_return_set(mxfFile, &key, len, headerMetadata, 1, &networkLocatorSet));
                CHK_OFAIL(convert_string(newFilename, &tempString));
                CHK_OFAIL(mxf_set_fixed_size_utf16string_item(networkLocatorSet, &MXF_ITEM_K(NetworkLocator, URLString), 
                    tempString, NETWORK_LOCATOR_URL_SIZE));
                CHK_OFAIL(llen + len + mxfKey_extlen == mxf_get_set_size(mxfFile, networkLocatorSet));
                
                /* re-write set */
                CHK_OFAIL(mxf_file_seek(mxfFile, - mxfKey_extlen - llen - len, SEEK_CUR));
                CHK_OFAIL(mxf_write_set(mxfFile, networkLocatorSet));
                networkLocatorSetFoundAndUpdated = 1;
            }
            
            else
            {
                CHK_OFAIL(mxf_skip(mxfFile, len));
            }
        }
        count += len;
    }
    CHK_OFAIL(ltoInfaxSetFoundAndUpdated && networkLocatorSetFoundAndUpdated);

    
    SAFE_FREE(&tempString);
    mxf_free_header_metadata(&headerMetadata);
    mxf_free_data_model(&dataModel);
    return 1;
    
fail:
    SAFE_FREE(&tempString);
    mxf_free_header_metadata(&headerMetadata);
    mxf_free_data_model(&dataModel);
    return 0;
}

int update_archive_mxf_file(const char* filePath, const char* newFilename, const char* ltoInfaxDataString, int beStrict)
{
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    MXFPartition* headerPartition = NULL;
    MXFPartition* footerPartition = NULL;
    InfaxData ltoInfaxData;
    MXFFile* mxfFile = NULL;
    
    CHK_ORET(filePath != NULL && newFilename != NULL && ltoInfaxDataString != NULL);
    

    /* open mxf file */    
    CHK_ORET(mxf_disk_file_open_modify(filePath, &mxfFile));
    mxf_file_set_min_llen(mxfFile, MIN_LLEN);

    
    /* parse the LTO infax data string */
    CHK_OFAIL(parse_infax_data(ltoInfaxDataString, &ltoInfaxData, beStrict));

    
    /* update the header partition header metadata LTO Infax data set */ 
    /* Note: the header partition remains open and complete because it doesn't
       contain the PSE failure and D3 VTR error data */
    
    /* read the header partition pack */
    if (!mxf_read_header_pp_kl(mxfFile, &key, &llen, &len))
    {
        mxf_log(MXF_ELOG, "Could not find header partition pack key" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
        return 0;
    }
    CHK_OFAIL(mxf_read_partition(mxfFile, &key, &headerPartition));

    CHK_OFAIL(update_header_metadata(mxfFile, headerPartition->headerByteCount, &ltoInfaxData, 
        newFilename));

    
    
    /* update the footer partition header metadata LTO Infax data set */    

    CHK_OFAIL(mxf_file_seek(mxfFile, headerPartition->footerPartition, SEEK_SET));
    
    /* read the footer partition pack */
    if (!mxf_read_kl(mxfFile, &key, &llen, &len))
    {
        mxf_log(MXF_ELOG, "Could not find footer partition pack key" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
        return 0;
    }
    CHK_OFAIL(mxf_read_partition(mxfFile, &key, &footerPartition));

    CHK_OFAIL(update_header_metadata(mxfFile, footerPartition->headerByteCount, &ltoInfaxData,
        newFilename));


    /* re-write the footer partition pack with closed and complete status */

    CHK_OFAIL(mxf_file_seek(mxfFile, headerPartition->footerPartition, SEEK_SET));
    footerPartition->key = MXF_PP_K(ClosedComplete, Footer);
    CHK_OFAIL(mxf_write_partition(mxfFile, footerPartition));

    
    
    mxf_file_close(&mxfFile);
    mxf_free_partition(&headerPartition);
    mxf_free_partition(&footerPartition);
    return 1;
    
fail:
    mxf_file_close(&mxfFile);
    mxf_free_partition(&headerPartition);
    mxf_free_partition(&footerPartition);
    return 0;
}


