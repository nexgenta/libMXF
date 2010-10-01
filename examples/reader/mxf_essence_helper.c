/*
 * $Id: mxf_essence_helper.c,v 1.18 2010/10/01 15:51:10 john_f Exp $
 *
 * Utilities for processing essence data and associated metadata
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
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include <mxf_reader_int.h>
#include <mxf_essence_helper.h>
#include <mxf/mxf_avid.h>


/* TODO: check for best effort distinguished values in incomplete header metadata */
/* TODO: handle frame size sequences for audio */


/* System item used in the BBC D3 preservation project. The system item contains a 
   local set item which has an array of 2 timecodes, VITC followed by LTC */
static const mxfKey g_SysItemElementKey1 = MXF_SS1_ELEMENT_KEY(0x01, 0x00);


static void convert_12m_to_timecode(uint8_t* t12m, int* isDropFrame, 
    uint8_t* hour, uint8_t* min, uint8_t* sec, uint8_t* frame)
{
    *isDropFrame = (t12m[0] & 0x40) != 0;
    *frame = ((t12m[0] >> 4) & 0x03) * 10 + (t12m[0] & 0x0f);
    *sec = ((t12m[1] >> 4) & 0x07) * 10 + (t12m[1] & 0x0f);
    *min = ((t12m[2] >> 4) & 0x07) * 10 + (t12m[2] & 0x0f);
    *hour = ((t12m[3] >> 4) & 0x03) * 10 + (t12m[3] & 0x0f);
}



int is_d10_essence(const mxfUL* label)
{
    if (mxf_equals_ul(label, &MXF_EC_L(D10_50_625_50_defined_template)) ||
        mxf_equals_ul(label, &MXF_EC_L(D10_50_625_50_extended_template)) ||
        mxf_equals_ul(label, &MXF_EC_L(D10_50_625_50_picture_only)) ||
        mxf_equals_ul(label, &MXF_EC_L(D10_50_525_60_defined_template)) ||
        mxf_equals_ul(label, &MXF_EC_L(D10_50_525_60_extended_template)) ||
        mxf_equals_ul(label, &MXF_EC_L(D10_50_525_60_picture_only)) ||
        mxf_equals_ul(label, &MXF_EC_L(D10_40_625_50_defined_template)) ||
        mxf_equals_ul(label, &MXF_EC_L(D10_40_625_50_extended_template)) ||
        mxf_equals_ul(label, &MXF_EC_L(D10_40_625_50_picture_only)) ||
        mxf_equals_ul(label, &MXF_EC_L(D10_40_525_60_defined_template)) ||
        mxf_equals_ul(label, &MXF_EC_L(D10_40_525_60_extended_template)) ||
        mxf_equals_ul(label, &MXF_EC_L(D10_40_525_60_picture_only)) ||
        mxf_equals_ul(label, &MXF_EC_L(D10_30_625_50_defined_template)) ||
        mxf_equals_ul(label, &MXF_EC_L(D10_30_625_50_extended_template)) ||
        mxf_equals_ul(label, &MXF_EC_L(D10_30_625_50_picture_only)) ||
        mxf_equals_ul(label, &MXF_EC_L(D10_30_525_60_defined_template)) ||
        mxf_equals_ul(label, &MXF_EC_L(D10_30_525_60_extended_template)) ||
        mxf_equals_ul(label, &MXF_EC_L(D10_30_525_60_picture_only)))
    {
        return 1;
    }
    
    return 0;
}

