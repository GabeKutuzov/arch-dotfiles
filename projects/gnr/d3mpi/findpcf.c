/* (c) Copyright 1996-2005, The Rockefeller University *11115* */
/* $Id: findpcf.c 70 2017-01-16 19:27:55Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               findpcf                                *
*                                                                      *
*  Synopsis:  struct PCF *findpcf(txtid hpcfn)                         *
*                                                                      *
*  This function searches for a PCF table name and returns a pointer   *
*  to it.  Returns NULL and prints error message if not found.         *
************************************************************************
*  V8A, 10/08/96, GNR - Broken out from d3tree()                       *
*  V8B, 08/26/01, GNR - Use name locator instead of name itself        *
*  ==>, 03/19/05, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "rocks.h"
#include "rkxtra.h"

struct PCF *findpcf(int hpcfn) {
   struct PCF *ppcf;
   for (ppcf=RP->ppcf1; ppcf; ppcf=ppcf->pnxpcf)
      if (hpcfn == (int)ppcf->hpcfnm) break;
   if (!ppcf)
      cryout(RK_E1, "0***PCF TABLE >>", RK_LN2, getrktxt(hpcfn),
         LTEXTF, "<< NOT FOUND.", RK_CCL, NULL);
   return ppcf;
   } /* End findpcf() */

