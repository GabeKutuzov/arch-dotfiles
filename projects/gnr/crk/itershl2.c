/* (c) Copyright 2011, The Rockefeller University *11115* */
/* $Id: itershl2.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                              itershl2.c                              *
*                                                                      *
*  This module contains routines that may be used to find successively *
*  all the points on some rectangular lattice that fall in a shell     *
*  between two given concentric circles.  The center of the circles    *
*  need not fall on a grid point.  Points exactly on the outer radius  *
*  are included.  Points exactly on the inner radius are excluded.     *
*  This prevents double counting when a circle is extended in shells.  *
*                                                                      *
*  The user must provide an IterShl2 structure which is used both to   *
*  store the internal state of the iterator routine and to return      *
*  results.  (This is less than beautifully clean, but is efficient    *
*  and will have to do until C++ comes along...)                       *
*                                                                      *
*  The user calls InitShell2Indexing to initialize indexing for some   *
*  particular lattice and circles, and then calls GetNextPointInShell2 *
*  to obtain the coordinates of the next point and its offset from the *
*  start of the lattice array.                                         *
*                                                                      *
*  The user may carry out multiple iterations at the same time by      *
*  providing a serate IterShl2 structure for each.                     *
*                                                                      *
*  The IterShl2 struct and the two calls are prototyped in itershl2.h  *
*                                                                      *
************************************************************************
*  V1A, 10/17/11, GNR - Initial version, based on itershl.c            *
*  ==>, 10/24/11, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "itershl2.h"

/*---------------------------------------------------------------------*
*                          InitShell2Indexing                          *
*                                                                      *
*  Arguments:                                                          *
*     IShl2                Ptr to IterShl2 structure that may be used  *
*                          to hold state of shell indexing routine     *
*     xc,yc                Center of circle in GRID UNITS along x,y    *
*     r1,r2                Inner and outer radii of circular shell     *
*                          (r1 may be 0.0 to start before expanding)   *
*     nx,ny                Number of lattice units along x,y           *
*  Returns:                                                            *
*     IShl2 is initialized for following calls to GetNextPointInShell2 *
*                                                                      *
*  Notes:                                                              *
*     Arguments nx,ny define the size of the rectangular lattice.      *
*  These values are used for two purposes:  (1) to calculate the off-  *
*  set within the lattice to a point in the circle, and (2) to limit   *
*  the points returned to points inside the given bounds.  It is val-  *
*  id for part or all of the shell to lie outside the given lattice    *
*  bounds--points outside the bounds are never returned.               *
*                                                                      *
*  Restrictions:                                                       *
*     Currently the lattice must have right angles between the axes    *
*  and the lower bound of each axis is always taken to be zero.  The   *
*  lattice must be Euclidean (x and y intervals equal).  The itershl   *
*  routines can be used if the lattice axes are unequal (set z size    *
*  to 1 grid unit).  The ordering of indexes for computing lattice     *
*  offsets is y slow moving, then x.                                   *
*---------------------------------------------------------------------*/

void InitShell2Indexing(IterShl2 *IShl2, double xc, double yc,
      double r1, double r2, long nx, long ny) {

   /* Check for bad radii */
   if (r2 <= r1) abexit(67);

   /* Save shell parameters */
   IShl2->xc = xc;
   IShl2->yc = yc;
   IShl2->r1sq = r1*r1;
   IShl2->r2sq = r2*r2;
   IShl2->nx = nx;
   IShl2->ny = ny;

   /* Compute y limits.  Set current y to one row below bottom
   *  of circle, so increment on first GetNextPointInShell2 call
   *  will just bring y into the circle.  */
   IShl2->iymax = (long)floor(yc + r2);
   if (IShl2->iymax >= ny) IShl2->iymax = ny - 1;
   IShl2->iy = (long)ceil(yc - r2);
   if (IShl2->iy < 0) IShl2->iy = 0;
   IShl2->iy -= 1;
   IShl2->ioffrow = nx*IShl2->iy;

   /* Force new row on first GetNextPointInShell2 call */
   IShl2->ixmax = IShl2->ix2hi = IShl2->ix = 0;
   IShl2->ix2lo = 1;

   } /* End InitShell2Indexing() */

