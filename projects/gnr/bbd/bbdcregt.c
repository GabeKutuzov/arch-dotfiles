/* (c) Copyright 2017, The Rockefeller University *11115* */
/* $Id: bbdcregt.c 29 2017-09-15 18:02:30Z  $ */
/***********************************************************************
*                      BBD (Client Side) Package                       *
*                             bbdcregt.c                               *
*                                                                      *
*  Register a camera image ("t" is for TV in bbdcregt) for trans-      *
*  mission to the nervous system of a BBD.  N.B.  After all TVs,       *
*  senses, effectors, and values have been registered, bbdcchk()       *
*  must be called to check that the same registrations have been       *
*  made in CNS and to establish the communications link between        *
*  client and server.                                                  *
*                                                                      *
*  Synopsis:                                                           *
*  BBDDev *bbdcregt(char *camnm, byte *pim, si32 camx, si32 camy,      *
*     int kcol, int kfreq)                                             *
*                                                                      *
*  Arguments:                                                          *
*     camnm       Character string giving a name to this camera.       *
*                 This name must not exceed 15 characters and must     *
*                 match the corresponding name on the CAMERA card      *
*                 in the CNS control file.                             *
*     pim         Ptr to static image array of size camx x camy.       *
*                 The image will be sent from this location to CNS     *
*                 each time bbdcsimg() is called.                      *
*     camx,camy   Size of the image along x and y dimensions.          *
*     kcol        Color mode:                                          *
*                    BBDBig        (0x80) Data are big-endian          *
*                    BBDCol_C8     (0x10) Legacy C8 mode (2-3-3 color) *
*                    BBDCol_C16    (0x11) Legacy C16 mode (5-5-5 color)*
*                    BBDCol_Pal    (0x12) Legacy Palette (color index) *
*                    BBDCol_C24    (0x0a) Combo code for 24-bit color  *
*                    BBDCol_GS     (0x00) Grayscale image              *
*                    BBDCol_Col    (0x08) Colored image                *
*                    BBDCol_Int    (0x00) Unsigned integer data        *
*                    BBDCol_Flt    (0x04) Real, i.e. float-point data  *
*                    BBDCol_B8     (0x03) 8 bytes per color            *
*                    BBDCol_B4     (0x02) 4 bytes per color            *
*                    BBDCol_B2     (0x01) 2 bytes per color            *
*                    BBDCol_B1     (0x00) 1 byte per color             *
*     kfreq       Frequency of sending data to CNS:                    *
*                    BBDCFr_TRIAL  (0x01) Grab on every trial          *
*                    BBDCFr_EVENT  (0x02) Grab on designated events    *
*                    DDBCFr_SERIES (0x03) Grab on new series only      *
*                                                                      *
*  Return value:                                                       *
*     Ptr to BBDDev structure defining this camera.                    *
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
*                                                                      *
************************************************************************
*  V1A, 02/07/07, GNR - New program                                    *
*  ==>, 12/21/07, GNR - Last mod before committing to svn repository   *
*  R29, 05/31/17, GNR - Revise bdflgs for additional color modes       *
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "bbd.h"
#include "rksubs.h"

extern struct BBDComData BBDcd;

BBDDev *bbdcregt(char *camnm, byte *pim, si32 camx, si32 camy,
      int kcol, int kfreq) {
   
   BBDDev *pcam;              /* Ptr to data for this camera */

/* A little basic error checking */

   if (camx <= 0 || camx > SHRT_MAX || camy <= 0 || camy > SHRT_MAX)
      abexitm(BBDcErrParams, "Camera array size out-of-range");
   if (kcol & ~(BBDColMask|BBDBig))
      abexitm(BBDcErrParams, "Unknown camera color mode");
   if (kfreq & ~BBDCFrMask)
      abexitm(BBDcErrParams, "Unknown camera frequency option");
   
/* Allocate a new BBDDev struct to hold the data, clear it,
*  link it onto the end of the correct chain according to the
*  kfreq option, and mark it CAMERA type.  */

   pcam = (BBDDev *)callocv(1, sizeof(BBDDev), "Camera descriptor");
   switch (kfreq) {
   case BBDCFr_TRIAL:
      *BBDcd.ppfttv = pcam;
      BBDcd.ppfttv = &pcam->pndev;
      break;
   case BBDCFr_EVENT:
      *BBDcd.ppfetv = pcam;
      BBDcd.ppfetv = &pcam->pndev;
      break;
   case BBDCFr_SERIES:
      *BBDcd.ppfstv = pcam;
      BBDcd.ppfstv = &pcam->pndev;
      break;
      } /* End kfreq switch */
   pcam->bdflgs = BBDTCam | kcol & BBDBig |
      (kcol & BBDColMask) << BBDColShft | kfreq << BBDCFrShft;

/* Store the data */

   pcam->UD.Cam.pim  = pim;
   pcam->UD.Cam.camx = camx;
   pcam->UD.Cam.camy = camy;
   pcam->ldat = camx * camy * (1 << (kcol & BBDCol_B8));
   strncpy(pcam->devnm, camnm, MaxDevNameLen);

   return pcam;
   } /* End bbdcregt() */
