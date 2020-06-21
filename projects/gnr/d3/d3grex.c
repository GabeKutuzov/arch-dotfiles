/* (c) Copyright 2017, The Rockefeller University *21115* */
/* $Id: d3grex.c 71 2017-02-03 19:42:45Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                        Graceful exit routine                         *
*                                                                      *
*      d3grex - Shutdown parallel computation with message info        *
*                                                                      *
*  This routine may be called by any node that wants to terminate      *
*  execution at the next rendezvous point (end of a simulation trial)  *
*  with an error message to the user and if possible statistics and    *
*  closure of grafdata, simdata, and other files.  In order to allow   *
*  the node code to progress to the rendezvous, d3grex sets an outnow  *
*  flag (serial computer) or sends a message to the host node to set   *
*  that flag (parallel computer) and returns.                          *
*                                                                      *
*  Arguments to d3grex are:                                            *
*     (1) A pointer to a string with error information, or NULL.       *
*         Meaning is specific to the particular error.                 *
*         Usually points to an identifying string that will be         *
*         incorporated in the text of the error message.               *
*     (2) An integer error code (documented in d3global.h)             *
*     (3) An integer containing information specific to the            *
*         particular error.                                            *
*                                                                      *
*  CNS now has provision for two kinds of error exits:                 *
*  -- d3grex (this code) can be called when the nature of the error    *
*     does not preclude continuing to the next rendezvous point, where *
*     the error is recognized and CNS is terminated with cleanup.      *
*  -- d3exit (the other code) should be called when the state of CNS   *
*     may not support continuing to rendezvous.  In this case, termi-  *
*     nation is immediate.  In serial versions, d3exit completes the   *
*     error exit after calling d3term() to perform file and graphics   *
*     cleanup.  On a parallel computer, MPI_Abort is called and no     *
*     attempt is made to perform cleanup.                              *
*                                                                      *
*----------------------------------------------------------------------*
*    d3grout - Perform output for d3grex exits at termination time     *
*                                                                      *
*  This routine is called from the main program when stored info for   *
*  d3grex output is detected.  There are no arguments.                 *
************************************************************************
*  R70, 01/21/17, GNR - New code                                       *
*  ==>, 01/21/17, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "sysdef.h"
#include "mpitools.h"
#include "rksubs.h"
#include "d3global.h"

/* Prototype d3perr */
void d3perr(struct exit_msg *EMSG, int source);

/*=====================================================================*
*                               d3grex                                 *
*=====================================================================*/

void d3grex(char *mtxt, int ierr, int ival) {

#ifdef PAR           /*** Parallel d3grex ***/
   /* Inasmuch as this call will happen at most a few times per run,
   *  we use the somewhat inefficient method of sending two messages,
   *  one with the error code and length info, then the message text.
   *  Both can go to the host node, where they will be enqueued until
   *  received at rendezvous time.  Multiple messages may come from
   *  multiple nodes--should be no problem if not too many of them.
   *  And MPI allows host node to send messages to itself.  */
   struct ErrMsg mgrem;
   int rc;
   mgrem.ec = ierr;
   mgrem.src = NC.node;
   mgrem.iv = ival;
   mgrem.ltxt = mtxt ? strlen(mtxt) : 0;
   rc = MPI_Send(&mgrem, sizeof(mgrem), MPI_UNSIGNED_CHAR,
      NC.hostid, GREX_HDR_MSG, NC.commc);
   if (rc) abexitm(49, ssprintf(NULL, "d3grex: MPI_Send "
         "GREX_HDR_MSG TO HOST GAVE ERROR %d.", rc));
   if (mtxt) {
      rc = MPI_Send(mtxt, mgrem.ltxt, MPI_UNSIGNED_CHAR,
         NC.hostid, GREX_DATA_MSG, NC.commc);
      if (rc) abexitm(49, ssprintf(NULL, "d3grex: MPI_Send "
         "GREX_DATA_MSG TO HOST GAVE ERROR %d.", rc));
      }
   /* This will be picked up at RENDEZVOUS_2 by coppack code */
   RP->CP.outnow |= OUT_GRACEFUL_EXIT;
   return;

#else                /*** Serial d3grex ***/
   /* These cases can be handled without sending any messages.
   *  Messages are printed right away (so we don't have to queue
   *  them up) and RP->CP.outnow is set to stop the code at the
   *  rendezvous.  */
   struct exit_msg grem;
   grem.pit = mtxt;
   grem.ec = ierr;
   grem.iv = ival;
   d3perr(&grem, 0);
   RP->CP.outnow |= OUT_GRACEFUL_EXIT;
   return;

#endif
   } /* End d3grex() */

/*=====================================================================*
*                               d3grout                                *
*=====================================================================*/

#ifdef PAR0
void d3grout(void) {

   char   *pgrmtxt = NULL;    /* Allocated space for message texts */
   struct exit_msg grem;
   struct ErrMsg mgrem;
   MPI_Status sgrx;
   int flag, msrc, mlen;

   while (1) {
      int rc = MPI_Iprobe(MPI_ANY_SOURCE, GREX_HDR_MSG, NC.commc,
         &flag, &sgrx);
      if (rc) abexitm(169, ssprintf(NULL,
         "MPI_Iprobe FOR GREX_HDR_MSG ON ANY SOURCE RETURNED "
            "ERROR CODE %d.", rc));
      if (!flag) break;
      /* Got one, read it and print it.  */
      msrc = sgrx.MPI_SOURCE;
      rc = MPI_Recv(&mgrem, sizeof(mgrem), MPI_UNSIGNED_CHAR,
         msrc, GREX_HDR_MSG, NC.commc, &sgrx);
      if (rc) abexitm(48, ssprintf(NULL, "d3grex: MPI_Recv "
         "GREX_HDR_MSG ON HOST GAVE ERROR %d.", rc));
      mlen = mgrem.ltxt;
      if (mlen > 0) {
         pgrmtxt = reallocv(pgrmtxt, mlen+1, "Grex Text");
         rc = MPI_Recv(pgrmtxt, mlen, MPI_UNSIGNED_CHAR,
            msrc, GREX_DATA_MSG, NC.commc, &sgrx);
         if (rc) abexitm(48, ssprintf(NULL, "d3grex: MPI_Recv "
            "GREX_DATA_MSG ON HOST GAVE ERROR %d.", rc));
         pgrmtxt[mlen] = '\0';
         grem.pit = pgrmtxt;
         }
      else
         grem.pit = NULL;
      grem.ec = mgrem.ec;
      grem.iv = mgrem.iv;
      d3perr(&grem, mgrem.src);
      }

   } /* End d3grout() */
#endif
