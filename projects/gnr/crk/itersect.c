/* (c) Copyright 2014, The Rockefeller University *11115* */
/* $Id: itersect.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                             itersect.c                               *
*                                                                      *
*  This module contains routines that may be used to find successively *
*  all the points on some rectangular lattice that fall within a given *
*  sector of a given circle.  The points that define the sector need   *
*  not fall on grid points.                                            *
*                                                                      *
*  This version works with coordinates of type float.  Another version *
*  will need to be written if double precision coordinates are needed. *
*                                                                      *
*  The user calls InitSectorIndexing to initialize indexing for some   *
*  particular lattice and sector, and then repeatedly calls GetNext-   *
*  PointInSector to obtain the coordinates of the next point and its   *
*  offset from the start of the lattice array.                         *
*                                                                      *
*  The user must provide an IterSect structure which is used both to   *
*  store the internal state of the iterator routine and to return      *
*  results.  (This is less than beautifully clean, but is efficient    *
*  and will have to do until C++ comes along...)                       *
*                                                                      *
*  The user may carry out multiple iterations at the same time by      *
*  providing a separate IterSect structure for each.                   *
*                                                                      *
*  The IterSect structure and the calls are prototyped in itersect.h.  *
*                                                                      *
*  Notes:                                                              *
*  --This code is written so normal compilation produces the normal    *
*  version for the crk library.  If NO_CRKLIB is defined at compile    *
*  time, no features of the crk library or its include files are used. *
*  This produces a version suitable for inclusion in MATLAB mex files. *
*  In this case, only systems with 64 bit integers arithmetic are      *
*  supported.  The crk library version can compile for 32 or 64 bits.  *
*  --This code can handle either left-handed coordinate systems (the   *
*  normal crk library convention for tv scans with low y at the top)   *
*  or right-handed coordinate systems (as with MATLAB images where x   *
*  is down and y is to the right.  The code is the same provided the   *
*  caller understands that the sector arc rotation is clockwise with   *
*  right-handed coordinates and counterclockwise with left-handed.     *
*  --Coordinates are stored in the IterSect structure as 32-bit fixed- *
*  point values with 16 fraction bits.  This facilitates checking for  *
*  exact grid points but restricts grid size to < 32768 rows or cols.  *
*  --Arguments nx,ny and returns ioff,ix,iy are longs to agree with    *
*  other geometric iterators in the crk library.  Nonetheless, nx,ny   *
*  are restricted to < 32768 rows or cols as just explained.           *
************************************************************************
*  V1A, 11/15/14, GNR - Initial version, based on itercirc             *
*  ==>, 11/21/14, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "itersect.h"

/*---------------------------------------------------------------------*
*                         InitSectorIndexing                           *
*                                                                      *
*  Synopsis:                                                           *
*     ret_t InitSectorIndexing(IterSect *IS, xyf p0, xyf p1, xyf p2,   *
*        long nx, long ny)                                             *
*                                                                      *
*  Arguments:                                                          *
*     IS        Ptr to IterSect structure that may be used to hold     *
*                 state of sector indexing routine.                    *
*     p0          Center of circle in grid units along x,y.            *
*     p1          Coordinates of the point at the end of the starting  *
*                 radius of the sector--this also defines the radius   *
*                 of the circle.                                       *
*     p2          Coordinates of a point on the line that defines the  *
*                 end of the sector, rotating counterclockwise from p1.*
*                 This point need not be at the same radius as p1--    *
*                 the radius is adjusted internally if needed.         *
*     nx,ny       Number of lattice units along x (fast moving) and    *
*                 y (slow moving) directions, 5<=nx,ny<32768.          *
*                                                                      *
*  Returns:                                                            *
*     IS is initialized for later calls to GetNextPointInSector.       *
*  If compiled as part of the crk library, the return type is void.    *
*  If an error occurs, abexit() is called with an abexit termination   *
*  code. However, if NO_CRKLIB is defined at compile time, the program *
*  returns an integer that is 0 when execution is successful, other-   *
*  wise an integer error code.  The possible error codes are defined   *
*  in the header file itersect.h.  This allows easier integration into *
*  MATLAB mex files.                                                   *
*                                                                      *
*  Notes:                                                              *
*     xyf is a structure type defined to hold an x,y coordinate pair.  *
*     Arguments nx,ny define the size of the rectangular lattice.      *
*  These values are used for two purposes: (1) to calculate the offset *
*  within the lattice to a point in the circle, and (2) to limit the   *
*  points returned to points inside the given bounds.  It is valid for *
*  part or all of the sector to lie outside the given lattice bounds-- *
*  points outside the bounds are never returned.                       *
*                                                                      *
*  Restrictions:                                                       *
*     Currently the lattice edge spacings along x and y must be equal  *
*  and the lower bound of each axis is always taken to be zero.  The   *
*  ordering of indexes for computing lattice offsets is y slow moving, *
*  x fast moving.                                                      *
*---------------------------------------------------------------------*/

