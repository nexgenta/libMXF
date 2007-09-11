/*
 * $Id: mxf_metadata_int.h,v 1.1 2007/09/11 13:24:54 stuart_hc Exp $
 *
 * Internal MXF metadata header
 *
 * Copyright (C) 2007  Philip de Nier <philipn@users.sourceforge.net>
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
 
#ifndef __MXF_METADATA_INT_H__
#define __MXF_METADATA_INT_H__


#define GET_OPT_SIMPLE_VALUE(set, name, SetName, ItemName, type) \
    if (mxf_have_item(set, &MXF_ITEM_K(SetName, ItemName))) \
    { \
        CHK_ORET(mxf_get_## type ## _item(set, &MXF_ITEM_K(SetName, ItemName), &name)); \
        name##_isPresent = 1; \
    } \
    else \
    { \
        name##_isPresent = 0; \
    }

#define GET_OPT_STRING_VALUE(set, name, SetName, ItemName) \
    if (mxf_have_item(set, &MXF_ITEM_K(SetName, ItemName))) \
    { \
        uint16_t size; \
        CHK_ORET(mxf_get_utf16string_item_size(set, &MXF_ITEM_K(SetName, ItemName), &size)); \
        CHK_MALLOC_ARRAY_ORET(name, mxfUTF16Char, size); \
        CHK_ORET(mxf_get_utf16string_item(set, &MXF_ITEM_K(SetName, ItemName), name)); \
        name##_isPresent = 1; \
    } \
    else \
    { \
        name##_isPresent = 0; \
    }

#define GET_OPT_ARRAY_VALUE(set, name, SetName, ItemName, type, maxElements) \
    if (mxf_have_item(set, &MXF_ITEM_K(SetName, ItemName))) \
    { \
        uint32_t numElements; \
        uint32_t i; \
        CHK_ORET(mxf_get_array_item_count(set, &MXF_ITEM_K(SetName, ItemName), &numElements)); \
        CHK_ORET(numElements <= maxElements); \
        for (i = 0; i < numElements; i++) \
        { \
            uint8_t* data; \
            CHK_ORET(mxf_get_array_item_element(set, &MXF_ITEM_K(SetName, ItemName), i, &data)); \
            mxf_get_##type(data, &name[i]); \
        } \
        name##_size = numElements; \
        name##_isPresent = 1; \
    } \
    else \
    { \
        name##_size = 0; \
        name##_isPresent = 0; \
    }

#define GET_VARARRAY_VALUE(set, name, SetName, ItemName, type, functype) \
    if (mxf_have_item(set, &MXF_ITEM_K(SetName, ItemName))) \
    { \
        uint32_t numElements; \
        uint32_t i; \
        CHK_ORET(mxf_get_array_item_count(set, &MXF_ITEM_K(SetName, ItemName), &numElements)); \
        CHK_MALLOC_ARRAY_ORET(name, type, numElements); \
        for (i = 0; i < numElements; i++) \
        { \
            uint8_t* data; \
            CHK_ORET(mxf_get_array_item_element(set, &MXF_ITEM_K(SetName, ItemName), i, &data)); \
            mxf_get_##functype(data, &name[i]); \
        } \
        name##_size = numElements; \
    } \

#define GET_SIMPLE_VALUE(set, name, SetName, ItemName, type) \
    if (mxf_have_item(set, &MXF_ITEM_K(SetName, ItemName))) \
    { \
        CHK_ORET(mxf_get_## type ## _item(set, &MXF_ITEM_K(SetName, ItemName), &name)) \
    }

    
    
    
#define SET_SIMPLE_VALUE(set, name, SetName, ItemName, type) \
    CHK_ORET(mxf_set_## type ## _item(set, &MXF_ITEM_K(SetName, ItemName), name))

#define SET_OPT_SIMPLE_VALUE(set, name, SetName, ItemName, type) \
    if (name##_isPresent) \
    { \
        SET_SIMPLE_VALUE(set, name, SetName, ItemName, type); \
    }

#define SET_OPT_STRING_VALUE(set, name, SetName, ItemName) \
    if (name##_isPresent) \
    { \
        CHK_ORET(mxf_set_utf16string_item(set, &MXF_ITEM_K(SetName, ItemName), name)); \
    }

#define SET_VARARRAY_VALUE(set, name, SetName, ItemName, type, typefunc) \
    uint8_t* data; \
    uint32_t i; \
    CHK_ORET(mxf_alloc_array_item_elements(set, &MXF_ITEM_K(SetName, ItemName), \
        type##_extlen, name ##_size, &data)); \
    for (i = 0; i < name ##_size; i++) \
    { \
        mxf_set_##typefunc(name[i], data); \
        data += elementLen; \
    }
    
#define SET_PVARARRAY_VALUE(set, name, SetName, ItemName, type, typefunc) \
    uint8_t* data; \
    uint32_t i; \
    CHK_ORET(mxf_alloc_array_item_elements(set, &MXF_ITEM_K(SetName, ItemName), \
        type##_extlen, name ##_size, &data)); \
    for (i = 0; i < name ##_size; i++) \
    { \
        mxf_set_##typefunc(&name[i], data); \
        data += type##_extlen; \
    }
    
#define SET_ARRAY_VALUE(set, name, SetName, ItemName, type, elementLen) \
    uint8_t* data; \
    uint32_t i; \
    CHK_ORET(mxf_alloc_array_item_elements(set, &MXF_ITEM_K(SetName, ItemName), \
        elementLen, name ##_size, &data)); \
    for (i = 0; i < name ##_size; i++) \
    { \
        mxf_set_##type(name[i], data); \
        data += elementLen; \
    }

#define SET_OPT_ARRAY_VALUE(set, name, SetName, ItemName, type, elementLen) \
    if (name##_isPresent) \
    { \
        SET_ARRAY_VALUE(set, name, SetName, ItemName, type, elementLen); \
    }

    
#define SET_PSIMPLE_VALUE(set, name, SetName, ItemName, type) \
    CHK_ORET(mxf_set_## type ## _item(set, &MXF_ITEM_K(SetName, ItemName), &name))

#define SET_OPT_PSIMPLE_VALUE(set, name, SetName, ItemName, type) \
    if (name##_isPresent) \
    { \
        SET_PSIMPLE_VALUE(set, name, SetName, ItemName, type); \
    }

    
#define CLEAR_ARRAY_VALUE(name) \
    SAFE_FREE(&name); \
    name##_size = 0;
    
#define CLEAR_OPT_ARRAY_VALUE(name) \
    if (name##_isPresent) \
    { \
        CLEAR_ARRAY_VALUE(name); \
        name##_isPresent = 0; \
    }
    
#define CLEAR_VARARRAY_VALUE(name) \
    CLEAR_ARRAY_VALUE(name);
    
#define CLEAR_OPT_VARARRAY_VALUE(name) \
    CLEAR_OPT_ARRAY_VALUE(name);
    
#define CLEAR_STRING_VALUE(name) \
    SAFE_FREE(&name);
    
#define CLEAR_OPT_STRING_VALUE(name) \
    if (name##_isPresent) \
    { \
        CLEAR_STRING_VALUE(name); \
        name##_isPresent = 0; \
    }
    


#endif


