/* (c) Copyright 2009-2010, The Rockefeller University *11115* */
/* $Id: rkwdev.h 59 2017-01-13 20:34:16Z  $ */
/***********************************************************************
*                        RKWDEV.H Header File                          *
*      Prototype declarations of ROCKS random number generators        *
*                                                                      *
*  This file contains prototypes for a new set of random number gene-  *
*  rators for the ROCKS library.  They have the following properties:  *
*  (1) One or 'n' results can be returned, signed or unsigned integer, *
*      float, or double.  'n' is a long, so can exceed 2**31 in 64-bit *
*      compilations.  Floats and doubles are in range 0 < r < 1.0.     *
*  (2) Uniform or normal distributions can be selected.                *
*  (3) A 64-bit seed is used that comprises two 32-bit positive        *
*      integers.  If the high-order seed is negative, results are      *
*      guaranteed to be the same as with the old 32-bit routines.      *
*  (4) The period is about 2**57.                                      *
*  (5) A combination of two linear-congruential generators is used.    *
*      This permits a call that skips the seed forward 'n' as if a     *
*      generator had been called 'n' times.                            *
*  (6) Algorithms that do not require division are used.               *
*                                                                      *
*  Note:  In wndev() and nwndev(), the mean can have any binary scale  *
*  up to 27.  The sigma scale must be 4 greater than the mean scale.   *
************************************************************************
*  V1A, 11/15/09, GNR - New routines and new header                    *
*  ==>, 11/15/09, GNR - Last date before committing to svn repository  *
*  V1B, 01/08/10, GNR - Add [n]wdevi, [n]wdevs, move impl to wdevcom.h *
***********************************************************************/

#ifndef RKWDEV_H_INCLUDED
#define RKWDEV_H_INCLUDED

#include "sysdef.h"

#ifdef __cplusplus
extern "C" {
#endif
/* Routines that return one uniform random deviate: */
si32   wdev(wseed *seed);                 /* Positive */
si32   wdevs(wseed *seed);                /* Signed */
ui32   wdevi(wseed *seed, ui32 setsz);    /* Selector */
float  wdevf(wseed *seed);                /* Float */
double wdevd(wseed *seed);                /* Double */
/* Routines that return one Gaussian random deviate */
si32   wndev(wseed *seed, si32 mean, si32 sigma);
float  wndevf(wseed *seed, float mean, float sigma);
double wndevd(wseed *seed, double mean, double sigme);

/* The following routines that return multiple random deviates
*  may be useful to reduce call overhead in applications.  */

/* Routines that return 'n' uniform random deviates */
void   nwdev(si32 *ir, wseed *seed, long n);
void   nwdevs(si32 *ir, wseed *seed, long n);
void   nwdevi(ui32 *ir, wseed *seed, long n, ui32 setsz);
void   nwdevf(float *fr, wseed *seed, long n);
void   nwdevd(double *dr, wseed *seed, long n);
/* Routines that return 'n' Gaussian random deviates */
void   nwndev(si32 *ir, wseed *seed, long n, si32 mean, si32 sigma);
void   nwndevf(float *fr, wseed *seed, long n,
         float mean, float sigma);
void   nwndevd(double *dr, wseed *seed, long n,
         double mean, double sigma);
/* Routine to skip a seed ahead by equivalent of 'n' wdev() calls */
void   wdevskip(wseed *seed, long n);
#ifdef __cplusplus
}
#endif

#endif /* not defined(RKWDEV_H_INCLUDED) */
