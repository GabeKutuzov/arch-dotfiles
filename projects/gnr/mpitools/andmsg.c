/* (c) Copyright 2016-2018, The Rockefeller University *11114* */
/* $Id: andmsg.c 6 2018-02-01 20:18:47Z  $ */
/***********************************************************************
*                              andmsg.c                                *
*                                                                      *
*  This is the main program spawned by a parallel program using the    *
*  mpitools library to receive "out-of-stream" messages and act upon   *
*  them.  These include debug output and may include termination error *
*  messages and other messages to be added as needed.  This program    *
*  should not be accessed by users directly.  The executable should    *
*  be stored in the same directory as the user program, or somewhere   *
*  available via a PATH location.                                      *
*                                                                      *
*  This program should receive (as ASCII strings) two command-line     *
*  arguments from aninit in the parent (spawning) process:             *
*  (1) The number of SHUTDOWN_ANDMSG messages needed to quit           *
*  (2) Any debug codes relevant to this process                        *
************************************************************************
*  V1A, 07/06/16, GNR - New program                                    *
*  Rev, 10/22/16, GNR - Add specific messages for CNS gconn debugging  *
*  Rev, 01/29/18, GNR - Add DBG_ERLOG code, write debugs to andlog     *
***********************************************************************/

#define MAIN
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "sysdef.h"
#include "mpitools.h"
#include "swap.h"

/*** For shutdown info, pound-define ANDDBG ***/


/*=====================================================================*
*                             andmsg main                              *
*=====================================================================*/

int main(int argc, char *argv[]) {

   FILE *andlog;           /* Debug log file */
   struct {                /* Struct for receiving messages */
      struct ErrMsg Err;
      char msg[MAX_EMSG_LENGTH];
      } emsg;
   MPI_Status mstat;
   int msrc, mtag;
   int sdmsg;
   int num2stop;

/* Startup */

   aninit(0, 0, 0);
   MPI_Comm_get_parent(&NC.commd);
   /* Interpret command-line arguments */
   if (argc > 1) NC.debug = atoi(argv[2]);
   if (argc > 0) num2stop = atoi(argv[1]);
#ifdef ANDDBG
   NC.debug |= DBG_ERLOG;
#endif
   if (NC.debug & (DBG_START|DBG_ERLOG))
      andlog = fopen("/var/tmp/andmsg.debug.log", "w");
   else
      andlog = stderr;

   if (NC.debug & DBG_START) {
      fputs(ssprintf(NULL,"andmsg started w/num2stop = %d, "
         "NC.debug = 0x%x\n", num2stop, NC.debug), andlog);
      fflush(andlog);
      }

/* Loop waiting for messages */

   while (1) {
      /* Terminate if all nodes have finished */
      if (num2stop <= 0) {
         int rc;
#ifdef ANDDBG
         fputs("Andmsg got num2stop == 0\n", andlog);
         fflush(andlog);
#endif
         /* Without this last ack, kept getting uninterpretable errors
         *  when some node called MPI_Finalize.  These errors did not
         *  happen if all nodes sat on a DBG_NOXIT and were stepped
         *  through MPI_Finalize() individually.  -GNR */
         rc = MPI_Send(&num2stop, 1, MPI_INT, NC.hostid,
            CLOSING_ANDMSG, NC.commd);
#ifdef ANDDBG
         fputs(ssprintf(NULL, "Andmsg sent CLOSING, rc = %d\n", rc), andlog);
         fflush(andlog);
#endif
         rc = MPI_Recv(&sdmsg, 1, MPI_INT, NC.hostid, MPI_ANY_TAG, NC.commd,
            MPI_STATUS_IGNORE);
#ifdef ANDDBG
         fputs(ssprintf(NULL, "Andmsg recvd 2nd CLOSING, rc = %d\n", rc), andlog);
         fflush(andlog);
#endif
         sleep(1);
         rc = MPI_Comm_disconnect(&NC.commd);
#ifdef ANDDBG
         fputs(ssprintf(NULL, "Andmsg disconnected, rc = %d\n", rc), andlog);
         fflush(andlog);
#endif
         rc = MPI_Finalize();
#ifdef ANDDBG
         fputs(ssprintf(NULL, "Andmsg finalized, rc = %d\n", rc), andlog);
         fflush(andlog);
         fclose(andlog);
#endif
         exit(0);
         }

      /* Block until a message arrives */
      MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, NC.commd, &mstat);
      msrc = mstat.MPI_SOURCE, mtag = mstat.MPI_TAG;

      switch (mstat.MPI_TAG) {
      /* Handle debug output */
      case DEBUG_MSG:
         MPI_Recv(emsg.msg, MAX_EMSG_LENGTH, MPI_UNSIGNED_CHAR,
            msrc, mtag, NC.commd, &mstat);
         fputs(ssprintf(NULL, "DBG MSG from Node %d: ",
            mstat.MPI_SOURCE), andlog);
         emsg.msg[MAX_EMSG_LENGTH-1] = '\0';    /* JIC */
         fputs(emsg.msg, andlog);
         fputs("\n", andlog);
         fflush(andlog);
         break;
      /* Handle terminal messages */
      case INIT_ABORT_MSG:
         MPI_Recv(&emsg, sizeof(emsg), MPI_UNSIGNED_CHAR,
            msrc, mtag, NC.commd, &mstat);
         if (emsg.Err.ec == 0) { MPI_Finalize(); exit(0); }
         fputs(ssprintf(NULL,
            "TERMINAL MSG from Node %d, Code %d, Subcode %d:\n",
            emsg.Err.src, emsg.Err.ec, emsg.Err.iv), andlog);
         if (emsg.Err.ltxt) {
            emsg.msg[MAX_EMSG_LENGTH-1] = '\0';    /* JIC */
            fputs("   ", andlog);
            fputs(emsg.msg, andlog);
            fputs("\n", andlog);
            }
         fflush(andlog);
         sleep(1);
         if (NC.debug & DBG_NOXIT) {
            volatile int dbgwait = 1;
            while (dbgwait) {
               sleep(1);
               }
            }
         else
            MPI_Abort(MPI_COMM_WORLD, emsg.Err.ec);
         MPI_Finalize();
         /* I am doing an exit(0) here because with 'exit(err)' MPI
         *  causes a termination and dump, when all I want is my
         *  andmsg output.  */
         exit(0);
         /* exit(emsg.Err.ec); */

      /* Clean termination, just shut down */
      case SHUTDOWN_ANDMSG:
         /* Clear the message or the probe keeps returning */
         MPI_Recv(&sdmsg, 1, MPI_INT, msrc, mtag, NC.commd,
            MPI_STATUS_IGNORE);
         num2stop -= 1;
         break;

      /* Unidentified tag, read and print message anyway */
      default:
         MPI_Recv(emsg.msg, MAX_EMSG_LENGTH, MPI_UNSIGNED_CHAR,
            msrc, mtag, NC.commd, &mstat);
         fputs(ssprintf(NULL,
            "UNIDENTIFIED MSG from Node %d, Tag %d IGNORED\n",
            mstat.MPI_SOURCE, mstat.MPI_TAG), andlog);
         fflush(andlog);
         break;

         } /* End msgtype switch */

      } /* End main wait loop */

   } /* End andmsg() */
