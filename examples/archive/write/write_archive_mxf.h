/*
 * $Id: write_archive_mxf.h,v 1.1 2007/09/11 13:24:47 stuart_hc Exp $
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



typedef struct _ArchiveMXFWriter ArchiveMXFWriter;


/* create a new D3 MXF file and prepare for writing the essence */
int prepare_archive_mxf_file(const char* filename, int numAudioTracks, int64_t startPosition, int beStrict, ArchiveMXFWriter** output);

    
/* write the essence, in order, starting with the timecode, followed by video and then 0 or more audio */
int write_timecode(ArchiveMXFWriter* output, ArchiveTimecode vitc, ArchiveTimecode ltc);     
int write_video_frame(ArchiveMXFWriter* output, uint8_t* data, uint32_t size);     
int write_audio_frame(ArchiveMXFWriter* output, uint8_t* data, uint32_t size);     

/* close and delete the file and free output */
int abort_archive_mxf_file(ArchiveMXFWriter** output);

/* write the header metadata, do misc. fixups, close the file and free output */
int complete_archive_mxf_file(ArchiveMXFWriter** output, const char* d3InfaxDataString,
    const PSEFailure* pseFailures, long numPSEFailures,
    const VTRError* vtrErrors, long numVTRErrors);


/* update the file source package in the header metadata with the infax data */
int update_archive_mxf_file(const char* filePath, const char* newFilename, const char* ltoInfaxDataString, int beStrict);


#ifdef __cplusplus
}
#endif


#endif

