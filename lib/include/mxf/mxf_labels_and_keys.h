/*
 * $Id: mxf_labels_and_keys.h,v 1.8 2009/06/18 11:56:48 philipn Exp $
 *
 * MXF labels, keys, track numbers, etc
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
 
#ifndef __MXF_LABELS_AND_KEYS_H__
#define __MXF_LABELS_AND_KEYS_H__



#ifdef __cplusplus
extern "C" 
{
#endif


/*
 *
 * Data definition labels
 *
 */
 

#define MXF_DDEF_L(name) \
    g_##name##_datadef_label

    
static const mxfUL MXF_DDEF_L(Picture) = 
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x01, 0x03, 0x02, 0x02, 0x01, 0x00, 0x00, 0x00};

static const mxfUL MXF_DDEF_L(Sound) = 
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x01, 0x03, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00};
    
static const mxfUL MXF_DDEF_L(Timecode) = 
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x01, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0x00};
    
static const mxfUL MXF_DDEF_L(LegacyPicture) = 
    {0x80, 0x7d, 0x00, 0x60, 0x08, 0x14, 0x3e, 0x6f, 0x6f, 0x3c, 0x8c, 0xe1, 0x6c, 0xef, 0x11, 0xd2};

static const mxfUL MXF_DDEF_L(LegacySound) = 
    {0x80, 0x7d, 0x00, 0x60, 0x08, 0x14, 0x3e, 0x6f, 0x78, 0xe1, 0xeb, 0xe1, 0x6c, 0xef, 0x11, 0xd2};
    
static const mxfUL MXF_DDEF_L(LegacyTimecode) = 
    {0x80, 0x7f, 0x00, 0x60, 0x08, 0x14, 0x3e, 0x6f, 0x7f, 0x27, 0x5e, 0x81, 0x77, 0xe5, 0x11, 0xd2};
    
static const mxfUL MXF_DDEF_L(Data) = 
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x01, 0x03, 0x02, 0x02, 0x03, 0x00, 0x00, 0x00};

static const mxfUL MXF_DDEF_L(DescriptiveMetadata) = 
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x01, 0x03, 0x02, 0x01, 0x10, 0x00, 0x00, 0x00};

  
int mxf_is_picture(const mxfUL* label);
int mxf_is_sound(const mxfUL* label);
int mxf_is_timecode(const mxfUL* label);
int mxf_is_data(const mxfUL* label);
int mxf_is_descriptive_metadata(const mxfUL* label);
    
/*
 *
 * Picture essence coding compression labels
 *
 */
 

#define MXF_CMDEF_L(name) \
    g_##name##_compdef_label

    
/* IEC-DV and DV-based mappings */

#define MXF_DV_CMDEV_L(regver, byte14, byte15) \
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, regver, 0x04, 0x01, 0x02, 0x02, 0x02, byte14, byte15, 0x00}

    
static const mxfUL MXF_CMDEF_L(IECDV_25_525_60) = 
    MXF_DV_CMDEV_L(0x01, 0x01, 0x01);

static const mxfUL MXF_CMDEF_L(IECDV_25_625_50) = 
    MXF_DV_CMDEV_L(0x01, 0x01, 0x02);

    
static const mxfUL MXF_CMDEF_L(DVBased_25_525_60) = 
    MXF_DV_CMDEV_L(0x01, 0x02, 0x01);

static const mxfUL MXF_CMDEF_L(DVBased_25_625_50) = 
    MXF_DV_CMDEV_L(0x01, 0x02, 0x02);

static const mxfUL MXF_CMDEF_L(DVBased_50_525_60) = 
    MXF_DV_CMDEV_L(0x01, 0x02, 0x03);

static const mxfUL MXF_CMDEF_L(DVBased_50_625_50) = 
    MXF_DV_CMDEV_L(0x01, 0x02, 0x04);

static const mxfUL MXF_CMDEF_L(DVBased_100_1080_60_I) = 
    MXF_DV_CMDEV_L(0x01, 0x02, 0x05);

static const mxfUL MXF_CMDEF_L(DVBased_100_1080_50_I) = 
    MXF_DV_CMDEV_L(0x01, 0x02, 0x06);

