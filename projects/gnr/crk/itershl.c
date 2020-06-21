/* (c) Copyright 2000-2008, The Rockefeller University *11115* */
/* $Id: itershl.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                              itershl.c                               *
*                                                                      *
*  This module contains routines that may be used to find successively *
*  all the points on some rectangular lattice that fall in a shell     *
*  between two given concentric spheres.  The center of the spheres    *
*  need not fall on a grid point.  Points exactly on the outer radius  *
*  are included.  Points exactly on the inner radius are excluded.     *
*  This prevents double counting when a sphere is extended in shells.  *
*                                                                      *
*  The user must provide an IterShl structure which is used both to    *
*  store the internal state of the iterator routine and to return      *
*  results.  (This is less than beautifully clean, but is efficient    *
*  and will have to do until C++ comes along...)                       *
*                                                                      *
*  The user calls InitShellIndexing to initialize indexing for some    *
*  particular lattice and spheres, and then calls GetNextPointInShell  *
*  to obtain the coordinates of the next point and its offset from the *
*  start of the lattice array.                                         *
*                                                                      *
*  The user may carry out multiple iterations at the same time by      *
*  providing a serate IterShl structure for each.                      *
*                                                                      *
*  The IterShl structure and the two calls are prototyped in itershl.h *
*                                                                      *
************************************************************************
*  V1A, 08/08/00, GNR - Initial version, based on itersph.c            *
*  ==>, 11/16/08, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "itershl.h"

/*---------------------------------------------------------------------*
*                          InitShellIndexing                           *
*                                                                      *
*  Arguments:                                                          *
*     IShl                 Ptr to IterShl structure that may be used   *
*                          to hold state of shell indexing routine     *
*     dx,dy,dz             Size of lattice units along x,y,z           *
*     xc,yc,zc             Center of sphere in GRID UNITS along x,y,z  *
*     r1,r2                Inner and outer radii of spherical shell    *
*     nx,ny,nz             Number of lattice units along x,y,z         *
*  Returns:                                                            *
*     IShl is initialized for subsequent calls to GetNextPointInShell  *
*                                                                      *
*  Notes:                                                              *
*     Arguments nx,ny,nz define the size of the rectangular lattice.   *
*  These values are used for two purposes:  (1) to calculate the       *
*  offset within the lattice to a point in the sphere, and (2) to      *
*  limit the points returned to points inside the given bounds.  It is *
*  valid for part or all of the shell to lie outside the given lattice *
*  bounds--points outside the bounds are never returned.               *
*                                                                      *
*  Restrictions:                                                       *
*     Currently the lattice must have right angles between the axes    *
*  and the lower bound of each axis is always taken to be zero.  The   *
*  ordering of indexes for computing lattice offsets is z slow moving, *
*  then y, then x.                                                     *
*---------------------------------------------------------------------*/

void InitShellIndexing(IterShl *IShl,
      double dx, double dy, double dz,
      double xc, double yc, double zc,
      double r1, double r2,
      long nx,   long ny,   long nz) {

   double zrad = r2/dz;
   double zlow = zc - zrad;

   /* Save shell parameters */
   IShl->dx = dx;
   IShl->dy = dy;
   IShl->dz = dz;
   IShl->xc = xc;
   IShl->yc = yc;
   IShl->zc = zc;
   IShl->r1sq = r1*r1;
   IShl->r2sq = r2*r2;
   IShl->nx = nx;
   IShl->ny = ny;
   IShl->nz = nz;
   IShl->nxy = nx*ny;

   /* Force new section on first GetNextPointInShell call */
   IShl->ixmax = IShl->ix2hi = IShl->iymax = IShl->ix = IShl->iy = 0;
   IShl->ix2lo = 1;

   /* Set test for highest z that passes inside the sphere */
   IShl->izmax = (long)(zc + zrad);
   if (IShl->izmax >= nz) IShl->izmax = nz - 1;

   /* Set current z to one section below bottom of sphere.
   *  When zlow is positive, rounding down will put the
   *  first iz just outside the sphere, so the first call
   *  to GetNextPointInShell will bring us into the sphere.  */
   if (zlow > 0.0) {
      IShl->iz = (long)zlow;
      if ((double)IShl->iz == zlow) IShl->iz -= 1;
      }
   else
      IShl->iz = -1;

   } /* End InitShellIndexing() */

