/* (c) Copyright 1991-2003, Neurosciences Research Foundation, Inc. */
/* $Id: d1genr.c 3 2008-03-11 19:50:00Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d1genr                                 *
*                                                                      *
*     This routine generates Darwin I repertoires for use in CNS.      *
*  Note that testing for #ifdef D1 not needed--only linked if used.    *
*  As in d3genr, the override bits are assumed to have all been set    *
*  if there is no restoration from files to save redundant testing.    *
*                                                                      *
*  V5C, 11/21/91, GNR - Newly written                                  *
*  Rev, 10/18/01, GNR - Separate kd1 into three components             *
*  V8C, 02/22/03, GNR - Cell responses in millivolts, add conductances *
*  ==>, 04/08/06, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#define D1TYPE  struct D1BLK

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rkarith.h"
#include "d3global.h"
#include "d1def.h"

#define BPU 3              /* Number of usable bytes per udev() */

void d1genr(void) {

#ifndef PAR0

   D1TYPE *pd1;            /* Ptr to current D1BLK */
   d1s_type *pd;           /* Ptr to repertoire data */
   long ibyte,icell;       /* Byte and cell counters */
   register long tu;       /* Temp returned by udev() */

/* Loop over all D1BLK's and fill in the repertoire data.
*  Up to three bytes are filled in from each udev() call.
*  (Shift-and-store is used rather than memcpy for big-endian
*  vs. little-endian consistency.  The ggui32() routine used
*  in the original Darwin I is not easily replicated except
*  on IBM370 and anyway was a kludge.)  The only reasonable
*  way to get deterministic results across varying cube sizes
*  is to start a new udev() call for each new cell, and then
*  udevskip by lod1e*(number of udevs/cell) on each node.  */

   /* Forget the whole thing if replay with NOGENR */
   if (RP->ksv & KSVNR) goto OMIT_D1_GENR;
   for (pd1=RP->pd1b1; pd1; pd1=pd1->pnd1b) {
#ifdef PAR
      udevskip(&pd1->d1seed,pd1->lod1e*(pd1->nbyt+(BPU-1))/BPU);
#endif
      if (!(pd1->kd1o & KD1_OVER)) continue;
      /* Clear scores */
      memset((char *)pd1->pd1s,0,pd1->nd1r*pd1->nepr);
      pd = pd1->pd1r;      /* Point to data */
      for (icell=0; icell<pd1->mynd1; icell++) {
         for (ibyte=pd1->nbyt; ; ) {   /* Store up to 3 bytes */
            tu = udev(&pd1->d1seed);
            *pd++ = tu & 0xff;
            if (!--ibyte) break;
            *pd++ = (tu>>=BITSPERBYTE) & 0xff;
            if (!--ibyte) break;
            *pd++ = (tu>>=BITSPERBYTE) & 0xff;
            if (!--ibyte) break;
            } /* End byte loop */
         } /* End cell loop */
      } /* End generating Darwin I repertoires */

OMIT_D1_GENR: ;            /* Here if omitting all generating */
#endif                     /* End Node 0 exclusion block */

   } /* End d1genr */
