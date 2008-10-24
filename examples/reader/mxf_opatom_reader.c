/*
 * $Id: mxf_opatom_reader.c,v 1.3 2008/10/24 19:14:07 john_f Exp $
 *
 * MXF OP-Atom reader
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
#include <stdarg.h>
#include <assert.h>

#include <mxf_reader_int.h>
#include <mxf/mxf_avid.h>
#include <mxf_opatom_reader.h>
#include <mxf_essence_helper.h>
#include <mxf/mxf_uu_metadata.h>



/* TODO: handle frame size sequences for audio */

struct _EssenceReaderData
{
    MXFPartition* headerPartition;
    MXFHeaderMetadata* headerMetadata;
    int haveFooterMetadata;
    
    uint64_t essenceStartPos;
    
    mxfPosition currentPosition;
    
    int64_t* avidFrameOffsets;
    int64_t numAvidFrameOffsets;
};


static int is_avid_mjpeg_essence_element(const mxfKey* key)
{
    return mxf_equals_key_prefix(key, &MXF_EE_K(AvidMJPEGClipWrapped), 13) && key->octet14 == 0x01;
}

static int is_avid_dnxhd_essence_element(const mxfKey* key)
{
    return mxf_equals_key_prefix(key, &MXF_EE_K(DNxHD), 13) && key->octet14 == 0x06;
}

