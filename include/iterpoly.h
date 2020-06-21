/* (c) Copyright 2011-2014, The Rockefeller University *11114* */
/* $Id: iterpoly.h 59 2017-01-13 20:34:16Z  $ */
/***********************************************************************
*                             iterpoly.h                               *
*                                                                      *
*  This header file declares a pair of routines that may be used to    *
*  find successively all the points on some rectangular lattice that   *
*  fall within a given (mostly-convex) polygon.                        *
*                                                                      *
*  This version works with coordinates of type float.  Another version *
*  will need to be written if double precision coordinates or concave  *
*  polygons are needed.                                                *
*                                                                      *
*  The user must provide an instance of the IterPoly data structure    *
*  declared here for each invocation of the routines.                  *
*                                                                      *
*  V1A, 08/29/11, GNR - Initial version                                *
*  ==>, 08/31/11, GNR - Last date before committing to svn repository  *
*  Rev, 11/18/14, GNR - Add NO_CRKLIB definition                       *
***********************************************************************/

#ifndef ITERPOLY_HDR_INCLUDED
#define ITERPOLY_HDR_INCLUDED

#include "sysdef.h"

#ifdef NO_CRKLIB
/* ret_t must be a symbol, not a typedef, because it occurs in
*  multiple iterxxxx headers.  */
#define ret_t int
/* Define error return codes when abexit() not available */
#define IPI_NORMAL      0  /* Normal return */
#define IPI_BADGRID     1  /* Lattice edge <= 0 */
#define IPI_LT3VERT     2  /* Polygon has fewer than 3 vertices */
#else
#define ret_t void
#endif

#define  NLENDS    2       /* Number of ends on a line segment */

typedef struct IPolyWk_t { /* Information stored in work area */
   float slope;               /* 1/slope of an edge */
   long  loy,hiy;             /* Low y, high y per edge */
   } IPolyWk;

/* Structure used by polygon indexing routine to save its state and
*  to return results.  All coordinates are integer grid coordinates
*  starting from 0 in the ULHC.  */
typedef struct IterPoly_t {
   /* Variables used to return results to caller */
   long  ioff;             /* Returns offset to next grid point */
   long  ix,iy;            /* Returns coords of current grid point */
   /* Internal variables, not for user access */
   xyf   *pgon;            /* Vertices of polygon */
   IPolyWk *wk;            /* Caller-supplied work area */
   float topx;             /* Largest legal x coord */
   long  nsx,nsy;          /* Size of full lattice along x,y */
   long  xmx;              /* Highest x at current y */
   long  ymn,ymx;          /* Starting,ending y coords */
   long  yoff;             /* Offset in lattice of current y row */
   int   nvtx;             /* Number of vertices */
   } IterPoly;

#ifdef __cplusplus
extern "C" {
#endif
/* Call to initialize polygon indexing */
ret_t InitPolygonIndexing(IterPoly *Poly, xyf *pgon, IPolyWk *work,
   long nsx, long nsy, int nvtx);

/* Call to obtain next point in polygon */
int GetNextPointInPolygon(IterPoly *Poly);
#ifdef __cplusplus
}
#endif

#endif /* ifndef ITERPOLY_HDR_INCLUDED */
