/*
 * $Id: package_definitions.c,v 1.1 2006/12/20 15:45:52 john_f Exp $
 *
 * Functions to create package definitions
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

#include <package_definitions.h>
#include <mxf/mxf.h>
#include <mxf/mxf_avid.h>



static void free_user_comment(UserComment** userComment)
{
    if ((*userComment) == NULL)
    {
        return;
    }
    
    SAFE_FREE(&(*userComment)->name);
    SAFE_FREE(&(*userComment)->value);
    
    SAFE_FREE(userComment);
}

static void free_tagged_value_in_list(void* data)
{
    UserComment* userComment;
    
    if (data == NULL)
    {
        return;
    }
    
    userComment = (UserComment*)data;
    free_user_comment(&userComment);
}

static int create_user_comment(const char* name, const char* value, UserComment** userComment)
{
    UserComment* newUserComment = NULL;

    CHK_ORET(name != NULL);    
    CHK_ORET(value != NULL);
    
    CHK_MALLOC_ORET(newUserComment, UserComment);
    memset(newUserComment, 0, sizeof(UserComment));
    
    CHK_MALLOC_ARRAY_OFAIL(newUserComment->name, char, strlen(name) + 1);
    strcpy(newUserComment->name, name);
    CHK_MALLOC_ARRAY_OFAIL(newUserComment->value, char, strlen(value) + 1);
    strcpy(newUserComment->value, value);
    
    *userComment = newUserComment;
    return 1;
    
fail:
    free_user_comment(&newUserComment);
    return 0;
}

static void free_track(Track** track)
{
    if ((*track) == NULL)
    {
        return;
    }
    
    SAFE_FREE(&(*track)->name);
    
    SAFE_FREE(track);
}

static void free_track_in_list(void* data)
{
    Track* track;
    
    if (data == NULL)
    {
        return;
    }
    
    track = (Track*)data;
    free_track(&track);
}

static void free_package(Package** package)
{
    if ((*package) == NULL)
    {
        return;
    }

    SAFE_FREE(&(*package)->name);
    mxf_clear_list(&(*package)->tracks);
    SAFE_FREE(&(*package)->filename);
    mxf_clear_list(&(*package)->userComments);

    SAFE_FREE(package);    
}

static void free_package_in_list(void* data)
{
    Package* package;
    
    if (data == NULL)
    {
        return;
    }
    
    package = (Package*)data;
    free_package(&package);
}

static int create_package(const mxfUMID* uid, const char* name, const mxfTimestamp* creationDate, Package** package)
{
    Package* newPackage = NULL;
    
    CHK_MALLOC_ORET(newPackage, Package);
    memset(newPackage, 0, sizeof(Package));
    mxf_initialise_list(&newPackage->tracks, free_track_in_list);
    mxf_initialise_list(&newPackage->userComments, free_tagged_value_in_list);
    
    newPackage->uid = *uid;
    if (name != NULL)
    {
        CHK_MALLOC_ARRAY_OFAIL(newPackage->name, char, strlen(name) + 1);
        strcpy(newPackage->name, name);
    }
    else
    {
        newPackage->name = NULL;
    }
    newPackage->creationDate = *creationDate;
    
    *package = newPackage;
    return 1;
    
fail:
    free_package(&newPackage);
    return 0;
}


int create_package_definitions(PackageDefinitions** definitions)
{
    PackageDefinitions* newDefinitions;
    
    CHK_MALLOC_ORET(newDefinitions, PackageDefinitions);
    memset(newDefinitions, 0, sizeof(PackageDefinitions));
    mxf_initialise_list(&newDefinitions->fileSourcePackages, free_package_in_list);
    
    *definitions = newDefinitions;
    return 1;
}

void free_package_definitions(PackageDefinitions** definitions)
{
    if (*definitions == NULL)
    {
        return;
    }
    
    free_package(&(*definitions)->materialPackage);
    mxf_clear_list(&(*definitions)->fileSourcePackages);
    free_package(&(*definitions)->tapeSourcePackage);
    
    SAFE_FREE(definitions);
}

int create_material_package(PackageDefinitions* definitions, const mxfUMID* uid, 
    const char* name, const mxfTimestamp* creationDate)
{
    CHK_ORET(create_package(uid, name, creationDate, &definitions->materialPackage));
    return 1;
}

int create_file_source_package(PackageDefinitions* definitions, const mxfUMID* uid, 
    const char* name, const mxfTimestamp* creationDate, 
    const char* filename, EssenceType essenceType, const EssenceInfo* essenceInfo,
    Package** filePackage)
{
    Package* newFilePackage = NULL;
    
    if (filename == NULL)
    {
        mxf_log(MXF_ELOG, "File source package filename is null" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
        return 0;
    }
    
    CHK_ORET(create_package(uid, name, creationDate, &newFilePackage));
    CHK_ORET(mxf_append_list_element(&definitions->fileSourcePackages, newFilePackage));

    CHK_MALLOC_ARRAY_ORET(newFilePackage->filename, char, strlen(filename) + 1);
    strcpy(newFilePackage->filename, filename);
    newFilePackage->essenceType = essenceType;
    newFilePackage->essenceInfo = *essenceInfo;
    
    *filePackage = newFilePackage;
    return 1;
}

int create_tape_source_package(PackageDefinitions* definitions, const mxfUMID* uid, 
    const char* name, const mxfTimestamp* creationDate)
{
    CHK_ORET(create_package(uid, name, creationDate, &definitions->tapeSourcePackage));
    return 1;
}

int add_user_comment(Package* package, const char* name, const char* value)
{
    UserComment* newUserComment = NULL;
    
    CHK_ORET(create_user_comment(name, value, &newUserComment));
    CHK_OFAIL(mxf_append_list_element(&package->userComments, newUserComment));
    
    return 1;
fail:
    free_user_comment(&newUserComment);
    return 0;
}

int create_track(Package* package, uint32_t id, uint32_t number, const char* name, int isPicture, 
    const mxfRational* editRate, const mxfUMID* sourcePackageUID, uint32_t sourceTrackID, 
    int64_t startPosition, int64_t length, Track** track)
{
    Track* newTrack = NULL;
    
    CHK_MALLOC_ORET(newTrack, Track);
    memset(newTrack, 0, sizeof(Track));
    
    newTrack->id = id;
    if (name != NULL)
    {
        CHK_MALLOC_ARRAY_OFAIL(newTrack->name, char, strlen(name) + 1);
        strcpy(newTrack->name, name);
    }
    else
    {
        newTrack->name = NULL;
    }
    newTrack->id = id;
    newTrack->number = number;
    newTrack->isPicture = isPicture;
    newTrack->editRate = *editRate;
    newTrack->sourcePackageUID = *sourcePackageUID;
    newTrack->sourceTrackID = sourceTrackID;
    newTrack->startPosition = startPosition;
    newTrack->length = length;
    
    CHK_OFAIL(mxf_append_list_element(&package->tracks, newTrack));
    
    *track = newTrack;
    return 1;
    
fail:
    free_track(&newTrack);
    return 0;
}


