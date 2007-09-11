#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>



typedef struct
{
    uint8_t octet0;
    uint8_t octet1;
    uint8_t octet2;
    uint8_t octet3;
    uint8_t octet4;
    uint8_t octet5;
    uint8_t octet6;
    uint8_t octet7;
    uint8_t octet8;
    uint8_t octet9;
    uint8_t octet10;
    uint8_t octet11;
    uint8_t octet12;
    uint8_t octet13;
    uint8_t octet14;
    uint8_t octet15;
} Key;


const Key g_AvidMetaDictionary_key =
    {0x06, 0x0E, 0x2B, 0x34, 0x02, 0x53, 0x01, 0x01, 0x0D, 0x01, 0x01, 0x01, 0x02, 0x25, 0x00, 0x00};

    
int read_tl(FILE *fp, uint16_t* tag, uint16_t* itemLen)
{
    unsigned char buffer[2];
    if (fread(buffer, 1, 2, fp) != 2)
    {
        fprintf(stderr, "Failed to read tag\n");
        exit(1);
    }
    
    *tag = (buffer[0]<<8) + (buffer[1]);
    
    if (fread(buffer, 1, 2, fp) != 2)
    {
        fprintf(stderr, "Failed to read item length\n");
        exit(1);
    }
    
    *itemLen = (buffer[0]<<8) + (buffer[1]);
    
    return 1;
}



int read_kl(FILE *fp, Key *key, uint8_t* llen, uint64_t *len)
{
	int i;

	if (fread((uint8_t*)key, 16, 1, fp) != 1) 
    {
		if (feof(fp))
        {
			return 0;
        }
		perror("fread");
		fprintf(stderr, "Could not read Key\n");
		return 0;
	}

	// Read BER integer (ISO/IEC 8825-1).
	int c;
	uint64_t length = 0;
	if ((c = fgetc(fp)) == EOF) 
    {
		perror("fgetc");
		fprintf(stderr, "Could not read Length\n");
        return 0;
	}

    *llen = 1;
	if (c < 128) 			// high-bit set on first byte? 
    {
		length = c;
	}
	else				// else stores number of bytes in BER integer 
    {
		int bytes_to_read = c & 0x7f;
		for (i = 0; i < bytes_to_read; i++) 
        {
			if ((c = fgetc(fp)) == EOF) 
            {
				perror("fgetc");
				fprintf(stderr, "Could not read Length\n");
				return 0;
			}
			length = length << 8;
			length = length | c;
		}
        *llen += bytes_to_read;
	}
	*len = length;

	return 1;
}


void gen_metadict_instanceuid(FILE* in)
{
    printf("const mxfUUID g_AvidMetaDictInstanceUID_uuid = \n");
    
    Key key;
    uint8_t llen;
    uint64_t len;
    uint64_t readLen;
    uint16_t tag;
    uint16_t itemLen;
    Key uid;
    int found = 0;
    while (1)
    {
        if (!read_kl(in, &key, &llen, &len))
        {
            if (!feof(in))
            {
                fprintf(stderr, "Failed to read kl\n");
                exit(1);
            }
            break;
        }
        
        if (memcmp(&key, &g_AvidMetaDictionary_key, 16) == 0)
        {
            readLen = 0;
            while (readLen < len)
            {
                if (!read_tl(in, &tag, &itemLen))
                {
                    fprintf(stderr, "Failed to read tl\n");
                    exit(1);
                }
                readLen += 4;
                if (tag == 0x3c0a)
                {
                    if (itemLen != 16 || fread((void*)&uid, 16, 1, in) != 1)
                    {
                        fprintf(stderr, "Failed to read instanceUID item\n");
                        exit(1);
                    }
                    found = 1;
                    break;
                }
                else
                {
                    if (fseek(in, itemLen, SEEK_CUR) != 0)
                    {
                        fprintf(stderr, "Failed to seek past item value\n");
                        exit(1);
                    }
                    readLen += itemLen;
                }
            }
            if (!found)
            {
                fprintf(stderr, "Missing MetaDictionary::InstanceUID item\n");
                exit(1);
            }
            break;
        }
    }
    if (!found)
    {
        fprintf(stderr, "Missing MetaDictionary::InstanceUID item\n");
        exit(1);
    }
        
    printf("    {0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, "
        "0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x};\n", 
       ((unsigned char*)&uid)[0], ((unsigned char*)&uid)[1], ((unsigned char*)&uid)[2], 
       ((unsigned char*)&uid)[3], ((unsigned char*)&uid)[4], ((unsigned char*)&uid)[5], 
       ((unsigned char*)&uid)[6], ((unsigned char*)&uid)[7], ((unsigned char*)&uid)[8],
       ((unsigned char*)&uid)[9], ((unsigned char*)&uid)[10], ((unsigned char*)&uid)[11], 
       ((unsigned char*)&uid)[12], ((unsigned char*)&uid)[13], ((unsigned char*)&uid)[14], 
       ((unsigned char*)&uid)[15]);
    
    printf("\n");
}

