/* (c) Copyright 2007-2010, The Rockefeller University *11115* */
/* $Id: bbdsrege.c 28 2017-01-13 20:35:15Z  $ */
/***********************************************************************
*                      BBD (Server Side) Package                       *
*                             bbdsrege.c                               *
*                                                                      *
*  Register an effector array ("e" is for "effector" in bbdsrege) for  *
*  transmission of motor neuron command data from a nervous system     *
*  simulated in CNS to a BBD.  N.B.  After all senses, effectors,      *
*  and values have been registered, bbdschk() must be called to        *
*  check that the same registrations have been made in the BBD, to     *
*  supply the location of the motor neuron output data, and to         *
*  establish the communications link between client(s) and server.     *
*                                                                      *
*  Synopsis:                                                           *
*  BBDDev *bbdsrege(char *effnm, float *phnm, int mnad)                *
*                                                                      *
*  Arguments:                                                          *
*     effnm       Character string giving a name to this effector.     *
*                 This name must not exceed 15 characters and must     *
*                 match the corresponding name given in a bbdcrege()   *
*                 call in the BBD.                                     *
*     phnm        Pointer to client BBD host name or host:port.        *
*                 This should be NULL to indicate the same host        *
*                 that initiated the server.                           *
*     mnad        Motor neuron array dimension--Number of neurons      *
*                 contributing Si and/or phase to the pmno array.      *
*                 (Note:  This value can be stored into peff->ldat     *
*                 later if not known at this time.)                    *
*                                                                      *
*  Returns:                                                            *
*  Location of a BBDDev for this device (where the caller must         *
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
*  V1A, 02/08/07, GNR - New program                                    *
*  ==>, 12/21/07, GNR - Last mod before committing to svn repository   *
*  Rev, 12/01/10, GNR - Remove mnad check, may be supplied later       *
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

BBDDev *bbdsrege(char *effnm, char *phnm, int mnad) {

   BBDDev *peff;              /* Ptr to data for this effector */

/* Allocate a new BBDDev struct to hold the data, clear it, and link
*  it to the end of the effector chain.  */

   peff = (BBDDev *)callocv(1, sizeof(BBDDev), "Effector data");
   *BBDsd.ppfeff = peff;
   BBDsd.ppfeff = &peff->pndev;

/* Set the mode and store the data */

   strncpy(peff->devnm, effnm, MaxDevNameLen);
   peff->bdflgs = BBDTEff;
   peff->ldat = (si32)mnad;

/* Find or make space to store the host info */

   peff->pshost = bbdsfhnm(phnm);

   return peff;

   } /* End bbdsrege() */
