/* (c) Copyright 2017, The Rockefeller University *21115* */
/* $Id: d3lkni.c 78 2018-08-02 18:36:58Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3lkni                                 *
*                                                                      *
*  This file contains d3lkni, a routine used to compute setup data     *
*  for connection-generating kernels implementing the KGEN=K option.   *
*  It has two modes of operation:                                      *
*     If argument plkn is set, d3lkni() just determines how many       *
*  source cells are touched by the kernel for the connection type      *
*  passed as argument and how much space will be required to store     *
*  this kernel prototype.  The space requirement is returned in the    *
*  location pointed to by plkn.                                        *
*     If plkn is NULL, d3lkni() repeats the exact same calculation     *
*  but also stores the results at ix->ul2.k.pkern data for use by      *
*  d3lij.  By explicit definition, rows are in order low y to high y,  *
*  angles are in radians, counterclockwise up from positive x axis.    *
*                                                                      *
*  Synopsis:                                                           *
*     si32 d3lkni(struct CONNTYPE *ix, ui32 *plkn)                     *
*                                                                      *
*  Arguments:                                                          *
*     ix       Pointer to connection type for this calculation.        *
*     plkn     Pointer to location where total memory needed to store  *
*              kernel data is returned and mode of calculation as      *
*              described above.                                        *
*                                                                      *
*  Parameters assumed set by user and checked or defaulted in d3tchk:  *
*     nel      Number of cells/group = number of divisions of pi       *
*              angular range of kernel.  It is explicitly NOT required *
*              that source nel == target nel.                          *
*     nrx,nry  Size of kernel in cols, rows of groups.                 *
*     tcwid    Full width at half max in degrees.                      *
*     hradius  Exponential falloff radius.                             *
*     cmin     Minimum acceptable Cij value (cijmin in code).          *
*     stride   Row, column stride, >1 to reduce kernel size.           *
*                                                                      *
*  Return values:                                                      *
*     Function value:  Maximum over target orientations of total num-  *
*  ber of connections in kernel (nc).  These numbers should be similar *
*  for all angles, but all cells need to have same nc in our setup.    *
*     The data structure contains all information needed to generate   *
*  the Lij and also the Cij values, which can be picked up in d3genr   *
*  and copied to all the cells.  This is because most of the work is   *
*  already done just finding out which orientations are touched in     *
*  each cell group.                                                    *
*     Sets the ix->cnflgs UNIKQ bit.                                   *
*                                                                      *
*  The initial count must be done from d3tchk, where the final value   *
*  of nc is supposed to be established, but the actual calculation of  *
*  the kernel tables has to be done later, after the storage has been  *
*  allocated.  This is currently in d3genr(), duplicated on all nodes. *
*                                                                      *
*  The output data structure is as follows:                            *
*  There is a table 'ptori' of pointers of type *kern with one pointer *
*     for each target cell orientation to the start of a table 'pkern' *
*     with elements that are structs of type 'kern'.                   *
*  There is one kern for each source contact made by the kernel.  This *
*     contains 'kx' and 'ky', the x and y coordinates of the source    *
*     cell relative to the target at the center of the kernel, 'ori',  *
*     the index of the orientation of the source cell, and 'Cij', the  *
*     initial connection strength.                                     *
*  Following the last connection, there is an extra kern struct with   *
*     ori = -1.  This is used by the lijbr routines to detect it is    *
*     at the end of the list.                                          *
*                                                                      *
*  This code is written with some anticipation that if there is need   *
*  to implement different kernel geometries, this can be done by       *
*  adding a suitable switch on the CONNTYPE card that can be interro-  *
*  gated here.                                                         *
************************************************************************
*  R72, 02/06/17, GNR - New routine, just circles and ellipses for now.*
*  ==>, 03/05/17, GNR - Last mod before committing to svn repository   *
*  R74, 07/13/17, GNR - Add exponential falloff factor                 *
*  R75, 10/11/17, GNR - Bug fix:  cfwhm not reducing absarg to < pi/2  *
*  R76, 10/18/17, GNR - Do not emit negative Cij when KNOPT=I is on    *
*  R78, 03/15/18, GNR - Allow any KGEN 1st conn code with KGNKN kernel *
***********************************************************************/

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#ifndef PARn
#include "rocks.h"
#endif
#include "rkarith.h"
#include "d3global.h"
#include "celldata.h"

