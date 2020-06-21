/* (c) Copyright 1995-2008, The Rockefeller University *11115* */
/* $Id: itercyl.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                              itercyl.c                               *
*                                                                      *
*  This module contains routines that may be used to find successively *
*  all the points on some rectangular lattice that fall within a given *
*  cylinder.  The boundaries of the cylinder need not fall on grid     *
*  points.                                                             *
*                                                                      *
*  The user must provide an IterCyl structure which is used both to    *
*  store the internal state of the iterator routine and to return      *
*  results.  (This is less than beautifully clean, but is efficient    *
*  and will have to do until C++ comes along...)                       *
*                                                                      *
*  The user must call InitCylinderIndexing to initialize indexing for  *
*  some particular cylinder.  Each time another grid point is wanted,  *
*  a call to GetNextPointInCylinder returns the x,y,z coordinates of   *
*  the next point and its offset from the start of the lattice array.  *
*                                                                      *
*  The user may carry out multiple iterations at the same time by      *
*  providing a separate IterCyl structure for each.                    *
*                                                                      *
*  The IterCyl structure and the two calls are prototyped in itercyl.h *
*                                                                      *
************************************************************************
*                                                                      *
*  METHOD:                                                             *
*  InitCylinderIndexing determines the extremal values of z that can   *
*  possibly intersect the given cylinder.  Each time indexing pro-     *
*  gresses to a new z, extremal values of y that can possibly          *
*  intersect the cylinder at that z are computed and looped over.      *
*  Each time a indexing progresses to a new y, extremal values of x    *
*  that can possibly intersect the cylinder at that y are computed     *
*  and looped over.  This strategy makes the inner x loop very fast.   *
*  The calculations take proper account of intersections on the        *
*  cylindrical surface proper or on the planar end plates.             *
*                                                                      *
************************************************************************
*  V1A, 12/28/95, GNR - Initial version                                *
*  Rev, 08/18/00, GNR - Avoid (double)->(long) overflows               *
*  V2A, 11/16/08, GNR - Cleanup, header revised for 64-bit operation   *
*  ==>, 11/16/08, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "itercyl.h"

#define X   0              /* X component of a vector */
#define Y   1              /* Y component of a vector */
#define Z   2              /* Z component of a vector */

#define BIG   1.0E20       /* A large number */
#define SMALL 1.0E-4       /* Roundoff error */

/*---------------------------------------------------------------------*
*                        InitCylinderIndexing                          *
*                                                                      *
*  Arguments:                                                          *
*     Cyl                  Ptr to IterCyl structure that pgm will use  *
*                          to hold state of cylinder indexing routine  *
*     dx,dy,dz             Size of lattice units along x,y,z           *
*     xe1,ye1,ze1          Grid coords of one end of cylinder axis     *
*     xe2,ye2,ze2          Grid coords of other end of cylinder axis   *
*     radius               Radius of cylinder (same units as dx,dy,dz) *
*     nx,ny,nz             Number of lattice units along x,y,z         *
*  Returns:                                                            *
*     Cyl is initialized for later calls to GetNextPointInCylinder     *
*                                                                      *
*  Notes:                                                              *
*     Arguments nx,ny,nz define the size of the rectangular lattice.   *
*  These values are used for two purposes: (1) to calculate the        *
*  offset within the lattice to a point in the cylinder, and (2) to    *
*  limit the points returned to points inside the given bounds.  It    *
*  is valid for part or all of the cylinder to lie outside the given   *
*  lattice bounds--points outside the bounds are never returned.       *
*                                                                      *
*  Restrictions:                                                       *
*     Currently the lattice must have right angles between the axes    *
*  and the lower bound of each axis is always taken to be zero.  The   *
*  ordering of indexes for computing lattice offsets is z slow moving, *
*  then y, then x.                                                     *
*---------------------------------------------------------------------*/

