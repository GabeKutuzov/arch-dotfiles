/* (c) Copyright 1992-2017, The Rockefeller University *21116* */
/* $Id: d3ngph.c 72 2017-04-14 19:11:21Z  $ */
/***********************************************************************
*                             CNS Program                              *
*           d3ngph - Call newplt(), allowing for timing and            *
*                        Return code remapping                         *
*                                                                      *
* Arguments:                                                           *
*  Being a wrapper for newplt(), arguments are identical to those for  *
*  newplt().  Whenever newplt argument list is changed, make a similar *
*  change to d3ngph.                                                   *
*                                                                      *
* Return Codes:                                                        *
*  Return codes from newplt() are 0, BUT_INTX (=1), BUT_QUIT (=2), or  *
*  BUT_DEAD (=0x80) [definitive list in plotdefs.h]. The corresponding *
*  CNS outnow codes formerly had the same numeric values, but now are  *
*  different.  Hence, all calls to newplt() (from host or comp nodes)  *
*  will be funnelled through d3ngph to remap the BUT_xxxx codes.       *
*                                                                      *
************************************************************************
*  Initial Version, 04/14/92 - ABP                                     *
*  Rev, 05/20/92, ABP - Add stattmr() calls                            *
*  V8D, 06/05/07, GNR - Call newplt() instead of ngraph()              *
*  ==>, 02/28/08, GNR - Last mod before committing to svn repository   *
*  V8F, 02/21/10, GNR - Remove VDR support                             *
*  R72, 04/10/17, GNR - Add BUT_DEAD newplt return code                *
***********************************************************************/

#include <stdio.h>
#include <stddef.h>
#include "sysdef.h"
#include "rocks.h"
#include "bapkg.h"
#include "d3global.h"
#include "plots.h"

int d3ngph(float xsiz, float ysiz, float xorg, float yorg,
      char *pentyp, char *expcol, char *chart, int kout) {

   int rc;

   stattmr(OP_PUSH_TMR, GRPH_TMR);
   rc = newplt(xsiz, ysiz, xorg, yorg, pentyp, expcol, chart,
      kout | DO_MVEXP*bittst(RP->CP.trtimer, (long)RP->jmvtim));
   if (rc & (BUT_DEAD|BUT_QUIT)) rc = OUT_USER_QUIT;
   else if (rc & BUT_INTX)       rc = OUT_USER_PAUSE;
   stattmr(OP_POP_TMR, GRPH_TMR);

   return rc;
   }
