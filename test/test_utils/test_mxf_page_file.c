/*
 * $Id: test_mxf_page_file.c,v 1.1 2008/10/27 18:00:39 john_f Exp $
 *
 * 
 *
 * Copyright (C) 2007  Philip de Nier <philipn@users.sourceforge.net>
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

#include <mxf/mxf.h>
#include <mxf/mxf_page_file.h>


#define PAGE_SIZE   (10 * 1024 * 1024)
#define DATA_SIZE   (PAGE_SIZE * 3 / 2)

static const char* g_testFile = "pagetest___%d.mxf";



#define CHECK(cmd) \
    if (!(cmd)) \
    { \
        fprintf(stderr, "'%s' failed in %s:%d\n", #cmd, __FILE__, __LINE__); \
        exit(1); \
    }

    
static void remove_test_files()
{
    char filename[4096];
    int count = 0;
    
    while (1)
    {
        sprintf(filename, g_testFile, count);
        if (remove(filename) != 0)
        {
            break;
        }
        count++;
    }
}

int main()
{
    MXFPageFile* mxfPageFile;
    MXFFile* mxfFile;
    uint8_t* data;
    int i;
    
    data = malloc(DATA_SIZE);
    
    
    remove_test_files();
    
    
    CHECK(mxf_page_file_open_new(g_testFile, PAGE_SIZE, &mxfPageFile));
    mxfFile = mxf_page_file_get_file(mxfPageFile);
    
    memset(data, 0, DATA_SIZE);
    
    CHECK(mxf_file_write(mxfFile, data, DATA_SIZE) == DATA_SIZE);
    
    mxf_file_close(&mxfFile);
    

    
    CHECK(mxf_page_file_open_modify(g_testFile, PAGE_SIZE, &mxfPageFile));
    mxfFile = mxf_page_file_get_file(mxfPageFile);

    memset(data, 1, DATA_SIZE);
    
    CHECK(mxf_file_size(mxfFile) == DATA_SIZE);
    CHECK(mxf_file_read(mxfFile, data, DATA_SIZE) == DATA_SIZE);
    CHECK(mxf_file_eof(mxfFile));
    CHECK(mxf_file_tell(mxfFile) == DATA_SIZE);
    
    CHECK(mxf_file_write(mxfFile, data, DATA_SIZE) == DATA_SIZE);
    CHECK(mxf_file_eof(mxfFile));
    CHECK(mxf_file_tell(mxfFile) == DATA_SIZE * 2);

    CHECK(mxf_file_seek(mxfFile, 0, SEEK_SET));
    CHECK(mxf_file_tell(mxfFile) == 0);
    CHECK(mxf_file_write(mxfFile, data, DATA_SIZE) == DATA_SIZE);
    
    mxf_file_close(&mxfFile);

    
    
    CHECK(mxf_page_file_open_read(g_testFile, &mxfPageFile));
    mxfFile = mxf_page_file_get_file(mxfPageFile);

    memset(data, 1, DATA_SIZE);
    
    CHECK(mxf_file_size(mxfFile) == DATA_SIZE * 2);
    CHECK(mxf_file_read(mxfFile, data, DATA_SIZE) == DATA_SIZE);
    CHECK(mxf_file_tell(mxfFile) == DATA_SIZE);
    CHECK(mxf_file_read(mxfFile, data, DATA_SIZE) == DATA_SIZE);
    CHECK(mxf_file_eof(mxfFile));
    CHECK(mxf_file_tell(mxfFile) == DATA_SIZE * 2);
    CHECK(mxf_file_seek(mxfFile, 0, SEEK_SET));
    CHECK(mxf_file_tell(mxfFile) == 0);
    CHECK(mxf_file_read(mxfFile, data, DATA_SIZE) == DATA_SIZE);
    CHECK(mxf_file_tell(mxfFile) == DATA_SIZE);
    CHECK(mxf_file_seek(mxfFile, DATA_SIZE + 1, SEEK_SET));
    CHECK(mxf_file_read(mxfFile, data, DATA_SIZE - 1) == DATA_SIZE - 1);
    CHECK(mxf_file_tell(mxfFile) == DATA_SIZE * 2);
    CHECK(mxf_file_seek(mxfFile, DATA_SIZE - 1, SEEK_SET));
    CHECK(mxf_file_tell(mxfFile) == DATA_SIZE - 1);
    CHECK(mxf_file_seek(mxfFile, DATA_SIZE * 2, SEEK_SET));
    CHECK(mxf_file_tell(mxfFile) == DATA_SIZE * 2);
    CHECK(mxf_file_seek(mxfFile, 0, SEEK_END));
    CHECK(mxf_file_tell(mxfFile) == DATA_SIZE * 2);
    CHECK(mxf_file_seek(mxfFile, -DATA_SIZE * 2, SEEK_END));
    CHECK(mxf_file_tell(mxfFile) == 0);
    CHECK(mxf_file_seek(mxfFile, DATA_SIZE - 5, SEEK_CUR));
    CHECK(mxf_file_tell(mxfFile) == DATA_SIZE - 5);
    CHECK(mxf_file_read(mxfFile, data, DATA_SIZE) == DATA_SIZE);
    
    mxf_file_close(&mxfFile);

    
    
    CHECK(mxf_page_file_open_new("pagetest_out__%d.mxf", PAGE_SIZE, &mxfPageFile));
    mxfFile = mxf_page_file_get_file(mxfPageFile);
    
    CHECK(mxf_file_write(mxfFile, data, DATA_SIZE) == DATA_SIZE);
    
    mxf_file_close(&mxfFile);

    CHECK(mxf_page_file_remove("pagetest_out__%d.mxf"));
    

    /* test forward truncate */
    
    CHECK(mxf_page_file_open_new(g_testFile, PAGE_SIZE, &mxfPageFile));
    mxfFile = mxf_page_file_get_file(mxfPageFile);
    
    memset(data, 0, DATA_SIZE);
    
    for (i = 0; i < 10; i++)
    {
        CHECK(mxf_file_write(mxfFile, data, DATA_SIZE) == DATA_SIZE);
    }
    
    mxf_file_close(&mxfFile);

    
    CHECK(mxf_page_file_open_modify(g_testFile, PAGE_SIZE, &mxfPageFile));
    mxfFile = mxf_page_file_get_file(mxfPageFile);

    memset(data, 1, DATA_SIZE);

    CHECK(mxf_page_file_forward_truncate(mxfPageFile));    
    
    for (i = 0; i < 5; i++)
    {
        CHECK(mxf_file_read(mxfFile, data, DATA_SIZE) == DATA_SIZE);
    }
    CHECK(mxf_page_file_forward_truncate(mxfPageFile));    

    for (i = 0; i < 4; i++)
    {
        CHECK(mxf_file_read(mxfFile, data, DATA_SIZE) == DATA_SIZE);
    }
    CHECK(mxf_page_file_forward_truncate(mxfPageFile));    

    CHECK(mxf_file_read(mxfFile, data, DATA_SIZE) == DATA_SIZE);
    CHECK(mxf_page_file_forward_truncate(mxfPageFile));    
    
    mxf_file_close(&mxfFile);

    CHECK(mxf_page_file_remove(g_testFile));
    
    
    free(data);
    
    return 0;
}

