/* (c) Copyright 2012-2013, The Rockefeller University *11115* */
/* $Id: itertape.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                             itertape.c                               *
*                                                                      *
*  This module contains routines that may be used to scan along a rec- *
*  tangular "tape" that falls at some angle on a rectangular lattice.  *
*  The tape may be divided into smaller rectangular boxes so as to be  *
*  able to terminate the scan when all objects of interest have been   *
*  found.   The program carefully returns points that lie exactly on   *
*  the lower and side borders, but not on the leading edge, so as to   *
*  avoid duplicates when the tape is extended.                         *
*                                                                      *
*  All coordinates are integer grid coordinates starting from 0 in the *
*  ULHC, with x to the right and y down. (Angles internally are clock- *
*  wise from horizontal.) Results are presented x fast moving, y slow. *
*                                                                      *
*  The user must provide an instance of the IterTape data structure    *
*  for each invocation of the routines.  This structure is used both   *
*  to store the internal state of the iterator and to return results.  *
*                                                                      *
*  The user must call InitTapeIndexing to initialize indexing for some *
*  particular tape, and ExtendTapeIndexing to add another rectangular  *
*  section to the end of the tape.  Each time a point on the tape is   *
*  wanted, a call to GetNextPointOnTape returns the x,y coordinates    *
*  of the next point and its offset from the start of the underlying   *
*  lattice array.                                                      *
*                                                                      *
*  The IterTape structure and the two calls are prototyped in          *
*  itertape.h                                                          *
*                                                                      *
*  This version works with coordinates of type float.  Another version *
*  will need to be written if double precision coordinates are needed. *
*  It may also be desirable to have a version that multiplies the den- *
*  sities in each box by some shape-recognition kernel.                *
************************************************************************
*  V1A, 02/17/12, GNR - Initial version                                *
*  Rev, 03/07/12, GNR - More robust boundary tests                     *
*  ==>, 03/07/12, GNR - Last date before committing to svn repository  *
*  Rev, 03/14/13, GNR - Correct gxmn,gxmx limits setting bug           *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "itertape.h"

#define BigCoord  1.0E8
#define RoundErr  1.0E-6
#define TooSmall  0.5

/*---------------------------------------------------------------------*
*                          InitTapeIndexing                            *
*                                                                      *
*  Synopsis:                                                           *
*     void InitTapeIndexing(IterTape *T, xyf *pb1, xyf *pb2,           *
*        float hgt, long nsx, long nsy)                                *
*                                                                      *
*  Arguments:                                                          *
*     T                 Ptr to IterTape structure that pgm will use to *
*                       hold state of tape indexing routine.           *
*     pb1,pb2           Ptrs to pairs of x,y coordinates defining the  *
*                       base of the tape.  The direction of tape ex-   *
*                       tension will be along a direction 90 degrees   *
*                       clockwise to a line from b1 to b2.             *
*     hgt               Height of the first scan box along the tape.   *
*     nsx,nsy           Size of full rectangular lattice along x,y.    *
*                                                                      *
*  Returns:                                                            *
*     T is initialized for later calls to GetNextPointOnTape and       *
*  ExtendTape Indexing                                                 *
*                                                                      *
*  Notes:                                                              *
*     Arguments nsx,nsy define the size of a full rectangular lattice, *
*  on which the required tape is placed.  These values are used for    *
*  two purposes: (1) to calculate the offset within the full lattice   *
*  to each point on the desired tape, and (2) to limit the points      *
*  returned to points inside the given bounds.  It is valid for part   *
*  or all of the tape to lie outside the given bounds--points outside  *
*  the bounds are never returned.                                      *
*     The contents of T should not be changed by the user until the    *
*  entire tape scan is complete (or abandoned).                        *
*---------------------------------------------------------------------*/

