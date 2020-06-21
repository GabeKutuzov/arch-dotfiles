/* (c) Copyright 1995-2008, The Rockefeller University *11114* */
/* $Id: itercyl.h 59 2017-01-13 20:34:16Z  $ */
/***********************************************************************
*                              itercyl.h                               *
*                                                                      *
*  This header file declares a pair of routines that may be used to    *
*  find successively all the points on some rectangular lattice that   *
*  fall within a given cylinder.  The ends of the cylinder need not    *
*  fall on grid points.                                                *
*                                                                      *
*  The user must provide an instance of the IterCyl data structure     *
*  declared here for each invocation of the routines.                  *
*                                                                      *
*  V1A, 12/28/95, GNR - Initial version                                *
*  ==>, 03/22/08, GNR - Last date before committing to svn repository  *
*  V2A, 11/16/08, GNR - Revise for 64-bit and C++ operation            *
***********************************************************************/

#ifndef ITERCYL_HDR_INCLUDED
#define ITERCYL_HDR_INCLUDED

#include "sysdef.h"

/* Structure used by cylinder indexing routine to save its state and
*  to return results.  Variables beginning with 'C' are in Cartesian
*  coordinates, others are in grid units.  */
typedef struct IterCyl {
   /* Variables used to return results to caller */
   long ioff;              /* Returns offset to next grid point */
   long ix,iy,iz;          /* Returns coords of current grid point */
   /* Internal variables, not for user access */
   double dnx,dny;         /* (double)nx, (double)ny */
   double dx,dy,dz;        /* Lattice size along x,y,z */
   double Cn[3];           /* Cylinder axis unit vector */
   double Co[3];           /* Cylinder center point */
   double haxis;           /* Half axis of cylinder */
   double radius;          /* Radius of cylinder */
   double rr;              /* Radius squared */
   double rrhh;            /* Radius squared plus haxis squared */
   double csxh;            /* haxis * sign*xn) */
   double csyzh;           /* haxis * sign(yn) * sign(zn) */
   long nx,ny,nz;          /* Size of lattice along ix,iy,iz */
   long nxy;               /* nx * ny */
   long ixmax,iymax,izmax; /* Upper bounds on ix,iy,iz */
   long iofsec,iofrow;     /* Offset to current section, row */
   } IterCyl;

#ifdef __cplusplus
extern "C" {
#endif
/* Call to initialize cylinder indexing */
void InitCylinderIndexing(IterCyl *Cyl,
   double dx, double dy, double dz,
   double xe1, double ye1, double ze1,
   double xe2, double ye2, double ze2,
   double radius, long nx, long ny, long nz);

/* Call to obtain next point in cylinder */
int GetNextPointInCylinder(IterCyl *CYL);
#ifdef __cplusplus
}
#endif

#endif /* ifndef ITERCYL_HDR_INCLUDED */
