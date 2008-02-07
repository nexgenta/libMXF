/*
 * $Id: mxf_page_file.c,v 1.1 2008/02/07 15:02:08 john_f Exp $
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
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <mxf/mxf.h>
#include <mxf/mxf_page_file.h>


#define MAX_FILE_DESCRIPTORS        32

#define PAGE_ALLOC_INCR             64


typedef enum
{
    READ_MODE,
    WRITE_MODE,
    MODIFY_MODE
} FileMode;

typedef struct FileDescriptor
{
    struct FileDescriptor* prev;
    struct FileDescriptor* next;
    
    struct Page* page;
    
    FILE* file;
} FileDescriptor;

typedef struct Page
{
    FileDescriptor* fileDescriptor;
    int wasOpenedBefore;
    int64_t index;
    
    int64_t size;
    int64_t offset;
} Page;

struct MXFFileSysData
{
    int64_t pageSize;
    FileMode mode;
    char* filenameTemplate;
    
    int64_t position;
    
    Page* pages;
    int64_t numPages;
    int64_t numPagesAllocated;

    FileDescriptor* fileDescriptorHead;
    FileDescriptor* fileDescriptorTail;
    int numFileDescriptors;
};


static int open_file(MXFFileSysData* sysData, Page* page)
{
    FILE* newFile = NULL;
    char filename[4096];
    FileDescriptor* newFileDescriptor = NULL;
    

    /* move file descriptor to tail if already open, and return */    
    if (page->fileDescriptor != NULL)
    {
        if (page->fileDescriptor == sysData->fileDescriptorTail)
        {
            return 1;
        }
        
        /* extract file descriptor */
        if (page->fileDescriptor->next != NULL)
        {
            page->fileDescriptor->next->prev = page->fileDescriptor->prev;
        }
        if (page->fileDescriptor->prev != NULL)
        {
            page->fileDescriptor->prev->next = page->fileDescriptor->next;
        }
        if (sysData->fileDescriptorHead == page->fileDescriptor)
        {
            sysData->fileDescriptorHead = page->fileDescriptor->next;
        }
        
        /* put file descriptor at tail */
        page->fileDescriptor->next = NULL;
        page->fileDescriptor->prev = sysData->fileDescriptorTail;
        if (sysData->fileDescriptorTail != NULL)
        {
            sysData->fileDescriptorTail->next = page->fileDescriptor;
        }
        sysData->fileDescriptorTail = page->fileDescriptor;
        
        return 1;
    }
    
    
    /* close the least used file descriptor (the head) if too many file descriptors are open */
    if (sysData->numFileDescriptors >= MAX_FILE_DESCRIPTORS)
    {
        if (sysData->fileDescriptorTail == sysData->fileDescriptorHead)
        {
            /* single file descriptor */
            
            sysData->fileDescriptorHead->page->fileDescriptor = NULL;
            fclose(sysData->fileDescriptorHead->file);
            free(sysData->fileDescriptorHead);
            
            sysData->fileDescriptorHead = NULL;
            sysData->fileDescriptorTail = NULL;
            sysData->numFileDescriptors--;
        }
        else
        {
            /* multiple file descriptors */
            
            FileDescriptor* newHead = sysData->fileDescriptorHead->next;
            
            sysData->fileDescriptorHead->page->fileDescriptor = NULL;
            fclose(sysData->fileDescriptorHead->file);
            SAFE_FREE(&sysData->fileDescriptorHead);
    
            sysData->fileDescriptorHead = newHead;
            newHead->prev = NULL;
            sysData->numFileDescriptors--;
        }
    }
    
    /* open the file */
    sprintf(filename, sysData->filenameTemplate, page->index);
    switch (sysData->mode)
    {
        case READ_MODE:
            newFile = fopen(filename, "rb");
            break;
        case WRITE_MODE:
            if (!page->wasOpenedBefore)
            {
                newFile = fopen(filename, "w+b");
            }
            else
            {
                newFile = fopen(filename, "r+b");
            }
            break;
        case MODIFY_MODE:
            newFile = fopen(filename, "r+b");
            if (newFile == NULL)
            {
                newFile = fopen(filename, "w+b");
            }
            break;
    }
    if (newFile == NULL)
    {
        mxf_log(MXF_ELOG, "Failed to open paged mxf file '%s': %s\n", filename, strerror(errno));
        return 0;
    }
    
    /* create the new file descriptor */
    CHK_MALLOC_OFAIL(newFileDescriptor, FileDescriptor);
    memset(newFileDescriptor, 0, sizeof(*newFileDescriptor));
    newFileDescriptor->file = newFile;
    newFile = NULL;
    newFileDescriptor->page = page;
    
    page->fileDescriptor = newFileDescriptor;
    page->wasOpenedBefore = 1;
    page->offset = 0;
    
    if (sysData->fileDescriptorTail != NULL)
    {
        sysData->fileDescriptorTail->next = newFileDescriptor;
    }
    newFileDescriptor->prev = sysData->fileDescriptorTail;
    sysData->fileDescriptorTail = newFileDescriptor;
    if (sysData->fileDescriptorHead == NULL)
    {
        sysData->fileDescriptorHead = newFileDescriptor;
    }
    sysData->numFileDescriptors++;

    
    return 1;
    
