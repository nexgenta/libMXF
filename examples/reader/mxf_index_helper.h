/*
 * $Id: mxf_index_helper.h,v 1.1 2007/09/11 13:24:47 stuart_hc Exp $
 *
 * Utility functions for navigating through the essence data
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
 
#ifndef __MXF_INDEX_HELPER_H__
#define __MXF_INDEX_HELPER_H__


typedef struct _FileIndex FileIndex;


int create_index(MXFFile* mxfFile, MXFList* partitions, uint32_t indexSID, uint32_t bodySID, FileIndex** index);
void free_index(FileIndex** index);

int set_position(MXFFile* mxfFile, FileIndex* index, mxfPosition frameNumber);
int64_t ix_get_last_written_frame_number(MXFFile* mxfFile, FileIndex* index, int64_t duration);
int end_of_essence(FileIndex* index);

void set_next_kl(FileIndex* index, const mxfKey* key, uint8_t llen, uint64_t len);
void get_next_kl(FileIndex* index, mxfKey* key, uint8_t* llen, uint64_t* len);
void get_start_cp_key(FileIndex* index, mxfKey* key);
uint64_t get_cp_len(FileIndex* index);

void increment_current_position(FileIndex* index);

mxfPosition get_current_position(FileIndex* index);
mxfLength get_indexed_duration(FileIndex* index);


#endif

