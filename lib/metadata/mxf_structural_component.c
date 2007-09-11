/*
 * $Id: mxf_structural_component.c,v 1.1 2007/09/11 13:24:54 stuart_hc Exp $
 *
 * MXF structural component metadata
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


int mxf_get_structural_component(MXFMetadataSet* set, MXFStructuralComponent* structuralComponent)
{
    memset(structuralComponent, 0, sizeof(MXFStructuralComponent));
    
    GET_SIMPLE_VALUE(set, structuralComponent->dataDefinition, StructuralComponent, DataDefinition, ul);
    GET_OPT_SIMPLE_VALUE(set, structuralComponent->duration, StructuralComponent, Duration, length);

    return 1;
}

void mxf_clear_structural_component(MXFStructuralComponent* structuralComponent)
{
    memset(structuralComponent, 0, sizeof(MXFStructuralComponent));
}

int mxf_get_sequence(MXFMetadataSet* set, MXFSequence* sequence)
{
    memset(sequence, 0, sizeof(MXFSequence));
    
    CHK_ORET(mxf_get_structural_component(set, (MXFStructuralComponent*)sequence));

    GET_VARARRAY_VALUE(set, sequence->structuralComponents, Sequence, StructuralComponents, mxfUUID, uuid);
    
    return 1;
}

void mxf_clear_sequence(MXFSequence* sequence)
{
    mxf_clear_structural_component((MXFStructuralComponent*)sequence);
    
    CLEAR_VARARRAY_VALUE(sequence->structuralComponents);

    memset(sequence, 0, sizeof(MXFSequence));
}

int mxf_get_source_clip(MXFMetadataSet* set, MXFSourceClip* sourceClip)
{
    memset(sourceClip, 0, sizeof(MXFSourceClip));
    
    CHK_ORET(mxf_get_structural_component(set, (MXFStructuralComponent*)sourceClip));

    GET_SIMPLE_VALUE(set, sourceClip->startPosition, SourceClip, StartPosition, position);
    GET_SIMPLE_VALUE(set, sourceClip->sourcePackageID, SourceClip, SourcePackageID, umid);
    GET_SIMPLE_VALUE(set, sourceClip->sourceTrackID, SourceClip, SourceTrackID, uint32);
    
    return 1;
}

void mxf_clear_source_clip(MXFSourceClip* sourceClip)
{
    mxf_clear_structural_component((MXFStructuralComponent*)sourceClip);
    
    memset(sourceClip, 0, sizeof(MXFSourceClip));
}


