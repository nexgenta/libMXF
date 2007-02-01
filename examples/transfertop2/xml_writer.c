/*
 * $Id: xml_writer.c,v 1.1 2007/02/01 10:31:42 philipn Exp $
 *
 * Simple XML writer
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

#include <xml_writer.h>
#include <mxf/mxf_macros.h>
#include <mxf/mxf_logging.h>


typedef enum 
{
    ELEMENT_START,
    ATTRIBUTE,
    ELEMENT_END,
    CHARACTER_DATA
} PreviousWrite;

struct _XMLWriter
{
    FILE* file;
    int indent;
    PreviousWrite previousWrite;
};


static int write_indent(XMLWriter* writer)
{
    int i;
    
    CHK_ORET(fprintf(writer->file, "\r\n") > 0);
    for (i = 0; i < writer->indent; i++)
    {
        CHK_ORET(fprintf(writer->file, "  ") > 0);
    }
    
    return 1;
}

int xml_writer_open(const char* filename, XMLWriter** writer)
{
    XMLWriter* newWriter;
    
    CHK_MALLOC_ORET(newWriter, XMLWriter);
    memset(newWriter, 0, sizeof(XMLWriter));
    newWriter->previousWrite = ELEMENT_END;

    if ((newWriter->file = fopen(filename, "wb")) == NULL)
    {
        mxf_log(MXF_ELOG, "Failed to open xml file '%s'" LOG_LOC_FORMAT, filename, LOG_LOC_PARAMS);
        goto fail;
    }
    
    CHK_OFAIL(fprintf(newWriter->file, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>") > 0);
    
    *writer = newWriter;
    return 1;
    
fail:
    SAFE_FREE(&newWriter);
    return 0;
}

void xml_writer_close(XMLWriter** writer)
{
    if (*writer == NULL)
    {
        return;
    }
    
    if ((*writer)->file != NULL)
    {
        fprintf((*writer)->file, "\r\n");
        fclose((*writer)->file);
    }
    SAFE_FREE(writer);
}


int xml_writer_element_start(XMLWriter* writer, const char* name)
{
    if (writer->previousWrite == ELEMENT_START || writer->previousWrite == ATTRIBUTE)
    {
        CHK_ORET(fprintf(writer->file, ">") > 0);
    }
    
    write_indent(writer);
    CHK_ORET(fprintf(writer->file, "<%s", name) > 0);
    
    writer->indent++;
    writer->previousWrite = ELEMENT_START;
    return 1;
}

int xml_writer_attribute(XMLWriter* writer, const char* name, const char* value)
{
    assert(writer->previousWrite == ELEMENT_START || writer->previousWrite == ATTRIBUTE);
    
    CHK_ORET(fprintf(writer->file, " %s=\"%s\"", name, value) > 0);

    writer->previousWrite = ATTRIBUTE;
    return 1;
}


int xml_writer_element_end(XMLWriter* writer, const char* name)
{
    writer->indent--;
    if (writer->previousWrite == ELEMENT_END)
    {
        write_indent(writer);
    }
    else if (writer->previousWrite == ELEMENT_START || writer->previousWrite == ATTRIBUTE)
    {
        CHK_ORET(fprintf(writer->file, ">") > 0);
    }
    CHK_ORET(fprintf(writer->file, "</%s>", name) > 0);

    writer->previousWrite = ELEMENT_END;
    return 1;
}

int xml_writer_character_data(XMLWriter* writer, const char* data)
{
    if (writer->previousWrite == ELEMENT_START || writer->previousWrite == ATTRIBUTE)
    {
        CHK_ORET(fprintf(writer->file, ">") > 0);
    }
    CHK_ORET(fprintf(writer->file, "%s", data) > 0);

    writer->previousWrite = CHARACTER_DATA;
    return 1;
}
