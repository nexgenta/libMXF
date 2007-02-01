/*
 * $Id: avidp2transfer.h,v 1.1 2007/02/01 10:31:43 philipn Exp $
 *
 * Reads an Avid AAF composition file and transfers referenced MXF files to P2
 *
 * Copyright (C) 2006  Philip de Nier <philipn@users.sourceforge.net>
 * Copyright (C) 2006  Stuart Cunningham <stuart_hc@users.sourceforge.net>
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
 
#ifndef __AVIDP2TRANSFER_H__
#define __AVIDP2TRANSFER_H__


#include <vector>
#include <string>

#include <avid_mxf_to_p2.h>



#if defined(_MSC_VER) && defined(_WIN32)

typedef unsigned char avtUInt8;
typedef unsigned short int avtUInt16;
typedef unsigned long int avtUInt32;

typedef unsigned __int64 avtUInt64;

typedef signed char avtInt8;
typedef signed short int avtInt16;
typedef signed long int avtInt32;
typedef __int64 avtInt64;

#else

#include <inttypes.h>

typedef uint8_t	avtUInt8;
typedef uint16_t avtUInt16;
typedef uint32_t avtUInt32;
typedef uint64_t avtUInt64;

typedef int8_t avtInt8;
typedef int16_t avtInt16;
typedef int32_t avtInt32;
typedef int64_t avtInt64;

#endif


class APTException
{
public:
    APTException(const char *format, ...);
    ~APTException();
    
    const char* getMessage();
    
private:
    std::string _message;
};

typedef struct _APTRational
{
    avtInt32 numerator;
    avtInt32 denominator;
} APTRational;


class APTTrackInfo
{
public:
    bool isPicture;
    APTRational compositionEditRate; // the track edit rate in the CompositionMob
    avtInt64 compositionTrackLength; // the track length in the CompositionMob
    APTRational sourceEditRate; // the track edit rate in the SourceMob
    avtInt64 sourceTrackLength; // the track length in the SourceMob 
    std::string mxfFilename;
    std::string name;
};


typedef void (*progress_callback) (float percentCompleted);
    
typedef int (*insert_timecode_callback) (avtUInt8* frame, avtUInt32 frameSize, 
    avtInt64 timecode, int drop);


class AvidP2Transfer
{
public:
    AvidP2Transfer(const char* aafFilename, progress_callback progress,
        insert_timecode_callback insertTimecode,
        const char* filepathPrefix, bool omitDriveColon);
    ~AvidP2Transfer();

    bool transferToP2(const char* p2path);
    
    std::string clipName;
    avtUInt64 totalStorageSizeEstimate;  /* not an exact size */
    
    std::vector<APTTrackInfo> trackInfo;
    
private:
    void processAvidComposition(const char* filename);
    std::string AvidP2Transfer::rewriteFilepath(std::string filepath);
    
    AvidMXFToP2Transfer* _transfer;
    bool _readyToTransfer;

    std::string _filepathPrefix;
    bool _omitDriveColon;
    
    avtInt64 _timecodeStart;
    bool _timecodeDrop;
    avtUInt16 _timecodeFPS;
};




#endif 