extern struct CELLDATA CDAT;  /* For picking up il if needed */

/*=====================================================================*
*                                cfwhm                                 *
*                                                                      *
*  This function implements the cosine formula, Equation 9 in the      *
*  Piech, Li, Reeke, and Gilbert paper, PNAS E4108 (2013).             *
*=====================================================================*/

static double cfwhm(double phi, double phiopt, double phifwhm) {

   double adphi = fabs(phi - phiopt);
   /* VP takes mod(adphi+PIOV2,PI) - PIOV2 here--mod not needed
   *  because arguments are known to be in range (-PIOV2,+PIOV2)  */
   if (adphi > PIOV2) adphi = fabs(adphi - PI);
   if (adphi >= phifwhm) return 0.0;
   else return 0.5*(1.0 + cos(PI*adphi/phifwhm));

   } /* End cfwhm() */


/*=====================================================================*
*                               d3lkni                                 *
*                                                                      *
*  As in d3inhi, we determine the exact number of connections needed,  *
*  not just an approximation.  This is done here for the two initial   *
*  cases of circles or ellipses, actually using the same code for both.*
*  The self connection at the origin is always skipped because it has  *
*  an indeterminate source-target vector.                              *
*                                                                      *
*  For the circle and ellipse geometries so far implemented, kernels   *
*  are symmetric, but the entire kernel spec is generated because the  *
*  bookkeeping gets complicated to reverse the orientation indices in  *
*  the lower half.                                                     *
*                                                                      *
*  All counts here are in kerns until plkn result is returned.         *
*=====================================================================*/