static const mxfUL MXF_CMDEF_L(DVBased_100_720_60_P) = 
    MXF_DV_CMDEV_L(0x01, 0x02, 0x07);

static const mxfUL MXF_CMDEF_L(DVBased_100_720_50_P) = 
    MXF_DV_CMDEV_L(0x01, 0x02, 0x08);

    
/* D-10 mappings */

#define MXF_D10_CMDEV_L(regver, byte16) \
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, regver, 0x04, 0x01, 0x02, 0x02, 0x01, 0x02, 0x01, byte16}

static const mxfUL MXF_CMDEF_L(D10_50_625_50) = 
    MXF_D10_CMDEV_L(0x01, 0x01);

static const mxfUL MXF_CMDEF_L(D10_50_525_60) = 
    MXF_D10_CMDEV_L(0x01, 0x02);

static const mxfUL MXF_CMDEF_L(D10_40_625_50) = 
    MXF_D10_CMDEV_L(0x01, 0x03);

static const mxfUL MXF_CMDEF_L(D10_40_525_60) = 
    MXF_D10_CMDEV_L(0x01, 0x04);

static const mxfUL MXF_CMDEF_L(D10_30_625_50) = 
    MXF_D10_CMDEV_L(0x01, 0x05);

static const mxfUL MXF_CMDEF_L(D10_30_525_60) = 
    MXF_D10_CMDEV_L(0x01, 0x06);

    
/* A-law audio mapping */

static const mxfUL MXF_CMDEF_L(ALaw) = 
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x03 , 0x04, 0x02, 0x02, 0x02, 0x03, 0x01, 0x01, 0x00};


/* MPEG mappings */

static const mxfUL MXF_CMDEF_L(MP4AdvancedRealTimeSimpleL3) = 
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x03, 0x04, 0x01, 0x02, 0x02, 0x01, 0x20, 0x02, 0x03};


/* AVC Intra-Frame Coding */

#define MXF_AVCI_CMDEV_L(profile, variant) \
    {0x06, 0x0E, 0x2B, 0x34, 0x04, 0x01, 0x01, 0x0A, 0x04, 0x01, 0x02, 0x02, 0x01, 0x32, profile, variant}

static const mxfUL MXF_CMDEF_L(AVCI_50_1080_60_I) =
    MXF_AVCI_CMDEV_L(0x21, 0x01);

static const mxfUL MXF_CMDEF_L(AVCI_50_1080_50_I) =
    MXF_AVCI_CMDEV_L(0x21, 0x02);

static const mxfUL MXF_CMDEF_L(AVCI_50_1080_30_P) =
    MXF_AVCI_CMDEV_L(0x21, 0x03);

static const mxfUL MXF_CMDEF_L(AVCI_50_1080_25_P) =
    MXF_AVCI_CMDEV_L(0x21, 0x04);

static const mxfUL MXF_CMDEF_L(AVCI_50_720_60_P) =
    MXF_AVCI_CMDEV_L(0x21, 0x08);

static const mxfUL MXF_CMDEF_L(AVCI_50_720_50_P) =
    MXF_AVCI_CMDEV_L(0x21, 0x09);

static const mxfUL MXF_CMDEF_L(AVCI_100_1080_60_I) =
    MXF_AVCI_CMDEV_L(0x31, 0x01);

static const mxfUL MXF_CMDEF_L(AVCI_100_1080_50_I) =
    MXF_AVCI_CMDEV_L(0x31, 0x02);

static const mxfUL MXF_CMDEF_L(AVCI_100_1080_30_P) =
    MXF_AVCI_CMDEV_L(0x31, 0x03);

static const mxfUL MXF_CMDEF_L(AVCI_100_1080_25_P) =
    MXF_AVCI_CMDEV_L(0x31, 0x04);

static const mxfUL MXF_CMDEF_L(AVCI_100_720_60_P) =
    MXF_AVCI_CMDEV_L(0x31, 0x08);

static const mxfUL MXF_CMDEF_L(AVCI_100_720_50_P) =
    MXF_AVCI_CMDEV_L(0x31, 0x09);


/* DNxHD */

