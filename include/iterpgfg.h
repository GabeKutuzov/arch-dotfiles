/* (c) Copyright 2011, The Rockefeller University *11114* */
/* $Id: iterpgfg.h 59 2017-01-13 20:34:16Z  $ */
/***********************************************************************
*                             iterpgfg.h                               *
*                                                                      *
*  This header file declares a pair of routines that may be used to    *
*  find successively all the points on some rectangular lattice that   *
*  fall either within or outside a figure defined by one or more       *
*  possibly concave, possibly overlapping, polygons or polygons with   *
*  holes.                                                              *
*                                                                      *
*  This version works with coordinates of type float.  Another version *
*  will need to be written if double precision coordinates are needed. *
*                                                                      *
*  The user must provide an instance of the IterPgfg data structure    *
*  declared here for each invocation of the routines.                  *
*                                                                      *
*  V1A, 12/12/11, GNR - Initial version                                *
*  ==>, 12/19/11, GNR - Last date before committing to svn repository  *
***********************************************************************/

#ifndef ITERPGFG_HDR_INCLUDED
#define ITERPGFG_HDR_INCLUDED

#include "sysdef.h"

/* Macro to mark y coord of last point in a polygon */
#define PGFG_LAST_Y_SHFT  2
#define PgfgPly(y,nsy) ((y) - ((nsy) << PGFG_LAST_Y_SHFT))

typedef struct IPgfgWk_t { /* Information stored in work area */
   struct IPgfgWk_t *pne;     /* Ptr to info for next edge */
   si32 sly,shy,slx,shx;      /* Low,high x,y coords */
   si32 rix;                  /* Row-intercept x */
   si32 slope;                /* 1/slope of this edge */
   } IPgfgWk;

/* Structure used by polygon indexing routine to save its state and
*  to return results.  All coordinates are integer grid coordinates
*  starting from 0 in the ULHC.  */
typedef struct IterPgfg_t {
   /* Variables used to return results to caller */
   long  ioff;             /* Returns offset to next grid point */
   long  ix,iy;            /* Returns coords of current grid point */
   /* Internal variables, not for user access */
   IPgfgWk *wkel;          /* Working edge list */
   IPgfgWk *wkal;          /* Working active edge list */
   IPgfgWk *wkil;          /* Ptr to inactive edge list */
   IPgfgWk **ppil;         /* Ptr to ptr to last wkil entry */
   long  yoff;             /* Offset in lattice of current y row */
   si32  nsx,nsy;          /* Size of full lattice along x,y */
   si32  smnx,smxx;        /* Scaled min x (-nsx) and max x (nsx) */
   si32  topy;             /* Largest y in whole figure + 1 */
   si32  um1;              /* Scaled unity - 1 = fraction mask */
   int   bs;               /* Binary scale */
   ui16  par,par0;         /* Current, initial parity */
   } IterPgfg;

#ifdef __cplusplus
extern "C" {
#endif
/* Call to initialize polygon figure indexing */
void InitPgfgIndexing(IterPgfg *pit, xyf *pgon, IPgfgWk *work,
   si32 nsx, si32 nsy, int nvtx);

/* Call to obtain next point in polygon figure */
int GetNextPointInPgfg(IterPgfg *pit);
#ifdef __cplusplus
}
#endif

#endif /* ifndef ITERPGFG_HDR_INCLUDED */
