/* (c) Copyright 2000-2008, The Rockefeller University *11114* */
/* $Id: itershl.h 59 2017-01-13 20:34:16Z  $ */
/***********************************************************************
*                              itershl.h                               *
*                                                                      *
*  This header file declares a pair of routines that may be used to    *
*  find successively all the points on some rectangular lattice that   *
*  fall between two given concentric spheres.  The center of the two   *
*  spheres need not fall on a grid point.                              *
*                                                                      *
*  The user must provide an instance of the IterShl data structure     *
*  declared here for each invocation of the routines.                  *
*                                                                      *
*  V1A, 08/04/00, GNR - Initial version                                *
*  ==>, 03/22/08, GNR - Last date before committing to svn repository  *
*  V2A, 11/16/08, GNR - Revise for 64-bit and C++ operation            *
***********************************************************************/

#ifndef ITERSHL_HDR_INCLUDED
#define ITERSHL_HDR_INCLUDED

#include "sysdef.h"

/* Structure used by shell indexing routine to save state and
*  return results  */
typedef struct IterShl {
   /* Variables used to return results to caller */
   long ioff;              /* Returns offset to next grid point */
   long ix,iy,iz;          /* Returns current grid point x,y,z  */
   /* Internal variables, not for user access */
   double dx,dy,dz;        /* Lattice size along x,y,z */
   double xc,yc,zc;        /* Center of sphere */
   double r1sq,r2sq;       /* Squared radii of 2 spheres */
   double r1xy2,r2xy2;     /* Squared radii at current z */
   long nx,ny,nz;          /* Size of lattice along ix,iy,iz */
   long nxy;               /* nx * ny */
   long ixmax,iymax,izmax; /* Upper bounds on ix,iy,iz */
   long ix2lo,ix2hi;       /* Stored bounds of second sector */
   long iofsec,iofrow;     /* Offset to current section, row */
   } IterShl;

#ifdef __cplusplus
extern "C" {
#endif
/* Call to initialize sphere indexing */
void InitShellIndexing(IterShl *IShl,
   double dx, double dy, double dz,
   double xc, double yc, double zc,
   double r1, double r2,
   long nx,   long ny,   long nz);

/* Call to obtain next point in spherical shell */
int GetNextPointInShell(IterShl *ISHL);
#ifdef __cplusplus
}
#endif

#endif /* ifndef ITERSHL_HDR_INCLUDED */
