/* (c) Copyright 2015, The Rockefeller University */
/* $Id: mdksqset.c 6 2014-06-09 19:19:58Z  $ */
/***********************************************************************
*           Multi-Dimensional Kolmogorov Smirnov Statistic             *
*                             mdksqset.c                               *
*                                                                      *
*  This software was written by George N. Reeke in the Laboratory of   *
*  Biological Modelling at The Rockefeller University.  Please send    *
*  any corrections, suggestions, or improvements to the author by      *
*  email to reeke@mail.rockefeller.edu for possible incorporation in   *
*  future revisions.                                                   *
*                                                                      *
*  This software is distributed under GPL, version 2.  This program is *
*  free software; you can redistribute it and/or modify it under the   *
*  terms of the GNU General Public License as published by the Free    *
*  Software Foundation; either version 2 of the License, or (at your   *
*  option) any later version. Accordingly, this program is distributed *
*  in the hope that it will be useful, but WITHOUT ANY WARRANTY; with- *
*  out even the implied warranty of MERCHANTABILITY or FITNESS FOR A   *
*  PARTICULAR PURPOSE.  See the GNU General Public License for more    *
*  details.  You should have received a copy of the GNU General Public *
*  License along with this program.  If not, see                       *
*  <http://www.gnu.org/licenses/>.                                     *
*----------------------------------------------------------------------*
*                              mdksqset                                *
*                                                                      *
*  This routine may be called by an application that is using the MDKS *
*  library to control whether the one-quad-array or two-quad-arrays    *
*  method of summation is used in following calls by those routines    *
*  that provide both methods (currently mdksxxx only).                 *
*                                                                      *
*  mdksqset() may be called at any time after the call to mdksallox()  *
*  and before the work area is freed by a call to mdksfree(), (where,  *
*  as described in the readme, 'x' is 'd' for double precision and 'f' *
*  for float data).                                                    *
*                                                                      *
*  MATLAB Synopsis:                                                    *
*     mdksqset(MDKSCOM, qset)                                          *
*                                                                      *
*  C or C++ Synopsis:                                                  *
*     #include "mdks.h"                                                *
*     int mdksqset(void *pwk, int qset)                                *
*                                                                      *
*  Arguments:                                                          *
*     'MDKSCOM' is the work area returned by a previous MATLAB call to *
*        mdksallox() and 'pwk' is the pointer stored at *ppwk by the   *
*        most recent C call to mdksallox().                            *
*     'qset' must be one of the following values:                      *
*        0  to cause each calculation routine to use its default       *
*           summation method.                                          *
*        1  to cause each calculation routine that provides the        *
*           one-quad-array method to use that method.                  *
*        2  to cause each calculation routine that provides the        *
*           two-quad-array method to use that method.                  *
*                                                                      *
*  Error conditions and C return values:                               *
*     The value of the C function return is 0 for success, 1 (defined  *
*  in the header file mdks.h as MDKSA_ERR_BADWK) when the argument     *
*  'pwk' does not point to a valid mdks work space block, and 2        *
*  (defined as MDKSA_ERR_BADQS) when qset is not one of the three      *
*  values defined above.  The MATLAB version gives a terminal error    *
*  for each of these error conditions.                                 *
*                                                                      *
************************************************************************
*  V1A, 11/28/15, GNR - New program                                    *
*  ==>, 11/28/15, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mdks.h"
#include "mdksint.h"

#ifndef SVNVRS
#define SVNVRS Unknown
#endif
#define vcat(txt,vers) txt #vers

#ifdef I_AM_MATLAB
#include "mex.h"

void InnerQset(int nlhs, mxArray *plhs[],
   int nrhs, const mxArray *prhs[]) {

   Wkhd *phd;
   double dqs;
   int qset;
   int rc;

/* Standard MATLAB check for proper numbers of arguments */

   if (nlhs != 0)
      mexErrMsgTxt("mdksqset takes no left-hand args.");
   if (nrhs != 2)
      mexErrMsgTxt("mdksqset requires 2 right-hand args.");

/* Check that the right-hand argument is a pointer to Wkhd */

   if (!mxIsUint8(prhs[0]) ||
         mxGetNumberOfElements(prhs[0]) != sizeof(void *))
      mexErrMsgTxt("mdksqset MDKSCOM arg is not expected "
         "type or size.");
   phd = *(Wkhd **)mxGetData(prhs[0]);
   if (phd->hdrlbl != HLBL)
      mexErrMsgTxt("mdksqset MDKSCOM arg appears not to be "
         "a work area returned by mdksallox().");

/* Pick up the qset argument and set the control bits.  (The
*  action is so simple, don't bother calling the C function.) */

   if (!mxIsDouble(prhs[1]) ||
         mxGetNumberOfElements(prhs[1]) != 1)
      mexErrMsgTxt("mdksqset 2nd arg must be double scalar.");
   dqs = mxGetScalar(prhs[1]);
   if (dqs != 0.0 && dqs != 1.0 && dqs != 2.0)
      mexErrMsgTxt("mdksqset qset arg must be 0, 1, or 2.");
   qset = (int)dqs;

   phd->kcalc &= ~(KC_QS1|KC_QS2);
   if (qset == 1) phd->kcalc |= KC_QS1;
   else if (qset == 2) phd->kcalc |= KC_QS2;

   } /* End mexFunction() */

/* Wrapper for the wrapper (so gdb can be used) */

void mexFunction(int nlhs, mxArray *plhs[],
   int nrhs, const mxArray *prhs[]) {

   InnerQset(nlhs, plhs, nrhs, prhs);

   }

#else /* not I_AM_MATLAB */

int mdksqset(void *pwk, int qset) {

   Wkhd *phd = (Wkhd *)pwk;

   if (phd->hdrlbl != HLBL) return MDKSA_ERR_BADWK;
   if (qset < 0 || qset > 2) return MDKSA_ERR_BADQS;

   phd->kcalc &= ~(KC_QS1|KC_QS2);
   if (qset == 1) phd->kcalc |= KC_QS1;
   else if (qset == 2) phd->kcalc |= KC_QS2;

   return 0;

   } /* End mdksqset() */

#endif
