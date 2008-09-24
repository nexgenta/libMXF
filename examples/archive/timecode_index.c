/*
 * $Id: timecode_index.c,v 1.2 2008/09/24 17:29:57 philipn Exp $
 *
 * 
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
#include <assert.h>

#include <timecode_index.h>
#include <mxf/mxf_utils.h>
#include <mxf/mxf_logging.h>
#include <mxf/mxf_macros.h>



static void free_index_array(TimecodeIndexArray** indexArray)
{
    if (*indexArray == NULL)
    {
        return;
    }
    
    SAFE_FREE(&(*indexArray)->elements);
    
    SAFE_FREE(indexArray);
}

static void free_index_array_in_list(void* data)
{
    TimecodeIndexArray* indexArray;
    
    if (data == NULL)
    {
        return;
    }
    
    indexArray = (TimecodeIndexArray*)data;
    free_index_array(&indexArray);
}

static int64_t timecode_to_position(const ArchiveTimecode* timecode)
{
    return timecode->hour * 60 * 60 * 25 +
        timecode->min * 60 * 25 +
        timecode->sec * 25 +
        timecode->frame;
}

static void position_to_timecode(int64_t position, ArchiveTimecode* timecode)
{
    timecode->hour = (uint8_t)(position / (60 * 60 * 25));
    timecode->min = (uint8_t)((position % (60 * 60 * 25)) / (60 * 25));
    timecode->sec = (uint8_t)(((position % (60 * 60 * 25)) % (60 * 25)) / 25);
    timecode->frame = (uint8_t)(((position % (60 * 60 * 25)) % (60 * 25)) % 25);
}

static int move_timecode_index_searcher(TimecodeIndexSearcher* searcher, int64_t position)
{
    TimecodeIndexArray* indexArray;
    TimecodeIndexElement* arrayElement;
    MXFListIterator searcherCopy = searcher->indexArrayIter;
    
    if (position == searcher->position)
    {
        return 1;
    }
    if (searcher->atEnd || position < searcher->position)
    {
        return 0;
    }
    
    
    indexArray = (TimecodeIndexArray*)mxf_get_iter_element(&searcher->indexArrayIter);
    while (1)
    {
        arrayElement = &indexArray->elements[searcher->elementNum];
        
        if (position < searcher->position + (arrayElement->duration - searcher->elementOffset))
        {
            /* found it the right element */
            searcher->elementOffset += position - searcher->position;
            searcher->position = position;
            searcher->beforeStart = 0;
            return 1;
        }
        else if (searcher->elementNum + 1 < indexArray->numElements)
        {
            /* move to next element in array */
            searcher->position += arrayElement->duration - searcher->elementOffset;
            searcher->elementOffset = 0;
            searcher->elementNum++;
        }
        else if (searcher->elementNum + 1 >= indexArray->numElements && 
            indexArray->numElements == searcher->index->arraySize)
        {
            /* move to next array in list */
            searcher->position += arrayElement->duration - searcher->elementOffset;
            searcher->elementOffset = 0;
            searcher->elementNum = 0;
            searcher->atEnd = !mxf_next_list_iter_element(&searcher->indexArrayIter);
            if (searcher->atEnd)
            {
                /* end of index */
                break;
            }
            indexArray = (TimecodeIndexArray*)mxf_get_iter_element(&searcher->indexArrayIter);
        }
        else
        {
            /* end of index */
            break;
        }
    }
    
    
    searcher->indexArrayIter = searcherCopy;
    return 0;
}



void initialise_timecode_index(TimecodeIndex* index, int arraySize)
{
    mxf_initialise_list(&index->indexArrays, free_index_array_in_list);
    index->arraySize = arraySize;
}

void clear_timecode_index(TimecodeIndex* index)
{
    mxf_clear_list(&index->indexArrays);
}

