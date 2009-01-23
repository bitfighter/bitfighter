/*
  LPC-10 voice codec, part of the HawkVoice Direct Interface (HVDI)
  cross platform network voice library
  Copyright (C) 2001-2003 Phil Frisbie, Jr. (phil@hawksoft.com)

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
#define lrintf(d) ((int)(d))
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

typedef struct lpc10_e_state {
    /* State used only by function hp100 */
    float z11;
    float z21;
    float z12;
    float z22;
    
    /* State used by function analys */
    float inbuf[540], pebuf[540];
    float lpbuf[696], ivbuf[312];
    float bias;
    long osbuf[10];  /* no initial value necessary */
    long osptr;     /* initial value 1 */
    long obound[3];
    long vwin[6]	/* was [2][3] */;   /* initial value vwin[4] = 307; vwin[5] = 462; */
    long awin[6]	/* was [2][3] */;   /* initial value awin[4] = 307; awin[5] = 462; */
    long voibuf[8]	/* was [2][4] */;
    float rmsbuf[3];
    float rcbuf[30]	/* was [10][3] */;
    float zpre;


    /* State used by function onset */
    float n;
    float d;   /* initial value 1.f */
    float fpc;   /* no initial value necessary */
    float l2buf[16];
    float l2sum1;
    long l2ptr1;   /* initial value 1 */
    long l2ptr2;   /* initial value 9 */
    long lasti;    /* no initial value necessary */
    int hyst;   /* initial value FALSE_ */

    /* State used by function voicin */
    float dither;   /* initial value 20.f */
    float snr;
    float maxmin;
    float voice[6]	/* was [2][3] */;   /* initial value is probably unnecessary */
    long lbve, lbue, fbve, fbue;
    long ofbue, sfbue;
    long olbue, slbue;

    /* State used by function dyptrk */
    float s[60];
    long p[120]	/* was [60][2] */;
    long ipoint;
    float alphax;

    /* State used by function chanwr */
    long isync;

} lpc10_e_state_t;

/* Table of constant values */
long lpcbits[10] = { 5,5,5,5,4,4,4,4,3,3 };

static long entau[60] = { 19,11,27,25,29,21,23,22,30,14,15,7,39,38,46,
	    42,43,41,45,37,53,49,51,50,54,52,60,56,58,26,90,88,92,84,86,82,83,
	    81,85,69,77,73,75,74,78,70,71,67,99,97,113,112,114,98,106,104,108,
	    100,101,76 };
static long enadd[8] = { 1920,-768,2432,1280,3584,1536,2816,-1152 };
static float enscl[8] = { .0204f,.0167f,.0145f,.0147f,.0143f,.0135f,.0125f,.0112f };
static long enbits[8] = { 6,5,4,4,4,4,3,3 };
static long entab6[64] = { 0,0,0,0,0,0,1,1,1,1,1,1,1,2,2,2,2,2,2,2,3,3,
	    3,3,3,3,3,4,4,4,4,4,4,4,5,5,5,5,5,6,6,6,6,6,7,7,7,7,7,8,8,8,8,9,9,
	    9,10,10,11,11,12,13,14,15 };
static long rmst[64] = { 1024,936,856,784,718,656,600,550,502,460,420,
	    384,352,328,294,270,246,226,206,188,172,158,144,132,120,110,102,
	    92,84,78,70,64,60,54,50,46,42,38,34,32,30,26,24,22,20,18,17,16,15,
	    14,13,12,11,10,9,8,7,6,5,4,3,2,1,0 };

static void vparms(long *vwin, float *inbuf, float *lpbuf, long half,
                   float *dither, long *mintau, long *zc, long *lbe,
                   long *fbe, float *qs, float *rc1, float *ar_b, float *ar_f)
{
    /* Local variables */
    float temp;
    long vlen, stop, i;
    float e_pre;
    long start;
    float ap_rms, e_0, oldsgn, lp_rms, e_b, e_f, r_b, r_f, e0ap;
    
    /* Parameter adjustments */
    lpbuf -= 25;
    inbuf -= 181;
    
    /* Function Body */
    lp_rms = 0.f;
    ap_rms = 0.f;
    e_pre = 0.f;
    e0ap = 0.f;
    *rc1 = 0.f;
    e_0 = 0.f;
    e_b = 0.f;
    e_f = 0.f;
    r_f = 0.f;
    r_b = 0.f;
    *zc = 0;
    vlen = vwin[1] - vwin[0] + 1;
    start = vwin[0] + (half - 1) * vlen / 2 + 1;
    stop = start + vlen / 2 - 1;
    
    oldsgn = ((inbuf[start - 1] - *dither)<0.0f)?-1.0f:1.0f;
    for (i = start; i <= stop; ++i) {
        if(lpbuf[i] < 0.0f)
            lp_rms -= lpbuf[i];
        else
            lp_rms += lpbuf[i];
        if(inbuf[i] < 0.0f)
            ap_rms -= inbuf[i];
        else
            ap_rms += inbuf[i];
        temp = inbuf[i] - inbuf[i - 1];
        if(temp < 0.0f)
            e_pre -= temp;
        else
            e_pre += temp;
        e0ap += inbuf[i] * inbuf[i];
        *rc1 += inbuf[i] * inbuf[i - 1];
        e_0 += lpbuf[i] * lpbuf[i];
        e_b += lpbuf[i - *mintau] * lpbuf[i - *mintau];
        e_f += lpbuf[i + *mintau] * lpbuf[i + *mintau];
        r_f += lpbuf[i] * lpbuf[i + *mintau];
        r_b += lpbuf[i] * lpbuf[i - *mintau];
        if ((((inbuf[i] + *dither)<0.0f)?-1.0f:1.0f) != oldsgn) {
            ++(*zc);
            oldsgn = -oldsgn;
        }
        *dither = -(*dither);
    }
    *rc1 /= max(e0ap,1.f);
    *qs = e_pre / max(ap_rms * 2.f, 1.f);
    *ar_b = r_b / max(e_b,1.f) * (r_b / max(e_0,1.f));
    *ar_f = r_f / max(e_f,1.f) * (r_f / max(e_0,1.f));
    *zc = lrintf((float) (*zc << 1) * (90.f / vlen));
    /* Computing MIN */
    *lbe = min(lrintf(lp_rms * 0.25f * (90.f / vlen)),32767);
    /* Computing MIN */
    *fbe = min(lrintf(ap_rms * 0.25f * (90.f / vlen)),32767);
} /* vparms_ */