static const mxfUL MXF_CMDEF_L(DNxHD) =
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0e, 0x04, 0x02, 0x01, 0x02, 0x04, 0x01, 0x00};


    
/*
 *
 * Essence container labels
 *
 */
 
#define MXF_EC_L(name) \
    g_##name##_esscont_label

    
    
#define MXF_GENERIC_CONTAINER_LABEL(regver, eckind, mappingkind, byte15, byte16) \
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, regver, 0x0d, 0x01, 0x03, 0x01, eckind, mappingkind, byte15, byte16}
    
/* Multiple wrappings for interleaved essence */

static const mxfUL MXF_EC_L(MultipleWrappings) = 
    MXF_GENERIC_CONTAINER_LABEL(0x03, 0x02, 0x7f, 0x01, 0x00);


/* AES3/BWF mappings */

#define MXF_AES3BWF_EC_L(regver, byte15) \
    MXF_GENERIC_CONTAINER_LABEL(regver, 0x02, 0x06, byte15, 0x00)

static const mxfUL MXF_EC_L(BWFFrameWrapped) = 
    MXF_AES3BWF_EC_L(0x01, 0x01);

static const mxfUL MXF_EC_L(BWFClipWrapped) = 
    MXF_AES3BWF_EC_L(0x01, 0x02);

static const mxfUL MXF_EC_L(AES3FrameWrapped) = 
    MXF_AES3BWF_EC_L(0x01, 0x03);

static const mxfUL MXF_EC_L(AES3ClipWrapped) = 
    MXF_AES3BWF_EC_L(0x01, 0x04);

static const mxfUL MXF_EC_L(BWFCustomWrapped) = 
    MXF_AES3BWF_EC_L(0x05, 0x08);

static const mxfUL MXF_EC_L(AES3CustomWrapped) = 
    MXF_AES3BWF_EC_L(0x05, 0x09);

    
/* IEC-DV and DV-based mappings */


#define MXF_DV_EC_L(regver, byte15, byte16) \
    MXF_GENERIC_CONTAINER_LABEL(regver, 0x02, 0x02, byte15, byte16)

static const mxfUL MXF_EC_L(IECDV_25_525_60_FrameWrapped) = 
    MXF_DV_EC_L(0x01, 0x01, 0x01);

static const mxfUL MXF_EC_L(IECDV_25_525_60_ClipWrapped) = 
    MXF_DV_EC_L(0x01, 0x01, 0x02);

static const mxfUL MXF_EC_L(IECDV_25_625_50_FrameWrapped) = 
    MXF_DV_EC_L(0x01, 0x02, 0x01);

static const mxfUL MXF_EC_L(IECDV_25_625_50_ClipWrapped) = 
    MXF_DV_EC_L(0x01, 0x02, 0x02);

    
static const mxfUL MXF_EC_L(DVBased_25_525_60_FrameWrapped) = 
    MXF_DV_EC_L(0x01, 0x40, 0x01);

static const mxfUL MXF_EC_L(DVBased_25_525_60_ClipWrapped) = 
    MXF_DV_EC_L(0x01, 0x40, 0x02);

static const mxfUL MXF_EC_L(DVBased_25_625_50_FrameWrapped) = 
    MXF_DV_EC_L(0x01, 0x41, 0x01);

static const mxfUL MXF_EC_L(DVBased_25_625_50_ClipWrapped) = 
    MXF_DV_EC_L(0x01, 0x41, 0x02);

static const mxfUL MXF_EC_L(DVBased_50_525_60_FrameWrapped) = 
    MXF_DV_EC_L(0x01, 0x50, 0x01);

static const mxfUL MXF_EC_L(DVBased_50_525_60_ClipWrapped) = 
    MXF_DV_EC_L(0x01, 0x50, 0x02);

static const mxfUL MXF_EC_L(DVBased_50_625_50_FrameWrapped) = 
    MXF_DV_EC_L(0x01, 0x51, 0x01);

static const mxfUL MXF_EC_L(DVBased_50_625_50_ClipWrapped) = 
    MXF_DV_EC_L(0x01, 0x51, 0x02);

static const mxfUL MXF_EC_L(DVBased_100_1080_60_I_FrameWrapped) = 
    MXF_DV_EC_L(0x01, 0x60, 0x01);

