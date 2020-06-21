/* (c) Copyright 2013-2015, The Rockefeller University */
/* $Id: mdksint.h 9 2016-01-04 19:30:24Z  $ */
/***********************************************************************
*           Multi-Dimensional Kolmogorov-Smirnov Statistic             *
*                              mdksint.h                               *
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
*                               mdksint                                *
*                                                                      *
*  This header file contains definitions used internally by routines   *
*  in the mdks package.  These should be of no concern to programs     *
*  that are using routines in this package.                            *
*                                                                      *
*  Documentation of the functions and their usage is in the README     *
*  file and in the individual C source files.                          *
************************************************************************
*  V1A, 11/17/13, G.N. Reeke - Declarations extracted from mdks.c      *
*  V2A, 12/06/13, GNR - Modify for brick method                        *
*  ==>, 02/18/14, GNR - Last date before committing to svn repository  *
*  V5A, 04/19/14, GNR - Modify for V5 (partial quads) algorithm        *
*  Rev, 05/01/14, GNR - Add 'const' qualifiers to pointers to X1,X2    *
*  V5D, 05/02/14, GNR - Cleaner (high-->low) brick assignment method   *
*  V6A, 05/10/14, GNR - Hierarchical bricks                            *
*  V6B, 05/22/14, GNR - Bricks keep index of next occupied brick       *
*  V6C, 05/27/14, GNR - Store ocube in Lvl, eliminate oprnt            *
*  V7A, 08/13/15, GNR - Add ribbon, equi-partition, unequal-partition, *
*                       and center-of-gravity approximations           *
*  Rev, 08/21/15, GNR - Store quad differences in one set of bricks    *
*  R09, 11/25/15, GNR - Add controls to force 1- or 2-quad sums        *
*  Rev, 12/19/15, GNR - Bug fix:  NBINS was twice correct size         *
*  Rev, 12/29/15, GNR - Bug fix:  Allow to free one of multiple Wkhds  *
*                       This required merging bfksint into mdksint.    *
***********************************************************************/

/*=====================================================================*
*  N.B.  Programs using this header are intended to be included in a   *
*  wrapper program that defines the type specifiers KTX and KTN and    *
*  the version identifier VL.  Alternatively, these variables can be   *
*  defined in a makefile.  If compiled without external definitions,   *
*  mdks routines will default to double-precision data with integer    *
*  'ni' and no name suffix.                                            *
*  (This indirect method is used because #if cannot test strings.)     *
*=====================================================================*/

#ifndef MDKSINT_HDR_INCLUDED
#define MDKSINT_HDR_INCLUDED

/* Define 'byte' */
#ifndef __SYSDEF__
typedef unsigned char  byte;
#endif
/* Define the type used for the X1 and X2 data ('float' or 'double') */
#ifndef KTX
#define KTX 1
#endif
#if KTX == 0
typedef float Xdat;
#elif KTX == 1
typedef double Xdat;
#else
#undef KTX
#error KTX must be 1 (for double) or 0 (float) or add code for new type
#endif
/* Define the type used for 'n' (usually 'int' but could be a
*  longer type) */
#ifndef KTN
#define KTN 0
#endif
#if KTN == 0
typedef int Nint;
#else
typedef long Nint;
#endif
#ifdef I_AM_MATLAB
#define  mxInt  (2.0*(double)(1 << (NBPB*sizeof( int)-2)) - 1.0)
#define  mxNint (2.0*(double)(1 << (NBPB*sizeof(Nint)-2)) - 1.0)
#endif
/* Define a macro to append a version letter to a function name to
*  give a unique name to the version compiled for the above types.
*  (This crazy nested macro is needed because the '##' operator
*  does not perform substitution on its arguments.)  */
#ifdef  VL
#define vfncat(fn,vlet)  fn ## vlet
#define vfnname(fn,vlet) vfncat(fn,vlet)
#define vfnmcat(fn,vlet) fn #vlet
#define vfnmex(fn,vlet)  vfnmcat(fn,vlet)
#else
#define vfnname(fn,vlet) fn
#define vfnmex(fn,vlet) fn
#endif

#include <mdks.h>          /* Just in case */

/* Code assumes the following defined values--change only with care */
#define HLBL  0x4D444B53   /* Header label: Ascii "MDKS" */
#define NBPB    8          /* Number of bits in a byte */
#define NBINS (1<<NBPB)    /* Number of bins for radix sort */
#define NDIMS   2          /* Dimensions of MATLAB X arguments */
#define NDSET   2          /* Number of data sets being compared */
#define NENDS   2          /* Number of ends of a line */
#define NRD     2          /* Dimensionality of ribbon method */
#define NRQ     4          /* Number of ribbon quadrants (2D) */
#define IXb     0          /* Index for lists spanning X1 and X2 */
#define IX1     0          /* Index of data for data set X1 */
#define IX2     1          /* Index of data for data set X2 */
#define TINY    1E-12      /* Sanity check for small differences */
/* The following gives just a quick sanity check for max(k)--
*  mdksallox() does a final check based on available memory.  */
#define MXK (NBPB*sizeof(size_t) - 4 - (KTN > 0))