static void voicin(long *vwin, float *inbuf, float *lpbuf, long half,
            float *minamd, float *maxamd, long *mintau, float *ivrc, long *obound,
            long *voibuf, lpc10_encoder_state *st)
{
    /* Initialized data */
    
    float *dither;
    static float vdc[100]	/* was [10][10] */ = { 0.f,1714.f,-110.f,
        334.f,-4096.f,-654.f,3752.f,3769.f,0.f,1181.f,0.f,874.f,-97.f,
        300.f,-4096.f,-1021.f,2451.f,2527.f,0.f,-500.f,0.f,510.f,-70.f,
        250.f,-4096.f,-1270.f,2194.f,2491.f,0.f,-1500.f,0.f,500.f,-10.f,
        200.f,-4096.f,-1300.f,2e3f,2e3f,0.f,-2e3f,0.f,500.f,0.f,0.f,
        -4096.f,-1300.f,2e3f,2e3f,0.f,-2500.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,
        0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,
        0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,
        0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f };
    static float vdcl[10] = { 600.f,450.f,300.f,200.f,0.f,0.f,0.f,0.f,0.f,0.f };
    
    /* Local variables */
    float ar_b, ar_f;
    long snrl, i;
    float *voice;
    float value[9];
    long zc;
    int ot;
    float qs;
    long vstate;
    float rc1;
    long fbe, lbe;
    float snr2;
    
    
    /*   Declare and initialize filters: */
    
    dither = (&st->dither);
    voice = (&st->voice[0]);
    
    /* Function Body */
    
    /*   Update linear discriminant function history each frame: */
    if (half == 1) {
        voice[0] = voice[2];
        voice[1] = voice[3];
        voice[2] = voice[4];
        voice[3] = voice[5];
        st->maxmin = *maxamd / max(*minamd, 1.f);
    }
    /*   Calculate voicing parameters twice per frame: */
    vparms(vwin, inbuf, lpbuf, half, dither, mintau,
        &zc, &lbe, &fbe, &qs, &rc1, &ar_b, &ar_f);
        /*   Estimate signal-to-noise ratio to select the appropriate VDC vector. */
    st->snr = (float) lrintf((st->snr + st->fbve / (float) max(st->fbue, 1)) * 63 / 64.f);
    snr2 = st->snr * st->fbue / max(st->lbue, 1);
    /*   Quantize SNR to SNRL according to VDCL thresholds. */
    for (snrl = 1; snrl <= 4; ++snrl) {
        if (snr2 > vdcl[snrl - 1]) {
            break;
        }
    }
    /*   Linear discriminant voicing parameters: */
    value[0] = st->maxmin;
    value[1] = (float) lbe / max(st->lbve, 1);
    value[2] = (float) zc;
    value[3] = rc1;
    value[4] = qs;
    value[5] = ivrc[1];
    value[6] = ar_b;
    value[7] = ar_f;
    /*   Evaluation of linear discriminant function: */
    voice[half + 3] = vdc[snrl * 10 - 1];
    for (i = 1; i <= 8; ++i) {
        voice[half + 3] += vdc[i + snrl * 10 - 11] * value[i - 1];
    }
    /*   Classify as voiced if discriminant > 0, otherwise unvoiced */
    /*   Voicing decision for current half-frame:  1 = Voiced; 0 = Unvoiced */
    if (voice[half + 3] > 0.f) {
        voibuf[half + 5] = 1;
    } else {
        voibuf[half + 5] = 0;
    }
    /*   Skip voicing decision smoothing in first half-frame: */
    vstate = -1;
    if (half != 1) {
        /*   Determine if there is an onset transition between P and 1F. */
        /*   OT (Onset Transition) is true if there is an onset between */
        /*   P and 1F but not after 1F. */
        ot = ((obound[0] & 2) != 0 || obound[1] == 1) && (obound[2] & 1) == 0;
        /*   Multi-way dispatch on voicing decision history: */
        vstate = (voibuf[2] << 3) + (voibuf[3] << 2) + (voibuf[4] << 1) + voibuf[5];
        
        switch (vstate + 1) {
        case 1:
            break;
        case 2:
            if (ot && voibuf[6] == 1) {
                voibuf[4] = 1;
            }
            break;
        case 3:
            if (voibuf[6] == 0 || voice[2] < -voice[3]) {
                voibuf[4] = 0;
            } else {
                voibuf[5] = 1;
            }
            break;
        case 4:
            break;
        case 5:
            voibuf[3] = 0;
            break;
        case 6:
            if (voice[1] < -voice[2]) {
                voibuf[3] = 0;
            } else {
                voibuf[4] = 1;
            }
            break;
        case 7:
            if (voibuf[0] == 1 || voibuf[6] == 1 || voice[3] > voice[0]) {
                voibuf[5] = 1;
            } else {
                voibuf[2] = 1;
            }
            break;
        case 8:
            if (ot) {
                voibuf[3] = 0;
            }
            break;
        case 9:
            if (ot) {
                voibuf[3] = 1;
            }
            break;
        case 10:
            break;
        case 11:
            if (voice[2] < -voice[1]) {
                voibuf[4] = 0;
            } else {
                voibuf[3] = 1;
            }
            break;
        case 12:
            voibuf[3] = 1;
            break;
        case 13:
            break;
        case 14:
            if (voibuf[6] == 0 && voice[3] < -voice[2]) {
                voibuf[5] = 0;
            } else {
                voibuf[4] = 1;
            }
            break;
        case 15:
            if (ot && voibuf[6] == 0) {
                voibuf[4] = 0;
            }
            break;
        case 16:
            break;
        }
    }
    /*   Now update parameters: */
    if (voibuf[half + 5] == 0) {
        /* Computing MIN */
        st->sfbue = lrintf((st->sfbue * 63 + (min(fbe, st->ofbue * 3) << 3)) / 64.f);
        st->fbue = st->sfbue / 8;
        st->ofbue = fbe;
        /* Computing MIN */
        st->slbue = lrintf((st->slbue * 63 + (min(lbe, st->olbue * 3) << 3)) / 64.f);
        st->lbue = st->slbue / 8;
        st->olbue = lbe;
    } else {
        st->lbve = lrintf((st->lbve * 63 + lbe) / 64.f);
        st->fbve = lrintf((st->fbve * 63 + fbe) / 64.f);
    }
    /*   Set dither threshold to yield proper zero crossing rates in the */
    /*   presence of low frequency noise and low level signal input. */
    *dither = min(max(((float)sqrt((float) (st->lbue * st->lbve)) * 64 / 3000), 1.f),20.f);
} /* voicin_ */

