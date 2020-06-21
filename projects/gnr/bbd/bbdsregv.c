/* (c) Copyright 2007, The Rockefeller University *11115* */
/* $Id: bbdsregv.c 28 2017-01-13 20:35:15Z  $ */
/***********************************************************************
*                      BBD (Server Side) Package                       *
*                             bbdsregv.c                               *
*                                                                      *
*  Register a value ("v" is for "value" in bbdsregv) for transmis-     *
*  sion from a BBD to the nervous system in CNS.  N.B.  After all      *
*  senses, effectors, and values have been registered, bbdschk()       *
*  must be called to check that the same registrations have been       *
*  made in the BBD and to establish the communications link between    *
*  client and server.                                                  *
*                                                                      *
*  Synopsis:                                                           *
*  BBDDev *bbdsregv(char *valnm, char *phnm, int hport)                *
*                                                                      *
*  Arguments:                                                          *
*     valnm       Character string giving a name to this value.        *
*                 This name must not exceed 15 characters and must     *
*                 match the corresponding name given in a bbdcregv()   *
*                 call in the BBD.                                     *
*     phnm        Pointer to client BBD host name or host:port.        *
*                 This should be NULL to indicate the same host        *
*                 that initiated the server.                           *
*                                                                      *
*  Returns:                                                            *
*  Location of a BBDDev for this value (where the caller must          *
*  eventually deposit the data pointer and BBDUsed flags, etc.).       *
*                                                                      *
*  Prerequisites:  bbdsinit                                            *
*                                                                      *
*  Error Handling:                                                     *
*  All errors are terminal and result in calls to abexitm() with       *
*  a suitable message.  This makes it easier to use these routines     *
*  with some application other than CNS.                               *
*                                                                      *
************************************************************************
*  V1A, 02/07/07, GNR - New program                                    *
*  ==>, 12/21/07, GNR - Last mod before committing to svn repository   *
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

BBDDev *bbdsregv(char *valnm, char *phnm) {

   BBDDev *pvl;               /* Ptr to data for this value */
   
/* Allocate a new BBDDev struct to hold the data, clear it, and link
*  it to the end of the value chain.  */

   pvl = (BBDDev *)callocv(1, sizeof(BBDDev), "Value descriptor");
   *BBDsd.ppfval = pvl;
   BBDsd.ppfval = &pvl->pndev;

/* Set the mode and store the data */

   strncpy(pvl->devnm, valnm, MaxDevNameLen);
   pvl->bdflgs = BBDTVal;
   pvl->ldat = 1;

/* Find or make space to store the host info */

   pvl->pshost = bbdsfhnm(phnm);

   return pvl;

   } /* End bbdsregv() */
