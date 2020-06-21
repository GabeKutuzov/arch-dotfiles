/* (c) Copyright 2007-2008, The Rockefeller University *11116* */
/* $Id: bbdcevnt.c 28 2017-01-13 20:35:15Z  $ */
/***********************************************************************
*                      BBD (Client Side) Package                       *
*                             bbdcevnt.c                               *
*                       Signal an event to CNS                         *
*                                                                      *
*  This routine may be called at any time by a BBD client.  It sends   *
*  a bdid code to CNS with the BBDLC_EVNT bit set, followed by a       *
*  caller-specified BBDSigCode value.  This signal will be held on     *
*  the server through various bbdsgetx() calls until bbdsevnt() is     *
*  called, i.e., there is no real out-of-band signalling, which CNS    *
*  would not be able to handle anyway.                                 *
*                                                                      *
*  Synopsis:                                                           *
*     void bbdcevnt(ui32 ecode)                                        *
*                                                                      *
*  Arguments:                                                          *
*     ecode is a non-zero event code.  Codes that are meaningful to    *
*     the CNS server are defined in the bbd.h header file with names   *
*     beginning with BBDSig....  Other codes may be sent if this       *
*     package is used with other servers.                              *
*                                                                      *
********************************************************************** *
*  V1A, 11/20/07, GNR - New program                                    *
*  ==>, 11/24/07, GNR - Last mod before committing to svn repository   *
*  Rev, 09/26/08, GNR - Minor type changes for 64-bit compilation      *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include "sysdef.h"
#include "bbd.h"
#include "rfdef.h"

struct BBDComData BBDcd;               /* Common BBD data struct */

void bbdcevnt(ui32 ecode) {

   struct RFdef *pf;          /* File descriptor */
   ui16 ehdr;                 /* Event header */

   /* Check whether server already terminated */
   if (BBDcd.Mlogrc > 0)
      abexitm(BBDcErrServer, ssprintf(NULL, "Server (CNS) terminated "
         "with abexit code %5d", BBDcd.Mlogrc));

   pf = BBDcd.ndsend;

   /* Write the event signal */
   ehdr = BBDLC_EVNT;
   rfwi2(pf,ehdr);
   rfwi4(pf,(si32)ecode);

   /* Flush the stream */
   rfflush(pf, ABORT);

   } /* End bbdcevnt() */