static void difmag(float *speech, long *tau, 
            long ltau, long maxlag, float *amdf,
            long *minptr, long *maxptr)
{
    /* Local variables */
    long i, j, n1, n2;
    long lmin, lmax;
    float sum;
    
    /* Function Body */
    lmin = 0;
    lmax = 0;

    for (i = 0; i < ltau; ++i) {
        long t = tau[i];

        n1 = (maxlag - t) / 2;
        n2 = n1 + 156;
        sum = 0.f;
        t += n1;
        for (j = n1; j < n2; j += 4, t += 4) {
            float temp = speech[j] - speech[t];

            if(temp < 0.0f)
            {
                sum -= temp;
            }
            else
            {
                sum += temp;
            }
        }
        if (sum < amdf[lmin]) {
            lmin = i;
        } else if (sum > amdf[lmax]) {
            lmax = i;
        }
        amdf[i] = sum;
    }
    *minptr = lmin + 1;
    *maxptr = lmax + 1;
} /* difmag_ */

static void tbdm(float *speech, long *tau, float *amdf, long *minptr, long *maxptr, long *mintau)
{
    /* Local variables */
    float amdf2[6];
    long minp2, ltau2, maxp2, i, j;
    long minamd, ptr, tau2[6];
    
    /*   Compute full AMDF using log spaced lags, find coarse minimum */
    /* Parameter adjustments */
    --amdf;
    --tau;
    
    /* Function Body */
    difmag(speech, &tau[1], 60, tau[60], &amdf[1], minptr, maxptr);
    *mintau = tau[*minptr];
    minamd = (long)amdf[*minptr];
    /*   Build table containing all lags within +/- 3 of the AMDF minimum */
    /*    excluding all that have already been computed */
    ltau2 = 0;
    ptr = *minptr - 2;
    /* Computing MIN */
    j = min(*mintau + 3, tau[60] - 1);
    /* Computing MAX */
    i = max(*mintau - 3, 41);
    for (; i <= j; ++i) {
        while(tau[ptr] < i) {
            ++ptr;
        }
        if (tau[ptr] != i) {
            ++ltau2;
            tau2[ltau2 - 1] = i;
        }
    }
    /*   Compute AMDF of the new lags, if there are any, and choose one */
    /*    if it is better than the coarse minimum */
    if (ltau2 > 0) {
        difmag(speech, tau2, ltau2, tau[60], amdf2, &minp2, &maxp2);
        if (amdf2[minp2 - 1] < (float) minamd) {
            *mintau = tau2[minp2 - 1];
            minamd = (long)amdf2[minp2 - 1];
        }
    }
    /*   Check one octave up, if there are any lags not yet computed */
    if (*mintau >= 80) {
        i = *mintau / 2;
        if ((i & 1) == 0) {
            ltau2 = 2;
            tau2[0] = i - 1;
            tau2[1] = i + 1;
        } else {
            ltau2 = 1;
            tau2[0] = i;
        }
        difmag(speech, tau2, ltau2, tau[60], amdf2, &minp2, &maxp2);
        if (amdf2[minp2 - 1] < (float) minamd) {
            *mintau = tau2[minp2 - 1];
            minamd = (long)amdf2[minp2 - 1];
            *minptr += -20;
        }
    }
    /*   Force minimum of the AMDF array to the high resolution minimum */
    amdf[*minptr] = (float) minamd;
    /*   Find maximum of AMDF within 1/2 octave of minimum */
    /* Computing MAX */
    *maxptr = max(*minptr - 5,1);
    /* Computing MIN */
    j = min(*minptr + 5, 60);
    for (i = *maxptr + 1; i <= j; ++i) {
        if (amdf[i] > amdf[*maxptr]) {
            *maxptr = i;
        }
    }
} /* tbdm_ */

static void placev(long *osbuf, long osptr, long *obound, long *vwin)
{
    /* Local variables */
    int crit;
    long i, q, osptr1, hrange, lrange;
    
    /* Compute the placement range */
    /* Parameter adjustments */
    --osbuf;
    
    /* Function Body */
    /* Computing MAX */
    lrange = max(vwin[3] + 1, LPC10_SAMPLES_PER_FRAME + 1);
    hrange = 3 * LPC10_SAMPLES_PER_FRAME;
    /* Compute OSPTR1, so the following code only looks at relevant onsets. */
    for (osptr1 = osptr; osptr1 >= 1; --osptr1) {
        if (osbuf[osptr1] <= hrange) {
            break;
        }
    }
    ++osptr1;
    /* Check for case 1 first (fast case): */
    if (osptr1 <= 1 || osbuf[osptr1 - 1] < lrange) {
        /* Computing MAX */
        vwin[4] = max(vwin[3] + 1,307);
        vwin[5] = vwin[4] + 156 - 1;
        *obound = 0;
    } else {
        /* Search backward in OSBUF for first onset in range. */
        /* This code relies on the above check being performed first. */
        for (q = osptr1 - 1; q >= 1; --q) {
            if (osbuf[q] < lrange) {
                break;
            }
        }
        ++q;
        /* Check for case 2 (placement before onset): */
        /* Check for critical region exception: */
        crit = FALSE;
        for (i = q + 1; i <= (osptr1 - 1); ++i) {
            if (osbuf[i] - osbuf[q] >= 90) {
                crit = TRUE;
                break;
            }
        }
        /* Computing MAX */
        if (! crit && osbuf[q] > max(2 * LPC10_SAMPLES_PER_FRAME, lrange + 90 - 1)) {
            vwin[5] = osbuf[q] - 1;
            /* Computing MAX */
            vwin[4] = max(lrange, vwin[5] - 156 + 1);
            *obound = 2;
            /* Case 3 (placement after onset) */
        } else {
            vwin[4] = osbuf[q];
L110:
            ++q;
            if (q >= osptr1) {
                goto L120;
            }
            if (osbuf[q] > vwin[4] + 156) {
                goto L120;
            }
            if (osbuf[q] < vwin[4] + 90) {
                goto L110;
            }
            vwin[5] = osbuf[q] - 1;
            *obound = 3;
            return;
L120:
            /* Computing MIN */
            vwin[5] = min(vwin[4] + 156 - 1,hrange);
            *obound = 1;
        }
    }
} /* placev_ */

