/* (c) Copyright 2010, The Rockefeller University *11116* */
/* $Id: bbdcevnt.c 16 2017-01-13 20:36:40Z  $ */
/***********************************************************************
*               BBD (Client Side) mex-Function Package                 *
*                             bbdcevnt.c                               *
*                       Signal an event to CNS                         *
*                                                                      *
*  This is the matlab mex-function version of bbdcevnt().              *
*                                                                      *
*  This routine may be called at any time by a MATLAB BBD client.  It  *
*  sends a bdid code to CNS with the BBDLC_EVNT bit set, followed by   *
*  a caller-specified BBDSigCode value.  This signal will be held on   *
*  the server through various bbdsgetx() calls until bbdsevnt() is     *
*  called, i.e., there is no real out-of-band signalling, which CNS    *
*  would not be able to handle anyway.                                 *
*                                                                      *
*  MATLAB Synopsis:                                                    *
*     bbdcevnt(BBDComm, ecode)                                         *
*                                                                      *
*  Arguments:                                                          *
*     BBDComm  MATLAB array containing private data returned by a      *
*              previous call to bbdcinit.                              *
*     ecode    Non-zero int32 event code.  Codes that are meaningful   *
*              to the CNS server are defined in the bbd.h header file  *
*              with names beginning with BBDSig....  These will have   *
*              to be coded as explicit numeric values in the calling   *
*              MATLAB program.                                         *
*                                                                      *
************************************************************************
*  V1A, 10/22/10, GNR - New mex version                                *
*  ==>, 11/04/10, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#define I_AM_MEX

#include "mex.h"
#include "sysdef.h"
#include "bbd.h"
#include "rfdef.h"

void mexFunction(int nlhs, mxArray *plhs[],
      int nrhs, const mxArray *prhs[]) {

   struct BBDComData *BBDcd;  /* Common BBD data struct */
   struct RFdef *pf;          /* File descriptor */
   si32 ecode;                /* Event code to be written */
   ui16 ehdr;                 /* Event header */

   /* Standard MATLAB check for proper number and type of
   *  arguments, then locate BBDComData */
   if (nlhs != 0 || nrhs != 2 || !mxIsInt32(prhs[1]))
      abexitm(BBDcErrMexCall, "Bad arguments to bbdcevnt");
   BBDcd = (struct BBDComData *)mxGetData(prhs[0]);
   if (!BBDcd)
      abexitm(BBDcErrMexCall, "Bad BBDcd arg to bbdcevnt");

   /* Check whether server already terminated */
   if (BBDcd->Mlogrc > 0)
      abexitm(BBDcErrServer, ssprintf(NULL, "Server (CNS) terminated "
         "with abexit code %5d", BBDcd->Mlogrc));

   pf = BBDcd->ndsend;
   ecode = *(si32 *)mxGetData(prhs[1]);

   /* Write the event signal */
   ehdr = BBDLC_EVNT;
   rfwi2(pf,ehdr);
   rfwi4(pf,(si32)ecode);

   /* Flush the stream */
   rfflush(pf, ABORT);

   } /* End bbdcevnt() */

