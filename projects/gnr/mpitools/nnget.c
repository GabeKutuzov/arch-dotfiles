/* (c) Copyright 1991-2016, The Rockefeller University *21115* */
/* $Id: nnget.c 3 2017-01-16 19:29:37Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                      nnget - node-to-node get                        *
*                                                                      *
*  This routine is part of the mpi hybrid tools collection.  Working   *
*  with nnput, it provides a mechanism for two nodes to exchange data  *
*  in buffered streams.  Buffers are allocated dynamically in message  *
*  buffer space and are of length MAX_MSG_LENGTH (see mpitools.h).     *
*  Message sending is always prompted by the receiver to avoid buffer  *
*  bottlenecks.  As many streams as will fit may be created, as long   *
*  as an NNSTR structure and a distinctive message type are provided   *
*  for each (the same message type is used for prompting and for data  *
*  transmission).  Calls are provided to create, read, readp, skip     *
*  data, and destroy streams:                                          *
*                                                                      *
*  void nngcr(struct NNSTR *pnn, int node, int type)                   *
*     Initializes structure '*pnn' to receive data from node 'node'    *
*     of type 'type'.  The structure must be provided by the caller.   *
*  void nnget(struct NNSTR *pnn, void *item, int ilen)                 *
*     Using the buffer, node, and type specified in '*pnn', gets       *
*     'ilen' bytes of data into 'item'.  Terminates if unsuccessful.   *
*  int nngetp(struct NNSTR *pnn, byte **pbfr)                          *
*     For stream pnn, returns a pointer to current buffered data in    *
*     'pbfr' and length of that data as function value.  If no data    *
*     buffered up, prompts sender and reads another buffer.            *
*  void nngsk(struct NNSTR *pnn, int ilen)                             *
*     Skips over 'ilen' bytes of data on stream 'pnn'.  May be called  *
*     after nngetp to report the amount of data processed.  Terminates *
*     execution if not successful.                                     *
*  void nngcl(struct NNSTR *pnn)                                       *
*     Closes stream defined in '*pnn', freeing any buffer space.       *
************************************************************************
*  V1A, 11/27/91, GNR - Initial version                                *
*  V2A, 12/15/92, ABP - Add support for non nCube systems              *
*  V2B, 11/16/96, GNR - Revise conditional comp for addtl hybrid vers. *
*  V2C, 07/25/98, GNR - Add internal termination on errors             *
*  ==>, 12/29/09, GNR - Last mod before committing to svn repository   *
*  Rev, 05/02/16, GNR - Comment cleanups MPI                           *
*  Rev, 07/17/16ff, GNR - Get rid of anreadxx, anwritexx use MPI       *
*  ==>, 09/08/16, GNR - Last mod before committing to svn mpi repo     *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "mpitools.h"
#include "rksubs.h"

/* Define NNGDBG for debug output */
/*** #define NNGDBG ***/

/*---------------------------------------------------------------------*
*                               nngetb                                 *
*  Routine to prompt sender and read a buffer.  This routine is        *
*  internal and should not be called by application routines.          *
*                                                                      *
*  N.B.  In case of a broadcast stream, MPI_Bcast requires that all    *
*  nodes have the same count, which may not be available on comp nodes *
*  in our protocols.  Therefore, we first broadcast the count, then    *
*  the actual data.  Eliminating this extra broadcast is a target for  *
*  future optimization.  Also, we perform an MPI_Barrier() before any- *
*  thing is sent to be sure all nodes are ready to receive the same    *
*  data, as there are no tags in MPI_BCast.  (The barrier may or may   *
*  not be provided with the MPI_Bcast function and there is no obvious *
*  way to find out.)                                                   *
*---------------------------------------------------------------------*/

