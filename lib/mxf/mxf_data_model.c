/*
 * $Id: mxf_data_model.c,v 1.1 2006/12/20 15:40:27 john_f Exp $
 *
 * MXF header metadata data model
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

#include <mxf/mxf.h>


static const MXFItemType g_builtinTypes[] = 
{
    /* basic */
    {MXF_BASIC_TYPE_CAT, MXF_INT8_TYPE, "Int8", 
        .info.basic = { 1 } },
    {MXF_BASIC_TYPE_CAT, MXF_INT16_TYPE, "Int16", 
        .info.basic = { 2 } },
    {MXF_BASIC_TYPE_CAT, MXF_INT32_TYPE, "Int32", 
        .info.basic = { 3 } },
    {MXF_BASIC_TYPE_CAT, MXF_INT64_TYPE, "Int64", 
        .info.basic = { 4 } },
    {MXF_BASIC_TYPE_CAT, MXF_UINT8_TYPE, "UInt8", 
        .info.basic = { 1 } },
    {MXF_BASIC_TYPE_CAT, MXF_UINT16_TYPE, "UInt16", 
        .info.basic = { 2 } },
    {MXF_BASIC_TYPE_CAT, MXF_UINT32_TYPE, "UInt32", 
        .info.basic = { 3 } },
    {MXF_BASIC_TYPE_CAT, MXF_UINT64_TYPE, "UInt64", 
        .info.basic = { 4 } },
    {MXF_BASIC_TYPE_CAT, MXF_RAW_TYPE, "Raw", 
        .info.basic = { 0 } },

    /* array */
    {MXF_ARRAY_TYPE_CAT, MXF_UTF16STRING_TYPE, "UTF16String", 
        .info.array = { MXF_UTF16_TYPE, 0 } },
    {MXF_ARRAY_TYPE_CAT, MXF_INT32ARRAY_TYPE, "Int32Array",
        .info.array = { MXF_INT32_TYPE, 0 } },
    {MXF_ARRAY_TYPE_CAT, MXF_UINT32ARRAY_TYPE, "UInt32Array",
        .info.array = { MXF_UINT32_TYPE, 0 } },
    {MXF_ARRAY_TYPE_CAT, MXF_INT64ARRAY_TYPE, "Int64Array",
        .info.array = { MXF_INT64_TYPE, 0 } },
    {MXF_ARRAY_TYPE_CAT, MXF_UINT8ARRAY_TYPE, "UInt8Array",
        .info.array = { MXF_UINT8_TYPE, 0 } },
    {MXF_ARRAY_TYPE_CAT, MXF_ISO7STRING_TYPE, "ISO7String",
        .info.array = { MXF_ISO7_TYPE, 0 } },
    {MXF_ARRAY_TYPE_CAT, MXF_INT32BATCH_TYPE, "Int32Batch",
        .info.array = { MXF_INT32_TYPE, 0 } },
    {MXF_ARRAY_TYPE_CAT, MXF_UINT32BATCH_TYPE, "UInt32Batch",
        .info.array = { MXF_UINT32_TYPE, 0 } },
    {MXF_ARRAY_TYPE_CAT, MXF_AUIDARRAY_TYPE, "AUIDArray",
        .info.array = { MXF_AUID_TYPE, 0 } },
    {MXF_ARRAY_TYPE_CAT, MXF_ULBATCH_TYPE, "ULBatch",
        .info.array = { MXF_UL_TYPE, 0 } },
    {MXF_ARRAY_TYPE_CAT, MXF_STRONGREFARRAY_TYPE, "StrongRefArray",
        .info.array = { MXF_STRONGREF_TYPE, 0 } },
    {MXF_ARRAY_TYPE_CAT, MXF_STRONGREFBATCH_TYPE, "StrongRefBatch",
        .info.array = { MXF_STRONGREF_TYPE, 0 } },
    {MXF_ARRAY_TYPE_CAT, MXF_WEAKREFARRAY_TYPE, "WeakRefArray",
        .info.array = { MXF_WEAKREF_TYPE, 0 } },
    {MXF_ARRAY_TYPE_CAT, MXF_WEAKREFBATCH_TYPE, "WeakRefBatch",
        .info.array = { MXF_WEAKREF_TYPE, 0 } },
    {MXF_ARRAY_TYPE_CAT, MXF_RATIONALARRAY_TYPE, "RationalArray",
        .info.array = { MXF_RATIONAL_TYPE, 0 } },
    {MXF_ARRAY_TYPE_CAT, MXF_RGBALAYOUT_TYPE, "RGBALayout",
        .info.array = { MXF_RGBALAYOUTCOMPONENT_TYPE, 0 } },

    /* compound */
    {MXF_COMPOUND_TYPE_CAT, MXF_RATIONAL_TYPE, "Rational", 
        .info.compound = {{
            {"Numerator", MXF_INT32_TYPE}, 
            {"Denominator", MXF_INT32_TYPE} 
        }} },
    {MXF_COMPOUND_TYPE_CAT, MXF_TIMESTAMP_TYPE, "Timestamp", 
        .info.compound = {{
            {"Year", MXF_UINT16_TYPE}, 
            {"Month", MXF_UINT8_TYPE} ,
            {"Day", MXF_UINT8_TYPE}, 
            {"Hours", MXF_UINT8_TYPE}, 
            {"Minutes", MXF_UINT8_TYPE}, 
            {"Seconds", MXF_UINT8_TYPE},
            {"QMSec", MXF_UINT8_TYPE} 
        }} },
    {MXF_COMPOUND_TYPE_CAT, MXF_PRODUCTVERSION_TYPE, "ProductVersion", 
        .info.compound = {{
            {"Major", MXF_UINT16_TYPE}, 
            {"Minor", MXF_UINT16_TYPE}, 
            {"Patch", MXF_UINT16_TYPE}, 
            {"Build", MXF_UINT16_TYPE}, 
            {"Release", MXF_UINT16_TYPE} 
        }} },
    {MXF_COMPOUND_TYPE_CAT, MXF_INDIRECT_TYPE, "Indirect", 
        .info.compound = {{
            {"Type", MXF_UL_TYPE}, 
            {"Value", MXF_UINT8ARRAY_TYPE} 
        }} },
    {MXF_COMPOUND_TYPE_CAT, MXF_RGBALAYOUTCOMPONENT_TYPE, "RGBALayoutComponent", 
        .info.compound = {{
            {"Code", MXF_RGBACODE_TYPE}, 
            {"Depth", MXF_UINT8_TYPE} 
        }} },

    /* interpreted */
    {MXF_INTERPRET_TYPE_CAT, MXF_VERSIONTYPE_TYPE, "VersionType", 
        .info.interpret = { MXF_UINT16_TYPE, 0 } },
    {MXF_INTERPRET_TYPE_CAT, MXF_UTF16_TYPE, "UTF16", 
        .info.interpret = { MXF_UINT16_TYPE, 0 } },
    {MXF_INTERPRET_TYPE_CAT, MXF_BOOLEAN_TYPE, "Boolean", 
        .info.interpret = { MXF_UINT8_TYPE, 0 } },
    {MXF_INTERPRET_TYPE_CAT, MXF_ISO7_TYPE, "ISO7", 
        .info.interpret = { MXF_UINT8_TYPE, 0 } },
    {MXF_INTERPRET_TYPE_CAT, MXF_LENGTH_TYPE, "Length", 
        .info.interpret = { MXF_INT64_TYPE, 0 } },
    {MXF_INTERPRET_TYPE_CAT, MXF_POSITION_TYPE, "Position", 
        .info.interpret = { MXF_INT64_TYPE, 0 } },
    {MXF_INTERPRET_TYPE_CAT, MXF_RGBACODE_TYPE, "RGBACode", 
        .info.interpret = { MXF_UINT8_TYPE, 0 } },
    {MXF_INTERPRET_TYPE_CAT, MXF_STREAM_TYPE, "Stream", 
        .info.interpret = { MXF_RAW_TYPE, 0 } },
    {MXF_INTERPRET_TYPE_CAT, MXF_DATAVALUE_TYPE, "DataValue", 
        .info.interpret = { MXF_UINT8ARRAY_TYPE, 0 } },
    {MXF_INTERPRET_TYPE_CAT, MXF_IDENTIFIER_TYPE, "Identifier", 
        .info.interpret = { MXF_UINT8ARRAY_TYPE, 0 } },
    {MXF_INTERPRET_TYPE_CAT, MXF_OPAQUE_TYPE, "Opaque", 
        .info.interpret = { MXF_UINT8ARRAY_TYPE, 0 } },
    {MXF_INTERPRET_TYPE_CAT, MXF_UMID_TYPE, "UMID", 
        .info.interpret = { MXF_IDENTIFIER_TYPE, 32 } },
    {MXF_INTERPRET_TYPE_CAT, MXF_UID_TYPE, "UID", 
        .info.interpret = { MXF_IDENTIFIER_TYPE, 16 } },
    {MXF_INTERPRET_TYPE_CAT, MXF_UL_TYPE, "UL", 
        .info.interpret = { MXF_IDENTIFIER_TYPE, 16 } },
    {MXF_INTERPRET_TYPE_CAT, MXF_UUID_TYPE, "UUID", 
        .info.interpret = { MXF_IDENTIFIER_TYPE, 16 } },
    {MXF_INTERPRET_TYPE_CAT, MXF_AUID_TYPE, "AUID", 
        .info.interpret = { MXF_UL_TYPE, 16 } },
    {MXF_INTERPRET_TYPE_CAT, MXF_PACKAGEID_TYPE, "PackageID", 
        .info.interpret = { MXF_UMID_TYPE, 32 } },
    {MXF_INTERPRET_TYPE_CAT, MXF_STRONGREF_TYPE, "StrongRef", 
        .info.interpret = { MXF_UUID_TYPE, 0 } },
    {MXF_INTERPRET_TYPE_CAT, MXF_WEAKREF_TYPE, "WeakRef", 
        .info.interpret = { MXF_UUID_TYPE, 0 } },
    {MXF_INTERPRET_TYPE_CAT, MXF_ORIENTATION_TYPE, "Orientation", 
        .info.interpret = { MXF_UINT8_TYPE, 0 } },
    
};


