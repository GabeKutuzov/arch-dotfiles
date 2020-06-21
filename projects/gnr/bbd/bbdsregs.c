/* (c) Copyright 2017, The Rockefeller University *11115* */
/* $Id: bbdsregs.c 29 2017-09-15 18:02:30Z  $ */
/***********************************************************************
*                      BBD (Server Side) Package                       *
*                             bbdsregs.c                               *
*                                                                      *
*  Register a sensory array ("s" is for "sense" in bbdsregs) for       *
*  transmission from a BBD to the nervous system in CNS.  N.B.         *
*  After all cameras, senses, effectors, and values have been          *
*  registered, bbdschk() must be called to check that the same         *
*  registrations have been made in the BBD and to establish the        *
*  communications link between client and server.                      *
*                                                                      *
*  Synopsis:                                                           *
*  BBDDev *bbdsregs(char *snsnm, char *phnm, si32 snsx, si32 snsy,     *
*     int ksns);
*                                                                      *
*  Arguments:                                                          *
*     snsnm       Character string giving a name to this sense.        *
*                 This name must not exceed 15 characters and must     *
*                 match the corresponding name given in a bbdcregs()   *
*                 call in the BBD.                                     *
*     phnm        Pointer to client BBD host name or host:port.        *
*                 This should be NULL to indicate the same host        *
*                 that initiated the server.                           *
*     snsx,snsy   Size of the sensor array along x and y dimensions.   *
*     ksns        BBDSns_Byte (=0) if sense data are single bytes.     *
*                 BBDSns_R4 (=0x0400) if 4-byte real (float) sense     *
*                    data are to be transmitted.                       *
*                 BBDBig (=0x80) if real sense data are big-endian     *
*                                                                      *
*  Returns:                                                            *
*  Location of a BBDDev for this sensor (where the caller must         *
*  eventually deposit the data pointer and BBDUsed flags, etc.).       *
*                                                                      *
*  Prerequisites:  bbdsinit()                                          *
*                                                                      *
*  Error Handling:                                                     *
*  All errors are terminal and result in calls to abexitm() with       *
*  a suitable message.  This makes it easier to use these routines     *
*  with some application other than CNS.                               *
*                                                                      *
************************************************************************
*  V1A, 02/07/07, GNR - New program                                    *
*  ==>, 12/21/07, GNR - Last mod before committing to svn repository   *
*  R29, 06/05/17, GNR - Add BBDBig ksns flag bit                       *
***********************************************************************/

#define BBDS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "bbd.h"
#include "rksubs.h"

/* Common BBD data--actual instance in bbdsinit */
extern struct BBDComData BBDsd;

BBDDev *bbdsregs(char *snsnm, char *phnm, si32 snsx, si32 snsy,
   int ksns) {

   BBDDev *pvs;               /* Ptr to data for this sense */

/* A little basic error checking */

   if (snsx <= 0 || snsx > SHRT_MAX || snsy <= 0 || snsy > SHRT_MAX)
      abexitm(BBDsErrParams, "Sensory data array size out-of-range");
   if (ksns & ~(BBDSns_R4|BBDBig))
      abexitm(BBDsErrParams, "Undefined sensory data mode spec");
   
/* Allocate a new BBDDev struct to hold the data, clear it, and link
*  it to the end of the sensor chain.  */

   pvs = (BBDDev *)callocv(1, sizeof(BBDDev), "Sense descriptor");
   *BBDsd.ppfsns = pvs;
   BBDsd.ppfsns = &pvs->pndev;

/* Set the mode and store the data */

   strncpy(pvs->devnm, snsnm, MaxDevNameLen);
   pvs->bdflgs = BBDTSns | ksns;
   pvs->UD.Sns.snsx = snsx;
   pvs->UD.Sns.snsy = snsy;
   pvs->ldat = snsx * snsy;

/* Find or make space to store the host info */

   pvs->pshost = bbdsfhnm(phnm);

   return pvs;

   } /* End bbdsregs() */
