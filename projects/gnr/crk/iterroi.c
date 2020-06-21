/* (c) Copyright 2013, The Rockefeller University *11115* */
/* $Id: iterroi.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                              iterroi.c                               *
*                                                                      *
*  This module contains routines that may be used to visit in turn     *
*  all the points on some rectangular lattice that fall within a given *
*  rectangle, working in a spiral from ULHC to the center.             *
*                                                                      *
*  The user must provide an IterRoi structure which is used both to    *
*  store the internal state of the iterator routine and to return      *
*  results.                                                            *
*                                                                      *
*  The user must call InitOIRectIndexing to initialize indexing for    *
*  some particular rectangle.  Each time another grid point is wanted, *
*  a call to GetNextPointInOIRect returns the x,y coordinates of the   *
*  next point and its offset from the start of the lattice array.      *
*                                                                      *
*  The user may carry out multiple iterations at the same time by      *
*  providing a separate IterRoi structure for each.                    *
*                                                                      *
*  The IterRoi structure and the two calls are prototyped in iterroi.h *
*                                                                      *
************************************************************************
*  V1A, 02/02/13, GNR - Initial version                                *
*  ==>, 02/02/13, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "iterroi.h"


/*---------------------------------------------------------------------*
*                         InitOIRectIndexing                           *
*                                                                      *
*  Synopsis:                                                           *
*     void InitOIRectIndexing(IterRoi *Rect, long nsx, long nsy,       *
*        long ix0, long iy0, long nrx, long nry)                       *
*                                                                      *
*  Arguments:                                                          *
*     Rect                 Ptr to IterRoi structure that pgm will use  *
*                          to hold state of rectangle indexing routine.*
*     nsx,nsy              Size of full rectangle along x,y            *
*     ix0,iy0              Coordinates of ULHC of desired rectangle,   *
*                          counting from 0 in each direction.          *
*     nrx,nry              Size of desired subrectangle along x,y.     *
*                                                                      *
*  Returns:                                                            *
*     Rect is initialized for later calls to GetNextPointInOIRect      *
*                                                                      *
*  Notes:                                                              *
*     Arguments nsx,nsy define the size of a full rectangular lattice, *
*  of which the required rectangle is a sublattice.  These values are  *
*  used for two purposes: (1) to calculate the offset within the full  *
*  lattice to a point in the desired rectangle, and (2) to limit the   *
*  points returned to points inside the given bounds.  It is valid for *
*  part or all of the rectangle to lie outside the given bounds--      *
*  points outside the bounds are never returned.                       *
*---------------------------------------------------------------------*/

void InitOIRectIndexing(IterRoi *Rect, long nsx, long nsy,
      long ix0, long iy0, long nrx, long nry) {

   /* Check for invalid parameters */
   if (nsx <= 0 || nsy <= 0 || nrx <= 0 || nry <= 0)
      abexitm(67, "Attempt to scan negative-edge rectangle");

   /* Save rectangle parameters */
   Rect->nsx = nsx, Rect->nsy = nsy;
   Rect->ix1 = max(ix0, 0);
   Rect->ix2 = min(ix0 + nrx, nsx) - 1;
   Rect->iy1 = max(iy0, 0);
   Rect->iy2 = min(iy0 + nry, nsy) - 1;

   /* Check for rectangle with no points on lattice */
   if (Rect->ix1 > Rect->ix2 || Rect->iy1 > Rect->iy2)
      Rect->js = RSG_Done;

   /* Set current point so one operation puts it at start of scan
   *  and set offset in larger rectangle to initial point */
   else {
      Rect->ix = Rect->ix1 - 1, Rect->iy = Rect->iy1;
      Rect->js = RSG_Right;
      Rect->ioff = Rect->iy * Rect->nsx + Rect->ix;
      }

   } /* End InitOIRectIndexing() */

/*---------------------------------------------------------------------*
*                        GetNextPointInOIRect                          *
*                                                                      *
*  Synopsis:                                                           *
*     int GetNextPointInOIRect(IterRoi *Rect)                          *
*                                                                      *
*  Arguments:                                                          *
*     Rect              Ptr to IterRoi structure initialized by        *
*                       previous call to InitOIRectIndexing.           *
*  Returns:                                                            *
*     Rect->ix,iy       x,y coords of next lattice point in rectangle. *
*     Rect->ioff        Offset of point ix,iy from lattice origin.     *
*     Function value    TRUE if a grid point was found inside the      *
*                          given rectangle.  Values in Rect->ix,iy,    *
*                          and ioff are valid.                         *
*                       FALSE if all grid points in the rectangle have *
*                          already been returned.  Values in Rect->ix, *
*                          iy, and ioff are no longer valid.           *
*  Note:                                                               *
*     If no points are eligible (because requested rectangle lies      *
*  outside the base lattice), GetNextPointInOIRect returns FALSE at    *
*  once.  This is not considered an error condition.                   *
*---------------------------------------------------------------------*/

int GetNextPointInOIRect(IterRoi *Rect) {

RestartNewState:
   switch (Rect->js) {

   case RSG_Right:         /* Scan right along top of rectangle */
      if (Rect->ix == Rect->ix2) {
         Rect->iy1 += 1;
         Rect->js = Rect->iy1 > Rect->iy2 ? RSG_Done : RSG_Down;
         goto RestartNewState;
         }
      Rect->ix += 1, Rect->ioff += 1;
      break;

   case RSG_Down:          /* Scan down along right of rectangle */
      if (Rect->iy == Rect->iy2) {
         Rect->ix2 -= 1;
         Rect->js = Rect->ix2 < Rect->ix1 ? RSG_Done : RSG_Left;
         goto RestartNewState;
         }
      Rect->iy += 1, Rect->ioff += Rect->nsx;
      break;

   case RSG_Left:          /* Scan left along bottom of rectangle */
      if (Rect->ix == Rect->ix1) {
         Rect->iy2 -= 1;
         Rect->js = Rect->iy2 < Rect->iy1 ? RSG_Done : RSG_Up;
         goto RestartNewState;
         } /* End left of bottom edge scan */
      Rect->ix -= 1, Rect->ioff -= 1;
      break;

   case RSG_Up:            /* Scan up along left of rectangle */
      if (Rect->iy == Rect->iy1) {
         Rect->ix1 += 1;
         Rect->js = Rect->ix1 > Rect->ix2 ? RSG_Done : RSG_Right;
         goto RestartNewState;
         } /* End top of left edge up scan */
      Rect->iy -= 1, Rect->ioff -= Rect->nsx;
      break;

   case RSG_Done:          /* Scan complete */
      return FALSE;

      } /* End state switch */

   return TRUE;
   } /* End GetNextPointInOIRect() */
