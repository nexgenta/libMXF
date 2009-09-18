/*
 * $Id: writeaviddv50.c,v 1.5 2009/09/18 14:39:15 philipn Exp $
 *
 * Example showing how to create Avid supported MXF OP-Atom files containing DV-50 
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
 
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mxf/mxf.h>
#include <mxf/mxf_avid.h>


static const mxfUUID g_WrapDV50ProductUID_uuid = 
    {0x86, 0x78, 0xf7, 0x50, 0x59, 0x4d, 0x42, 0x2a, 0x98, 0x49, 0x66, 0x7a, 0xc0, 0xee, 0xfa, 0xe0};

    
    
/*
 * Write DV-50 essence data to an MXF Op Atom file
 *
 * The MXF file consists of the following parts (in order):
 *  - header partition pack
 *  - primer pack
 *  - header metadata sets
 *  - body partition pack
 *  - dv50 clip wrapped essence element
 *  - footer partition pack
 *  - index table segment
 *  - random index pack
 *
 */

int write_dv50(FILE* dv50File, MXFFile* mxfFile, int test)
{
    MXFFilePartitions partitions;
    MXFPartition* headerPartition;
    MXFPartition* bodyPartition;
    MXFPartition* footerPartition;
    MXFHeaderMetadata* headerMetadata = NULL;
    MXFMetadataSet* metaDictSet = NULL;
    MXFMetadataSet* prefaceSet = NULL;
    MXFMetadataSet* identSet = NULL;
    MXFMetadataSet* contentStorageSet = NULL;
    MXFMetadataSet* sourcePackageSet = NULL;
    MXFMetadataSet* materialPackageSet = NULL;
    MXFMetadataSet* sourcePackageTrackSet = NULL;
    MXFMetadataSet* materialPackageTrackSet = NULL;
    MXFMetadataSet* sequenceSet = NULL;
    MXFMetadataSet* sourceClipSet = NULL;
    MXFMetadataSet* essContainerDataSet = NULL;
    MXFMetadataSet* cdciDescriptorSet = NULL;
    MXFDataModel* dataModel = NULL;
    MXFIndexTableSegment* indexSegment = NULL;
    MXFEssenceElement* essenceElement = NULL;
    MXFMetadataItem* durationItem1 = NULL;
    MXFMetadataItem* durationItem2 = NULL;
    MXFMetadataItem* durationItem3 = NULL;
    MXFMetadataItem* durationItem4 = NULL;
    MXFMetadataItem* durationItem5 = NULL;
    MXFMetadataItem* imageSizeItem = NULL;
    mxfTimestamp now;
    uint32_t bodySID = 1;
    uint32_t indexSID = 2;
    mxfUUID thisGeneration;
    mxfUTF16Char* companyName = L"BBC Research";
    mxfUTF16Char* productName = L"Write Avid DV-50 example";
    mxfUTF16Char* versionString = L"Alpha version";
    mxfUMID sourcePackageUMID;
    mxfUMID materialPackageUMID;
    uint32_t sourceTrackID = 1;
    uint32_t sourceTrackNumber = 0x18010201;
    mxfRational sampleRate = {25, 1};
    mxfRational editRate = sampleRate;
    mxfLength duration = 0;
    mxfRational aspectRatio = {4, 3};
    mxfUUID indexSegmentUUID;
    uint32_t frameSize = 288000;
    int32_t imageSize = 0;
    int32_t resolutionID = 0x8e;
    const uint32_t essenceBufferSize = 4096;
    uint8_t buffer[4096];
    int done = 0;
    uint8_t* arrayElement;
    int64_t headerMetadataPos;
    
    
    mxf_generate_uuid(&thisGeneration);
    mxf_get_timestamp_now(&now);
    /* Older Avids could fail when given files with UMIDs generated using
       other methods. (Note: not 100% sure this is true) */
    mxf_generate_aafsdk_umid(&sourcePackageUMID);
    mxf_generate_aafsdk_umid(&materialPackageUMID);
    mxf_generate_uuid(&indexSegmentUUID);
    
    mxf_initialise_file_partitions(&partitions);

    
    /* set the minimum llen */
    mxf_file_set_min_llen(mxfFile, 4);
    
    
    /* load the data model, plus AVID extensions */
    
    CHK_ORET(mxf_load_data_model(&dataModel));
    CHK_ORET(mxf_avid_load_extensions(dataModel));
    CHK_ORET(mxf_finalise_data_model(dataModel));
    
    
    
    /* write the header partition pack */

    CHK_ORET(mxf_append_new_partition(&partitions, &headerPartition));
    headerPartition->key = MXF_PP_K(ClosedComplete, Header);
    headerPartition->majorVersion = 1;
    headerPartition->minorVersion = 2;
    headerPartition->kagSize = 0x100;
    headerPartition->operationalPattern = MXF_OP_L(atom, complexity02);
    CHK_ORET(mxf_append_partition_esscont_label(headerPartition, &MXF_EC_L(DVBased_50_625_50_ClipWrapped)));
    
    CHK_ORET(mxf_write_partition(mxfFile, headerPartition));
    CHK_ORET(mxf_fill_to_kag(mxfFile, headerPartition));
    

    
    /* create the header metadata */
    
    CHK_ORET(mxf_create_header_metadata(&headerMetadata, dataModel));
    
    
    /* create the Avid meta-dictionary */
    
    CHK_ORET(mxf_avid_create_default_metadictionary(headerMetadata, &metaDictSet));
    
    
    /* Preface */
    CHK_ORET(mxf_create_set(headerMetadata, &MXF_SET_K(Preface), &prefaceSet));
    CHK_ORET(mxf_set_timestamp_item(prefaceSet, &MXF_ITEM_K(Preface, LastModifiedDate), &now));
    CHK_ORET(mxf_set_version_type_item(prefaceSet, &MXF_ITEM_K(Preface, Version), 0x0102));
    CHK_ORET(mxf_set_ul_item(prefaceSet, &MXF_ITEM_K(Preface, OperationalPattern), &MXF_OP_L(atom, complexity02)));
    CHK_ORET(mxf_alloc_array_item_elements(prefaceSet, &MXF_ITEM_K(Preface, EssenceContainers), mxfUL_extlen, 1, &arrayElement));
    mxf_set_ul(&MXF_EC_L(DVBased_50_625_50_ClipWrapped), arrayElement);

    
    /* Preface - Identification */
    CHK_ORET(mxf_create_set(headerMetadata, &MXF_SET_K(Identification), &identSet));
    CHK_ORET(mxf_add_array_item_strongref(prefaceSet, &MXF_ITEM_K(Preface, Identifications), identSet));
    CHK_ORET(mxf_set_uuid_item(identSet, &MXF_ITEM_K(Identification, ThisGenerationUID), &thisGeneration));
    CHK_ORET(mxf_set_utf16string_item(identSet, &MXF_ITEM_K(Identification, CompanyName), companyName));
    CHK_ORET(mxf_set_utf16string_item(identSet, &MXF_ITEM_K(Identification, ProductName), productName));
    CHK_ORET(mxf_set_utf16string_item(identSet, &MXF_ITEM_K(Identification, VersionString), versionString));
    CHK_ORET(mxf_set_uuid_item(identSet, &MXF_ITEM_K(Identification, ProductUID), &g_WrapDV50ProductUID_uuid));
    CHK_ORET(mxf_set_timestamp_item(identSet, &MXF_ITEM_K(Identification, ModificationDate), &now));
    CHK_ORET(mxf_set_product_version_item(identSet, &MXF_ITEM_K(Identification, ToolkitVersion), mxf_get_version()));
    if (test)
    {
        /* use the same string on all platforms to make checking diffs easier */
        CHK_ORET(mxf_set_utf16string_item(identSet, &MXF_ITEM_K(Identification, Platform), L"test platform string"));
    }
    else
    {
        CHK_ORET(mxf_set_utf16string_item(identSet, &MXF_ITEM_K(Identification, Platform), mxf_get_platform_wstring()));
    }
    
    
    /* Preface - ContentStorage */
    CHK_ORET(mxf_create_set(headerMetadata, &MXF_SET_K(ContentStorage), &contentStorageSet));
    CHK_ORET(mxf_set_strongref_item(prefaceSet, &MXF_ITEM_K(Preface, ContentStorage), contentStorageSet));
    
    
    /* Preface - ContentStorage - MaterialPackage */
    CHK_ORET(mxf_create_set(headerMetadata, &MXF_SET_K(MaterialPackage), &materialPackageSet));
    CHK_ORET(mxf_add_array_item_strongref(contentStorageSet, &MXF_ITEM_K(ContentStorage, Packages), materialPackageSet));
    CHK_ORET(mxf_set_umid_item(materialPackageSet, &MXF_ITEM_K(GenericPackage, PackageUID), &materialPackageUMID));
    CHK_ORET(mxf_set_timestamp_item(materialPackageSet, &MXF_ITEM_K(GenericPackage, PackageCreationDate), &now));
    CHK_ORET(mxf_set_timestamp_item(materialPackageSet, &MXF_ITEM_K(GenericPackage, PackageModifiedDate), &now));
    CHK_ORET(mxf_set_utf16string_item(materialPackageSet, &MXF_ITEM_K(GenericPackage, Name), L"writedv50 material"));

    /* Preface - ContentStorage - MaterialPackage - Timeline Track */    
    CHK_ORET(mxf_create_set(headerMetadata, &MXF_SET_K(Track), &materialPackageTrackSet));
    CHK_ORET(mxf_add_array_item_strongref(materialPackageSet, &MXF_ITEM_K(GenericPackage, Tracks), materialPackageTrackSet));
    CHK_ORET(mxf_set_uint32_item(materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackID), sourceTrackID));
    CHK_ORET(mxf_set_uint32_item(materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), sourceTrackNumber));
    CHK_ORET(mxf_set_rational_item(materialPackageTrackSet, &MXF_ITEM_K(Track, EditRate), &editRate));
    CHK_ORET(mxf_set_position_item(materialPackageTrackSet, &MXF_ITEM_K(Track, Origin), 0));

    /* Preface - ContentStorage - MaterialPackage - Timeline Track - Sequence */    
    CHK_ORET(mxf_create_set(headerMetadata, &MXF_SET_K(Sequence), &sequenceSet));
    CHK_ORET(mxf_set_strongref_item(materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), sequenceSet));
    CHK_ORET(mxf_set_ul_item(sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(LegacyPicture)));
    CHK_ORET(mxf_set_length_item(sequenceSet, &MXF_ITEM_K(StructuralComponent, Duration), duration));

    CHK_ORET(mxf_get_item(sequenceSet, &MXF_ITEM_K(StructuralComponent, Duration), &durationItem1));

    /* Preface - ContentStorage - MaterialPackage - Timeline Track - Sequence - SourceClip */    
    CHK_ORET(mxf_create_set(headerMetadata, &MXF_SET_K(SourceClip), &sourceClipSet));
    CHK_ORET(mxf_add_array_item_strongref(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), sourceClipSet));
    CHK_ORET(mxf_set_ul_item(sourceClipSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(LegacyPicture)));
    CHK_ORET(mxf_set_length_item(sourceClipSet, &MXF_ITEM_K(StructuralComponent, Duration), duration));
    CHK_ORET(mxf_set_position_item(sourceClipSet, &MXF_ITEM_K(SourceClip, StartPosition), 0));
    CHK_ORET(mxf_set_umid_item(sourceClipSet, &MXF_ITEM_K(SourceClip, SourcePackageID), &sourcePackageUMID));
    CHK_ORET(mxf_set_uint32_item(sourceClipSet, &MXF_ITEM_K(SourceClip, SourceTrackID), sourceTrackID));

    CHK_ORET(mxf_get_item(sourceClipSet, &MXF_ITEM_K(StructuralComponent, Duration), &durationItem2));
    
    
    /* Preface - ContentStorage - SourcePackage */
    CHK_ORET(mxf_create_set(headerMetadata, &MXF_SET_K(SourcePackage), &sourcePackageSet));
    CHK_ORET(mxf_add_array_item_strongref(contentStorageSet, &MXF_ITEM_K(ContentStorage, Packages), sourcePackageSet));
    CHK_ORET(mxf_set_weakref_item(prefaceSet, &MXF_ITEM_K(Preface, PrimaryPackage), sourcePackageSet));
    CHK_ORET(mxf_set_umid_item(sourcePackageSet, &MXF_ITEM_K(GenericPackage, PackageUID), &sourcePackageUMID));
    CHK_ORET(mxf_set_timestamp_item(sourcePackageSet, &MXF_ITEM_K(GenericPackage, PackageCreationDate), &now));
    CHK_ORET(mxf_set_timestamp_item(sourcePackageSet, &MXF_ITEM_K(GenericPackage, PackageModifiedDate), &now));
    CHK_ORET(mxf_set_utf16string_item(sourcePackageSet, &MXF_ITEM_K(GenericPackage, Name), L"writedv50 source"));

    /* Preface - ContentStorage - SourcePackage - Timeline Track */    
    CHK_ORET(mxf_create_set(headerMetadata, &MXF_SET_K(Track), &sourcePackageTrackSet));
    CHK_ORET(mxf_add_array_item_strongref(sourcePackageSet, &MXF_ITEM_K(GenericPackage, Tracks), sourcePackageTrackSet));
    CHK_ORET(mxf_set_uint32_item(sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackID), sourceTrackID));
    CHK_ORET(mxf_set_uint32_item(sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), sourceTrackNumber));
    CHK_ORET(mxf_set_rational_item(sourcePackageTrackSet, &MXF_ITEM_K(Track, EditRate), &editRate));
    CHK_ORET(mxf_set_position_item(sourcePackageTrackSet, &MXF_ITEM_K(Track, Origin), 0));

    /* Preface - ContentStorage - SourcePackage - Timeline Track - Sequence */    
    CHK_ORET(mxf_create_set(headerMetadata, &MXF_SET_K(Sequence), &sequenceSet));
    CHK_ORET(mxf_set_strongref_item(sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), sequenceSet));
    CHK_ORET(mxf_set_ul_item(sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(LegacyPicture)));
    CHK_ORET(mxf_set_length_item(sequenceSet, &MXF_ITEM_K(StructuralComponent, Duration), duration));

    CHK_ORET(mxf_get_item(sequenceSet, &MXF_ITEM_K(StructuralComponent, Duration), &durationItem3));

    /* Preface - ContentStorage - SourcePackage - Timeline Track - Sequence - SourceClip */    
    CHK_ORET(mxf_create_set(headerMetadata, &MXF_SET_K(SourceClip), &sourceClipSet));
    CHK_ORET(mxf_add_array_item_strongref(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), sourceClipSet));
    CHK_ORET(mxf_set_ul_item(sourceClipSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(LegacyPicture)));
    CHK_ORET(mxf_set_length_item(sourceClipSet, &MXF_ITEM_K(StructuralComponent, Duration), duration));
    CHK_ORET(mxf_set_position_item(sourceClipSet, &MXF_ITEM_K(SourceClip, StartPosition), 0));
    CHK_ORET(mxf_set_umid_item(sourceClipSet, &MXF_ITEM_K(SourceClip, SourcePackageID), &g_Null_UMID));
    CHK_ORET(mxf_set_uint32_item(sourceClipSet, &MXF_ITEM_K(SourceClip, SourceTrackID), 0));

    CHK_ORET(mxf_get_item(sourceClipSet, &MXF_ITEM_K(StructuralComponent, Duration), &durationItem4));
    
    /* Preface - ContentStorage - SourcePackage - CDCIEssenceDescriptor */    
    CHK_ORET(mxf_create_set(headerMetadata, &MXF_SET_K(CDCIEssenceDescriptor), &cdciDescriptorSet));
    CHK_ORET(mxf_set_strongref_item(sourcePackageSet, &MXF_ITEM_K(SourcePackage, Descriptor), cdciDescriptorSet));
    CHK_ORET(mxf_set_rational_item(cdciDescriptorSet, &MXF_ITEM_K(FileDescriptor, SampleRate), &sampleRate));
    CHK_ORET(mxf_set_length_item(cdciDescriptorSet, &MXF_ITEM_K(FileDescriptor, ContainerDuration), duration));
    CHK_ORET(mxf_set_ul_item(cdciDescriptorSet, &MXF_ITEM_K(FileDescriptor, EssenceContainer), &MXF_EC_L(DVBased_50_625_50_ClipWrapped)));
    CHK_ORET(mxf_set_ul_item(cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, PictureEssenceCoding), &MXF_CMDEF_L(DVBased_50_625_50)));
    CHK_ORET(mxf_set_uint32_item(cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, StoredHeight), 288));
    CHK_ORET(mxf_set_uint32_item(cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, StoredWidth), 720));
    CHK_ORET(mxf_set_uint32_item(cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, SampledHeight), 288));
    CHK_ORET(mxf_set_uint32_item(cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, SampledWidth), 720));
    CHK_ORET(mxf_set_int32_item(cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, SampledXOffset), 0));
    CHK_ORET(mxf_set_int32_item(cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, SampledYOffset), 0));
    CHK_ORET(mxf_set_uint32_item(cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayHeight), 288));
    CHK_ORET(mxf_set_uint32_item(cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayWidth), 720));
    CHK_ORET(mxf_set_int32_item(cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayXOffset), 0));
    CHK_ORET(mxf_set_int32_item(cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayYOffset), 0));
    CHK_ORET(mxf_set_uint8_item(cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameLayout), 1));
    CHK_ORET(mxf_alloc_array_item_elements(cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, VideoLineMap), 4, 2, &arrayElement));
    mxf_set_int32(23, arrayElement);
    mxf_set_int32(335, &arrayElement[4]);
    CHK_ORET(mxf_set_rational_item(cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, AspectRatio), &aspectRatio));
    CHK_ORET(mxf_set_uint32_item(cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageAlignmentOffset), 1));
    CHK_ORET(mxf_set_uint32_item(cdciDescriptorSet, &MXF_ITEM_K(CDCIEssenceDescriptor, ComponentDepth), 8));
    CHK_ORET(mxf_set_uint32_item(cdciDescriptorSet, &MXF_ITEM_K(CDCIEssenceDescriptor, HorizontalSubsampling), 2));
    CHK_ORET(mxf_set_uint32_item(cdciDescriptorSet, &MXF_ITEM_K(CDCIEssenceDescriptor, VerticalSubsampling), 1));
    CHK_ORET(mxf_set_uint8_item(cdciDescriptorSet, &MXF_ITEM_K(CDCIEssenceDescriptor, ColorSiting), 4));
    CHK_ORET(mxf_set_uint32_item(cdciDescriptorSet, &MXF_ITEM_K(CDCIEssenceDescriptor, BlackRefLevel), 16));
    CHK_ORET(mxf_set_uint32_item(cdciDescriptorSet, &MXF_ITEM_K(CDCIEssenceDescriptor, WhiteReflevel), 235));
    CHK_ORET(mxf_set_uint32_item(cdciDescriptorSet, &MXF_ITEM_K(CDCIEssenceDescriptor, ColorRange), 225));

    CHK_ORET(mxf_get_item(cdciDescriptorSet, &MXF_ITEM_K(FileDescriptor, ContainerDuration), &durationItem5));
    
    CHK_ORET(mxf_set_int32_item(cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, ResolutionID), resolutionID));
    CHK_ORET(mxf_set_int32_item(cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameSampleSize), frameSize));
    CHK_ORET(mxf_set_int32_item(cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageSize), 0));

    CHK_ORET(mxf_get_item(cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageSize), &imageSizeItem));
    
    
    /* Preface - ContentStorage - EssenceContainerData */    
    CHK_ORET(mxf_create_set(headerMetadata, &MXF_SET_K(EssenceContainerData), &essContainerDataSet));
    CHK_ORET(mxf_add_array_item_strongref(contentStorageSet, &MXF_ITEM_K(ContentStorage, EssenceContainerData), essContainerDataSet));
    CHK_ORET(mxf_set_umid_item(essContainerDataSet, &MXF_ITEM_K(EssenceContainerData, LinkedPackageUID), &sourcePackageUMID));
    CHK_ORET(mxf_set_uint32_item(essContainerDataSet, &MXF_ITEM_K(EssenceContainerData, IndexSID), indexSID));
    CHK_ORET(mxf_set_uint32_item(essContainerDataSet, &MXF_ITEM_K(EssenceContainerData, BodySID), bodySID));


    /* write the header metadata with Avid extensions */    

    CHK_ORET((headerMetadataPos = mxf_file_tell(mxfFile)) >= 0);
    
    CHK_ORET(mxf_mark_header_start(mxfFile, headerPartition));
    CHK_ORET(mxf_avid_write_header_metadata(mxfFile, headerMetadata, headerPartition));
    CHK_ORET(mxf_fill_to_kag(mxfFile, headerPartition));
    CHK_ORET(mxf_mark_header_end(mxfFile, headerPartition));
    


    /* write the body partition pack */

    CHK_ORET(mxf_append_new_from_partition(&partitions, headerPartition, &bodyPartition));
    bodyPartition->key = MXF_PP_K(ClosedComplete, Body);
    bodyPartition->kagSize = 0x200;
    bodyPartition->bodySID = bodySID;

    CHK_ORET(mxf_write_partition(mxfFile, bodyPartition));
    CHK_ORET(mxf_fill_to_kag(mxfFile, bodyPartition));
    
    
    /* write the DV-50 essence element */
    
    CHK_ORET(mxf_open_essence_element_write(mxfFile, &MXF_EE_K(DVClipWrapped), 8, 0, &essenceElement));
    while (!done)
    {
        size_t numRead = fread(buffer, 1, essenceBufferSize, dv50File);
        if (numRead < essenceBufferSize)
        {
            if (!feof(dv50File))
            {
                fprintf(stderr, "Failed to read bytes from dv50 file\n");
                return 0;
            }
            done = 1;
        }
        
        CHK_ORET(mxf_write_essence_element_data(mxfFile, essenceElement, buffer, (uint32_t)numRead));
    }
    duration = essenceElement->totalLen / frameSize;
    imageSize = (int32_t)essenceElement->totalLen;
    CHK_ORET(mxf_finalize_essence_element_write(mxfFile, essenceElement));
    mxf_close_essence_element(&essenceElement);

    CHK_ORET(mxf_fill_to_kag(mxfFile, bodyPartition));


    
    /* write the footer partition pack */

    CHK_ORET(mxf_append_new_from_partition(&partitions, headerPartition, &footerPartition));
    footerPartition->key = MXF_PP_K(ClosedComplete, Footer);
    footerPartition->kagSize = 0x200;
    footerPartition->indexSID = indexSID;

    CHK_ORET(mxf_write_partition(mxfFile, footerPartition));
    CHK_ORET(mxf_fill_to_kag(mxfFile, footerPartition));

    
    /* write the index table segment */
    
    CHK_ORET(mxf_mark_index_start(mxfFile, footerPartition));
    
    CHK_ORET(mxf_create_index_table_segment(&indexSegment)); 
    indexSegment->instanceUID = indexSegmentUUID;
    indexSegment->indexEditRate = editRate;
    indexSegment->indexStartPosition = 0;
    indexSegment->indexDuration = duration;
    indexSegment->editUnitByteCount = frameSize;
    indexSegment->indexSID = indexSID;
    indexSegment->bodySID = bodySID;
    indexSegment->sliceCount = 0;
    indexSegment->posTableCount = 0;
    indexSegment->deltaEntryArray = NULL;
    indexSegment->indexEntryArray = NULL;
    
    CHK_ORET(mxf_write_index_table_segment(mxfFile, indexSegment));
    CHK_ORET(mxf_fill_to_kag(mxfFile, footerPartition));
    
    CHK_ORET(mxf_mark_index_end(mxfFile, footerPartition));
    

    /* write the random index pack */
    
    CHK_ORET(mxf_write_rip(mxfFile, &partitions));

    
    /* update and re-write the header metadata */
    /* Note: the size will not change so it is safe to re-write */
    
    CHK_ORET(mxf_set_length_item(durationItem1->set, &durationItem1->key, duration));
    CHK_ORET(mxf_set_length_item(durationItem2->set, &durationItem2->key, duration));
    CHK_ORET(mxf_set_length_item(durationItem3->set, &durationItem3->key, duration));
    CHK_ORET(mxf_set_length_item(durationItem4->set, &durationItem4->key, duration));
    CHK_ORET(mxf_set_length_item(durationItem5->set, &durationItem5->key, duration));
    CHK_ORET(mxf_set_int32_item(imageSizeItem->set, &imageSizeItem->key, imageSize));

    CHK_ORET(mxf_file_seek(mxfFile, headerMetadataPos, SEEK_SET));
    CHK_ORET(mxf_mark_header_start(mxfFile, headerPartition));
    CHK_ORET(mxf_avid_write_header_metadata(mxfFile, headerMetadata, headerPartition));
    CHK_ORET(mxf_fill_to_kag(mxfFile, headerPartition));
    CHK_ORET(mxf_mark_header_end(mxfFile, headerPartition));
    

    
    
    /* update the partitions */
    
    CHK_ORET(mxf_update_partitions(mxfFile, &partitions));
    
    
       
    /* free memory resources */

    mxf_free_index_table_segment(&indexSegment);
    mxf_clear_file_partitions(&partitions);
    mxf_free_header_metadata(&headerMetadata);
    mxf_free_data_model(&dataModel);
    
    return 1;
}

