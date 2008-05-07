/*
 * $Id: test_timecode_index.c,v 1.2 2008/05/07 15:21:55 philipn Exp $
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


#define MAX_TIMECODE_SHOW       40

#define CHECK(cmd) \
    if (!(cmd)) \
    { \
        fprintf(stderr, "'%s' failed in %s:%d\n", #cmd, __FILE__, __LINE__); \
        exit(1); \
    }

#define INITIALISE_SEARCHERS() \
    initialise_timecode_index_searcher(&vitcIndex, &vitcSearcher); \
    initialise_timecode_index_searcher(&ltcIndex, &ltcSearcher);

    
    
static void increment_timecode(ArchiveTimecode* timecode)
{
    timecode->frame++;
    if (timecode->frame > 24)
    {
        timecode->frame = 0;
        timecode->sec++;
        if (timecode->sec > 59)
        {
            timecode->sec = 0;
            timecode->min++;
            if (timecode->min > 59)
            {
                timecode->min = 0;
                timecode->hour++;
                if (timecode->hour > 23)
                {
                    timecode->hour = 0;
                    timecode->frame++;
                }
            }
        }
    }
}

static void decrement_timecode(ArchiveTimecode* timecode)
{
    if (timecode->frame == 0)
    {
        timecode->frame = 24;
        if (timecode->sec == 0)
        {
            timecode->sec = 59;
            if (timecode->min == 0)
            {
                timecode->min = 59;
                if (timecode->hour == 0)
                {
                    timecode->hour = 23;
                }
                else
                {
                    timecode->hour--;
                }
            }
            else
            {
                timecode->min--;
            }
        }
        else
        {
            timecode->sec--;
        }
    }
    else
    {
        timecode->frame--;
    }
}


int main()
{
    TimecodeIndex vitcIndex;
    TimecodeIndex ltcIndex;
    ArchiveTimecode vitcTimecode;
    ArchiveTimecode ltcTimecode;
    TimecodeIndexSearcher vitcSearcher;
    TimecodeIndexSearcher ltcSearcher;
    int64_t vitcPosition;
    int64_t ltcPosition;
    int64_t position;
    int i;
    int total = 0;
    
    memset(&vitcTimecode, 0, sizeof(ArchiveTimecode));
    memset(&ltcTimecode, 0, sizeof(ArchiveTimecode));
    
    initialise_timecode_index(&vitcIndex, 512);
    initialise_timecode_index(&ltcIndex, 512);
    
    
    /* linear, equal */
    for (i = 0; i < 10; i++)
    {
        add_timecode(&vitcIndex, &vitcTimecode);
        add_timecode(&ltcIndex, &ltcTimecode);
        
        increment_timecode(&vitcTimecode);
        increment_timecode(&ltcTimecode);
        total++;
    }

    /* linear, not-equal */
    increment_timecode(&vitcTimecode);
    for (i = 0; i < 10; i++)
    {
        add_timecode(&vitcIndex, &vitcTimecode);
        add_timecode(&ltcIndex, &ltcTimecode);
        
        increment_timecode(&vitcTimecode);
        increment_timecode(&ltcTimecode);
        total++;
    }

    
    /* linear, not-equal, broken */
    for (i = 0; i < 10; i++)
    {
        add_timecode(&vitcIndex, &vitcTimecode);
        add_timecode(&ltcIndex, &ltcTimecode);
        
        if (i % 2 == 0)
        {
            increment_timecode(&vitcTimecode);
        }
        else if (i % 3 == 0)
        {
            increment_timecode(&vitcTimecode);
            increment_timecode(&vitcTimecode);
            increment_timecode(&vitcTimecode);
            increment_timecode(&ltcTimecode);
            increment_timecode(&ltcTimecode);
            increment_timecode(&ltcTimecode);
        }
        else
        {
            increment_timecode(&vitcTimecode);
            increment_timecode(&ltcTimecode);
        }
        total++;
    }
    
    /* non-linear, not-equal, broken */
    increment_timecode(&ltcTimecode);
    increment_timecode(&ltcTimecode);
    for (i = 0; i < 10; i++)
    {
        add_timecode(&vitcIndex, &vitcTimecode);
        add_timecode(&ltcIndex, &ltcTimecode);
        
        if (i % 2 == 0)
        {
            decrement_timecode(&vitcTimecode);
            decrement_timecode(&vitcTimecode);
            decrement_timecode(&vitcTimecode);
            decrement_timecode(&vitcTimecode);
            decrement_timecode(&ltcTimecode);
            decrement_timecode(&ltcTimecode);
            decrement_timecode(&ltcTimecode);
            decrement_timecode(&ltcTimecode);
        }
        else
        {
            increment_timecode(&vitcTimecode);
            increment_timecode(&ltcTimecode);
        }
        total++;
    }

    /* linear, broken, not equal */
    for (i = 0; i < 90 * 60 * 25; i++)
    {
        add_timecode(&vitcIndex, &vitcTimecode);
        add_timecode(&ltcIndex, &ltcTimecode);
        
        increment_timecode(&vitcTimecode);
        increment_timecode(&vitcTimecode);
        increment_timecode(&ltcTimecode);
        increment_timecode(&ltcTimecode);
        total++;
    }

    printf("Total timecodes = %d (%d minutes)\n", total, total / (60 * 25));
    printf("Memory size = %.2lf Mb\n", 2 * (sizeof(TimecodeIndex) + 
        mxf_get_list_length(&vitcIndex.indexArrays) * (sizeof(MXFListElement) + 
            sizeof(TimecodeIndexArray) + sizeof(TimecodeIndexElement) * vitcIndex.arraySize)) / 
            (1024.0 * 1024.0)); 
    
        
    /* print out the index */
    initialise_timecode_index_searcher(&vitcIndex, &vitcSearcher);
    initialise_timecode_index_searcher(&ltcIndex, &ltcSearcher);
    i = 0;
    while (i < MAX_TIMECODE_SHOW)
    {
        if (!find_timecode(&vitcSearcher, i, &vitcTimecode) ||
            !find_timecode(&ltcSearcher, i, &ltcTimecode))
        {
            if (!find_timecode(&vitcSearcher, i, &vitcTimecode))
            {
                printf("End of VITC\n");
            }
            if (!find_timecode(&ltcSearcher, i, &ltcTimecode))
            {
                printf("End of LTC\n");
            }
            break;
        }
        if (i != 0 && i % 10 == 0)
        {
            printf("\n");
        }
        printf("%3d    %02u:%02u:%02u:%02u  %02u:%02u:%02u:%02u\n",
            i,
            vitcTimecode.hour, vitcTimecode.min, vitcTimecode.sec, vitcTimecode.frame, 
            ltcTimecode.hour, ltcTimecode.min, ltcTimecode.sec, ltcTimecode.frame);
        i++;
    }

    
    printf("Check linear timecode search...\n");
    fflush(stdout);
    /* find timecode at position 0 */
    INITIALISE_SEARCHERS();
    CHECK(find_timecode(&vitcSearcher, 0, &vitcTimecode) &&
        find_timecode(&ltcSearcher, 0, &ltcTimecode));
    INITIALISE_SEARCHERS();
    CHECK(find_position(&vitcSearcher, &vitcTimecode, &vitcPosition) &&
        find_position(&ltcSearcher, &ltcTimecode, &ltcPosition));
    CHECK(vitcPosition == 0 && ltcPosition == 0);
    /* find timecode at position 5 */
    INITIALISE_SEARCHERS();
    CHECK(find_timecode(&vitcSearcher, 5, &vitcTimecode) &&
        find_timecode(&ltcSearcher, 5, &ltcTimecode));
    INITIALISE_SEARCHERS();
    CHECK(find_position(&vitcSearcher, &vitcTimecode, &vitcPosition) &&
        find_position(&ltcSearcher, &ltcTimecode, &ltcPosition));
    CHECK(vitcPosition == 5 && ltcPosition == 5);
    /* find timecode at position 5, then 6 and fail back one to 5 */
    INITIALISE_SEARCHERS();
    CHECK(find_timecode(&vitcSearcher, 5, &vitcTimecode) &&
        find_timecode(&ltcSearcher, 5, &ltcTimecode));
    CHECK(find_timecode(&vitcSearcher, 5, &vitcTimecode) &&
        find_timecode(&ltcSearcher, 5, &ltcTimecode));
    CHECK(find_timecode(&vitcSearcher, 6, &vitcTimecode) &&
        find_timecode(&ltcSearcher, 6, &ltcTimecode));
    CHECK(!find_timecode(&vitcSearcher, 5, &vitcTimecode) &&
        !find_timecode(&ltcSearcher, 5, &ltcTimecode));
    INITIALISE_SEARCHERS();
    CHECK(find_position(&vitcSearcher, &vitcTimecode, &vitcPosition) &&
        find_position(&ltcSearcher, &ltcTimecode, &ltcPosition));
    CHECK(vitcPosition == 6 && ltcPosition == 6);
    /* find timecode at position 9 */
    INITIALISE_SEARCHERS();
    CHECK(find_timecode(&vitcSearcher, 9, &vitcTimecode) &&
        find_timecode(&ltcSearcher, 9, &ltcTimecode));
    INITIALISE_SEARCHERS();
    CHECK(find_position(&vitcSearcher, &vitcTimecode, &vitcPosition) &&
        find_position(&ltcSearcher, &ltcTimecode, &ltcPosition));
    CHECK(vitcPosition == 9 && ltcPosition == 9);
    printf("Done.\n");

    
    printf("Check broken timecode search...\n");
    fflush(stdout);
    INITIALISE_SEARCHERS();
    vitcTimecode.hour = 0;
    vitcTimecode.min = 0;
    vitcTimecode.sec = 1;
    vitcTimecode.frame = 10;
    ltcTimecode.hour = 0;
    ltcTimecode.min = 0;
    ltcTimecode.sec = 1;
    ltcTimecode.frame = 6;
    CHECK(find_position(&vitcSearcher, &vitcTimecode, &vitcPosition) &&
        find_position(&ltcSearcher, &ltcTimecode, &ltcPosition));
    CHECK(vitcPosition == 30 && ltcPosition == 30);
    vitcTimecode.sec = 1;
    vitcTimecode.frame = 4;
    ltcTimecode.sec = 1;
    ltcTimecode.frame = 0;
    CHECK(find_position(&vitcSearcher, &vitcTimecode, &vitcPosition) &&
        find_position(&ltcSearcher, &ltcTimecode, &ltcPosition));
    CHECK(vitcPosition == 34 && ltcPosition == 34);
    vitcTimecode.sec = 0;
    vitcTimecode.frame = 19;
    ltcTimecode.sec = 0;
    ltcTimecode.frame = 15;
    CHECK(find_position(&vitcSearcher, &vitcTimecode, &vitcPosition) &&
        find_position(&ltcSearcher, &ltcTimecode, &ltcPosition));
    CHECK(vitcPosition == 39 && ltcPosition == 39);
    printf("Done.\n");
    

    printf("Check dual search...\n");
    fflush(stdout);
    INITIALISE_SEARCHERS();
    vitcTimecode.hour = 0;
    vitcTimecode.min = 0;
    vitcTimecode.sec = 0;
    vitcTimecode.frame = 1;
    ltcTimecode.hour = 0;
    ltcTimecode.min = 0;
    ltcTimecode.sec = 0;
    ltcTimecode.frame = 1;
    CHECK(find_position_at_dual_timecode(&vitcSearcher, &vitcTimecode, 
        &ltcSearcher, &ltcTimecode, &position));
    CHECK(position == 1);
    INITIALISE_SEARCHERS();
    vitcTimecode.frame = 3;
    ltcTimecode.hour = INVALID_TIMECODE_HOUR;
    CHECK(find_position_at_dual_timecode(&vitcSearcher, &vitcTimecode, 
        &ltcSearcher, &ltcTimecode, &position));
    CHECK(position == 3);
    ltcTimecode.hour = 0;
    vitcTimecode.frame = 7;
    ltcTimecode.frame = 7;
    CHECK(find_position_at_dual_timecode(&vitcSearcher, &vitcTimecode, 
        &ltcSearcher, &ltcTimecode, &position));
    CHECK(position == 7);
    vitcTimecode.frame = 12;
    ltcTimecode.frame = 11;
    CHECK(find_position_at_dual_timecode(&vitcSearcher, &vitcTimecode, 
        &ltcSearcher, &ltcTimecode, &position));
    CHECK(position == 11);
    vitcTimecode.sec = 1;
    vitcTimecode.frame = 2;
    ltcTimecode.sec = 0;
    ltcTimecode.frame = 24;
    CHECK(find_position_at_dual_timecode(&vitcSearcher, &vitcTimecode, 
        &ltcSearcher, &ltcTimecode, &position));
    CHECK(position == 24);
    vitcTimecode.sec = 1;
    vitcTimecode.frame = 3;
    ltcTimecode.sec = 0;
    ltcTimecode.frame = 24;
    CHECK(find_position_at_dual_timecode(&vitcSearcher, &vitcTimecode, 
        &ltcSearcher, &ltcTimecode, &position));
    CHECK(position == 25);
    vitcTimecode.sec = 1;
    vitcTimecode.frame = 5;
    ltcTimecode.sec = 1;
    ltcTimecode.frame = 0;
    CHECK(find_position_at_dual_timecode(&vitcSearcher, &vitcTimecode, 
        &ltcSearcher, &ltcTimecode, &position));
    CHECK(position == 27);
    INITIALISE_SEARCHERS();
    vitcTimecode.sec = 0;
    vitcTimecode.frame = 22;
    ltcTimecode.sec = 0;
    ltcTimecode.frame = 18;
    CHECK(find_position_at_dual_timecode(&vitcSearcher, &vitcTimecode, 
        &ltcSearcher, &ltcTimecode, &position));
    CHECK(position == 37);
    INITIALISE_SEARCHERS();
    vitcTimecode.hour = 3;
    vitcTimecode.min = 0;
    vitcTimecode.sec = 0;
    vitcTimecode.frame = 0;
    ltcTimecode.hour = 2;
    ltcTimecode.min = 59;
    ltcTimecode.sec = 59;
    ltcTimecode.frame = 21;
    CHECK(find_position_at_dual_timecode(&vitcSearcher, &vitcTimecode, 
        &ltcSearcher, &ltcTimecode, &position));
    CHECK(position == 135030);
    printf("Done.\n");

    
    clear_timecode_index(&vitcIndex);
    clear_timecode_index(&ltcIndex);
    return 0;
}

