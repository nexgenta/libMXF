/*
 * $Id: write_avid_mxf.c,v 1.6 2008/09/16 17:20:13 john_f Exp $
 *
 * Write video and audio to MXF files supported by Avid editing software
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

#include <mxf/mxf.h>
#include <mxf/mxf_avid.h>
#include <write_avid_mxf.h>


/* TODO: legacy switch (Note frameLayout and subsamplings) */

/* size of array used to hold stream offsets for the MJPEG index tables */
/* a new array is added to the list each time this number is exceeded */
/* Note: this number does not take into account the size limits (16-bit) imposed by the local tag
   encoding used in the index table. Avid index tables ignore this limit and instead 
   use the array header (32-bit) to provide the length */
#define MJPEG_STREAM_OFFSETS_ARRAY_SIZE    65535

#define MAX_TRACKS      17


typedef struct
{
    uint64_t* offsets;
    uint32_t len;
} MJPEGOffsetsArray;

typedef struct
{
    MXFMetadataItem* item;
    mxfRational editRate;
    uint32_t materialTrackID;
} TrackDurationItem;

typedef struct  
{
    char* filename;
    MXFFile* mxfFile;
    
    EssenceType essenceType;
    
    uint32_t materialTrackID;
    mxfUMID fileSourcePackageUID;
    
    
    /* used when using the start..., write..., end... functions to write a frame */
    uint32_t sampleDataSize;
    
    /* audio and video */
    mxfUL essenceContainerLabel;
    mxfKey essenceElementKey;
    uint8_t essenceElementLLen;
    mxfLength duration;
    mxfRational sampleRate;
    uint32_t editUnitByteCount;
    uint32_t sourceTrackNumber;
    uint64_t essenceLength;
    mxfUL pictureDataDef;
    mxfUL soundDataDef;
    mxfUL timecodeDataDef;
    
    
    /* video only */
    mxfUL cdciEssenceContainerLabel;
    uint32_t frameSize;
    int32_t resolutionID;
    mxfUL pictureEssenceCoding;
    uint32_t storedHeight;
    uint32_t storedWidth;
    uint32_t sampledHeight;
    uint32_t sampledWidth;
    uint32_t displayHeight;
    uint32_t displayWidth;
    uint32_t displayYOffset;
    uint32_t displayXOffset;
    int32_t videoLineMap[2];
    int videoLineMapLen;
    uint32_t horizSubsampling;
    uint32_t vertSubsampling;
    uint8_t frameLayout;
    uint8_t colorSiting;
    uint32_t imageAlignmentOffset;
    uint32_t imageStartOffset;
    
    
    /* audio only */
    mxfRational samplingRate;
    uint32_t bitsPerSample;
    uint16_t blockAlign;
    uint32_t avgBps;

    
    int64_t headerMetadataFilePos;
    
    MXFDataModel* dataModel;
    MXFList* partitions;
    MXFHeaderMetadata* headerMetadata;
    MXFIndexTableSegment* indexSegment;
    MXFEssenceElement* essenceElement;
    
    /* Avid MJPEG index table frame offsets */
    MXFList mjpegFrameOffsets;
    MJPEGOffsetsArray* currentMJPEGOffsetsArray;
    uint64_t prevFrameOffset;
    
    /* Avid uncompressed static essence data */
    uint8_t* vbiData;
    uint8_t* startOffsetData;
    
    /* these are references and should not be free'd */
    MXFPartition* headerPartition;
    MXFPartition* bodyPartition;
    MXFPartition* footerPartition;
    MXFMetadataSet* prefaceSet;
    MXFMetadataSet* identSet;
    MXFMetadataSet* contentStorageSet;
    MXFMetadataSet* materialPackageSet;
    MXFMetadataSet* sourcePackageSet;
    MXFMetadataSet* sourcePackageTrackSet;
    MXFMetadataSet* materialPackageTrackSet;
    MXFMetadataSet* sequenceSet;
    MXFMetadataSet* sourceClipSet;
    MXFMetadataSet* dmSet;
    MXFMetadataSet* dmFrameworkSet;
    MXFMetadataSet* timecodeComponentSet;    
    MXFMetadataSet* essContainerDataSet;
    MXFMetadataSet* multipleDescriptorSet;
    MXFMetadataSet* cdciDescriptorSet;
    MXFMetadataSet* bwfDescriptorSet;
    MXFMetadataSet* videoMaterialPackageTrackSet;
    MXFMetadataSet* videoSequenceSet;
    MXFMetadataSet* taggedValueSet;
    MXFMetadataSet* tapeDescriptorSet;
    
    
    /* duration items that are updated when writing is completed */
    TrackDurationItem durationItems[MAX_TRACKS * 2 + 2];
    int numDurationItems;
    
    /* container duration item that are created when writing is completed */
    MXFMetadataSet* descriptorSet;
} TrackWriter;

struct _AvidClipWriter
{
    TrackWriter* tracks[MAX_TRACKS];
    int numTracks;

    mxfUTF16Char* wProjectName;
    ProjectFormat projectFormat;
    mxfRational imageAspectRatio;
    int dropFrameFlag;
    int useLegacy;
    mxfRational projectEditRate;
    
    uint8_t dropFrameTimecode;
    
    mxfTimestamp now;
    
    /* used to temporarily hold strings */
    mxfUTF16Char* wTmpString;
    mxfUTF16Char* wTmpString2;
};
    


static const mxfUUID g_mxfIdentProductUID = 
    {0x05, 0x7c, 0xd8, 0x49, 0x17, 0x8a, 0x4b, 0x88, 0xb4, 0xc7, 0x82, 0x5a, 0xf8, 0x76, 0x1b, 0x23};    
static const mxfUTF16Char* g_mxfIdentCompanyName = L"BBC Research";
static const mxfUTF16Char* g_mxfIdentProductName = L"Avid MXF Writer";
static const mxfUTF16Char* g_mxfIdentVersionString = L"Beta version";

static const mxfKey MXF_EE_K(DVClipWrapped) = 
    MXF_DV_EE_K(0x01, MXF_DV_CLIP_WRAPPED_EE_TYPE, 0x01);
    
static const mxfKey MXF_EE_K(BWFClipWrapped) = 
    MXF_AES3BWF_EE_K(0x01, MXF_BWF_CLIP_WRAPPED_EE_TYPE, 0x01);

/* Avid MJPEG labels observed in files created by Media Composer 2.1.x */
    /* CompressionId's */
static const mxfUL MXF_CMDEF_L(AvidMJPEG21) = 
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0e, 0x04, 0x02, 0x01, 0x02, 0x01, 0x01, 0x08};
static const mxfUL MXF_CMDEF_L(AvidMJPEG31) = 
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0e, 0x04, 0x02, 0x01, 0x02, 0x01, 0x01, 0x06};
static const mxfUL MXF_CMDEF_L(AvidMJPEG101) = 
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0e, 0x04, 0x02, 0x01, 0x02, 0x01, 0x01, 0x04};
static const mxfUL MXF_CMDEF_L(AvidMJPEG101m) = 
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0e, 0x04, 0x02, 0x01, 0x02, 0x01, 0x04, 0x02};
static const mxfUL MXF_CMDEF_L(AvidMJPEG151s) = 
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0e, 0x04, 0x02, 0x01, 0x02, 0x01, 0x02, 0x02};
static const mxfUL MXF_CMDEF_L(AvidMJPEG201) = 
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0e, 0x04, 0x02, 0x01, 0x02, 0x01, 0x01, 0x02};

    /* EssenceContainer label */
static const mxfUL MXF_EC_L(AvidMJPEGClipWrapped) = 
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0e, 0x04, 0x03, 0x01, 0x02, 0x01, 0x00, 0x00};

static const mxfUL g_AvidAAFKLVEssenceContainer_ul = 
    {0x06, 0x0e, 0x2b, 0x34, 0x01, 0x01, 0x01, 0xff, 0x4b, 0x46, 0x41, 0x41, 0x00, 0x0d, 0x4d, 0x4f};
    
static const uint32_t g_AvidMJPEG21_ResolutionID = 0x4c;    /* 76 */
static const uint32_t g_AvidMJPEG31_ResolutionID = 0x4d;    /* 77 */
static const uint32_t g_AvidMJPEG101_ResolutionID = 0x4b;   /* 75 */
static const uint32_t g_AvidMJPEG101m_ResolutionID = 0x6e;  /* 110 */
static const uint32_t g_AvidMJPEG151s_ResolutionID = 0x4e;  /* 78 */
static const uint32_t g_AvidMJPEG201_ResolutionID = 0x52;   /* 82 */

static const mxfKey MXF_EE_K(AvidMJPEGClipWrapped) = 
    {0x06, 0x0e, 0x2b, 0x34, 0x01, 0x02, 0x01, 0x01, 0x0e, 0x04, 0x03, 0x01, 0x15, 0x01, 0x01, 0x01};

/* IMX (D10) labels observed in files by Media Composer 2.6 */
static const mxfKey MXF_EE_K(IMX) = MXF_D10_PICTURE_EE_K(0x01);

  /* To be identical to the Avid don't use MXF_EC_L(D10_50_625_50_picture_only)
     etc since they use regver=2 while Avid uses regver=1 */
static const mxfUL MXF_EC_L(IMX30) = 
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x01, 0x05, 0x7f};
static const mxfUL MXF_EC_L(IMX40) = 
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x01, 0x03, 0x7f};
static const mxfUL MXF_EC_L(IMX50) = 
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x01, 0x01, 0x7f};

/* DV100 labels observed in files by Media Composer 2.6 */
static const mxfUL MXF_EC_L(DV1080i50ClipWrapped) = 
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x02, 0x61, 0x02};
static const mxfUL MXF_CMDEF_L(DV1080i50) =
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x04, 0x01, 0x02, 0x02, 0x02, 0x02, 0x06, 0x00};
static const mxfKey MXF_EE_K(DV1080i50) = 
    {0x06, 0x0e, 0x2b, 0x34, 0x01, 0x02, 0x01, 0x01, 0x0d, 0x01, 0x03, 0x01, 0x18, 0x01, 0x02, 0x01};

/* DV100 720p50 is not supported by Media Composer 2.6, labels found in P2 created media */
static const mxfUL MXF_EC_L(DV720p50ClipWrapped) = 
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x02, 0x63, 0x02};
static const mxfUL MXF_CMDEF_L(DV720p50) =
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x04, 0x01, 0x02, 0x02, 0x02, 0x02, 0x08, 0x00};
static const mxfKey MXF_EE_K(DV720p50) = 
    {0x06, 0x0e, 0x2b, 0x34, 0x01, 0x02, 0x01, 0x01, 0x0d, 0x01, 0x03, 0x01, 0x18, 0x01, 0x02, 0x01};

static const mxfKey MXF_EE_K(DNxHD) =
    MXF_DNXHD_PICT_EE_K(0x01, 0x06, 0x01)

/* 15 = GC Picture, 01 = element count=1, 01 = Avid JFIF, 01 = element number=1 */
static const uint32_t g_AvidMJPEGTrackNumber = 0x15010101;

/* 15 = GC Picture, 01 = element count=1, 06 = DNxHD, 01 = element number=1 */
static const uint32_t g_DNxHDTrackNumber = 0x15010601;


static const mxfKey MXF_EE_K(UncClipWrapped) = 
    MXF_UNC_EE_K(0x01, MXF_UNC_CLIP_WRAPPED_EE_TYPE, 0x01);

static const uint32_t g_uncImageAlignmentOffset = 8192;
/* 852480 = 720*592*2 */
static const uint32_t g_uncPALFrameSize = 852480;
/* 860160 = 720*592*2 rounded up to image alignment factor 8192 */
static const uint32_t g_uncAlignedPALFrameSize = 860160;
static const uint32_t g_uncPALStartOffsetSize = 
    860160 /*g_uncAlignedPALFrameSize*/ - 852480 /*g_uncPALFrameSize*/;

static const uint32_t g_uncPALVBISize = 720 * 16 * 2;

static const uint32_t g_uncAligned1080i50FrameSize = 4153344;    /* 0x3f6000 (6144 pad + 4147200 frame) */
static const uint32_t g_unc1080i50StartOffsetSize = 6144;


static const uint32_t g_bodySID = 1;
static const uint32_t g_indexSID = 2;

static const uint64_t g_fixedBodyPPOffset = 0x40020;
static const uint64_t g_uncFixedBodyPPOffset = 0x60020;


static void free_offsets_array_in_list(void* data)
{
    MJPEGOffsetsArray* offsetsArray;
    
    if (data == NULL)
    {
        return;
    }
    
    offsetsArray = (MJPEGOffsetsArray*)data;
    SAFE_FREE(&offsetsArray->offsets);
    SAFE_FREE(&offsetsArray);
}

