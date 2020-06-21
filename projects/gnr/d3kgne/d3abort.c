/* (c) Copyright 1993-2011, Neurosciences Research Foundation, Inc. */
/* $Id: d3abort.c 51 2012-05-24 20:53:36Z  $ */
/*---------------------------------------------------------------------*
*                             CNS Program                              *
*                 d3abort - abort parallel execution                   *
*                                                                      *
*  This routine is an interrupt handler installed at the start of      *
*     execution on each node to receive control each time a message    *
*     is received.  It implements a standardized shutdown procedure:   *
*  If running on host, it looks for an INIT_ABORT_MSG, and, when       *
*     one is found, it prints an appropriate termination message       *
*     encoded in the INIT_ABORT_MSG (this is what makes this an        *
*     application-specific procedure), then it broadcasts an           *
*     EXEC_ABORT_MSG throughout the processor array and terminates.    *
*  If running in a computational node, it just has to look for an      *
*     EXEC_ABORT_MSG and when one is found, complete the broadcast     *
*     and terminate.                                                   *
*  If a message is received which is not one of the above, it just     *
*     returns to the caller, allowing the message to be processed by   *
*     the normal application code.                                     *
*  Any node which wants to initiate a shutdown should call d3exit to   *
*     send an INIT_ABORT_MSG to the host. The text of this message is  *
*     an exit_msg struct containing the message number and possibly    *
*     additional information for constructing the error message text.  *
*                                                                      *
*  V1A, 08/28/89, G. N. Reeke                                          *
*  Rev, 07/26/90, JMS - Break most of old d3abort() out into new fn    *
*                       d3term(), which is both a handler for error    *
*                       termination signals and directly called for    *
*                       error term. on node 0.  d3abort() becomes a    *
*                       wrapper which tests incoming messsage types.   *
*  Rev, 07/31/90, JMS - Modified for overlays                          *
*  V5C, 02/19/92, GNR - Removed uncalled msgs, added missing ones.     *
*  V5E, 07/01/92, GNR - Adapt for serial version                       *
*  Rev, 12/01/92, GNR,ABP - Special handling of develop. errors        *
*  V6B, 08/02/93, ABP,GNR - Remove message printing to d3perr.c        *
*  V8A, 06/04/96, GNR - Return calling error code on exit              *
*  Rev, 11/27/96, GNR - Remove support for non-hybrid versions         *
*  Rev, 09/05/99, GNR - Remove remaining support for NCUBE             *
*  Rev, 08/09/01, GNR - Make 'shutdown' handler available for non-PAR  *
*  ==>, 02/17/05, GNR - Last mod before committing to svn repository   *
*  V8H, 01/15/11, GNR - Add call to bbdssvrc                           *
*---------------------------------------------------------------------*/

/* Include standard library functions */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#ifdef UNIX
#include <errno.h>
#include <signal.h>
#endif
#include "d3global.h"
#ifndef PARn
#ifdef BBDS
#include "bbd.h"
#endif
#include "rocks.h"
#include "rkxtra.h"
#endif

/* Define maximum length of msgtext buffer (not checked in d3exit) */
#define EMTXTLEN   120

/*--------------------------------------------------------------------*/
/*                                                                    */
/*                              d3term                                */
/*                                                                    */
/*--------------------------------------------------------------------*/

void d3term(struct exit_msg *EMSG, int source) {

#ifdef PAR

/* Broadcast message to all nodes, telling them to terminate.
*  Since we are in process of terminating, omit error checking. */

   anwrite(EMSG, (int)sizeof(struct exit_msg), BCSTADDR,
      EXEC_ABORT_MSG, NULL);

#if defined(PAR0) && defined(UNIX)
   if (NC.hmniof & HNIO_NC) {
      signal(SIGUSR1, SIG_IGN);
      kill(0, SIGUSR1);
      }
#endif

#else    /* !PAR */
   d3perr(EMSG, source);
#endif
#if defined(BBDS) && !defined(PARn)
   bbdssvrc(EMSG->ec);
#endif
   exit(EMSG->ec);
   } /* End d3term */


/*--------------------------------------------------------------------*/
/*                                                                    */
/*                             shutdown                               */
/*                                                                    */
/* This routine is a handler for SIGTERM that is used only on the     */
/* hybrid host.  At present, all it does is call exit() to flush      */
/* output buffers.  It is planned to make it aware of cryout() calls, */
/* so it can flush rocks output buffers as well.                      */
/*--------------------------------------------------------------------*/

#ifndef PARn

void shutdown(int signum) {

   exit(signum);

   }

#endif
