/* (c) Copyright 2010, Neurosciences Research Foundation, Inc. */
/* $Id: d3gvin.c 29 2010-06-15 22:02:00Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3gvin                                 *
*                                                                      *
*  This routine returns a pointer to a routine that can be used to     *
*  pick up any virtual input data (virtual group, gray-scale or        *
*  colored image data from the input array or an image in any of       *
*  the supported color modes described by enum ColorMode.  It is       *
*  intended for use by touch senses (d3vtch and d3vitp) and virtual    *
*  modulatory connection types.  Testing for invalid combinations      *
*  (extracting color from a BandW source) should be done earlier.      *
*                                                                      *
*  Synopsis:  gvin_fn *d3gvin(int ksrc, struct EXTRCOLR *pxc)          *
*  where gvin_fn is a function of the following type                   *
*  typedef si32 gvin_fn(byte *pvg, struct EXTRCOLR *pxc)               *
*                                                                      *
*  Arguments to the setup routine:                                     *
*     ksrc        Kind of source, one of the codes from d3global.h.    *
*     pxc         Ptr to an EXTRCOLR structure defining the source     *
*                 color mode and desired color selection.              *
*                                                                      *
*  Arguments to the data pick-up routine:                              *
*     pvg         Ptr to the data item.  It is assumed that input      *
*                 validating code will prevent use of this routine     *
*                 by a caller that cannot handle the data type in      *
*                 question (e.g. TV data by d3vtch).                   *
*     pxc         Same as above.                                       *
*                                                                      *
*  The selected input data are returned on scale S14 = mV S7.          *
*                                                                      *
*  All of these routines currently are expected to be compiled and     *
*  run only on serial and PAR0 nodes.                                  *
************************************************************************
*  Written by George N. Reeke                                          *
*  V8D, 04/01/07, GNR - New routine based on d3sj                      *
*  ==>, 08/30/07, GNR - Last mod before committing to svn repository   *
*  Rev, 06/03/08, GNR - Add color opponency                            *
*  V8F, 04/20/10, GNR - Handle PREPROC inputs                          *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "swap.h"
#include "nccolors.h"

/*---------------------------------------------------------------------*
*              Prototypes for image data lookup routines               *
*---------------------------------------------------------------------*/

si32 gvgggs(byte *pvg, struct EXTRCOLR *pxc); /* Gray from 8-bit gray */
si32 gvggr4(byte *pvg, struct EXTRCOLR *pxc); /* Gray from float data */
si32 giag08(byte *pvg, struct EXTRCOLR *pxc); /* IA -- Gray from C8 */
si32 gtvg08(byte *pvg, struct EXTRCOLR *pxc); /* TV -- Gray from C8 */
si32 gtvg16(byte *pvg, struct EXTRCOLR *pxc); /* TV -- Gray from C16 */
si32 gtvg24(byte *pvg, struct EXTRCOLR *pxc); /* TV -- Gray from C24 */
si32 gimc08(byte *pvg, struct EXTRCOLR *pxc); /* IA,TV Color from C8 */
si32 gimo08(byte *pvg, struct EXTRCOLR *pxc); /* IA,TV OpCol from C8 */
si32 gtvc16(byte *pvg, struct EXTRCOLR *pxc); /* TV -- Color from C16 */
si32 gtvo16(byte *pvg, struct EXTRCOLR *pxc); /* TV -- OpCol from C16 */
si32 gtvc24(byte *pvg, struct EXTRCOLR *pxc); /* TV -- Color from C24 */
si32 gtvo24(byte *pvg, struct EXTRCOLR *pxc); /* TV -- OpCol from C24 */

/***********************************************************************
*                               d3gvin                                 *
*                                                                      *
*  Return a pointer to the appropriate virtual input pick-up routine   *
***********************************************************************/

gvin_fn d3gvin(int ksrc, struct EXTRCOLR *pxc) {

   int ktvc = pxc->tvcsel;
   int mode = pxc->tvmode;

   /* Tables of gvin routines selected by color mode */
   static gvin_fn GTvo[3] =
      { (gvin_fn)gimo08, (gvin_fn)gtvo16, (gvin_fn)gtvo24 };
   static gvin_fn GTvc[3] =
      { (gvin_fn)gimc08, (gvin_fn)gtvc16, (gvin_fn)gtvc24 };
   static gvin_fn GTvg[4] = { (gvin_fn)gvgggs,
        (gvin_fn)gtvg08, (gvin_fn)gtvg16, (gvin_fn)gtvg24 };

   if (ksrc == IA_SRC) {            /* Input from input array */
      if ((enum ColorMode)mode == Col_GS)
         return gvgggs;
      if ((enum ColorSens)ktvc == BandW)
         return giag08;
      if ((enum ColorSens)ktvc >= RmG)
         return gimo08;
      else
         return gimc08;
      } /* End IA source */
   else if (ksrc == TV_SRC || ksrc == PP_SRC) { /* Image input */
      if ((enum ColorSens)ktvc >= RmG)
         /* Color opponency being extracted */
         return GTvo[mode - Col_C8];
      if ((enum ColorSens)ktvc > BandW)
         /* A single color is being extracted */
         return GTvc[mode - Col_C8];
      else
         /* Cases where a gray value is desired */
         return GTvg[mode];
      } /* End TV or PP source */
   else if ((enum ColorMode)mode == Col_R4)
      return gvggr4;                /* Input from float sensor */
   else
      return gvgggs;                /* Input from other VGs */

   } /* End d3gvin() */


/***********************************************************************
*                  Virtual input extraction routines                   *
*                                                                      *
*  These routines extract the correct input color or gray scale value  *
*  according to the input source and video mode.  By making a separate *
*  routine for each kind of source, a switch in the inner loop is      *
*  replaced with a call via a function pointer, which should be faster.*
*     In all cases, the data value is returned as an S14 fraction,     *
*  equivalent to mV on S7 scale.                                       *
***********************************************************************/

