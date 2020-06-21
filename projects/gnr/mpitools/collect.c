/* (c) Copyright 1989-2018, The Rockefeller University *11115* */
/* $Id: collect.c 3 2017-01-16 19:29:37Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                             vecollect()                              *
*                                                                      *
*  Routine for parallel computers to perform global combination of     *
*     message in 'mypart', the results of which end up in 'mypart'     *
*     on node 0.                                                       *
*                                                                      *
*  Call:                                                               *
*  void vecollect(char *mypart, struct COPDEF *ops, int opcnt,         *
*            int type, int datlen)                                     *
*                                                                      *
*  Arguments:                                                          *
*  mypart -- Pointer to contribution of current node to global vector. *
*                                                                      *
*  ops    -- Pointer to array of COPDEF structures.  This routine      *
*            passes 'ops' to vecombine() to specify the manner in      *
*            which contributions are to be combined.                   *
*                                                                      *
*  opcnt  -- Number of entries of 'ops' to be used.                    *
*                                                                      *
*  type   -- Type assigned to messages sent to accomplish the global   *
*            combination.                                              *
*                                                                      *
*  datlen -- Length in bytes of vector collection data--used to        *
*            allocate temporary read buffer.  Collections larger than  *
*            2**31 bytes must be broken up by caller.                  *
*                                                                      *
*  Notes:  - Each node must call this routine with MYPART pointing     *
*            to contribution of that node to the global data vector.   *
*            When routine finishes, MYPART will point to the assembled *
*            global vector on node zero.  Node 0 may (in CNS) make no  *
*            contribution, but in each cycle will collect from other   *
*            nodes whose numbers are powers of two.  If file output is *
*            from a separate host node, ordinary nnput/nnget can be    *
*            used to send results from node 0 to that host.            *
*                                                                      *
************************************************************************
*  Rev, 05/29/94, GNR - Use nsitools swap support                      *
*  Rev, 11/16/96, GNR - Remove non-hybrid support                      *
*  Rev, 05/18/99, GNR - Use new swapping scheme                        *
*  ==>, 12/29/09, GNR - Last mod before committing to svn repository   *
*  Rev, 05/15/12, GNR - Add codes for si32,ui32 variables              *
*  Rev, 05/23/13, GNR - Revise COPDEF to allow 2-byte repeat counts    *
*  Rev, 05/25/16, GNR - Change compile-time EXTNODE test to run time.  *
*                       Eliminate separate code for ring topology.     *
*                       Modify hypercube case for any number of nodes. *
*  ==>, 06/23/16, GNR - Last mod before committing to mpi svn repo     *
*  R10, 08/26/18, GNR - Eliminate anread/anwrite/anrelp and neg datlen *
***********************************************************************/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "mpitools.h"
#include "collect.h"
#include "rkarith.h"
#include "rksubs.h"
#include "swap.h"

void vecollect(char *mypart, struct COPDEF *ops, int opcnt, int type,
      int datlen) {

   char *part = NULL;
   int chan;
   int mynode = rnode(NC.node);
   int nnodes = NC.cnodes + 1;      /* One more for host */
   int rc;                          /* Return code */

   if (datlen <= 0) abexit(198);

   /* If mynode has at least one zero in the low-order bit, it is
   *  going to be reading from a partner at least once, and so a
   *  buffer of length datlen is allocated and kept until the end.
   *  This is more efficient than the old code using anreadp, which
   *  deallocated the part buffer after each use.  */
   if (!(mynode & 1))
      part = mallocv((size_t)datlen, "vecollect");

   for (chan=1; chan<nnodes; chan<<=1) {
      int pard = mynode ^ chan;
      int next = anode(pard);

      if (mynode & chan) {
         /* Nodes with odd numbers in the current channel send to
         *  their next lower partner, then drop out.  */
         rc = MPI_Send(mypart, datlen, MPI_UNSIGNED_CHAR, next, type,
            NC.commc);
         if (rc) mpiwex(type, next, rc, "vecollect");
         break;
         }

       else if (pard < nnodes) {
         /* Nodes with even numbers in the current channel read a
         *  message from the partner node (unless the partner is above
         *  nnodes and therefore does not exist) and combine the
         *  contribution read into 'mypart'.  'mypart' is retained
         *  until sent (on a higher channel).  In the case of node 0,
         *  'mypart' is never sent and is the place in which the final
         *  global statistics end up at the end of this loop.  */
         rc = MPI_Recv(part, datlen, MPI_UNSIGNED_CHAR, next, type,
            NC.commc, MPI_STATUS_IGNORE);
         if (rc) mpirex(type, next, rc, "vecollect");
         vecombine(mypart, part, ops, opcnt);
         }
      } /* End of loop over channels */

   if (part) freev(part, "vecollect");

   } /* End of vecollect() */


