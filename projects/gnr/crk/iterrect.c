/* (c) Copyright 2011, The Rockefeller University *11115* */
/* $Id: iterrect.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                             iterrect.c                               *
*                                                                      *
*  This module contains routines that may be used to visit in turn     *
*  all the points on some rectangular lattice that fall within a given *
*  rectangle, working in a spiral from the center to the ULHC.         *
*                                                                      *
*  The user must provide an IterRect structure which is used both to   *
*  store the internal state of the iterator routine and to return      *
*  results.  (This is less than beautifully clean, but is efficient    *
*  and will have to do until C++ comes along...)                       *
*                                                                      *
*  The user must call InitRectangleIndexing to initialize indexing for *
*  some particular rectangle.  Each time another grid point is wanted, *
*  a call to GetNextPointInRectangle returns the x,y coordinates of    *
*  the next point and its offset from the start of the lattice array.  *
*                                                                      *
*  The user may carry out multiple iterations at the same time by      *
*  providing a separate IterRect structure for each.                   *
*                                                                      *
*  The IterRect structure and the two calls are prototyped in          *
*  iterrect.h                                                          *
*                                                                      *
************************************************************************
*                                                                      *
*  METHOD:                                                             *
*  The program first identifies which is the larger and which is the   *
*  smaller dimension of the rectangle.  If the short dimension is odd, *
*  a scan is made along the center of the long dimension to make the   *
*  remainder the same in both dimensions.  The starting cell is placed *
*  at x0 = y0 = floor((n-1)/2) where n is the short dimension and the  *
*  starting scan tops (for loop with < test) are set to 2 + (n&1) for  *
*  the short dimension and n - 2*x0 for the long dimension.  The pro-  *
*  gram then enters a 4 state loop engine that terminates when the     *
*  ending x coordinate is 0.  The states are:                          *
*     (1) Increment x until x loop is finished                         *
*     (2) Leave x, increment y until y loop is finished                *
*     (3) Leave y, decrement x until x returns to original value       *
*     (4) Leave x, decrement y until y is one greater than start.      *
*  Then decrease starting coords by 1 in both directions, increment    *
*  loop lengths by 2, and repeat.                                      *
*                                                                      *
************************************************************************
*  V1A, 08/11/11, GNR - Initial version                                *
*  ==>, 08/11/11, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "iterrect.h"


/*---------------------------------------------------------------------*
*                        InitRectangleIndexing                         *
*                                                                      *
*  Synopsis:                                                           *
*     void InitRectangleIndexing(IterRect *Rect, long nsx, long nsy,   *
*        long ix0, long iy0, long nrx, long nry)                       *
*                                                                      *
*  Arguments:                                                          *
*     Rect                 Ptr to IterRect structure that pgm will use *
*                          to hold state of rectangle indexing routine.*
*     nsx,nsy              Size of full rectangle along x,y            *
*     ix0,iy0              Coordinates of ULHC of desired rectangle,   *
*                          counting from 0 in each direction.          *
*     nrx,nry              Size of desired subrectangle along x,y.     *
*                                                                      *
*  Returns:                                                            *
*     Rect is initialized for later calls to GetNextPointInRectangle   *
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

void InitRectangleIndexing(IterRect *Rect, long nsx, long nsy,
      long ix0, long iy0, long nrx, long nry) {

   long jx0 = max(ix0, 0);
   long jy0 = max(iy0, 0);
   long nux = min(nrx, nsx - jx0);
   long nuy = min(nry, nsy - jy0);
   long ymx = nuy - nux;

   if (nsx <= 0 || nsy <= 0 || nux <= 0 || nuy <= 0)
      abexitm(67, "Attempt to scan negative-edge rectangle");

   /* Save rectangle parameters */
   Rect->nsx = nsx, Rect->nsy = nsy;
   Rect->ix0 = jx0, Rect->iy0 = jy0;

   if (ymx < 0) {
      /* Set up when short axis is y */
      long yodd = nuy & 1, ylow = (nuy - 1) >> 1;
      Rect->ix1 = jx0 + ylow;
      Rect->ix2 = jx0 + nux - ylow - 1;
      Rect->iy1 = Rect->iy = jy0 + ylow;
      Rect->iy2 = Rect->iy1 - yodd + 1;
      if (yodd)
         Rect->js = SSG_Left, Rect->ix = Rect->ix2;
      else
         Rect->js = SSG_Right, Rect->ix = Rect->ix1;
      }
   else {
      /* Set up when short axis is x */
      long xodd = nux & 1, xlow = (nux - 1) >> 1;
      Rect->ix1 = Rect->ix = jx0 + xlow;
      Rect->ix2 = Rect->ix1 - xodd + 1;
      Rect->iy1 = jy0 + xlow;
      Rect->iy2 = jy0 + nuy - xlow - 1;
      if (xodd)
         Rect->js = SSG_Up, Rect->iy = Rect->iy2;
      else
         Rect->js = SSG_Right, Rect->iy = Rect->iy1;
      }

   /* Set point so one operation puts it at start of scan */
   switch (Rect->js) {
   case SSG_Right:
      Rect->ix -= 1; break;
   case SSG_Down:    /* JIC -- can't get here */
      Rect->iy -= 1; break;
   case SSG_Left:
      Rect->ix += 1; break;
   case SSG_Up:
      Rect->iy += 1; break;
      } /* End js switch */

   /* Set offset in larger rectangle to initial point */
   Rect->ioff = Rect->iy * Rect->nsx + Rect->ix;

   } /* End InitRectangleIndexing() */

