/*
 * $Id: archive_mxf_info_lib.c,v 1.1 2010/01/12 17:40:26 john_f Exp $
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
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <assert.h>

#include <archive_mxf_info_lib.h>
#include <mxf/mxf_uu_metadata.h>
#include <mxf/mxf_page_file.h>


#define CHK_OFAIL_NOMSG(cmd) \
    if (!(cmd)) \
    { \
        goto fail; \
    }

    
/* declare the BBC archive extensions */

#define MXF_LABEL(d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15) \
    {d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15}

#define MXF_SET_DEFINITION(parentName, name, label) \
    static const mxfUL MXF_SET_K(name) = label;
    
#define MXF_ITEM_DEFINITION(setName, name, label, localTag, typeId, isRequired) \
    static const mxfUL MXF_ITEM_K(setName, name) = label;

#include <bbc_archive_extensions_data_model.h>

/* TODO: move this somewhere shared */
static const mxfUL MXF_DM_L(APP_PreservationDescriptiveScheme) = 
    {0x06, 0x0E, 0x2B, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0D, 0x04, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00};



#define MXF_LABEL(d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15) \
    {d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15}

#define MXF_SET_DEFINITION(parentName, name, label) \
    CHK_ORET(mxf_register_set_def(dataModel, #name, &MXF_SET_K(parentName), &MXF_SET_K(name)));
    
#define MXF_ITEM_DEFINITION(setName, name, label, tag, typeId, isRequired) \
    CHK_ORET(mxf_register_item_def(dataModel, #name, &MXF_SET_K(setName), &MXF_ITEM_K(setName, name), tag, typeId, isRequired));
    
int archive_mxf_load_extensions(MXFDataModel* dataModel)
{
#include <bbc_archive_extensions_data_model.h>

    return 1;
}


#define GET_STRING_ITEM(name, cName) \
    if (mxf_have_item(dmFrameworkSet, &MXF_ITEM_K(APP_InfaxFramework, name))) \
    { \
        CHK_OFAIL(mxf_uu_get_utf16string_item(dmFrameworkSet, &MXF_ITEM_K(APP_InfaxFramework, name), &tempWString)); \
        CHK_OFAIL(wcstombs(infaxData->cName, tempWString, sizeof(infaxData->cName)) != (size_t)(-1)); \
        infaxData->cName[sizeof(infaxData->cName) - 1] = '\0'; \
        SAFE_FREE(&tempWString); \
    } 

#define GET_DATE_ITEM(name, cName) \
    if (mxf_have_item(dmFrameworkSet, &MXF_ITEM_K(APP_InfaxFramework, name))) \
    { \
        CHK_OFAIL(mxf_get_timestamp_item(dmFrameworkSet, &MXF_ITEM_K(APP_InfaxFramework, name), &infaxData->cName)); \
        infaxData->cName.hour = 0; \
        infaxData->cName.min = 0; \
        infaxData->cName.sec = 0; \
        infaxData->cName.qmsec = 0; \
    } 

#define GET_INT64_ITEM(name, cName) \
    if (mxf_have_item(dmFrameworkSet, &MXF_ITEM_K(APP_InfaxFramework, name))) \
    { \
        CHK_OFAIL(mxf_get_int64_item(dmFrameworkSet, &MXF_ITEM_K(APP_InfaxFramework, name), &infaxData->cName)); \
    } 

#define GET_UINT32_ITEM(name, cName) \
    if (mxf_have_item(dmFrameworkSet, &MXF_ITEM_K(APP_InfaxFramework, name))) \
    { \
        CHK_OFAIL(mxf_get_uint32_item(dmFrameworkSet, &MXF_ITEM_K(APP_InfaxFramework, name), &infaxData->cName)); \
    } 

static int get_infax_data(MXFMetadataSet* dmFrameworkSet, InfaxData* infaxData)
{
    mxfUTF16Char* tempWString = NULL;
    
    GET_STRING_ITEM(APP_Format, format);
    GET_STRING_ITEM(APP_ProgrammeTitle, progTitle);
    GET_STRING_ITEM(APP_EpisodeTitle, epTitle);
    GET_DATE_ITEM(APP_TransmissionDate, txDate);
    GET_STRING_ITEM(APP_MagazinePrefix, magPrefix);
    GET_STRING_ITEM(APP_ProgrammeNumber, progNo);
    GET_STRING_ITEM(APP_ProductionCode, prodCode);
    GET_STRING_ITEM(APP_SpoolStatus, spoolStatus);
    GET_DATE_ITEM(APP_StockDate, stockDate);
    GET_STRING_ITEM(APP_SpoolDescriptor, spoolDesc);
    GET_STRING_ITEM(APP_Memo, memo);
    GET_INT64_ITEM(APP_Duration, duration);
    GET_STRING_ITEM(APP_SpoolNumber, spoolNo);
    GET_STRING_ITEM(APP_AccessionNumber, accNo);
    GET_STRING_ITEM(APP_CatalogueDetail, catDetail);
    GET_UINT32_ITEM(APP_ItemNumber, itemNo);
    
    SAFE_FREE(&tempWString);
    return 1;
    
fail:
    SAFE_FREE(&tempWString);
    return 0;
}


int is_archive_mxf(MXFHeaderMetadata* headerMetadata)
{
    MXFMetadataSet* prefaceSet;
    MXFArrayItemIterator arrayIter;
    uint8_t* arrayElement;
    uint32_t arrayElementLen;
    int haveBBCScheme = 0;
    int haveSDUncompressed = 0;
    int havePCM = 0;
    mxfUL ul;
    
    CHK_OFAIL(mxf_find_singular_set_by_key(headerMetadata, &MXF_SET_K(Preface), &prefaceSet));

    /* check is OP-1A */
    CHK_OFAIL(mxf_get_ul_item(prefaceSet, &MXF_ITEM_K(Preface, OperationalPattern), &ul));
    if (!is_op_1a(&ul))
    {
        return 0;
    }

    
    /* check BBC descriptive metadata scheme */
    CHK_OFAIL(mxf_initialise_array_item_iterator(prefaceSet, &MXF_ITEM_K(Preface, DMSchemes), &arrayIter));
    while (mxf_next_array_item_element(&arrayIter, &arrayElement, &arrayElementLen))
    {
        mxf_get_ul(arrayElement, &ul);
        if (mxf_equals_ul(&ul, &MXF_DM_L(APP_PreservationDescriptiveScheme)))
        {
            haveBBCScheme = 1;
            break;
        }
    }
    if (!haveBBCScheme)
    {
        return 0;
    }

    
    /* check essence containers include uncompressed SD and PCM */
    CHK_OFAIL(mxf_initialise_array_item_iterator(prefaceSet, &MXF_ITEM_K(Preface, EssenceContainers), &arrayIter));
    while (mxf_next_array_item_element(&arrayIter, &arrayElement, &arrayElementLen))
    {
        mxf_get_ul(arrayElement, &ul);
        if (mxf_equals_ul(&ul, &MXF_EC_L(MultipleWrappings)))
        {
            continue;
        }
        else if (mxf_equals_ul(&ul, &MXF_EC_L(SD_Unc_625_50i_422_135_FrameWrapped)))
        {
            haveSDUncompressed = 1;
        }
        else if (mxf_equals_ul(&ul, &MXF_EC_L(BWFFrameWrapped)))
        {
            havePCM = 1;
        }
        else
        {
            return 0;
        }
    }
    if (!haveSDUncompressed || !havePCM)
    {
        return 0;
    }


    return 1;    
    
fail:
    return 0;    
}

int archive_mxf_get_info(MXFHeaderMetadata* headerMetadata, ArchiveMXFInfo* info)
{
    MXFList* list = NULL;
    MXFListIterator iter;
    MXFArrayItemIterator arrayIter;
    uint8_t* arrayElement;
    uint32_t arrayElementLen;
    mxfUL dataDef;
    uint32_t count;
    mxfUTF16Char* tempWString = NULL;
    int haveSourceInfaxData = 0;
    MXFMetadataSet* identSet;
    MXFMetadataSet* fileSourcePackageSet;
    MXFMetadataSet* sourcePackageSet;
    MXFMetadataSet* sourcePackageTrackSet;
    MXFMetadataSet* sequenceSet;
    MXFMetadataSet* dmSet;
    MXFMetadataSet* dmFrameworkSet;
    MXFMetadataSet* descriptorSet;
    MXFMetadataSet* locatorSet;
    
    
    /* Creation timestamp identification info */
    
    CHK_OFAIL(mxf_find_set_by_key(headerMetadata, &MXF_SET_K(Identification), &list));
    mxf_initialise_list_iter(&iter, list);
    if (mxf_next_list_iter_element(&iter))
    {
        identSet = (MXFMetadataSet*)mxf_get_iter_element(&iter);
        CHK_OFAIL(mxf_get_timestamp_item(identSet, &MXF_ITEM_K(Identification, ModificationDate), &info->creationDate));
    }
    mxf_free_list(&list);
    
    
    /* LTO Infax data */    
    
    CHK_OFAIL(mxf_uu_get_top_file_package(headerMetadata, &fileSourcePackageSet));
    CHK_OFAIL(mxf_uu_get_package_tracks(fileSourcePackageSet, &arrayIter));
    while (mxf_uu_next_track(headerMetadata, &arrayIter, &sourcePackageTrackSet))
    {
        CHK_OFAIL(mxf_uu_get_track_datadef(sourcePackageTrackSet, &dataDef));

        if (mxf_is_descriptive_metadata(&dataDef))
        {
            /* get to the single DMSegment */
            CHK_OFAIL(mxf_get_strongref_item(sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), &sequenceSet));
            if (mxf_is_subclass_of(headerMetadata->dataModel, &sequenceSet->key, &MXF_SET_K(Sequence)))
            {
                CHK_OFAIL(mxf_get_array_item_count(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), &count));
                if (count != 1)
                {
                    /* Sequence of length 1 is expected for the DMS track */
                    continue;
                }
                
                CHK_OFAIL(mxf_get_array_item_element(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), 0, &arrayElement));
                CHK_OFAIL(mxf_get_strongref(headerMetadata, arrayElement, &dmSet));
            }
            else
            {
                dmSet = sequenceSet;
            }
            
            /* if it is a DMSegment with a DMFramework reference then we have the DMS track */
            if (mxf_is_subclass_of(headerMetadata->dataModel, &dmSet->key, &MXF_SET_K(DMSegment)))
            {
                if (mxf_have_item(dmSet, &MXF_ITEM_K(DMSegment, DMFramework)))
                {
                    CHK_OFAIL(mxf_get_strongref_item(dmSet, &MXF_ITEM_K(DMSegment, DMFramework), &dmFrameworkSet));
                    
                    /* if it is a APP_InfaxFramework then it is the Infax data */
                    if (mxf_is_subclass_of(headerMetadata->dataModel, &dmFrameworkSet->key, &MXF_SET_K(APP_InfaxFramework)))
                    {
                        CHK_OFAIL(get_infax_data(dmFrameworkSet, &info->ltoInfaxData));
                        break;
                    }
                }
            }
        }
    }


    /* original filename */

    CHK_OFAIL(mxf_get_strongref_item(fileSourcePackageSet, &MXF_ITEM_K(SourcePackage, Descriptor), &descriptorSet));
    if (mxf_have_item(descriptorSet, &MXF_ITEM_K(GenericDescriptor, Locators)))
    {
        CHK_OFAIL(mxf_initialise_array_item_iterator(descriptorSet, &MXF_ITEM_K(GenericDescriptor, Locators), &arrayIter));
        while (mxf_next_array_item_element(&arrayIter, &arrayElement, &arrayElementLen))
        {
            CHK_OFAIL(mxf_get_strongref(headerMetadata, arrayElement, &locatorSet));

            if (mxf_is_subclass_of(headerMetadata->dataModel, &locatorSet->key, &MXF_SET_K(NetworkLocator)))
            {
                CHK_OFAIL(mxf_uu_get_utf16string_item(locatorSet, &MXF_ITEM_K(NetworkLocator, URLString), &tempWString));
                CHK_OFAIL(wcstombs(info->filename, tempWString, sizeof(info->filename)) != (size_t)(-1));
                info->filename[sizeof(info->filename) - 1] = '\0';
                SAFE_FREE(&tempWString);
                break;
            }
        }
    }    
    
    
    /* source Infax data */
    
    CHK_OFAIL(mxf_find_set_by_key(headerMetadata, &MXF_SET_K(SourcePackage), &list));
    mxf_initialise_list_iter(&iter, list);
    while (mxf_next_list_iter_element(&iter))
    {
        sourcePackageSet = (MXFMetadataSet*)(mxf_get_iter_element(&iter));
        
        /* it is the tape SourcePackage if it has a TapeDescriptor */
        CHK_OFAIL(mxf_get_strongref_item(sourcePackageSet, &MXF_ITEM_K(SourcePackage, Descriptor), &descriptorSet));
        if (mxf_is_subclass_of(headerMetadata->dataModel, &descriptorSet->key, &MXF_SET_K(TapeDescriptor)))
        {
            /* go through the tracks and find the DMS track */
            CHK_OFAIL(mxf_uu_get_package_tracks(sourcePackageSet, &arrayIter));
            while (mxf_uu_next_track(headerMetadata, &arrayIter, &sourcePackageTrackSet))
            {
                CHK_OFAIL(mxf_uu_get_track_datadef(sourcePackageTrackSet, &dataDef));
        
                if (mxf_is_descriptive_metadata(&dataDef))
                {
                    /* get to the single DMSegment */
                    CHK_OFAIL(mxf_get_strongref_item(sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), &sequenceSet));
                    if (mxf_is_subclass_of(headerMetadata->dataModel, &sequenceSet->key, &MXF_SET_K(Sequence)))
                    {
                        CHK_OFAIL(mxf_get_array_item_count(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), &count));
                        if (count != 1)
                        {
                            /* Sequence of length 1 is expected for the DMS track */
                            continue;
                        }
                        
                        CHK_OFAIL(mxf_get_array_item_element(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), 0, &arrayElement));
                        CHK_OFAIL(mxf_get_strongref(headerMetadata, arrayElement, &dmSet));
                    }
                    else
                    {
                        dmSet = sequenceSet;
                    }
                    
                    /* if it is a DMSegment with a DMFramework reference then we have the DMS track */
                    if (mxf_is_subclass_of(headerMetadata->dataModel, &dmSet->key, &MXF_SET_K(DMSegment)))
                    {
                        if (mxf_have_item(dmSet, &MXF_ITEM_K(DMSegment, DMFramework)))
                        {
                            CHK_OFAIL(mxf_get_strongref_item(dmSet, &MXF_ITEM_K(DMSegment, DMFramework), &dmFrameworkSet));
                            
                            /* if it is a APP_InfaxFramework then it is the Infax data */
                            if (mxf_is_subclass_of(headerMetadata->dataModel, &dmFrameworkSet->key, &MXF_SET_K(APP_InfaxFramework)))
                            {
                                CHK_OFAIL(get_infax_data(dmFrameworkSet, &info->sourceInfaxData));
                                haveSourceInfaxData = 1;
                                break;
                            }
                        }
                    }
                }
            }
                    
            break;
        }
    }
    mxf_free_list(&list);

    
    return haveSourceInfaxData;
    
