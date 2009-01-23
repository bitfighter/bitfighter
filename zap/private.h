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


#ifndef	PRIVATE_H
#define	PRIVATE_H

#ifdef __GNUC__
  #define INLINE extern __inline__
#elif defined(__MWERKS__)
  #define INLINE __inline
#else
  #ifdef _MSC_VER
    #define INLINE __inline
    #pragma warning (disable:4514)
    #pragma warning (disable:4711)
  #else 
    #define INLINE
  #endif
#endif

#if defined(__MWERKS__)
  #include <string.h>
#else
  #include <memory.h>
#endif
#include "gsm.h"
#define	GSM_MAGIC		0xD		  	/* 13 kbit/s RPE-LTP */

typedef long			gsmword;	/* 16 or 32 bit signed int	*/
typedef long			longword;	/* 32 bit signed int	*/

struct gsm_state {

	gsmword		dp0[ 280 ];

	gsmword		z1;		/* preprocessing.c, Offset_com. */
	longword	L_z2;	/*                  Offset_com. */
	int		    mp;		/*                  Preemphasis	*/

	gsmword		u[8];		/* short_term_aly_filter.c	*/
	gsmword		LARpp[2][8];/*                          */
	gsmword		j;		    /*                          */

	gsmword     ltp_cut;        /* long_term.c, LTP crosscorr.  */
	gsmword		nrp; /* 40 */	/* long_term.c, synthesis	*/
	gsmword		v[9];		/* short_term.c, synthesis	*/
	gsmword		msr;		/* decoder.c,	Postprocessing	*/

	char		fast;		/* only used if FAST		*/
};


#define	MIN_WORD	(-32767 - 1)
#define	MAX_WORD	  32767

#define	SASR(x, by)	((x) >> (by))
#define SASL(x, by)	((x) << (by))

# define GSM_MULT_R(a, b)   (SASR( ((longword)(a) * (longword)(b)), 15 ))

# define GSM_MULT(a, b)     (SASR( ((longword)(a) * (longword)(b)), 15 ))

# define GSM_ADD(a, b)      (gsmword)((ltmp = (longword)(a) + (longword)(b)) >= MAX_WORD ? MAX_WORD : ltmp <= MIN_WORD ? MIN_WORD : ltmp)

# define GSM_SUB(a, b)      (gsmword)((ltmp = (longword)(a) - (longword)(b)) >= MAX_WORD ? MAX_WORD : ltmp <= MIN_WORD ? MIN_WORD : ltmp)

# define GSM_ABS(a)         (gsmword)((a) < 0 ?  -(a) : (a))

INLINE gsmword gsm_div (gsmword num, gsmword denum)
{
	longword	L_num   = SASL( num, 15 );
	longword	L_denum = denum;

    if (L_num == 0)
	    return 0;
    return (gsmword)(L_num/L_denum);
}

#endif	/* PRIVATE_H */
