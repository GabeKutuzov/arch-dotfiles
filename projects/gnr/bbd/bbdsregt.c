/* (c) Copyright 2017, The Rockefeller University *11115* */
/* $Id: bbdsregt.c 29 2017-09-15 18:02:30Z  $ */
/***********************************************************************
*                      BBD (Server Side) Package                       *
*                             bbdsregt.c                               *
*                                                                      *
*  Register a video or still camera ("t" is for TV in bbdsregt) for    *
*  image transmission from a BBD to the nervous system in CNS.         *
*  N.B.  After all cameras, senses, effectors, and values have         *
*  been registered, bbdschk() must be called to check that the         *
*  same registrations have been made in the BBD and to establish       *
*  the communications link between client and server.                  *
*                                                                      *
*  Synopsis:                                                           *
*  BBDDev *bbdsregt(char *camnm, char *phnm, int hport,                *
*     si32 camx, si32 camy, int kcol, int kfreq)                       *
*                                                                      *
*  Arguments:                                                          *
*     camnm       Character string giving a name for this camera.      *
*                 This name must not exceed 15 characters and must     *
*                 match the corresponding name given in a bbdcregt()   *
*                 call in the BBD.                                     *
*     phnm        Pointer to client BBD host name or host:port.        *
*                 This should be NULL to indicate the same host        *
*                 that initiated the server.                           *
*     camx,camy   Size of the image along x and y dimensions.          *
*     kcol        Color mode:  sum of relevant bits (see bbd.h):       *
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
*  Returns:                                                            *
*  Location of a BBDDev for this camera (where the caller must         *
*  eventually deposit the data pointer and BBDUsed flags, etc.).       *
*                                                                      *
*  Prerequisites:   bbdsinit                                           *
*                                                                      *
*  Error Handling:                                                     *
*  All errors are terminal and result in calls to abexitm() with       *
*  a suitable message.  This makes it easier to use these routines     *
*  with some application other than CNS.                               *
*                                                                      *
*  Note:  For performance reasons (to minimize communications band-    *
*  width), any temporal or spatial image averaging that is required    *
*  should generally be performed in the client before transmission     *
*  to CNS.  However, if CNS is run on a much faster machine, these     *
*  (and any filters to be defined later) can be specified on the CNS   *
*  CAMERA card and performed in CNS after the images are received.     *
*                                                                      *
************************************************************************
*  V1A, 02/08/07, GNR - New program                                    *
*  ==>, 12/21/07, GNR - Last mod before committing to svn repository   *
*  R29, 05/31/17, GNR - Revise bdflgs for additional color modes       *
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

BBDDev *bbdsregt(char *camnm, char *phnm, 
      si32 camx, si32 camy, int kcol, int kfreq) {
   
   BBDDev *pcam;              /* Ptr to data for this camera */

/* A little basic error checking */

   if (camx <= 0 || camx > SHRT_MAX || camy <= 0 || camy > SHRT_MAX)
      abexitm(BBDsErrParams, "Camera array size out-of-range");
   if (kcol & ~(BBDColMask|BBDBig))
      abexitm(BBDsErrParams, "Unknown camera color mode");
   if (kfreq & ~BBDCFrMask)
      abexitm(BBDsErrParams, "Unknown camera frequency option");
   
/* Allocate a new BBDDev struct to hold the data, clear it, and link
*  it to the end of the appropriate camera chain.  */

   pcam = (BBDDev *)callocv(1, sizeof(BBDDev), "Camera descriptor");
   switch (kfreq) {
   case BBDCFr_TRIAL:
      *BBDsd.ppfttv = pcam;
      BBDsd.ppfttv = &pcam->pndev;
      break;
   case BBDCFr_EVENT:
      *BBDsd.ppfetv = pcam;
      BBDsd.ppfetv = &pcam->pndev;
      break;
   case BBDCFr_SERIES:
      *BBDsd.ppfstv = pcam;
      BBDsd.ppfstv = &pcam->pndev;
      break;
      } /* End kfreq switch */

/* Set the mode and store the data */

   strncpy(pcam->devnm, camnm, MaxDevNameLen);
   pcam->bdflgs = BBDTCam | kcol & BBDBig | 
      (kcol & BBDColMask) << BBDColShft | kfreq << BBDCFrShft;
   pcam->UD.Cam.camx = camx;
   pcam->UD.Cam.camy = camy;
   pcam->ldat = camx * camy * (1 << (kcol & BBDCol_B8));

/* Find or make space to store the host info */

   pcam->pshost = bbdsfhnm(phnm);

   return pcam;

   } /* End bbdsregt() */