static int create_avid_mjpeg_offsets_array(MXFList* mjpegFrameOffsets, 
    MJPEGOffsetsArray** offsetsArray)
{
    MJPEGOffsetsArray* newOffsetsArray = NULL;
    
    CHK_MALLOC_ORET(newOffsetsArray, MJPEGOffsetsArray);
    memset(newOffsetsArray, 0, sizeof(MJPEGOffsetsArray));
    CHK_MALLOC_ARRAY_OFAIL(newOffsetsArray->offsets, uint64_t, MJPEG_STREAM_OFFSETS_ARRAY_SIZE);
    
    CHK_OFAIL(mxf_append_list_element(mjpegFrameOffsets, newOffsetsArray));
    
    *offsetsArray = newOffsetsArray;
    return 1;
    
fail:
    if (newOffsetsArray != NULL)
    {
        SAFE_FREE(&newOffsetsArray->offsets);
    }
    SAFE_FREE(&newOffsetsArray);
    return 0;
}

static int add_avid_mjpeg_offset(MXFList* mjpegFrameOffsets, uint64_t offset, 
    MJPEGOffsetsArray** offsetsArray)
{
    if ((*offsetsArray) == NULL || (*offsetsArray)->len + 1 == MJPEG_STREAM_OFFSETS_ARRAY_SIZE)
    {
        CHK_ORET(create_avid_mjpeg_offsets_array(mjpegFrameOffsets, offsetsArray));
    }
    
    (*offsetsArray)->offsets[(*offsetsArray)->len] = offset;
    (*offsetsArray)->len++;
    
    return 1;
}

static uint32_t get_num_offsets(MXFList* mjpegFrameOffsets)
{
    MXFListIterator iter;
    uint32_t numOffsets = 0;
    MJPEGOffsetsArray* offsetsArray;
    
    mxf_initialise_list_iter(&iter, mjpegFrameOffsets);
    while (mxf_next_list_iter_element(&iter))
    {
        offsetsArray = (MJPEGOffsetsArray*)mxf_get_iter_element(&iter);
        numOffsets += offsetsArray->len;
    }
    
    return numOffsets;
}



static void free_track_writer(TrackWriter** writer)
{
    if (*writer == NULL)
    {
        return;
    }
    
    SAFE_FREE(&(*writer)->filename);

    mxf_free_index_table_segment(&(*writer)->indexSegment);
    mxf_free_file_partitions(&(*writer)->partitions);
    mxf_free_header_metadata(&(*writer)->headerMetadata);
    mxf_free_data_model(&(*writer)->dataModel);
    mxf_close_essence_element(&(*writer)->essenceElement);
    
    mxf_clear_list(&(*writer)->mjpegFrameOffsets);

    SAFE_FREE(&(*writer)->vbiData);    
    SAFE_FREE(&(*writer)->startOffsetData);
    
    mxf_file_close(&(*writer)->mxfFile);
    
    SAFE_FREE(writer);
}

static void free_avid_clip_writer(AvidClipWriter** clipWriter)
{
    int i;
    
    if (*clipWriter == NULL)
    {
        return;
    }
    
    SAFE_FREE(&(*clipWriter)->wProjectName);

    for (i = 0; i < (*clipWriter)->numTracks; i++)
    {
        free_track_writer(&(*clipWriter)->tracks[i]);
    }

    SAFE_FREE(&(*clipWriter)->wTmpString);    
    SAFE_FREE(&(*clipWriter)->wTmpString2);    
    
    SAFE_FREE(clipWriter);
}

static int convert_string(AvidClipWriter* clipWriter, const char* input)
{
    mxfUTF16Char* newOutput = NULL;
    size_t len = strlen(input);
    
    CHK_MALLOC_ARRAY_ORET(newOutput, mxfUTF16Char, len + 1);
    memset(newOutput, 0, sizeof(mxfUTF16Char) * (len + 1));
    
    CHK_OFAIL(mbstowcs(newOutput, input, len + 1) != (size_t)(-1));

    SAFE_FREE(&clipWriter->wTmpString);    
    clipWriter->wTmpString = newOutput;
    return 1;
    
fail:
    SAFE_FREE(&newOutput);
    return 0;
}

static int get_track_writer(AvidClipWriter* clipWriter, uint32_t materialTrackID, TrackWriter** writer)
{
    int i;
    
    for (i = 0; i < clipWriter->numTracks; i++)
    {
        if (clipWriter->tracks[i]->materialTrackID == materialTrackID)
        {
            *writer = clipWriter->tracks[i];
            return 1;
        }
    }
    
    return 0;
}

static int get_file_package(TrackWriter* trackWriter, PackageDefinitions* packageDefinitions, Package** filePackage)
{
    MXFListIterator iter;
    Package* package;

    mxf_initialise_list_iter(&iter, &packageDefinitions->fileSourcePackages);
    while (mxf_next_list_iter_element(&iter))
    {
        package = mxf_get_iter_element(&iter);
        
        if (mxf_equals_umid(&trackWriter->fileSourcePackageUID, &package->uid))
        {
            *filePackage = package;
            return 1;
        }
    }
    
    return 0;
}

