/* (c) Copyright 2007-2010, The Rockefeller University *11115* */
/* $Id: bbdcrngr.c 28 2017-01-13 20:35:15Z  $ */
/***********************************************************************
*                      BBD (Client Side) Package                       *
*                             bbdcrngr.c                               *
*                                                                      *
*  Implement a family of virtual range finders (differences between    *
*  IR and laser not significant here) for a virtual BBD.  The range    *
*  finders are assumed to be mounted on a platform that has a known    *
*  location and orientation relative to some bitmap at each simcycle.  *
*                                                                      *
*  A control integer, jrfcid, is provided in the Ranger data struct    *
*  to indicate what test, if any, is to be used to determine that an   *
*  object has intersected the range-finder beam.  At this writing,     *
*  only one value of jrfcid has been implemented, namely, RFC_GRAY,    *
*  which checks that the pixel is within a particular range of gray    *
*  values.  Other tests can be added as required based on template     *
*  code here.                                                          *
*                                                                      *
*  In addition, an array of OppPos structures can be provided in which *
*  the positions, sizes, and orientations of one or more rectangular   *
*  opponents can be specified.  If one of these comes within range of  *
*  a sensor, OPP_HIT_COUNT is added to the hitlist entry for that      *
*  sensor and an estimate of the minimum distance is included in the   *
*  calculation of the minimum distance to the obstacle.                *
*                                                                      *
*  These routines are not strictly part of the bbdc package, because   *
*  they interact with specific client data structures and pixel access *
*  routines, in this case, with the Allegro game library.  However,    *
*  this file is placed in the bbd source folder where it may serve as  *
*  a useful prototype for implementation in other BBD environments.    *
*                                                                      *
*  The only part of these routines that is specific to the Allegro     *
*  game library is the access to red, green, and blue components of    *
*  pixels in a BITMAP.  This code is clearly marked with comments in   *
*  the rngrget() routine, so it should be relatively easy to replace   *
*  this code with suitable alternatives in a different environment.    *
*                                                                      *
*  To implement a set of range finders, three calls are needed:        *
*  (1) At the start of the run, when the parameters of the desired     *
*      range finders and of the underlying visual bitmap are known,    *
*      call rngrinit().                                                *
*  (2) During each simulation cycle, when the current location and     *
*      orientation of the platform are known, call rngrget() to        *
*      check the receptive field(s) of each sensor and return a        *
*      list of all hits that are found.                                *
*  (4) At the end of the run, call rngrkill() once for each call to    *
*      rngrinit in order to release any allocated storage.             *
*                                                                      *
*  Error Handling:                                                     *
*     All errors are terminal and result in a call to abexit() with a  *
*     suitable message.  There is nothing useful the caller can do.    *
*     This package is assigned abexit error codes 156 and 157.         *
********************************************************************** *
*  V1A, 08/23/07, GNR - New program                                    *
*  Rev, 12/04/07, GNR - Correct trapezoid angclr calc, better errmsgs  *
*  V1B, 01/08/08, GNR - Add return of obstacle distances and angles    *
*  V1C, 01/21/08, GNR - Use rectangle for opponent detection           *
*  V1D, 01/31/08, GNR - RFC_NONE, distances, not hits, for "squeeze"   *
*  Rev, 02/10/08, GNR - Handle all overlap cases, add RNGROPT_NODA     *
*  Rev, 02/16/08, GNR - Add RNGROPT_DHITS                              *
*  Rev, 02/21/08, GNR - Add BMOff concept                              *
*  ==>, 02/22/08, GNR - Last mod before committing to svn repository   *
*  Rev, 04/09/10, GNR - Minor type changes for 64-bit compilation      *
***********************************************************************/

#define BBD_ALLEGRO

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "sysdef.h"
#include "rksubs.h"
#include "itersph.h"
#include "bbd.h"
#include "bbdcxtra.h"
#include "allegro.h"          /* ALLEGRO DEPENDENCY */

#define PI      3.141592654
#define TORADS  0.017453293   /* Degrees to radians converter */
#define TODEGS 57.29577951    /* Radians to degrees converter */
#define HCIRC 180.0           /* Degrees in half a circle */
#define nE2     2             /* Number of coords in 2-space */
#define nRC     4             /* Number of corners of a rectangle */
#define NCOLR3  3             /* Number of colors in color model */
#define EPSDIST 1E-6          /* A tiny distance */

#ifdef DEBUG                  /*** DEBUG ***/
FILE *pdbgf;
long wrejct[4];
#endif

/* Define offsets of colors in Col_C24 pixel */
enum colors { Red, Green, Blue } ;
/* Define reasons for rejection of a pixel */
enum whyrej { grayok=0, ming, maxg, maxrad } ;

/* Define struct for communication between rngrget and rngrohit */
struct HitInfo {
   double xs[nRC];            /* Sensor x coords rel to platform */
   double ys[nRC];            /* Sensor y coords rel to platform */
   double crad;               /* Radius (circular sensors only) */
   double cdx,cdy,cd2;        /* Closest distance x,y,squared */
   double mxdi2;              /* Current working mxdist squared */
   int    kopt;               /* Option flags */
   int    nopp;               /* Number of opponents */
   };

/*---------------------------------------------------------------------*
*                              rngrgray                                *
*  Local (static) routine to check that a pixel is in the range of     *
*  gray values of interest when jrfcid == RFC_GRAY.  This is a         *
*  separate function so it can be called from within the pixel         *
*  generators for the various supported geometries.                    *
*                                                                      *
*  Synopsis:                                                           *
*     int rngrgray(RngrSet *prs, int pix)                              *
*                                                                      *
*  Arguments:                                                          *
*     prs      Ptr to RngrSet returned by previous call to rngrinit(). *
*     pix      A 24-bit BITMAP pixel value.                            *
*                                                                      *
*  Global parameters (inherited from rngrinit call)                    *
*     MinGray    Minimum (red or green or blue)                        *
*     MaxGray    Maximum (red or green or blue)                        *
*     MaxCRad    Maximum radius in color space from gray line          *
*                                                                      *
*  Value returned:                                                     *
*     0     if this pixel does constitute a hit                        *
*     1     if this pixel rejected due to MinGray test                 *
*     2     if this pixel rejected due to MaxGray test                 *
*     3     if this pixel rejected due to MaxCRad test                 *
*---------------------------------------------------------------------*/