void InitTapeIndexing(IterTape *T, xyf *pb1, xyf *pb2,
      float hgt, long nsx, long nsy) {

   float tdx,tdy,td;          /* Projection of base on x,y */
   float adx,ady;             /* abs(tdx), abs(tdy) */

   /* Sanity checking */
   if (nsx <= 0 || nsy <= 0)
      abexitm(67, "Lattice edge <= 0 for tape indexing");
   tdx = pb2->x - pb1->x, tdy = pb2->y - pb1->y;
   if ((td = tdx*tdx + tdy*tdy) < TooSmall)
      abexitm(67, "Tape width too small for indexing");
   td = 1.0/sqrtf(td);

   /* Save input and derived parameters */
   T->bxy[3] = *pb1, T->bxy[2] = *pb2;
   T->xmx = T->nsx = nsx, T->nsy = nsy;
   T->topx = (float)(nsx - 1);
   T->bcos = tdx*td, T->bsin = tdy*td;
   adx = fabsf(tdx), ady = fabsf(tdy);
   /* If slope is BigCoord, sign doesn't matter --
   *  value is just used as a test to skip this edge */
   T->slope[0] = (adx > BigCoord*ady) ? BigCoord : tdx/tdy;
   T->slope[1] = (ady > BigCoord*adx) ? BigCoord : -tdy/tdx;

   /* Set up for first scan box */
   ExtendTapeIndexing(T, hgt);

   } /* End InitTapeIndexing() */


/***********************************************************************
*                         ExtendTapeIndexing                           *
*                                                                      *
*  Synopsis:                                                           *
*     int ExtendTapeIndexing(IterTape *T, float hgt)                   *
*                                                                      *
*  Arguments:                                                          *
*     T                 Ptr to IterTape structure initialized by       *
*                       a previous call to InitTapeIndexing.           *
*     hgt               Height of the next scan box along the tape.    *
*                                                                      *
*  Returns:             TRUE if a box of the given height can still    *
*                          be fit at least partially within the given  *
*                          lattice                                     *
*                       FALSE if the tape now extends entirely outside *
*                          the given lattice, i.e. any further calls   *
*                          to GetNextPointOnTape will return FALSE.    *
***********************************************************************/

int ExtendTapeIndexing(IterTape *T, float hgt) {

   float hdx,hdy;             /* Projection of height on x,y */
   float fxmn,fxmx;           /* Lowest, highest vertex x */
   float fymn,fymx;           /* Lowest, highest vertex y */
   long  ymn,ymx;             /* Lowest, highest vertex y */
   int   iv,iv2;              /* Vertex loop indices */

   /* Immediate exit if x bounds test below indicated the tape
   *  will never pass inside the x bounds */
   if (T->xmx < 0) return FALSE;

   /* Make old top of box new bottom */
   T->bxy[0] = T->bxy[3], T->bxy[1] = T->bxy[2];

   /* Create new top of box */
   hdx = -hgt*T->bsin, hdy = hgt*T->bcos;
   T->bxy[2].x += hdx, T->bxy[2].y += hdy;
   T->bxy[3].x += hdx, T->bxy[3].y += hdy;

   /* Save low and high x,y coords for scan box */
   fymn = fxmn = BigCoord, fymx = fxmx = -BigCoord;
   ymn = T->nsy, ymx = -1;    /* Just out-of-bounds */
   for (iv=0; iv<NVQ; ++iv) {
      float x1,x2,y1,y2,tx,ty;
      iv2 = (iv+1) & (NVQ-1);
      x1 = T->bxy[iv].x, x2 = T->bxy[iv2].x;
      y1 = T->bxy[iv].y, y2 = T->bxy[iv2].y;
      if (x1 < fxmn) fxmn = x1;
      if (x1 > fxmx) fxmx = x1;
      if (x1 > x2) tx = x1, x1 = x2, x2 = tx;
      T->lox[iv] = x1 - RoundErr;
      T->hix[iv] = x2 + RoundErr;
      if (y1 < fymn) fymn = y1;
      if (y1 > fymx) fymx = y1;
      if (y1 > y2) ty = y1, y1 = y2, y2 = ty;
      /* If this line never crosses a lattice line, set loy
      *  very large so the line is effectively skipped.  */
      ty = floorf(y2);
      T->loy[iv] = (floorf(y1) == ty) ? LONG_MAX : (long)ceilf(y1);
      T->hiy[iv] = (long)ty;
      } /* End vertex loop */

   /* If no x is in bounds or could be in bounds on future ex-
   *  tensions (i.e. moving right if bsin < 0, left if bsin > 0),
   *  then set to always return FALSE and return FALSE now.  */
   if (T->bsin < 0.0) {
      if (fxmn > T->topx) { T->xmx = -1; return FALSE; }
      }
   else {
      if (fxmx < 0.0) { T->xmx = -1; return FALSE; }
      }

   /* Set y bounds, starting x,y, and y offset */
   ymn = (long)ceilf(fymn);
   ymx = (long)floorf(fymx);
   if (ymn < 0) ymn = 0;
   if (ymx >= T->nsy) ymx = T->nsy - 1;
   T->ymn = ymn, T->ymx = ymx;
   T->ix = (T->xmx = T->nsx) + 1;   /* Force new y first time */
   T->iy = ymn - 1;
   T->yoff = T->nsx * T->iy;

   return (ymn < T->nsy);

   } /* End Extend Tape Indexing() */