si32 d3lkni(struct CONNTYPE *ix, ui32 *plkn) {

   struct CELLTYPE *il = CDAT.cdlayr;
   /* pk used on PAR0 only if printing, no need to test here */
   kern *pk = ix->ul2.k.pkern;
   double dfwhm,dnsor,dntor,dopt,dd,ds2,dx,dx2,dy,dy2,duy2,erat,ewt;
   double dxex,dyex;             /* Expanded kernel dimensions */
   si32 ia,ja;
   si32 jx, ixhi, jy, iyhi, nnc, mxnnc = 0;
   si32 bigc15 = ix->bigc16>>1;  /* Someday get rid of this mess */
   si32 cijminu = ix->cijmin >> Ss2hi;    /* cijmin to S15 */
   ui32 tnnc = 0;
   int  ksto = !plkn;            /* TRUE to store data */
#ifndef PARn
   int dolist = ksto && ix->knopt & KNO_PRINT &&
      RP->CP.runflags & RP_OKPRIN;
   if (dolist) convrt("(P4,'0Listing of kgen=K kernel for ',J0A20,"
      "', conntype ',J0UH6)", fmtlrlnm(il), &ix->ict, NULL);
#endif
#ifdef PAR0
   ksto &= dolist;
#endif

   /* Expand the size of the figure by adding 1/2 to each axis, then
   *  can just use points exactly inside the expanded figure.  Note
   *  that nrx,y are diameters, we need radii here */
   dxex = 0.5*(double)(ix->nrx+1), dyex = 0.5*(double)(ix->nry+1);
   dfwhm = DEGS_TO_RADS * ix->Cn.tcwid;
   dopt = (ix->knopt & KNO_INHIB) ? PIOV2 : 0;
   dnsor = (double)ix->srcnel;
   dntor = (double)il->nel;
   {  double ds = (double)ix->hradius/dS16;
      ds2 = 2*ds*ds; }
   /* Loop over nel target orientations */
   for (ia=0; ia<il->nel; ++ia) {
      double torr,sintor,costor; /* Target orientation info */
      if (ksto)
         ix->ul2.k.ptori[ia] = pk;
      torr = PI * (double)ia/dntor;
      sintor = sin(torr), costor = cos(torr);

      /* Count number of points on each row.  This calculation ignores
      *  mirror axes in order to get the low-to-high order right.  */
      duy2 = dyex*dyex;
      erat = dxex/dyex;
      iyhi = (si32)(ix->nry >> 1);
      nnc = 0;
      /* Loop over rows */
      for (jy=-iyhi; jy<=iyhi; jy+=ix->stride) {
         dy = (double)jy; dy2 = dy*dy;
         dx = (dy2 >= duy2) ? 0 : erat*sqrt(duy2 - dy2);
         ixhi = (si32)dx;

         /* Loop over cols in this row */
         for (jx=-ixhi; jx<=ixhi; jx+=ix->stride) {
            double asa,th1,th2;
            dx = (double)jx; dx2 = dx*dx;
            dd = dx2 + dy2;
            if (dd == 0) continue;
            ewt = exp(-dd/ds2);
            dd = sqrt(dd);
            /* Angle between target orientation and kernel vector */
            asa = (sintor*dx - costor*dy)/dd;
            /* Pick orientation that is along source->target line */
            if (dx*costor + dy*sintor < 0) asa = -asa;
            if (fabs(asa) >= 1.0) th1 = (asa > 0) ? PIOV2 : -PIOV2;
            else                  th1 = asin(asa);

            /* Loop over nel source orientations */
            for (ja=0; ja<ix->srcnel; ++ja) {
               double dCij,sorr,sinsor,cossor;
               si32 Cij;
               sorr = PI * (double)ja/dnsor;
               sinsor = sin(sorr), cossor = cos(sorr);
               asa = (sinsor*dx - cossor*dy)/dd;
               /* Pick orientation that is along source->target line */
               if (dx*cossor + dy*sinsor < 0) asa = -asa;
               if (fabs(asa) >= 1.0) th2 = (asa > 0) ? PIOV2 : -PIOV2;
               else                  th2 = asin(asa);
               /* We have to compute Cij whether or not it is being
               *  stored at this time in order to carry out the cijmin
               *  test.  No need to test |Cij|, cfwhm always returns
               *  positive Cij.  */
               dCij = cfwhm(-th2, th1, dfwhm);
               if (dCij == 0) continue;
               dCij *= cfwhm(0.5*(fabs(th1) + fabs(th2)), dopt, dfwhm);
               dCij *= ewt;
               Cij = (si32)(dS15*dCij);
               if (Cij < cijminu) continue;
               if (Cij > bigc15) Cij = bigc15;
               nnc += 1;      /* Got a keeper */
               if (ksto) {
                  pk->kx = jx, pk->ky = jy;
                  pk->ori = ja;
                  pk->Cij = Cij;
                  ++pk;
                  }
               } /* End loop over source orientations */
            } /* End loop over columns */
         } /* End loop over rows */
#ifndef PARn
      if (nnc > (1 << IBnc) - nnc) {
         d3lime("d3lkni", PLcb(PLC_KERN)+ix->ict);
         nnc = 1 << IBnc; }
      if (dolist) {
         si32 adeg = (180<<16)*ia/il->nel;
         int kdim = 4*nnc;
         convrt("(P4,' Elements for orientation',IJ4,' (',J0B16IJ8.4,"
            "' deg)')", &ia, &adeg, NULL);
         cryout(RK_P4,"     x    y ori     Cij      x    y ori     Cij"
            "      x    y ori     Cij      x    y ori     Cij",
            RK_NFSUBTTL+96, NULL);
         convrt("(P4,#,4(IH7IH5IH4,B15IH8.4))", &kdim,
            ix->ul2.k.ptori[ia], NULL);
         cryout(RK_P4, "    ", RK_NTSUBTTL+4, NULL);
         }
#endif
      if (nnc > mxnnc) mxnnc = nnc;
      /* Add another kern with ori = -1 to stop l(ij) gen */
      if (ksto) {
         pk->kx = pk->ky = pk->Cij = 0;
         pk->ori = -1;
         ++pk;
         }
      tnnc += nnc + 1;
      } /* End loop over target orientations */

   /* Add space for kernel index by target angle */
   if (plkn) {
      *plkn = tnnc*sizeof(kern) + il->nel*sizeof(kern *);
      }
#ifndef PARn
   if (dolist) convrt("(P4,' There are a grand total of ',J1UJ8,"
      "'connections in this kernel')", &tnnc, NULL);
#endif
   ix->cnflgs |= UNIKQ;
   return mxnnc;
   }  /* End of d3lkni() */