static int rngrgray(RngrSet *prs, int pix) {

   int r,g,b;                 /* Red, green, blue values */
   int rg,gb,br;              /* Color differences */

/*---------------------------------------------------------------------*
*                      BEGIN ALLEGRO DEPENDENCY                        *
*---------------------------------------------------------------------*/
   if (pix == -1) {
      /* Point is outside the bitmap */
      r = g = b = 0;
      }
   else {
      /* Point is inside the bitmap, pick up color values */
      r = getr24(pix);
      g = getg24(pix);
      b = getb24(pix);
      }
/*---------------------------------------------------------------------*
*                       END ALLEGRO DEPENDENCY                         *
*---------------------------------------------------------------------*/

   if (r < prs->MinGray || g < prs->MinGray || b < prs->MinGray)
      return ming;
   if (r > prs->MaxGray || g > prs->MaxGray || b > prs->MaxGray)
      return maxg;

/* Use a cross product in color space to determine closeness to
*  gray line.  */

   rg = r - g, gb = g - b, br = b - r;
   if (rg*rg + gb*gb + br*br > prs->MaxCRad2X3)
      return maxrad;

   return grayok;
   } /* End rngrgray() */


/*---------------------------------------------------------------------*
*                              rngrohit                                *
*                                                                      *
*  Private routine to determine whether a range finder detects a       *
*  rectangular opponent.                                               *
*                                                                      *
*  Arguments:                                                          *
*     pop      Ptr to array of HI.nopp OppPos structs giving specs     *
*              of one or more opponents (T2) (see bbdcxtra.h).         *
*     pHI      Ptr to a HitInfo array containing info that describes   *
*              the geometry of the current sensor, the number of       *
*              opponents described in the pOpp array, and slots for    *
*              information to be returned to the caller.  This         *
*              arrangement allows the call to rngrohit to be moved     *
*              outside the nopp loop at some savings in execution      *
*              time for calls and duplicated code in rngrget.          *
*              N.B.  All coords must be relative to the center of      *
*              the sensor platform in a Euclidean system with the Y    *
*              axis oriented according to YAX_HAND (bbdcxtra.h) and    *
*              using the same axial units as are used for the rx,ry,   *
*              hw,hh values in the OppPos structure.                   *
*     jrfshp   Integer indicating the type of sensor.                  *
*                                                                      *
*  Return value:                                                       *
*     NO  (0)  Opponent out of range of this detector.                 *
*     YES (1)  Opponent detected.                                      *
*     Squared distance from platform to nearest point on opponent and  *
*     its x and y components (also in YAX_HAND) are returned in the    *
*     "private" part of the OppPos struct.  N.B.  For quicker code,    *
*     and because it probably makes good sense, the nearest point      *
*     returned is not constrained to be inside the sensor RF.          *
*                                                                      *
*  Algorithm:  See design notes.                                       *
*  Rev, 02/10/08, GNR - Make all tests mathematically complete, i.e.   *
*  remove assumptions that sensors can never fit inside opponent boxes *
*  or vice-versa.  In the case where the platform origin lies inside   *
*  an opponent box, return a distance of zero but cdx,cdy pointing in  *
*  the direction opposite the nearest escape vector, consistent with   *
*  the more normal case where the platform origin is outside the box.  *
*---------------------------------------------------------------------*/

