/*
  GSM voice codec, part of the HawkVoice Direct Interface (HVDI)
  cross platform network voice library
  Copyright (C) 2001-2003 Phil Frisbie, Jr. (phil@hawksoft.com)

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

/*
 * Copyright 1992 by Jutta Degener and Carsten Bormann, Technische
 * Universitaet Berlin.  See the accompanying file "COPYRIGHT" for
 * details.  THERE IS ABSOLUTELY NO WARRANTY FOR THIS SOFTWARE.
 */

#include "private.h"

static gsmword gsm_FACd[8]	= {18431, 20479, 22527, 24575, 26623, 28671, 30719, 32767};
static gsmword gsm_QLB[4] = {3277, 11469, 21299, 32767};

INLINE void Postprocessing(struct gsm_state *S, short *s)
{
	int		    k;
	gsmword	    msr = S->msr;
	longword	ltmp;

	for(k = 160; k--; s++)
    {
		msr = *s + (gsmword)GSM_MULT_R( msr, 28672 );  	   /* Deemphasis 	     */
		*s  = (short)(GSM_ADD(msr, msr) & 0xFFF8);  /* Truncation & Upscaling */
	}
	S->msr = msr;
}

static void APCM_quantization_xmaxc_to_exp_mant(gsmword xmaxc, gsmword *exp_out,
                                                gsmword *mant_out)
{
	gsmword	exp, mant;

	exp = 0;
	if(xmaxc > 15)
        exp = (gsmword)(SASR(xmaxc, 3) - 1);
	mant = (gsmword)(xmaxc - SASL( exp, 3 ));

	if(mant == 0)
    {
		exp  = -4;
		mant = 7;
	}
	else
    {
		while(mant <= 7)
        {
			mant = (gsmword)(SASL(mant, 1) | 1);
			exp--;
		}
		mant -= 8;
	}

	*exp_out  = exp;
	*mant_out = mant;
}

static void APCM_inverse_quantization(gsmword *xMc, gsmword mant, gsmword exp, gsmword *xMp)
{
	int	i;
	gsmword	temp, temp1, temp2, temp3;
	longword	ltmp;

	temp1 = gsm_FACd[ mant ];	/* see 4.2-15 for mant */
	temp2 = (gsmword)GSM_SUB( 6, exp );	/* see 4.2-15 for exp  */
	temp3 = (gsmword)SASL( 1, GSM_SUB( temp2, 1 ));

	for(i = 13; i--;)
    {

		temp = (gsmword)(SASL(*xMc++, 1) - 7);	        /* restore sign   */

		temp = (gsmword)SASL(temp, 12);				/* 16 bit signed  */
		temp = (gsmword)GSM_MULT_R( temp1, temp );
		temp = (gsmword)GSM_ADD( temp, temp3 );
		*xMp++ = (gsmword)SASR( temp, temp2 );
	}
}

static void RPE_grid_positioning(gsmword Mc, gsmword *xMp, gsmword *ep)
{
	int	i = 13;

    switch (Mc) {
        case 3: *ep++ = 0;
        case 2:  do
                 {
                             *ep++ = 0;
                  case 1:    *ep++ = 0;
                  case 0:    *ep++ = *xMp++;
                  } while (--i != 0);
    }
    while(++Mc < 4)
        *ep++ = 0;
}

static void Gsm_RPE_Decoding(gsmword xmaxcr, gsmword Mcr, gsmword *xMcr, gsmword *erp)
{
	gsmword	exp, mant;
	gsmword	xMp[ 13 ];

	APCM_quantization_xmaxc_to_exp_mant(xmaxcr, &exp, &mant);
	APCM_inverse_quantization(xMcr, mant, exp, xMp);
	RPE_grid_positioning(Mcr, xMp, erp);
}