/*---------------------------------------------------------------------*
*                         GetNextPointOnTape                           *
*                                                                      *
*  Synopsis:                                                           *
*     int GetNextPointOnTape(IterTape *T)                              *
*                                                                      *
*  Arguments:                                                          *
*     T                 Ptr to IterTape structure initialized by       *
*                       a previous call to InitTapeIndexing.           *
*  Returns:                                                            *
*     T->ix,iy          x,y coords of next lattice point on tape.      *
*     T->ioff           Offset of point ix,iy from lattice origin.     *
*     Function value    TRUE if a grid point was found inside the      *
*                          current scan box.  Values in T->ix,iy,      *
*                          and ioff are valid.                         *
*                       FALSE if all grid points in the scan box have  *
*                          already been returned.  Values in T->ix,    *
*                          iy, and ioff are no longer valid.           *
*  Note:                                                               *
*     If no points are eligible (because current scan box of specified *
*  tape lies outside the base lattice), GetNextPointOnTape returns     *
*  FALSE at once.  This is not considered an error condition.          *
*---------------------------------------------------------------------*/

int GetNextPointOnTape(IterTape *T) {

   float gxmn,gxmx;           /* Global min,max x */
   float fty;                 /* (float)ty */
   long  ty;                  /* Current y */
   int   iv;                  /* Vertex loop indices */

TryNextX:
   if (T->ix >= T->xmx) {
TryNextY:
      if (T->iy >= T->ymx) return FALSE;
      /* Start a new row--check intersections with all edges */
      gxmn = T->topx + 1.0, gxmx = -1.0;
      fty = (float)(ty = T->iy += 1);
      T->yoff += T->nsx;
      for (iv=0; iv<NVQ; ++iv) {
         float tix,vtan;
         if (ty < T->loy[iv] || ty > T->hiy[iv]) continue;
         /* If the slope is BigCoord, either the edge never crosses a
         *  lattice line and so is irrelevant, or it is horizontal and
         *  falls right on a lattice line, in which case both ends will
         *  get tested as solutions for the preceding and following
         *  lines.  */
         if ((vtan = T->slope[iv&1]) == BigCoord) continue;
         tix = T->bxy[iv].x + vtan * (fty - T->bxy[iv].y);
         /* Check tix against lox and hix for this line.  Although
         *  in principle tests on y guarantee that tix is within the
         *  line, in testing, deviations were found due to accumula-
         *  tion of round-off errors in successive tape extensions.  */
         if (tix < T->lox[iv] || tix > T->hix[iv]) continue;
         /* It is also necessary to modify x ranges that are outside
         *  the lattice.  gxmn,gxmx are used for this purpose.  */
         if (tix < gxmn) gxmn = tix;
         if (tix > gxmx) gxmx = tix;
         } /* End vertex loop */
      /* Protect against the perverse case where the whole
      *  line is out-of-bounds */
      if (gxmn > T->topx || gxmx < 0.0) goto TryNextY;
      /* Now set x loop and impose lattice bounds.
      *  (A small round-off fix is made here)  */
      T->ix = (long)ceilf(max(gxmn-RoundErr,0.0));
      T->xmx = (long)floorf(min(gxmx,T->topx));
      if (T->ix > T->xmx) goto TryNextY;
      T->ybcos = (fty - T->bxy[2].y)*T->bcos;
      }
   else
      T->ix += 1;

   /* If this point lies exactly on the top base line, it
   *  is excluded.  Because slope can be + or -, this could
   *  occur at the start or end of the current y row.  */
   if (fabsf(T->ybcos - (float)T->ix*T->bsin) < RoundErr)
      goto TryNextX;

   /* Compute offset of this point in the overall lattice */
   T->ioff = T->yoff + T->ix;
   return TRUE;
   } /* End GetNextPointOnTape() */