fail:
    SAFE_FREE(&tempWString);
    mxf_free_list(&list);
    return 0;    
}


int archive_mxf_get_pse_failures(MXFHeaderMetadata* headerMetadata, PSEFailure** failures, long* numFailures)
{
    MXFArrayItemIterator arrayIter;
    MXFArrayItemIterator arrayIter2;
    uint8_t* arrayElement;
    uint32_t arrayElementLen;
    mxfUL dataDef;
    uint32_t count;
    MXFMetadataSet* fileSourcePackageSet;
    MXFMetadataSet* sourcePackageTrackSet;
    MXFMetadataSet* sequenceSet;
    MXFMetadataSet* dmSet;
    MXFMetadataSet* dmFrameworkSet;
    MXFListIterator setsIter;
    PSEFailure* newFailures = NULL;
    long countedPSEFailures = 0;
    PSEFailure* tmp;
    int32_t i;
    

    CHK_OFAIL(mxf_uu_get_top_file_package(headerMetadata, &fileSourcePackageSet));
    CHK_OFAIL(mxf_uu_get_package_tracks(fileSourcePackageSet, &arrayIter));
    while (mxf_uu_next_track(headerMetadata, &arrayIter, &sourcePackageTrackSet))
    {
        CHK_OFAIL(mxf_uu_get_track_datadef(sourcePackageTrackSet, &dataDef));
        if (mxf_is_descriptive_metadata(&dataDef))
        {
            /* get to the sequence */
            CHK_OFAIL(mxf_get_strongref_item(sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), &sequenceSet));
            if (mxf_is_subclass_of(headerMetadata->dataModel, &sequenceSet->key, &MXF_SET_K(Sequence)))
            {
                CHK_OFAIL(mxf_get_array_item_count(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), &count));
                if (count == 0)
                {
                    continue;
                }
                
                CHK_OFAIL(mxf_get_array_item_element(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), 0, &arrayElement));
                CHK_OFAIL(mxf_get_strongref(headerMetadata, arrayElement, &dmSet));
            }
            else
            {
                dmSet = sequenceSet;
            }
            
            /* if it is a DMSegment with a DMFramework reference then we have the DMS track */
            if (mxf_is_subclass_of(headerMetadata->dataModel, &dmSet->key, &MXF_SET_K(DMSegment)))
            {
                if (mxf_have_item(dmSet, &MXF_ITEM_K(DMSegment, DMFramework)))
                {
                    CHK_OFAIL(mxf_get_strongref_item(dmSet, &MXF_ITEM_K(DMSegment, DMFramework), &dmFrameworkSet));

                    /* check whether the DMFrameworkSet is a APP_PSEAnalysisFramework */                   
                    if (mxf_is_subclass_of(headerMetadata->dataModel, &dmFrameworkSet->key, &MXF_SET_K(APP_PSEAnalysisFramework)))
                    {
                        /* go back to the sequence and extract the PSE failures */
                        
                        CHK_OFAIL(mxf_get_array_item_count(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), &count));
                        countedPSEFailures += count;
                        if (newFailures == NULL)
                        {
                            CHK_OFAIL((tmp = malloc(sizeof(PSEFailure) * countedPSEFailures)) != NULL);
                        }
                        else
                        {
                            /* multiple tracks with PSE failures - reallocate the array */
                            CHK_OFAIL((tmp = realloc(newFailures, sizeof(PSEFailure) * countedPSEFailures)) != NULL);
                        }
                        newFailures = tmp;
                        
                        /* extract the PSE failures */
                        initialise_sets_iter(headerMetadata, &setsIter);
                        i = 0;
                        mxf_initialise_array_item_iterator(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), &arrayIter2);
                        while (mxf_next_array_item_element(&arrayIter2, &arrayElement, &arrayElementLen))
                        {
                            PSEFailure* pseFailure = &newFailures[i];
                            
                            CHK_OFAIL(mxf_get_strongref_s(headerMetadata, &setsIter, arrayElement, &dmSet));
                            CHK_OFAIL(mxf_get_position_item(dmSet, &MXF_ITEM_K(DMSegment, EventStartPosition), &pseFailure->position));
                            CHK_OFAIL(mxf_get_strongref_item_s(&setsIter, dmSet, &MXF_ITEM_K(DMSegment, DMFramework), &dmFrameworkSet));
                            CHK_OFAIL(mxf_get_int16_item(dmFrameworkSet, &MXF_ITEM_K(APP_PSEAnalysisFramework, APP_RedFlash), &pseFailure->redFlash));
                            CHK_OFAIL(mxf_get_int16_item(dmFrameworkSet, &MXF_ITEM_K(APP_PSEAnalysisFramework, APP_SpatialPattern), &pseFailure->spatialPattern));
                            CHK_OFAIL(mxf_get_int16_item(dmFrameworkSet, &MXF_ITEM_K(APP_PSEAnalysisFramework, APP_LuminanceFlash), &pseFailure->luminanceFlash));
                            CHK_OFAIL(mxf_get_boolean_item(dmFrameworkSet, &MXF_ITEM_K(APP_PSEAnalysisFramework, APP_ExtendedFailure), &pseFailure->extendedFailure));
                            i++;
                        }
                        break;
                    }
                }
            }
        }
    }
    
    *failures = newFailures;
    *numFailures = countedPSEFailures;
    return 1;