static int rngrohit(OppPos *pop, struct HitInfo *pHI, int jrfshp) {

   OppPos *pope = pop + pHI->nopp;
   int rc = NO;                  /* Return value */

   for ( ; pop<pope; ++pop) {
      double xhat[nRC+1],yhat[nRC+1];  /* Rotated vertices */
      double ow = pop->hw;       /* Local copies of OppPos data */
      double oh = pop->hh;
      double oc = pop->co;
      double os = pop->so;
      double ox = pop->rx;
      double oy = pop->ry;
      int    iv,ie;              /* Vertex, edge loop indices */

/*---------------------------------------------------------------------*
*      Stage I.  Check for an overlap of sensor and opponent box       *
*---------------------------------------------------------------------*/

      switch (jrfshp) {          /* Which geometry? */
      case RFS_CIRCLE: {         /* CIRCLE */
         double dw,dh,dx,dy;
         double tsx = pHI->xs[0] - ox;
         double tsy = pHI->ys[0] - oy;
         double tr = pHI->crad;
         /* Check outer bounds of opponent rectangle */
         dw = fabs(oc*tsx YOP os*tsy);
         if ((dx = dw - ow) > tr) continue;
         dh = fabs(os*tsx YOM oc*tsy);
         if ((dy = dh - oh) > tr) continue;
         /* Check corners */
         if (dx > 0.0 && dy > 0.0 && (dx*dx + dy*dy) > tr*tr) continue;
         goto GotOverlap;
         } /* End RFS_CIRCLE local scope */
      case RFS_TRAPEZ:           /* TRAPEZOID */
      case RFS_RECTGL:           /* RECTANGLE */
         /* Rotate coords to opponent axes and perform
         *  initial simple check for overlap  */
         for (iv=0; iv<nRC; ++iv) {
            double tsx = pHI->xs[iv] - ox;
            double tsy = pHI->ys[iv] - oy;
            xhat[iv] = oc*tsx YOP os*tsy;
            yhat[iv] = os*tsx YOM oc*tsy;
            if (fabs(xhat[iv]) <= ow && fabs(yhat[iv]) <= oh)
               goto GotOverlap;
            }
         /* Even if no vertex is inside the opponent box, there
         *  can still be overlapped edges.  Test for those now.
         *  Make vertex array circular */
         xhat[nRC] = xhat[0], yhat[nRC] = yhat[0];
         for (ie=0; ie<nRC; ++ie) {
            double tdx = xhat[ie+1] - xhat[ie];
            double tdy = yhat[ie+1] - yhat[ie];
            double txy = yhat[ie]*xhat[ie+1] - yhat[ie+1]*xhat[ie];
            double thdx = oh*tdx, twdy = ow*tdy;
            if (fabs(thdx) >= fabs(twdy)) {
               /* Line is closer to horizontal, solve for
               *  y = f(x) to avoid division by small tdy  */
               double ymin = min(yhat[ie], yhat[ie+1]);
               double ymax = ymin + fabs(tdy);
               double ty;
               ty = (twdy + txy)/tdx;
               if (fabs(ty) <= oh && ymin <= ty && ty <= ymax)
                  goto GotOverlap;
               ty = (-twdy + txy)/tdx;
               if (fabs(ty) <= oh && ymin <= ty && ty <= ymax)
                  goto GotOverlap;
               }
            else {
               /* Line is closer to vertical */
               double xmin = min(xhat[ie], xhat[ie+1]);
               double xmax = xmin + fabs(tdx);
               double tx;
               tx = (thdx - txy)/tdy;
               if (fabs(tx) <= ow && xmin <= tx && tx <= xmax)
                  goto GotOverlap;
               tx = (-thdx - txy)/tdy;
               if (fabs(tx) <= ow && xmin <= tx && tx <= xmax)
                  goto GotOverlap;
               }
            } /* End edge loop */
         continue;               /* No overlap, goto next OppPos */
      default:                   /* Unrecognized type */
         abexitm(157, "Unknown sensor geometry");
         } /* End jrfshp switch */

/*---------------------------------------------------------------------*
*              Stage II.  Find direction to nearest exit               *
*                                                                      *
*  This code ignores the sensor for both speed and simplicity.         *
*  Presumably the caller is interested in the closest distance of      *
*  approach to the opponent object, whether or not it happens to       *
*  fall inside the RF of the present sensor.                           *
*                                                                      *
*  There are two major cases:                                          *
*  (1) If the platform box does not overlap the opponent box, it is    *
*      just necessary to find the nearest box edge to the origin, and  *
*      then the nearest point on that edge (which may or may not be    *
*      one of its ends).                                               *
*  (2) If the platform overlaps the opponent box, then in principle    *
*      it is necessary to consider both rectangles.  The distance is   *
*      zero, but the best exit may be either the perpendicular dist-   *
*      ance the farthest vertex inside either box has to travel to     *
*      get to the nearest edge, or the farthest distance one of the    *
*      edges has to travel to get to a vertex, whichever is less.      *
*      However, this added complication is unlikely to be of use to    *
*      a BBD entity in a real situation, where overlap will generally  *
*      be avoided before it happens.  Hence, for the present, the      *
*      platform box is is ignored and the solution is case (1) is      *
*      taken, rotated 180 degrees.                                     *
*---------------------------------------------------------------------*/

GotOverlap: if (pHI->kopt & RNGROPT_NODA) return YES;
      {
      double dx,dy;              /* Coords of T1 relative to T2 */
      double d,dd;               /* A distance or its square */
      double dx1,dy1,dx2,dy2;    /* Coords of two closest points */
      double dd1,dd2;            /* Smallest two distances (squared) */
      double wc,ws,hc,hs;        /* Width,height * cos,sine */
      wc = ow*oc, ws = ow*os, hc = oh*oc, hs = oh*os;
      /* First corner */
      dx1 = ox - wc + hs, dy1 = oy YOP hc YOP ws;
      dd1 = dx1*dx1 + dy1*dy1;
      /* Second corner */
      dx = ox + wc + hs, dy = oy YOP hc YOM ws;
      dd = dx*dx + dy*dy;
      if (dd < dd1) {
         dx2 = dx1, dy2 = dy1, dd2 = dd1;
         dx1 = dx,  dy1 = dy,  dd1 = dd; }
      else
         dx2 = dx, dy2 = dy, dd2 = dd;
      /* Third corner */
      dx = ox + wc - hs, dy = oy YOM hc YOM ws;
      dd = dx*dx + dy*dy;
      if (dd < dd1) {
         dx2 = dx1, dy2 = dy1, dd2 = dd1;
         dx1 = dx,  dy1 = dy,  dd1 = dd; }
      else if (dd < dd2)
         dx2 = dx, dy2 = dy, dd2 = dd;
      /* Fourth corner */
      dx = ox - wc - hs, dy = oy YOM hc YOP ws;
      dd = dx*dx + dy*dy;
      if (dd < dd1) {
         dx2 = dx1, dy2 = dy1, dd2 = dd1;
         dx1 = dx,  dy1 = dy,  dd1 = dd; }
      else if (dd < dd2)
         dx2 = dx, dy2 = dy, dd2 = dd;

      /* At this point, we have the two closest corners.  The closest
      *  point could be at the nearest corner or possibly on the line
      *  between the two closest.  So now calculate the perpendicular
      *  distance and whether it falls between the two endpoints.  */
      dx = dx2 - dx1, dy = dy2 - dy1;
      d = dx*dx1 + dy*dy1, dd = dx1*dx1 + dy1*dy1;
      if (d > 0.0 && d < dd) {
         /* Best point falls between dd1 and dd2 on that line */
         double r = d/dd;
         dx1 += r*dx, dy1 += r*dy, dd -= r*d;
         }

      /* Reverse dx1,dy1 directions if platform origin is inside
      *  the opponent box and set the distance to zero.  */
      dx = oc*ox YOP os*oy, dy = os*ox YOM oc*oy;
      if (fabs(dx) <= ow && fabs(dy) <= oh)
         dx1 = -dx1, dx2 = -dx2, dd = 0.0;

      /* Record shortest distance across opponents */
      if (dd < pHI->mxdi2)
         rc = YES;
      if (dd < pHI->cd2)
         pHI->cdx = dx1, pHI->cdy = dy1, pHI->cd2 = dd;
      }} /* End local scope and pop loop */

   return rc;
   } /* End rngrohit() */


/***********************************************************************
*                              rngrinit                                *
*                Initialize and check range finder set                 *
*----------------------------------------------------------------------*
*                                                                      *
*  Synopsis:                                                           *
*     RngrSet rngrinit(BITMAP *psBM, BITMAP *PdBM, Ranger *prng,       *
*        BMOff *pbmo, int nrng, int nbmo, int MinGray, int MaxGray,    *
*        int MaxCRad, float hwid, float MTC)                           *
*                                                                      *
*  Prototyped in:                                                      *
*     bbdcxtra.h                                                       *
*                                                                      *
*  Arguments:                                                          *
*     psBM     Ptr to BITMAP structure defining an Allegro bitmap      *
*              where the image to be viewed by the sensors is stored.  *
*              Note that it is assumed the image is always in the      *
*              same place, just the contents change on each rngrget()  *
*              call.                                                   *
*     pdBM     Ptr to BITMAP where sensors are to be drawn.            *
*     prng     Ptr to an array of nrng Ranger structures that have     *
*              been initialized by the caller with the parameters      *
*              indicated in the header file.  Each Ranger struct       *
*              results in generation of a pair of sensors at a given   *
*              angle from the straight-ahead direction (rfang = 0),    *
*              or a single sensor when rfang is exactly 0 or 180 deg.  *
*              Sensors must be in order from low angle to high angle.  *
*     pbmo     Ptr to an array of nbmo BMOFF structures containing     *
*              the offsets within the psBM and pdBM structures where   *
*              pixels corresponding to x,y coordinates 0 passed to     *
*              rngrget are to be found.                                *
*     nrng     Number of Ranger structures in the prng array.          *
*     nbmo     Number of BMOff structures in the pbmo array.           *
*     MinGray  See discussion above for rngrgray() function.           *
*     MaxGray  See discussion above for rngrgray() function.           *
*     MaxCRad  See discussion above for rngrgray() function.           *
*     hwid     Half-width of moving platform on which sensors are      *
*              mounted, for use in calculating clearance angles.       *
*              Set to 0 to omit calculation of clearance angels.       *
*     MTC      Minimum turn clearance within which sensors must be     *
*              checked when rngrget() called in RNGROPT_SPEC modes.    *
*                                                                      *
*  Return value:                                                       *
*     Pointer to a RngrSet structure.  This pointer must be passed     *
*     to subsequent calls to rngrget() and rngrkill() for this set     *
*     of sensors.  Computed variables in Ranger structs are set.       *
***********************************************************************/

