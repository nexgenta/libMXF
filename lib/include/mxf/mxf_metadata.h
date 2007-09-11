/*
 * $Id: mxf_metadata.h,v 1.2 2007/09/11 13:24:54 stuart_hc Exp $
 *
 * MXF metadata
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
 
#ifndef __MXF_METADATA_H__
#define __MXF_METADATA_H__



#ifdef __cplusplus
extern "C" 
{
#endif


#define OPTIONAL(type, name) \
    type name; \
    int name ## _isPresent;
    
#define OPTIONAL_ARRAY(type, name, maxElements) \
    type name[maxElements]; \
    uint32_t name ##_size; \
    int name ## _isPresent;

#define OPTIONAL_VARARRAY(type, name, maxElements) \
    type* name; \
    uint32_t name ##_size; \
    int name ## _isPresent;

#define VARARRAY(type, name) \
    type* name; \
    uint32_t name ##_size;


#define SET_OPTIONAL_VALUE(name, val) \
    name##_isPresent = 1; \
    name = val;

/* Note: setting values with index > maxElements will be ignored */     
#define SET_OPTIONAL_ARRAY_VALUE(name, index, val) \
    if (index < sizeof(name)) \
    { \
        name##_isPresent = 1; \
        name[index] = val; \
        name##_size = (index + 1 > name##_size) ? index + 1 : name##_size; \
    }

#define UNSET_OPTIONAL_VALUE(name) \
    name##_isPresent = 0; \
    name = 0;



#define INTERCHANGE_OBJECT_ITEMS

typedef struct 
{
    INTERCHANGE_OBJECT_ITEMS;
    
} MXFInterchangeObject;


#define LOCATOR_ITEMS \
    INTERCHANGE_OBJECT_ITEMS;

typedef struct
{
    LOCATOR_ITEMS;
    
} MXFLocator;


#define NETWORK_LOCATOR_ITEMS \
    LOCATOR_ITEMS; \
    mxfUTF16Char* urlString;    

typedef struct
{
    NETWORK_LOCATOR_ITEMS;
} MXFNetworkLocator;


#define TEXT_LOCATOR_ITEMS \
    LOCATOR_ITEMS; \
    mxfUTF16Char* locatorName;    

typedef struct
{
    TEXT_LOCATOR_ITEMS;
} MXFTextLocator;


#define GENERIC_DESCRIPTOR_ITEMS \
    INTERCHANGE_OBJECT_ITEMS;
    /*locators*/

typedef struct 
{
    GENERIC_DESCRIPTOR_ITEMS;
} MXFGenericDescriptor;


#define FILE_DESCRIPTOR_ITEMS \
    GENERIC_DESCRIPTOR_ITEMS; \
    OPTIONAL(uint32_t, linkedTrackID); \
    mxfRational sampleRate; \
    OPTIONAL(mxfLength, containerDuration); \
    mxfUL essenceContainer; \
    OPTIONAL(mxfUL, codec);
    
typedef struct
{
    FILE_DESCRIPTOR_ITEMS;
} MXFFileDescriptor;


#define GENERIC_PICTURE_ESSENCE_DESCRIPTOR_ITEMS \
    FILE_DESCRIPTOR_ITEMS; \
    OPTIONAL(uint8_t, signalStandard); \
    OPTIONAL(uint8_t, frameLayout); \
    OPTIONAL(uint32_t, storedWidth); \
    OPTIONAL(uint32_t, storedHeight); \
    OPTIONAL(int32_t, storedF2Offset); \
    OPTIONAL(uint32_t, sampledWidth); \
    OPTIONAL(uint32_t, sampledHeight); \
    OPTIONAL(int32_t, sampledXOffset); \
    OPTIONAL(uint32_t, sampledYOffset); \
    OPTIONAL(uint32_t, displayHeight); \
    OPTIONAL(uint32_t, displayWidth); \
    OPTIONAL(int32_t, displayXOffset); \
    OPTIONAL(int32_t, displayYOffset); \
    OPTIONAL(int32_t, displayF2Offset); \
    OPTIONAL(mxfRational, aspectRatio); \
    OPTIONAL(uint8_t, activeFormatDescriptor); \
    OPTIONAL(uint8_t, alphaTransparency); \
    OPTIONAL(mxfUL, gamma); \
    OPTIONAL(uint32_t, imageAlignmentOffset); \
    OPTIONAL(uint32_t, imageStartOffset); \
    OPTIONAL(uint32_t, imageEndOffset); \
    OPTIONAL(uint8_t, fieldDominance); \
    OPTIONAL(mxfUL, pictureEssenceCoding); \
    OPTIONAL_ARRAY(uint32_t, videoLineMap, 16); /* is 16 enough ? */
    
typedef struct
{
    GENERIC_PICTURE_ESSENCE_DESCRIPTOR_ITEMS;
} MXFGenericPictureEssenceDescriptor;