static int read_avid_mjpeg_index_segment(MXFReader* reader)
{
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    uint64_t segmentLen;
    MXFIndexTableSegment* newSegment = NULL;
    mxfLocalTag localTag;
    uint16_t localLen;
    uint64_t totalLen;
    uint32_t deltaEntryArrayLen;
    uint32_t deltaEntryLen;
    int8_t posTableIndex;
    uint8_t slice;
    uint32_t elementData;    
    uint32_t indexEntryArrayLen;
    uint32_t indexEntryLen;
    uint8_t temporalOffset;
    uint8_t keyFrameOffset;
    uint8_t flags;
    uint64_t streamOffset;
    uint32_t* sliceOffset = NULL;
    mxfRational* posTable = NULL; 
    uint32_t i;
    uint32_t k;

    SAFE_FREE(&reader->essenceReader->data->avidFrameOffsets);
    reader->essenceReader->data->numAvidFrameOffsets = 0;
    
    /* search for index table key and then read */
    while (1)
    {
        CHK_OFAIL(mxf_read_next_nonfiller_kl(reader->mxfFile, &key, &llen, &len));
        if (mxf_is_index_table_segment(&key))
        {
            segmentLen = len;
            
            CHK_ORET(mxf_create_index_table_segment(&newSegment));
            
            totalLen = 0;
            while (totalLen < segmentLen)
            {
                CHK_OFAIL(mxf_read_local_tag(reader->mxfFile, &localTag));
                CHK_OFAIL(mxf_read_uint16(reader->mxfFile, &localLen));
                totalLen += mxfLocalTag_extlen + 2;
                
                switch (localTag)
                {
                    case 0x3c0a:
                        CHK_OFAIL(mxf_read_uuid(reader->mxfFile, &newSegment->instanceUID));
                        CHK_OFAIL(localLen == mxfUUID_extlen);
                        totalLen += localLen;
                        break;
                    case 0x3f0b:
                        CHK_OFAIL(mxf_read_int32(reader->mxfFile, &newSegment->indexEditRate.numerator));
                        CHK_OFAIL(mxf_read_int32(reader->mxfFile, &newSegment->indexEditRate.denominator));
                        CHK_OFAIL(localLen == 8);
                        totalLen += localLen;
                        break;
                    case 0x3f0c:
                        CHK_OFAIL(mxf_read_int64(reader->mxfFile, &newSegment->indexStartPosition));
                        CHK_OFAIL(localLen == 8);
                        totalLen += localLen;
                        break;
                    case 0x3f0d:
                        CHK_OFAIL(mxf_read_int64(reader->mxfFile, &newSegment->indexDuration));
                        CHK_OFAIL(localLen == 8);
                        totalLen += localLen;
                        break;
                    case 0x3f05:
                        CHK_OFAIL(mxf_read_uint32(reader->mxfFile, &newSegment->editUnitByteCount));
                        CHK_OFAIL(localLen == 4);
                        totalLen += localLen;
                        break;
                    case 0x3f06:
                        CHK_OFAIL(mxf_read_uint32(reader->mxfFile, &newSegment->indexSID));
                        CHK_OFAIL(localLen == 4);
                        totalLen += localLen;
                        break;
                    case 0x3f07:
                        CHK_OFAIL(mxf_read_uint32(reader->mxfFile, &newSegment->bodySID));
                        CHK_OFAIL(localLen == 4);
                        totalLen += localLen;
                        break;
                    case 0x3f08:
                        CHK_OFAIL(mxf_read_uint8(reader->mxfFile, &newSegment->sliceCount));
                        CHK_OFAIL(localLen == 1);
                        totalLen += localLen;
                        break;
                    case 0x3f0e:
                        CHK_OFAIL(mxf_read_uint8(reader->mxfFile, &newSegment->posTableCount));
                        CHK_OFAIL(localLen == 1);
                        totalLen += localLen;
                        break;
                    case 0x3f09:
                        CHK_OFAIL(mxf_read_uint32(reader->mxfFile, &deltaEntryArrayLen));
                        CHK_OFAIL(mxf_read_uint32(reader->mxfFile, &deltaEntryLen));
                        CHK_OFAIL(deltaEntryLen == 6);
                        CHK_OFAIL(localLen == 8 + deltaEntryArrayLen * 6);
                        for (; deltaEntryArrayLen > 0; deltaEntryArrayLen--)
                        {
                            CHK_OFAIL(mxf_read_int8(reader->mxfFile, &posTableIndex));
                            CHK_OFAIL(mxf_read_uint8(reader->mxfFile, &slice));
                            CHK_OFAIL(mxf_read_uint32(reader->mxfFile, &elementData));
                            CHK_OFAIL(mxf_add_delta_entry(newSegment, posTableIndex, slice, elementData));
                        }
                        totalLen += localLen;
                        break;
                    case 0x3f0a:
                        if (newSegment->sliceCount > 0)
                        {
                            CHK_MALLOC_ARRAY_OFAIL(sliceOffset, uint32_t, newSegment->sliceCount);
                        }
                        if (newSegment->posTableCount > 0)
                        {
                            CHK_MALLOC_ARRAY_OFAIL(posTable, mxfRational, newSegment->posTableCount);
                        }
                        /* NOTE: Avid ignores the local len and only looks at the index array len value
                           so we don't check that the local len is correct and update the total len
                           after each read */
                        CHK_OFAIL(mxf_read_uint32(reader->mxfFile, &indexEntryArrayLen));
                        totalLen += 4;
                        CHK_OFAIL(mxf_read_uint32(reader->mxfFile, &indexEntryLen));
                        totalLen += 4;
                        CHK_OFAIL(indexEntryLen == (uint32_t)11 + newSegment->sliceCount * 4 + newSegment->posTableCount * 8);
        
                        CHK_MALLOC_ARRAY_OFAIL(reader->essenceReader->data->avidFrameOffsets, int64_t, indexEntryArrayLen);
                        reader->essenceReader->data->numAvidFrameOffsets = indexEntryArrayLen;
                            
                        for (k = 0; k < indexEntryArrayLen; k++)
                        {
                            CHK_OFAIL(mxf_read_uint8(reader->mxfFile, &temporalOffset));
                            totalLen += 1;
                            CHK_OFAIL(mxf_read_uint8(reader->mxfFile, &keyFrameOffset));
                            totalLen += 1;
                            CHK_OFAIL(mxf_read_uint8(reader->mxfFile, &flags));
                            totalLen += 1;
                            CHK_OFAIL(mxf_read_uint64(reader->mxfFile, &streamOffset));
                            totalLen += 8;
                            for (i = 0; i < newSegment->sliceCount; i++)
                            {
                                CHK_OFAIL(mxf_read_uint32(reader->mxfFile, &sliceOffset[i]));
                                totalLen += 4;
                            }
                            for (i = 0; i < newSegment->posTableCount; i++)
                            {
                                CHK_OFAIL(mxf_read_int32(reader->mxfFile, &posTable[i].numerator));
                                totalLen += 4;
                                CHK_OFAIL(mxf_read_int32(reader->mxfFile, &posTable[i].denominator));
                                totalLen += 4;
                            }
                            
                            reader->essenceReader->data->avidFrameOffsets[k] = streamOffset;
                        }
                        break;
                    default:
                        mxf_log(MXF_WLOG, "Unknown local item (%u) in index table segment", localTag);
                        CHK_OFAIL(mxf_skip(reader->mxfFile, localLen));
                        totalLen += localLen;
                }
                
            }
            CHK_ORET(totalLen == segmentLen);
            
            SAFE_FREE(&sliceOffset);
            SAFE_FREE(&posTable);
            mxf_free_index_table_segment(&newSegment);
            return 1;
        }
        else
        {
            CHK_OFAIL(mxf_skip(reader->mxfFile, len));
        }
    }
    
fail:
    SAFE_FREE(&reader->essenceReader->data->avidFrameOffsets);
    reader->essenceReader->data->numAvidFrameOffsets = 0;
    SAFE_FREE(&sliceOffset);
    SAFE_FREE(&posTable);
    mxf_free_index_table_segment(&newSegment);
    return 0;
}

