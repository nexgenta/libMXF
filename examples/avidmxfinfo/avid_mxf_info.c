/*
 * $Id: avid_mxf_info.c,v 1.8 2009/10/13 09:21:51 philipn Exp $
 *
 * Parse metadata from an Avid MXF file
 *
 * Copyright (C) 2008  Philip de Nier <philipn@users.sourceforge.net>
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
#include <mxf/mxf_uu_metadata.h>

#include "avid_mxf_info.h"


#define DEBUG_PRINT_ERROR(cmd) \
    if (printDebugError) \
    { \
        fprintf(stderr, "'%s' failed in %s, line %d\n", #cmd, __FILE__, __LINE__); \
    } \

#define CHECK(cmd, ecode) \
    if (!(cmd)) \
    { \
        DEBUG_PRINT_ERROR(cmd); \
        errorCode = ecode; \
        goto fail; \
    }

#define DCHECK(cmd) \
    if (!(cmd)) \
    { \
        DEBUG_PRINT_ERROR(cmd); \
        errorCode = -1; \
        goto fail; \
    }

#define FCHECK(cmd) \
    if (!(cmd)) \
    { \
        DEBUG_PRINT_ERROR(cmd); \
        goto fail; \
    }

    
typedef struct
{
    uint32_t first;
    uint32_t last;
} TrackNumberRange;

    
static void print_umid(const mxfUMID* umid)
{
    printf("%02x%02x%02x%02x%02x%02x%02x%02x"
        "%02x%02x%02x%02x%02x%02x%02x%02x" 
        "%02x%02x%02x%02x%02x%02x%02x%02x" 
        "%02x%02x%02x%02x%02x%02x%02x%02x", 
        umid->octet0, umid->octet1, umid->octet2, umid->octet3,
        umid->octet4, umid->octet5, umid->octet6, umid->octet7,
        umid->octet8, umid->octet9, umid->octet10, umid->octet11,
        umid->octet12, umid->octet13, umid->octet14, umid->octet15,
        umid->octet16, umid->octet17, umid->octet18, umid->octet19,
        umid->octet20, umid->octet21, umid->octet22, umid->octet23,
        umid->octet24, umid->octet25, umid->octet26, umid->octet27,
        umid->octet28, umid->octet29, umid->octet30, umid->octet31);
}
    
static void print_timestamp(const mxfTimestamp* timestamp)
{
    printf("%d-%02u-%02u %02u:%02u:%02u.%03u", 
        timestamp->year, timestamp->month, timestamp->day,
        timestamp->hour, timestamp->min, timestamp->sec, timestamp->qmsec * 4);
}

static void print_label(const mxfUL* label)
{
    printf("%02x%02x%02x%02x%02x%02x%02x%02x"
        "%02x%02x%02x%02x%02x%02x%02x%02x", 
        label->octet0, label->octet1, label->octet2, label->octet3,
        label->octet4, label->octet5, label->octet6, label->octet7,
        label->octet8, label->octet9, label->octet10, label->octet11,
        label->octet12, label->octet13, label->octet14, label->octet15);
}
 
static void print_timecode(int64_t timecode, const mxfRational* sampleRate)
{
    int hour, min, sec, frame;
    int roundedTimecodeBase = (int)(sampleRate->numerator / (double)sampleRate->denominator + 0.5);
    
    hour = (int)(timecode / (60 * 60 * roundedTimecodeBase));
    min = (int)((timecode % (60 * 60 * roundedTimecodeBase)) / (60 * roundedTimecodeBase));
    sec = (int)(((timecode % (60 * 60 * roundedTimecodeBase)) % (60 * roundedTimecodeBase)) / roundedTimecodeBase);
    frame = (int)(((timecode % (60 * 60 * roundedTimecodeBase)) % (60 * roundedTimecodeBase)) % roundedTimecodeBase);
    
    printf("%02d:%02d:%02d:%02d", hour, min, sec, frame);
}

static int convert_string(const mxfUTF16Char* utf16Str, char** str, int printDebugError)
{
    size_t utf8Size;
    
    utf8Size = wcstombs(0, utf16Str, 0);
    FCHECK(utf8Size != (size_t)(-1));
    utf8Size += 1;
    FCHECK((*str = malloc(utf8Size)) != NULL);
    wcstombs(*str, utf16Str, utf8Size);

    return 1;
    
fail:
    return 0;
}

static int get_string_value(MXFMetadataSet* set, const mxfKey* itemKey, char** str, int printDebugError)
{
    uint16_t utf16Size;
    mxfUTF16Char* utf16Str = NULL;
    
    FCHECK(mxf_get_utf16string_item_size(set, itemKey, &utf16Size));
    FCHECK((utf16Str = malloc(utf16Size * sizeof(mxfUTF16Char))) != NULL);
    FCHECK(mxf_get_utf16string_item(set, itemKey, utf16Str));
    
    FCHECK(convert_string(utf16Str, str, printDebugError));
    
    SAFE_FREE(&utf16Str);
    return 1;
    
fail:
    SAFE_FREE(&utf16Str);
    return 0;
}

static int get_single_track_component(MXFMetadataSet* trackSet, const mxfKey* componentSetKey, MXFMetadataSet** componentSet, int printDebugError)
{
    MXFMetadataSet* sequenceSet = NULL;
    MXFMetadataSet* cSet = NULL;
    uint32_t componentCount;
    uint8_t* arrayElementValue;

    /* get the single component in the sequence which is a subclass of componentSetKey */
    
    FCHECK(mxf_get_strongref_item(trackSet, &MXF_ITEM_K(GenericTrack, Sequence), &sequenceSet));
    if (mxf_set_is_subclass_of(sequenceSet, &MXF_SET_K(Sequence)))
    {
        /* is a sequence, so we get the first component */
    
        FCHECK(mxf_get_array_item_count(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), &componentCount));
        if (componentCount != 1)
        {
            /* empty sequence or > 1 are not what we expect */
            return 0;
        }
        
        /* get first component */
        
        FCHECK(mxf_get_array_item_element(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), 0, &arrayElementValue)); 
        if (!mxf_get_strongref(trackSet->headerMetadata, arrayElementValue, &cSet))
        {
            /* reference to a dark set and we assume it isn't something we're interested in */
            return 0;
        }
    }
    else
    {
        /* something other than a sequence */
        cSet = sequenceSet;
    }
    
    if (!mxf_set_is_subclass_of(cSet, componentSetKey))
    {
        /* not a componentSetKey component */
        return 0;
    }
    
    *componentSet = cSet;
    return 1;
    
fail:
    return 0;
}

