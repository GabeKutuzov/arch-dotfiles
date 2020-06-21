/* (c) Copyright 2007, The Rockefeller University *11115* */
/* $Id: bbdcaztv.c 28 2017-01-13 20:35:15Z  $ */
/***********************************************************************
*                      BBD (Client Side) Package                       *
*                             bbdcaztv.c                               *
*                                                                      *
*   Pick up image data from an "overhead" view as seen by a camera     *
*    with a restricted view pointed at a particular azimuth angle      *
*                                                                      *
*  These routines are not strictly part of the bbdc package, because   *
*  they interact with specific client data structures and pixel access *
*  routines, in this case, with the Allegro game library.  However,    *
*  this file is placed in the bbd source folder where it may serve as  *
*  a useful prototype for implementation in other BBD environments.    *
*                                                                      *
*  The only part of these routines that is specific to the Allegro     *
*  game library is the access to red, green, and blue components of    *
*  pixels in a BITMAP.  This code is clearly marked with comments in   *
*  the aztvgeti() routine, so it should be relatively easy to replace  *
*  this code with suitable alternatives in a different environment.    *
*                                                                      *
*  To implement an azimuth camera, four calls are needed:              *
*  (1) At the start of the run, when the parameters of the azimuth     *
*      camera receptive field are known, call aztvinit().              *
*  (2) For each camera (possibly more than 1) that uses the geometry   *
*      specified in the aztvinit() call, call aztvcchk() to verify     *
*      that this geometry is compatible with the bitmap and camera.    *
*  (3) During each simulation cycle, when the current location and     *
*      orientation of the camera are known, call aztvgeti() to get     *
*      the requested image pixels and transfer them into the pim       *
*      array for transmission to the neural simulation via bbdcputt(). *
*  (4) At the end of the run, call aztvkill() once for each call to    *
*      aztvinit in order to release any allocated storage.             *
*  If there are multiple aztv cameras in a run, aztvinit() and         *
*  aztvkill() only need to be called once for each unique set of       *
*  receptive field parameters.  The data array aztvinit() creates      *
*  can be passed in the calls to aztvcchk() and aztvgeti() for each    *
*  camera that uses those parameters.                                  *
*                                                                      *
*  Error Handling:                                                     *
*     All errors are terminal and result in a call to abexit() with a  *
*     suitable message.  There is nothing useful the caller can do.    *
************************************************************************
*  V1A, 08/13/07, GNR - New program                                    *
*  ==>, 12/17/07, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#define BBD_ALLEGRO

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "sysdef.h"
#include "rksubs.h"
#include "bbd.h"
#include "bbdcxtra.h"
#include "allegro.h"          /* ALLEGRO DEPENDENCY */

#define TORADS  0.017453293   /* Degrees to radians converter */
#define NCOLR3  3             /* Number of colors in color model */

/* Define offsets of items in azrf array */
enum azos { onta, ontl, oscl, oiyh, oixw, oil1, oil2 } ;
/* Define offsets of colors in Col_C24 pixel */
enum colors { Red, Green, Blue } ;

/***********************************************************************
*                              aztvinit                                *
*             Initialize camera receptive field geometry               *
*----------------------------------------------------------------------*
*                                                                      *
*  Synopsis:                                                           *
*     double *aztvinit(float hoang, float scale, int nta, int ntl)     *
*                                                                      *
*  Arguments:                                                          *
*     hoang    Half-opening angle of the camera, measured in degrees   *
*              from a direction straight ahead (along the nta axis).   *
*     scale    Number of bitmap pixels per one image pixel.            *
*     nta      Number of image pixels in the axial (straight ahead)    *
*              direction.                                              *
*     ntl      Number of pixels in the lateral (sideways) direction.   *
*                                                                      *
*  Return value:                                                       *
*     Pointer to an array of doubles that needs to be passed to        *
*     aztvgeti() and aztvkill() in subsequent calls.  The contents     *
*     are of no concern to the caller.                                 *
***********************************************************************/