static int get_avid_mjpeg_frame_info(MXFReader* reader, int64_t frameNumber, int64_t* offset, int64_t* frameSize)
{
    CHK_ORET(frameNumber < reader->essenceReader->data->numAvidFrameOffsets - 1);
    
    *offset = reader->essenceReader->data->avidFrameOffsets[frameNumber];
    *frameSize = reader->essenceReader->data->avidFrameOffsets[frameNumber + 1] - *offset;
    
    return 1;
}

static int process_metadata(MXFReader* reader, MXFPartition* partition)
{
    MXFFile* mxfFile = reader->mxfFile;
    EssenceReader* essenceReader = reader->essenceReader;
    EssenceReaderData* data = essenceReader->data;
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    MXFMetadataSet* essContainerDataSet;
    MXFMetadataSet* sourcePackageSet;
    MXFMetadataSet* sourcePackageTrackSet;
    MXFMetadataSet* materialPackageSet = NULL;
    MXFMetadataSet* materialPackageTrackSet;
    MXFMetadataSet* descriptorSet;
    MXFArrayItemIterator arrayIter;
    mxfUL dataDefUL;
    int haveVideoOrAudioTrack;
    MXFTrack* track;
    EssenceTrack* essenceTrack;
    mxfRational videoEditRate;
    int haveVideoTrack;
    mxfUMID sourcePackageUID;
    mxfUMID packageUID;
    uint32_t trackID;

    
    CHK_ORET(add_track(reader, &track));
    CHK_ORET(add_essence_track(essenceReader, &essenceTrack));

    /* use this essence container label rather than the one in the FileDescriptor
     which is sometimes a weak reference to a ContainerDefinition in Avid files,
     and the ContainerDefinition is not of much use */
    CHK_ORET(mxf_get_list_length(&partition->essenceContainers) == 1);
    track->essenceContainerLabel = *(mxfUL*)mxf_get_list_element(&partition->essenceContainers, 0);
    
    
    /* load Avid extensions to the data model */
    
    CHK_ORET(mxf_avid_load_extensions(reader->dataModel));
    CHK_ORET(mxf_finalise_data_model(reader->dataModel));
    
    
    /* create and read the header metadata */
    
    CHK_ORET(mxf_read_next_nonfiller_kl(mxfFile, &key, &llen, &len));
    CHK_ORET(mxf_is_header_metadata(&key));
    CHK_ORET(mxf_create_header_metadata(&data->headerMetadata, reader->dataModel));
    CHK_ORET(mxf_read_header_metadata(mxfFile, data->headerMetadata, 
        partition->headerByteCount, &key, llen, len));
    
    
    /* get the body and index SID */
    
    CHK_ORET(mxf_find_singular_set_by_key(data->headerMetadata, &MXF_SET_K(EssenceContainerData), &essContainerDataSet));
    CHK_ORET(mxf_get_uint32_item(essContainerDataSet, &MXF_ITEM_K(EssenceContainerData, BodySID), &essenceTrack->bodySID));
    if (mxf_have_item(essContainerDataSet, &MXF_ITEM_K(EssenceContainerData, IndexSID)))
    {
        CHK_ORET(mxf_get_uint32_item(essContainerDataSet, &MXF_ITEM_K(EssenceContainerData, IndexSID), &essenceTrack->indexSID));
    }
    else
    {
        essenceTrack->indexSID = 0;
    }
    
    
    /* get the top level file source package */
    
    CHK_ORET(mxf_uu_get_top_file_package(data->headerMetadata, &sourcePackageSet));

    
    /* get the id and number of the material track referencing the file source package */
    /* Note: is will equal 0 if no material package is present */     

    track->materialTrackID = 0;
    CHK_ORET(mxf_get_umid_item(sourcePackageSet, &MXF_ITEM_K(GenericPackage, PackageUID), &sourcePackageUID));
    if (mxf_find_singular_set_by_key(data->headerMetadata, &MXF_SET_K(MaterialPackage), &materialPackageSet))
    {
        CHK_ORET(mxf_uu_get_package_tracks(materialPackageSet, &arrayIter));
        while (mxf_uu_next_track(data->headerMetadata, &arrayIter, &materialPackageTrackSet))
        {
            if (mxf_uu_get_track_reference(materialPackageTrackSet, &packageUID, &trackID))
            {
                if (mxf_equals_umid(&sourcePackageUID, &packageUID))
                {
                    if (mxf_have_item(materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber)))
                    {
                        CHK_ORET(mxf_get_uint32_item(materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), 
                            &track->materialTrackNumber));
                    }
                    else
                    {
                        track->materialTrackNumber = 0;
                    }
                    CHK_ORET(mxf_get_uint32_item(materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackID), 
                        &track->materialTrackID));
                    break;
                }
            }

        }
    }
    
    
    /* get the track info for the audio or video track */
    
    haveVideoOrAudioTrack = 0;
    CHK_ORET(mxf_uu_get_package_tracks(sourcePackageSet, &arrayIter));
    while (mxf_uu_next_track(data->headerMetadata, &arrayIter, &sourcePackageTrackSet))
    {
        CHK_ORET(mxf_uu_get_track_datadef(sourcePackageTrackSet, &dataDefUL));
        if (!mxf_is_picture(&dataDefUL) && !mxf_is_sound(&dataDefUL) && !mxf_is_timecode(&dataDefUL))
        {
            /* some Avid files have a weak reference to a DataDefinition instead of a UL */ 
            mxf_avid_get_data_def(data->headerMetadata, (mxfUUID*)&dataDefUL, &dataDefUL);
        }
        
        if (mxf_is_picture(&dataDefUL) || mxf_is_sound(&dataDefUL))
        {
            CHK_ORET(!haveVideoOrAudioTrack);
            
            if (mxf_have_item(sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber)))
            {
                CHK_ORET(mxf_get_uint32_item(sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), &essenceTrack->trackNumber));
            }
            else
            {
                essenceTrack->trackNumber = 0;
            }
            CHK_ORET(mxf_get_rational_item(sourcePackageTrackSet, &MXF_ITEM_K(Track, EditRate), &essenceTrack->frameRate));
            clean_rate(&essenceTrack->frameRate);
            reader->clip.frameRate = essenceTrack->frameRate;
            CHK_ORET(mxf_uu_get_track_duration(sourcePackageTrackSet, &essenceTrack->playoutDuration));
            reader->clip.duration = essenceTrack->playoutDuration;
            if (mxf_is_picture(&dataDefUL))
            {
                track->video.frameRate = essenceTrack->frameRate;
                track->isVideo = 1;
            }
            else
            {
                track->isVideo = 0;
            }
            
            haveVideoOrAudioTrack = 1;
            break;
        }
    }
    CHK_ORET(haveVideoOrAudioTrack);
    
    
    /* get the info from the descriptor */

    CHK_ORET(mxf_get_strongref_item(sourcePackageSet, &MXF_ITEM_K(SourcePackage, Descriptor), &descriptorSet));

    if (mxf_is_subclass_of(data->headerMetadata->dataModel, &descriptorSet->key, &MXF_SET_K(CDCIEssenceDescriptor)))
    {
        CHK_ORET(process_cdci_descriptor(descriptorSet, track, essenceTrack));
    }
    else if (mxf_is_subclass_of(data->headerMetadata->dataModel, &descriptorSet->key, &MXF_SET_K(WaveAudioDescriptor)))
    {
        CHK_ORET(process_wav_descriptor(descriptorSet, track, essenceTrack));
    }
    else
    {
        mxf_log(MXF_ELOG, "Unsupported file descriptor" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
        return 0;
    }

    
    
    /* if essence is audio and the material package has a video track then use that 
       edit rate for the clip, else default to 25/1 */

    materialPackageSet = NULL;
    mxf_find_singular_set_by_key(data->headerMetadata, &MXF_SET_K(MaterialPackage), &materialPackageSet);
    if (!track->isVideo)
    {
        haveVideoTrack = 0;
        if (materialPackageSet != NULL)
        {
            CHK_ORET(mxf_uu_get_package_tracks(materialPackageSet, &arrayIter));
            while (mxf_uu_next_track(data->headerMetadata, &arrayIter, &materialPackageTrackSet))
            {
                CHK_ORET(mxf_uu_get_track_datadef(materialPackageTrackSet, &dataDefUL));
                if (mxf_is_picture(&dataDefUL))
                {
                    CHK_ORET(mxf_get_rational_item(materialPackageTrackSet, &MXF_ITEM_K(Track, EditRate), &videoEditRate));
                    clean_rate(&videoEditRate);
                    haveVideoTrack = 1;
                    break;
                }
            }
        }
        
        /* default to 25/1 edit rate */
        if (!haveVideoTrack)
        {
            videoEditRate.numerator = 25;
            videoEditRate.denominator = 1;
        }

        essenceTrack->playoutDuration = essenceTrack->playoutDuration * 
            videoEditRate.numerator * essenceTrack->frameRate.denominator / 
            (double)(essenceTrack->frameRate.numerator * videoEditRate.denominator);
        reader->clip.duration = essenceTrack->playoutDuration;
        
        essenceTrack->frameRate = videoEditRate;
        reader->clip.frameRate = essenceTrack->frameRate;
        
        essenceTrack->frameSize = (uint32_t)(track->audio.blockAlign * 
            track->audio.samplingRate.numerator * essenceTrack->frameRate.denominator / 
            (double)(track->audio.samplingRate.denominator * essenceTrack->frameRate.numerator));
    }

    
    /* initialise the playout timecode */
    
    if (materialPackageSet != NULL)
    {
        if (!initialise_playout_timecode(reader, materialPackageSet))
        {
            CHK_ORET(initialise_default_playout_timecode(reader));
        }
    }
    else
    {
        CHK_ORET(initialise_default_playout_timecode(reader));
    }

    
    /* initialise the source timecodes */
    
    initialise_source_timecodes(reader, sourcePackageSet);
    
    
    return 1;
}