fail:
    if (newFile != NULL)
    {
        fclose(newFile);
    }
    return 0;
}


static Page* open_page(MXFFileSysData* sysData, int64_t position)
{
    int64_t page = position / sysData->pageSize;
    if (page > sysData->numPages)
    {
        /* only allowed to open pages 0 .. last + 1 */
        return 0;
    }
    
    if (page == sysData->numPages)
    {
        if (sysData->mode == READ_MODE)
        {
            /* no more pages to open */
            return 0;
        }

        if (sysData->numPages == sysData->numPagesAllocated)
        {
            /* reallocate the pages */
            
            Page* newPages;
            CHK_MALLOC_ARRAY_ORET(newPages, Page, sysData->numPagesAllocated + PAGE_ALLOC_INCR);
            memcpy(newPages, sysData->pages, sizeof(Page) * sysData->numPagesAllocated);
            free(sysData->pages);
            sysData->pages = newPages;
            sysData->numPagesAllocated += PAGE_ALLOC_INCR;
            
            /* reset the link back from file descriptors to the new pages */
            int i;
            for (i = 0; i < sysData->numPages; i++)
            {
                if (sysData->pages[i].fileDescriptor != NULL)
                {
                    sysData->pages[i].fileDescriptor->page = &sysData->pages[i];
                }
            }
        }
        
        /* set new page data */
        memset(&sysData->pages[sysData->numPages], 0, sizeof(Page));
        sysData->pages[sysData->numPages].index = sysData->numPages;
        
        sysData->numPages++;
    }
    
    /* open the file */
    if (!open_file(sysData, &sysData->pages[page]))
    {
        return 0;
    }
    
    return &sysData->pages[page];
}

static uint32_t read_from_page(MXFFileSysData* sysData, uint8_t* data, uint32_t count)
{
    Page* page = open_page(sysData, sysData->position);
    if (page == 0)
    {
        return 0;
    }
    
    if (sysData->position > sysData->pageSize * page->index + page->size)
    {
        /* can't read beyond the end of the data in the page */
        /* TODO: assertion here? */
        return 0;
    }
    
    /* set the file at the current position */
    if (page->offset < 0 ||
        sysData->position != sysData->pageSize * page->index + page->offset)
    {
        int64_t offset = sysData->position - sysData->pageSize * page->index;
        if (fseeko(page->fileDescriptor->file, offset, SEEK_SET) != 0)
        {
            page->offset = -1; /* invalidate the position within the page */
            return 0;
        }
        page->offset = offset;
        page->size = (page->offset > page->size) ? page->offset : page->size;
    }
    
    /* read count bytes or 'till the end of the page */
    uint32_t numRead = (count > (uint32_t)(sysData->pageSize - page->offset)) ? 
        (uint32_t)(sysData->pageSize - page->offset) : count;
    numRead = fread(data, 1, numRead, page->fileDescriptor->file);
    
    page->offset += numRead;
    page->size = (page->offset > page->size) ? page->offset : page->size;

    sysData->position += numRead;
    
    return numRead;
}

static uint32_t write_to_page(MXFFileSysData* sysData, const uint8_t* data, uint32_t count)
{
    Page* page = open_page(sysData, sysData->position);
    if (page == 0)
    {
        return 0;
    }

    if (sysData->position > sysData->pageSize * page->index + page->size)
    {
        /* can't write from beyond the end of the data in the page */
        /* TODO: assertion here? */
        return 0;
    }
    
    /* set the file at the current position */
    if (page->offset < 0 ||
        sysData->position != sysData->pageSize * page->index + page->offset)
    {
        int64_t offset = sysData->position - sysData->pageSize * page->index;
        if (fseeko(page->fileDescriptor->file, offset, SEEK_SET) != 0)
        {
            page->offset = -1; /* invalidate the position within the page */
            return 0;
        }
        page->offset = offset;
        page->size = (page->offset > page->size) ? page->offset : page->size;
    }
    
    /* write count bytes or 'till the end of the page */
    uint32_t numWrite = (count > (uint32_t)(sysData->pageSize - page->offset)) ? 
        (uint32_t)(sysData->pageSize - page->offset) : count;
    numWrite = fwrite(data, 1, numWrite, page->fileDescriptor->file);

    page->offset += numWrite;
    page->size = (page->offset > page->size) ? page->offset : page->size;
    
    
    sysData->position += numWrite;
    
    return numWrite;
}





