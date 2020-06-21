/* (c) Copyright 2017, The Rockefeller University *11116* */
/* $Id: bbdcrsid.c 29 2017-09-15 18:02:30Z  $ */
/***********************************************************************
*                      BBD (Client Side) Package                       *
*                             bbdcrsid.c                               *
*                        Register a BBD sense                          *
*                                                                      *
*  Register a sensory array ("s" is for "sense" in bbdcrsid) for       *
*  transmission to the nervous system of a BBD.  This sense will       *
*  also return stimulus and group id information to the server.        *
*                                                                      *
*  Synopsis:                                                           *
*  BBDDev *bbdcrsid(char *snsnm, byte *psa, ui16 *pisgn,               *
*     si32 snsx, si32 snsy, int ksns)                                  *
*                                                                      *
*  Arguments:                                                          *
*     snsnm       Character string giving a name to this sense.        *
*                 This name must not exceed 15 characters and must     *
*                 match the corresponding name on the SENSE card in    *
*                 the CNS control file.                                *
*     psa         Ptr to static sensor array of size snsx x snsy.      *
*                 The sense data will be sent from this location       *
*                 to CNS each time bbdcputs() is called.               *
*     pisgn       Ptr to static ui16 identifier array, expected to     *
*                 contain, in this order, the stimulus and group id    *
*                 values for each stimulus.  The id information will   *
*                 be sent from this location to CNS each time          *
*                 bbdcputs() is called.                                *
*     snsx,snsy   Size of the sensor array along x and y dimensions.   *
*     ksns        BBDSns_Byte (=0) if sense data are single bytes.     *
*                 BBDSns_R4 (=0x0400) if 4-byte real (float) sense     *
*                    data are to be transmitted.                       *
*                 BBDBig (=0x80) if real sense data are big-endian     *
*                                                                      *
*  Return value:                                                       *
*     Ptr to BBDDev structure defining this sense.                     *
*                                                                      *
*  Prerequisites:                                                      *
*  bbdcinit() must be called before this routine is called.            *
*                                                                      *
*  Error Handling:                                                     *
*  All errors are terminal and result in calls to abexitm() with       *
*  a suitable message.  There is nothing useful the caller can do      *
*  to recover.                                                         *
************************************************************************
*  V1A, 11/20/10, GNR - New program, based on bbdmex version           *
*  ==>, 11/20/10, GNR - Last mod before committing to svn repository   *
*  R29, 06/05/17, GNR - Add BBDBig ksns flag bit                       *
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "bbd.h"
#include "rksubs.h"

extern struct BBDComData BBDcd;

BBDDev *bbdcrsid(char *snsnm, byte *psa, ui16 *pisgn,
      si32 snsx, si32 snsy, int ksns) {

   BBDDev *psns;              /* Ptr to data for this sense */

/* A little basic error checking */

   if (snsx <= 0 || snsx > SHRT_MAX || snsy <= 0 || snsy > SHRT_MAX)
      abexitm(BBDcErrParams, "Sensory data array size out-of-range");
   if (ksns & ~(BBDSns_R4|BBDBig))
      abexitm(BBDcErrParams, "Undefined sensory data mode spec");
   if (ksns & BBDSns_R4) {
      if ((int)psa & 3) abexitm(BBDcErrParams,
         "Float sense data not on 4-byte boundary"); }

/* Allocate a new BBDDev struct to hold the data, clear it,
*  link it into the end of the chain, mark it SENSE type.  */

   psns = (BBDDev *)callocv(1, sizeof(BBDDev), "Sense descriptor");
   *BBDcd.ppfsns = psns;
   BBDcd.ppfsns = &psns->pndev;
   psns->bdflgs = BBDTSns | BBDMdlt | ksns;

/* Store the data */

   psns->UD.Sns.psa = psa;
   psns->UD.Sns.pisgn = pisgn;
   psns->UD.Sns.snsx = snsx;
   psns->UD.Sns.snsy = snsy;
   psns->ldat = snsx * snsy;
   strncpy(psns->devnm, snsnm, MaxDevNameLen);

   return psns;
   } /* End bbdcrsid() */
