/*
 * $Id: archive_mxf_info_lib.h,v 1.1 2010/01/12 17:40:26 john_f Exp $
 *
 * 
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
 
#ifndef __ARCHIVE_MXF_INFO_LIB_H__
#define __ARCHIVE_MXF_INFO_LIB_H__


#ifdef __cplusplus
extern "C" 
{
#endif


#include <archive_types.h>
#include <mxf/mxf.h>


typedef struct
{
    mxfTimestamp creationDate;
    char filename[256];
    InfaxData sourceInfaxData;
    InfaxData ltoInfaxData;
} ArchiveMXFInfo;

int archive_mxf_load_extensions(MXFDataModel* dataModel);

int is_archive_mxf(MXFHeaderMetadata* headerMetadata);
int archive_mxf_get_info(MXFHeaderMetadata* headerMetadata, ArchiveMXFInfo* info);
int archive_mxf_get_pse_failures(MXFHeaderMetadata* headerMetadata, PSEFailure** failures, long* numFailures);
int archive_mxf_get_vtr_errors(MXFHeaderMetadata* headerMetadata, VTRErrorAtPos** errors, long* numErrors);
int archive_mxf_get_digibeta_dropouts(MXFHeaderMetadata* headerMetadata, DigiBetaDropout** digibetaDropouts, long* numDigiBetaDropouts);


/* returns 1 if footer headermetadata was read, return 2 if none is present (*headerMetadata is NULL) */
int archive_mxf_read_footer_metadata(const char* filename, MXFDataModel* dataModel, MXFHeaderMetadata** headerMetadata);


#ifdef __cplusplus
}
#endif



#endif
