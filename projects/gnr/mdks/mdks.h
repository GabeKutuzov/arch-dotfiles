/* (c) Copyright 2013-2015, The Rockefeller University */
/* $Id: mdks.h 9 2016-01-04 19:30:24Z  $ */
/***********************************************************************
*                         MDKS.H Header File                           *
*           Multi-Dimensional Kolmogorov-Smirnov Statistic             *
*                                                                      *
*  This file contains declarations of the routines available in the    *
*  multi-dimensional Kolmogorov-Smirnov package.  These functions      *
*  compute the multi-dimensional Kolmogorov-Smirnov statistic, which   *
*  may be used to quantitate the degree to which two distributions of  *
*  points in k-dimensional space are statistically distinguishable.    *
*  The present versions can handle up to the number of data points     *
*  that will allow the work area to fit in memory, with k < 28.        *
*                                                                      *
*  The routines with names beginning "bfks" are the "brute-force"      *
*  version of the MDKS package, intended only for test and timing      *
*  comparison purposes.                                                *
*                                                                      *
*  The routines with names beginning "rmks" are the "ribbon-method"    *
*  version of the MDKS package, which compute an approximation for     *
*  the two-dimensional case that allocates data points in ribbons      *
*  surrounding each test point vertically and horizontally according   *
*  to the position of the test point in the ribbon, thereby avoiding   *
*  individually checking each data point in each ribbon to determine   *
*  which quadrant centered on the test point it belongs to.            *
*                                                                      *
*  The routines with names beginning "epks" are the "equi-partitioned" *
*  method version of the MDKS package.  These routines compute an      *
*  approximation in which the number of points in any brick that over- *
*  laps the current test point are distributed equally among the quad- *
*  rants with overlapping coordinates.  This approximation is suitable *
*  for problems of any dimensionality.                                 *
*                                                                      *
*  The routines with names beginning "upks" are the "unequally-parti-  *
*  tioned method version of the MDKS package.  In this version, the    *
*  number of points in any brick that overlaps the current test point  *
*  are distributed among the quadrants with overlapping coordinates    *
*  in proportion to the fractional position of the test point in its   *
*  brick.                                                              *
*                                                                      *
*  The routines with nemaes beginning "cgks" are the "center-of-       *
*  gravity" method version of the MDKS package.  These routines        *
*  compute an alternative approximation to the "epks" version, also    *
*  usable in any dimensionality.  This method may be more or less      *
*  accurate than "epks" depending on the data.                         *
*                                                                      *
*  The usage instructions for these programs are in the README file    *
*  and in the individual C source files.  The algorithms are described *
*  briefly in the README and in more detail in the C source files.     *
*                                                                      *
*  The ribbon-method software was written by Aparna Nair-Kanneganti    *
*  and the other versions by George N. Reeke in the Laboratory of      *
*  Biological Modelling at The Rockefeller University.  Please send    *
*  any corrections, suggestions, or improvements to the GNR by email   *
*  to reeke@mail.rockefeller.edu for possible incorporation in future  *
*  revisions.                                                          *
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
************************************************************************
*  V1A, 11/16/13, G.N. Reeke - New header file                         *
*  ==>, 02/08/14, GNR - Last date before committing to svn repository  *
*  Rev, 05/01/14, GNR - Add 'const' qualifiers to pointers to X1,X2    *
*  V6A, 05/10/14, GNR - Hierarchical bricks                            *
*  V7A, 08/13/15, GNR - Add ribbon, equi-partition, unequal-partition, *
*                       and center-of-gravity approximations           *
*  R09, 11/25/15, GNR - Add controls to force 1- or 2-quad sums        *
*  Rev, 12/30/15, GNR - Eliminate bfksallox() and bfksfree()           *
***********************************************************************/

#ifndef MDKS_HDR_INCLUDED
#define MDKS_HDR_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/* Routines to allocate and modify work areas */
int mdksallod(void **ppwk, int *g1, int *g2, int n1, int n2, int k);
int mdksallof(void **ppwk, int *g1, int *g2, int n1, int n2, int k);

