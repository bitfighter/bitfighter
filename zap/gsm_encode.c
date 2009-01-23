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

static unsigned char const bitoff[ 256 ] = {
    8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static gsmword gsm_FAC[8]  = { 18431, 20479, 22527, 24575, 26623, 28671, 30719, 32767 };
static gsmword gsm_NRFAC[8] = { 29128, 26215, 23832, 21846, 20165, 18725, 17476, 16384 };
static gsmword gsm_H[11] = {-134, -374, 0, 2054, 5741, 8192, 5741, 2054, 0, -374, -134 };
static gsmword gsm_DLB[4] = {6554, 16384, 26214, 32767};

static gsmword gsm_norm(longword a)
{
   if(a < 0)
    {
      if(a <= -1073741824)
            return 0;
      a = ~a;
   }

   return (gsmword)(a & 0xffff0000
      ? ( a & 0xff000000
        ?  -1 + bitoff[ 0xFF & (a >> 24) ]
        :   7 + bitoff[ 0xFF & (a >> 16) ] )
      : ( a & 0xff00
        ?  15 + bitoff[ 0xFF & (a >> 8) ]
        :  23 + bitoff[ 0xFF & a ] ));
}

static void Gsm_Preprocess(struct gsm_state * S, short *s, gsmword *so)
{
   gsmword     z1 = S->z1;
   longword    L_z2 = S->L_z2;
   gsmword  mp = (gsmword)S->mp;
   gsmword  s1;
   longword    L_s2;
    longword    L_temp;
   gsmword     msp;
   gsmword     SO;
   int          k = 160;

   while(k--)
    {

      SO = (gsmword)(SASL( SASR( *s, 3 ), 2 ));
      s++;

      s1 = (gsmword)(SO - z1);
      z1 = SO;

      L_s2 = s1;
      L_s2 = SASL( L_s2, 15 );

        L_z2 += L_s2;
      L_temp = L_z2 + 16384;

      msp   = (gsmword)GSM_MULT_R( mp, -28672 );
      mp    = (gsmword)SASR( L_temp, 15 );
      *so++ = (gsmword)(mp + msp);
   }

   S->z1   = z1;
   S->L_z2 = L_z2;
   S->mp   = mp;
}

static void Autocorrelation(gsmword *s, longword *L_ACF)
{
   int               k, i;
   gsmword         * sp = s;
   gsmword         sl;
   gsmword         temp, smax, scalauto;
    gsmword         ss[160];
    gsmword         * ssp = ss;

    smax = 0;
   for(k = 160; k--; sp++)
    {
      temp = (gsmword)GSM_ABS( *sp );
      if(temp > smax)
            smax = temp;
   }

   if(smax == 0)
    {
        scalauto = 0;
    }
   else {
      scalauto = (gsmword)(4 - gsm_norm( SASL( (longword)smax, 16 ) ));/* sub(4,..) */
   }

    sp = s;
   if(scalauto > 0)
    {
       for(k = 160; k--; sp++, ssp++)
         *ssp = (gsmword)SASR( *sp, scalauto );
   }
    else
    {
        memcpy(ssp, sp, sizeof(ss));
    }

#  define STEP(k)  L_ACF[k] += ((longword)sl * ssp[ -(k) ]);

#  define NEXTI    sl = *++ssp

    ssp = ss;
    sl = *ssp;
   for(k = 9; k--; L_ACF[k] = 0) ;

   STEP (0);
   NEXTI;
   STEP(0); STEP(1);
   NEXTI;
   STEP(0); STEP(1); STEP(2);
   NEXTI;
   STEP(0); STEP(1); STEP(2); STEP(3);
   NEXTI;
   STEP(0); STEP(1); STEP(2); STEP(3); STEP(4);
   NEXTI;
   STEP(0); STEP(1); STEP(2); STEP(3); STEP(4); STEP(5);
   NEXTI;
   STEP(0); STEP(1); STEP(2); STEP(3); STEP(4); STEP(5); STEP(6);
   NEXTI;
   STEP(0); STEP(1); STEP(2); STEP(3); STEP(4); STEP(5); STEP(6); STEP(7);

   for(i = 8; i < 160; i++)
    {

      NEXTI;

      STEP(0);
      STEP(1); STEP(2); STEP(3); STEP(4);
      STEP(5); STEP(6); STEP(7); STEP(8);
   }

   for(k = 9; k--; L_ACF[k] *= 2) ;

}

static void Reflection_coefficients(longword *L_ACF, gsmword *r)
{
   int           i, m, n;
   gsmword      temp;
   gsmword     ACF[9];  /* 0..8 */
   gsmword     P[  9];  /* 0..8 */
   gsmword     K[  9];  /* 2..8 */

   if(L_ACF[0] == 0)
    {
      for(i = 8; i--; *r++ = 0) ;
      return;
   }

   temp = gsm_norm( L_ACF[0] );

   for(i = 0; i <= 8; i++)
        ACF[i] = (gsmword)SASR( SASL (L_ACF[i], temp ), 16 );

   for(i = 1; i <= 7; i++)
        K[ i ] = ACF[ i ];

   for(i = 0; i <= 8; i++)
        P[ i ] = ACF[ i ];

   for(n = 1; n <= 8; n++, r++)
    {

      temp = P[1];
      temp = (gsmword)GSM_ABS(temp);
      if(P[0] < temp)
        {
         for(i = n; i <= 8; i++)
                *r++ = 0;
         return;
      }

      *r = gsm_div( temp, P[0] );

      if(P[1] > 0)
            *r = (gsmword)-*r;

      if(n == 8)
            return;

      temp = (gsmword)GSM_MULT_R( P[1], *r );
      P[0] += temp;

      for(m = 1; m <= 8 - n; m++)
        {
         P[m] = (gsmword)P[ m+1 ] + (gsmword)GSM_MULT_R( K[m], *r );
         K[m] = (gsmword)K[ m ] + (gsmword)GSM_MULT_R( P[ m+1 ], *r );
      }
   }
}

static void Transformation_to_Log_Area_Ratios(gsmword *r)
{
   gsmword  temp;
   int       i;

    for(i = 1; i <= 8; i++, r++)
    {

      temp = *r;
      temp = (gsmword)GSM_ABS(temp);

      if(temp < 22118)
        {
         temp = (gsmword)SASR( temp, 1 );
      }
        else if(temp < 31130)
        {
         temp -= 11059;
      }
        else
        {
         temp -= 26112;
         temp = (gsmword)SASL( temp, 2 );
      }

      *r = (gsmword)(*r < 0 ? -temp : temp);
   }
}

static void Quantization_and_coding(gsmword *LAR)
{
   gsmword  temp;

#  undef STEP
#  define   STEP( A, B, MAC, MIC )     \
      temp = (gsmword)GSM_MULT( A,   *LAR ); \
      temp += B;  \
      temp += 256;   \
      temp = (gsmword)SASR( temp, 9 ); \
      *LAR  =  (gsmword)(temp>MAC ? MAC - MIC : (temp<MIC ? 0 : temp - MIC)); \
      LAR++;

   STEP(  20480,     0,  31, -32 );
   STEP(  20480,     0,  31, -32 );
   STEP(  20480,  2048,  15, -16 );
   STEP(  20480, -2560,  15, -16 );

   STEP(  13964,    94,   7,  -8 );
   STEP(  15360, -1792,   7,  -8 );
   STEP(   8534,  -341,   3,  -4 );
   STEP(   9036, -1144,   3,  -4 );

#  undef STEP
}

static void Gsm_LPC_Analysis(gsmword *s, gsmword *LARc)
{
   longword L_ACF[9];

   Autocorrelation   (s, L_ACF);
   Reflection_coefficients(L_ACF, LARc);
   Transformation_to_Log_Area_Ratios(LARc);
   Quantization_and_coding(LARc);
}

static void Weighting_filter(gsmword *e, gsmword *x)
{
   longword    L_result, ltmp;
    gsmword     * pe = e;
   int          k;

   pe -= 5;

   for (k = 40; k--; pe++)
    {
#undef   STEP
#define  STEP( i, H )   (SASL( pe[i], (H) ) )

      L_result = 4096 - STEP( 0, 7 ) - (STEP( 1, 8 ))
      + STEP(  3,    11 ) + STEP( 4, 12 ) + STEP( 4, 10 )
      + STEP(  5,    13 ) + STEP( 6, 12 ) + STEP( 6, 10 )
      + STEP(  7,    11 ) - STEP( 9, 8 ) - STEP( 10, 7 );

      *x++ =  (gsmword)GSM_ADD( SASR( L_result, 13 ), 0 );
   }
}

static void RPE_grid_selection(gsmword *x, gsmword *xM, gsmword *Mc_out)
{
   int          i;
   longword L_result, L_temp;
   longword EM;
   gsmword     Mc;
   longword L_common_0_3;

   EM = 0;
   Mc = 0;

#undef   STEP
#define  STEP( m, i )      L_temp = SASR( x[m + 3 * i], 2 );   \
            L_result += L_temp * L_temp;

   L_result = 0;
   STEP( 0, 1 ); STEP( 0, 2 ); STEP( 0, 3 ); STEP( 0, 4 );
   STEP( 0, 5 ); STEP( 0, 6 ); STEP( 0, 7 ); STEP( 0, 8 );
   STEP( 0, 9 ); STEP( 0, 10); STEP( 0, 11); STEP( 0, 12);
   L_common_0_3 = L_result;

   STEP( 0, 0 );
   L_result = SASL( L_result, 1 );
   EM = L_result;

   L_result = 0;
   STEP( 1, 0 );
   STEP( 1, 1 ); STEP( 1, 2 ); STEP( 1, 3 ); STEP( 1, 4 );
   STEP( 1, 5 ); STEP( 1, 6 ); STEP( 1, 7 ); STEP( 1, 8 );
   STEP( 1, 9 ); STEP( 1, 10); STEP( 1, 11); STEP( 1, 12);
   L_result = SASL( L_result, 1 );
   if(L_result > EM)
    {
      Mc = 1;
      EM = L_result;
   }

   L_result = 0;
   STEP( 2, 0 );
   STEP( 2, 1 ); STEP( 2, 2 ); STEP( 2, 3 ); STEP( 2, 4 );
   STEP( 2, 5 ); STEP( 2, 6 ); STEP( 2, 7 ); STEP( 2, 8 );
   STEP( 2, 9 ); STEP( 2, 10); STEP( 2, 11); STEP( 2, 12);
   L_result = SASL( L_result, 1 );

   if(L_result > EM)
    {
      Mc = 2;
      EM = L_result;
   }

   L_result = L_common_0_3;
   STEP( 3, 12 );
   L_result = SASL( L_result, 1 );
   if(L_result > EM)
    {
      Mc = 3;
      EM = L_result;
   }

   for(i = 0; i <= 12; i ++)
        xM[i] = x[Mc + 3*i];

   *Mc_out = Mc;
}

static void APCM_quantization_xmaxc_to_exp_mant(gsmword xmaxc, gsmword *exp_out,
                                                gsmword *mant_out)
{
   gsmword  exp, mant;

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

static void APCM_quantization(gsmword *xM, gsmword *xMc, gsmword *mant_out,
                              gsmword *exp_out, gsmword *xmaxc_out)
{
   int           i, itest;
   gsmword      xmax, xmaxc, temp, temp1, temp2;
   gsmword      exp, mant;
    longword    ltmp;

   xmax = 0;
   for(i = 0; i <= 12; i++)
    {
      temp = xM[i];
      temp = (gsmword)GSM_ABS(temp);
      if (temp > xmax) xmax = temp;
   }

   exp   = 0;
   temp  = (gsmword)SASR( xmax, 9 );
   itest = 0;

   for(i = 0; i <= 5; i++)
    {

      itest |= (temp <= 0);
      temp = (gsmword)SASR( temp, 1 );

      if(itest == 0)
            exp++;      /* exp = add (exp, 1) */
   }

   temp = (gsmword)(exp + 5);
   xmaxc = (gsmword)GSM_ADD( (gsmword)SASR(xmax, temp), (gsmword)SASL(exp, 3) );

   APCM_quantization_xmaxc_to_exp_mant( xmaxc, &exp, &mant );

   temp1 = (gsmword)(6 - exp);      /* normalization by the exponent */
   temp2 = gsm_NRFAC[ mant ];    /* inverse mantissa      */

   for(i = 0; i <= 12; i++)
    {
      temp = (gsmword)SASL(xM[i], temp1);
      temp = (gsmword)GSM_MULT( temp, temp2 );
      temp = (gsmword)SASR(temp, 12);
      xMc[i] = (gsmword)(temp + 4);
   }

   *mant_out  = mant;
   *exp_out   = exp;
   *xmaxc_out = xmaxc;
}

static void APCM_inverse_quantization(gsmword *xMc, gsmword mant, gsmword exp, gsmword *xMp)
{
   int   i;
   gsmword  temp, temp1, temp2, temp3;
   longword ltmp;

   temp1 = gsm_FAC[ mant ];   /* see 4.2-15 for mant */
   temp2 = (gsmword)GSM_SUB( 6, exp ); /* see 4.2-15 for exp  */
   temp3 = (gsmword)SASL( 1, GSM_SUB( temp2, 1 ));

   for(i = 13; i--;)
    {

      temp = (gsmword)(SASL(*xMc++, 1) - 7);         /* restore sign   */

      temp = (gsmword)SASL(temp, 12);           /* 16 bit signed  */
      temp = (gsmword)GSM_MULT_R( temp1, temp );
      temp = (gsmword)GSM_ADD( temp, temp3 );
      *xMp++ = (gsmword)SASR( temp, temp2 );
   }
}

static void RPE_grid_positioning(gsmword Mc, gsmword *xMp, gsmword *ep)
{
   int   i = 13;

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

static void Gsm_RPE_Encoding(gsmword *e, gsmword *xmaxc, gsmword *Mc, gsmword *xMc)
{
   gsmword  x[40];
   gsmword  xM[13], xMp[13];
   gsmword  mant, exp;

   Weighting_filter(e, x);
   RPE_grid_selection(x, xM, Mc);

   APCM_quantization(   xM, xMc, &mant, &exp, xmaxc);
   APCM_inverse_quantization(  xMc,  mant,  exp, xMp);

   RPE_grid_positioning( *Mc, xMp, e );
}

static void Decoding_of_the_coded_Log_Area_Ratios(gsmword *LARc, gsmword *LARpp)
{
   gsmword      temp1;
   longword ltmp;

#undef   STEP
#define  STEP( B, MIC, INVA ) \
      temp1    = (gsmword)(SASL( *LARc++ + MIC , 10));   \
      temp1    -= SASL( B, 1 );     \
      temp1    = (gsmword)GSM_MULT_R( INVA, temp1 );     \
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
   int   i;

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
   int      i;
   gsmword  temp;

   for(i = 1; i <= 8; i++, LARp++)
    {
      if(*LARp < 0)
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

static void Short_term_analysis_filtering(struct gsm_state *S, gsmword *rp,
                                          int k_n, gsmword *s)
{
    gsmword     *u = S->u;
    gsmword     di, ui, sav;
    longword   ltmp;
    gsmword     u0=u[0], u1=u[1], u2=u[2], u3=u[3],
                u4=u[4], u5=u[5], u6=u[6], u7=u[7];
    gsmword     rp0, rp1, rp2, rp3,
                rp4, rp5, rp6, rp7;

    rp0 = rp[0]; rp1 = rp[1];
    rp2 = rp[2]; rp3 = rp[3];
    rp4 = rp[4]; rp5 = rp[5];
    rp6 = rp[6]; rp7 = rp[7];

    while( k_n-- != 0)
    {

        di = sav = *s;

        ui  = u0;
        u0  = sav;

        sav = ui + (gsmword)GSM_MULT_R(rp0, di);
        di  += (gsmword)GSM_MULT_R(rp0, ui);

        ui  = u1;
        u1  = sav;

        sav = ui + (gsmword)GSM_MULT_R(rp1, di);
        di  += (gsmword)GSM_MULT_R(rp1, ui);

        ui  = u2;
        u2  = sav;

        sav = ui + (gsmword)GSM_MULT_R(rp2, di);
        di  += (gsmword)GSM_MULT_R(rp2, ui);

        ui  = u3;
        u3  = sav;

        sav = ui + (gsmword)GSM_MULT_R(rp3, di);
        di  += (gsmword)GSM_MULT_R(rp3, ui);

        ui  = u4;
        u4  = sav;

        sav = ui + (gsmword)GSM_MULT_R(rp4, di);
        di  += (gsmword)GSM_MULT_R(rp4, ui);

        ui  = u5;
        u5  = sav;

        sav = ui + (gsmword)GSM_MULT_R(rp5,di);
        di  += (gsmword)GSM_MULT_R(rp5,ui);

        ui  = u6;
        u6  = sav;

        sav = ui + (gsmword)GSM_MULT_R(rp6,di);
        di  += (gsmword)GSM_MULT_R(rp6,ui);

        ui  = u7;
        u7  = sav;

        *s++ = (gsmword)GSM_ADD( di, (gsmword)GSM_MULT_R(rp7,ui) ); /* This GSM_ADD is needed for over/under flow */
    }
    u[0]=GSM_ADD(u0, 0);
    u[1]=GSM_ADD(u1, 0);
    u[2]=GSM_ADD(u2, 0);
    u[3]=GSM_ADD(u3, 0);
    u[4]=GSM_ADD(u4, 0);
    u[5]=GSM_ADD(u5, 0);
    u[6]=GSM_ADD(u6, 0);
    u[7]=GSM_ADD(u7, 0);
}

static void Gsm_Short_Term_Analysis_Filter( struct gsm_state *S, gsmword *LARc, gsmword *s)
{
   gsmword     * LARpp_j   = S->LARpp[ S->j      ];
   gsmword     * LARpp_j_1 = S->LARpp[ S->j ^= 1 ];

   gsmword     LARp[8];

   Decoding_of_the_coded_Log_Area_Ratios(LARc, LARpp_j);

   Coefficients_0_12(LARpp_j_1, LARpp_j, LARp);
   LARp_to_rp(LARp);
   Short_term_analysis_filtering(S, LARp, 13, s);

   Coefficients_13_26(LARpp_j_1, LARpp_j, LARp);
   LARp_to_rp(LARp);
   Short_term_analysis_filtering(S, LARp, 14, s + 13);

   Coefficients_27_39(LARpp_j_1, LARpp_j, LARp);
   LARp_to_rp(LARp);
   Short_term_analysis_filtering(S, LARp, 13, s + 27);

   Coefficients_40_159(LARpp_j, LARp);
   LARp_to_rp(LARp);
   Short_term_analysis_filtering(S, LARp, 120, s + 40);
}

static void Calculation_of_the_LTP_parameters(gsmword *d, gsmword *dp,
                                              gsmword *bc_out, gsmword *Nc_out)
{
   int       k, lambda;
   gsmword     Nc, bc;
   gsmword     wt[40];
    gsmword     * pwt = wt;
   longword L_max, L_power;
   gsmword     R, S, dmax, scal;
   gsmword      temp;
    gsmword     * pd = d;

   dmax = 0;

   for (k = 0; k < 40; k+=8)
    {
# undef STEP
# define STEP temp = (gsmword)GSM_ABS( *pd );\
      if (temp > dmax) dmax = temp;\
        pd++;

        STEP;STEP;STEP;STEP;
        STEP;STEP;STEP;STEP;
    }

   temp = 0;
   if(dmax == 0)
       scal = 0;
   else
      temp = gsm_norm( SASL( (longword)dmax, 16 ) );

   if(temp > 6)
        scal = 0;
   else
        scal = (gsmword)(6 - temp);

    pd = d;
    for (k = 0; k < 40; k+=8)
    {
# undef STEP
# define STEP        *pwt = (gsmword)SASR( *pd, scal ); pd++; pwt++;

        STEP;STEP;STEP;STEP;
        STEP;STEP;STEP;STEP;
    }

   L_max = 0;
   Nc    = 40; /* index for the maximum cross-correlation */

   for (lambda = 40; lambda <= 120; lambda++)
    {

# undef STEP
#     define STEP(k)    (longword)wt[k] * dp[k - lambda];

      register longword L_result;

      L_result  = STEP(0)  ; L_result += STEP(1) ;
      L_result += STEP(2)  ; L_result += STEP(3) ;
      L_result += STEP(4)  ; L_result += STEP(5)  ;
      L_result += STEP(6)  ; L_result += STEP(7)  ;
      L_result += STEP(8)  ; L_result += STEP(9)  ;
      L_result += STEP(10) ; L_result += STEP(11) ;
      L_result += STEP(12) ; L_result += STEP(13) ;
      L_result += STEP(14) ; L_result += STEP(15) ;
      L_result += STEP(16) ; L_result += STEP(17) ;
      L_result += STEP(18) ; L_result += STEP(19) ;
      L_result += STEP(20) ; L_result += STEP(21) ;
      L_result += STEP(22) ; L_result += STEP(23) ;
      L_result += STEP(24) ; L_result += STEP(25) ;
      L_result += STEP(26) ; L_result += STEP(27) ;
      L_result += STEP(28) ; L_result += STEP(29) ;
      L_result += STEP(30) ; L_result += STEP(31) ;
      L_result += STEP(32) ; L_result += STEP(33) ;
      L_result += STEP(34) ; L_result += STEP(35) ;
      L_result += STEP(36) ; L_result += STEP(37) ;
      L_result += STEP(38) ; L_result += STEP(39) ;

      if (L_result > L_max)
        {

         Nc    = (gsmword)lambda;
         L_max = L_result;
      }
   }

   *Nc_out = Nc;

   L_max = SASL(L_max, 1 );

   L_max = SASR( L_max, (6 - scal) );  /* sub(6, scal) */

   L_power = 0;
   for (k = 0; k < 40; k+=4)
    {

      register longword L_temp;

      L_temp   = SASR( dp[k - Nc], 3 );
      L_power += L_temp * L_temp;

      L_temp   = SASR( dp[k + 1 - Nc], 3 );
      L_power += L_temp * L_temp;

      L_temp   = SASR( dp[k + 2 - Nc], 3 );
      L_power += L_temp * L_temp;

      L_temp   = SASR( dp[k + 3 - Nc], 3 );
      L_power += L_temp * L_temp;

    }
   L_power = SASL( L_power, 1 );

   if (L_max <= 0)
    {
      *bc_out = 0;
      return;
   }
   if (L_max >= L_power)
    {
      *bc_out = 3;
      return;
   }

   temp = gsm_norm( L_power );

   R = (gsmword)SASR( SASL( L_max, temp ), 16 );
   S = (gsmword)SASR( SASL( L_power, temp ), 16 );

   for(bc = 0; bc <= 2; bc++)
    {
        if(R <= GSM_MULT(S, gsm_DLB[bc]))
            break;
    }
   *bc_out = bc;
}

static void Cut_Calculation_of_the_LTP_parameters(gsmword *d, gsmword *dp,
                                                  gsmword *bc_out, gsmword *Nc_out)
{
   int       k, lambda;
   gsmword     Nc, bc;

   longword L_result;
   longword L_max, L_power;
   gsmword     R, S, dmax, scal, best_k;

   gsmword      temp, wt_k;

   dmax = best_k = 0;
   for (k = 0; k <= 39; k++)
    {
      temp = (gsmword)GSM_ABS( d[k] );
      if (temp > dmax)
        {
         dmax = temp;
         best_k = (gsmword)k;
      }
   }
   temp = 0;
   if (dmax == 0)
    {
        scal = 0;
    }
   else
    {
      temp = gsm_norm( (longword)dmax << 16 );
   }
   if (temp > 6) scal = 0;
   else scal = (gsmword)(6 - temp);

   L_max = 0;
   Nc    = 40; /* index for the maximum cross-correlation */
   wt_k  = (gsmword)SASR(d[best_k], scal);

   for (lambda = 40; lambda <= 120; lambda++)
    {
      L_result = (longword)wt_k * dp[best_k - lambda];
      if (L_result > L_max)
        {
         Nc    = (gsmword)lambda;
         L_max = L_result;
      }
   }
   *Nc_out = Nc;
   L_max <<= 1;

   L_max = L_max >> (6 - scal);  /* sub(6, scal) */

   L_power = 0;
   for (k = 0; k <= 39; k++)
    {
      longword L_temp;

      L_temp   = SASR( dp[k - Nc], 3 );
      L_power += L_temp * L_temp;
   }
   L_power <<= 1;

   if (L_max <= 0)
    {
      *bc_out = 0;
      return;
   }
   if (L_max >= L_power)
    {
      *bc_out = 3;
      return;
   }

   temp = gsm_norm(L_power);

   R = (gsmword)SASR( SASL(L_max, temp), 16 );
   S = (gsmword)SASR( SASL(L_power, temp), 16 );

   for (bc = 0; bc <= 2; bc++)
    {
        if (R <= GSM_MULT(S, gsm_DLB[bc]))
            break;
    }
   *bc_out = bc;
}

static void Long_term_analysis_filtering(gsmword bc, gsmword Nc, gsmword *dp, gsmword *d,
                                         gsmword *dpp, gsmword *e)
{
   int k;

    dp -= Nc;

#  undef STEP
#  define STEP(BP)               \
   for (k = 40; k--; e++, dpp++, d++, *dp++) {        \
      *dpp = (gsmword)GSM_MULT_R( BP, *dp);  \
      *e = (gsmword)( *d - *dpp );  \
   }

   switch (bc) {
   case 0:  STEP(  3277 ); break;
   case 1:  STEP( 11469 ); break;
   case 2: STEP( 21299 ); break;
   case 3: STEP( 32767 ); break;
   }
}

static void Gsm_Long_Term_Predictor(struct gsm_state *S, gsmword *d, gsmword *dp, gsmword *e,
                              gsmword *dpp, gsmword *Nc, gsmword *bc)
{
    if(S->ltp_cut != 0)
    {
       Cut_Calculation_of_the_LTP_parameters(d, dp, bc, Nc);
    }
    else
    {
       Calculation_of_the_LTP_parameters(d, dp, bc, Nc);
    }

   Long_term_analysis_filtering( *bc, *Nc, dp, d, dpp, e );
}

static void Gsm_Coder(struct gsm_state *S, short *s, gsmword *LARc, gsmword *Nc,
            gsmword *bc, gsmword *Mc, gsmword *xmaxc, gsmword *xMc)
{
   int   k;
   gsmword  *dp  = S->dp0 + 120; /* [ -120...-1 ] */
   gsmword e[50];
    gsmword *pe = &e[5];
   gsmword  so[160];

    memset(e, 0, sizeof(e));
   Gsm_Preprocess(S, s, so);
   Gsm_LPC_Analysis(so, LARc);
   Gsm_Short_Term_Analysis_Filter   (S, LARc, so);

   for (k = 0; k <= 3; k++, xMc += 13)
    {
        int i;

      Gsm_Long_Term_Predictor(S, so+k*40, dp, pe, dp, Nc++, bc++);

      Gsm_RPE_Encoding(pe, xmaxc++, Mc++, xMc );

      for (i = 0; i <= 39; i++, dp++)
         *dp +=  (gsmword)pe[i];
   }
   (void)memcpy( (char *)S->dp0, (char *)(S->dp0 + 160), 120 * sizeof(*S->dp0) );
}

int gsm_encode(struct gsm_state * s, short * source, unsigned char * c)
{
    gsmword    LARc[8], Nc[4], Mc[4], bc[4], xmaxc[4], xmc[13*4];

    Gsm_Coder(s, source, LARc, Nc, bc, Mc, xmaxc, xmc);

    *c++ =   (unsigned char)(((GSM_MAGIC & 0xF) << 4)          /* 1 */
        | ((LARc[0] >> 2) & 0xF));
    *c++ =   (unsigned char)(((LARc[0] & 0x3) << 6)
        | (LARc[1] & 0x3F));
    *c++ =   (unsigned char)(((LARc[2] & 0x1F) << 3)
        | ((LARc[3] >> 2) & 0x7));
    *c++ =   (unsigned char)(((LARc[3] & 0x3) << 6)
        | ((LARc[4] & 0xF) << 2)
        | ((LARc[5] >> 2) & 0x3));
    *c++ =   (unsigned char)(((LARc[5] & 0x3) << 6)
        | ((LARc[6] & 0x7) << 3)
        | (LARc[7] & 0x7));
    *c++ =   (unsigned char)(((Nc[0] & 0x7F) << 1)
        | ((bc[0] >> 1) & 0x1));
    *c++ =   (unsigned char)(((bc[0] & 0x1) << 7)
        | ((Mc[0] & 0x3) << 5)
        | ((xmaxc[0] >> 1) & 0x1F));
    *c++ =   (unsigned char)(((xmaxc[0] & 0x1) << 7)
        | ((xmc[0] & 0x7) << 4)
        | ((xmc[1] & 0x7) << 1)
        | ((xmc[2] >> 2) & 0x1));
    *c++ =   (unsigned char)(((xmc[2] & 0x3) << 6)
        | ((xmc[3] & 0x7) << 3)
        | (xmc[4] & 0x7));
    *c++ =   (unsigned char)(((xmc[5] & 0x7) << 5)             /* 10 */
        | ((xmc[6] & 0x7) << 2)
        | ((xmc[7] >> 1) & 0x3));
    *c++ =   (unsigned char)(((xmc[7] & 0x1) << 7)
        | ((xmc[8] & 0x7) << 4)
        | ((xmc[9] & 0x7) << 1)
        | ((xmc[10] >> 2) & 0x1));
    *c++ =   (unsigned char)(((xmc[10] & 0x3) << 6)
        | ((xmc[11] & 0x7) << 3)
        | (xmc[12] & 0x7));
    *c++ =   (unsigned char)(((Nc[1] & 0x7F) << 1)
        | ((bc[1] >> 1) & 0x1));
    *c++ =   (unsigned char)(((bc[1] & 0x1) << 7)
        | ((Mc[1] & 0x3) << 5)
        | ((xmaxc[1] >> 1) & 0x1F));
    *c++ =   (unsigned char)(((xmaxc[1] & 0x1) << 7)
        | ((xmc[13] & 0x7) << 4)
        | ((xmc[14] & 0x7) << 1)
        | ((xmc[15] >> 2) & 0x1));
    *c++ =   (unsigned char)(((xmc[15] & 0x3) << 6)
        | ((xmc[16] & 0x7) << 3)
        | (xmc[17] & 0x7));
    *c++ =   (unsigned char)(((xmc[18] & 0x7) << 5)
        | ((xmc[19] & 0x7) << 2)
        | ((xmc[20] >> 1) & 0x3));
    *c++ =   (unsigned char)(((xmc[20] & 0x1) << 7)
        | ((xmc[21] & 0x7) << 4)
        | ((xmc[22] & 0x7) << 1)
        | ((xmc[23] >> 2) & 0x1));
    *c++ =   (unsigned char)(((xmc[23] & 0x3) << 6)
        | ((xmc[24] & 0x7) << 3)
        | (xmc[25] & 0x7));
    *c++ =   (unsigned char)(((Nc[2] & 0x7F) << 1)             /* 20 */
        | ((bc[2] >> 1) & 0x1));
    *c++ =   (unsigned char)(((bc[2] & 0x1) << 7)
        | ((Mc[2] & 0x3) << 5)
        | ((xmaxc[2] >> 1) & 0x1F));
    *c++ =   (unsigned char)(((xmaxc[2] & 0x1) << 7)
        | ((xmc[26] & 0x7) << 4)
        | ((xmc[27] & 0x7) << 1)
        | ((xmc[28] >> 2) & 0x1));
    *c++ =   (unsigned char)(((xmc[28] & 0x3) << 6)
        | ((xmc[29] & 0x7) << 3)
        | (xmc[30] & 0x7));
    *c++ =   (unsigned char)(((xmc[31] & 0x7) << 5)
        | ((xmc[32] & 0x7) << 2)
        | ((xmc[33] >> 1) & 0x3));
    *c++ =   (unsigned char)(((xmc[33] & 0x1) << 7)
        | ((xmc[34] & 0x7) << 4)
        | ((xmc[35] & 0x7) << 1)
        | ((xmc[36] >> 2) & 0x1));
    *c++ =   (unsigned char)(((xmc[36] & 0x3) << 6)
        | ((xmc[37] & 0x7) << 3)
        | (xmc[38] & 0x7));
    *c++ =   (unsigned char)(((Nc[3] & 0x7F) << 1)
        | ((bc[3] >> 1) & 0x1));
    *c++ =   (unsigned char)(((bc[3] & 0x1) << 7)
        | ((Mc[3] & 0x3) << 5)
        | ((xmaxc[3] >> 1) & 0x1F));
    *c++ =   (unsigned char)(((xmaxc[3] & 0x1) << 7)
        | ((xmc[39] & 0x7) << 4)
        | ((xmc[40] & 0x7) << 1)
        | ((xmc[41] >> 2) & 0x1));
    *c++ =   (unsigned char)(((xmc[41] & 0x3) << 6)            /* 30 */
        | ((xmc[42] & 0x7) << 3)
        | (xmc[43] & 0x7));
    *c++ =   (unsigned char)(((xmc[44] & 0x7) << 5)
        | ((xmc[45] & 0x7) << 2)
        | ((xmc[46] >> 1) & 0x3));
    *c++ =   (unsigned char)(((xmc[46] & 0x1) << 7)
        | ((xmc[47] & 0x7) << 4)
        | ((xmc[48] & 0x7) << 1)
        | ((xmc[49] >> 2) & 0x1));
    *c++ =   (unsigned char)(((xmc[49] & 0x3) << 6)
        | ((xmc[50] & 0x7) << 3)
        | (xmc[51] & 0x7));

    return GSM_ENCODED_FRAME_SIZE;
}
