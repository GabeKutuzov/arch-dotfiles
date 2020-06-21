/* (c) Copyright 2012-2013, The Rockefeller University *11115* */
/* $Id: iterepgf.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                             iterepgf.c                               *
*                                                                      *
*  This module contains routines that may be used to visit in turn all *
*  the points on some rectangular lattice that fall either within or   *
*  outside a figure defined by one or more possibly concave, possibly  *
*  overlapping, polygons or polygons with holes and a border of fixed  *
*  width around the figure boundary. Results are presented in order x  *
*  fast moving, y slow.                                                *
*                                                                      *
*  This version works with coordinates of type float.  Another version *
*  will need to be written if double precision coordinates are needed. *
*                                                                      *
*  The user must provide an instance of the IterEpgf data structure    *
*  for each invocation of the routines.  This structure is used both   *
*  to store the internal state of the iterator and to return results.  *
*                                                                      *
*  The user must call InitEpgfIndexing to initialize indexing for      *
*  some particular figure.  Each time another grid point is wanted,    *
*  a call to GetNextPointInEpgf returns the x,y coordinates of the     *
*  next point and its offset from the start of the lattice array.      *
*                                                                      *
*  The user may carry out multiple iterations at the same time by      *
*  providing a separate IterEpgf structure for each.  The IterEpgf     *
*  structure and the two calls are prototyped in iterepgf.h.  If the   *
*  figure is known to consist of a single convex polygon, the iterpoly *
*  routines may be faster.                                             *
*                                                                      *
*  Design Notes:  Coordinates are kept in fixed-point for faster       *
*  arithmetic and easier special-case testing.  The binary scale       *
*  is set according to the size of the raster for best accuracy.       *
*  Looping ranges in this program are inclusive.                       *
*  Names of scaled coordinates will generally begin with 's'.          *
*  Returned ix,iy,ioff are longs for compat with other iterators.      *
************************************************************************
*  V1A, 11/14/12, GNR - Initial version, based on iterpgfg.c           *
*  ==>, 11/20/12, GNR - Last date before committing to svn repository  *
*  Rev, 11/21/12, GNR - Omit short lines instead of abending           *
*  Rev, 10/24/13, GNR - SRA() for signed '>>'                          *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "iterepgf.h"
#include "rkarith.h"
#include "rksubs.h"

#define  MNNVTX    3       /* Minimum vertices in a polygon */
#define  MNBSCL    3       /* Minimum reasonable binary scale */
#define  MXSCBT   29       /* Maximum bits in scaled coords */
#define  NRVTX     4       /* Number of vertices in a rectangle */
#define  BigCoord  1E10    /* Test for vertical slope */
#define  SMALL     1E-6    /* Test for vertices too close */

