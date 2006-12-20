/*
 * $Id: write_avid_mxf.h,v 1.1 2006/12/20 15:45:52 john_f Exp $
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
 
#ifndef __WRITE_AVID_MXF_H__
#define __WRITE_AVID_MXF_H__


#ifdef __cplusplus
extern "C" 
{
#endif


#include <package_definitions.h>


typedef struct _AvidClipWriter AvidClipWriter;

typedef enum
{
    PAL_25i,
    NTSC_30i
} ProjectFormat;


/* create the writer */
int create_clip_writer(const char* projectName, ProjectFormat projectFormat,
    mxfRational imageAspectRatio, int dropFrameFlag, int useLegacy, 
    PackageDefinitions* packageDefinitions, AvidClipWriter** clipWriter);
    

/* write essence samples
    the number of samples is a multiple of the file package track edit rate
    eg. if the edit rate is 48000/1 then the number of sample in a PAL video frame
        is 1920. If the edit rate is 25/1 in the audio track then the number of 
        samples is 1
    
    Note: numSamples must equal 1 for variable size samples (eg. MJPEG) to allow indexing to work
    Note: numSamples must equal 1 for uncompressed video
*/
int write_samples(AvidClipWriter* clipWriter, uint32_t materialTrackID, uint32_t numSamples,
    uint8_t* data, uint32_t size);     

/* same as write_samples, but data is written in multiple calls */
int start_write_samples(AvidClipWriter* clipWriter, uint32_t materialTrackID);     
int write_sample_data(AvidClipWriter* clipWriter, uint32_t materialTrackID, uint8_t* data, uint32_t size);     
int end_write_samples(AvidClipWriter* clipWriter, uint32_t materialTrackID, uint32_t numSamples);     


/* delete the output files and free the writer */
void abort_writing(AvidClipWriter** clipWriter, int deleteFile);

/* complete and save the output files and free the writer */
int complete_writing(AvidClipWriter** clipWriter);



#ifdef __cplusplus
}
#endif


#endif