RngrSet *rngrinit(BITMAP *psBM, BITMAP *pdBM, Ranger *prng,
      BMOff *pbmo, int nrng, int nbmo, int MinGray, int MaxGray,
      int MaxCRad, float hwid, float MTC) {

   RngrSet *prs;
   Ranger  *prf,*prfe,**ppb;
   float   d1,d2,dnct;        /* Dist to near corner of trapezoid */
   int     irf,hnas = -1;

   if (hwid < 0.0) abexitm(156, "Platform width is < 0.0");

   prs = (RngrSet *)mallocv(sizeof(RngrSet), "Ranger Set");
   ppb = prs->pprh = (struct Ranger_type **)
      mallocv(2*nrng*sizeof(void *), "Ranger Set");

   prs->psBM = psBM;
   prs->pdBM = pdBM;
   prs->prng = prng;
   prs->pbmo = pbmo;
   prs->hwid = hwid;
   prs->MTC  = MTC;
   prs->nrng = nrng;
   prs->nbmo = nbmo;
   prs->MinGray = MinGray;
   prs->MaxGray = MaxGray;
   prs->MaxCRad2X3 = 3 * MaxCRad * MaxCRad;

   prfe = prng + nrng;
   for (prf=prng,irf=1; prf<prfe; ++prf,++irf) {

/* Do a little basic parameter checking and compute angsub,
*  angclr, and mysize for each geometry.  */

      if (prf->rfang < 0.0 || prf->rfang > HCIRC)
         abexitm(156, ssprintf(NULL, "Range finder %d: Angle not in "
            "range 0 to 180 deg.", irf));
      if (prf->rfsra <= 0.0)
         abexitm(156, ssprintf(NULL, "Range finder %d: Radius is "
            "<= 0.0", irf));
      if (prf->jrfcid != RFC_GRAY)
         abexitm(156, ssprintf(NULL, "Ranger finder %d: Unsupported "
            "color signature", irf));
      prf->angclr = 0.0;         /* Default if hwid <= 0.0 */
      switch (prf->jrfshp) {
      case RFS_CIRCLE:
         prf->mxd = prf->rfdis + prf->rfsra;
         prf->mysize = (int)(PI*prf->rfsra*prf->rfsra);
         /* Rev, 01/13/08, GNR - If hwid == 0, the circle is allowed
         *  to overlap the center of the platform, in which case
         *  angsub and angclr are meaningless and set to 0.0  */
         if (hwid > 0.0) {
            if (prf->rfsra + hwid >= prf->rfdis)
               abexitm(156, ssprintf(NULL, "Range finder %d: Radius "
                  "+ platform size exceeds center distance", irf));
            prf->angsub = TODEGS*asinf(    prf->rfsra     /prf->rfdis);
            prf->angclr = TODEGS*asinf((prf->rfsra + hwid)/prf->rfdis);
            }
         else
            prf->angsub = prf->angclr = 0.0;
         break;
      case RFS_TRAPEZ:
         d1 = prf->rfdis - prf->rfsra;
         if (d1 <= 0.0)
            abexitm(156, ssprintf(NULL, "Range finder %d: Axial size "
               "exceeds outer radius", irf));
         prf->mxd = sqrtf(prf->rfdis*prf->rfdis +
            0.25*prf->rfsrl*prf->rfsrl);
         d2 = 0.5*prf->rfsrl*d1/prf->rfdis;
         dnct = sqrtf(d1*d1 + d2*d2);
         prf->angsub = TODEGS * asinf(d2/dnct);
         prf->mysize = (int)(prf->rfsra*prf->rfsrl*
            (1.0 - 0.5*prf->rfsra/prf->rfdis));
         if (hwid > 0.0) {
            if (prf->rfdis <= hwid)
               abexitm(156, ssprintf(NULL, "Range finder %d: Outer "
                  "radius < object halfwidth", irf));
            if (hwid > dnct)
               abexitm(156, ssprintf(NULL, "Range finder %d: Too "
                  "close to platform",irf));
            prf->angclr = prf->angsub + TODEGS * asinf(hwid/dnct);
            }
         break;
      case RFS_RECTGL:
         /* Whether or not hwid == 0, the rectangle is allowed to
         *  include the center of the object, in which case angsub
         *  is meaningless and set to 0.0.  */
         if (prf->rfsra <= 0.0 || prf->rfsrl <= 0.0)
            abexitm(156, ssprintf(NULL, "Range finder %d: Rectangle "
               "has side <= 0", irf));
         d1 = prf->rfdis + 0.5*prf->rfsra;
         d2 = 0.5*prf->rfsrl;
         prf->mxd = sqrtf(d1*d1 + d2*d2);
         d1 = prf->rfdis - 0.5*prf->rfsra;
         dnct = sqrtf(d1*d1 + d2*d2);
         prf->angsub = (d1 > 0.0) ? TODEGS * asinf(d2/dnct) : 0.0;
         prf->mysize = (int)(prf->rfsra*prf->rfsrl);
         if (hwid > 0.0) {
            if (hwid > dnct)
               abexitm(156, ssprintf(NULL, "Range finder %d: Too "
                  "close to platform",irf));
            prf->angclr = prf->angsub + TODEGS * asinf(hwid/dnct);
            }
         break;
      default:
         abexitm(156, "Unsupported range finder geometry");
         } /* End jrfshp switch */
      if (prf->rfang == 0.0 || prf->rfang == HCIRC)
         prf->half = 1.0, hnas += 1;
      else
         prf->half = -1.0, hnas += 2;
      *ppb++ = prf;
      } /* End range finder loop */
   prs->hnas = hnas;

/* Finish up the angle-ordered pointer list */

   for (--prf; prf>=prng; --prf)
      if (prf->half < 0.0) *ppb++ = prf;

#ifdef DEBUG                  /*** DEBUG ***/
   pdbgf = fopen("DebugData", "w");
#endif

   return prs;
   } /* End rngrinit() */


