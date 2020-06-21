/* (c) Copyright 1991-2016, The Rockefeller University *21115* */
/* $Id: nnput.c 4 2017-02-03 18:57:19Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                      nnput - node-to-node put                        *
*                                                                      *
*  This routine is part of the mpi hybrid tools collection.  Working   *
*  with nnget, it provides a mechanism for two nodes to exchange data  *
*  in buffered streams.  Buffers are allocated dynamically in message  *
*  buffer space and are of length MAX_MSG_LENGTH (see mpitools.h).     *
*  Message sending is always prompted by the receiver to avoid buffer  *
*  bottlenecks.  As many streams as will fit may be created, as long   *
*  as an NNSTR structure and a distinctive message type are provided   *
*  for each (the same message type is used for prompting and for data  *
*  transmission).  Calls are provided to create, write, writep, and    *
*  destroy streams:                                                    *
*                                                                      *
*  void nnpcr(struct NNSTR *pnn, int node, int type)                   *
*     Initializes structure '*pnn' to send data to node 'node'         *
*     of type 'type'.  The structure must be provided by the caller.   *
*  void nnput(struct NNSTR *pnn, void *item, int ilen)                 *
*     Using the buffer, node, and type specified in '*pnn', sends      *
*     'ilen' bytes of data from 'item'.  Terminates if unsuccessful.   *
*  char *nnputp(struct NNSTR *pnn, int ilen)                           *
*     For stream pnn, returns a pointer to a buffer area able to hold  *
*     'ilen' bytes of data.  Terminates execution if 'ilen' is larger  *
*     than MAX_MSG_LENGTH or if an I/O error occurs.                   *
*  void nnpsk(struct NNSTR *pnn, int jlen)                             *
*     Registers storage of 'jlen' bytes of data on stream 'pnn' in     *
*     area provided by most recent call to nnputp().  Terminates       *
*     execution if an I/O error occurs or if 'jlen' exceeds 'ilen'.    *
*  void nnpfl(struct NNSTR *pnn)                                       *
*     Flush stream defined in '*pnn', writing last buffer.             *
*     Terminates execution if unsuccessful.                            *
*  void nnpcl(struct NNSTR *pnn)                                       *
*     Closes stream defined in '*pnn', writing and freeing last buffer.*
*     Terminates execution if unsuccessful.                            *
************************************************************************
*  V1A, 11/27/91, GNR - Initial version                                *
*  V2A, 12/14/92, ABP - Add function nnpfl and support for non nCube   *
*  V2B, 11/16/96, GNR - Revise conditional comp for addtl hybrid vers. *
*  V2C, 10/24/98, GNR - Add functions nnputp and nnpsk                 *
*  ==>, 12/29/09, GNR - Last mod before committing to svn repository   *
*  Rev, 05/23/16, GNR - Only comment mods for MPI                      *
*  Rev, 07/17/16ff, GNR - Get rid of anreadxx, anwritexx use MPI       *
*  ==>, 09/08/16, GNR - Last mode before committing to svn mpi repo    *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "mpitools.h"

/* Define NNPDBG for debug output */
/*** #define NNPDBG ***/

/*---------------------------------------------------------------------*
*                                nnpcr                                 *
*---------------------------------------------------------------------*/

void nnpcr(struct NNSTR *pnn, int node, int type) {

   /* Store destination node and message type for use by nnput */
   pnn->nnode = node;
   pnn->type = type;
   pnn->buff = pnn->bptr = malloc(MAX_MSG_LENGTH);
   pnn->lbsr = MAX_MSG_LENGTH;

   } /* End nnpcr() */

/*---------------------------------------------------------------------*
*                                nnpfl                                 *
*                                                                      *
*  N.B.  In case of a broadcast stream, MPI_Bcast requires that all    *
*  nodes have the same count, which may not be available on comp nodes *
*  in our protocols.  Therefore, we first broadcast the count, then    *
*  the actual data.  Eliminating this extra broadcast is a target for  *
*  future optimization.  Also, we perform an isynch() before anything  *
*  is sent to be sure all nodes are ready to receive the same data, as *
*  there are no tags in MPI_Bcast.  (The isynch() may or may not be    *
*  provided with the MPI_Bcast function and there is no obvious way to *
*  find out.)                                                          *
*                                                                      *
*  In the broadcast case, we do the length exchange even if the length *
*  is zero, because the comp nodes may not be aware of this until they *
*  get the prompt.  Non-broadcast exchanges are skipped w/zero count.  *
*---------------------------------------------------------------------*/

