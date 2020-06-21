/* (c) Copyright 1989-2016, The Rockefeller University *11116* */
/* $Id: d3tsvp.c 72 2017-04-14 19:11:21Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3tsvp                                 *
*                 Verify tree structure for debugging                  *
*                                                                      *
*  Synopsis: void d3tsvp(int kblk)                                     *
*                                                                      *
*  Argument:                                                           *
*     kblk     Sum of following switch values:                         *
*                 1  to print specific conntype block locations        *
*                 2  to print geometric conntype block locations       *
*                 4  to print modulatory conntype block locations      *
*                 8  to check for bad pointers without printing        *
*                                                                      *
*  This program prints the memory location of each repertoire and      *
*  celltype block along with lower-level block selected by 'kblk'.     *
*  For each block, one or two key variables are also printed to help   *
*  the user see at a glance whether memory has been corrupted.         *
*                                                                      *
*  Warning: This code uses ssprintf/dbgprt on comp nodes, convrt on    *
*           host and PAR0 nodes.                                       *
************************************************************************
*  V4A, 03/01/89, Translated to 'C' from version V2H                   *
*  V6C, 09/22/93, GNR - Add comp node code, GCONNs and MODULATE blocks *
*  V8A, 03/01/97, GNR - ps2 no longer defined on PAR0 node             *
*  Rev, 09/22/98, GNR - Use ssprintf instead of sprintf                *
*  Rev, 08/18/01, GNR - Use short, not long, repertoire names          *
*  ==>, 01/31/07, GNR - Last mod before committing to svn repository   *
*  R67, 05/03/16, GNR - mpitools call args regularized                 *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "d3global.h"
#include "rocks.h"
#include "rkxtra.h"

void d3tsvp(int kblk) {

   struct REPBLOCK *ir;
   struct CELLTYPE *il;
   struct CONNTYPE *ix;
   struct INHIBBLK *ib;
   struct MODBY    *im;
   int ict;                /* Connection type counter */

/* Set subtitle for printing tree structure */

#ifndef PARn
   cryout(RK_P2,"0TREE STRUCTURE:",RK_SUBTTL + RK_LN2,NULL);
#else
   dbgprt("TREE STRUCTURE");
#endif

/* Loop over all repertoires, printing repblock locations */

   for (ir=RP->tree; ir; ir=ir->pnrp) {

#ifndef PARn
      convrt("(YP2,'0Region ',J1A32H(J0A" qLSN
         "H),' block at ',Z8,' has ',J1UH6,'layers')",
         getrktxt(ir->hrname),ir->sname,&ir,&ir->nlyr,NULL);
#else
      dbgprt("  ");
      dbgprt(ssprintf(NULL, "Region <%" qLSN
         "s> block at %p has %6d layers",
         ir->sname,ir,(int)ir->nlyr));
#endif

/* Loop over cell types, printing celltype and data locations */

      for (il=ir->play1; il; il=il->play) {

#ifndef PAR
         convrt("(YP2,3X,'Celltype ',J1A" qLSN ",' block at ',Z8,"
            "' has data for ',J1I10,'cells at ',Z8,"
            "', s(t,t-1) at ',Z8,2H, Z8)",
            il->lname,&il,&il->nelt,&il->prd,il->pps,&il->ps2,NULL);
#endif
#ifdef PAR0
         convrt("(YP2,3X,'Celltype ',J1A" qLSN ",' block at ',Z8,"
            "' has data for ',J1I10,'cells at ',Z8,"
            "', s(t) at ',Z8)",
            il->lname,&il,&il->nelt,&il->prd,il->pps,NULL);
#endif
#ifdef PARn
         dbgprt(ssprintf(NULL,"  Celltype %" qLSN "s block at %p "
            "has data for %6d cells at %p, s(t,t-1) at %p, %p",
            il->lname,il,il->mcells,il->prd,il->pps,il->ps2));
#endif

/* Print info for specific connection types if requested */

         if (kblk & 1)
               for (ict=1,ix=il->pct1; ix; ix=ix->pct,ict++) {

#ifndef PARn
            convrt("(YP2,5X,'Conntype   ',J1I4,'from (',J0A" qLSN
               "H,J0A" qLSN "') block at ',Z8,' has ',J1UJ10,"
               "'conns/cell')",
               &ict,ix->srcnm.rnm,ix->srcnm.lnm,&ix,&ix->nc,NULL);
#else
            if (!(kblk & 8)) {
               dbgprt(ssprintf(NULL,"    Conntype   %4d from (%"
                  qLSN "s,%" qLSN "s) block at %p has %6jd conns/cell",
                  ict,ix->srcnm.rnm,ix->srcnm.lnm,ix,ix->nc));
               }
#endif
            } /* End ix loop */

/* Print info for geometric connection types if requested */

         if (kblk & 2)
               for (ict=1,ib=il->pib1; ib; ib=ib->pib,ict++) {

#ifndef PARn
            convrt("YP2,5X,'GCONN type ',J1I4,'from (',J0A" qLSN
               "H,J0A" qLSN "') block at ',Z8,' has ',J1UH4,'bands')",
               &ict,ib->gsrcnm.rnm,ib->gsrcnm.lnm,&ib,&ib->nib,NULL);
#else
            if (!(kblk & 8)) {
               dbgprt(ssprintf(NULL,"    GCONN type %4d from (%"
                  qLSN "s,%" qLSN "s) block at %p has %4d bands",
                  ict,ib->gsrcnm.rnm,ib->gsrcnm.lnm,ib,(int)ib->nib));
               }
#endif
            }

/* Print info for modulatory connection types if requested */

         if (kblk & 4) {
            for (ict=1,im=il->pmby1; im; im=im->pmby,ict++) {

#ifndef PARn
               convrt("YP2,5X,'Modulation ',J1I4,'from (',J0A" qLSN
                  "H,J0A" qLSN "') block at ',Z8,' has modval at ',Z8)",
                  &ict, im->msrcnm.rnm, im->msrcnm.lnm, &im, &im->pmvb,
                  NULL);
#else
               if (!(kblk & 8)) {
                  dbgprt(ssprintf(NULL,"    Modulation %4d from (%" qLSN
                     "s,%" qLSN "s) block at %p has modval at %p",ict,
                     im->msrcnm.rnm,im->msrcnm.lnm,im,im->pmvb));
                  }
#endif
               }
            }

         } /* End il loop */
      } /* End ir loop */
   } /* End of d3tsvp() */