#define CDCI_DESCRIPTOR_ITEMS \
    GENERIC_PICTURE_ESSENCE_DESCRIPTOR_ITEMS; \
    OPTIONAL(uint32_t, componentDepth); \
    OPTIONAL(uint32_t, horizontalSubSampling); \
    OPTIONAL(uint32_t, verticalSubSampling); \
    OPTIONAL(uint8_t, colorSiting); \
    OPTIONAL(mxfBoolean, reversedByteOrder); \
    OPTIONAL(int16_t, paddingBits); \
    OPTIONAL(uint32_t, alphaSampleDepth); \
    OPTIONAL(uint32_t, blackRefLevel); \
    OPTIONAL(uint32_t, whiteRefLevel); \
    OPTIONAL(uint32_t, colorRange);
    
typedef struct
{
    CDCI_DESCRIPTOR_ITEMS;
} MXFCDCIDescriptor;


#define GENERIC_SOUND_ESSENCE_DESCRIPTOR_ITEMS \
    FILE_DESCRIPTOR_ITEMS; \
    OPTIONAL(mxfRational, audioSamplingRate); \
    OPTIONAL(mxfBoolean, locked); \
    OPTIONAL(int8_t, audioRefLevel); \
    OPTIONAL(uint8_t, electroSpatialFormulation); \
    OPTIONAL(uint32_t, channelCount); \
    OPTIONAL(uint32_t, quantizationBits); \
    OPTIONAL(int8_t, dialNorm); \
    OPTIONAL(mxfUL, soundEssenceCompression);
    
typedef struct
{
    GENERIC_SOUND_ESSENCE_DESCRIPTOR_ITEMS;
} MXFGenericSoundEssenceDescriptor;


#define WAVE_AUDIO_DESCRIPTOR_ITEMS \
    GENERIC_SOUND_ESSENCE_DESCRIPTOR_ITEMS; \
    uint16_t blockAlign; \
    OPTIONAL(uint8_t, sequenceOffset); \
    uint32_t avgBps;
    //OPTIONAL(mxfUL, channelAssignment);
    //OPTIONAL(uint32_t, peakEnvelopeVersion);
    //OPTIONAL(uint32_t, peakEnvelopeFormat);
    //OPTIONAL(uint32_t, pointsPerPeakValue);
    //OPTIONAL(uint32_t, peakEnvelopeBlockSize);
    //OPTIONAL(uint32_t, peakChannels);
    //OPTIONAL(uint32_t, peakFrames);
    //OPTIONAL(mxfPosition, peakOfPeaksPosition);
    //OPTIONAL(mxfTimestamp, peakEnvelopeTimestamp);
    /* peakEnvelopeData */
    
typedef struct
{
    WAVE_AUDIO_DESCRIPTOR_ITEMS;
} MXFWaveAudioDescriptor;


#define MULTIPLE_DESCRIPTOR_ITEMS \
    FILE_DESCRIPTOR_ITEMS; \
    /*subDescriptorUIDs*/

typedef struct
{
    MULTIPLE_DESCRIPTOR_ITEMS;
} MXFMultipleDescriptor;



int mxf_get_generic_descriptor(MXFMetadataSet* set, MXFGenericDescriptor* descriptor);
int mxf_set_generic_descriptor(MXFMetadataSet* set, MXFGenericDescriptor* descriptor);
void mxf_clear_generic_descriptor(MXFGenericDescriptor* descriptor); /* TODO */

int mxf_get_file_descriptor(MXFMetadataSet* set, MXFFileDescriptor* descriptor);
int mxf_set_file_descriptor(MXFMetadataSet* set, MXFFileDescriptor* descriptor);
void mxf_clear_file_descriptor(MXFFileDescriptor* descriptor); /* TODO */

int mxf_get_picture_descriptor(MXFMetadataSet* set, MXFGenericPictureEssenceDescriptor* descriptor);
int mxf_set_picture_descriptor(MXFMetadataSet* set, MXFGenericPictureEssenceDescriptor* descriptor);
void mxf_clear_picture_descriptor(MXFGenericPictureEssenceDescriptor* descriptor); /* TODO */

int mxf_get_cdci_descriptor(MXFMetadataSet* set, MXFCDCIDescriptor* descriptor);
int mxf_set_cdci_descriptor(MXFMetadataSet* set, MXFCDCIDescriptor* descriptor);
void mxf_clear_cdci_descriptor(MXFCDCIDescriptor* descriptor); /* TODO */


int mxf_get_sound_descriptor(MXFMetadataSet* set, MXFGenericSoundEssenceDescriptor* descriptor);
int mxf_set_sound_descriptor(MXFMetadataSet* set, MXFGenericSoundEssenceDescriptor* descriptor);
void mxf_clear_sound_descriptor(MXFGenericSoundEssenceDescriptor* descriptor); /* TODO */

