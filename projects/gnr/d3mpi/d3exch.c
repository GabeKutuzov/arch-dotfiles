/* (c) Copyright 1989-2016, The Rockefeller University *21115* */
/* $Id: d3exch.c 70 2017-01-16 19:27:55Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                 D3EXCH - Exchange s(i) among nodes                   *
*                                                                      *
*  Purpose:  After parallel calculation of s(i), each node contains    *
*     only the data for cells stored on that node.  After d3exch       *
*     is executed, each node has a copy of all the data (for the       *
*     current cycle only--data in the delay pipe are not exchanged).   *
*     (This includes nodes that do not participate in the calculation  *
*     of s(i)--currently only host node, but eventually controlled by  *
*     il->node1 and il->nodes variables.)                              *
*                                                                      *
*  MPI version:  Previous code sensitive to hypercube vs ring topology *
*     has been removed.  Given fast switch architecture, an algorithm  *
*     that works like a subset of a hypercube should always be good.   *
*     At some point we should experiment with gather algorithms        *
*     provided as built-in MPI functions.                              *
*                                                                      *
*  Synopsis:  void d3exch(struct CELLTYPE *il)                         *
*     il      is a pointer to the celltype to be exchanged.            *
************************************************************************
*  V2A, 09/05/89, G.N. Reeke - Based on benchmark routine written      *
*                       by Matthew Hall of NCUBE                       *
*  V2C, 01/29/91,  MC - Double message length for phased celltypes     *
*  Rev, 05/20/92, ABP - Add stattmr() calls                            *
*  Rev, 12/09/92, ABP - Add HYB code                                   *
*  V6C, 08/14/93, GNR - Generalize for arbitrary range of nodes        *
*  Rev, 10/20/93, GNR - Avoid RING lock up--read segs in arrival order *
*  V6D, 02/08/94, GNR - Revise for axonal delays                       *
*  V7A, 04/20/94, GNR - Revise to work with one comp node              *
*  V8A, 11/27/96, GNR - Remove support for non-hybrid version          *
*  Rev, 03/01/97, GNR - Remove second argument, always load host       *
*  V8C, 02/27/03, GNR - Cell responses in millivolts, add conductances *
*  ==>, 02/17/05, GNR - Last mod before committing to svn repository   *
*  R67, 06/22/16, GNR - Remove topology specificity, allow numbers of  *
*                       nodes which may not be powers of two.          *
*  Rev, 09/26/16, GNR - Just compute for cluster starting at il->node1 *
***********************************************************************/

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"

/* Macro for computing locell of a cluster:
*
*  Arg:  nn = relative node within set assigned to this celltype.
*  nn*cpn   = (relative node * cells per node [before the remainders
*     are distributed]).  This counts the total number of cells on all
*     previous nodes, not including the remainder cells.  These are
*     counted by the min term.  If the node is above those that get
*     a remainder cell, then il->crn <= nn and we add il->crn cells.
*     Otherwise, we're on a node that gets a remainder, and we add
*     nn cells.  */

#define low(nn) (nn = min(nn,il->nodes), \
   nn*il->cpn + min(nn,il->crn))

/***********************************************************************
*                              d3exch()                                *
***********************************************************************/