static int nngetb(struct NNSTR *pnn) {

   AN_Status rstat;
   int count;
   int rc;
   int src = pnn->nnode;      /* Source node */

   /* If buffer was freed, allocate another one -- this is for compat
   *  with any code that calls nngcl() before the end of exection.  */
   if (!pnn->buff) {
      pnn->buff = pnn->bptr = mallocv(MAX_MSG_LENGTH,"NNGCR");
      pnn->lbsr = 0;
#ifdef NNGDBG
      dbgprt(ssprintf(NULL,"nngetb entered w/no buff, lbsr<--0, "
         "nnode = %d, type = %d, flags = %d", pnn->nnode, pnn->type,
         pnn->flags));
      }
   else {
      dbgprt(ssprintf(NULL,"nngetb entered w/buff, lbsr = %d, "
         "nnode = %d, type = %d, flags = %d", pnn->lbsr, pnn->nnode,
         pnn->type, pnn->flags));
#endif
      }


   /* If this is a broadcast, code is same on PAR0 and PARn nodes--
   *  Traditionally, only PARn is the receiver, but this code should
   *  work for other cases by changing the NC.hostid in the calls.  */
   if (src == BCSTADDR) {
      rc = MPI_Barrier(NC.commc);
#ifdef NNGDBG
   dbgprt("nngetb bcst returned from barrier");
#endif
      if (rc) appexit("NNGETB: MPI_Barrier ERROR", 168, rc);
      rc = MPI_Bcast(&count, 1, MPI_INT, NC.hostid, NC.commc);
      if (rc) appexit("NNGETB: MPI_Bcst COUNT, ERROR", 168, rc);
      if (count > 0) {
#ifdef NNGDBG
         dbgprt(ssprintf(NULL,"nngetb at MPI_Bcast, buff = %p, "
            "count = %d", pnn->buff, count));
#endif
         rc = MPI_Bcast(pnn->buff, count, MPI_UNSIGNED_CHAR,
            NC.hostid, NC.commc);
         if (rc) appexit("NNGETB: MPI_Bcst DATA, ERROR", 168, rc);
         }
      }
   else {                     /* Not a broadcast */
      /* Prompt sender */
      count = MAX_MSG_LENGTH; /* Any old prompt currently will do */
      rc = MPI_Send(&count, 1, MPI_INT, pnn->nnode, pnn->type,
         NC.commc);
      if (rc) appexit("NNGETB: MPI_Send PROMPT", 49, rc);
#ifdef NNGDBG
   dbgprt("nngetb nonbcst sent prompt");
#endif
      rc = MPI_Recv(pnn->buff, MAX_MSG_LENGTH, MPI_UNSIGNED_CHAR,
         pnn->nnode, pnn->type, NC.commc, &rstat);
      if (rc) appexit("NNGETB: MPI_Recv ERROR", 48, rc);
      MPI_Get_count(&rstat, MPI_UNSIGNED_CHAR, &count);
      }
#ifdef NNGDBG
   dbgprt(ssprintf(NULL, "nngetb received %d of type %d from %d",
      count, pnn->type, src));
#endif
   pnn->bptr = pnn->buff;
   pnn->lbsr = count;
   return rc;
   } /* End nngetb() */


/*---------------------------------------------------------------------*
*                                nngcr                                 *
*---------------------------------------------------------------------*/

void nngcr(struct NNSTR *pnn, int node, int type) {

   /* Store source node and message type for use by nnget */
   pnn->nnode = node;
   pnn->type = type;
   pnn->buff = pnn->bptr = mallocv(MAX_MSG_LENGTH,"NNGCR");
   pnn->lbsr = 0;
   } /* End nngcr() */

/*---------------------------------------------------------------------*
*                                nnget                                 *
*                                                                      *
*  Check amount of data in current buffer.  If not sufficient, copy    *
*  what there is to caller, prompt for another, and read it.  Then     *
*  copy remainder of data requested by caller and advance pointer.     *
*  Terminate execution on any type of error.                           *
*---------------------------------------------------------------------*/

void nnget(struct NNSTR *pnn, void *item, int ilen) {

   char *pitem = (char *)item;   /* Avoid incrementing void ptr */
   int jlen;                     /* Length of actual move */

   while (ilen) {

      /* If current buffer is empty, send prompt
      *  and receive another full or partial buffer */
      if (!pnn->lbsr) nngetb(pnn);
      /* Move lesser of data requested or data in buffer */
      jlen = (ilen < pnn->lbsr) ? ilen : pnn->lbsr;
      memcpy(pitem, (char *)pnn->bptr, jlen);
      pitem += jlen;
      pnn->bptr += jlen;
      ilen -= jlen;
      pnn->lbsr -= jlen;

      } /* End while ilen */

   } /* End nnget() */

/*---------------------------------------------------------------------*
*                               nngetp                                 *
*                                                                      *
*  If no current buffer exists, prompt sender for another and read it. *
*  Return address of buffer in pbfr and length of data as fn value.    *
*---------------------------------------------------------------------*/

int nngetp(struct NNSTR *pnn, void **pbfr) {

   /* If current buffer is empty, send prompt
   *  and receive another full or partial buffer */
   if (!pnn->lbsr) nngetb(pnn);
   /* Return requested info */
   *pbfr = pnn->bptr;
   return pnn->lbsr;
   } /* End nngetp() */

/*---------------------------------------------------------------------*
*                                nngsk                                 *
*                                                                      *
*  Skip over 'ilen' bytes of data on stream 'pnn'.  May be called      *
*  after nngetp to report the amount of data processed.  Terminate     *
*  execution on any type of error.                                     *
*---------------------------------------------------------------------*/

void nngsk(struct NNSTR *pnn, int ilen) {

   int jlen;               /* Length of actual skip */

   while (ilen) {

      /* If current buffer is empty, send prompt
      *  and receive another full or partial buffer */
      if (!pnn->lbsr) nngetb(pnn);
      /* Skip lesser of data requested or data in buffer */
      jlen = (ilen < pnn->lbsr) ? ilen : pnn->lbsr;
      pnn->bptr += jlen;
      ilen -= jlen;
      pnn->lbsr -= jlen;

      } /* End while ilen */

   } /* End nngsk() */

/*---------------------------------------------------------------------*
*                                nngcl                                 *
*---------------------------------------------------------------------*/

void nngcl(struct NNSTR *pnn) {

   /* If a buffer has been allocated, release it.
   *  Assume bookkeeping is OK, so there will be no errors.  */
   if (pnn->buff) {
      freev(pnn->buff, "NNGCL");
      pnn->buff = pnn->bptr = NULL;
      pnn->lbsr = 0;
      }
   } /* End nngcl() */
