/*
 * $Id: archive_types.h,v 1.1 2007/09/11 13:24:46 stuart_hc Exp $
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
 
#ifndef __ARCHIVE_TYPES_H__
#define __ARCHIVE_TYPES_H__


#ifdef __cplusplus
extern "C" 
{
#endif


#include <mxf/mxf_types.h>


#define INVALID_TIMECODE_HOUR           0xff


#define FORMAT_SIZE                     7
#define PROGTITLE_SIZE                  73
#define EPTITLE_SIZE                    145
#define MAGPREFIX_SIZE                  2
#define PROGNO_SIZE                     9
#define SPOOLSTATUS_SIZE                2
#define SPOOLDESC_SIZE                  30
#define MEMO_SIZE                       121
#define SPOOLNO_SIZE                    15
#define ACCNO_SIZE                      15
#define CATDETAIL_SIZE                  11

/* "the string sizes above" * 2 + 2 * "timestamp size" + "duration size" + 
14 * ("local tag" + "local length")
= 430 * 2 + 2 * 8 + 8 + 14 * (2 + 2)
= 940 */
#define COMPLETE_INFAX_EXTERNAL_SIZE    940


typedef struct
{
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint8_t frame;
    int dropFrame;
} ArchiveTimecode;

typedef struct
{
    int64_t position;
    ArchiveTimecode vitcTimecode;
    ArchiveTimecode ltcTimecode;
    
    int16_t redFlash;
    int16_t spatialPattern;
    int16_t luminanceFlash;
    mxfBoolean extendedFailure;
} PSEFailure;

typedef struct
{
    ArchiveTimecode vitcTimecode;
    ArchiveTimecode ltcTimecode;
    uint8_t errorCode;
} VTRError;

typedef struct
{
    int64_t position;
    uint8_t errorCode;
} VTRErrorAtPos;

typedef struct
{
    char format[FORMAT_SIZE];
    char progTitle[PROGTITLE_SIZE];
    char epTitle[EPTITLE_SIZE];
    mxfTimestamp txDate; /* only date part is relevant */
    char magPrefix[MAGPREFIX_SIZE];
    char progNo[PROGNO_SIZE];
    char spoolStatus[SPOOLSTATUS_SIZE];
    mxfTimestamp stockDate; /* only date part is relevant */
    char spoolDesc[SPOOLDESC_SIZE];
    char memo[MEMO_SIZE];
    int64_t duration; /* number of seconds */
    char spoolNo[SPOOLNO_SIZE]; /* max 4 character prefix followed by integer (max 10 digits) */
                      /* used as the tape SourcePackage name and part of the MaterialPackage name */
    char accNo[ACCNO_SIZE]; /* max 4 character prefix followed by integer (max 10 digits) */
    char catDetail[CATDETAIL_SIZE];
} InfaxData;




#ifdef __cplusplus
}
#endif


#endif

