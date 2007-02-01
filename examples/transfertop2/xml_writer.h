/*
 * $Id: xml_writer.h,v 1.1 2007/02/01 10:31:43 philipn Exp $
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
 
#ifndef __XML_WRITER_H__
#define __XML_WRITER_H__


typedef struct _XMLWriter XMLWriter;

/* Note: XML writer uses DOS line endings */

int xml_writer_open(const char* filename, XMLWriter** writer);
void xml_writer_close(XMLWriter** writer);

int xml_writer_element_start(XMLWriter* writer, const char* name);
int xml_writer_attribute(XMLWriter* writer, const char* name, const char* value);
int xml_writer_element_end(XMLWriter* writer, const char* name);

int xml_writer_character_data(XMLWriter* writer, const char* data);



#endif