static void placea(long ipitch, long *voibuf, long obound,
             long *vwin, long *awin, long *ewin)
{
    /* Local variables */
    int allv, winv;
    long i, j, k, l, hrange;
    int ephase;
    long lrange;

    /* Function Body */
    lrange = (3 - 2) * LPC10_SAMPLES_PER_FRAME + 1;
    hrange = 3 * LPC10_SAMPLES_PER_FRAME;
    allv = voibuf[3] == 1;
    allv = allv && voibuf[4] == 1;
    allv = allv && voibuf[5] == 1;
    allv = allv && voibuf[6] == 1;
    allv = allv && voibuf[7] == 1;
    winv = voibuf[6] == 1 || voibuf[7] == 1;
    if ((allv || winv) && (obound == 0)) {
/* APHASE:  Phase synchronous window placement. */
/* Get minimum lower index of the window. */
	i = (lrange + ipitch - 1 - awin[2]) / ipitch;
	i *= ipitch;
	i += awin[2];
/* L = the actual length of this frame's analysis window. */
	l = 156;
/* Calculate the location where a perfectly centered window would star
t. */
	k = (vwin[4] + vwin[5] + 1 - l) / 2;
/* Choose the actual location to be the pitch multiple closest to this
. */
	awin[4] = i + lrintf((float) (k - i) / ipitch) * ipitch;
	awin[5] = awin[4] + l - 1;
	if (obound >= 2 && awin[5] > vwin[5]) {
	    awin[4] -= ipitch;
	    awin[5] -= ipitch;
	}
/* Similarly for the left of the voicing window. */
	if ((obound == 1 || obound == 3) && awin[4] < vwin[4]) {
	    awin[4] += ipitch;
	    awin[5] += ipitch;
	}
/* If this placement puts the analysis window above HRANGE, then */
/* move it backward an integer number of pitch periods. */
	while(awin[5] > hrange) {
	    awin[4] -= ipitch;
	    awin[5] -= ipitch;
	}
/* Similarly if the placement puts the analysis window below LRANGE. 
*/
	while(awin[4] < lrange) {
	    awin[4] += ipitch;
	    awin[5] += ipitch;
	}
/* Make Energy window be phase-synchronous. */
	ephase = TRUE;
/* Case 3 */
    } else {
	awin[4] = vwin[4];
	awin[5] = vwin[5];
	ephase = FALSE;
    }

    j = (awin[5] - awin[4] + 1) / ipitch * ipitch;
    if (j == 0 || ! winv) {
	ewin[4] = vwin[4];
	ewin[5] = vwin[5];
    } else if (! ephase && obound == 2) {
	ewin[4] = awin[5] - j + 1;
	ewin[5] = awin[5];
    } else {
	ewin[4] = awin[4];
	ewin[5] = awin[4] + j - 1;
    }
} /* placea_ */

static void mload(long awinf, float *speech, float *phi, float *psi)
{
    long c, i, r;
    
    /* Function Body */
    for (r = 0; r < 10; ++r) {
        phi[r] = 0.f;
        for (i = 10; i < awinf; ++i) {
            phi[r] += speech[i - 1] * speech[i - r - 1];
        }
    }
    /*   Load last element of vector PSI */
    psi[9] = 0.f;
    for (i = 10; i < awinf; ++i) {
        psi[9] += speech[i] * speech[i - 10];
    }
    /*   End correct to get additional columns of PHI */
    for (r = 1; r < 10; ++r) {
        for (c = 1; c <= r; ++c) {
            phi[r + (c) * 10] = phi[r + (c - 1) * 10 - 1] - 
                speech[awinf - r - 1] * speech[awinf - c - 1] + 
                speech[10 - r - 1] * speech[10 - c - 1];
        }
    }
    /*   End correct to get additional elements of PSI */
    for (c = 0; c < 9; ++c) {
        psi[c] = phi[c + 1] - speech[10] * speech[10 - 2 - c]
                + speech[awinf - 1] * speech[awinf - c - 2];
    }
} /* mload_ */

static void rcchk(float *rc1f, float *rc2f)
{
    /* Local variables */
    long i;
    
    /* Function Body */
    for (i = 0; i < 10; ++i) {
        if ((fabs(rc2f[i])) > .99f) {
            goto L10;
        }
    }
    return;
L10:
    for (i = 0; i < 10; ++i) {
        rc2f[i] = rc1f[i];
    }
} /* rcchk_ */

static void dcbias(long len, float *speech, float *sigout)
{
    /* Local variables */
    float bias;
    long i;

    /* Function Body */
    bias = 0.f;
    for (i = 0; i < len; ++i) {
        bias += speech[i];
    }
    bias /= len;
    for (i = 0; i < len; ++i) {
        *sigout++ = *speech++ - bias;
    }
} /* dcbias_ */

static void preemp(float *inbuf, float *pebuf, long nsamp, float *z)
{
    /* Local variables */
    float temp;
    long i;

    /* Function Body */
    for (i = 0; i< nsamp; ++i) {
	    temp = *inbuf - .9375f * *z;
	    *z = *inbuf++;
	    *pebuf++ = temp;
    }
} /* preemp_ */