static void opatom_close(MXFReader* reader)
{
    if (reader->essenceReader == NULL || reader->essenceReader->data == NULL)
    {
        return;
    }

    mxf_free_header_metadata(&reader->essenceReader->data->headerMetadata);
    mxf_free_partition(&reader->essenceReader->data->headerPartition);
    SAFE_FREE(&reader->essenceReader->data->avidFrameOffsets);
    
    SAFE_FREE(&reader->essenceReader->data);
}

static int opatom_position_at_frame(MXFReader* reader, int64_t frameNumber)
{
    MXFFile* mxfFile = reader->mxfFile;
    int64_t filePos;
    EssenceReaderData* data = reader->essenceReader->data;
    EssenceTrack* essenceTrack;
    int64_t frameSize;
    int64_t fileOffset;
    
    CHK_ORET(mxf_file_is_seekable(mxfFile));
    
    /* get file position so we can reset when something fails */
    CHK_ORET((filePos = mxf_file_tell(mxfFile)) >= 0);
    
    essenceTrack = get_essence_track(reader->essenceReader, 0);
    if (essenceTrack->frameSize < 0)
    {
        CHK_OFAIL(reader->essenceReader->data->avidFrameOffsets != NULL);
        CHK_OFAIL(get_avid_mjpeg_frame_info(reader, frameNumber, &fileOffset, &frameSize));
        CHK_OFAIL(mxf_file_seek(mxfFile, data->essenceStartPos + fileOffset, SEEK_SET));
    }
    else
    {
        CHK_OFAIL(mxf_file_seek(mxfFile, 
            data->essenceStartPos + essenceTrack->frameSize * frameNumber, 
            SEEK_SET));
    }

    data->currentPosition = frameNumber;
    
    return 1;
    
fail:
    CHK_ORET(mxf_file_seek(mxfFile, filePos, SEEK_SET));
    return 0;
}

