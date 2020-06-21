/* (c) Copyright 2016, The Rockefeller University *11115* */
/* $Id: coppack.c 3 2017-01-16 19:29:37Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                              coppack()                               *
*                                                                      *
*  Routines for parallel computers to take information (typically      *
*     statistics) provided in multiple calls, pack into buffers, and   *
*     combine (by add, max, etc.) across nodes according to COPDEF     *
*     descriptors.  The final results are collected into the source    *
*     arrays on node 0 as the calculation proceeds, and are complete   *
*     when a final call to copcomb() is made.  Data on other nodes are *
*     not disturbed.  This mechanism is an improvement on vecombine in *
*     that messages are passed (up to the end) in buffer-sized chunks  *
*     rather than COPDEF-sized chunks.                                 *
*                                                                      *
*  Data in the pdat areas on the NC.hostid node may or may not be      *
*     included in any data combination operations as controlled by a   *
*     flags option in the coppini() startup routine.  These data are   *
*     written over with the collected data from the comp nodes.        *
*                                                                      *
*  Each node on a parallel computer must call coppini() to initialize  *
*     use of this package for a particular collection of data, call    *
*     coppack() repeatedly to contribute portions of the data when     *
*     available, copcomb() to ensure that the entire collected data    *
*     array is available on the host node, and copfree() to free any   *
*     memory that was allocated by coppini().  Multiple batches of     *
*     data can be handled by one coppini() setup as long as copcomb()  *
*     is called before a new batch is started.                         *
*                                                                      *
*  Prototypes are in collect.h                                         *
*                                                                      *
*----------------------------------------------------------------------*
*                                                                      *
*  Startup call:                                                       *
*  CPKD *coppini(int type, int bsize, int flags)                       *
*                                                                      *
*  Arguments:                                                          *
*  type  -- Type assigned to messages sent to accomplish the global    *
*           combination.                                               *
*  bsize -- Length of buffers to be used for collecting data.  If set  *
*           to 0, the default MAX_MSG_LENGTH from mpitools.h will be   *
*           used.  This argument is really here for debugging use,     *
*           but there may be some benefit to modifying the buffer      *
*           size for unusually large or small data collections.        *
*  flags -- Control options.  At present, only one is defined:         *
*           CPK_INC0    Include Node 0 data in collection operations.  *
*                                                                      *
*  Returns:                                                            *
*  Pointer to a CPKD structure that is created and initialized by this *
*           call.  This structure holds all internal state data for a  *
*           sequence of coppack() calls.  When no longer needed, its   *
*           memory should be released by a copfree() call.             *
*                                                                      *
*----------------------------------------------------------------------*
*                                                                      *
*  Data collection call:                                               *
*  void coppack(CPKD *pcpk, void *pdat, COPD *ops, int opcnt)          *
*                                                                      *
*  Arguments:                                                          *
*  pcpk  -- Pointer to a CPKD structure that has been initialized      *
*           by a previous call to coppini().                           *
*  pdat --  Pointer to the contribution of the current node to the     *
*           global data being collected, as described by the 'ops'     *
*           argument.  Only data on node 0 are modified by this call   *
*           to contain the final results.  After the first call in a   *
*           sequence of calls gives a data address, pdat may be a      *
*           NULL pointer, indicating that the data are contiguous with *
*           the end of the data from the last call.  Each new non-NULL *
*           pdat indicates noncontiguous data are to be collected.     *
*  ops   -- Pointer to an array of 'opcnt' COPDEF structures.  The     *
*           length of the data in 'pdat' must match the total length   *
*           of the data described by the COPDEF array.                 *
*  opcnt -- Number of COPDEFs in the 'ops' array.                      *
*                                                                      *
*----------------------------------------------------------------------*
*                                                                      *
*  Data batch termination call:                                        *
*  void copcomb(CPKD *pcpk)                                            *
*                                                                      *
*  Arguments:                                                          *
*  pcpk  -- Pointer to a CPKD structure that has been initialized      *
*           by a previous call to coppini().  When this call           *
*           returns, all data transmission and combination has         *
*           been completed and the completely combined data are        *
*           available on the PAR0 node at the locations given in       *
*           the various coppack calls.                                 *
*                                                                      *
*----------------------------------------------------------------------*
*                                                                      *
*  CPKD usage completion and memory release call:                      *
*  void copfree(CPKD *pcpk)                                            *
*                                                                      *
*  Arguments:                                                          *
*  pcpk  -- Same as for copcomb().  After this call returns, the       *
*           storage occupied by the CPKD structure can be reused       *
*           for other purposes.                                        *
*                                                                      *
*----------------------------------------------------------------------*
*                                                                      *
*  Notes:                                                              *
*  -- Two data collection buffers are allocated on each node by the    *
*     coppini() call and released by the copfree() call.  Their        *
*     length is given by the bsize argument to coppini().              *
*  -- When a larger data set is broken down into smaller pieces        *
*     described by COPDEFs in multiple coppack() calls, the same       *
*     data in the same order must be provided/requested on all nodes,  *
*     with the same breakdown into separate calls within a buffer.     *
*     The COPDEFs are stored with the data in the collection buffer.   *
*  -- This code makes the historical assumption that message byte      *
*     order and alignment of items are same on all comp processor      *
*     nodes.  If this ever changes, code must be rewritten to use      *
*     lemfm/bemfm routines or MPI library functions with similar       *
*     semantics.                                                       *
*  -- All types except UI32V are assumed to be signed statistics.      *
*                                                                      *
*  Enhancements:                                                       *
*  -- We may want to expand the count field in COPDEFs from 16 bits    *
*     to maybe 48 bits, leaving the old 16 for the operation code.     *
*  -- If we ever add operations on byte data, code needs to be added   *
*     to align operation codes stored in pcpk buffers.                 *
*                                                                      *
*  Errors:                                                             *
*  All errors are non-recoverable and result in abexit() termination.  *
*  Error codes in the range 200-208 are assigned to this package.      *
*                                                                      *
************************************************************************
*  V1A, 07/07/12, GNR - New routines defined, not written              *
*  Rev, 06/17/16, GNR - Resumed work on these incomplete routines      *
*  Rev, 08/31/16, GNR - Use MPI calls, do not include host node data   *
*  Rev, 09/03/16, GNR - Add CPK_INC0 flag                              *
*  ==>, 09/29/16, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rksubs.h"
#include "mpitools.h"
#include "collect.h"
#include "rkarith.h"
#include "swap.h"

/* Macro to align destination offset to various types */
#define odup(o,sz) (((o) + (sz-1)) & ~(sz-1))