static const mxfUL MXF_EC_L(DVBased_100_1080_60_I_ClipWrapped) = 
    MXF_DV_EC_L(0x01, 0x60, 0x02);

static const mxfUL MXF_EC_L(DVBased_100_1080_50_I_FrameWrapped) = 
    MXF_DV_EC_L(0x01, 0x61, 0x01);

static const mxfUL MXF_EC_L(DVBased_100_1080_50_I_ClipWrapped) = 
    MXF_DV_EC_L(0x01, 0x61, 0x02);

static const mxfUL MXF_EC_L(DVBased_100_720_60_P_FrameWrapped) = 
    MXF_DV_EC_L(0x01, 0x62, 0x01);

static const mxfUL MXF_EC_L(DVBased_100_720_60_P_ClipWrapped) = 
    MXF_DV_EC_L(0x01, 0x62, 0x02);

static const mxfUL MXF_EC_L(DVBased_100_720_50_P_FrameWrapped) = 
    MXF_DV_EC_L(0x01, 0x63, 0x01);

static const mxfUL MXF_EC_L(DVBased_100_720_50_P_ClipWrapped) = 
    MXF_DV_EC_L(0x01, 0x63, 0x02);


/* Uncompressed mappings */

#define MXF_UNC_EC_L(regver, byte15, byte16) \
    MXF_GENERIC_CONTAINER_LABEL(regver, 0x02, 0x05, byte15, byte16)

static const mxfUL MXF_EC_L(SD_Unc_525_60i_422_135_FrameWrapped) = 
    MXF_UNC_EC_L(0x01, 0x01, 0x01);

static const mxfUL MXF_EC_L(SD_Unc_525_60i_422_135_ClipWrapped) = 
    MXF_UNC_EC_L(0x01, 0x01, 0x02);

static const mxfUL MXF_EC_L(SD_Unc_625_50i_422_135_FrameWrapped) = 
    MXF_UNC_EC_L(0x01, 0x01, 0x05);

static const mxfUL MXF_EC_L(SD_Unc_625_50i_422_135_ClipWrapped) = 
    MXF_UNC_EC_L(0x01, 0x01, 0x06);

static const mxfUL MXF_EC_L(HD_Unc_1080_50i_422_ClipWrapped) = 
    MXF_UNC_EC_L(0x01, 0x02, 0x2a);


/* D-10 mapping */

#define MXF_D10_EC_L(regver, byte15, byte16) \
    MXF_GENERIC_CONTAINER_LABEL(regver, 0x02, 0x01, byte15, byte16)

static const mxfUL MXF_EC_L(D10_50_625_50_defined_template) = 
    MXF_D10_EC_L(0x01, 0x01, 0x01);

static const mxfUL MXF_EC_L(D10_50_625_50_extended_template) = 
    MXF_D10_EC_L(0x01, 0x01, 0x02);

static const mxfUL MXF_EC_L(D10_50_625_50_picture_only) = 
    MXF_D10_EC_L(0x02, 0x01, 0x7f);

static const mxfUL MXF_EC_L(D10_50_525_60_defined_template) = 
    MXF_D10_EC_L(0x01, 0x02, 0x01);

static const mxfUL MXF_EC_L(D10_50_525_60_extended_template) = 
    MXF_D10_EC_L(0x01, 0x02, 0x02);

static const mxfUL MXF_EC_L(D10_50_525_60_picture_only) = 
    MXF_D10_EC_L(0x02, 0x02, 0x7f);

static const mxfUL MXF_EC_L(D10_40_625_50_defined_template) = 
    MXF_D10_EC_L(0x01, 0x03, 0x01);

static const mxfUL MXF_EC_L(D10_40_625_50_extended_template) = 
    MXF_D10_EC_L(0x01, 0x03, 0x02);

static const mxfUL MXF_EC_L(D10_40_625_50_picture_only) = 
    MXF_D10_EC_L(0x02, 0x03, 0x7f);

static const mxfUL MXF_EC_L(D10_40_525_60_defined_template) = 
    MXF_D10_EC_L(0x01, 0x04, 0x01);

static const mxfUL MXF_EC_L(D10_40_525_60_extended_template) = 
    MXF_D10_EC_L(0x01, 0x04, 0x02);