void d3exch(struct CELLTYPE *il) {

   char *ps = (char *)il->pps[0];   /* Locate current s(i) row */
#ifdef PAR0
   int src = il->node1;
#else
#ifndef USE_MPI
   struct NNSTR nnio;         /* Stream for nnput/nnget */
#endif
   int mynode = NC.node - il->node1;
   int chan;
   int isend,jsend;           /* Low, high cells of cluster */
   int nsend,nrecv,next;      /* Sending, receiving nodes */
   int oclust,lclust;         /* Offset, length of cluster */
   int tnn;                   /* Assignable arg for low(nn) */
#endif
   int type = UPDATE_SI_MSG;
   int ispt = uwlod(il->wspt);
#ifdef USE_MPI
   int rc;                    /* Return code from MPI call */
#endif

   stattmr(OP_PUSH_TMR, COMM_TMR);

#ifdef PAR0

   /* Receive the s(i),phase data */
#ifdef USE_MPI
   rc = MPI_Recv(ps, ispt, MPI_UNSIGNED_CHAR, src, type,
      NC.commc, MPI_STATUS_IGNORE);
   if (rc) d3exit("d3exch: MPI_Recv error", NREAD_ERR, rc);
#else
   anread(ps, ispt, &src, &type, "S(I) DATA");
#endif

#else    /* I am a comp node */

   /* Move contribution from this node into ps array */
   memcpy(ps + spsize(il->locell,il->phshft),
      (char *)il->ps2, spsize(il->mcells,il->phshft) );

/* Assemble a copy of all the data on base node.  The hypercube
*  algorithm fails when the number of nodes is not a power of
*  two.  Here we collect all the data to relative node 0, then
*  broadcast it back in 2*log2(N) passes.  */

   for (chan=1; chan<il->nodes; chan<<=1) {
      /* Nodes that already sent data drop out of exchange */
      if (mynode & (chan-1)) continue;
      nrecv = mynode & ~chan; /* Receiving node for this round */
      nsend = mynode | chan;  /* Sending node for this round */
      /* If sending node does not exist, skip this round */
      if (nsend >= il->nodes) continue;
      isend = low(nsend);     /* Get low icell of sent cluster */
      tnn = nsend + chan;     /* Get top node of sent cluster */
      jsend = low(tnn);       /* Get high icell of sent cluster */
      /* Get size of cluster.  N.B. low macro imposes arrau top. */
      oclust = spsize(isend,il->phshft);
      lclust = spsize(jsend,il->phshft) - oclust;
      if (mynode == nsend) {  /* Sending node sends */
         next = nrecv + il->node1;
#ifdef USE_MPI
         rc = MPI_Send(ps+oclust, lclust, MPI_UNSIGNED_CHAR,
            next, type, NC.commc);
         if (rc) d3exit("d3exch: MPI_Send error", NWRITE_ERR, rc);
#else
         nnpcr(&nnio, next, type);
         nnput(&nnio, ps+oclust, lclust);
         nnpcl(&nnio);
#endif
         }
      else {                  /* Receiving node receives */
         next = nsend + il->node1;
#ifdef USE_MPI
         rc = MPI_Recv(ps+oclust, lclust, MPI_UNSIGNED_CHAR,
            next, type, NC.commc, MPI_STATUS_IGNORE);
         if (rc) d3exit("d3exch: MPI_Recv error", NREAD_ERR, rc);
#else
         nngcr(&nnio, next, type);
         nnget(&nnio, ps+oclust, lclust);
         nngcl(&nnio);
#endif
         }
      } /* End collection loop over channels */

/* Now broadcast full data back to all nodes.  We could use a
*  broadcast call here, but for symmetry, the algorithm is
*  fully written out.  */

   for (chan>>=1; chan; chan>>=1) {
      /* Nodes that are not ready to receive data skip this round */
      if (mynode & (chan-1)) continue;
      nrecv = mynode | chan;  /* Receiving node for this round */
      nsend = mynode & ~chan; /* Sending node for this round */
      /* If receiving node does not exist, skip this round */
      if (nrecv >= il->nodes) continue;
      if (mynode == nsend) {  /* Sending node sends */
         next = nrecv + il->node1;
#ifdef USE_MPI
         rc = MPI_Send(ps, ispt, MPI_UNSIGNED_CHAR,
            next, type, NC.commc);
         if (rc) d3exit("d3exch: MPI_Send error", NWRITE_ERR, rc);
#else
         nnpcr(&nnio, next, type);
         nnput(&nnio, ps, ispt);
         nnpcl(&nnio);
#endif
         }
      else {                  /* Receiving node receives */
         next = nsend + il->node1;
#ifdef USE_MPI
         rc = MPI_Recv(ps, ispt, MPI_UNSIGNED_CHAR,
            next, type, NC.commc, MPI_STATUS_IGNORE);
         if (rc) d3exit("d3exch: MPI_Recv error", NREAD_ERR, rc);
#else
         nngcr(&nnio, next, type);
         nnget(&nnio, ps, ispt);
         nngcl(&nnio);
#endif
         }
      } /* End distribution loop over channels */

/* Head node returns updated s(i) data to host */

   if (mynode == 0) {
#ifdef USE_MPI
      rc = MPI_Send(ps, ispt, MPI_UNSIGNED_CHAR, NC.hostid, type,
         NC.commc);
      if (rc) d3exit("d3exch: MPI_Send error", NWRITE_ERR, rc);
#else
      anwrite(ps, ispt, NC.hostid, type, "S(I) DATA");
#endif
      }

#endif /* Not PAR0 */

   stattmr(OP_POP_TMR, COMM_TMR);
   return;
   } /* End d3exch() */


