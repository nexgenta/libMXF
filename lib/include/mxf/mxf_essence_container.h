/*
 * $Id: mxf_essence_container.h,v 1.2 2007/09/11 13:24:54 stuart_hc Exp $
 *
 * MXF file body essence container functions
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
 
#ifndef __MXF_ESSENCE_CONTAINER_H__
#define __MXF_ESSENCE_CONTAINER_H__


#ifdef __cplusplus
extern "C" 
{
#endif


typedef struct _MXFEssenceElement
{
    mxfKey key;
    uint8_t llen;
    uint64_t startFilePos;
    uint64_t totalLen;
    uint64_t currentFilePos;
} MXFEssenceElement;


int mxf_is_gc_essence_element(const mxfKey* key);

int mxf_open_essence_element_write(MXFFile* mxfFile, const mxfKey* key, uint8_t llen, 
    uint64_t len, MXFEssenceElement** essenceElement);
int mxf_write_essence_element_data(MXFFile* mxfFile, MXFEssenceElement* essenceElement,
    uint8_t* data, uint32_t len);
int mxf_finalize_essence_element_write(MXFFile* mxfFile, MXFEssenceElement* essenceElement);

int mxf_open_essence_element_read(MXFFile* mxfFile, const mxfKey* key, uint8_t llen, uint64_t len, 
    MXFEssenceElement** essenceElement);
int mxf_read_essence_element_data(MXFFile* mxfFile, MXFEssenceElement* essenceElement,
    uint32_t len, uint8_t* data, uint32_t* numRead);

void mxf_close_essence_element(MXFEssenceElement** essenceElement);

uint64_t mxf_get_essence_element_size(MXFEssenceElement* essenceElement);

uint32_t mxf_get_track_number(const mxfKey* essenceElementKey);
uint8_t mxf_get_essence_element_item_type(uint32_t trackNumber);
uint8_t mxf_get_essence_element_count(uint32_t trackNumber);
uint8_t mxf_get_essence_element_type(uint32_t trackNumber);
uint8_t mxf_get_essence_element_number(uint32_t trackNumber);


#ifdef __cplusplus
}
#endif


#endif