int process_cdci_descriptor(MXFMetadataSet* descriptorSet, MXFTrack* track, EssenceTrack* essenceTrack)
{
    uint8_t frameLayout;
    uint32_t fieldWidth;
    uint32_t fieldHeight;
    uint32_t displayWidth;
    uint32_t displayHeight;
    uint32_t displayXOffset;
    uint32_t displayYOffset;
    int32_t avidResolutionID;
    int32_t avidFrameSize;
    
    
    /* in some Avid files the essence container label extracted from the partition packs or Preface is more
    useful, and the label will already have been set */
    if (mxf_equals_ul(&track->essenceContainerLabel, &g_Null_UL))
    {
        CHK_ORET(mxf_get_ul_item(descriptorSet, &MXF_ITEM_K(FileDescriptor, EssenceContainer), &track->essenceContainerLabel));
    }
    CHK_ORET(mxf_get_rational_item(descriptorSet, &MXF_ITEM_K(FileDescriptor, SampleRate), &essenceTrack->sampleRate));
    clean_rate(&essenceTrack->sampleRate);
    if (mxf_have_item(descriptorSet, &MXF_ITEM_K(FileDescriptor, ContainerDuration)))
    {
        CHK_ORET(mxf_get_length_item(descriptorSet, &MXF_ITEM_K(FileDescriptor, ContainerDuration), &essenceTrack->containerDuration));
    }
    else
    {
        essenceTrack->containerDuration = -1;
    }
    /* Note: AspectRatio is best effort, with distinguished value (0,0) */
    CHK_ORET(mxf_get_rational_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, AspectRatio), &track->video.aspectRatio));
    /* Note: ComponentDepth is best effort */
    CHK_ORET(mxf_get_uint32_item(descriptorSet, &MXF_ITEM_K(CDCIEssenceDescriptor, ComponentDepth), &track->video.componentDepth));
    CHK_ORET(track->video.componentDepth != 0);

    if (mxf_have_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, PictureEssenceCoding)))
    {
        CHK_ORET(mxf_get_ul_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, PictureEssenceCoding), &track->pictureEssenceCodingLabel));
    }

    /* Avid extension: FirstFrameOffset is the offset from the start of the essence data element to the first frame */
    if (mxf_have_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, FirstFrameOffset)))
    {
        CHK_ORET(mxf_get_int32_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, FirstFrameOffset), &essenceTrack->avidFirstFrameOffset));
    }
    
    if (mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(IECDV_25_525_60_ClipWrapped)) ||
        mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(IECDV_25_525_60_FrameWrapped)))
    {
        track->video.frameWidth = 720;
        track->video.frameHeight = 240 * 2;
        track->video.displayWidth = track->video.frameWidth;
        track->video.displayHeight = track->video.frameHeight;
        track->video.displayXOffset = 0;
        track->video.displayYOffset = 0;
        track->video.horizSubsampling = 4;
        track->video.vertSubsampling = 1;
        essenceTrack->frameSize = 120000;
    }
    else if (mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(IECDV_25_625_50_ClipWrapped)) ||
        mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(IECDV_25_625_50_FrameWrapped)))
    {
        track->video.frameWidth = 720;
        track->video.frameHeight = 288 * 2;
        track->video.displayWidth = track->video.frameWidth;
        track->video.displayHeight = track->video.frameHeight;
        track->video.displayXOffset = 0;
        track->video.displayYOffset = 0;
        track->video.horizSubsampling = 2;
        track->video.vertSubsampling = 2;
        essenceTrack->frameSize = 144000;
    }
    else if (mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(DVBased_25_525_60_ClipWrapped)) ||
        mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(DVBased_25_525_60_FrameWrapped)))
    {
        track->video.frameWidth = 720;
        track->video.frameHeight = 240 * 2;
        track->video.displayWidth = track->video.frameWidth;
        track->video.displayHeight = track->video.frameHeight;
        track->video.displayXOffset = 0;
        track->video.displayYOffset = 0;
        track->video.horizSubsampling = 4;
        track->video.vertSubsampling = 1;
        essenceTrack->frameSize = 120000;
    }
    else if (mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(DVBased_25_625_50_ClipWrapped)) ||
        mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(DVBased_25_625_50_FrameWrapped)))
    {
        track->video.frameWidth = 720;
        track->video.frameHeight = 288 * 2;
        track->video.displayWidth = track->video.frameWidth;
        track->video.displayHeight = track->video.frameHeight;
        track->video.displayXOffset = 0;
        track->video.displayYOffset = 0;
        track->video.horizSubsampling = 4;
        track->video.vertSubsampling = 1;
        essenceTrack->frameSize = 144000;
    }
    else if (mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(DVBased_50_525_60_ClipWrapped)) ||
        mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(DVBased_50_525_60_FrameWrapped)))
    {
        track->video.frameWidth = 720;
        track->video.frameHeight = 240 * 2;
        track->video.displayWidth = track->video.frameWidth;
        track->video.displayHeight = track->video.frameHeight;
        track->video.displayXOffset = 0;
        track->video.displayYOffset = 0;
        track->video.horizSubsampling = 2;
        track->video.vertSubsampling = 1;
        essenceTrack->frameSize = 240000;
    }
    else if (mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(DVBased_50_625_50_ClipWrapped)) || 
        mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(DVBased_50_625_50_FrameWrapped)))
    {
        track->video.frameWidth = 720;
        track->video.frameHeight = 288 * 2;
        track->video.displayWidth = track->video.frameWidth;
        track->video.displayHeight = track->video.frameHeight;
        track->video.displayXOffset = 0;
        track->video.displayYOffset = 0;
        track->video.horizSubsampling = 2;
        track->video.vertSubsampling = 1;
        essenceTrack->frameSize = 288000;
    }
    else if (mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(D10_50_625_50_defined_template)) ||
        mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(D10_50_625_50_extended_template)) ||
        mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(D10_50_625_50_picture_only)) ||
        mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(AvidIMX50_625_50)))
    {
        track->video.frameWidth = 720;
        track->video.frameHeight = 304 * 2;
        track->video.displayWidth = 720;
        track->video.displayHeight = 288 * 2;
        track->video.displayXOffset = 0;
        track->video.displayYOffset = 16 * 2;
        track->video.horizSubsampling = 2;
        track->video.vertSubsampling = 1;
    }
    else if (mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(D10_50_525_60_defined_template)) ||
        mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(D10_50_525_60_extended_template)) ||
        mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(D10_50_525_60_picture_only)) ||
        mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(AvidIMX50_525_60)))
    {
        track->video.frameWidth = 720;
        track->video.frameHeight = 256 * 2;
        track->video.displayWidth = 720;
        track->video.displayHeight = 243 * 2;
        track->video.displayXOffset = 0;
        track->video.displayYOffset = 13 * 2;
        track->video.horizSubsampling = 2;
        track->video.vertSubsampling = 1;
    }
    else if (mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(D10_40_625_50_defined_template)) ||
        mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(D10_40_625_50_extended_template)) ||
        mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(D10_40_625_50_picture_only)) ||
        mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(AvidIMX40_625_50)))
    {
        track->video.frameWidth = 720;
        track->video.frameHeight = 304 * 2;
        track->video.displayWidth = 720;
        track->video.displayHeight = 288 * 2;
        track->video.displayXOffset = 0;
        track->video.displayYOffset = 16 * 2;
        track->video.horizSubsampling = 2;
        track->video.vertSubsampling = 1;
    }
    else if (mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(D10_40_525_60_defined_template)) ||
        mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(D10_40_525_60_extended_template)) ||
        mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(D10_40_525_60_picture_only)) ||
        mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(AvidIMX40_525_60)))
    {
        track->video.frameWidth = 720;
        track->video.frameHeight = 256 * 2;
        track->video.displayWidth = 720;
        track->video.displayHeight = 243 * 2;
        track->video.displayXOffset = 0;
        track->video.displayYOffset = 13 * 2;
        track->video.horizSubsampling = 2;
        track->video.vertSubsampling = 1;
    }
    else if (mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(D10_30_625_50_defined_template)) ||
        mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(D10_30_625_50_extended_template)) ||
        mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(D10_30_625_50_picture_only)) ||
        mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(AvidIMX30_625_50)))
    {
        track->video.frameWidth = 720;
        track->video.frameHeight = 304 * 2;
        track->video.displayWidth = 720;
        track->video.displayHeight = 288 * 2;
        track->video.displayXOffset = 0;
        track->video.displayYOffset = 16 * 2;
        track->video.horizSubsampling = 2;
        track->video.vertSubsampling = 1;
    }
    else if (mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(D10_30_525_60_defined_template)) ||
        mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(D10_30_525_60_extended_template)) ||
        mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(D10_30_525_60_picture_only)) ||
        mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(AvidIMX30_525_60)))
    {
        track->video.frameWidth = 720;
        track->video.frameHeight = 256 * 2;
        track->video.displayWidth = 720;
        track->video.displayHeight = 243 * 2;
        track->video.displayXOffset = 0;
        track->video.displayYOffset = 13 * 2;
        track->video.horizSubsampling = 2;
        track->video.vertSubsampling = 1;
    }
    else if (mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(DVBased_100_1080_50_I_ClipWrapped)) ||
             mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(DVBased_100_1080_50_I_FrameWrapped)) ||
             mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(DVBased_100_1080_60_I_ClipWrapped)) ||
             mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(DVBased_100_1080_60_I_FrameWrapped)))
    {
        track->video.frameWidth = 1440;
        track->video.frameHeight = 540 * 2;
        track->video.displayWidth = 1920;
        track->video.displayHeight = 540 * 2;
        track->video.displayXOffset = 0;
        track->video.displayYOffset = 0;
        track->video.horizSubsampling = 2;
        track->video.vertSubsampling = 1;
        essenceTrack->frameSize = 576000;
    }
    else if (mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(DVBased_100_720_50_P_ClipWrapped)) ||
             mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(DVBased_100_720_50_P_FrameWrapped)) ||
             mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(DVBased_100_720_60_P_ClipWrapped)) ||
             mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(DVBased_100_720_60_P_FrameWrapped)))
    {
        track->video.frameWidth = 960;
        track->video.frameHeight = 720;
        track->video.displayWidth = 1290;
        track->video.displayHeight = 720;
        track->video.displayXOffset = 0;
        track->video.displayYOffset = 0;
        track->video.horizSubsampling = 2;
        track->video.vertSubsampling = 1;
        essenceTrack->frameSize = 288000;
    }
    else if (mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(DNxHD1080i120ClipWrapped)))
    {
        track->video.frameWidth = 1920;
        track->video.frameHeight = 540 * 2;
        track->video.displayWidth = 1920;
        track->video.displayHeight = 540 * 2;
        track->video.displayXOffset = 0;
        track->video.displayYOffset = 0;
        track->video.horizSubsampling = 2;
        track->video.vertSubsampling = 1;
        essenceTrack->frameSize = 606208;
    }
    else if (mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(DNxHD1080i185ClipWrapped)))
    {
        track->video.frameWidth = 1920;
        track->video.frameHeight = 540 * 2;
        track->video.displayWidth = 1920;
        track->video.displayHeight = 540 * 2;
        track->video.displayXOffset = 0;
        track->video.displayYOffset = 0;
        track->video.horizSubsampling = 2;
        track->video.vertSubsampling = 1;
        essenceTrack->frameSize = 917504;
    }
    else if (mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(DNxHD1080p120ClipWrapped)))
    {
        track->video.frameWidth = 1920;
        track->video.frameHeight = 1080;
        track->video.displayWidth = 1920;
        track->video.displayHeight = 1080;
        track->video.displayXOffset = 0;
        track->video.displayYOffset = 0;
        track->video.horizSubsampling = 2;
        track->video.vertSubsampling = 1;
        essenceTrack->frameSize = 606208;
    }
    else if (mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(DNxHD1080p185ClipWrapped)))
    {
        track->video.frameWidth = 1920;
        track->video.frameHeight = 1080;
        track->video.displayWidth = 1920;
        track->video.displayHeight = 1080;
        track->video.displayXOffset = 0;
        track->video.displayYOffset = 0;
        track->video.horizSubsampling = 2;
        track->video.vertSubsampling = 1;
        essenceTrack->frameSize = 917504;
    }
    else if (mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(DNxHD1080p36ClipWrapped)))
    {
        track->video.frameWidth = 1920;
        track->video.frameHeight = 1080;
        track->video.displayWidth = 1920;
        track->video.displayHeight = 1080;
        track->video.displayXOffset = 0;
        track->video.displayYOffset = 0;
        track->video.horizSubsampling = 2;
        track->video.vertSubsampling = 1;
        essenceTrack->frameSize = 188416;
    }
    else if (mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(DNxHD720p120ClipWrapped)))
    {
        track->video.frameWidth = 1280;
        track->video.frameHeight = 720;
        track->video.displayWidth = 1280;
        track->video.displayHeight = 720;
        track->video.displayXOffset = 0;
        track->video.displayYOffset = 0;
        track->video.horizSubsampling = 2;
        track->video.vertSubsampling = 1;
        essenceTrack->frameSize = 303104;
    }
    else if (mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(DNxHD720p185ClipWrapped)))
    {
        track->video.frameWidth = 1280;
        track->video.frameHeight = 720;
        track->video.displayWidth = 1280;
        track->video.displayHeight = 720;
        track->video.displayXOffset = 0;
        track->video.displayYOffset = 0;
        track->video.horizSubsampling = 2;
        track->video.vertSubsampling = 1;
        essenceTrack->frameSize = 458752;
    }
    else if (mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(SD_Unc_625_50i_422_135_FrameWrapped)) ||
             mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(SD_Unc_625_50i_422_135_ClipWrapped)) ||
             mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(SD_Unc_525_5994i_422_135_ClipWrapped)) ||
             mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(SD_Unc_525_5994i_422_135_FrameWrapped)))
    {
        CHK_ORET(mxf_get_uint32_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, StoredHeight), &fieldHeight));
        if (fieldHeight == 0) /* best effort distinguished value */
        {
            fieldHeight = 0; /* TODO: how will players react to 0 ? */
        }
        CHK_ORET(mxf_get_uint32_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, StoredWidth), &fieldWidth));
        if (fieldWidth == 0) /* best effort distinguished value */
        {
            fieldWidth = 0; /* TODO: how will players react to 0 ? */
        }
        if (mxf_have_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayHeight)))
        {
            CHK_ORET(mxf_get_uint32_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayHeight), &displayHeight));
        }
        else
        {
            displayHeight = fieldHeight;
        }
        if (mxf_have_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayWidth)))
        {
            CHK_ORET(mxf_get_uint32_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayWidth), &displayWidth));
        }
        else
        {
            displayWidth = fieldWidth;
        }
        if (mxf_have_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayYOffset)))
        {
            CHK_ORET(mxf_get_uint32_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayYOffset), &displayYOffset));
        }
        else
        {
            displayYOffset = 0;
        }
        if (mxf_have_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayXOffset)))
        {
            CHK_ORET(mxf_get_uint32_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayXOffset), &displayXOffset));
        }
        else
        {
            displayXOffset = 0;
        }
        CHK_ORET(mxf_get_uint32_item(descriptorSet, &MXF_ITEM_K(CDCIEssenceDescriptor, HorizontalSubsampling), &track->video.horizSubsampling));
        CHK_ORET(mxf_get_uint32_item(descriptorSet, &MXF_ITEM_K(CDCIEssenceDescriptor, VerticalSubsampling), &track->video.vertSubsampling));
        CHK_ORET(mxf_get_uint8_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameLayout), &frameLayout));
        CHK_ORET(frameLayout == 3); /* TODO: only mixed fields supported for now */

        track->video.frameWidth = fieldWidth;
        track->video.frameHeight = fieldHeight;
        track->video.displayWidth = displayWidth;
        track->video.displayHeight = displayHeight;
        track->video.displayXOffset = displayXOffset;
        track->video.displayYOffset = displayYOffset;
        
        /* check for Avid uncompressed data */
        if (mxf_have_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, ResolutionID)))
        {
            /* Only support 8-bit */
            CHK_ORET(track->video.componentDepth == 8);
            
            CHK_ORET(mxf_get_int32_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, ResolutionID), &avidResolutionID));
            if (avidResolutionID == 0xaa) /* Avid 8-bit uncompressed UYVY */
            {
                /* get frame size and image start offset */
                CHK_ORET(mxf_get_int32_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameSampleSize), &avidFrameSize));
                CHK_ORET(avidFrameSize > 0);
                essenceTrack->frameSize = avidFrameSize;
                CHK_ORET(mxf_get_uint32_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageStartOffset), &essenceTrack->imageStartOffset));
            }
        }
        else
        {
            if (track->video.componentDepth == 8)
            {
                essenceTrack->frameSize = (uint32_t)(fieldWidth * fieldHeight * 
                    (1 + 2.0 / (track->video.horizSubsampling * track->video.vertSubsampling)) + 0.5);
            }
            else
            {
                /* Only support 10-bit UYVY 4:2:2 */
                CHK_ORET(mxf_equals_ul(&track->pictureEssenceCodingLabel, &MXF_CMDEF_L(UNC_10B_422_INTERLEAVED)));
                /* SMPTE 377M-2009 states in G.2.25 that "Stored width shall be a multiple of 6" */
                CHK_ORET((fieldWidth / 6) * 6 == fieldWidth);
                
                essenceTrack->frameSize = (uint32_t)(fieldWidth / 6 * 16 * fieldHeight);
            }
        }
    }
    else if (mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(HD_Unc_1080_50i_422_FrameWrapped)) ||
             mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(HD_Unc_1080_50i_422_ClipWrapped)) ||
             mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(HD_Unc_1080_5994i_422_FrameWrapped)) ||
             mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(HD_Unc_1080_5994i_422_ClipWrapped)))
    {
        /* only 8-bit supported */
        CHK_ORET(track->video.componentDepth == 8);


        CHK_ORET(mxf_get_uint32_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, StoredHeight), &fieldHeight));
        if (fieldHeight == 0) /* best effort distinguished value */
        {
            fieldHeight = 0; /* TODO: how will players react to 0 ? */
        }
        CHK_ORET(mxf_get_uint32_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, StoredWidth), &fieldWidth));
        if (fieldWidth == 0) /* best effort distinguished value */
        {
            fieldWidth = 0; /* TODO: how will players react to 0 ? */
        }
        if (mxf_have_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayHeight)))
        {
            CHK_ORET(mxf_get_uint32_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayHeight), &displayHeight));
        }
        else
        {
            displayHeight = fieldHeight;
        }
        if (mxf_have_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayWidth)))
        {
            CHK_ORET(mxf_get_uint32_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayWidth), &displayWidth));
        }
        else
        {
            displayWidth = fieldWidth;
        }
        if (mxf_have_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayYOffset)))
        {
            CHK_ORET(mxf_get_uint32_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayYOffset), &displayYOffset));
        }
        else
        {
            displayYOffset = 0;
        }
        if (mxf_have_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayXOffset)))
        {
            CHK_ORET(mxf_get_uint32_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayXOffset), &displayXOffset));
        }
        else
        {
            displayXOffset = 0;
        }
        CHK_ORET(mxf_get_uint32_item(descriptorSet, &MXF_ITEM_K(CDCIEssenceDescriptor, HorizontalSubsampling), &track->video.horizSubsampling));
        CHK_ORET(mxf_get_uint32_item(descriptorSet, &MXF_ITEM_K(CDCIEssenceDescriptor, VerticalSubsampling), &track->video.vertSubsampling));
        CHK_ORET(mxf_get_uint8_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameLayout), &frameLayout));
        CHK_ORET(frameLayout == 3); /* TODO: only mixed fields supported for now */

        track->video.frameWidth = fieldWidth;
        track->video.frameHeight = fieldHeight;
        track->video.displayWidth = displayWidth;
        track->video.displayHeight = displayHeight;
        track->video.displayXOffset = displayXOffset;
        track->video.displayYOffset = displayYOffset;
        
        if (mxf_have_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameSampleSize)))
        {
            CHK_ORET(mxf_get_int32_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameSampleSize), &avidFrameSize));
            CHK_ORET(avidFrameSize > 0);
            essenceTrack->frameSize = avidFrameSize;
            CHK_ORET(mxf_get_uint32_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageStartOffset), &essenceTrack->imageStartOffset));
        }
        else
        {
            essenceTrack->frameSize = (uint32_t)(fieldWidth * fieldHeight * 
                (1 + 2.0 / (track->video.horizSubsampling * track->video.vertSubsampling)) + 0.5);
        }
    }
    else if (mxf_equals_ul(&track->essenceContainerLabel, &MXF_EC_L(AvidMJPEGClipWrapped)))
    {
        /* MJPEG is essentially a private Avid extension, so we can rely on observed MXF output from Avid */
        /* e.g. Avid always outputs correct DisplayWidth/HeightX/YOffset properties for MJPEG */
        CHK_ORET(mxf_get_uint32_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, StoredHeight), &fieldHeight));
        CHK_ORET(mxf_get_uint32_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, StoredWidth), &fieldWidth));
        CHK_ORET(mxf_get_uint32_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayHeight), &displayHeight));
        CHK_ORET(mxf_get_uint32_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayWidth), &displayWidth));
        CHK_ORET(mxf_get_uint32_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayYOffset), &displayYOffset));
        CHK_ORET(mxf_get_uint32_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayXOffset), &displayXOffset));
        CHK_ORET(mxf_get_uint8_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameLayout), &frameLayout));
        CHK_ORET(mxf_get_uint32_item(descriptorSet, &MXF_ITEM_K(CDCIEssenceDescriptor, HorizontalSubsampling), &track->video.horizSubsampling));
        CHK_ORET(mxf_get_uint32_item(descriptorSet, &MXF_ITEM_K(CDCIEssenceDescriptor, VerticalSubsampling), &track->video.vertSubsampling));

        /* SeparateFields formats (2:1, 3:1, 10:1, 20:1) need to have their height doubled */
        /* for display while OneField formats (2:1s, 15:1s, 4:1s, 10:1m, 4:1m) don't.      */ 
        int factor;
        if (frameLayout == 1)  /* SeparateFields */
            factor = 2;
        else
            factor = 1;

        track->video.frameWidth = fieldWidth;
        track->video.frameHeight = fieldHeight * factor;
        track->video.displayWidth = displayWidth;
        track->video.displayHeight = displayHeight * factor;
        track->video.displayXOffset = displayXOffset;
        track->video.displayYOffset = displayYOffset * 2;

        essenceTrack->frameSize = -1; /* variable */
    }
    else
    {
        mxf_log_error("Unsupported essence type" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
        return 0;
    }
    
    return 1;
}