static void lpfilt(float *inbuf, float *lpbuf, long nsamp)
{
    /* Local variables */
    long j;
    
    /* Function Body */
    lpbuf = &lpbuf[312 - nsamp];
    for (j = 312 - nsamp; j < 312; ++j) {
        *lpbuf++ = (inbuf[j] + inbuf[j - 30]) * -.0097201988f
        + (inbuf[j - 1] + inbuf[j - 29]) * -.0105179986f
        + (inbuf[j - 2] + inbuf[j - 28]) * -.0083479648f
        + (inbuf[j - 3] + inbuf[j - 27]) * 5.860774e-4f
        + (inbuf[j - 4] + inbuf[j - 26]) * .0130892089f
        + (inbuf[j - 5] + inbuf[j - 25]) * .0217052232f
        + (inbuf[j - 6] + inbuf[j - 24]) * .0184161253f
        + (inbuf[j - 7] + inbuf[j - 23]) * 3.39723e-4f
        + (inbuf[j - 8] + inbuf[j - 22]) * -.0260797087f
        + (inbuf[j - 9] + inbuf[j - 21]) * -.0455563702f
        + (inbuf[j - 10] + inbuf[j - 20]) * -.040306855f
        + (inbuf[j - 11] + inbuf[j - 19]) * 5.029835e-4f
        + (inbuf[j - 12] + inbuf[j - 18]) * .0729262903f
        + (inbuf[j - 13] + inbuf[j - 17]) * .1572008878f
        + (inbuf[j - 14] + inbuf[j - 16]) * .2247288674f
        + inbuf[j - 15] * .250535965f;
    }
} /* lpfilt_ */

static void ivfilt(float *lpbuf, float *ivbuf, long nsamp, float *ivrc)
{
    /* Local variables */
    long i, j, k;
    float r[3], pc1, pc2;

    /* Function Body */
    for (i = 0; i < 3; ++i) {
        r[i] = 0.f;
        k = (i) << 2;
        for (j = ((i + 1) << 2) + 312 - nsamp - 1; j < 312; j += 2) {
            r[i] += lpbuf[j] * lpbuf[j - k];
        }
    }
    /*  Calculate predictor coefficients */
    pc1 = 0.f;
    pc2 = 0.f;
    ivrc[0] = 0.f;
    ivrc[1] = 0.f;
    if (r[0] > 1e-10f) {
        ivrc[0] = r[1] / r[0];
        ivrc[1] = (r[2] - ivrc[0] * r[1]) / (r[0] - ivrc[0] * r[1]);
        pc1 = ivrc[0] - ivrc[0] * ivrc[1];
        pc2 = ivrc[1];
    }
    /*  Inverse filter LPBUF into IVBUF */
    for (i = 312 - nsamp; i < 312; ++i) {
        ivbuf[i] = lpbuf[i] - pc1 * lpbuf[i - 4] - pc2 * lpbuf[i - 8];
    }
} /* ivfilt_ */

static void invert(float *phi, float *psi, float *rc)
{
    /* Local variables */
    float save;
    long i, j, k;
    float v[100]	/* was [10][10] */;

    
    /* Function Body */
    for (j = 0; j < 10; ++j) {
        for (i = j; i < 10; ++i) {
            v[i + j * 10] = phi[i + j * 10];
        }
        for (k = 0; k < j; ++k) {
            save = v[j + k * 10] * v[k + k * 10];
            for (i = j; i < 10; ++i) {
                v[i + j * 10] -= v[i + k * 10] * save;
            }
        }
        /*  Compute intermediate results, which are similar to RC's */
        if ((fabs(v[j + j * 10])) < 1e-10f) {
            goto L100;
        }
        rc[j] = psi[j];
        for (k = 0; k < j; ++k) {
            rc[j] -= rc[k] * v[j + k * 10];
        }
        v[j + j * 10] = 1.f / v[j + j * 10];
        rc[j] *= v[j + j * 10];

        rc[j] = max(min(rc[j],.999f),-.999f);
    }
    return;
    /*  Zero out higher order RC's if algorithm terminated early */
L100:
    for (i = j; i < 10; ++i) {
        rc[i] = 0.f;
    }
} /* invert_ */

static void energy(long len, float *speech, float *rms)
{
    /* Local variables */
    long i;

    /* Function Body */
    *rms = 0.f;
    for (i = 0; i < len; ++i) {
	    *rms += speech[i] * speech[i];
    }
    *rms = (float)sqrt(*rms / len);
} /* energy_ */

static void dyptrk(float *amdf, long *minptr, long *voice, long *pitch,
            long *midx, lpc10_encoder_state *st)
{
    /* Initialized data */
    
    float *s;
    long *p;
    long *ipoint;
    float *alphax;
    
    
    /* Local variables */
    long pbar;
    float sbar;
    long path[2], iptr, i, j;
    float alpha, minsc, maxsc;
    
    s = &(st->s[0]);
    p = &(st->p[0]);
    ipoint = &(st->ipoint);
    alphax = &(st->alphax);
    
    
    /* Parameter adjustments */
    if (amdf) {
        --amdf;
    }
    
    /* Function Body */
    
    if (*voice == 1) {
        *alphax = *alphax * .75f + amdf[*minptr] / 2.f;
    } else {
        *alphax *= .984375f;
    }
    alpha = *alphax / 16;
    if (*voice == 0 && *alphax < 128.f) {
        alpha = 8.f;
    }
    /* SEESAW: Construct a pitch pointer array and intermediate winner function*/
    /*   Left to right pass: */
    iptr = *ipoint + 1;
    p[iptr * 60 - 60] = 1;
    i = 1;
    pbar = 1;
    sbar = s[0];
    for (i = 1; i <= 60; ++i) {
        sbar += alpha;
        if (sbar < s[i - 1]) {
            s[i - 1] = sbar;
            p[i + iptr * 60 - 61] = pbar;
        } else {
            sbar = s[i - 1];
            p[i + iptr * 60 - 61] = i;
            pbar = i;
        }
    }
    /*   Right to left pass: */
    i = pbar - 1;
    sbar = s[i];
    while(i >= 1) {
        sbar += alpha;
        if (sbar < s[i - 1]) {
            s[i - 1] = sbar;
            p[i + iptr * 60 - 61] = pbar;
        } else {
            pbar = p[i + iptr * 60 - 61];
            i = pbar;
            sbar = s[i - 1];
        }
        --i;
    }
    /*   Update S using AMDF */
    /*   Find maximum, minimum, and location of minimum */
    s[0] += amdf[1] / 2;
    minsc = s[0];
    maxsc = minsc;
    *midx = 1;
    for (i = 2; i <= 60; ++i) {
        s[i - 1] += amdf[i] / 2;
        if (s[i - 1] > maxsc) {
            maxsc = s[i - 1];
        }
        if (s[i - 1] < minsc) {
            *midx = i;
            minsc = s[i - 1];
        }
    }
    /*   Subtract MINSC from S to prevent overflow */
    for (i = 1; i <= 60; ++i) {
        s[i - 1] -= minsc;
    }
    maxsc -= minsc;
    /*   Use higher octave pitch if significant null there */
    j = 0;
    for (i = 20; i <= 40; i += 10) {
        if (*midx > i) {
            if (s[*midx - i - 1] < maxsc / 4) {
                j = i;
            }
        }
    }
    *midx -= j;
    /*   TRACE: look back two frames to find minimum cost pitch estimate */
    j = *ipoint;
    *pitch = *midx;
    for (i = 1; i <= 2; ++i) {
        j = j % 2 + 1;
        *pitch = p[*pitch + j * 60 - 61];
        path[i - 1] = *pitch;
    }
    
    *ipoint = (*ipoint + 1) % 2;
} /* dyptrk_ */

