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

#if !defined( MACOSX ) && !defined (__APPLE__)
#include <malloc.h>
#endif
#include <stdlib.h>
#ifdef __MWERKS__
#include <string.h>
#else
#include <memory.h>
#endif
#ifdef _MSC_VER
#pragma warning (disable:4711) /* to disable automatic inline warning */
#endif
#include <math.h>
#include "ftol.h"
#include "lpc10.h"

#if __GNUC__ == 2
#define lrintf(d) ((int) (d))
#endif

#define LPC10_BITS_IN_COMPRESSED_FRAME 54

#ifndef TRUE
#define TRUE (1)
#define FALSE (0)
#endif

#ifndef min
#define min(a,b) ((a) <= (b) ? (a) : (b))
#define max(a,b) ((a) >= (b) ? (a) : (b))
#endif

typedef struct lpc10_d_state {

    /* State used by function decode */
    long iptold;   /* initial value 60 */

    /* State used by function synths */
    float buf[360];
    long buflen;   /* initial value 180 */

    /* State used by function pitsyn */
    long ivoico;   /* no initial value necessary as long as first_pitsyn is initially TRUE_ */
    long ipito;   /* no initial value necessary as long as first_pitsyn is initially TRUE_ */
    float rmso;   /* initial value 1.f */
    float rco[10];   /* no initial value necessary as long as first_pitsyn is initially TRUE_ */
    long jsamp;   /* no initial value necessary as long as first_pitsyn is initially TRUE_ */
    int first_pitsyn;   /* initial value TRUE_ */

    /* State used by function bsynz */
    long ipo;
    float exc[166];
    float exc2[166];
    float lpi1;
    float lpi2;
    float lpi3;
    float hpi1;
    float hpi2;
    float hpi3;
    float rmso_bsynz;

    /* State used by function deemp */
    float dei1;
    float dei2;
    float deo1;
    float deo2;
    float deo3;

} lpc10_d_state_t;


extern long lpcbits[10];

/* Table of constant values */
static long detau[128] = { 0,0,0,3,0,3,3,31,0,3,3,21,3,3,29,30,0,3,3,
20,3,25,27,26,3,23,58,22,3,24,28,3,0,3,3,3,3,39,33,32,3,37,35,36,
3,38,34,3,3,42,46,44,50,40,48,3,54,3,56,3,52,3,3,1,0,3,3,108,3,78,
100,104,3,84,92,88,156,80,96,3,3,74,70,72,66,76,68,3,62,3,60,3,64,
3,3,1,3,116,132,112,148,152,3,3,140,3,136,3,144,3,3,1,124,120,128,
3,3,3,3,1,3,3,3,1,3,1,1,1 };

static long rmst[64] = { 1024,936,856,784,718,656,600,550,502,460,420,
384,352,328,294,270,246,226,206,188,172,158,144,132,120,110,102,
92,84,78,70,64,60,54,50,46,42,38,34,32,30,26,24,22,20,18,17,16,15,
14,13,12,11,10,9,8,7,6,5,4,3,2,1,0 };

static long detab7[32] = { 4,11,18,25,32,39,46,53,60,66,72,77,82,87,92,
96,101,104,108,111,114,115,117,119,121,122,123,124,125,126,127,127 };

static float descl[8] = { .6953f,.625f,.5781f,.5469f,.5312f,.5391f,.4688f,.3828f };
static long deadd[8] = { 1152,-2816,-1536,-3584,-1280,-2432,768,-1920 };
static long qb[8] = { 511,511,1023,1023,1023,1023,2047,4095 };
static long nbit[10] = { 8,8,5,5,4,4,4,4,3,2 };
static long kexc[25] = { 8,-16,26,-48,86,-162,294,-502,718,-728,184,
672,-610,-672,184,728,718,502,294,162,86,48,26,16,8 };