static int create_header_metadata(AvidClipWriter* clipWriter, PackageDefinitions* packageDefinitions, 
    Package* filePackage, TrackWriter* writer)
{
    uint8_t* arrayElement;
    mxfUUID thisGeneration;
    uint16_t roundedTimecodeBase;
    Package* materialPackage = packageDefinitions->materialPackage;
    Package* tapePackage = packageDefinitions->tapeSourcePackage;
    MXFListIterator iter;
    Track* track;
    int i;
    uint32_t maxTrackID;
    int64_t tapeLen;
    UserComment* userComment;
        
    
    mxf_generate_uuid(&thisGeneration);

    roundedTimecodeBase = (uint16_t)((float)clipWriter->projectEditRate.numerator / 
        clipWriter->projectEditRate.denominator + 0.5);
    
    
    /* load the data model, plus AVID extensions */
    
    if (writer->dataModel == NULL)
    {
        CHK_ORET(mxf_load_data_model(&writer->dataModel));
        CHK_ORET(mxf_avid_load_extensions(writer->dataModel));
        CHK_ORET(mxf_finalise_data_model(writer->dataModel));
    }
    
    
    /* delete existing header metadata */
    
    if (writer->headerMetadata != NULL)
    {
        mxf_free_header_metadata(&writer->headerMetadata);
    }
    
    
    /* create the header metadata and associated primer pack */
    
    CHK_ORET(mxf_create_header_metadata(&writer->headerMetadata, writer->dataModel));

    
    /* Preface */
    CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(Preface), &writer->prefaceSet));
    CHK_ORET(mxf_set_timestamp_item(writer->prefaceSet, &MXF_ITEM_K(Preface, LastModifiedDate), &clipWriter->now));
    CHK_ORET(mxf_set_version_type_item(writer->prefaceSet, &MXF_ITEM_K(Preface, Version), 0x0102));
    CHK_ORET(mxf_set_ul_item(writer->prefaceSet, &MXF_ITEM_K(Preface, OperationalPattern), &MXF_OP_L(atom, complexity02)));
    CHK_ORET(mxf_alloc_array_item_elements(writer->prefaceSet, &MXF_ITEM_K(Preface, EssenceContainers), mxfUL_extlen, 1, &arrayElement));
    mxf_set_ul(&writer->essenceContainerLabel, arrayElement);
    if (clipWriter->wProjectName != NULL)
    {
        CHK_ORET(mxf_set_utf16string_item(writer->prefaceSet, &MXF_ITEM_K(Preface, ProjectName), clipWriter->wProjectName));
    }
    CHK_ORET(mxf_set_rational_item(writer->prefaceSet, &MXF_ITEM_K(Preface, ProjectEditRate), &clipWriter->projectEditRate));

    
    /* Preface - ContentStorage */
    CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(ContentStorage), &writer->contentStorageSet));
    CHK_ORET(mxf_set_strongref_item(writer->prefaceSet, &MXF_ITEM_K(Preface, ContentStorage), writer->contentStorageSet));
 
    
    /* Preface - ContentStorage - MaterialPackage */
    CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(MaterialPackage), &writer->materialPackageSet));
    CHK_ORET(mxf_add_array_item_strongref(writer->contentStorageSet, &MXF_ITEM_K(ContentStorage, Packages), writer->materialPackageSet));
    CHK_ORET(mxf_set_umid_item(writer->materialPackageSet, &MXF_ITEM_K(GenericPackage, PackageUID), &materialPackage->uid));
    CHK_ORET(mxf_set_timestamp_item(writer->materialPackageSet, &MXF_ITEM_K(GenericPackage, PackageCreationDate), &materialPackage->creationDate));
    CHK_ORET(mxf_set_timestamp_item(writer->materialPackageSet, &MXF_ITEM_K(GenericPackage, PackageModifiedDate), &materialPackage->creationDate));
    if (materialPackage->name != NULL)
    {
        CHK_ORET(convert_string(clipWriter, materialPackage->name));
        CHK_ORET(mxf_set_utf16string_item(writer->materialPackageSet, &MXF_ITEM_K(GenericPackage, Name), clipWriter->wTmpString));
    }

    mxf_initialise_list_iter(&iter, &materialPackage->tracks);
    while (mxf_next_list_iter_element(&iter))
    {
        track = (Track*)mxf_get_iter_element(&iter);
        
        /* Preface - ContentStorage - MaterialPackage - Timeline Track */    
        CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(Track), &writer->materialPackageTrackSet));
        CHK_ORET(mxf_add_array_item_strongref(writer->materialPackageSet, &MXF_ITEM_K(GenericPackage, Tracks), writer->materialPackageTrackSet));
        CHK_ORET(mxf_set_uint32_item(writer->materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackID), track->id));
        if (track->name != NULL)
        {
            CHK_ORET(convert_string(clipWriter, track->name));
            CHK_ORET(mxf_set_utf16string_item(writer->materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackName), clipWriter->wTmpString));
        }
        CHK_ORET(mxf_set_uint32_item(writer->materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), track->number));
        CHK_ORET(mxf_set_rational_item(writer->materialPackageTrackSet, &MXF_ITEM_K(Track, EditRate), &track->editRate));
        CHK_ORET(mxf_set_position_item(writer->materialPackageTrackSet, &MXF_ITEM_K(Track, Origin), 0));
    
        /* Preface - ContentStorage - MaterialPackage - Timeline Track - Sequence */    
        CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(Sequence), &writer->sequenceSet));
        CHK_ORET(mxf_set_strongref_item(writer->materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), writer->sequenceSet));
        if (track->isPicture)
        {
            CHK_ORET(mxf_set_ul_item(writer->sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &writer->pictureDataDef));
        }
        else
        {
            CHK_ORET(mxf_set_ul_item(writer->sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &writer->soundDataDef));
        }
        CHK_ORET(mxf_set_length_item(writer->sequenceSet, &MXF_ITEM_K(StructuralComponent, Duration), -1)); /* to be filled in */

        CHK_ORET(mxf_get_item(writer->sequenceSet, &MXF_ITEM_K(StructuralComponent, Duration), &writer->durationItems[writer->numDurationItems].item));
        writer->durationItems[writer->numDurationItems].editRate = track->editRate;
        writer->durationItems[writer->numDurationItems].materialTrackID = track->id;
        writer->numDurationItems++;
        
    
        /* Preface - ContentStorage - MaterialPackage - Timeline Track - Sequence - SourceClip */    
        CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(SourceClip), &writer->sourceClipSet));
        CHK_ORET(mxf_add_array_item_strongref(writer->sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), writer->sourceClipSet));
        if (track->isPicture)
        {
            CHK_ORET(mxf_set_ul_item(writer->sourceClipSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &writer->pictureDataDef));
        }
        else
        {
            CHK_ORET(mxf_set_ul_item(writer->sourceClipSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &writer->soundDataDef));
        }
        CHK_ORET(mxf_set_length_item(writer->sourceClipSet, &MXF_ITEM_K(StructuralComponent, Duration), -1)); /* to be filled in */
        CHK_ORET(mxf_set_position_item(writer->sourceClipSet, &MXF_ITEM_K(SourceClip, StartPosition), track->startPosition));
        CHK_ORET(mxf_set_umid_item(writer->sourceClipSet, &MXF_ITEM_K(SourceClip, SourcePackageID), &track->sourcePackageUID));
        CHK_ORET(mxf_set_uint32_item(writer->sourceClipSet, &MXF_ITEM_K(SourceClip, SourceTrackID), track->sourceTrackID));

        CHK_ORET(mxf_get_item(writer->sourceClipSet, &MXF_ITEM_K(StructuralComponent, Duration), &writer->durationItems[writer->numDurationItems].item));
        writer->durationItems[writer->numDurationItems].editRate = track->editRate;
        writer->durationItems[writer->numDurationItems].materialTrackID = track->id;
        writer->numDurationItems++;
        
    }

    /* attach a project name attribute to the material package */
    if (clipWriter->wProjectName != NULL)
    {
        CHK_ORET(mxf_avid_attach_mob_attribute(writer->headerMetadata, writer->materialPackageSet, L"_PJ", clipWriter->wProjectName));
    }

    /* attach user comments to the material package*/
    mxf_initialise_list_iter(&iter, &packageDefinitions->userComments);
    while (mxf_next_list_iter_element(&iter))
    {
        userComment = (UserComment*)mxf_get_iter_element(&iter);
        
        if (userComment->name != NULL && userComment->value != NULL)
        {
            CHK_ORET(convert_string(clipWriter, userComment->name));
            SAFE_FREE(&clipWriter->wTmpString2);
            clipWriter->wTmpString2 = clipWriter->wTmpString;
            clipWriter->wTmpString = NULL;
            CHK_ORET(convert_string(clipWriter, userComment->value));
            CHK_ORET(mxf_avid_attach_user_comment(writer->headerMetadata, writer->materialPackageSet, 
                clipWriter->wTmpString2, clipWriter->wTmpString));
        }
    }
    

    CHK_ORET(mxf_get_list_length(&filePackage->tracks) == 1);
    track = (Track*)mxf_get_list_element(&filePackage->tracks, 0);
    
    /* Preface - ContentStorage - SourcePackage */
    CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(SourcePackage), &writer->sourcePackageSet));
    CHK_ORET(mxf_add_array_item_strongref(writer->contentStorageSet, &MXF_ITEM_K(ContentStorage, Packages), writer->sourcePackageSet));
    CHK_ORET(mxf_set_umid_item(writer->sourcePackageSet, &MXF_ITEM_K(GenericPackage, PackageUID), &filePackage->uid));
    CHK_ORET(mxf_set_timestamp_item(writer->sourcePackageSet, &MXF_ITEM_K(GenericPackage, PackageCreationDate), &filePackage->creationDate));
    CHK_ORET(mxf_set_timestamp_item(writer->sourcePackageSet, &MXF_ITEM_K(GenericPackage, PackageModifiedDate), &filePackage->creationDate));
    if (filePackage->name != NULL)
    {
        CHK_ORET(convert_string(clipWriter, filePackage->name));
        CHK_ORET(mxf_set_utf16string_item(writer->sourcePackageSet, &MXF_ITEM_K(GenericPackage, Name), clipWriter->wTmpString));
    }

    /* Preface - ContentStorage - SourcePackage - Timeline Track */    
    CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(Track), &writer->sourcePackageTrackSet));
    CHK_ORET(mxf_add_array_item_strongref(writer->sourcePackageSet, &MXF_ITEM_K(GenericPackage, Tracks), writer->sourcePackageTrackSet));
    CHK_ORET(mxf_set_uint32_item(writer->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackID), track->id));
    CHK_ORET(mxf_set_uint32_item(writer->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), writer->sourceTrackNumber));
    CHK_ORET(mxf_set_rational_item(writer->sourcePackageTrackSet, &MXF_ITEM_K(Track, EditRate), &track->editRate));
    CHK_ORET(mxf_set_position_item(writer->sourcePackageTrackSet, &MXF_ITEM_K(Track, Origin), 0));

    /* Preface - ContentStorage - SourcePackage - Timeline Track - Sequence */    
    CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(Sequence), &writer->sequenceSet));
    CHK_ORET(mxf_set_strongref_item(writer->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), writer->sequenceSet));
    if (track->isPicture)
    {
        CHK_ORET(mxf_set_ul_item(writer->sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &writer->pictureDataDef));
    }
    else
    {
        CHK_ORET(mxf_set_ul_item(writer->sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &writer->soundDataDef));
    }
    CHK_ORET(mxf_set_length_item(writer->sequenceSet, &MXF_ITEM_K(StructuralComponent, Duration), -1));  /* to be filled in */

    CHK_ORET(mxf_get_item(writer->sequenceSet, &MXF_ITEM_K(StructuralComponent, Duration), &writer->durationItems[writer->numDurationItems].item));
    writer->durationItems[writer->numDurationItems].editRate = track->editRate;
    writer->durationItems[writer->numDurationItems].materialTrackID = writer->materialTrackID;
    writer->numDurationItems++;
        
    
    /* Preface - ContentStorage - SourcePackage - Timeline Track - Sequence - SourceClip */    
    CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(SourceClip), &writer->sourceClipSet));
    CHK_ORET(mxf_add_array_item_strongref(writer->sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), writer->sourceClipSet));
    if (track->isPicture)
    {
        CHK_ORET(mxf_set_ul_item(writer->sourceClipSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &writer->pictureDataDef));
    }
    else
    {
        CHK_ORET(mxf_set_ul_item(writer->sourceClipSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &writer->soundDataDef));
    }
    CHK_ORET(mxf_set_length_item(writer->sourceClipSet, &MXF_ITEM_K(StructuralComponent, Duration), -1)); /* to be filled in */
    CHK_ORET(mxf_set_position_item(writer->sourceClipSet, &MXF_ITEM_K(SourceClip, StartPosition), track->startPosition));
    CHK_ORET(mxf_set_umid_item(writer->sourceClipSet, &MXF_ITEM_K(SourceClip, SourcePackageID), &track->sourcePackageUID));
    CHK_ORET(mxf_set_uint32_item(writer->sourceClipSet, &MXF_ITEM_K(SourceClip, SourceTrackID), track->sourceTrackID));

    CHK_ORET(mxf_get_item(writer->sourceClipSet, &MXF_ITEM_K(StructuralComponent, Duration), &writer->durationItems[writer->numDurationItems].item));
    writer->durationItems[writer->numDurationItems].editRate = track->editRate;
    writer->durationItems[writer->numDurationItems].materialTrackID = writer->materialTrackID;
    writer->numDurationItems++;

    
    if (track->isPicture)
    {
        // const mxfUL codecUL = 
        // {0xbf, 0xd6, 0x00, 0x10, 0x4b, 0xc9, 0x15, 0x6d, 0x18, 0x63, 0x4f, 0x8c, 0x3b, 0xab, 0x11, 0xd3};
        /* Preface - ContentStorage - SourcePackage - CDCIEssenceDescriptor */    
        CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(CDCIEssenceDescriptor), &writer->cdciDescriptorSet));
        CHK_ORET(mxf_set_strongref_item(writer->sourcePackageSet, &MXF_ITEM_K(SourcePackage, Descriptor), writer->cdciDescriptorSet));
        CHK_ORET(mxf_set_rational_item(writer->cdciDescriptorSet, &MXF_ITEM_K(FileDescriptor, SampleRate), &writer->sampleRate));
        CHK_ORET(mxf_set_length_item(writer->cdciDescriptorSet, &MXF_ITEM_K(FileDescriptor, ContainerDuration), 0));
        if (mxf_equals_ul(&writer->cdciEssenceContainerLabel, &g_Null_UL))
        {
            CHK_ORET(mxf_set_ul_item(writer->cdciDescriptorSet, &MXF_ITEM_K(FileDescriptor, EssenceContainer), &writer->essenceContainerLabel));
        }
        else
        {
            /* the Avid MJPEGs and uncompressed (?) require the AAF-KLV container label here */
            CHK_ORET(mxf_set_ul_item(writer->cdciDescriptorSet, &MXF_ITEM_K(FileDescriptor, EssenceContainer), &writer->cdciEssenceContainerLabel));
        }
        // CHK_ORET(mxf_set_ul_item(writer->cdciDescriptorSet, &MXF_ITEM_K(FileDescriptor, Codec), &codecUL));
        if (!mxf_equals_ul(&writer->pictureEssenceCoding, &g_Null_UL))
        {
            CHK_ORET(mxf_set_ul_item(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, PictureEssenceCoding), &writer->pictureEssenceCoding));
        }
        CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, StoredHeight), writer->storedHeight));
        CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, StoredWidth), writer->storedWidth));
        if (writer->sampledHeight != 0 || writer->sampledWidth != 0)
        {
            CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, SampledHeight), writer->sampledHeight));
            CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, SampledWidth), writer->sampledWidth));
        }
        if (writer->displayHeight != 0 || writer->displayWidth != 0)
        {
            CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayHeight), writer->displayHeight));
            CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayWidth), writer->displayWidth));
        }
        if (writer->displayYOffset != 0 || writer->displayXOffset != 0)
        {
            CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayYOffset), writer->displayYOffset));
            CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayXOffset), writer->displayXOffset));
        }
        CHK_ORET(mxf_alloc_array_item_elements(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, VideoLineMap), 4, writer->videoLineMapLen, &arrayElement));
        for (i = 0; i < writer->videoLineMapLen; i++)
        {
            mxf_set_int32(writer->videoLineMap[i], &arrayElement[i * 4]);
        }
        CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(CDCIEssenceDescriptor, HorizontalSubsampling), writer->horizSubsampling));
        CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(CDCIEssenceDescriptor, VerticalSubsampling), writer->vertSubsampling));
        CHK_ORET(mxf_set_uint8_item(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameLayout), writer->frameLayout));
        CHK_ORET(mxf_set_rational_item(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, AspectRatio), &clipWriter->imageAspectRatio));
        CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageAlignmentOffset), 1));
        CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(CDCIEssenceDescriptor, ComponentDepth), 8));
        CHK_ORET(mxf_set_uint8_item(writer->cdciDescriptorSet, &MXF_ITEM_K(CDCIEssenceDescriptor, ColorSiting), writer->colorSiting));
        CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(CDCIEssenceDescriptor, BlackRefLevel), 16));
        CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(CDCIEssenceDescriptor, WhiteReflevel), 235));
        CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(CDCIEssenceDescriptor, ColorRange), 225));
        if (writer->imageAlignmentOffset != 0)
        {
            CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageAlignmentOffset), writer->imageAlignmentOffset));
        }
        if (writer->imageStartOffset != 0)
        {
            CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageStartOffset), writer->imageStartOffset));
        }
        if (writer->resolutionID != 0) {
            CHK_ORET(mxf_set_int32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, ResolutionID), writer->resolutionID));
        }
        CHK_ORET(mxf_set_int32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameSampleSize), writer->frameSize));
        CHK_ORET(mxf_set_int32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageSize), (int32_t)writer->essenceLength));

        writer->descriptorSet = writer->cdciDescriptorSet;  /* ContainerDuration and ImageSize updated when writing completed */
    }
    else
    {
        /* Preface - ContentStorage - SourcePackage - WaveEssenceDescriptor */    
        CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(WaveAudioDescriptor), &writer->bwfDescriptorSet));
        CHK_ORET(mxf_set_strongref_item(writer->sourcePackageSet, &MXF_ITEM_K(SourcePackage, Descriptor), writer->bwfDescriptorSet));
        CHK_ORET(mxf_set_rational_item(writer->bwfDescriptorSet, &MXF_ITEM_K(FileDescriptor, SampleRate), &writer->sampleRate));
        CHK_ORET(mxf_set_length_item(writer->bwfDescriptorSet, &MXF_ITEM_K(FileDescriptor, ContainerDuration), 0));
        CHK_ORET(mxf_set_ul_item(writer->bwfDescriptorSet, &MXF_ITEM_K(FileDescriptor, EssenceContainer), &writer->essenceContainerLabel));
        CHK_ORET(mxf_set_rational_item(writer->bwfDescriptorSet, &MXF_ITEM_K(GenericSoundEssenceDescriptor, AudioSamplingRate), &writer->samplingRate));
        CHK_ORET(mxf_set_uint32_item(writer->bwfDescriptorSet, &MXF_ITEM_K(GenericSoundEssenceDescriptor, ChannelCount), 1));
        CHK_ORET(mxf_set_uint32_item(writer->bwfDescriptorSet, &MXF_ITEM_K(GenericSoundEssenceDescriptor, QuantizationBits), writer->bitsPerSample));
        CHK_ORET(mxf_set_uint16_item(writer->bwfDescriptorSet, &MXF_ITEM_K(WaveAudioDescriptor, BlockAlign), writer->blockAlign));
        CHK_ORET(mxf_set_uint32_item(writer->bwfDescriptorSet, &MXF_ITEM_K(WaveAudioDescriptor, AvgBps), writer->avgBps));

        writer->descriptorSet = writer->bwfDescriptorSet;  /* ContainerDuration updated when writing completed */
    }
    
    /* attach a project name attribute to the file source package */
    if (clipWriter->wProjectName != NULL)
    {
        CHK_ORET(mxf_avid_attach_mob_attribute(writer->headerMetadata, writer->sourcePackageSet, L"_PJ", clipWriter->wProjectName));
    }
    
    
    /* Preface - ContentStorage - EssenceContainerData */    
    CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(EssenceContainerData), &writer->essContainerDataSet));
    CHK_ORET(mxf_add_array_item_strongref(writer->contentStorageSet, &MXF_ITEM_K(ContentStorage, EssenceContainerData), writer->essContainerDataSet));
    CHK_ORET(mxf_set_umid_item(writer->essContainerDataSet, &MXF_ITEM_K(EssenceContainerData, LinkedPackageUID), &filePackage->uid));
    CHK_ORET(mxf_set_uint32_item(writer->essContainerDataSet, &MXF_ITEM_K(EssenceContainerData, IndexSID), g_indexSID));
    CHK_ORET(mxf_set_uint32_item(writer->essContainerDataSet, &MXF_ITEM_K(EssenceContainerData, BodySID), g_bodySID));


    
    if (tapePackage != NULL)
    {
        /* Preface - ContentStorage - tape SourcePackage */
        CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(SourcePackage), &writer->sourcePackageSet));
        CHK_ORET(mxf_add_array_item_strongref(writer->contentStorageSet, &MXF_ITEM_K(ContentStorage, Packages), writer->sourcePackageSet));
        CHK_ORET(mxf_set_umid_item(writer->sourcePackageSet, &MXF_ITEM_K(GenericPackage, PackageUID), &tapePackage->uid));
        CHK_ORET(mxf_set_timestamp_item(writer->sourcePackageSet, &MXF_ITEM_K(GenericPackage, PackageCreationDate), &tapePackage->creationDate));
        CHK_ORET(mxf_set_timestamp_item(writer->sourcePackageSet, &MXF_ITEM_K(GenericPackage, PackageModifiedDate), &tapePackage->creationDate));
        if (tapePackage->name != NULL)
        {
            CHK_ORET(convert_string(clipWriter, tapePackage->name));
            CHK_ORET(mxf_set_utf16string_item(writer->sourcePackageSet, &MXF_ITEM_K(GenericPackage, Name), clipWriter->wTmpString));
        }
        
        maxTrackID = 0;
        tapeLen = 0;
        mxf_initialise_list_iter(&iter, &tapePackage->tracks);
        while (mxf_next_list_iter_element(&iter))
        {
            track = (Track*)mxf_get_iter_element(&iter);
            
            /* Preface - ContentStorage - SourcePackage - Timeline Track */    
            CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(Track), &writer->sourcePackageTrackSet));
            CHK_ORET(mxf_add_array_item_strongref(writer->sourcePackageSet, &MXF_ITEM_K(GenericPackage, Tracks), writer->sourcePackageTrackSet));
            CHK_ORET(mxf_set_uint32_item(writer->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackID), track->id));
            if (track->name != NULL)
            {
                CHK_ORET(convert_string(clipWriter, track->name));
                CHK_ORET(mxf_set_utf16string_item(writer->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackName), clipWriter->wTmpString));
            }
            CHK_ORET(mxf_set_uint32_item(writer->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), track->number));
            CHK_ORET(mxf_set_rational_item(writer->sourcePackageTrackSet, &MXF_ITEM_K(Track, EditRate), &track->editRate));
            CHK_ORET(mxf_set_position_item(writer->sourcePackageTrackSet, &MXF_ITEM_K(Track, Origin), 0));
        
            /* Preface - ContentStorage - SourcePackage - Timeline Track - Sequence */    
            CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(Sequence), &writer->sequenceSet));
            CHK_ORET(mxf_set_strongref_item(writer->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), writer->sequenceSet));
            if (track->isPicture)
            {
                CHK_ORET(mxf_set_ul_item(writer->sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &writer->pictureDataDef));
            }
            else
            {
                CHK_ORET(mxf_set_ul_item(writer->sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &writer->soundDataDef));
            }
            CHK_ORET(mxf_set_length_item(writer->sequenceSet, &MXF_ITEM_K(StructuralComponent, Duration), track->length));
        
            /* Preface - ContentStorage - SourcePackage - Timeline Track - Sequence - SourceClip */    
            CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(SourceClip), &writer->sourceClipSet));
            CHK_ORET(mxf_add_array_item_strongref(writer->sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), writer->sourceClipSet));
            if (track->isPicture)
            {
                CHK_ORET(mxf_set_ul_item(writer->sourceClipSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &writer->pictureDataDef));
            }
            else
            {
                CHK_ORET(mxf_set_ul_item(writer->sourceClipSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &writer->soundDataDef));
            }
            CHK_ORET(mxf_set_length_item(writer->sourceClipSet, &MXF_ITEM_K(StructuralComponent, Duration), track->length));
            CHK_ORET(mxf_set_position_item(writer->sourceClipSet, &MXF_ITEM_K(SourceClip, StartPosition), track->startPosition));
            CHK_ORET(mxf_set_umid_item(writer->sourceClipSet, &MXF_ITEM_K(SourceClip, SourcePackageID), &track->sourcePackageUID));
            CHK_ORET(mxf_set_uint32_item(writer->sourceClipSet, &MXF_ITEM_K(SourceClip, SourceTrackID), track->sourceTrackID));
    
            if (track->id > maxTrackID)
            {
                maxTrackID = track->id;
            }
            tapeLen = track->length;
        }            

        /* Preface - ContentStorage - SourcePackage - timecode Timeline Track */    
        CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(Track), &writer->sourcePackageTrackSet));
        CHK_ORET(mxf_add_array_item_strongref(writer->sourcePackageSet, &MXF_ITEM_K(GenericPackage, Tracks), writer->sourcePackageTrackSet));
        CHK_ORET(mxf_set_uint32_item(writer->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackID), maxTrackID + 1));
        CHK_ORET(mxf_set_utf16string_item(writer->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackName), L"TC1"));
        CHK_ORET(mxf_set_uint32_item(writer->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), 0));
        CHK_ORET(mxf_set_rational_item(writer->sourcePackageTrackSet, &MXF_ITEM_K(Track, EditRate), &clipWriter->projectEditRate));
        CHK_ORET(mxf_set_position_item(writer->sourcePackageTrackSet, &MXF_ITEM_K(Track, Origin), 0));
    
        /* Preface - ContentStorage - SourcePackage - timecode Timeline Track - Sequence */    
        CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(Sequence), &writer->sequenceSet));
        CHK_ORET(mxf_set_strongref_item(writer->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), writer->sequenceSet));
        CHK_ORET(mxf_set_ul_item(writer->sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &writer->timecodeDataDef));
        CHK_ORET(mxf_set_length_item(writer->sequenceSet, &MXF_ITEM_K(StructuralComponent, Duration), tapeLen));
    
        /* Preface - ContentStorage - SourcePackage - Timecode Track - Sequence - TimecodeComponent */    
        CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(TimecodeComponent), &writer->timecodeComponentSet));
        CHK_ORET(mxf_add_array_item_strongref(writer->sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), writer->timecodeComponentSet));
        CHK_ORET(mxf_set_ul_item(writer->timecodeComponentSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &writer->timecodeDataDef));
        CHK_ORET(mxf_set_length_item(writer->timecodeComponentSet, &MXF_ITEM_K(StructuralComponent, Duration), tapeLen));
        CHK_ORET(mxf_set_uint16_item(writer->timecodeComponentSet, &MXF_ITEM_K(TimecodeComponent, RoundedTimecodeBase), roundedTimecodeBase));
        CHK_ORET(mxf_set_boolean_item(writer->timecodeComponentSet, &MXF_ITEM_K(TimecodeComponent, DropFrame), clipWriter->dropFrameTimecode));
        CHK_ORET(mxf_set_position_item(writer->timecodeComponentSet, &MXF_ITEM_K(TimecodeComponent, StartTimecode), 0));

        
        /* Preface - ContentStorage - tape SourcePackage - TapeDescriptor */
        CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(TapeDescriptor), &writer->tapeDescriptorSet));
        CHK_ORET(mxf_set_strongref_item(writer->sourcePackageSet, &MXF_ITEM_K(SourcePackage, Descriptor), writer->tapeDescriptorSet));
        
        
        /* attach a project name attribute to the package */
        if (clipWriter->wProjectName != NULL)
        {
            CHK_ORET(mxf_avid_attach_mob_attribute(writer->headerMetadata, writer->sourcePackageSet, L"_PJ", clipWriter->wProjectName));
        }

    }

    /* Preface - Identification */
    CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(Identification), &writer->identSet));
    CHK_ORET(mxf_add_array_item_strongref(writer->prefaceSet, &MXF_ITEM_K(Preface, Identifications), writer->identSet));
    CHK_ORET(mxf_set_uuid_item(writer->identSet, &MXF_ITEM_K(Identification, ThisGenerationUID), &thisGeneration));
    CHK_ORET(mxf_set_utf16string_item(writer->identSet, &MXF_ITEM_K(Identification, CompanyName), g_mxfIdentCompanyName));
    CHK_ORET(mxf_set_utf16string_item(writer->identSet, &MXF_ITEM_K(Identification, ProductName), g_mxfIdentProductName));
    CHK_ORET(mxf_set_utf16string_item(writer->identSet, &MXF_ITEM_K(Identification, VersionString), g_mxfIdentVersionString));
    CHK_ORET(mxf_set_uuid_item(writer->identSet, &MXF_ITEM_K(Identification, ProductUID), &g_mxfIdentProductUID));
    CHK_ORET(mxf_set_timestamp_item(writer->identSet, &MXF_ITEM_K(Identification, ModificationDate), &clipWriter->now));
    CHK_ORET(mxf_set_product_version_item(writer->identSet, &MXF_ITEM_K(Identification, ToolkitVersion), mxf_get_version()));
    CHK_ORET(mxf_set_utf16string_item(writer->identSet, &MXF_ITEM_K(Identification, Platform), mxf_get_platform_wstring()));
    
    
    return 1;
}

