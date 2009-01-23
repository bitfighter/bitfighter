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

#if !defined( MACOSX ) && !defined (__APPLE__)
#include <malloc.h>
#endif
#include <stdlib.h>

#include "private.h"

struct gsm_state *gsm_create(void)
{
	struct gsm_state *  r;

	r = (struct gsm_state *)malloc(sizeof(struct gsm_state));
	if (!r) return r;

	memset((char *)r, 0, sizeof(*r));
	r->nrp = 40;

	return r;
}

void gsm_destroy(struct gsm_state *S)
{
	if(S != NULL)
        free(S);
}

int gsm_option( struct gsm_state *r, int opt, int *val)
{
	int 	result = -1;

	switch (opt) {
	case GSM_OPT_LTP_CUT:
		result = r->ltp_cut;
		if(val != NULL)
            r->ltp_cut = (gsmword)*val;
		break;

	default:
		break;
	}
	return result;
}
