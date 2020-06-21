/* (c) Copyright 2011, The Rockefeller University *11115* */
/* $Id: itercirc.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                             itercirc.c                               *
*                                                                      *
*  This module contains routines that may be used to find successively *
*  all the points on some rectangular lattice that fall within a given *
*  circle.  The center of the circle need not fall on a grid point.    *
*                                                                      *
*  The user calls InitCircleIndexing to initialize indexing for some   *
*  particular lattice and circle, and then calls GetNextPointInCircle  *
*  to obtain the coordinates of the next point and its offset from the *
*  start of the lattice array.                                         *
*                                                                      *
*  The user must provide an IterCirc structure which is used both to   *
*  store the internal state of the iterator routine and to return      *
*  results.  (This is less than beautifully clean, but is efficient    *
*  and will have to do until C++ comes along...)                       *
*                                                                      *
*  The user may carry out multiple iterations at the same time by      *
*  providing a separate IterCirc structure for each.                   *
*                                                                      *
*  The IterCirc structure and the calls are prototyped in itercirc.h.  *
*                                                                      *
************************************************************************
*  V1A, 08/13/11, GNR - Initial version                                *
*  ==>, 08/13/11, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "itercirc.h"

/*---------------------------------------------------------------------*
*                         InitCircleIndexing                           *
*                                                                      *
*  Synopsis:                                                           *
*     void InitCircleIndexing(IterCirc *Circ, double xc, double yc,    *
*        double radius, long nx, long ny)                              *
*                                                                      *
*  Arguments:                                                          *
*     Circ                 Ptr to IterCirc structure that may be used  *
*                          to hold state of circle indexing routine.   *
*     xc,yc                Center of circle in grid units along x,y.   *
*     radius               Radius of circle in grid units.             *
*     nx,ny                Number of lattice units along x,y.          *
*                                                                      *
*  Returns:                                                            *
*     Circ is initialized for later calls to GetNextPointInCircle.     *
*                                                                      *
*  Notes:                                                              *
*     Arguments nx,ny define the size of the rectangular lattice.      *
*  These values are used for two purposes: (1) to calculate the offset *
*  within the lattice to a point in the circle, and (2) to limit the   *
*  points returned to points inside the given bounds.  It is valid for *
*  part or all of the circle to lie outside the given lattice bounds-- *
*  points outside the bounds are never returned.                       *
*                                                                      *
*  Restrictions:                                                       *
*     Currently the lattice must be Euclidean and the lower bound of   *
*  each axis is always taken to be zero.  The itersph routines can be  *
*  used if the lattice axes are unequal (set z size to 1 grid unit).   *
*  The ordering of indexes for computing lattice offsets is y slow     *
*  moving, then x.                                                     *
*---------------------------------------------------------------------*/

void InitCircleIndexing(IterCirc *Circ, double xc, double yc,
      double radius, long nx, long ny) {

   /* Sanity checks */
   if (nx <= 0 || ny <= 0 || radius <= 0.0)
      abexitm(67, "Attempt to scan negative circle");

   /* Save circle parameters */
   Circ->xc = xc, Circ->yc = yc;
   Circ->radius = radius;
   Circ->r2 = radius*radius;
   Circ->nx = nx, Circ->ny = ny;

   /* Force new row on first GetNextPointInCircle call */
   Circ->ix = Circ->mx = nx;
   Circ->iy = (long)(ceil(yc - radius)) - 1;
   if (Circ->iy < -1) Circ->iy = -1;
   Circ->my = (long)(floor(yc + radius));
   if (Circ->my >= Circ->ny) Circ->my = Circ->ny - 1;

   } /* End InitCircleIndexing */

/*---------------------------------------------------------------------*
*                        GetNextPointInCircle                          *
*                                                                      *
*  Synopsis:                                                           *
*     int GetNextPointInCircle(IterCirc *Circ)                         *
*                                                                      *
*  Arguments:                                                          *
*     Circ             Ptr to IterCirc structure initialized by        *
*                       previous call to InitCircleIndexing            *
*  Returns:                                                            *
*     Circ->ix,iy      Index values of next lattice point in circle    *
*                       along x,y directions.                          *
*     Circ->ioff       Offset of point ix,iy from origin of lattice.   *
*     Function value    TRUE if a grid point was found inside the      *
*                          given circle.  Values in Circ->ix,iy,ioff   *
*                          are valid.                                  *
*                       FALSE if all grid points in the circle have    *
*                          already been returned.  Values in Circ      *
*                          are no longer valid.                        *
*---------------------------------------------------------------------*/

int GetNextPointInCircle(IterCirc *Circ) {

   /* The loop is in case there are no acceptable x's in the current
   *  y row--unlikely, but possible.  Also, code is now arranged to
   *  test before incrementing, so it will always return FALSE no
   *  matter how many calls come after the end is reached.  */
   while (Circ->ix >= Circ->mx) {
      if (Circ->iy >= Circ->my) return FALSE;
      Circ->iy += 1;

      /* New y, calculate new starting and ending x */
      {  double yr = (double)Circ->iy - Circ->yc;
         double xr = sqrt(Circ->r2 - yr*yr);
         Circ->mx = (long)(floor(Circ->xc + xr));
         if (Circ->mx >= Circ->nx) Circ->mx = Circ->nx - 1;
         Circ->ix = (long)(ceil(Circ->xc - xr)) - 1;
         if (Circ->ix < -1) Circ->ix = -1;
         } /* End x limits local scope */
      Circ->iofrow = Circ->iy * Circ->nx;
      } /* End y update */

   /* New point, calculate lattice offset and return */
   Circ->ioff = Circ->iofrow + (Circ->ix += 1);
   return TRUE;
   } /* End GetNextPointInCircle() */