static const mxfUL MXF_EC_L(D10_40_525_60_picture_only) = 
    MXF_D10_EC_L(0x02, 0x04, 0x7f);

static const mxfUL MXF_EC_L(D10_30_625_50_defined_template) = 
    MXF_D10_EC_L(0x01, 0x05, 0x01);

static const mxfUL MXF_EC_L(D10_30_625_50_extended_template) = 
    MXF_D10_EC_L(0x01, 0x05, 0x02);

static const mxfUL MXF_EC_L(D10_30_625_50_picture_only) = 
    MXF_D10_EC_L(0x02, 0x05, 0x7f);

static const mxfUL MXF_EC_L(D10_30_525_60_defined_template) = 
    MXF_D10_EC_L(0x01, 0x06, 0x01);

static const mxfUL MXF_EC_L(D10_30_525_60_extended_template) = 
    MXF_D10_EC_L(0x01, 0x06, 0x02);

static const mxfUL MXF_EC_L(D10_30_525_60_picture_only) = 
    MXF_D10_EC_L(0x02, 0x06, 0x7f);

    
/* A-law mapping */

#define MXF_ALAW_EC_L(byte15) \
    MXF_GENERIC_CONTAINER_LABEL(0x03, 0x02, 0x0A, byte15, 0x00)

static const mxfUL MXF_EC_L(ALawFrameWrapped) = 
    MXF_ALAW_EC_L(0x01);

static const mxfUL MXF_EC_L(ALawClipWrapped) = 
    MXF_ALAW_EC_L(0x02);

static const mxfUL MXF_EC_L(ALawCustomWrapped) = 
    MXF_ALAW_EC_L(0x03);

    
/* MPEG mapping */

#define MXF_MPEG_EC_L(regver, eckind, byte15, byte16) \
    MXF_GENERIC_CONTAINER_LABEL(regver, 0x02, eckind, byte15, byte16)


/* AVC Intra-Frame Coding */

static const mxfUL MXF_EC_L(AVCIFrameWrapped) =
    MXF_MPEG_EC_L(0x0A, 0x10, 0x60, 0x01);

static const mxfUL MXF_EC_L(AVCIClipWrapped) =
    MXF_MPEG_EC_L(0x0A, 0x10, 0x60, 0x02);


/* DNxHD */

/* DNxHD EssenceContainer labels observed in files created by Media Composer Software 2.7.2 */
static const mxfUL MXF_EC_L(DNxHD720p120ClipWrapped) = 
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0e, 0x04, 0x03, 0x01, 0x02, 0x06, 0x03, 0x03};
static const mxfUL MXF_EC_L(DNxHD720p185ClipWrapped) =
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0e, 0x04, 0x03, 0x01, 0x02, 0x06, 0x03, 0x02};
static const mxfUL MXF_EC_L(DNxHD1080p185XClipWrapped) = 
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0e, 0x04, 0x03, 0x01, 0x02, 0x06, 0x01, 0x01};
static const mxfUL MXF_EC_L(DNxHD1080p120ClipWrapped) = 
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0e, 0x04, 0x03, 0x01, 0x02, 0x06, 0x01, 0x02};
static const mxfUL MXF_EC_L(DNxHD1080p185ClipWrapped) = 
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0e, 0x04, 0x03, 0x01, 0x02, 0x06, 0x01, 0x03};
static const mxfUL MXF_EC_L(DNxHD1080p36ClipWrapped) = 
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0e, 0x04, 0x03, 0x01, 0x02, 0x06, 0x01, 0x04};
/* DNxHD EssenceContainer labels observed in files created by Media Composer Adrenaline 2.2.9 */
static const mxfUL MXF_EC_L(DNxHD1080i185XClipWrapped) = 
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0e, 0x04, 0x03, 0x01, 0x02, 0x06, 0x02, 0x01};
static const mxfUL MXF_EC_L(DNxHD1080i120ClipWrapped) = 
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0e, 0x04, 0x03, 0x01, 0x02, 0x06, 0x02, 0x02};
static const mxfUL MXF_EC_L(DNxHD1080i185ClipWrapped) = 
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0e, 0x04, 0x03, 0x01, 0x02, 0x06, 0x02, 0x03};


    
/*
 *
 * Essence element keys
 *
 */
 