static int64_t opatom_get_last_written_frame_number(MXFReader* reader)
{
    MXFFile* mxfFile = reader->mxfFile;
    EssenceReaderData* data = reader->essenceReader->data;
    EssenceTrack* essenceTrack;
    int64_t fileSize;
    int64_t targetPosition;

    if (reader->clip.duration < 0)
    {
        /* not supported because we could return a position past the end into the footer */
        return -1;
    }
    if ((fileSize = mxf_file_size(mxfFile)) < 0)
    {
        return -1;
    }
    
    essenceTrack = get_essence_track(reader->essenceReader, 0);

    if (essenceTrack->frameSize < 0)
    {
        /* if the index table wasn't read then we don't know the length */
        if (reader->essenceReader->data->avidFrameOffsets == NULL)
        {
            return -1;
        }
        targetPosition = reader->clip.duration - 1;
    }
    else
    {
        /* get file size and calculate the furthest position */    
        targetPosition = (fileSize - data->essenceStartPos) / essenceTrack->frameSize - 1;
        if (reader->clip.duration >= 0 && targetPosition >= reader->clip.duration)
        {
            targetPosition = reader->clip.duration - 1;
        }
    }
    
    return targetPosition;
}

static int opatom_skip_next_frame(MXFReader* reader)
{
    MXFFile* mxfFile = reader->mxfFile;
    int64_t filePos;
    EssenceTrack* essenceTrack;
    int64_t frameSize;
    int64_t fileOffset;
    
    essenceTrack = get_essence_track(reader->essenceReader, 0);
    
    /* get file position so we can reset when something fails */
    CHK_ORET((filePos = mxf_file_tell(mxfFile)) >= 0);
    
    if (essenceTrack->frameSize < 0)
    {
        CHK_OFAIL(reader->essenceReader->data->avidFrameOffsets != NULL);
        CHK_OFAIL(get_avid_mjpeg_frame_info(reader, reader->essenceReader->data->currentPosition, &fileOffset, &frameSize));
        CHK_OFAIL(mxf_skip(mxfFile, frameSize));
    }
    else
    {
        CHK_OFAIL(mxf_skip(mxfFile, essenceTrack->frameSize));
    }
    
    reader->essenceReader->data->currentPosition++;
    
    return 1;
    
fail:
    if (mxf_file_is_seekable(mxfFile))
    {
        CHK_ORET(mxf_file_seek(mxfFile, filePos, SEEK_SET));
    }
    /* TODO: recovery when file is not seekable; eg. store state and try recover 
       when next frame is read or position is set */
    return 0;
}