/*---------------------------------------------------------------------*
*                        InitEpgfIndexing                              *
*                                                                      *
*  Synopsis:                                                           *
*     void InitEpgfIndexing(IterEpgf *pit, xyf *pgon, IEpgfWk *work,   *
*        byte *pwim, float border, si32 nsx, si32 nsy, int nvtx,       *
*        int mode)                                                     *
*                                                                      *
*  Arguments:                                                          *
*     pit                  Ptr to IterEpgf structure that pgm will use *
*                          to hold the state of the indexing routine.  *
*     pgon                 Pairs of x,y coordinates defining in order  *
*                          the vertices of the polygon(s).  Concave    *
*                          polygons and multiple polygons are allowed. *
*                          Coordinates must lie in the ranges          *
*                          -nsx <= x < 2*nsx, -nsy <= y < 2*nsy.       *
*                          To indicate that a point is the last vertex *
*                          in its polygon, store EpgfPly(y,nsy) in     *
*                          place of the y coordinate.  (EpgfPly() is   *
*                          a macro defined in iterepgf.h.              *
*                          Subsequent vertices may define additional   *
*                          polygons or holes in the original polygon.  *
*     work                 Ptr to a work area large enough to hold     *
*                          nvtx IEpgfWk structs defined in iterepgf.h. *
*     pwim                 Ptr to a work area large enough to hold     *
*                          nsy * (bytes to hold nsx bits).             *
*     border               Width of the border to be drawn on either   *
*                          side of the defining polygon (pixel units). *
*     nsx,nsy              Size of full rectangular lattice along x,y. *
*                          These need to be less than ~2^24 to reduce  *
*                          round-off errors.                           *
*     nvtx                 Number of vertices of pgon.                 *
*     mode                 Sum of bits from the following list         *
*                          describing the operations to be performed:  *
*        EPGF_INT  (=1)    Return points inside the pgon figure.       *
*        EPGF_EXT  (=2)    Return points outside the pgon figure.      *
*        EPGF_ADDB (=0)    Also return points within 'border' pixels   *
*                          of the defining polygon (default).          *
*        EPGF_RMB  (=4)    Erase (do not return) points within         *
*                          'border' pixels of the defining polygon.    *
*                                                                      *
*  Returns:                                                            *
*     pit is initialized for later calls to GetNextPointInEpgf         *
*                                                                      *
*  Errors:                                                             *
*     An abexit code 67 error occurs if border <= 0 or nvtx < 3 or     *
*  if any one polygon in the figure has fewer than 3 vertices.         *
*  Error 68 is generated if any coordinate is outside the specified    *
*  limits.  (This condition is likely to be the result of a coding     *
*  error, but the code can be modified if a larger coord range is      *
*  needed.)                                                            *
*                                                                      *
*  Notes:                                                              *
*     Arguments nsx,nsy define the size of a full rectangular lattice, *
*  on which the specified figure is placed.  These values are used for *
*  two purposes: (1) to calculate the offset within the full lattice   *
*  to each point in the desired figure, and (2) to limit the points    *
*  returned to points inside the given bounds.  It is valid for part   *
*  or all of the figure to lie outside the given bounds--points        *
*  outside the bounds are never returned.                              *
*     It is not necessary to code a negative y coordinate for the last *
*  point in the pgon list.                                             *
*     The contents of 'work' and 'pwim' should not be changed by the   *
*  user from the InitEpgfIndexing() call until GetNextPointInEpgf()    *
*  returns FALSE.  (These work areas are defined in the call so the    *
*  user can allocate them once in a run rather than at each polygon    *
*  call, possibly using high-water-mark allocation if images of        *
*  different sizes are being processed.                                *
*     Only the low-order 3 bits of 'mode' are interpreted.  Values     *
*  '4' (return no points) and '3' (return all points on lattice) are   *
*  valid but may do a lot of work for a trivial result.  Value '0'     *
*  returns a thick line with rounded vertices.  To get same results    *
*  as iterpgfg, use mode '1' or '2' with 'border' = 0.                 *
*---------------------------------------------------------------------*/

#define XY2Fix(v) ((si32)((v)*sclm + 0.5))