/* Lengths by element type codes */
static int typlen[8] =
   {  LSIZE, ISIZE, HSIZE, ESIZE, DSIZE, WSIZE, JSIZE, JSIZE };

/*=====================================================================*
*                               coppini                                *
*=====================================================================*/

CPKD *coppini(int type, int bsize, int flags) {

   CPKD *pcpk = callocv(1, sizeof(struct CPKDEF), "CPKDEF");
   pcpk->bsize = bsize > 0 ? bsize : MAX_MSG_LENGTH;
   pcpk->bsize = ALIGN_UP(pcpk->bsize);
   if (pcpk->bsize < 200) abexit(CPK_SMBSIZ);
   pcpk->buffw = mallocv(2*pcpk->bsize, "coppack buffers");
   pcpk->buffr = pcpk->buffw + pcpk->bsize;
   pcpk->pal = (byte **)mallocv(NCPKALL*sizeof(void *),
      "coppack data ptrs");
   pcpk->alallo = NCPKALL;
   pcpk->type = type;
   pcpk->flags = flags & CPK_INC0;
   return pcpk;
   } /* End coppini() */


/*=====================================================================*
*                               coppack                                *
*=====================================================================*/

void coppack(CPKD *pcpk, void *pdat, COPD *ops, int opcnt) {

   struct COPDEF *op,   /* Current COPDEF ptr  */
      *opstart,         /* Starting COPDEF ptr */
      *opstop,          /* Terminal COPDEF ptr */
      *optop;           /* End of COPDEF array */
   int cnt = opcnt;     /* # COPDEFs in current RCG */
   int doload;          /* TRUE to copy data to buffer */
   int irep;            /* Repeat loop counter */
   int repcnt = 1;      /* # of passes over current RCG */

   /* Sanity check */
   if (pcpk->flags & CPK_FREED) abexit(CPK_EFREED);
   doload = NC.node > NC.hostid || pcpk->flags & CPK_INC0;

/* Error if no pdat, save if new pdat */

/* The use of pcpk->pcd, alused, pal, and ipal is as follows:  The pal
*  list contains addresses for the starts of data blocks.  The same
*  data on different nodes will have different addresses.  alused is
*  incremented each time a new array address is encountered.  Each
*  COPD copied into a buffer has the index of an entry in the pal list
*  in the left 10 bits of the optype field (this cannot conflict with
*  its use with repeat operators, as they are expanded locally).  ipal
*  keeps track of this index as new COPDs are encountered.  pcd keeps
*  track of the location in the current pal array.  Each time a new
*  buffer is started, the list is emptied and alused restarts at zero.
*  When a new COPD has a NULL pdat, ipal repeats, and this is a signal
*  to copcomb to continue using the current data array.  In this
*  situation, or when a COPD is divided between buffers, the pcd field
*  keeps track of where to continue in the current data array.  */

   if (pdat) {          /* New pdat, save and use */
      if ((byte *)pdat != pcpk->pcd) {
         if (pcpk->alused >= pcpk->alallo) {
            int nxtallo = pcpk->alallo + NCPKALL;
            if (nxtallo > (UI16_MAX >> RepShft)) abexit(CPK_TMBLKS);
            pcpk->pal = (byte **)reallocv(pcpk->pal,
               nxtallo*sizeof(void *), "coppack data ptrs");
            pcpk->alallo = nxtallo;
            }
         pcpk->pcd = pcpk->pal[pcpk->alused++] = (byte *)pdat;
         }
      }
   else {               /* Use previous pdat or error */
      if (!pcpk->pcd) abexit(CPK_NOPREV);
      }
   optop = (opstart = ops) + opcnt; /* Start and end of COPDEFs */

/* Loop over current RCG (= repeating COPDEF group)
*  requested number of times */

REP_LOOP:
   opstop = opstart + cnt;
   if (repcnt == 0) op = opstop;
   else for (irep=0; irep<repcnt; irep++) {

      /* Loop over operation control structures and move data into
      *  the write buffer without combining until buffer is full.  */
      for (op=opstart; op<opstop; op++) {
         int ityp;            /* Type code for current item */
         int icnt;            /* Count extracted from COPDEF */
         int jcnt;            /* Count for current store */
         int lcnt;            /* Bytes in jcnt items */
         ui16 hcnt;           /* (ui16) COPD count field */

         if ((op->optype & OPMSK) == REPEAT) {  /* Repeat group */
            if (repcnt > 1)
               abexit(CPK_NSTREP); /* Nested repeats */
            repcnt = op->count;       /* # of times to repeat RCG */
            cnt = op->optype >> RepShft;   /* # of COPDEFS in RCG */
            if (cnt > optop - op) abexit(CPK_RCGLNG);
            opstart = ++op;           /* Inc to first RCG COPDEF  */
            goto REP_LOOP;
            }

         ityp = (int)(op->optype & TPMSK);
         icnt = (int)op->count;
         /* Normally on op->count is handled as a single block of
         *  data, but may have to be divided if buffer is full...
         *  The COPDEF, as long as it contains only ui16s, will
         *  always be aligned, so alignment of odat will be done
         *  after the COPDEF is moved in, not before.  */
         while (icnt > 0) {
            byte *tbw;        /* Temp for buffw */
            jcnt = ((pcpk->bsize - sizeof(COPD)) - pcpk->odat) /
               typlen[ityp];
            if (jcnt > icnt) jcnt = icnt;
            if (jcnt <= 0) {
               /*** HERE IS WHERE WE DO THE ACTUAL COMBINING ***/
               copcomb(pcpk);
               pcpk->pal[pcpk->alused++] = pcpk->pcd;
               continue;      /* Test icnt again */
               }
            /* Move a copy of the COPDEF and jcnt items to buffw */
            tbw = pcpk->buffw + pcpk->odat;
            hcnt = op->optype & OPMSK | (pcpk->alused-1) << RepShft;
            memcpy(tbw, &hcnt, HSIZE); tbw += HSIZE;
            hcnt = (ui16)jcnt;
            memcpy(tbw, &hcnt, HSIZE);
            pcpk->odat = odup(pcpk->odat+sizeof(COPD), typlen[ityp]);
            tbw = pcpk->buffw + pcpk->odat;
            lcnt = jcnt * typlen[ityp];
            if (doload) memcpy(tbw, pcpk->pcd, lcnt);
            pcpk->odat += lcnt;
            pcpk->pcd += lcnt;
            icnt -= jcnt;
            }  /* End icnt loop */
         }  /* End op loop */
      }  /* End irep loop */

/* End of one operation or a repeat block--determine whether
*  original opcnt copdefs have been completed and loop if not.  */

   if ((cnt = optop - op) > 0)  {
      repcnt = 1;     /* Castrate outer loop (reset from REPEAT) */
      opstart = op;   /* Resume processing at first COPDEF after RCG */
      goto REP_LOOP;
      }

   }  /* End coppack() */