static void pitsyn(long *voice, long *pitch, float *rms, float *rc,
             long *ivuv, long *ipiti, float *rmsi, float *rci, long *nout,
             float *ratio, lpc10_decoder_state *st)
{
    /* Initialized data */
    
    float *rmso;
    
    /* Local variables */
    float alrn, alro, yarc[10], prop;
    long i, j, vflag, jused, lsamp;
    long *jsamp;
    float slope;
    long *ipito;
    float uvpit;
    long ip, nl, ivoice;
    long *ivoico;
    long istart;
    float *rco;
    float xxy;
    
    
    /* Function Body */
    ivoico = &(st->ivoico);
    ipito = &(st->ipito);
    rmso = &(st->rmso);
    rco = &(st->rco[0]);
    jsamp = &(st->jsamp);
    
    if (*rms < 1.f) {
        *rms = 1.f;
    }
    if (*rmso < 1.f) {
        *rmso = 1.f;
    }
    uvpit = 0.f;
    *ratio = *rms / (*rmso + 8.f);

    if (st->first_pitsyn) {
        lsamp = 0;
        ivoice = voice[1];
        if (ivoice == 0) {
            *pitch = LPC10_SAMPLES_PER_FRAME / 4;
        }
        *nout = LPC10_SAMPLES_PER_FRAME / *pitch;
        *jsamp = LPC10_SAMPLES_PER_FRAME - *nout * *pitch;
        
        for (i = 0; i < *nout; ++i) {
            for (j = 0; j < 10; ++j) {
                rci[j + i * 10] = rc[j];
            }
            ivuv[i] = ivoice;
            ipiti[i] = *pitch;
            rmsi[i] = *rms;
        }
        st->first_pitsyn = FALSE;

    } else {
        vflag = 0;
        lsamp = LPC10_SAMPLES_PER_FRAME + *jsamp;
        slope = (*pitch - *ipito) / (float) lsamp;
        *nout = 0;
        jused = 0;
        istart = 1;
        if (voice[0] == *ivoico && voice[1] == voice[0]) {
            if (voice[1] == 0) {
                /* SSUV - -   0  ,  0  ,  0 */
                *pitch = LPC10_SAMPLES_PER_FRAME / 4;
                *ipito = *pitch;
                if (*ratio > 8.f) {
                    *rmso = *rms;
                }
            }
            /* SSVC - -   1  ,  1  ,  1 */
            slope = (*pitch - *ipito) / (float) lsamp;
            ivoice = voice[1];
        } else {
            if (*ivoico != 1) {
                if (*ivoico == voice[0]) {
                    /* UV2VC2 - -  0  ,  0  ,  1 */
                    nl = lsamp - LPC10_SAMPLES_PER_FRAME / 4;
                } else {
                    /* UV2VC1 - -  0  ,  1  ,  1 */
                    nl = lsamp - LPC10_SAMPLES_PER_FRAME * 3 / 4;
                }
                ipiti[0] = nl / 2;
                ipiti[1] = nl - ipiti[0];
                ivuv[0] = 0;
                ivuv[1] = 0;
                rmsi[0] = *rmso;
                rmsi[1] = *rmso;
                for (i = 0; i < 10; ++i) {
                    rci[i] = rco[i];
                    rci[i + 10] = rco[i];
                    rco[i] = rc[i];
                }
                slope = 0.f;
                *nout = 2;
                *ipito = *pitch;
                jused = nl;
                istart = nl + 1;
                ivoice = 1;
            } else {
                if (*ivoico != voice[0]) {
                    lsamp = LPC10_SAMPLES_PER_FRAME / 4 + *jsamp;
                } else {
                    lsamp = LPC10_SAMPLES_PER_FRAME * 3 / 4 + *jsamp;
                }
                for (i = 0; i < 10; ++i) {
                    yarc[i] = rc[i];
                    rc[i] = rco[i];
                }
                ivoice = 1;
                slope = 0.f;
                vflag = 1;
            }
        }
        for(;;) {
            
            for (i = istart; i <= lsamp; ++i) {
                if (uvpit != 0.f) {
                    ip = lrintf(uvpit);
                }
                else {
                    ip = lrintf(*ipito + slope * i + .5f);
                }
                
                if (ip <= i - jused) {
                    ipiti[*nout] = ip;
                    *pitch = ip;
                    ivuv[*nout] = ivoice;
                    jused += ip;
                    prop = (jused - ip / 2) / (float) lsamp;
                    for (j = 0; j < 10; ++j) {
                        alro = (float)log((rco[j] + 1) / (1 - rco[j]));
                        alrn = (float)log((rc[j] + 1) / (1 - rc[j]));
                        xxy = alro + prop * (alrn - alro);
                        xxy = (float)exp(xxy);
                        rci[j + *nout * 10] = (xxy - 1) / (xxy + 1);
                    }
                    rmsi[*nout] = (float)(log(*rmso) + prop * (log(*rms) - log(*rmso)));
                    rmsi[*nout] = (float)exp(rmsi[*nout]);
                    ++(*nout);
                }
            }
            if (vflag != 1) {
                break;
            }
            
            vflag = 0;
            istart = jused + 1;
            lsamp = LPC10_SAMPLES_PER_FRAME + *jsamp;
            slope = 0.f;
            ivoice = 0;
            uvpit = (float) ((lsamp - istart) / 2);
            if (uvpit > 90.f) {
                uvpit /= 2;
            }
            *rmso = *rms;
            for (i = 1; i <= 10; ++i) {
                rco[i - 1] = rc[i - 1] = yarc[i - 1];
            }
        }
       *jsamp = lsamp - jused;
    }
    if (*nout != 0) {
        *ivoico = voice[1];
        *ipito = *pitch;
        *rmso = *rms;
        for (i = 0; i < 10; ++i) {
            rco[i] = rc[i];
        }
    }
} /* pitsyn_ */

