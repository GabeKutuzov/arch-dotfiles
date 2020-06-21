/* (c) Copyright 1989, Neurosciences Research Foundation, Inc. */
/* $Id: d3cnmd.c 43 2011-02-24 04:00:37Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3cnmd                                 *
*                                                                      *
*  Establish conditions for canonical trace and grip modes             *
*                                                                      *
************************************************************************
*  Written by George N. Reeke                                          *
*  V4A, 05/10/89, Translated to C                                      *
*  ==>, 04/16/07, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#define NEWARB          /* Use new EFFARB mechanism */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "d3global.h"
#include "jntdef.h"
#include "armdef.h"

void d3cnmd(struct VDTDEF *valary) {

   struct ARMDEF *pa;
   struct JNTDEF *pj;

/* Loop over all arms */

   for (pa=RP0->parm1; pa; pa=pa->pnxarm) {
      pj = pa->pjnt + pa->njnts;

/* Determine whether it is time to go into canonical trace mode */

      if (pa->jatt & ARMAU) {
         if (pa->jatt & ARMUJ) {
            if (pj->u.ujnt.ujct > 0) pj->u.ujnt.ujct--;
            if (valary[pj->u.ujnt.ujvi-1].fullvalue >=
               pj->u.ujnt.ujvt) pj->u.ujnt.ujct = pj->u.ujnt.ctime;
            if (pj->u.ujnt.ujct > 0) pa->jatt |= ARMTR;
            else pa->jatt &= ~((long)ARMTR);
            }
         pj++;
         }

/* Determine whether it is time to go into grip mode */

      if (pa->jatt & ARMGE) {
         if (pj->u.grip.grct > 0) pj->u.grip.grct--;
         if (valary[pj->u.grip.grvi-1].fullvalue >= pj->u.grip.grvt)
            pj->u.grip.grct = pj->u.grip.gtime;
         if (pj->u.grip.grct > 0) {
            pa->jatt |= ARMGP;
         /* Turn off trace mode if gripping is on */
            if (pa->jatt & ARMUJ) {
               pa->jatt &= ~((long)ARMTR);
               (pj-1)->u.ujnt.ujct = 0;
               }
            }
         else pa->jatt &= ~((long)ARMGP);
         }
      } /* End of pa loop */

   } /* End of d3cnmd() */
