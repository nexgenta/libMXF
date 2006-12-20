/*
 * $Id: mxf_avid.c,v 1.1 2006/12/20 16:01:10 john_f Exp $
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
 
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


#if defined(_WIN32)

#include <time.h>
#include <windows.h>

#else

#include <uuid/uuid.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/times.h>

#endif


#include <mxf/mxf.h>
#include <mxf/mxf_avid.h>
#include <mxf/mxf_avid_metadict_blob.h>


static int mxf_avid_create_object_directory(MXFAvidObjectDirectory** directory)
{
    MXFAvidObjectDirectory* newDirectory;
    
    CHK_MALLOC_ORET(newDirectory, MXFAvidObjectDirectory);
    memset(newDirectory, 0, sizeof(MXFAvidObjectDirectory));
    
    *directory = newDirectory;
    return 1;   
}

static void mxf_avid_free_object_directory(MXFAvidObjectDirectory** directory)
{
    if (*directory == NULL)
    {
        return;
    }
    
    if ((*directory)->references != NULL)
    {
        MXFAvidObjectReference* entry = (*directory)->references->next;
        while (entry != NULL)
        {
            MXFAvidObjectReference* tmpNextEntry = entry->next;
            SAFE_FREE(&entry);
            entry = tmpNextEntry;
        }
        SAFE_FREE(&(*directory)->references);
    }
    SAFE_FREE(directory);
}

static int mxf_avid_add_object_directory_entry(MXFAvidObjectDirectory* directory,
    const mxfUUID* instanceUID, uint64_t offset, uint8_t flags)
{
    MXFAvidObjectReference* newEntry = NULL;
    
    CHK_MALLOC_ORET(newEntry, MXFAvidObjectReference);
    memset(newEntry, 0, sizeof(MXFAvidObjectReference));
    newEntry->instanceUID = *instanceUID;
    newEntry->offset = offset;
    newEntry->flags = flags;
    
    if (directory->references == NULL)
    {
        directory->references = newEntry;
    }
    else
    {
        MXFAvidObjectReference* lastEntry = directory->references;
        while (lastEntry->next != NULL)
        {
            lastEntry = lastEntry->next;
        }
        lastEntry->next = newEntry;
    }

    return 1;
}

/* Note: must call this function just prior to writing the header metadata so 
   that the initial offset is correct */
static int mxf_avid_add_header_dir_entries(MXFFile* mxfFile, MXFAvidObjectDirectory* directory, 
    MXFHeaderMetadata* headerMetadata)
{
    MXFListIterator iter;
    int64_t offset;
    MXFMetadataSet* set;
    uint64_t len;
    uint8_t llen;
    
    CHK_ORET((offset = mxf_file_tell(mxfFile)) >= 0);
    
    mxf_initialise_list_iter(&iter, &headerMetadata->sets);
    while (mxf_next_list_iter_element(&iter))
    {
        set = (MXFMetadataSet*)mxf_get_iter_element(&iter);
        mxf_avid_add_object_directory_entry(directory, &set->instanceUID, offset, 0x00);

        mxf_get_set_len(mxfFile, set, &llen, &len);
        offset += mxfKey_extlen + llen + len;
    }
    
    return 1;
}        

static int mxf_avid_write_object_directory(MXFFile* mxfFile, const MXFAvidObjectDirectory* directory)
{
    const MXFAvidObjectReference* entry = NULL;
    uint64_t numEntries = 0;

    entry = directory->references;
    while (entry != NULL)
    {
        numEntries++;
        entry = entry->next;
    }
    
    CHK_ORET(mxf_write_k(mxfFile, &g_AvidObjectDirectory_key));
    CHK_ORET(mxf_write_l(mxfFile, 9 + 25*numEntries));

    CHK_ORET(mxf_write_uint64(mxfFile, numEntries));
    CHK_ORET(mxf_write_uint8(mxfFile, 25));
        
    entry = directory->references;
    while (entry != NULL)
    {
        CHK_ORET(mxf_write_uuid(mxfFile, &entry->instanceUID));
        CHK_ORET(mxf_write_uint64(mxfFile, entry->offset));
        CHK_ORET(mxf_write_uint8(mxfFile, entry->flags));
        entry = entry->next;
    }
    
    return 1;
}

