/* (c) Copyright 2011-2014, The Rockefeller University *11115* */
/* $Id: iterpoly.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                             iterpoly.c                               *
*                                                                      *
*  This module contains routines that may be used to visit in turn     *
*  all the points on some rectangular lattice that fall within a given *
*  (mostly-convex -- see below) polygon.  Results are presented x fast *
*  moving, y slow.                                                     *
*                                                                      *
*  This version works with coordinates of type float.  Another version *
*  will need to be written if double precision coordinates or concave  *
*  polygons are needed.                                                *
*                                                                      *
*  The user must call InitPolygonIndexing to initialize indexing for   *
*  some particular polygon.  Each time another grid point is wanted,   *
*  a call to GetNextPointInPolygon returns the x,y coordinates of the  *
*  next point and its offset from the start of the lattice array.      *
*                                                                      *
*  The user must provide an instance of the IterPoly data structure    *
*  for each invocation of the routines.  This structure is used both   *
*  to store the internal state of the iterator and to return results.  *
*                                                                      *
*  The user may carry out multiple iterations at the same time by      *
*  providing a separate IterPoly structure for each.                   *
*                                                                      *
*  The IterPoly structure and the two calls are prototyped in          *
*  iterpoly.h                                                          *
************************************************************************
*  V1A, 08/29/11, GNR - Initial version                                *
*  ==>, 08/31/11, GNR - Last date before committing to svn repository  *
*  Rev, 02/20/12, GNR - Eliminate unnecessary sign of large slopes     *
*  Rev, 11/18/14, GNR - Add NO_CRKLIB definition                       *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "iterpoly.h"

#define BigCoord  1.0E10

/*---------------------------------------------------------------------*
*                        InitPolygonIndexing                           *
*                                                                      *
*  Synopsis:                                                           *
*     ret_t InitPolygonIndexing(IterPoly *Poly, xyf *pgon,             *
*        IPolyWk *work, long nsx, long nsy, int nvtx)                  *
*                                                                      *
*  Arguments:                                                          *
*     Poly                 Ptr to IterPoly structure that pgm will use *
*                          to hold state of polygon indexing routine.  *
*     pgon                 Pairs of x,y coordinates defining in order  *
*                          the edges of the polygon.  Certain types of *
*                          concavities can result in erroneous results *
*                          as described in detail below.               *
*     work                 Ptr to a work area to hold nvtx IPolyWk     *
*                          structs as defined in iterpoly.h.           *
*     nsx,nsy              Size of full rectangular lattice along x,y. *
*     nvtx                 Number of vertices of pgon.  An abexit      *
*                          code 67 error occurs if nvtx < 3.           *
*                                                                      *
*  Returns:                                                            *
*     Poly is initialized for later calls to GetNextPointInPolygon     *
*  If compiled as part of the crk library, the return type is void.    *
*  If an error occurs, abexit() is called with an abexit termination   *
*  code. However, if NO_CRKLIB is defined at compile time, the program *
*  returns an integer that is 0 when execution is successful, other-   *
*  wise an integer error code.  The possible error codes are defined   *
*  in the header file iterpoly.h.  This allows easier integration into *
*  MATLAB mex files.                                                   *
*                                                                      *
*  Notes:                                                              *
*     The program generally expects polygons to be convex, but can     *
*  tolerate concavities that do not result in more than two crossings  *
*  of any line of constant y by edges of the polygon.  This situation  *
*  is detected by GetNextPointInPolygon and reported with an abexit    *
*  code 68.  Horizontal edges are always OK.                           *
*     Arguments nsx,nsy define the size of a full rectangular lattice, *
*  on which the required polygon is placed.  These values are used for *
*  two purposes: (1) to calculate the offset within the full lattice   *
*  to each point in the desired polygon, and (2) to limit the points   *
*  returned to points inside the given bounds.  It is valid for part   *
*  or all of the polygon to lie outside the given bounds--points       *
*  outside the bounds are never returned.                              *
*     The contents of pgon and work should not be changed by the user  *
*  from the InitPolygonIndexing call until GetNextPointInPolygon       *
*  returns FALSE.                                                      *
*---------------------------------------------------------------------*/

