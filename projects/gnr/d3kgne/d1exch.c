/* (c) Copyright 1991-2003, Neurosciences Research Foundation, Inc. */
/* $Id: d1exch.c 3 2008-03-11 19:50:00Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d1exch                                 *
*                                                                      *
*     Exchange D1 data across all nodes, i.e. provide all nodes with   *
*  identical copies of all the D1 data by merging the data calculated  *
*  separately on each node by d1go.  (This routine is based on the     *
*  code of d3exch, specialized to work with a single score array and   *
*  no phase doubling.  The algorithm is sensitive to the topology of   *
*  the parallel computer being used.  Versions for ring and hypercube  *
*  are currently supplied.)                                            *
*     Routine should be called in parallel from all nodes, but only    *
*  if PAR and D1 are defined to preprocessor and RP->pd1b1 is not NULL.*
*                                                                      *
*  Synopsis:  void d1exch(struct D1BLK *pd1, int retflg)               *
*     pd1 is a pointer to the D1BLK to be exchanged.                   *
*     retflg is TRUE if the host in a hybrid version also requires     *
*        a copy of the score data, otherwise FALSE.                    *
*                                                                      *
*  V5C, 02/04/92, GNR - Split out from d1go for use by d3save          *
*  Rev, 05/20/92, ABP - Add stattmr() calls                            *
*  Rev, 12/09/92, ABP - Add HYB code                                   *
*  V6C, 08/19/93, GNR - Add ring version, retflg argument              *
*  V8A, 11/16/96, GNR - Remove support for non-hybrid version          *
*  V8A, 07/25/98, GNR - Add error checking on node-node stream         *
*  V8C, 02/22/03, GNR - Cell responses in millivolts, add conductances *
*  ==>, 02/17/05, GNR - Last mod before committing to svn repository   *
***********************************************************************/
#define D1TYPE  struct D1BLK

#ifndef PAR
#error Attempt to compile d1exch for serial version of CNS
#endif
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "d3global.h"
#include "d1def.h"

/* Macro for computing locell of a cluster (see d3exch) */
#define Low(n) (nm1 = (n) - NC.headnode, \
   nm1 = max(nm1,0), nm1*pd1->cpn + min(nm1,pd1->crn))

#ifdef RING
/* Macro for computing the number of cells on a given node */
#define cls(n) (pd1->cpn + ((n) - NC.headnode < pd1->crn))

/* Macro for computing the upstream neighbour of a node */
#define upnbr(n) (n==NC.tailnode ? NC.headnode : n+1)
#endif

/*---------------------------------------------------------------------*
*                       d1exch executable code                         *
*---------------------------------------------------------------------*/

void d1exch(struct D1BLK *pd1, int retflg) {

#ifdef PAR0
   d1s_type *psi = RP0->ps0;  /* Ptr to exchange result */
#else
   d1s_type *psi = pd1->pd1s; /* Local copy of pd1->pd1s */
   long nelt = pd1->nd1r*pd1->nepr; /* Total elements in block */
#endif
   int type = EXCH_D1_MSG;    /* Message type for exchange */
#ifdef PAR0
   int src = ANYNODE;
#else
   int nm1;                   /* Bottom node-1 of a cluster */
#ifdef HYPERCUBE
   struct NNSTR nnio;         /* Stream for nnput/nnget */
   int chan,mask,nnext;       /* Used to traverse hypercube */
   int nsend,isend,lsend;     /* Send node, data index, length */
   int nrecv,irecv,lrecv;     /* Recv node, data index, length */
#else
   int node;
   int len;
   int ltr;                   /* Data left to receive */
#endif
#endif

   stattmr(OP_PUSH_TMR,D1EX_TMR);

#ifdef PAR0

   /* Receive the data array */
   if (retflg) anread((char *)psi, pd1->nepr*pd1->nd1r, &src,
         &type, "D1 CELLS");

#else    /* I am a comp node */

/* Assemble a copy of all the data on all nodes */

#ifdef HYPERCUBE   

   for (chan=1; chan<NC.total; chan<<=1) {
      nnext = NC.node^chan;   /* Node I will send to now */
      mask = ~(chan-1);       /* Bit pattern = high bits set */

      nsend = NC.node&mask;   /* Bottom node of sent cluster */
      nrecv = nnext&mask;     /* Bottom node of rcvd cluster */

      isend = Low(nsend);     /* Get locell of sent cluster */
      irecv = Low(nrecv);     /* Get locell of rcvd cluster */

      lsend = Low(nsend+chan) - isend; /* Length sent */
      lrecv = Low(nrecv+chan) - irecv; /* Length received */

      if (NC.node & chan) {
         if (lrecv) {
            nngcr(&nnio, nnext, type);
            nnget(&nnio, psi+irecv, lrecv);
            nngcl(&nnio);
            }
         if (lsend) {
            nnpcr(&nnio, nnext, type);
            nnput(&nnio, psi+isend, lsend);
            nnpcl(&nnio);
            }
         } 
      else {
         if (lsend) {
            nnpcr(&nnio, nnext, type);
            nnput(&nnio, psi+isend, lsend);
            nnpcl(&nnio);
            }
         if (lrecv) {
            nngcr(&nnio, nnext, type);
            nnget(&nnio, psi+irecv, lrecv);
            nngcl(&nnio);
            }
         }

      } /* End channel loop */

#else /* Not HYPERCUBE, must be RING */

/* If local node has cells of this layer, send them around the
*  ring */

   if (pd1->mynd1) anwrite(psi+pd1->lod1e, pd1->mynd1,
      EXCHADDR, type, "D1 CELLS");

   ltr = nelt - pd1->mynd1;   /* Left to receive */

/* Start receiving data, starting with upper neighbor.
*  Note:  If this program ever hangs, see d3exch() for a
*  newer variant that receives data in order of arrival.  */

   for (node = upnbr(NC.node); ltr; node=upnbr(node)) {
      if ((len = cls(node)) == 0) continue;
      anread(psi+Low(node), len, &node, &type, "D1 CELLS");
      ltr -= len;
      }

#endif /* HYPERCUBE/RING */

/* The headnode now sends the data to the host if requested */

   if (retflg && NC.node==NC.headnode)
      anwrite(psi, nelt, NC.hostid, type, "D1 CELLS");

#endif /* Not PAR0 */

   stattmr(OP_POP_TMR,D1EX_TMR);
   } /* End d1exch */