/* Define method-allocation ('g2') parameter values for mdksallox() */
#define KC_DFLT   0x0000   /* Default: F&F method only */
#define KC_MDKS   0x0001   /* Exact Fasano & Franceschini method */
#define KC_RMKS   0x0002   /* Ribbon method (2D problems only) */
#define KC_ROKS   0x0004   /* Ribbon with approx centers (2D only) */
#define KC_EPKS   0x0008   /* Equi-partition method */
#define KC_CGKS   0x0010   /* Center-of-gravity method */
#define KC_UPKS   0x0020   /* Unequal-partition method */
#define KC_BFKS   0x0040   /* Brute-force method */
#define KC_QS1    0x0100   /* Use single quad sums if avail */
#define KC_QS2    0x0200   /* Use double quad sums if avail */
#define KC_QS2A   0x0400   /* Allocate but not force double sums */
#define KC_ERLIM  0x04ff   /* Max g2 for error checking */

/* Routines to release work area storage */
int  mdksfree(void *pwk);
void mdksexit(void);

/* Define errors returned by mdksallox(), mdksqset(), mdksfreex() */
#define MDKSA_ERR_BADWK    1
#define MDKSA_ERR_BADQS    2
#define MDKSA_ERR_KLOW     2
#define MDKSA_ERR_KBIG     3
#define MDKSA_ERR_N1LOW    4
#define MDKSA_ERR_N2LOW    5
#define MDKSA_ERR_G1HIGH   6
#define MDKSA_ERR_BADG2    7
#define MDKSA_ERR_RIBK2    8
#define MDKSA_ERR_ADSPCE   9
#define MDKSA_ERR_NOMEM   10

/* Routines to compute the one-sided MDKS statistic */
double mdks1sd(void *pwk, double const *X1, double const *X2);
double mdks1sf(void *pwk, float const *X1, float const *X2);

/* Routines to compute the 2-sample (symmetric) MDKS statistic */
double mdks2sd(void *pwk, double const *X1, double const *X2);
double mdks2sf(void *pwk, float const *X1, float const *X2);

/* Routines to compute the MDKS statistic by brute-force methods
*  (basically for debugging and timing studies)  */
double bfks1sd(void *pwk, double const *X1, double const *X2);
double bfks1sf(void *pwk, float const *X1, float const *X2);
double bfks2sd(void *pwk, double const *X1, double const *X2);
double bfks2sf(void *pwk, float const *X1, float const *X2);

/* Routines to compute the MDKS statistic by the "ribbon method"
*  approximation (two-dimensional problems only).  */
double rmks1sd(void *pwk, double const *X1, double const *X2);
double rmks1sf(void *pwk, float const *X1, float const *X2);
double rmks2sd(void *pwk, double const *X1, double const *X2);
double rmks2sf(void *pwk, float const *X1, float const *X2);

/* Routines to compute the MDKS statistic by the "equi-partition"
*  approximation method  */
double epks1sd(void *pwk, double const *X1, double const *X2);
double epks1sf(void *pwk, float const *X1, float const *X2);
double epks2sd(void *pwk, double const *X1, double const *X2);
double epks2sf(void *pwk, float const *X1, float const *X2);

/* Routines to compute the MDKS statistic by the "unequal-
*  partition" approximation method  */
double upks1sd(void *pwk, double const *X1, double const *X2);
double upks1sf(void *pwk, float const *X1, float const *X2);
double upks2sd(void *pwk, double const *X1, double const *X2);
double upks2sf(void *pwk, float const *X1, float const *X2);

/* Routines to compute the MDKS statistic by the "center-of-gravity"
*  approximation method  */
double cgks1sd(void *pwk, double const *X1, double const *X2);
double cgks1sf(void *pwk, float const *X1, float const *X2);
double cgks2sd(void *pwk, double const *X1, double const *X2);
double cgks2sf(void *pwk, float const *X1, float const *X2);

/* Define errors returned by mdks1sx() and mdks2sx(), etc. */
#define MDKS_ERR_BADWK   -1.0
#define MDKS_ERR_FLATX   -2.0
#define MDKS_ERR_No2QA   -3.0
#define MDKS_ERR_RMKN2   -4.0

/* Routine to return pointers to the axial divisions */
int mdksdivs(void *pwk, int *pdivs[2]);

/* Routine to modify summation method */
int mdksqset(void *pwk, int qset);

/* Routine to return compilation version of mdks library */
char *mdksvers(void);

#ifdef __cplusplus
}
#endif

#endif /* MDKS_HDR_INCLUDED */

