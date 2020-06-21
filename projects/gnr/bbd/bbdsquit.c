/* (c) Copyright 2011-2018, The Rockefeller University *11115* */
/* $Id: bbdsquit.c 30 2018-08-15 19:46:51Z  $ */
/***********************************************************************
*                      BBD (Server Side) Package                       *
*                             bbdsquit,c                               *
*                                                                      *
*----------------------------------------------------------------------*
*                              bbdssvrc                                *
*                   Saves return code for bbdsquit                     *
*                                                                      *
*  This routine should be called when the server is about to perform   *
*  an abnormal exit.  The return code is stored so that when bbdsquit  *
*  runs (it is registered as an 'atexit' routine), it can pass this    *
*  code to all the BBD clients for reporting to the user.              *
*----------------------------------------------------------------------*
*                              bbdsquit                                *
*                                                                      *
*  This routine is registered via atexit() to run when exit() is       *
*  called. It opens sockets to clients if not already done by bbdschk, *
*  sends a terminate code to each, then closes the sockets and returns *
*  to the regular shutdown process.  Other shutdown code can be added  *
*  here as needed.                                                     *
*  (It is assumed that memory will be freed adequately by the OS.)     *
************************************************************************
*  V1A, 01/15/11, GNR - bbdsquit removed from bbdsinit, new bbdssvrc   *
*  ==>, 01/15/11, GNR - Last mod before committing to svn repository   *
*  Rev, 04/18/13, GNR - Add external access to bbdsquit                *
*  R30, 08/04/18, GNR - Remove SUNOS support                           *
***********************************************************************/

#define BBDS

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sysdef.h"
#include "bbd.h"
#include "rfdef.h"
#include "rksubs.h"

/* Common BBD data--actual instance in bbdsinit */
extern struct BBDComData BBDsd;

/*=====================================================================*
*                              bbdssvrc                                *
*=====================================================================*/

void bbdssvrc(int rc) {

   BBDsd.svrc = rc;

   } /* End bbdssvrc() */


/*=====================================================================*
*                              bbdsquit                                *
*=====================================================================*/

void bbdsquit(void) {

   struct BBDsHost *pbsh;

   for (pbsh=BBDsd.phlist; pbsh; pbsh=pbsh->pnhost) {
      if (pbsh->kpute & BBDsClosed) continue;
      if (!pbsh->ndsend) {
         pbsh->ndsend = rfallo(pbsh->pbbdnm, WRITE, BINARY,
            INITIATOR, TOP, NO_LOOKAHEAD, REWIND, RELEASE_BUFF,
            IGNORE, IGNORE, pbsh->bbdport, NOMSG_ABORT);
         rfopen(pbsh->ndsend, NULL, WRITE, BINARY,
            INITIATOR, TOP, NO_LOOKAHEAD, REWIND, RELEASE_BUFF,
            IGNORE, IGNORE, pbsh->bbdport, NOMSG_ABORT);
         }
      rfwi2(pbsh->ndsend, BBDLC_TERM);
      rfwi4(pbsh->ndsend, BBDsd.svrc);
      rksleep(1,0);
      rfclose(pbsh->ndsend, REWIND, RELEASE_BUFF, NOMSG_ABORT);
      if (pbsh->ndrecv)
         rfclose(pbsh->ndrecv, REWIND, RELEASE_BUFF, NOMSG_ABORT);
      pbsh->kpute |= BBDsClosed;
      } /* End host loop */

   } /* End bbdsquit() */
