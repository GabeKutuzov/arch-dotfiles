/* (c) Copyright 2007-2015, The Rockefeller University *11115* */
/* $Id: bbdcrege.c 28 2017-01-13 20:35:15Z  $ */
/***********************************************************************
*                      BBD (Client Side) Package                       *
*                             bbdcrege.c                               *
*                                                                      *
*  Register an effector array ("e" is for "effector" in bbdcrege)      *
*  for receipt of motor neuron command data from the nervous system    *
*  of a BBD.  N.B.  After all senses, effectors, and values have       *
*  been registered, bbdcchk() must be called to check that the same    *
*  registrations have been made in the server and to establish the     *
*  communications link between client and server.                      *
*                                                                      *
*  Synopsis:                                                           *
*  BBDDev *bbdcrege(char *effnm, float *pmno, int mnad)                *
*                                                                      *
*  Arguments:                                                          *
*     effnm       Character string giving a name to this effector.     *
*                 This name must not exceed 15 characters and must     *
*                 match the corresponding name assigned in the server. *
*     pmno        Ptr to static effector activation array of size      *
*                 mnad float values.  The requested values, the sums   *
*                 of above-threshold cell activity in the cell types   *
*                 specified in the server, will be placed into this    *
*                 array every time bbdcgetr() is called.               *
*     mnad        Motor neuron array size (number of elements).  Note  *
*                 that although the MATLAB mex-file version of this    *
*                 routine allows mnad = 0 to be coded to indicate that *
*                 the array size received from CNS should be used with *
*                 no validation, mnad = 0 is not allowed here because  *
*                 the size of the static array 'pmno' must be checked  *
*                 to assure it is large enough to receive the data.    *
*                                                                      *
*  Return value:                                                       *
*     Ptr to BBDDev structure defining this effector.                  *
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
*  Rev, 12/20/15, GNR - Added comment why mnad = 0 not allowed here.   *
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "bbd.h"
#include "rksubs.h"

extern struct BBDComData BBDcd;

BBDDev *bbdcrege(char *effnm, float *pmno, int mnad) {

   BBDDev *peff;              /* Ptr to data for this effector */

/* A little basic error checking */

   if (mnad <= 0) abexitm(BBDcErrParams,
      "Invalid motor neuron array size");

/* Allocate a new BBDDev struct to hold the data, clear it,
*  link it onto the end of the chain, mark it EFFECTOR type.  */

   peff = (BBDDev *)callocv(1, sizeof(BBDDev), "Effector data");
   *BBDcd.ppfeff = peff;
   BBDcd.ppfeff = &peff->pndev;
   peff->bdflgs = BBDTEff;

/* Store the data */

   peff->UD.Eff.pmno = pmno;
   peff->ldat = (si32)mnad;
   strncpy(peff->devnm, effnm, MaxDevNameLen);

   return peff;
   } /* End bbdcrege() */
