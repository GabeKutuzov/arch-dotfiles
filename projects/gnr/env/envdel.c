/* (c) Copyright 1990-2010, The Rockefeller University *11116* */
/* $Id: envdel.c 23 2017-01-16 19:28:30Z  $ */
/***********************************************************************
*              Darwin III Environment Simulation Package               *
*                               ENVDEL                                 *
*                   Delete object from object list                     *
*                                                                      *
*  void envdel(struct ENVDEF *Eptr, struct OBJDEF *obj, *prev)         *
*                                                                      *
*     Eptr: ptr to current environment                                 *
*      obj: ptr to object to delete                                    *
*     prev: ptr to previous object in list (NULL if obj is header)     *
************************************************************************
*  V1A, 03/28/90, GNR - Initial version                                *
*  ==>, 02/22/03, GNR - Last mod before committing to svn repository   *
*  V8H, 12/01/10, GNR - Separate env.h,envglob.h w/ECOLMAXCH change    *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "envglob.h"
#include "rocks.h"

void envdel(struct ENVDEF *Eptr, OBJECT *obj, OBJECT *prev) {

/* Release storage for matrix and permutation, if any */
   if (obj->shpsiz) freev(obj->pshape,"ENVDEL: pshape");
   if (obj->seltyp == LIB_PERM || obj->seltyp == PIX_PERM)
      freev(obj->sel.psperm,"ENVDEL: psperm");

/* Remove lookup table entry */
   Eptr->pobjrf[obj->irf] = NULL;

/* Remove object from list and release storage */
   if (prev) prev->pnxobj = obj->pnxobj;
   else /* Delete object is list header */ Eptr->pobj1 = obj->pnxobj;
   freev(obj,"ENVDEL: object");

   }  /* End ENVDEL */
