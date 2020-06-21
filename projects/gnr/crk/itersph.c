/* (c) Copyright 1995-2008, The Rockefeller University *11115* */
/* $Id: itersph.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                              itersph.c                               *
*                                                                      *
*  This module contains routines that may be used to find successively *
*  all the points on some rectangular lattice that fall within a given *
*  sphere.  The center of the sphere need not fall on a grid point.    *
*                                                                      *
*  The user must provide an IterSph structure which is used both to    *
*  store the internal state of the iterator routine and to return      *
*  results.  (This is less than beautifully clean, but is efficient    *
*  and will have to do until C++ comes along...)                       *
*                                                                      *
*  The user calls InitSphereIndexing to initialize indexing for some   *
*  particular lattice and sphere, and then calls GetNextPointInSphere  *
*  to obtain the coordinates of the next point and its offset from the *
*  start of the lattice array.                                         *
*                                                                      *
*  The user may carry out multiple iterations at the same time by      *
*  providing a separate IterSph structure for each.                    *
*                                                                      *
*  The IterSph structure and the two calls are prototyped in itersph.h *
*                                                                      *
************************************************************************
*  V1A, 12/28/95, GNR - Initial version                                *
*  Rev, 08/17/00, GNR - Minor corrections for edge effects             *
*  ==>, 11/16/08, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "itersph.h"

/*---------------------------------------------------------------------*
*                         InitSphereIndexing                           *
*                                                                      *
*  Arguments:                                                          *
*     ISph                 Ptr to IterSph structure that may be used   *
*                          to hold state of sphere indexing routine    *
*     dx,dy,dz             Size of lattice units along x,y,z           *
*     xc,yc,zc             Center of sphere in GRID UNITS along x,y,z  *
*     radius               Radius of sphere (same units as dx,dy,dz)   *
*     ixmax,iymax,izmax    Number of lattice units along x,y,z         *
*  Returns:                                                            *
*     ISph is initialized for subsequent calls to GetNextPointInSphere *
*                                                                      *
*  Notes:                                                              *
*     Arguments ixmax,iymax,izmax define the size of the rectangular   *
*  lattice.  These values are used for two purposes: (1) to calculate  *
*  the offset within the lattice to a point in the sphere, and (2) to  *
*  limit the points returned to points inside the given bounds.  It    *
*  is valid for part or all of the sphere to lie outside the given     *
*  lattice bounds--points outside the bounds are never returned.       *
*                                                                      *
*  Restrictions:                                                       *
*     Currently the lattice must have right angles between the axes    *
*  and the lower bound of each axis is always taken to be zero.  The   *
*  ordering of indexes for computing lattice offsets is z slow moving, *
*  then y, then x.                                                     *
*---------------------------------------------------------------------*/

void InitSphereIndexing(IterSph *ISph,
      double dx, double dy, double dz,
      double xc, double yc, double zc, double radius,
      long nx,   long ny,   long nz) {

   double zrad = radius/dz;
   double zlow = zc - zrad;

   /* Save sphere parameters */
   ISph->dx = dx;
   ISph->dy = dy;
   ISph->dz = dz;
   ISph->xc = xc;
   ISph->yc = yc;
   ISph->zc = zc;
   ISph->radius = radius;
   ISph->r2 = radius*radius;
   ISph->nx = nx;
   ISph->ny = ny;
   ISph->nz = nz;
   ISph->nxy = nx*ny;

   /* Force new section on first GetNextPointInSphere call */
   ISph->ixmax = ISph->iymax = ISph->ix = ISph->iy = 0;

   /* Set test for highest z that passes inside the sphere */
   ISph->izmax = (int)(zc + zrad);
   if (ISph->izmax >= nz) ISph->izmax = nz - 1;

   /* Set current z to one section below bottom of sphere.
   *  When zlow is positive, rounding down will put the
   *  first iz just outside the sphere, so the first call
   *  to GetNextPointInSphere will bring us into the sphere.  */
   if (zlow > 0.0) {
      ISph->iz = (long)zlow;
      if ((double)ISph->iz == zlow) ISph->iz -= 1;
      }
   else
      ISph->iz = -1;

   } /* End InitSphereIndexing */

/*---------------------------------------------------------------------*
*                        GetNextPointInSphere                          *
*                                                                      *
*  Arguments:                                                          *
*     ISph              Ptr to IterSph structure initialized by        *
*                       previous call to InitSphereIndexing            *
*  Returns:                                                            *
*     ISph->ix,iy,iz    Index values of next lattice point in sphere   *
*                       along x,y,z directions                         *
*     ISph->ioff        Offset of point ix,iy,iz from origin of        *
*                       lattice                                        *
*     Function value    TRUE if a grid point was found inside the      *
*                          given sphere.  Values in ISph->ix,iy,iz,    *
*                          and ioff are valid.                         *
*                       FALSE if all grid points in the sphere have    *
*                          already been returned.  Values in ISph      *
*                          are no longer valid.                        *
*---------------------------------------------------------------------*/

int GetNextPointInSphere(IterSph *ISph) {

   double q,q2,qrad,qlow;

   /* The loops are in case there are no acceptable x's (y's)
   *  in the current z plane--unlikely, but possible.  */
   ISph->ix += 1;
   while (ISph->ix > ISph->ixmax) {
      ISph->iy += 1;
      while (ISph->iy > ISph->iymax) {
         if (++ISph->iz > ISph->izmax) return FALSE;

         /* New z, calculate new starting and ending y */
         ISph->iofsec = ISph->iz * ISph->nxy;
         q = ((double)ISph->iz - ISph->zc)*ISph->dz;
         ISph->rxy2 = ISph->r2 - q*q;
         if (ISph->rxy2 < 0.0) ISph->rxy2 = 0.0; /* JIC */
         qrad = sqrt(ISph->rxy2)/ISph->dy;
         ISph->iymax = ISph->yc + qrad;
         if (ISph->iymax >= ISph->ny) ISph->iymax = ISph->ny - 1;
         if ((qlow = ISph->yc - qrad) > 0.0) {
            ISph->iy = (long)qlow;
            if ((double)ISph->iy < qlow) ISph->iy += 1;
            }
         else
            ISph->iy = 0;
         } /* End z update */

      /* New y, calculate new starting and ending x */
      ISph->iofrow = ISph->iofsec + ISph->iy * ISph->nx;
      q = ((double)ISph->iy - ISph->yc)*ISph->dy;
      q2 = ISph->rxy2 - q*q;
      qrad = (q2 > 0.0) ? sqrt(q2)/ISph->dx : 0.0;
      ISph->ixmax = (int)(ISph->xc + qrad);
      if (ISph->ixmax >= ISph->nx) ISph->ixmax = ISph->nx - 1;
      if ((qlow = ISph->xc - qrad) > 0.0) {
         ISph->ix = (long)qlow;
         if ((double)ISph->ix < qlow) ISph->ix += 1;
         }
      else
         ISph->ix = 0;
      } /* End y update */

   /* New x, calculate lattice offset and return */
   ISph->ioff = ISph->iofrow + ISph->ix;
   return TRUE;
   } /* End GetNextPointInSphere */
