/*
 * $Id: avid_mxf_info.h,v 1.4 2009/10/21 10:10:29 philipn Exp $
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
 
#ifndef __AVID_MXF_INFO_H__
#define __AVID_MXF_INFO_H__


#ifdef __cplusplus
extern "C" 
{
#endif


#include <mxf/mxf_types.h>


/* Note: keep essence_type_string() in sync when changes are made here */
typedef enum
{
    UNKNOWN_ESSENCE_TYPE = 0,
    MPEG_30_ESSENCE_TYPE,
    MPEG_40_ESSENCE_TYPE,
    MPEG_50_ESSENCE_TYPE,
    DV_25_411_ESSENCE_TYPE,
    DV_25_420_ESSENCE_TYPE,
    DV_50_ESSENCE_TYPE,
    DV_100_ESSENCE_TYPE,
    MJPEG_20_1_ESSENCE_TYPE,
    MJPEG_2_1_S_ESSENCE_TYPE,
    MJPEG_4_1_S_ESSENCE_TYPE,
    MJPEG_15_1_S_ESSENCE_TYPE,
    MJPEG_10_1_ESSENCE_TYPE,
    MJPEG_10_1_M_ESSENCE_TYPE,
    MJPEG_4_1_M_ESSENCE_TYPE,
    MJPEG_3_1_ESSENCE_TYPE,
    MJPEG_2_1_ESSENCE_TYPE,
    UNC_ESSENCE_TYPE,
    MJPEG_1_1_8b_ESSENCE_TYPE,
    MJPEG_1_1_10b_ESSENCE_TYPE,
    MJPEG_35_1_P_ESSENCE_TYPE,
    MJPEG_28_1_P_ESSENCE_TYPE,
    MJPEG_14_1_P_ESSENCE_TYPE,
    MJPEG_3_1_P_ESSENCE_TYPE,
    MJPEG_2_1_P_ESSENCE_TYPE,
    MJPEG_3_1_M_ESSENCE_TYPE,
    MJPEG_8_1_M_ESSENCE_TYPE,
    DNXHD_185_ESSENCE_TYPE,
    DNXHD_120_ESSENCE_TYPE,
    DNXHD_36_ESSENCE_TYPE,
    PCM_ESSENCE_TYPE
} AvidEssenceType;

typedef enum
{
    UNKNOWN_PHYS_TYPE = 0,
    TAPE_PHYS_TYPE,
    IMPORT_PHYS_TYPE,
    RECORDING_PHYS_TYPE
} AvidPhysicalPackageType;

typedef enum
{
    FULL_FRAME_FRAME_LAYOUT = 0,
    SEPARATE_FIELDS_FRAME_LAYOUT,
    SINGLE_FIELD_FRAME_LAYOUT,
    MIXED_FIELD_FRAME_LAYOUT,
    SEGMENTED_FRAME_FRAME_LAYOUT,
} AvidFrameLayout;

typedef struct
{
    char* name;
    char* value;
} AvidNameValuePair;

typedef struct
{
    /* clip info */
    char* clipName;
    char* projectName;
    mxfTimestamp clipCreated;
    mxfRational projectEditRate;
    int64_t clipDuration;
    mxfUMID materialPackageUID;
    AvidNameValuePair* userComments;
    int numUserComments;
    AvidNameValuePair* materialPackageAttributes;
    int numMaterialPackageAttributes;
    /* TODO: handle complexity of __AttributeList in attributes */
    int numVideoTracks;
    int numAudioTracks;
    char* tracksString;
    
    /* track info */
    uint32_t trackNumber;
    int isVideo;
    mxfRational editRate;
    int64_t trackDuration;
    int64_t segmentDuration;
    int64_t segmentOffset;
    int64_t startTimecode;

    /* file essence info */
    AvidEssenceType essenceType;
    mxfUMID fileSourcePackageUID;
    mxfUL essenceContainerLabel;
    
    /* picture info */
    uint8_t frameLayout;
    mxfRational aspectRatio;
    uint32_t storedWidth;
    uint32_t storedHeight;
    uint32_t displayWidth;
    uint32_t displayHeight;
    
    /* sound info */
    mxfRational audioSamplingRate;
    uint32_t channelCount;
    uint32_t quantizationBits;
    
    /* physical source info */
    char* physicalPackageName;
    mxfUMID physicalSourcePackageUID;
    AvidPhysicalPackageType physicalPackageType;

} AvidMXFInfo;



int ami_read_info(const char* filename, AvidMXFInfo* info, int printDebugError);

void ami_free_info(AvidMXFInfo* info);

void ami_print_info(AvidMXFInfo* info);
/* TODO: add function to print machine readable info e.g. newline delimited fields */



#ifdef __cplusplus
}
#endif


#endif

