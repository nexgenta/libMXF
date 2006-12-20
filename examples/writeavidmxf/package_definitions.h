/*
 * $Id: package_definitions.h,v 1.1 2006/12/20 15:45:52 john_f Exp $
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


typedef enum
{
    AvidMJPEG,
    DVBased25,
    DVBased50,
    DNxHD1080i120,
    DNxHD1080i180,
    UncUYVY,
    PCM
} EssenceType;

typedef enum
{
    Res21, 
    Res31, 
    Res101, 
    Res151, 
    Res201
} AvidMJPEGResolution;

typedef struct
{
    AvidMJPEGResolution resolution;
} AvidMJPEGInfo;

typedef struct
{
    int bitsPerSample;
} PCMInfo;

typedef union
{
    AvidMJPEGInfo avidMJPEGInfo;
    PCMInfo pcmInfo;
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
    MXFList userComments;
} Package;


typedef struct
{
    uint32_t id;
    uint32_t number;
    char* name;
    int isPicture;
    mxfRational editRate;
    mxfUMID sourcePackageUID;
    uint32_t sourceTrackID;
    int64_t startPosition;
    int64_t length;
} Track;


typedef struct
{
    Package* materialPackage;
    MXFList fileSourcePackages;
    Package* tapeSourcePackage; 
} PackageDefinitions;


int create_package_definitions(PackageDefinitions** definitions);
void free_package_definitions(PackageDefinitions** definitions);

int create_material_package(PackageDefinitions* definitions, const mxfUMID* uid, 
    const char* name, const mxfTimestamp* creationDate);
int create_file_source_package(PackageDefinitions* definitions, const mxfUMID* uid, 
    const char* name, const mxfTimestamp* creationDate, 
    const char* filename, EssenceType essenceType, const EssenceInfo* essenceInfo,
    Package** filePackage);
int create_tape_source_package(PackageDefinitions* definitions, const mxfUMID* uid, 
    const char* name, const mxfTimestamp* creationDate);

int add_user_comment(Package* package, const char* name, const char* value);

/* note: number is ignored for file source packages */
int create_track(Package* package, uint32_t id, uint32_t number, const char* name, int isPicture, 
    const mxfRational* editRate, const mxfUMID* sourcePackageUID, uint32_t sourceTrackID, 
    int64_t startPosition, int64_t length, Track** track);



#ifdef __cplusplus
}
#endif


#endif


