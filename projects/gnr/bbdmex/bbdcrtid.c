/* (c) Copyright 2017, The Rockefeller University *11116* */
/* $Id: bbdcrtid.c 17 2017-10-13 19:00:07Z  $ */
/***********************************************************************
*               BBD (Client Side) mex-Function Package                 *
*                             bbdcrtid.c                               *
*                        Register a BBD camera                         *
*                                                                      *
*  This is the matlab mex-function version of bbdcrtid().              *
*                                                                      *
*  Register a camera image source ("t" is for TV in bbdcrtid) for      *
*  transmission to the nervous system of a BBD.  This image source     *
*  will also return group and stimulus id information to the server.   *
*                                                                      *
*  N.B.  After all TVs, senses, effectors, and values have been        *
*  registered, bbdcchk() must be called to check that the same         *
*  registrations have been made in CNS and to establish the            *
*  communications link between client and server.                      *
*                                                                      *
*  MATLAB Synopsis:                                                    *
*     bbdcrtid(BBDComm, camnm, camx, camy, kcol, kfreq)                *
*                                                                      *
*  Arguments:                                                          *
*     BBDComm  MATLAB array containing private data returned by a      *
*              previous call to bbdcinit.                              *
*     camnm    Character string giving a name to this camera.          *
*              This name must not exceed 15 characters and must        *
*              match the corresponding name on the CAMERA card         *
*              in the CNS control file.                                *
*     camx,camy   MATLAB int32 integers giving the size of this        *
*              image along the x and y dimensions.                     *
*     kcol     MATLAB int32 integer giving the color mode:             *
*                 BBDBig        (0x80) Data are big-endian             *
*                 BBDCol_C8     (0x10) Legacy C8 mode (2-3-3 color)    *
*                 BBDCol_C16    (0x11) Legacy C16 mode (5-5-5 color)   *
*                 BBDCol_Pal    (0x12) Legacy Palette (color index)    *
*                 BBDCol_C24    (0x0a) Combo code for 24-bit color     *
*                 BBDCol_GS     (0x00) Grayscale image                 *
*                 BBDCol_Col    (0x08) Colored image                   *
*                 BBDCol_Int    (0x00) Unsigned integer data           *
*                 BBDCol_Flt    (0x04) Real, i.e. float-point data     *
*                 BBDCol_B8     (0x03) 8 bytes per color               *
*                 BBDCol_B4     (0x02) 4 bytes per color               *
*                 BBDCol_B2     (0x01) 2 bytes per color               *
*                 BBDCol_B1     (0x00) 1 byte per color                *
*     kfreq    MATLAB int32 integer giving the frequency of sending    *
*              data to CNS for this image:                             *
*                 BBDCFr_TRIAL  (0x01) Grab on every trial             *
*                 BBDCFr_EVENT  (0x02) Grab on designated events       *
*                 DDBCFr_SERIES (0x03) Grab on new series only         *
*                                                                      *
*  Prerequisites:                                                      *
*  bbdcinit() must be called before this routine is called.            *
*                                                                      *
*  Error Handling:                                                     *
*  All errors are terminal and result in calls to abexitm() with       *
*  a suitable message.  There is nothing useful the caller can do      *
*  to recover.                                                         *
*                                                                      *
*  Note:  For performance reasons (to minimize communications          *
*  bandwidth), any temporal or spatial image averaging that is         *
*  required should generally be performed in the client before         *
*  transmission of images to CNS.  However, if CNS is run on           *
*  a much faster machine, these (and any filters to be defined         *
*  later) can be specified on the CNS CAMERA card and performed        *
*  in CNS after the images are received.                               *
************************************************************************
*  V1A, 11/09/10, GNR - New mex program derived from bbdcregt.c        *
*  ==>, 11/09/10, GNR - Last mod before committing to svn repository   *
*  R29, 05/31/17, GNR - Revise bdflgs for additional color modes       *
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
   si32   camx,camy;          /* Camera dimensions */
   si32   kcol,kfreq;         /* kcol,kfreq args */
   BBDDev *pcam;              /* Ptr to data for this camera */

   /* Standard MATLAB check for proper number and type of
   *  arguments, then locate BBDComData */

   if (nlhs != 0 || nrhs != 6 || !mxIsChar(prhs[1]) ||
         !mxIsInt32(prhs[2]) || !mxIsInt32(prhs[3]) ||
         !mxIsInt32(prhs[4]) || !mxIsInt32(prhs[5]))
      abexitm(BBDcErrMexCall, "Bad arguments to bbdcrtid");
   BBDcd = (struct BBDComData *)mxGetData(prhs[0]);
   if (!BBDcd)
      abexitm(BBDcErrMexCall, "Bad BBDcd arg to bbdcrtid");
   camx = *(si32 *)mxGetData(prhs[2]);
   camy = *(si32 *)mxGetData(prhs[3]);
   kcol  = *(si32 *)mxGetData(prhs[4]);
   kfreq = *(si32 *)mxGetData(prhs[5]);

/* A little basic error checking */

   if (mxGetNumberOfElements(prhs[1]) > 15)
      abexitm(BBDcErrParams, "Camera name > 15 chars");
   if (camx <= 0 || camx > SHRT_MAX || camy <= 0 || camy > SHRT_MAX)
      abexitm(BBDcErrParams, "Camera array size out-of-range");
   if (kcol & ~(BBDColMask|BBDBig))
      abexitm(BBDcErrParams, "Unknown camera color mode");
   if (kfreq & ~BBDCFrMask)
      abexitm(BBDcErrParams, "Unknown camera frequency option");

/* Allocate a new BBDDev struct to hold the data, clear it,
*  link it into the end of the correct chain according to the
*  kfreq option, and mark it CAMERA type.  */

   pcam = (BBDDev *)mxCalloc(1, sizeof(BBDDev));
   mexMakeMemoryPersistent(pcam);
   switch (kfreq) {
   case BBDCFr_TRIAL:
      *BBDcd->ppfttv = pcam;
      BBDcd->ppfttv = &pcam->pndev;
      break;
   case BBDCFr_EVENT:
      *BBDcd->ppfetv = pcam;
      BBDcd->ppfetv = &pcam->pndev;
      break;
   case BBDCFr_SERIES:
      *BBDcd->ppfstv = pcam;
      BBDcd->ppfstv = &pcam->pndev;
      break;
      } /* End kfreq switch */
   pcam->bdflgs = BBDTCam | BBDMdlt | kcol & BBDBig |
      (kcol & BBDColMask) << BBDColShft | kfreq << BBDCFrShft;

/* Store the data */

   pcam->UD.Cam.camx = camx;
   pcam->UD.Cam.camy = camy;
   pcam->ldat = camx * camy * (1 << (kcol&3));
   mxGetString(prhs[1], pcam->devnm, MaxDevNameLen+1);

   } /* End bbdcrtid() */