static void Decoding_of_the_coded_Log_Area_Ratios(gsmword *LARc, gsmword *LARpp)
{
	gsmword	temp1;
	long	ltmp;

#undef	STEP
#define	STEP( B, MIC, INVA )	\
		temp1    = (gsmword)(SASL( *LARc++ + MIC , 10));	\
		temp1    -= SASL( B, 1 );		\
		temp1    = (gsmword)GSM_MULT_R( INVA, temp1 );		\
		*LARpp++ = (gsmword)GSM_ADD( temp1, temp1 );

	STEP(      0,  -32,  13107 );
	STEP(      0,  -32,  13107 );
	STEP(   2048,  -16,  13107 );
	STEP(  -2560,  -16,  13107 );

	STEP(     94,   -8,  19223 );
	STEP(  -1792,   -8,  17476 );
	STEP(   -341,   -4,  31454 );
	STEP(  -1144,   -4,  29708 );
}

INLINE void Coefficients_0_12(gsmword * LARpp_j_1, gsmword * LARpp_j, gsmword * LARp)
{
	int 	i;

	for(i = 1; i <= 8; i++, LARp++, LARpp_j_1++, LARpp_j++)
    {
		*LARp = (gsmword)(SASR( *LARpp_j_1, 2 ) + SASR( *LARpp_j, 2 ));
		*LARp += (gsmword)SASR( *LARpp_j_1, 1);
	}
}

INLINE void Coefficients_13_26(gsmword * LARpp_j_1, gsmword * LARpp_j, gsmword * LARp)
{
	int i;

	for(i = 1; i <= 8; i++, LARpp_j_1++, LARpp_j++, LARp++)
		*LARp = SASR( *LARpp_j_1, 1) + SASR( *LARpp_j, 1 );
}

INLINE void Coefficients_27_39(gsmword * LARpp_j_1, gsmword * LARpp_j, gsmword * LARp)
{
	int i;

	for(i = 1; i <= 8; i++, LARpp_j_1++, LARpp_j++, LARp++)
    {
		*LARp = (gsmword)(SASR( *LARpp_j_1, 2 ) + SASR( *LARpp_j, 2 ));
		*LARp += (gsmword)SASR( *LARpp_j, 1 );
	}
}

INLINE void Coefficients_40_159(gsmword * LARpp_j, gsmword * LARp)
{
	int i;

	for(i = 1; i <= 8; i++, LARp++, LARpp_j++)
		*LARp = *LARpp_j;
}

static void LARp_to_rp(gsmword * LARp)
{
	int 		i;
	gsmword	temp;

	for (i = 1; i <= 8; i++, LARp++)
    {

		if (*LARp < 0)
        {
			temp = (gsmword)GSM_ABS( *LARp );
			*LARp = (gsmword)(- ((temp < 11059) ? SASL( temp, 1 )
				: ((temp < 20070) ? temp + 11059
				:  ( SASR( temp, 2 ) + 26112 ))));
		}
        else
        {
			temp  = *LARp;
			*LARp =    (gsmword)((temp < 11059) ? SASL( temp, 1 )
				: ((temp < 20070) ? temp + 11059
				:  ( SASR( temp, 2 ) + 26112 )));
		}
	}
}