void gen_tags()
{
    printf("const struct AvidMetaDictTagStruct g_AvidMetaDictTags[] = \n");
    printf("{\n");

    printf("    {0x0003, {0x06, 0x0E, 0x2B, 0x34, 0x01, 0x01, 0x01, 0x02, 0x06, 0x01, 0x01, 0x07, 0x07, 0x00, 0x00, 0x00}},\n");    
    printf("    {0x0004, {0x06, 0x0E, 0x2B, 0x34, 0x01, 0x01, 0x01, 0x02, 0x06, 0x01, 0x01, 0x07, 0x08, 0x00, 0x00, 0x00}},\n");
    printf("    {0x0005, {0x06, 0x0E, 0x2B, 0x34, 0x01, 0x01, 0x01, 0x02, 0x06, 0x01, 0x01, 0x07, 0x13, 0x00, 0x00, 0x00}},\n");
    printf("    {0x0006, {0x06, 0x0E, 0x2B, 0x34, 0x01, 0x01, 0x01, 0x02, 0x03, 0x02, 0x04, 0x01, 0x02, 0x01, 0x00, 0x00}},\n");
    printf("    {0x0007, {0x06, 0x0E, 0x2B, 0x34, 0x01, 0x01, 0x01, 0x02, 0x06, 0x01, 0x01, 0x07, 0x14, 0x01, 0x00, 0x00}},\n");
    printf("    {0x0008, {0x06, 0x0E, 0x2B, 0x34, 0x01, 0x01, 0x01, 0x02, 0x06, 0x01, 0x01, 0x07, 0x01, 0x00, 0x00, 0x00}},\n");    
    printf("    {0x0009, {0x06, 0x0E, 0x2B, 0x34, 0x01, 0x01, 0x01, 0x02, 0x06, 0x01, 0x01, 0x07, 0x02, 0x00, 0x00, 0x00}},\n");    
    printf("    {0x000a, {0x06, 0x0E, 0x2B, 0x34, 0x01, 0x01, 0x01, 0x02, 0x06, 0x01, 0x01, 0x07, 0x03, 0x00, 0x00, 0x00}},\n");
    printf("    {0x000b, {0x06, 0x0E, 0x2B, 0x34, 0x01, 0x01, 0x01, 0x02, 0x06, 0x01, 0x01, 0x07, 0x04, 0x00, 0x00, 0x00}},\n");    
    printf("    {0x000c, {0x06, 0x0E, 0x2B, 0x34, 0x01, 0x01, 0x01, 0x02, 0x03, 0x01, 0x02, 0x02, 0x01, 0x00, 0x00, 0x00}},\n");    
    printf("    {0x000d, {0x06, 0x0E, 0x2B, 0x34, 0x01, 0x01, 0x01, 0x02, 0x06, 0x01, 0x01, 0x07, 0x05, 0x00, 0x00, 0x00}},\n");
    printf("    {0x000e, {0x06, 0x0E, 0x2B, 0x34, 0x01, 0x01, 0x01, 0x02, 0x06, 0x01, 0x01, 0x07, 0x06, 0x00, 0x00, 0x00}},\n");
    printf("    {0x000f, {0x06, 0x0E, 0x2B, 0x34, 0x01, 0x01, 0x01, 0x02, 0x03, 0x01, 0x02, 0x03, 0x01, 0x00, 0x00, 0x00}},\n");
    printf("    {0x0010, {0x06, 0x0E, 0x2B, 0x34, 0x01, 0x01, 0x01, 0x02, 0x03, 0x01, 0x02, 0x03, 0x02, 0x00, 0x00, 0x00}},\n");
    printf("    {0x0011, {0x06, 0x0E, 0x2B, 0x34, 0x01, 0x01, 0x01, 0x02, 0x06, 0x01, 0x01, 0x07, 0x09, 0x00, 0x00, 0x00}},\n");
    printf("    {0x0012, {0x06, 0x0E, 0x2B, 0x34, 0x01, 0x01, 0x01, 0x02, 0x06, 0x01, 0x01, 0x07, 0x0A, 0x00, 0x00, 0x00}},\n");
    printf("    {0x0013, {0x06, 0x0E, 0x2B, 0x34, 0x01, 0x01, 0x01, 0x02, 0x03, 0x01, 0x02, 0x03, 0x0B, 0x00, 0x00, 0x00}},\n");
    printf("    {0x0014, {0x06, 0x0E, 0x2B, 0x34, 0x01, 0x01, 0x01, 0x02, 0x06, 0x01, 0x01, 0x07, 0x0B, 0x00, 0x00, 0x00}},\n");
    printf("    {0x0015, {0x06, 0x0E, 0x2B, 0x34, 0x01, 0x01, 0x01, 0x02, 0x03, 0x01, 0x02, 0x03, 0x04, 0x00, 0x00, 0x00}},\n");
    printf("    {0x0016, {0x06, 0x0E, 0x2B, 0x34, 0x01, 0x01, 0x01, 0x02, 0x03, 0x01, 0x02, 0x03, 0x05, 0x00, 0x00, 0x00}},\n");
    printf("    {0x0017, {0x06, 0x0E, 0x2B, 0x34, 0x01, 0x01, 0x01, 0x02, 0x06, 0x01, 0x01, 0x07, 0x0C, 0x00, 0x00, 0x00}},\n");
    printf("    {0x0018, {0x06, 0x0E, 0x2B, 0x34, 0x01, 0x01, 0x01, 0x02, 0x03, 0x01, 0x02, 0x03, 0x03, 0x00, 0x00, 0x00}},\n");
    printf("    {0x0019, {0x06, 0x0E, 0x2B, 0x34, 0x01, 0x01, 0x01, 0x02, 0x06, 0x01, 0x01, 0x07, 0x0D, 0x00, 0x00, 0x00}},\n");
    printf("    {0x001a, {0x06, 0x0E, 0x2B, 0x34, 0x01, 0x01, 0x01, 0x02, 0x06, 0x01, 0x01, 0x07, 0x0E, 0x00, 0x00, 0x00}},\n");
    printf("    {0x001b, {0x06, 0x0E, 0x2B, 0x34, 0x01, 0x01, 0x01, 0x02, 0x06, 0x01, 0x01, 0x07, 0x0F, 0x00, 0x00, 0x00}},\n");
    printf("    {0x001c, {0x06, 0x0E, 0x2B, 0x34, 0x01, 0x01, 0x01, 0x02, 0x06, 0x01, 0x01, 0x07, 0x11, 0x00, 0x00, 0x00}},\n");
    printf("    {0x001d, {0x06, 0x0E, 0x2B, 0x34, 0x01, 0x01, 0x01, 0x02, 0x03, 0x01, 0x02, 0x03, 0x06, 0x00, 0x00, 0x00}},\n");
    printf("    {0x001e, {0x06, 0x0E, 0x2B, 0x34, 0x01, 0x01, 0x01, 0x02, 0x06, 0x01, 0x01, 0x07, 0x12, 0x00, 0x00, 0x00}},\n");
    printf("    {0x001f, {0x06, 0x0E, 0x2B, 0x34, 0x01, 0x01, 0x01, 0x02, 0x03, 0x01, 0x02, 0x03, 0x07, 0x00, 0x00, 0x00}},\n");
    printf("    {0x0020, {0x06, 0x0E, 0x2B, 0x34, 0x01, 0x01, 0x01, 0x02, 0x03, 0x01, 0x02, 0x03, 0x08, 0x00, 0x00, 0x00}}\n");

    printf("};\n");
    
    printf("const uint32_t g_AvidMetaDictTags_len = sizeof(g_AvidMetaDictTags) / sizeof(struct AvidMetaDictTagStruct);\n");
}


