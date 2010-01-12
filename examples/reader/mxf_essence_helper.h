/*
 * $Id: mxf_essence_helper.h,v 1.2 2010/01/12 16:25:04 john_f Exp $
 *
 * Utilities for processing essence data and associated metadata
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
 
#ifndef __MXF_ESSENCE_HELPER_H__
#define __MXF_ESSENCE_HELPER_H__


int is_d10_essence(const mxfUL* label);

int process_cdci_descriptor(MXFMetadataSet* descriptorSet, MXFTrack* track, EssenceTrack* essenceTrack);
int process_wav_descriptor(MXFMetadataSet* descriptorSet, MXFTrack* track, EssenceTrack* essenceTrack);
int process_sound_descriptor(MXFMetadataSet* descriptorSet, MXFTrack* track, EssenceTrack* essenceTrack);

int convert_aes_to_pcm(uint32_t channelCount, uint32_t bitsPerSample, 
    uint8_t* buffer, uint64_t aesDataLen, uint64_t* pcmDataLen);

int accept_frame(MXFReaderListener* listener, int trackIndex);
int read_frame(MXFReader* reader, MXFReaderListener* listener, int trackIndex, 
    uint64_t frameSize, uint8_t** buffer, uint64_t* bufferSize);
int send_frame(MXFReader* reader, MXFReaderListener* listener, int trackIndex, 
    uint8_t* buffer, uint64_t dataLen);
    
int element_is_known_system_item(const mxfKey* key);
int extract_system_item_info(MXFReader* reader, const mxfKey* key, uint64_t len, mxfPosition position);


#endif