static int opatom_read_next_frame(MXFReader* reader, MXFReaderListener* listener)
{
    MXFFile* mxfFile = reader->mxfFile;
    int64_t filePos;
    EssenceTrack* essenceTrack;
    uint8_t* buffer;
    uint64_t bufferSize;
    int64_t frameSize;
    int64_t fileOffset;
    
    essenceTrack = get_essence_track(reader->essenceReader, 0);
    
    /* get file position so we can reset when something fails */
    CHK_ORET((filePos = mxf_file_tell(mxfFile)) >= 0);
    
    if (essenceTrack->frameSize < 0)
    {
        CHK_OFAIL(reader->essenceReader->data->avidFrameOffsets != NULL);
        CHK_OFAIL(get_avid_mjpeg_frame_info(reader, reader->essenceReader->data->currentPosition, &fileOffset, &frameSize));
        if (accept_frame(listener, 0))
        {
            CHK_OFAIL(read_frame(reader, listener, 0, frameSize, &buffer, &bufferSize));
            CHK_OFAIL(send_frame(reader, listener, 0, buffer, bufferSize));
        }
        else
        {
            CHK_OFAIL(mxf_skip(mxfFile, frameSize));
        }
    }
    else
    {
        if (accept_frame(listener, 0))
        {
            CHK_OFAIL(read_frame(reader, listener, 0, essenceTrack->frameSize, &buffer, &bufferSize));
            CHK_OFAIL(send_frame(reader, listener, 0, buffer, bufferSize));
        }
        else
        {
            CHK_OFAIL(mxf_skip(mxfFile, essenceTrack->frameSize));
        }
    }

    reader->essenceReader->data->currentPosition++;    
    
    return 1;
    
fail:
    if (mxf_file_is_seekable(mxfFile))
    {
        CHK_ORET(mxf_file_seek(mxfFile, filePos, SEEK_SET));
    }
    /* TODO: recovery when file is not seekable; eg. store state and try recover 
       when next frame is read or position is set */
    return 0;
}