void InitEpgfIndexing(IterEpgf *pit, xyf *pgon, IEpgfWk *work,
   byte *pwim, float border, si32 nsx, si32 nsy, int nvtx, int mode) {

   byte  *pim;                /* Location in working image */
   float b2;                  /* Border squared */
   float rmnx,rmxx;           /* Min,max allowed x coords */
   float rmny,rmxy;           /* Min,max allowed y coords */
   float ysig;                /* Last y in polygon signal */
   si32  gmny,gmxy;           /* Global min,max y */
   si32  ltir;                /* Length of temp image row */
   si32  nsxm1,nsym1;         /* nsx-1, nsy-1; */
   int   iv,iv1,iv2;          /* Vertex loop indices */
   int   lvtx;                /* Index of the last vertex */
   int   kvtx;                /* Tricky vertex number test */
   int   mvtx;                /* Number of vertices in one polygon */
   int   par,par0;            /* Current, initial parity */

   /* Sanity checking */
   if (border < 0.0 || nsx <= 0 || nsy <= 0 || nvtx < 3) abexit(67);

   /* Save global parameters */
   pit->ltir = ltir = BytesInXBits(nsx);
   pit->pwim = pwim;
   pit->nsx = nsx, pit->nsy = nsy;

   /* Prepare working image according to mode */
   {  size_t lwim = ltir * nsy;
      if ((mode & EPGF_ALL) == EPGF_ALL)
         memset(pwim, ~0, lwim);
      else
         memset(pwim, 0, lwim);
      } /* End local lwim scope */

   /* Set GetNextPointInEpgf to scan the entire image,
   *  then handle trivial cases right away */
   pit->ix = (long)nsx;       /* Force new y on first call */
   pit->topy = (long)nsy;     /* And end at top of lattice */
   mode &= (EPGF_INT|EPGF_EXT|EPGF_RMB);
   if (mode == EPGF_RMB) {
      /* Return no lattice points */
      pit->iy = (long)nsy;
      return;
      }
   pit->iy = -1L;
   if (mode == EPGF_ALL) {
      /* Return all lattice points */
      return;
      }

   /* Constants needed for filling and line drawing */
   lvtx = nvtx - 1, nsxm1 = nsx - 1, nsym1 = nsy - 1;
   rmnx = -(float)nsx, rmxx = (float)(nsx<<1);
   rmny = -(float)nsy, rmxy = (float)(nsy<<1);
   ysig = (float)(nsy<<EPGF_LAST_Y_SHFT);

   /* Set initial parity (also used as local EPGF_EXT switch) */
   par0 = mode & EPGF_EXT;
   if (par0) gmny = 0, gmxy = nsym1;
   else      gmny = nsy, gmxy = -1;

/* Perform marking of inside or outside of boundary polygon
*  when one or the other but not both are called for.
*  *** ONE LEVEL OF INDENTING SUPPRESSED TO END OF ROUTINE *** */

kvtx = mode & EPGF_ALL;
if (kvtx == EPGF_INT || kvtx == EPGF_EXT) {

   IEpgfWk *pwe,**ppwe;       /* Ptr for building edge list */
   IEpgfWk *wkel;             /* Working edge list */
   IEpgfWk *wkal;             /* Working active edge list */
   float sclm;                /* Scale multiplier */
   float dytest;              /* Part of slope overflow test */
   si32  imsk;                /* Integer mask */
   si32  smnx;                /* Scaled min x (-nsx) */
   si32  snsx,snsy;           /* Scaled nsx,nsy */
   si32  um1;                 /* Scaled unity - 1 = fraction mask */
   int   bs,ibs;              /* Binary scale, integer scale */
   int   ix,iy;               /* Current x,y coords */

   /* Determine binary scale that will just fit largest coords
   *  in an si32 variable */
   ibs = bitszs32(max(nsx,nsy));
   bs = MXSCBT - ibs;
   if (bs < MNBSCL) abexit(67);
   sclm = (float)(um1 = 1 << bs);
   imsk = -um1;
   um1 -= 1;
   dytest = (float)(2 << ibs);
   snsx = nsx << bs, snsy = nsy << bs;
   smnx = -snsx;

   /* Initialize building of working edge list */
   wkal = NULL;
   pwe = work; ppwe = &wkel;

   /* Save edge coords and slopes */
   kvtx = mvtx = 0;
   for (iv=iv1=0; iv<nvtx; ++iv) {
      float x1,x2,y1,y2;
      si32  txy;                 /* Temp x or y */
      si32  tsx1,tsx2,tsy1,tsy2; /* Temp x,y for swapping */
      x1 = pgon[iv].x, y1 = pgon[iv].y;
      if (iv == lvtx || y1 < rmny) {
         /* Complete one polygon */
         iv2 = iv1, iv1 = iv + 1;
         kvtx = MNNVTX; }
      else
         /* Continue current polygon */
         iv2 = iv + 1;
      x2 = pgon[iv2].x, y2 = pgon[iv2].y;
      if (y1 < rmny) y1 += ysig;
      if (y2 < rmny) y2 += ysig;
      /* Test boundary limits as documented (JIC).  Do this
      *  before scaling to catch outliers that might overflow. */
      if (x1 < rmnx || x1 >= rmxx || y1 < rmny || y1 >= rmxy ||
          x2 < rmnx || x2 >= rmxx || y2 < rmny || y2 >= rmxy)
         abexit(68);
      /* Generate scaled fixed-point coordinates */
      tsx1 = XY2Fix(x1), tsx2 = XY2Fix(x2);
      tsy1 = XY2Fix(y1), tsy2 = XY2Fix(y2);
      /* Make x1,y1 be the point at lower y.  Do this test
      *  on fixed-point coords to capture very close floats.  */
      if (tsy2 < tsy1) {
         txy = tsx1, tsx1 = tsx2, tsx2 = txy;
         txy = tsy1, tsy1 = tsy2, tsy2 = txy;
         }
      else if (tsy2 == tsy1 && tsx2 < tsx1) {
         txy = tsx1, tsx1 = tsx2, tsx2 = txy;
         }
      /* Store ordered fixed-point coordinates */
      pwe->sly = tsy1, pwe->shy = tsy2;
      pwe->slx = tsx1, pwe->shx = tsx2;
      /* If end of polygon, be sure at least 3 vertices.  Count
      *  lines even if omitted, e.g. might be a horizontal triangle
      *  base below y = 0.  */
      mvtx += 1;
      if (kvtx) {
         if (mvtx < kvtx) abexit(67);
         kvtx = mvtx = 0; }
      /* Omit line if entirely outside the rectangle
      *  (except on the left--may be needed for parity). */
      if (tsy1 >= snsy || tsy2 < 0.0 || min(tsx1,tsx2) >= snsx)
         continue;
      /* Omit line if it never crosses a grid row--this includes
      *  case of horizontal lines not exactly on a grid row */
      if ((tsy1 & imsk) == (tsy2 & imsk) && tsy1 & um1)
         continue;
      /* Avoid a divide check in slope calculation.  Explanation:
      *  dx/dy overflows if |(dx<<bs)/dy| >= 2^30, i.e.
      *  |dx| >= dy*(2^(30-bs)) = dy*(2^(ibs + 1)).  */
      {  float dx = (float)(tsx2 - tsx1), dy = (float)(tsy2 - tsy1);
         pwe->slope = (fabsf(dx) >= dytest*dy) ?
         (dx >= 0 ? SI32_MAX : -SI32_MAX) : XY2Fix(dx/dy); }
      /* Track bottom and top of figure */
      if (!par0) {
         txy = SRA((tsy1 + um1),bs);
         if (txy < gmny) gmny = txy;
         txy = SRA(tsy2,bs);
         if (txy > gmxy) gmxy = txy;
         }
      /* Insert edge in linked list */
      *ppwe = pwe, ppwe = &pwe->pne, ++pwe;
      } /* End vertex loop */
   *ppwe = NULL;              /* End linked list */

   /* Sort the edge list on ascending y */
   wkel = sort(wkel, (int)((char *)(&((IEpgfWk *)0)->sly) - (char *)0),
      2*I32SIZE, 0);

   /* Make sure global y range is inside the lattice */
   if (gmny < 0) gmny = 0;
   if (gmxy > nsym1) gmxy = nsym1;

   /* Mark bits in working image for all points either inside or
   *  outside the specified boundary.  */
   pim = pwim + gmny*ltir;
   for (iy=gmny; iy<=gmxy; ++iy,pim+=ltir) {
      long  ochk;             /* Overflow check flag */
      si32  sty;              /* Scaled current y */

      sty = (si32)(iy << bs);
      par = par0;

      /* Check for edges that can be deleted from active list and
      *  update x crossing for edges already on the active list.  */
      for (ppwe=&wkal; (pwe=*ppwe); ) { /* Assignment intended */
         if (sty > pwe->shy) goto DeleteLineFromActiveList;
         pwe->rix += pwe->slope;
         /* If off the right edge, can delete from active list */
         if (pwe->slope >= 0) {
            if (pwe->rix >= snsx) goto DeleteLineFromActiveList; }
         /* If way off to the left, can move to the left boundary */
         else if (pwe->rix < smnx) pwe->rix = smnx;
         /* Advance to next line on active list */
         ppwe = &pwe->pne;
         continue;
DeleteLineFromActiveList:
         *ppwe = pwe->pne;
         } /* End loop over edges on active list */

      /* Check for new,higher edges to be added to active list.
      *  Save user's e64 state and switch to E64_FLAGS mode.  */
      e64push(E64_FLAGS, &ochk);
      while (wkel && wkel->sly <= sty) {
         pwe = wkel;
         wkel = pwe->pne;
         pwe->pne = wkal;
         wkal = pwe;
         /* Compute starting x.  ochk will return overflow status */
         ochk = 0;
         pwe->rix = mrsrsle(sty - pwe->sly, pwe->slope, bs, 1) +
            pwe->slx;
         /* Check for overflow/underflow */
         if (pwe->slope >= 0) {  /* Positive slope */
            if (pwe->rix > snsx | ochk)
               pwe->rix = snsx;
            }
         else {                  /* Negative slope */
            if (pwe->rix < smnx | ochk)
               pwe->rix = smnx;
            }
         }
      e64pop();
      /* If the list is empty and parity is set, mark all the pixels
      *  in the current row.  If parity is not set, advance y to
      *  next edge (this could be a discontinuous figure) or finish */
      if (!wkal) {
         if (par) {
            memset(pim, ~0, ltir);
            continue;   /* Next y */
            }
         if (wkel) {
            iy = SRA((wkel->sly - 1), bs);/* Loop will increment iy */
            pim = pwim + iy*ltir;
            continue;   /* Next y */
            }
         else
            break;      /* End y loop */
         }
      /* Sort the active list on x.  A simple insertion sort is per-
      *  formed here because crk sort assumes unsigned keys and the
      *  list is short so any method will do.  */
      pwe = wkal, wkal = NULL;
      while (pwe) {
         IEpgfWk *rwe, *qwe = pwe->pne;
         for (ppwe=&wkal;
               (rwe = *ppwe) && (pwe->rix > rwe->rix);
               ppwe=&rwe->pne) ;
         /* Insert in output list */
         pwe->pne = rwe, *ppwe = pwe;
         pwe = qwe;
         }

      /* Loop over edges in the active list */
      ix = 0;           /* Start at left edge of box */
      for (pwe=wkal; pwe; pwe=pwe->pne) {
         /* If parity is on, fill from current location to line if on
         *  a grid point, otherwise next point below, i.e. floor(rix).
         *  To speed things up a little, fill a whole byte at once if
         *  possible.  (This code is repeated each time needed to save
         *  time for a subroutine call in this inner loop.)  */
         if (par) {
            int ix2 = (int)SRA(pwe->rix,bs);
            if (ix2 > nsxm1) ix2 = nsxm1;
            while (ix <= ix2) {
               int ibo = ByteOffset(ix), ibr = BitRemainder(ix);
               if (ibr == 0 && ix + (BITSPERBYTE-1) <= ix2)
                  pim[ibo] = ~0, ix += BITSPERBYTE;
               else
                  pim[ibo] |= 1 << ((BITSPERBYTE-1) - ibr), ix += 1;
               }
            }
         /* But if parity is off, next fill will begin on this line
         *  if it is a grid point, otherwise next grid point right.  */
         else {
            ix = (int)SRA((pwe->rix + um1), bs);
            if (ix < 0) ix = 0;
            }
         /* If line is horizontal, it is known to be on the current
         *  row, otherwise it was dropped during setup, and all its
         *  points must be marked without changing parity.  */
         if (pwe->sly == pwe->shy) {
            int ix2 = (int)SRA(pwe->shx, bs);
            if (ix2 > nsxm1) ix2 = nsxm1;
            while (ix <= ix2) {
               int ibo = ByteOffset(ix), ibr = BitRemainder(ix);
               if (ibr == 0 && ix + (BITSPERBYTE-1) <= ix2)
                  pim[ibo] = ~0, ix += BITSPERBYTE;
               else
                  pim[ibo] |= 1 << ((BITSPERBYTE-1) - ibr), ix += 1;
               }
            }
         /* If not horizontal, just invert parity unless we are at
         *  the low end of the line and this low end is on a grid
         *  point (to avoid double inverting by top end of next
         *  line).  */
         else if (pwe->sly & um1 || sty != pwe->sly)
            par = !par;
         } /* End loop over active lines */
      /* If parity is on and we did not reach the right edge of
      *  the box, now fill in the rest of the line.  */
      if (par) {
         int ix2 = nsxm1;
         while (ix <= ix2) {
            int ibo = ByteOffset(ix), ibr = BitRemainder(ix);
            if (ibr == 0 && ix + (BITSPERBYTE-1) <= ix2)
               pim[ibo] = ~0, ix += BITSPERBYTE;
            else
               pim[ibo] |= 1 << ((BITSPERBYTE-1) - ibr), ix += 1;
            }
         }
      } /* End iy loop */

   } /* End interior or exterior polygon fill */

/* Now go over the original vertex list a second time and draw (or
*  erase) a thick border line around the polygonal boundary.  */

if (border == 0.0) goto NoBorder;

b2 = border*border;
for (iv=iv1=0; iv<nvtx; ++iv) {
   float xb[NRVTX],yb[NRVTX];       /* Border box vertices */
   float ylo[NRVTX],yhi[NRVTX];     /* Ordered box vertices */
   float rs[NRVTX];                 /* 1/slopes of edges */
   float x1,x2,y1,y2,dx,dy,db,dd;   /* Temps */
   float ybmn,ybmx;                 /* y limits of border box */
   int   iy1,iy2,iy;                /* y range and loop control */
   int   jv,jv2;                    /* Box vertex controls */
   x1 = pgon[iv].x, y1 = pgon[iv].y;
   if (iv == lvtx || y1 < rmny)
      /* Complete one polygon */
      iv2 = iv1, iv1 = iv + 1;
   else
      /* Continue current polygon */
      iv2 = iv + 1;
   x2 = pgon[iv2].x, y2 = pgon[iv2].y;
   if (y1 < rmny) y1 += ysig;
   if (y2 < rmny) y2 += ysig;

   /* Compute locations of four vertices of a rectangle
   *  making a border of width 'border' on either side of
   *  the current edge.  */
   dx = x2 - x1, dy = y2 - y1;
   dd = sqrtf(dx*dx + dy*dy);
   /* Originally we had an abend(67) here because clearly a
   *  figure with two vertices in the same place is an error,
   *  but these actually exist in the CIT image annotations,
   *  so the code was changed to just skip these vertices.
   *  The paint code above handles these OK.  */
   if (dd < SMALL) continue;
   db = border/dd;
   dx *= db, dy *= db;
   xb[0] = x1 + dy, yb[0] = y1 - dx;
   xb[1] = x2 + dy, yb[1] = y2 - dx;
   xb[2] = x2 - dy, yb[2] = y2 + dx;
   xb[3] = x1 - dy, yb[3] = y1 + dx;
   /* Avoid a divide check in slope calculations */
   rs[0] = rs[2] = (fabsf(dx) > BigCoord*fabsf(dy)) ?
      BigCoord : dx/dy;
   rs[1] = rs[3] = (fabsf(dy) > BigCoord*fabsf(dx)) ?
      BigCoord : -dy/dx;

   /* Store items needed to compute edge intersections */
   ybmn = (float)nsy, ybmx = -1;
   for (jv=0; jv<NRVTX; ++jv) {
      jv2 = (jv + 1) & (NRVTX-1);
      if (yb[jv2] >= yb[jv])
         ylo[jv] = yb[jv], yhi[jv] = yb[jv2];
      else
         ylo[jv] = yb[jv2], yhi[jv] = yb[jv];
      if (yb[jv] < ybmn) ybmn = yb[jv];
      if (yb[jv] > ybmx) ybmx = yb[jv];
      }
   iy1 = (int)ceilf(ybmn), iy2 = (int)floorf(ybmx);
   if (iy1 < 0) iy1 = 0;
   if (iy2 > nsym1) iy2 = nsym1;

   /* Update global y limits */
   if (!par0) {
      if (iy1 < gmny) gmny = iy1;
      if (iy2 > gmxy) gmxy = iy2;
      }

   /* Loop over scan lines within current border box
   *  and fill (or erase) points on line within box.  */
   pim = pwim + iy1*ltir;
   for (iy=iy1; iy<=iy2; ++iy,pim+=ltir) {
      float fx,fy = (float)iy;
      float xbmn,xbmx;           /* x limits of scan line */
      int   ix,ix2;              /* Scan line controls */
      xbmn = (float)nsx, xbmx = -1;
      for (jv=0; jv<NRVTX; ++jv) {
         if (fy < ylo[jv] || fy > yhi[jv]) continue;
         /* If the slope is BigCoord, the edge is horizontal and
         *  both ends will get tested as solutions for the preced-
         *  ing and following lines.  */
         if (rs[jv] == BigCoord) continue;
         fx = xb[jv] + rs[jv] * (fy - yb[jv]);
         /* Given that the fx calc is skipped when y is outside the
         *  bounds of this line, fx is guaranteed to be within the
         *  line, but this could be outside the lattice.  It is
         *  easier to check for this after xbmn,xbmx are known.  */
         if (fx < xbmn) xbmn = fx;
         if (fx > xbmx) xbmx = fx;
         } /* End box vertex loop */
      /* Move the scan line in bounds and do the marking.
      *  (In the perverse case that the line is entirely above
      *  or below the lattice, the marking loop will be empty.)  */
      ix = (int)ceilf(xbmn);
      if (ix < 0) ix = 0;
      ix2 = (int)floorf(xbmx);
      if (ix2 > nsxm1) ix2 = nsxm1;
      if (mode & EPGF_RMB) {        /* Erase thick line */
         while (ix <= ix2) {
            int ibo = ByteOffset(ix), ibr = BitRemainder(ix);
            if (ibr == 0 && ix + (BITSPERBYTE-1) <= ix2)
               pim[ibo] = 0, ix += BITSPERBYTE;
            else
               pim[ibo] &= ~(1 << ((BITSPERBYTE-1) - ibr)), ix += 1;
            }
         } /* End erase loop */
      else {                        /* Draw thick line */
         while (ix <= ix2) {
            int ibo = ByteOffset(ix), ibr = BitRemainder(ix);
            if (ibr == 0 && ix + (BITSPERBYTE-1) <= ix2)
               pim[ibo] = ~0, ix += BITSPERBYTE;
            else
               pim[ibo] |= 1 << ((BITSPERBYTE-1) - ibr), ix += 1;
            }
         } /* End drawing along one scan line */
      } /* End loop over scan lines in one box */

   /* Now it is time to erase or fill in a circle of radius
   *  'border' around the current polygon vertex.  The scheme
   *  is pretty much the same as with the boxes, but simpler.  */
   iy1 = (int)ceilf(y1 - border);
   if (iy1 < 0) iy1 = 0;
   iy2 = (int)floorf(y1 + border);
   if (iy2 > nsym1) iy2 = nsym1;

   /* Update global y limits */
   if (!par0) {
      if (iy1 < gmny) gmny = iy1;
      if (iy2 > gmxy) gmxy = iy2;
      }

   pim = pwim + iy1*ltir;
   for (iy=iy1; iy<=iy2; ++iy,pim+=ltir) {
      float fy = (float)iy;
      float dy = fy - y1;
      float dy2 = dy*dy;
      float dx2 = b2 - dy2;
      float dx = dx2 > 0 ? sqrtf(dx2) : 0;
      int   ix = (int)ceilf(x1 - dx);
      int   ix2 = (int)floorf(x1 + dx);

      /* Move the scan line in bounds and do the marking */
      if (ix < 0) ix = 0;
      if (ix2 > nsxm1) ix2 = nsxm1;
      if (mode & EPGF_RMB) {        /* Erase thick line */
         while (ix <= ix2) {
            int ibo = ByteOffset(ix), ibr = BitRemainder(ix);
            if (ibr == 0 && ix + (BITSPERBYTE-1) <= ix2)
               pim[ibo] = 0, ix += BITSPERBYTE;
            else
               pim[ibo] &= ~(1 << ((BITSPERBYTE-1) - ibr)), ix += 1;
            }
         } /* End erase loop */
      else {                        /* Draw thick line */
         while (ix <= ix2) {
            int ibo = ByteOffset(ix), ibr = BitRemainder(ix);
            if (ibr == 0 && ix + (BITSPERBYTE-1) <= ix2)
               pim[ibo] = ~0, ix += BITSPERBYTE;
            else
               pim[ibo] |= 1 << ((BITSPERBYTE-1) - ibr), ix += 1;
            }
         } /* End drawing along one scan line */
      } /* End loop over scan lines in one circular joint */

   } /* End vertex loop for border drawing */

/* Save global y limits for GetNextPointInEpgf routine */

NoBorder:
pit->iy = (long)(gmny - 1);
pit->topy = (long)(gmxy + 1);

} /* End InitEpgfIndexing */