int process_sound_descriptor(MXFMetadataSet* descriptorSet, MXFTrack* track, EssenceTrack* essenceTrack)
{
    /* in some Avid files the essence container label extracted from the partition packs or Preface is more
       useful, and the label will already have been set */
    if (mxf_equals_ul(&track->essenceContainerLabel, &g_Null_UL))
    {
        CHK_ORET(mxf_get_ul_item(descriptorSet, &MXF_ITEM_K(FileDescriptor, EssenceContainer), &track->essenceContainerLabel));
    }
    CHK_ORET(mxf_get_rational_item(descriptorSet, &MXF_ITEM_K(FileDescriptor, SampleRate), &essenceTrack->sampleRate));
    clean_rate(&essenceTrack->sampleRate);
    if (mxf_have_item(descriptorSet, &MXF_ITEM_K(FileDescriptor, ContainerDuration)))
    {
        CHK_ORET(mxf_get_length_item(descriptorSet, &MXF_ITEM_K(FileDescriptor, ContainerDuration), &essenceTrack->containerDuration));
    }
    else
    {
        essenceTrack->containerDuration = -1;
    }
    /* Note: AudioSamplingRate is best effort */
    CHK_ORET(mxf_get_rational_item(descriptorSet, &MXF_ITEM_K(GenericSoundEssenceDescriptor, AudioSamplingRate), &track->audio.samplingRate));
    CHK_ORET(track->audio.samplingRate.numerator != 0 && track->audio.samplingRate.denominator != 0); 
    /* Note: ChannelCount is best effort (distinguished value is 0) */
    CHK_ORET(mxf_get_uint32_item(descriptorSet, &MXF_ITEM_K(GenericSoundEssenceDescriptor, ChannelCount), &track->audio.channelCount));
    /* Note: QuantizationBits is best effort */
    CHK_ORET(mxf_get_uint32_item(descriptorSet, &MXF_ITEM_K(GenericSoundEssenceDescriptor, QuantizationBits), &track->audio.bitsPerSample));
    CHK_ORET(track->audio.bitsPerSample != 0); 
    
    /* TODO: mustn't assume PCM here */
    track->audio.blockAlign = track->audio.channelCount * ((track->audio.bitsPerSample + 7) / 8);
    
    essenceTrack->frameSize = -1;
    
    return 1;
}

