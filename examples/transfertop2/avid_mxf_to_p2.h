/*
 * $Id: avid_mxf_to_p2.h,v 1.1 2007/02/01 10:31:43 philipn Exp $
 *
 * Transfers Avid MXF files to P2
 *
 * Copyright (C) 2006  Philip de Nier <philipn@users.sourceforge.net>
 * Copyright (C) 2006  Stuart Cunningham <stuart_hc@users.sourceforge.net>
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
 
#ifndef __AVID_MXF_TO_P2_H__
#define __AVID_MXF_TO_P2_H__

#ifdef __cplusplus
extern "C" 
{
#endif


#include <mxf/mxf.h>
#include <mxf/mxf_avid.h>
#include <mxf/mxf_p2.h>


typedef void (*progress_callback) (float percentCompleted);
    
typedef int (*insert_timecode_callback) (uint8_t* frame, uint32_t frameSize, 
    mxfPosition timecode, int drop);


typedef struct 
{
    char* filename;
    
    MXFFile* mxfFile;
    
    MXFPartition* headerPartition;    
    MXFDataModel* dataModel;
    MXFHeaderMetadata* headerMetadata;
    MXFEssenceElement* essenceElement;
    MXFList* aList;

    /* these are references and should not be free'd */
    MXFMetadataSet* prefaceSet;
    MXFMetadataSet* contentStorageSet;
    MXFMetadataSet* materialPackageSet;
    MXFMetadataSet* sourcePackageSet;
    MXFMetadataSet* sourcePackageTrackSet;
    MXFMetadataSet* materialPackageTrackSet;
    MXFMetadataSet* sequenceSet;
    MXFMetadataSet* sourceClipSet;
    MXFMetadataSet* essContainerDataSet;
    MXFMetadataSet* descriptorSet;
    MXFMetadataSet* cdciDescriptorSet;
    MXFMetadataSet* bwfDescriptorSet;
    MXFMetadataSet* videoMaterialPackageTrackSet;
    MXFMetadataSet* videoSequenceSet;
} AvidMXFFile;


typedef struct 
{
    char* filename;
    
    MXFFile* mxfFile;

    MXFDataModel* dataModel;
    MXFFilePartitions* partitions;
    MXFHeaderMetadata* headerMetadata;
    MXFEssenceElement* essenceElement;
    MXFIndexTableSegment* indexSegment;

    /* these are references and should not be free'd */
    MXFMetadataSet* prefaceSet;
    MXFMetadataSet* contentStorageSet;
    MXFMetadataSet* identSet;
    MXFMetadataSet* materialPackageSet;
    MXFMetadataSet* sourcePackageSet;
    MXFMetadataSet* sourcePackageTrackSet;
    MXFMetadataSet* materialPackageTrackSet;
    MXFMetadataSet* sequenceSet;
    MXFMetadataSet* sourceClipSet;
    MXFMetadataSet* essContainerDataSet;
    MXFMetadataSet* cdciDescriptorSet;
    MXFMetadataSet* aes3DescriptorSet;
    MXFMetadataSet* timecodeComponentSet;

    
    /* video */
    char codecString[256];
    char frameRateString[256];
    mxfRational frameRate;
    mxfRational aspectRatio;
    uint32_t verticalSubsampling;
    uint32_t horizSubsampling;
    mxfUL pictureEssenceCoding;
    uint32_t storedHeight;
    uint32_t storedWidth;
    uint32_t videoLineMap[2];
    uint32_t frameSize;

    /* audio */
    mxfRational samplingRate;
    uint32_t bitsPerSample;
    uint16_t blockAlign;
    uint32_t avgBps;

    /* audio and video */
    mxfRational editRate;
    mxfLength duration;
    uint64_t startByteOffset;
    uint64_t dataSize;
    mxfUL essenceContainerLabel;
    mxfUL dataDef;
    mxfKey essElementKey;
    int isPicture;
    mxfUMID sourcePackageUID;
    uint32_t sourceTrackNumber;    
    uint32_t sourceTrackID;    
    uint32_t materialTrackNumber;    
    uint32_t materialTrackID;
    mxfLength containerDuration;
    
    
    uint64_t essenceBytesLength;
    float percentCompletedContribution;
} P2MXFFile;


typedef struct 
{
    /* set after calling prepare_transfer() */
    
    AvidMXFFile inputs[17];
    P2MXFFile outputs[17];
    int numInputs;
    
    insert_timecode_callback insertTimecode;
    progress_callback progress;

    mxfUMID globalClipID;
    int pictureOutputIndex;
    mxfLength duration;
    mxfRational editUnit;
    mxfPosition timecodeStart;
    int dropFrameFlag;
    
    uint64_t totalStorageSizeEstimate; /* not an exact size */

    
    /* set after calling transfer_avid_mxf_to_p2() */
    
    char clipName[7];
    const char* p2path;
    mxfTimestamp now;
    
    
    /* progress % */
    float percentCompleted;
    float lastCallPercentCompleted;
    
} AvidMXFToP2Transfer;



int prepare_transfer(char* inputFilenames[17], int numInputs, 
    int64_t timecodeStart, int dropFrameFlag,
    progress_callback progress, insert_timecode_callback insertTimecode, 
    AvidMXFToP2Transfer** transfer);

int transfer_avid_mxf_to_p2(const char* p2path, AvidMXFToP2Transfer* transfer, int* isComplete);

void free_transfer(AvidMXFToP2Transfer** transfer);



#ifdef __cplusplus
}
#endif


#endif 