/***********************************************************************
*                                                                      *
*  vecombine -- Combine vectors.                                       *
*                                                                      *
*  Call:                                                               *
*  void vecombine(void *dest,void *src,struct COPDEF *ops,int opcnt)   *
*                                                                      *
*  Arguments:                                                          *
*  dest  -- Pointer to destination vector.                             *
*                                                                      *
*  src   -- Pointer to source array (unchanged by this call).          *
*                                                                      *
*  ops   -- Pointer to array of COPDEF structs which specify the       *
*           manner in which to combine the vectors.                    *
*                                                                      *
*  opcnt -- Maximum number of entries of 'ops' to be used.             *
*                                                                      *
*  The vector pointed to by 'src' is combined into the vector pointed  *
*     to by 'dest' under control of the 'ops' array.  This code makes  *
*     the historical assumption that message byte order and alignment  *
*     of items are same on all comp processor nodes.  If this ever     *
*     changes, code must be rewritten to use lemfm/bemfm routines or   *
*     MPI library functions with similar semantics.                    *
*  All types except UI32V are assumed to be signed statistics.         *
*  'RCG' below refers to "repeating COPDEF group"                      *
*                                                                      *
************************************************************************
*  Rev, 04/05/91, GNR - Add LOR operator                               *
*  Rev, 06/07/03, GNR - Add addition of long long                      *
*  Rev, 05/15/12, GNR - Add si32 variables, si64 max and min           *
*  Rev, 06/27/12, GNR - Add ui32 variables                             *
***********************************************************************/