static void clear_type(MXFItemType* type)
{
    size_t i;
    
    if (type == NULL)
    {
        return;
    }
    
    if (type->typeId != 0)
    {
        SAFE_FREE(&type->name);
        if (type->category == MXF_COMPOUND_TYPE_CAT)
        {
            for (i = 0; i < sizeof(type->info.compound.members) / sizeof(MXFCompoundTypeMemberInfo); i++)
            {
                SAFE_FREE(&type->info.compound.members[i].name);
            }
        }
    }
    memset(type, 0, sizeof(MXFItemType));
}

static void free_item_def(MXFItemDef** itemDef)
{
    if (*itemDef == NULL)
    {
        return;
    }
    
    SAFE_FREE(&(*itemDef)->name);
    SAFE_FREE(itemDef);
}

static void free_set_def(MXFSetDef** setDef)
{
    if (*setDef == NULL)
    {
        return;
    }
    
    SAFE_FREE(&(*setDef)->name);
    SAFE_FREE(setDef);
}

static void free_item_def_in_list(void* data)
{
    MXFItemDef* itemDef;
    
    if (data == NULL)
    {
        return;
    }
    
    itemDef = (MXFItemDef*)data;
    free_item_def(&itemDef);
}

static void free_set_def_in_list(void* data)
{
    MXFSetDef* setDef;
    
    if (data == NULL)
    {
        return;
    }
    
    setDef = (MXFSetDef*)data;
    mxf_clear_list(&setDef->itemDefs);
    free_set_def(&setDef);
}

