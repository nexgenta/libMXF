/*
 * $Id: mxf_labels_and_keys.c,v 1.2 2007/09/11 13:24:55 stuart_hc Exp $
 *
 * MXF labels, keys, track numbers, etc.
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

#include <mxf/mxf.h>


static const mxfUL g_opAtomPrefix = MXF_ATOM_OP_L(0);
static const mxfUL g_op1APrefix = MXF_1A_OP_L(0);
    


int mxf_is_picture(const mxfUL* label)
{
    return memcmp(label, &MXF_DDEF_L(Picture), sizeof(mxfUL)) == 0 ||
        memcmp(label, &MXF_DDEF_L(LegacyPicture), sizeof(mxfUL)) == 0;
}

int mxf_is_sound(const mxfUL* label)
{
    return memcmp(label, &MXF_DDEF_L(Sound), sizeof(mxfUL)) == 0 ||
        memcmp(label, &MXF_DDEF_L(LegacySound), sizeof(mxfUL)) == 0;
}

int mxf_is_timecode(const mxfUL* label)
{
    return memcmp(label, &MXF_DDEF_L(Timecode), sizeof(mxfUL)) == 0 ||
        memcmp(label, &MXF_DDEF_L(LegacyTimecode), sizeof(mxfUL)) == 0;
}

int mxf_is_data(const mxfUL* label)
{
    return memcmp(label, &MXF_DDEF_L(Data), sizeof(mxfUL)) == 0;
}
    
int mxf_is_descriptive_metadata(const mxfUL* label)
{
    return memcmp(label, &MXF_DDEF_L(DescriptiveMetadata), sizeof(mxfUL)) == 0;
}


void mxf_complete_essence_element_key(mxfKey* key, uint8_t count, uint8_t type, uint8_t num)
{
    key->octet13 = count;
    key->octet14 = type;
    key->octet15 = num;
}

void mxf_complete_essence_element_track_num(uint32_t* trackNum, uint8_t count, uint8_t type, uint8_t num)
{
    *trackNum &= 0xFF000000;
    *trackNum |= ((uint32_t)count) << 16;
    *trackNum |= ((uint32_t)type) << 8;
    *trackNum |= (uint32_t)(num);
}


int is_op_atom(const mxfUL* label)
{
    return memcmp(&g_opAtomPrefix, label, 13) == 0;
}

int is_op_1a(const mxfUL* label)
{
    return memcmp(&g_op1APrefix, label, 13) == 0;
}


