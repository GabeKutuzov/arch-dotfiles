/* (c) Copyright 2007-2008, The Rockefeller University *11116* */
/* $Id: bbdsevnt.c 28 2017-01-13 20:35:15Z  $ */
/***********************************************************************
*                      BBD (Server Side) Package                       *
*                             bbdsevnt.c                               *
*          Return an event code sent by a BBD client to CNS            *
*                                                                      *
*  This routine may be called at any time by the BBD server.  It       *
*  returns a 32-bit code which may contain any bit flags agreed        *
*  on by the client and server.  The value returned is the OR of       *
*  any flags that were sent by clients over any open sockets by        *
*  calling bbdcevnt().  The flags are then cleared so the same         *
*  events are not reported more than once.  A return value of 0        *
*  indicates that no flags were received.                              *
*                                                                      *
*  Synopsis:                                                           *
*     ui32 bbdsevnt(int wait)                                          *
*                                                                      *
*  Argument:                                                           *
*     wait     NO (0) if bbdsevnt should return at once if no event    *
*              code record is in any input socket, otherwise the OR    *
*              of bits indicating which sockets to block on until an   *
*              event record is received (define these bits in order    *
*              the client sockets were defined in the setup process--  *
*              1 for first client, 2 for second client, etc.)          *
*                                                                      *
*  Note that there is no true out-of-channel signalling.  Signals      *
*  are attended to only when the server wishes to receive them.        *
*  Because there is no standard way to "push back" data received       *
*  over a socket, bbdsevnt() stores headers for normal sense data      *
*  that it may read in the BBDsHost structs for the sockets, and       *
*  the bbdsgetx() routines look for those before reading more.         *
*                                                                      *
************************************************************************
*  V1A, 11/21/07, GNR - New program                                    *
*  ==>, 11/23/07, GNR - Last mod before committing to svn repository   *
*  V1B, 05/02/08, GNR - Add wait argument                              *
*  Rev, 09/26/08, GNR - Minor type changes for 64-bit compilation      *
***********************************************************************/

#define BBDS

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include "sysdef.h"
#include "bbd.h"
#include "rfdef.h"

struct BBDComData BBDsd;               /* Common BBD data struct */

/*=====================================================================*
*                              bbdsevnt                                *
*=====================================================================*/

ui32 bbdsevnt(int wait) {

   fd_set        rdsock;      /* Set for read select */
   struct timeval tpoll;      /* Zero time value for polling */
   struct RFdef     *pf;      /* Current device */
   struct BBDsHost *psh;      /* Ptr to client host info */
   int  cnsrf;                /* Read socket descriptor */
   int  rc;                   /* Return code from select() */
   int  wbit,wck;             /* Wait check bit */
   ui32 ecode;                /* This will be the return value */
   ui16 ckint;                /* Check integer from an interface */

/* Start with any signal codes already received by bbdsgetx routines */

   ecode = BBDsd.BBDSigCode;
   BBDsd.BBDSigCode = 0;

/* Loop over all open socket interfaces.  Check whether any data
*  are available.  If so, read a check integer.  If it indicates
*  a signal, read the signal.  Otherwise, store it back for the
*  bbdsgetx() routine to use later.  */

   for (psh=BBDsd.phlist,wbit=1; psh; psh=psh->pnhost,wbit<<=1) {
      if (psh->qlabd) continue;  /* Already found normal data here */
      pf = psh->ndrecv;
      if (!(pf->iamopen & IAM_ANYOPEN)) continue;  /* JIC */
      cnsrf = pf->frwd;                /* Get read descriptor */
      wck = wait & wbit;               /* First pass wait request */
      while (1) {                      /* There could be > 1 signal */
         FD_ZERO(&rdsock);
         FD_SET(cnsrf, &rdsock);
         tpoll.tv_sec = 0;
         tpoll.tv_usec = 0;
         rc = select(cnsrf+1, &rdsock, NULL, NULL,
            wck ? NULL : &tpoll);
         if (rc < 0)
            abexitme(BBDsErrSelect, "Could not select client socket");
         if (!(FD_ISSET(cnsrf, &rdsock))) break;   /* No data there */
         ckint = rfri2(pf);
         if (ckint != BBDLC_EVNT) {    /* Is it an event signal? */
            psh->labdid = ckint;       /* No, save it and go on */
            psh->qlabd = TRUE;
            break;
            }
         else {                        /* Yes, OR it and read again */
            ecode |= rfri4(pf);
            wck = 0;                   /* But don't block again */
            }
         } /* End signal reading loop */
      } /* End loop over socket interfaces */

   return ecode;

   } /* End bbdsevnt() */

