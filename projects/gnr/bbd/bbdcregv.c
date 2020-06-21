/* (c) Copyright 2007, The Rockefeller University *11115* */
/* $Id: bbdcregv.c 28 2017-01-13 20:35:15Z  $ */
/***********************************************************************
*                      BBD (Client Side) Package                       *
*                             bbdcregv.c                               *
*                                                                      *
*  Register a value ("v" is for "value" in bbdcregv) for               *
*  transmission to the nervous system of a BBD.  N.B.  After all       *
*  senses, effectors, and values have been registered, bbdcchk()       *
*  must be called to check that the same registrations have been       *
*  made in CNS and to establish the communications link between        *
*  client and server.                                                  *
*                                                                      *
*  Synopsis:                                                           *
*  BBDDev *bbdcregv(char *valnm, float *pval)                          *
*                                                                      *
*  Arguments:                                                          *
*     valnm       Character string giving a name to this value.        *
*                 This name must not exceed 15 characters and must     *
*                 match the corresponding name on the VALUE card in    *
*                 the CNS control file.                                *
*     pval        Ptr to static 32-bit floating point scalar value.    *
*                 The value, after multiplication by the scale given   *
*                 in the CNS control file, should lie in the range     *
*                 -1 <= value <= +1.  The value data will be sent      *
*                 from this location to CNS each time bbdcputv()       *
*                 is called.                                           *
*                                                                      *
*  Return value:                                                       *
*     Ptr to BBDDev structure defining this value.                     *
*                                                                      *
*  Prerequisites:                                                      *
*  bbdcinit() must be called before this routine is called.            *
*                                                                      *
*  Error Handling:                                                     *
*  All errors are terminal and result in calls to abexitm() with       *
*  a suitable message.  There is nothing useful the caller can do      *
*  to recover.                                                         *
*                                                                      *
************************************************************************
*  V1A, 02/07/07, GNR - New program                                    *
*  ==>, 12/21/07, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "bbd.h"
#include "rksubs.h"

extern struct BBDComData BBDcd;

BBDDev *bbdcregv(char *valnm, float *pval) {

   BBDDev *pvl;               /* Ptr to data for this value */
   
/* Allocate a new BBDDev struct to hold the data, clear it,
*  link it into the end of the chain, and mark it VALUE type.  */

   pvl = (BBDDev *)callocv(1, sizeof(BBDDev), "Value descriptor");
   *BBDcd.ppfval = pvl;
   BBDcd.ppfval = &pvl->pndev;
   pvl->bdflgs = BBDTVal;

/* Store the data */

   pvl->UD.Val.pval = pval;
   pvl->ldat = 1;
   strncpy(pvl->devnm, valnm, MaxDevNameLen);

   return pvl;
   } /* End bbdcregv() */