static void Short_term_synthesis_filtering(struct gsm_state *S, gsmword *rrp, int k,
                                           gsmword *wt, short *sr)
{
	gsmword	    * v = S->v;
	gsmword	    sri;
	longword	ltmp;
    gsmword     v0=v[0], v1=v[1], v2=v[2], v3=v[3],
                v4=v[4], v5=v[5], v6=v[6], v7=v[7];
    gsmword     rrp0, rrp1, rrp2, rrp3,
                rrp4, rrp5, rrp6, rrp7;

    rrp0 = rrp[0]; rrp1 = rrp[1];
    rrp2 = rrp[2]; rrp3 = rrp[3];
    rrp4 = rrp[4]; rrp5 = rrp[5];
    rrp6 = rrp[6]; rrp7 = rrp[7];

    while(k-- != 0)
    {
        sri = *wt++;

        sri  -= (gsmword)GSM_MULT_R(rrp7, v7);
 
        sri  -= (gsmword)GSM_MULT_R(rrp6, v6);
        v7 = v6 + (gsmword)GSM_MULT_R(rrp6, sri);
        
        sri  -= (gsmword)GSM_MULT_R(rrp5, v5);
        v6 = v5 + (gsmword)GSM_MULT_R(rrp5, sri);
        
        sri  -= (gsmword)GSM_MULT_R(rrp4, v4);
        v5 = v4 + (gsmword)GSM_MULT_R(rrp4, sri);
        
        sri  -= (gsmword)GSM_MULT_R(rrp3, v3);
        v4 = v3 + (gsmword)GSM_MULT_R(rrp3, sri);
        
        sri  -= (gsmword)GSM_MULT_R(rrp2, v2);
        v3 = v2 + (gsmword)GSM_MULT_R(rrp2, sri);
        
        sri  -= (gsmword)GSM_MULT_R(rrp1, v1);
        v2 = v1 + (gsmword)GSM_MULT_R(rrp1, sri);
        
        sri  = (gsmword)GSM_SUB( sri, (gsmword)GSM_MULT_R(rrp0, v0) );
        v1 = v0 + (gsmword)GSM_MULT_R(rrp0, sri);

        v0 = sri;
        *sr++ = (short)sri;
    }
    v[0]=GSM_ADD(v0, 0);
    v[1]=GSM_ADD(v1, 0);
    v[2]=GSM_ADD(v2, 0);
    v[3]=GSM_ADD(v3, 0);
    v[4]=GSM_ADD(v4, 0);
    v[5]=GSM_ADD(v5, 0);
    v[6]=GSM_ADD(v6, 0);
    v[7]=GSM_ADD(v7, 0);
}

static void Gsm_Short_Term_Synthesis_Filter(struct gsm_state * S, gsmword *LARcr,
                                            gsmword *wt, short *s)
{
	gsmword		* LARpp_j	= S->LARpp[ S->j     ];
	gsmword		* LARpp_j_1	= S->LARpp[ S->j ^=1 ];
	gsmword		LARp[8];

	Decoding_of_the_coded_Log_Area_Ratios(LARcr, LARpp_j);

	Coefficients_0_12(LARpp_j_1, LARpp_j, LARp);
	LARp_to_rp(LARp);
	Short_term_synthesis_filtering(S, LARp, 13, wt, s);

	Coefficients_13_26(LARpp_j_1, LARpp_j, LARp);
	LARp_to_rp(LARp);
	Short_term_synthesis_filtering(S, LARp, 14, wt + 13, s + 13);

	Coefficients_27_39(LARpp_j_1, LARpp_j, LARp);
	LARp_to_rp(LARp);
	Short_term_synthesis_filtering(S, LARp, 13, wt + 27, s + 27);

	Coefficients_40_159(LARpp_j, LARp);
	LARp_to_rp(LARp);
	Short_term_synthesis_filtering(S, LARp, 120, wt + 40, s + 40);
}

static void Gsm_Long_Term_Synthesis_Filtering(struct gsm_state *S, gsmword Ncr, gsmword bcr,
                                              gsmword *erp, gsmword *drp)
{
	int 		k;
	gsmword		brp, Nr;
    gsmword		* pdrp;

	Nr = (gsmword)(Ncr < 40 || Ncr > 120 ? S->nrp : Ncr);
	S->nrp = Nr;

	brp = gsm_QLB[ bcr ];
    pdrp = drp;
	for(k = 40; k--; pdrp++)
    {
		*pdrp = (gsmword)(*erp++ + GSM_MULT_R( brp, pdrp[ -(Nr) ] ));
	}

    memcpy(&drp[-120], &drp[-80], (sizeof(drp[0]) * 120));
}

static void Gsm_Decoder(struct gsm_state *S, gsmword *LARcr, gsmword *Ncr, gsmword *bcr,
                        gsmword *Mcr, gsmword *xmaxcr, gsmword *xMcr, short *s)
{
	int		    j;
	gsmword		erp[40], wt[160];
	gsmword		* drp = S->dp0 + 120;

	for(j=0; j <= 3; j++, xmaxcr++, bcr++, Ncr++, Mcr++, xMcr += 13)
    {
		Gsm_RPE_Decoding( *xmaxcr, *Mcr, xMcr, erp );
		Gsm_Long_Term_Synthesis_Filtering( S, *Ncr, *bcr, erp, drp );

        memcpy(&wt[ j * 40 ], drp, sizeof(drp[0]) * 40);
	}

	Gsm_Short_Term_Synthesis_Filter(S, LARcr, wt, s);
	Postprocessing(S, s);
}