int mxf_get_wave_descriptor(MXFMetadataSet* set, MXFWaveAudioDescriptor* descriptor);
int mxf_set_wave_descriptor(MXFMetadataSet* set, MXFWaveAudioDescriptor* descriptor);
void mxf_clear_wave_descriptor(MXFWaveAudioDescriptor* descriptor); /* TODO */


#define GENERIC_PACKAGE_ITEMS \
    INTERCHANGE_OBJECT_ITEMS; \
    mxfUMID packageUID; \
    OPTIONAL(mxfUTF16Char*, name); \
    mxfTimestamp packageCreationDate; \
    mxfTimestamp packageModifiedDate; \
    VARARRAY(mxfUUID, tracks);
    
typedef struct
{
    GENERIC_PACKAGE_ITEMS;
} MXFGenericPackage;

#define MATERIAL_PACKAGE_ITEMS \
    GENERIC_PACKAGE_ITEMS;
    
typedef struct
{
    MATERIAL_PACKAGE_ITEMS;
} MXFMaterialPackage;

#define SOURCE_PACKAGE_ITEMS \
    GENERIC_PACKAGE_ITEMS; \
    mxfUUID descriptor;
    
typedef struct
{
    SOURCE_PACKAGE_ITEMS;
} MXFSourcePackage;


int mxf_get_generic_package(MXFMetadataSet* set, MXFGenericPackage* genericPackage);
int mxf_set_generic_package(MXFMetadataSet* set, MXFGenericPackage* genericPackage);
void mxf_clear_generic_package(MXFGenericPackage* genericPackage);

int mxf_get_material_package(MXFMetadataSet* set, MXFMaterialPackage* materialPackage);
int mxf_set_material_package(MXFMetadataSet* set, MXFMaterialPackage* materialPackage);
void mxf_clear_material_package(MXFMaterialPackage* materialPackage);

int mxf_get_source_package(MXFMetadataSet* set, MXFSourcePackage* sourcePackage);
int mxf_set_source_package(MXFMetadataSet* set, MXFSourcePackage* sourcePackage);
void mxf_clear_source_package(MXFSourcePackage* sourcePackage);


#define GENERIC_TRACK_ITEMS \
    INTERCHANGE_OBJECT_ITEMS; \
    uint32_t trackID; \
    uint32_t trackNumber; \
    OPTIONAL(mxfUTF16Char*, trackName); \
    mxfUUID sequence;
    
typedef struct
{
    GENERIC_TRACK_ITEMS;
} MXFGenericTrack;

#define TRACK_ITEMS \
    GENERIC_TRACK_ITEMS; \
    mxfRational editRate; \
    mxfPosition origin;
    
typedef struct
{
    TRACK_ITEMS;
} MXFTrack;


int mxf_get_generic_track(MXFMetadataSet* set, MXFGenericTrack* genericTrack);
int mxf_set_generic_track(MXFMetadataSet* set, MXFGenericTrack* genericTrack);
void mxf_clear_generic_track(MXFGenericTrack* genericTrack);

int mxf_get_track(MXFMetadataSet* set, MXFTrack* track);
int mxf_set_track(MXFMetadataSet* set, MXFTrack* track);
void mxf_clear_track(MXFTrack* track);



#define STRUCTURAL_COMPONENT_ITEMS \
    INTERCHANGE_OBJECT_ITEMS; \
    mxfUL dataDefinition; \
    OPTIONAL(mxfLength, duration);
    
typedef struct
{
    STRUCTURAL_COMPONENT_ITEMS;
} MXFStructuralComponent;

#define SEQUENCE_ITEMS \
    STRUCTURAL_COMPONENT_ITEMS; \
    VARARRAY(mxfUUID, structuralComponents);
    
typedef struct
{
    SEQUENCE_ITEMS;
} MXFSequence;

#define SOURCE_CLIP_ITEMS \
    STRUCTURAL_COMPONENT_ITEMS; \
    mxfPosition startPosition; \
    mxfUMID sourcePackageID; \
    uint32_t sourceTrackID;
    
typedef struct
{
    SOURCE_CLIP_ITEMS;
} MXFSourceClip;


int mxf_get_structural_component(MXFMetadataSet* set, MXFStructuralComponent* structuralComponent);
int mxf_set_structural_component(MXFMetadataSet* set, MXFStructuralComponent* structuralComponent);
void mxf_clear_structural_component(MXFStructuralComponent* structuralComponent);

int mxf_get_sequence(MXFMetadataSet* set, MXFSequence* sequence);
int mxf_set_sequence(MXFMetadataSet* set, MXFSequence* sequence);
void mxf_clear_sequence(MXFSequence* sequence);

int mxf_get_source_clip(MXFMetadataSet* set, MXFSourceClip* sourceClip);
int mxf_set_source_clip(MXFMetadataSet* set, MXFSourceClip* sourceClip);
void mxf_clear_source_clip(MXFSourceClip* sourceClip);



#ifdef __cplusplus
}
#endif


#endif