static int set_def_eq(void* data, void* info)
{
    assert(data != NULL && info != NULL);
    
    return mxf_equals_key((mxfKey*)info, &((MXFSetDef*)data)->key);
}

static int item_def_eq(void* data, void* info)
{
    assert(data != NULL && info != NULL);
    
    return mxf_equals_key((mxfKey*)info, &((MXFItemDef*)data)->key);
}

static int add_set_def(MXFDataModel* dataModel, MXFSetDef* setDef)
{
    assert(setDef != NULL);
    
    CHK_ORET(mxf_append_list_element(&dataModel->setDefs, (void*)setDef));
    
    return 1;
}

static int add_item_def(MXFDataModel* dataModel, MXFItemDef* itemDef)
{
    assert(itemDef != NULL);
    
    CHK_ORET(mxf_append_list_element(&dataModel->itemDefs, (void*)itemDef));
    
    return 1;
}

static unsigned int get_type_id(MXFDataModel* dataModel)
{
    size_t i;
    unsigned int lastTypeId;
    unsigned int typeId = 0;
    
    if (dataModel->lastTypeId == 0 ||
        dataModel->lastTypeId >= sizeof(dataModel->types) / sizeof(MXFItemType))
    {
        lastTypeId = MXF_EXTENSION_TYPE;
    }
    else
    {
        lastTypeId = dataModel->lastTypeId;
    }
    
    /* try from the last type id to the end of the list */
    for (i = lastTypeId; i < sizeof(dataModel->types) / sizeof(MXFItemType); i++)
    {
        if (dataModel->types[i].typeId == 0)
        {
            typeId = i;
            break;
        }
    }
    
    if (typeId == 0 && lastTypeId > MXF_EXTENSION_TYPE)
    {
        /* try from MXF_EXTENSION_TYPE to lastTypeId */
        for (i = MXF_EXTENSION_TYPE; i < lastTypeId; i++)
        {
            if (dataModel->types[i].typeId == 0)
            {
                typeId = i;
                break;
            }
        }
    }
    
    return typeId;
}



