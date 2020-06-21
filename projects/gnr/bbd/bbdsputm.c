/* (c) Copyright 2008, The Rockefeller University *11116* */
/* $Id: bbdsputm.c 2 2008-03-13 20:30:06Z  $ */
/***********************************************************************
*                      BBD (Server Side) Package                       *
*                             bbdsputm.c                               *
*         Send an arbitrary message from neural system to BBD          *
*                                                                      *
*  This routine may be called on a server neural system to send        *
*  arbitrary data to BBD clients.  For example, CNS can send its       *
*  version number to the client for recording.  The client must        *
*  call bbdcgetm() in the same sequence relative to any sensor or      *
*  effector data transmission as the corresponding bbdsputm() call.    *
*                                                                      *
*  Synopsis:                                                           *
*     void bbdsputm(char *msg, int who)                                *
*                                                                      *
*  Arguments:                                                          *
*     'msg' is a pointer to the data to be sent.  It should be an      *
*        ASCII text terminated by a '\0' character.  No byte-order     *
*        swapping is done.                                             *
*     'who' is the OR of bits indicating to which clients the message  *
*        should be sent (define these bits in the order the client     *
*        sockets were defined in the setup process--1 for the first    *
*        client, 2 for second client, etc.)                            *
*                                                                      *
*  Prerequisites:                                                      *
*     bbdsinit() and bbdschk() must have been called and completed     *
*     successfully.                                                    *
*                                                                      *
*  Error Handling:                                                     *
*     Error checking is minimal because we are now in the main         *
*     simulation loop.  Everything should have already been checked.   *
*     All error are terminal and result in a call to abexit() with a   *
*     suitable message.  There is nothing useful the caller can do.    *
************************************************************************
*  V1A, 09/02/08, GNR - New program                                    *
*  ==>, 09/02/08, GNR - Last mod before committing to svn repository   *
*  Rev, 09/26/08, GNR - Minor type changes for 64-bit compilation      *
***********************************************************************/

#define BBDS

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sysdef.h"
#include "bbd.h"
#include "rfdef.h"

struct BBDComData BBDsd;      /* Common BBD data struct */

void bbdsputm(char *msg, int who) {

   struct BBDsHost *pbsh;
   struct RFdef *pf;          /* File descriptor */
   size_t lmsg;               /* Message length */
   int    iwho;               /* Client selector */

   lmsg = strlen(msg);

/* Loop over all the clients on the who list */

   for (pbsh=BBDsd.phlist,iwho=1; pbsh; pbsh=pbsh->pnhost,iwho<<=1) {
      if (!(who & iwho)) continue;

      /* Locate the correct socket */
      pf = pbsh->ndsend;

      /* Send the control integer, message length,
      *  and message, then flush */
      rfwi2(pf, BBDLC_MSG);
      rfwi4(pf, (si32)lmsg);
      rfwrite(pf, msg, lmsg, ABORT);
      rfflush(pf, ABORT);
      } /* End client loop */

   } /* End bbdsputm() */