void gen_dynamic_tags_offsets(FILE* in)
{
    printf("const struct AvidMetaDictDynTagOffsetsStruct g_AvidMetaDictDynTagOffsets[] = \n");
    printf("{\n");
    
    Key key;
    uint8_t llen;
    uint64_t len;
    uint64_t readLen;
    Key id;
    long offset;
    uint16_t itemTag;
    int first = 1;
    uint16_t tag;
    uint16_t itemLen;
    while (1)
    {
        offset = ftell(in);
        if (offset < 0)
        {
            fprintf(stderr, "Failed to get file position\n");
            exit(1);
        }
        if (!read_kl(in, &key, &llen, &len))
        {
            if (!feof(in))
            {
                fprintf(stderr, "Failed to read kl\n");
                exit(1);
            }
            break;
        }
        
        readLen = 0;
        offset = -1;
        itemTag = 0;
        while (readLen < len)
        {
            if (!read_tl(in, &tag, &itemLen))
            {
                fprintf(stderr, "Failed to read tl\n");
                exit(1);
            }
            readLen += 4;
            if (tag == 0x0005)
            {
                if (itemLen != 16 || fread((void*)&id, 16, 1, in) != 1)
                {
                    fprintf(stderr, "Failed to read MetaDef::Identification item\n");
                    exit(1);
                }
                readLen += 16;
            }
            else if (tag == 0x000d)
            {
                offset = ftell(in);
                if (itemLen != 2 || fread((void*)&itemTag, 2, 1, in) != 1)
                {
                    fprintf(stderr, "Failed to read PropertyDef::LocalTag item\n");
                    exit(1);
                }
                readLen += 2;
            }
            else
            {
                if (fseek(in, itemLen, SEEK_CUR) != 0)
                {
                    fprintf(stderr, "Failed to seek past item value\n");
                    exit(1);
                }
                readLen += itemLen;
            }
        }
        if (readLen != len)
        {
            fprintf(stderr, "Set length does not equal length of sum of items\n");
            exit(1);
        }
        
        if (itemTag >= 0x8000)
        {
            if (!first)
            {
                printf(",\n");
            }
            printf("    {{0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, "
                "0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x}, 0x%lx}", 
               ((unsigned char*)&id)[0], ((unsigned char*)&id)[1], ((unsigned char*)&id)[2], 
               ((unsigned char*)&id)[3], ((unsigned char*)&id)[4], ((unsigned char*)&id)[5], 
               ((unsigned char*)&id)[6], ((unsigned char*)&id)[7], ((unsigned char*)&id)[8],
               ((unsigned char*)&id)[9], ((unsigned char*)&id)[10], ((unsigned char*)&id)[11], 
               ((unsigned char*)&id)[12], ((unsigned char*)&id)[13], ((unsigned char*)&id)[14], 
               ((unsigned char*)&id)[15], offset);
            first = 0;
        }
    }
    
    printf("\n};\n");

    printf("const uint32_t g_AvidMetaDictDynTagOffsets_len = sizeof(g_AvidMetaDictDynTagOffsets) / sizeof(struct AvidMetaDictDynTagOffsetsStruct);\n");
}