static void onset(float *pebuf, long *osbuf, long *osptr, lpc10_encoder_state *st)
{
    /* Initialized data */
    
    float n;
    float d;
    float *l2buf;
    float *l2sum1;
    long *l2ptr1;
    long *l2ptr2;
    int *hyst;
    
    /* System generated locals */
    float temp;
    
    /* Local variables */
    long i;
    long *lasti;
    float l2sum2;
    float *fpc;


    n = st->n;
    d = st->d;
    fpc = &(st->fpc);
    l2buf = &(st->l2buf[0]);
    l2sum1 = &(st->l2sum1);
    l2ptr1 = &(st->l2ptr1);
    l2ptr2 = &(st->l2ptr2);
    lasti = &(st->lasti);
    hyst = &(st->hyst);
    
    /* Parameter adjustments */
    if (pebuf) {
        pebuf -= 181;
    }
    
    /* Function Body */
    
    if (*hyst) {
        *lasti -= LPC10_SAMPLES_PER_FRAME;
    }
    for (i = 720 - LPC10_SAMPLES_PER_FRAME + 1; i <= 720; ++i) {
    /*   Compute FPC; Use old FPC on divide by zero; Clamp FPC to +/- 1. */
        n = (pebuf[i] * pebuf[i - 1] + n * 63.f) * 0.015625f;
        /* Computing 2nd power */
        temp = pebuf[i - 1];
        d = (temp * temp + d * 63.f) * 0.015625f;
        if (d != 0.f) {
            if(n > d || n < -(d)){
                *fpc = (n<0.0f)?-1.0f:1.0f;
            } else {
                *fpc = n / d;
            }
        }
        
        l2sum2 = l2buf[*l2ptr1 - 1];
        *l2sum1 = *l2sum1 - l2buf[*l2ptr2 - 1] + *fpc;
        l2buf[*l2ptr2 - 1] = *l2sum1;
        l2buf[*l2ptr1 - 1] = *fpc;
        *l2ptr1 = *l2ptr1 % 16 + 1;
        *l2ptr2 = *l2ptr2 % 16 + 1;
        temp = *l2sum1 - l2sum2;
        if (temp > 1.7f || temp < -1.7f) {
            if (! (*hyst)) {
                /*   Ignore if buffer full */
                if (*osptr < 10) {
                    osbuf[*osptr] = i - 9;
                    ++(*osptr);
                }
                *hyst = TRUE;
            }
            *lasti = i;
            /*       After one onset detection, at least OSHYST sample times must go */
            /*       by before another is allowed to occur. */
        } else if ((*hyst) && i - *lasti >= 10) {
            *hyst = FALSE;
        }
    }
    st->n = n;
    st->d = d;
} /* onset_ */