fail:
    SAFE_FREE(&newFailures);
    return 0;
}

int archive_mxf_get_vtr_errors(MXFHeaderMetadata* headerMetadata, VTRErrorAtPos** errors, long* numErrors)
{
    MXFArrayItemIterator arrayIter;
    MXFArrayItemIterator arrayIter2;
    uint8_t* arrayElement;
    uint32_t arrayElementLen;
    mxfUL dataDef;
    uint32_t count;
    MXFMetadataSet* fileSourcePackageSet;
    MXFMetadataSet* sourcePackageTrackSet;
    MXFMetadataSet* sequenceSet;
    MXFMetadataSet* dmSet;
    MXFMetadataSet* dmFrameworkSet;
    MXFListIterator setsIter;
    VTRErrorAtPos* newErrors = NULL;
    long totalErrors = 0;
    VTRErrorAtPos* tmp;
    

    CHK_OFAIL(mxf_uu_get_top_file_package(headerMetadata, &fileSourcePackageSet));
    CHK_OFAIL(mxf_uu_get_package_tracks(fileSourcePackageSet, &arrayIter));
    while (mxf_uu_next_track(headerMetadata, &arrayIter, &sourcePackageTrackSet))
    {
        CHK_OFAIL(mxf_uu_get_track_datadef(sourcePackageTrackSet, &dataDef));
        if (mxf_is_descriptive_metadata(&dataDef))
        {
            /* get to the sequence */
            CHK_OFAIL(mxf_get_strongref_item(sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), &sequenceSet));
            if (mxf_is_subclass_of(headerMetadata->dataModel, &sequenceSet->key, &MXF_SET_K(Sequence)))
            {
                CHK_OFAIL(mxf_get_array_item_count(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), &count));
                if (count == 0)
                {
                    continue;
                }
                
                CHK_OFAIL(mxf_get_array_item_element(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), 0, &arrayElement));
                CHK_OFAIL(mxf_get_strongref(headerMetadata, arrayElement, &dmSet));
            }
            else
            {
                dmSet = sequenceSet;
            }
            
            /* if it is a DMSegment with a DMFramework reference then we have the DMS track */
            if (mxf_is_subclass_of(headerMetadata->dataModel, &dmSet->key, &MXF_SET_K(DMSegment)))
            {
                if (mxf_have_item(dmSet, &MXF_ITEM_K(DMSegment, DMFramework)))
                {
                    CHK_OFAIL(mxf_get_strongref_item(dmSet, &MXF_ITEM_K(DMSegment, DMFramework), &dmFrameworkSet));

                    /* check whether the DMFrameworkSet is a APP_VTRReplayErrorFramework */                   
                    if (mxf_is_subclass_of(headerMetadata->dataModel, &dmFrameworkSet->key, &MXF_SET_K(APP_VTRReplayErrorFramework)))
                    {
                        /* go back to the sequence and extract the VTR errors */
                        
                        CHK_OFAIL(mxf_get_array_item_count(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), &count));
                        if (newErrors == NULL)
                        {
                            CHK_OFAIL((tmp = malloc(sizeof(VTRErrorAtPos) * (totalErrors + count))) != NULL);
                        }
                        else
                        {
                            /* multiple tracks with VTR errors - reallocate the array */
                            CHK_OFAIL((tmp = realloc(newErrors, sizeof(VTRErrorAtPos) * (totalErrors + count))) != NULL);
                        }
                        newErrors = tmp;
                        
                        /* extract the VTR errors */
                        initialise_sets_iter(headerMetadata, &setsIter);
                        mxf_initialise_array_item_iterator(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), &arrayIter2);
                        while (mxf_next_array_item_element(&arrayIter2, &arrayElement, &arrayElementLen))
                        {
                            VTRErrorAtPos* vtrError = &newErrors[totalErrors];
                            
                            CHK_OFAIL(mxf_get_strongref_s(headerMetadata, &setsIter, arrayElement, &dmSet));
                            CHK_OFAIL(mxf_get_position_item(dmSet, &MXF_ITEM_K(DMSegment, EventStartPosition), &vtrError->position));
                            CHK_OFAIL(mxf_get_strongref_item_s(&setsIter, dmSet, &MXF_ITEM_K(DMSegment, DMFramework), &dmFrameworkSet));
                            CHK_OFAIL(mxf_get_uint8_item(dmFrameworkSet, &MXF_ITEM_K(APP_VTRReplayErrorFramework, APP_VTRErrorCode), &vtrError->errorCode));
                            
                            totalErrors++;
                        }
                    }
                }
            }
        }
    }
    
    *errors = newErrors;
    *numErrors = totalErrors;
    return 1;

