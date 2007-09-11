/*
 * $Id: mxf_track.c,v 1.1 2007/09/11 13:24:54 stuart_hc Exp $
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



int mxf_get_generic_track(MXFMetadataSet* set, MXFGenericTrack* genericTrack)
{
    memset(genericTrack, 0, sizeof(MXFGenericTrack));
    
    GET_SIMPLE_VALUE(set, genericTrack->trackID, GenericTrack, TrackID, uint32);
    GET_SIMPLE_VALUE(set, genericTrack->trackNumber, GenericTrack, TrackNumber, uint32);
    GET_OPT_STRING_VALUE(set, genericTrack->trackName, GenericTrack, TrackName);
    GET_SIMPLE_VALUE(set, genericTrack->sequence, GenericTrack, Sequence, uuid);

    return 1;
}

void mxf_clear_generic_track(MXFGenericTrack* genericTrack)
{
    CLEAR_OPT_STRING_VALUE(genericTrack->trackName);

    memset(genericTrack, 0, sizeof(MXFGenericTrack));
}

int mxf_get_track(MXFMetadataSet* set, MXFTrack* track)
{
    memset(track, 0, sizeof(MXFTrack));
    
    CHK_ORET(mxf_get_generic_track(set, (MXFGenericTrack*)track));
    
    GET_SIMPLE_VALUE(set, track->editRate, Track, EditRate, rational);
    GET_SIMPLE_VALUE(set, track->origin, Track, Origin, position);
    
    return 1;
}

void mxf_clear_track(MXFTrack* track)
{
    mxf_clear_generic_track((MXFGenericTrack*)track);

    memset(track, 0, sizeof(MXFTrack));
}