double *aztvinit(double hoang, double scale, int nta, int ntl) {
   
   double *azrf,*paz;            /* Info array passed to aztvgeti */
   double dsa, dna = (double)nta;
   double dsl, dnl = (double)ntl;
   double hoar = TORADS*hoang;
   double choa,shoa,thoa;        /* = cos, sin, tan of hoang */
   double h,w;                   /* Coords rel to ellipse center */
   double oix,oiy;               /* Offsets of ix,iy from h,w */
   double saa,sal;               /* Axial, lateral semi-axes */
   double sal2,sam2;             /* = sal*sal, sal*sal/(saa*saa) */
   int    ia;                    /* Axis loop counter */
   int    near90;                /* TRUE if hoang is near 90 deg. */

/* A tad of argument checking */

   if (hoang < 15.0 || hoang > 165.0)
      abexitm(159, "Azimuth-camera opening angle is not sensible");
   if (nta < 2 || ntl < 2)
      abexitm(159, "Azimuth-camera image size is not sensible");
   
/* Allocate space for azrf array to contain dimensions of RF, then
*  ellipse origin in image array coords, then bounds for each
*  lateral line at each axial position in the array */

   azrf = (double *)mallocv((nta+oil1)*(2*sizeof(double)),
      "azrf array");
   azrf[onta] = dna, azrf[ontl] = dnl, azrf[oscl] = scale;

/* Calculate semi-axes of RF ellipse and limits for full elliptical
*  portion of image.  As described in design notes, the ellipse is
*  made one pixel too big on purpose to avoid single-point inter-
*  sections with edges of image array.  Store offsets from h,w
*  coords to iy,ix coords in azrf array.  */

   dsa = scale*dna, dsl = scale*dnl;
   choa = cos(hoar), shoa = sin(hoar);
   near90 = fabs(choa) < 0.001;
   azrf[oixw] = oix = 0.5*(dsl - scale);
   if (hoang > 90.0) {
      saa = dsa/(1.0 - choa);
      sal = 0.5*dsl;
      azrf[oiyh] = oiy = (scale - dsa)/(1.0 - choa);
      }
   else {
      saa = dsa;
      sal = dsl/(2.0*shoa);
      azrf[oiyh] = oiy = scale - dsa;
      }

/* Compute constants needed to get intersections of lateral lines
*  at various heights with ellipse.  */

   sal2 = sal*sal, sam2 = sal2/(saa*saa);

/* Store lower and upper limits for looping at each axial position
*  in the azrf array.  These are in h,w coords, i.e. relative to
*  the center of the ellipse.  If the second value is positive, it
*  indicates that there is just one range to loop over at this axial
*  position.  If a negative value is recorded, there are two loop
*  ranges:  First between the limits given (both negative), then
*  between the positive values of the same limits.  */

/* Start with the full elliptical portion of the image.
*  (Include a little redundant safety check to avoid
*  overstepping the azrf array in case of a logic flaw.)  */

   paz = azrf + oil1;
   for (ia=0,h=oiy; ia<nta && h<=0.0; ++ia,h+=scale) {
      w = sqrt(sal2 - sam2*h*h);
      *paz++ = -w, *paz++ = w;
      }

/* If hoang is near 90 degrees, now we are done,
*  exit to avoid divide check calculating tan(hoang) */

   if (near90) while (ia++ < nta)   /* Just in case */
      *paz++ = 0.0, *paz++ = 0.0;

   else {
      thoa = shoa/choa;

/* If hoang > 90 degrees, now have split tails */

      if (hoang > 90.0) for ( ; ia<nta; ++ia,h+=scale) {
         *paz++ = -sqrt(sal2 - sam2*h*h);
         *paz++ = h*thoa;
         }

/* Otherwise, have a single triangular tail */

      else for ( ; ia<nta; ++ia,h+=scale) {
         w = h*thoa;
         *paz++ = -w, *paz++ = w;
         }

      } /* End not near 90 degrees */

   return azrf;
   } /* End aztvinit() */


/***********************************************************************
*                              aztvcchk                                *
*         Check compatibility of bitmap, camera, and geometry          *
*----------------------------------------------------------------------*
*                                                                      *
*  Synopsis:                                                           *
*     void aztvcchk(BITMAP *pBM, BBDDev *pdev, double *azrf)           *
*                                                                      *
*  Arguments:                                                          *
*     pBM      Ptr to BITMAP structure defining an Allegro bitmap      *
*              where the image to be viewed by the camera is stored.   *
*     pdev     Ptr to BBDDev structure defining the camera.            *
*     azrf     Ptr to info array returned by a previous call to        *
*              aztvinit().  This array defines the geometry of the     *
*              camera's receptive field.                               *
***********************************************************************/

void aztvcchk(BITMAP *pBM, BBDDev *pdev, double *azrf) {
   
   if ((double)pBM->w < azrf[oscl]*azrf[ontl] ||
         (double)pBM->h < azrf[oscl]*azrf[onta])
      abexitm(158, "Bitmap is smaller than camera field size");
   if ((pdev->bdflgs & BBDTMask) != BBDTCam)
      abexitm(158, "Azimuth camera device is not a camera");
   if ((pdev->bdflgs >> BBDColShft & BBDColMask) != BBDCol_C24)
      abexitm(158, "Azimuth camera requires 24-bit color mode");
   if ((double)pdev->UD.Cam.camx != azrf[ontl] ||
         (double)pdev->UD.Cam.camy != azrf[onta])
      abexitm(158, "Azimuth camera RF does not match image size");

   } /* End aztvcchk() */