int process_wav_descriptor(MXFMetadataSet* descriptorSet, MXFTrack* track, EssenceTrack* essenceTrack)
{
    CHK_ORET(process_sound_descriptor(descriptorSet, track, essenceTrack));
    
    CHK_ORET(mxf_get_uint16_item(descriptorSet, &MXF_ITEM_K(WaveAudioDescriptor, BlockAlign), &track->audio.blockAlign));
    
    essenceTrack->frameSize = (uint32_t)(track->audio.blockAlign * 
        track->audio.samplingRate.numerator * essenceTrack->frameRate.denominator / 
        (double)(track->audio.samplingRate.denominator * essenceTrack->frameRate.numerator));

    return 1;        
}


/* TODO: handle situations where data is not audio */
/* TODO: handle cases where channels are in different order (is this allowed?) */
int convert_aes_to_pcm(uint32_t channelCount, uint32_t bitsPerSample, 
    uint8_t* buffer, uint64_t aesDataLen, uint64_t* pcmDataLen)
{
    uint16_t audioSampleCount = (buffer[2] << 8) | buffer[1];
    uint8_t channelValidFlags = buffer[3];
    uint32_t blockAlign = (bitsPerSample + 7) / 8;
    uint8_t aes3ChannelCount = (channelValidFlags & 0x01) +
        ((channelValidFlags >> 1) & 0x01) +
        ((channelValidFlags >> 2) & 0x01) +
        ((channelValidFlags >> 3) & 0x01) +
        ((channelValidFlags >> 4) & 0x01) +
        ((channelValidFlags >> 5) & 0x01) +
        ((channelValidFlags >> 6) & 0x01) +
        ((channelValidFlags >> 7) & 0x01);
    uint8_t* aesDataPtr;
    uint8_t* pcmDataPtr;
    uint16_t sampleNum;
    uint8_t channel;

    CHK_ORET(channelCount <= aes3ChannelCount);
    CHK_ORET(blockAlign >= 1 && blockAlign <= 3); /* only 8-bit to 24-bit sample size possible */
    CHK_ORET(audioSampleCount == (aesDataLen - 4) / (8 * 4)); /* 4 bytes per sample, 8 channels */
    
    aesDataPtr = &buffer[4];
    pcmDataPtr = &buffer[0];
    for (sampleNum = 0; sampleNum < audioSampleCount; sampleNum++)
    {
        for (channel = 0; channel < 8; channel++)
        {
            /* write audio channel if contains valid audio data */
            if (channelValidFlags & (0x01 << channel))
            {
                /* write PCM word */
                switch (blockAlign)
                {
                    case 1:
                        pcmDataPtr[0] = ((aesDataPtr[2] & 0xf0 ) >> 4) |
                            ((aesDataPtr[3] << 4) & 0xff);
                        break;
                    case 2:
                        pcmDataPtr[0] = ((aesDataPtr[1] & 0xf0 ) >> 4) |
                            ((aesDataPtr[2] << 4) & 0xff);
                        pcmDataPtr[1] = ((aesDataPtr[2] & 0xf0 ) >> 4) |
                            ((aesDataPtr[3] << 4) & 0xff);
                        break;
                    case 3:
                        pcmDataPtr[0] = ((aesDataPtr[0] & 0xf0) >> 4) |
                            ((aesDataPtr[1] << 4) & 0xff);
                        pcmDataPtr[1] = ((aesDataPtr[1] & 0xf0 ) >> 4) |
                            ((aesDataPtr[2] << 4) & 0xff);
                        pcmDataPtr[2] = ((aesDataPtr[2] & 0xf0 ) >> 4) |
                            ((aesDataPtr[3] << 4) & 0xff);
                        break;
                    default:
                        assert(0);
                        return 0;
                }
                pcmDataPtr += blockAlign;
            }
            aesDataPtr += 4 ; /* 4 bytes per sample */
        }
    }
    
    *pcmDataLen = pcmDataPtr - buffer;
    
    return 1;
}