/*---------------------------------------------------------------------*
*                       GetNextPointInEpgf                             *
*                                                                      *
*  Synopsis:                                                           *
*     int GetNextPointInEpgf(IterEpgf *pit)                            *
*                                                                      *
*  Arguments:                                                          *
*     pit               Ptr to IterEpgf structure initialized by       *
*                       previous call to InitEpgfIndexing.             *
*  Returns:                                                            *
*     pit->ix,iy        x,y coords of next lattice point in figure.    *
*     pit->ioff         Offset of point ix,iy from lattice origin.     *
*     Function value    TRUE if a grid point was found inside the      *
*                          given figure.  Values in pit->ix,iy, and    *
*                          ioff are valid.                             *
*                       FALSE if all grid points in the figure have    *
*                          already been returned.  Values in pit->ix,  *
*                          iy, and ioff are no longer valid.           *
*  Note:                                                               *
*     If no points are eligible (because the requested figure lies     *
*  outside the base lattice), GetNextPointInEpgf returns FALSE at      *
*  once.  This is not considered an error condition.                   *
*---------------------------------------------------------------------*/

int GetNextPointInEpgf(IterEpgf *pit) {

   si32 ibo,ibr;           /* Byte offset in pim, bit remainder */

   /* All done, make quick exit */
   if (pit->iy >= pit->topy) return FALSE;

   /* Advance to next x.  If this goes past the end of the row,
   *  advance to next y.  */
TryNextX:
   if ((pit->ix += 1) >= (long)pit->nsx) {
      if (++pit->iy >= pit->topy) return FALSE;
      /* Start a new row--set yoff and initial x coord */
      pit->ix = 0;
      pit->pim = pit->pwim + pit->iy*pit->ltir;
      pit->yoff = pit->iy*pit->nsx;
      } /* End setting up new scan row */

   /* If on a byte boundary, and whole byte is 0, advance x
   *  by length of a byte and try again  */
   ibo = (si32)ByteOffset(pit->ix);
   ibr = (si32)BitRemainder(pit->ix);
   if (ibr == 0) while (pit->ix + BITSPERBYTE <= pit->nsx) {
      if (pit->pim[ibo]) break;
      pit->ix += BITSPERBYTE;
      ibo += 1;
      }

   /* Got a byte with a nonzero entry (marked pixel).
   *  Return coords of this pixel and set for next call.  */
   if (pit->pim[ibo] & 1 << ((BITSPERBYTE-1) - ibr)) {
      pit->ioff = pit->yoff + pit->ix;
      return TRUE;
      }
   goto TryNextX;

   } /* End GetNextPointInEpgf() */
