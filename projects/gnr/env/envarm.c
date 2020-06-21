/* (c) Copyright 1989-2010, The Rockefeller University *21115* */
/* $Id: envarm.c 23 2017-01-16 19:28:30Z  $ */
/***********************************************************************
*              Darwin III Environment Simulation Package               *
*                               ENVARM                                 *
*                                                                      *
*  Updates the arm start position (AATTX,AATTY) in all arm blocks      *
*     according to fields JATT and JPOS;                               *
*  Called from ENVGET or ENVCYC                                        *
*                                                                      *
*  envarm(Eptr)                                                        *
*     Eptr = pointer to current environment block                      *
************************************************************************
*  V4A, 01/04/89, JWT - Translated to C                                *
*  ==>, 02/22/03, GNR - Last mod before committing to svn repository   *
*  V8H, 12/01/10, GNR - Separate env.h,envglob.h w/ECOLMAXCH change    *
***********************************************************************/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include "sysdef.h"
#include "envglob.h"
#include "jntdef.h"
#include "armdef.h"
#include "wdwdef.h"

#define window_att(A) ((A)->jatt & ARMAW)

int envarm(struct ENVDEF *Eptr) {

   struct WDWDEF *pw;       /* Ptr to current window */
   struct ARMDEF *pa;       /* Ptr to current arm */
   float rlx1,rly1;         /* float (X,Y) input array size */

/* Starting coordinates for all possible arm positions */
   static float xorg[NARMPOS] = { 1.0, 0.85355, 0.5, 0.14645, 0.0,
              0.14645, 0.5, 0.85355, 0.5, 0.25, 0.75, 0.5, 0.5 } ;
   static float yorg[NARMPOS] = { 0.5, 0.14645, 0.0, 0.14645, 0.5,
              0.85355, 1.0, 0.85355, 0.5, 0.5, 0.5, 0.25, 0.75 } ;

/* Assign local copies of environment values */
   rlx1 = Eptr->rlx;
   rly1 = Eptr->rly;

/* Loop over all arms */
   pw = Eptr->pwdw1;
   for (pa=Eptr->parm1; pa; pa=pa->pnxarm) {
      char jp = pa->jpos;
      if (!Eptr->pwdw1 || !window_att(pa)) /* Arm attached to frame */ {
         pa->aattx = rlx1*xorg[jp];
         pa->aatty = rly1*yorg[jp];                           }
      else /* Arm attached to first window */ {
         pa->aattx = pw->wx + pw->wwd*xorg[jp];
         pa->aatty = pw->wy + pw->wht*(yorg[jp]-1.0);       }
      } /* End loop over arms */

  return ENV_NORMAL;

   } /* End envarm() */

