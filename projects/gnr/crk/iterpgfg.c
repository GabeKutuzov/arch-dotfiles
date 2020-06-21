/* (c) Copyright 2011-2013, The Rockefeller University *11115* */
/* $Id: iterpgfg.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                             iterpgfg.c                               *
*                                                                      *
*  This module contains routines that may be used to visit in turn all *
*  the points on some rectangular lattice that fall either within or   *
*  outside a figure defined by one or more possibly concave, possibly  *
*  overlapping, polygons or polygons with holes. Results are presented *
*  in order x fast moving, y slow.                                     *
*                                                                      *
*  This version works with coordinates of type float.  Another version *
*  will need to be written if double precision coordinates are needed. *
*                                                                      *
*  The user must provide an instance of the IterPgfg data structure    *
*  for each invocation of the routines.  This structure is used both   *
*  to store the internal state of the iterator and to return results.  *
*                                                                      *
*  The user must call InitPgfgIndexing to initialize indexing for      *
*  some particular figure.  Each time another grid point is wanted,    *
*  a call to GetNextPointInPgfg returns the x,y coordinates of the     *
*  next point and its offset from the start of the lattice array.      *
*                                                                      *
*  The user may carry out multiple iterations at the same time by      *
*  providing a separate IterPgfg structure for each.  The IterPgfg     *
*  structure and the two calls are prototyped in iterpgfg.h.  If the   *
*  figure is known to consist of a single convex polygon, the iterpoly *
*  routines may be faster.                                             *
*                                                                      *
*  Design Notes:  Coordinates are kept in fixed-point for faster       *
*  arithmetic and easier special-case testing.  The binary scale       *
*  is set according to the size of the raster for best accuracy.       *
*  Names of scaled coordinates will generally begin with 's'.          *
*  Returned ix,iy,ioff are longs for compat with other iterators.      *
************************************************************************
*  V1A, 12/12/11, GNR - Initial version                                *
*  ==>, 12/17/11, GNR - Last date before committing to svn repository  *
*  Rev, 10/24/13, GNR - SRA() for signed '>>'                          *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "iterpgfg.h"
#include "rkarith.h"
#include "rksubs.h"

#define  MNNVTX    3       /* Minimum vertices in a polygon */
#define  MNBSCL    3       /* Minimum reasonable binary scale */
#define  MXSCBT   29       /* Maximum bits in scaled coords */
/* Bits in par,par0 */
#define  WORK_PAR  1       /* Current line-crossing parity */
#define  INIT_PAR  2       /* Initial line-crossing parity */
#define  FORCEINT  4       /* Force internal for one move */

/*---------------------------------------------------------------------*
*                        InitPgfgIndexing                              *
*                                                                      *
*  Synopsis:                                                           *
*     void InitPgfgIndexing(IterPgfg *pit, xyf *pgon,                  *
*        IPgfgWk *work, si32 nsx, si32 nsy, int nvtx)                  *
*                                                                      *
*  Arguments:                                                          *
*     pit                  Ptr to IterPgfg structure that pgm will use *
*                          to hold the state of the indexing routine.  *
*     pgon                 Pairs of x,y coordinates defining in order  *
*                          the vertices of the polygon(s).  Concave    *
*                          polygons and multiple polygons are allowed. *
*                          Coordinates must lie in the ranges          *
*                          -nsx <= x < 2*nsx, -nsy <= y < 2*nsy.       *
*                          To indicate that a point is the last vertex *
*                          in its polygon, store PgfgPly(y,nsy) in     *
*                          place of the y coordinate.  (PgfgPly() is   *
*                          a macro defined in iterpgfg.h.              *
*                          Subsequent vertices may define additional   *
*                          polygons or holes in the original polygon.  *
*     work                 Ptr to a work area large enough to hold     *
*                          nvtx IPgfgWk structs defined in iterpgfg.h. *
*     nsx,nsy              Size of full rectangular lattice along x,y. *
*                          These need to be less than ~2^24 to reduce  *
*                          round-off errors.                           *
*     nvtx                 Number of vertices of pgon.  A negative     *
*                          value indicates that points outside the     *
*                          figure defined by the polygons should be    *
*                          returned.                                   *
*                                                                      *
*  Returns:                                                            *
*     pit is initialized for later calls to GetNextPointInPgfg         *
*                                                                      *
*  Errors:                                                             *
*     An abexit code 67 error occurs if |nvtx| < 3 or if any one       *
*  polygon in the figure has fewer than 3 vertices.  Error 68 is       *
*  generated if any coordinate is outside the specified limits.        *
*  (This condition is likely to be the result of a coding error,       *
*  but the test can be modified if a larger coord range is needed.)    *
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
*     The contents of 'work' should not be changed by the user from    *
*  the InitPgfgIndexing call until GetNextPointInPgfg returns FALSE.   *
*---------------------------------------------------------------------*/

