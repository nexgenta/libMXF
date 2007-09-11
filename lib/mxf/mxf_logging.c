/*
 * $Id: mxf_logging.c,v 1.2 2007/09/11 13:24:55 stuart_hc Exp $
 *
 * libMXF logging functions
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

#include <time.h>


#include <mxf/mxf_logging.h>


mxf_log_func mxf_log = mxf_log_default;
MXFLogLevel g_mxfLogLevel = MXF_DLOG;


static FILE* g_mxfFileLog = NULL;

static void logmsg(FILE* file, MXFLogLevel level, const char* format, va_list p_arg)
{
    switch (level)
    {
        case MXF_DLOG:
            fprintf(file, "Debug: ");
            break;            
        case MXF_ILOG:
            fprintf(file, "Info: ");
            break;            
        case MXF_WLOG:
            fprintf(file, "Warning: ");
            break;            
        case MXF_ELOG:
            fprintf(file, "ERROR: ");
            break;            
    };

    vfprintf(file, format, p_arg);
}

static void log_to_file(MXFLogLevel level, const char* format, ...)
{
    char timeStr[128];
    const time_t t = time(NULL);
    const struct tm* gmt = gmtime(&t);
    va_list p_arg;

    if (level < g_mxfLogLevel)
    {
        return;
    }
    
    assert(gmt != NULL);
    assert(g_mxfFileLog != NULL);
    if (g_mxfFileLog == NULL)
    {
        return;
    }
    
    strftime(timeStr, 128, "%Y-%m-%d %H:%M:%S", gmt);
    fprintf(g_mxfFileLog, "(%s) ", timeStr);
    
    va_start(p_arg, format);
    logmsg(g_mxfFileLog, level, format, p_arg);
    va_end(p_arg);
}



void mxf_log_default(MXFLogLevel level, const char* format, ...)
{
    va_list p_arg;
    
    if (level < g_mxfLogLevel)
    {
        return;
    }
    
    va_start(p_arg, format);
    if (level == MXF_ELOG)
    {
        logmsg(stderr, level, format, p_arg);
    }
    else
    {
        logmsg(stdout, level, format, p_arg);
    }
    va_end(p_arg);
}

int mxf_log_file_open(const char* filename)
{
    if ((g_mxfFileLog = fopen(filename, "wb")) == NULL)
    {
        return 0;
    }
    
    mxf_log = log_to_file;
    return 1;
}

void mxf_log_file_close()
{
    if (g_mxfFileLog != NULL)
    {
        fclose(g_mxfFileLog);
        g_mxfFileLog = NULL;
    }
}