/*=====================================================================*
*                               copcomb                                *
*                                                                      *
*  This code is a lot like old vecollect except the COPDEFs have been  *
*  stored in the CPKD buffers and repeats have been expanded already.  *
*=====================================================================*/

void copcomb(CPKD *pcpk) {

   COPD tcop;                       /* Temp for aligning COPDEFs */
   int chan;                        /* Channel for log collection */
   int incr;                        /* Size of data in this COPDEF */
   int inc0 = pcpk->flags & CPK_INC0 ? 1 : 0;   /* Include host? */
   int lown = 1 - inc0;             /* Low collection node */
   int myrnode = NC.node - lown;    /* My relative node number */
   int ncnodes = NC.cnodes + inc0;  /* Number nodes in collection */
   int odbe, odb;                   /* Offset into data buffers */
   int rc;                          /* Send/receive return code */

   /* Perform data combination by a hypercube-like algorithm */
   if (myrnode >= 0) for (chan=1; chan<ncnodes; chan<<=1) {
      int pard = myrnode ^ chan;    /* Partner relative node number */
      int next = pard + lown;       /* Partner absolute node number */

      if (myrnode & chan) {
         /* Nodes with odd numbers in the current channel send to
         *  their next lower partner, then drop out.  */
         rc = MPI_Send(pcpk->buffw, pcpk->odat, MPI_UNSIGNED_CHAR,
            next, pcpk->type, NC.commc);
         if (rc) appexit("COPCOMB: MPI_Send error", CPK_MPIWRE, rc);
         goto SetForNextBuffer;
         }

       else if (pard < ncnodes) {
         /* Nodes with even numbers in the current channel read a
         *  message from the partner node (unless the partner is above
         *  ncnodes and therefore does not exist) and combine the
         *  contribution read into 'buffw'.  'buffw' is retained until
         *  all contributions have been merged in, then the combined
         *  data are copied out to the user's (possibly noncontiguous)
         *  arrays in accord with the information stored in pcpk->pal.
         */
         rc = MPI_Recv(pcpk->buffr, pcpk->odat, MPI_UNSIGNED_CHAR,
            next, pcpk->type, NC.commc, MPI_STATUS_IGNORE);
         if (rc) appexit("COPCOMB: MPI_Recv error", CPK_MPIRDE, rc);
         odb = 0;
         while (odb < pcpk->odat) {
            int itop;               /* Extracted operation and type */
            int ityp;               /* Type of this data element */
            /* Sanity check */
            if (memcmp(pcpk->buffw+odb, pcpk->buffr+odb,
               sizeof(COPD))) abexit(CPK_BLKCMP);
            memcpy(&tcop, pcpk->buffw+odb, sizeof(COPD));
            odb += sizeof(COPD);
            itop = tcop.optype & OPMSK;
            ityp = tcop.optype & TPMSK;
            incr = tcop.count * typlen[ityp];
            odb = odup(odb, typlen[ityp]);
            odbe = odb + incr;

            /* Switch according to operation type */
            switch(itop) {

               case REPEAT:
                  abexit(CPK_NSTREP);

               case (ADD+LONG):     /* Addition (long) */
                  for ( ; odb<odbe; odb+=LSIZE) {
                     long *s1 = (long *)(pcpk->buffw + odb);
                     long *s2 = (long *)(pcpk->buffr + odb);
                     *s1 = *s1 + *s2;
                     }
                  break;

               case (ADD+INT):      /* Addition (int) */
                  for ( ; odb<odbe; odb+=ISIZE) {
                     int *s1 = (int *)(pcpk->buffw + odb);
                     int *s2 = (int *)(pcpk->buffr + odb);
                     *s1 = *s1 + *s2;
                     }
                  break;

               case (ADD+SHORT):    /* Addition (short) */
                  for ( ; odb<odbe; odb+=HSIZE) {
                     short *s1 = (short *)(pcpk->buffw + odb);
                     short *s2 = (short *)(pcpk->buffr + odb);
                     *s1 = *s1 + *s2;
                     }
                  break;

               case (ADD+FLOAT):    /* Addition (float) */
                  for ( ; odb<odbe; odb+=ESIZE) {
                     float *s1 = (float *)(pcpk->buffw + odb);
                     float *s2 = (float *)(pcpk->buffr + odb);
                     *s1 = *s1 + *s2;
                     }
                  break;

               case (ADD+DOUBLE):   /* Addition (double) */
                  for ( ; odb<odbe; odb+=DSIZE) {
                     double *s1 = (double *)(pcpk->buffw + odb);
                     double *s2 = (double *)(pcpk->buffr + odb);
                     *s1 = *s1 + *s2;
                     }
                  break;

               case (ADD+LLONG):    /* Addition (si64) */
                  for ( ; odb<odbe; odb+=WSIZE) {
                     si64 *s1 = (si64 *)(pcpk->buffw + odb);
                     si64 *s2 = (si64 *)(pcpk->buffr + odb);
                     *s1 = jasw(*s1, *s2);
                     }
                  break;

               case (ADD+SI32V):    /* Addition (si32) */
                  for ( ; odb<odbe; odb+=I32SIZE) {
                     si32 *s1 = (si32 *)(pcpk->buffw + odb);
                     si32 *s2 = (si32 *)(pcpk->buffr + odb);
                     *s1 = *s1 + *s2;
                     }
                  break;

               case (ADD+UI32V):    /* Addition (ui32) */
                  for ( ; odb<odbe; odb+=I32SIZE) {
                     ui32 *s1 = (ui32 *)(pcpk->buffw + odb);
                     ui32 *s2 = (ui32 *)(pcpk->buffr + odb);
                     *s1 = *s1 + *s2;
                     }
                  break;

               case (MAX+LONG):     /* Max (long) */
                  for ( ; odb<odbe; odb+=LSIZE) {
                     long *s1 = (long *)(pcpk->buffw + odb);
                     long *s2 = (long *)(pcpk->buffr + odb);
                     if (*s2 > *s1) *s1 = *s2;
                     }
                  break;

               case (MAX+INT):      /* Max (int) */
                  for ( ; odb<odbe; odb+=ISIZE) {
                     int *s1 = (int *)(pcpk->buffw + odb);
                     int *s2 = (int *)(pcpk->buffr + odb);
                     if (*s2 > *s1) *s1 = *s2;
                     }
                  break;

               case (MAX+SHORT):    /* Max (short) */
                  for ( ; odb<odbe; odb+=HSIZE) {
                     short *s1 = (short *)(pcpk->buffw + odb);
                     short *s2 = (short *)(pcpk->buffr + odb);
                     if (*s2 > *s1) *s1 = *s2;
                     }
                  break;

               case (MAX+FLOAT):    /* Max (float) */
                  for ( ; odb<odbe; odb+=ESIZE) {
                     float *s1 = (float *)(pcpk->buffw + odb);
                     float *s2 = (float *)(pcpk->buffr + odb);
                     if (*s2 > *s1) *s1 = *s2;
                     }
                  break;

               case (MAX+DOUBLE):   /* Max (double) */
                  for ( ; odb<odbe; odb+=DSIZE) {
                     double *s1 = (double *)(pcpk->buffw + odb);
                     double *s2 = (double *)(pcpk->buffr + odb);
                     if (*s2 > *s1) *s1 = *s2;
                     }
                  break;

               case (MAX+LLONG):    /* Max (si64) */
                  for ( ; odb<odbe; odb+=WSIZE) {
                     si64 *s1 = (si64 *)(pcpk->buffw + odb);
                     si64 *s2 = (si64 *)(pcpk->buffr + odb);
                     if (qsw(jrsw(*s2,*s1)) > 0) *s1 = *s2;
                     }
                  break;

               case (MAX+SI32V):    /* Max (si32) */
                  for ( ; odb<odbe; odb+=I32SIZE) {
                     si32 *s1 = (si32 *)(pcpk->buffw + odb);
                     si32 *s2 = (si32 *)(pcpk->buffr + odb);
                     if (*s2 > *s1) *s1 = *s2;
                     }
                  break;

               case (MAX+UI32V):    /* Max (ui32) */
                  for ( ; odb<odbe; odb+=I32SIZE) {
                     ui32 *s1 = (ui32 *)(pcpk->buffw + odb);
                     ui32 *s2 = (ui32 *)(pcpk->buffr + odb);
                     if (*s2 > *s1) *s1 = *s2;
                     }
                  break;

               case (MIN+LONG):     /* Min (long) */
                  for ( ; odb<odbe; odb+=LSIZE) {
                     long *s1 = (long *)(pcpk->buffw + odb);
                     long *s2 = (long *)(pcpk->buffr + odb);
                     if (*s2 < *s1) *s1 = *s2;
                     }
                  break;

               case (MIN+INT):      /* Min (int) */
                  for ( ; odb<odbe; odb+=ISIZE) {
                     int *s1 = (int *)(pcpk->buffw + odb);
                     int *s2 = (int *)(pcpk->buffr + odb);
                     if (*s2 < *s1) *s1 = *s2;
                     }
                  break;

               case (MIN+SHORT):    /* Min (short) */
                  for ( ; odb<odbe; odb+=HSIZE) {
                     short *s1 = (short *)(pcpk->buffw + odb);
                     short *s2 = (short *)(pcpk->buffr + odb);
                     if (*s2 < *s1) *s1 = *s2;
                     }
                  break;

               case (MIN+FLOAT):    /* Min (float) */
                  for ( ; odb<odbe; odb+=ESIZE) {
                     float *s1 = (float *)(pcpk->buffw + odb);
                     float *s2 = (float *)(pcpk->buffr + odb);
                     if (*s2 < *s1) *s1 = *s2;
                     }
                  break;

               case (MIN+DOUBLE):   /* Min (double) */
                  for ( ; odb<odbe; odb+=DSIZE) {
                     double *s1 = (double *)(pcpk->buffw + odb);
                     double *s2 = (double *)(pcpk->buffr + odb);
                     if (*s2 < *s1) *s1 = *s2;
                     }
                  break;

               case (MIN+LLONG):    /* Min (si64) */
                  for ( ; odb<odbe; odb+=WSIZE) {
                     si64 *s1 = (si64 *)(pcpk->buffw + odb);
                     si64 *s2 = (si64 *)(pcpk->buffr + odb);
                     if (qsw(jrsw(*s2,*s1)) < 0) *s1 = *s2;
                     }
                  break;

               case (MIN+SI32V):    /* Min (si32) */
                  for ( ; odb<odbe; odb+=I32SIZE) {
                     si32 *s1 = (si32 *)(pcpk->buffw + odb);
                     si32 *s2 = (si32 *)(pcpk->buffr + odb);
                     if (*s2 < *s1) *s1 = *s2;
                     }
                  break;

               case (MIN+UI32V):    /* Min (ui32) */
                  for ( ; odb<odbe; odb+=I32SIZE) {
                     ui32 *s1 = (ui32 *)(pcpk->buffw + odb);
                     ui32 *s2 = (ui32 *)(pcpk->buffr + odb);
                     if (*s2 < *s1) *s1 = *s2;
                     }
                  break;

               case (NOOP+LONG):    /* Noop (long) */
               case (NOOP+INT):     /* Noop (int) */
               case (NOOP+SHORT):   /* Noop (short) */
               case (NOOP+FLOAT):   /* Noop (float) */
               case (NOOP+DOUBLE):  /* Noop (double) */
               case (NOOP+LLONG):   /* Noop (si64) */
               case (NOOP+SI32V):   /* Noop (si32) */
               case (NOOP+UI32V):   /* Noop (ui32) */
                  odb += incr;
                  break;

               case (LOR+LONG):     /* Logical OR (long) */
                  for ( ; odb<odbe; odb+=LSIZE) {
                     long *s1 = (long *)(pcpk->buffw + odb);
                     long *s2 = (long *)(pcpk->buffr + odb);
                     *s1 |= *s2;
                     }
                  break;

               case (LOR+INT):      /* Logical OR (int) */
                  for ( ; odb<odbe; odb+=ISIZE) {
                     int *s1 = (int *)(pcpk->buffw + odb);
                     int *s2 = (int *)(pcpk->buffr + odb);
                     *s1 |= *s2;
                     }
                  break;

               case (LOR+SHORT):    /* Logical OR (short) */
                  for ( ; odb<odbe; odb+=HSIZE) {
                     short *s1 = (short *)(pcpk->buffw + odb);
                     short *s2 = (short *)(pcpk->buffr + odb);
                     *s1 |= *s2;
                     }
                  break;

               /* LOR on float or double is not meaningful */
               case (LOR+FLOAT):    /* Logical OR (float) */
               case (LOR+DOUBLE): abexit(28);

               case (LOR+LLONG):    /* Logical OR (si64) */
                  for ( ; odb<odbe; odb+=WSIZE) {
                     si64 *s1 = (si64 *)(pcpk->buffw + odb);
                     si64 *s2 = (si64 *)(pcpk->buffr + odb);
#ifdef HAS_I64
                     *s1 |= *s2;
#else
                     s1->hi |= s2.hi, s1->lo |= s2.lo;
#endif
                     }
                  break;

               case (LOR+SI32V): /* Logical OR (si32) */
                  for ( ; odb<odbe; odb+=I32SIZE) {
                     si32 *s1 = (si32 *)(pcpk->buffw + odb);
                     si32 *s2 = (si32 *)(pcpk->buffr + odb);
                     *s1 |= *s2;
                     }
                  break;

               case (LOR+UI32V): /* Logical OR (ui32) */
                  for ( ; odb<odbe; odb+=I32SIZE) {
                     ui32 *s1 = (ui32 *)(pcpk->buffw + odb);
                     ui32 *s2 = (ui32 *)(pcpk->buffr + odb);
                     *s1 |= *s2;
                     }
                  break;

               }  /* End of optype switch */
            }  /* End odb loop over buffer contents */
         } /* End else on a read channel */
      } /* End of loop over channels */

/* At this point, data combination for this buffer load is complete.
*  Results must be distributed to caller's data arrays at this time,
*  because collection buffers may be reused for more data.  If node
*  0 was not included in the combination, now copy data from node 1
*  to node 0.  */

   if (!inc0) {
      if (NC.node == NC.headnode) {
         rc = MPI_Send(pcpk->buffw, pcpk->odat, MPI_UNSIGNED_CHAR,
            NC.hostid, pcpk->type, NC.commc);
         if (rc) appexit("COPCOMB: MPI_Send error", CPK_MPIWRE, rc);
         }
      else {
         rc = MPI_Recv(pcpk->buffw, pcpk->odat, MPI_UNSIGNED_CHAR,
            NC.headnode, pcpk->type, NC.commc, MPI_STATUS_IGNORE);
         if (rc) appexit("COPCOMB: MPI_Recv error", CPK_MPIRDE, rc);
         }
      }

   /* Move data back to caller */
   if (NC.node == NC.hostid) {
      byte *dest;          /* Output data pointer */
      int ipal,jpal;       /* Index of dest address */
      int ityp;            /* Data type code from COPD */

      /* Set to use first user data address */
      ipal = 0;
      dest = pcpk->pal[ipal];
      odb = 0;

      while (odb < pcpk->odat) {
         memcpy(&tcop, pcpk->buffw+odb, sizeof(COPD));
         odb += sizeof(COPD);
         jpal = tcop.optype >> RepShft;
         if (jpal != ipal)
            dest = pcpk->pal[ipal = jpal];
         ityp = tcop.optype & TPMSK;
         incr = tcop.count * typlen[ityp];
         odb = odup(odb, typlen[ityp]);
         memcpy(dest, pcpk->buffw+odb, incr);
         dest += incr;
         odb += incr;
         }
      }

   /* Set for next coppack call to start a new buffer */
SetForNextBuffer:
   pcpk->alused = pcpk->odat = 0;

   } /* End of copcomb() */



/*=====================================================================*
*                               copfree                                *
*=====================================================================*/

void copfree(CPKD *pcpk) {

   if (pcpk->flags & CPK_FREED) appexit(
      "COPFREE: Tried to free CPKDEF already freed", CPK_EFREED, 0);
   pcpk->flags = CPK_FREED;
   freev(pcpk->pal, "coppack data ptrs");
   freev(pcpk->buffw, "coppack buffers");
   freev(pcpk, "CPKDEF");
   }  /* End copfree() */