#define MXF_SET_DEFINITION(parentName, name, label) \
    CHK_OFAIL(mxf_register_set_def(newDataModel, #name, &MXF_SET_K(parentName), &MXF_SET_K(name)));
    
#define MXF_ITEM_DEFINITION(setName, name, label, tag, typeId) \
    CHK_OFAIL(mxf_register_item_def(newDataModel, #name, &MXF_SET_K(setName), &MXF_ITEM_K(setName, name), tag, typeId));
    

int mxf_load_data_model(MXFDataModel** dataModel)
{
    MXFDataModel* newDataModel;
    size_t i;
    
    CHK_MALLOC_ORET(newDataModel, MXFDataModel);
    memset(newDataModel, 0, sizeof(MXFDataModel));
    mxf_initialise_list(&newDataModel->itemDefs, free_item_def_in_list); 
    mxf_initialise_list(&newDataModel->setDefs, free_set_def_in_list); 
    
#include <mxf/mxf_baseline_data_model.h>

    for (i = 0; i < sizeof(g_builtinTypes) / sizeof(MXFItemType); i++)
    {
        const MXFItemType* type = &g_builtinTypes[i];
        
        switch (type->category)
        {
            case MXF_BASIC_TYPE_CAT: 
                CHK_OFAIL(mxf_register_basic_type(newDataModel, type->name, type->typeId, 
                    type->info.basic.size));
                break;
            case MXF_ARRAY_TYPE_CAT: 
                CHK_OFAIL(mxf_register_array_type(newDataModel, type->name, type->typeId, 
                    type->info.array.elementTypeId, type->info.array.fixedSize));
                break;
            case MXF_COMPOUND_TYPE_CAT: 
                CHK_OFAIL(mxf_register_compound_type(newDataModel, type->name, type->typeId, 
                    type->info.compound.members));
                break;
            case MXF_INTERPRET_TYPE_CAT: 
                CHK_OFAIL(mxf_register_interpret_type(newDataModel, type->name, type->typeId, 
                    type->info.interpret.typeId, type->info.interpret.fixedArraySize));
                break;
            default: 
                mxf_log(MXF_ELOG, "Unknown type category %d" 
                    LOG_LOC_FORMAT, type->category, LOG_LOC_PARAMS); 
                goto fail;
        }
    }

    *dataModel = newDataModel;
    return 1;
    
fail:
    mxf_free_data_model(&newDataModel);
    return 0;
}

#undef MXF_SET_DEFINITION
#undef MXF_ITEM_DEFINITION


void mxf_free_data_model(MXFDataModel** dataModel)
{
    size_t i;

    if (*dataModel == NULL)
    {
        return;
    }
    
    mxf_clear_list(&(*dataModel)->setDefs);
    mxf_clear_list(&(*dataModel)->itemDefs);
    
    for (i = 0; i < sizeof((*dataModel)->types) / sizeof(MXFItemType); i++)
    {
        clear_type(&(*dataModel)->types[i]);
    }
    
    SAFE_FREE(dataModel);
}



int mxf_register_set_def(MXFDataModel* dataModel, const char* name, const mxfKey* parentKey, 
    const mxfKey* key)
{
    MXFSetDef* newSetDef = NULL;
    
    CHK_MALLOC_ORET(newSetDef, MXFSetDef);
    memset(newSetDef, 0, sizeof(MXFSetDef));
    if (name != NULL)
    {
        CHK_MALLOC_ARRAY_OFAIL(newSetDef->name, char, strlen(name) + 1); 
        strcpy(newSetDef->name, name);
    }
    newSetDef->parentSetDefKey = *parentKey;
    newSetDef->key = *key;
    mxf_initialise_list(&newSetDef->itemDefs, NULL);
    
    CHK_OFAIL(add_set_def(dataModel, newSetDef));
    
    return 1;
    
fail:
    free_set_def(&newSetDef);
    return 0;
}

int mxf_register_item_def(MXFDataModel* dataModel, const char* name, const mxfKey* setKey, 
    const mxfKey* key, mxfLocalTag tag, unsigned int typeId)
{
    MXFItemDef* newItemDef = NULL;
    
    CHK_MALLOC_ORET(newItemDef, MXFItemDef);
    memset(newItemDef, 0, sizeof(MXFItemDef));
    if (name != NULL)
    {
        CHK_MALLOC_ARRAY_OFAIL(newItemDef->name, char, strlen(name) + 1); 
        strcpy(newItemDef->name, name);
    }
    newItemDef->setDefKey = *setKey;
    newItemDef->key = *key;
    newItemDef->localTag = tag;
    newItemDef->typeId = typeId;
    
    CHK_OFAIL(add_item_def(dataModel, newItemDef));
    
    return 1;
    
fail:
    free_item_def(&newItemDef);
    return 0;
}


unsigned int mxf_register_basic_type(MXFDataModel* dataModel, const char* name, unsigned int typeId, unsigned int size)
{
    MXFItemType* type;
    
    /* basic types can only be built-in */
    CHK_ORET(typeId > 0 && typeId < MXF_EXTENSION_TYPE);
    
    /* check the type id is valid and free */
    CHK_ORET(typeId < sizeof(dataModel->types) / sizeof(MXFItemType) && 
        dataModel->types[typeId].typeId == 0);
    
    type = &dataModel->types[typeId];
    type->typeId = typeId; /* set first to indicate type is present */
    type->category = MXF_BASIC_TYPE_CAT;
    if (name != NULL)
    {
        CHK_MALLOC_ARRAY_OFAIL(type->name, char, strlen(name) + 1); 
        strcpy(type->name, name);
    }
    type->info.basic.size = size;
    
    return typeId;

fail:  
    clear_type(type);
    return 0;
}

unsigned int mxf_register_array_type(MXFDataModel* dataModel, const char* name, unsigned int typeId, unsigned int elementTypeId, unsigned int fixedSize)
{
    unsigned int retTypeId;
    MXFItemType* type;
    
    if (typeId <= 0)
    {
        retTypeId = get_type_id(dataModel);
    }
    else
    {
        /* check the type id is valid and free */
        CHK_ORET(typeId < sizeof(dataModel->types) / sizeof(MXFItemType) && 
            dataModel->types[typeId].typeId == 0);
        retTypeId = typeId;
    }
    
    type = &dataModel->types[retTypeId];
    type->typeId = retTypeId; /* set first to indicate type is present */
    type->category = MXF_ARRAY_TYPE_CAT;
    if (name != NULL)
    {
        CHK_MALLOC_ARRAY_OFAIL(type->name, char, strlen(name) + 1); 
        strcpy(type->name, name);
    }
    type->info.array.elementTypeId = elementTypeId;
    type->info.array.fixedSize = fixedSize;
    
    return retTypeId;

fail:    
    clear_type(type);
    return 0;
}

unsigned int mxf_register_compound_type(MXFDataModel* dataModel, const char* name, unsigned int typeId, const MXFCompoundTypeMemberInfo* members)
{
    unsigned int retTypeId;
    MXFItemType* type = NULL;
    size_t i;
    
    if (typeId == 0)
    {
        retTypeId = get_type_id(dataModel);
    }
    else
    {
        /* check the type id is valid and free */
        CHK_ORET(typeId < sizeof(dataModel->types) / sizeof(MXFItemType) && 
            dataModel->types[typeId].typeId == 0);
        retTypeId = typeId;
    }
    
    type = &dataModel->types[retTypeId];
    type->typeId = retTypeId; /* set first to indicate type is present */
    type->category = MXF_COMPOUND_TYPE_CAT;
    if (name != NULL)
    {
        CHK_MALLOC_ARRAY_OFAIL(type->name, char, strlen(name) + 1); 
        strcpy(type->name, name);
    }
    for (i = 0; i < sizeof(type->info.compound.members) / sizeof(MXFCompoundTypeMemberInfo); i++)
    {
        if (members[i].typeId == 0)
        {
            /* members array is terminated by typeId == 0 */
            break;
        }
        else
        {
            if (members[i].name != NULL)
            {
                CHK_MALLOC_ARRAY_OFAIL(type->info.compound.members[i].name, char, strlen(members[i].name) + 1); 
                strcpy(type->info.compound.members[i].name, members[i].name);
            }
            type->info.compound.members[i].typeId = members[i].typeId;
        }
    }
    
    if (i == sizeof(type->info.compound.members) / sizeof(MXFCompoundTypeMemberInfo))
    {
        mxf_log(MXF_ELOG, "Number of compound item type members exceeds hardcoded maximum %d" 
            LOG_LOC_FORMAT, sizeof(type->info.compound.members) / sizeof(MXFCompoundTypeMemberInfo), LOG_LOC_PARAMS);
        return 0;
    }
    
    return retTypeId;
    
fail:
    clear_type(type);
    return 0;
}

unsigned int mxf_register_interpret_type(MXFDataModel* dataModel, const char* name, unsigned int typeId, 
    unsigned int interpretedTypeId, unsigned int fixedArraySize)
{
    unsigned int retTypeId;
    MXFItemType* type;
    
    if (typeId == 0)
    {
        retTypeId = get_type_id(dataModel);
    }
    else
    {
        /* check the type id is valid and free */
        CHK_ORET(typeId < sizeof(dataModel->types) / sizeof(MXFItemType) && 
            dataModel->types[typeId].typeId == 0);
        retTypeId = typeId;
    }
    
    type = &dataModel->types[retTypeId];
    type->typeId = retTypeId; /* set first to indicate type is present */
    type->category = MXF_INTERPRET_TYPE_CAT;
    if (name != NULL)
    {
        CHK_MALLOC_ARRAY_OFAIL(type->name, char, strlen(name) + 1); 
        strcpy(type->name, name);
    }
    type->info.interpret.typeId = interpretedTypeId;
    type->info.interpret.fixedArraySize = fixedArraySize;
    
    return retTypeId;
    
fail:    
    clear_type(type);
    return 0;
}


int mxf_finalise_data_model(MXFDataModel* dataModel)
{
    MXFListIterator iter;
    MXFItemDef* itemDef;
    MXFSetDef* setDef;

    /* reset set defs and set the parent set def if the parent set def key != g_Null_Key */
    mxf_initialise_list_iter(&iter, &dataModel->setDefs);
    while (mxf_next_list_iter_element(&iter))
    {
        setDef = (MXFSetDef*)mxf_get_iter_element(&iter);
        mxf_clear_list(&setDef->itemDefs);
        setDef->parentSetDef = NULL;

        if (!mxf_equals_key(&setDef->parentSetDefKey, &g_Null_Key))
        {
            CHK_ORET(mxf_find_set_def(dataModel, &setDef->parentSetDefKey, &setDef->parentSetDef));
        }
    }
    
    /* add item defs to owner set def */
    mxf_initialise_list_iter(&iter, &dataModel->itemDefs);
    while (mxf_next_list_iter_element(&iter))
    {
        itemDef = (MXFItemDef*)mxf_get_iter_element(&iter);

        CHK_ORET(mxf_find_set_def(dataModel, &itemDef->setDefKey, &setDef));
        CHK_ORET(mxf_append_list_element(&setDef->itemDefs, (void*)itemDef));
    }
    
    return 1;
}

int mxf_check_data_model(MXFDataModel* dataModel)
{
    MXFListIterator iter1;
    MXFListIterator iter2;
    MXFSetDef* setDef1;
    MXFSetDef* setDef2;
    MXFItemDef* itemDef1;
    MXFItemDef* itemDef2;
    long listIndex;

    
    /* check that the set defs are unique */
    listIndex = 0;
    mxf_initialise_list_iter(&iter1, &dataModel->setDefs);
    while (mxf_next_list_iter_element(&iter1))
    {
        setDef1 = (MXFSetDef*)mxf_get_iter_element(&iter1);

        /* check with set defs with higher index in list */        
        mxf_initialise_list_iter_at(&iter2, &dataModel->setDefs, listIndex + 1);
        while (mxf_next_list_iter_element(&iter2))
        {
            setDef2 = (MXFSetDef*)mxf_get_iter_element(&iter2);
            if (mxf_equals_key(&setDef1->key, &setDef2->key))
            {
                char keyStr[KEY_STR_SIZE];
                mxf_sprint_key(keyStr, &setDef1->key);
                mxf_log(MXF_WLOG, "Duplicate set def found. Key = %s" 
                    LOG_LOC_FORMAT, keyStr, LOG_LOC_PARAMS); 
                return 0;
            }
        }
        listIndex++;
    }

    /* check that the item defs are unique (both key and static local tag),
       , that the item def is contained in a set def
       and the item type is known */
    listIndex = 0;
    mxf_initialise_list_iter(&iter1, &dataModel->itemDefs);
    while (mxf_next_list_iter_element(&iter1))
    {
        itemDef1 = (MXFItemDef*)mxf_get_iter_element(&iter1);

        /* check item def is contained in a set def */
        if (mxf_equals_key(&itemDef1->setDefKey, &g_Null_Key))
        {
            char keyStr[KEY_STR_SIZE];
            mxf_sprint_key(keyStr, &itemDef1->key);
            mxf_log(MXF_WLOG, "Found item def not contained in any set def. Key = %s" 
                LOG_LOC_FORMAT, keyStr, LOG_LOC_PARAMS); 
            return 0;
        }
        
        /* check with item defs with higher index in list */        
        mxf_initialise_list_iter_at(&iter2, &dataModel->itemDefs, listIndex + 1);
        while (mxf_next_list_iter_element(&iter2))
        {
            itemDef2 = (MXFItemDef*)mxf_get_iter_element(&iter2);
            if (mxf_equals_key(&itemDef1->key, &itemDef2->key))
            {
                char keyStr[KEY_STR_SIZE];
                mxf_sprint_key(keyStr, &itemDef1->key);
                mxf_log(MXF_WLOG, "Duplicate item def found. Key = %s" 
                    LOG_LOC_FORMAT, keyStr, LOG_LOC_PARAMS); 
                return 0;
            }
            if (itemDef1->localTag != 0 && itemDef1->localTag == itemDef2->localTag)
            {
                char keyStr[KEY_STR_SIZE];
                mxf_sprint_key(keyStr, &itemDef1->key);
                mxf_log(MXF_WLOG, "Duplicate item def local tag found. LocalTag = 0x%04x, Key = %s" 
                    LOG_LOC_FORMAT, itemDef1->localTag, keyStr, LOG_LOC_PARAMS); 
                return 0;
            }
        }
        
        /* check item type is valid and known */
        if (mxf_get_item_def_type(dataModel, itemDef1->typeId) == NULL)
        {
            char keyStr[KEY_STR_SIZE];
            mxf_sprint_key(keyStr, &itemDef1->key);
            mxf_log(MXF_WLOG, "Item def has unknown type (%d). LocalTag = 0x%04x, Key = %s" 
                LOG_LOC_FORMAT, itemDef1->typeId, itemDef1->localTag, keyStr, LOG_LOC_PARAMS); 
            return 0;
        }
        
        listIndex++;
    }
    
    return 1;
}

int mxf_find_set_def(MXFDataModel* dataModel, const mxfKey* key, MXFSetDef** setDef)
{
    void* result;
    
    if ((result = mxf_find_list_element(&dataModel->setDefs, (void*)key, set_def_eq)) != NULL)
    {
        *setDef = (MXFSetDef*)result;
        return 1;
    }

    return 0;
}

int mxf_find_item_def(MXFDataModel* dataModel, const mxfKey* key, MXFItemDef** itemDef)
{
    void* result;
    
    if ((result = mxf_find_list_element(&dataModel->itemDefs, (void*)key, item_def_eq)) != NULL)
    {
        *itemDef = (MXFItemDef*)result;
        return 1;
    }

    return 0;
}

int mxf_find_item_def_in_set_def(const mxfKey* key, const MXFSetDef* setDef, MXFItemDef** itemDef)
{
    void* result;
    
    if ((result = mxf_find_list_element(&setDef->itemDefs, (void*)key, item_def_eq)) != NULL)
    {
        *itemDef = (MXFItemDef*)result;
        return 1;
    }
    
    if (setDef->parentSetDef != NULL)
    {
        return mxf_find_item_def_in_set_def(key, setDef->parentSetDef, itemDef);
    }
    
    return 0;
}


const MXFItemType* mxf_get_item_def_type(MXFDataModel* dataModel, unsigned int typeId)
{
    if (typeId == 0 || typeId >= sizeof(dataModel->types) / sizeof(MXFItemType))
    {
        return NULL;
    }
    if (dataModel->types[typeId].typeId == MXF_UNKNOWN_TYPE)
    {
        return NULL;
    }
    
    return &dataModel->types[typeId];
}



int mxf_is_subclass_of(MXFDataModel* dataModel, const mxfKey* setKey, const mxfKey* parentSetKey)
{
    MXFSetDef* set;

    if (mxf_equals_key(setKey, parentSetKey))
    {
        return 1;
    }
    
    if (!mxf_find_set_def(dataModel, setKey, &set))
    {
        return 0;
    }
    if (mxf_equals_key(setKey, &set->parentSetDefKey))
    {
        return 0;
    }
    return mxf_is_subclass_of(dataModel, &set->parentSetDefKey, parentSetKey);
}