#define MXF_EE_K(name) \
    g_##name##_esselement_key

    
    
#define MXF_GENERIC_CONTAINER_ELEMENT_KEY(regver, itemtype, elecount, eletype, elenum) \
    {0x06, 0x0e, 0x2b, 0x34, 0x01, 0x02, 0x01, regver, 0x0d, 0x01, 0x03, 0x01, itemtype, elecount, eletype, elenum}

    
#define MXF_TRACK_NUM(itemtype, elecount, eletype, elenum) \
    ((((uint32_t)itemtype) << 24) | \
    (((uint32_t)elecount) << 16) | \
    (((uint32_t)eletype) << 8) | \
    ((uint32_t)elenum))

    
/* AES3/BWF mappings */

#define MXF_AES3BWF_EE_K(elecount, eletype, elenum) \
    MXF_GENERIC_CONTAINER_ELEMENT_KEY(0x01, 0x16, elecount, eletype, elenum)

#define MXF_AES3BWF_TRACK_NUM(elecount, eletype, elenum) \
    MXF_TRACK_NUM(0x16, elecount, eletype, elenum);
    
#define MXF_BWF_FRAME_WRAPPED_EE_TYPE       0x01
#define MXF_BWF_CLIP_WRAPPED_EE_TYPE        0x02
#define MXF_AES3_FRAME_WRAPPED_EE_TYPE      0x03
#define MXF_AES3_CLIP_WRAPPED_EE_TYPE       0x04
#define MXF_BWF_CUSTOM_WRAPPED_EE_TYPE      0x0B
#define MXF_AES3_CUSTOM_WRAPPED_EE_TYPE     0x0C
    
    
/* IEC-DV and DV-based mappings */

#define MXF_DV_EE_K(elecount, eletype, elenum) \
    MXF_GENERIC_CONTAINER_ELEMENT_KEY(0x01, 0x18, elecount, eletype, elenum)
    
#define MXF_DV_TRACK_NUM(elecount, eletype, elenum) \
    MXF_TRACK_NUM(0x18, elecount, eletype, elenum);
    
#define MXF_DV_FRAME_WRAPPED_EE_TYPE        0x01
#define MXF_DV_CLIP_WRAPPED_EE_TYPE         0x02
    

/* Uncompressed mappings */

#define MXF_UNC_EE_K(elecount, eletype, elenum) \
    MXF_GENERIC_CONTAINER_ELEMENT_KEY(0x01, 0x15, elecount, eletype, elenum)
    
#define MXF_UNC_TRACK_NUM(elecount, eletype, elenum) \
    MXF_TRACK_NUM(0x15, elecount, eletype, elenum);
    
#define MXF_UNC_FRAME_WRAPPED_EE_TYPE       0x02
#define MXF_UNC_CLIP_WRAPPED_EE_TYPE        0x03
#define MXF_UNC_LINE_WRAPPED_EE_TYPE        0x04


void mxf_complete_essence_element_key(mxfKey* key, uint8_t count, uint8_t type, uint8_t num);
void mxf_complete_essence_element_track_num(uint32_t* trackNum, uint8_t count, uint8_t type, uint8_t num);


/* System items */

#define MXF_GC_SYSTEM_ITEM_ELEMENT_KEY(regver, itemtype, schemeid, eleid, elenum) \
    {0x06, 0x0e, 0x2b, 0x34, 0x02, 0x53, 0x01, regver , 0x0d, 0x01, 0x03, 0x01, itemtype, schemeid, eleid, elenum}
    
/* System Scheme 1 - GC compatible */

#define MXF_SS1_ELEMENT_KEY(eleid, elenum) \
    MXF_GC_SYSTEM_ITEM_ELEMENT_KEY(0x01, 0x14, 0x02, eleid, elenum)

/* SDTI-CP */
static const mxfKey MXF_EE_K(SDTI_CP_System_Pack) = 
    {0x06, 0x0e, 0x2b, 0x34, 0x02, 0x05, 0x01, 0x01, 0x0d, 0x01, 0x03, 0x01, 0x04, 0x01, 0x01, 0x00};