int add_timecode(TimecodeIndex* index, ArchiveTimecode* timecode)
{
    TimecodeIndexArray* newArray = NULL;
    TimecodeIndexArray* lastArray;
    int64_t timecodePos = timecode_to_position(timecode);
    
    if (mxf_get_list_length(&index->indexArrays) == 0)
    {
        CHK_MALLOC_OFAIL(newArray, TimecodeIndexArray);
        CHK_MALLOC_ARRAY_OFAIL(newArray->elements, TimecodeIndexElement, index->arraySize);
        newArray->numElements = 0;
        CHK_OFAIL(mxf_append_list_element(&index->indexArrays, newArray));
        newArray = NULL; /* list has ownership */
    }
    
    lastArray = (TimecodeIndexArray*)mxf_get_last_list_element(&index->indexArrays);
    
    if (lastArray->numElements != 0)
    {
        if (lastArray->elements[lastArray->numElements - 1].timecodePos + 
                lastArray->elements[lastArray->numElements - 1].duration == timecodePos)
        {
            /* timecode is previous + 1 */
            lastArray->elements[lastArray->numElements - 1].duration++;
        }
        else
        {
            /* timecode is not previous + 1 */
            
            if (lastArray->numElements == index->arraySize)
            {
                CHK_MALLOC_OFAIL(newArray, TimecodeIndexArray);
                CHK_MALLOC_ARRAY_OFAIL(newArray->elements, TimecodeIndexElement, index->arraySize);
                newArray->numElements = 0;
                CHK_OFAIL(mxf_append_list_element(&index->indexArrays, newArray));
                newArray = NULL; /* list has ownership */

                lastArray = (TimecodeIndexArray*)mxf_get_last_list_element(&index->indexArrays);
            }
            
            lastArray->numElements++;
            lastArray->elements[lastArray->numElements - 1].timecodePos = timecodePos;
            lastArray->elements[lastArray->numElements - 1].duration = 1;
        }
    }
    else
    {
        lastArray->numElements++;
        lastArray->elements[lastArray->numElements - 1].timecodePos = timecodePos;
        lastArray->elements[lastArray->numElements - 1].duration = 1;
    }
    
    
    return 1;
    
fail:
    free_index_array(&newArray);
    return 0;
}

void initialise_timecode_index_searcher(TimecodeIndex* index, TimecodeIndexSearcher* searcher)
{
    mxf_initialise_list_iter(&searcher->indexArrayIter, &index->indexArrays);
    searcher->elementNum = 0;
    searcher->elementOffset = 0;
    searcher->position = 0;
    searcher->index = index;
    searcher->atEnd = !mxf_next_list_iter_element(&searcher->indexArrayIter);
    searcher->beforeStart = 1;
}

int find_timecode(TimecodeIndexSearcher* searcher, int64_t position, ArchiveTimecode* timecode)
{
    TimecodeIndexArray* indexArray;
    TimecodeIndexElement* arrayElement;
    int64_t timecodePos;
    
    if (!move_timecode_index_searcher(searcher, position))
    {
        return 0;
    }
    
    indexArray = (TimecodeIndexArray*)mxf_get_iter_element(&searcher->indexArrayIter);
    arrayElement = &indexArray->elements[searcher->elementNum];
    timecodePos = arrayElement->timecodePos + searcher->elementOffset;
    position_to_timecode(timecodePos, timecode);
    
    return 1;
}

