/* (c) Copyright 2011, The Rockefeller University *11114* */
/* $Id: itershl2.h 59 2017-01-13 20:34:16Z  $ */
/***********************************************************************
*                             itershl2.h                               *
*                                                                      *
*  This header file declares a pair of routines that may be used to    *
*  find successively all the points on some rectangular lattice that   *
*  fall between two given concentric circles.  The center of the two   *
*  circles need not fall on a grid point.                              *
*                                                                      *
*  This is a two-dimensional version of itershl.h                      *
*                                                                      *
*  The user must provide an instance of the IterShl2 data structure    *
*  declared here for each invocation of the routines.                  *
*                                                                      *
*  V1A, 10/17/11, GNR - Initial version                                *
*  ==>, 10/24/11, GNR - Last date before committing to svn repository  *
***********************************************************************/

#ifndef ITERSHL2_HDR_INCLUDED
#define ITERSHL2_HDR_INCLUDED

#include "sysdef.h"

/* Structure used by shell indexing routine to save state and
*  return results  */
typedef struct IterShl2 {
   /* Variables used to return results to caller */
   long ioff;              /* Returns offset to next grid point */
   long ix,iy;             /* Returns current grid point coords */
   /* Internal variables, not for user access */
   double xc,yc;           /* Center of circle */
   double r1sq,r2sq;       /* Squared radii of 2 circles */
   long nx,ny;             /* Size of lattice along ix,iy */
   long ixmax,iymax;       /* Upper bounds on ix,iy */
   long ix2lo,ix2hi;       /* Stored bounds of second sector */
   long ioffrow;           /* Offset to current row */
   } IterShl2;

#ifdef __cplusplus
extern "C" {
#endif
/* Call to initialize 2-D shell indexing */
void InitShell2Indexing(IterShl2 *IShl2, double xc,
   double yc, double r1, double r2, long nx, long ny);

/* Call to obtain next point in spherical shell */
int GetNextPointInShell2(IterShl2 *IShl2);
#ifdef __cplusplus
}
#endif

#endif /* ifndef ITERSHL2_HDR_INCLUDED */
