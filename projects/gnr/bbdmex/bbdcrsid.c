/* (c) Copyright 2017, The Rockefeller University *11116* */
/* $Id: bbdcrsid.c 17 2017-10-13 19:00:07Z  $ */
/***********************************************************************
*               BBD (Client Side) mex-Function Package                 *
*                             bbdcrsid.c                               *
*                        Register a BBD sense                          *
*                                                                      *
*  This is the matlab mex-function version of bbdcrsid().              *
*                                                                      *
*  Register a sensory array ("s" is for "sense" in bbdcrsid) for       *
*  transmission to the nervous system of a BBD.  This sense will       *
*  also return group and stimulus id information to the server.        *
*                                                                      *
*  N.B.  After all senses, effectors, and values have been registered, *
*  bbdcchk() must be called to check that the same registrations have  *
*  been made in CNS and to establish the communications link between   *
*  client and server.                                                  *
*                                                                      *
*  MATLAB Synopsis:                                                    *
*     bbdcrsid(BBDComm, snsnm, snsx, snsy, ksns)                       *
*                                                                      *
*  Arguments:                                                          *
*     BBDComm  MATLAB array containing private data returned by a      *
*              previous call to bbdcinit.                              *
*     snsnm    Character string giving a name to this sense.           *
*              This name must not exceed 15 characters and must        *
*              match the corresponding name on the SENSE card in       *
*              the CNS control file.                                   *
*     snsx,snsy   MATLAB int32 integers giving the size of the sensor  *
*              array along the x and y dimensions.                     *
*     ksns     MATLAB int32 integer giving the type of the sense data. *
*              Possible values are BBDSns_Byte (=0) if sense data are  *
*              single bytes, BBDSns_R4 (=0x0400) if 4-byte real        *
*              (float) sense data are to be transmitted plus BBDBig    *
*              (=0x80) if data are big-endian.                         *
*                                                                      *
*  Prerequisites:                                                      *
*  bbdcinit() must be called before this routine is called.            *
*                                                                      *
*  Error Handling:                                                     *
*  All errors are terminal and result in calls to abexitm() with       *
*  a suitable message.  There is nothing useful the caller can do      *
*  to recover.                                                         *
************************************************************************
*  V1A, 11/09/10, GNR - New mex program derived from bbdcregs.c        *
*  ==>, 11/09/10, GNR - Last mod before committing to svn repository   *
*  R17, 06/05/17, GNR - Add BBDBig ksns flag bit                       *
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define I_AM_MEX

#include "mex.h"
#include "sysdef.h"
#include "bbd.h"
#include "rksubs.h"

void mexFunction(int nlhs, mxArray *plhs[],
      int nrhs, const mxArray *prhs[]) {

   struct BBDComData *BBDcd;  /* Common BBD data struct */
   si32   snsx,snsy;          /* Array dimensions */
   si32   ksns;               /* Sense type */
   BBDDev *psns;              /* Ptr to data for this sense */

   /* Standard MATLAB check for proper number and type of
   *  arguments, then locate BBDComData */

   if (nlhs != 0 || nrhs != 5 || !mxIsChar(prhs[1]) ||
         !mxIsInt32(prhs[2]) || !mxIsInt32(prhs[3]) ||
         !mxIsInt32(prhs[4]))
      abexitm(BBDcErrMexCall, "Bad arguments to bbdcrsid");
   BBDcd = (struct BBDComData *)mxGetData(prhs[0]);
   if (!BBDcd)
      abexitm(BBDcErrMexCall, "Bad BBDcd arg to bbdcrsid");
   snsx = *(si32 *)mxGetData(prhs[2]);
   snsy = *(si32 *)mxGetData(prhs[3]);
   ksns = *(si32 *)mxGetData(prhs[4]);

/* A little basic error checking */

   if (mxGetNumberOfElements(prhs[1]) > 15)
      abexitm(BBDcErrParams, "Effector name > 15 chars");
   if (snsx <= 0 || snsx > SHRT_MAX || snsy <= 0 || snsy > SHRT_MAX)
      abexitm(BBDcErrParams, "Sensory data array size out-of-range");
   if (ksns & ~(BBDSns_R4|BBDBig))
      abexitm(BBDcErrParams, "Undefined sensory data mode spec");

/* Allocate a new BBDDev struct to hold the data, clear it,
*  link it into the end of the chain, mark it SENSE type with
*  modality information.  */

   psns = (BBDDev *)mxCalloc(1, sizeof(BBDDev));
   mexMakeMemoryPersistent(psns);
   *BBDcd->ppfsns = psns;
   BBDcd->ppfsns = &psns->pndev;
   psns->bdflgs = BBDTSns | BBDMdlt | ksns;

/* Store the data */

   psns->UD.Sns.snsx = snsx;
   psns->UD.Sns.snsy = snsy;
   psns->ldat = snsx * snsy;
   mxGetString(prhs[1], psns->devnm, MaxDevNameLen+1);

   } /* End bbdcrsid() */
