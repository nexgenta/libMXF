/*
 * $Id: main.c,v 1.1 2008/10/08 09:38:51 philipn Exp $
 *
 * Parse metadata from an Avid MXF file and print to screen
 *
 * Copyright (C) 2008  Philip de Nier <philipn@users.sourceforge.net>
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

#include "avid_mxf_info.h"



static void usage(const char* cmd)
{
    fprintf(stderr, "Usage: %s [options] <input>+\n", cmd);
    fprintf(stderr, "\n");
    fprintf(stderr, "Options: (options marked with * are required)\n");
    fprintf(stderr, "  -h, --help                 display this usage message\n");
}


int main(int argc, const char* argv[])
{
    int cmdlnIndex = 1;
    int inputFilenamesIndex;
    AvidMXFInfo info;
    int result;
    int i;
    const char* inputFilename;
    
    while (cmdlnIndex < argc)
    {
        if (strcmp(argv[cmdlnIndex], "-h") == 0 ||
            strcmp(argv[cmdlnIndex], "--help") == 0)
        {
            usage(argv[0]);
            return 0;
        }
        else
        {
            break;
        }
    }

    if (cmdlnIndex >= argc)
    {
        usage(argv[0]);
        fprintf(stderr, "Missing <input> filename\n");
        return 1;
    }
    
    inputFilenamesIndex = cmdlnIndex;



    for (i = inputFilenamesIndex; i < argc; i++)
    {
        inputFilename = argv[i];
        
        printf("\nFilename = %s\n", inputFilename);
        
        result = ami_read_info(inputFilename, &info, 1);
        if (result != 0)
        {
            switch (result)
            {
                case -2:
                    fprintf(stderr, "Failed to open file (%s)\n", inputFilename);
                    break;
                case -3:
                    fprintf(stderr, "Failed to read header partition (%s)\n", inputFilename);
                    break;
                case -4:
                    fprintf(stderr, "File is not OP-Atom (%s)\n", inputFilename);
                    break;
                case -1:
                default:
                    fprintf(stderr, "Failed to read info (%s)\n", inputFilename);
                    break;
            }
            
            continue;
        }
        
        ami_print_info(&info);
        
        ami_free_info(&info);
    }
    
    
    return 0;
}