/* Note: the set is not appended to the header metadata set list. This ensures that
   the items in the root are not registered in the Primer. 
   However, applications must update the Avid ObjectDirectory with the position of the root set 
   because this would only happen automatically if the set were part of the header metadata */
static int mxf_avid_create_metadata_root(MXFHeaderMetadata* headerMetadata, MXFAvidMetadataRootSet** set)
{
    MXFMetadataSet* newSet;
    CHK_ORET(mxf_create_set(headerMetadata, &g_AvidMetadataRoot_key, &newSet));
    CHK_ORET(mxf_remove_set(headerMetadata, newSet));
        
    *set = newSet;
    return 1;
}

/* Note: the assumption is that metadata items are only added to the root set using this function 
   This allows us to re-write items in the same order */
static int mxf_avid_set_metadata_root(MXFAvidMetadataRootSet* set, const MXFAvidMetadataRoot* root)
{
    MXFMetadataItem* newItem;
    uint8_t value[24];
    uint16_t len;
    mxfUUID instanceUID;
    

    /* keep the instanceUID and remove all other items */
    CHK_ORET(mxf_get_uuid_item(set, &MXF_ITEM_K(InterchangeObject, InstanceUID), &instanceUID));
    mxf_clear_list(&set->items);
    CHK_ORET(mxf_create_item(set, &MXF_ITEM_K(InterchangeObject, InstanceUID), 0x3c0a, &newItem));
    mxf_set_uuid(&instanceUID, value);
    CHK_ORET(mxf_set_item_value(newItem, value, mxfUUID_extlen));

    
    /* (re-)write the items */
    
    mxf_set_uuid(&root->id, value);
    mxf_set_uint64(root->directoryOffset, &value[mxfUUID_extlen]);
    len = mxfUUID_extlen + 8;
    CHK_ORET(mxf_create_item(set, &g_Null_Key, 0x0003, &newItem));
    CHK_ORET(mxf_set_item_value(newItem, value, len));

    mxf_set_uint32(root->formatVersion, value);
    len = 4;
    CHK_ORET(mxf_create_item(set, &g_Null_Key, 0x0004, &newItem));
    CHK_ORET(mxf_set_item_value(newItem, value, len));
        
    mxf_set_uuid(&root->metaDictionaryInstanceUID, value);
    len = mxfUUID_extlen;
    CHK_ORET(mxf_create_item(set, &g_Null_Key, 0x0001, &newItem));
    CHK_ORET(mxf_set_item_value(newItem, value, len));

    mxf_set_uuid(&root->prefaceInstanceUID, value);
    len = mxfUUID_extlen;
    CHK_ORET(mxf_create_item(set, &g_Null_Key, 0x0002, &newItem));
    CHK_ORET(mxf_set_item_value(newItem, value, len));

    return 1;
}

static int mxf_avid_register_metadict_tags(MXFPrimerPack* primerPack)
{
    uint32_t i;
    mxfLocalTag assignedTag;  // ignored
    for (i = 0; i < g_AvidMetaDictTags_len; i++)
    {
        CHK_ORET(mxf_register_primer_entry(primerPack, &g_AvidMetaDictTags[i].uid, 
            g_AvidMetaDictTags[i].localTag, &assignedTag));
    }
    
    return 1;
}

static int mxf_avid_write_metadict_blob(MXFFile* mxfFile)
{
    const uint32_t maxWriteBytes = 4096;
    const uint8_t* dataPtr = g_AvidMetaDictBlob;
    uint32_t numBytes;
    uint32_t totalBytes = 0;
    int done = 0;
    while (!done)
    {
        if (g_AvidMetaDictBlob_len - totalBytes < maxWriteBytes)
        {
            done = 1;
            numBytes = g_AvidMetaDictBlob_len - totalBytes;
        }
        else
        {
            numBytes = maxWriteBytes;
        }
        if (numBytes > 0)
        {
            CHK_ORET(mxf_file_write(mxfFile, dataPtr, numBytes) == numBytes);
            dataPtr += numBytes;
            totalBytes += numBytes;
        }
    }
    
    return 1;
}