static int64_t opatom_get_next_frame_number(MXFReader* reader)
{
    return reader->essenceReader->data->currentPosition;
}

static MXFHeaderMetadata* opatom_get_header_metadata(MXFReader* reader)
{
    return reader->essenceReader->data->headerMetadata;
}

static int opatom_have_footer_metadata(MXFReader* reader)
{
    return reader->essenceReader->data->haveFooterMetadata;
}

int opa_is_supported(MXFPartition* headerPartition)
{
    mxfUL* label;
    
    if (!is_op_atom(&headerPartition->operationalPattern))
    {
        return 0;
    }
   
    
    CHK_ORET(mxf_get_list_length(&headerPartition->essenceContainers) == 1);
    label = (mxfUL*)mxf_get_list_element(&headerPartition->essenceContainers, 0);

    if (mxf_equals_ul(label, &MXF_EC_L(IECDV_25_525_60_ClipWrapped)) || 
        mxf_equals_ul(label, &MXF_EC_L(IECDV_25_625_50_ClipWrapped)) || 
        mxf_equals_ul(label, &MXF_EC_L(DVBased_25_525_60_ClipWrapped)) || 
        mxf_equals_ul(label, &MXF_EC_L(DVBased_25_625_50_ClipWrapped)) || 
        mxf_equals_ul(label, &MXF_EC_L(DVBased_50_525_60_ClipWrapped)) || 
        mxf_equals_ul(label, &MXF_EC_L(DVBased_50_625_50_ClipWrapped)))
    {
        return 1;
    }
    else if (mxf_equals_ul(label, &MXF_EC_L(BWFClipWrapped)) ||
        mxf_equals_ul(label, &MXF_EC_L(AES3ClipWrapped)))
    {
        return 1;
    }
    else if (mxf_equals_ul(label, &MXF_EC_L(SD_Unc_625_50i_422_135_ClipWrapped)))
    {
        return 1;
    }
    else if (mxf_equals_ul(label, &MXF_EC_L(HD_Unc_1080_50i_422_ClipWrapped)))
    {
        return 1;
    }
    else if (mxf_equals_ul(label, &MXF_EC_L(AvidMJPEGClipWrapped)))
    {
        return 1;
    }
    else if (mxf_equals_ul(label, &MXF_EC_L(DNxHD1080i120ClipWrapped)))
    {
        return 1;
    }

        
    
    return 0;
}