/* D-10 mappings */

#define MXF_D10_PICTURE_EE_K(elenum) \
    MXF_GENERIC_CONTAINER_ELEMENT_KEY(0x01, 0x05, 0x01, 0x01, elenum)
    
#define MXF_D10_PICTURE_TRACK_NUM(elenum) \
    MXF_TRACK_NUM(0x05, 0x01, 0x01, elenum);
    
#define MXF_D10_SOUND_EE_K(elenum) \
    MXF_GENERIC_CONTAINER_ELEMENT_KEY(0x01, 0x06, 0x01, 0x10, elenum)
    
#define MXF_D10_SOUND_TRACK_NUM(elenum) \
    MXF_TRACK_NUM(0x06, 0x01, 0x10, elenum);
    
#define MXF_D10_AUX_EE_K(elecount, eletype, elenum) \
    MXF_GENERIC_CONTAINER_ELEMENT_KEY(0x01, 0x07, elecount, eletype, elenum)
    
#define MXF_D10_AUX_TRACK_NUM(elecount, eletype, elenum) \
    MXF_TRACK_NUM(0x07, elecount, eletype, elenum);
    

/* A-law mappings */

#define MXF_ALAW_EE_K(elecount, eletype, elenum) \
    MXF_GENERIC_CONTAINER_ELEMENT_KEY(0x01, 0x16, elecount, eletype, elenum)

#define MXF_ALAW_TRACK_NUM(elecount, eletype, elenum) \
    MXF_TRACK_NUM(0x16, elecount, eletype, elenum);
    
#define MXF_ALAW_FRAME_WRAPPED_EE_TYPE      0x08
#define MXF_ALAW_CLIP_WRAPPED_EE_TYPE       0x09
#define MXF_ALAW_CUSTOM_WRAPPED_EE_TYPE     0x0A
    
    
/* MPEG mappings */

#define MXF_MPEG_PICT_EE_K(elecount, eletype, elenum) \
    MXF_GENERIC_CONTAINER_ELEMENT_KEY(0x01, 0x15, elecount, eletype, elenum)

#define MXF_MPEG_PICT_TRACK_NUM(elecount, eletype, elenum) \
    MXF_TRACK_NUM(0x15, elecount, eletype, elenum);
    
#define MXF_MPEG_PICT_FRAME_WRAPPED_EE_TYPE      0x05
#define MXF_MPEG_PICT_CLIP_WRAPPED_EE_TYPE       0x06
#define MXF_MPEG_PICT_CUSTOM_WRAPPED_EE_TYPE     0x07
    
    
/* DNxHD */

#define MXF_DNXHD_PICT_EE_K(elecount, eletype, elenum) \
    {0x06, 0x0e, 0x2b, 0x34, 0x01, 0x02, 0x01, 0x01, 0x0e, 0x04, 0x03, 0x01, 0x15, elecount, eletype, elenum};


    
/*
 *
 * Partition pack keys
 *
 */
 
#define MXF_PP_K(statusName, kindName) \
    g_##statusName##_##kindName##_pp_key

#define MXF_PP_KEY(regver, kind, status) \
    {0x06, 0x0e, 0x2b, 0x34, 0x02, 0x05, 0x01, regver, 0x0d, 0x01, 0x02, 0x01, 0x01, kind, status, 0x00}
    
    
static const mxfKey MXF_PP_K(OpenIncomplete, Header) = 
    MXF_PP_KEY(0x01, 0x02, 0x01);

static const mxfKey MXF_PP_K(ClosedIncomplete, Header) = 
    MXF_PP_KEY(0x01, 0x02, 0x02);

static const mxfKey MXF_PP_K(OpenComplete, Header) = 
    MXF_PP_KEY(0x01, 0x02, 0x03);

static const mxfKey MXF_PP_K(ClosedComplete, Header) = 
    MXF_PP_KEY(0x01, 0x02, 0x04);

static const mxfKey MXF_PP_K(OpenIncomplete, Body) = 
    MXF_PP_KEY(0x01, 0x03, 0x01);

