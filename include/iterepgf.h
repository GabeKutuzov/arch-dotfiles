/* (c) Copyright 2012, The Rockefeller University *11114* */
/* $Id: iterepgf.h 59 2017-01-13 20:34:16Z  $ */
/***********************************************************************
*                             iterepgf.h                               *
*                                                                      *
*  This header file declares a pair of routines that may be used to    *
*  find successively all the points on some rectangular lattice that   *
*  fall either within or outside a figure defined by one or more       *
*  possibly concave, possibly overlapping, polygons or polygons with   *
*  holes and a border of fixed width around the figure boundary.       *
*                                                                      *
*  This version works with coordinates of type float.  Another version *
*  will need to be written if double precision coordinates are needed. *
*                                                                      *
*  V1A, 11/14/12, GNR - Initial version, based on iterpgfg.h           *
*  ==>, 11/14/12, GNR - Last date before committing to svn repository  *
***********************************************************************/

#ifndef ITEREPGF_HDR_INCLUDED
#define ITEREPGF_HDR_INCLUDED

#include "sysdef.h"

/* Macro to mark y coord of last point in a polygon */
#define EPGF_LAST_Y_SHFT  2
#define EpgfPly(y,nsy) ((y) - ((nsy) << EPGF_LAST_Y_SHFT))

/* Definitions of mode bits */
#define EPGF_INT  1        /* Paint inside of figure */
#define EPGF_EXT  2        /* Paint outside of figure */
#define EPGF_ALL  3        /* Start with full lattice */
#define EPGF_ADDB 0        /* Add border (default) */
#define EPGF_RMB  4        /* Remove border */

typedef struct IEpgfWk_t { /* Information stored in work area */
   struct IEpgfWk_t *pne;     /* Ptr to info for next edge */
   si32 sly,shy,slx,shx;      /* Low,high x,y coords */
   si32 rix;                  /* Row-intercept x */
   si32 slope;                /* 1/slope of this edge */
   } IEpgfWk;

/* Structure used by polygon indexing routine to save its state and
*  to return results.  All coordinates are integer grid coordinates
*  starting from 0 in the ULHC.  */
typedef struct IterEpgf_t {
   /* Variables used to return results to caller */
   long  ioff;             /* Returns offset to next grid point */
   long  ix,iy;            /* Returns coords of current grid point */
   /* Internal variables, not for user access */
   byte *pwim,*pim;        /* Ptrs to working image area */
   long  yoff;             /* Offset in lattice of current y row */
   long  topy;             /* Largest y in whole figure + 1 */
   si32  nsx,nsy;          /* Size of full lattice along x,y */
   si32  ltir;             /* Length of temp imge row (bytes) */
   } IterEpgf;

#ifdef __cplusplus
extern "C" {
#endif
/* Call to initialize polygon figure indexing */
void InitEpgfIndexing(IterEpgf *pit, xyf *pgon, IEpgfWk *work,
   byte *pwim, float border, si32 nsx, si32 nsy, int nvtx, int mode);

/* Call to obtain next point in polygon figure */
int GetNextPointInEpgf(IterEpgf *pit);
#ifdef __cplusplus
}
#endif

#endif /* ifndef ITEREPGF_HDR_INCLUDED */