static const char* frame_layout_string(uint8_t frameLayout)
{
    static const char* frameLayoutStrings[] = 
        {"full frame", "separate fields", "single field", "mixed field", "segmented frame"};
    static const char* unknownFrameLyoutString = "unknown layout";
        
    if (frameLayout < sizeof(frameLayoutStrings))
    {
        return frameLayoutStrings[frameLayout];
    }
    return unknownFrameLyoutString;
}

static int64_t convert_length(const mxfRational* targetEditRate, const mxfRational* editRate, int64_t length)
{
    return (int64_t)(length * 
        targetEditRate->numerator * editRate->denominator /
            (double)(targetEditRate->denominator * editRate->numerator) + 0.5);
}
    
static int64_t compare_length(const mxfRational* editRateA, int64_t lengthA, const mxfRational* editRateB, int64_t lengthB)
{
    return lengthA - convert_length(editRateA, editRateB, lengthB);
}

static const char* essence_type_string(AvidEssenceType essenceType)
{
    /* keep this in sync with AvidEssenceType in the header file */
    static const char* essenceTypeStrings[] = 
    {
        "not recognized", 
        "MPEG 30", "MPEG 40", "MPEG 50", 
        "DV 25 411", "DV 25 420", "DV 50", "DV 100",
        "20:1", "15:1s", "10:1", "10:1m", "4:1m", "3:1", "2:1",
        "1:1",
        "DNxHD 185", "DNxHD 120", "DNxHD 36",
        "PCM"
    };
    assert(essenceType < sizeof(essenceTypeStrings) / sizeof(const char*));
    assert(PCM_ESSENCE_TYPE < sizeof(essenceTypeStrings) / sizeof(const char*));
    
    return essenceTypeStrings[essenceType];
}

static void insert_track_number(TrackNumberRange* trackNumbers, uint32_t trackNumber, int* numTrackNumberRanges)
{
    int i;
    int j;
    
    for (i = 0; i < *numTrackNumberRanges; i++)
    {
        if (trackNumber < trackNumbers[i].first - 1)
        {
            /* insert new track range */
            for (j = *numTrackNumberRanges - 1; j >= i; j--)
            {
                trackNumbers[j + 1] = trackNumbers[j];
            }
            trackNumbers[i].first = trackNumber;
            trackNumbers[i].last = trackNumber;
            
            (*numTrackNumberRanges)++;
            return;
        }
        else if (trackNumber == trackNumbers[i].first - 1)
        {
            /* extend range back one */
            trackNumbers[i].first = trackNumber;
            return;
        }
        else if (trackNumber == trackNumbers[i].last + 1)
        {
            if (i + 1 < *numTrackNumberRanges &&
                trackNumber == trackNumbers[i + 1].first - 1)
            {
                /* merge range forwards */
                trackNumbers[i + 1].first = trackNumbers[i].first;
                for (j = i; j < *numTrackNumberRanges - 1; j++)
                {
                    trackNumbers[j] = trackNumbers[j + 1];
                }
                (*numTrackNumberRanges)--;
            }
            else
            {
                /* extend range forward one */
                trackNumbers[i].last = trackNumber;
            }

            return;
        }
        else if (trackNumber == trackNumbers[i].first ||
            trackNumber == trackNumbers[i].last)
        {
            /* duplicate */
            return;
        }
    }

    
    /* append new track range */
    trackNumbers[i].first = trackNumber;
    trackNumbers[i].last = trackNumber;
    
    (*numTrackNumberRanges)++;
    return;
}

static size_t get_range_string(char* buffer, size_t maxSize, int isFirst, int isVideo, const TrackNumberRange* range)
{
    size_t strLen = 0;
    
    if (isFirst)
    {
#if defined(_MSC_VER)
        strLen = _snprintf(buffer, maxSize, "%s", (isVideo) ? "V" : "A");
#else
        strLen = snprintf(buffer, maxSize, "%s", (isVideo) ? "V" : "A");
#endif
        buffer += strLen;
        maxSize -= strLen;
    }
    else
    {
#if defined(_MSC_VER)
        strLen = _snprintf(buffer, maxSize, ",");
#else
        strLen = snprintf(buffer, maxSize, ",");
#endif
        buffer += strLen;
        maxSize -= strLen;
    }
    
    
    if (range->first == range->last)
    {
#if defined(_MSC_VER)
        strLen += _snprintf(buffer, maxSize, "%d", range->first);
#else
        strLen += snprintf(buffer, maxSize, "%d", range->first);
#endif
    }
    else
    {
#if defined(_MSC_VER)
        strLen += _snprintf(buffer, maxSize, "%d-%d", range->first, range->last);
#else
        strLen += snprintf(buffer, maxSize, "%d-%d", range->first, range->last);
#endif
    }
    
    return strLen;
}



