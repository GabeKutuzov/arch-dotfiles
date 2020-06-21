/* (c) Copyright 2014, The Rockefeller University *11114* */
/* $Id: itersect.h 59 2017-01-13 20:34:16Z  $ */
/***********************************************************************
*                             itersect.h                               *
*                                                                      *
*  This header file declares a pair of routines that may be used to    *
*  find successively all the points on some square lattice that fall   *
*  within a given sector of a given circle.  The points that define    *
*  the sector need not fall on grid points.                            *
*                                                                      *
*  This version works with coordinates of type float.  Another version *
*  will need to be written if double precision coordinates are needed. *
*                                                                      *
*  The user must provide an instance of the IterSect data structure    *
*  declared here for each invocation of the routines.                  *
*                                                                      *
*  Note that this program can be configured to use or not use the crk  *
*  library routines (not use if NO_CRKLIB defined).  This is useful    *
*  when itersect is to be used in a MATLAB mex file.                   *
*                                                                      *
*  V1A, 11/15/14, GNR - Initial version, based on itercirc             *
*  ==>, 11/21/14, GNR - Last date before committing to svn repository  *
***********************************************************************/

#ifndef ITERSECT_HDR_INCLUDED
#define ITERSECT_HDR_INCLUDED

#include "sysdef.h"

/* Define NO_CRKLIB at compile time to compile this package outside
*  the crk library with no abexitm() and no rkarith definitions.  */
#ifdef NO_CRKLIB
/* ret_t must be a symbol, not a typedef, because it occurs in
*  multiple iterxxxx headers.  */
#define ret_t int
#define dm64nq(m1,m2,d) ((si32)((si64)(m1)*(si64)(m2)/(si64)(d)))
/* Return codes that may be produced by InitSectorIndexing() */
#define ISI_NORM     0     /* Normal return */
#define ISI_BADGRID  1     /* nx or ny was absurdly small (< 5) */
#define ISI_BADRAD1  2     /* Radius of sector point 1 very small */
#define ISI_BADRAD2  3     /* Radius of sector point 2 very small */
#define ISI_OUTSIDE  4     /* Circle is entirely outside grid */
#else
#include "rkarith.h"
#define ret_t void
#endif

/* Structure used by sector indexing routine to save state and
*  return results  */
typedef struct IterSect {
   /* Variables used to return results to caller */
   long ioff;              /* Returns offset to next grid point */
   long ix,iy;             /* Returns current grid point x,y */
   /* Internal variables, not for user access */
   long iofrow;            /* Offset to current row */
   double radsq;           /* Radius of circle squared (*S32) */
   long mxy;               /* Max y overall */
   si32 xc,yc;             /* Center of circle (S16) */
   si32 x1,y1;             /* Coords of point 1 rel to ctr. (S16) */
   si32 x2,y2;             /* Coords of point 2 rel to ctr. and
                           *  adjusted to same radius as p1 (S16) */
   si32 xint[4];           /* Up to 4 intersection of y w/sector */
   si32 nx,ny;             /* Size of lattice along x,y */
   short ixop;             /* Next x operation--see enum ixop */
   short yac;              /* y action code from setup */
   } IterSect;

#ifdef __cplusplus
extern "C" {
#endif
/* Call to initialize sector indexing */
ret_t InitSectorIndexing(IterSect *IS, xyf p0, xyf p1, xyf p2,
   long nx, long ny);

/* Call to obtain next point in sector */
int GetNextPointInSector(IterSect *IS);
#ifdef __cplusplus
}
#endif

#endif /* ifndef ITERSECT_HDR_INCLUDED */
