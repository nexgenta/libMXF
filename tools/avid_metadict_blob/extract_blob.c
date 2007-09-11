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

const Key g_MetaDict_key = 
    {0x06, 0x0e, 0x2b, 0x34, 0x02, 0x53, 0x01, 0x01, 0x0d, 0x01, 0x01, 0x01, 0x02, 0x25, 0x00, 0x00};

const Key g_Preface_key = 
    {0x06, 0x0e, 0x2b, 0x34, 0x02, 0x53, 0x01, 0x01, 0x0d, 0x01, 0x01, 0x01, 0x01, 0x01, 0x2f, 0x00};
    


void print_key(const Key *p_key)
{
	int i;
	printf("K=");
	for (i = 0; i < 16; i++)
    {
		printf("%02x ", ((uint8_t*)p_key)[i]);
    }
	printf("\n");
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


void extract_raw_blob(FILE* in, FILE* out)
{
    Key key;
    uint8_t llen;
    uint64_t len;
    uint64_t startOffset = 0;
    uint64_t endOffset = 0;
    uint32_t numRead;
    uint64_t totalRead;
    const uint32_t bufferSize = 1024;
    uint8_t buffer[bufferSize];
    
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
        
        if (memcmp(&key, &g_MetaDict_key, 16) == 0)
        {
            if ((startOffset = ftell(in)) < 0)
            {
                fprintf(stderr, "Failed to get file position\n");
                exit(1);
            }
            startOffset -= 16 + llen;
        }
        else if (memcmp(&key, &g_Preface_key, 16) == 0)
        {
            if ((endOffset = ftell(in)) < 0)
            {
                fprintf(stderr, "Failed to get file position\n");
                exit(1);
            }
            endOffset -= 16 + llen;
            break;
        }
        
        if (fseek(in, len, SEEK_CUR) != 0)
        {
            fprintf(stderr, "Failed to seek past set value\n");
            exit(1);
        }
    }

    if (startOffset == 0 || endOffset < startOffset)
    {
        fprintf(stderr, "Failed to find avid metadict sets\n");
        exit(1);
    }
    
    if (fseek(in, startOffset, SEEK_SET) != 0)
    {
        fprintf(stderr, "Failed to seek to start of avid metadict sets\n");
        exit(1);
    }
    
    
    totalRead = 0;
    do
    {
        numRead = bufferSize;
        if (numRead > (endOffset - startOffset) - totalRead)
        {
            numRead = (endOffset - startOffset) - totalRead;
        }
        if (numRead > 0)
        {
            if (fread(buffer, 1, numRead, in) != numRead)
            {
                fprintf(stderr, "Failed to read data\n");
                exit(1);
            }
            if (fwrite(buffer, 1, numRead, out) != numRead)
            {
                fprintf(stderr, "Failed to write data\n");
                exit(1);
            }
            totalRead += numRead;
        }
    }
    while (numRead == bufferSize);
    
}



void usage(const char* cmd)
{
    fprintf(stderr, "%s <avid mxf filename> <blob filename>\n", cmd);
}

int main(int argv, const char* argc[])
{
    if (argv < 3)
    {
        usage(argc[0]);
        exit(1);
    }
    
    FILE* in;
    FILE* out;
    
    if ((in = fopen(argc[1], "rb")) == NULL)
    {
        fprintf(stderr, "Could not open %s\n", argc[1]);
        exit(1);
    }

    if ((out = fopen(argc[2], "wb")) == NULL)
    {
        fprintf(stderr, "Could not open %s\n", argc[2]);
        exit(1);
    }

    extract_raw_blob(in, out);
    
    fclose(in);
    fclose(out);
    
    return 0;
}


