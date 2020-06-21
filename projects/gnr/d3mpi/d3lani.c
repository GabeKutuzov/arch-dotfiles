/* (c) Copyright 2017, The Rockefeller University *21115* */
/* $Id: d3lani.c 75 2017-10-13 19:38:52Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3lani                                 *
*                                                                      *
*  This file contains d3lani, a routine used to compute setup data     *
*  for annular connections implementing the revised KGEN=Q option.     *
*  It has two modes of operation:                                      *
*     If argument pann is set, d3lani() just determines how many       *
*  source cells are touched by the annulus for the connection type     *
*  passed as argument and therefore how much space will be required    *
*  to store the annulus prototype table.                               *
*     If pann is NULL, d3lani() repeats the exact same calculation     *
*  but also stores the results in the ix->ul2.q.pann data for use by   *
*  d3lij.  By explicit definition, rows are in order low y to high y,  *
*  then increasing x.                                                  *
*                                                                      *
*  Synopsis:                                                           *
*     si32 d3lani(struct CONNTYPE *ix, ui32 *pann)                     *
*                                                                      *
*  Arguments:                                                          *
*     ix       Pointer to connection type for this calculation.        *
*     pann     Pointer to location where number of entries in the      *
*              annulus connection list is returned, and sets mode of   *
*              operation as described above.                           *
*                                                                      *
*  Parameters assumed set by user and checked or defaulted in d3tchk:  *
*     nrx,nry  Exterior size of annulus in cols, rows of groups.       *
*     nhx,nhy  Size of interior hole in cols, rows of groups.          *
*                                                                      *
*  Return values:                                                      *
*     Function value:  Total number of source groups in annulus.  It   *
*  is expected that the lb2q routine will pick one of these at random, *
*  then, if srcnel > 1, pick a cell at random from within this group.  *
*     The data structure returned contains just a list of group x,y    *
*  coordinates for groups contained in the area between the ellipses   *
*  specified by nrx,nry (outside) and nhx,nhy (interior hole).         *
*     Sets the ix->cnflgs UNIKQ bit.                                   *
*                                                                      *
*  The initial count must be done from d3tchk, where the final value   *
*  of nc is supposed to be established, but the actual calculation of  *
*  the annulus tables has to be done later, after the storage has been *
*  allocated.  This is currently in d3genr(), duplicated on all nodes. *
*                                                                      *
*  The output data structure is as follows:                            *
*  The pointer at ix->ul2.q.paxy points to a table of xysh structs,    *
*     each of which contains a short x,y pair.  The length of the      *
*     table is the size of an xysh structure times the number of       *
*     entries returned by the pann != 0 call.                          *
************************************************************************
*  R73, 04/16/17, GNR - New routine, just circles and ellipses for now.*
*  ==>, 04/24/17, GNR - Last mod before committing to svn repository   *
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

#define DBG_PRT_XYMAP   /* Defind this to print the list */
#if defined(DBG_PRT_XYMAP) && !defined(PAR)
#include "celldata.h"
extern struct CELLDATA CDAT;
#endif

/*=====================================================================*
*                               d3lani                                 *
*                                                                      *
*  As in d3inhi, we determine the exact number of connections needed,  *
*  not just an approximation.  This is done here for the area betwen   *
*  two ellipses, with principal axes along x and y.                    *
*                                                                      *
*  For the geometry so far implemented, annuli are symmetric, but the  *
*  entire point list is generated to avoid the need for the l(ij)      *
*  generating routine to half the probability of selecting points      *
*  on the axes.                                                        *
*                                                                      *
*  Unlike d3lkni, length returned is in xysh counts, not bytes, and    *
*  there is no provision to print the list on a parallel computer.     *
*=====================================================================*/

si32 d3lani(struct CONNTYPE *ix, si32 *pann) {

   xysh *pxy = ix->ul2.q.paxy;
   double drx,dry,dhx,dhy;
   double rratio,hratio,dry2,dhy2;
   si32 nnc = 0;                    /* Connection count */
   int iy,iyhi;
#ifdef PAR0
   int ksto = FALSE;
#else
   int ksto = !pann;                /* TRUE to store data */
#endif
#if defined(DBG_PRT_XYMAP) && !defined(PAR)
   xysh *dbgxy = pxy;
   si32 ndbg;
   if (ksto) {
      struct CELLTYPE *il = CDAT.cdlayr;
      if (ksto) convrt("(P4,'0Listing of kgen=Q map for ',J0A20,"
         "', conntype ',J0UH6)", fmtlrlnm(il), &ix->ict, NULL);
      }
#endif

   /* Note that n[rh][xy] are diameters, we need radii here */
   drx = 0.5*(double)ix->nrx, dry = 0.5*(double)ix->nry;
   dhx = 0.5*(double)ix->nhx, dhy = 0.5*(double)ix->nhy;
   rratio = drx/dry, hratio = dhy != 0 ? dhx/dhy : 0;
   dry2 = dry*dry, dhy2 = dhy*dhy;
   iyhi = (int)dry;
   for (iy=-iyhi; iy<=iyhi; ++iy) {
      /* This y always intersects the nrx,nry ellipse */
      double y, y2;
      int jx, jxr, jxh;
      y = (double)iy, y2 = y*y;
      jxr = (int)(rratio*sqrt(dry2 - y2));
      if (fabs(y) > dhy) {
         /* Setup for only one x loop */
         jxh = (int)ix->nrx + 1;
         }
      else {
         jxh = (int)(hratio*sqrt(dhy2 - y2));
         if (jxh >= jxr) continue;     /* Hole is outside ellipse */
         }

      /* Loop over two possible intervals */
#if defined(DBG_PRT_XYMAP) && !defined(PAR)
      ndbg = nnc;
#endif
      for (jx=-jxr; jx<=jxr; ++jx) {
         /* Skip the hole when it is reached */
         if (jx == -jxh) jx = jxh + 1;
         nnc += 1;                     /* Got a keeper */
         if (ksto) {
            pxy->sx = (short)jx, pxy->sy = (short)iy;
            ++pxy;
            }
         }  /* End x loop */
#if defined(DBG_PRT_XYMAP) && !defined(PAR)
      if (ksto) {
         ndbg = nnc - ndbg;
         convrt("(P4,' X values for y = ',J0I6)", &iy, NULL);
         convrt("(P4,#<^4,12IH6>)", &ndbg, dbgxy, NULL);
         dbgxy += ndbg;
         } 
#endif
      }  /* End y loop */

#ifndef PARn
   if (nnc > (1 << IBnc) - nnc) {
      d3lime("d3lani", PLcb(PLC_KERN)+ix->ict);
      nnc = 1 << IBnc; }
#endif

   if (pann) {
      *pann = nnc;
      }
   ix->cnflgs |= UNIKQ;
   return nnc;
   }  /* End of d3lani() */