static void free_page_file(MXFFileSysData* sysData)
{
    if (sysData == NULL)
    {
        return;
    }
    
    free(sysData);
}

static void page_file_close(MXFFileSysData* sysData)
{
    if (sysData == NULL)
    {
        return;
    }
    
    SAFE_FREE(&sysData->filenameTemplate);
    
    SAFE_FREE(&sysData->pages);
    sysData->numPages = 0;
    sysData->numPagesAllocated = 0;
    
    FileDescriptor* fd = sysData->fileDescriptorHead;
    FileDescriptor* nextFd;
    while (fd != NULL)
    {
        if (fd->file != NULL)
        {
            fclose(fd->file);
        }
        nextFd = fd->next;
        SAFE_FREE(&fd);
        fd = nextFd;
    }
    sysData->fileDescriptorHead = NULL;
    sysData->fileDescriptorTail = NULL;
    sysData->numFileDescriptors = 0;
    
    sysData->position = 0;
}

static int64_t page_file_size(MXFFileSysData* sysData)
{
    if (sysData == NULL)
    {
        return -1;
    }
    
    if (sysData->numPages == 0)
    {
        return 0;
    }
    
    return sysData->pageSize * (sysData->numPages - 1) + sysData->pages[sysData->numPages - 1].size;
}

static uint32_t page_file_read(MXFFileSysData* sysData, uint8_t* data, uint32_t count)
{
    uint32_t numRead = 0;
    uint32_t totalRead = 0;
    while (totalRead < count)
    {
        numRead = read_from_page(sysData, &data[totalRead], count - totalRead);
        totalRead += numRead;
        
        if (numRead == 0)
        {
            break;
        }
    }

    return totalRead;
}

static uint32_t page_file_write(MXFFileSysData* sysData, const uint8_t* data, uint32_t count)
{
    uint32_t numWrite = 0;
    uint32_t totalWrite = 0;
    while (totalWrite < count)
    {
        numWrite = write_to_page(sysData, &data[totalWrite], count - totalWrite);
        totalWrite += numWrite;
        
        if (numWrite == 0)
        {
            break;
        }
    }

    return totalWrite;
}

static int page_file_getchar(MXFFileSysData* sysData)
{
    uint8_t data[1];
    
    if (read_from_page(sysData, &data[0], 1) == 0)
    {
        return EOF;
    }
    
    return (int)data[0];
}

static int page_file_putchar(MXFFileSysData* sysData, int c)
{
    uint8_t data[1];
    data[0] = (char)c;
    
    if (write_to_page(sysData, &data[0], 1) == 0)
    {
        return EOF;
    }
    
    return c;
}

static int page_file_eof(MXFFileSysData* sysData)
{
    int64_t size = page_file_size(sysData);
    if (size < 0)
    {
        return 1;
    }
    
    return sysData->position >= size;
}

static int page_file_seek(MXFFileSysData* sysData, int64_t offset, int whence)
{
    int64_t size = page_file_size(sysData);
    if (size < 0)
    {
        return 0;
    }
    
    int64_t position;
    switch (whence)
    {
        case SEEK_SET:
            position = offset;
            break;
        case SEEK_CUR:
            position = sysData->position + offset;
            break;
        case SEEK_END:
            position = size + offset;
            break;
        default:
            position = sysData->position;
            break;
    }
    if (position < 0 || position > size)
    {
        return 0;
    }
    
    sysData->position = position;
    
    return 1;
}

static int64_t page_file_tell(MXFFileSysData* sysData)
{
    return sysData->position;
}

static int page_file_is_seekable(MXFFileSysData* sysData)
{
    return sysData != NULL;
}