void InitCylinderIndexing(IterCyl *Cyl,
      double dx,  double dy,  double dz,
      double xe1, double ye1, double ze1,
      double xe2, double ye2, double ze2,
      double radius, long nx, long ny, long nz) {

   double sz2;                /* Sin squared of angle from axis to z */
   double x,y,z,d;            /* For calc of length of cylinder axis */
   double zmin,zmax;          /* Extremal values of z */
   double zrad;               /* Radius of cylinder projected on z */

   /* Save cylinder parameters */
   Cyl->dx = dx;
   Cyl->dy = dy;
   Cyl->dz = dz;
   Cyl->radius = radius;
   Cyl->nx = nx;
   Cyl->ny = ny;
   Cyl->nz = nz;
   Cyl->nxy = nx*ny;
   Cyl->dnx = (double)nx;
   Cyl->dny = (double)ny;

   /* Calculate semiaxis length, unit axis vector, and center of
   *  cylinder.  Assumes checking for short axis and radius has
   *  already been done by caller.  */
   x = (xe2 - xe1)*dx;
   y = (ye2 - ye1)*dy;
   z = (ze2 - ze1)*dz;
   d = sqrt(x*x + y*y + z*z);
   Cyl->haxis = 0.5*d;
   Cyl->Cn[X] = x/d;
   Cyl->Cn[Y] = y/d;
   Cyl->Cn[Z] = z/d;
   Cyl->Co[X] = 0.5*(xe1 + xe2)*dx;
   Cyl->Co[Y] = 0.5*(ye1 + ye2)*dy;
   Cyl->Co[Z] = 0.5*(ze1 + ze2)*dz;

   /* Useful quantities derived from radius and semiaxis */
   Cyl->rr = radius*radius;
   Cyl->rrhh = Cyl->rr + Cyl->haxis*Cyl->haxis;
   Cyl->csxh  = Cyl->haxis * (x >= 0.0 ? 1.0 : -1.0);
   Cyl->csyzh = Cyl->haxis * (y >= 0.0 ? 1.0 : -1.0) *
                             (z >= 0.0 ? 1.0 : -1.0);

   /* Force new section on first GetNextPointInCylinder call */
   Cyl->ixmax = Cyl->iymax = Cyl->ix = Cyl->iy = 0;

/* Determine maximal extent of cylinder in grid units along z by
*  taking the projection of its axis on z plus the contribution
*  the radius would make when aligned in plane of axis and z.  */

   sz2 = 1.0 - Cyl->Cn[Z]*Cyl->Cn[Z];
   zrad = (sz2 > 0.0) ? (radius*sqrt(sz2)/dz) : 0.0;
   zmin = min(ze1, ze2) - zrad;  /* In grid units */
   zmax = max(ze1, ze2) + zrad;  /* In grid units */

   /* Set test for highest z that passes inside the cylinder */
   Cyl->izmax = (long)zmax;
   if (Cyl->izmax >= nz) Cyl->izmax = nz - 1;

   /* Set current z to one section below bottom of cylinder.
   *  When zmin is positive, rounding down will put the
   *  first iz just outside the cylinder, so the first call
   *  to GetNextPointInCylinder will bring us into the cylinder.  */
   if (zmin > 0.0) {
      Cyl->iz = (long)zmin;
      if ((double)Cyl->iz == zmin) Cyl->iz = -1;
      }
   else
      Cyl->iz = -1;

   } /* End InitCylinderIndexing */

/*---------------------------------------------------------------------*
*                       GetNextPointInCylinder                         *
*                                                                      *
*  Arguments:                                                          *
*     Cyl               Ptr to IterCyl structure initialized by        *
*                       previous call to InitCylinderIndexing          *
*  Returns:                                                            *
*     Cyl->ix,iy,iz     Grid coords of next lattice point in cylinder  *
*     Cyl->ioff         Offset of point ix,iy,iz from lattice origin   *
*     Function value    TRUE if a grid point was found inside the      *
*                          given cylinder.  Values in Cyl->ix,iy,iz,   *
*                          and ioff are valid.                         *
*                       FALSE if all grid points in the cylinder have  *
*                          already been returned.  Values in Cyl->ix,  *
*                          iy,iz, and ioff are no longer valid.        *
*---------------------------------------------------------------------*/