static int complete_track(AvidClipWriter* clipWriter, TrackWriter* writer, PackageDefinitions* packageDefinitions, Package* filePackage)
{
    MXFListIterator iter;
    MJPEGOffsetsArray* offsetsArray;
    int i;
    uint32_t j;
    MXFIndexEntry indexEntry;
    uint32_t numIndexEntries;
    int64_t filePos;

    
    /* finalise and close the essence element */
    
    writer->essenceLength = writer->essenceElement->totalLen;
    CHK_ORET(mxf_finalize_essence_element_write(writer->mxfFile, writer->essenceElement));
    mxf_close_essence_element(&writer->essenceElement);

    CHK_ORET(mxf_fill_to_kag(writer->mxfFile, writer->bodyPartition));
    
    
    /* write the footer partition pack */

    CHK_ORET(mxf_append_new_from_partition(writer->partitions, writer->headerPartition, &writer->footerPartition));
    writer->footerPartition->key = MXF_PP_K(ClosedComplete, Footer);
    writer->footerPartition->kagSize = 0x100;
    writer->footerPartition->indexSID = g_indexSID;

    CHK_ORET((filePos = mxf_file_tell(writer->mxfFile)) >= 0);
    CHK_ORET(mxf_write_partition(writer->mxfFile, writer->footerPartition));
    CHK_ORET(mxf_fill_to_position(writer->mxfFile, filePos + 199));

    
    /* write the index table segment */
    
    CHK_ORET(mxf_mark_index_start(writer->mxfFile, writer->footerPartition));
    
    
    if (writer->essenceType == AvidMJPEG)
    {
        /* Avid extension: last entry provides the length of the essence */
        CHK_ORET(add_avid_mjpeg_offset(&writer->mjpegFrameOffsets, writer->prevFrameOffset,
            &writer->currentMJPEGOffsetsArray));
            
        CHK_ORET(mxf_create_index_table_segment(&writer->indexSegment)); 
        mxf_generate_uuid(&writer->indexSegment->instanceUID);
        writer->indexSegment->indexEditRate = writer->sampleRate;
        writer->indexSegment->indexDuration = writer->duration;
        writer->indexSegment->indexSID = g_indexSID;
        writer->indexSegment->bodySID = g_bodySID;
        
        numIndexEntries = get_num_offsets(&writer->mjpegFrameOffsets);
        CHK_ORET(mxf_write_index_table_segment_header(writer->mxfFile, writer->indexSegment,
            0, numIndexEntries));
        
        if (numIndexEntries > 0)
        {
            CHK_ORET(mxf_avid_write_index_entry_array_header(writer->mxfFile, 0, 0,
                numIndexEntries));
                
            memset(&indexEntry, 0, sizeof(MXFIndexEntry));
            indexEntry.flags = 0x80; /* random access */
            
            mxf_initialise_list_iter(&iter, &writer->mjpegFrameOffsets);
            while (mxf_next_list_iter_element(&iter))
            {
                offsetsArray = (MJPEGOffsetsArray*)mxf_get_iter_element(&iter);
                for (j = 0; j < offsetsArray->len; j++)
                {
                    indexEntry.streamOffset = offsetsArray->offsets[j];
                    CHK_ORET(mxf_write_index_entry(writer->mxfFile, 0, 0, &indexEntry));
                }
            }
        }
    }
    else
    {
        CHK_ORET(mxf_create_index_table_segment(&writer->indexSegment)); 
        mxf_generate_uuid(&writer->indexSegment->instanceUID);
        writer->indexSegment->indexEditRate = writer->sampleRate;
        writer->indexSegment->indexDuration = writer->duration;
        writer->indexSegment->editUnitByteCount = writer->editUnitByteCount;
        writer->indexSegment->indexSID = g_indexSID;
        writer->indexSegment->bodySID = g_bodySID;
        
        CHK_ORET(mxf_write_index_table_segment(writer->mxfFile, writer->indexSegment));
    }
    
    CHK_ORET(mxf_fill_to_kag(writer->mxfFile, writer->footerPartition));
    
    CHK_ORET(mxf_mark_index_end(writer->mxfFile, writer->footerPartition));
    

    /* write the random index pack */
    CHK_ORET(mxf_write_rip(writer->mxfFile, writer->partitions));

    
    /* update the header metadata if the package definitions have changed */
    
    if (packageDefinitions != NULL)
    {
        writer->numDurationItems = 0;
        writer->descriptorSet = NULL;
        
        CHK_ORET(create_header_metadata(clipWriter, packageDefinitions, filePackage, writer));
    }
    
    
    /* update the header metadata with durations */
    
    for (i = 0; i < writer->numDurationItems; i++)
    {
        TrackWriter* trackWriter;
        CHK_ORET(get_track_writer(clipWriter, writer->durationItems[i].materialTrackID, &trackWriter));
        
        if (memcmp(&writer->durationItems[i].editRate, &trackWriter->sampleRate, sizeof(mxfRational)) == 0)
        {
            CHK_ORET(mxf_set_length_item(writer->durationItems[i].item->set, &writer->durationItems[i].item->key, 
                trackWriter->duration));
        }
        else
        {
            /* the duration is the number of samples at the file package track edit rate
            If the duration item is for a different track with a different edit rate (eg a material
            package track), then the duration must be converted */
            double factor = writer->durationItems[i].editRate.numerator * trackWriter->sampleRate.denominator / 
                (double)(writer->durationItems[i].editRate.denominator * trackWriter->sampleRate.numerator);
            CHK_ORET(mxf_set_length_item(writer->durationItems[i].item->set, &writer->durationItems[i].item->key, 
                (int64_t)(trackWriter->duration * factor + 0.5)));
        }
    }
    CHK_ORET(mxf_set_length_item(writer->descriptorSet, &MXF_ITEM_K(FileDescriptor, ContainerDuration), writer->duration));
    if (mxf_have_item(writer->descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageSize)))
    {
        CHK_ORET(mxf_set_int32_item(writer->descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageSize), (int32_t)writer->essenceLength));
    }
    

    
    /* re-write header metadata with avid extensions */

    CHK_ORET(mxf_file_seek(writer->mxfFile, writer->headerMetadataFilePos, SEEK_SET));
    
    CHK_ORET(mxf_mark_header_start(writer->mxfFile, writer->headerPartition));
    CHK_ORET(mxf_avid_write_header_metadata(writer->mxfFile, writer->headerMetadata));    
    if (writer->essenceType == Unc1080iUYVY ||
        writer->essenceType == Unc720pUYVY)
    {
        CHK_ORET(mxf_fill_to_position(writer->mxfFile, g_uncFixedBodyPPOffset));
    }
    else
    {
        CHK_ORET(mxf_fill_to_position(writer->mxfFile, g_fixedBodyPPOffset));
    }
    CHK_ORET(mxf_mark_header_end(writer->mxfFile, writer->headerPartition));

    
    /* update the partitions */
    writer->headerPartition->key = MXF_PP_K(ClosedComplete, Header);
    CHK_ORET(mxf_update_partitions(writer->mxfFile, writer->partitions));

    
    return 1;
}

