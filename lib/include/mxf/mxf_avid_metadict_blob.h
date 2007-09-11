/*
 * $Id: mxf_avid_metadict_blob.h,v 1.2 2007/09/11 13:24:53 stuart_hc Exp $
 *
 * Blobs of data containing Avid header metadata extensions
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
 
#ifndef __AVID_METADICT_BLOB_H__
#define __AVID_METADICT_BLOB_H__


#ifdef __cplusplus
extern "C" 
{
#endif


extern const mxfUUID g_AvidMetaDictInstanceUID_uuid;

extern const struct AvidMetaDictTagStruct
{
    mxfLocalTag localTag;
    mxfUID uid;
} g_AvidMetaDictTags[];

extern const uint32_t g_AvidMetaDictTags_len;


extern const struct AvidMetaDictDynTagOffsetsStruct
{
    mxfKey itemKey;
    uint64_t tagOffset;
} g_AvidMetaDictDynTagOffsets[]; 

extern const uint32_t g_AvidMetaDictDynTagOffsets_len;


extern const struct AvidMetaDictObjectOffsetsStruct
{
    mxfUUID instanceUID;
    uint64_t offset;
    uint8_t flags;
} g_AvidMetaDictObjectOffsets[];

extern const uint32_t g_AvidMetaDictObjectOffsets_len;


extern const uint8_t g_AvidMetaDictBlob[];

extern const uint32_t g_AvidMetaDictBlob_len;


#ifdef __cplusplus
}
#endif


#endif

