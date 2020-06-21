/* (c) Copyright 1991-2010, Neurosciences Research Foundation, Inc. */
/* $Id: d3gsta.c 50 2012-05-17 19:36:30Z  $ */
/*---------------------------------------------------------------------*
*                             CNS Program                              *
*                                                                      *
*                   d3gsta - gather the statistics                     *
*                                                                      *
*  This routine is called at the end of each trial.                    *
*  It is used only in the parallel version, and is executed on all     *
*  nodes to gather statistics.  Cross-response statistics are gathered *
*  one cell layer at a time, interleaved with printing, by d3stat.     *
*                                                                      *
*  RESTRICTION:  Code currently assumes a homogeneous collection of    *
*  comp nodes, so nxdr reformatting is used only for sending to host.  *
*                                                                      *
*  V5C, 12/26/91, GNR - Add D1 stats, remove d3stat call               *
*  V5E, 07/15/92, GNR - Use prompted messages throughout               *
*  Rev, 12/14/92, ABP - Comment out odd node conditional on host node, *
*                       add HYB code to return data to host            *
*  V6A, 03/29/93, GNR - Add mdist (distribution of modulation value)   *
*  V6C, 09/17/93, GNR - Improved ring algorithm, make DAS double       *
*  V7A, 04/20/94, GNR - Revise to work with one comp node              *
*  V7C, 11/19/94, GNR - Add C,F dist stats, DIDSTATS tests             *
*  V8A, 05/12/96, GNR - Make GCONN/MOD stats dynamically allocated     *
*  Rev, 11/26/96, GNR - Remove support for non-hybrid version          *
*  Rev, 09/28/97, GNR - Independent dynamic allocations per conntype   *
*  V8C, 06/08/03, GNR - Cell responses in millivolts, add conductances *
*  V8D, 05/20/06, GNR - Change hhcom() calls to nncom()                *
*  ==>, 12/28/07, GNR - Last mod before committing to svn repository   *
*  V8F, 05/17/10, GNR - Add KRPGP statistics                           *
*---------------------------------------------------------------------*/

/*
*  D3GSTA can handle amounts of statistics that exceed the size of a
*  message buffer--a merge is done each time the buffer gets full.
*  The repertoire tree itself is used to store the intermediate and
*  final results, where they are handy for printing out with d3stat().
*  Nodes other than host have garbage in their statistics at the end.
*  Rather than create a giant COPDEF to define the merging process, it
*  is done in pieces, one CELLTYPE or CONNBLK at a time.

*  The actual merge is done on hypercubes by a logarithmic folding
*  process--in each pass, the odd nodes send their data to the corres-
*  ponding even nodes, then drop out of subsequent passes.  At the end,
*  all the data ends up on the host, where it is distributed back into
*  the tree.  On ring systems, each node just passes the data to its
*  lower neighbor, where it is merged and passed on.
*/

#ifdef D1
#define D1TYPE  struct D1BLK
#endif

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "collect.h"
#ifdef D1
#include "d1def.h"
#endif

/* Macro to handle data merge.  On hypercube and first ring comp
*  node, merge is from buffer into tree.  On other ring nodes,
*  merge is from tree into buffer.  This expands each vecombine
*  call into two--could be eliminated by making a compile-time
*  test for first comp node.  */

#define VECOMBINE(tree,buff,ops,n) { if (sendflg == NNC_Send) \
   vecombine(buff,tree,ops,n); else vecombine(tree,buff,ops,n); }

