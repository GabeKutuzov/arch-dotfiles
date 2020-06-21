/* (c) Copyright 1989-2010, The Rockefeller University *21116* */
/* $Id: envget.c 23 2017-01-16 19:28:30Z  $ */
/***********************************************************************
*              Darwin III Environment Simulation Package               *
*                               ENVGET                                 *
*              Restore an environment from a replay file               *
*                                                                      *
*  Synopsis:  int envget(Eptr, ia)                                     *
*        Eptr: pointer to current environment block                    *
*        ia: pixel information array to be filled                      *
*                                                                      *
*  Notes on reading old-style replay files (VERS=1 option): DEFUNCT    *
************************************************************************
*  V4A, 01/12/89, JWT - Translated into C                              *
*  Rev, 03/04/92, GNR - Use rf io routines                             *
*  Rev, 08/09/92, GNR - Introduce LIBHANDLE for port                   *
*  Rev, 07/20/95, GNR - Assign DFLT_ENVVAL to objects w/o user value   *
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
#include "bapkg.h"

int envget(struct ENVDEF *Eptr, stim_type *ia) {

   LIBHANDLE handle;             /* Handle for library object        */
   struct OBJDEF *iptr;          /* Ptr to object under construction */
   struct ESVDEF esv;            /* Stored environment info record   */
   struct ESVDEF *esvptr = &esv; /* Ptr to stored env info rec       */
   struct ARMDEF *pa;            /* Current arm pointer              */
   struct JNTDEF *pj;            /* Current joint pointer            */
   struct WDWDEF *pw;            /* Current window pointer           */
   struct RFdef *rpfd = Eptr->repfile;
   long i;                       /* Loop counter                     */
   short eorchk;                 /* Check for end of lib recs (-1)   */

/* Local copies of environment values */
   long ltotal1 = Eptr->ltotal;
   long lymul21 = Eptr->lymul2;
   short ly1 = Eptr->ly;

/* Open input file (use rfallo settings) */
   rfopen(rpfd,NULL,IGNORE,IGNORE,IGNORE,
      IGNORE,IGNORE,IGNORE,IGNORE,IGNORE,IGNORE,IGNORE,ABORT);

/* Restore full pixel information if signalled in kesave */
   if (Eptr->kesave) {
      for (i=0; i<ltotal1; i+=lymul21)
         rfread(rpfd,ia+i,ly1,ABORT);  }
   else {
   /* If this is first call, build the linked list */
      if (!Eptr->callnum++) {
         struct OBJDEF **ppobj = &Eptr->pobj1;
         short objnum  = Eptr->mxobj;
         if (Eptr->pobj1) goto ENVCTL_ERR;
         for (i=0; i<objnum; ppobj=&iptr->pnxobj,i++) {
            iptr = *ppobj = (struct OBJDEF *)
                mallocv(sizeof(struct OBJDEF),"ENVGET: Obj builder");
            iptr->irf = i;
            iptr->iadapt = (short)DFLT_ENVVAL;
            iptr->seltyp = iptr->shpsiz = 0;
            iptr->pshape = NULL;
            } /* End OBJ loop */
         iptr->pnxobj = NULL;
         } /* End if FIRST CALL */

   /* Clear the input array */
      memset((char *)ia, Eptr->eilo, ltotal1);

   /* Restore the objects into the input array */
      iptr = Eptr->pobj1;
      rfread(rpfd,&eorchk,sizeof(short),ABORT);
      while (eorchk != END_OF_RECS) {
         if (!iptr) goto MXOBJ_ERR;
         rfseek(rpfd,-sizeof(short),SEEKREL,ABORT);
      /* Read an object from a new replay file */
         rfread(rpfd,esvptr,sizeof(struct ESVDEF),ABORT);
      /* Find object by name in library or SHAPE list */
         if (envloc(Eptr,esvptr->shpname,SEARCH_ALL,&handle))
            cryout(RK_P1,"0--> ENVGET: Object ",RK_LN2,
               esvptr->shpname,RK_CCL+4,
               " not found... Replay still valid",RK_CCL, NULL);
         else {
            iptr->pxmin = esvptr->xmin;
            iptr->pxmax = esvptr->xmax;
            iptr->pymin = esvptr->ymin;
            iptr->pymax = esvptr->ymax;
            iptr->icurx = esvptr->xcurr;
            iptr->icury = esvptr->ycurr;
            iptr->iz = esvptr->z;
            iptr->objflag = esvptr->flag;
            iptr->orient = esvptr->orien;
            envlod(Eptr,iptr,&handle,esvptr->xsize,esvptr->ysize);
            iptr = iptr->pnxobj;
            } /* End else iq */
         envpia(Eptr,ia,iptr);
         rfread(rpfd,&eorchk,sizeof(short),ABORT);
         } /* End while eorchk loop */
      } /* End else (not full pixel info) */

/* Get the arm information */
   for (pa=Eptr->parm1; pa; pa=pa->pnxarm) {
      pj = pa->pjnt;
   /* Load the (x,y) coords */
      for (i=1; i<=pa->njnts; pj++,i++) {
         rfread(rpfd,&pj->u.jnt.jx,sizeof(float),ABORT);
         rfread(rpfd,&pj->u.jnt.jy,sizeof(float),ABORT);
         } /* End loop over joints */
      } /* End loop over arms */

/* Get the window information */
   for (pw=Eptr->pwdw1; pw; pw=pw->pnxwdw) {
      rfread(rpfd,&pw->wx,sizeof(float),ABORT);
      rfread(rpfd,&pw->wy,sizeof(float),ABORT);
      rfread(rpfd,&pw->wwd,sizeof(float),ABORT);
      rfread(rpfd,&pw->wht,sizeof(float),ABORT);
      }

/* Update current arm start position (may be window-dependent)
 * note: this updating is unnecessary with new-style input) */
   if (Eptr->parm1) {
   /* Get starting positions into (AATTX,AATTY) */
      envarm(Eptr);
      for (pa=Eptr->parm1; pa; pa=pa->pnxarm) {
         pj = pa->pjnt + (pa->njnts-1);
         pa->armx = pj->u.jnt.jx;
         pa->army = pj->u.jnt.jy;
         } /* End loop over arms */
      } /* End if arms */
      return ENV_NORMAL;

/* ERROR EXITS */
ENVCTL_ERR:
   cryout(RK_P1,"0*** ENVGET: Replay should not do envctl() call",
          RK_LN2,NULL);
   RK.iexit |= ENV_ERR;
   return ENV_FATAL;

MXOBJ_ERR:
   cryout(RK_P1,"0*** ENVGET: Replay has more than maxobj objects",
          RK_LN2,NULL);
   RK.iexit |= ENV_ERR;
   return ENV_FATAL;

   } /* End envget */