int ami_read_info(const char* filename, AvidMXFInfo* info, int printDebugError)
{
    int errorCode = -1;
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    uint32_t sequenceComponentCount;
    uint8_t* arrayElement;
    MXFList* list = NULL;
    MXFListIterator listIter;
    MXFListIterator namesIter;
    MXFListIterator valuesIter;
    MXFArrayItemIterator arrayIter;
    MXFFile* mxfFile = NULL;
    MXFPartition* headerPartition = NULL;
    MXFDataModel* dataModel = NULL;
    MXFHeaderMetadata* headerMetadata = NULL;
    MXFMetadataSet* set = NULL;
    MXFMetadataSet* prefaceSet = NULL;
    MXFMetadataSet* fileSourcePackageSet = NULL;
    MXFMetadataSet* materialPackageSet = NULL;
    MXFMetadataSet* descriptorSet = NULL;
    MXFMetadataSet* physicalSourcePackageSet = NULL;
    MXFMetadataSet* materialPackageTrackSet = NULL;
    MXFMetadataSet* trackSet = NULL;
    MXFMetadataSet* sourceClipSet = NULL;
    MXFMetadataSet* sequenceSet = NULL;
    MXFMetadataSet* timecodeComponentSet = NULL;
    MXFMetadataSet* refSourcePackageSet = NULL;
    mxfUMID packageUID;
    MXFList* taggedValueNames = NULL;
    MXFList* taggedValueValues = NULL;
    const mxfUTF16Char* taggedValue;
    int index;
    mxfUL dataDef;
    mxfUMID sourcePackageID;
    MXFArrayItemIterator iter3;
    int64_t filePackageStartPosition;
    int64_t startPosition;
    mxfRational filePackageEditRate;
    int64_t startTimecode;
    uint16_t roundedTimecodeBase;
    int haveStartTimecode;
    mxfRational editRate;
    int64_t trackDuration;
    int64_t segmentDuration;
    int64_t segmentOffset;
    mxfRational maxEditRate = {25, 1};
    int64_t maxDuration = 0;
    int32_t avidResolutionID = 0x00;
    mxfUL pictureEssenceCoding = g_Null_UL;
    TrackNumberRange videoTrackNumberRanges[64];
    int numVideoTrackNumberRanges = 0;
    TrackNumberRange audioTrackNumberRanges[64];
    int numAudioTrackNumberRanges = 0;
    uint32_t trackNumber;
    char tracksString[256] = {0};
    int i;
    size_t remSize;
    char* tracksStringPtr;
    size_t strLen;
    
    memset(info, 0, sizeof(AvidMXFInfo));
    info->frameLayout = 0xff; /* unknown (0 is known) */


    /* open file */
    
    CHECK(mxf_disk_file_open_read(filename, &mxfFile), -2);
    
    
    /* read header partition pack */
    
    CHECK(mxf_read_header_pp_kl(mxfFile, &key, &llen, &len), -3);
    CHECK(mxf_read_partition(mxfFile, &key, &headerPartition), -3);
    
    
    /* check is OP-Atom */
    
    CHECK(is_op_atom(&headerPartition->operationalPattern), -4);
    
    
    /* read the header metadata (filter out meta-dictionary and dictionary except data defs) */
    
    DCHECK(mxf_load_data_model(&dataModel));
    DCHECK(mxf_load_extensions_data_model(dataModel));
    DCHECK(mxf_avid_load_extensions(dataModel));
    
    DCHECK(mxf_finalise_data_model(dataModel));
    
    DCHECK(mxf_read_next_nonfiller_kl(mxfFile, &key, &llen, &len));
    DCHECK(mxf_is_header_metadata(&key));
    DCHECK(mxf_create_header_metadata(&headerMetadata, dataModel));
    DCHECK(mxf_avid_read_filtered_header_metadata(mxfFile, 0, headerMetadata, headerPartition->headerByteCount, &key, llen, len));
    
    
    /* get the preface and info */
    
    DCHECK(mxf_find_singular_set_by_key(headerMetadata, &MXF_SET_K(Preface), &prefaceSet));
    if (mxf_have_item(prefaceSet, &MXF_ITEM_K(Preface, ProjectName)))
    {
        DCHECK(get_string_value(prefaceSet, &MXF_ITEM_K(Preface, ProjectName), &info->projectName, printDebugError));
    }
    if (mxf_have_item(prefaceSet, &MXF_ITEM_K(Preface, ProjectEditRate)))
    {
        DCHECK(mxf_get_rational_item(prefaceSet, &MXF_ITEM_K(Preface, ProjectEditRate), &info->projectEditRate));
    }
    
    
    /* get the material package and info */
    
    DCHECK(mxf_find_singular_set_by_key(headerMetadata, &MXF_SET_K(MaterialPackage), &materialPackageSet));
    DCHECK(mxf_get_umid_item(materialPackageSet, &MXF_ITEM_K(GenericPackage, PackageUID), &info->materialPackageUID));
    if (mxf_have_item(materialPackageSet, &MXF_ITEM_K(GenericPackage, Name)))
    {
        DCHECK(get_string_value(materialPackageSet, &MXF_ITEM_K(GenericPackage, Name), &info->clipName, printDebugError));
    }
    if (mxf_have_item(materialPackageSet, &MXF_ITEM_K(GenericPackage, PackageCreationDate)))
    {
        DCHECK(mxf_get_timestamp_item(materialPackageSet, &MXF_ITEM_K(GenericPackage, PackageCreationDate), &info->clipCreated));
    }
    
    
    /* get the material package project name tagged value if not already set */
    
    if (info->projectName == NULL &&
        mxf_have_item(materialPackageSet, &MXF_ITEM_K(GenericPackage, MobAttributeList)))
    {
        DCHECK(mxf_avid_read_string_mob_attributes(materialPackageSet, &taggedValueNames, &taggedValueValues));
        if (mxf_avid_get_mob_attribute(L"_PJ", taggedValueNames, taggedValueValues, &taggedValue))
        {
            DCHECK(convert_string(taggedValue, &info->projectName, printDebugError));
        }
    }
    mxf_free_list(&taggedValueNames);
    mxf_free_list(&taggedValueValues);
    
    
    /* get the material package user comments */
    
    if (mxf_have_item(materialPackageSet, &MXF_ITEM_K(GenericPackage, UserComments)))
    {
        DCHECK(mxf_avid_read_string_user_comments(materialPackageSet, &taggedValueNames, &taggedValueValues));
        info->numUserComments = (int)mxf_get_list_length(taggedValueNames);
        
        DCHECK((info->userComments = (AvidNameValuePair*)malloc(info->numUserComments * sizeof(AvidNameValuePair))) != NULL);
        memset(info->userComments, 0, info->numUserComments * sizeof(AvidNameValuePair));
        
        index = 0;
        mxf_initialise_list_iter(&namesIter, taggedValueNames);
        mxf_initialise_list_iter(&valuesIter, taggedValueValues);
        while (mxf_next_list_iter_element(&namesIter) && mxf_next_list_iter_element(&valuesIter))
        {
            DCHECK(convert_string((const mxfUTF16Char*)mxf_get_iter_element(&namesIter), 
                &info->userComments[index].name, printDebugError));
            DCHECK(convert_string((const mxfUTF16Char*)mxf_get_iter_element(&valuesIter), 
                &info->userComments[index].value, printDebugError));
            
            index++;
        }
        
    }
    mxf_free_list(&taggedValueNames);
    mxf_free_list(&taggedValueValues);
    
    
    /* get the material package attributes */
    
    if (mxf_have_item(materialPackageSet, &MXF_ITEM_K(GenericPackage, MobAttributeList)))
    {
        DCHECK(mxf_avid_read_string_mob_attributes(materialPackageSet, &taggedValueNames, &taggedValueValues));
        info->numMaterialPackageAttributes = (int)mxf_get_list_length(taggedValueNames);
        
        DCHECK((info->materialPackageAttributes = (AvidNameValuePair*)malloc(info->numMaterialPackageAttributes * sizeof(AvidNameValuePair))) != NULL);
        memset(info->materialPackageAttributes, 0, info->numMaterialPackageAttributes * sizeof(AvidNameValuePair));
        
        index = 0;
        mxf_initialise_list_iter(&namesIter, taggedValueNames);
        mxf_initialise_list_iter(&valuesIter, taggedValueValues);
        while (mxf_next_list_iter_element(&namesIter) && mxf_next_list_iter_element(&valuesIter))
        {
            DCHECK(convert_string((const mxfUTF16Char*)mxf_get_iter_element(&namesIter), 
                &info->materialPackageAttributes[index].name, printDebugError));
            DCHECK(convert_string((const mxfUTF16Char*)mxf_get_iter_element(&valuesIter), 
                &info->materialPackageAttributes[index].value, printDebugError));
            
            index++;
        }
        
    }
    mxf_free_list(&taggedValueNames);
    mxf_free_list(&taggedValueValues);
    
    
    /* get the top level file source package and info */
    
    DCHECK(mxf_uu_get_top_file_package(headerMetadata, &fileSourcePackageSet));
    DCHECK(mxf_get_umid_item(fileSourcePackageSet, &MXF_ITEM_K(GenericPackage, PackageUID), &info->fileSourcePackageUID));

    
    /* get the file source package essence descriptor info */
    
    DCHECK(mxf_get_strongref_item(fileSourcePackageSet, &MXF_ITEM_K(SourcePackage, Descriptor), &descriptorSet));
    if (mxf_is_subclass_of(dataModel, &descriptorSet->key, &MXF_SET_K(GenericPictureEssenceDescriptor)))
    {
        /* image aspect ratio */
        if (mxf_have_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, AspectRatio)))
        {
            DCHECK(mxf_get_rational_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, AspectRatio), &info->aspectRatio));
        }

        /* frame layout */
        if (mxf_have_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameLayout)))
        {
            DCHECK(mxf_get_uint8_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameLayout), &info->frameLayout));
        }
        
        /* stored width and height */
        if (mxf_have_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, StoredWidth)))
        {
            DCHECK(mxf_get_uint32_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, StoredWidth), &info->storedWidth));
        }
        if (mxf_have_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, StoredHeight)))
        {
            DCHECK(mxf_get_uint32_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, StoredHeight), &info->storedHeight));
        }

        /* display width and height */
        if (mxf_have_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayWidth)))
        {
            DCHECK(mxf_get_uint32_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayWidth), &info->displayWidth));
        }
        if (mxf_have_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayHeight)))
        {
            DCHECK(mxf_get_uint32_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayHeight), &info->displayHeight));
        }
        
        /* Avid resolution Id */
        if (mxf_have_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, ResolutionID)))
        {
            DCHECK(mxf_get_int32_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, ResolutionID), &avidResolutionID));
        }

        /* picture essence coding label */
        if (mxf_have_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, PictureEssenceCoding)))
        {
            DCHECK(mxf_get_ul_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, PictureEssenceCoding), &pictureEssenceCoding));
        }
    }
    else if (mxf_is_subclass_of(dataModel, &descriptorSet->key, &MXF_SET_K(GenericSoundEssenceDescriptor)))
    {
        /* audio sampling rate */
        if (mxf_have_item(descriptorSet, &MXF_ITEM_K(GenericSoundEssenceDescriptor, AudioSamplingRate)))
        {
            DCHECK(mxf_get_rational_item(descriptorSet, &MXF_ITEM_K(GenericSoundEssenceDescriptor, AudioSamplingRate), &info->audioSamplingRate));
        }
        /* quantization bits */
        if (mxf_have_item(descriptorSet, &MXF_ITEM_K(GenericSoundEssenceDescriptor, QuantizationBits)))
        {
            DCHECK(mxf_get_uint32_item(descriptorSet, &MXF_ITEM_K(GenericSoundEssenceDescriptor, QuantizationBits), &info->quantizationBits));
        }
        /* channel count */
        if (mxf_have_item(descriptorSet, &MXF_ITEM_K(GenericSoundEssenceDescriptor, ChannelCount)))
        {
            DCHECK(mxf_get_uint32_item(descriptorSet, &MXF_ITEM_K(GenericSoundEssenceDescriptor, ChannelCount), &info->channelCount));
        }
    }
    

    /* get the material track referencing the file source package and info */
    /* get the clip duration ( = duration of the longest track) */

    DCHECK(mxf_uu_get_package_tracks(materialPackageSet, &arrayIter));
    while (mxf_uu_next_track(headerMetadata, &arrayIter, &materialPackageTrackSet))
    {
        DCHECK(mxf_uu_get_track_datadef(materialPackageTrackSet, &dataDef));
        
        /* some Avid files have a weak reference to a DataDefinition instead of a UL */ 
        if (!mxf_is_picture(&dataDef) && !mxf_is_sound(&dataDef) && !mxf_is_timecode(&dataDef))
        {
            if (!mxf_avid_get_data_def(headerMetadata, (mxfUUID*)&dataDef, &dataDef))
            {
                continue;
            }
        }
        
        /* skip non-video and audio tracks */
        if (!mxf_is_picture(&dataDef) && !mxf_is_sound(&dataDef))
        {
            continue;
        }
        
        /* track counts */
        if (mxf_is_picture(&dataDef))
        {
            info->numVideoTracks++;
        }
        else
        {
            info->numAudioTracks++;
        }
        
        /* track number */
        if (mxf_have_item(materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber)))
        {
            DCHECK(mxf_get_uint32_item(materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), &trackNumber));
            if (mxf_is_picture(&dataDef))
            {
                insert_track_number(videoTrackNumberRanges, trackNumber, &numVideoTrackNumberRanges);
            }
            else
            {
                insert_track_number(audioTrackNumberRanges, trackNumber, &numAudioTrackNumberRanges);
            }
        }
        else
        {
            trackNumber = 0;
        }
        
        /* edit rate */
        DCHECK(mxf_get_rational_item(materialPackageTrackSet, &MXF_ITEM_K(Track, EditRate), &editRate));
        
        /* track duration */
        DCHECK(mxf_uu_get_track_duration(materialPackageTrackSet, &trackDuration));
    
        /* get max duration for the clip duration */            
        if (compare_length(&maxEditRate, maxDuration, &editRate, trackDuration) <= 0)
        {
            maxEditRate = editRate;
            maxDuration = trackDuration;
        }
    
        /* assume the project edit rate equals the video edit rate if not set */
        if (memcmp(&info->projectEditRate, &g_Null_Rational, sizeof(g_Null_Rational)) == 0 &&
            mxf_is_picture(&dataDef))
        {
            info->projectEditRate = editRate;
        }
    
        /* get the file source package if this track references it through a child source clip */
        packageUID = g_Null_UMID;
        segmentOffset = 0;
        CHK_ORET(mxf_get_strongref_item(materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), &sequenceSet));
        if (!mxf_is_subclass_of(sequenceSet->headerMetadata->dataModel, &sequenceSet->key, &MXF_SET_K(SourceClip)))
        {
            /* iterate through sequence components */
            CHK_ORET(mxf_get_array_item_count(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), &sequenceComponentCount));
            for (i = 0; i < (int)sequenceComponentCount; i++)
            {
                CHK_ORET(mxf_get_array_item_element(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), i, &arrayElement));
                if (!mxf_get_strongref(sequenceSet->headerMetadata, arrayElement, &sourceClipSet))
                {
                    /* dark set not registered in the dictionary */
                    continue;
                }

                CHK_ORET(mxf_get_length_item(sourceClipSet, &MXF_ITEM_K(StructuralComponent, Duration), &segmentDuration));
                
                if (mxf_is_subclass_of(sourceClipSet->headerMetadata->dataModel, &sourceClipSet->key, &MXF_SET_K(SourceClip)))
                {
                    CHK_ORET(mxf_get_umid_item(sourceClipSet, &MXF_ITEM_K(SourceClip, SourcePackageID), &packageUID));
                    if (mxf_equals_umid(&info->fileSourcePackageUID, &packageUID))
                    {
                        break;
                    }
                }
                
                segmentOffset += segmentDuration;
            }
        }
        else
        {
            /* track sequence is a source clip */
            sourceClipSet = sequenceSet;
            CHK_ORET(mxf_get_umid_item(sourceClipSet, &MXF_ITEM_K(SourceClip, SourcePackageID), &packageUID));
            CHK_ORET(mxf_get_length_item(sourceClipSet, &MXF_ITEM_K(StructuralComponent, Duration), &segmentDuration));
            segmentOffset = 0;
        }

        if (mxf_equals_umid(&info->fileSourcePackageUID, &packageUID))
        {
            info->isVideo = mxf_is_picture(&dataDef);
            info->editRate = editRate;
            info->trackDuration = trackDuration;
            info->trackNumber = trackNumber;
            info->segmentDuration = segmentDuration;
            info->segmentOffset = segmentOffset;
        }
    }

    
    info->clipDuration = convert_length(&info->projectEditRate, &maxEditRate, maxDuration);

    
    
    /* construct the tracks string */
    
    remSize = sizeof(tracksString);
    tracksStringPtr = tracksString;
    for (i = 0; i < numVideoTrackNumberRanges; i++)
    {
        if (remSize < 4)
        {
            break;
        }
        
        strLen = get_range_string(tracksStringPtr, remSize, i == 0, 1, &videoTrackNumberRanges[i]);

        tracksStringPtr += strLen;
        remSize -= strLen;
    }
    if (numVideoTrackNumberRanges > 0 && numAudioTrackNumberRanges > 0)
    {
#if defined(_MSC_VER)
        strLen = _snprintf(tracksStringPtr, remSize, " ");
#else
        strLen = snprintf(tracksStringPtr, remSize, " ");
#endif
        tracksStringPtr += strLen;
        remSize -= strLen;
    }
    for (i = 0; i < numAudioTrackNumberRanges; i++)
    {
        if (remSize < 4)
        {
            break;
        }
        strLen = get_range_string(tracksStringPtr, remSize, i == 0, 0, &audioTrackNumberRanges[i]);

        tracksStringPtr += strLen;
        remSize -= strLen;
    }
    
    strLen = strlen(tracksString);
    DCHECK((info->tracksString = (char*)malloc(strLen + 1)) != NULL);
    strcpy(info->tracksString, tracksString);

    

    /* get the physical source package and info */
    
    DCHECK(mxf_find_set_by_key(headerMetadata, &MXF_SET_K(SourcePackage), &list));
    mxf_initialise_list_iter(&listIter, list);
    while (mxf_next_list_iter_element(&listIter))
    {
        set = (MXFMetadataSet*)mxf_get_iter_element(&listIter);
        
        /* the physical source package is the source package that references a physical descriptor */
        if (mxf_have_item(set, &MXF_ITEM_K(SourcePackage, Descriptor)))
        {
            /* NOTE/TODO: some descriptors could be dark and so we don't assume we can dereference */
            if (mxf_get_strongref_item(set, &MXF_ITEM_K(SourcePackage, Descriptor), &descriptorSet) &&
                mxf_is_subclass_of(dataModel, &descriptorSet->key, &MXF_SET_K(PhysicalDescriptor)))
            {
                if (mxf_is_subclass_of(dataModel, &descriptorSet->key, &MXF_SET_K(TapeDescriptor)))
                {
                    info->physicalPackageType = TAPE_PHYS_TYPE;
                }
                else if (mxf_is_subclass_of(dataModel, &descriptorSet->key, &MXF_SET_K(ImportDescriptor)))
                {
                    info->physicalPackageType = IMPORT_PHYS_TYPE;
                }
                else if (mxf_is_subclass_of(dataModel, &descriptorSet->key, &MXF_SET_K(RecordingDescriptor)))
                {
                    info->physicalPackageType = RECORDING_PHYS_TYPE;
                }
                else
                {
                    info->physicalPackageType = UNKNOWN_PHYS_TYPE;
                }
                physicalSourcePackageSet = set;
                
                DCHECK(mxf_get_umid_item(physicalSourcePackageSet, &MXF_ITEM_K(GenericPackage, PackageUID), &info->physicalSourcePackageUID));
                if (mxf_have_item(physicalSourcePackageSet, &MXF_ITEM_K(GenericPackage, Name)))
                {
                    DCHECK(get_string_value(physicalSourcePackageSet, &MXF_ITEM_K(GenericPackage, Name), &info->physicalPackageName, printDebugError));
                }
                
                break;
            }
        }
    }
    mxf_free_list(&list);


    /* get the start timecode */
    
    /* the source timecode is calculated using the SourceClip::StartPosition in the file source package
    in conjunction with the TimecodeComponent in the referenced physical source package */
       
    haveStartTimecode = 0;
    DCHECK(mxf_uu_get_package_tracks(fileSourcePackageSet, &arrayIter));
    while (!haveStartTimecode &&
        mxf_uu_next_track(headerMetadata, &arrayIter, &trackSet))
    {
        /* skip tracks that are not picture or sound */
        DCHECK(mxf_uu_get_track_datadef(trackSet, &dataDef));
        
        /* some Avid files have a weak reference to a DataDefinition instead of a UL */ 
        if (!mxf_is_picture(&dataDef) && !mxf_is_sound(&dataDef) && !mxf_is_timecode(&dataDef))
        {
            if (!mxf_avid_get_data_def(headerMetadata, (mxfUUID*)&dataDef, &dataDef))
            {
                continue;
            }
        }
        
        
        if (!mxf_is_picture(&dataDef) && !mxf_is_sound(&dataDef))
        {
            continue;
        }
        
        
        /* get the source clip */
        if (!get_single_track_component(trackSet, &MXF_SET_K(SourceClip), &sourceClipSet, printDebugError))
        {
            continue;
        }
        
        /* get the start position and edit rate for the file source package source clip */
        DCHECK(mxf_get_rational_item(trackSet, &MXF_ITEM_K(Track, EditRate), &filePackageEditRate));
        DCHECK(mxf_get_position_item(sourceClipSet, &MXF_ITEM_K(SourceClip, StartPosition), &filePackageStartPosition));
        
        
        /* get the package referenced by the source clip */
        DCHECK(mxf_get_umid_item(sourceClipSet, &MXF_ITEM_K(SourceClip, SourcePackageID), &sourcePackageID));
        if (mxf_equals_umid(&g_Null_UMID, &sourcePackageID) ||
            !mxf_uu_get_referenced_package(sourceClipSet->headerMetadata, &sourcePackageID, &refSourcePackageSet))
        {
            /* either at the end of chain or don't have the referenced package */
            continue;
        }
        
        /* find the timecode component in the physical source package and calculate the start timecode */
        DCHECK(mxf_uu_get_package_tracks(refSourcePackageSet, &iter3));
        while (mxf_uu_next_track(headerMetadata, &iter3, &trackSet))
        {
            /* skip non-timecode tracks */
            DCHECK(mxf_uu_get_track_datadef(trackSet, &dataDef));
            
            /* some Avid files have a weak reference to a DataDefinition instead of a UL */ 
            if (!mxf_is_picture(&dataDef) && !mxf_is_sound(&dataDef) && !mxf_is_timecode(&dataDef))
            {
                if (!mxf_avid_get_data_def(headerMetadata, (mxfUUID*)&dataDef, &dataDef))
                {
                    continue;
                }
            }
            
            if (!mxf_is_timecode(&dataDef))
            {
                continue;
            }
            
            /* get the timecode component */
            if (!get_single_track_component(trackSet, &MXF_SET_K(TimecodeComponent), &timecodeComponentSet, printDebugError))
            {
                continue;
            }
            
            /* get the start timecode and rounded timecode base for the timecode component */
            DCHECK(mxf_get_position_item(timecodeComponentSet, &MXF_ITEM_K(TimecodeComponent, StartTimecode), &startTimecode));
            DCHECK(mxf_get_uint16_item(timecodeComponentSet, &MXF_ITEM_K(TimecodeComponent, RoundedTimecodeBase), &roundedTimecodeBase));
            
            /* convert the physical package start timecode to a start position in the file source package */
            startPosition = filePackageStartPosition + 
                (int64_t)(startTimecode * 
                    filePackageEditRate.numerator / (double)(filePackageEditRate.denominator * roundedTimecodeBase) + 0.5);
            
            /* convert the start position to material package track edit rate units */
            info->startTimecode = (int64_t)(startPosition * 
                info->editRate.numerator * filePackageEditRate.denominator /
                    (double)(info->editRate.denominator * filePackageEditRate.numerator) + 0.5);

            haveStartTimecode = 1;
            break;
        }
    }
    
    
    /* get the essence type */
    
    /* using the header partition's essence container label because the label in the FileDescriptor
     is sometimes a weak reference to a ContainerDefinition in Avid files and the ContainerDefinition 
     is not of much use */
    info->essenceContainerLabel = *(mxfUL*)mxf_get_list_element(&headerPartition->essenceContainers, 0);

    /* Note: using mxf_equals_ul_mod_regver function below because the Avid labels use a different registry version byte */
    if (mxf_equals_ul_mod_regver(&info->essenceContainerLabel, &MXF_EC_L(D10_50_625_50_defined_template)) ||
        mxf_equals_ul_mod_regver(&info->essenceContainerLabel, &MXF_EC_L(D10_50_625_50_extended_template)) ||
        mxf_equals_ul_mod_regver(&info->essenceContainerLabel, &MXF_EC_L(D10_50_625_50_picture_only)) ||
        mxf_equals_ul_mod_regver(&info->essenceContainerLabel, &MXF_EC_L(D10_50_525_60_defined_template)) ||
        mxf_equals_ul_mod_regver(&info->essenceContainerLabel, &MXF_EC_L(D10_50_525_60_extended_template)) ||
        mxf_equals_ul_mod_regver(&info->essenceContainerLabel, &MXF_EC_L(D10_50_525_60_picture_only)) ||
        mxf_equals_ul_mod_regver(&info->essenceContainerLabel, &MXF_EC_L(D10_40_625_50_defined_template)))
    {
        info->essenceType = MPEG_50_ESSENCE_TYPE;
    }
    else if (mxf_equals_ul_mod_regver(&info->essenceContainerLabel, &MXF_EC_L(D10_40_625_50_extended_template)) ||
        mxf_equals_ul_mod_regver(&info->essenceContainerLabel, &MXF_EC_L(D10_40_625_50_picture_only)) ||
        mxf_equals_ul_mod_regver(&info->essenceContainerLabel, &MXF_EC_L(D10_40_525_60_defined_template)) ||
        mxf_equals_ul_mod_regver(&info->essenceContainerLabel, &MXF_EC_L(D10_40_525_60_extended_template)) ||
        mxf_equals_ul_mod_regver(&info->essenceContainerLabel, &MXF_EC_L(D10_40_525_60_picture_only)))
    {
        info->essenceType = MPEG_40_ESSENCE_TYPE;
    }
    else if (mxf_equals_ul_mod_regver(&info->essenceContainerLabel, &MXF_EC_L(D10_30_625_50_defined_template)) ||
        mxf_equals_ul_mod_regver(&info->essenceContainerLabel, &MXF_EC_L(D10_30_625_50_extended_template)) ||
        mxf_equals_ul_mod_regver(&info->essenceContainerLabel, &MXF_EC_L(D10_30_625_50_picture_only)) ||
        mxf_equals_ul_mod_regver(&info->essenceContainerLabel, &MXF_EC_L(D10_30_525_60_defined_template)) ||
        mxf_equals_ul_mod_regver(&info->essenceContainerLabel, &MXF_EC_L(D10_30_525_60_extended_template)) ||
        mxf_equals_ul_mod_regver(&info->essenceContainerLabel, &MXF_EC_L(D10_30_525_60_picture_only)))
    {
        info->essenceType = MPEG_30_ESSENCE_TYPE;
    }
    else if (mxf_equals_ul(&info->essenceContainerLabel, &MXF_EC_L(IECDV_25_525_60_ClipWrapped)) ||
        mxf_equals_ul(&info->essenceContainerLabel, &MXF_EC_L(IECDV_25_525_60_FrameWrapped)))
    {
        info->essenceType = DV_25_411_ESSENCE_TYPE;
    }
    else if (mxf_equals_ul(&info->essenceContainerLabel, &MXF_EC_L(IECDV_25_625_50_ClipWrapped)) ||
        mxf_equals_ul(&info->essenceContainerLabel, &MXF_EC_L(IECDV_25_625_50_FrameWrapped)))
    {
        info->essenceType = DV_25_420_ESSENCE_TYPE;
    }
    else if (mxf_equals_ul(&info->essenceContainerLabel, &MXF_EC_L(DVBased_25_525_60_ClipWrapped)) ||
        mxf_equals_ul(&info->essenceContainerLabel, &MXF_EC_L(DVBased_25_525_60_FrameWrapped)) ||
        mxf_equals_ul(&info->essenceContainerLabel, &MXF_EC_L(DVBased_25_625_50_ClipWrapped)) ||
        mxf_equals_ul(&info->essenceContainerLabel, &MXF_EC_L(DVBased_25_625_50_FrameWrapped)))
    {
        info->essenceType = DV_25_411_ESSENCE_TYPE;
    }
    else if (mxf_equals_ul(&info->essenceContainerLabel, &MXF_EC_L(DVBased_50_525_60_ClipWrapped)) ||
        mxf_equals_ul(&info->essenceContainerLabel, &MXF_EC_L(DVBased_50_525_60_FrameWrapped)) ||
        mxf_equals_ul(&info->essenceContainerLabel, &MXF_EC_L(DVBased_50_625_50_ClipWrapped)) || 
        mxf_equals_ul(&info->essenceContainerLabel, &MXF_EC_L(DVBased_50_625_50_FrameWrapped)))
    {
        info->essenceType = DV_50_ESSENCE_TYPE;
    }
    else if (mxf_equals_ul(&info->essenceContainerLabel, &MXF_EC_L(DV720p50ClipWrapped)) ||
        mxf_equals_ul(&info->essenceContainerLabel, &MXF_EC_L(DV1080i50ClipWrapped)))
    {
        info->essenceType = DV_100_ESSENCE_TYPE;
    }
    else if (mxf_equals_ul(&info->essenceContainerLabel, &MXF_EC_L(AvidMJPEGClipWrapped)))
    {
        /* use the Avid resolution id if present, else use the picture essence coding label */ 
        if (avidResolutionID != 0x00)
        {
            switch (avidResolutionID)
            {
                case 0x4c:
                    info->essenceType = MJPEG_2_1_ESSENCE_TYPE;
                    break;
                case 0x4d:
                    info->essenceType = MJPEG_3_1_ESSENCE_TYPE;
                    break;
                case 0x6f:
                    info->essenceType = MJPEG_4_1_M_ESSENCE_TYPE;
                    break;
                case 0x4b:
                    info->essenceType = MJPEG_10_1_ESSENCE_TYPE;
                    break;
                case 0x6e:
                    info->essenceType = MJPEG_10_1_M_ESSENCE_TYPE;
                    break;
                case 0x4e:
                    info->essenceType = MJPEG_15_1_S_ESSENCE_TYPE;
                    break;
                case 0x52:
                    info->essenceType = MJPEG_20_1_ESSENCE_TYPE;
                    break;
                default:
                    info->essenceType = UNKNOWN_ESSENCE_TYPE;
                    break;
            }
        }
        else
        {
            if (mxf_equals_ul(&pictureEssenceCoding, &MXF_CMDEF_L(AvidMJPEG21_PAL)) ||
                mxf_equals_ul(&pictureEssenceCoding, &MXF_CMDEF_L(AvidMJPEG21_NTSC)))
            {
                info->essenceType = MJPEG_2_1_ESSENCE_TYPE;
            }
            if (mxf_equals_ul(&pictureEssenceCoding, &MXF_CMDEF_L(AvidMJPEG31_PAL)) ||
                mxf_equals_ul(&pictureEssenceCoding, &MXF_CMDEF_L(AvidMJPEG31_NTSC)))
            {
                info->essenceType = MJPEG_3_1_ESSENCE_TYPE;
            }
            if (mxf_equals_ul(&pictureEssenceCoding, &MXF_CMDEF_L(AvidMJPEG101_PAL)) ||
                mxf_equals_ul(&pictureEssenceCoding, &MXF_CMDEF_L(AvidMJPEG101_NTSC)))
            {
                info->essenceType = MJPEG_10_1_ESSENCE_TYPE;
            }
            if (mxf_equals_ul(&pictureEssenceCoding, &MXF_CMDEF_L(AvidMJPEG101m_PAL)) ||
                mxf_equals_ul(&pictureEssenceCoding, &MXF_CMDEF_L(AvidMJPEG101m_NTSC)))
            {
                info->essenceType = MJPEG_10_1_M_ESSENCE_TYPE;
            }
            if (mxf_equals_ul(&pictureEssenceCoding, &MXF_CMDEF_L(AvidMJPEG151s_PAL)) ||
                mxf_equals_ul(&pictureEssenceCoding, &MXF_CMDEF_L(AvidMJPEG151s_NTSC)))
            {
                info->essenceType = MJPEG_15_1_S_ESSENCE_TYPE;
            }
            if (mxf_equals_ul(&pictureEssenceCoding, &MXF_CMDEF_L(AvidMJPEG201_PAL)) ||
                mxf_equals_ul(&pictureEssenceCoding, &MXF_CMDEF_L(AvidMJPEG201_NTSC)))
            {
                info->essenceType = MJPEG_20_1_ESSENCE_TYPE;
            }
            else
            {
                info->essenceType = UNKNOWN_ESSENCE_TYPE;
            }
        }
    }
    else if (mxf_equals_ul(&info->essenceContainerLabel, &MXF_EC_L(HD_Unc_1080_50i_422_ClipWrapped)) ||
        mxf_equals_ul(&info->essenceContainerLabel, &MXF_EC_L(SD_Unc_625_50i_422_135_ClipWrapped)))
    {
        info->essenceType = UNC_ESSENCE_TYPE;
    }
    else if (mxf_equals_ul(&info->essenceContainerLabel, &MXF_EC_L(DNxHD1080i185ClipWrapped)) ||
        mxf_equals_ul(&info->essenceContainerLabel, &MXF_EC_L(DNxHD1080p185ClipWrapped)) ||
        mxf_equals_ul(&info->essenceContainerLabel, &MXF_EC_L(DNxHD720p185ClipWrapped)))
    {
        info->essenceType = DNXHD_185_ESSENCE_TYPE;
    }
    else if (mxf_equals_ul(&info->essenceContainerLabel, &MXF_EC_L(DNxHD1080i120ClipWrapped)) ||
        mxf_equals_ul(&info->essenceContainerLabel, &MXF_EC_L(DNxHD1080p120ClipWrapped)) ||
        mxf_equals_ul(&info->essenceContainerLabel, &MXF_EC_L(DNxHD720p120ClipWrapped)))
    {
        info->essenceType = DNXHD_120_ESSENCE_TYPE;
    }
    else if (mxf_equals_ul(&info->essenceContainerLabel, &MXF_EC_L(DNxHD1080p36ClipWrapped)))
    {
        info->essenceType = DNXHD_36_ESSENCE_TYPE;
    }
    else if (mxf_equals_ul(&info->essenceContainerLabel, &MXF_EC_L(BWFClipWrapped)))
    {
        info->essenceType = PCM_ESSENCE_TYPE;
    }
    else
    {
        info->essenceType = UNKNOWN_ESSENCE_TYPE;
    }

    
    
    
    /* clean up */
    
    mxf_file_close(&mxfFile);
    mxf_free_header_metadata(&headerMetadata);
    mxf_free_data_model(&dataModel);
    mxf_free_partition(&headerPartition);
    mxf_free_list(&list);
    mxf_free_list(&taggedValueNames);
    mxf_free_list(&taggedValueValues);
    
    return 0;
    
    