int gsm_decode ( struct gsm_state *s, unsigned char *c, short *target)
{
	gsmword  	LARc[8], Nc[4], Mc[4], bc[4], xmaxc[4], xmc[13*4];

	{
		if(((*c >> 4) & 0x0F) != GSM_MAGIC)
            return 0;

		LARc[0]  = (gsmword)((*c++ & 0xF) << 2);			/* 1 */
		LARc[0] |= (gsmword)((*c >> 6) & 0x3);
		LARc[1]  = (gsmword)(*c++ & 0x3F);
		LARc[2]  = (gsmword)((*c >> 3) & 0x1F);
		LARc[3]  = (gsmword)((*c++ & 0x7) << 2);
		LARc[3] |= (gsmword)((*c >> 6) & 0x3);
		LARc[4]  = (gsmword)((*c >> 2) & 0xF);
		LARc[5]  = (gsmword)((*c++ & 0x3) << 2);
		LARc[5] |= (gsmword)((*c >> 6) & 0x3);
		LARc[6]  = (gsmword)((*c >> 3) & 0x7);
		LARc[7]  = (gsmword)(*c++ & 0x7);
		Nc[0]  = (gsmword)((*c >> 1) & 0x7F);
		bc[0]  = (gsmword)((*c++ & 0x1) << 1);
		bc[0] |= (gsmword)((*c >> 7) & 0x1);
		Mc[0]  = (gsmword)((*c >> 5) & 0x3);
		xmaxc[0]  = (gsmword)((*c++ & 0x1F) << 1);
		xmaxc[0] |= (gsmword)((*c >> 7) & 0x1);
		xmc[0]	= (gsmword)((*c >> 4) & 0x7);
		xmc[1]	= (gsmword)((*c >> 1) & 0x7);
		xmc[2]	= (gsmword)((*c++ & 0x1) << 2);
		xmc[2] |= (gsmword)((*c >> 6) & 0x3);
		xmc[3]	= (gsmword)((*c >> 3) & 0x7);
		xmc[4]	= (gsmword)(*c++ & 0x7);
		xmc[5]	= (gsmword)((*c >> 5) & 0x7);
		xmc[6]	= (gsmword)((*c >> 2) & 0x7);
		xmc[7]	= (gsmword)((*c++ & 0x3) << 1);			/* 10 */
		xmc[7] |= (gsmword)((*c >> 7) & 0x1);
		xmc[8]	= (gsmword)((*c >> 4) & 0x7);
		xmc[9]	= (gsmword)((*c >> 1) & 0x7);
		xmc[10]  = (gsmword)((*c++ & 0x1) << 2);
		xmc[10] |= (gsmword)((*c >> 6) & 0x3);
		xmc[11]  = (gsmword)((*c >> 3) & 0x7);
		xmc[12]  = (gsmword)(*c++ & 0x7);
		Nc[1]  = (gsmword)((*c >> 1) & 0x7F);
		bc[1]  = (gsmword)((*c++ & 0x1) << 1);
		bc[1] |= (gsmword)((*c >> 7) & 0x1);
		Mc[1]  = (gsmword)((*c >> 5) & 0x3);
		xmaxc[1]  = (gsmword)((*c++ & 0x1F) << 1);
		xmaxc[1] |= (gsmword)((*c >> 7) & 0x1);
		xmc[13]  = (gsmword)((*c >> 4) & 0x7);
		xmc[14]  = (gsmword)((*c >> 1) & 0x7);
		xmc[15]  = (gsmword)((*c++ & 0x1) << 2);
		xmc[15] |= (gsmword)((*c >> 6) & 0x3);
		xmc[16]  = (gsmword)((*c >> 3) & 0x7);
		xmc[17]  = (gsmword)(*c++ & 0x7);
		xmc[18]  = (gsmword)((*c >> 5) & 0x7);
		xmc[19]  = (gsmword)((*c >> 2) & 0x7);
		xmc[20]  = (gsmword)((*c++ & 0x3) << 1);
		xmc[20] |= (gsmword)((*c >> 7) & 0x1);
		xmc[21]  = (gsmword)((*c >> 4) & 0x7);
		xmc[22]  = (gsmword)((*c >> 1) & 0x7);
		xmc[23]  = (gsmword)((*c++ & 0x1) << 2);
		xmc[23] |= (gsmword)((*c >> 6) & 0x3);
		xmc[24]  = (gsmword)((*c >> 3) & 0x7);
		xmc[25]  = (gsmword)(*c++ & 0x7);
		Nc[2]  = (gsmword)((*c >> 1) & 0x7F);
		bc[2]  = (gsmword)((*c++ & 0x1) << 1); 			/* 20 */
		bc[2] |= (gsmword)((*c >> 7) & 0x1);
		Mc[2]  = (gsmword)((*c >> 5) & 0x3);
		xmaxc[2]  = (gsmword)((*c++ & 0x1F) << 1);
		xmaxc[2] |= (gsmword)((*c >> 7) & 0x1);
		xmc[26]  = (gsmword)((*c >> 4) & 0x7);
		xmc[27]  = (gsmword)((*c >> 1) & 0x7);
		xmc[28]  = (gsmword)((*c++ & 0x1) << 2);
		xmc[28] |= (gsmword)((*c >> 6) & 0x3);
		xmc[29]  = (gsmword)((*c >> 3) & 0x7);
		xmc[30]  = (gsmword)(*c++ & 0x7);
		xmc[31]  = (gsmword)((*c >> 5) & 0x7);
		xmc[32]  = (gsmword)((*c >> 2) & 0x7);
		xmc[33]  = (gsmword)((*c++ & 0x3) << 1);
		xmc[33] |= (gsmword)((*c >> 7) & 0x1);
		xmc[34]  = (gsmword)((*c >> 4) & 0x7);
		xmc[35]  = (gsmword)((*c >> 1) & 0x7);
		xmc[36]  = (gsmword)((*c++ & 0x1) << 2);
		xmc[36] |= (gsmword)((*c >> 6) & 0x3);
		xmc[37]  = (gsmword)((*c >> 3) & 0x7);
		xmc[38]  = (gsmword)(*c++ & 0x7);
		Nc[3]  = (gsmword)((*c >> 1) & 0x7F);
		bc[3]  = (gsmword)((*c++ & 0x1) << 1);
		bc[3] |= (gsmword)((*c >> 7) & 0x1);
		Mc[3]  = (gsmword)((*c >> 5) & 0x3);
		xmaxc[3]  = (gsmword)((*c++ & 0x1F) << 1);
		xmaxc[3] |= (gsmword)((*c >> 7) & 0x1);
		xmc[39]  = (gsmword)((*c >> 4) & 0x7);
		xmc[40]  = (gsmword)((*c >> 1) & 0x7);
		xmc[41]  = (gsmword)((*c++ & 0x1) << 2);
		xmc[41] |= (gsmword)((*c >> 6) & 0x3);
		xmc[42]  = (gsmword)((*c >> 3) & 0x7);
		xmc[43]  = (gsmword)(*c++ & 0x7);					/* 30  */
		xmc[44]  = (gsmword)((*c >> 5) & 0x7);
		xmc[45]  = (gsmword)((*c >> 2) & 0x7);
		xmc[46]  = (gsmword)((*c++ & 0x3) << 1);
		xmc[46] |= (gsmword)((*c >> 7) & 0x1);
		xmc[47]  = (gsmword)((*c >> 4) & 0x7);
		xmc[48]  = (gsmword)((*c >> 1) & 0x7);
		xmc[49]  = (gsmword)((*c++ & 0x1) << 2);
		xmc[49] |= (gsmword)((*c >> 6) & 0x3);
		xmc[50]  = (gsmword)((*c >> 3) & 0x7);
		xmc[51]  = (gsmword)(*c & 0x7);					/* 33 */
	}

	Gsm_Decoder(s, LARc, Nc, bc, Mc, xmaxc, xmc, target);

	return GSM_SAMPLES_PER_FRAME;
}
