/* (c) Copyright 2011, The Rockefeller University *11114* */
/* $Id: itercirc.h 59 2017-01-13 20:34:16Z  $ */
/***********************************************************************
*                             itercirc.h                               *
*                                                                      *
*  This header file declares a pair of routines that may be used to    *
*  find successively all the points on some 2-D rectangular lattice    *
*  that fall within a given circle.  The center of the circle need     *
*  not fall on a grid point.  (Use itersph if the lattice axes are     *
*  not the same length.)                                               *
*                                                                      *
*  The user must provide an instance of the IterCirc data structure    *
*  declared here for each invocation of the routines.                  *
*                                                                      *
*  V1A, 08/13/11, GNR - Initial version, a subset of itersph           *
*  ==>, 08/13/11, GNR - Last date before committing to svn repository  *
***********************************************************************/

#ifndef ITERCIRC_HDR_INCLUDED
#define ITERCIRC_HDR_INCLUDED

#include "sysdef.h"

/* Structure used by circle indexing routine to save state and
*  return results  */
typedef struct IterCirc {
   /* Variables used to return results to caller */
   long ioff;              /* Returns offset to next grid point */
   long ix,iy;             /* Returns current grid point x,y */
   /* Internal variables, not for user access */
   double xc,yc;           /* Center of circle */
   double radius;          /* Radius of circle */
   double r2;              /* Squared radius of circle */
   long mx,my;             /* Max x on row, y overall */
   long nx,ny;             /* Size of lattice along x,y */
   long iofrow;            /* Offset to current row */
   } IterCirc;

#ifdef __cplusplus
extern "C" {
#endif
/* Call to initialize circle indexing */
void InitCircleIndexing(IterCirc *ICirc, double xc, double yc,
   double radius, long nx, long ny);

/* Call to obtain next point in circle */
int GetNextPointInCircle(IterCirc *ICirc);
#ifdef __cplusplus
}
#endif

#endif /* ifndef ITERCIRC_HDR_INCLUDED */
