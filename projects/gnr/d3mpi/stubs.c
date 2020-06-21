/* (c) Copyright 1991-2017, The Rockefeller University *11116* */
/* $Id: stubs.c 71 2017-02-03 19:42:45Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                                stubs                                 *
*       Stubs for routines that are currently omitted from CNS         *
*                                                                      *
************************************************************************
*  V7A, 04/20/94, GNR - Move video stubs here from sunstubs, xpstubs   *
*  V7C, 01/19/95, GNR - Remove d3rprt stub, d3rprt now is implemented  *
*  V8A, 11/28/96, GNR - Remove support for nonhybrid, NCUBE video      *
*  V8B, 02/17/01, GNR - Merge all stubs into this file                 *
*  V8D, 02/13/07, GNR - Remove PNS, so stubs are no longer needed      *
*  ==>, 08/21/07, GNR - Last mod before committing to svn repository   *
*  V8F, 02/21/10, GNR - Remove stubs for PLATFORM and VDR routines     *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "d3global.h"
#ifndef PARn
#include "rocks.h"
#endif

/*---------------------------------------------------------------------*
*                     Unimplemented CNS features                       *
*---------------------------------------------------------------------*/

#ifndef PARn
void d3cnfg(int a1,s_type *a2) {
   cryout(RK_P1,"0***d3cnfg called (not implemented)",RK_LN2,NULL);
   }

int d3pxq(int waitflg) {
   return 0;
   }

void d3rpgt(FILE *a1) {
   /* When implemented, don't forget to set dpitem DPDIDL bit on
   *  all nodes when detail print completed for each CELLTYPE.
   *  Also, start any iterators that print/plot routines use.  */
   d3exit("0***d3rpgt called (not implemented)", REPLAY_ERR, 0);
   }

void npcmd(char *a1) {
   cryout(RK_P1,"0***npcmd called (not implemented)",RK_LN2,NULL);
   }

#else /* PARn */

void d3cnfg(int a1,s_type *a2) {
   return;
   }

void d3rpgt(FILE *a1) {
   return;
   }

#endif /* PARn */