#define MIDTAP 1
#define MAXTAP 4
static short y[MAXTAP+1]={-21161, -8478, 30892,-10216, 16950};
static int j=MIDTAP, k=MAXTAP;

static int random16 (void)
{
    int the_random;
    
    /*   The following is a 16 bit 2's complement addition,
    *   with overflow checking disabled	*/
    
    y[k] = (short)(y[k] + y[j]);
    
    the_random = y[k];
    k--;
    if (k < 0) k = MAXTAP;
    j--;
    if (j < 0) j = MAXTAP;
    
    return(the_random);
}

static void bsynz(float *coef, long ip, long *iv, 
                  float *sout, float *rms, float *ratio, float *g2pass,
                  lpc10_decoder_state *st)
{
    /* Initialized data */
    
    long *ipo;
    float *rmso;
    float *exc;
    float *exc2;
    float lpi1;
    float lpi2;
    float hpi1;
    float hpi2;
    
    /* Local variables */
    float gain, xssq;
    long i, j, k;
    float pulse;
    long px;
    float sscale;
    float xy, sum, ssq;
    float lpi0, hpi0;
    
    /* Parameter adjustments */
    if (coef) {
        --coef;
    }
    
    /* Function Body */
    ipo = &(st->ipo);
    exc = &(st->exc[0]);
    exc2 = &(st->exc2[0]);
    lpi1 = st->lpi1;
    lpi2 = st->lpi2;
    hpi1 = st->hpi1;
    hpi2 = st->hpi2;
    rmso = &(st->rmso_bsynz);
    
    /*                  MAXPIT+MAXORD=166 */
    /*  Calculate history scale factor XY and scale filter state */
    /* Computing MIN */
    xy = min((*rmso / (*rms + 1e-6f)),8.f);
    *rmso = *rms;
    for (i = 0; i < 10; ++i) {
        exc2[i] = exc2[*ipo + i] * xy;
    }
    *ipo = ip;
    if (*iv == 0) {
        /*  Generate white noise for unvoiced */
        for (i = 0; i < ip; ++i) {
            exc[10 + i] = (float) (random16() >> 6);
        }
        px = ((random16() + 32768) * (ip - 1) >> 16) + 10 + 1;
        pulse = *ratio * 85.5f;
        if (pulse > 2e3f) {
            pulse = 2e3f;
        }
        exc[px - 1] += pulse;
        exc[px] -= pulse;
        /*  Load voiced excitation */
    } else {
        sscale = (float)sqrt((float) (ip)) * 0.144341801f;
        for (i = 0; i < ip; ++i) {
            float temp;
            
            if (i > 27) {
                temp = 0.f;
            }
            else if (i < 25) {
                lpi0 = temp = sscale * kexc[i];
                temp = lpi0 * .125f + lpi1 * .75f + lpi2 * .125f;
                lpi2 = lpi1;
                lpi1 = lpi0;
            }
            else{
                lpi0 = temp = 0.f;
                temp = lpi1 * .75f + lpi2 * .125f;
                lpi2 = lpi1;
                lpi1 = lpi0;
            }
            hpi0 = (float)(random16() >> 6);
            exc[10 + i] = temp + hpi0 * -.125f + hpi1 * .25f + hpi2 * -.125f;
            hpi2 = hpi1;
            hpi1 = hpi0;
        }
    }
    /*   Synthesis filters: */
    /*    Modify the excitation with all-zero filter  1 + G*SUM */
    xssq = 0.f;
    for (i = 0; i < ip; ++i) {
        k = 10 + i;
        sum = 0.f;
        for (j = 1; j <= 10; ++j) {
            sum += coef[j] * exc[k - j];
        }
        sum *= *g2pass;
        exc2[k] = sum + exc[k];
    }
    /*   Synthesize using the all pole filter  1 / (1 - SUM) */
    for (i = 0; i < ip; ++i) {
        k = 10 + i;
        sum = 0.f;
        for (j = 1; j <= 10; ++j) {
            sum += coef[j] * exc2[k - j];
        }
        exc2[k] += sum;
        xssq += exc2[k] * exc2[k];
    }
    /*  Save filter history for next epoch */
    for (i = 0; i < 10; ++i) {
        exc[i] = exc[ip + i];
        exc2[i] = exc2[ip + i];
    }
    /*  Apply gain to match RMS */
    ssq = *rms * *rms * ip;
    gain = (float)sqrt(ssq / xssq);
    for (i = 0; i < ip; ++i) {
        sout[i] = gain * exc2[10 + i];
    }
    st->lpi1 = lpi1;
    st->lpi2 = lpi2;
    st->hpi1 = hpi1;
    st->hpi2 = hpi2;
} /* bsynz_ */

