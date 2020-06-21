/* (c) Copyright 2007-2018, The Rockefeller University *11115* */
/* $Id: d3gvin.c 78 2018-08-02 18:36:58Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3gvin                                 *
*                                                                      *
*  This routine returns a pointer to a routine that can be used to     *
*  pick up any virtual input data (virtual group, gray-scale or        *
*  colored image data from the input array or an image in any of       *
*  the supported internal color modes (BM_GS, BM_GS16, BM_C8, BM_C16,  *
*  BM_C24, BM_C48 defined in plotdefs.h, formerly enum ColorMode).     *
*  d3gvin is used by image touch sensors (d3vitp), virtual modulatory  *
*  connections (d3go), and detailed print of raw sj (d3dprt, d3pdpz).  *
*  It is also intended for use by IA touch senses (d3vtch), but this   *
*  has not been implemented.  There are no routines to pick steerable  *
*  filter (PP_STGH) data because none of these targets has orientation *
*  specificity (d3kij/d3sj handles real conntypes).                    *
*                                                                      *
*  N.B.  Images in memory use only the modes listed above.  Others are *
*  converted to these in d3imgt, so should not be a problem here.      *
*                                                                      *
*  Synopsis:  gvin_fn d3gvin(int ksrc, struct EXTRCOLR *pxc)           *
*  where gvin_fn is a function of the following type                   *
*  typedef si32 (*gvin_fn)(byte *pvg, struct EXTRCOLR *pxc)            *
*                                                                      *
*  Arguments to the setup routine:                                     *
*     ksrc        Kind of source, one of the codes from d3global.h.    *
*                 DP_SRC is a special code for d3dprt RGB output       *
*                 used to invoke GTvc routine.                         *
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
*  Design decree:  All images in core will be native system endian--   *
*  input routines (d3imgt) must reverse if needed as they are called   *
*  only once per image while images may be accessed multiple times.    *
*                                                                      *
*  All of these routines currently are expected to be compiled and     *
*  run only on serial and PARn nodes.                                  *
************************************************************************
*  Written by George N. Reeke                                          *
*  V8D, 04/01/07, GNR - New routine based on d3sj                      *
*  ==>, 08/30/07, GNR - Last mod before committing to svn repository   *
*  Rev, 06/03/08, GNR - Add color opponency                            *
*  V8F, 04/20/10, GNR - Handle PREPROC inputs                          *
*  R74, 06/01/17, GNR - Eliminate old GrayFromRGB and ColorMode, use   *
*                       BM_xxxx codes, special ksrc == REPSRC code.    *
*  R78, 04/23/18, GNR - Image & preproc gray-from-color now in d3imgt, *
*                       remove C16 access and C8 except from IA        *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "swap.h"

/*---------------------------------------------------------------------*
*              Prototypes for image data lookup routines               *
*---------------------------------------------------------------------*/

si32 gvgggs(byte *pvg, struct EXTRCOLR *pxc); /* Gray from 8-bit gray */
si32 gvggr4(byte *pvg, struct EXTRCOLR *pxc); /* Gray from float data */
si32 gtvc24(byte *pvg, struct EXTRCOLR *pxc); /* TV -- Color from C24 */
si32 gtvo24(byte *pvg, struct EXTRCOLR *pxc); /* TV -- OpCol from C24 */
si32 gvgg16(byte *pvg, struct EXTRCOLR *pxc); /* Gray from 16-bit gry */
si32 gtvc48(byte *pvg, struct EXTRCOLR *pxc); /* TV -- Color from C48 */
si32 gtvo48(byte *pvg, struct EXTRCOLR *pxc); /* TV -- OpCol from C48 */
si32 giag08(byte *pvg, struct EXTRCOLR *pxc); /* IA -- Gray from C8  */
si32 giac08(byte *pvg, struct EXTRCOLR *pxc); /* IA -- Color from C8 */
si32 giao08(byte *pvg, struct EXTRCOLR *pxc); /* IA -- OpCol from C8 */
si32 gppg16(byte *pvg, struct EXTRCOLR *pxc); /* PREPR Gray from GS16 */
si32 gppc48(byte *pvg, struct EXTRCOLR *pxc); /* PREPR Color from C48 */
si32 gppo48(byte *pvg, struct EXTRCOLR *pxc); /* PREPR OpCol from C48 */

