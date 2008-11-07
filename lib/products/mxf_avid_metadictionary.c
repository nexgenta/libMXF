/*
 * $Id: mxf_avid_metadictionary.c,v 1.1 2008/11/07 14:12:59 philipn Exp $
 *
 * Avid (AAF) Meta-dictionary
 *
 * Copyright (C) 2008  Philip de Nier <philipn@users.sourceforge.net>
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <mxf/mxf.h>
#include <mxf/mxf_avid.h>


typedef struct
{
    mxfUL identification;
    mxfUUID instanceUID;
} MetaDefData;

typedef struct
{
    MXFMetadataItem* item;
    int arrayIndex;
    mxfUL targetIdentification;
} WeakRefData;



static int add_weakref_to_list(MXFList* list, MXFMetadataItem* item, int arrayIndex, const mxfUL* targetIdentification)
{
    WeakRefData* data = NULL;
    
    CHK_MALLOC_ORET(data, WeakRefData);
    data->item = item;
    data->arrayIndex = arrayIndex;
    data->targetIdentification = *targetIdentification;
    
    CHK_OFAIL(mxf_append_list_element(list, (void*)data));
    data = NULL;
    
    return 1;
    
fail:
    SAFE_FREE(&data);
    return 0;
}

static int add_metadef_to_list(MXFList* list, const mxfUL* identification, const mxfUUID* instanceUID)
{
    MetaDefData* data = NULL;
    
    CHK_MALLOC_ORET(data, MetaDefData);
    data->identification = *identification;
    data->instanceUID = *instanceUID;
    
    CHK_OFAIL(mxf_append_list_element(list, (void*)data));
    data = NULL;
    
    return 1;
    
fail:
    SAFE_FREE(&data);
    return 0;
}

static uint8_t* get_array_element(MXFMetadataItem* item, int index)
{
    uint32_t arrayLen;
    uint32_t arrayItemLen;
    
    mxf_get_array_header(item->value, &arrayLen, &arrayItemLen);
    
    return item->value + 8 + index * arrayItemLen;
}

static int find_weakref_target_instance_uid(MXFList* mapList, const mxfUL* targetIdentification, mxfUUID* instanceUID)
{
    MXFListIterator iter;
    
    mxf_initialise_list_iter(&iter, mapList);
    while (mxf_next_list_iter_element(&iter))
    {
        MetaDefData* data = (MetaDefData*)mxf_get_iter_element(&iter);
        
        if (mxf_equals_ul(&data->identification, targetIdentification))
        {
            *instanceUID = data->instanceUID;
            return 1;
        }
    }
    
    return 0;
}


static int metadict_before_set_read(void* privateData, MXFHeaderMetadata* headerMetadata, 
        const mxfKey* key, uint8_t llen, uint64_t len, int* skip)
{
    if (mxf_avid_is_metadictionary(headerMetadata->dataModel, key) ||
        mxf_avid_is_metadef(headerMetadata->dataModel, key))
    {
        *skip = 1;
    }
    else
    {
        *skip = 0;
    }
    
    return 1;
}

static int append_name_to_string_array(MXFMetadataSet* set, const mxfKey* itemKey, const mxfUTF16Char* name)
{
    uint8_t* nameArray = NULL;
    uint16_t existingNameArraySize = 0;
    uint16_t nameArraySize = 0;
    MXFMetadataItem* namesItem = NULL;
    
    if (mxf_have_item(set, itemKey))
    {
        CHK_ORET(mxf_get_item(set, itemKey, &namesItem));
        existingNameArraySize = namesItem->length;
    }
    nameArraySize = existingNameArraySize + (uint16_t)(mxfUTF16Char_extlen * (wcslen(name) + 1));
    
    CHK_MALLOC_ARRAY_ORET(nameArray, uint8_t, nameArraySize);
    if (existingNameArraySize > 0)
    {
        memcpy(nameArray, namesItem->value, existingNameArraySize);
    }
    mxf_set_utf16string(name, &nameArray[existingNameArraySize]);
    
    CHK_OFAIL(mxf_set_item(set, itemKey, nameArray, nameArraySize));

    SAFE_FREE(&nameArray);
    return 1;
    
fail:
    SAFE_FREE(&nameArray);
    return 0;
}

static mxfUL* bounce_label(uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3, uint8_t d4, uint8_t d5, 
    uint8_t d6, uint8_t d7, uint8_t d8, uint8_t d9, uint8_t d10, uint8_t d11, uint8_t d12, uint8_t d13, 
    uint8_t d14, uint8_t d15, mxfUL* result)
{
    result->octet0 = d0;
    result->octet1 = d1;
    result->octet2 = d2;
    result->octet3 = d3;
    result->octet4 = d4;
    result->octet5 = d5;
    result->octet6 = d6;
    result->octet7 = d7;
    result->octet8 = d8;
    result->octet9 = d9;
    result->octet10 = d10;
    result->octet11 = d11;
    result->octet12 = d12;
    result->octet13 = d13;
    result->octet14 = d14;
    result->octet15 = d15;

    return result;
}




int mxf_avid_is_metadictionary(MXFDataModel* dataModel, const mxfKey* setKey)
{
    return mxf_is_subclass_of(dataModel, setKey, &MXF_SET_K(MetaDictionary));
}

int mxf_avid_is_metadef(MXFDataModel* dataModel, const mxfKey* setKey)
{
    return mxf_is_subclass_of(dataModel, setKey, &MXF_SET_K(MetaDefinition));
}


int mxf_avid_create_metadictionary(MXFHeaderMetadata* headerMetadata, MXFMetadataSet** metaDictSet)
{
    MXFMetadataSet* newSet = NULL;
    
    CHK_ORET(mxf_create_set(headerMetadata, &MXF_SET_K(MetaDictionary), &newSet));

    *metaDictSet = newSet;
    return 1;
}


int mxf_avid_set_metadef_items(MXFMetadataSet* set, const mxfUL* id, const mxfUTF16Char* name, const mxfUTF16Char* description)
{
    CHK_ORET(mxf_set_ul_item(set, &MXF_ITEM_K(MetaDefinition, Identification), id));
    CHK_ORET(name != NULL);
    CHK_ORET(mxf_set_utf16string_item(set, &MXF_ITEM_K(MetaDefinition, Name), name));
    if (description != NULL)
    {
        CHK_ORET(mxf_set_utf16string_item(set, &MXF_ITEM_K(MetaDefinition, Description), description));
    }
    
    return 1;
}

int mxf_avid_create_classdef(MXFMetadataSet* metaDictSet, const mxfUL* id, const mxfUTF16Char* name, const mxfUTF16Char* description, const mxfUL* parentId, mxfBoolean isConcrete, MXFMetadataSet** classDefSet)
{
    MXFMetadataSet* newSet = NULL;
    
    CHK_ORET(mxf_create_set(metaDictSet->headerMetadata, &MXF_SET_K(ClassDefinition), &newSet));
    CHK_ORET(mxf_add_array_item_strongref(metaDictSet, &MXF_ITEM_K(MetaDictionary, ClassDefinitions), newSet));
    
    CHK_ORET(mxf_avid_set_metadef_items(newSet, id, name, description));

    CHK_ORET(mxf_set_ul_item(newSet, &MXF_ITEM_K(ClassDefinition, ParentClass), parentId));
    CHK_ORET(mxf_set_boolean_item(newSet, &MXF_ITEM_K(ClassDefinition, IsConcrete), isConcrete));
    
    
    *classDefSet = newSet;
    return 1;
}

int mxf_avid_create_propertydef(MXFPrimerPack* primerPack, MXFMetadataSet* classDefSet, const mxfUL* id, const mxfUTF16Char* name, const mxfUTF16Char* description, const mxfUL* typeId, mxfBoolean isOptional, mxfLocalTag localId, mxfBoolean isUniqueIdentifier, MXFMetadataSet** propertyDefSet)
{
    MXFMetadataSet* newSet = NULL;
    mxfLocalTag assignedLocalId;
    
    CHK_ORET(mxf_register_primer_entry(primerPack, id, localId, &assignedLocalId));
    
    CHK_ORET(mxf_create_set(classDefSet->headerMetadata, &MXF_SET_K(PropertyDefinition), &newSet));
    CHK_ORET(mxf_add_array_item_strongref(classDefSet, &MXF_ITEM_K(ClassDefinition, Properties), newSet));
    
    CHK_ORET(mxf_avid_set_metadef_items(newSet, id, name, description));

    CHK_ORET(mxf_set_ul_item(newSet, &MXF_ITEM_K(PropertyDefinition, Type), typeId));
    CHK_ORET(mxf_set_boolean_item(newSet, &MXF_ITEM_K(PropertyDefinition, IsOptional), isOptional));
    CHK_ORET(mxf_set_uint16_item(newSet, &MXF_ITEM_K(PropertyDefinition, LocalIdentification), assignedLocalId));
    if (isUniqueIdentifier)
    {
        CHK_ORET(mxf_set_boolean_item(newSet, &MXF_ITEM_K(PropertyDefinition, IsUniqueIdentifier), isUniqueIdentifier));
    }
    
    *propertyDefSet = newSet;
    return 1;
}

int mxf_avid_create_typedef(MXFMetadataSet* metaDictSet, const mxfKey* setId, const mxfUL* id, const mxfUTF16Char* name, const mxfUTF16Char* description, MXFMetadataSet** typeDefSet)
{
    MXFMetadataSet* newSet = NULL;
    
    CHK_ORET(mxf_create_set(metaDictSet->headerMetadata, setId, &newSet));
    CHK_ORET(mxf_add_array_item_strongref(metaDictSet, &MXF_ITEM_K(MetaDictionary, TypeDefinitions), newSet));
    
    CHK_ORET(mxf_avid_set_metadef_items(newSet, id, name, description));
    
    
    *typeDefSet = newSet;
    return 1;
}

int mxf_avid_create_typedef_char(MXFMetadataSet* metaDictSet, const mxfUL* id, const mxfUTF16Char* name, const mxfUTF16Char* description, MXFMetadataSet** typeDefSet)
{
    return mxf_avid_create_typedef(metaDictSet, &MXF_SET_K(TypeDefinitionCharacter), id, name, description, typeDefSet);
}

int mxf_avid_create_typedef_enum(MXFMetadataSet* metaDictSet, const mxfUL* id, const mxfUTF16Char* name, const mxfUTF16Char* description, const mxfUL* typeId, MXFMetadataSet** typeDefSet)
{
    MXFMetadataSet* newSet = NULL;
    
    CHK_ORET(mxf_avid_create_typedef(metaDictSet, &MXF_SET_K(TypeDefinitionEnumeration), id, name, description, &newSet));

    CHK_ORET(mxf_set_ul_item(newSet, &MXF_ITEM_K(TypeDefinitionEnumeration, Type), typeId));
    
    *typeDefSet = newSet;
    return 1;
}

int mxf_avid_add_typedef_enum_element(MXFMetadataSet* typeDefSet, const mxfUTF16Char* name, int64_t value)
{
    uint8_t* elementValue;

    CHK_ORET(append_name_to_string_array(typeDefSet, &MXF_ITEM_K(TypeDefinitionEnumeration, Names), name));    
    
    CHK_ORET(mxf_grow_array_item(typeDefSet, &MXF_ITEM_K(TypeDefinitionEnumeration, Values), 8, 1, &elementValue));
    mxf_set_int64(value, elementValue);

    return 1;
}

int mxf_avid_create_typedef_extenum(MXFMetadataSet* metaDictSet, const mxfUL* id, const mxfUTF16Char* name, const mxfUTF16Char* description, MXFMetadataSet** typeDefSet)
{
    MXFMetadataSet* newSet = NULL;
    
    CHK_ORET(mxf_avid_create_typedef(metaDictSet, &MXF_SET_K(TypeDefinitionExtendibleEnumeration), id, name, description, &newSet));
    
    *typeDefSet = newSet;
    return 1;
}

int mxf_avid_add_typedef_extenum_element(MXFMetadataSet* typeDefSet, const mxfUTF16Char* name, const mxfUL* value)
{
    uint8_t* elementValue;

    CHK_ORET(append_name_to_string_array(typeDefSet, &MXF_ITEM_K(TypeDefinitionExtendibleEnumeration, Names), name));    
    
    CHK_ORET(mxf_grow_array_item(typeDefSet, &MXF_ITEM_K(TypeDefinitionExtendibleEnumeration, Values), mxfUL_extlen, 1, &elementValue));
    mxf_set_ul(value, elementValue);

    return 1;
}

int mxf_avid_create_typedef_fixedarray(MXFMetadataSet* metaDictSet, const mxfUL* id, const mxfUTF16Char* name, const mxfUTF16Char* description, const mxfUL* elementTypeId, uint32_t elementCount, MXFMetadataSet** typeDefSet)
{
    MXFMetadataSet* newSet = NULL;
    
    CHK_ORET(mxf_avid_create_typedef(metaDictSet, &MXF_SET_K(TypeDefinitionFixedArray), id, name, description, &newSet));

    CHK_ORET(mxf_set_ul_item(newSet, &MXF_ITEM_K(TypeDefinitionFixedArray, ElementType), elementTypeId));
    CHK_ORET(mxf_set_uint32_item(newSet, &MXF_ITEM_K(TypeDefinitionFixedArray, ElementCount), elementCount));

    
    *typeDefSet = newSet;
    return 1;
}

int mxf_avid_create_typedef_indirect(MXFMetadataSet* metaDictSet, const mxfUL* id, const mxfUTF16Char* name, const mxfUTF16Char* description, MXFMetadataSet** typeDefSet)
{
    return mxf_avid_create_typedef(metaDictSet, &MXF_SET_K(TypeDefinitionIndirect), id, name, description, typeDefSet);
}

int mxf_avid_create_typedef_integer(MXFMetadataSet* metaDictSet, const mxfUL* id, const mxfUTF16Char* name, const mxfUTF16Char* description, uint8_t size, mxfBoolean isSigned, MXFMetadataSet** typeDefSet)
{
    MXFMetadataSet* newSet = NULL;
    
    CHK_ORET(mxf_avid_create_typedef(metaDictSet, &MXF_SET_K(TypeDefinitionInteger), id, name, description, &newSet));

    CHK_ORET(mxf_set_uint8_item(newSet, &MXF_ITEM_K(TypeDefinitionInteger, Size), size));
    CHK_ORET(mxf_set_boolean_item(newSet, &MXF_ITEM_K(TypeDefinitionInteger, IsSigned), isSigned));
    
    
    *typeDefSet = newSet;
    return 1;
}

int mxf_avid_create_typedef_opaque(MXFMetadataSet* metaDictSet, const mxfUL* id, const mxfUTF16Char* name, const mxfUTF16Char* description, MXFMetadataSet** typeDefSet)
{
    return mxf_avid_create_typedef(metaDictSet, &MXF_SET_K(TypeDefinitionOpaque), id, name, description, typeDefSet);
}

int mxf_avid_create_typedef_record(MXFMetadataSet* metaDictSet, const mxfUL* id, const mxfUTF16Char* name, const mxfUTF16Char* description, MXFMetadataSet** typeDefSet)
{
    MXFMetadataSet* newSet = NULL;
    
    CHK_ORET(mxf_avid_create_typedef(metaDictSet, &MXF_SET_K(TypeDefinitionRecord), id, name, description, &newSet));
    
    *typeDefSet = newSet;
    return 1;
}

int mxf_avid_add_typedef_record_member(MXFMetadataSet* typeDefSet, const mxfUTF16Char* name, const mxfUL* typeId)
{
    uint8_t* elementValue;

    CHK_ORET(append_name_to_string_array(typeDefSet, &MXF_ITEM_K(TypeDefinitionRecord, MemberNames), name));    
    
    CHK_ORET(mxf_grow_array_item(typeDefSet, &MXF_ITEM_K(TypeDefinitionRecord, MemberTypes), mxfUL_extlen, 1, &elementValue));
    mxf_set_ul(typeId, elementValue);

    return 1;
}

int mxf_avid_create_typedef_rename(MXFMetadataSet* metaDictSet, const mxfUL* id, const mxfUTF16Char* name, const mxfUTF16Char* description, const mxfUL* renamedTypeId, MXFMetadataSet** typeDefSet)
{
    MXFMetadataSet* newSet = NULL;
    
    CHK_ORET(mxf_avid_create_typedef(metaDictSet, &MXF_SET_K(TypeDefinitionRename), id, name, description, &newSet));
    
    CHK_ORET(mxf_set_ul_item(newSet, &MXF_ITEM_K(TypeDefinitionRename, RenamedType), renamedTypeId));


    *typeDefSet = newSet;
    return 1;
}

int mxf_avid_create_typedef_set(MXFMetadataSet* metaDictSet, const mxfUL* id, const mxfUTF16Char* name, const mxfUTF16Char* description, const mxfUL* elementTypeId, MXFMetadataSet** typeDefSet)
{
    MXFMetadataSet* newSet = NULL;
    
    CHK_ORET(mxf_avid_create_typedef(metaDictSet, &MXF_SET_K(TypeDefinitionSet), id, name, description, &newSet));
    
    CHK_ORET(mxf_set_ul_item(newSet, &MXF_ITEM_K(TypeDefinitionSet, ElementType), elementTypeId));


    *typeDefSet = newSet;
    return 1;
}

int mxf_avid_create_typedef_stream(MXFMetadataSet* metaDictSet, const mxfUL* id, const mxfUTF16Char* name, const mxfUTF16Char* description, MXFMetadataSet** typeDefSet)
{
    return mxf_avid_create_typedef(metaDictSet, &MXF_SET_K(TypeDefinitionStream), id, name, description, typeDefSet);
}

int mxf_avid_create_typedef_string(MXFMetadataSet* metaDictSet, const mxfUL* id, const mxfUTF16Char* name, const mxfUTF16Char* description, const mxfUL* elementTypeId, MXFMetadataSet** typeDefSet)
{
    MXFMetadataSet* newSet = NULL;
    
    CHK_ORET(mxf_avid_create_typedef(metaDictSet, &MXF_SET_K(TypeDefinitionString), id, name, description, &newSet));
    
    CHK_ORET(mxf_set_ul_item(newSet, &MXF_ITEM_K(TypeDefinitionString, ElementType), elementTypeId));


    *typeDefSet = newSet;
    return 1;
}

int mxf_avid_create_typedef_strongref(MXFMetadataSet* metaDictSet, const mxfUL* id, const mxfUTF16Char* name, const mxfUTF16Char* description, const mxfUL* referencedTypeId, MXFMetadataSet** typeDefSet)
{
    MXFMetadataSet* newSet = NULL;
    
    CHK_ORET(mxf_avid_create_typedef(metaDictSet, &MXF_SET_K(TypeDefinitionStrongObjectReference), id, name, description, &newSet));
    
    CHK_ORET(mxf_set_ul_item(newSet, &MXF_ITEM_K(TypeDefinitionStrongObjectReference, ReferencedType), referencedTypeId));


    *typeDefSet = newSet;
    return 1;
}

int mxf_avid_create_typedef_vararray(MXFMetadataSet* metaDictSet, const mxfUL* id, const mxfUTF16Char* name, const mxfUTF16Char* description, const mxfUL* elementTypeId, MXFMetadataSet** typeDefSet)
{
    MXFMetadataSet* newSet = NULL;
    
    CHK_ORET(mxf_avid_create_typedef(metaDictSet, &MXF_SET_K(TypeDefinitionVariableArray), id, name, description, &newSet));
    
    CHK_ORET(mxf_set_ul_item(newSet, &MXF_ITEM_K(TypeDefinitionVariableArray, ElementType), elementTypeId));


    *typeDefSet = newSet;
    return 1;
}

int mxf_avid_create_typedef_weakref(MXFMetadataSet* metaDictSet, const mxfUL* id, const mxfUTF16Char* name, const mxfUTF16Char* description, const mxfUL* referencedTypeId, MXFMetadataSet** typeDefSet)
{
    MXFMetadataSet* newSet = NULL;

    CHK_ORET(mxf_avid_create_typedef(metaDictSet, &MXF_SET_K(TypeDefinitionWeakObjectReference), id, name, description, &newSet));
    
    CHK_ORET(mxf_set_ul_item(newSet, &MXF_ITEM_K(TypeDefinitionWeakObjectReference, ReferencedType), referencedTypeId));
    
    
    *typeDefSet = newSet;
    return 1;
}

int mxf_avid_add_typedef_weakref_target(MXFMetadataSet* typeDefSet, const mxfUL* targetId)
{
    uint8_t* elementValue;
    
    CHK_ORET(mxf_grow_array_item(typeDefSet, &MXF_ITEM_K(TypeDefinitionWeakObjectReference, ReferenceTargetSet), mxfUL_extlen, 1, &elementValue));
    mxf_set_ul(targetId, elementValue);
    
    return 1;
}


int mxf_initialise_metadict_read_filter(MXFReadFilter* filter)
{
    filter->privateData = NULL;
    filter->before_set_read = metadict_before_set_read;
    filter->after_set_read = NULL;
    
    return 1;
}

void mxf_clear_metadict_read_filter(MXFReadFilter* filter)
{
    if (filter == NULL)
    {
        return;
    }
    
    /* nothing to clear */
    return;
}




