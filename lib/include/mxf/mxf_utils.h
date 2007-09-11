/*
 * $Id: mxf_utils.h,v 1.2 2007/09/11 13:24:54 stuart_hc Exp $
 *
 * General purpose utilities
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
 
#ifndef __MXF_UTILS_H__
#define __MXF_UTILS_H__



#ifdef __cplusplus
extern "C" 
{
#endif


#define KEY_STR_SIZE        48
#define LABEL_STR_SIZE      48
#define UMID_STR_SIZE       96


void mxf_print_key(const mxfKey* key);
void mxf_sprint_key(char* str, const mxfKey* key);

void mxf_print_label(const mxfUL* label);
void mxf_sprint_label(char* str, const mxfUL* label);

void mxf_print_umid(const mxfUMID* umid);
void mxf_sprint_umid(char* str, const mxfUMID* umid);

void mxf_generate_uuid(mxfUUID* uuid);

void mxf_get_timestamp_now(mxfTimestamp* now);

void mxf_generate_umid(mxfUMID* umid);

void mxf_generate_key(mxfKey* key);


#ifdef __cplusplus
}
#endif


#endif