void vecombine(void *dest, void *src, struct COPDEF *ops, int opcnt) {

   struct COPDEF *op,   /* Current COPDEF ptr  */
      *opstart = ops,   /* Starting COPDEF ptr */
      *opstop;          /* Terminal COPDEF ptr */
   int repcnt = 1;      /* # of passes over current RCG */
   int cnt = opcnt;     /* # COPDEFs in current RCG     */
   int nextop = opcnt;  /* Index of first COPDEF after current RCG */
   int i, repi;         /* Inner, outer loop counters */

/* Loop over current RCG requested number of times */

REP_LOOP:
   for (repi=0; repi<repcnt; repi++) {

      /* Loop over operation control structures and move along the
      *  source and destination arrays while performing requested
      *  operations. */
      opstop = opstart + cnt;
      for (op=opstart; op<opstop; op++) {

         /* Switch according to operation type */
         switch (op->optype & 077) {

            case REPEAT:         /* Repeat following operations */
               repcnt = op->count;     /* # of times to repeat RCG */
               cnt = op->optype >> RepShft;  /* # of COPDEFS in RCG */
               opstart = ++op;         /* Inc to first RCG COPDEF  */
               /* Record index into COPDEF array of where to resume
               *  processing after RCG done =
               *  (# COPDEFS processed so far) + (# COPDEFS in RCG) */
               nextop = (op - ops) + cnt;
               goto REP_LOOP;

            case (ADD+LONG):     /* Addition (long)   */   {
               long *d = dest, *s = src;
               for (i=0; i<op->count; i++) *d++ += *s++;
               dest = d; src = s;
               }
               break;

            case (ADD+INT):      /* Addition (int)    */   {
               int *d = dest, *s = src;
               for (i=0; i<op->count; i++) *d++ += *s++;
               dest = d; src = s;
               }
               break;

            case (ADD+SHORT):    /* Addition (short)  */   {
               short *d = dest, *s = src;
               for (i=0; i<op->count; i++) *d++ += *s++;
               dest = d; src = s;
               }
               break;

            case (ADD+FLOAT):    /* Addition (float)  */   {
               float *d = dest, *s = src;
               for (i=0; i<op->count; i++) *d++ += *s++;
               dest = d; src = s;
               }
               break;

            case (ADD+DOUBLE):   /* Addition (double) */   {
               double *d = dest, *s = src;
               for (i=0; i<op->count; i++) *d++ += *s++;
               dest = d; src = s;
               }
               break;

            case (ADD+LLONG):    /* Addition (si64)   */   {
               si64 *d = dest, *s = src;
               for (i=0; i<op->count; i++,d++,s++) *d = jasw(*d,*s);
               dest = d; src = s;
               }
               break;

            case (ADD+SI32V):    /* Addition (si32)   */   {
               si32 *d = dest, *s = src;
               for (i=0; i<op->count; i++) *d++ += *s++;
               dest = d; src = s;
               }
               break;

            case (ADD+UI32V):    /* Addition (ui32)   */   {
               ui32 *d = dest, *s = src;
               for (i=0; i<op->count; i++) *d++ += *s++;
               dest = d; src = s;
               }
               break;

            case MAX+LONG:       /* Max (long)   */    {
               long *d = dest, *s = src;
               for (i=0; i<op->count; i++,d++,s++) *d = max(*d,*s);
               dest = d; src = s;
               }
               break;

            case MAX+INT:        /* Max (int)    */    {
               int *d = dest, *s = src;
               for (i=0; i<op->count; i++,d++,s++) *d = max(*d,*s);
               dest = d; src = s;
               }
               break;

            case MAX+SHORT:      /* Max (short)  */    {
               short *d = dest, *s = src;
               for (i=0; i<op->count; i++,d++,s++) *d = max(*d,*s);
               dest = d; src = s;
               }
               break;

            case MAX+FLOAT:      /* Max (float)  */    {
               float *d = dest, *s = src;
               for (i=0; i<op->count; i++,d++,s++) *d = max(*d,*s);
               dest = d; src = s;
               }
               break;

            case MAX+DOUBLE:     /* Max (double) */    {
               double *d = dest, *s = src;
               for (i=0; i<op->count; i++,d++,s++) *d = max(*d,*s);
               dest = d; src = s;
               }
               break;

            case MAX+LLONG:      /* Max (si64)   */    {
               si64 *d = dest, *s = src;
               for (i=0; i<op->count; i++,d++,s++)
                  if (qsw(jrsw(*d,*s)) < 0) *d = *s;
               dest = d; src = s;
               }
               break;

            case MAX+SI32V:      /* Max (si32)   */    {
               si32 *d = dest, *s = src;
               for (i=0; i<op->count; i++,d++,s++) *d = max(*d,*s);
               dest = d; src = s;
               }
               break;

            case MAX+UI32V:      /* Max (ui32)   */    {
               ui32 *d = dest, *s = src;
               for (i=0; i<op->count; i++,d++,s++) *d = max(*d,*s);
               dest = d; src = s;
               }
               break;

            case MIN+LONG:       /* Min (long)   */    {
               long *d = dest, *s = src;
               for (i=0; i<op->count; i++,d++,s++) *d = min(*d,*s);
               dest = d; src = s;
               }
               break;

            case MIN+INT:        /* Min (int)    */    {
               int *d = dest, *s = src;
               for (i=0; i<op->count; i++,d++,s++) *d = min(*d,*s);
               dest = d; src = s;
               }
               break;

            case MIN+SHORT:      /* Min (short)  */    {
               short *d = dest, *s = src;
               for (i=0; i<op->count; i++,d++,s++) *d = min(*d,*s);
               dest = d; src = s;
               }
               break;

            case MIN+FLOAT:      /* Min (float)  */    {
               float *d = dest, *s = src;
               for (i=0; i<op->count; i++,d++,s++) *d = min(*d,*s);
               dest = d; src = s;
               }
               break;

            case MIN+DOUBLE:     /* Min (double) */    {
               double *d = dest, *s = src;
               for (i=0; i<op->count; i++,d++,s++) *d = min(*d,*s);
               dest = d; src = s;
               }
               break;

            case MIN+LLONG:      /* Min (si64)   */    {
               si64 *d = dest, *s = src;
               for (i=0; i<op->count; i++,d++,s++)
                  if (qsw(jrsw(*d,*s)) >= 0) *d = *s;
               dest = d; src = s;
               }
               break;

            case MIN+SI32V:      /* Min (si32)   */    {
               si32 *d = dest, *s = src;
               for (i=0; i<op->count; i++,d++,s++) *d = min(*d,*s);
               dest = d; src = s;
               }
               break;

            case MIN+UI32V:      /* Min (ui32)   */    {
               ui32 *d = dest, *s = src;
               for (i=0; i<op->count; i++,d++,s++) *d = min(*d,*s);
               dest = d; src = s;
               }
               break;

            case NOOP+LONG:      /* Noop (long)   */
               dest = (long *)dest + op->count;
               src  = (long *)src  + op->count;
               break;

            case NOOP+INT:       /* Noop (int)    */
               dest = (int *)dest + op->count;
               src  = (int *)src  + op->count;
               break;

            case NOOP+SHORT:     /* Noop (short)  */
               dest = (short *)dest + op->count;
               src  = (short *)src  + op->count;
               break;

            case NOOP+FLOAT:     /* Noop (float)  */
               dest = (float *)dest + op->count;
               src  = (float *)src  + op->count;
               break;

            case NOOP+DOUBLE:    /* Noop (double) */
               dest = (double *)dest + op->count;
               src  = (double *)src  + op->count;
               break;

            case NOOP+LLONG:     /* Noop (si64)   */
               dest = (si64 *)dest + op->count;
               src  = (si64 *)src  + op->count;
               break;

            case NOOP+SI32V:     /* Noop (si32)   */
               dest = (si32 *)dest + op->count;
               src  = (si32 *)src  + op->count;

            case NOOP+UI32V:     /* Noop (ui32)   */
               dest = (ui32 *)dest + op->count;
               src  = (ui32 *)src  + op->count;

            case (LOR+LONG):  /* Logical OR (long)   */  {
               long *d = dest, *s = src;
               for (i=0; i<op->count; i++) *d++ |= *s++;
               dest = d; src = s;
               }
               break;

            case (LOR+INT):   /* Logical OR (int)    */  {
               int *d = dest, *s = src;
               for (i=0; i<op->count; i++) *d++ |= *s++;
               dest = d; src = s;
               }
               break;

            case (LOR+SHORT): /* Logical OR (short)  */  {
               short *d = dest, *s = src;
               for (i=0; i<op->count; i++) *d++ |= *s++;
               dest = d; src = s;
               }
               break;

            /* LOR on float or double is not meaningful */
            case (LOR+FLOAT): /* Logical OR (float) */
            case (LOR+DOUBLE):   abexit(28);

            case (LOR+LLONG): /* Logical OR (si64) */    {
               si64 *d = dest, *s = src;
               for (i=0; i<op->count; i++,d++,s++) {
#ifdef HAS_I64
                  *d |= *s;
#else
                  d.hi |= s.hi, d.lo |= s.lo;
#endif
                  }
               }
               break;

            case (LOR+SI32V): /* Logical OR (si32) */    {
               si32 *d = dest, *s = src;
               for (i=0; i<op->count; i++) *d++ |= *s++;
               dest = d; src = s;
               }
               break;

            case (LOR+UI32V): /* Logical OR (ui32) */    {
               ui32 *d = dest, *s = src;
               for (i=0; i<op->count; i++) *d++ |= *s++;
               dest = d; src = s;
               }
               break;

               }  /* End of optype switch */

            }  /* End of loop over COPDEFs */

        } /* End of outer loop */

/* If REPEAT op used, may still be more COPDEFS to process... */

   if (cnt = (opcnt - nextop))  {   /* Assignment intended */
      repcnt = 1;     /* Castrate outer loop (reset from REPEAT) */
      nextop = opcnt; /* COPDEFS done sequentially until next REPEAT */
      opstart = op;   /* Resume processing at first COPDEF after RCG */
      goto REP_LOOP;
      }

   } /* End of vecombine() */

