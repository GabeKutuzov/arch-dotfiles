/* (c) Copyright 1992-2010, Neurosciences Research Foundation */
/* $Id: d3ngph.c 42 2011-01-03 21:37:10Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                                                                      *
*           d3ngph - Call newplt(), allowing for timing and            *
*                        return code remapping                         *
*                                                                      *
* Arguments:                                                           *
* Being a wrapper for newplt(), arguments are identical to those for   *
* newplt().  Whenever newplt argument list is changed, make a similar  *
* change to d3ngph.                                                    *
*                                                                      *
* Return Codes:                                                        *
* Return codes from newplt() are 0, BUT_INTX (=1), or BUT_QUIT (=2).   *
* The corresponding CNS outnow codes are 0, OUT_USER_PAUSE, and        *
* OUT_USER_QUIT, which formerly had the same numeric values.           *
* However, as a safe coding practice, all calls to newplt() (from      *
* host or comp nodes) will be funnelled through d3ngph so these        *
* codes can be silently remapped if necessary.                         *
*                                                                      *
************************************************************************
*  Initial Version, 04/14/92 - ABP                                     *
*  Rev, 05/20/92, ABP - Add stattmr() calls                            *
*  V8D, 06/05/07, GNR - Call newplt() instead of ngraph()              *
*  ==>, 02/28/08, GNR - Last mod before committing to svn repository   *
*  V8F, 02/21/10, GNR - Remove VDR support                             *
***********************************************************************/

#include <stdio.h>
#include <stddef.h>
#include "sysdef.h"
#include "rocks.h"
#include "d3global.h"
#include "plots.h"

int d3ngph(float xsiz, float ysiz, float xorg, float yorg,
      char *pentyp, char *expcol, char *chart, int kout) {

   int rc;

   stattmr(OP_PUSH_TMR, GRPH_TMR);
   rc = newplt(xsiz, ysiz, xorg, yorg, pentyp, expcol, chart,
      kout | DO_MVEXP*bittst(RP->CP.trtimer, (long)RP->jmvtim));
#if OUT_USER_PAUSE != BUT_INTX || OUT_USER_QUIT != BUT_QUIT
   if (rc == BUT_INTX)      rc = OUT_USER_PAUSE;
   else if (rc == BUT_QUIT) rc = OUT_USER_QUIT;
#endif
   stattmr(OP_POP_TMR, GRPH_TMR);

   return rc;
   }