/***********************************************************************
*                              aztvgeti                                *
*              Generate rotated image for one time step                *
*----------------------------------------------------------------------*
*                                                                      *
*  Synopsis:                                                           *
*     void aztvgeti(BITMAP *pBM, BBDDev *pdev, double *azrf,           *
*        double o, double x, double y)                                 *
*                                                                      *
*  Arguments:                                                          *
*     pBM      Ptr to BITMAP structure defining an Allegro bitmap      *
*              where the image to be viewed by the camera is stored.   *
*     pdev     Ptr to BBDDev structure defining the camera.  The pim   *
*              array belonging to this camera should be cleared when   *
*              allocated, because aztvgeti() does not clear pixels     *
*              that are not included in the camera's receptive field.  *
*     azrf     Ptr to info array returned by a previous call to        *
*              aztvinit().  This array defines the geometry of the     *
*              camera's receptive field, which does not change from    *
*              one time step to another.                               *
*     o        Orientation of the camera at this time step.  This      *
*              is measured in degrees clockwise from vertical as       *
*              defined in the pBM bitmap to the direction of the       *
*              positive camera axis as defined by the nta variable     *
*              in the aztvinit() call.                                 *
*     x        X-axis position of the camera center in the bitmap.     *
*     y        Y-axis position of the camera center in the bitmap.     *
*                                                                      *
*  Prerequisites:                                                      *
*     aztvinit() must be called first to set up the azrf array and     *
*     aztvcchk() must be called to be sure the pBM, pdev, and azrf     *
*     arguments point to compatible data structures.                   *
***********************************************************************/

void aztvgeti(BITMAP *pBM, BBDDev *pdev, double *azrf,
      double o, double x, double y) {

   byte *pima,*piml;                /* Ptrs to bbd image data */
   double *paz;                     /* Ptr into azrf array */
   double ra,rl;                    /* Axial, lateral RF coords */
   double rado;                     /* Angle o in radians */
   double coso,sino;                /* Cos(o), sin(o) */
   double oix;                      /* Offset of ix from w */
   double scale;                    /* BM pixels per image pixel */
   double w1,w2;                    /* Lateral loop limits */
   double xa,ya;                    /* X,Y at current axial radius */
   int nta,ntl;                     /* Axial, lateral RF dims */
   int ia,il;                       /* Axial, lateral pixel indexes */
   int il1,il2;                     /* Lateral loop limits */
   int ll3;                         /* Bytes in a row of pixels */
   int pix;                         /* Current pixel value */
   int qx,qy;                       /* Bitmap coords of pixel */

   rado = TORADS*o;
   coso = cos(rado), sino = sin(rado);
   nta = (int)azrf[onta], ntl = (int)azrf[ontl];
   scale = azrf[oscl];
   ll3 = NCOLR3 * ntl;
   paz = azrf + oil1;

   /* Ptr to first row of image data */
   pima = pdev->UD.Cam.pim;

   /* Note:  The axial and lateral loops are coded with integer
   *  limits rather than the obvious ra,rl limits to provide
   *  absolute safety against overflowing pim array bounds.  */
   oix = azrf[oixw], ra = azrf[oiyh];
   for (ia=0; ia<nta; ++ia,ra+=scale,pima+=ll3) {    /* Axial loop */
      w1 = *paz++, w2 = *paz++;
      xa = x - ra*sino + 0.5, ya = y YOP ra*coso + 0.5;
      while (1) {                                    /* Range loop */
         il1 = ceil((w1 + oix)/scale);
         il2 = floor((w2 + oix)/scale);
         /* Safety checks--possible to delete after debugging */
         if (il1 < 0) il1 = 0; else if (il1 >= ntl) il1 = ntl-1;
         if (il2 < 0) il2 = 0; else if (il2 >= ntl) il2 = ntl-1;
         piml = pima + NCOLR3*il1;                 /* Lateral loop */
         for (rl=w1,il=il1; il<=il2; ++il,piml+=NCOLR3,rl+=scale) {
            /* Rotate coords to bitmap system */
            qx = (int)(xa  +  rl*coso);
            qy = (int)(ya YOP rl*sino);
/*---------------------------------------------------------------------*
*                      BEGIN ALLEGRO DEPENDENCY                        *
*---------------------------------------------------------------------*/
            pix = getpixel(pBM, qx, qy);
            if (pix == -1) {
               /* Point is outside the bitmap */
               piml[Red] = piml[Green] = piml[Blue] = 0;
               }
            else {
               /* Point is inside the bitmap, pick up color values */
               piml[Red]   = getr24(pix);
               piml[Green] = getg24(pix);
               piml[Blue]  = getb24(pix);
               }
/*---------------------------------------------------------------------*
*                       END ALLEGRO DEPENDENCY                         *
*---------------------------------------------------------------------*/
            } /* End il loop */
         if (w2 >= 0) break;              /* Single range */
         rl = w1; w1 = fabs(w2); w2 = fabs(rl);
         } /* End range loop */
      } /* End ia loop */

   } /* End aztvgeti() */


/***********************************************************************
*                              aztvkill                                *
*            Release storage allocated for camera geometry             *
*----------------------------------------------------------------------*
*                                                                      *
*  Synopsis:                                                           *
*     void aztvkill(double *azrf)                                      *
*                                                                      *
*  Argument:                                                           *
*     azrf     Pointer to info array returned by a previous call to    *
*              aztvinit().                                             *
*                                                                      *
*  Note:  After this call returns, the azrf array no longer exists     *
*  and cannot be used in further aztvgeti() calls.                     *
***********************************************************************/

void aztvkill(double *azrf) {
   
   if (azrf) freev(azrf, "azrf array");

   } /* End aztvkill() */
