/*
 * $Id: mxf_avid_labels_and_keys.h,v 1.2 2007/09/11 13:24:53 stuart_hc Exp $
 *
 * Avid labels, keys, etc.
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
 
#ifndef __MXF_AVID_LABELS_AND_KEYS_H__
#define __MXF_AVID_LABELS_AND_KEYS_H__


#ifdef __cplusplus
extern "C" 
{
#endif



static const mxfUL g_avid_DV25ClipWrappedEssenceContainer_label = 
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0e, 0x04, 0x03, 0x01, 0x02, 0x01, 0x00, 0x00};

    
    
static const mxfKey g_AvidObjectDirectory_key = 
    {0x96, 0x13, 0xb3, 0x8a, 0x87, 0x34, 0x87, 0x46, 0xf1, 0x02, 0x96, 0xf0, 0x56, 0xe0, 0x4d, 0x2a};

static const mxfKey g_AvidMetadataRoot_key = 
    {0x80, 0x53, 0x08, 0x00, 0x36, 0x21, 0x08, 0x04, 0xb3, 0xb3, 0x98, 0xa5, 0x1c, 0x90, 0x11, 0xd4};

    
    
#ifdef __cplusplus
}
#endif


#endif