void d3gsta(void) {

/*---------------------------------------------------------------------*
*                                                                      *
*                          COPDEF Structures                           *
*                                                                      *
*---------------------------------------------------------------------*/

/* The following declarations define the structure of the statistics
*  in D1BLKs, CELLTYPEs, CONNTYPEs, INHIBBLKs, and MODBY blocks.  They
*  must be maintained in step with the definitions of those structures
*  in d1def.h, celltype.h, conntype.h, inhibblk.h, and modby.h, bzw. */

#ifndef PAR0
#ifdef D1
/* Define statistics in D1STATS structure */
#define NumD1Ops    4         /* Number of COPDEFs in D1 stats */
#define SizeD1Stats   (sizeof(struct D1STATS))
   static struct COPDEF D1Cop[NumD1Ops] = {
      { ADD+LONG,  1 },
      { MAX+LONG,  2 },
      { MIN+LONG,  1 },
      { ADD+LONG,  1 },
      } ;
#endif

/* Define statistics in CELLTYPE structure */
#define NumCellOps  7         /* Number of COPDEFs in cell stats */
#define SizeCellStats (sizeof(struct CLSTAT))
   static struct COPDEF CellCop[NumCellOps] = {
      { ADD+LLONG, 4 },
      { MIN+LONG,  1 },
      { MAX+LONG,  2 },       /* Max(nstcap) OK, same all nodes */
      { ADD+LONG, 10 },
      { MIN+SI32V, 1 },
      { MAX+SI32V, 1 },
      { ADD+LONG,  2*LDSTAT },
      } ;

#define NumGPOps    3         /* Number of COPDEFS in KRPGP stats */
#define SizeGPStats (sizeof(struct GPSTAT))
   static struct COPDEF GPCop[NumGPOps] = {
      { ADD+LONG,  2 },
      { MAX+LONG,  1 },
      { MIN+LONG,  2 }
      } ;

/* Define statistics in CONNTYPE structure */
#define NumConnOps  3         /* Number of COPDEFs in conn stats */
#define SizeConnStats (sizeof(struct CNSTAT))
   static struct COPDEF ConnCop[NumConnOps] = {
      { ADD+LLONG, 2 },
      { NOOP+LONG, 1 },
      { ADD+LONG,  7 }
      } ;

#define NumDasOps   1         /* Number of COPDEFs in DAS stats  */
   static struct COPDEF DasCop[NumDasOps] = {
      { ADD+LLONG,  0 }       /* Size filled in dynamically */
      } ;

/* Define long and long long distribution statistics */
#define NumDistOps  1         /* Number of COPDEFs in a distribution */
#define SizeDdistStat (sizeof(ddist))
   static struct COPDEF DdistCop[NumDistOps] = {
      { ADD+LLONG, LDSTAT }};
#define SizeDistStat (sizeof(dist))
   static struct COPDEF DistCop[NumDistOps] = {
      { ADD+LONG,  LDSTAT }} ;
#endif

/*---------------------------------------------------------------------*
*                                                                      *
*                         Other Declarations                           *
*                                                                      *
*---------------------------------------------------------------------*/

#ifdef D1
   struct D1BLK    *pd1;      /* Starting D1BLK pointer */
   struct D1BLK    *id1;      /* Scanning D1BLK pointer */
#endif
   struct REPBLOCK *pr;       /* Starting repertoire pointer */
   struct REPBLOCK *ir;       /* Scanning repertoire pointer */
   struct CELLTYPE *pl;       /* Starting celltype pointer */
   struct CELLTYPE *il;       /* Scanning celltype pointer */
   struct CONNTYPE *px;       /* Starting conntype pointer */
   struct CONNTYPE *ix;       /* Scanning conntype pointer */
#ifdef PARn
   register byte *r;          /* Working buffer pointer */
   byte *rbuf;                /* Dynamic read buffer */
   byte *wbuf;                /* Dynamic write buffer */
   byte *btop;                /* Top of either buffer */
   int ictdist,ictddst;       /* Cell type distribution counters */
   int save_ictdist,save_ictddst;
   int igpstat;               /* GPSTAT counters */
   int save_igpstat;
#ifdef HYPERCUBE
   int chan;                  /* Channel number */
#endif
   int next;                  /* Partner node */
   int type;                  /* Message type */
   int prompt;                /* Dummy prompt message */
   int rlen;                  /* Length of nreadp data */
   byte done_cell_stats;      /* TRUE if CELLTYPE stats done */
   byte save_done_cell_stats; /* Channel loop restart done_cell_stats */
   byte done_das_stats;       /* TRUE if detailed amp stats done */
   byte save_done_das_stats;  /* Channel loop restart done_das_stats */
#endif
   int sendflg;               /* Direction flag for vecombine, nncom */

   /* The hhgs stream and its associated variables are only used
   *  on the head comp node to communicate with the host.  */
   struct NNSTR hhgs;         /* Control block for stream */
   void *phob;                /* Pointer to object for nxdr comm */
   /* Space to create local conversion table for DAS */
   long nxtabl[3];

/*---------------------------------------------------------------------*
*                                                                      *
*                          Executable Code                             *
*                                                                      *
*---------------------------------------------------------------------*/

#ifndef PAR0

/* If there is only one comp node, then the initial cycle of merging
*  is not needed--drop down directly to the code that sends the data
*  to the host.  */

   if (NC.total == 2) goto send_to_host;

/* Initialize tree traversal pointers and flags.
*  Repeat merge until all cell types completed.  */

   type = STATS_MSG;
   pl = (pr=RP->tree)->play1;
   px = pl->pct1;
   save_ictdist = save_ictddst = save_igpstat = 0;
   save_done_cell_stats = save_done_das_stats = FALSE;
#ifdef D1
   pd1 = RP->pd1b1;
   while (pd1 || pr)
#else
   while (pr)
#endif
      {

#ifdef HYPERCUBE
/* Perform cube merge, looping over dimensions */

      sendflg = NNC_Rcv;
      for (chan=1; chan<NC.total; chan<<=1) {
         next = NC.node ^ chan;
#endif

/* Resume tree tranversal at point left off when last buffer filled */

#ifdef D1
         id1 = pd1;
#endif
         ir = pr; il = pl; ix = px;
         ictdist         = save_ictdist;
         ictddst         = save_ictddst;
         igpstat         = save_igpstat;
         done_cell_stats = save_done_cell_stats;
         done_das_stats  = save_done_das_stats;

#ifdef HYPERCUBE
/* If on an even node, prompt the partner, receive the data, and
*  merge into the data in the repertoire tree here.  These data
*  will get sent on the next cycle if this in turn becomes an odd
*  node.  However, some nodes will receive from two or more channels
*  consecutively (host node always receives) and for this reason, the
*  merging must go back into the tree to free up buffer storage.  */

         if (!(NC.node & chan)) {
#else
/* Machines other than hypercubes currently imply ring topology.
*  Read the buffer from our upstream neighbor, if one exists.
*  Here, the data are sent on down the ring as soon as the merge
*  is completed, so there is no need to transfer back into the
*  tree.  The VECOMBINE macro reverses the direction of merge
*  in this case.  */

         if (NC.node < NC.tailnode) {
            next = NC.node + 1;
            sendflg = (NC.node > NC.headnode) ? NNC_Send : NNC_Rcv;
#endif
            anwrite(&prompt, 0, next, type, "STAT PROMPT");
            rlen = anreadp((void **)&rbuf, MAX_MSG_LENGTH,
               &next, &type, "STATS");

/* Merge the statistics from the partner node with mine */

            r = rbuf; btop = rbuf + rlen;

#ifdef D1
            for (; id1; id1=id1->pnd1b) {
               /* Skip gather if statistics suppressed */
               if (id1->kd1p & KD1_NOST) continue;
               if (r + SizeD1Stats > btop) goto end_merge;
               VECOMBINE(&id1->stats, r, D1Cop, NumD1Ops);
               r += SizeD1Stats;
               } /* End receiving D1 blocks */
#endif

            /* Following code avoids picking up il from bad address */
            for (; ir; ir=ir->pnrp,
                  il=ir?ir->play1:NULL, done_cell_stats=FALSE) {
               /* Skip gather if statistics suppressed */
               if (ir->Rp.krp & KRPNS) continue;

               /* Gather CELLTYPE stats */
               for (; il; il=il->play, done_cell_stats=FALSE) {
                  /* Skip gather if statistics suppressed */
                  if (!(il->ctf&DIDSTATS)) continue;

                  /* New CELLTYPE--collect cell-level stats */
                  if (!done_cell_stats) {
                     if (r + SizeCellStats > btop) goto end_merge;
                     VECOMBINE(il->CTST, r, CellCop, NumCellOps);
                     r += SizeCellStats;
                     done_cell_stats = TRUE;
                     ix = il->pct1;
                     ictdist = ictddst = igpstat = 0;
                     done_das_stats = FALSE;
                     }

                  /* Get distribution stats for this CELLTYPE and
                  *  all three classes of connection types.  */
                  for (; ictddst<il->nctddst; ictddst++) {
                     if (r + SizeDdistStat > btop) goto end_merge;
                     VECOMBINE(il->pctddst+ictddst, r,
                        DdistCop, NumDistOps);
                     r += SizeDdistStat;
                     }

                  for (; ictdist<il->nctdist; ictdist++) {
                     if (r + SizeDistStat > btop) goto end_merge;
                     VECOMBINE(il->pctdist+ictdist, r,
                        DistCop, NumDistOps);
                     r += SizeDistStat;
                     }

                  /* Get KRPGP|KRPMG stats for this CELLTYPE */
                  for (; igpstat<(int)il->nCdist; igpstat++) {
                     if (r + SizeGPStats > btop) goto end_merge;
                     VECOMBINE(CDAT.pgstat+il->oGstat+igpstat, r,
                        GPCop, NumGPOps);
                     r += SizeGPStats;
                     }

                  /* Get connection-level stats */
                  for (; ix; ix=ix->pct, done_das_stats=FALSE) {
                     /* Gather detailed amplification statistics */
                     if (!done_das_stats && ix->Cn.kam&KAMDS) {
                        byte *re = r + ix->ldas;
                        if (re > btop) goto end_merge;
                        DasCop[0].u.count = ix->ldas/sizeof(si64);
                        VECOMBINE(ix->pdas, r, DasCop, NumDasOps);
                        r = re;
                        done_das_stats = TRUE;
                        }
                     /* Gather regular connection stats */
                     if (r + SizeConnStats > btop) goto end_merge;
                     VECOMBINE(&ix->CNST, r, ConnCop, NumConnOps);
                     r += SizeConnStats;
                     }

                  } /* End loop over cell blocks */

               } /* End repertoire merge loop */

end_merge:
#ifdef HYPERCUBE
            anrelp(rbuf);      /* Release read buffer */
#else
/* In non-hypercube (i.e. ring), pass the message down to the first
*  comp node.  The buffer received in the anreadp() above is used
*  directly for writing (which releases it).  There is no need to
*  merge on nodes that do not take part in the computation.  */

            if (NC.node > NC.headnode) {
               next = NC.node - 1;
               anread(&prompt, 0, &next, &type, "STAT PROMPT");
               anwritep(rbuf, r-rbuf, next, type, "STATS");
               } /* End collection on nonhead node */
#endif   /* not HYPERCUBE */

            } /* End processing on even or nontail node */

/* The following code copies data from tree structure to buffer
*  and simply writes it to the partner node.  On a hypercube, it
*  runs anytime a node is odd with respect to the current channel.
*  On a ring, it runs only on the tail node, and this is determined
*  at run time because there is no compile-time test for the tail
*  node.  These conditions are satisfied by making this block an
*  "else" dependent on the block above.  */

         else {
#ifndef HYPERCUBE
            next = NC.node - 1;
#endif
            r = wbuf = angetpv(MAX_MSG_LENGTH, "GSTA buffer");
            btop = wbuf + MAX_MSG_LENGTH;

/* Collect as much data as will fit */

#ifdef D1
            for (; id1; id1=id1->pnd1b) {
               /* Skip gather if statistics suppressed */
               if (id1->kd1p & KD1_NOST) continue;
               if (r + SizeD1Stats > btop) goto end_collect;
               memcpy((char *)r, (char *)&id1->stats, SizeD1Stats);
               r += SizeD1Stats;
               } /* End sending D1 blocks */
#endif

            /* Following code avoids picking up il from bad address */
            for (; ir; ir=ir->pnrp,
                  il=ir?ir->play1:NULL, done_cell_stats=FALSE) {
               /* Skip gather if statistics suppressed */
               if (ir->Rp.krp & KRPNS) continue;

               /* Gather CELLTYPE stats */
               for (; il; il=il->play, done_cell_stats=FALSE) {
                  /* Skip gather if statistics suppressed */
                  if (!(il->ctf&DIDSTATS)) continue;

                  /* New CELLTYPE--collect cell-level stats */
                  if (!done_cell_stats) {
                     if (r + SizeCellStats > btop) goto end_collect;
                     memcpy((char *)r, (char *)il->CTST,
                        SizeCellStats);
                     r += SizeCellStats;
                     done_cell_stats = TRUE;
                     ix = il->pct1;
                     ictdist = ictddst = igpstat = 0;
                     done_das_stats = FALSE;
                     }

                  /* Get distribution stats for this CELLTYPE and
                  *  all three classes of connection types.  */
                  for (; ictddst<il->nctddst; ictddst++) {
                     if (r + SizeDdistStat > btop) goto end_collect;
                     memcpy((char *)r, (char *)
                        (il->pctddst+ictddst), SizeDdistStat);
                     r += SizeDistStat;
                     }

                  for (; ictdist<il->nctdist; ictdist++) {
                     if (r + SizeDistStat > btop) goto end_collect;
                     memcpy((char *)r, (char *)
                        (il->pctdist+ictdist), SizeDistStat);
                     r += SizeDistStat;
                     }

                  /* Get KRPGP|KRPMG stats for this CELLTYPE */
                  for (; igpstat<(int)il->nCdist; igpstat++) {
                     if (r + SizeGPStats > btop) goto end_collect;
                     memcpy((char *)r, (char *)
                        (CDAT.pgstat+il->oGstat+igpstat), SizeGPStats);
                     r += SizeGPStats;
                     }

                  /* Collect connection-level stats */
                  for (; ix; ix=ix->pct, done_das_stats=FALSE) {
                     /* Collect detailed amplification statistics */
                     if (!done_das_stats && ix->Cn.kam&KAMDS) {
                        byte *re = r + ix->ldas;
                        if (re > btop) goto end_collect;
                        memcpy((char *)r, (char *)ix->pdas,
                           (size_t)ix->ldas);
                        r = re;
                        done_das_stats = TRUE;
                        }
                     /* Collect regular connection stats */
                     if (r + SizeConnStats > btop) goto end_collect;
                     memcpy((char *)r, (char *)&ix->CNST,
                        SizeConnStats);
                     r += SizeConnStats;
                     }

                  } /* End loop over cell blocks */

               } /* End repertoire collect loop */

/* Wait for prompt, then send to the partner node (releasing the
*  buffer), and drop out of the collection process. */

end_collect:
            anread(&prompt, 0, &next, &type, "STAT PROMPT");
            anwritep(wbuf, r-wbuf, next, type, "STATS");
#ifdef HYPERCUBE
            /* Drop out of channel loop after sending */
            break;
#endif
            } /* End collection on odd node */

#ifdef HYPERCUBE
         } /* End processing all channels */
#endif

/* One merge pass, consisting of all the data or one buffer load,
*  whichever is less, has now been completed.  Update the starting
*  repertoire tree pointers and perform more passes if required. */

#ifdef D1
      pd1 = id1;
#endif
      pr = ir; pl = il; px = ix;
      save_ictdist         = ictdist;
      save_ictddst         = ictddst;
      save_igpstat         = igpstat;
      save_done_cell_stats = done_cell_stats;
      save_done_das_stats  = done_das_stats;
      } /* End WHILE loop over repertoires */

#endif /* ifndef PAR0 */

/* All the statistics have now been merged onto the first comp node.
*  They must now be sent on to the host.  Since no further vecombine
*  is required, we can use a node-to-node stream and let the library
*  buffer everything for us.  */

send_to_host:
#ifdef PAR0
   nngcr(&hhgs, NC.headnode, GSTA_MSG);
   sendflg = NNC_Rcv;
#else
   if (NC.node != NC.headnode)
      return;
   nnpcr(&hhgs, NC.hostid, GSTA_MSG);
   sendflg = NNC_Send;
#endif

/* Traverse the tree, and send all statistics back to the host */

#ifdef D1
   for (id1=RP->pd1b1; id1; id1=id1->pnd1b) {
      /* Skip gather if statistics suppressed */
      if (id1->kd1p & KD1_NOST) continue;
      phob = &id1->stats;
      nncom(&hhgs, &phob, ttloc(IXstr_D1STATS), sendflg);
      } /* End sending D1 blocks */
#endif

   /* Traverse repertoire tree */
   for (ir=RP->tree; ir; ir=ir->pnrp) {
      /* Skip gather if statistics suppressed */
      if (ir->Rp.krp & KRPNS) continue;

      /* Gather CELLTYPE stats */
      for (il=ir->play1; il; il=il->play) {
         /* Skip gather if statistics suppressed */
         if (!(il->ctf&DIDSTATS)) continue;
         phob = il->CTST;
         nncom(&hhgs, &phob, ttloc(IXstr_CLSTAT), sendflg);

         /* Collect CELLTYPE distribution stats.  Use a synthetic nxtab
         *  entry to make nncom do the swaps for us.  See nxdr2 doc. */
         if (il->nctddst) {
            nxtabl[0] = (LDSTAT*il->nctddst)*sizeof(si64);
            nxtabl[1] = (LDSTAT*il->nctddst)<<8 | 'W' ;
            phob = il->pctddst;
            nncom(&hhgs, &phob, nxtabl, sendflg);
            }

         if (il->nctdist) {
            nxtabl[0] = (LDSTAT*il->nctdist)*sizeof(long);
            nxtabl[1] = (LDSTAT*il->nctdist)<<8 | 'L' ;
            phob = il->pctdist;
            nncom(&hhgs, &phob, nxtabl, sendflg);
            }

         if (il->Ct.kctp & (KRPGP|KRPMG)) {
            nxtabl[0] = il->nCdist * sizeof(struct GPSTAT);
            nxtabl[1] = il->nCdist<<8 | 'X' ;
            nxtabl[2] = -IXstr_GPSTAT;
            }

         /* Collect connection-level stats */
         for (ix=il->pct1; ix; ix=ix->pct) {
            /* Collect detailed amplification statistics */
            if (ix->Cn.kam & KAMDS) {
               nxtabl[0] = ix->ldas;
               nxtabl[1] = (ix->ldas/sizeof(*(ix->pdas)))<<8 | 'W' ;
               phob = ix->pdas;
               nncom(&hhgs, &phob, nxtabl, sendflg);
               }
            /* Collect regular connection stats */
            phob = &ix->CNST;
            nncom(&hhgs, &phob, ttloc(IXstr_CNSTAT), sendflg);

            } /* End loop over conntypes */
         } /* End loop over cell blocks */
      } /* End repertoire collect loop */
#ifdef PAR0
   nngcl(&hhgs);
#else
   nnpcl(&hhgs);
#endif
   } /* End d3gsta() */
