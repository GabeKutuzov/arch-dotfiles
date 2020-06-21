/* (c) Copyright 1989-2010, The Rockefeller University *21115* */
/* $Id: envsav.c 23 2017-01-16 19:28:30Z  $ */
/***********************************************************************
*              Darwin III Environment Simulation Package               *
*                               ENVSAV                                 *
*                                                                      *
*  Save current environment configuration (objects, arms, & window)    *
*                                                                      *
*  int envsav(Eptr,ia)                                                 *
*             Eptr: pointer to current environment block               *
*               ia: pixel information array                            *
************************************************************************
*  V4A, 01/12/89, JWT - Translated into C                              *
*  Rev, 03/04/92, GNR - Use rf io routines                             *
*  Rev, 02/22/03, GNR - Change s_type to stim_type                     *
*  ==>, 02/22/03, GNR - Last mod before committing to svn repository   *
*  V8H, 12/01/10, GNR - Separate env.h,envglob.h w/ECOLMAXCH change    *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "envglob.h"
#include "jntdef.h"
#include "armdef.h"
#include "wdwdef.h"
#include "rocks.h"
#include "rkxtra.h"

int envsav(struct ENVDEF *Eptr, stim_type *ia) {

   struct ESVDEF esvptr;    /* Pointer to record where env info stored */
   struct ARMDEF *pa;       /* Pointer to current arm     */
   struct JNTDEF *pj;       /* Pointer to current joint   */
   struct WDWDEF *pw;       /* Pointer to current window  */
   struct RFdef *pofl = Eptr->repfile; /* Ptr to output file */
   long i;                  /* Loop counter */
   short rec_end = END_OF_RECS;  /* Addressable (&) END_OF_RECS */

/* Local copies of environment values */
   long  ltotal1 = Eptr->ltotal;
   long  lymul21 = Eptr->lymul2;
   short ly1 = Eptr->ly;

/* Open output file (use rfallo settings) */
   rfopen(pofl,NULL,IGNORE,IGNORE,IGNORE,
      IGNORE,IGNORE,IGNORE,IGNORE,IGNORE,IGNORE,IGNORE,ABORT);

/* Save the pixel information if requested in ESVFIA */
   if (Eptr->kesave == ENV_SAVALL) {
      for (i=0; i<ltotal1; i+=lymul21)
         rfwrite(pofl,ia,ly1,ABORT);   }
   else {
   /* If don't need to save the individual pixels, it is more
    *    economical just to save the information needed to recreate
    *    the object.  In V3A, this was changed to save the object
    *    name rather than the number, the z coord, and the boundary
    *    condition info, so reconstruction can be accurate.  ENVGET
    *    will have the ability to read the old type of file if
    *    VERS=1 specified on the REPLAY card, but there is no need
    *    to retain here the ability to write the old format.
    *
    * The following items are saved in this order (in NEW_SAVREC):
    *    1) NSHP = object name         (4 characters)
    *    2) PXMIN, PXMAX, PYMIN, PYMAX (4 shorts)
    *    3) (TSX,TSY)                  (2 shorts)
    *    4) current (X,Y)              (2 longs)
    *    5) Z                          (1 short)
    *    6) OBJFLG                     (1 short)
    *    7) ORIENT = rotation angle    (1 float)
    */
      struct OBJDEF *Obj;
      struct DSLBDEF *shptr;
      short itotal = 0;
      for (Obj=Eptr->pobj1; Obj; Obj=Obj->pnxobj) {
         char *nameptr = (shptr = Obj->pshape)->nshp;
         strncpy(esvptr.shpname,nameptr,SHP_NAME_SIZE);
         esvptr.xmin  = Obj->pxmin;
         esvptr.xmax  = Obj->pxmax;
         esvptr.ymin  = Obj->pymin;
         esvptr.ymax  = Obj->pymax;
         esvptr.xsize = shptr->tsx;
         esvptr.ysize = shptr->tsy;
         esvptr.xcurr = Obj->icurx;
         esvptr.ycurr = Obj->icury;
         esvptr.z     = Obj->iz;
         esvptr.flag  = Obj->objflag;
         esvptr.orien = Obj->orient;
         rfwrite(pofl,&esvptr,sizeof(struct ESVDEF),ABORT);
         if (++itotal > Eptr->mxobj) goto MXOBJ_ERR;
         } /* End OBJ loop */

   /* Indicate end of object list in replay file */
      rfwrite(pofl,(short *)&rec_end,sizeof(short),ABORT);
      } /* End else */

/* Save arm info */
   for (pa=Eptr->parm1; pa; pa=pa->pnxarm) {
   /* In V3A, compatible writing of old-style arm information (angles
    *    instead of coords) was removed.  ENVGET will distinguish the
    *    two formats according to presence of universal joint only if
    *    VERS=1 specified on REPLAY card, otherwise expects coords.
    */
      char njnts1 = pa->njnts;
      pj = pa->pjnt;
      for (i=0; i<njnts1; pj++,i++) {
         rfwrite(pofl,&pj->u.jnt.jx,sizeof(float),ABORT);
         rfwrite(pofl,&pj->u.jnt.jy,sizeof(float),ABORT);
         }
      }

/* Save window info */
   for (pw=Eptr->pwdw1; pw; pw=pw->pnxwdw)  {
      rfwrite(pofl,&pw->wx,sizeof(float),ABORT);
      rfwrite(pofl,&pw->wy,sizeof(float),ABORT);
      rfwrite(pofl,&pw->wwd,sizeof(float),ABORT);
      rfwrite(pofl,&pw->wht,sizeof(float),ABORT);
      }

   return ENV_NORMAL;

/* ERROR RETURNS */

MXOBJ_ERR:
   cryout(RK_P1,"0*** ENVSAV: Only MXOBJ objects recorded",RK_LN2,NULL);
   RK.iexit |= ENV_ERR;
   return ENV_FATAL;

   } /* End envsav */
