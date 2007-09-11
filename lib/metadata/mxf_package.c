/*
 * $Id: mxf_package.c,v 1.1 2007/09/11 13:24:54 stuart_hc Exp $
 *
 * MXF package metadata
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

#include <stdlib.h>
#include <string.h>

#include <mxf/mxf.h>
#include <mxf/mxf_metadata.h>

#include "mxf_metadata_int.h"

    

int mxf_get_generic_package(MXFMetadataSet* set, MXFGenericPackage* genericPackage)
{
    memset(genericPackage, 0, sizeof(MXFGenericPackage));
    
    GET_SIMPLE_VALUE(set, genericPackage->packageUID, GenericPackage, PackageUID, umid);
    GET_OPT_STRING_VALUE(set, genericPackage->name, GenericPackage, Name);
    GET_SIMPLE_VALUE(set, genericPackage->packageCreationDate, GenericPackage, PackageCreationDate, timestamp);
    GET_SIMPLE_VALUE(set, genericPackage->packageModifiedDate, GenericPackage, PackageModifiedDate, timestamp);
    GET_VARARRAY_VALUE(set, genericPackage->tracks, GenericPackage, Tracks, mxfUUID, uuid);

    return 1;
}

void mxf_clear_generic_package(MXFGenericPackage* genericPackage)
{
    CLEAR_OPT_STRING_VALUE(genericPackage->name);
    CLEAR_VARARRAY_VALUE(genericPackage->tracks);
    
    memset(genericPackage, 0, sizeof(MXFGenericPackage));
}

int mxf_get_material_package(MXFMetadataSet* set, MXFMaterialPackage* materialPackage)
{
    memset(materialPackage, 0, sizeof(MXFMaterialPackage));

    CHK_ORET(mxf_get_generic_package(set, (MXFGenericPackage*)materialPackage));

    return 1;
}

void mxf_clear_material_package(MXFMaterialPackage* materialPackage)
{
    mxf_clear_generic_package((MXFGenericPackage*)materialPackage);
    
    memset(materialPackage, 0, sizeof(MXFGenericPackage));
}

int mxf_get_source_package(MXFMetadataSet* set, MXFSourcePackage* sourcePackage)
{
    memset(sourcePackage, 0, sizeof(MXFSourcePackage));

    CHK_ORET(mxf_get_generic_package(set, (MXFGenericPackage*)sourcePackage));

    GET_SIMPLE_VALUE(set, sourcePackage->descriptor, SourcePackage, Descriptor, uuid);
    
    return 1;
}

void mxf_clear_source_package(MXFSourcePackage* sourcePackage)
{
    mxf_clear_generic_package((MXFGenericPackage*)sourcePackage);
    
    memset(sourcePackage, 0, sizeof(MXFSourcePackage));
}