#define XY2Fix(v) ((si32)((v)*sclm + 0.5))

void InitPgfgIndexing(IterPgfg *pit, xyf *pgon, IPgfgWk *work,
   si32 nsx, si32 nsy, int nvtx) {

   IPgfgWk *pwe,**ppwe;       /* Ptr for building edge list */
   float rmnx,rmxx;           /* Min,max allowed x coords */
   float rmny,rmxy;           /* Min,max allowed y coords */
   float sclm;                /* Scale multiplier */
   float ysig;                /* Last y in polygon signal */
   float dytest;              /* Part of slope overflow test */
   si32  imsk;                /* Integer mask */
   si32  snsx,snsy;           /* Scaled nsx,nsy */
   si32  txy;                 /* Temp x or y */
   si32  tsx1,tsx2,tsy1,tsy2; /* Temp x,y for swapping */
   int   bs,ibs;              /* Binary scale, integer scale */
   int   iv,iv1,iv2;          /* Vertex loop indices */
   int   lvtx = abs(nvtx)-1;  /* Index of the last vertex */
   int   kvtx;                /* Tricky vertex number test */
   int   mvtx;                /* Number of vertices in one polygon */

   /* Sanity checking */
   if (nsx <= 0 || nsy <= 0 || lvtx < 2) abexit(67);

   /* Determine binary scale that will just fit largest coords
   *  in an si32 variable */
   ibs = bitszs32(max(nsx,nsy));
   pit->bs = bs = MXSCBT - ibs;
   if (bs < MNBSCL) abexit(67);
   sclm = (float)(pit->um1 = 1 << bs);
   imsk = -pit->um1;
   pit->um1 -= 1;
   dytest = (float)(2 << ibs);
   snsx = nsx << bs, snsy = nsy << bs;

   /* Save polygon parameters */
   pit->wkal = pit->wkil = NULL;
   pit->nsx = nsx, pit->nsy = nsy;
   pit->smnx = -snsx, pit->smxx = snsx;
   pit->par0 = nvtx < 0 ? (WORK_PAR|INIT_PAR) : 0;

   /* Initialize building of working edge list */
   pwe = work; ppwe = &pit->wkel;
   rmnx = -(float)nsx, rmxx = (float)(nsx<<1);
   rmny = -(float)nsy, rmxy = (float)(nsy<<1);
   ysig = (float)(nsy<<PGFG_LAST_Y_SHFT);

   /* Save edge coords and slopes */
   kvtx = mvtx = 0;
   pit->topy = 0;
   for (iv=iv1=0; iv<=lvtx; ++iv) {
      float x1,x2,y1,y2;
      x1 = pgon[iv].x, y1 = pgon[iv].y;
      if (iv == lvtx || y1 < rmny) {
         /* Complete one polygon */
         iv2 = iv1, iv1 = iv + 1;
         kvtx = MNNVTX; }
      else
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
      if ((tsy1 & imsk) == (tsy2 & imsk) && tsy1 & pit->um1)
         continue;
      /* Avoid a divide check in slope calculation.  Explanation:
      *  dx/dy overflows if |(dx<<bs)/dy| >= 2^30, i.e.
      *  |dx| >= dy*(2^(30-bs)) = dy*(2^(ibs + 1)).  */
      {  float dx = (float)(tsx2 - tsx1), dy = (float)(tsy2 - tsy1);
         pwe->slope = (fabsf(dx) >= dytest*dy) ?
         (dx >= 0 ? SI32_MAX : -SI32_MAX) : XY2Fix(dx/dy); }
      /* Track top of figure */
      txy = SRA(tsy2, bs) + 1;
      if (txy > pit->topy) pit->topy = txy;
      /* Insert edge in linked list */
      *ppwe = pwe, ppwe = &pwe->pne, ++pwe;
      } /* End vertex loop */
   *ppwe = NULL;              /* End linked list */

   /* Sort the edge list on ascending y */
   pit->wkel = sort(pit->wkel,
      (int)((char *)(&((IPgfgWk *)0)->sly) - (char *)0), 2*I32SIZE, 0);

   /* Set starting y, y offset, x to force immediate new y */
   pit->ix = (long)(nsx + 1);
   if (pit->par0)             /* Returning points outside the figure */
      pit->iy = -1, pit->topy = nsy;
   else {                     /* Returning points inside the figure */
      txy = SRA((pit->wkel->sly + pit->um1), bs);
      pit->iy = (long)(max(0,txy) - 1);
      if (pit->topy > nsy) pit->topy = nsy;
      }
   pit->yoff = (long)nsx * pit->iy;

   } /* End InitPgfgIndexing() */


