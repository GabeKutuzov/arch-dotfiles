/* (c) Copyright 1993-2014, The Rockefeller University *21114* */
/* $Id: polyln.c 29 2017-01-03 22:07:40Z  $ */
/***********************************************************************
*                              polyln()                                *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void polyln(int kf, int np, const float *x, const float *y)      *
*                                                                      *
*  DESCRIPTION:                                                        *
*     The routine draws open or closed ployline (a series of points    *
*     connected by lines) in the current color.                        *
*                                                                      *
*  Arguments:                                                          *
*     'kf' is a fill control switch with values:  0 (THIN) an open,    *
*          hairline-thin polyline is drawn; 1 (THICK) an open          *
*          polyline in the current retrace thickness is drawn;         *
*          2 (CLOSED_THIN) a closed and thin polyline is drawn;        *
*          3 (CLOSED_THICK) a closed and thick polyline is drawn;      *
*          -1 (FILLED) a closed polygon filled with color is drawn.    *
*          In cases CLOSED_THIN, CLOSED_THICK, and FILLED, the         *
*          first and last points are connected by a line to form a     *
*          closed figure.  In cases THIN and THICK, the first and      *
*          last points are not joined (unless np = 2).                 *
*     'np' is the number of points in the polyline.                    *
*     'x'  is an array of dimension 'np' containing the x              *
*          coordinates of the points to be plotted.                    *
*     'y'  is an array of dimension 'np' containing the y              *
*          coordinates of the points to be plotted.                    *
*                                                                      *
*  RETURN VALUES:                                                      *
*     None                                                             *
*                                                                      *
*  N.B.  Currently the distinction between thick and thin lines is     *
*  not supported in the metafile format, so both kinds of calls give   *
*  the same result.  This may be a documentation issue (call retrace() *
*  to specify the desired line width.)                                 *
*  N.B.  This program can handle a polyline that does not fit in one   *
*  buffer only in the serial version.  The fix for the parallel case   *
*  is the MFB_ATOM MFCountWord code used in the Version II library.    *
************************************************************************
*  Version 2, 10/28/18, GMK                                            *
***********************************************************************/

#include <string.h>
#include "mfint.h"
#include "plots.h"

void polyln(int kf, int np, const float *x, const float *y) {

   int i;

   if (np > 0 && _RKG.s.MFActive) {

      mfbchk(_RKG.s.mxlPoly);

      if (kf == THIN || kf == THICK) {
         _RKG.xcurr = x[np-1];
         _RKG.ycurr = y[np-1]; }
      else {
         _RKG.xcurr = x[0];
         _RKG.ycurr = y[0]; }

      *_RKG.MFCurrPos++ = 'P';

      switch (kf) {
         case THIN:
         case THICK:
            *_RKG.MFCurrPos++ = 'O';
            break;
         case FILLED:
            *_RKG.MFCurrPos++ = 'F';
            break;
         case CLOSED_THIN:
         case CLOSED_THICK:
            *_RKG.MFCurrPos++ = 'L';
            break;
         default:
            abexitm(288,"Unknown polyline type");
         }
      i2a(_RKG.MFCurrPos++,np,3,DECENCD);
      for (i=0;i<np;i++) {
#ifndef PAR
         mfbchk(10);
#endif
         i2a(_RKG.MFCurrPos,(int)(x[i]*1000.0),5,DECENCD);
         i2a(_RKG.MFCurrPos,(int)(y[i]*1000.0),5,DECENCD);
         }
      *_RKG.MFCurrPos++ = '\n';
      }

   } /* End polyln() */