int GetNextPointInCylinder(IterCyl *Cyl) {

   double xmin,xmax;             /* New x limits */
   double ymin,ymax;             /* New y limits */
   double xn = Cyl->Cn[X];       /* Normalized axis vector */
   double yn = Cyl->Cn[Y];
   double zn = Cyl->Cn[Z];
   double y,z;                   /* Coords of current point,
                                 *  relative to origin at Co */
   double a,b,c;                 /* Coeffs of quadratic eqn. */
   double d;                     /* Discriminant of quadratic */
   double s;                     /* End selector (-1.0 or +1.0) */
   double dxx,dyz;               /* Temps for calcs below */
   double txx,tyy,tzz;
   double trx,tzy,tzh,ty;
   double tyz,tyyzz,tx;

   /* The loops are in case there are no acceptable x's (y's)
   *  in the current z plane--unlikely, but possible.  */
   Cyl->ix += 1;
   while (Cyl->ix > Cyl->ixmax) {

      Cyl->iy += 1;
      while (Cyl->iy > Cyl->iymax) {

         if (++Cyl->iz > Cyl->izmax) return FALSE;

/* New z, calculate lattice offset, then new starting and ending y */

         Cyl->iofsec = Cyl->iz * Cyl->nxy;

         z = (double)Cyl->iz*Cyl->dz - Cyl->Co[Z];
         ymin = BIG; ymax = -BIG;
         dxx = xn*xn;
         dyz = sqrt(yn*yn + zn*zn);
         tzy = z*yn;
         tzz = z*zn;
         trx = Cyl->radius*dyz;
         /* Check positive and negative faces of cylinder in turn.
         *  Note that if xn = 1.0 or yn = 1.0, then zn = 0.0, and
         *  we jigger txx,tyy to force end-plane intersection test.
         */
         for (s=-1.0; s<=1.0; s+=2.0) {
            if (fabs(zn) < SMALL)
               txx = tyy = BIG;
            else {
               ty = (tzy + s*trx)/zn;
               txx = dxx*((tyy = ty*yn) + tzz)/(1.0 - dxx);
               }
            if (fabs(txx + tyy + tzz) <= Cyl->haxis) {
               ymin = min(ty,ymin); ymax = max(ty,ymax);
               }
            else {
            /* No intersection of plane with cylindrical surface.
            *  Test end plane at same end instead.  */
               a = 1.0 - zn*zn;
               if (fabs(a) < SMALL) continue;
               tzh = tzz - s*Cyl->csyzh;
               b = yn*tzh;
               c = tzh*tzh - dxx*(Cyl->rrhh - z*z);
               if ((d = b*b - a*c) < 0.0) continue;
               d = sqrt(d);
               ty = (-b + d)/a;
               ymin = min(ty,ymin); ymax = max(ty,ymax);
               ty = (-b - d)/a;
               ymin = min(ty,ymin); ymax = max(ty,ymax);
               } /* End end-plane test */
            } /* End loop over two faces */
         /* Adjust y range to included integral grid points.
         *  These fancy tests are needed to avoid overflows
         *  when floating point ymax,ymin converted to longs.  */
         ymax = (ymax + Cyl->Co[Y])/Cyl->dy;
         if (ymax >= Cyl->dny)
            Cyl->iymax = Cyl->ny - 1;
         else if (ymax <= 0.0)
            Cyl->iymax = 0;
         else
            Cyl->iymax = (long)ymax;
         ymin = (ymin + Cyl->Co[Y])/Cyl->dy;
         if (ymin >= Cyl->dny)
            Cyl->iy = Cyl->ny - 1;
         else if (ymin <= 0.0)
            Cyl->iy = 0;
         else {
            Cyl->iy = (long)ymin;
            if ((double)Cyl->iy == ymin) Cyl->iy += 1;
            }
         } /* End z update */

/* New y, calculate lattice offset, then new starting and ending x */

      Cyl->iofrow = Cyl->iofsec + Cyl->iy * Cyl->nx;

      y = (double)Cyl->iy*Cyl->dy - Cyl->Co[Y];
      xmin = BIG; xmax = -BIG;
      tyz = y*yn + z*zn;
      tyyzz = y*y + z*z;
      a = 1.0 - xn*xn;
      b = xn*tyz;
      d = tyz*tyz - a*(tyyzz - Cyl->rr);
      /* Intersect y,z line with cylindrical surface */
      if (d > 0.0) d = sqrt(d);
      for (s=-1.0; s<=1.0; s+=2.0) {
         int ksurf = FALSE;
         if (fabs(a) < SMALL || d < 0) ksurf = TRUE;
         else {
            tx = (b + s*d)/a;
            if (fabs(tx*xn + tyz) > Cyl->haxis) ksurf = TRUE;
            }
         if (ksurf) {
            /* No solutions on round surface, test ends */
            if (fabs(xn) < SMALL) continue;
            tx = (s*Cyl->csxh - tyz)/xn;
            if (tx*tx + tyyzz > Cyl->rrhh) continue;
            }
         xmin = min(tx,xmin); xmax = max(tx,xmax);
         } /* End loop over two surfaces */
      /* Adjust x range to included integral grid points */
      xmax = (xmax + Cyl->Co[X])/Cyl->dx;
      if (xmax >= Cyl->dnx)
         Cyl->ixmax = Cyl->nx - 1;
      else if (xmax <= 0.0)
         Cyl->ixmax = 0;
      else
         Cyl->ixmax = (long)xmax;
      xmin = (xmin + Cyl->Co[X])/Cyl->dx;
      if (xmin >= Cyl->dnx)
         Cyl->ix = Cyl->nx - 1;
      else if (xmin <= 0.0)
         Cyl->ix = 0;
      else {
         Cyl->ix = (long)xmin;
         if ((double)Cyl->ix == xmin) Cyl->ix += 1;
         }
      } /* End y update */

/* New x, calculate lattice offset and return */

   Cyl->ioff = Cyl->iofrow + Cyl->ix;

   return TRUE;
   } /* End GetNextPointInCylinder */