ret_t InitPolygonIndexing(IterPoly *Poly, xyf *pgon, IPolyWk *work,
   long nsx, long nsy, int nvtx) {

   long  ymn,ymx;             /* Lowest, highest vertex y */
   int   iv,iv2;              /* Vertex loop indices */

   /* Sanity checking */
#ifdef NO_CRKLIB
   if (nsx <= 0 || nsy <= 0)  return IPI_BADGRID;
   if (nvtx < 3)              return IPI_LT3VERT;
#else
   if (nsx <= 0 || nsy <= 0)
      abexitm(67, "Lattice edge <= 0 for polygon indexing");
   if (nvtx < 3)
      abexitm(67, "< 3 vertices for polygon indexing");
#endif

   /* Save polygon parameters */
   Poly->pgon = pgon;
   Poly->wk = work;
   Poly->topx = (float)(nsx - 1);
   Poly->nsx = nsx, Poly->nsy = nsy;
   Poly->nvtx = nvtx;

   /* Save low and high y coords and slopes */
   ymn = nsy, ymx = -1;    /* Just out-of-bounds */
   for (iv=0; iv<nvtx; ++iv) {
      float x1,x2,y1,y2,dx,dy,ty;
      long  ity;
      iv2 = (iv+1) % nvtx;
      x1 = pgon[iv].x,  y1 = pgon[iv].y;
      x2 = pgon[iv2].x, y2 = pgon[iv2].y;
      ity = (long)ceilf(y1);
      if (ity < ymn) ymn = ity;
      ity = (long)floorf(y1);
      if (ity > ymx) ymx = ity;
      dx = x2 - x1, dy = y2 - y1;
      /* Avoid a divide check in slope calculation */
      Poly->wk[iv].slope = (fabsf(dx) > BigCoord*fabsf(dy)) ?
         BigCoord : dx/dy;
      if (y1 > y2) ty = y1, y1 = y2, y2 = ty;
      Poly->wk[iv].loy = (long)ceilf(y1);
      Poly->wk[iv].hiy = (long)floorf(y2);
      } /* End vertex loop */

   /* Set y bounds, starting x,y, and y offset */
   if (ymn < 0) ymn = 0;
   if (ymx >= nsy) ymx = nsy - 1;
   Poly->ymn = ymn, Poly->ymx = ymx;
   Poly->ix = (Poly->xmx = nsx) + 1;   /* Force new y first time */
   Poly->iy = ymn - 1;
   Poly->yoff = nsx * Poly->iy;

#ifdef NO_CRKLIB
   return IPI_NORMAL;
#endif

   } /* End InitPolygonIndexing() */


/*---------------------------------------------------------------------*
*                       GetNextPointInPolygon                          *
*                                                                      *
*  Synopsis:                                                           *
*     int GetNextPointInPolygon(IterPoly *Poly)                        *
*                                                                      *
*  Arguments:                                                          *
*     Poly              Ptr to IterPoly structure initialized by       *
*                       previous call to InitPolygonIndexing.          *
*  Returns:                                                            *
*     Poly->ix,iy       x,y coords of next lattice point in polygon.   *
*     Poly->ioff        Offset of point ix,iy from lattice origin.     *
*     Function value    TRUE if a grid point was found inside the      *
*                          given polygon.  Values in Poly->ix,iy,      *
*                          and ioff are valid.                         *
*                       FALSE if all grid points in the polygon have   *
*                          already been returned.  Values in Poly->ix, *
*                          iy, and ioff are no longer valid.           *
*  Note:                                                               *
*     If no points are eligible (because requested polygon lies        *
*  outside the base lattice), GetNextPointinPolygon returns FALSE      *
*  at once.  This is not considered an error condition.                *
*---------------------------------------------------------------------*/

int GetNextPointInPolygon(IterPoly *Poly) {

   float gxmn,gxmx;           /* Global min,max x */
   long  ty;                  /* Current y */
   int   iv;                  /* Vertex loop indices */
   int   nyc;                 /* Number of y crossings */

   if (Poly->ix >= Poly->xmx) {
TryNextY:
      if (Poly->iy >= Poly->ymx) return FALSE;
      /* Start a new row--check interesections with all edges */
      gxmn = Poly->topx, gxmx = 0.0;
      ty = Poly->iy += 1;
      Poly->yoff += Poly->nsx;
      nyc = 0;
      for (iv=0; iv<Poly->nvtx; ++iv) {
         float tix;
         if (ty < Poly->wk[iv].loy || ty > Poly->wk[iv].hiy) continue;
         /* If the slope is BigCoord, the edge is horizontal and
         *  both ends will get tested as solutions for the preced-
         *  ing and following lines.  */
         if (Poly->wk[iv].slope == BigCoord) continue;
         tix = Poly->pgon[iv].x +
            Poly->wk[iv].slope * ((float)ty - Poly->pgon[iv].y);
         /* Given that the tix calc is skipped when y is outside the
         *  bounds of this line, tix is guaranteed to be within the
         *  line and a check is not necessary. However, by the spec,
         *  this could be outside the lattice.  It is easier to check
         *  this after gxmn,gxmx are known.  */
         if (tix < gxmn) gxmn = tix;
         if (tix > gxmx) gxmx = tix;
         /* Check for concavity as indicated by more than two y-axis
         *  crossings.  But if an endpoint is exactly on the current
         *  grid line, count it only if it is the second end to avoid
         *  double counting.  Horizontal lines are skipped above.  */
         if ((float)ty != Poly->pgon[iv].y) nyc += 1;
         } /* End vertex loop */
      /* Protect against the perverse case where the whole
      *  line is out-of-bounds */
      if (gxmn > Poly->topx || gxmx < 0.0) goto TryNextY;
      /* Concavity error if other than two grid crossings
      *  (one is OK if at y extrema)  */
      if (nyc > NLENDS || nyc < NLENDS &&
            ty != Poly->ymn && ty != Poly->ymx) {
#ifdef NO_CRKLIB
         Poly->iy = Poly->ymx;
         return FALSE;
#else
         abexitm(68, "Degenerate or concave polygon");
#endif
         }
      /* Now set x loop and impose lattice bounds */
      Poly->ix = (long)ceilf(max(gxmn,0.0));
      Poly->xmx = (long)floorf(min(gxmx,Poly->topx));
      if (Poly->ix > Poly->xmx) goto TryNextY;
      }
   else
      Poly->ix += 1;

   /* Compute offset of this point in the overall lattice */
   Poly->ioff = Poly->yoff + Poly->ix;
   return TRUE;
   } /* End GetNextPointInPolygon() */