fail:
    SAFE_FREE(&newErrors);
    return 0;
}

int archive_mxf_get_digibeta_dropouts(MXFHeaderMetadata* headerMetadata, DigiBetaDropout** digiBetaDropouts, long* numDigiBetaDropouts)
{
    MXFArrayItemIterator arrayIter;
    MXFArrayItemIterator arrayIter2;
    uint8_t* arrayElement;
    uint32_t arrayElementLen;
    mxfUL dataDef;
    uint32_t count;
    MXFMetadataSet* fileSourcePackageSet;
    MXFMetadataSet* sourcePackageTrackSet;
    MXFMetadataSet* sequenceSet;
    MXFMetadataSet* dmSet;
    MXFMetadataSet* dmFrameworkSet;
    MXFListIterator setsIter;
    DigiBetaDropout* newDigiBetaDropouts = NULL;
    long totalDropouts = 0;
    DigiBetaDropout* tmp;
    

    CHK_OFAIL(mxf_uu_get_top_file_package(headerMetadata, &fileSourcePackageSet));
    CHK_OFAIL(mxf_uu_get_package_tracks(fileSourcePackageSet, &arrayIter));
    while (mxf_uu_next_track(headerMetadata, &arrayIter, &sourcePackageTrackSet))
    {
        CHK_OFAIL(mxf_uu_get_track_datadef(sourcePackageTrackSet, &dataDef));
        if (mxf_is_descriptive_metadata(&dataDef))
        {
            /* get to the sequence */
            CHK_OFAIL(mxf_get_strongref_item(sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), &sequenceSet));
            if (mxf_is_subclass_of(headerMetadata->dataModel, &sequenceSet->key, &MXF_SET_K(Sequence)))
            {
                CHK_OFAIL(mxf_get_array_item_count(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), &count));
                if (count == 0)
                {
                    continue;
                }
                
                CHK_OFAIL(mxf_get_array_item_element(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), 0, &arrayElement));
                CHK_OFAIL(mxf_get_strongref(headerMetadata, arrayElement, &dmSet));
            }
            else
            {
                dmSet = sequenceSet;
            }
            
            /* if it is a DMSegment with a DMFramework reference then we have the DMS track */
            if (mxf_is_subclass_of(headerMetadata->dataModel, &dmSet->key, &MXF_SET_K(DMSegment)))
            {
                if (mxf_have_item(dmSet, &MXF_ITEM_K(DMSegment, DMFramework)))
                {
                    CHK_OFAIL(mxf_get_strongref_item(dmSet, &MXF_ITEM_K(DMSegment, DMFramework), &dmFrameworkSet));

                    /* check whether the DMFrameworkSet is a APP_DigiBetaDropoutFramework */
                    if (mxf_is_subclass_of(headerMetadata->dataModel, &dmFrameworkSet->key, &MXF_SET_K(APP_DigiBetaDropoutFramework)))
                    {
                        /* go back to the sequence and extract the digibeta dropouts */
                        
                        CHK_OFAIL(mxf_get_array_item_count(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), &count));
                        if (newDigiBetaDropouts == NULL)
                        {
                            CHK_OFAIL((tmp = malloc(sizeof(DigiBetaDropout) * (totalDropouts + count))) != NULL);
                        }
                        else
                        {
                            /* multiple tracks with digibeta dropouts - reallocate the array */
                            CHK_OFAIL((tmp = realloc(newDigiBetaDropouts, sizeof(DigiBetaDropout) * (totalDropouts + count))) != NULL);
                        }
                        newDigiBetaDropouts = tmp;
                        
                        /* extract the digibeta dropouts */
                        initialise_sets_iter(headerMetadata, &setsIter);
                        mxf_initialise_array_item_iterator(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), &arrayIter2);
                        while (mxf_next_array_item_element(&arrayIter2, &arrayElement, &arrayElementLen))
                        {
                            DigiBetaDropout* digiBetaDropout = &newDigiBetaDropouts[totalDropouts];
                            
                            CHK_OFAIL(mxf_get_strongref_s(headerMetadata, &setsIter, arrayElement, &dmSet));
                            CHK_OFAIL(mxf_get_position_item(dmSet, &MXF_ITEM_K(DMSegment, EventStartPosition), &digiBetaDropout->position));
                            CHK_OFAIL(mxf_get_strongref_item_s(&setsIter, dmSet, &MXF_ITEM_K(DMSegment, DMFramework), &dmFrameworkSet));
                            CHK_OFAIL(mxf_get_int32_item(dmFrameworkSet, &MXF_ITEM_K(APP_DigiBetaDropoutFramework, APP_Strength), &digiBetaDropout->strength));
                            
                            totalDropouts++;
                        }
                    }
                }
            }
        }
    }
    
    *digiBetaDropouts = newDigiBetaDropouts;
    *numDigiBetaDropouts = totalDropouts;
    return 1;