static void irc2pc(float *rc, float *pc, float gprime, float *g2pass)
{
    /* System generated locals */
    long i2;
    
    /* Local variables */
    float temp[10];
    long i, j;
    
    /* Parameter adjustments */
    --pc;
    --rc;
    
    /* Function Body */
    *g2pass = 1.f;
    for (i = 1; i <= 10; ++i) {
        *g2pass *= 1.f - rc[i] * rc[i];
    }
    *g2pass = gprime * (float)sqrt(*g2pass);
    pc[1] = rc[1];
    for (i = 2; i <= 10; ++i) {
        i2 = i - 1;
        for (j = 1; j <= i2; ++j) {
            temp[j - 1] = pc[j] - rc[i] * pc[i - j];
        }
        i2 = i - 1;
        for (j = 1; j <= i2; ++j) {
            pc[j] = temp[j - 1];
        }
        pc[i] = rc[i];
    }
} /* irc2pc_ */

static void deemp(float *x, long n, lpc10_decoder_state *st)
{
    /* Initialized data */
    
    float dei1;
    float dei2;
    float deo1;
    float deo2;
    float deo3;
    
    /* Local variables */
    long k;
    float dei0;
    
    /* Function Body */
    
    dei1 = st->dei1;
    dei2 = st->dei2;
    deo1 = st->deo1;
    deo2 = st->deo2;
    deo3 = st->deo3;
    
    for (k = 0; k < n; ++k) {
        dei0 = x[k];
        x[k] = dei0 - dei1 * 1.9998f + dei2 + deo1 * 2.5f - deo2 * 2.0925f + deo3 * .585f;
        dei2 = dei1;
        dei1 = dei0;
        deo3 = deo2;
        deo2 = deo1;
        deo1 = x[k];
    }
    st->dei1 = dei1;
    st->dei2 = dei2;
    st->deo1 = deo1;
    st->deo2 = deo2;
    st->deo3 = deo3;
} /* deemp_ */

