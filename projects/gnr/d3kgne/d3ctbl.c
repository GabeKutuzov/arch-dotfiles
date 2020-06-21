/* (c) Copyright 2005, Neurosciences Research Foundation, Inc. */
/* $Id: d3ctbl.c 22 2009-05-12 21:22:26Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3ctbl                                 *
*                                                                      *
*  Calculate and store activation tables for gated ion channels.       *
*  Return total length of these tables.                                *
*                                                                      *
*  Argument:                                                           *
*     mktbls      =0  Just calculate length, do not store tables.      *
*                     (Called by d3cchk to provide cumcnd for d3echo)  *
*                 =1  Calculate length and make tables.                *
*                     (Called at Group III time from node 0 only)      *
*  (This routine may be skipped if there are no gated conductances.)   *
************************************************************************
*  V8D, 07/12/05, GNR - New routine                                    *
*  ==>, 08/03/07, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sysdef.h"
#include "d3global.h"
#include "rksubs.h"

struct GCTBLCI {                 /* Activation table consolidation */
   struct CONDUCT *pbc;          /* Ptr to prototype activation data */
   long   ogat;                  /* Offset to this table */
   };

/*=====================================================================*
*                               d3ctbl                                 *
*=====================================================================*/

long d3ctbl(int mktbls) {

   struct CELLTYPE *il;
   struct CONDUCT  *pcnd,*qcnd;
   struct GCTBLCI  *pgc0;
   long   ngatbl;                /* Total length of gca tables */
   int    igci,ngciu;            /* GCTBLCI index, number used */
   int    igat;

/* Make a preliminary pass through all the gated conductances to
*  determine how much space is needed for unique activation tables.
*  (There aren't too many of these, so a linear search will do.)  */

   if (!RP0->orcdfl & (CDFL_GNDCY|CDFL_GgDCY)) return 0; /* JIC */

   /* Make enough GCTBLCI blocks for all gated conductances */
   pgc0 = mallocv(RP->ncndg*sizeof(struct GCTBLCI),
            "Activation table info");
   ngciu = 0;
   for (il=RP->pfct; il; il=il->pnct) {
      for (pcnd=il->pcnd1; pcnd; pcnd=pcnd->pncd)
            if (pcnd->cdflgs & (CDFL_GNDCY|CDFL_GgDCY)) {
         for (igci=0; igci<ngciu; ++igci) {
            qcnd = pgc0[igci].pbc;
            if ((pcnd->kcond == qcnd->kcond) &&
                ((qcnd->kcond == CDTP_ALPHA &&
                  qcnd->ugt.alf.ttpk == pcnd->ugt.alf.ttpk &&
                  qcnd->ugt.alf.taum == pcnd->ugt.alf.taum) ||
                 (qcnd->kcond == CDTP_DOUBLE &&
                  qcnd->ugt.dbl.taur == pcnd->ugt.dbl.taur &&
                  qcnd->ugt.dbl.taud == pcnd->ugt.dbl.taud))) {
               /* This conductance has same activation parameters
               *  as an earlier one, so set to build table based
               *  on the one that needs the longer decay time.  */
               if (pcnd->nc2dc > qcnd->nc2dc) pgc0[igci].pbc = pcnd;
               pcnd->ogcat = igci;
               goto MatchNextConductance;
               }
            } /* End GCTBLCI scan */
         /* Match not found, make new GCTBLCI entry */
         pgc0[ngciu].pbc = pcnd;
         pcnd->ogcat = ngciu++;
MatchNextConductance: ;
         } /* End scan of gated conductances */
      } /* End celltype loop */

/* Determine total length of tables and allocate.
*  (If length changes, free and allocate again rather than
*  realloc because there is no need to copy the old contents.)  */

   for (igci=0,ngatbl=0L; igci<ngciu; ++igci) {
      pgc0[igci].ogat = ngatbl;
      ngatbl += pgc0[igci].pbc->nc2dc; }

   if (!mktbls) return ngatbl;

   if (ngatbl != RP->ngact) {
      if (RP->pgact) freepv(RP->pgact,
         "Conductance activation tables");
      RP->pgact = (gac_type *)allocpcv(MPS_Dynamic, ngatbl,
         IXgac_type, "Conductance activation tables");
      RP->ngact = ngatbl;
      }

/* Make the activation tables (S30).  These are needed regardless of
*  whether full evaluation or decay shortcut is selected.  Tables may
*  change at Group III time in the decay case, but not in the full
*  evaluation case (because of d3allo memory allocation).  */

   for (igci=0,ngatbl=0L; igci<ngciu; ++igci) {
      qcnd = pgc0[igci].pbc;
      switch (qcnd->kcond) {

      case CDTP_ALPHA: {
         double t,tpk = (dS20/1.0E3)*
            (double)RP->timestep/(double)qcnd->ugt.alf.ttpk;
         for (igat=0; igat<(int)qcnd->nc2dc; igat++) {
            t = (double)igat*tpk;
            RP->pgact[ngatbl++] = (gac_type)(S30*t*exp(1.0 - t));
            }} /* End time step loop and local scope */
         break;

      case CDTP_DOUBLE: {
         double tr = -(double)qcnd->ugt.dbl.taur;
         double td = -(double)qcnd->ugt.dbl.taud;
         double tc = td - tr;
         double lnrd = log(tr/td)/tc;
         double N = S30/(exp(tr*lnrd) - exp(td*lnrd));
         double t,ts = (double)RP->timestep/1.0E3;
         for (igat=0; igat<(int)qcnd->nc2dc; igat++) {
            t = (double)igat*ts;
            RP->pgact[ngatbl++] = (gac_type)(N*(exp(t/td)-exp(t/tr)));
            }} /* End time step loop and local scope */
         } /* End conductance type switch */
      } /* End loop over tables */

/* Store offset to appropriate table in each conductance block */

   for (il=RP->pfct; il; il=il->pnct) {

      /* Fix up conductances for this cell type */
      for (pcnd=il->pcnd1; pcnd; pcnd=pcnd->pncd) {

         /* Code above temporarily stored index of correct
         *  GCTBLCI in ogcat.  Now replace that value with
         *  the offset to the table relative to RP->pgact.  */
         pcnd->ogcat = pgc0[pcnd->ogcat].ogat;

         } /* End conductance loop */

      } /* End il loop */

   freev(pgc0, "Activation table info");
   RP0->n0flags &= ~N0F_MKTBLS;  /* Turn off table flag */

   return ngatbl;

   } /* End d3ctbl() */
