/*
  LPC-10 voice codec, part of the HawkVoice Direct Interface (HVDI)
  cross platform network voice library
  Copyright (C) 2001-2004 Phil Frisbie, Jr. (phil@hawksoft.com)

  The VBR algorithm was contributed by
  Ben Appleton <appleton@bigpond.net.au>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.
  
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.
    
  You should have received a copy of the GNU Library General Public
  License along with this library; if not, write to the
  Free Software Foundation, Inc., 59 Temple Place - Suite 330,
  Boston, MA  02111-1307, USA.
      
  Or go to http://www.gnu.org/copyleft/lgpl.html
*/

#ifndef LPC10_H
#define LPC10_H

#ifdef __cplusplus
extern "C" {
#endif

#define LPC10_SAMPLES_PER_FRAME     180
#define LPC10_ENCODED_FRAME_SIZE    7
/* NOTE ON FRAME SIZE
The VBR codec creates encoded frame sizes of 1, 4, or 7 bytes.
1 byte, silence frames, about 356 bps
4 bytes, unvoiced frames, about 1422 bps
7 bytes, voiced frames, about 2488 bps */

/* NOTE ON DECODING A VBR STREAM
The VBR decoder relies on the frame type encoded into the first
byte of the frame. If you receive a stream of more than one frame
appended together, there is no way for you to know how many times to
call vbr_lpc10_decode() before hand. Therefore, a forth parameter
has been added to vbr_lpc10_decode(), the int *p. *p returns with
a value to tell you how many bytes were in the frame that was just
processed. For example code, see the file decpacket.c, and look at
the function vbrlpc10decode().
*/

typedef struct lpc10_e_state lpc10_encoder_state;
typedef struct lpc10_d_state lpc10_decoder_state;

lpc10_encoder_state * create_lpc10_encoder_state (void);
void init_lpc10_encoder_state (lpc10_encoder_state *st);
int lpc10_encode(short *in, unsigned char *out, lpc10_encoder_state *st);
int vbr_lpc10_encode(short *in, unsigned char *out, lpc10_encoder_state *st);
void destroy_lpc10_encoder_state (lpc10_encoder_state *st);

lpc10_decoder_state * create_lpc10_decoder_state (void);
void init_lpc10_decoder_state (lpc10_decoder_state *st);
int lpc10_decode(unsigned char *in, int inSz, short *out, lpc10_decoder_state *st);
int vbr_lpc10_decode(unsigned char *in, int inSz, short *out, lpc10_decoder_state *st, int *p);
void destroy_lpc10_decoder_state (lpc10_decoder_state *st);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* LPC10_H */