static void synths(long *voice, long pitch, float rms, float *rc, short *speech, lpc10_decoder_state *st)
{
    /* Initialized data */
    
    float *buf;
    long *buflen;
    
    /* Local variables */
    float rmsi[16];
    long nout, ivuv[16], i, j;
    float ratio;
    long ipiti[16];
    float g2pass;
    float pc[10];
    float rci[160]	/* was [10][16] */;
    
    /* Function Body */
    buf = &(st->buf[0]);
    buflen = &(st->buflen);
    
    pitch = max(min(pitch,156), 20);
    for (i = 0; i < 10; ++i) {
        rc[i] = max(min(rc[i],.99f), -.99f);
    }
    pitsyn(voice, &pitch, &rms, rc, ivuv, ipiti, rmsi, rci, &nout, &ratio, st);
    if (nout > 0) {
        for (j = 0; j < nout; ++j) {
            
            irc2pc(&rci[j * 10], pc, 0.7f, &g2pass);
            bsynz(pc, ipiti[j], &ivuv[j], &buf[*buflen], &rmsi[j], &ratio, &g2pass, st);
            deemp(&buf[*buflen], ipiti[j], st);
            *buflen += ipiti[j];
        }
        
        for (i = 0; i < LPC10_SAMPLES_PER_FRAME; ++i) {
            speech[i] = (short) max(-32768, min(lrintf(8.0f * buf[i]), 32767));
        }
        *buflen -= LPC10_SAMPLES_PER_FRAME;
        for (i = 0; i < *buflen; ++i) {
            buf[i] = buf[i + LPC10_SAMPLES_PER_FRAME];
        }
    }
} /* synths_ */

static void decode(long ipitv, long irms, long *irc, long *voice, long *pitch,
                   float *rms, float *rc, lpc10_decoder_state *st)
{
    
    /* Local variables */
    long i, i1, i2, i4;
    long ishift;
    
    /* Function Body */
    
    i4 = detau[ipitv];
    voice[0] = 1;
    voice[1] = 1;
    if (ipitv <= 1) {
        voice[0] = 0;
    }
    if (ipitv == 0 || ipitv == 2) {
        voice[1] = 0;
    }
    *pitch = i4;
    if (*pitch <= 4) {
        *pitch = st->iptold;
    }
    if (voice[0] == 1 && voice[1] == 1) {
        st->iptold = *pitch;
    }
    if (voice[0] != voice[1]) {
        *pitch = st->iptold;
    }
    /*   Decode RMS */
    irms = rmst[(31 - irms) * 2];
    /*  Decode RC(1) and RC(2) from log-area-ratios */
    /*  Protect from illegal coded value (-16) caused by bit errors */
    for (i = 0; i < 2; ++i) {
        i2 = irc[i];
        i1 = 0;
        if (i2 < 0) {
            i1 = 1;
            i2 = -i2;
            if (i2 > 15) {
                i2 = 0;
            }
        }
        i2 = detab7[i2 * 2];
        if (i1 == 1) {
            i2 = -i2;
        }
        ishift = 15 - nbit[i];
        irc[i] = i2 * (2 << (ishift-1));
    }
    /*  Decode RC(3)-RC(10) to sign plus 14 bits */
    for (i = 2; i < 10; ++i) {
        i2 = irc[i];
        ishift = 15 - nbit[i];
        i2 *= (2 << (ishift-1));
        i2 += qb[i - 2];
        irc[i] = (int)(i2 * descl[i - 2] + deadd[i - 2]);
    }
    /*  Scale RMS and RC's to floats */
    *rms = (float) (irms);
    for (i = 0; i < 10; ++i) {
        rc[i] = irc[i] / 16384.f;
    }
} /* decode_ */

static void unpack(long *array, long bits, long *value, long *pointer)
{
    int i;
    
    for (i = 0, *value = 0; i < bits; i++, (*pointer)++)
        *value |= array[*pointer] << i;
}

