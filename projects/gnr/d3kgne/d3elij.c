/* (c) Copyright 1991-2010, Neurosciences Research Foundation, Inc. */
/* $Id: d3elij.c 30 2010-07-09 21:26:56Z  $ */
/***********************************************************************
*                            CNS Program                               *
*                          d3elij routines                             *
*                                                                      *
*     This file provides the routines needed by d3genr to read Lij     *
*  and Cij values from an external file.  To simplify sharing file     *
*  and nnio descriptors, open/close functions are provided even        *
*  though they are trivial.                                            *
*     elijopen()     Opens CONNLIST file and, if parallel, reads file  *
*                       and passes Lij's to comp nodes (call from se-  *
*                       rial or host parallel code once per celltype). *
*     elijopnn()     Opens stream from host (call from comp node).     *
*     d3elij()       Returns one Lij value (serial or comp node) and   *
*                       stores Cij, which may or may not actually be   *
*                       there, in Lija.cijext for pick up by gcije().  *
*     elijclos()     Close CONNLIST file or stream from host node      *
*                       (call from all nodes, serial or parallel).     *
*                                                                      *
*     CONNLIST files are assumed to be big-endian and to contain       *
*  ECLREC structures sorted by target cell and conntype (struct ECLREC *
*  is defined in lijarg.h and described in d3grp3.memo).  There may be *
*  more or fewer than ix->nc connections in the input file for any one *
*  cell.  Excess connections are silently discarded.  An Lij equal to  *
*  either -1 or to the lijlim value defined for the connection type    *
*  causes that one connection to be skipped (i.e. the Cij that would   *
*  have been generated is absorbed).  Out-of-bounds connections cause  *
*  execution to be terminated--otherwise would have to restrict to     *
*  Class II lb2 routines.  Program that prepares the data file can     *
*  always detect these and send -1 code if necessary.                  *
*     External Lij's are stored in memory even if OPTIMIZE=STORAGE     *
*  has been specified--basically because reading the file repeatedly   *
*  would slow things down too much.  Also, if conntype evaluation      *
*  order is changed for voltage-dependent connections, special code    *
*  would be needed to seek back and forth in CONNLIST files to the     *
*  required conntypes.                                                 *
*     I/O errors terminate execution via d3exit or abexit--no error    *
*  code is returned.                                                   *
*     In the parallel version, the strategy used is that the code on   *
*  host examines the cell numbers found in the input file and sends    *
*  only the actual Lij (and Cij if needed) values (in blocks of size   *
*  MAX_MSG_LENGTH) to the node where the target cell is found.  If a   *
*  cell has fewer than the expected number of Lij's, the list for that *
*  cell is ended with a value of -2.  If the file has too many ECLREC  *
*  records, the extra ones are skipped on the host and never sent to   *
*  the comp node. If a node has no cells on it, nothing is sent to it. *
*     RESTRICTION:  The current version requires that there be exact-  *
*  ly one input file for each cell type that has one or more external  *
*  connection types.  The files are opened and closed in turn by       *
*  d3genr.  Files will be processed even if all conntypes have NOPOR   *
*  bit set.  It is permitted to use any third-letter kgen code in      *
*  conjunction with external connections, including "I", which causes  *
*  all ix->nc connections to be external, except that if Cij is also   *
*  being read (KGEN=2), 'I' is required.  This is handled in d3genr.   *
*                                                                      *
*  V5E, 07/22/92, GNR and KJF - Initial version                        *
*  Rev, 10/20/92, GNR - Make CONNLISTs little-endian on all systems    *
*  Rev, 12/09/92, ABP - Add byte swapping for hybrid host              *
*  Rev, 03/04/93, GNR - Add lijxyr call for subarbor support, add code *
*                       to convert SKIP_ONE to SKIP_TYP if not EI      *
*  V6C, 08/14/93, GNR - Remove 2**n dependency                         *
*  V8A, 11/27/96, GNR - Remove support for non-hybrid version          *
*  Rev, 09/28/97, GNR - Independent dynamic allocations per conntype,  *
*                       back to big-endian on all systems.             *
*  Rev, 05/31/99, GNR - Use new swapping scheme                        *
*  V8C, 07/21/03, GNR - Change d3elij return type to int               *
*  ==>, 10/31/07, GNR - Last mod before committing to svn repository   *
*  Rev, 09/26/08, GNR - Minor type changes for 64-bit compilation      *
*  V8F, 06/05/10, GNR - Add reading of Cij for KGEN=2                  *
***********************************************************************/

#define LIJTYPE  struct LIJARG

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "d3global.h"
#include "d3fdef.h"

