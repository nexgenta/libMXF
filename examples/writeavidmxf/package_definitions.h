/*
 * $Id: package_definitions.h,v 1.12 2010/07/21 16:29:33 john_f Exp $
 *
 * Defines MXF package data structures and functions to create them
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
 
#ifndef __PACKAGE_DEFINITIONS_H__
#define __PACKAGE_DEFINITIONS_H__


#ifdef __cplusplus
extern "C" 
{
#endif



#include <mxf/mxf_types.h>
#include <mxf/mxf_list.h>
#include <mxf/mxf_utils.h>


#define MAX_LOCATORS        128


typedef enum
{
    AvidMJPEG,
    IECDV25,
    DVBased25,
    DVBased50,
    DV1080i50,
    /* DV1080i60, not yet supported */
    DV720p50,
    /* DV720p60, not yet supported */
    IMX30,
    IMX40,
    IMX50,
    DNxHD720p120,
    DNxHD720p185,
    DNxHD1080p36,
    DNxHD1080p120,     /* identical format to "DNxHD1080p115" */
    DNxHD1080p185,
    DNxHD1080p185X,    /* identical format to "DNxHD1080p175X" */
    DNxHD1080i120,
    DNxHD1080i185,
    DNxHD1080i185X,    /* identical format to "DNxHD1080i175X" */
    UncUYVY,
    Unc1080iUYVY,
    Unc720p50UYVY,
    PCM
} EssenceType;

typedef enum
{
    Res21, 
    Res31, 
    Res101, 
    Res41m, 
    Res101m, 
    Res151s, 
    Res201
} AvidMJPEGResolution;

typedef enum
{
    AVID_WHITE = 0,
    AVID_RED,
    AVID_YELLOW,
    AVID_GREEN,
    AVID_CYAN,
    AVID_BLUE,
    AVID_MAGENTA,
    AVID_BLACK
} AvidRGBColor;



typedef struct
{
    mxfRational imageAspectRatio;
    AvidMJPEGResolution mjpegResolution;
    int imxFrameSize;
    
    int pcmBitsPerSample;
    int locked; /* value != 0 and value != 1 means not set */
    int audioRefLevel; /* value < -128 or value > 127 means not set */
    int dialNorm; /* value < -128 or value > 127 means not set */
    int sequenceOffset; /* value < 0 or value >= 5 means not set */
} EssenceInfo;


typedef struct
{
    char* name;
    char* value;
} UserComment;


typedef struct
{
    mxfUMID uid;
    char* name;
    mxfTimestamp creationDate;
    MXFList tracks;
    char* filename;
    EssenceType essenceType;    
    EssenceInfo essenceInfo;
} Package;


typedef struct
{
    uint32_t id;
    uint32_t number;
    char* name;
    int isPicture;
    mxfRational editRate;
    int64_t origin;
    mxfUMID sourcePackageUID;
    uint32_t sourceTrackID;
    int64_t startPosition;
    int64_t length;
} Track;


typedef struct
{
    int64_t position;
    int64_t duration;
    char* comment;
    AvidRGBColor color;
} Locator;


typedef struct _PackageDefinitions
{
    Package* materialPackage;
    MXFList fileSourcePackages;
    Package* tapeSourcePackage; 
    /* user comments are attached to the material package */
    MXFList userComments;
    /* locators are added to an event track in the material package */
    mxfRational locatorEditRate;
    MXFList locators;
} PackageDefinitions;


int create_package_definitions(PackageDefinitions** definitions, const mxfRational* locatorEditRate);
void free_package_definitions(PackageDefinitions** definitions);

void init_essence_info(EssenceInfo* essenceInfo);

int create_material_package(PackageDefinitions* definitions, const mxfUMID* uid, 
    const char* name, const mxfTimestamp* creationDate);
int create_file_source_package(PackageDefinitions* definitions, const mxfUMID* uid, 
    const char* name, const mxfTimestamp* creationDate, 
    const char* filename, EssenceType essenceType, const EssenceInfo* essenceInfo,
    Package** filePackage);
int create_tape_source_package(PackageDefinitions* definitions, const mxfUMID* uid, 
    const char* name, const mxfTimestamp* creationDate);

int set_user_comment(PackageDefinitions* definitions, const char* name, const char* value);
void clear_user_comments(PackageDefinitions* definitions);

int add_locator(PackageDefinitions* definitions, int64_t position, const char *comment, AvidRGBColor color);
void clear_locators(PackageDefinitions* definitions);

/* note: number is ignored for file source packages */
int create_track(Package* package, uint32_t id, uint32_t number, const char* name, int isPicture, 
    const mxfRational* editRate, const mxfUMID* sourcePackageUID, uint32_t sourceTrackID, 
    int64_t startPosition, int64_t length, int64_t origin, Track** track);


void get_image_aspect_ratio(PackageDefinitions* definitions, const mxfRational* defaultImageAspectRatio,
    mxfRational* imageAspectRatio);

    
#ifdef __cplusplus
}
#endif


#endif