/*---------------------------------------------------------------------*
*      gvggggs:  Get gray-scale input from gray-scale input data       *
*  The color specifier is irrelevant (and an error will have been      *
*  generated earlier if it is not BandW).                              *
*---------------------------------------------------------------------*/

si32 gvgggs(byte *pvg, struct EXTRCOLR *pxc) {

   return (si32)*pvg << Sv2mV;

   } /* End gvgggs() */

/*---------------------------------------------------------------------*
*      gvggr4:  Get input from a floating-point external sensor        *
*---------------------------------------------------------------------*/

si32 gvggr4(byte *pvg, struct EXTRCOLR *pxc) {

   float *pfvg = (float *)pvg;
   return (si32)(RP->fvcscl * (*pfvg));

   } /* End gvggr4() */

/*---------------------------------------------------------------------*
*    giag08:  Get gray-scale input from an 8-bit color input array     *
*  This uses the traditional GrayFromRGB macro for compatibility       *
*---------------------------------------------------------------------*/

si32 giag08(byte *pvg, struct EXTRCOLR *pxc) {

   si32 v = (si32)(*pvg);
   v = GrayFromRGB(v);
   return v << Sv2mV;

   } /* End giag08() */

/*---------------------------------------------------------------------*
*    gtvg08:  Get gray-scale input from an 8-bit color input array     *
*  This uses a more accurate averaging algorithm that is possible      *
*  because the result is S14 rather than the old S8.                   *
*---------------------------------------------------------------------*/

si32 gtvg08(byte *pvg, struct EXTRCOLR *pxc) {

   si32 v = (si32)(*pvg);
   v = (((v << 7 & 0x6000) +
      (v << 9 & 0x7000) + (v << 12 & 07000))/NColorDims + 1) >> 1;

   return v;

   } /* End gtvg08() */

/*---------------------------------------------------------------------*
*       gtvg16:  Get gray-scale input from a 16-bit color image        *
*---------------------------------------------------------------------*/

si32 gtvg16(byte *pvg, struct EXTRCOLR *pxc) {

   si32 v = (si32)(bemtoi2(pvg));
   v = ((v & 0x7c00) + (v << 5 & 0x7c00) + (v << 10 & 0x7c00))/
      NColorDims;
   return (v + 1) >> 1;

   } /* End gtvg16() */

/*---------------------------------------------------------------------*
*       gtvg24:  Get gray-scale input from a 24-bit color image        *
*---------------------------------------------------------------------*/

si32 gtvg24(byte *pvg, struct EXTRCOLR *pxc) {

   return ((((si32)pvg[0] + (si32)pvg[1] + (si32)pvg[2]) <<
      (Sv2mV+1))/NColorDims + 1) >> 1;

   } /* End gtvg24() */

/*---------------------------------------------------------------------*
*    gimc08:  Get colored input from an 8-bit colored IA or image      *
*---------------------------------------------------------------------*/

si32 gimc08(byte *pvg, struct EXTRCOLR *pxc) {

   return ((si32)*pvg & pxc->tvmask[0]) << pxc->tvshft[0];

   } /* End gimc08() */

/*---------------------------------------------------------------------*
* gimo08:  Get color opponent input from an 8-bit colored IA or image  *
*---------------------------------------------------------------------*/

si32 gimo08(byte *pvg, struct EXTRCOLR *pxc) {

   si32 v,rgb[NColorDims];

   v = (si32)*pvg;
   rgb[0] = (v & 0x07) << 11;    /* Red */
   rgb[1] = (v & 0x38) <<  8;    /* Green */
   rgb[2] = (v & 0xc0) <<  6;    /* Blue */

   return rgb[pxc->tvshft[0]] - rgb[pxc->tvmask[0]];

   } /* End gimo08() */

/*---------------------------------------------------------------------*
*          gtvc16:  Get the pixel from a 16-bit color image            *
*---------------------------------------------------------------------*/

si32 gtvc16(byte *pvg, struct EXTRCOLR *pxc) {

   si32 v = (si32)(bemtoi2(pvg));

   return (v << pxc->tvshft[0] & 0x7c00) >> 1;

   } /* End vitpgc16() */

/*---------------------------------------------------------------------*
*       gtvo16:  Get color opponency from a 16-bit color image         *
*---------------------------------------------------------------------*/

si32 gtvo16(byte *pvg, struct EXTRCOLR *pxc) {

   si32 v,rgb[NColorDims];

   v = (si32)(bemtoi2(pvg));
   rgb[0] = v >> 1 & 0x3e00;     /* Red */
   rgb[1] = v << 4 & 0x3e00;     /* Green */
   rgb[2] = v << 9 & 0x3e00;     /* Blue */

   return rgb[pxc->tvshft[0]] - rgb[pxc->tvmask[0]];

   } /* End vitpgo16() */

/*---------------------------------------------------------------------*
*          gtvc24:  Get the pixel from a 24-bit color image            *
*---------------------------------------------------------------------*/

si32 gtvc24(byte *pvg, struct EXTRCOLR *pxc) {

   return (si32)pvg[pxc->tvshft[0]] << Sv2mV;

   } /* End gtvc24() */

/*---------------------------------------------------------------------*
*       gtvo24:  Get color opponency from a 24-bit color image         *
*---------------------------------------------------------------------*/

si32 gtvo24(byte *pvg, struct EXTRCOLR *pxc) {

   return ((si32)pvg[pxc->tvshft[0]] - (si32)pvg[pxc->tvmask[0]]) <<
      Sv2mV;

   } /* End gtvo24() */