/***********************************************************************
*                               rngrget                                *
*             Check a list of range finders for contacts               *
*----------------------------------------------------------------------*
*                                                                      *
*  Synopsis:                                                           *
*     int rngrget(RngrSet *prs, HitList *hitlist, OppPos *opprxy,      *
*        double o, double x, double y, double mxdist, int ibmo,        *
*        int kopt, int nopp)                                           *
*                                                                      *
*  Prototyped in:                                                      *
*     bbdcxtra.h                                                       *
*                                                                      *
*  Arguments:                                                          *
*     prs      Ptr to a RngrSet structure initialized by a previous    *
*              call to rngrinit().                                     *
*     hitlist  Ptr to an array of HitList structs where numbers of     *
*              hits (nhits) and distance to closest hit (hitdist) for  *
*              each sensor will be stored in order clockwise starting  *
*              with first sensor (code assumes sensors are defined in  *
*              order of increasing angle).  Additionally, if hwid > 0  *
*              in the rngrinit call, the angle the object must rotate  *
*              through to clear the obstacle will be stored (angclr).  *
*              This array must have one entry for each sensor at 0 or  *
*              180 degrees and two for every sensor at some other      *
*              angle.                                                  *
*     opprxy   Ptr to an array of nopp OppPos structs giving the       *
*              positions and sizes of nopp opponents whose collisions  *
*              with the range finders are to be recorded.              *
*     o        Orientation of the platform at this time step.  This    *
*              is measured in degrees clockwise from vertical as       *
*              defined in the bitmaps to the direction of the          *
*              positive platform axis as defined by a value of zero    *
*              in the Ranger structure.                                *
*     x        X-axis position of the platform center in the bitmap.   *
*     y        Y-axis position of the platform center in the bitmap.   *
*              Sign of y axis is determined by YAX_HAND in bbdcxtra.h. *
*     mxdist   Maximum distance at which hits shall be reported, or    *
*              0.0 to omit this test.  This parameter may be used      *
*              effectively to reduce the range of a set of sensors     *
*              in order that the platform may find a path that         *
*              "squeezes" past a nearby obstacle.                      *
*     ibmo     Index into prestored pbmo array used to map objects     *
*              onto search and draw bitmaps.                           *
*     kopt     OR of desired option flags from the following list:     *
*              RNGROPT_NORM (0) Placeholder for no options selected.   *
*              RNGROPT_SPEC_FWD (1) Calculate only forward sensors.    *
*              RNGROPT_SPEC_BCK (2) Calculate only aft sensors.        *
*              RNGROPT_DRAW (4) Draw the sensors on the bitmap.        *
*              RNGROPT_NODA (8) Do not return distances & angles.      *
*              RNGROPT_DHITS (16) Draw sensors with hits only.         *
*     nopp     Size of the opprxy array.  If zero, opprxy may be NULL, *
*              no test for collisions with opponents is performed.     *
*                                                                      *
*  Return value:                                                       *
*     0  if no sensors contacted any objects in this time step.        *
*     n  if n sensors contacted objects.  The identities of these      *
*        sensors, the number of pixels that hit objects, and their     *
*        distances may be found by checking the values stored in       *
*        the 'hitlist' arg.                                            *
*                                                                      *
*  Prerequisites:                                                      *
*     rngrinit() must be called first to set up the RngrSet array and  *
*     check that all the range finder parameters are allowable.        *
*                                                                      *
*  Notes:                                                              *
*     The hitdist and angclr returned by this routine are calculated   *
*     assuming that the range finder can return the distance to an     *
*     obstacle, but it does not know exactly where in azimuth within   *
*     the receptive field of the sensor that obstacle is located.      *
*                                                                      *
*     If a mxdist test is applied, the sensors (except circles) will   *
*     be drawn smaller to reflect the reduced range.  It would be      *
*     possible, but more difficult, to use a different color to        *
*     indicate the unused portion of the sensor receptive field, and   *
*     yet another color to indicate sensors with RFC_NONE detection.   *
***********************************************************************/