#define TooSmall    1E-6
#define MinGrid     5      /* Minimum value for a grid edge */
#define MaxGrid 32767      /* Maximum value for a grid edge */
#define S16     65536      /* Multiplier to get S16 fixed-point */
#define dS32 4294967296.0  /* S16 squared */   
#define P16        16      /* Shift equivalent to S16 */
#define EvenGrid(x) (((x) & (S16-1)) == 0)
#define sFloor(x) ((x) & ~(S16-1))
#define sCeil(x) (((x) + (S16-1)) & ~(S16-1))

/* Actions specified in IS->ixop */
enum ixp { ixfin=0, ixseg1of1, ixseg1of2, ixseg2, iynext };
/* Segments of x scan stored in IS->xint */
enum ixseg { s1lft, s1rgt, s2lft, s2rgt };

ret_t InitSectorIndexing(IterSect *IS, xyf p0, xyf p1, xyf p2,
      long nx, long ny) {

   float r1,r2;            /* Radius of points 1,2 */
   float fmny, fmxy;       /* Minimum, maximum y grid in sector */
   /* Table to determine which y bounds to use vs p1, p2 quadrants.
   *  Indexing order is p1 quad (2 bits), then p2 quad (2 bits).
   *  Actions are defined here: */
#define YMN_MR    1        /* Ymin (start y) is -radius */
#define YMX_PR    2        /* mxy (final y) is +radius */
#define Q02_UR    4        /* Quads 0,3 y2 < y1 ==> use radii */
#define Q13_UR    8        /* Quads 2,4 y2 > y1 ==> use radii */
   static byte iylim[16] = {
      Q02_UR, YMX_PR, YMN_MR|YMX_PR, YMX_PR,    /* p1 in quad 0 */
      YMN_MR, Q13_UR, YMN_MR,        0,         /* p1 in quad 1 */
      0,      YMX_PR, Q02_UR,        YMX_PR,    /* p1 in quad 2 */
      YMN_MR, YMN_MR|YMX_PR, YMN_MR, Q13_UR };  /* p1 in quad 3 */
   int q12;                /* Index into iylim table */
   int yac;                /* Y action */

   /* Set exit in case user ignores err return */
   IS->ixop = ixfin;

   /* Sanity check */
   if (nx < MinGrid || nx > MaxGrid || ny < MinGrid || ny > MaxGrid) { 
#ifdef NO_CRKLIB
      return ISI_BADGRID;
#else
      abexitm(67, "InitSectIndexing: Bad lattice edge");
#endif
      }

   /* Compute sector parameters relative to circle origin */
   p1.x -= p0.x, p1.y -= p0.y;
   r1 = sqrtf(p1.x*p1.x + p1.y*p1.y);
   p2.x -= p0.x, p2.y -= p0.y;
   r2 = sqrtf(p2.x*p2.x + p2.y*p2.y);
#ifdef NO_CRKLIB
   if (r1 < TooSmall) return ISI_BADRAD1;
   if (p0.x + r1 < 0 || p0.x - r1 > (float)(nx-1) ||
       p0.y + r1 < 0 || p0.y - r1 > (float)(ny-1)) return ISI_OUTSIDE;
   if (r2 < TooSmall) return ISI_BADRAD2;
#else
   if (r1 < TooSmall) abexitm(67,
      "InitSectIndexing: Radius of point 1 too small"); 
   if (p0.x + r1 < 0 || p0.x - r1 > (float)(nx-1) ||
       p0.y + r1 < 0 || p0.y - r1 > (float)(ny-1)) abexitm(67,
      "InitSectIndexing: Circle is outside grid");
   if (r2 < TooSmall) abexitm(67,
      "InitSectIndexing: Radius of point 2 too small"); 
#endif
   r2 = r1/r2;          /* Factor to normalize (x2,y2) radius */
   /* Save sector parameters */
   IS->xc = (si32)roundf(S16*p0.x);
   IS->yc = (si32)roundf(S16*p0.y);
   IS->x1 = (si32)roundf(S16*p1.x);
   IS->y1 = (si32)roundf(S16*p1.y);
   IS->x2 = (si32)roundf(S16*(p2.x *= r2));
   IS->y2 = (si32)roundf(S16*(p2.y *= r2));
   IS->radsq = (double)(dS32*r1*r1);
   IS->nx = (si32)nx, IS->ny = (si32)ny;

   /* Determine range of y coordinate */
   q12 = (p1.y < 0) << 3 | (p1.x < 0) << 2 |
      (p2.y < 0) << 1 | (p2.x < 0);
   IS->yac = (short)(yac = (int)iylim[q12]);
   fmny = fmxy = 0;     /* Center of circle is one vertex */
   if (p1.y < fmny) fmny = p1.y; if (p1.y > fmxy) fmxy = p1.y;
   if (p2.y < fmny) fmny = p2.y; if (p2.y > fmxy) fmxy = p2.y;
   if (yac & Q13_UR) {
      if (p2.y > p1.y) {
         fmny = -r1, fmxy = r1;
         IS->yac |= (YMN_MR|YMX_PR); }
      }
   else if (yac & Q02_UR) {
      if (p2.y < p1.y) {
         fmny = -r1, fmxy = r1;
         IS->yac |= (YMN_MR|YMX_PR); }
      }
   else {
      if (yac & YMX_PR) fmxy = r1;
      if (yac & YMN_MR) fmny = -r1;
      }
   IS->iy = (long)ceilf(fmny + p0.y);
   if (IS->iy < 0) IS->iy = 0;
   IS->mxy = (long)floorf(fmxy + p0.y);
   {  long nym1 = ny - 1;
      if (IS->mxy > nym1) IS->mxy = nym1;
      } /* End nym1 local scope */

   /* Force new row on first GetNextPointInSector call */
   if (IS->mxy >= IS->iy) {
      IS->ixop = iynext; 
      IS->iy -= 1;
      }

#ifdef NO_CRKLIB
   return ISI_NORM;
#endif

   } /* End InitSectorIndexing */