static const mxfKey MXF_PP_K(ClosedIncomplete, Body) = 
    MXF_PP_KEY(0x01, 0x03, 0x02);

static const mxfKey MXF_PP_K(OpenComplete, Body) = 
    MXF_PP_KEY(0x01, 0x03, 0x03);

static const mxfKey MXF_PP_K(ClosedComplete, Body) = 
    MXF_PP_KEY(0x01, 0x03, 0x04);

static const mxfKey MXF_PP_K(OpenIncomplete, Footer) = 
    MXF_PP_KEY(0x01, 0x04, 0x01);

static const mxfKey MXF_PP_K(ClosedIncomplete, Footer) = 
    MXF_PP_KEY(0x01, 0x04, 0x02);

static const mxfKey MXF_PP_K(OpenComplete, Footer) = 
    MXF_PP_KEY(0x01, 0x04, 0x03);

static const mxfKey MXF_PP_K(ClosedComplete, Footer) = 
    MXF_PP_KEY(0x01, 0x04, 0x04);



/*
 *
 * Filler key
 *
 */
 
/* Note: byte 7 (registry version) should actually be 0x02, but 0x01 is defined here
   for compatibility with existing applications */ 
static const mxfKey g_KLVFill_key =
    {0x06, 0x0e, 0x2b, 0x34, 0x01, 0x01, 0x01, 0x01, 0x03, 0x01, 0x02, 0x10, 0x01, 0x00, 0x00, 0x00};

    
/*
 *
 * Random index pack key
 *
 */
 
static const mxfKey g_RandomIndexPack_key =
    {0x06, 0x0e, 0x2b, 0x34, 0x02, 0x05, 0x01, 0x01, 0x0d, 0x01, 0x02, 0x01, 0x01, 0x11, 0x01, 0x00};


/*
 *
 * Primer pack key
 *
 */
 
static const mxfKey g_PrimerPack_key = 
    {0x06, 0x0e, 0x2b, 0x34, 0x02, 0x05, 0x01, 0x01, 0x0d, 0x01, 0x02, 0x01, 0x01, 0x05, 0x01, 0x00};


    
/*
 *
 * Index table segment key
 *
 */
 
static const mxfKey g_IndexTableSegment_key = 
    {0x06, 0x0e, 0x2b, 0x34, 0x02, 0x53, 0x01, 0x01, 0x0d, 0x01, 0x02, 0x01, 0x01, 0x10, 0x01, 0x00};


    

/*
 *
 * Operational patterns labels
 *
 */
 
#define MXF_OP_L(def, name) \
    g_##name##_op_##def##_label

#define MXF_OP_L_LABEL(regver, complexity, byte14, qualifier) \
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, regver, 0x0d, 0x01, 0x02, 0x01, complexity, byte14, qualifier, 0x00}
    
    
/* OP Atom labels */
 
#define MXF_ATOM_OP_L(byte14) \
    MXF_OP_L_LABEL(0x02, 0x10, byte14, 0x00)
    
static const mxfUL MXF_OP_L(atom, complexity00) = 
    MXF_ATOM_OP_L(0x00);
    
static const mxfUL MXF_OP_L(atom, complexity01) = 
    MXF_ATOM_OP_L(0x01);
    
static const mxfUL MXF_OP_L(atom, complexity02) = 
    MXF_ATOM_OP_L(0x02);
    
static const mxfUL MXF_OP_L(atom, complexity03) = 
    MXF_ATOM_OP_L(0x03);
    
    
int is_op_atom(const mxfUL* label);


/* OP-1A labels */
 
#define MXF_1A_OP_L(qualifier) \
    MXF_OP_L_LABEL(0x01, 0x01, 0x01, qualifier)
    
/* internal essence, stream file, multi-track */
static const mxfUL MXF_OP_L(1a, qq09) = 
    MXF_1A_OP_L(0x09);
    
    
int is_op_1a(const mxfUL* label);



/* Descriptive metadata schemes labels */

#define MXF_DM_L(name) \
    g_##name##_dmscheme_label

static const mxfUL MXF_DM_L(DMS1) = 
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0d, 0x01, 0x04, 0x01, 0x01, 0x01, 0x01, 0x00};



#ifdef __cplusplus
}
#endif


#endif