/*---------------------------------------------------------------------*
*                       GetNextPointInPgfg                             *
*                                                                      *
*  Synopsis:                                                           *
*     int GetNextPointInPgfg(IterPgfg *pit)                            *
*                                                                      *
*  Arguments:                                                          *
*     pit               Ptr to IterPgfg structure initialized by       *
*                       previous call to InitPgfgIndexing.             *
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
*  outside the base lattice), GetNextPointInPgfg returns FALSE at      *
*  once.  This is not considered an error condition.                   *
*---------------------------------------------------------------------*/

int GetNextPointInPgfg(IterPgfg *pit) {

   IPgfgWk *pwe,**ppwe,*qwe;  /* Ptrs to working edge */
   long ochk;                 /* Overflow check flag */
   si32 stx,sty;              /* Scaled current x,y */
   int  qint;                 /* True if in internal state */
   /* Internal/external state as function of parity bits */
   static byte intck = 0x3a;

   if (pit->ix >= (long)pit->nsx) {
      if (pit->iy >= (long)pit->topy) return FALSE;

/* Advance to a new row.  Incrementing after testing and testing again
*  is to deal with the unlikely case that this routine is called
*  billions of times after the first time it returns FALSE.  */

TryNextY:
      if (++pit->iy >= (long)pit->topy) return FALSE;
      sty = (si32)(pit->iy << pit->bs);
      pit->par = pit->par0;
      /* Prepend inactive list to active list and
      *  initialize an empty inactive list */
      if (pit->wkil) {
         *pit->ppil = pit->wkal, pit->wkal = pit->wkil;
         pit->wkil = NULL; }
      pit->ppil = &pit->wkil;
      /* Check for edges that can be deleted from active list */
      for (ppwe=&pit->wkal; (pwe=*ppwe); ) { /* Assignment intended */
         if (sty > pwe->shy) *ppwe = pwe->pne;
         else                ppwe = &pwe->pne;
         }
      /* Update x crossing for edges already on the active list */
      for (pwe=pit->wkal; pwe; pwe=pwe->pne) {
         si32 schng,oldrix = pwe->rix;
         pwe->rix += pwe->slope;
         /* Check for overflow/underflow */
         schng = ((pwe->rix ^ oldrix) & (pwe->rix ^ pwe->slope)) < 0;
         if (pwe->slope >= 0) {  /* Positive slope */
            if (pwe->rix > pit->smxx | schng)
               pwe->rix = pit->smxx; }
         else {                  /* Negative slope */
            if (pwe->rix < pit->smnx | schng)
               pwe->rix = pit->smnx; }
         } /* End loop over edges on active list */
      /* Check for new,higher edges to be added to active list.
      *  Save user's e64 state and switch to E64_FLAGS mode.  */
      e64push(E64_FLAGS, &ochk);
      while (pit->wkel && pit->wkel->sly <= sty) {
         pwe = pit->wkel;
         qwe = pwe->pne;
         pwe->pne = pit->wkal;
         pit->wkal = pwe;
         pit->wkel = qwe;
         /* Compute starting x.  ochk will return overflow status */
         ochk = 0;
         pwe->rix = mrsrsle(sty - pwe->sly, pwe->slope, pit->bs, 1) +
            pwe->slx;
         /* Check for overflow/underflow */
         if (pwe->slope >= 0) {  /* Positive slope */
            if (pwe->rix > pit->smxx | ochk)
               pwe->rix = pit->smxx;
            }
         else {                  /* Negative slope */
            if (pwe->rix < pit->smnx | ochk)
               pwe->rix = pit->smnx;
            }
         }
      e64pop();
      /* If the list is empty, and INIT_PAR is not set, advance y to
      *  next edge (this could be a discontinuous figure) or top */
      if (!pit->wkal) {
         if (pit->par && pit->iy < pit->topy) {
            pit->ix = -1; goto FinNewRow; }
         if (pit->wkel) {
            pit->iy = (long)SRA((pit->wkel->sly-1), pit->bs);
            pit->yoff = pit->iy * (long)pit->nsx;
            }
         else
            pit->iy = pit->topy;
         goto TryNextY;
         }
      /* Sort the active list on x.  A simple insertion sort is per-
      *  formed here because crk sort assumes unsigned keys and the
      *  list is short so any method will do.  */
      pwe = pit->wkal, pit->wkal = NULL;
      while (pwe) {
         IPgfgWk *rwe;
         qwe = pwe->pne;
         for (ppwe=&pit->wkal;
               (rwe = *ppwe) && (pwe->rix > rwe->rix);
               ppwe=&rwe->pne) ;
         /* Insert in output list */
         pwe->pne = rwe, *ppwe = pwe;
         pwe = qwe;
         }
      /* Start a new row--set yoff and initial x coord */
      pit->ix = SRA((pit->wkal->rix-1), pit->bs);
      if (pit->par && pit->ix >= 0) pit->ix = -1;
FinNewRow:
      pit->yoff += (long)pit->nsx;
      } /* End setting up new scan row */

/* Advance to a new column */

TryNextX:
   if ((pit->ix += 1) >= (long)pit->nsx) goto TryNextY;
   stx = (si32)(pit->ix << pit->bs);
   sty = (si32)(pit->iy << pit->bs);
   /* For each edge crossed, reverse parity and remove the line
   *  from the wkal list by moving it to the wkil inactive list.  */
   for (ppwe=&pit->wkal; (pwe=*ppwe) && pwe->rix <= stx; ) {
      /* If line is horizontal, it is known to be on the current row,
      *  otherwise it was dropped during setup.  If still active, set
      *  FORCEINT parity bit, otherwise just remove line from wkal
      *  list.  rix is not used, so no need to update.  */
      if (pwe->sly == pwe->shy) {
         if (stx <= pwe->shx) {
            pit->par |= FORCEINT;
            goto KeepThisLine; }
         else
            goto DropThisLine;
         }
      if (pwe->rix == stx)             /* On a grid point */
         pit->par |= FORCEINT;
      /* Invert parity unless at low end of line and
      *  this low end is on a grid line.  */
      if (pwe->sly & pit->um1 || sty != pwe->sly)
         pit->par ^= WORK_PAR;
DropThisLine:
      *ppwe = pwe->pne, pwe->pne = NULL;
      *pit->ppil = pwe, pit->ppil = &pwe->pne;
      continue;
KeepThisLine:
      ppwe = &pwe->pne;
      }
   /* If inside the box and now drawing, return this point */
   qint = SRA(intck, pit->par) & WORK_PAR;
   pit->par &= ~FORCEINT;
   if (pit->ix >= 0 && qint) {
      pit->ioff = pit->yoff + pit->ix;
      return TRUE;
      }
   /* Not drawing.  There are two cases:
   *  (1) ix < 0 and we are scanning for parity.  Advance ix
   *      to next line crossing or -1, whichever is closer.  */
   pwe = pit->wkal;
   if (pit->ix < 0) {
      if (pwe) {
         si32 sx = (pwe->sly == pwe->shy) ? pwe->shx : pwe->rix;
         long nx = (long)SRA((sx-1), pit->bs);
         pit->ix = min(nx, -1);
         }
      else
         pit->ix = -1;
      goto TryNextX;
      }
   /* (2) In the box but entered (or left) a non-drawing segment.
   *      Advance until another edge is crossed or there are no
   *      more edges.  */
   if (pwe) {
      si32 sx;
      if (pwe->sly == pwe->shy)
         sx = pit->par0 & INIT_PAR ? pwe->shx : pwe->slx-1;
      else
         sx = pit->par0 & INIT_PAR ? pwe->rix : pwe->rix-1;
      pit->ix = (long)SRA(sx, pit->bs);
      goto TryNextX;
      }
   goto TryNextY;

   } /* End GetNextPointInPgfg() */