static int mxf_avid_register_metadict_object_offsets(uint64_t startOffset,
    MXFAvidObjectDirectory* directory)
{
    uint32_t i;
    for (i = 0; i < g_AvidMetaDictObjectOffsets_len; i++)
    {
        CHK_ORET(mxf_avid_add_object_directory_entry(directory, &g_AvidMetaDictObjectOffsets[i].instanceUID,
            g_AvidMetaDictObjectOffsets[i].offset + startOffset, g_AvidMetaDictObjectOffsets[i].flags));
    }
    
    return 1;
}
    

static int mxf_avid_fixup_dynamic_tags_in_blob(MXFPrimerPack* primerPack)
{
    mxfLocalTag tag;
    uint32_t i;
    for (i = 0; i < g_AvidMetaDictDynTagOffsets_len; i++)
    {
        /* check if the Avid item extension has been used and therefore that a tag 
           has been registered in the primer pack */
        if (!mxf_get_item_tag(primerPack, &g_AvidMetaDictDynTagOffsets[i].itemKey,
            &tag))
        {
            /* item extension has not been used so create a new unique tag in the meta-definition */
            CHK_ORET(mxf_create_item_tag(primerPack, &tag));
        }
        
        /* fixup the tag in the meta-definition */
        mxf_set_uint16(tag, &g_AvidMetaDictBlob[g_AvidMetaDictDynTagOffsets[i].tagOffset]);
    }
    
    return 1;
}



