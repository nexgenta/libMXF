/*
 * $Id: avsync_eval.h,v 1.1 2007/09/11 13:24:47 stuart_hc Exp $
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
 
#ifndef AVSYNC_EVAL_H
#define AVSYNC_EVAL_H

// Compute PSNR between classic red flash and 4 byte UYVY macropixel
extern double red_diff_uyvy(const unsigned char *video);

// Return true if a double-clapper tape red flash is found
extern int find_red_flash_uyvy(const unsigned char *video_buf, int line_size);

// Results are returned through pointers
extern void find_audio_click_32bit_stereo(const unsigned char *p_audio,
				int *p_click1, int *p_offset1,
				int *p_click2, int *p_offset2);

// p_audio - 1 frame of 32bit mono audio buffer (25fps, 48kHz)
// bitPerSample - supported values 32, 24, 16, 8
// p_click - true if click found
// p_offset - offset in samples of where start of click was found
extern void find_audio_click_mono(const unsigned char *p_audio, int bitsPerSample, int *p_click, int *p_offset);

#endif