int opa_initialise_reader(MXFReader* reader, MXFPartition** headerPartition)
{
    MXFFile* mxfFile = reader->mxfFile;
    EssenceReader* essenceReader = reader->essenceReader;
    EssenceReaderData* data;
    EssenceTrack* essenceTrack;
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    int64_t filePos;

    essenceReader->data = NULL;
    
    
    /* init essence reader */
    
    CHK_MALLOC_OFAIL(essenceReader->data, EssenceReaderData);
    memset(essenceReader->data, 0, sizeof(EssenceReaderData));
    essenceReader->data->headerPartition = *headerPartition;
    essenceReader->close = opatom_close;
    essenceReader->position_at_frame = opatom_position_at_frame;
    essenceReader->skip_next_frame = opatom_skip_next_frame;
    essenceReader->read_next_frame = opatom_read_next_frame;
    essenceReader->get_next_frame_number = opatom_get_next_frame_number;
    essenceReader->get_last_written_frame_number = opatom_get_last_written_frame_number;
    essenceReader->get_header_metadata = opatom_get_header_metadata;
    essenceReader->have_footer_metadata = opatom_have_footer_metadata;
    
    data = essenceReader->data;

    
    /* process the header metadata */
    
    CHK_OFAIL(process_metadata(reader, data->headerPartition));
    CHK_OFAIL(get_num_essence_tracks(essenceReader) == 1);
    essenceTrack = get_essence_track(essenceReader, 0);
    
    
    /* read the index table for Avid MJPEG files */
    
    if (mxf_equals_ul(&MXF_EC_L(AvidMJPEGClipWrapped), &get_mxf_track(reader, 0)->essenceContainerLabel))
    {
        CHK_OFAIL((filePos = mxf_file_tell(mxfFile)) >= 0);
        CHK_OFAIL(read_avid_mjpeg_index_segment(reader));
        CHK_OFAIL(mxf_file_seek(mxfFile, filePos, SEEK_SET));    
    }
    
    
    /* move to start of essence container in the body partition */

    CHK_ORET(mxf_read_next_nonfiller_kl(mxfFile, &key, &llen, &len));
    if (!mxf_is_body_partition_pack(&key))
    {
        CHK_OFAIL(mxf_skip(mxfFile, data->headerPartition->indexByteCount - mxfKey_extlen - llen));
        CHK_OFAIL(mxf_read_next_nonfiller_kl(mxfFile, &key, &llen, &len));
    }
    CHK_OFAIL(mxf_is_body_partition_pack(&key));
    CHK_OFAIL(mxf_skip(mxfFile, len));
    CHK_OFAIL(mxf_read_next_nonfiller_kl(mxfFile, &key, &llen, &len));
    CHK_OFAIL(mxf_is_gc_essence_element(&key) || 
        is_avid_mjpeg_essence_element(&key) ||
        is_avid_dnxhd_essence_element(&key));

    CHK_OFAIL((filePos = mxf_file_tell(mxfFile)) >= 0);
    data->essenceStartPos = filePos;
    data->currentPosition = 0;

    
    *headerPartition = NULL; /* take ownership */
    return 1;
    
fail:
    reader->essenceReader->data->headerPartition = NULL; /* release ownership */
    /* essenceReader->close() will be called when closing the reader */
    return 0;
}