/*---------------------------------------------------------------------*
*                         GetNextPointInShell2                         *
*                                                                      *
*  Arguments:                                                          *
*     IShl2             Ptr to IterShl2 structure initialized by       *
*                       previous call to InitShell2Indexing            *
*  Returns:                                                            *
*     IShl2->ix,iy      Index values of next lattice point in shell    *
*                       along x,y directions                           *
*     IShl2->ioff       Offset of point ix,iy from origin of lattice   *
*     Function value    TRUE if a grid point was found inside the      *
*                          given shell.  Values in IShl2->ix,iy and    *
*                          ioff are valid.                             *
*                       FALSE if all grid points in the shell have     *
*                          already been returned.  Values in IShl2     *
*                          are no longer valid.                        *
*---------------------------------------------------------------------*/

int GetNextPointInShell2(IterShl2 *IShl2) {

   if (IShl2->ix >= IShl2->ixmax) {
      if (IShl2->ix2hi >= IShl2->ix2lo) {
         /* Execute the second discontinuous range along x */
         IShl2->ix = IShl2->ix2lo;
         IShl2->ixmax = IShl2->ix2hi;
         IShl2->ix2hi = -1;    /* Advance y next time */
         }
      else {
         double x,y,x2,y2;
TryAnotherY:
         if (IShl2->iy >= IShl2->iymax) return FALSE;
         /* New y, calculate new starting and ending x */
         IShl2->iy += 1;
         IShl2->ioffrow += IShl2->nx;
         y = (double)IShl2->iy - IShl2->yc;
         /* Check x bounds at outer radius */
         y2 = y*y;
         x2 = IShl2->r2sq - y2;
         x = (x2 > 0.0) ? sqrt(x2) : 0.0;
         IShl2->ixmax = (long)floor(IShl2->xc + x);
         if (IShl2->ixmax >= IShl2->nx) IShl2->ixmax = IShl2->nx - 1;
         IShl2->ix = (long)ceil(IShl2->xc - x);
         if (IShl2->ix < 0) IShl2->ix = 0;
         if (IShl2->ix > IShl2->ixmax) goto TryAnotherY;
         /* Check x bounds at inner radius */
         x2 = IShl2->r1sq - y2;
         if (x2 >= 0) {
            double q = sqrt(x2);
            long   iqmn,iqmx;
            iqmx = (long)floor(IShl2->xc + q);
            if (iqmx >= IShl2->nx) iqmx = IShl2->nx - 1;
            iqmn = (long)ceil(IShl2->xc - q);
            if (iqmn < 0) iqmn = 0;
            if (iqmx - iqmn > 1) {
               int k2 = TRUE;
               /* There will be two cuts unless one is outside
               *  the bounding rectangle  */
               if (iqmn <= IShl2->ix)
                  k2 = FALSE, IShl2->ix = iqmx;
               if (iqmx >= IShl2->ixmax)
                  k2 = FALSE, IShl2->ixmax = iqmn;
               /* If inner circle larger than outer rectangle,
               *  can be inside inner circle at this y  */
               if (IShl2->ix > IShl2->ixmax) goto TryAnotherY;
               if (k2) {
                  /* There are still 2 cuts */
                  IShl2->ix2hi = IShl2->ixmax;
                  IShl2->ix2lo = iqmx;
                  IShl2->ixmax = iqmn;
                  goto UsingInnerCut;
                  }
               }
            }
         /* X can be scanned in a single range, no inner cut */
         IShl2->ix2lo = 1;
         IShl2->ix2hi = 0;
         } /* End y update */
      } /* End update at end of x range */
   else
      /* Within a range, just advance x */
      IShl2->ix += 1;

/* X will be scanned in two ranges */
UsingInnerCut:

   /* New x, calculate lattice offset and return */
   IShl2->ioff = IShl2->ioffrow + IShl2->ix;
   return TRUE;
   } /* End GetNextPointInShell2() */
