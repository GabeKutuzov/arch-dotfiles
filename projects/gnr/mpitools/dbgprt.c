/* (c) Copyright 1996-2018, The Rockefeller University *11115* */
/* $Id: dbgprt.c 9 2018-08-21 18:13:05Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                               dbgprt                                 *
*                                                                      *
*  This routine may be used in hybrid systems to print a debug message *
*  from a computational node that does not have printf() support.      *
************************************************************************
*  V2A, 11/17/96, GNR - Broken out from abscom.c                       *
*  V2B, 03/29/98, GNR - Use ssprintf instead of fprintf                *
*  V2C, 03/11/00, GNR - No ssprintf on host, just print the msg        *
*  ==>, 12/29/09, GNR - Last mod before committing to svn repository   *
*  V3A, 07/10/16, GNR - Modify for MPI environment                     *
*  ==>, 07/10/16, GNR - Last mod before committing to svn mpi repo     *
*  R08, 08/19/18, GNR - Write host info to andmsg if PAR               *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sysdef.h"
#include "rksubs.h"
#include "mpitools.h"

void dbgprt(char *msg) {

   char tmsg[MAX_EMSG_LENGTH];
   int lmsg = strnlen(msg, MAX_EMSG_LENGTH-1);
   if (lmsg <= 0) {
      strcpy(tmsg, "***Empty debug message***");
      lmsg = strlen(msg);
      }
   else {
      strncpy(tmsg, msg, lmsg);
      tmsg[lmsg] = '\0';
      }

#ifdef PAR        /* Computational node: send message to dmsgid */
   if (NC.commd) {      /* Ignore if andmsg not up yet */
      MPI_Send(msg, lmsg+1, MPI_UNSIGNED_CHAR, NC.dmsgid,
         DEBUG_MSG, NC.commd);
      }
#else             /* Serial: just print the message on stderr */
   fputs("DBG MSG from host: ", stderr);
   fputs(msg, stderr);
   fputs("\n", stderr);
   fflush(stderr);

#endif

   } /* End dbgprt() */