fail:
    SAFE_FREE(&newDigiBetaDropouts);
    return 0;
}


int archive_mxf_read_footer_metadata(const char* filename, MXFDataModel* dataModel, MXFHeaderMetadata** headerMetadata)
{
    MXFPageFile* mxfPageFile = NULL;
    MXFFile* mxfFile = NULL;
    MXFRIP rip;
    MXFRIPEntry* lastRIPEntry = NULL;
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    MXFPartition* footerPartition = NULL;
    int result = 0;
    MXFHeaderMetadata* newHeaderMetadata = NULL;
    
    memset(&rip, 0, sizeof(rip));
 
    
    /* open MXF file */    
    if (strstr(filename, "%d") != NULL)
    {
        CHK_OFAIL_NOMSG(mxf_page_file_open_read(filename, &mxfPageFile));
        mxfFile = mxf_page_file_get_file(mxfPageFile);
    }
    else
    {
        CHK_OFAIL_NOMSG(mxf_disk_file_open_read(filename, &mxfFile));
    }
    
    /* read the RIP */
    CHK_OFAIL_NOMSG(mxf_read_rip(mxfFile, &rip));
    
    /* read footer partition pack */
    CHK_OFAIL_NOMSG((lastRIPEntry = (MXFRIPEntry*)mxf_get_last_list_element(&rip.entries)) != NULL);
    CHK_OFAIL_NOMSG(mxf_file_seek(mxfFile, mxf_get_runin_len(mxfFile) + lastRIPEntry->thisPartition, SEEK_SET));
    CHK_OFAIL_NOMSG(mxf_read_kl(mxfFile, &key, &llen, &len));
    CHK_OFAIL_NOMSG(mxf_is_partition_pack(&key));
    result = 2; /* the file is complete and the presence, or not, of the header metadata will not change */
    *headerMetadata = NULL;
    
    CHK_OFAIL_NOMSG(mxf_is_footer_partition_pack(&key));
    CHK_OFAIL_NOMSG(mxf_read_partition(mxfFile, &key, &footerPartition));
    
    /* read the header metadata */
    CHK_OFAIL_NOMSG(mxf_read_next_nonfiller_kl(mxfFile, &key, &llen, &len));
    CHK_OFAIL_NOMSG(mxf_is_header_metadata(&key));
    CHK_OFAIL_NOMSG(mxf_create_header_metadata(&newHeaderMetadata, dataModel));
    CHK_OFAIL_NOMSG(mxf_read_header_metadata(mxfFile, newHeaderMetadata, 
        footerPartition->headerByteCount, &key, llen, len));

    mxf_free_partition(&footerPartition);
    mxf_clear_rip(&rip);
    mxf_file_close(&mxfFile);
    
    *headerMetadata = newHeaderMetadata;
    newHeaderMetadata = NULL;
    return 1;
 
    
fail:
    mxf_free_header_metadata(&newHeaderMetadata);
    mxf_free_partition(&footerPartition);
    mxf_clear_rip(&rip);
    mxf_file_close(&mxfFile);
    return result;
}


