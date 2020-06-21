/* (c) Copyright 1991-2003, Neurosciences Research Foundation, Inc. */
/* $Id: d3exch.c 3 2008-03-11 19:50:00Z  $ */
/*---------------------------------------------------------------------*
*                             CNS Program                              *
*                 D3EXCH - Exchange s(i) among nodes                   *
*                                                                      *
*  Purpose:  After parallel calculation of s(i), each node contains    *
*     only the data for cells stored on that node.  After d3exch       *
*     is executed, each node has a copy of all the data (for the       *
*     current cycle only--data in the delay pipe are not exchanged).   *
*     (This includes nodes that do not participate in the calculation  *
*     of s(i)--currently only host node, but eventually controlled by  *
*     il->node1 and il->nodes variables.)  The algorithm used here is  *
*     sensitive to the topology of the parallel computer being used.   *
*     Versions for ring and hypercube topology are currently supplied. *
*                                                                      *
*  Synopsis:  void d3exch(struct CELLTYPE *il)                         *
*     il      is a pointer to the celltype to be exchanged.            *
*                                                                      *
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
*---------------------------------------------------------------------*/

/*    This routine now contains alternative versions for use with ring
*     or hypercube geometries.  The hypercube version carries out one
*     exchange step for each dimension of the hypercube.  Given chan =
*     1,2,4,...  total nodes and given a mask = ~(chan-1) which, when
*     applied to a node number, returns the low node of the cluster
*     for which the given node already has data, then at each step
*     each node exchanges data with node ^ chan.  It writes data
*     beginning with that of the (node & mask) node and extending up
*     to that of the ((node & mask) + chan) node.  It receives the
*     data sent by its partner into the bottom of the cluster in which
*     the partner is located, viz. the data of the (partner & mask)
*     node.
*        The loary used by MH to store the locell number of each
*     node has been eliminated in the present version, in favor of
*     computing the various locell numbers as needed.  These numbers
*     had to be recomputed for each new cell type anyway, in time
*     proportional to the number of nodes, rather than the log of the
*     number of nodes, so there really wasn't any savings.  The MH
*     routine has also been changed to compute the length of the data
*     to be received, rather than using 0x7fffffff.  This has two
*     benefits:  (1) it is needed to use nnget/nnput for handling
*     large numbers of cells, and (2) it lets us skip null messages
*     altogether.
*        Note that EXCHADDR is currently supported on rings, but not
*     on hypercubes.
*/

/* Include standard library functions */

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"

/* Macro for computing locell of a cluster:
*
*  nm1 = relative node within set assigned to this celltype.
*  nm1*cpn  = (relative node * cells per node [before the
*                      remainders are distributed]).
*  This counts the total number of cells on all previous
*  nodes, not including the remainder cells.  These are
*  counted by the min term.  If the node is above those
*  that get a remainder cell, then il->crn <= nm1 and we
*  add il->crn cells.  Otherwise, we're on a node that gets
*  a remainder, and we add nm1 cells.  */

#define low(n) (nm1 = (n)-il->node1, \
   nm1 = max(nm1,0), nm1 = min(nm1,il->nodes), \
   nm1*il->cpn + min(nm1,il->crn))

#ifdef RING

/* Macro for computing the number of cells on a given node.
*  This is done carefully to handle nodes that have no cells. */

#define cls(n) (low(n+1) - low(n))

#endif

/***********************************************************************
*                              d3exch()                                *
***********************************************************************/