fail:
    
    mxf_file_close(&mxfFile);
    mxf_free_header_metadata(&headerMetadata);
    mxf_free_data_model(&dataModel);
    mxf_free_partition(&headerPartition);
    mxf_free_list(&list);
    mxf_free_list(&taggedValueNames);
    mxf_free_list(&taggedValueValues);

    ami_free_info(info);
    
    return errorCode;
}

void ami_free_info(AvidMXFInfo* info)
{
    int i;
    
    SAFE_FREE(&info->clipName);
    SAFE_FREE(&info->projectName);
    SAFE_FREE(&info->physicalPackageName);
    SAFE_FREE(&info->tracksString);
    
    if (info->userComments != NULL)
    {
        for (i = 0; i < info->numUserComments; i++)
        {
            SAFE_FREE(&info->userComments[i].name);
            SAFE_FREE(&info->userComments[i].value);
        }
        SAFE_FREE(&info->userComments);
    }

    if (info->materialPackageAttributes != NULL)
    {
        for (i = 0; i < info->numMaterialPackageAttributes; i++)
        {
            SAFE_FREE(&info->materialPackageAttributes[i].name);
            SAFE_FREE(&info->materialPackageAttributes[i].value);
        }
        SAFE_FREE(&info->materialPackageAttributes);
    }
}