/*---------------------------------------------------------------------*
*                       GetNextPointInRectangle                        *
*                                                                      *
*  Synopsis:                                                           *
*     int GetNextPointInRectangle(IterRect *Rect)                      *
*                                                                      *
*  Arguments:                                                          *
*     Rect              Ptr to IterRect structure initialized by       *
*                       previous call to InitRectangleIndexing.        *
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
*  outside the base lattice), GetNextPointinRectangle returns FALSE    *
*  at once.  This is not considered an error condition.                *
*---------------------------------------------------------------------*/

int GetNextPointInRectangle(IterRect *Rect) {

RestartNewState:
   switch (Rect->js) {

   case SSG_Right:         /* Scan right along top of rectangle */
      if (Rect->ix == Rect->ix2) {
         Rect->js = SSG_Down;
         goto RestartNewState;
         }
      Rect->ix += 1, Rect->ioff += 1;
      break;

   case SSG_Down:          /* Scan down along right of rectangle */
      if (Rect->iy == Rect->iy2) {
         Rect->js = SSG_Left;
         goto RestartNewState;
         }
      Rect->iy += 1, Rect->ioff += Rect->nsx;
      break;

   case SSG_Left:          /* Scan left along bottom of rectangle */
      if (Rect->ix == Rect->ix1) {
         Rect->js = SSG_Up;
         goto RestartNewState;
         } /* End left of bottom edge scan */
      Rect->ix -= 1, Rect->ioff -= 1;
      break;

   case SSG_Up:            /* Scan up along left of rectangle */
      /* Need this test for spirals or single columns */
      if (Rect->iy <= (Rect->iy1 + 1)) {
         Rect->js = SSG_Next;
         goto RestartNewState;
         } /* End top of left edge up scan */
      Rect->iy -= 1, Rect->ioff -= Rect->nsx;
      break;

   case SSG_Next:          /* Expand to next outer rectangle */
      /* Startup is designed so all four edges hit outer
      *  boundary at the same time -- only need to test one.  */
      if ((Rect->ix = Rect->ix1 -= 1) < Rect->ix0) {
         Rect->js = SSG_Done;
         return FALSE;
         }
      Rect->iy = Rect->iy1 -= 1;
      Rect->ix2 += 1;
      Rect->iy2 += 1;
      Rect->ioff = Rect->iy * Rect->nsx + Rect->ix;
      Rect->js = SSG_Right;
      break;

   case SSG_Done:          /* Scan complete */
      return FALSE;

      } /* End state switch */

   return TRUE;
   } /* End GetNextPointInRectangle() */