/* Define Lij values for encoding skipping in nnio */
#define ELIJ_SKIP_ONE  -1
#define ELIJ_SKIP_ALL  -2

/* Common data */
#ifndef PARn
static struct RFdef *eclpf;   /* Ptr to external connection file */
static int eclrr;             /* Reread flag */
#endif
#ifdef PAR
static struct NNSTR eclnnio;  /* Node-to-node data stream */
#endif
extern struct LIJARG Lija;

void lijxyr(struct CONNTYPE *ix);

/***********************************************************************
*                          elijopen/elijopnn                           *
*                                                                      *
*  Synopsis: void elijopen(struct CELLTYPE *il, struct RFdef *fptr)    *
*                                                                      *
*  On host or serial system, opens external file specified by fptr.    *
*  On host node of parallel system, reads the file and sends the data  *
*  to the appropriate comp nodes for storage.                          *
*                                                                      *
*  Synopsis: void elijopnn(void)                                       *
*                                                                      *
*  On comp nodes, opens the input data stream from the host node.      *
*                                                                      *
*  These routines should be called once per cell layer at the start of *
*  cell layer processing, but only if at least one conntype has an     *
*  external connection list.                                           *
***********************************************************************/

#ifndef PARn
void elijopen(struct CELLTYPE *il, struct RFdef *fptr) {

#ifdef PAR0
   struct CONNTYPE *ix;       /* Ptr to current conntype */
   struct ECLREC cld;         /* Connection list data record */
   long ncr;                  /* Cells remaining on current node */
   long icell,ictyp,iconn;    /* Cell, conntype, conn loop counts */
   si32 lij;                  /* Value of Lij sent to comp node */
   int node;                  /* Relative destination node */
   si16 cij;                  /* Value of Cij sent to comp node */
   char IAinpt;               /* Flags input from input array */
   char Cinput;               /* Flags input of Cij with Lij */
#endif

/*---------------------------------------------------------------------*
*                   Open the external connlist file                    *
*---------------------------------------------------------------------*/

   rfopen((eclpf = fptr),NULL,READ,SAME,SEQUENTIAL,TOP,LOOKAHEAD,
      REWIND,RELEASE_BUFF,SAME,SAME,SAME,ABORT);
   eclrr = FALSE;

/*---------------------------------------------------------------------*
*                 Read the Lij data from external file                 *
*                                                                      *
*  Note:  By convention, if KGEN=2, comp node will expect to receive   *
*  Cij value even if this is a skipped connection.  It should read     *
*  only one record if the ELIJ_SKIP_ALL code is sent.                  *
*---------------------------------------------------------------------*/

#ifdef PAR0

   /* Loop over nodes and cells, sending Lij values */
   for (node=0,icell=0; node<il->nodes; node++) {

      /* Calculate number of cells on node (break when = 0) */
      if (!(ncr = il->cpn + (node < il->crn))) break;

      /* Open stream to current node */
      nnpcr(&eclnnio,node+il->node1,ELIJ_MSG);

      /* Loop over cells, connection types, and connections */
      for (; ncr--; icell++) {

         for (ix=il->pct1,ictyp=0; ix; ix=ix->pct) {
            if (!(ix->kgen & KGNEX)) continue;
            ++ictyp;          /* Increment conntype number */
            if (!(ix->Cn.cnopt & NOPOR)) continue;
            IAinpt = (ix->kgfl & KGNIA) != 0;
            Cinput = (ix->kgen & KGNE2) != 0;
            for (iconn=0; iconn<ix->nc; ) {
               /* Read a data record and swap bytes if needed */
               if (!eclrr) {
                  cld.xjcell = rfri4(eclpf);
                  if (eclpf->lbsr != ATEOF) {
                     cld.xicell = rfri4(eclpf);
                     cld.ecij   = rfri2(eclpf);
                     cld.jct    = rfri2(eclpf);
                     }
                  }
               else eclrr = FALSE;

               /* Handle matching input records */
               if ((cld.xicell == icell) && (cld.jct == ictyp)) {
                  lij = cld.xjcell;
                  /* If in range and input is from IA,
                  *  convert Lij to IA offset.  */
                  if (lij < ix->srcnelt)
                     { if (IAinpt) lij += (lij & (-RP->nsy)); }
                  /* If is skip code, skip this connection */
                  else if ((lij == ix->lijlim) || (lij == -1))
                     lij = ELIJ_SKIP_ONE;
                  /* Otherwise, external Lij is invalid, terminate */
                  else
                     d3exit(LIJBDV_ERR, fmturlnm(il), (long)ix->ict);
                  /* Send Lij value to corresponding node */
                  nnputi4(&eclnnio, lij);
                  if (Cinput) nnputi2(&iclnnio, cld.ecij);
                  ++iconn;    /* Increment connection number */
                  } /* End processing matching record */

               /* If input record belongs to a later cell or connec-
               *  tion type, set to reread it and send skip code. */
               else if ((cld.xicell > icell) ||
                     ((cld.xicell == icell) && (cld.jct > ictyp))) {
                  eclrr = TRUE;
                  lij = ELIJ_SKIP_ALL;
                  nnputi4(&eclnnio, lij);
                  if (Cinput) nnputi2(&iclnnio, cld.ecij);
                  break;
                  }

               /* Otherwise, input record is an extranumerary one,
               *  so just continue loop and read another */
               } /* End connection loop */
            } /* End connection type loop */
         } /* End cell loop */
      nnpcl(&eclnnio);        /* Close stream */
      } /* End node loop */
#endif
   } /* End elijopen */