static void chanrd(long *ipitv, long *irms, long *irc, long *ibits, int *p, int vbr)
{
    static long bit[] = {0, 1, 2, 4, 8, 16, 32, 64, 128};
    long pointer, i;
    
    pointer = 0;
    
    unpack(ibits, 7, ipitv, &pointer);
    if(vbr == TRUE)
    {
        /* check for silence packet */
        if (*ipitv == 127) {
            *ipitv = 0;
            *irms = 0;
            *p = 1;
            return;
        }
    }
    unpack(ibits, 5, irms, &pointer);
    *p = 4;
    
    for (i = 0; i < 4; ++i) {
        unpack(ibits, lpcbits[i], &irc[i], &pointer);
    }
    if ((*ipitv != 0 && *ipitv != 126) || vbr == FALSE) {
        *p = 7;
        for (i = 4; i < 10; ++i) {
            unpack(ibits, lpcbits[i], &irc[i], &pointer);
        }
    }

    /*   Sign extend RC's */
    for (i = 0; i < 10; ++i) {
        if ((irc[i] & bit[lpcbits[i]]) != 0) {
            irc[i] -= bit[lpcbits[i]] << 1;
        }
    }
}

static int lpc10_decode_int(unsigned char *in, int inLenMax, short *speech,
                 lpc10_decoder_state *st, int *p, int vbr)
{
    long bits[LPC10_BITS_IN_COMPRESSED_FRAME];
    int     i;
    long irms, voice[2], pitch, ipitv;
    float rc[10];
    long irc[10];
    float rms;
    
    /* unpack bits into array */
    int maxBits = inLenMax << 3;
    if(maxBits > LPC10_BITS_IN_COMPRESSED_FRAME)
       maxBits = LPC10_BITS_IN_COMPRESSED_FRAME;

    for (i = 0; i < maxBits; i++)
    {
        bits[i] = (in[i >> 3] & (1 << (i & 7))) != 0 ? 1 : 0;
    }
    for (; i < LPC10_BITS_IN_COMPRESSED_FRAME; i++)
        bits[i] = 0;
    
    /* decode speech */
    memset(irc, 0, sizeof(irc));
    chanrd(&ipitv, &irms, irc, bits, p, vbr);
    decode(ipitv, irms, irc, voice, &pitch, &rms, rc, st);
    synths(voice, pitch, rms, rc, speech, st);
    
    return LPC10_SAMPLES_PER_FRAME;
}

int lpc10_decode(unsigned char *in, int inLenMax, short *speech, lpc10_decoder_state *st)
{
    int p = 0;

    return lpc10_decode_int(in, inLenMax, speech, st, &p, FALSE);
}

int vbr_lpc10_decode(unsigned char *in, int inLenMax, short *speech, lpc10_decoder_state *st, int *p)
{
    return lpc10_decode_int(in, inLenMax, speech, st, p, TRUE);
}

/* Allocate memory for, and initialize, the state that needs to be
kept from decoding one frame to the next for a single
LPC-10-compressed audio stream.  Return 0 if malloc fails,
otherwise return pointer to new structure. */

lpc10_decoder_state *create_lpc10_decoder_state(void)
{
    lpc10_decoder_state *st;
    
    st = (lpc10_decoder_state *)malloc((unsigned) sizeof (lpc10_decoder_state));
    return (st);
}

void init_lpc10_decoder_state(lpc10_decoder_state *st)
{
    int i;
    
    /* State used by function decode */
    st->iptold = 60;
    
    /* State used by function synths */
    for (i = 0; i < 360; i++) {
        st->buf[i] = 0.0f;
    }
    st->buflen = 180;
    
    /* State used by function pitsyn */
    st->rmso = 1.0f;
    st->first_pitsyn = TRUE;
    
    /* State used by function bsynz */
    st->ipo = 0;
    for (i = 0; i < 166; i++) {
        st->exc[i] = 0.0f;
        st->exc2[i] = 0.0f;
    }
    st->lpi1 = 0.0f;
    st->lpi2 = 0.0f;
    st->lpi3 = 0.0f;
    st->hpi1 = 0.0f;
    st->hpi2 = 0.0f;
    st->hpi3 = 0.0f;
    st->rmso_bsynz = 0.0f;
    
    /* State used by function deemp */
    st->dei1 = 0.0f;
    st->dei2 = 0.0f;
    st->deo1 = 0.0f;
    st->deo2 = 0.0f;
    st->deo3 = 0.0f;
}

void destroy_lpc10_decoder_state (lpc10_decoder_state *st)
{
    if(st != NULL)
    {
        free(st);
        st = NULL;
    }
}
