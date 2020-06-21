/* (c) Copyright 1991-2008, Neurosciences Research Foundation, Inc. */
/* $Id: d3woff.c 43 2011-02-24 04:00:37Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3woff                                 *
*                                                                      *
*  CNS window offset routine                                           *
*                                                                      *
*  Call: woff = d3woff(struct CONNTYPE *ix)                            *
*        where 'ix' = ptr to relevant CONNTYPE block.                  *
*  Returns:  Offset in IA coords of upper left corner of connection    *
*        matrix when positioned so center of arbor is on center of     *
*        window (KGNST) or of entire IA (KGNTP).                       *
*                                                                      *
*  Note:  The rule for IA bounds checking as of V8D, 06/23/08, is      *
*  that Lij generating routines are responsible to store Lij that lie  *
*  within a rectangle the size of the whole IA or window (KGNST only). *
*  But this must be checked again when the window offset is added by   *
*  d3sj routines, etc., just in case nux,nuy are not the same as the   *
*  window size.  The earlier checking at Lij generation time assures   *
*  that y will not overflow into x when the offset is added.  It is    *
*  understood that lijbr1 routines may generate out-of-bounds connec-  *
*  tions that are brought in bounds by lijbr2--this checking must be   *
*  done using the fstx,fsty mechanism.                                 *
*                                                                      *
*  Note:  d3woff() can be called on serial, PAR0 or PARn nodes.        *
*                                                                      *
*  V2C, 02/25/87, GNR - Newly written for d3rplt                       *
*  V4A, 04/26/89, XXX - Translated to C                                *
*  V5E, 06/30/92, GNR - Restore serial version                         *
*  Rev, 03/24/93, GNR - Remove boundary test                           *
*  Rev, 08/02/93, GNR - Put in SRA where needed                        *
*  V8B, 12/27/00, GNR - Move to new memory management routines         *
*  ==>, 05/30/07, GNR - Last mod before committing to svn repository   *
*  Rev, 06/21/08, GNR - Add above comments, modify d3lij,d3sj thusly   *
*  Rev, 06/26/08, GNR - Adjustment for KGNBL box size removed due to   *
*                       the change in lb1tia that confines generated   *
*                       Lij to a box of size nux,nuy                   *
***********************************************************************/

#define NEWARB          /* Use new EFFARB mechanism */

#include <stdio.h>
#include <stddef.h>
#include "sysdef.h"
#include "d3global.h"
#include "wdwdef.h"

/*---------------------------------------------------------------------*
*                               d3woff                                 *
*                                                                      *
*  Note:  This routine calculates everything multiplied by 2, then     *
*  divides by two at the end in order to get half matrix size easily.  *
*---------------------------------------------------------------------*/

long d3woff(struct CONNTYPE *ix) {

   long xoff,yoff;

   /* Exit if inapplicable case */
   if (!(ix->kgfl & KGNIA) || !(ix->kgen & KGNTP)) return 0;

   /* Find twice center coords of window or full IA */
   if (ix->kgen & KGNST) {
#ifdef PAR
      /* Skip over arm and lower-numbered window data */
      float *wdim = (float *)RP->paw +
         (RP->narms*BARM_VALS + ix->wdwi*BWDW_VALS);
      xoff = (long)(2.0 * wdim[AWX] + wdim[WWD]);
      yoff = (long)(2.0 * wdim[AWY] - wdim[WHT]);
#else
      /* Serial version--access WDWDEF block via CONNTYPE block */
      struct WDWDEF *pw = (struct WDWDEF *)ix->psrc;
      xoff = (long)(2.0 * pw->wx + pw->wwd);
      yoff = (long)(2.0 * pw->wy - pw->wht);
#endif
      }
   else {  /* Input from full IA--size is simply nsx,nsy */
      xoff = RP->nsx;
      yoff = RP->nsy;
      }

   xoff -= ix->nux;
   yoff -= ix->nuy;
   xoff = SRA(xoff,1);
   yoff = SRA(yoff,1);

   return (xoff << RP->ky1) + yoff;
   }  /* End of d3woff() */

