/* Copyright (c) 1994-2008, Neurosciences Research Foundation, Inc. */
/* $Id: d3decay.c 14 2009-01-06 20:09:33Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3decay                                *
*                                                                      *
************************************************************************
*                                                                      *
*     This routine handles a standardized decay calculation that may   *
*  be used with any connection type.  For that reason, arguments are   *
*  gathered individually by the caller.  The calculation performed     *
*  depends on a method code stored in the half-effectiveness para-     *
*  meter.  Notation:                                                   *
*     Aeff(t)     Effective value at time t of afferent term after     *
*                    decay.  Value of Aeff(t-1) is passed to d3decay.  *
*     Acal(t)     Calculated (new) value of afferent term in question. *
*                    (Both Aeff and Acal are allowed to be signed.     *
*                    They have same sign if GEOM type, but not         *
*                    necessarily when doing global cell response.)     *
*     omega       Persistence constant for this kind of term (S30).    *
*     halfeff     Value of Aeff at which saturation is 0.5.            *
*     hesam       1.3170/(2**20 or 2**27)/halfeff (float).             *
*                                                                      *
*  I.    EXPONENTIAL method (hesam = 0.0)                              *
*        Aeff(t) = Acal(t) + omega*Aeff(t-1)                           *
*  II.   LIMITING method (hesam = -1.0)                                *
*        Aeff(t) = sign(Acal)*max(|Acal(t)|, omega*|Aeff(t-1)|)        *
*  III.  SATURATING method                                             *
*        Aeff(t) = sech(hesam*|Aeff(t-1)|)*Acal(t) + omega*Aeff(t-1)   *
*        (sech(x) is computed by using cosh() library function.  An    *
*        experimental table lookup implementation was too inaccurate.) *
*                                                                      *
*  Synopsis: long d3decay(struct DECAYDEF *pdcd, long *pAeff,          *
*        long Acal, long *pPers)                                       *
*  Arguments:                                                          *
*     pdcd           Ptr to DECAYDEF structure containing parameters   *
*                    governing decay process.                          *
*     pAeff          Ptr to location where Aeff(t-1) is stored (S20).  *
*                    This is replaced by Aeff(t) on return.  Must be   *
*                    initialized at reset or first cycle.              *
*     Acal           Calculated new activity value (S20).              *
*     pPers          Ptr to location where new persistence value is    *
*                    left on return for use by detail print or stats   *
*                    (S20).                                            *
*  Results returned:                                                   *
*     Aeff(t)        New effective activity value (S20).               *
*     *pAeff         Same as Aeff(t) (S20).                            *
*     *pPers         Persistence (S20).                                *
*                                                                      *
*  Note:                                                               *
*     The only thing that has to be changed if the scale of Aeff,      *
*     Acal is changed is the definition of the constant DCYSDFLT.      *
*                                                                      *
*  V7C, 08/28/94, GNR - Initial version (hesam to float, 02/02/95)     *
*  V8A, 04/08/96, GNR - Revise calling args for use with cell decay    *
*  Rev, 01/12/98, GNR - New argument arrangement, pers not in DECAYDEF *
*  V8C, 05/04/03, GNR - Scale Aeff, Acal etc. to S20 from S24          *
*  V8D, 07/04/05, GNR - Scale omega to S30 from S16                    *
*  ==>, 08/07/07, GNR - Last mod before committing to svn repository   *
*  Rev, 12/31/08, GNR - Replace jm64sh with msrsle                     *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sysdef.h"
#include "d3global.h"
#include "rkarith.h"

long d3decay(struct DECAYDEF *pdcd, long *pAeff, long acal,
      long *pPers) {

/* Proceed according to requested mode of calculation */

   if (pdcd->omega) {

      long pers = msrsle(*pAeff, pdcd->omega, -FBod, OVF_DCY);

      if (pdcd->hesam == DCYEXP) {
         /* Exponential decay */
         acal = acal + pers;
         }

      else if (pdcd->hesam == DCYLIM) {
         /* Limiting persistence */
         if (acal >= 0)
            acal = max(acal, pers);
         else
            acal = min(acal, pers);
         }

      else {
         /* Saturating persistence */
         double x = (double)(*pAeff)*pdcd->hesam;
         acal = (long)((double)acal/cosh(x)) + pers;
         }

      /* Store values returned via pointers */
      *pPers = pers;
      *pAeff = acal;

      } /* End if omega */

   return acal;
   } /* End d3decay */
