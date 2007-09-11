/*
 * $Id: test_avid_mxf_to_p2.c,v 1.2 2007/09/11 13:24:48 stuart_hc Exp $
 *
 * Tests transfers of Avid MXF files to P2
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

#include <avid_mxf_to_p2.h>


void usage(const char* cmd)
{
    fprintf(stderr, "%s -r p2path (-i filename)+\n", cmd);
    fprintf(stderr, "\n");
    fprintf(stderr, "    -r p2path        directory path to the P2 card root directory\n");
    fprintf(stderr, "    -i filename      input MXF filename\n");
    fprintf(stderr, "\n");
}

int main(int argc, const char* argv[])
{
    const char* p2path = NULL;
    char* inputFilenames[17];
    int inputIndex;
    int cmdlIndex;
    AvidMXFToP2Transfer* transfer = NULL;
    int isComplete;
    int i;

    inputIndex = 0;
    cmdlIndex = 1;
    while (cmdlIndex < argc)
    {
        if (!strcmp(argv[cmdlIndex], "-r"))
        {
            if (cmdlIndex >= argc-1)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing -r argument\n");
                return 1;
            }
            p2path = argv[cmdlIndex + 1];
            cmdlIndex += 2;
        }
        else if (!strcmp(argv[cmdlIndex], "-i"))
        {
            if (cmdlIndex >= argc-1)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing -i argument\n");
                return 1;
            }
            if (inputIndex >= 17)
            {
                fprintf(stderr, "Too many inputs\n");
                goto fail;
            }
            inputFilenames[inputIndex] = (char*)malloc(sizeof(char) * strlen(argv[cmdlIndex + 1]) + 1);
            if (inputFilenames[inputIndex] == NULL)
            {
                goto fail;
            }
            strcpy(inputFilenames[inputIndex], argv[cmdlIndex + 1]);
            inputIndex++;
            cmdlIndex += 2;
        }
        else
        {
            usage(argv[0]);
            fprintf(stderr, "Unknown argument '%s'\n", argv[cmdlIndex]);
            return 1;
        }
    }
    
    if (p2path == NULL)
    {
        usage(argv[0]);
        fprintf(stderr, "Missing -r p2path\n");
        goto fail;
    }
    if (inputIndex == 0)
    {
        usage(argv[0]);
        fprintf(stderr, "No -i filename inputs provided\n");
        goto fail;
    }

    if (!prepare_transfer(inputFilenames, inputIndex, 900000, 0, 
        NULL, NULL, &transfer))
    {
        fprintf(stderr, "prepare_transfer failed\n");
        goto fail;
    }
    
    if (!transfer_avid_mxf_to_p2(p2path, transfer, &isComplete))
    {
        fprintf(stderr, "transfer_avid_mxf_to_p2 failed\n");
        goto fail;
    }
    
    free_transfer(&transfer);
    
    for (i = 0; i < inputIndex; i++)
    {
        free(inputFilenames[i]);
    }
    
    return 0;
    
    
fail:
    free_transfer(&transfer);
    for (i = 0; i < inputIndex; i++)
    {
        free(inputFilenames[i]);
    }
    return 1;
}