void d3exch(struct CELLTYPE *il) {

   char *ps = (char *)il->pps[0];   /* Locate current s(i) row */
#ifdef PAR0
   int src = ANYNODE;
#else
   register int nm1;          /* Intermediate for low macro */
#ifdef HYPERCUBE
   int chan, mask, nnext;
   int nsend,isend,lsend;     /* Send node, data index, length */
   int nrecv,irecv,lrecv;     /* Recv node, data index, length */
   struct NNSTR nnio;         /* Stream for nnput/nnget */
#else
   char *pdb;                 /* Pointer to data buffer */
   long ltr;                  /* Length of data remaining to get */
   int len;                   /* Length of data in one piece */
   int src;
#endif
#endif
   int type = UPDATE_SI_MSG;

   stattmr(OP_PUSH_TMR, COMM_TMR);

#ifdef PAR0

   /* Receive the s(i) (phase) data */
   anread(ps, il->lspt, &src, &type, "S(I) DATA");

#else    /* I am a comp node */

   /* Move contribution from this node into ps array */
   memcpy(ps + spsize(il->locell,il->phshft),
      (char *)il->ps2, spsize(il->mcells,il->phshft) );

/* Assemble a copy of all the data on all nodes */

#ifdef HYPERCUBE

   for (chan=1; chan<NC.total; chan<<=1) {
      nnext = NC.node^chan;   /* Node I will send to now */
      mask = ~(chan-1);       /* Bit pattern = high bits set */

      nsend = NC.node&mask;   /* Bottom node of sent cluster */
      nrecv = nnext&mask;     /* Bottom node of rcvd cluster */

      isend = low(nsend);     /* Get locell of sent cluster */
      irecv = low(nrecv);     /* Get locell of rcvd cluster */

      lsend = low(nsend+chan) - isend; /* Length sent */
      lrecv = low(nrecv+chan) - irecv; /* Length received */

      /* Calc message lengths without multiplying */
      lsend = spsize(lsend,il->phshft);
      lrecv = spsize(lrecv,il->phshft);
      isend = spsize(isend,il->phshft);
      irecv = spsize(irecv,il->phshft);

      if (NC.node & chan) {   /* Odd node receives first */
         if (lrecv) {
            nngcr(&nnio, nnext, type);
            nnget(&nnio, ps+irecv, lrecv);
            nngcl(&nnio);
            }
         if (lsend) {
            nnpcr(&nnio, nnext, type);
            nnput(&nnio, ps+isend, lsend);
            nnpcl(&nnio);
            }
         }
      else {                  /* Even node sends first */
         if (lsend) {
            nnpcr(&nnio, nnext, type);
            nnput(&nnio, ps+isend, lsend);
            nnpcl(&nnio);
            }
         if (lrecv) {
            nngcr(&nnio, nnext, type);
            nnget(&nnio, ps+irecv, lrecv);
            nngcl(&nnio);
            }
         }
      }

#else /* Not HYPERCUBE, must be RING */

/* N.B.  This algorithm is designed to reduce the likelihood of ring
*  lock-up.  It is NOT guaranteed to be deadlock-free.  Each node
*  receives the pieces of the ps array in whatever order they arrive.
*  In the original, reading was done in order of increasing sender
*  node number, thus allowing incoming buffers to pile up depending
*  on order in which senders reached this code.  With this method,
*  we do not know where each piece goes or how long it will be until
*  after we read it and find out where it came from.  Use of anreadp
*  lets us figure this out before moving the data.  The algorithm is
*  potentially faulty in that the data segments could be long enough
*  to require segmentation.  All that can be said is, the original
*  (nonworking) algorithm had the same problem, so we are no worse
*  off than before.  */

/* If local node has cells of this layer, and machine has more than
*  one computational node, send data around the ring */

   if (il->mcells > 0 && NC.totalm1 > 0)
      anwrite(il->ps2, spsize(il->mcells,il->phshft),
         EXCHADDR, type, "S(I) DATA");

/* Receive data in whatever order they arrive */

   /* Calculate total length of data we expect to receive */
   ltr = il->nelt - il->mcells;
   ltr = spsize(ltr, il->phshft);
   while (ltr) {
      long ltt;
      src = -1;               /* Read from anybody */
      len = anreadp(&pdb, ltr, &src, &type, "EXCHANGE DATA LENGTH");
      ltt = cls(src);
      if (len != spsize(ltt,il->phshft))
         d3exit(EXCH_LEN_ERR, NULL, len);
      ltt = low(src);
      memcpy(ps+spsize(ltt,il->phshft), pdb, len);
      anrelp(pdb);
      ltr -= len;    
      }

#endif /* HYPERCUBE/RING */

/* Head node returns updated s(i) data to host */

   if (NC.node==NC.headnode)
      anwrite(ps, il->lspt, NC.hostid, type, "S(I) DATA");

#endif /* Not PAR0 */
 
   stattmr(OP_POP_TMR, COMM_TMR);
   return;
   } /* End d3exch() */


