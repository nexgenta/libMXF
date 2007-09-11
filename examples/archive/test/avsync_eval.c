/*
 * $Id: avsync_eval.c,v 1.1 2007/09/11 13:24:47 stuart_hc Exp $
 *
 * Functions to find click and flash in clapper board video and audio
 *
 * Copyright (C) 2005  Stuart Cunningham <stuart_hc@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <inttypes.h>
#include <math.h>
#include <stdlib.h>

#include "avsync_eval.h"

// compile with:
// gcc -Wall -g -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -O3 -c avsync_eval.c


// Compute PSNR between classic red flash and 4 byte UYVY macropixel
extern double red_diff_uyvy(const unsigned char *video)
{
	int				sumSqDiff = 0;
	unsigned char	red[] = {0x5f, 0x4b, 0xe6, 0x4b};

	sumSqDiff += ( red[0] - video[0] ) * ( red[0] - video[0] );
	sumSqDiff += ( red[1] - video[1] ) * ( red[1] - video[1] );
	sumSqDiff += ( red[2] - video[2] ) * ( red[2] - video[2] );
	sumSqDiff += ( red[3] - video[3] ) * ( red[3] - video[3] );

	if (sumSqDiff == 0)
	{
		return 20.0 * log10( 255.0 / sqrt(1.0 / 4) );
	}
	return 20.0 * log10( 255.0 / sqrt((double)sumSqDiff / 4) );
}

// Return true if a double-clapper tape red flash is found
extern int find_red_flash_uyvy(const unsigned char *video_buf, int line_size)
{
	// Look for strip of red flash on line 30, from pixel 20 to 80
	// Improve this by searching surrounding area in case of VT menu overlay
	double total_diff = 0;
	int	num_diffs = 0;
	int i;
	for (i = 20; i < 80; i += 4) {
		double red_diff = red_diff_uyvy( video_buf + 30*line_size + i );
		//printf("%02x ", video_buf[ 30*line_size + i*4 + 0 ]);
		//printf("%02x ", video_buf[ 30*line_size + i*4 + 1 ]);
		//printf("%02x ", video_buf[ 30*line_size + i*4 + 2 ]);
		//printf("%02x ", video_buf[ 30*line_size + i*4 + 3 ]);
		//printf("red_diff = %.3f\n", red_diff);
		total_diff += red_diff;
		num_diffs++;
	}
	double avg_diff = total_diff / num_diffs;
	//printf("avg_diff=%.3f\n", total_diff / num_diffs);
	if (avg_diff > 30.0)
		return 1;
	return 0;
}

// Results are returned through pointers
extern void find_audio_click_32bit_stereo(const unsigned char *p_audio,
				int *p_click1, int *p_offset1,
				int *p_click2, int *p_offset2)
{
	int audio_size = 1920*4*2;		// 2 channel 32bit audio
	int found1 = 0, moderate1_off = -1;
	int found2 = 0, moderate2_off = -1;

	// Experiment on double-clapper-board tape showed:
	// + large amplitude first seen at 1040 bytes chan1,2 (130 single channel samples)
	// + large amplitude first seen at 4528 bytes chan3,4 (566 single channel samples)

	// 0x1b000000 is a 32bit large amplitude found by experiment
	// to be in the first frame of the "click" but not subsequent frames
	int threshold		= 0x1b000000;		// loudest part of 'click'
	int mod_threshold	= 0x06000000;		// to catch the very start of a 'click'

	int i;
	for (i = 0; i < audio_size; i += 8) {
		int32_t samp1 = *(int32_t*)(p_audio + i);
		int32_t samp2 = *(int32_t*)(p_audio + i + 4);

		if (abs(samp1) > mod_threshold && moderate1_off == -1)
			moderate1_off = i / (4 * 2);
		if (abs(samp2) > mod_threshold && moderate2_off == -1)
			moderate2_off = i / (4 * 2);

		if (abs(samp1) > threshold && ! found1) {
			*p_click1 = 1;
			*p_offset1 = moderate1_off;
			found1 = 1;
		}
		if (abs(samp2) > threshold && ! found2) {
			*p_click2 = 1;
			*p_offset2 = moderate2_off;
			found2 = 1;
		}

		//if (verbose > 1) {
		//	printf("%02x ", p_audio[i+0]);
		//	printf("%02x ", p_audio[i+1]);
		//	printf("%02x ", p_audio[i+2]);
		//	printf("%02x ", p_audio[i+3]);
		//	printf("%02x ", p_audio[i+4]);
		//	printf("%02x ", p_audio[i+5]);
		//	printf("%02x ", p_audio[i+6]);
		//	printf("%02x ", p_audio[i+7]);
		//	printf(" 0x%08x: %8x %8x\n", i, abs(samp1), abs(samp2));
		//}
		if (found1 && found2)
			return;
	}
	*p_click1 = 0;
	*p_offset1 = -1;
	*p_click2 = 0;
	*p_offset2 = -1;
	return;
}

// p_audio - 1 frame of 32bit mono audio buffer (25fps, 48kHz)
// p_click - true if click found
// p_offset - offset in samples of where start of click was found
extern void find_audio_click_mono(const unsigned char *p_audio, int bitsPerSample, int *p_click, int *p_offset)
{
	int moderate_off = -1;

	// Experiment on double-clapper-board tape showed:
	// + large amplitude first seen at 1040 bytes chan1,2 (130 single channel samples)
	// + large amplitude first seen at 4528 bytes chan3,4 (566 single channel samples)

	// 0x1b000000 is a 32bit large amplitude found by experiment
	// to be in the first frame of the "click" but not subsequent frames
	int threshold		= 0x1b000000;		// loudest part of 'click'
	int mod_threshold	= 0x06000000;		// to catch the very start of a 'click'

	// Round up e.g. 20 bits-per-sample needs 3 bytes-per-sample
	int bytesPerSample = (bitsPerSample + 7) / 8;
	int audio_size = 1920 * bytesPerSample;

	int i;
	for (i = 0; i < audio_size; i += bytesPerSample) {
		// Convert all audio samples into 32bit integer
		int32_t samp;
		switch (bytesPerSample) {
			case 4:
				samp = *(int32_t*)(p_audio + i);
				break;
			case 3:
				samp =	(int32_t)(	p_audio[i+2] << 24 |
									p_audio[i+1] << 16 |
									p_audio[i+0] << 8);
				break;
			case 2:
				samp =	(*(int16_t*)(p_audio + i)) << 16;
				break;
			case 1:
				samp =	((int8_t)(p_audio[i])) << 24;
				break;
			default:	// silently fail
				return;
		}

		// Compare against start-of-click threshold
		if (abs(samp) > mod_threshold && moderate_off == -1)
			moderate_off = i / 4;

		// Compare against peak-of-click threshold
		if (abs(samp) > threshold) {
			*p_click = 1;
			*p_offset = moderate_off;
			return;
		}
	}
	*p_click = 0;
	*p_offset = -1;
	return;
}