int mxf_avid_create_default_metadictionary(MXFHeaderMetadata* headerMetadata, MXFMetadataSet** metaDictSet)
{
    MXFMetadataSet* newMetaDictSet = NULL;
    MXFMetadataSet* classDefSet;
    MXFMetadataSet* set;
    mxfUL label1;
    mxfUL label2;
    mxfUL label3;
    MXFList classMetaDefList;
    MXFList typeMetaDefList;
    MXFList classWeakRefList;
    MXFList typeWeakRefList;
    MXFMetadataItem* item;
    MXFListIterator iter;
    mxfUUID targetInstanceUID;
    int arrayIndex;
    
    mxf_initialise_list(&classMetaDefList, free);
    mxf_initialise_list(&typeMetaDefList, free);
    mxf_initialise_list(&classWeakRefList, free);
    mxf_initialise_list(&typeWeakRefList, free);

    
    CHK_OFAIL(mxf_avid_create_metadictionary(headerMetadata, &newMetaDictSet));
    
    /* register meta-definitions */
    /* set temporary weak reference values which will be replaced later */
    
#define LABEL(d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15) \
    bounce_label(d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15, &label1)

#define LABEL_2(d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15) \
    bounce_label(d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15, &label2)

#define WEAKREF(d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15) \
    bounce_label(d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15, &label3)

#define CLASS_DEF(id, name, description, parentId, isConcrete) \
    CHK_OFAIL(mxf_avid_create_classdef(newMetaDictSet, id, name, description, parentId, isConcrete, &classDefSet)); \
    CHK_OFAIL(mxf_get_item(classDefSet, &MXF_ITEM_K(ClassDefinition, ParentClass), &item)); \
    CHK_OFAIL(add_weakref_to_list(&classWeakRefList, item, -1, parentId)); \
    CHK_OFAIL(add_metadef_to_list(&classMetaDefList, id, &classDefSet->instanceUID));
    
#define PROPERTY_DEF(id, name, description, typeId, isOptional, localId, isUniqueId) \
    CHK_OFAIL(mxf_avid_create_propertydef(classDefSet->headerMetadata->primerPack, classDefSet, \
        id, name, description, typeId, isOptional, localId, isUniqueId, &set));
    
#define CHARACTER_DEF(id, name, description) \
    CHK_OFAIL(mxf_avid_create_typedef_char(newMetaDictSet, id, name, description, &set)); \
    CHK_OFAIL(add_metadef_to_list(&typeMetaDefList, id, &set->instanceUID));

#define ENUM_DEF(id, name, description, typeId) \
    CHK_OFAIL(mxf_avid_create_typedef_enum(newMetaDictSet, id, name, description, typeId, &set)); \
    CHK_OFAIL(mxf_get_item(set, &MXF_ITEM_K(TypeDefinitionEnumeration, Type), &item)); \
    CHK_OFAIL(add_weakref_to_list(&typeWeakRefList, item, -1, typeId)); \
    CHK_OFAIL(add_metadef_to_list(&typeMetaDefList, id, &set->instanceUID));
#define ENUM_ELEMENT(name, value) \
    CHK_OFAIL(mxf_avid_add_typedef_enum_element(set, name, value));

#define EXTENUM_DEF(id, name, description) \
    CHK_OFAIL(mxf_avid_create_typedef_extenum(newMetaDictSet, id, name, description, &set)); \
    CHK_OFAIL(add_metadef_to_list(&typeMetaDefList, id, &set->instanceUID));
#define EXTENUM_ELEMENT(name, value) \
    CHK_OFAIL(mxf_avid_add_typedef_extenum_element(set, name, value));

#define FIXEDARRAY_DEF(id, name, description, typeId, count) \
    CHK_OFAIL(mxf_avid_create_typedef_fixedarray(newMetaDictSet, id, name, description, typeId, count, &set)); \
    CHK_OFAIL(mxf_get_item(set, &MXF_ITEM_K(TypeDefinitionFixedArray, ElementType), &item)); \
    CHK_OFAIL(add_weakref_to_list(&typeWeakRefList, item, -1, typeId)); \
    CHK_OFAIL(add_metadef_to_list(&typeMetaDefList, id, &set->instanceUID));

#define INDIRECT_DEF(id, name, description) \
    CHK_OFAIL(mxf_avid_create_typedef_indirect(newMetaDictSet, id, name, description, &set)); \
    CHK_OFAIL(add_metadef_to_list(&typeMetaDefList, id, &set->instanceUID));

#define INTEGER_DEF(id, name, description, size, isSigned) \
    CHK_OFAIL(mxf_avid_create_typedef_integer(newMetaDictSet, id, name, description, size, isSigned, &set)); \
    CHK_OFAIL(add_metadef_to_list(&typeMetaDefList, id, &set->instanceUID));

#define OPAQUE_DEF(id, name, description) \
    CHK_OFAIL(mxf_avid_create_typedef_opaque(newMetaDictSet, id, name, description, &set)); \
    CHK_OFAIL(add_metadef_to_list(&typeMetaDefList, id, &set->instanceUID));

#define RENAME_DEF(id, name, description, typeId) \
    CHK_OFAIL(mxf_avid_create_typedef_rename(newMetaDictSet, id, name, description, typeId, &set)); \
    CHK_OFAIL(mxf_get_item(set, &MXF_ITEM_K(TypeDefinitionRename, RenamedType), &item)); \
    CHK_OFAIL(add_weakref_to_list(&typeWeakRefList, item, -1, typeId)); \
    CHK_OFAIL(add_metadef_to_list(&typeMetaDefList, id, &set->instanceUID));

#define RECORD_DEF(id, name, description) \
    CHK_OFAIL(mxf_avid_create_typedef_record(newMetaDictSet, id, name, description, &set)); \
    CHK_OFAIL(add_metadef_to_list(&typeMetaDefList, id, &set->instanceUID)); \
    arrayIndex = 0;
#define RECORD_MEMBER(name, type) \
    CHK_OFAIL(mxf_avid_add_typedef_record_member(set, name, type)); \
    CHK_OFAIL(mxf_get_item(set, &MXF_ITEM_K(TypeDefinitionRecord, MemberTypes), &item)); \
    CHK_OFAIL(add_weakref_to_list(&typeWeakRefList, item, arrayIndex, type)); \
    arrayIndex++;

#define SET_DEF(id, name, description, typeId) \
    CHK_OFAIL(mxf_avid_create_typedef_set(newMetaDictSet, id, name, description, typeId, &set)); \
    CHK_OFAIL(mxf_get_item(set, &MXF_ITEM_K(TypeDefinitionSet, ElementType), &item)); \
    CHK_OFAIL(add_weakref_to_list(&typeWeakRefList, item, -1, typeId)); \
    CHK_OFAIL(add_metadef_to_list(&typeMetaDefList, id, &set->instanceUID));

#define STREAM_DEF(id, name, description) \
    CHK_OFAIL(mxf_avid_create_typedef_stream(newMetaDictSet, id, name, description, &set)); \
    CHK_OFAIL(add_metadef_to_list(&typeMetaDefList, id, &set->instanceUID));

#define STRING_DEF(id, name, description, typeId) \
    CHK_OFAIL(mxf_avid_create_typedef_string(newMetaDictSet, id, name, description, typeId, &set)); \
    CHK_OFAIL(mxf_get_item(set, &MXF_ITEM_K(TypeDefinitionString, ElementType), &item)); \
    CHK_OFAIL(add_weakref_to_list(&typeWeakRefList, item, -1, typeId)); \
    CHK_OFAIL(add_metadef_to_list(&typeMetaDefList, id, &set->instanceUID));

#define STRONGOBJREF_DEF(id, name, description, refTypeId) \
    CHK_OFAIL(mxf_avid_create_typedef_strongref(newMetaDictSet, id, name, description, refTypeId, &set)); \
    CHK_OFAIL(mxf_get_item(set, &MXF_ITEM_K(TypeDefinitionStrongObjectReference, ReferencedType), &item)); \
    CHK_OFAIL(add_weakref_to_list(&classWeakRefList, item, -1, refTypeId)); \
    CHK_OFAIL(add_metadef_to_list(&typeMetaDefList, id, &set->instanceUID));

#define WEAKOBJREF_DEF(id, name, description, refTypeId) \
    CHK_OFAIL(mxf_avid_create_typedef_weakref(newMetaDictSet, id, name, description, refTypeId, &set)); \
    CHK_OFAIL(mxf_get_item(set, &MXF_ITEM_K(TypeDefinitionWeakObjectReference, ReferencedType), &item)); \
    CHK_OFAIL(add_weakref_to_list(&classWeakRefList, item, -1, refTypeId)); \
    CHK_OFAIL(add_metadef_to_list(&typeMetaDefList, id, &set->instanceUID));
#define WEAKOBJREF_TARGET_ELEMENT(id) \
    CHK_OFAIL(mxf_avid_add_typedef_weakref_target(set, id));

#define VARARRAY_DEF(id, name, description, typeId) \
    CHK_OFAIL(mxf_avid_create_typedef_vararray(newMetaDictSet, id, name, description, typeId, &set)); \
    CHK_OFAIL(mxf_get_item(set, &MXF_ITEM_K(TypeDefinitionVariableArray, ElementType), &item)); \
    CHK_OFAIL(add_weakref_to_list(&typeWeakRefList, item, -1, typeId)); \
    CHK_OFAIL(add_metadef_to_list(&typeMetaDefList, id, &set->instanceUID));
    

    
#include "mxf_avid_metadictionary_data.h"

    
    
    /* de-reference class and type weak references and replace weak reference value with
    instanceUID of target set */

    mxf_initialise_list_iter(&iter, &classWeakRefList);
    while (mxf_next_list_iter_element(&iter))
    {
        WeakRefData* data = (WeakRefData*)mxf_get_iter_element(&iter);
        
        CHK_OFAIL(find_weakref_target_instance_uid(&classMetaDefList, &data->targetIdentification, &targetInstanceUID));
        
        if (data->arrayIndex >= 0)
        {
            mxf_set_uuid(&targetInstanceUID, get_array_element(data->item, data->arrayIndex));
        }
        else
        {
            mxf_set_uuid(&targetInstanceUID, data->item->value);
        }
    }
    
    mxf_initialise_list_iter(&iter, &typeWeakRefList);
    while (mxf_next_list_iter_element(&iter))
    {
        WeakRefData* data = (WeakRefData*)mxf_get_iter_element(&iter);
        
        CHK_OFAIL(find_weakref_target_instance_uid(&typeMetaDefList, &data->targetIdentification, &targetInstanceUID));
        
        if (data->arrayIndex >= 0)
        {
            mxf_set_uuid(&targetInstanceUID, get_array_element(data->item, data->arrayIndex));
        }
        else
        {
            mxf_set_uuid(&targetInstanceUID, data->item->value);
        }
    }
    


    mxf_clear_list(&classMetaDefList);
    mxf_clear_list(&typeMetaDefList);
    mxf_clear_list(&classWeakRefList);
    mxf_clear_list(&typeWeakRefList);

    *metaDictSet = newMetaDictSet;
    return 1;
    
fail:
    if (newMetaDictSet != NULL)
    {
        mxf_remove_set(headerMetadata, newMetaDictSet);
        mxf_free_set(&newMetaDictSet);
    }

    mxf_clear_list(&classMetaDefList);
    mxf_clear_list(&typeMetaDefList);
    mxf_clear_list(&classWeakRefList);
    mxf_clear_list(&typeWeakRefList);

    return 0;
}