#define MXF_SET_DEFINITION(parentName, name, label) \
    CHK_ORET(mxf_register_set_def(dataModel, #name, &MXF_SET_K(parentName), &MXF_SET_K(name)));
    
#define MXF_ITEM_DEFINITION(setName, name, label, tag, typeId) \
    CHK_ORET(mxf_register_item_def(dataModel, #name, &MXF_SET_K(setName), &MXF_ITEM_K(setName, name), tag, typeId));
    
int mxf_avid_load_extensions(MXFDataModel* dataModel)
{
#include <mxf/mxf_avid_extensions_data_model.h>

    return 1;
}

#undef MXF_SET_DEFINITION
#undef MXF_ITEM_DEFINITION



int mxf_avid_write_header_metadata(MXFFile* mxfFile, MXFHeaderMetadata* headerMetadata)
{
    int64_t rootMetadataSetPos;
    int64_t headerMetadataSetsPos;
    int64_t endPos;
    MXFAvidObjectDirectory* objectDirectory = NULL;
    MXFAvidMetadataRootSet* avidRootSet = NULL;
    MXFAvidMetadataRoot avidRoot;
    MXFMetadataSet* prefaceSet;

    /* init the avid root */    
    avidRoot.id = g_Null_UUID;
    avidRoot.directoryOffset = 0;
    avidRoot.formatVersion = 0x0008;
    avidRoot.metaDictionaryInstanceUID = g_AvidMetaDictInstanceUID_uuid;
    CHK_OFAIL(mxf_find_singular_set_by_key(headerMetadata, &MXF_SET_K(Preface), &prefaceSet));
    avidRoot.prefaceInstanceUID = prefaceSet->instanceUID;
    
    /* create object directory and root metadata set */
    CHK_OFAIL(mxf_avid_create_object_directory(&objectDirectory));
    CHK_OFAIL(mxf_avid_create_metadata_root(headerMetadata, &avidRootSet));

    
    /* update the primer with the tags in the blob and write */
    CHK_OFAIL(mxf_avid_register_metadict_tags(headerMetadata->primerPack));
    CHK_OFAIL(mxf_write_header_primer_pack(mxfFile, headerMetadata));
    
    /* write the avid root metadata set (and update the object directory) */
    CHK_OFAIL((rootMetadataSetPos = mxf_file_tell(mxfFile)) >= 0);
    mxf_avid_add_object_directory_entry(objectDirectory,
        &avidRootSet->instanceUID, rootMetadataSetPos, 0x00);
    CHK_OFAIL(mxf_avid_set_metadata_root(avidRootSet, &avidRoot));
    CHK_OFAIL(mxf_write_set(mxfFile, avidRootSet));
    
    /* write the Avid meta-dictionary blob */
    CHK_OFAIL((headerMetadataSetsPos = mxf_file_tell(mxfFile)) >= 0);
    CHK_OFAIL(mxf_avid_fixup_dynamic_tags_in_blob(headerMetadata->primerPack));
    CHK_OFAIL(mxf_avid_write_metadict_blob(mxfFile));
    CHK_OFAIL(mxf_avid_register_metadict_object_offsets(headerMetadataSetsPos, objectDirectory));
        
    /* write the header metadata; record the object directory entries before writing */
    CHK_OFAIL(mxf_avid_add_header_dir_entries(mxfFile, objectDirectory, headerMetadata));
    CHK_OFAIL(mxf_write_header_sets(mxfFile, headerMetadata));
    
    /* write the avid object directory */
    CHK_OFAIL((avidRoot.directoryOffset = mxf_file_tell(mxfFile)) >= 0);
    CHK_OFAIL(mxf_avid_write_object_directory(mxfFile, objectDirectory));
    CHK_OFAIL((endPos = mxf_file_tell(mxfFile)) >= 0);

    /* go back and re-write the Avid root set with an updated object directory offset */
    CHK_OFAIL(mxf_avid_set_metadata_root(avidRootSet, &avidRoot));
    CHK_OFAIL(mxf_file_seek(mxfFile, rootMetadataSetPos, SEEK_SET));
    CHK_OFAIL(mxf_write_set(mxfFile, avidRootSet));
 
    /* position file after object directory */
    CHK_OFAIL(mxf_file_seek(mxfFile, endPos, SEEK_SET));
    
    
    mxf_free_set(&avidRootSet);
    mxf_avid_free_object_directory(&objectDirectory);
    return 1;
    
fail:
    mxf_free_set(&avidRootSet);
    mxf_avid_free_object_directory(&objectDirectory);
    return 0;
}



/* MobID generation code following the same algorithm as implemented in the AAF SDK */
/* NOTE: this function is not guaranteed (but is it highly unlikely?) to create a 
   unique UMID in multi-threaded environment */
void mxf_generate_aafsdk_umid(mxfUMID* umid)
{
    uint32_t major, minor;
    static uint32_t last_part2 = 0;

    major = (uint32_t)time(NULL);
#if defined(_WIN32)
    minor = (uint32_t)GetTickCount();
#else
    struct tms tms_buf;
    minor = (uint32_t)times(&tms_buf);
    assert(minor != 0 && minor != (uint32_t)-1);
#endif

    if (last_part2 >= minor)
    {
        minor = last_part2 + 1;
    }
        
    last_part2 = minor;

    umid->octet0 = 0x06;
    umid->octet1 = 0x0a;
    umid->octet2 = 0x2b;
    umid->octet3 = 0x34;
    umid->octet4 = 0x01;         
    umid->octet5 = 0x01;     
    umid->octet6 = 0x01; 
    umid->octet7 = 0x01;
    umid->octet8 = 0x01;
    umid->octet9 = 0x01;
    umid->octet10 = 0x0f; /* material type not identified */
    umid->octet11 = 0x00; /* no method specified for material and instance number generation */
    umid->octet12 = 0x13;
    umid->octet13 = 0x00;
    umid->octet14 = 0x00;
    umid->octet15 = 0x00;

    umid->octet24 = 0x06;
    umid->octet25 = 0x0e;
    umid->octet26 = 0x2b;
    umid->octet27 = 0x34;
    umid->octet28 = 0x7f;
    umid->octet29 = 0x7f;
    umid->octet30 = (uint8_t)(42 & 0x7f); /* company specific prefix = 42 */
    umid->octet31 = (uint8_t)((42 >> 7L) | 0x80);  /* company specific prefix = 42 */
    
    umid->octet16 = (uint8_t)((major >> 24) & 0xff);
    umid->octet17 = (uint8_t)((major >> 16) & 0xff);
    umid->octet18 = (uint8_t)((major >> 8) & 0xff);
    umid->octet19 = (uint8_t)(major & 0xff);
    
    umid->octet20 = (uint8_t)(((uint16_t)(minor & 0xFFFF) >> 8) & 0xff);
    umid->octet21 = (uint8_t)((uint16_t)(minor & 0xFFFF) & 0xff);
    
    umid->octet22 = (uint8_t)(((uint16_t)((minor >> 16L) & 0xFFFF) >> 8) & 0xff);
    umid->octet23 = (uint8_t)((uint16_t)((minor >> 16L) & 0xFFFF) & 0xff);

}


int mxf_avid_set_indirect_string_item(MXFMetadataSet* set, const mxfKey* itemKey, const mxfUTF16Char* value)
{
    uint8_t* buffer = NULL;
    uint16_t size;
    /* prefix is 0x42 ('B') for big endian, followed by half-swapped key for String type */
    const uint8_t prefix[17] =
        {0x42, 0x01, 0x10, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x0e, 0x2b, 0x34, 0x01, 0x04, 0x01, 0x01};
                       
    size = 17 + mxf_get_external_utf16string_size(value);
    
    CHK_MALLOC_ARRAY_ORET(buffer, uint8_t, size);
    memset(buffer, 0, size);
    
    memcpy(buffer, prefix, 17);
    mxf_set_utf16string(value, &buffer[17]);
    
    CHK_OFAIL(mxf_set_item(set, itemKey, buffer, size));

    SAFE_FREE(&buffer);    
    return 1;
    
fail:
    SAFE_FREE(&buffer);    
    return 0;    
}


/* in some Avid files, the StructuralComponent::DataDefinition is not a UL but is a 
   weak reference to a DataDefinition object in the Dictionary 
   So we try dereferencing it, expecting to find a DataDefinition object with Identification item */
int mxf_avid_get_data_def(MXFHeaderMetadata* headerMetadata, mxfUUID* uuid, mxfUL* dataDef)
{
    MXFMetadataSet* dataDefSet;
    
    if (mxf_dereference(headerMetadata, uuid, &dataDefSet))
    {
        if (mxf_get_ul_item(dataDefSet, &MXF_ITEM_K(DefinitionObject, Identification), dataDef))
        {
            return 1;
        }
    }
    return 0;
}


int mxf_avid_write_index_entry_array_header(MXFFile* mxfFile, uint8_t sliceCount, uint8_t posTableCount, 
    uint32_t numIndexEntries)
{
    CHK_ORET(mxf_write_local_tag(mxfFile, 0x3f0a));
    if (8 + numIndexEntries * (11 + sliceCount * 4 + posTableCount * 8) > 0xffff)
    {
        /* Avid ignores the local set 16-bit size restriction and relies on the array header to
           provide the size */
        CHK_ORET(mxf_write_uint16(mxfFile, 0xffff));
    }
    else
    {
        CHK_ORET(mxf_write_uint16(mxfFile, (uint16_t)(8 + numIndexEntries * (11 + sliceCount * 4 + posTableCount * 8))));
    }
    CHK_ORET(mxf_write_uint32(mxfFile, numIndexEntries));
    CHK_ORET(mxf_write_uint32(mxfFile, 11 + sliceCount * 4 + posTableCount * 8));
    
    return 1;
}

int mxf_avid_attach_mob_attribute(MXFHeaderMetadata* headerMetadata, MXFMetadataSet* packageSet, 
    mxfUTF16Char* name, mxfUTF16Char* value)
{
    MXFMetadataSet* taggedValueSet;
    CHK_ORET(name != NULL && value != NULL);
    
    CHK_ORET(mxf_create_set(headerMetadata, &MXF_SET_K(TaggedValue), &taggedValueSet));
    CHK_ORET(mxf_add_array_item_strongref(packageSet, &MXF_ITEM_K(GenericPackage, MobAttributeList), taggedValueSet));
    CHK_ORET(mxf_set_utf16string_item(taggedValueSet, &MXF_ITEM_K(TaggedValue, Name), name));
    CHK_ORET(mxf_avid_set_indirect_string_item(taggedValueSet, &MXF_ITEM_K(TaggedValue, Value), value));
    
    return 1;
}    

int mxf_avid_attach_user_comment(MXFHeaderMetadata* headerMetadata, MXFMetadataSet* packageSet, 
    mxfUTF16Char* name, mxfUTF16Char* value)
{
    MXFMetadataSet* taggedValueSet;
    CHK_ORET(name != NULL && value != NULL);
    
    CHK_ORET(mxf_create_set(headerMetadata, &MXF_SET_K(TaggedValue), &taggedValueSet));
    CHK_ORET(mxf_add_array_item_strongref(packageSet, &MXF_ITEM_K(GenericPackage, UserComments), taggedValueSet));
    CHK_ORET(mxf_set_utf16string_item(taggedValueSet, &MXF_ITEM_K(TaggedValue, Name), name));
    CHK_ORET(mxf_avid_set_indirect_string_item(taggedValueSet, &MXF_ITEM_K(TaggedValue, Value), value));
    
    return 1;
}    