int accept_frame(MXFReaderListener* listener, int trackIndex)
{
    if (listener && listener->accept_frame)
    {
        return listener->accept_frame(listener, trackIndex);
    }
    return 0;
}

int read_frame(MXFReader* reader, MXFReaderListener* listener, int trackIndex, 
    uint64_t frameSize, uint8_t** buffer, uint64_t* bufferSize)
{
    MXFFile* mxfFile = reader->mxfFile;
    EssenceTrack* essenceTrack;
    uint8_t* newBuffer = NULL;
    uint64_t newBufferSize;

    CHK_ORET((essenceTrack = get_essence_track(reader->essenceReader, trackIndex)) != NULL);
    
    if (essenceTrack->imageStartOffset != 0)
    {
        CHK_ORET(frameSize > essenceTrack->imageStartOffset);
        
        /* experiments have shown that seeking or reading twice instead of reading once can 
        effect the disk access speed badly. Experiment with clip wrapped HD material (1920x1080 uncompressed) 
        has shown that doing a seek and read or 2 reads causes a bit rate reduction from 100 MB/s to 40 MB/s */ 
        
        /* allocate internal buffer if neccessary */
        if (reader->buffer == NULL || reader->bufferSize < frameSize)
        {
            SAFE_FREE(&reader->buffer);
            CHK_MALLOC_ARRAY_ORET(reader->buffer, uint8_t, frameSize);
        }
        
        /* read frame with padding into internal buffer */
        CHK_OFAIL(mxf_file_read(mxfFile, reader->buffer, frameSize) == frameSize);
        
        /* get client to allocate a buffer to contain just the image data */        
        newBufferSize = frameSize - essenceTrack->imageStartOffset;
        CHK_OFAIL(listener->allocate_buffer(listener, trackIndex, &newBuffer, newBufferSize));

        /* copy image data to the client buffer */        
        CHK_OFAIL(memcpy(newBuffer, &reader->buffer[essenceTrack->imageStartOffset], newBufferSize));
    }
    else
    {
        /* get client to allocate a buffer */        
        newBufferSize = frameSize;
        CHK_OFAIL(listener->allocate_buffer(listener, trackIndex, &newBuffer, newBufferSize));
        
        /* read data into the client's buffer */
        CHK_OFAIL(mxf_file_read(mxfFile, newBuffer, newBufferSize) == newBufferSize);
    }

    *bufferSize = newBufferSize;
    *buffer = newBuffer;
    return 1;
    
fail:
    listener->deallocate_buffer(listener, trackIndex, &newBuffer);
    return 0;
}

