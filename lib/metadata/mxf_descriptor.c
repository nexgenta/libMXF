/*
 * $Id: mxf_descriptor.c,v 1.1 2006/12/20 16:01:09 john_f Exp $
 *
 * MXF descriptor metadata
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
 
#include <mxf/mxf.h>
#include <mxf/mxf_metadata.h>


#define GET_OPT_SIMPLE_VALUE(set, name, SetName, ItemName, type) \
    if (mxf_have_item(set, &MXF_ITEM_K(SetName, ItemName))) \
    { \
        CHK_ORET(mxf_get_## type ## _item(set, &MXF_ITEM_K(SetName, ItemName), &name)); \
        name##_isPresent = 1; \
    } \
    else \
    { \
        name##_isPresent = 0; \
    }

#define GET_OPT_ARRAY_VALUE(set, name, SetName, ItemName, type, maxElements) \
    if (mxf_have_item(set, &MXF_ITEM_K(SetName, ItemName))) \
    { \
        uint32_t numElements; \
        uint32_t i; \
        CHK_ORET(mxf_get_array_item_count(set, &MXF_ITEM_K(SetName, ItemName), &numElements)); \
        CHK_ORET(numElements <= maxElements); \
        for (i = 0; i < numElements; i++) \
        { \
            uint8_t* data; \
            CHK_ORET(mxf_get_array_item_element(set, &MXF_ITEM_K(SetName, ItemName), i, &data)); \
            mxf_get_##type(data, &name[i]); \
        } \
        name##_size = numElements; \
        name##_isPresent = 1; \
    } \
    else \
    { \
        name##_size = 0; \
        name##_isPresent = 0; \
    }

#define GET_SIMPLE_VALUE(set, name, SetName, ItemName, type) \
    if (mxf_have_item(set, &MXF_ITEM_K(SetName, ItemName))) \
    { \
        CHK_ORET(mxf_get_## type ## _item(set, &MXF_ITEM_K(SetName, ItemName), &name)) \
    }

    
#define SET_OPT_SIMPLE_VALUE(set, name, SetName, ItemName, type) \
    if (name##_isPresent) \
    { \
        CHK_ORET(mxf_set_## type ## _item(set, &MXF_ITEM_K(SetName, ItemName), name)); \
    }

#define SET_OPT_ARRAY_VALUE(set, name, SetName, ItemName, type, elementLen) \
    if (name##_isPresent) \
    { \
        uint8_t* data; \
        uint32_t i; \
        CHK_ORET(mxf_alloc_array_item_elements(set, &MXF_ITEM_K(SetName, ItemName), \
            elementLen, name ##_size, &data)); \
        for (i = 0; i < name ##_size; i++) \
        { \
            mxf_set_##type(name[i], data); \
            data += elementLen; \
        } \
    }

#define SET_SIMPLE_VALUE(set, name, SetName, ItemName, type) \
    CHK_ORET(mxf_set_## type ## _item(set, &MXF_ITEM_K(SetName, ItemName), name))

#define SET_OPT_PSIMPLE_VALUE(set, name, SetName, ItemName, type) \
    if (name##_isPresent) \
    { \
        CHK_ORET(mxf_set_## type ## _item(set, &MXF_ITEM_K(SetName, ItemName), &name)); \
    }

#define SET_PSIMPLE_VALUE(set, name, SetName, ItemName, type) \
    CHK_ORET(mxf_set_## type ## _item(set, &MXF_ITEM_K(SetName, ItemName), &name))

    
    
    

int mxf_get_generic_descriptor(MXFMetadataSet* set, MXFGenericDescriptor* descriptor)
{
    return 1;
}

int mxf_set_generic_descriptor(MXFMetadataSet* set, MXFGenericDescriptor* descriptor)
{
    return 1;
}

int mxf_get_file_descriptor(MXFMetadataSet* set, MXFFileDescriptor* descriptor)
{
    CHK_ORET(mxf_get_generic_descriptor(set, (MXFGenericDescriptor*)descriptor));
    
    GET_OPT_SIMPLE_VALUE(set, descriptor->linkedTrackID, FileDescriptor, LinkedTrackID, uint32);
    GET_SIMPLE_VALUE(set, descriptor->sampleRate, FileDescriptor, SampleRate, rational);
    GET_OPT_SIMPLE_VALUE(set, descriptor->containerDuration, FileDescriptor, ContainerDuration, length);
    GET_SIMPLE_VALUE(set, descriptor->essenceContainer, FileDescriptor, EssenceContainer, ul);
    GET_OPT_SIMPLE_VALUE(set, descriptor->codec, FileDescriptor, Codec, ul);
    
    return 1;
}

int mxf_set_file_descriptor(MXFMetadataSet* set, MXFFileDescriptor* descriptor)
{
    CHK_ORET(mxf_set_generic_descriptor(set, (MXFGenericDescriptor*)descriptor));
    
    SET_OPT_SIMPLE_VALUE(set, descriptor->linkedTrackID, FileDescriptor, LinkedTrackID, uint32);
    SET_PSIMPLE_VALUE(set, descriptor->sampleRate, FileDescriptor, SampleRate, rational);
    SET_OPT_SIMPLE_VALUE(set, descriptor->containerDuration, FileDescriptor, ContainerDuration, length);
    SET_PSIMPLE_VALUE(set, descriptor->essenceContainer, FileDescriptor, EssenceContainer, ul);
    SET_OPT_PSIMPLE_VALUE(set, descriptor->codec, FileDescriptor, Codec, ul);
    
    return 1;
}


int mxf_get_picture_descriptor(MXFMetadataSet* set, MXFGenericPictureEssenceDescriptor* descriptor)
{
    CHK_ORET(mxf_get_file_descriptor(set, (MXFFileDescriptor*)descriptor));

    GET_OPT_SIMPLE_VALUE(set, descriptor->signalStandard, GenericPictureEssenceDescriptor, SignalStandard, uint8);
    GET_OPT_SIMPLE_VALUE(set, descriptor->frameLayout, GenericPictureEssenceDescriptor, FrameLayout, uint8);
    GET_OPT_SIMPLE_VALUE(set, descriptor->storedWidth, GenericPictureEssenceDescriptor, StoredWidth, uint32);
    GET_OPT_SIMPLE_VALUE(set, descriptor->storedHeight, GenericPictureEssenceDescriptor, StoredHeight, uint32);
    GET_OPT_SIMPLE_VALUE(set, descriptor->storedF2Offset, GenericPictureEssenceDescriptor, StoredF2Offset, int32);
    GET_OPT_SIMPLE_VALUE(set, descriptor->sampledWidth, GenericPictureEssenceDescriptor, SampledWidth, uint32);
    GET_OPT_SIMPLE_VALUE(set, descriptor->sampledHeight, GenericPictureEssenceDescriptor, SampledHeight, uint32);
    GET_OPT_SIMPLE_VALUE(set, descriptor->sampledXOffset, GenericPictureEssenceDescriptor, SampledXOffset, int32);
    GET_OPT_SIMPLE_VALUE(set, descriptor->sampledYOffset, GenericPictureEssenceDescriptor, SampledYOffset, uint32);
    GET_OPT_SIMPLE_VALUE(set, descriptor->displayHeight, GenericPictureEssenceDescriptor, DisplayHeight, uint32);
    GET_OPT_SIMPLE_VALUE(set, descriptor->displayWidth, GenericPictureEssenceDescriptor, DisplayWidth, uint32);
    GET_OPT_SIMPLE_VALUE(set, descriptor->displayXOffset, GenericPictureEssenceDescriptor, DisplayXOffset, int32);
    GET_OPT_SIMPLE_VALUE(set, descriptor->displayYOffset, GenericPictureEssenceDescriptor, DisplayYOffset, int32);
    GET_OPT_SIMPLE_VALUE(set, descriptor->displayF2Offset, GenericPictureEssenceDescriptor, DisplayF2Offset, int32);
    GET_OPT_SIMPLE_VALUE(set, descriptor->aspectRatio, GenericPictureEssenceDescriptor, AspectRatio, rational);
    GET_OPT_SIMPLE_VALUE(set, descriptor->activeFormatDescriptor, GenericPictureEssenceDescriptor, ActiveFormatDescriptor, uint8);
    GET_OPT_ARRAY_VALUE(set, descriptor->videoLineMap, GenericPictureEssenceDescriptor, VideoLineMap, uint32, 2);
    GET_OPT_SIMPLE_VALUE(set, descriptor->alphaTransparency, GenericPictureEssenceDescriptor, AlphaTransparency, uint8);
    GET_OPT_SIMPLE_VALUE(set, descriptor->gamma, GenericPictureEssenceDescriptor, CaptureGamma, ul);
    GET_OPT_SIMPLE_VALUE(set, descriptor->imageAlignmentOffset, GenericPictureEssenceDescriptor, ImageAlignmentOffset, uint32);
    GET_OPT_SIMPLE_VALUE(set, descriptor->imageStartOffset, GenericPictureEssenceDescriptor, ImageStartOffset, uint32);
    GET_OPT_SIMPLE_VALUE(set, descriptor->imageEndOffset, GenericPictureEssenceDescriptor, ImageEndOffset, uint32);
    GET_OPT_SIMPLE_VALUE(set, descriptor->fieldDominance, GenericPictureEssenceDescriptor, FieldDominance, uint8);
    GET_OPT_SIMPLE_VALUE(set, descriptor->pictureEssenceCoding, GenericPictureEssenceDescriptor, PictureEssenceCoding, ul);
    
    return 1;    
}

int mxf_set_picture_descriptor(MXFMetadataSet* set, MXFGenericPictureEssenceDescriptor* descriptor)
{
    CHK_ORET(mxf_set_file_descriptor(set, (MXFFileDescriptor*)descriptor));

    SET_OPT_SIMPLE_VALUE(set, descriptor->signalStandard, GenericPictureEssenceDescriptor, SignalStandard, uint8);
    SET_OPT_SIMPLE_VALUE(set, descriptor->frameLayout, GenericPictureEssenceDescriptor, FrameLayout, uint8);
    SET_OPT_SIMPLE_VALUE(set, descriptor->storedWidth, GenericPictureEssenceDescriptor, StoredWidth, uint32);
    SET_OPT_SIMPLE_VALUE(set, descriptor->storedHeight, GenericPictureEssenceDescriptor, StoredHeight, uint32);
    SET_OPT_SIMPLE_VALUE(set, descriptor->storedF2Offset, GenericPictureEssenceDescriptor, StoredF2Offset, int32);
    SET_OPT_SIMPLE_VALUE(set, descriptor->sampledWidth, GenericPictureEssenceDescriptor, SampledWidth, uint32);
    SET_OPT_SIMPLE_VALUE(set, descriptor->sampledHeight, GenericPictureEssenceDescriptor, SampledHeight, uint32);
    SET_OPT_SIMPLE_VALUE(set, descriptor->sampledXOffset, GenericPictureEssenceDescriptor, SampledXOffset, int32);
    SET_OPT_SIMPLE_VALUE(set, descriptor->sampledYOffset, GenericPictureEssenceDescriptor, SampledYOffset, uint32);
    SET_OPT_SIMPLE_VALUE(set, descriptor->displayHeight, GenericPictureEssenceDescriptor, DisplayHeight, uint32);
    SET_OPT_SIMPLE_VALUE(set, descriptor->displayWidth, GenericPictureEssenceDescriptor, DisplayWidth, uint32);
    SET_OPT_SIMPLE_VALUE(set, descriptor->displayXOffset, GenericPictureEssenceDescriptor, DisplayXOffset, int32);
    SET_OPT_SIMPLE_VALUE(set, descriptor->displayYOffset, GenericPictureEssenceDescriptor, DisplayYOffset, int32);
    SET_OPT_SIMPLE_VALUE(set, descriptor->displayF2Offset, GenericPictureEssenceDescriptor, DisplayF2Offset, int32);
    SET_OPT_PSIMPLE_VALUE(set, descriptor->aspectRatio, GenericPictureEssenceDescriptor, AspectRatio, rational);
    SET_OPT_SIMPLE_VALUE(set, descriptor->activeFormatDescriptor, GenericPictureEssenceDescriptor, ActiveFormatDescriptor, uint8);
    SET_OPT_ARRAY_VALUE(set, descriptor->videoLineMap, GenericPictureEssenceDescriptor, VideoLineMap, uint32, 4);
    SET_OPT_SIMPLE_VALUE(set, descriptor->alphaTransparency, GenericPictureEssenceDescriptor, AlphaTransparency, uint8);
    SET_OPT_PSIMPLE_VALUE(set, descriptor->gamma, GenericPictureEssenceDescriptor, CaptureGamma, ul);
    SET_OPT_SIMPLE_VALUE(set, descriptor->imageAlignmentOffset, GenericPictureEssenceDescriptor, ImageAlignmentOffset, uint32);
    SET_OPT_SIMPLE_VALUE(set, descriptor->imageStartOffset, GenericPictureEssenceDescriptor, ImageStartOffset, uint32);
    SET_OPT_SIMPLE_VALUE(set, descriptor->imageEndOffset, GenericPictureEssenceDescriptor, ImageEndOffset, uint32);
    SET_OPT_SIMPLE_VALUE(set, descriptor->fieldDominance, GenericPictureEssenceDescriptor, FieldDominance, uint8);
    SET_OPT_PSIMPLE_VALUE(set, descriptor->pictureEssenceCoding, GenericPictureEssenceDescriptor, PictureEssenceCoding, ul);
    
    return 1;    
}


int mxf_get_cdci_descriptor(MXFMetadataSet* set, MXFCDCIDescriptor* descriptor)
{
    CHK_ORET(mxf_get_picture_descriptor(set, (MXFGenericPictureEssenceDescriptor*)descriptor));

    GET_OPT_SIMPLE_VALUE(set, descriptor->componentDepth, CDCIEssenceDescriptor, ComponentDepth, uint32);
    GET_OPT_SIMPLE_VALUE(set, descriptor->horizontalSubSampling, CDCIEssenceDescriptor, HorizontalSubsampling, uint32);
    GET_OPT_SIMPLE_VALUE(set, descriptor->verticalSubSampling, CDCIEssenceDescriptor, VerticalSubsampling, uint32);
    GET_OPT_SIMPLE_VALUE(set, descriptor->colorSiting, CDCIEssenceDescriptor, ColorSiting, uint8);
    GET_OPT_SIMPLE_VALUE(set, descriptor->reversedByteOrder, CDCIEssenceDescriptor, ReversedByteOrder, boolean);
    GET_OPT_SIMPLE_VALUE(set, descriptor->paddingBits, CDCIEssenceDescriptor, PaddingBits, int16);
    GET_OPT_SIMPLE_VALUE(set, descriptor->alphaSampleDepth, CDCIEssenceDescriptor, AlphaSampleDepth, uint32);
    GET_OPT_SIMPLE_VALUE(set, descriptor->blackRefLevel, CDCIEssenceDescriptor, BlackRefLevel, uint32);
    GET_OPT_SIMPLE_VALUE(set, descriptor->whiteRefLevel, CDCIEssenceDescriptor, WhiteReflevel, uint32);
    GET_OPT_SIMPLE_VALUE(set, descriptor->colorRange, CDCIEssenceDescriptor, ColorRange, uint32);
    
    return 1;    
}

int mxf_set_cdci_descriptor(MXFMetadataSet* set, MXFCDCIDescriptor* descriptor)
{
    CHK_ORET(mxf_set_picture_descriptor(set, (MXFGenericPictureEssenceDescriptor*)descriptor));

    SET_OPT_SIMPLE_VALUE(set, descriptor->componentDepth, CDCIEssenceDescriptor, ComponentDepth, uint32);
    SET_OPT_SIMPLE_VALUE(set, descriptor->horizontalSubSampling, CDCIEssenceDescriptor, HorizontalSubsampling, uint32);
    SET_OPT_SIMPLE_VALUE(set, descriptor->verticalSubSampling, CDCIEssenceDescriptor, VerticalSubsampling, uint32);
    SET_OPT_SIMPLE_VALUE(set, descriptor->colorSiting, CDCIEssenceDescriptor, ColorSiting, uint8);
    SET_OPT_SIMPLE_VALUE(set, descriptor->reversedByteOrder, CDCIEssenceDescriptor, ReversedByteOrder, boolean);
    SET_OPT_SIMPLE_VALUE(set, descriptor->paddingBits, CDCIEssenceDescriptor, PaddingBits, int16);
    SET_OPT_SIMPLE_VALUE(set, descriptor->alphaSampleDepth, CDCIEssenceDescriptor, AlphaSampleDepth, uint32);
    SET_OPT_SIMPLE_VALUE(set, descriptor->blackRefLevel, CDCIEssenceDescriptor, BlackRefLevel, uint32);
    SET_OPT_SIMPLE_VALUE(set, descriptor->whiteRefLevel, CDCIEssenceDescriptor, WhiteReflevel, uint32);
    SET_OPT_SIMPLE_VALUE(set, descriptor->colorRange, CDCIEssenceDescriptor, ColorRange, uint32);
    
    return 1;    
}


int mxf_get_sound_descriptor(MXFMetadataSet* set, MXFGenericSoundEssenceDescriptor* descriptor)
{
    CHK_ORET(mxf_get_file_descriptor(set, (MXFFileDescriptor*)descriptor));

    GET_OPT_SIMPLE_VALUE(set, descriptor->audioSamplingRate, GenericSoundEssenceDescriptor, AudioSamplingRate, rational);
    GET_OPT_SIMPLE_VALUE(set, descriptor->locked, GenericSoundEssenceDescriptor, Locked, boolean);
    GET_OPT_SIMPLE_VALUE(set, descriptor->audioRefLevel, GenericSoundEssenceDescriptor, AudioRefLevel, int8);
    GET_OPT_SIMPLE_VALUE(set, descriptor->electroSpatialFormulation, GenericSoundEssenceDescriptor, ElectroSpatialFormulation, uint8);
    GET_OPT_SIMPLE_VALUE(set, descriptor->channelCount, GenericSoundEssenceDescriptor, ChannelCount, uint32);
    GET_OPT_SIMPLE_VALUE(set, descriptor->quantizationBits, GenericSoundEssenceDescriptor, QuantizationBits, uint32);
    GET_OPT_SIMPLE_VALUE(set, descriptor->dialNorm, GenericSoundEssenceDescriptor, DialNorm, int8);
    GET_OPT_SIMPLE_VALUE(set, descriptor->soundEssenceCompression, GenericSoundEssenceDescriptor, SoundEssenceCompression, ul);

    return 1;    
}

int mxf_set_sound_descriptor(MXFMetadataSet* set, MXFGenericSoundEssenceDescriptor* descriptor)
{
    CHK_ORET(mxf_set_file_descriptor(set, (MXFFileDescriptor*)descriptor));

    SET_OPT_PSIMPLE_VALUE(set, descriptor->audioSamplingRate, GenericSoundEssenceDescriptor, AudioSamplingRate, rational);
    SET_OPT_SIMPLE_VALUE(set, descriptor->locked, GenericSoundEssenceDescriptor, Locked, boolean);
    SET_OPT_SIMPLE_VALUE(set, descriptor->audioRefLevel, GenericSoundEssenceDescriptor, AudioRefLevel, int8);
    SET_OPT_SIMPLE_VALUE(set, descriptor->electroSpatialFormulation, GenericSoundEssenceDescriptor, ElectroSpatialFormulation, uint8);
    SET_OPT_SIMPLE_VALUE(set, descriptor->channelCount, GenericSoundEssenceDescriptor, ChannelCount, uint32);
    SET_OPT_SIMPLE_VALUE(set, descriptor->quantizationBits, GenericSoundEssenceDescriptor, QuantizationBits, uint32);
    SET_OPT_SIMPLE_VALUE(set, descriptor->dialNorm, GenericSoundEssenceDescriptor, DialNorm, int8);
    SET_OPT_PSIMPLE_VALUE(set, descriptor->soundEssenceCompression, GenericSoundEssenceDescriptor, SoundEssenceCompression, ul);

    return 1;    
}


int mxf_get_wave_descriptor(MXFMetadataSet* set, MXFWaveAudioDescriptor* descriptor)
{
    CHK_ORET(mxf_get_sound_descriptor(set, (MXFGenericSoundEssenceDescriptor*)descriptor));

    GET_SIMPLE_VALUE(set, descriptor->blockAlign, WaveAudioDescriptor, BlockAlign, uint16);
    GET_OPT_SIMPLE_VALUE(set, descriptor->sequenceOffset, WaveAudioDescriptor, SequenceOffset, uint8);
    GET_SIMPLE_VALUE(set, descriptor->avgBps, WaveAudioDescriptor, AvgBps, uint32);

    return 1;    
}

int mxf_set_wave_descriptor(MXFMetadataSet* set, MXFWaveAudioDescriptor* descriptor)
{
    CHK_ORET(mxf_set_sound_descriptor(set, (MXFGenericSoundEssenceDescriptor*)descriptor));

    SET_SIMPLE_VALUE(set, descriptor->blockAlign, WaveAudioDescriptor, BlockAlign, uint16);
    SET_OPT_SIMPLE_VALUE(set, descriptor->sequenceOffset, WaveAudioDescriptor, SequenceOffset, uint8);
    SET_SIMPLE_VALUE(set, descriptor->avgBps, WaveAudioDescriptor, AvgBps, uint32);

    return 1;    
}