/*---------------------------------------------------------------------*
*                         GetNextPointInShell                          *
*                                                                      *
*  Arguments:                                                          *
*     IShl              Ptr to IterShl structure initialized by        *
*                       previous call to InitShellIndexing             *
*  Returns:                                                            *
*     IShl->ix,iy,iz    Index values of next lattice point in shell    *
*                       along x,y,z directions                         *
*     IShl->ioff        Offset of point ix,iy,iz from origin of        *
*                       lattice                                        *
*     Function value    TRUE if a grid point was found inside the      *
*                          given shell.  Values in IShl->ix,iy,iz,     *
*                          and ioff are valid.                         *
*                       FALSE if all grid points in the shell have     *
*                          already been returned.  Values in IShl      *
*                          are no longer valid.                        *
*---------------------------------------------------------------------*/

int GetNextPointInShell(IterShl *IShl) {

   if (++IShl->ix > IShl->ixmax) {
      double q,q2,qrad,qlow,x2;
      long iqmx,iqmn;
      if (IShl->ix2hi >= IShl->ix2lo) {
         /* Execute the second discontinuous range along x */
         IShl->ix = IShl->ix2lo;
         IShl->ixmax = IShl->ix2hi;
         IShl->ix2hi = -1;    /* Advance y next time */
         }
      else {
TryAnotherY:
         IShl->iy += 1;
         /* The loop is in case there are no acceptable y's
         *  in the current z plane--unlikely, but possible.  */
         while (IShl->iy > IShl->iymax) {
            if (++IShl->iz > IShl->izmax) return FALSE;

            /* New z, calculate new starting and ending y */
            IShl->iofsec = IShl->iz * IShl->nxy;
            q = ((double)IShl->iz - IShl->zc)*IShl->dz;
            q2 = q*q;
            IShl->r1xy2 = IShl->r1sq - q2;
            IShl->r2xy2 = IShl->r2sq - q2;
            if (IShl->r2xy2 < 0.0) IShl->r2xy2 = 0.0; /* JIC */
            qrad = sqrt(IShl->r2xy2)/IShl->dy;
            IShl->iymax = IShl->yc + qrad;
            if (IShl->iymax >= IShl->ny) IShl->iymax = IShl->ny - 1;
            if ((qlow = IShl->yc - qrad) > 0.0) {
               IShl->iy = (long)qlow;
               if ((double)IShl->iy < qlow) IShl->iy += 1;
               }
            else
               IShl->iy = 0;
            } /* End z update */

         /* New y, calculate new starting and ending x */
         IShl->iofrow = IShl->iofsec + IShl->iy * IShl->nx;
         q = ((double)IShl->iy - IShl->yc)*IShl->dy;
         q2 = q*q;
         /* Check x bounds at outer radius */
         x2 = IShl->r2xy2 - q2;
         qrad = (x2 > 0.0) ? sqrt(x2)/IShl->dx : 0.0;
         IShl->ixmax = IShl->xc + qrad;
         if (IShl->ixmax >= IShl->nx) IShl->ixmax = IShl->nx - 1;
         if ((qlow = IShl->xc - qrad) > 0.0) {
            IShl->ix = (long)qlow;
            if ((double)IShl->ix < qlow) IShl->ix += 1;
            }
         else
            IShl->ix = 0;
         if (IShl->ix > IShl->ixmax) goto TryAnotherY;
         /* Check x bounds at inner radius */
         x2 = IShl->r1xy2 - q2;
         if (x2 >= 0) {
            qrad = sqrt(x2)/IShl->dx;
            iqmx = (long)(IShl->xc + qrad + 1.0);
            qlow = IShl->xc - qrad;
            iqmn = (long)qlow;
            if ((double)iqmn == qlow) iqmn -= 1;
            if (iqmx - iqmn > 1) {
               if (iqmn < IShl->ix) {
                  if (iqmx > IShl->ixmax) goto TryAnotherY;
                  IShl->ix = iqmx;
                  }
               else if (iqmx > IShl->ixmax) {
                  IShl->ixmax = iqmn;
                  }
               else {
                  IShl->ix2hi = IShl->ixmax;
                  IShl->ix2lo = iqmx;
                  IShl->ixmax = iqmn;
                  goto UsingInnerCut;
                  }
               }
            }
         /* X can be scanned in a single range, no inner cut */
         IShl->ix2lo = 1;
         IShl->ix2hi = 0;
         } /* End y update */
      } /* End update at end of x range */

/* X will be scanned in two ranges */
UsingInnerCut:

   /* New x, calculate lattice offset and return */
   IShl->ioff = IShl->iofrow + IShl->ix;
   return TRUE;
   } /* End GetNextPointInShell() */
