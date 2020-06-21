/* (c) Copyright 1990-2010, The Rockefeller University *11116* */
/* $Id: envgob.c 23 2017-01-16 19:28:30Z  $ */
/***********************************************************************
*              Darwin III Environment Simulation Package               *
*                              ENVGOB.C                                *
*        Get a pointer to the first object in the object chain         *
*                                                                      *
*  Synopsis: struct OBJDEF *envgob1(struct ENVDEF *Eptr)               *
*                                                                      *
*  Argument:                                                           *
*     Eptr     Ptr to current environment                              *
*  Returns:                                                            *
*     Pointer to first object in the environment                       *
*                                                                      *
*----------------------------------------------------------------------*
*                                                                      *
*                              envgob()                                *
*                                                                      *
*  Get a pointer to an object given its reference number               *
*                                                                      *
*  Synopsis: struct OBJDEF *envgob(struct ENVDEF *Eptr, int iref)      *
*                                                                      *
*  Arguments:                                                          *
*     Eptr     Ptr to current environment                              *
*     iref     Index number of desired object                          *
*  Returns:                                                            *
*     Pointer to desired object or NULL if object does not exist       *
*                                                                      *
*----------------------------------------------------------------------*
*                                                                      *
*                               envgmxr                                *
*                                                                      *
*  Get current value of maximum reference number                       *
*                                                                      *
*  Synopsis: int envgmxr(struct ENVDEF *Eptr)                          *
*                                                                      *
*  Argument:                                                           *
*     Eptr     Ptr to current environment                              *
*  Returns:                                                            *
*     Maximum reference number of any currently allocated object       *
************************************************************************
*  V1A, 03/28/90, GNR - Initial version                                *
*  Rev, 04/13/97, GNR - New routines, help to hide ENVDEF from user    *
*  Rev, 08/29/01, GNR - Add envgmxr()                                  *
*  ==>, 02/22/03, GNR - Last mod before committing to svn repository   *
*  V8H, 12/01/10, GNR - Separate env.h,envglob.h w/ECOLMAXCH change    *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "envglob.h"

/*---------------------------------------------------------------------*
*                              envgob1()                               *
*---------------------------------------------------------------------*/

struct OBJDEF *envgob1(struct ENVDEF *Eptr) {

   return Eptr->pobj1;

   } /* End envgob1 */

/*---------------------------------------------------------------------*
*                              envgob()                                *
*---------------------------------------------------------------------*/

struct OBJDEF *envgob(struct ENVDEF *Eptr, int iref) {

   if (!Eptr || iref < 0 || iref > Eptr->mxref) return NULL;
   return Eptr->pobjrf[iref];

   }  /* End envgob() */

/*---------------------------------------------------------------------*
*                              envgmxr()                               *
*                                                                      *
*  N.B.  It won't do to just return Eptr->ipreprf, because this object *
*  may have been deleted, nor Eptr->mxref, because it is really the    *
*  size of the reference number table, which may have empty entries.   *
*---------------------------------------------------------------------*/

int envgmxr(struct ENVDEF *Eptr) {

   struct OBJDEF *pobj;       /* Ptr to an object */
   int lmxobj = 0;            /* Max object ref no. in actual use */
   for (pobj=Eptr->pobj1; pobj; pobj=pobj->pnxobj) {
      register int irfno = (int)pobj->irf;
      if (irfno > lmxobj) lmxobj = irfno; }
   return lmxobj;

   } /* End envgmxr() */