int mxf_page_file_open_new(const char* filenameTemplate, int64_t pageSize, MXFFile** mxfFile)
{
    MXFFile* newMXFFile = NULL;
    
    if (strstr(filenameTemplate, "%d") == NULL)
    {
        mxf_log(MXF_ELOG, "Filename template '%s' doesn't contain %%d\n", filenameTemplate);
        return 0;
    }
    
    CHK_MALLOC_ORET(newMXFFile, MXFFile);
    memset(newMXFFile, 0, sizeof(*newMXFFile));

    newMXFFile->close = page_file_close;
    newMXFFile->read = page_file_read;
    newMXFFile->write = page_file_write;
    newMXFFile->getchar = page_file_getchar;
    newMXFFile->putchar = page_file_putchar;
    newMXFFile->eof = page_file_eof;
    newMXFFile->seek = page_file_seek;
    newMXFFile->tell = page_file_tell;
    newMXFFile->is_seekable = page_file_is_seekable;
    newMXFFile->size = page_file_size;
    newMXFFile->free_sys_data = free_page_file;
    
    
    CHK_MALLOC_OFAIL(newMXFFile->sysData, MXFFileSysData);
    memset(newMXFFile->sysData, 0, sizeof(*newMXFFile->sysData));
    
    CHK_MALLOC_ARRAY_OFAIL(newMXFFile->sysData->filenameTemplate, char, strlen(filenameTemplate) + 1);
    strcpy(newMXFFile->sysData->filenameTemplate, filenameTemplate);
    newMXFFile->sysData->pageSize = pageSize;
    newMXFFile->sysData->mode = WRITE_MODE;
    
    
    *mxfFile = newMXFFile;
    return 1;
    
fail:
    if (newMXFFile != NULL)
    {
        mxf_file_close(&newMXFFile);
    }
    return 0;
}

int mxf_page_file_open_read(const char* filenameTemplate, MXFFile** mxfFile)
{
    MXFFile* newMXFFile = NULL;
    int pageCount;
    int allocatedPages;
    char filename[4096];
    FILE* file;
    struct stat st;
    
    
    if (strstr(filenameTemplate, "%d") == NULL)
    {
        mxf_log(MXF_ELOG, "Filename template '%s' doesn't contain %%d\n", filenameTemplate);
        return 0;
    }
    
    /* count number of page files */
    pageCount = 0;
    while (1)
    {
        sprintf(filename, filenameTemplate, pageCount);
        if ((file = fopen(filename, "rb")) == NULL)
        {
            break;
        }
        fclose(file);
        pageCount++;
    }
    
    if (pageCount == 0)
    {
        /* file not found */
        return 0;
    }

    
    CHK_MALLOC_ORET(newMXFFile, MXFFile);
    memset(newMXFFile, 0, sizeof(*newMXFFile));

    newMXFFile->close = page_file_close;
    newMXFFile->read = page_file_read;
    newMXFFile->write = page_file_write;
    newMXFFile->getchar = page_file_getchar;
    newMXFFile->putchar = page_file_putchar;
    newMXFFile->eof = page_file_eof;
    newMXFFile->seek = page_file_seek;
    newMXFFile->tell = page_file_tell;
    newMXFFile->is_seekable = page_file_is_seekable;
    newMXFFile->size = page_file_size;
    newMXFFile->free_sys_data = free_page_file;

    
    CHK_MALLOC_OFAIL(newMXFFile->sysData, MXFFileSysData);
    memset(newMXFFile->sysData, 0, sizeof(*newMXFFile->sysData));
    
    CHK_MALLOC_ARRAY_OFAIL(newMXFFile->sysData->filenameTemplate, char, strlen(filenameTemplate) + 1);
    strcpy(newMXFFile->sysData->filenameTemplate, filenameTemplate);
    newMXFFile->sysData->mode = READ_MODE;
    
    
    
    /* get the page size from the first file */
    sprintf(filename, filenameTemplate, 0);
    if (stat(filename, &st) != 0)
    {
        mxf_log(MXF_ELOG, "Failed to stat file '%s': %s\n", filename, strerror(errno));
        goto fail;
    }
    newMXFFile->sysData->pageSize = st.st_size;
    
    /* allocate pages */
    allocatedPages = (pageCount < PAGE_ALLOC_INCR) ? PAGE_ALLOC_INCR : pageCount;
    CHK_MALLOC_ARRAY_ORET(newMXFFile->sysData->pages, Page, allocatedPages);
    memset(newMXFFile->sysData->pages, 0, allocatedPages * sizeof(Page));
    newMXFFile->sysData->numPages = pageCount;
    newMXFFile->sysData->numPagesAllocated = allocatedPages;
    for (pageCount = 0; pageCount < newMXFFile->sysData->numPages; pageCount++)
    {
        newMXFFile->sysData->pages[pageCount].index = pageCount;
        newMXFFile->sysData->pages[pageCount].size = newMXFFile->sysData->pageSize;
    }
    
    /* set the file size of the last file, which could be less than newMXFFile->sysData->pageSize */
    sprintf(filename, filenameTemplate, newMXFFile->sysData->numPages - 1);
    if (stat(filename, &st) != 0)
    {
        mxf_log(MXF_ELOG, "Failed to stat file '%s': %s\n", filename, strerror(errno));
        goto fail;
    }
    newMXFFile->sysData->pages[newMXFFile->sysData->numPages - 1].size = st.st_size;

    
    *mxfFile = newMXFFile;
    return 1;
    
fail:
    if (newMXFFile != NULL)
    {
        mxf_file_close(&newMXFFile);
    }
    return 0;
}