void ami_print_info(AvidMXFInfo* info)
{
    int i;
    
    printf("Project name = %s\n", (info->projectName == NULL) ? "": info->projectName);
    printf("Project edit rate = %d/%d\n", info->projectEditRate.numerator, info->projectEditRate.denominator);
    printf("Clip name = %s\n", (info->clipName == NULL) ? "": info->clipName);
    printf("Clip created = ");
    print_timestamp(&info->clipCreated);
    printf("\n");
    printf("Clip edit rate = %d/%d\n", info->projectEditRate.numerator, info->projectEditRate.denominator);
    printf("Clip duration = %"PFi64" samples (", info->clipDuration);
    print_timecode(info->clipDuration, &info->projectEditRate);
    printf(")\n");
    printf("Clip video tracks = %d\n", info->numVideoTracks);
    printf("Clip audio tracks = %d\n", info->numAudioTracks);
    printf("Clip track string = %s\n", (info->tracksString == NULL) ? "" : info->tracksString);
    printf("%s essence\n", info->isVideo ? "Video": "Audio");
    printf("Essence type = %s\n", essence_type_string(info->essenceType));
    printf("Essence label = ");
    print_label(&info->essenceContainerLabel);
    printf("\n");
    printf("Track number = %d\n", info->trackNumber);
    printf("Edit rate = %d/%d\n", info->editRate.numerator, info->editRate.denominator);
    printf("Track duration = %"PFi64" samples (", info->trackDuration);
    print_timecode(convert_length(&info->projectEditRate, &info->editRate, info->trackDuration), &info->projectEditRate);
    printf(")\n");
    printf("Track segment duration = %"PFi64" samples (", info->segmentDuration);
    print_timecode(convert_length(&info->projectEditRate, &info->editRate, info->segmentDuration), &info->projectEditRate);
    printf(")\n");
    printf("Track segment offset = %"PFi64" samples (", info->segmentOffset);
    print_timecode(convert_length(&info->projectEditRate, &info->editRate, info->segmentOffset), &info->projectEditRate);
    printf(")\n");
    printf("Start timecode = %"PFi64" samples (", info->startTimecode);
    print_timecode(convert_length(&info->projectEditRate, &info->editRate, info->startTimecode), &info->projectEditRate);
    printf(")\n");
    if (info->isVideo)
    {
        printf("Image aspect ratio = %d/%d\n", info->aspectRatio.numerator, info->aspectRatio.denominator);
        printf("Stored WxH = %dx%d (%s)\n", info->storedWidth, info->storedHeight, frame_layout_string(info->frameLayout));
        printf("Display WxH = %dx%d (%s)\n", info->storedWidth, info->storedHeight, frame_layout_string(info->frameLayout));
    }
    else
    {
        printf("Audio sampling rate = %d/%d\n", info->audioSamplingRate.numerator, info->audioSamplingRate.denominator);
        printf("Channel count = %d\n", info->channelCount);
        printf("Quantization bits = %d\n", info->quantizationBits);
    }
    if (info->userComments != NULL)
    {
        printf("User comments:\n");
        for (i = 0; i < info->numUserComments; i++)
        {
            printf("  %s = %s\n", info->userComments[i].name, info->userComments[i].value);
        }
    }
    if (info->materialPackageAttributes != NULL)
    {
        printf("Material package attributes:\n");
        for (i = 0; i < info->numMaterialPackageAttributes; i++)
        {
            printf("  %s = %s\n", info->materialPackageAttributes[i].name, info->materialPackageAttributes[i].value);
        }
    }
    printf("Material package UID = ");
    print_umid(&info->materialPackageUID);
    printf("\n");
    printf("File package UID     = ");
    print_umid(&info->fileSourcePackageUID);
    printf("\n");
    printf("Physical package UID = ");
    print_umid(&info->physicalSourcePackageUID);
    printf("\n");
    printf("Physical package type = ");
    switch (info->physicalPackageType)
    {
        case TAPE_PHYS_TYPE:
            printf("Tape");
            break;
        case IMPORT_PHYS_TYPE:
            printf("Import");
            break;
        case RECORDING_PHYS_TYPE:
            printf("Recording");
            break;
        case UNKNOWN_PHYS_TYPE:
        default:
            break;
    }
    printf("\n");
    printf("Physical package name = %s\n", (info->physicalPackageName == NULL) ? "": info->physicalPackageName);
}



