/*
 * $Id: mxf_utils.c,v 1.3 2007/09/11 13:24:55 stuart_hc Exp $
 *
 * General purpose utilities
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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


#if defined(_WIN32)

#include <sys/timeb.h>
#include <time.h>
#include <windows.h>

#else

#include <uuid/uuid.h>
#include <sys/time.h>

#endif


#include <mxf/mxf.h>


void mxf_print_key(const mxfKey* key)
{
    char keyStr[KEY_STR_SIZE];
    mxf_sprint_key(keyStr, key);
    printf("K = %s\n", keyStr);
}

void mxf_sprint_key(char* str, const mxfKey* key)
{
#if defined(_MSC_VER)
    _snprintf(
#else
    snprintf(
#endif
        str, KEY_STR_SIZE, "%02x %02x %02x %02x %02x %02x %02x %02x "
        "%02x %02x %02x %02x %02x %02x %02x %02x", 
        key->octet0, key->octet1, key->octet2, key->octet3,
        key->octet4, key->octet5, key->octet6, key->octet7,
        key->octet8, key->octet9, key->octet10, key->octet11,
        key->octet12, key->octet13, key->octet14, key->octet15);
}

void mxf_print_label(const mxfUL* label)
{
    mxf_print_key((const mxfKey*)label);
}

void mxf_sprint_label(char* str, const mxfUL* label)
{
    mxf_sprint_key(str, (const mxfKey*)label);
}

void mxf_print_umid(const mxfUMID* umid)
{
    char umidStr[UMID_STR_SIZE];
    mxf_sprint_umid(umidStr, umid);
    printf("UMID = %s\n", umidStr);
}

void mxf_sprint_umid(char* str, const mxfUMID* umid)
{
#if defined(_MSC_VER)
    _snprintf(
#else
    snprintf(
#endif
        str, UMID_STR_SIZE, "%02x %02x %02x %02x %02x %02x %02x %02x "
        "%02x %02x %02x %02x %02x %02x %02x %02x " 
        "%02x %02x %02x %02x %02x %02x %02x %02x " 
        "%02x %02x %02x %02x %02x %02x %02x %02x", 
        umid->octet0, umid->octet1, umid->octet2, umid->octet3,
        umid->octet4, umid->octet5, umid->octet6, umid->octet7,
        umid->octet8, umid->octet9, umid->octet10, umid->octet11,
        umid->octet12, umid->octet13, umid->octet14, umid->octet15,
        umid->octet16, umid->octet17, umid->octet18, umid->octet19,
        umid->octet20, umid->octet21, umid->octet22, umid->octet23,
        umid->octet24, umid->octet25, umid->octet26, umid->octet27,
        umid->octet28, umid->octet29, umid->octet30, umid->octet31);
}

void mxf_generate_uuid(mxfUUID* uuid)
{
#if defined(_WIN32)

    GUID guid;
    CoCreateGuid(&guid);
    memcpy(uuid, &guid, 16);
    
#else

    uuid_t newUUID;
    uuid_generate(newUUID);
    memcpy(uuid, newUUID, 16);
    
#endif
}

void mxf_get_timestamp_now(mxfTimestamp* now)
{
#if defined(_MSC_VER) && _MSC_VER < 1400

    /* NOTE: gmtime is not thread safe (not reentrant) */
    
    struct _timeb tb;
    struct tm gmt;
    
    memset(&gmt, 0, sizeof(struct tm));
    
    _ftime(&tb);
    memcpy(&gmt, gmtime(&tb.time), sizeof(struct tm)); /* memcpy does nothing if gmtime returns NULL */
    
    now->year = gmt.tm_year + 1900;
    now->month = gmt.tm_mon + 1;
    now->day = gmt.tm_mday;
    now->hour = gmt.tm_hour;
    now->min = gmt.tm_min;
    now->sec = gmt.tm_sec;
    now->qmsec = (uint8_t)(tb.millitm / 4 + 0.5); /* 1/250th second */
    
#elif defined(_MSC_VER)

    struct _timeb tb;
    struct tm gmt;
    struct tm* gmtPtr = NULL;
    
    memset(&gmt, 0, sizeof(struct tm));
    
    /* using the secure _ftime */
    _ftime_s(&tb);
    
    /* using the secure (and reentrant) gmtime */
    gmtime_s(&tb.time, &gmt);

    now->year = gmt.tm_year + 1900;
    now->month = gmt.tm_mon + 1;
    now->day = gmt.tm_mday;
    now->hour = gmt.tm_hour;
    now->min = gmt.tm_min;
    now->sec = gmt.tm_sec;
    now->qmsec = (uint8_t)(tb.millitm / 4 + 0.5); /* 1/250th second */
    
#else

    struct timeval tv;
    gettimeofday(&tv, NULL);

    /* use the reentrant gmtime */
    struct tm gmt;
    memset(&gmt, 0, sizeof(struct tm));
    gmtime_r(&tv.tv_sec, &gmt);
    
    now->year = gmt.tm_year + 1900;
    now->month = gmt.tm_mon + 1;
    now->day = gmt.tm_mday;
    now->hour = gmt.tm_hour;
    now->min = gmt.tm_min;
    now->sec = gmt.tm_sec;
    now->qmsec = (uint8_t)(tv.tv_usec / 4000 + 0.5); /* 1/250th second */

#endif
}


void mxf_generate_umid(mxfUMID* umid)
{
    mxfUUID uuid;
    
    umid->octet0 = 0x06;
    umid->octet1 = 0x0a;
    umid->octet2 = 0x2b;
    umid->octet3 = 0x34;
    umid->octet4 = 0x01;         
    umid->octet5 = 0x01;     
    umid->octet6 = 0x01; 
    umid->octet7 = 0x05; /* registry version */
    umid->octet8 = 0x01;
    umid->octet9 = 0x01;
    umid->octet10 = 0x0f; /* material type not identified */
    umid->octet11 = 0x20; /* UUID/UL material generation method, no instance method defined */
    
    umid->octet12 = 0x13;
    umid->octet13 = 0x00;
    umid->octet14 = 0x00;
    umid->octet15 = 0x00;
    
    /* Note: a UUID is mapped directly and a UL is half swapped */
    mxf_generate_uuid(&uuid);
    memcpy(&umid->octet16, &uuid, 16);
}

void mxf_generate_key(mxfKey* key)
{
    mxfUUID uuid;
    
    mxf_generate_uuid(&uuid);
    
    /* half-swap */
    memcpy(key, &uuid.octet8, 8);
    memcpy(&key->octet8, &uuid.octet0, 8);
}