/* Define value of k for which methods that have both one- and
*  two-quad sums versions switch over to two-quads.  For methods
*  that have only one version, this is a place-holder.  */
#define K2Q_MD    3
#define K2Q_RM    3
#define K2Q_RO    3
#define K2Q_EP    3
#define K2Q_CG    3
#define K2Q_UP    3
#define K2Q_BF    3

/* Define precision used for quad totals.  These need to be floating
*  point now that n2/n1 ratio is applied at every X2 point in KC_QS1
*  case, and double to prevent integer roundoff for large data sets. */
typedef double Prec;

/* Structure used to make linked lists of data points */
struct Link_t {
   struct Link_t *pnxt;    /* Ptr to next Link in list */
   Xdat const *pdat;       /* Ptr to data for this point */
   size_t obrk;            /* Offset to brick point belongs to */
   };
typedef struct Link_t Link;

/* Structure for a brick */
struct Brik_t {
   Prec  npts;             /* If KC_QS2: Points in brick, if KC_QS1:
                           *  Points in X1 - points in X2*(n1/n2) */
   Nint  inob;             /* Index of next occupied brick */
   };
typedef struct Brik_t Brick;

/* Structure used to perform radix sort on coordinates.
*  N.B.  As usual, the fiction is maintained that the head pointer
*  in a Bin is a complete Link.  This avoids extra tests inside the
*  sort loop but depends on pnxt being the first item in the Link
*  struct.  If this ever changes, the sort code must also change.  */
struct Bin_t {
   Link *head;             /* Ptr to head of a sort bin */
   Link *tail;             /* Ptr to last entry in each bin */
   };
typedef struct Bin_t Bin;

/* Structure to hold data needed for each cube depth level.
*  Note:  Levels are numbered from 1 (2^k hypercube) to d
*  (bottom level 2^kd hypercube where bricks have members).  */
struct Level_t {
   Brick *pbrk[NDSET];     /* Ptrs to brick arrays for this level */
#define pbrk1 pbrk[0]         /* Access only bricks for KC_QS1 */
   size_t jcube;           /* Index of cube currently in process */
   size_t ocube;           /* Offset of this cube in pbrk1 array */
   size_t relqn;           /* Bits in relqo that are not settled */
   size_t relqo;           /* Offset of relative quadrant */
   };
typedef struct Level_t Lvl;

/* Structure to hold constants needed for each dimension */
struct Dim_t {
   size_t mask;            /* Mask to clean coord sum */
   size_t oadd;            /* Number to increment a coordinate */
   int ikt;                /* Index of dims that need coord tests */
   };
typedef struct Dim_t Dim;

/* Structure which holds data for one ribbon */
struct Ribbon_t {
   Xdat qmin;              /* Minimum (left) coordinate */
   Xdat rdiff;             /* Reciprocal ribbon width */
   };
typedef struct Ribbon_t Ribn;

/* Structure used to make linked lists of work areas */
struct Wkhd_l {
   struct Wkhd_l *pnwhd;   /* Ptr to next Wkhd_l */
   struct Wkhd_l *ppwhd;   /* Ptr to previous Wkhd_l */
   };

/* Work area header */
struct Wkhd_t {
   struct Wkhd_l wlnk;     /* Links for managing freeing */
   Link  *pLnk0[NDSET];    /* Ptrs to starts of data list allocs */
   Link  *pLnk1[NDSET];    /* Ptrs to heads of sorted data lists */
   Link **ppBm0[NDSET];    /* Ptrs to brick member lists */
   Xdat  *pCog0[NDSET];    /* Ptrs to ctr of gravity data (cg method) */
   Prec  *pQtot[NDSET];    /* Ptrs to quad totals */
#define pQtot1 pQtot[0]       /* Access only Qtot for KC_QS1 */
   Prec  *pQfr0;           /* Ptr to start of coord frac array */
   Lvl   *pLvl0;           /* Ptr to start of Lvl array */
   Dim   *pDim0;           /* Ptr to start of Dim array */
   Bin   *pBin0;           /* Ptr to start of Bin array */
   Ribn  *pRib0;           /* Ptr to start of Ribn array */
   double sqrtn[NDSET];    /* sqrt(n1,n2) */
   size_t lbrks;           /* Length of bricks (all levels) */
   size_t lbml;            /* Length of brick membership lists */
   size_t lcog;            /* Length of center-of-gravity array */
   size_t lallo;           /* Size allocated to this Wkhd */
   Nint n[NDSET];          /* Number of points in X1, X2 */
   int  d;                 /* Depth of the brick hierarchy */
   int  kcalc;             /* Calc methods set up for this work area */
#define KC_2S 0x8000          /* Flag for two-sided setup */
#define KC_2Q 0x4000          /* Flag for two-quad sum calc */
#if KC_2S <= KC_ERLIM || KC_2Q <= KC_ERLIM
#error KC_2S or KC_2Q is <= KC_ERLIM, make larger
#endif
   int  k;                 /* Dimension of the problem */
   int  repct;             /* Debug info */
   int  hdrlbl;            /* Label to validate work area */
   int  qle;               /* 1 Little-endian, 0 Big-endian */
   };
typedef struct Wkhd_t Wkhd;

#endif /* MDKSINT_HDR_INCLUDED */