static void analys(float *speech, long *voice, long 
            *pitch, float *rms, float *rc, lpc10_encoder_state *st)
{
    /* Initialized data */
    
    static long tau[60] = { 20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,
        35,36,37,38,39,40,42,44,46,48,50,52,54,56,58,60,62,64,66,68,70,72,
        74,76,78,80,84,88,92,96,100,104,108,112,116,120,124,128,132,136,
        140,144,148,152,156 };
    
    /* System generated locals */
    long i1;
    
    /* Local variables */
    float amdf[60];
    long half;
    float abuf[156];
    float bias;
    long *awin;
    long midx, ewin[6]	/* was [2][3] */;
    float ivrc[2], temp;
    long *vwin;
    long i, j, lanal;
    float *inbuf, *pebuf;
    float *lpbuf, *ivbuf;
    float *rcbuf;
    long *osptr;
    long *osbuf;
    long ipitch;
    long *obound;
    long *voibuf;
    long mintau;
    float *rmsbuf;
    long minptr, maxptr;
    float phi[100]	/* was [10][10] */, psi[10];
    
    inbuf = &(st->inbuf[0]);
    pebuf = &(st->pebuf[0]);
    lpbuf = &(st->lpbuf[0]);
    ivbuf = &(st->ivbuf[0]);
    bias = st->bias;
    osbuf = &(st->osbuf[0]);
    osptr = &(st->osptr);
    obound = &(st->obound[0]);
    vwin = &(st->vwin[0]);
    awin = &(st->awin[0]);
    voibuf = &(st->voibuf[0]);
    rmsbuf = &(st->rmsbuf[0]);
    rcbuf = &(st->rcbuf[0]);
    
    i1 = 720 - LPC10_SAMPLES_PER_FRAME;
    for (i = LPC10_SAMPLES_PER_FRAME; i < i1; ++i) {
        inbuf[i - LPC10_SAMPLES_PER_FRAME] = inbuf[i];
        pebuf[i - LPC10_SAMPLES_PER_FRAME] = pebuf[i];
    }
    i1 = 540 - LPC10_SAMPLES_PER_FRAME - 229;
    for (i = 0; i <= i1; ++i) {
        ivbuf[i] = ivbuf[LPC10_SAMPLES_PER_FRAME + i];
    }
    i1 = 720 - LPC10_SAMPLES_PER_FRAME - 25;
    for (i = 0; i <= i1; ++i) {
        lpbuf[i] = lpbuf[LPC10_SAMPLES_PER_FRAME + i];
    }
    j = 0;
    for (i = 0; i < *osptr; ++i) {
        if (osbuf[i] > LPC10_SAMPLES_PER_FRAME) {
            osbuf[j] = osbuf[i] - LPC10_SAMPLES_PER_FRAME;
            ++j;
        }
    }
    *osptr = j;
    voibuf[0] = voibuf[2];
    voibuf[1] = voibuf[3];
    for (i = 1; i <= 2; ++i) {
        vwin[(i << 1) - 2] = vwin[((i + 1) << 1) - 2] - LPC10_SAMPLES_PER_FRAME;
        vwin[(i << 1) - 1] = vwin[((i + 1) << 1) - 1] - LPC10_SAMPLES_PER_FRAME;
        awin[(i << 1) - 2] = awin[((i + 1) << 1) - 2] - LPC10_SAMPLES_PER_FRAME;
        awin[(i << 1) - 1] = awin[((i + 1) << 1) - 1] - LPC10_SAMPLES_PER_FRAME;
        obound[i - 1] = obound[i];
        voibuf[i << 1] = voibuf[(i + 1) << 1];
        voibuf[(i << 1) + 1] = voibuf[((i + 1) << 1) + 1];
        rmsbuf[i - 1] = rmsbuf[i];
        for (j = 1; j <= 10; ++j) {
            rcbuf[j + i * 10 - 11] = rcbuf[j + (i + 1) * 10 - 11];
        }
    }
    temp = 0.f;
    for (i = 0; i < LPC10_SAMPLES_PER_FRAME; ++i) {
        inbuf[720 - LPC10_SAMPLES_PER_FRAME + i - 180] = speech[i] * 4096.f - bias;
        temp += inbuf[720 - LPC10_SAMPLES_PER_FRAME + i - 180];
    }
    if (temp > (float) LPC10_SAMPLES_PER_FRAME) {
        st->bias += 1;
    }
    if (temp < (float) (-LPC10_SAMPLES_PER_FRAME)) {
        st->bias += -1;
    }
    /*   Place Voicing Window */
    i = 720 - LPC10_SAMPLES_PER_FRAME;
    preemp(&inbuf[i - 180], &pebuf[i - 180], LPC10_SAMPLES_PER_FRAME, &(st->zpre));
    onset(pebuf, osbuf, osptr, st);
    
    placev(osbuf, *osptr, &obound[2], vwin);
    lpfilt(&inbuf[228], &lpbuf[384], LPC10_SAMPLES_PER_FRAME);
    ivfilt(&lpbuf[204], ivbuf, LPC10_SAMPLES_PER_FRAME, ivrc);
    tbdm(ivbuf, tau, amdf, &minptr, &maxptr, &mintau);
    /*   voicing decisions. */
    for (half = 1; half <= 2; ++half) {
        voicin(&vwin[4], inbuf, lpbuf, half, &amdf[minptr - 1],
                &amdf[maxptr - 1], &mintau, ivrc, obound, voibuf, st);
    }
    dyptrk(amdf, &minptr, &voibuf[7], pitch, &midx, st);
    ipitch = tau[midx - 1];
    placea(ipitch, voibuf, obound[2], vwin, awin, ewin);
    lanal = awin[5] + 1 - awin[4];
    dcbias(lanal, &pebuf[awin[4] - 181], abuf);
    i1 = ewin[5] - ewin[4] + 1;
    energy(i1, &abuf[ewin[4] - awin[4]], &rmsbuf[2]);
    /*   Matrix load and invert, check RC's for stability */
    mload(lanal, abuf, phi, psi);
    invert(phi, psi, &rcbuf[20]);
    rcchk(&rcbuf[10], &rcbuf[20]);
    /*   Set return parameters */
    voice[0] = voibuf[2];
    voice[1] = voibuf[3];
    *rms = rmsbuf[0];
    for (i = 0; i < 10; ++i) {
        rc[i] = rcbuf[i];
    }
} /* analys_ */

static void encode(long *voice, long pitch, float rms, float *rc, long *ipitch,
            long *irms, long *irc)
{
    /* Local variables */
    long idel, nbit, i, j, i2, i3, mrk;
    
    /* Function Body */
    /*  Scale RMS and RC's to integers */
    *irms = lrintf(rms);
    for (i = 0; i < 10; ++i) {
        irc[i] = lrintf(rc[i] * 32768.f);
    }
    /*  Encode pitch and voicing */
    if (voice[0] != 0 && voice[1] != 0) {
        *ipitch = entau[pitch - 1];
    } else {
        *ipitch = (voice[0] << 1) + voice[1];
    }
    /*  Encode RMS by binary table search */
    j = 32;
    idel = 16;
    *irms = min(*irms,1023);
    while(idel > 0) {
        if (*irms > rmst[j - 1]) {
            j -= idel;
        }
        if (*irms < rmst[j - 1]) {
            j += idel;
        }
        idel /= 2;
    }
    if (*irms > rmst[j - 1]) {
        --j;
    }
    *irms = 31 - j / 2;
    /*  Encode RC(1) and (2) as log-area-ratios */
    for (i = 0; i < 2; ++i) {
        i2 = irc[i];
        mrk = 0;
        if (i2 < 0) {
            i2 = -i2;
            mrk = 1;
        }
        i2 /= 512;
        i2 = min(i2,63);
        i2 = entab6[i2];
        if (mrk != 0) {
            i2 = -i2;
        }
        irc[i] = i2;
    }
    /*  Encode RC(3) - (10) linearly, remove bias then scale */
    for (i = 2; i < 10; ++i) {
        i2 = irc[i] / 2;
        i2 = lrintf((i2 + enadd[10 + 1 - i - 2]) * enscl[10 + 1 - i - 2]);
        /* Computing MIN */
        i2 = min(max(i2,-127),127);
        nbit = enbits[10 + 1 - i - 2];
        i3 = 0;
        if (i2 < 0) {
            i3 = -1;
        }
        i2 = i2 / (2 << (nbit-1));
        if (i3 == -1) {
            --i2;
        }
        irc[i] = i2;
    }
/*    printf("%d,\t%d,\t%d,\t%d,\t%d,\t%d,\t%d,\t%d,\t%d,\t%d,\t%d,\t%d\n",irc[0],irc[1],irc[2],irc[3],irc[4],irc[5]
        ,irc[6],irc[7],irc[8],irc[9], *ipitch, *irms);
*/
} /* encode_ */

