#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <mxf/mxf.h>


static const mxfKey someKey1 = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const mxfKey someKey2 = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


int test_read(const char* filename)
{
    MXFFile* mxfFile = NULL;
    MXFPartition* headerPartition = NULL;
    MXFPrimerPack* primer = NULL;
    
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    mxfLocalTag tag;

    if (!mxf_disk_file_open_read(filename, &mxfFile))
    {
        mxf_log_error("Failed to open '%s'" LOG_LOC_FORMAT, filename, LOG_LOC_PARAMS);
        return 0;
    }

    /* read header pp */
    CHK_OFAIL(mxf_read_header_pp_kl(mxfFile, &key, &llen, &len));
    CHK_OFAIL(mxf_read_partition(mxfFile, &key, &headerPartition));

    
    /* TEST */

    /* read primer */
    CHK_OFAIL(mxf_read_next_nonfiller_kl(mxfFile, &key, &llen, &len));
    CHK_OFAIL(mxf_is_primer_pack(&key));
    CHK_OFAIL(mxf_read_primer_pack(mxfFile, &primer));
    
    /* get item key */
    CHK_OFAIL(mxf_get_item_key(primer, 0x0101, &key));
    CHK_OFAIL(mxf_equals_key(&someKey1, &key));
    
    /* get item tag */
    CHK_OFAIL(mxf_get_item_tag(primer, &someKey1, &tag));
    CHK_OFAIL(tag == 0x0101);
    

    
    mxf_file_close(&mxfFile);
    mxf_free_primer_pack(&primer);
    mxf_free_partition(&headerPartition);
    return 1;
    
fail:
    mxf_file_close(&mxfFile);
    mxf_free_primer_pack(&primer);
    mxf_free_partition(&headerPartition);
    return 0;
}

int test_create_and_write(const char* filename)
{
    MXFFile* mxfFile = NULL;
    MXFPartition* headerPartition = NULL;
    MXFPrimerPack* primer = NULL;

    mxfLocalTag tag;
    mxfLocalTag tag2;
    
    
    if (!mxf_disk_file_open_new(filename, &mxfFile))
    {
        mxf_log_error("Failed to create '%s'" LOG_LOC_FORMAT, filename, LOG_LOC_PARAMS);
        return 0;
    }

    /* write the header pp */    
    CHK_OFAIL(mxf_create_partition(&headerPartition));
    headerPartition->key = MXF_PP_K(ClosedComplete, Header);
    CHK_OFAIL(mxf_write_partition(mxfFile, headerPartition));
    

    /* TEST */
    
    /* create */
    CHK_OFAIL(mxf_create_primer_pack(&primer));
    
    /* register entry */
    CHK_OFAIL(mxf_register_primer_entry(primer, &someKey1, 0x0101, &tag));
    CHK_OFAIL(tag == 0x0101);
    
    /* registration with same key */
    CHK_OFAIL(mxf_register_primer_entry(primer, &someKey1, 0x0101, &tag));
    CHK_OFAIL(tag == 0x0101);
    
    /* registration dynamic value */
    CHK_OFAIL(mxf_register_primer_entry(primer, &someKey2, 0x0000, &tag));
    CHK_OFAIL(tag >= 0x8000);
    
    /* re-registration dynamic value */
    CHK_OFAIL(mxf_register_primer_entry(primer, &someKey2, 0x0000, &tag2));
    CHK_OFAIL(tag2 == tag);
    
    /* re-registration with different tag value which is ignored */
    CHK_OFAIL(mxf_register_primer_entry(primer, &someKey2, 0xff00, &tag2));
    CHK_OFAIL(tag2 == tag);

    /* write primer */
    CHK_OFAIL(mxf_write_primer_pack(mxfFile, primer));

    
    
    mxf_file_close(&mxfFile);
    mxf_free_primer_pack(&primer);
    mxf_free_partition(&headerPartition);
    return 1;
    
fail:
    mxf_file_close(&mxfFile);
    mxf_free_primer_pack(&primer);
    mxf_free_partition(&headerPartition);
    return 0;
}


void usage(const char* cmd)
{
    fprintf(stderr, "Usage: %s filename\n", cmd);
}

int main(int argc, const char* argv[])
{
    if (argc != 2)
    {
        usage(argv[0]);
        return 1;
    }

    if (!test_create_and_write(argv[1]))
    {
        return 1;
    }
    
    if (!test_read(argv[1]))
    {
        return 1;
    }

    return 0;
}