int send_frame(MXFReader* reader, MXFReaderListener* listener, int trackIndex, 
    uint8_t* buffer, uint64_t dataLen)
{
    MXFTrack* track;
    EssenceTrack* essenceTrack;
    uint64_t newDataLen;
    
    CHK_ORET((track = get_mxf_track(reader, trackIndex)) != NULL);
    CHK_ORET((essenceTrack = get_essence_track(reader->essenceReader, trackIndex)) != NULL);
    
    /* extract raw pcm from AES data (SDTI-CP sound item, 8-channel A£S3 element) */
    if (mxf_get_essence_element_item_type(essenceTrack->trackNumber) == 0x06 &&
        mxf_get_essence_element_type(essenceTrack->trackNumber) == 0x10)
    {
        CHK_ORET(convert_aes_to_pcm(track->audio.channelCount, track->audio.bitsPerSample, 
            buffer, dataLen, &newDataLen));
        CHK_ORET(listener->receive_frame(listener, trackIndex, buffer, newDataLen));
    }
    else
    {
        CHK_ORET(listener->receive_frame(listener, trackIndex, buffer, dataLen));
    }
    
    return 1;
}
    

int element_is_known_system_item(const mxfKey* key)
{
    return mxf_equals_key(key, &g_SysItemElementKey1) ||
        mxf_equals_key(key, &MXF_EE_K(SDTI_CP_System_Pack));
}