static void hp100(float *speech, long end, lpc10_encoder_state *st)
{
    float z11;
    float z21;
    float z12;
    float z22;

    /* Local variables */
    long i;
    float si, err;

    /* Function Body */

    z11 = st->z11;
    z21 = st->z21;
    z12 = st->z12;
    z22 = st->z22;

    for (i = 0; i<end; ++i) {
	    err = *speech + z11 * 1.859076f - z21 * .8648249f;
	    si = err - z11 * 2.f + z21;
	    z21 = z11;
	    z11 = err;
	    err = si + z12 * 1.935715f - z22 * .9417004f;
	    si = err - z12 * 2.f + z22;
	    z22 = z12;
	    z12 = err;
	    *speech++ = si * .902428f;
    }

    st->z11 = z11;
    st->z21 = z21;
    st->z12 = z12;
    st->z22 = z22;

} /* hp100_ */

static void pack(long value, long bits, long *array, long *pointer)
{
    int i;
    
    for (i = 0; i < bits; (*pointer)++, i++)
        array[*pointer] = (long)((value & 1 << i) >> i);
}

static long chanwr(long ipitv, long irms, long *irc, long *ibits, int vbr)
{
    long i;
    long pointer;

    pointer = 0;

    if(vbr == TRUE)
    {
        /* test for silence */
        if(irms < 10) {
            pack(127, 7, ibits, &pointer);
            return 1;
        }
    }

    pack(ipitv, 7, ibits, &pointer);
    pack(irms, 5, ibits, &pointer);

    for (i = 0; i < 4; ++i) {
        pack(irc[i], lpcbits[i], ibits, &pointer);
    }
    /* test for unvoiced */
    if((ipitv != 0 && ipitv != 126) || vbr == FALSE) {
        for (i = 4; i < 10; ++i) {
            pack(irc[i], lpcbits[i], ibits, &pointer);
        }
        return 7;
    }
    return 4;
}

int lpc10_encode_int(short *in, unsigned char *out, lpc10_encoder_state *st, int vbr)
{
    float   speech[LPC10_SAMPLES_PER_FRAME];
    float   *sp = speech;
    long bits[LPC10_BITS_IN_COMPRESSED_FRAME];
    int     i;
    long irms, voice[2], pitch, ipitv;
    float rc[10];
    long irc[10];
    float rms;
    long framesize;

    /* convert sound from short to float */
    for(i=LPC10_SAMPLES_PER_FRAME;i--;)
    {
        *sp++ = (float)(*in++ / 32768.0f);
    }

    /* encode it */
    memset(bits, 0, sizeof(bits));
    hp100(speech, LPC10_SAMPLES_PER_FRAME, st);
    analys(speech, voice, &pitch, &rms, rc, st);
    encode(voice, pitch, rms, rc, &ipitv, &irms, irc);
    framesize = chanwr(ipitv, irms, irc, bits, vbr);

    /* pack the bits */
    memset(out, 0, 7);
	for (i = 0; i < 54; i++)
    {
        out[i >> 3] |= ((bits[i] != 0)? 1 : 0) << (i & 7);
	}

    /* return number of bytes encoded*/
    return framesize;
}

int lpc10_encode(short *in, unsigned char *out, lpc10_encoder_state *st)
{
    return lpc10_encode_int(in, out, st, FALSE);

}

int vbr_lpc10_encode(short *in, unsigned char *out, lpc10_encoder_state *st)
{
    return lpc10_encode_int(in, out, st, TRUE);

}

/* Allocate memory for, and initialize, the state that needs to be
   kept from encoding one frame to the next for a single
   LPC-10-compressed audio stream.  Return 0 if malloc fails,
   otherwise return pointer to new structure. */

lpc10_encoder_state *create_lpc10_encoder_state(void)
{
    lpc10_encoder_state *st;

    st = (lpc10_encoder_state *)malloc((unsigned) sizeof (lpc10_encoder_state));
    return (st);
}

void init_lpc10_encoder_state(lpc10_encoder_state *st)
{
    int i;

    /* State used only by function hp100 */
    st->z11 = 0.0f;
    st->z21 = 0.0f;
    st->z12 = 0.0f;
    st->z22 = 0.0f;
    
    /* State used by function analys */
    for (i = 0; i < 540; i++) {
	st->inbuf[i] = 0.0f;
	st->pebuf[i] = 0.0f;
    }
    for (i = 0; i < 696; i++) {
	st->lpbuf[i] = 0.0f;
    }
    for (i = 0; i < 312; i++) {
	st->ivbuf[i] = 0.0f;
    }
    st->bias = 0.0f;
    st->osptr = 0;
    for (i = 0; i < 3; i++) {
	st->obound[i] = 0;
    }
    st->vwin[4] = 307;
    st->vwin[5] = 462;
    st->awin[4] = 307;
    st->awin[5] = 462;
    for (i = 0; i < 8; i++) {
	st->voibuf[i] = 0;
    }
    for (i = 0; i < 3; i++) {
	st->rmsbuf[i] = 0.0f;
    }
    for (i = 0; i < 30; i++) {
	st->rcbuf[i] = 0.0f;
    }
    st->zpre = 0.0f;


    /* State used by function onset */
    st->n = 0.0f;
    st->d = 1.0f;
    for (i = 0; i < 16; i++) {
	st->l2buf[i] = 0.0f;
    }
    st->l2sum1 = 0.0f;
    st->l2ptr1 = 1;
    st->l2ptr2 = 9;
    st->hyst = FALSE;

    /* State used by function voicin */
    st->dither = 20.0f;
    st->maxmin = 0.0f;
    for (i = 0; i < 6; i++) {
	st->voice[i] = 0.0f;
    }
    st->lbve = 3000;
    st->fbve = 3000;
    st->fbue = 187;
    st->ofbue = 187;
    st->sfbue = 187;
    st->lbue = 93;
    st->olbue = 93;
    st->slbue = 93;
    st->snr = (float) (st->fbve / st->fbue << 6);

    /* State used by function dyptrk */
    for (i = 0; i < 60; i++) {
	st->s[i] = 0.0f;
    }
    for (i = 0; i < 120; i++) {
	st->p[i] = 0;
    }
    st->ipoint = 0;
    st->alphax = 0.0f;

    /* State used by function chanwr */
    st->isync = 0;
}

/* free the memory */
void destroy_lpc10_encoder_state (lpc10_encoder_state *st)
{
    if(st != NULL)
    {
        free(st);
        st = NULL;
    }
}