void usage(const char* cmd)
{
    fprintf(stderr, "%s [--test] <dv50 filename> <mxf filename>\n", cmd);
}

int main(int argv, const char* argc[])
{
    FILE* dv50File = NULL;
    MXFFile* mxfFile = NULL;
    int result;
    int cmdlnIndex = 1;
    int test = 0;
    
    if (argv < 3 || argv > 4)
    {
        usage(argc[0]);
        return 1;
    }
    
    if (argv == 4)
    {
        if (strcmp(argc[cmdlnIndex], "--test") != 0)
        {
            usage(argc[0]);
            return 1;
        }
        test = 1;
        
        cmdlnIndex++;
    }
    
    if ((dv50File = fopen(argc[cmdlnIndex], "rb")) == NULL)
    {
        fprintf(stderr, "Failed to open %s for reading\n", argc[1]);
        return 1;
    }
    cmdlnIndex++;
    
    if (!mxf_disk_file_open_new(argc[cmdlnIndex], &mxfFile))
    {
        fprintf(stderr, "Failed to open %s for writing\n", argc[2]);
        fclose(dv50File);
        return 1;
    }
    cmdlnIndex++;

    
    if (!write_dv50(dv50File, mxfFile, test))
    {
        result = 1;
    }
    else
    {
        result = 0;
    }

    
    fclose(dv50File);
    mxf_file_close(&mxfFile);
    
    return result;
}
