/*
 * $Id: test_avidp2transfer.cpp,v 1.3 2008/10/29 17:54:26 john_f Exp $
 *
 * Tests transfer of MXF files referenced in an Avid AAF composition to P2
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
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <avidp2transfer.h>

using namespace std;


void test_progress(float percentCompleted)
{
    printf("\r%6.2f%% completed", percentCompleted);
    fflush(stdout);
}


void usage(const char* cmd)
{
    fprintf(stderr, "%s -p prefix [-c] -r p2path -i filename\n", cmd);
    fprintf(stderr, "\n");
    fprintf(stderr, "    -p prefix        add the prefix to the MXF filepath\n");
    fprintf(stderr, "    -c               omit the colon after the MXF filepath drive letter\n");
    fprintf(stderr, "    -r p2path        directory path to the P2 card root directory\n");
    fprintf(stderr, "    -i filename      Avid AAF filename containing a sequence referencing the MXF file\n");
    fprintf(stderr, "\n");
}

int main(int argc, const char* argv[])
{
    const char* p2path = NULL;
    const char* filename = NULL;
    const char* prefix = NULL;
    bool omitDriveColon = false;
    int cmdlIndex;

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
            if (filename != NULL)
            {
                fprintf(stderr, "Too many inputs\n");
                return 0;
            }
            filename = argv[cmdlIndex + 1];
            cmdlIndex += 2;
        }
        else if (!strcmp(argv[cmdlIndex], "-p"))
        {
            if (cmdlIndex >= argc-1)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing -p argument\n");
                return 1;
            }
            prefix = argv[cmdlIndex + 1];
            cmdlIndex += 2;
        }
        else if (!strcmp(argv[cmdlIndex], "-c"))
        {
            omitDriveColon = true;
            cmdlIndex += 1;
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
        fprintf(stderr, "Missing p2 path\n");
        return 1;
    }
    if (filename == NULL)
    {
        usage(argv[0]);
        fprintf(stderr, "Missing input AAF filename\n");
        return 1;
    }
    
    try
    {
        AvidP2Transfer transfer(filename, test_progress, NULL, prefix, omitDriveColon);

        printf("Estimated total storage size = %"PFu64" bytes\n", transfer.totalStorageSizeEstimate);
        
        vector<APTTrackInfo>::const_iterator iter;
        for (iter = transfer.trackInfo.begin(); iter != transfer.trackInfo.end(); iter++)
        {
            printf("Input MXF file: '%s'\n", (*iter).mxfFilename.c_str());
            if ((*iter).isPicture)
            {
                printf("    Type = Picture\n");
            }
            else
            {
                printf("    Type = Sound\n");
            }
            printf("    Name = '%s'\n", (*iter).name.c_str());
            printf("    CompositionMob track length = (%d/%d) %"PFi64"\n",
                (*iter).compositionEditRate.numerator, (*iter).compositionEditRate.denominator,
                (*iter).compositionTrackLength);
            printf("    SourceMob track length = (%d/%d) %"PFi64"\n", 
                (*iter).sourceEditRate.numerator, (*iter).sourceEditRate.denominator,
                (*iter).sourceTrackLength);
        }

        if (transfer.trackInfo.size() > 0)
        {
            transfer.transferToP2(p2path);
            printf("\nCreated clip '%s'\n", transfer.clipName.c_str());
        }
        else
        {
            fprintf(stderr, "No tracks to transfer\n");
        }
    }
    catch (APTException& ex)
    {
        fprintf(stderr, "Exception: %s\n", ex.getMessage());
        return 1;
    }
    
    return 0;
}
