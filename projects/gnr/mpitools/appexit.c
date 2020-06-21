/* (c) Copyright 1993-2016, The Rockefeller University *21115* */
/* $Id: appexit.c 3 2017-01-16 19:29:37Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                       appexit.c (MPI version)                        *
*                                                                      *
* Synopsis: void appexit(char *txt, int err, int ival);                *
*                                                                      *
* Arguments:  txt - Optional error message.                            *
*             err - Error code.                                        *
*             ival - Integer value, specific to the error.             *
*                                                                      *
*  Description:                                                        *
*  In this MPI version, we are spawning a separate dmsgid process with *
*  a separate intercommunicator, NC.commd.  Error code 0 indicates     *
*  normal termination.  This is expected to be called from all nodes   *
*  and each node performs an MPI_Finalize().  Otherwise, there is an   *
*  error.  From all nodes (even Node 0 to assure same treatment) the   *
*  error information is sent to andmsg with tag INIT_ABORT_MSG.  This  *
*  process is expected to print the error on stderr, then initiate an  *
*  MPI_Abort.  If the DBG_NOXIT bit (2) is set in the node's NC.debug  *
*  flag, the message is not sent to andmsg, but the node enters an     *
*  infinite wait state.  This facilitates debugging, eliminating the   *
*  need to set breakpoints on all nodes.                               *
*                                                                      *
************************************************************************
*  Written by Ariel Ben-Porath                                         *
*  V1A, 07/29/93 Initial version                                       *
*  Rev, 03/28/98, GNR - Use ssprintf instead of fprintf, fix comments  *
*  Rev, 05/04/00, GNR - Add test of NC.debug flag                      *
*  ==>, 12/29/09, GNR - Last mod before committing to svn repository   *
*  Rev, 04/23/16. GNR - Change arg types & order so pointer is first   *
*  ==>, 10/19/16, GNR - Last mod before committing to svn mpi repo     *
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include "sysdef.h"
#include "mpitools.h"
#include "rksubs.h"

void appexit(char *txt, int err, int ival) {

   struct {
      struct ErrMsg Err;
      char msg[MAX_EMSG_LENGTH];
      } emsg;
   int rc, stl;

   if (err == 0) {
      int probeflag;
      AN_Status mstat;
#ifdef APPXDBG
      dbgprt("Arrived at appexit with err==0");
#endif
      while (1) {
         int msrc, mtag;
         rc = MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, NC.commc,
            &probeflag, &mstat);
#ifdef APPXDBG
         fputs(ssprintf(NULL, "Node %d did Iprobe, rc = %d, flag = %d\n",
            NC.node, rc, probeflag), stderr);
#endif
         if (!probeflag) break;
         msrc = mstat.MPI_SOURCE, mtag = mstat.MPI_TAG;
         rc = MPI_Recv(emsg.msg, 1, MPI_UNSIGNED_CHAR, msrc, mtag,
            NC.commc, MPI_STATUS_IGNORE);
#ifdef APPXDBG
         fputs(ssprintf(NULL, "Receive rc = %d\n", rc), stderr);
#endif
         }
      rc = MPI_Send(&ival, 1, MPI_INT, NC.dmsgid,
         SHUTDOWN_ANDMSG, NC.commd);
#ifdef APPXDBG
      fputs(ssprintf(NULL, "Node %d sent SHUTDOWN, rc = %d\n", NC.node, rc), stderr);
      fflush(stderr);
#endif
      /* See note in andmsg.c regarding why this MPI_Recv is here */
      if (is_host(NC.node)) { MPI_Recv(&ival, 1, MPI_INT, NC.dmsgid,
         CLOSING_ANDMSG, NC.commd, MPI_STATUS_IGNORE);
#ifdef APPXDBG
         fputs(ssprintf(NULL, "Node %d recvd CLOSING, rc = %d\n", NC.node, rc), stderr);
         fflush(stderr);
#endif
         }
      rc = MPI_Barrier(NC.commc);
#ifdef APPXDBG
      fputs(ssprintf(NULL, "Node %d back from barrier, rc = %d\n", NC.node, rc), stderr);
      fflush(stderr);
#endif
      /* Tell andmsg it is OK to disconnect and shut down */
      if (is_host(NC.node)) {
         ival = SHUTDOWN_ANDMSG;
         rc = MPI_Send(&ival, 1, MPI_INT, NC.dmsgid,
            SHUTDOWN_ANDMSG, NC.commd);
#ifdef APPXDBG
         fputs(ssprintf(NULL, "Node %d sent 2nd SHUTDOWN, rc = %d\n", NC.node, rc), stderr);
         fflush(stderr);
#endif
         }
      rc = MPI_Comm_disconnect(&NC.commd);
#ifdef APPXDBG
      fputs(ssprintf(NULL, "Node %d disconnect, rc = %d\n", NC.node, rc), stderr);
      fflush(stderr);
#endif
      if (NC.debug & DBG_NOXIT) {
         volatile int wct = 1;
         while (wct) {
            sleep(1);
            }
         }
      rc = MPI_Finalize();
      exit(0);
      }

   stl = txt ? strnlen(txt, MAX_EMSG_LENGTH-1) : 0;
   emsg.Err.ec = err;
   emsg.Err.src = NC.node;
   emsg.Err.iv = ival;
   emsg.Err.ltxt = stl;
   if (stl) memcpy(emsg.msg, txt, stl);
   emsg.msg[stl] = '\0';
   MPI_Send(&emsg, sizeof(struct ErrMsg)+1+stl, MPI_UNSIGNED_CHAR,
      NC.dmsgid, INIT_ABORT_MSG, NC.commd);

   /* Wait for debugger or just exit */
   if (NC.debug & DBG_NOXIT) {
      volatile int wct = 1;
      while (wct) {
         sleep(1);
         }
      }
   sleep(1);
   MPI_Finalize();
   exit(err);

   } /* End appexit() */