int rngrget(RngrSet *prs, HitList *hitlist, OppPos *opprxy,
      double o, double x, double y, double mxdist, int ibmo,
      int kopt, int nopp) {

   struct HitInfo HI;         /* Info for rngrohit */
   BITMAP  *psBM = prs->psBM; /* Ptr to search bitmap */
   BITMAP  *pdBM = prs->pdBM; /* Ptr to draw bitmap */
   BMOff   *pbmo;             /* Ptr to appropriate BMOff */
   Ranger  *prf,*prfe;        /* Range finder loop controls */
   HitList *phlp,*phlm;       /* Ptrs to pos,neg hit lists */
   HitList *phlu;             /* Ptr to hit list used */
   double dx,dy,dd;           /* Temps for distance calc */
   double hlc;                /* Half-circle loop control */
   double mxdi;               /* mxdist of i'th sensor */
   double ra;                 /* Sensor angle + o (radians) */
   double sa,ca;              /* sin(ra), cos(ra) */
   double dissa,disca;        /* rfdis*sa, 0.0 YOM rfdis*ca */
   double xbm,ybm;            /* Sensor coords offset to bitmap */
   double xcr,ycr;            /* Coords of center of circle or edge
                              *  of trapezoid or rect. adjusted for
                              *  rounding to nearest pixel */
   int myhdt;                 /* My hits draw test */
   int myhits;                /* Hits on current sensor */
   int notnoda = !(kopt & RNGROPT_NODA);
   int nswh = 0;              /* Number of sensors with hits */
   int scol;                  /* Sensor color */
   int scolnoh;               /* Sensor color with no hits */
   enum whyrej gtrc;          /* Gray test return code */

#ifdef DEBUG                  /*** DEBUG ***/
   int didhdr=0;
#endif

   if (ibmo >= prs->nbmo)
      abexitm(157, "Bad range finder unit index");
   pbmo = prs->pbmo + ibmo;
   prfe = prs->prng + prs->nrng;
   phlp = hitlist, phlm = hitlist + prs->hnas;
   scolnoh = makecol(120,0,240);
   HI.kopt = kopt;
   HI.nopp = nopp;

/* Loop over sensor array.  Quit if in RNGROPT_SPEC_FWD mode and
*  have moved beyond sensors in forward motion pathway.  Omit but
*  continue if in RNGROPT_SPEC_BCK mode and this is a forward
*  sensor.  If both bits are set, nothing will be tested.  */

   for (prf=prs->prng; prf<prfe; ++prf) {

      if (kopt & RNGROPT_SPEC_FWD &&
         prf->rfang - prf->angsub > prs->MTC) break;
      if (kopt & RNGROPT_SPEC_BCK &&
         HCIRC - prf->rfang + prf->angsub > prs->MTC) continue;

      mxdi = prf->mxd;
      if (mxdist > 0.0 && mxdist < mxdi) mxdi = mxdist;
      mxdi += EPSDIST;        /* So can accept d = mxdi */
      HI.mxdi2 = mxdi*mxdi;

/* Loop over positive and negative half-circles */

      for (hlc=prf->half; hlc<2.0; hlc+=2.0) {

#ifdef DEBUG                  /*** DEBUG ***/
         didhdr = 0;
#endif
         myhits = 0;
         myhdt = (kopt & RNGROPT_DRAW) ? -1 :
            ((kopt & RNGROPT_DHITS) ? prf->minhits : INT_MAX);
         HI.cd2 = HI.mxdi2;
         ra = TORADS * (o + hlc*prf->rfang);
         sa = sin(ra), ca = cos(ra);
         if (hlc < 0.0) phlu = phlm--;
         else           phlu = phlp++;

         switch(prf->jrfshp) {

/* Handle a circular sensor field.  In this case, we do not have
*  geometry calculations for a circle truncated at distance mxdist,
*  so the full circle is checked and then dist > mxdist ignored.  */

         case RFS_CIRCLE: {
            dissa = sa*prf->rfdis, disca = 0.0 YOM ca*prf->rfdis;
            if (prf->jrfcid) {
               IterSph ISph;              /* Sphere iterator */
               xbm = x + pbmo->xso, ybm = y + pbmo->yso;
               xcr = xbm + dissa + 0.5, ycr = ybm + disca + 0.5;
               InitSphereIndexing(&ISph, 1.0, 1.0, 1.0, xcr, ycr,
                  0.0, prf->rfsra, psBM->w, psBM->h, 1);

               while (GetNextPointInSphere(&ISph)) {
                  int pix;                /* Value of a pixel */
                  dx = (double)ISph.ix - xbm;
                  dy = (double)ISph.iy - ybm;
                  dd = dx*dx + dy*dy;
                  if (dd > HI.mxdi2) continue;

                  pix = getpixel(psBM, ISph.ix, ISph.iy);

                  /**** Here would go a switch on jrfcid
                  *     if we had more than color test code  ****/
                  gtrc = (enum whyrej)rngrgray(prs, pix);

                  /* If this is a hit, count it & store min dist */
                  if (gtrc == grayok) {
                     myhits += 1;
                     if (notnoda && dd < HI.cd2)
                        HI.cdx = dx, HI.cdy = dy, HI.cd2 = dd;
                     }

#ifdef DEBUG   /*** DEBUG ***/
                  wrejct[gtrc] += 1;
                  if (gtrc > grayok) {
                     if (!didhdr) {
                        fprintf(pdbgf, "Entering rngrget at o = %.2f, "
                           "x = %.2f, y = %.2f\n", o, x, y);
                        fprintf(pdbgf, "Sensor at %.2f, type %d\n",
                           hlc*prf->rfang, prf->jrfshp);
                        didhdr = YES;
                        }
                     fprintf(pdbgf, "Pixel at %d,%d, r,g,b %d,%d,%d, "
                        "hits %d\n", ISph.ix, ISph.iy, getr24(pix),
                        getg24(pix), getb24(pix), myhits);
                     }
#endif
                  } /* End loop over pixels in sphere */
               } /* End if jrfcid */

            /* Check for contact with opponent */
            if (nopp > 0) {
               HI.xs[0] = dissa, HI.ys[0] = disca;
               HI.crad = prf->rfsra;
               if (rngrohit(opprxy, &HI, prf->jrfshp))
                  myhits |= OPP_HIT_COUNT;
               }

            /* If any hits, and hwid given, calc angclr.  In the
            *  circle case, there may be obstacle points farther
            *  away than dclose that also have greater angsub, so
            *  to be conservative, just use the stored angclr.
            *  But if rfdis is 0, calc instead angle to closest
            *  obstacle position.  */
            if (notnoda && myhits > 0) {
               phlu->hitdist = (float)sqrt(HI.cd2);
               if (prf->rfdis > 0.0)
                  phlu->angclr = prf->angclr;
               else {
                  float tra = TORADS*o;
                  float fdx = (float)HI.cdx, fdy = (float)HI.cdy;
                  float tca = cosf(tra), tsa = sinf(tra);
                  phlu->angclr = TODEGS*atan2f(
                     fdx*tsa YOP fdy*tca, fdx*tca YOM fdy*tsa);
                  }
               }
            else
               phlu->hitdist = phlu->angclr = 0.0;

            if (myhits >= myhdt) {     /* Plot the sensor */
               if (myhits > 0) {
                  scol = 180 + 60*myhits/prf->mysize;
                  scol = min(scol, 240);
                  scol = makecol(scol, 0, 0); }
               else
                  scol = scolnoh;
               xcr = x + pbmo->xdo + dissa + 0.5;
               ycr = y + pbmo->ydo + disca + 0.5;
               circlefill(pdBM, (int)xcr, (int)ycr, prf->rfsra, scol);
               }

            } /* End circle local scope */
            break;

/* Handle a trapezoidal sensor field */

         case RFS_TRAPEZ: {
            double xx,yy;        /* Coords of a pixel in the box */
            float  edi,edo;      /* Radii of inner,outer edges */
            float  hle;          /* Half-lateral distance */
            float  rad;          /* Row radius */
            float  tanha;        /* tan(half opening angle) */

            /* Check four edges of trapezoid against mxdi.  (Can
            *  ignore angle, mxdi defines a circle around origin.
            *  Note that it is not correct to check corners--
            *  there can be points inside mxdi circle but outside
            *  line drawn between corner intersections with mxdi.) */
            hle  = 0.5*prf->rfsrl;
            edo = prf->rfdis;
            tanha = hle/edo;
            edi = edo - prf->rfsra;
            if (edo > mxdi) edo = mxdi;
            /* Max dist less than inner dist, skip this sensor */
            if (edo < edi) break;
            dissa = sa*edo, disca = 0.0 YOM ca*edo;

            if (prf->jrfcid) {
               xbm = x + pbmo->xso, ybm = y + pbmo->yso;
               xcr = xbm + dissa + 0.5, ycr = ybm + disca + 0.5;
               for (rad=edo; rad>=edi;
                     rad-=1.0,xcr-=sa,ycr=ycr YOP ca) {
                  hle = floor(tanha*rad);
                  xx = xcr - hle*ca, yy = ycr YOM hle*sa;
                  for ( ; hle>=0.0; hle-=0.5,xx+=ca,yy = yy YOP sa) {
                     int pix;             /* Value of a pixel */
                     /* Must check distance now for those corner
                     *  points missed by loop boundaries above  */
                     dx = xx - xbm, dy = yy - ybm;
                     dd = dx*dx + dy*dy;
                     if (dd > HI.mxdi2) continue;

                     pix = getpixel(psBM, (int)xx, (int)yy);

                     /**** Here would go a switch on jrfcid
                     *     if we had more than color test code  ****/
                     gtrc = (enum whyrej)rngrgray(prs, pix);

                     /* If this is a hit, count it & store min dist */
                     if (gtrc == grayok) {
                        myhits += 1;
                        if (notnoda && dd < HI.cd2)
                           HI.cdx = dx, HI.cdx = dy, HI.cd2 = dd;
                        }

#ifdef DEBUG   /*** DEBUG ***/
                     wrejct[gtrc] += 1;
                     if (gtrc > grayok) {
                        if (!didhdr) {
                           fprintf(pdbgf, "Entering rngrget at o = "
                              "%.2f, x = %.2f, y = %.2f\n", o, x, y);
                           fprintf(pdbgf, "Sensor at %.2f, type %d\n",
                              hlc*prf->rfang, prf->jrfshp);
                           didhdr = YES;
                           }
                        fprintf(pdbgf, "Pixel at %d,%d, r,g,b %d,%d,%d "
                           "hits %d\n", (int)xx, (int)yy, getr24(pix),
                           getg24(pix), getb24(pix), myhits);
                        }
#endif
                     } /* End lateral loop */
                  } /* End axial loop */
               } /* End if jrfcid */

            /* Check for contact with the opponent */
            if (nopp > 0) {
               /* Locate corners of sensor (OppPos coords) */
               xcr = dissa, ycr = disca;
               hle = tanha*edo;
               xx = hle*ca, yy = hle*sa;
               HI.xs[0] = xcr  -  xx;
               HI.ys[0] = ycr YOM yy;
               HI.xs[1] = xcr  +  xx;
               HI.ys[1] = ycr YOP yy;
               xcr = sa*edi, ycr = 0.0 YOM ca*edi;
               hle = tanha*edi;
               xx = hle*ca, yy = hle*sa;
               HI.xs[2] = xcr  +  xx;
               HI.ys[2] = ycr YOP yy;
               HI.xs[3] = xcr  -  xx;
               HI.ys[3] = ycr YOM yy;
               if (rngrohit(opprxy, &HI, prf->jrfshp))
                  myhits |= OPP_HIT_COUNT;
               }

            /* If any hits, and hwid given, calc angclr */
            if (notnoda && myhits > 0) {
               phlu->hitdist = (float)sqrt(HI.cd2);
               phlu->angclr = prs->hwid > 0.0 ? prf->angsub +
                  TODEGS * atan2f(prs->hwid,phlu->hitdist) : 0.0;
               }
            else
               phlu->hitdist = phlu->angclr = 0.0;

            if (myhits >= myhdt) {     /* Plot the sensor */
               int corners[nE2*nRC];
               /* Corner location for plotting (Bitmap coords) */
               hle = tanha*edo;
               xbm = x + pbmo->xdo, ybm = y + pbmo->ydo;
               xcr = xbm + dissa + 0.5, ycr = ybm + disca + 0.5;
               xx = hle*ca, yy = hle*sa;
               corners[0] = (int)(xcr  -  xx);
               corners[1] = (int)(ycr YOM yy);
               corners[2] = (int)(xcr  +  xx);
               corners[3] = (int)(ycr YOP yy);
               xcr = xbm + sa*edi + 0.5, ycr = ybm YOM ca*edi + 0.5;
               hle = tanha*edi;
               xx = hle*ca, yy = hle*sa;
               corners[4] = (int)(xcr  +  xx);
               corners[5] = (int)(ycr YOP yy);
               corners[6] = (int)(xcr  -  xx);
               corners[7] = (int)(ycr YOM yy);
               if (myhits > 0) {
                  scol = 180 + 60*myhits/prf->mysize;
                  scol = min(scol, 240);
                  scol = makecol(scol, 0, 0); }
               else
                  scol = scolnoh;
               polygon(pdBM, nRC, corners, scol);
               }

            } /* End trapezoid local scope */
            break;

/* Handle a rectangular sensor field centered on given coords */

         case RFS_RECTGL: {
            double hleca,hlesa;  /* hle*ca, hle*sa */
            double xx,yy;        /* Coords of a pixel in the box */
            float  edi,edo;      /* Radii of inner,outer edges */
            float  hae,hle;      /* Half-axial, half-lateral edge */

            /* Check four edges of rectangle against mxdi.  (Can
            *  ignore angle, mxdi defines a circle around origin.
            *  Note that it is not correct to check corners--
            *  there can be points inside mxdi circle but outside
            *  line drawn between corner intersections with mxdi.) */
            hle = 0.5*prf->rfsrl; if (hle > mxdi) hle = mxdi;
            hae = 0.5*prf->rfsra;
            edo = prf->rfdis + hae; if (edo >  mxdi) edo =  mxdi;
            edi = prf->rfdis - hae; if (edi < -mxdi) edi = -mxdi;
            /* Outer edge inside inner edge, skip this sensor */
            if (edo < edi) break;
            dissa = sa*edo, disca = 0.0 YOM ca*edo;
            hleca = hle*ca, hlesa = hle*sa;

            if (prf->jrfcid) {
               int irow, nrow = (int)(edo - edi + 1.0);
               int icol, ncol = (int)(2.0*hle + 1.0);
               xbm = x + pbmo->xso, ybm = y + pbmo->yso;
               xcr = xbm + dissa + 0.5, ycr = ybm + disca + 0.5;
               for (irow=0; irow<nrow; ++irow,xcr-=sa,ycr=ycr YOP ca) {
                  xx = xcr - hleca, yy = ycr YOM hlesa;
                  for (icol=0; icol<ncol; ++icol,xx+=ca,yy=yy YOP sa) {
                     int pix;             /* Value of a pixel */
                     /* Must check distance now for those corner
                     *  points missed by loop boundaries above  */
                     dx = xx - xbm, dy = yy - ybm;
                     dd = dx*dx + dy*dy;
                     if (dd > HI.mxdi2) continue;

                     pix = getpixel(psBM, (int)xx, (int)yy);

                     /**** Here would go a switch on jrfcid
                     *     if we had more than color test code  ****/
                     gtrc = (enum whyrej)rngrgray(prs, pix);

                     /* If this is a hit, count it & store min dist */
                     if (gtrc == grayok) {
                        myhits += 1;
                        if (notnoda && dd < HI.cd2)
                           HI.cdx = dx, HI.cdy = dy, HI.cd2 = dd;
                        }

#ifdef DEBUG   /*** DEBUG ***/
                     wrejct[gtrc] += 1;
                     if (gtrc > grayok) {
                        if (!didhdr) {
                           fprintf(pdbgf, "Entering rngrget at o = "
                              "%.2f, x = %.2f, y = %.2f\n", o, x, y);
                           fprintf(pdbgf, "Sensor at %.2f, type %d\n",
                              hlc*prf->rfang, prf->jrfshp);
                           didhdr = YES;
                           }
                        fprintf(pdbgf, "Pixel at %d,%d, r,g,b %d,%d,%d "
                           "hits %d\n", (int)xx, (int)yy, getr24(pix),
                           getg24(pix), getb24(pix), myhits);
                        }
   #endif
                     } /* End lateral loop */
                  } /* End axial loop */
               } /* End if jrfcid */

            /* Check for contact with opponent */
            if (nopp > 0) {
               xcr = dissa, ycr = disca;
               HI.xs[0] = xcr  -  hleca;
               HI.ys[0] = ycr YOM hlesa;
               HI.xs[1] = xcr  +  hleca;
               HI.ys[1] = ycr YOP hlesa;
               xcr = sa*edi, ycr = 0.0 YOM ca*edi;
               HI.xs[2] = xcr  +  hleca;
               HI.ys[2] = ycr YOP hlesa;
               HI.xs[3] = xcr  -  hleca;
               HI.ys[3] = ycr YOM hlesa;
               if (rngrohit(opprxy, &HI, prf->jrfshp))
                  myhits |= OPP_HIT_COUNT;
               }

            /* If any hits, and rfdis given, calc angclr.  But if
            *  rfdis is 0, calc instead angle to closest obstacle
            *  position.  */
            if (notnoda && myhits > 0) {
               phlu->hitdist = (float)sqrt(HI.cd2);
               if (prf->rfdis > 0.0) {
                  phlu->angclr = TODEGS*
                     (asinf(prf->rfsrl/phlu->hitdist) +
                     atan2f(prs->hwid, phlu->hitdist));
                  }
               else {
                  float tra = TORADS*o;
                  float fdx = (float)HI.cdx, fdy = (float)HI.cdy;
                  float tca = cosf(tra), tsa = sinf(tra);
                  phlu->angclr = TODEGS*atan2f(
                     fdx*tsa YOP fdy*tca, fdx*tca YOM fdy*tca);
                  }
               }
            else
               phlu->hitdist = phlu->angclr = 0.0;

            if (myhits >= myhdt) {     /* Plot the sensor */
               int corners[nE2*nRC];
               xbm = x + pbmo->xdo, ybm = y + pbmo->ydo;
               xcr = xbm + dissa + 0.5, ycr = ybm + disca + 0.5;
               corners[0] = (int)(xcr  -  hleca);
               corners[1] = (int)(ycr YOM hlesa);
               corners[2] = (int)(xcr  +  hleca);
               corners[3] = (int)(ycr YOP hlesa);
               xcr = xbm + sa*edi + 0.5, ycr = ybm YOM ca*edi + 0.5;
               corners[4] = (int)(xcr  +  hleca);
               corners[5] = (int)(ycr YOP hlesa);
               corners[6] = (int)(xcr  -  hleca);
               corners[7] = (int)(ycr YOM hlesa);
               if (myhits > 0) {
                  scol = 180 + 60*myhits/prf->mysize;
                  scol = min(scol, 240);
                  scol = makecol(scol, 0, 0); }
               else
                  scol = scolnoh;
               polygon(pdBM, nRC, corners, scol);
               }

            } /* End rectangle local scope */
            break;

            } /* End shape switch */

         phlu->nhits = myhits;
         nswh += (myhits >= prf->minhits);
         } /* End half-circle loop */
      } /* End sensor array loop */

#ifdef DEBUG   /*** DEBUG ***/
   if (didhdr) fprintf(pdbgf, "Returning nswh %d\n", nswh);
#endif

   return nswh;
   } /* End rngrget() */


/***********************************************************************
*                              rngrkill                                *
*          Release storage allocated for range finder array            *
*----------------------------------------------------------------------*
*                                                                      *
*  Synopsis:                                                           *
*     void rngrkill(RngrSet *prs)                                      *
*                                                                      *
*  Prototyped in:                                                      *
*     bbdcxtra.h                                                       *
*                                                                      *
*  Argument:                                                           *
*     prs      Pointer to RngrSet array returned by a previous call    *
*              to rngrinit().                                          *
*                                                                      *
*  Note:  After this call returns, the prs array no longer exists      *
*  and cannot be used in further rngrget() calls.  However, the        *
*  Ranger array is assumed to be static and is not freed.              *
***********************************************************************/

void rngrkill(RngrSet *prs) {

#ifdef DEBUG   /*** DEBUG ***/
   if (pdbgf) {
      fprintf(pdbgf,
         "\nGray test: OK %ld, Min %ld, Max %ld, Radius %ld\n",
         wrejct[grayok], wrejct[ming], wrejct[maxg], wrejct[maxrad]);
      fclose(pdbgf);
      }
#endif
   if (prs) {
      freev(prs->pprh, "Ranger Set");
      freev(prs, "Ranger Set");
      }

   } /* End rngrkill() */