void gen_offsets(FILE* in)
{
    printf("const struct AvidMetaDictObjectOffsetsStruct g_AvidMetaDictObjectOffsets[] = \n");
    printf("{\n");
    
    Key key;
    uint8_t llen;
    uint64_t len;
    uint64_t readLen;
    long offset;
    int first = 1;
    Key uid;
    uint16_t tag;
    uint16_t itemLen;
    while (1)
    {
        offset = ftell(in);
        if (offset < 0)
        {
            fprintf(stderr, "Failed to get file position\n");
            exit(1);
        }
        if (!read_kl(in, &key, &llen, &len))
        {
            if (!feof(in))
            {
                fprintf(stderr, "Failed to read kl\n");
                exit(1);
            }
            break;
        }
        
        readLen = 0;
        while (readLen < len)
        {
            if (!read_tl(in, &tag, &itemLen))
            {
                fprintf(stderr, "Failed to read tl\n");
                exit(1);
            }
            readLen += 4;
            if (tag == 0x3c0a)
            {
                if (itemLen != 16 || fread((void*)&uid, 16, 1, in) != 1)
                {
                    fprintf(stderr, "Failed to read instanceUID item\n");
                    exit(1);
                }
                readLen += 16;
            }
            else
            {
                if (fseek(in, itemLen, SEEK_CUR) != 0)
                {
                    fprintf(stderr, "Failed to seek past item value\n");
                    exit(1);
                }
                readLen += itemLen;
            }
        }
        if (readLen != len)
        {
            fprintf(stderr, "Set length does not equal length of sum of items\n");
            exit(1);
        }
        
        if (!first)
        {
            printf(",\n");
        }
        printf("    {{0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, "
            "0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x}, 0x%lx, 0x%02x}", 
           ((unsigned char*)&uid)[0], ((unsigned char*)&uid)[1], ((unsigned char*)&uid)[2], 
           ((unsigned char*)&uid)[3], ((unsigned char*)&uid)[4], ((unsigned char*)&uid)[5], 
           ((unsigned char*)&uid)[6], ((unsigned char*)&uid)[7], ((unsigned char*)&uid)[8],
           ((unsigned char*)&uid)[9], ((unsigned char*)&uid)[10], ((unsigned char*)&uid)[11], 
           ((unsigned char*)&uid)[12], ((unsigned char*)&uid)[13], ((unsigned char*)&uid)[14], 
           ((unsigned char*)&uid)[15], offset, 0x00);
        first = 0;
    }
    
    printf("\n};\n");

    printf("const uint32_t g_AvidMetaDictObjectOffsets_len = sizeof(g_AvidMetaDictObjectOffsets) / sizeof(struct AvidMetaDictObjectOffsetsStruct);\n");
}

