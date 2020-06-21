/* (c) Copyright 1987-2010, The Rockefeller University *21115* */
/* $Id: envlim.c 23 2017-01-16 19:28:30Z  $ */
/***********************************************************************
*              Darwin III Environment Simulation Package               *
*      subroutine ENVLIM:  determine maximum extent of an object       *
*                                                                      *
*  Call: envlim(Eptr,robj)                                             *
*  where: robj = pointer to object record link array                   *
*         Eptr = pointer to current environment record                 *
*                                                                      *
*  Program scans all pixels in the object, applying any rotation       *
*     that may be present, and returns the limiting coordinates,       *
*     relative to the center of the pixel array, in locations          *
*     elox(iobj),ehix(iobj),eloy(iobj),ehiy(iobj).  Each pixel         *
*     must be individually compared against the threshold.             *
************************************************************************
*  V3A, 09/04/87, GNR - Newly written                                  *
*  V4A, 01/27/89, SCD - Translated to 'C' from version V3A             *
*  ==>, 02/22/03, GNR - Last mod before committing to svn repository   *
*  V8H, 12/01/10, GNR - Separate env.h,envglob.h w/ECOLMAXCH change    *
***********************************************************************/

#define TWOPI 6.283185

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "sysdef.h"
#include "envglob.h"

int envlim(struct ENVDEF *Eptr, struct OBJDEF *robj) {

   stim_type *pxl_ptr;        /* Pointer into pixels of object        */
   float fwork;               /* Temp for rotation test               */
   si32 nx, ny;               /* Counters                             */
   si32 sinrot;               /* (S16) Sine of rotation               */
   si32 cosrot;               /* (S16) Cosine of rotation             */
/* ----------------- Local copies of various items ------------------ */
   si32 lelox;                /* Local copy of elox                   */
   si32 lehix;                /* Local copy of ehix                   */
   si32 leloy;                /* Local copy of eloy                   */
   si32 lehiy;                /* Local copy of ehiy                   */
   si32 xobjsiz;              /* Local copy of X size of object       */
   si32 yobjsiz;              /* Local copy of Y size of object       */
   stim_type bckgrnd;         /* Local copy of eilo                   */

/* Error exit if no shape found--no msg, is coding error */

   if (!robj || !robj->pshape) abexit(160);

/* Set initial values for coord limits and test threshold.  At
*  the time envlim is called, stored pixels have already been
*  expanded to density values.  Accordingly, anything above
*  'backgd' is considered to be visible.  */

   xobjsiz = robj->pshape->tsx;
   yobjsiz = robj->pshape->tsy;
   bckgrnd = Eptr->eilo;
   pxl_ptr = (stim_type *)dslpix(robj->pshape);

/* Handle the case of no rotation separately for greater speed
*
*  Test for whether mod(rot,2pi) != 0
*  It is worth doing a sophisticated test here. Idea:
*     if the angular difference from a multiple of 2pi exceeds
*     1/max(tsx,tsy)/256, then edge may move enough to
*     generate a difference of 1 density unit in some pixel,
*     and rotation is necessary.    */

   fwork = robj->orient>=0.0 ? robj->orient/TWOPI : robj->orient/-TWOPI;
   /* If difference is great enough (see comment above) go rotate. */
   if (((fwork - (long)fwork)*(max(xobjsiz,yobjsiz))) >= (1.0/256.0)) {

      si32 xcoords;      /* Starting X coord (S16) */
      si32 ycoords;      /* Starting Y coord (S16) */

/* Here if there is rotation.  Prepare sin/cos of rot to start.
*  (In this case, elo[xy] are S16 coordinates with fractions.)
*  The rotated coords are maintained by 'strength reduction',
*  i.e. coords are incremented for each pixel visited, rather
*  than multiplied when a hit is found.  (Note: the wisdom of
*  this move depends on how many black pixels are in a typical
*  shape.  Anyway, we're in fixed point, so no accuracy is lost.)
*/

      cosrot = (si32)(cos((double)robj->orient)*(double)(1L<<16));
      sinrot = (si32)(sin((double)robj->orient)*(double)(1L<<16));

/* Get rotated coords of upper left hand corner of object */

      xcoords = (-cosrot*xobjsiz - sinrot*yobjsiz)/2;
      ycoords = (-cosrot*yobjsiz + sinrot*xobjsiz)/2;

/* Initialize limits to extreme values--
*  These don't have to be exact, because if there is even
*     one non-zero pixel, it will reset them.  So, to
*     save a sqrt, we just use sum(x+y) as the diagonal.  */

      lelox = leloy = ((xcoords >= 0) ? xcoords : -xcoords) +
                      ((ycoords >= 0) ? ycoords : -ycoords);
      lehix = lehiy = -lelox;

      for (nx=1; nx<=xobjsiz; nx++) {
          si32 xcoord;    /* (S16) Current X coord   */
          si32 ycoord;    /* (S16) Current Y coord   */
          xcoord = xcoords;
          ycoord = ycoords;
          for (ny=1; ny<=yobjsiz; ny++,pxl_ptr++) {
              if (*pxl_ptr > bckgrnd) {
              /* Above--check the four limits */
                 if (xcoord < lelox) lelox = xcoord;
                 if (xcoord > lehix) lehix = xcoord;
                 if (ycoord < leloy) leloy = ycoord;
                 if (ycoord > lehiy) lehiy = ycoord;
                 }
              xcoord += sinrot;
              ycoord += cosrot;
              }  /* End of loop over pixels */
          xcoords += cosrot;
          ycoords += sinrot;
          }  /* End of loop over columns */
      }
   else {
      /* Set initial limits.  In this case (no rotation), elo[xy]
      *  are integer coordinates until shifted to S16 at end.  */
      lelox = xobjsiz;
      lehix = 1;
      leloy = yobjsiz;
      lehiy = 1;
      for (nx=1; nx<=xobjsiz; nx++ ) {
          for (ny=1; ny<=yobjsiz; ny++,pxl_ptr++) {
              if (*pxl_ptr <= bckgrnd) continue;
              /* Above--check the four limits */
              if (nx < lelox) lelox = nx;
              if (nx > lehix) lehix = nx;
              if (ny < leloy) leloy = ny;
              if (ny > lehiy) lehiy = ny;
              }  /* End of loop over pixels */
          }  /* End of loop over columns */
      xobjsiz = (xobjsiz + 1) << 15;      /* X size/2 (S16) */
      yobjsiz = (yobjsiz + 1) << 15;      /* Y size/2 (S16) */

      /* Adjust coords: Sub 1 for zero origin;
         scale to (S16) and sub half-sizes */
      robj->elox = (lelox << 16) - xobjsiz;
      robj->ehix = (lehix << 16) - xobjsiz;
      robj->eloy = (leloy << 16) - yobjsiz;
      robj->ehiy = (lehiy << 16) - yobjsiz;
      }


   robj->objflag &= ~OBJBR;      /* Clear bounds request flag */
   return ENV_NORMAL;
   }  /* End of envlim() */
