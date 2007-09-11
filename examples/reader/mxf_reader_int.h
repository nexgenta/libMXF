/*
 * $Id: mxf_reader_int.h,v 1.1 2007/09/11 13:24:47 stuart_hc Exp $
 *
 * Internal functions for reading MXF files
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
 
#ifndef __MXF_READER_INT_H__
#define __MXF_READER_INT_H__


#include <mxf_reader.h>


typedef struct _EssenceReaderData EssenceReaderData;

typedef struct _EssenceTrack
{
    struct _EssenceTrack* next;
    
    uint32_t trackNumber;
    
    int64_t frameSize;  /* -1 indicates variable frame size, 0 indicates sequence */
    uint32_t frameSizeSeq[15];
    
    mxfRational frameRate; /* required playout frame rate */
    int64_t playoutDuration;
    
    mxfRational sampleRate; /* sample rate of essence container */
    int64_t containerDuration;
    
    uint32_t imageStartOffset; /* used for Avid unc frames which are aligned to 8k boundaries */
    
    uint32_t bodySID;
    uint32_t indexSID;
} EssenceTrack;

typedef struct
{
    EssenceTrack* essenceTracks;

    void (*close) (MXFReader* reader);
    int (*position_at_frame) (MXFReader* reader, int64_t frameNumber);
    int (*skip_next_frame) (MXFReader* reader);
    int (*read_next_frame) (MXFReader* reader, MXFReaderListener* listener);
    int64_t (*get_next_frame_number) (MXFReader* reader);
    int64_t (*get_last_written_frame_number) (MXFReader* reader);
    MXFHeaderMetadata* (*get_header_metadata) (MXFReader* reader);
    int (*have_footer_metadata)(MXFReader* reader);

    EssenceReaderData* data;
} EssenceReader;

typedef struct
{
    mxfPosition startTimecode;
    mxfLength duration;
} TimecodeSegment;

typedef struct
{
    int type;
    int count;
    
    int isDropFrame;
    uint16_t roundedTimecodeBase;
    
    /* playout and source timeodes originating from the header metadata */
    MXFList segments;

    /* source timecodes originating from the system or video item in the essence container */
    mxfPosition position;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint8_t frame;
} TimecodeIndex;

struct _MXFReader
{
    MXFFile* mxfFile;
    MXFClip clip;
    
    int haveReadAFrame; /* is true if a frame has been read and therefore the number of source timecodes is up to date */
    TimecodeIndex playoutTimecodeIndex;
    MXFList sourceTimecodeIndexes;
    
    EssenceReader* essenceReader;
    
    MXFDataModel* dataModel;
    int ownDataModel;  /* the reader will free it when closed */
    
    /* buffer for internal use */
    uint8_t* buffer;
    uint32_t bufferSize;
};


int add_track(MXFReader* reader, MXFTrack** track);

int add_essence_track(EssenceReader* essenceReader, EssenceTrack** essenceTrack);
int get_num_essence_tracks(EssenceReader* essenceReader);
EssenceTrack* get_essence_track(EssenceReader* essenceReader, int trackIndex);
int get_essence_track_with_tracknumber(EssenceReader* essenceReader, uint32_t trackNumber,
    EssenceTrack**, int* trackIndex);

void clean_rate(mxfRational* rate);

int initialise_playout_timecode(MXFReader* reader, MXFMetadataSet* materialPackageSet);
int initialise_default_playout_timecode(MXFReader* reader);

int initialise_source_timecodes(MXFReader* reader, MXFMetadataSet* sourcePackageSet);
int set_essence_container_timecode(MXFReader* reader, mxfPosition position, 
    int type, int count, int isDropFrame, uint8_t hour, uint8_t min, uint8_t sec, uint8_t frame);


#endif


