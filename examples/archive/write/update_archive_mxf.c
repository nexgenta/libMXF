/*
 * $Id: update_archive_mxf.c,v 1.2 2008/09/24 17:29:57 philipn Exp $
 *
 * Update an archive MXF file with new filename and LTO Infax data
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

/*
    Example: update LTA00000501.mxf which will be the first file on LTO tape 'LTA000005'
    
        ./update_archive_mxf --file LTA00000501.mxf \
            --infax "LTO|D3 preservation programme||2006-02-02||LME1306H|71|T||D3 PRESERVATION COPY||1732|LTA000005||LONPROG|1" \
            LTA00000501.mxf
    
    Check parse_infax_data() in write_archive_mxf.c for the expected Infax 
    string.    
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <write_archive_mxf.h>


static void usage(const char* cmd)
{
    fprintf(stderr, "Usage: %s [options] <MXF filename>\n", cmd);
    fprintf(stderr, "\n");
    fprintf(stderr, "Options: (options marked with * are required)\n");
    fprintf(stderr, "  -h, --help                 display this usage message\n");
    fprintf(stderr, "* --infax <string>           infax data string for the LTO\n");
    fprintf(stderr, "* --file <name>              filename of the MXF file stored on the LTO\n");
    fprintf(stderr, "\n");
}

int main(int argc, const char* argv[])
{
    const char* infaxString = NULL;
    const char* ltoMXFFilename = NULL;
    const char* mxfFilename = NULL;
    int cmdlnIndex = 1;
    InfaxData infaxData;
    

    while (cmdlnIndex + 1 < argc)
    {
        if (strcmp(argv[cmdlnIndex], "-h") == 0 ||
            strcmp(argv[cmdlnIndex], "--help") == 0)
        {
            usage(argv[0]);
            return 0;
        }
        else if (strcmp(argv[cmdlnIndex], "--infax") == 0)
        {
            infaxString = argv[cmdlnIndex + 1];
            cmdlnIndex += 2;
        }
        else if (strcmp(argv[cmdlnIndex], "--file") == 0)
        {
            ltoMXFFilename = argv[cmdlnIndex + 1];
            cmdlnIndex += 2;
        }
        else
        {
            if (cmdlnIndex + 1 != argc)
            {
                fprintf(stderr, "Unknown argument '%s'\n", argv[cmdlnIndex]);
                usage(argv[0]);
                return 1;
            }
            
            break;
        }
    }
    
    if (cmdlnIndex + 1 != argc)
    {
        fprintf(stderr, "Missing MXF filename\n");
        usage(argv[0]);
        return 1;
    }
    if (infaxString == NULL)
    {
        fprintf(stderr, "--infax is required\n");
        usage(argv[0]);
        return 1;
    }
    if (ltoMXFFilename == NULL)
    {
        fprintf(stderr, "--file is required\n");
        usage(argv[0]);
        return 1;
    }
    
    mxfFilename = argv[cmdlnIndex];
    cmdlnIndex++;
    
    
    if (!parse_infax_data(infaxString, &infaxData, 1))
    {
        fprintf(stderr, "ERROR: Failed to parse the Infax data string '%s'\n", infaxString);
        exit(1);
    }
    
    if (!update_archive_mxf_file(mxfFilename, ltoMXFFilename, &infaxData))
    {
        fprintf(stderr, "ERROR: Failed to update MXF file '%s'\n", mxfFilename);
        exit(1);
    }

    
    return 0;
}