int find_position(TimecodeIndexSearcher* searcher, const ArchiveTimecode* timecode, int64_t* position)
{
    TimecodeIndexArray* indexArray;
    TimecodeIndexElement* arrayElement;
    int64_t timecodePos = timecode_to_position(timecode);
    int doneFirst = 0;
    MXFListIterator searcherCopy = searcher->indexArrayIter;

    if (searcher->atEnd)
    {
        return 0;
    }
    
    indexArray = (TimecodeIndexArray*)mxf_get_iter_element(&searcher->indexArrayIter);
    while (1)
    {
        arrayElement = &indexArray->elements[searcher->elementNum];
        
        if ((doneFirst || searcher->beforeStart || timecodePos > arrayElement->timecodePos + searcher->elementOffset) &&
            timecodePos >= arrayElement->timecodePos + searcher->elementOffset &&
            timecodePos < arrayElement->timecodePos + arrayElement->duration)
        {
            /* found it */
            searcher->position += timecodePos - (arrayElement->timecodePos + searcher->elementOffset);
            searcher->elementOffset = timecodePos - arrayElement->timecodePos;
            *position = searcher->position;
            searcher->beforeStart = 0;
            return 1;
        }
        else if (searcher->elementNum + 1 < indexArray->numElements)
        {
            /* move to next element in array */
            searcher->position += arrayElement->duration - searcher->elementOffset;
            searcher->elementOffset = 0;
            searcher->elementNum++;
        }
        else if (searcher->elementNum + 1 >= indexArray->numElements && 
            indexArray->numElements == searcher->index->arraySize)
        {
            /* move to next array in list */
            searcher->position += arrayElement->duration - searcher->elementOffset;
            searcher->elementOffset = 0;
            searcher->elementNum = 0;
            searcher->atEnd = !mxf_next_list_iter_element(&searcher->indexArrayIter);
            if (searcher->atEnd)
            {
                /* end of index */
                break;
            }
            indexArray = (TimecodeIndexArray*)mxf_get_iter_element(&searcher->indexArrayIter);
        }
        else
        {
            /* end of index */
            break;
        }
        
        doneFirst = 1;
    }
    
    
    searcher->indexArrayIter = searcherCopy;
    return 0;
}


int find_position_at_dual_timecode(TimecodeIndexSearcher* vitcSearcher, const ArchiveTimecode* vitcTimecode, 
    TimecodeIndexSearcher* ltcSearcher, const ArchiveTimecode* ltcTimecode, int64_t* position)
{
    TimecodeIndexSearcher vitcSearcherCopy = *vitcSearcher;
    TimecodeIndexSearcher ltcSearcherCopy = *ltcSearcher;
    int64_t vitcPosition;
    int64_t ltcPosition;

    if (vitcTimecode->hour == INVALID_TIMECODE_HOUR &&
        ltcTimecode->hour == INVALID_TIMECODE_HOUR)
    {
        /* can't find position when both timecode are invalid */
        return 0;
    }
    
    /* find the first hit */
    if (vitcTimecode->hour != INVALID_TIMECODE_HOUR &&
        !find_position(vitcSearcher, vitcTimecode, &vitcPosition))
    {
        goto fail;
    }
    if (ltcTimecode->hour != INVALID_TIMECODE_HOUR &&
        !find_position(ltcSearcher, ltcTimecode, &ltcPosition))
    {
        goto fail;
    }
    
    if (ltcTimecode->hour == INVALID_TIMECODE_HOUR)
    {
        /* move the LTC timecode to the same position */
        if (!move_timecode_index_searcher(ltcSearcher, vitcPosition))
        {
            goto fail;
        }
    }
    else if (vitcTimecode->hour == INVALID_TIMECODE_HOUR)
    {
        /* move the VITC timecode to the same position */
        if (!move_timecode_index_searcher(vitcSearcher, ltcPosition))
        {
            goto fail;
        }
    }
    else
    {
        /* if they aren't at the same position then look further */
        while (vitcPosition != ltcPosition)
        {
            if (vitcPosition < ltcPosition)
            {
                if (!find_position(vitcSearcher, vitcTimecode, &vitcPosition))
                {
                    goto fail;
                }
            }
            else
            {
                if (!find_position(ltcSearcher, ltcTimecode, &ltcPosition))
                {
                    goto fail;
                }
            }
        }
    }
    
    *position = vitcSearcher->position;
    return 1;
    
fail:
    /* restore the previous state */
    *vitcSearcher = vitcSearcherCopy;
    *ltcSearcher = ltcSearcherCopy;
    return 0;
}