#else                         /* Parallel comp node */
void elijopnn(void) {
   nngcr(&eclnnio,NC.hostid,ELIJ_MSG);
   } /* End elijopnn */
#endif

/***********************************************************************
*                              elijclos                                *
*                                                                      *
*  Synopsis: void elijclos(void)                                       *
*                                                                      *
*  Call on all nodes in serial or parallel systems at end of cell      *
*  layer, but only if at least one conntype has external connlist.     *
***********************************************************************/

void elijclos(void) {

#ifndef PARn
   rfclose(eclpf,SAME,SAME,ABORT);
#else
   nngcl(&eclnnio);
#endif
   } /* End elijclos */


/***********************************************************************
*                               d3elij                                 *
*                                                                      *
*  Synopsis:  int d3elij(struct CONNTYPE *ix)                                         *
*                                                                      *
*  Function is called once for each connection via lijbr mechanism     *
*  to return Lij.  Receives from host (parallel) or reads directly     *
*  from CONNLIST file (serial).  Not called on parallel host node.     *
***********************************************************************/

int d3elij(struct CONNTYPE *ix) {

/*---------------------------------------------------------------------*
*        Serial: Read the Lij data directly from external file         *
*---------------------------------------------------------------------*/

#ifndef PAR
   struct ECLREC cld;         /* Connection file record structure */

/* Read data records (and swap bytes) until one is accepted */

   for (;;) {

      if (!eclrr) {
         Lija.lijval = (long)rfri4(eclpf);
         /* If at end-of-file, return no connection */
         if (eclpf->lbsr == ATEOF) {
            Lija.lijval = ix->lijlim;
            return NO;
            }
         else {
            cld.xicell  = rfri4(eclpf);
            Lija.cijext = rfri2(eclpf);
            cld.jct     = rfri2(eclpf);
            }
         }
      else eclrr = FALSE;

      /* Handle matching input records */
      if ((cld.xicell == Lija.kcell) && (cld.jct == Lija.jctype)) {
         /* If is skip code, skip this connection or type */
         if ((Lija.lijval == ix->lijlim) || (Lija.lijval == -1))
            return NO;
         /* If in range and is first connection, decode Lij.
         *  If input is from IA, convert Lij to IA offset.  */
         else if (Lija.lijval < ix->srcnelt) {
            if (ix->kgfl & KGNIA)
               Lija.lijval += (Lija.lijval & (-RP->nsy));
            else if (Lija.isyn == 0) lijxyr(ix);
            return YES; }
         /* Otherwise, external Lij is invalid, terminate job */
         else d3exit(LIJBDV_ERR, fmturlnm(Lija.kl), (long)ix->ict);
         } /* End processing matching record */

      /* If input record belongs to a later cell or connection
      *  type, set to reread it and send skip code. */
      else if ((cld.xicell > Lija.kcell) ||
            ((cld.xicell == Lija.kcell) && (cld.jct > Lija.jctype))) {
         eclrr = TRUE;
         return NO;
         }
      /* Otherwise, input record is an extranumerary one,
      *  so just continue loop and read another */
      } /* End read loop */
#endif

/*---------------------------------------------------------------------*
*            parallel: get the Lij data from host node                 *
*---------------------------------------------------------------------*/

#ifdef PARn
   Lija.lijval = nngeti4(&eclnnio);
   if (ix->kgen & KGNE2) Lija.cijext = nngeti2(&eclnnio);

   switch (Lija.lijval) {

   case ELIJ_SKIP_ONE:        /* Skip one or all connections */
   case ELIJ_SKIP_ALL:        /* Skip rest of conntype */
      return NO;

   default:                   /* Store connection */
      if (Lija.isyn == 0) lijxyr(ix);
      return YES;
      } /* End switch */
#endif

   } /* End d3elij() */
