/* (c) Copyright 1995-2008, The Rockefeller University *11114* */
/* $Id: itersph.h 59 2017-01-13 20:34:16Z  $ */
/***********************************************************************
*                              itersph.h                               *
*                                                                      *
*  This header file declares a pair of routines that may be used to    *
*  find successively all the points on some rectangular lattice that   *
*  fall within a given sphere.  The center of the sphere need not      *
*  fall on a grid point.                                               *
*                                                                      *
*  The user must provide an instance of the IterSph data structure     *
*  declared here for each invocation of the routines.                  *
*                                                                      *
*  V1A, 12/28/95, GNR - Initial version                                *
*  ==>, 03/22/08, GNR - Last date before committing to svn repository  *
*  V2A, 11/16/08, GNR - Revise for 64-bit and C++ operation            *
***********************************************************************/

#ifndef ITERSPH_HDR_INCLUDED
#define ITERSPH_HDR_INCLUDED

#include "sysdef.h"

/* Structure used by sphere indexing routine to save state and
*  return results  */
typedef struct IterSph {
   /* Variables used to return results to caller */
   long ioff;              /* Returns offset to next grid point */
   long ix,iy,iz;          /* Returns current grid point x,y,z  */
   /* Internal variables, not for user access */
   double dx,dy,dz;        /* Lattice size along x,y,z */
   double xc,yc,zc;        /* Center of sphere */
   double radius;          /* Radius of sphere */
   double r2,rxy2;         /* Squared radius of sphere, and of
                           *  circle in xy plane at current z */
   long nx,ny,nz;          /* Size of lattice along ix,iy,iz */
   long nxy;               /* nx * ny */
   long ixmax,iymax,izmax; /* Upper bounds on ix,iy,iz */
   long iofsec,iofrow;     /* Offset to current section, row */
   } IterSph;

#ifdef __cplusplus
extern "C" {
#endif
/* Call to initialize sphere indexing */
void InitSphereIndexing(IterSph *ISph,
   double dx, double dy, double dz,
   double xc, double yc, double zc, double radius,
   long nx,   long ny,   long nz);

/* Call to obtain next point in sphere */
int GetNextPointInSphere(IterSph *ISPH);
#ifdef __cplusplus
}
#endif

#endif /* ifndef ITERSPH_HDR_INCLUDED */