int mxf_page_file_open_modify(const char* filenameTemplate, int64_t pageSize, MXFFile** mxfFile)
{
    MXFFile* newMXFFile = NULL;
    int pageCount;
    int allocatedPages;
    char filename[4096];
    FILE* file;
    struct stat st;
    
    
    if (strstr(filenameTemplate, "%d") == NULL)
    {
        mxf_log(MXF_ELOG, "Filename template '%s' doesn't contain %%d\n", filenameTemplate);
        return 0;
    }

    /* count number of page files */
    pageCount = 0;
    while (1)
    {
        sprintf(filename, filenameTemplate, pageCount);
        if ((file = fopen(filename, "rb")) == NULL)
        {
            break;
        }
        fclose(file);
        pageCount++;
    }
    
    if (pageCount == 0)
    {
        /* file not found */
        return 0;
    }

    /* check the size of the first file equals the pageSize */
    if (pageCount > 1)
    {
        sprintf(filename, filenameTemplate, 0);
        if (stat(filename, &st) != 0)
        {
            mxf_log(MXF_ELOG, "Failed to stat file '%s': %s\n", filename, strerror(errno));
            return 0;
        }
        if (pageSize != st.st_size)
        {
            mxf_log(MXF_ELOG, "Size of first file '%s' (%"PRId64" does not equal page size %"PRId64"\n", filename, st.st_size, pageSize);
            return 0;
        }
    }
    
    
    CHK_MALLOC_ORET(newMXFFile, MXFFile);
    memset(newMXFFile, 0, sizeof(*newMXFFile));

    newMXFFile->close = page_file_close;
    newMXFFile->read = page_file_read;
    newMXFFile->write = page_file_write;
    newMXFFile->getchar = page_file_getchar;
    newMXFFile->putchar = page_file_putchar;
    newMXFFile->eof = page_file_eof;
    newMXFFile->seek = page_file_seek;
    newMXFFile->tell = page_file_tell;
    newMXFFile->is_seekable = page_file_is_seekable;
    newMXFFile->size = page_file_size;
    newMXFFile->free_sys_data = free_page_file;
    
    
    CHK_MALLOC_OFAIL(newMXFFile->sysData, MXFFileSysData);
    memset(newMXFFile->sysData, 0, sizeof(*newMXFFile->sysData));
    
    CHK_MALLOC_ARRAY_OFAIL(newMXFFile->sysData->filenameTemplate, char, strlen(filenameTemplate) + 1);
    strcpy(newMXFFile->sysData->filenameTemplate, filenameTemplate);
    newMXFFile->sysData->pageSize = pageSize;
    newMXFFile->sysData->mode = MODIFY_MODE;
    
    
    /* allocate pages */
    allocatedPages = (pageCount < PAGE_ALLOC_INCR) ? PAGE_ALLOC_INCR : pageCount;
    CHK_MALLOC_ARRAY_ORET(newMXFFile->sysData->pages, Page, allocatedPages);
    memset(newMXFFile->sysData->pages, 0, allocatedPages * sizeof(Page));
    newMXFFile->sysData->numPages = pageCount;
    newMXFFile->sysData->numPagesAllocated = allocatedPages;
    for (pageCount = 0; pageCount < newMXFFile->sysData->numPages; pageCount++)
    {
        newMXFFile->sysData->pages[pageCount].index = pageCount;
        newMXFFile->sysData->pages[pageCount].size = pageSize;
    }
    
    /* set the files size of the last file, which could have size < pageSize */
    sprintf(filename, filenameTemplate, newMXFFile->sysData->numPages - 1);
    if (stat(filename, &st) != 0)
    {
        mxf_log(MXF_ELOG, "Failed to stat file '%s': %s\n", filename, strerror(errno));
        goto fail;
    }
    newMXFFile->sysData->pages[newMXFFile->sysData->numPages - 1].size = st.st_size;
    
    
    *mxfFile = newMXFFile;
    return 1;
    
fail:
    if (newMXFFile != NULL)
    {
        mxf_file_close(&newMXFFile);
    }
    return 0;
}