/*---------------------------------------------------------------------*
*                        GetNextPointInSector                          *
*                                                                      *
*  Synopsis:                                                           *
*     int GetNextPointInSector(IterSect *IS)                           *
*                                                                      *
*  Arguments:                                                          *
*     IS                Ptr to IterSect structure initialized by       *
*                       previous call to InitSectorIndexing            *
*  Returns:                                                            *
*     IS->ix,iy         Index values of next lattice point in sector   *
*                       along x,y directions.                          *
*     IS->ioff          Offset of point ix,iy from origin of lattice.  *
*     Function value    TRUE if a grid point was found inside the      *
*                          given sector.  Values in IS->ix,iy,ioff     *
*                          are valid.                                  *
*                       FALSE if all grid points in the sector have    *
*                          already been returned.  Values in IS        *
*                          are no longer valid.                        *
*---------------------------------------------------------------------*/

int GetNextPointInSector(IterSect *IS) {

EndSegRestart:
   switch (IS->ixop) {
   case ixseg1of1:         /* Scanning in only segment */
      if (++IS->ix > (long)IS->xint[s1rgt]) {
         IS->ixop = iynext; goto EndSegRestart; }
      break;
   case ixseg1of2:         /* Scanning in first of two segments */
      if (++IS->ix > (long)IS->xint[s1rgt]) {
         IS->ix = (long)IS->xint[s2lft];
         IS->ixop = ixseg2; }
      break;
   case ixseg2:            /* Scanning in second segment of two */
      if (++IS->ix > (long)IS->xint[s2rgt]) {
         IS->ixop = iynext; goto EndSegRestart; }
      break;
   case iynext:            /* No more in this row, step to next iy */
      while (++IS->iy <= IS->mxy) {
         double tiy;
         si32 iiy = (si32)IS->iy;
         si32 lxi[4];      /* Local line-edge intersections (S16) */
         si32 liy,lrx;     /* Local y, x radius (S16) */
         si32 nxm1 = (IS->nx - 1) << P16;
         int nint = 0;     /* Number of intersections found */
         int ib = 0;       /* Base index of one segment to use */
         liy = (iiy<<P16) - IS->yc; tiy = (double)liy;
         lrx = (si32)round(sqrt(IS->radsq - tiy*tiy));
         /* Special case:  liy == 0, scan line passes through center
         *  of circle.  If off a grid line, no x solutions.  If on a
         *  grid line, p1 or p2 can be horizontal, whence both ends
         *  are solutions, or slanted, giving just center point.
         *  Note that xint points will be sorted later, so no need
         *  to store lower intersection first.  */
         if (liy == 0) {
            if (EvenGrid(IS->yc)) {
               if (IS->y1 == 0) {
                  lxi[nint++] = IS->xc + IS->x1;
                  lxi[nint++] = IS->xc + (IS->y2 == 0 ? IS->x2 : 0); }
               else {
                  lxi[nint++] = IS->xc;
                  if (IS->y2 == 0) lxi[nint++] = IS->xc + IS->x2; }
               } /* End case, center of circle is on a grid line */
            } /* End special case, liy == 0 */
         /* Normal case:  Scan line does not pass through center.
         *  Sequence of tests is designed to prevent overflow in
         *  division by very small IS->y1 or IS->y2.  */
         else if (liy > 0) {
            /* Find intersection of this y with radius to point 1 */
            if (liy <= IS->y1)
               lxi[nint++] = IS->xc + dm64nq(liy,IS->x1,IS->y1);
            /* Same for intersection with point 2 */
            if (liy <= IS->y2)
               lxi[nint++] = IS->xc + dm64nq(liy,IS->x2,IS->y2);
            } 
         else {                     /* liy here must be < 0 */
            /* Find intersection of this y with radius to point 1 */
            if (liy >= IS->y1)
               lxi[nint++] = IS->xc + dm64nq(liy,IS->x1,IS->y1); 
            /* Same for intersection with point 2 */
            if (liy >= IS->y2)
               lxi[nint++] = IS->xc + dm64nq(liy,IS->x2,IS->y2);
            }
         /* It is necessary also to check intersections of new scan
         *  line with the circle under conditions derived in the
         *  notes.  */
         if (!(IS->yac & (YMX_PR|YMN_MR))) {
            /* Sector lies entirely to left or right of vertical */
            if ((IS->x1 | IS->x2) >= 0) { /* In Q0 or Q2 */
               if (liy > IS->y1 && liy < IS->y2)
                  lxi[nint++] = IS->xc + lrx; }
            else {                        /* In Q1 or Q3 */
               if (liy > IS->y2 && liy < IS->y1)
                  lxi[nint++] = IS->xc - lrx; }
            }
         else {
            /* We need to be careful here not to store +/-lrx twice */
            int topck = IS->yac & YMX_PR;
            int botck = IS->yac & YMN_MR;
            if (topck && liy > IS->y2 || botck && liy < IS->y1)
               lxi[nint++] = IS->xc - lrx;
            if (topck && liy > IS->y1 || botck && liy < IS->y2)
               lxi[nint++] = IS->xc + lrx;
            }

         /* Now sort the intersections, deleting duplicate ranges and
         *  ranges that lie entirely between two grid points.  Since
         *  this is a very short list, a bubble sort will do.  */

         /* If no intersections found, try another y */
         if (nint == 0)
            continue;
         /* One, must be on a grid point and in range */
         else if (nint == 1) {
            if (!EvenGrid(lxi[0]) || lxi[0] < 0 || lxi[0] > nxm1)
               continue;
            lxi[1] = lxi[0]; nint = 2;
            }
         else if (nint >= 2) {
            int ii,jj;              /* First do the sort */
            for (ii=0; ii<nint-1; ++ii) {
               for (jj=ii+1; jj<nint; ++jj) {
                  if (lxi[jj] < lxi[ii]) {
                     si32 tt = lxi[jj];
                     lxi[jj] = lxi[ii], lxi[ii] = tt;
                     }
                  }
               }
            /* If 3 points, the middle one must duplicate or overlap
            *  one of the others, so delete it.  */
            if (nint == 3)
               lxi[1] = lxi[2], nint = 2;
            /* Arriving here, there are either 2 or 4 interesections.
            *  If either segment does not encompass a grid point or
            *  is out of range, delete it.  If it overlaps the edge
            *  of the grid, clean it up.  Use ib to keep track of
            *  which segment is left if one of two is deleted.  */
            for (ii=nint-2; ii>=0; ii-=2) {
               if (lxi[ii+1] < 0 || lxi[ii] > nxm1)
                  nint -= 2, ib = 2 - ii;
               else {
                  if (lxi[ii] < 0) lxi[ii] = 0;
                  if (lxi[ii+1] > nxm1) lxi[ii+1] = nxm1;
                  if (sCeil(lxi[ii]) > sFloor(lxi[ii+1]))
                     nint -= 2, ib = 2 - ii;
                  }
               }
            /* If all x segements were deleted,
            *  that is it for this y.  */
            if (nint <= 0) continue;
            /* If still two segments left, now see if they can be
            *  merged.  */
            if (nint == 4 && sCeil(lxi[2]) - sFloor(lxi[1]) <= 1)
               lxi[1] = lxi[3], nint = 2;
            } /* End handling nint >= 2 */
         /* Now have 2 or 4 intersections, store the integer parts */
         if (nint > 2) {
            IS->xint[2] = sCeil(lxi[2]) >> P16;
            IS->xint[3] = sFloor(lxi[3]) >> P16;
            IS->ixop = ixseg1of2; }
         else
            IS->ixop = ixseg1of1;
         IS->ix = (long)(IS->xint[0] = sCeil(lxi[ib]) >> P16);
         IS->xint[1] = sFloor(lxi[ib+1]) >> P16;
         IS->iofrow = (long)IS->nx * IS->iy;
         /* It would be nice if the language had a 'break 2' */
         goto ReturnPoint;
         } /* End checking all remaining y lines */

      /* Exceeded max y, fall through to end it all */
      IS->ixop = ixfin;
   case ixfin:             /* Last point already returned */
      return FALSE;
   } /* End ixop switch */

/* When we break out of the switch, we have a valid ix,
*  so just compute ioff and return.  */

ReturnPoint:
   IS->ioff = IS->iofrow + IS->ix;
   return TRUE;

   } /* End GetNextPointInSector() */