/* TODO: check which essence container labels and essence element keys older Avids support */
static int create_track_writer(AvidClipWriter* clipWriter, PackageDefinitions* packageDefinitions, 
    Package* filePackage)
{
    TrackWriter* newTrackWriter = NULL;
    Track* track;
    MXFListIterator iter;
    int haveMaterialTrackRef;
    uint32_t i;
    int64_t filePos;
    
    CHK_ORET(filePackage->filename != NULL);
    CHK_ORET(clipWriter->projectFormat == PAL_25i || clipWriter->projectFormat == NTSC_30i);

    
    CHK_MALLOC_ORET(newTrackWriter, TrackWriter);
    memset(newTrackWriter, 0, sizeof(TrackWriter));
    mxf_initialise_list(&newTrackWriter->mjpegFrameOffsets, free_offsets_array_in_list);
    
    CHK_MALLOC_ARRAY_OFAIL(newTrackWriter->filename, char, strlen(filePackage->filename) + 1);
    strcpy(newTrackWriter->filename, filePackage->filename);
    
    newTrackWriter->essenceType = filePackage->essenceType;
    newTrackWriter->fileSourcePackageUID = filePackage->uid;
    
    
    /* get the material track id that references this file package */
    
    haveMaterialTrackRef = 0;
    mxf_initialise_list_iter(&iter, &packageDefinitions->materialPackage->tracks);
    while (mxf_next_list_iter_element(&iter))
    {
        track = (Track*)mxf_get_iter_element(&iter);
        if (mxf_equals_umid(&track->sourcePackageUID, &filePackage->uid))
        {
            newTrackWriter->materialTrackID = track->id;
            haveMaterialTrackRef = 1;
            break;
        }
    }
    CHK_OFAIL(haveMaterialTrackRef);
    
    
    /* set essence descriptor data */

    CHK_ORET(mxf_get_list_length(&filePackage->tracks) == 1);
    track = (Track*)mxf_get_list_element(&filePackage->tracks, 0);
    
    switch (filePackage->essenceType)
    {
        case AvidMJPEG:
            newTrackWriter->cdciEssenceContainerLabel = g_AvidAAFKLVEssenceContainer_ul;
            if (clipWriter->projectFormat == PAL_25i)
            {
                newTrackWriter->essenceContainerLabel = MXF_EC_L(AvidMJPEGClipWrapped);
                newTrackWriter->frameSize = 0; /* variable */
                switch (filePackage->essenceInfo.avidMJPEGInfo.resolution)
                {
                    case Res21:
                        newTrackWriter->resolutionID = g_AvidMJPEG21_ResolutionID;
                        newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(AvidMJPEG21);
                        newTrackWriter->videoLineMap[0] = 15;
                        newTrackWriter->videoLineMap[1] = 328;
                        newTrackWriter->videoLineMapLen = 2;
                        newTrackWriter->storedHeight = 296;
                        newTrackWriter->storedWidth = 720;
                        newTrackWriter->sampledHeight = 296;
                        newTrackWriter->sampledWidth = 720;
                        newTrackWriter->displayHeight = 288;
                        newTrackWriter->displayWidth = 720;
                        newTrackWriter->displayYOffset = 8;
                        newTrackWriter->displayXOffset = 0;
                        newTrackWriter->frameLayout = 1; /* SeparateFields */
                        newTrackWriter->colorSiting = 4; /* Rec601 */
                        break;
                    case Res31:
                        newTrackWriter->resolutionID = g_AvidMJPEG31_ResolutionID;
                        newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(AvidMJPEG31);
                        newTrackWriter->videoLineMap[0] = 15;
                        newTrackWriter->videoLineMap[1] = 328;
                        newTrackWriter->videoLineMapLen = 2;
                        newTrackWriter->storedHeight = 296;
                        newTrackWriter->storedWidth = 720;
                        newTrackWriter->sampledHeight = 296;
                        newTrackWriter->sampledWidth = 720;
                        newTrackWriter->displayHeight = 288;
                        newTrackWriter->displayWidth = 720;
                        newTrackWriter->displayYOffset = 8;
                        newTrackWriter->displayXOffset = 0;
                        newTrackWriter->frameLayout = 1; /* SeparateFields */
                        newTrackWriter->colorSiting = 4; /* Rec601 */
                        break;
                    case Res101:
                        newTrackWriter->resolutionID = g_AvidMJPEG101_ResolutionID;
                        newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(AvidMJPEG101);
                        newTrackWriter->videoLineMap[0] = 15;
                        newTrackWriter->videoLineMap[1] = 328;
                        newTrackWriter->videoLineMapLen = 2;
                        newTrackWriter->storedHeight = 296;
                        newTrackWriter->storedWidth = 720;
                        newTrackWriter->sampledHeight = 296;
                        newTrackWriter->sampledWidth = 720;
                        newTrackWriter->displayHeight = 288;
                        newTrackWriter->displayWidth = 720;
                        newTrackWriter->displayYOffset = 8;
                        newTrackWriter->displayXOffset = 0;
                        newTrackWriter->frameLayout = 1; /* SeparateFields */
                        newTrackWriter->colorSiting = 4; /* Rec601 */
                        break;
                    case Res101m:
                        newTrackWriter->resolutionID = g_AvidMJPEG101m_ResolutionID;
                        newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(AvidMJPEG101m);
                        newTrackWriter->videoLineMap[0] = 15;
                        newTrackWriter->videoLineMapLen = 1;
                        newTrackWriter->storedHeight = 296;
                        newTrackWriter->storedWidth = 288;
                        newTrackWriter->sampledHeight = 296;
                        newTrackWriter->sampledWidth = 288;
                        newTrackWriter->displayHeight = 288;
                        newTrackWriter->displayWidth = 288;
                        newTrackWriter->displayYOffset = 8;
                        newTrackWriter->displayXOffset = 0;
                        newTrackWriter->frameLayout = 2; /* SingleField */
                        newTrackWriter->colorSiting = 4; /* Rec601 */
                        break;
                    case Res151s:
                        newTrackWriter->resolutionID = g_AvidMJPEG151s_ResolutionID;
                        newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(AvidMJPEG151s);
                        newTrackWriter->videoLineMap[0] = 15;
                        newTrackWriter->videoLineMapLen = 1;
                        newTrackWriter->storedHeight = 296;
                        newTrackWriter->storedWidth = 352;
                        newTrackWriter->sampledHeight = 296;
                        newTrackWriter->sampledWidth = 352;
                        newTrackWriter->displayHeight = 288;
                        newTrackWriter->displayWidth = 352;
                        newTrackWriter->displayYOffset = 8;
                        newTrackWriter->displayXOffset = 0;
                        newTrackWriter->frameLayout = 2; /* SingleField */
                        newTrackWriter->colorSiting = 4; /* Rec601 */
                        break;
                    case Res201:
                        newTrackWriter->resolutionID = g_AvidMJPEG201_ResolutionID;
                        newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(AvidMJPEG201);
                        newTrackWriter->videoLineMap[0] = 15;
                        newTrackWriter->videoLineMap[1] = 328;
                        newTrackWriter->videoLineMapLen = 2;
                        newTrackWriter->storedHeight = 296;
                        newTrackWriter->storedWidth = 720;
                        newTrackWriter->sampledHeight = 296;
                        newTrackWriter->sampledWidth = 720;
                        newTrackWriter->displayHeight = 288;
                        newTrackWriter->displayWidth = 720;
                        newTrackWriter->displayYOffset = 8;
                        newTrackWriter->displayXOffset = 0;
                        newTrackWriter->frameLayout = 1; /* SeparateFields */
                        newTrackWriter->colorSiting = 4; /* Rec601 */
                        break;
                    default:
                        assert(0);
                        return 0;
                }
                newTrackWriter->horizSubsampling = 2;
                newTrackWriter->vertSubsampling = 1;
            }
            else
            {
                /* TODO */
                mxf_log(MXF_ELOG, "Avid MJPEG NTSC not yet implemented" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
                assert(0);
                return 0;
            }
            newTrackWriter->essenceElementKey = MXF_EE_K(AvidMJPEGClipWrapped);
            newTrackWriter->sourceTrackNumber = g_AvidMJPEGTrackNumber;
            newTrackWriter->essenceElementLLen = 8;
            CHK_ORET(memcmp(&track->editRate, &clipWriter->projectEditRate, sizeof(mxfRational)) == 0); /* why would it be different? */
            newTrackWriter->sampleRate = track->editRate;
            newTrackWriter->editUnitByteCount = newTrackWriter->frameSize;
            break;
        case DVBased25:
            newTrackWriter->cdciEssenceContainerLabel = g_Null_UL;
            if (clipWriter->projectFormat == PAL_25i)
            {
                /* TODO: this Avid label fails in Xpress HD 5.2.1, but what about earlier versions?
                  newTrackWriter->essenceContainerLabel = g_avid_DV25ClipWrappedEssenceContainer_label; */
                newTrackWriter->essenceContainerLabel = MXF_EC_L(DVBased_25_625_50_ClipWrapped);
                newTrackWriter->frameSize = 144000;
                newTrackWriter->resolutionID = 0x8d;
                newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(DVBased_25_625_50);
                newTrackWriter->storedHeight = 288;
                newTrackWriter->storedWidth = 720;
                newTrackWriter->videoLineMap[0] = 23;
                newTrackWriter->videoLineMap[1] = 335;
                newTrackWriter->videoLineMapLen = 2;
                newTrackWriter->colorSiting = 4; /* Rec601 */
                if (clipWriter->useLegacy)
                {
                    newTrackWriter->horizSubsampling = 2;
                    newTrackWriter->vertSubsampling = 2;
                    newTrackWriter->frameLayout = 3; /* legacy MixedFields */
                }
                else
                {
                    newTrackWriter->horizSubsampling = 4;
                    newTrackWriter->vertSubsampling = 1;
                    newTrackWriter->frameLayout = 1; /* SeparateFields */
                }
            }
            else
            {
                newTrackWriter->essenceContainerLabel = MXF_EC_L(DVBased_25_525_60_ClipWrapped);
                newTrackWriter->frameSize = 120000;
                newTrackWriter->resolutionID = 0x8c;
                newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(DVBased_25_525_60);
                newTrackWriter->storedHeight = 240;
                newTrackWriter->storedWidth = 720;
                newTrackWriter->videoLineMap[0] = 23;
                newTrackWriter->videoLineMap[1] = 285;
                newTrackWriter->videoLineMapLen = 2;
                newTrackWriter->colorSiting = 4; /* Rec601 */
                if (clipWriter->useLegacy)
                {
                    newTrackWriter->horizSubsampling = 1;
                    newTrackWriter->vertSubsampling = 1;
                }
                else
                {
                    newTrackWriter->horizSubsampling = 4;
                    newTrackWriter->vertSubsampling = 1;
                }
                newTrackWriter->frameLayout = 1;
            }
            newTrackWriter->displayHeight = 0;
            newTrackWriter->displayWidth = 0;
            newTrackWriter->essenceElementKey = MXF_EE_K(DVClipWrapped);
            newTrackWriter->sourceTrackNumber = MXF_DV_TRACK_NUM(0x01, MXF_DV_CLIP_WRAPPED_EE_TYPE, 0x01);
            newTrackWriter->essenceElementLLen = 8;
            CHK_ORET(memcmp(&track->editRate, &clipWriter->projectEditRate, sizeof(mxfRational)) == 0); /* why would it be different? */
            newTrackWriter->sampleRate = track->editRate;
            newTrackWriter->editUnitByteCount = newTrackWriter->frameSize;
            break;
        case DVBased50:
            newTrackWriter->cdciEssenceContainerLabel = g_Null_UL;
            if (clipWriter->projectFormat == PAL_25i)
            {
                newTrackWriter->essenceContainerLabel = MXF_EC_L(DVBased_50_625_50_ClipWrapped);
                newTrackWriter->frameSize = 288000;
                newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(DVBased_50_625_50);
                newTrackWriter->storedHeight = 288;
                newTrackWriter->storedWidth = 720;
                newTrackWriter->videoLineMap[0] = 23;
                newTrackWriter->videoLineMap[1] = 335;
                newTrackWriter->videoLineMapLen = 2;
                newTrackWriter->horizSubsampling = 2;
                newTrackWriter->vertSubsampling = 1;
                newTrackWriter->colorSiting = 4; /* Rec601 */
                if (clipWriter->useLegacy)
                {
                    newTrackWriter->frameLayout = 3; /* legacy MixedFields */
                }
                else
                {
                    newTrackWriter->frameLayout = 1; /* SeparateFields */
                }
            }
            else
            {
                newTrackWriter->essenceContainerLabel = MXF_EC_L(DVBased_50_525_60_ClipWrapped);
                newTrackWriter->frameSize = 240000;
                newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(DVBased_50_525_60);
                newTrackWriter->storedHeight = 240;
                newTrackWriter->storedWidth = 720;
                newTrackWriter->videoLineMap[0] = 23;
                newTrackWriter->videoLineMap[1] = 285;
                newTrackWriter->videoLineMapLen = 2;
                newTrackWriter->horizSubsampling = 2;
                newTrackWriter->vertSubsampling = 1;
                newTrackWriter->frameLayout = 1;
                newTrackWriter->colorSiting = 4; /* Rec601 */
            }
            newTrackWriter->displayHeight = 0;
            newTrackWriter->displayWidth = 0;
            newTrackWriter->essenceElementKey = MXF_EE_K(DVClipWrapped);
            newTrackWriter->sourceTrackNumber = MXF_DV_TRACK_NUM(0x01, MXF_DV_CLIP_WRAPPED_EE_TYPE, 0x01);
            newTrackWriter->essenceElementLLen = 8;
            CHK_ORET(memcmp(&track->editRate, &clipWriter->projectEditRate, sizeof(mxfRational)) == 0); /* why would it be different? */
            newTrackWriter->sampleRate = track->editRate;
            newTrackWriter->resolutionID = 0x8e;
            newTrackWriter->editUnitByteCount = newTrackWriter->frameSize;
            break;
        case DV1080i50:
        case DV720p50:
            newTrackWriter->cdciEssenceContainerLabel = g_AvidAAFKLVEssenceContainer_ul;
            newTrackWriter->videoLineMap[0] = 21;
            newTrackWriter->videoLineMap[1] = 584;
            newTrackWriter->videoLineMapLen = 2;
            newTrackWriter->storedHeight = 540;
            newTrackWriter->storedWidth = 1920;
            newTrackWriter->displayHeight = 540;
            newTrackWriter->displayWidth = 1920;
            newTrackWriter->displayYOffset = 0;
            newTrackWriter->displayXOffset = 0;
            newTrackWriter->frameLayout = 1; /* SeparateFields */
            newTrackWriter->colorSiting = 4; /* Rec601 */
            newTrackWriter->horizSubsampling = 2;
            newTrackWriter->vertSubsampling = 1;
            newTrackWriter->frameSize = 576000;

            switch (filePackage->essenceType)
            {
                case DV1080i50:        /* SMPTE 370M */
                    newTrackWriter->essenceElementKey = MXF_EE_K(DV1080i50);
                    newTrackWriter->essenceContainerLabel = MXF_EC_L(DV1080i50ClipWrapped);
                    newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(DV1080i50);
                    break;
                case DV720p50:        /* Not part of SMPTE 370M but being shipped by Panasonic */
                    newTrackWriter->essenceElementKey = MXF_EE_K(DV720p50);
                    newTrackWriter->essenceContainerLabel = MXF_EC_L(DV720p50ClipWrapped);
                    newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(DV720p50);
                    break;
                default:
                    assert(0);
                    return 0;
            }
            /* Note: no ResolutionID is set in DV100 files */

            newTrackWriter->sourceTrackNumber = MXF_DV_TRACK_NUM(0x01, MXF_DV_CLIP_WRAPPED_EE_TYPE, 0x01);
            newTrackWriter->essenceElementLLen = 9;
            CHK_ORET(memcmp(&track->editRate, &clipWriter->projectEditRate, sizeof(mxfRational)) == 0); /* why would it be different? */
            newTrackWriter->sampleRate = track->editRate;
            newTrackWriter->editUnitByteCount = newTrackWriter->frameSize;
            break;
        case IMX30:
        case IMX40:
        case IMX50:
            newTrackWriter->cdciEssenceContainerLabel = g_Null_UL;
            if (clipWriter->projectFormat == PAL_25i)
            {
                switch (filePackage->essenceType)
                {
                    case IMX30:
                        newTrackWriter->frameSize = 150000;
                        newTrackWriter->essenceContainerLabel = MXF_EC_L(IMX30);
                        newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(D10_50_625_30);
                        newTrackWriter->resolutionID = 162;
                        break;
                    case IMX40:
                        newTrackWriter->frameSize = 200000;
                        newTrackWriter->essenceContainerLabel = MXF_EC_L(IMX40);
                        newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(D10_50_625_40);
                        newTrackWriter->resolutionID = 161;
                        break;
                    case IMX50:
                        newTrackWriter->frameSize = 250000;
                        newTrackWriter->essenceContainerLabel = MXF_EC_L(IMX50);
                        newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(D10_50_625_50);
                        newTrackWriter->resolutionID = 160;
                        break;
                    default:
                        assert(0);
                        return 0;
                }
                newTrackWriter->displayHeight = 288;
                newTrackWriter->displayWidth = 720;
                newTrackWriter->displayYOffset = 16;
                newTrackWriter->displayXOffset = 0;
                newTrackWriter->storedHeight = 304;
                newTrackWriter->storedWidth = 720;
                newTrackWriter->videoLineMap[0] = 7;
                newTrackWriter->videoLineMap[1] = 320;
                newTrackWriter->videoLineMapLen = 2;
                newTrackWriter->horizSubsampling = 2;
                newTrackWriter->vertSubsampling = 1;
                newTrackWriter->colorSiting = 4; /* Rec601 */
                newTrackWriter->frameLayout = 1; /* SeparateFields */
            }
            else
            {
                mxf_log(MXF_ELOG, "IMX NTSC not yet implemented" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
                assert(0);
            }
            newTrackWriter->essenceElementKey = MXF_EE_K(IMX);
            newTrackWriter->sourceTrackNumber = MXF_D10_PICTURE_TRACK_NUM(0x01);
            newTrackWriter->essenceElementLLen = 8;
            CHK_ORET(memcmp(&track->editRate, &clipWriter->projectEditRate, sizeof(mxfRational)) == 0);
            newTrackWriter->sampleRate = track->editRate;
            newTrackWriter->editUnitByteCount = newTrackWriter->frameSize;
            break;
        case DNxHD1080i120:
        case DNxHD1080i185:
        case DNxHD1080p36:
        case DNxHD1080p120:
        case DNxHD1080p185:
        case DNxHD720p120:
        case DNxHD720p185:
            newTrackWriter->cdciEssenceContainerLabel = g_AvidAAFKLVEssenceContainer_ul;
            newTrackWriter->videoLineMap[0] = 21;
            newTrackWriter->videoLineMap[1] = 584;
            newTrackWriter->videoLineMapLen = 2;
            newTrackWriter->storedHeight = 540;
            newTrackWriter->storedWidth = 1920;
            newTrackWriter->displayHeight = 540;
            newTrackWriter->displayWidth = 1920;
            newTrackWriter->displayYOffset = 0;
            newTrackWriter->displayXOffset = 0;
            newTrackWriter->frameLayout = 1; /* SeparateFields */
            newTrackWriter->colorSiting = 4; /* Rec601 */
            newTrackWriter->horizSubsampling = 2;
            newTrackWriter->vertSubsampling = 1;
            newTrackWriter->imageAlignmentOffset = 8192;

            switch (filePackage->essenceType)
            {
                case DNxHD1080i120:        /* DNxHD 1920x1080 50i 120MBps */
                    newTrackWriter->essenceElementKey = MXF_EE_K(DNxHD);
                    newTrackWriter->essenceContainerLabel = MXF_EC_L(DNxHD1080i120ClipWrapped);
                    newTrackWriter->resolutionID = 1242;
                    newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(DNxHD);
                    newTrackWriter->frameSize = 606208;
                    break;
                case DNxHD1080i185:        /* DNxHD 1920x1080 50i 185MBps */
                    newTrackWriter->essenceElementKey = MXF_EE_K(DNxHD);
                    newTrackWriter->essenceContainerLabel = MXF_EC_L(DNxHD1080i185ClipWrapped);
                    newTrackWriter->resolutionID = 1243;
                    newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(DNxHD);
                    newTrackWriter->frameSize = 917504;
                    break;
                case DNxHD1080p36:         /* DNxHD 1920x1080 25p 36MBps */
                    newTrackWriter->videoLineMap[0] = 42;
                    newTrackWriter->videoLineMap[1] = 0;
                    newTrackWriter->storedHeight = 1080;
                    newTrackWriter->displayHeight = 1080;
                    newTrackWriter->frameLayout = 0; /* FullFrame */
                    newTrackWriter->essenceElementKey = MXF_EE_K(DNxHD);
                    newTrackWriter->essenceContainerLabel = MXF_EC_L(DNxHD1080p36ClipWrapped);
                    newTrackWriter->resolutionID = 1253;
                    newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(DNxHD);
                    newTrackWriter->frameSize = 188416;
                    break;
                case DNxHD1080p120:        /* DNxHD 1920x1080 25p 120MBps */
                    newTrackWriter->videoLineMap[0] = 42;
                    newTrackWriter->videoLineMap[1] = 0;
                    newTrackWriter->storedHeight = 1080;
                    newTrackWriter->displayHeight = 1080;
                    newTrackWriter->frameLayout = 0; /* FullFrame */
                    newTrackWriter->essenceElementKey = MXF_EE_K(DNxHD);
                    newTrackWriter->essenceContainerLabel = MXF_EC_L(DNxHD1080p120ClipWrapped);
                    newTrackWriter->resolutionID = 1237;
                    newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(DNxHD);
                    newTrackWriter->frameSize = 606208;
                    break;
                case DNxHD1080p185:        /* DNxHD 1920x1080 25p 185MBps */
                    newTrackWriter->videoLineMap[0] = 42;
                    newTrackWriter->videoLineMap[1] = 0;
                    newTrackWriter->storedHeight = 1080;
                    newTrackWriter->displayHeight = 1080;
                    newTrackWriter->frameLayout = 0; /* FullFrame */
                    newTrackWriter->essenceElementKey = MXF_EE_K(DNxHD);
                    newTrackWriter->essenceContainerLabel = MXF_EC_L(DNxHD1080p185ClipWrapped);
                    newTrackWriter->resolutionID = 1238;
                    newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(DNxHD);
                    newTrackWriter->frameSize = 917504;
                    break;
                case DNxHD720p120:        /* DNxHD 1280x720 50p 120MBps */
                    newTrackWriter->videoLineMap[0] = 26;
                    newTrackWriter->videoLineMap[1] = 0;
                    newTrackWriter->storedWidth = 1280;
                    newTrackWriter->storedHeight = 720;
                    newTrackWriter->displayWidth = 1280;
                    newTrackWriter->displayHeight = 720;
                    newTrackWriter->frameLayout = 0; /* FullFrame */
                    newTrackWriter->essenceElementKey = MXF_EE_K(DNxHD);
                    newTrackWriter->essenceContainerLabel = MXF_EC_L(DNxHD720p120ClipWrapped);
                    newTrackWriter->resolutionID = 1252;
                    newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(DNxHD);
                    newTrackWriter->frameSize = 303104;
                    break;
                case DNxHD720p185:        /* DNxHD 1280x720 50p 185MBps */
                    newTrackWriter->videoLineMap[0] = 26;
                    newTrackWriter->videoLineMap[1] = 0;
                    newTrackWriter->storedWidth = 1280;
                    newTrackWriter->storedHeight = 720;
                    newTrackWriter->displayWidth = 1280;
                    newTrackWriter->displayHeight = 720;
                    newTrackWriter->frameLayout = 0; /* FullFrame */
                    newTrackWriter->essenceElementKey = MXF_EE_K(DNxHD);
                    newTrackWriter->essenceContainerLabel = MXF_EC_L(DNxHD720p185ClipWrapped);
                    newTrackWriter->resolutionID = 1251;
                    newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(DNxHD);
                    newTrackWriter->frameSize = 458752;
                    break;
                default:
                    assert(0);
                    return 0;
            }
            newTrackWriter->sourceTrackNumber = g_DNxHDTrackNumber;
            newTrackWriter->essenceElementLLen = 9;
            CHK_ORET(memcmp(&track->editRate, &clipWriter->projectEditRate, sizeof(mxfRational)) == 0); /* why would it be different? */
            newTrackWriter->sampleRate = track->editRate;
            newTrackWriter->editUnitByteCount = newTrackWriter->frameSize;
            break;
        case UncUYVY:
            /* TODO: check whether the same thing must be done as MJPEG regarding the ess container label 
            in the descriptor */
            newTrackWriter->cdciEssenceContainerLabel = g_AvidAAFKLVEssenceContainer_ul;
            if (clipWriter->projectFormat == PAL_25i)
            {
                newTrackWriter->essenceContainerLabel = MXF_EC_L(SD_Unc_625_50i_422_135_ClipWrapped);
                newTrackWriter->frameSize = g_uncAlignedPALFrameSize;
                newTrackWriter->storedHeight = 592;
                newTrackWriter->storedWidth = 720;
                newTrackWriter->displayHeight = 576;
                newTrackWriter->displayWidth = 720;
                newTrackWriter->displayYOffset = 16;
                newTrackWriter->displayXOffset = 0;
                newTrackWriter->videoLineMap[0] = 15;
                newTrackWriter->videoLineMap[1] = 328;
                newTrackWriter->videoLineMapLen = 2;
                newTrackWriter->horizSubsampling = 2;
                newTrackWriter->vertSubsampling = 1;
                newTrackWriter->colorSiting = 4; /* Rec601 */
                newTrackWriter->frameLayout = 3; /* MixedFields */
                newTrackWriter->imageAlignmentOffset = g_uncImageAlignmentOffset;
                newTrackWriter->imageStartOffset = g_uncPALStartOffsetSize;
                
                CHK_MALLOC_ARRAY_OFAIL(newTrackWriter->vbiData, uint8_t, g_uncPALVBISize);
                for (i = 0; i < g_uncPALVBISize / 4; i++)
                {
                    newTrackWriter->vbiData[i * 4] = 0x80; // U
                    newTrackWriter->vbiData[i * 4 + 1] = 0x10; // Y
                    newTrackWriter->vbiData[i * 4 + 2] = 0x80; // V
                    newTrackWriter->vbiData[i * 4 + 3] = 0x10; // Y 
                }
                CHK_MALLOC_ARRAY_OFAIL(newTrackWriter->startOffsetData, uint8_t, g_uncPALStartOffsetSize);
                memset(newTrackWriter->startOffsetData, 0, g_uncPALStartOffsetSize);
            }
            else
            {
                /* TODO */
                mxf_log(MXF_ELOG, "Uncompressed NTSC not yet implemented" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
                assert(0);
                return 0;
            }
            newTrackWriter->essenceElementKey = MXF_EE_K(UncClipWrapped);
            newTrackWriter->sourceTrackNumber = MXF_UNC_TRACK_NUM(0x01, MXF_UNC_CLIP_WRAPPED_EE_TYPE, 0x01);
            newTrackWriter->essenceElementLLen = 8;
            CHK_ORET(memcmp(&track->editRate, &clipWriter->projectEditRate, sizeof(mxfRational)) == 0); /* why would it be different? */
            newTrackWriter->sampleRate = track->editRate;
            newTrackWriter->resolutionID = 0xaa;
            newTrackWriter->editUnitByteCount = newTrackWriter->frameSize;
            break;
        case Unc1080iUYVY:
            newTrackWriter->cdciEssenceContainerLabel = g_AvidAAFKLVEssenceContainer_ul;
            if (clipWriter->projectFormat == PAL_25i)
            {
                newTrackWriter->essenceContainerLabel = MXF_EC_L(HD_Unc_1080_50i_422_ClipWrapped);
                newTrackWriter->frameSize = g_uncAligned1080i50FrameSize;
                newTrackWriter->storedHeight = 1080;
                newTrackWriter->storedWidth = 1920;
                newTrackWriter->displayHeight = 1080;
                newTrackWriter->displayWidth = 1920;
                newTrackWriter->displayYOffset = 0;
                newTrackWriter->displayXOffset = 0;
                newTrackWriter->videoLineMap[0] = 21;
                newTrackWriter->videoLineMap[1] = 584;
                newTrackWriter->videoLineMapLen = 2;
                newTrackWriter->horizSubsampling = 2;
                newTrackWriter->vertSubsampling = 1;
                newTrackWriter->colorSiting = 4; /* Rec601 */
                newTrackWriter->frameLayout = 3; /* MixedFields */
                newTrackWriter->imageAlignmentOffset = g_uncImageAlignmentOffset;
                newTrackWriter->imageStartOffset = g_unc1080i50StartOffsetSize;

                CHK_MALLOC_ARRAY_OFAIL(newTrackWriter->startOffsetData, uint8_t, g_unc1080i50StartOffsetSize);
                memset(newTrackWriter->startOffsetData, 0, g_unc1080i50StartOffsetSize);
            }
            else
            {
                /* TODO */
                mxf_log(MXF_ELOG, "Uncompressed 1080i NTSC not yet implemented" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
                assert(0);
                return 0;
            }
            newTrackWriter->essenceElementKey = MXF_EE_K(UncClipWrapped);
            newTrackWriter->sourceTrackNumber = MXF_UNC_TRACK_NUM(0x01, MXF_UNC_CLIP_WRAPPED_EE_TYPE, 0x01);
            newTrackWriter->essenceElementLLen = 9;
            CHK_ORET(memcmp(&track->editRate, &clipWriter->projectEditRate, sizeof(mxfRational)) == 0); /* why would it be different? */
            newTrackWriter->sampleRate = track->editRate;
            newTrackWriter->editUnitByteCount = newTrackWriter->frameSize;
            break;
        case PCM:
            newTrackWriter->samplingRate.numerator = 48000;
            newTrackWriter->samplingRate.denominator = 1;
            CHK_ORET(memcmp(&track->editRate, &clipWriter->projectEditRate, sizeof(mxfRational)) == 0 ||
                memcmp(&track->editRate, &newTrackWriter->samplingRate, sizeof(mxfRational)) == 0); /* why would it be different? */
            newTrackWriter->sampleRate = track->editRate;
            newTrackWriter->essenceContainerLabel = MXF_EC_L(BWFClipWrapped);
            newTrackWriter->essenceElementKey = MXF_EE_K(BWFClipWrapped);
            newTrackWriter->sourceTrackNumber = MXF_AES3BWF_TRACK_NUM(0x01, MXF_BWF_CLIP_WRAPPED_EE_TYPE, 0x01);
            newTrackWriter->essenceElementLLen = 8;
            newTrackWriter->bitsPerSample = filePackage->essenceInfo.pcmInfo.bitsPerSample;
            newTrackWriter->blockAlign = (uint8_t)((newTrackWriter->bitsPerSample + 7) / 8);
            newTrackWriter->avgBps = newTrackWriter->blockAlign * 48000;
            if (memcmp(&newTrackWriter->sampleRate, &newTrackWriter->samplingRate, sizeof(mxfRational)) == 0)
            {
                newTrackWriter->editUnitByteCount = newTrackWriter->blockAlign;
            }
            else
            {
                double factor = newTrackWriter->samplingRate.numerator * newTrackWriter->sampleRate.denominator / 
                    (double)(newTrackWriter->samplingRate.denominator * newTrackWriter->sampleRate.numerator);
                newTrackWriter->editUnitByteCount = (uint32_t)(newTrackWriter->blockAlign * factor + 0.5);
            }
            break;
        default:
            assert(0);
    };

    if (clipWriter->useLegacy)
    {
        newTrackWriter->pictureDataDef = MXF_DDEF_L(LegacyPicture);
        newTrackWriter->soundDataDef = MXF_DDEF_L(LegacySound);
        newTrackWriter->timecodeDataDef = MXF_DDEF_L(LegacyTimecode);
    }
    else
    {
        newTrackWriter->pictureDataDef = MXF_DDEF_L(Picture);
        newTrackWriter->soundDataDef = MXF_DDEF_L(Sound);
        newTrackWriter->timecodeDataDef = MXF_DDEF_L(Timecode);
    }
    

    /* create the header metadata */
    
    CHK_OFAIL(create_header_metadata(clipWriter, packageDefinitions, filePackage, newTrackWriter));

    
    /* open the file */
    
    CHK_OFAIL(mxf_create_file_partitions(&newTrackWriter->partitions));
    CHK_OFAIL(mxf_disk_file_open_new(newTrackWriter->filename, &newTrackWriter->mxfFile));
    
    
    /* set the minimum llen */
    if (filePackage->essenceType == AvidMJPEG)
    {
        /* Avid only accepts llen == 9 for MJPEG files */
        mxf_file_set_min_llen(newTrackWriter->mxfFile, 9);
    }
    else
    {
        mxf_file_set_min_llen(newTrackWriter->mxfFile, 4);
    }
    
    
    /* write the (incomplete) header partition pack */
    
    CHK_OFAIL(mxf_append_new_partition(newTrackWriter->partitions, &newTrackWriter->headerPartition));
    newTrackWriter->headerPartition->key = MXF_PP_K(ClosedIncomplete, Header);
    newTrackWriter->headerPartition->kagSize = 0x100;
    newTrackWriter->headerPartition->operationalPattern = MXF_OP_L(atom, complexity02);
    CHK_OFAIL(mxf_append_partition_esscont_label(newTrackWriter->headerPartition, 
        &newTrackWriter->essenceContainerLabel));

    CHK_OFAIL(mxf_write_partition(newTrackWriter->mxfFile, newTrackWriter->headerPartition));
    CHK_OFAIL(mxf_fill_to_kag(newTrackWriter->mxfFile, newTrackWriter->headerPartition));

    
    /* write header metadata with avid extensions */

    CHK_OFAIL((newTrackWriter->headerMetadataFilePos = mxf_file_tell(newTrackWriter->mxfFile)) >= 0);
    
    CHK_OFAIL(mxf_mark_header_start(newTrackWriter->mxfFile, newTrackWriter->headerPartition));
    CHK_OFAIL(mxf_avid_write_header_metadata(newTrackWriter->mxfFile, newTrackWriter->headerMetadata));    
    if (newTrackWriter->essenceType == Unc1080iUYVY ||
        newTrackWriter->essenceType == Unc720pUYVY)
    {
        CHK_ORET(mxf_fill_to_position(newTrackWriter->mxfFile, g_uncFixedBodyPPOffset));
    }
    else
    {
        CHK_ORET(mxf_fill_to_position(newTrackWriter->mxfFile, g_fixedBodyPPOffset));
    }
    CHK_OFAIL(mxf_mark_header_end(newTrackWriter->mxfFile, newTrackWriter->headerPartition));

    
    /* write the body partition pack */

    CHK_OFAIL(mxf_append_new_from_partition(newTrackWriter->partitions, newTrackWriter->headerPartition, &newTrackWriter->bodyPartition));
    newTrackWriter->bodyPartition->key = MXF_PP_K(ClosedComplete, Body);
    if (newTrackWriter->essenceType == PCM) /* audio */
    {
        newTrackWriter->bodyPartition->kagSize = 0x200;
    }
    else /* video */
    {
        newTrackWriter->bodyPartition->kagSize = 0x20000;
    }
    newTrackWriter->bodyPartition->bodySID = g_bodySID;

    CHK_OFAIL((filePos = mxf_file_tell(newTrackWriter->mxfFile)) >= 0);
    CHK_OFAIL(mxf_write_partition(newTrackWriter->mxfFile, newTrackWriter->bodyPartition));
    if (newTrackWriter->essenceType == Unc1080iUYVY ||
        newTrackWriter->essenceType == Unc720pUYVY)
    {
        /* place the first byte of the essence data at 0x8000.
        Note: 57 = 0x20 + 16 + 9. This assumes a 9 byte llen and that the partition pack 
        position is 0x20 beyond 0x6000 (don't know why?) */        
        CHK_OFAIL(mxf_fill_to_position(newTrackWriter->mxfFile, filePos + newTrackWriter->bodyPartition->kagSize - 57));
    }
    else
    {
        /* TODO: it would make sense to make this filePos + newTrackWriter->bodyPartition->kagSize - 57 as well 
        Must first check that this doesn't break other formats */
        CHK_OFAIL(mxf_fill_to_position(newTrackWriter->mxfFile, filePos + 199));
    }
    
    
    /* update the partitions */
    
    CHK_OFAIL((filePos = mxf_file_tell(newTrackWriter->mxfFile)) >= 0);
    CHK_OFAIL(mxf_update_partitions(newTrackWriter->mxfFile, newTrackWriter->partitions));
    CHK_OFAIL(mxf_file_seek(newTrackWriter->mxfFile, filePos, SEEK_SET));

    
    /* open the essence element, ready for writing */
    
    CHK_OFAIL(mxf_open_essence_element_write(newTrackWriter->mxfFile, &newTrackWriter->essenceElementKey, 
        newTrackWriter->essenceElementLLen, 0, &newTrackWriter->essenceElement));

        
    /* set the new writer */
    
    clipWriter->tracks[clipWriter->numTracks] = newTrackWriter;
    clipWriter->numTracks++;

    
    return 1;
    
fail:
    free_track_writer(&newTrackWriter);
    return 0;
}




int create_clip_writer(const char* projectName, ProjectFormat projectFormat,
    mxfRational imageAspectRatio, mxfRational projectEditRate, int dropFrameFlag, int useLegacy, 
    PackageDefinitions* packageDefinitions, AvidClipWriter** clipWriter)
{
    AvidClipWriter* newClipWriter = NULL;
    MXFListIterator iter;

    CHK_ORET(mxf_get_list_length(&packageDefinitions->materialPackage->tracks) <= MAX_TRACKS);
    
    
    CHK_MALLOC_ORET(newClipWriter, AvidClipWriter);
    memset(newClipWriter, 0, sizeof(AvidClipWriter));

    if (projectName != NULL)
    {
        CHK_OFAIL(convert_string(newClipWriter, projectName));
        newClipWriter->wProjectName = newClipWriter->wTmpString;
        newClipWriter->wTmpString = NULL;
    }
    newClipWriter->projectFormat = projectFormat;
    newClipWriter->imageAspectRatio.numerator = imageAspectRatio.numerator;
    newClipWriter->imageAspectRatio.denominator = imageAspectRatio.denominator;
    newClipWriter->dropFrameFlag = dropFrameFlag;
    newClipWriter->useLegacy = useLegacy;

    newClipWriter->projectEditRate.numerator = projectEditRate.numerator;
    newClipWriter->projectEditRate.denominator = projectEditRate.denominator;

    /* create track writer for each file package */
    mxf_initialise_list_iter(&iter, &packageDefinitions->fileSourcePackages);
    while (mxf_next_list_iter_element(&iter))
    {
        CHK_OFAIL(create_track_writer(newClipWriter, packageDefinitions, (Package*)mxf_get_iter_element(&iter)));
    }

    *clipWriter = newClipWriter;
    return 1;
    
fail:
    free_avid_clip_writer(&newClipWriter);
    return 0;
}
    
int write_samples(AvidClipWriter* clipWriter, uint32_t materialTrackID, uint32_t numSamples,
    uint8_t* data, uint32_t size)
{
    TrackWriter* writer;
    CHK_ORET(get_track_writer(clipWriter, materialTrackID, &writer));
    
    switch (writer->essenceType)
    {
        case AvidMJPEG:
            CHK_ORET(numSamples == 1);
            /* update frame offsets array */
            CHK_ORET(add_avid_mjpeg_offset(&writer->mjpegFrameOffsets, writer->prevFrameOffset,
                &writer->currentMJPEGOffsetsArray));
            writer->prevFrameOffset += size;
            CHK_ORET(mxf_write_essence_element_data(writer->mxfFile, writer->essenceElement, data, size));
            writer->duration += numSamples;
            break;
        case DVBased25:
        case DVBased50:
        case DV1080i50:
        case DV720p50:
        case IMX30:
        case IMX40:
        case IMX50:
        case DNxHD720p120:
        case DNxHD720p185:
        case DNxHD1080i120:
        case DNxHD1080i185:
        case DNxHD1080p36:
        case DNxHD1080p120:
        case DNxHD1080p185:
        case PCM:
            CHK_ORET(size == numSamples * writer->editUnitByteCount);
            CHK_ORET(mxf_write_essence_element_data(writer->mxfFile, writer->essenceElement, data, size));
            writer->duration += numSamples;
            break;
        case UncUYVY:
            CHK_ORET(numSamples == 1);
            CHK_ORET((size + g_uncPALStartOffsetSize + g_uncPALVBISize) == 
                numSamples * writer->editUnitByteCount);
            /* write start offset for alignment */
            CHK_ORET(mxf_write_essence_element_data(writer->mxfFile, writer->essenceElement, writer->startOffsetData, 
                g_uncPALStartOffsetSize));
            /* write VBI */
            CHK_ORET(mxf_write_essence_element_data(writer->mxfFile, writer->essenceElement, writer->vbiData, 
                g_uncPALVBISize));
            CHK_ORET(mxf_write_essence_element_data(writer->mxfFile, writer->essenceElement, data, size));
            writer->duration += numSamples;
            break;
        case Unc1080iUYVY:
            CHK_ORET(numSamples == 1);
            CHK_ORET((size + g_unc1080i50StartOffsetSize) == numSamples * writer->editUnitByteCount);
            /* write start offset for alignment */
            CHK_ORET(mxf_write_essence_element_data(writer->mxfFile, writer->essenceElement, writer->startOffsetData, 
                g_unc1080i50StartOffsetSize));
            CHK_ORET(mxf_write_essence_element_data(writer->mxfFile, writer->essenceElement, data, size));
            writer->duration += numSamples;
            break;
        default:
            assert(0);
    }
    
    return 1;
}


int start_write_samples(AvidClipWriter* clipWriter, uint32_t materialTrackID)
{
    TrackWriter* writer;
    CHK_ORET(get_track_writer(clipWriter, materialTrackID, &writer));
    
    writer->sampleDataSize = 0;    

    return 1;
}

int write_sample_data(AvidClipWriter* clipWriter, uint32_t materialTrackID, uint8_t* data, uint32_t size)
{
    TrackWriter* writer;
    CHK_ORET(get_track_writer(clipWriter, materialTrackID, &writer));

    if (writer->essenceType == UncUYVY && writer->sampleDataSize == 0)
    {
        /* write start offset for alignment */
        CHK_ORET(mxf_write_essence_element_data(writer->mxfFile, writer->essenceElement, writer->startOffsetData, 
            g_uncPALStartOffsetSize));
        /* write VBI */
        CHK_ORET(mxf_write_essence_element_data(writer->mxfFile, writer->essenceElement, writer->vbiData, 
            g_uncPALVBISize));
    }
    else if (writer->essenceType == Unc1080iUYVY && writer->sampleDataSize == 0)
    {
        /* write start offset for alignment */
        CHK_ORET(mxf_write_essence_element_data(writer->mxfFile, writer->essenceElement, writer->startOffsetData, 
            g_uncPALStartOffsetSize));
    }
    else
    {
        CHK_ORET(mxf_write_essence_element_data(writer->mxfFile, writer->essenceElement, data, size));
    }
    
    writer->sampleDataSize += size;
    return 1;
}

int end_write_samples(AvidClipWriter* clipWriter, uint32_t materialTrackID, uint32_t numSamples)
{
    TrackWriter* writer;
    CHK_ORET(get_track_writer(clipWriter, materialTrackID, &writer));
    
    switch (writer->essenceType)
    {
        case AvidMJPEG:
            /* Avid MJPEG has variable size sample and indexing currently accepts only 1 sample at a time */
            CHK_ORET(numSamples == 1);
            /* update frame offsets array */
            CHK_ORET(add_avid_mjpeg_offset(&writer->mjpegFrameOffsets, writer->prevFrameOffset,
                &writer->currentMJPEGOffsetsArray));
            writer->prevFrameOffset += writer->sampleDataSize;
            writer->duration += numSamples;
            break;
        case DVBased25:
        case DVBased50:
        case DV1080i50:
        case DV720p50:
        case IMX30:
        case IMX40:
        case IMX50:
        case DNxHD720p120:
        case DNxHD720p185:
        case DNxHD1080i120:
        case DNxHD1080i185:
        case DNxHD1080p36:
        case DNxHD1080p120:
        case DNxHD1080p185:
        case PCM:
            CHK_ORET(writer->sampleDataSize == numSamples * writer->editUnitByteCount);
            writer->duration += numSamples;
            break;
        case UncUYVY:
            /* Avid uncompressed SD video requires padding and VBI and currently only accepts 1 sample at a time */
            CHK_ORET(numSamples == 1);
            CHK_ORET((writer->sampleDataSize + g_uncPALStartOffsetSize + g_uncPALVBISize) == 
                numSamples * writer->editUnitByteCount);
            writer->duration += numSamples;
            break;
        case Unc1080iUYVY:
            /* Avid uncompressed HD video requires padding and currently only accepts 1 sample at a time */
            CHK_ORET(numSamples == 1);
            CHK_ORET((writer->sampleDataSize + g_unc1080i50StartOffsetSize) == numSamples * writer->editUnitByteCount);
            writer->duration += numSamples;
            break;
        default:
            assert(0);
            return 0;
    }
    
    return 1;
}


void abort_writing(AvidClipWriter** clipWriter, int deleteFile)
{
    int i;
    TrackWriter* trackWriter;

    for (i = 0; i < (*clipWriter)->numTracks; i++)
    {
        trackWriter = (*clipWriter)->tracks[i];

        mxf_file_close(&trackWriter->mxfFile);
        
        if (deleteFile)
        {
            if (remove(trackWriter->filename) != 0)
            {
                mxf_log(MXF_WLOG, "Failed to delete MXF file '%s'" LOG_LOC_FORMAT, trackWriter->filename, LOG_LOC_PARAMS);
            }
        }
    }

    free_avid_clip_writer(clipWriter);
    return;
}

int complete_writing(AvidClipWriter** clipWriter)
{
    return update_and_complete_writing(clipWriter, NULL, NULL);
}

int update_and_complete_writing(AvidClipWriter** clipWriter, PackageDefinitions* packageDefinitions, const char* projectName)
{
    int i;
    Package* filePackage = NULL;

    if (packageDefinitions != NULL)
    {
        if (projectName != NULL)
        {
            SAFE_FREE(&(*clipWriter)->wProjectName);
            
            CHK_ORET(convert_string((*clipWriter), projectName));
            (*clipWriter)->wProjectName = (*clipWriter)->wTmpString;
            (*clipWriter)->wTmpString = NULL;
        }
    }
    
    for (i = 0; i < (*clipWriter)->numTracks; i++)
    {
        if (packageDefinitions != NULL)
        {
            CHK_ORET(get_file_package((*clipWriter)->tracks[i], packageDefinitions, &filePackage));
            CHK_ORET(complete_track(*clipWriter, (*clipWriter)->tracks[i], packageDefinitions, filePackage));
        }
        else
        {
            CHK_ORET(complete_track(*clipWriter, (*clipWriter)->tracks[i], NULL, NULL));
        }
    }
    
    free_avid_clip_writer(clipWriter);
    
    return 1;
}