void nnpfl(struct NNSTR *pnn) {

   int dest = pnn->nnode;     /* Destination node */
   int count;
   int prompt;
   int rc;

#ifdef NNPDBG
   dbgprt(ssprintf(NULL,"nnpfl entered, lbsr = %d, nnode = %d, "
      "type = %d, flags = %d", pnn->lbsr, dest, pnn->type, pnn->flags));
#endif

   /* If buffer was freed, allocate another one -- this is for compat
   *  with any code that calls nngcl() before the end of exection.  */
   if (!pnn->buff) {
      pnn->buff = pnn->bptr = malloc(MAX_MSG_LENGTH);
      pnn->lbsr = MAX_MSG_LENGTH;
      }

   /* If this is a broadcast, code is same on PAR0 and PARn nodes--
   *  Traditionally, only PAR0 is the sender, but this code should
   *  work for other cases by changing the NC.hostid in the calls.  */
   count = MAX_MSG_LENGTH - pnn->lbsr;
   if (count <= 0) goto ResetBuffer;

   if (dest == BCSTADDR) {
      int rc;
      rc = MPI_Barrier(NC.commc);
#ifdef NNPDBG
      dbgprt("nnpfl bcst returned from barrier");
#endif
      if (rc) appexit("NNPFL: MPI_Barrier ERROR", 168, rc);
      rc = MPI_Bcast(&count, 1, MPI_INT, NC.hostid, NC.commc);
      if (rc) appexit("NNPFL: MPI_Bcast COUNT ERROR", 168, rc);
      if (count > 0) {
#ifdef NNPDBG
         dbgprt(ssprintf(NULL,"nnpfl at MPI_Bcast, buff = %p, "
            "count = %d", pnn->buff, count));
#endif
         rc = MPI_Bcast(pnn->buff, count, MPI_UNSIGNED_CHAR,
            NC.hostid, NC.commc);
         if (rc) appexit("NNPFL: MPI_Bcast DATA ERROR", 168, rc);
         }
      }
   else {                     /* Not a broadcast */
      /* Wait for prompt from partner */
      rc = MPI_Recv(&prompt, 1, MPI_INT, pnn->nnode, pnn->type,
         NC.commc, MPI_STATUS_IGNORE);
      if (rc) appexit("NNPFL: MPI_Recv PROMPT", 48, rc);
#ifdef NNPDBG
   dbgprt("nnpfl nonbcst received prompt");
#endif
      rc = MPI_Send(pnn->buff, count, MPI_UNSIGNED_CHAR,
         pnn->nnode, pnn->type, NC.commc);
      if (rc) appexit("NNPFL: MPI_Send ERROR", 49, rc);
      }
#ifdef NNPDBG
   dbgprt(ssprintf(NULL, "nnpfl sent %d of type %d from %d",
      count, pnn->type, dest));
#endif
   /* Perform the flush that is the function of this routine */
ResetBuffer:
   pnn->bptr = pnn->buff;
   pnn->lbsr = MAX_MSG_LENGTH;
   } /* End nnpfl() */

/*---------------------------------------------------------------------*
*                                nnput                                 *
*                                                                      *
*  Check amount of space in current buffer.  If not sufficient, copy   *
*  partial data to buffer, wait for prompt from receiver, and send it. *
*  Then copy remainder of data from caller to buffer and advance bptr. *
*  Terminate execution on any kind of error.                           *
*---------------------------------------------------------------------*/

void nnput(struct NNSTR *pnn, void *item, int ilen) {

   char *pitem = (char *)item;   /* Avoid incrementing void ptr */
   int jlen;                     /* Length of actual move */

   while (ilen) {
      /* Move lesser of data requested or space in buffer */
      jlen = (ilen < pnn->lbsr) ? ilen : pnn->lbsr;
      memcpy((char *)pnn->bptr, pitem, jlen);
      pitem += jlen;
      pnn->bptr += jlen;
      ilen -= jlen;
      /* If buffer is full, wait for prompt and write it now */
      if ((pnn->lbsr -= jlen) <= 0) nnpfl(pnn);
      } /* End while ilen */

   } /* End nnput() */

/*---------------------------------------------------------------------*
*                               nnputp                                 *
*                                                                      *
*  Check amount of space in current buffer.  If not sufficient, flush  *
*  buffer and get another.  Return buffer address to caller.           *
*  Terminate execution on any kind of error.                           *
*---------------------------------------------------------------------*/

char *nnputp(struct NNSTR *pnn, int ilen) {

   /* Otherwise, if data will not fit in current buffer, flush it.  */
   if (ilen > pnn->lbsr) nnpfl(pnn);
   /* If there still isn't enough room, terminate.  */
   if (ilen > pnn->lbsr) abexit(47);
   return (char *)pnn->bptr;
   } /* End nnputp() */

/*---------------------------------------------------------------------*
*                                nnpsk                                 *
*---------------------------------------------------------------------*/

void nnpsk(struct NNSTR *pnn, int jlen) {

   /* If space skipped is larger than buffer, terminate */
   if (jlen > pnn->lbsr) abexit(47);
   /* Register the space */
   pnn->bptr += jlen;
   /* If buffer is full, wait for prompt and write it now */
   if ((pnn->lbsr -= jlen) <= 0) nnpfl(pnn);
   } /* End nnpsk() */

/*---------------------------------------------------------------------*
*                                nnpcl                                 *
*---------------------------------------------------------------------*/

void nnpcl(struct NNSTR *pnn) {

   if (pnn->buff) {
      nnpfl(pnn);
      free(pnn->buff);
      pnn->buff = pnn->bptr = NULL;
      pnn->lbsr = MAX_MSG_LENGTH;
      }

   } /* End nnpcl() */

