/*
 * $Id: mxf_avid.h,v 1.1 2006/12/20 15:40:19 john_f Exp $
 *
 * Avid data model extensions and utilities
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
 
#ifndef __MXF_AVID_H__
#define __MXF_AVID_H__


#ifdef __cplusplus
extern "C" 
{
#endif


#include <mxf/mxf_avid_labels_and_keys.h>
#include <mxf/mxf_avid_metadict_blob.h>


typedef struct _MXFAvidObjectReference
{
    struct _MXFAvidObjectReference* next;
    
    mxfUUID instanceUID;
    uint64_t offset;
    uint8_t flags;
} MXFAvidObjectReference;

typedef struct
{
    MXFAvidObjectReference* references;
} MXFAvidObjectDirectory;

typedef struct
{
    mxfUUID id;
    int64_t directoryOffset;
    uint32_t formatVersion;
    mxfUUID metaDictionaryInstanceUID;
    mxfUUID prefaceInstanceUID;
} MXFAvidMetadataRoot;

typedef MXFMetadataSet MXFAvidMetadataRootSet;


#define MXF_LABEL(d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15) \
    {d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15}

#define MXF_SET_DEFINITION(parentName, name, label) \
    static const mxfUL MXF_SET_K(name) = label;
    
#define MXF_ITEM_DEFINITION(setName, name, label, localTag, typeId) \
    static const mxfUL MXF_ITEM_K(setName, name) = label;

#include <mxf/mxf_avid_extensions_data_model.h>

#undef MXF_SET_DEFINITION
#undef MXF_ITEM_DEFINITION
#undef MXF_LABEL

int mxf_avid_load_extensions(MXFDataModel* dataModel);


int mxf_avid_write_header_metadata(MXFFile* mxfFile, MXFHeaderMetadata* headerMetadata);

void mxf_generate_aafsdk_umid(mxfUMID* umid);

int mxf_avid_set_indirect_string_item(MXFMetadataSet* set, const mxfKey* itemKey, const mxfUTF16Char* value);

int mxf_avid_get_data_def(MXFHeaderMetadata* headerMetadata, mxfUUID* uuid, mxfUL* dataDef);

int mxf_avid_write_index_entry_array_header(MXFFile* mxfFile, uint8_t sliceCount, uint8_t posTableCount, 
    uint32_t numIndexEntries);

int mxf_avid_attach_mob_attribute(MXFHeaderMetadata* headerMetadata, MXFMetadataSet* packageSet, 
    mxfUTF16Char* name, mxfUTF16Char* value);
int mxf_avid_attach_user_comment(MXFHeaderMetadata* headerMetadata, MXFMetadataSet* packageSet, 
    mxfUTF16Char* name, mxfUTF16Char* value);


#ifdef __cplusplus
}
#endif


#endif