void gen_blob(FILE* in)
{
    printf("const uint8_t g_AvidMetaDictBlob[] = \n");
    printf("{\n");
        
    const int bufferSize = 16;
    unsigned char buffer[bufferSize];
    size_t numRead;
    size_t i;
    while (1)
    {
        numRead = fread(buffer, 1, bufferSize, in);
        if (numRead != bufferSize)
        {
            if (!feof(in))
            {
                fprintf(stderr, "Failed to read\n");
                exit(1);
            }
        }
        if (numRead > 0)
        {
            printf("    ");
            for (i = 0; i < numRead; i++)
            {
                printf("0x%02x", buffer[i]);
                if (numRead == bufferSize || i < numRead - 1)
                {
                    printf(", ");
                }
            }
            printf("\n");
        }
        
        if (numRead != bufferSize)
        {
            break;
        }
    }
    
    printf("};\n");
    
    printf("const uint32_t g_AvidMetaDictBlob_len = sizeof(g_AvidMetaDictBlob) / sizeof(uint8_t);\n");
}


void usage(const char* cmd)
{
    fprintf(stderr, "%s <blob filename>\n", cmd);
}

int main(int argv, const char* argc[])
{
    if (argv < 2)
    {
        usage(argc[0]);
        exit(1);
    }
    
    FILE* in;
    if ((in = fopen(argc[1], "r")) == NULL)
    {
        fprintf(stderr, "Could not open %s\n", argc[1]);
        return 1;
    }

    printf(
        "/*\n"
        " * $Id""$\n"
        " *\n"
        " * Blobs of data containing Avid header metadata extensions\n"
        " *\n"
        " * Copyright (C) 2006  Philip de Nier <philipn@users.sourceforge.net>\n"
        " *\n"
        " * This library is free software; you can redistribute it and/or\n"
        " * modify it under the terms of the GNU Lesser General Public\n"
        " * License as published by the Free Software Foundation; either\n"
        " * version 2.1 of the License, or (at your option) any later version.\n"
        " *\n"
        " * This library is distributed in the hope that it will be useful,\n"
        " * but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        " * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU\n"
        " * Lesser General Public License for more details.\n"
        " *\n"
        " * You should have received a copy of the GNU Lesser General Public\n"
        " * License along with this library; if not, write to the Free Software\n"
        " * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA\n"
        " */\n\n");
    
    printf("#include <mxf/mxf.h>\n");
    printf("#include <mxf/mxf_avid.h>\n\n\n");    
     
    gen_metadict_instanceuid(in);
    fseek(in, 0, SEEK_SET);
    printf("\n\n");    
    
    gen_tags();
    printf("\n\n");    
    
    gen_dynamic_tags_offsets(in);
    fseek(in, 0, SEEK_SET);
    printf("\n\n");    

    gen_offsets(in);
    fseek(in, 0, SEEK_SET);
    printf("\n\n");    

    gen_blob(in);

    printf("\n");    
 
    fclose(in);
    
    return 0;
}


