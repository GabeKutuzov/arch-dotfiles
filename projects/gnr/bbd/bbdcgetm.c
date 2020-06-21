/* (c) Copyright 2008-2010, The Rockefeller University *11116* */
/* $Id: bbdcgetm.c 2 2008-03-13 20:30:06Z  $ */
/***********************************************************************
*                      BBD (Client Side) Package                       *
*                             bbdcgetm.c                               *
*       Receive an arbitrary message from neural system to BBD         *
*                                                                      *
*  This routine may be called on a client BBD to receive a message     *
*  sent with the bbdsputm() call.  The client must call bbdcgetm()     *
*  in the same sequence relative to any sensor or effector data        *
*  transmission as the corresponding bbdsputm() call.                  *
*                                                                      *
*  Synopsis:                                                           *
*     char *bbdcgetm(void)                                             *
*                                                                      *
*  Return value:                                                       *
*     bbdcgetm() returns a pointer to the received message.  The       *
*     pointer is valid until the next call to bbdcgetm().              *
*                                                                      *
*  Prerequisites:                                                      *
*     bbdcinit() and bbdcchk() must have been called and completed     *
*     successfully.                                                    *
*                                                                      *
*  Error Handling:                                                     *
*     Error checking is minimal because we are now in the main BBD     *
*     action loop.  Everything should have already been checked.       *
*     All error are terminal and result in a call to abexit() with a   *
*     suitable message.  There is nothing useful the caller can do.    *
************************************************************************
*  V1A, 09/02/08, GNR - New program                                    *
*  ==>, 09/02/08, GNR - Last mod before committing to svn repository   *
*  Rev, 09/26/08, GNR - Minor type changes for 64-bit compilation      *
*  Rev, 04/09/10, GNR - More type changes for 64-bit compilation       *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include "sysdef.h"
#include "bbd.h"
#include "rksubs.h"
#include "rfdef.h"

struct BBDComData BBDcd;      /* Common BBD data struct */

char *bbdcgetm(void) {

   struct RFdef *pf;          /* File descriptor */
   si32         llmsg;        /* Message length */
   ui16         ckint;        /* Control integer */

   /* Check whether server already terminated */
   if (BBDcd.Mlogrc > 0)
      abexitm(BBDcErrServer, ssprintf(NULL, "Server (CNS) terminated "
         "with abexit code %5d", BBDcd.Mlogrc));

   pf = BBDcd.ndrecv;

   /* Receive and check the control integer */
   ckint = rfri2(pf);
   if (ckint != BBDLC_MSG) abexitm(BBDcErrBadData, "Bad control "
      "integer received from neural server");

   /* Receive the message length and make sure enough space
   *  has been allocated to receive it  */
   llmsg = rfri4(pf);
   if (llmsg <= 0) abexitm(BBDcErrBadData, "Zero-length message "
      "received from neural server");
   else if (llmsg > BBDcd.lmsg) {
      if (BBDcd.smsg) freev(BBDcd.smsg, "Host message");
      BBDcd.smsg = (char *)mallocv((size_t)llmsg+1, "Host message");
      BBDcd.lmsg = llmsg;
      }

   /* Receive the actual message and terminate it with a zero */
   rfread(pf, BBDcd.smsg, (size_t)llmsg, ABORT);
   BBDcd.smsg[llmsg] = '\0';

   return BBDcd.smsg;

   } /* End bbdcgetm() */

