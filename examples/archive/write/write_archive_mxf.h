/*
 * $Id: write_archive_mxf.h,v 1.6 2010/02/12 13:46:25 philipn Exp $
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
 
#ifndef __WRITE_ARCHIVE_MXF_H__
#define __WRITE_ARCHIVE_MXF_H__


#ifdef __cplusplus
extern "C" 
{
#endif


#include <archive_types.h>
#include <mxf/mxf_file.h>


#define MAX_ARCHIVE_AUDIO_TRACKS        16


typedef struct _ArchiveMXFWriter ArchiveMXFWriter;


/* create a new Archive MXF file and prepare for writing the essence */
int prepare_archive_mxf_file(const char* filename, int componentDepth8Bit, const mxfRational* aspectRatio,
    int numAudioTracks, int includeCRC32, int64_t startPosition, int beStrict, ArchiveMXFWriter** output);

/* use the Archive MXF file (the filename is only used as metadata) and prepare for writing the essence */
/* note: if this function returns 0 then check whether *mxfFile is not NULL and needs to be closed */
int prepare_archive_mxf_file_2(MXFFile** mxfFile, const char* filename, int componentDepth8Bit,
    const mxfRational* aspectRatio, int numAudioTracks, int includeCRC32, int64_t startPosition, int beStrict,
    ArchiveMXFWriter** output);

    
/* write the essence, in order, starting with the system item, followed by video and then 0 or more audio */
int write_system_item(ArchiveMXFWriter* output, ArchiveTimecode vitc, ArchiveTimecode ltc,
    const uint32_t* crc32, int numCRC32);
int write_video_frame(ArchiveMXFWriter* output, const uint8_t* data, uint32_t size);
int write_audio_frame(ArchiveMXFWriter* output, const uint8_t* data, uint32_t size);

/* close and delete the file and free output */
int abort_archive_mxf_file(ArchiveMXFWriter** output);

/* write the header metadata, do misc. fixups, close the file and free output */
int complete_archive_mxf_file(ArchiveMXFWriter** output, InfaxData* sourceInfaxData,
    const PSEFailure* pseFailures, long numPSEFailures,
    const VTRError* vtrErrors, long numVTRErrors,
    const DigiBetaDropout* digiBetaDropouts, long numDigiBetaDropouts);

int64_t get_archive_mxf_file_size(ArchiveMXFWriter* writer);

mxfUMID get_material_package_uid(ArchiveMXFWriter* writer);
mxfUMID get_file_package_uid(ArchiveMXFWriter* writer);
mxfUMID get_tape_package_uid(ArchiveMXFWriter* writer);


/* update the file source package in the header metadata with the infax data */
int update_archive_mxf_file(const char* filePath, const char* newFilename, InfaxData* ltoInfaxData);

/* use the Archive MXF file, update the file source package in the header metadata with the infax data */
/* note: if this function returns 0 then check whether *mxfFile is not NULL and needs to be closed */
int update_archive_mxf_file_2(MXFFile** mxfFile, const char* newFilename, InfaxData* ltoInfaxData);


/* returns the content package (system, video + x audio elements) size */
int64_t get_archive_mxf_content_package_size(int componentDepth8Bit, int numAudioTracks, int includeCRC32);


int parse_infax_data(const char* infaxDataString, InfaxData* infaxData, int beStrict);

uint32_t calc_crc32(const uint8_t* data, uint32_t size);


#ifdef __cplusplus
}
#endif


#endif