/***********************************************************************
*                               d3gvin                                 *
*                                                                      *
*  Return a pointer to the appropriate virtual input pick-up routine   *
*                                                                      *
*  Note:  Earlier versions of this code had different mode selections  *
*  depending on ksrc.  This is no longer necessary.  Although some of  *
*  the cases encompassed by the shorter code should never occur, it    *
*  is the intent that all error cases should be captured here.         *
*                                                                      *
*  Further Note:  d3kij/d3sj has to handle IA separately because of    *
*  need to check Lij out-of-bounds for moving windows.  Not so here.   *
***********************************************************************/

gvin_fn d3gvin(int ksrc, struct EXTRCOLR *pxc) {

   enum ColorSens etvc = (ksrc == DP_SRC) ? (enum ColorSens)Red :
      (enum ColorSens)(pxc->tvcsel & EXMASK);
   enum gsj_e { GfmG, GfmC, CfmC, OfmC } egsj;
   int mode = pxc->tvmode & BM_MASK;   /* Source color mode */

/* Table giving type of pixel packing vs BM_xxx tvmode:
*  0 => 8 bits/color, 1 => 16 bits/color, 2 => 233 packed,
*  3 => 555 packed, 4 => Not supported.  */
   static byte tvsm[8] = { 4, 0, 1, 1, 4, 2, 3, 0 };
/* Tables of gvin_fns selected by operation per source type */
   static gvin_fn GTpp[4] = {          /* Preprocessor */
      (gvin_fn)gppg16, (gvin_fn)gppg16,
      (gvin_fn)gppc48, (gvin_fn)gppo48 };
   static gvin_fn GT08[4] = {          /* 8-bit/color camera or IA */
      (gvin_fn)gvgggs, (gvin_fn)gvgggs,
      (gvin_fn)gtvc24, (gvin_fn)gtvo24 };
   static gvin_fn GT16[4] = {          /* 16-bit/color camera */
      (gvin_fn)gvgg16, (gvin_fn)gvgg16,
      (gvin_fn)gtvc48, (gvin_fn)gtvo48 };
   static gvin_fn GT233[4] = {         /* 233-packed camera or IA */
      NULL, (gvin_fn)giag08, (gvin_fn)giac08, (gvin_fn)giao08 };

   /* Input from a value--no gvin_fn is needed */
   if (ksrc == VALSRC) return NULL;
   /* Input from real virtual groups -- flagged as BM_NONSTD.  Could
   *  add more cases (other bits with BM_NONSTD) here if needed.  */
   if (pxc->tvmode & BM_NONSTD) return gvggr4;

   /* Remaining cases depend on color selection option */
   if (qGray(mode)) egsj = GfmG;          /* Gray from gray */
   else if (etvc == BandW) egsj = GfmC;   /* Gray from color */
   else if (etvc < Opp) egsj = CfmC;      /* Color from color */
   else egsj = OfmC;                      /* Opponent colors */

   if (ksrc == PP_SRC)
      return GTpp[egsj];
   switch (tvsm[mode]) {
   case 0:                             /* 8-bit/color camera */
      return GT08[egsj];
   case 1:                             /* 16-bit/color camera */
      return GT16[egsj];
   case 2:                             /* 233-packed cam or IA */
      return GT233[egsj];
   default:                            /* Now includes case 3 */
      d3exit("Unsupported bitmap", COLOR_ERR, mode);
      } /* End pixel storage mode switch */

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
*     N.B.  The code before COMPAT=R was added in R74 never added      *
*  rounding to gray inputs, only color inputs, so we do not do it now. *
***********************************************************************/


/*---------------------------------------------------------------------*
*      gvgggs:  Get gray-scale input from gray-scale input data        *
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
*          gtvc24:  Get the pixel from a 24-bit color image            *
*---------------------------------------------------------------------*/

si32 gtvc24(byte *pvg, struct EXTRCOLR *pxc) {

   int iex = (int)pxc->tveoff;
   return (si32)pvg[iex] << Sv2mV;

   } /* End gtvc24() */

/*---------------------------------------------------------------------*
*       gtvo24:  Get color opponency from a 24-bit color image         *
*---------------------------------------------------------------------*/

si32 gtvo24(byte *pvg, struct EXTRCOLR *pxc) {

   int iex = (int)pxc->tveoff;
   int iin = (int)pxc->tvioff;
   return ((si32)pvg[iex] - (si32)pvg[iin]) << Sv2mV;

   } /* End gtvo24() */

/*---------------------------------------------------------------------*
*   gvgg16:  Get gray-scale input from 16-bit gray-scale input data    *
*---------------------------------------------------------------------*/

si32 gvgg16(byte *pvg, struct EXTRCOLR *pxc) {

   ui16 v16 = *(ui16 *)pvg >> 2;
   return (si32)v16;

   } /* End gvgg16() */

/*---------------------------------------------------------------------*
*          gtvc48:  Get the pixel from a 48-bit color image            *
*---------------------------------------------------------------------*/

si32 gtvc48(byte *pvg, struct EXTRCOLR *pxc) {

   ui16 *pv16 = (ui16 *)pvg;
   int iex = (int)pxc->tveoff;
   /* Be sure to right shift while still treated as unsigned */
   return (si32)(pv16[iex] >> 2);

   } /* End gtvc48() */

/*---------------------------------------------------------------------*
*       gtvo48:  Get color opponency from a 48-bit color image         *
*---------------------------------------------------------------------*/

si32 gtvo48(byte *pvg, struct EXTRCOLR *pxc) {

   ui16 *pv16 = (ui16 *)pvg;
   int iex = (int)pxc->tveoff;
   int iin = (int)pxc->tvioff;
   /* Be sure to right shift while still treated as unsigned */
   return (si32)(pv16[iex] >> 2) - (si32)(pv16[iin] >> 2);

   } /* End gtvo48() */

/*---------------------------------------------------------------------*
*    giag08:  Get gray-scale input from an 8-bit color input array     *
*  This uses a more accurate averaging algorithm than was possible     *
*  with the old S8 output because the result is now S14.               *
*---------------------------------------------------------------------*/

si32 giag08(byte *pvg, struct EXTRCOLR *pxc) {

   si32 v = (si32)(*pvg);
   return ((v << 7 & 0x6000) + (v << 9 & 0x7000) +
          (v << 12 & 0x7000) + pxc->tvrndc)/(2*NColorDims);

   } /* End giag08() */

/*---------------------------------------------------------------------*
*    giac08:  Get colored input from an 8-bit colored IA or image      *
*---------------------------------------------------------------------*/

si32 giac08(byte *pvg, struct EXTRCOLR *pxc) {

   int iex = (int)pxc->tveoff;
   return ((si32)*pvg & pxc->tvmask[iex]) << pxc->tvshft[iex];

   } /* End giac08() */

/*---------------------------------------------------------------------*
* giao08:  Get color opponent input from an 8-bit colored IA or image  *
*---------------------------------------------------------------------*/

si32 giao08(byte *pvg, struct EXTRCOLR *pxc) {

   int iex = (int)pxc->tveoff;
   int iin = (int)pxc->tvioff;
   si32 vex = ((si32)*pvg & pxc->tvmask[iex]) << pxc->tvshft[iex];
   si32 vin = ((si32)*pvg & pxc->tvmask[iin]) << pxc->tvshft[iin];
   return vex - vin;

   } /* End giao08() */

/*---------------------------------------------------------------------*
*  gppg16:  Get gray from 16-bit gray preproc output (already S7/14)   *
*---------------------------------------------------------------------*/

si32 gppg16(byte *pvg, struct EXTRCOLR *pxc) {

   return (si32)(*(si16 *)pvg);

   } /* End gppg16() */

/*---------------------------------------------------------------------*
* gppc48:  Selected color from 16-bit preproc output (already S7/14)   *
*---------------------------------------------------------------------*/

si32 gppc48(byte *pvg, struct EXTRCOLR *pxc) {

   si16 *pv16 = (si16 *)pvg;
   int iex = (int)pxc->tveoff;
   return (si32)(pv16[iex]);

   } /* End gppc48() */

/*---------------------------------------------------------------------*
*                               gppo48                                 *
*                                                                      *
*  Opponent color Sj from 16-bit preproc output (already S7/14)        *
*---------------------------------------------------------------------*/

si32 gppo48(byte *pvg, struct EXTRCOLR *pxc) {

   si16 *pv16 = (si16 *)pvg;
   int iex = (int)pxc->tveoff;
   int iin = (int)pxc->tvioff;
   return (si32)pv16[iex] - (si32)pv16[iin];

   } /* End gppo48() */