int extract_system_item_info(MXFReader* reader, const mxfKey* key, uint64_t len, mxfPosition position)
{
    MXFFile* mxfFile = reader->mxfFile;
    uint16_t localTag;
    uint16_t localItemLen;
    uint8_t arrayHeader[8];
    uint32_t arrayLen;
    uint32_t arrayItemLen;
    uint8_t t12m[8];
    uint32_t crc32;
    uint64_t lenRemaining;
    uint32_t i;
    int isDropFrame;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint8_t frame;
    uint8_t systemPackData[57];
    

    /* Read the array of SMPTE 12M timecodes in local set item 0x0102 
       This timecode representation is used for the BBC Archive preservation project, 
       where the first timecode in the array is the VITC and the second timecode is the LTC */    
    /* Read the array of CRC-32 in local set item 0xffff 
       This item contains an array of CRC-32s for BBC Archive preservation project,
       where each CRC-32 was calculated from the video element and audio elements */
    if (mxf_equals_key(key, &g_SysItemElementKey1))
    {
        lenRemaining = len;
        while (lenRemaining > 0)
        {
            if (lenRemaining > 4)
            {
                CHK_ORET(mxf_read_uint16(mxfFile, &localTag));
                lenRemaining -= 2;
                CHK_ORET(mxf_read_uint16(mxfFile, &localItemLen));
                lenRemaining -= 2;
                
                if (localTag == 0x0102)
                {
                    /* Archive MXF Timecode array */
                    
                    CHK_ORET(mxf_file_read(mxfFile, arrayHeader, 8) == 8);
                    lenRemaining -= 8;
                    mxf_get_array_header(arrayHeader, &arrayLen, &arrayItemLen);
                    CHK_ORET(arrayItemLen == 8);
                
                    for (i = 0; i < arrayLen; i++)
                    {
                        CHK_ORET(mxf_file_read(mxfFile, t12m, 8) == 8);
                        lenRemaining -= 8;
                        convert_12m_to_timecode(t12m, &isDropFrame, &hour, &min, &sec, &frame);
                        CHK_ORET(set_essence_container_timecode(reader, position, 
                                SYSTEM_ITEM_TC_ARRAY_TIMECODE, i, isDropFrame, hour, min, sec, frame));
                    }
                }
                else if (localTag == 0xffff)
                {
                    /* Archive MXF CRC-32 array */
                    
                    CHK_ORET(mxf_file_read(mxfFile, arrayHeader, 8) == 8);
                    lenRemaining -= 8;
                    mxf_get_array_header(arrayHeader, &arrayLen, &arrayItemLen);
                    CHK_ORET(arrayItemLen == 4);
                    
                    CHK_ORET(allocate_archive_crc32(reader, arrayLen));
                
                    for (i = 0; i < arrayLen; i++)
                    {
                        CHK_ORET(mxf_read_uint32(mxfFile, &crc32));
                        lenRemaining -= 4;
                        CHK_ORET(set_archive_crc32(reader, i, crc32));
                    }
                }
                else
                {
                    CHK_ORET(mxf_skip(mxfFile, localItemLen));
                    lenRemaining -= localItemLen;
                }
            }
            else
            {
                CHK_ORET(mxf_skip(mxfFile, lenRemaining));
                lenRemaining = 0;
                break;
            }
        }
        CHK_ORET(lenRemaining == 0);
    }

    /* SDTI-CP System Metadata Pack as used in D-10 for example */
    else if (mxf_equals_key(key, &MXF_EE_K(SDTI_CP_System_Pack)))
    {
        lenRemaining = len;
        if (lenRemaining >= 57)
        {
            CHK_ORET(mxf_file_read(mxfFile, systemPackData, 57) == 57);
            lenRemaining -= 57;
            
            if ((systemPackData[0] & 0x20))
            {
                /* Creation Date/Time is present */
                if (systemPackData[23] == 0x81) /* contains 12M timecode */
                {
                    convert_12m_to_timecode(&systemPackData[24], &isDropFrame, &hour, &min, &sec, &frame);
                    CHK_ORET(set_essence_container_timecode(reader, position, 
                            SYSTEM_ITEM_SDTI_CREATION_TIMECODE, 0, isDropFrame, hour, min, sec, frame));
                }
            }
            
            if ((systemPackData[0] & 0x10))
            {
                /* User Date/Time is present */
                if (systemPackData[40] == 0x81) /* contains 12M timecode */
                {
                    convert_12m_to_timecode(&systemPackData[41], &isDropFrame, &hour, &min, &sec, &frame);
                    CHK_ORET(set_essence_container_timecode(reader, position, 
                            SYSTEM_ITEM_SDTI_USER_TIMECODE, 0, isDropFrame, hour, min, sec, frame));
                }
            }
        }

        if (lenRemaining > 0)
        {
            CHK_ORET(mxf_skip(mxfFile, lenRemaining));
            lenRemaining = 0;
        }
    }
    else
    {
        /* shouldn't be here if result of element_is_known_system_item() is checked */
        CHK_ORET(mxf_skip(mxfFile, len));
    }
    
    return 1;
}


