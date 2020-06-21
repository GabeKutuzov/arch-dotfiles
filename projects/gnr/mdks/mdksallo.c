/* (c) Copyright 2013-2016, The Rockefeller University */
/* $Id: mdksallo.c 9 2016-01-04 19:30:24Z  $ */
/***********************************************************************
*           Multi-Dimensional Kolmogorov-Smirnov Statistic             *
*                             mdksallo.c                               *
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
*                              mdksallo                                *
*                                                                      *
*  This function allocates and initializes, or modifies for use with   *
*  data sets of different dimensionality, an mdks work area that is    *
*  used by the other routines in the mdks package.  This function is   *
*  performed by a separate routine so that the same work area can be   *
*  reused for multiple MDKS calculations with the same parameters and  *
*  multiple work areas for different parameter sets can coexist and be *
*  modified and/or freed individually as needed.                       *
*                                                                      *
*  This is generic code that can be compiled with different external   *
*  definitions to generate routines to handle different data types.    *
*  This provides a mechanism in C similar to C++ templates.  Each      *
*  routine is named with a terminal letter to indicate the types it    *
*  handles.  This letter is represented by an 'x' in the comments.     *
*  The routines with the terminal letters are basically wrappers that  *
*  define the type selectors 'KTX' and 'KTN' and the version letter    *
*  'VL' and then include the actual code from this file.  These        *
*  wrappers can also be compiled with 'I_AM_MATLAB' defined to create  *
*  a MATLAB mex file with the same name that performs the same action. *
*                                                                      *
*  Usage to allocate or reallocate a work area:                        *
*     mdksallod() for double-precision data or mdksallof() for single- *
*  precision data must be called before calls to any of the other rou- *
*  tines in the MDKS package (including brute-force routines) or any   *
*  time one of the parameters changes. The value returned is a pointer *
*  to a work area that is an argument to the other functions.  Once    *
*  allocated, the same work area can be used for multiple calculations *
*  as long as all the parameters remain unchanged.                     *
*                                                                      *
*     For all methods other than brute force, a hierarchy of "brick"   *
*  structures is used to cluster the data points in compact volumes.   *
*  The default depth of bisection in the brick hierarchy has been      *
*  established by a polynomial fit to the optimum depth found in tests *
*  with k=2,...,5 and n={200,1000,5000,25000,125000}.  This polynomial *
*  may not be optimal for other data set sizes and may be modified in  *
*  future versions of this program.  Letting mxn = max(n1,n2), lmxn =  *
*  log2(maxn), and lnok = lmxn/k, the default depth is:                *
*  (int)round(1.9287E-6*maxn*lnok + 1.0305*lnok + 0.0101*lmxn +        *
*  0.3616*k - 2.6180)                                                  *
*                                                                      *
*     The size of the work area allocated by this routine depends on   *
*  the problem parameters and the pointer (PSIZE), data (XSIZE), quad  *
*  sums (double, QSIZE), and data count (int or long, NSIZE) sizes.    *
*  Let nq=number of sums per quad (see 'g2'). Let n=n1+n2, d=hypercube *
*  hierarchy depth, which is either an input value or the default      *
*  described above, b=number of bricks=Sum(j=1..d)(2^(j*k)) and bb =   *
*  number of bottom-level bricks=2^(d*k).  Then when k > 1, the work   *
*  area size is max(512*PSIZE, nq*2^k*QSIZE) + (20+3n+2k+6d+2bb)*PSIZE *
*  + 4b*NSIZE (approximately) plus a small fixed amount of overhead.   *
*  To this amount add (2^d)*(8+4*XSIZE) for the ribbon method and      *
*  bb*k*n*XSIZE for the center-of-gravity method.  When k == 1, the    *
*  work area size is (532+3n)*PSIZE plus overhead.                     *
*  For brute force, space is needed only for the "quad" sums, namely   *
*  nq*(2^k)*QSIZE bytes, plus the fixed overhead.                      *
*                                                                      *
*  MATLAB Synopsis:                                                    *
*     MDKSCOM = mdksallox(MDKSCOM, g1, g2, n1, n2, k)                  *
*        where g1, g2, n1, n2, and k are MATLAB double-precision       *
*        scalars (1 x 1 matrices).  'x' is 'f' for floating-point      *
*        data and 'd' for double-precision data.                       *
*  C or C++ Synopsis:                                                  *
*     #include "mdks.h"                                                *
*     int mdksallox(void **ppwk, int *g1, int *g2, Nint n1,            *
*        Nint n2, int k)                                               *
*                                                                      *
*  Arguments:                                                          *
*     k        Number of dimensions of the data points, 1 <= k < MXK,  *
*              where MXK is at best (log2(available memory) - 4)/d     *
*              and more likely constrained by available compute time.  *
*     n1       Number of points in distribution X1, k < n1 < MXN.      *
*     n2       Number of points in distribution X2, k < n2 < MXN.      *
*              (Nint in the C call represents the type of n1 and n2    *
*              for which the particular version of mdksallox was com-  *
*              piled.  The type Nint is determined by the compile-time *
*              variable 'KTN', which is 0 for 'int' and 1 for 'long'.  *
*              The actual data are passed as arguments to mdks1sx() or *
*              mdks2sx().  MXN depends on the memory available in the  *
*              system.                                                 *
*     g1,g2    In C calls, these arguments are pointers to int for     *
*              compatibility with an earlier version of mdksallox().   *
*     g1       Depth of hypercube data tree.  g1 overrides the default *
*              depth of the calculation with a user-specified value,   *
*              0 <= g1 < MXK.  g1 may be 0 or a NULL pointer (empty    *
*              MATLAB array) to use the default depth.                 *
*     g2       A sum of codes that control which types of MDKS calcu-  *
*              lations can be performed using the work area created by *
*              this call to mdksallox().  For the C call, the named    *
*              constants defined in the header file mdks.h can be used.*
*              Acceptable values of g2 are:                            *
*                 KC_MDKS 0x0001 Set up for the exact binary-tree      *
*                                implementation of Fasano and          *
*                                Franceschini.                         *
*                 KC_RMKS 0x0002 Set up to allow ribbon method calls.  *
*                 KC_ROKS 0x0004 Set up to allow ribbon method with    *
*                                double-overlap brick approximation.   *
*                 KC_EPKS 0x0008 Set up for equi-partition calls.      *
*                 KC_CGKS 0x0010 Set up for center-of-gravity calls.   *
*                 KC_UPKS 0x0020 Set up for unequal-partition calls.   *
*                 KC_BFKS 0x0040 Set up for brute-force method calls.  *
*                 KC_QS1  0x0100 Use single quad sums where available. *
*                                (Quad sums are differences of brick   *
*                                counts for the X1 and X2 data sets.)  *
*                 KC_QS2  0x0200 Use double quad sums where available. *
*                                (Brick counts are summed separately   *
*                                for X1 and X2, subtracted later.  If  *
*                                n1 != n2, this option may result in   *
*                                less roundoff error in quad sums.)    *
*                 KC_QS2A 0x0400 Allocate for KC_QS2 double brick sums *
*                                but do not force--will call mdksnqs() *
*                                later to control summation mode.      *
*              If g2 is 0 or a NULL pointer (empty MATLAB array), the  *
*              default is KC_MDKS only.  g2 is ignored in versions of  *
*              the MDKS package prior to V7A.                          *
*     ppwk     Pointer to a location where mdksallox() will return a   *
*              pointer to the work area described above.  The pointer  *
*              at *ppwk must be initialized to NULL to create a new    *
*              work area and a non-NULL value to indicate that this    *
*              call is intended to update an existing work area at     *
*              that location for data with different size parameters.  *
*              The pointer at *ppwk must be passed as the first argu-  *
*              ment in each following call to mdks1sx(), mdks2sx(),    *
*              the corresponding calls for any of the approximation    *
*              methods or mdksfree().                                  *
*     MDKSCOM  MATLAB equivalent of pwk.  This should be an empty      *
*              array to make a new work area, otherwise a value        *
*              returned by a previous call if the intent is to modify  *
*              an existing work area.                                  *
*                                                                      *
*  Return values:                                                      *
*     mdksallox() returns (at *ppwk) a pointer to a work area that     *
*  will be used by mdks1sx(), mdks2sx(), etc. during mdks calculations.*
*  The contents of this work area are not of interest to the calling   *
*  program.  Care should be taken not to use stale pointers if multi-  *
*  ple calls are made to mdksallox() with different 'ppwk', as the     *
*  work area may be moved to a new location if it needs to be expanded.*
*  In this case, the pointer at *ppwk will be modified.  The function  *
*  return from mdksallox() is an integer which is zero in case of      *
*  success.  Nonzero values (C version) indicate errors as follows:    *
*     (1) On an attempt to modify the work area, the value at *ppwk    *
*         did not appear to point to a previously allocated work area. *
*     (2) 'k' is negative or zero.                                     *
*     (3) Quadrant sums will not fit in memory.                        *
*     (4) 'n1' is not larger than 'k'.                                 *
*     (5) 'n2' is not larger than 'k'.                                 *
*     (6) A cube depth specified in 'g1' is larger than 8*PSIZE/k.     *
*     (7) 'g2' specifies an undefined method code.                     *
*     (8) The ribbon method was requested for dimension larger than 2. *
*     (9) Size of work space exceeds system address space.  If running *
*         in a 32-bit system, problem might fit in a 64-bit system.    *
*    (10) The required amount of storage was not available.  In        *
*         this case the pointer at *ppwk is set to NULL.               *
*  The MATLAB version makes a mexErrMsgTxt() call and terminates on    *
*  any of the above errors.                                            *
*                                                                      *
*  Note:  On calls to mdksallox() to modify an existing work area      *
*  for use with different values of the parameters, if the new work    *
*  area fits in the space already allocated, the area is reused,       *
*  otherwise, it is expanded via a realloc() call.  This minimizes     *
*  the number of system calls needed when working with data sets of    *
*  different sizes.  (If under caller control, reallocation can be     *
*  minimized by processing the larger data sets first.)                *
*                                                                      *
*  Usage to release storage occupied by a work area:                   *
*     mdksfree() releases storage previously allocated by mdksallox()  *
*  back to the operating system.  (This program is the same for all    *
*  argument types and thus has no version identifier letter 'x'.)      *
*  MATLAB uses a different call to mdksallox() to release a work area. *
*                                                                      *
*  MATLAB Synopsis to release a work area:                             *
*     mdksallox(MDKSCOM, 0)                                            *
*  C or C++ Synopsis:                                                  *
*     int mdksfree(void *pwk)                                          *
*     void mdksexit(void)                                              *
*                                                                      *
*  Arguments:                                                          *
*     pwk      This must be the pointer stored at *ppwk by a previous  *
*              call to mdksallox().                                    *
*     MDKSCOM  MATLAB equivalent of pwk.                               *
*                                                                      *
*  Return values:                                                      *
*     mdksfree() releases one specified work area.  mdksexit() frees   *
*  all work areas that have not already been freed (and may be used    *
*  as an 'atexit' routine).  mdksfree() returns 0 on success and 1 if  *
*  the argument 'pwk' apparently did not point to an area allocated    *
*  by an earlier call to mdksallox().  The MATLAB mex file gives a     *
*  terminal error if the operation was not successful.                 *
*                                                                      *
*  Design Notes:                                                       *
*     A key requirement of this project was that a single work area    *
*  could be modified as needed to work with different data parameters, *
*  yet it should also be possible to create multiple work areas for    *
*  different parameters so the mdks calculations could be performed    *
*  in parallel in the calling application rather than sequentially.    *
*  A problem here is that no parameters can be passed to the MATLAB    *
*  mexAtExit routine nor is it obvious how to indicate to a later      *
*  function which of several persistent C objects are to be accessed.  *
*  See http://www.mathworks.com/matlabcentral/newsreader/view_thread/  *
*  278243 for a detailed discussion of this topic.  Here we construct  *
*  a new work area each time mdksallox() is called with the first      *
*  argument a pointer to a NULL pointer or empty MATLAB object.  Work  *
*  areas point to each other in a linked list so the exit routine can  *
*  free all of them given a single pointer in C static storage to the  *
*  head of the list.  Further, the list is doubly linked so individual *
*  work areas can be deleted from the list by mdksfree() without       *
*  access to the head pointer.                                         *
*     In MATLAB compilations, the linked list is a list of mxArrays so *
*  the mxArray container for each work area can be freed along with    *
*  the work area.  In C code, the list is just a list of Wkhd areas.   *
*     mdksallox() allocates the largest amount of storage needed for   *
*  any of the options requested by 'g2' so that mdks calculations can  *
*  then be requested for any or all of those options, typically to     *
*  compare speeds for different algorithms.  However, dimension and    *
*  depth and number of data points must be the same for all such calls.*
*                                                                      *
*  Reference:                                                          *
*     G. Fasano & A. Franceschini (1987).  A multidimensional version  *
*  of the Kolmogorov-Smirnov test.  Monthly Notices of the Royal       *
*  Astronomical Society, 225:155-170.                                  *
************************************************************************
*  V1A, 11/17/13, G.N. Reeke - New file, extracted from mdks.c         *
*  V1B, 11/19/13, GNR - Add mex file wrapper                           *
*  V2A, 12/06/13, GNR - Modify for brick method                        *
*  V3A, 02/08/14, GNR - More uniform distribution of data into bricks  *
*  ==>, 02/18/14, GNR - Last date before committing to svn repository  *
*  V5C, 04/25/14, GNR - Add brick links, partial quadrant totals       *
*  Rev, 04/26/14, GNR - Add memory overflow check                      *
*  Rev, 05/06/14, GNR - Add actual error messages to MATLAB wrapper    *
*  V6A, 05/10/14, GNR - Hierarchical bricks                            *
*  V6B, 05/22/14, GNR - Bricks keep index of next occupied brick       *
*  V6C, 05/27/14, GNR - Store ocube in Lvl, eliminate oprnt            *
*  Rev, 06/04/14, GNR - Revise default depth calculation per mdksoptV6 *
*  V7A, 08/13/15, GNR - Add ribbon, equi-partition, unequal-partition, *
*                       and center-of-gravity approximations           *
*  Rev, 08/21/15, GNR - Store quad differences in one set of bricks    *
*  R09, 11/25/15, GNR - Add controls to force 1- or 2-quad sums        *
*  Rev, 12/29/15, GNR - Bug fix:  Allow to free one of multiple Wkhds  *
*                       This required merging bfksallo with mdksallo.  *
*  Rev, 01/30/16, GNR - Add controls for cgks to use 1- or 2-quad sums *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#ifdef I_AM_MATLAB
#include "mex.h"
#endif
#include "mdks.h"
#include "mdksint.h"

/*=====================================================================*
*  N.B.  This program is intended to be included in a wrapper program  *
*  that defines the type specifiers KTX and KTN and the version iden-  *
*  tifier VL.  Alternatively, these variables can be defined in a      *
*  makefile.  If compiled without external definitions, mdksallo will  *
*  default to double-precision data with integer size parameters and   *
*  no name terminal letter.                                            *
*  (This indirect method is used because #if cannot test strings.)     *
*=====================================================================*/

static struct Wkhd_l pwhdlist;   /* Head of dynamic Wkhd list */

#ifdef I_AM_MATLAB
#define MyName vfnmex("mdksallo",VL)
#define printf mexPrintf

enum rhsargs { jwk, jd1, jg2, jn1, jn2, jk };

/*=====================================================================*
*                   mdksallox mex function wrapper                     *
*=====================================================================*/

void InnerAllo(int nlhs, mxArray *plhs[],
   int nrhs, const mxArray *prhs[]) {

   void *pwk,*qwk;               /* Ptrs to mdks work area */
   double darg;                  /* Double args for overflow checks */
   Nint  n1,n2;                  /* Array sizes converted to ints */
   int   d1,g2;                  /* Depths and method controls */
   int   k;                      /* Dimensionality */
   int   qfirst;                 /* TRUE if this is first call */
   int   rc;                     /* Return code from mdksallox */

/* Standard MATLAB check for proper numbers of arguments */

   if (nlhs == 1 && nrhs == jk+1) goto MdksDoAllo;
   else if (nlhs == 0 && nrhs == 2) goto MdksDoFree;
   else
      mexErrMsgTxt(MyName " expects 1 LH + 6 RH or 0 LH + 2 RH args.");

/* MATLAB call to free mdks work area */

MdksDoFree:

   /* Check that the 2nd arg is 0 -- this is just a sanity check */
   if (!mxIsDouble(prhs[1]) ||
         mxGetNumberOfElements(prhs[1]) != 1 ||
         mxGetScalar(prhs[1]) != 0.0)
      mexErrMsgTxt(MyName " 2nd arg must be 0 to free work area.");

   /* Extract the pointer to the work area from the jwk arg */
   if (!mxIsUint8(prhs[jwk]) ||
         mxGetNumberOfElements(prhs[jwk]) != sizeof(void *))
      mexErrMsgTxt(MyName " MDKSCOM arg is not expected type,size.");
   pwk = *(void **)mxGetData(prhs[jwk]);

   /* Perform here the same action as the mdksfree() routine
   *  to avoid having to link mdksfree() into this mex file.  */
   {  Wkhd *phd = (Wkhd *)pwk;
      if (!phd || phd->hdrlbl != HLBL) mexErrMsgTxt(MyName
         " MDKSCOM arg is not a valid work area.");
      /* Kill the hdrlbl in case user tries to reuse freed area */
      phd->hdrlbl = 0;
      /* Remove this one from the linked list */
      if (phd->wlnk.pnwhd) phd->wlnk.pnwhd->ppwhd = phd->wlnk.ppwhd;
      phd->wlnk.ppwhd->pnwhd = phd->wlnk.pnwhd;
      } /* End phd local scope */
   free(pwk);
   return;

/* MATLAB call to allocate or modify mdks work area */

MdksDoAllo:

   /* This is first call if pwhdlist.pnwhd is still NULL */
   qfirst = pwhdlist.pnwhd == NULL;

   /* Copy pwk from the first RHS arg because it may get changed
   *  and we should not try to change it in situ.  */
   if (mxIsEmpty(prhs[jwk])) pwk = NULL;
   else if (!mxIsUint8(prhs[jwk]) ||
         mxGetNumberOfElements(prhs[jwk]) != sizeof(void *))
      mexErrMsgTxt(MyName " MDKSCOM arg is not expected type,size.");
   else pwk = *(void **)mxGetData(prhs[0]);

   /* Convert the n1, n2, k, g1 args to ints or Nints */
   if (!mxIsDouble(prhs[jn1]) || mxGetNumberOfElements(prhs[jn1]) != 1)
      mexErrMsgTxt(MyName " n1 arg must be double-prec scalar.");
   darg = mxGetScalar(prhs[jn1]);
   if (darg < 0 || darg > mxNint)
      mexErrMsgTxt(MyName " n1 arg exceeds compiled max size.");
   n1 = (Nint)darg;

   if (!mxIsDouble(prhs[jn2]) || mxGetNumberOfElements(prhs[jn2]) != 1)
      mexErrMsgTxt(MyName " n2 arg must be double-prec scalar.");
   darg = mxGetScalar(prhs[jn2]);
   if (darg < 0 || darg > mxNint)
      mexErrMsgTxt(MyName " n2 arg exceeds compiled max size.");
   n2 = (Nint)darg;

   if (!mxIsDouble(prhs[jk]) || mxGetNumberOfElements(prhs[jk]) != 1)
      mexErrMsgTxt(MyName " k arg must be double-prec scalar.");
   darg = mxGetScalar(prhs[jk]);
   if (darg < 0 || darg > (double)MXK)
      mexErrMsgTxt(MyName " k arg exceeds max pgm can handle.");
   k = (int)darg;

   if (mxIsEmpty(prhs[jd1]))
      d1 = 0;
   else if (mxIsDouble(prhs[jd1]) &&
         mxGetNumberOfElements(prhs[jd1]) == 1) {
      darg = mxGetScalar(prhs[jd1]);
      if (darg < 0 || darg > MXK)
         mexErrMsgTxt(MyName " g1 arg exceeds max pgm can handle.");
      d1 = (int)darg; }
   else
      mexErrMsgTxt(MyName " g1 arg must be empty or scalar.");

   if (mxIsEmpty(prhs[jg2]))
      g2 = KC_MDKS;
   else if (mxIsDouble(prhs[jg2]) &&
         mxGetNumberOfElements(prhs[jg2]) == 1) {
      darg = mxGetScalar(prhs[jg2]);
      if (darg < 0 || darg > KC_ERLIM)
         mexErrMsgTxt(MyName " g2 arg specifies undefined method.");
      g2 = (int)darg; }
   else
      mexErrMsgTxt(MyName " g2 arg must be empty or scalar.");

   /* Allocate or update the work area */
   rc = vfnname(mdksallo,VL)(&pwk, &d1, &g2, n1, n2, k);
   switch (rc) {
   case 0:
      break;
   case MDKSA_ERR_BADWK:
      mexErrMsgTxt(MyName " first arg is not a previous work area.");
      break;
   case MDKSA_ERR_KLOW:
      mexErrMsgTxt(MyName " 'k' arg is negative or zero.");
      break;
   case MDKSA_ERR_KBIG:
      mexErrMsgTxt(MyName " quadrant sums will not fit in memory.");
      break;
   case MDKSA_ERR_N1LOW:
      mexErrMsgTxt(MyName " 'n1' is not larger than 'k'.");
      break;
   case MDKSA_ERR_N2LOW:
      mexErrMsgTxt(MyName " 'n2' is not larger than 'k'.");
      break;
   case MDKSA_ERR_G1HIGH:
      mexErrMsgTxt(MyName " 'g1' depth is too large to handle.");
      break;
   case MDKSA_ERR_BADG2:
      mexPrintf("g2 value is %d\n", g2);
      mexErrMsgTxt(MyName " 'g2' method is undefined.");
      break;
   case MDKSA_ERR_RIBK2:
      mexErrMsgTxt(MyName " ribbon method requires k == 2.");
      break;
   case MDKSA_ERR_ADSPCE:
      mexErrMsgTxt(MyName " size of work space exceeds system address "
         "space.\n  If on a 32-bit system, try a 64-bit system.");
      break;
   case MDKSA_ERR_NOMEM:
      mexErrMsgTxt(MyName " storage for work space not available.");
      break;
   default: {
      char Etxt[40];
      sprintf(Etxt, MyName " returned error %d.", rc);
      mexErrMsgTxt(Etxt);
      break;
      }
   }

   /* If this is the first call, activate the mexAtExit routine */
   if (qfirst) mexAtExit(mdksexit);

   /* Now return address of work area to caller */
   plhs[0] = mxCreateNumericMatrix(sizeof(void *), 1,
      mxUINT8_CLASS, mxREAL);
   qwk = mxGetData(plhs[0]);
   memcpy(qwk, &pwk, sizeof(void *));

   } /* End mexFunction */

/*** DEBUG - WRAPPER FOR THE WRAPPER ***/
void mexFunction(int nlhs, mxArray *plhs[],
   int nrhs, const mxArray *prhs[]) {

   InnerAllo(nlhs, plhs, nrhs, prhs);

   }
/*** ENDDEBUG ***/

#endif /* I_AM_MATLAB */


/*=====================================================================*
*                              mdksexit                                *
*                                                                      *
*  This routine releases any work area storage that has not already    *
*  been released by crawling the linked list at pwhdlist.  It may be   *
*  called from C programs or used as an "atexit" routine.  In this     *
*  capacity, it is the mexAtExit function that gets run when the       *
*  MATLAB user exits or issues 'clear mex'.  (MATLAB is assumed able   *
*  to manage its own arrays containing the pointers to work areas      *
*  returned by mdksallox().  The free-standing mdksfree() cannot be    *
*  used for this purpose because it has no way to access the static    *
*  pwhdlist (other than adding code to store a pointer to it in        *
*  system shared memory).                                              *
*=====================================================================*/

void mdksexit(void) {

   struct Wkhd_l *phl,*pnhl = NULL;
   for (phl=pwhdlist.pnwhd; phl; phl=pnhl) {
      Wkhd *phd = (Wkhd *)phl;
      pnhl = phl->pnwhd;               /* Save before release */
      if (phd->hdrlbl != HLBL) return; /* Error, no way to report it */
      /* Kill the hdrlbl in case user tries to reuse freed area */
      phd->hdrlbl = 0;
      /* Free the storage */
      free(phd);
      }

   /* Reinitialize the header list to indicate it is empty */
   pwhdlist.pnwhd = pwhdlist.ppwhd = NULL;

   } /* End mdksexit() */


/*=====================================================================*
*                              mdksallox                               *
*=====================================================================*/

/* Macro to align an item size to that of largest type used */
#define AlignUp(sz) ((((sz) + llal - 1)/llal) * llal)
/* Macro to test for overflow of sum of ints known to be positive */
#define jckis(A,B) (~(unsigned int)(A) < (unsigned int)(B))
#if KTN == 0
#define jckNs(A,B) (~(unsigned int)(A) < (unsigned int)(B))
#else
#define jckNs(A,B) (~(unsigned long)(A) < (unsigned long)(B))
#endif
#define jckzs(A,B) (~(A) < (B))
/* Indexes of items to be included in lwk when k > 1. */
enum jlwk { jlbr, jlbm, jlvl, jdim, jqfr, jrib, jcog, jln };

int vfnname(mdksallo,VL)(void **ppwk, int *g1, int *g2,
      Nint n1, Nint n2, int k) {
   char *pwk;
   Wkhd *phd;
   Dim  *pdim;
   Lvl  *plvl;
   size_t lwk,llal,lmxtb,lcog,l1qt,mxszt,nbrks,nq,tsz;
   size_t lhdr,lbins,llnkj[NDSET],litm[jln];
   Nint mxn,totn;
   int jk,jd,jx,kq,td;
   int kcalc,tkcal;
   /* Indexes of mdks methods in order of KC_ control bits */
   enum jksm { kcmd, kcrm, kcro, kcep, kccg, kcup, kcbf, kctop } jm;

   /* In order of KC method bits, which qtot codes are available
   *  and value of k where 2-quads are preferred.  */
   byte knqav[] = { 3, 1, 1, 1, 3, 1, 2 };
   byte k2qp[]  = { K2Q_MD, K2Q_RM, K2Q_RO, K2Q_EP, K2Q_CG,
      K2Q_UP, K2Q_BF };

   /* Sanity checks on args */
   /* Unsigned test will catch negs as big pos and give error */
   kcalc = (g2 && *g2 > 0) ? *g2 : KC_MDKS;
   if ((unsigned)kcalc > KC_ERLIM) return MDKSA_ERR_BADG2;
   if (kcalc && k == 1) return MDKSA_ERR_BADG2;
   if ((kcalc & (KC_QS1|KC_QS2)) == (KC_QS1|KC_QS2))
      return MDKSA_ERR_BADG2;
   if (kcalc & (KC_RMKS|KC_ROKS) && k != 2) return MDKSA_ERR_RIBK2;
   if (k <= 0)    return MDKSA_ERR_KLOW;
   if (k >= MXK)  return MDKSA_ERR_KBIG;
   if (n1 <= k)   return MDKSA_ERR_N1LOW;
   if (n2 <= k)   return MDKSA_ERR_N2LOW;

   /* Read depth or compute default depth.  The calculation given here
   *  was established as described in the comments above.  It has no
   *  theoretical basis and may be revised upon further study.  */
   mxn = n1 > n2 ? n1 : n2;
   if (g1 && *g1 > 0) {
      td = *g1;
      /* The following test must be commented out for some debug runs */
      if ((1 << td*k) > mxn) return MDKSA_ERR_G1HIGH;
      }
   else {
      double dmxn = (double)mxn;
      double dk   = (double)k;
      double dl2n = log2((double)mxn);
      double lnok = dl2n/dk;
      td = (int)round(1.9287E-6*dmxn*lnok + 1.0305*lnok +
         0.0101*dl2n + 0.3616*dk - 2.6180);
      }
   if (td <= 0) td = 1;

   /* If there is a valid existing work area, and new sizes
   *  are same as old, there is nothing to do.  */
   if ((phd = (Wkhd *)*ppwk)) {     /* Assignment intended */
      if (phd->hdrlbl != HLBL) return MDKSA_ERR_BADWK;
      if (n1 != phd->n[IX1] || n2 != phd->n[IX2] || k != phd->k)
         goto AllocArgsNotSame;
      if (kcalc != phd->kcalc)
         goto AllocArgsNotSame;
      if (k > 1 && td != phd->d)
         goto AllocArgsNotSame;
      /* The work area is OK as is */
      return 0;
      }

AllocArgsNotSame:
   /* Compute size of address space.  This must work regardless of
   *  whether size_t is a signed or an unsigned type...  */
   mxszt = ~(size_t)0 ^ (size_t)1 << (NBPB*sizeof(size_t)-1);
   /* These tests for memory overflow should be enough to assure
   *  that the allocation calculations below cannot overflow.  Then
   *  more careful tests can be done as allocations are added.  */
   tsz = mxszt/sizeof(Link);
   if (jckNs(n1,n2)) return MDKSA_ERR_ADSPCE;
   totn = n1 + n2;
   if (totn > tsz) return MDKSA_ERR_ADSPCE;
   tsz = (NBPB*sizeof(size_t)-1)/k;
   if (td > tsz) return MDKSA_ERR_ADSPCE;

   /* Compute size to align parts of work area */
   llal = sizeof(void *);
   if (sizeof(Xdat) > llal) llal = sizeof(Xdat);
   if (sizeof(Nint) > llal) llal = sizeof(Nint);
   if (sizeof(Prec) > llal) llal = sizeof(Prec);

   /* Determine whether it is necessary to allocate space for
   *  two sets of quad sums (or just one) */
   kq = (kcalc & KC_QS2A) ? 2 : 1;
   for (jm=kcmd,tkcal=kcalc; jm<kctop; ++jm,tkcal>>=1) {
      if (tkcal & 1 && knqav[jm] & 2 &&
         (!(knqav[jm] & 1) || kcalc & KC_QS2 || k >= (int)k2qp[jm])) {
            kq = 2; kcalc |= KC_QS2A; break; }
      }

   /* Compute size of work area.  Try to assure that lwk sum cannot
   *  exceed max value that will fit in a size_t.  Note:  sizeof()
   *  already aligns its argument up to the size needed to tile the
   *  type in an array, but to be fully safe the full array may need
   *  to be aligned up to the size of the largest type in the whole
   *  work area.  This can be omitted if the item contains a multiple
   *  of two pointers, Xdats, or Nints.  Also note that the Qtot
   *  arrays can be overlaid on the Bins array because they are not
   *  used at the same time.  Also, if k == 1, only the links and
   *  sorting bins are needed.  */
   lwk = lhdr = AlignUp(sizeof(Wkhd));
   /* Next three lines here to avoid warning, really for k > 1 case */
   memset(litm, 0, jln*sizeof(size_t));
   llnkj[IX1] = llnkj[IX2] = 0;
   l1qt = sizeof(Prec) << k;  /* Size of one set of quad totals */
   nq = (size_t)1 << k;
   lmxtb = (k > 1) ? kq*l1qt : 0;
   if (kcalc & (KC_MDKS|KC_RMKS|KC_ROKS|KC_EPKS|KC_UPKS)) {
      if (k > 1) {
         size_t lbrkj, nbrkj = 0 /* To remove warning */;
         nbrks = 0;              /* Count of bricks for one data set */
         lcog = k*sizeof(Xdat);  /* Len data for one point or cog */
         for (jd=1; jd<=td; ++jd) {
            nbrkj = (size_t)kq << jd*k;
            if (nbrkj > mxszt/sizeof(Brick)) return MDKSA_ERR_ADSPCE;
            if (jckzs(nbrks, nbrkj)) return MDKSA_ERR_ADSPCE;
            nbrks += nbrkj;
            lbrkj = AlignUp(nbrkj*sizeof(Brick));
            if (jckzs(litm[jlbr],lbrkj)) return MDKSA_ERR_ADSPCE;
            litm[jlbr] += lbrkj;
            } /* End loop over depth */
         /* Brick member lists are needed only for lowest depth */
         if (nbrkj > mxszt/(NDSET*sizeof(void *)))
            return MDKSA_ERR_ADSPCE;
         if (kcalc & (KC_MDKS|KC_RMKS))
            litm[jlbm] = AlignUp(NDSET*nbrkj*sizeof(void *));
         litm[jlvl] = AlignUp(td*sizeof(Lvl));
         litm[jdim] = AlignUp(k*sizeof(Dim));
         if (kcalc & (KC_RMKS|KC_ROKS|KC_UPKS))
            litm[jrib] = AlignUp((k<<td)*sizeof(Ribn));
         if (kcalc & KC_UPKS)
            litm[jqfr] = AlignUp(NENDS*k*sizeof(Prec));
         if (kcalc & KC_CGKS)
            litm[jcog] = AlignUp(NDSET*nbrkj*lcog);
         /* Safely compute the grand total */
         for (jx=0; jx<jln; ++jx) {
            if (jckzs(lwk,litm[jx])) return MDKSA_ERR_ADSPCE;
            lwk += litm[jx];
            }
         } /* End size calcs for k > 1 */
      /* Following items are used for all values of k
      *  (but not for brute force method)  */
      llnkj[IX1] = AlignUp(n1*sizeof(Link));
      if (jckzs(lwk,llnkj[IX1]))  return MDKSA_ERR_ADSPCE;
      lwk += llnkj[IX1];
      llnkj[IX2] = AlignUp(n2*sizeof(Link));
      if (jckzs(lwk,llnkj[IX2]))  return MDKSA_ERR_ADSPCE;
      lwk += llnkj[IX2];
      lbins = AlignUp(NBINS*sizeof(Bin));
      if (lbins > lmxtb) lmxtb = lbins;
      } /* End calcs for all but brute-force */
   /* Add in space for quad totals and/or sort bins */
   if (jckzs(lwk,lmxtb))         return MDKSA_ERR_ADSPCE;
   lwk += lmxtb;

   /* If an existing work area was not passed as an argument, allocate
   *  one now and place it at the start of the list of work areas at
   *  pwhdlist.  If there is an existing work area and the new one
   *  will fit in the allocation of the old one, just reuse it.
   *  Otherwise, use realloc() to make it big enough and update the
   *  pointers in the parent and child that point to this one if it
   *  moves.  Then, in all three cases, go on to fill in or replace
   *  the contents.  */
   if (!phd) {                   /* Make a new Wkhd */
      if (!(*ppwk = malloc(lwk))) return MDKSA_ERR_NOMEM;
      phd = (Wkhd *)(pwk = *ppwk);
      phd->wlnk.pnwhd = pwhdlist.pnwhd;
      phd->wlnk.ppwhd = &pwhdlist;
      if (pwhdlist.pnwhd) pwhdlist.pnwhd->ppwhd = &phd->wlnk;
      pwhdlist.pnwhd = &phd->wlnk;
      }
   else if (lwk > phd->lallo) {  /* Expand existing Wkhd */
      Wkhd *poldhd = phd;
      if (!(*ppwk = realloc(*ppwk, lwk))) return MDKSA_ERR_NOMEM;
      phd = (Wkhd *)(pwk = *ppwk);
      if (poldhd != phd) {
         if (phd->wlnk.pnwhd) phd->wlnk.pnwhd->ppwhd = &phd->wlnk;
         phd->wlnk.ppwhd->pnwhd = &phd->wlnk;
         }
      }
   else                          /* Reuse old Wkhd */
      phd = (Wkhd *)(pwk = *ppwk);

   /* Fill in the Wkhd */
   pwk += lhdr;
   phd->pLnk0[IX1] = (Link *)pwk, pwk += llnkj[IX1];
   phd->pLnk0[IX2] = (Link *)pwk, pwk += llnkj[IX2];
   /* Bins and Qtots share this space */
   phd->pQtot[0] = (Prec *)pwk;
   phd->pQtot[1] = (Prec *)(pwk + l1qt);  /* May never be used */
   phd->pBin0 = (Bin *)pwk, pwk += lmxtb;
   if (k > 1 && kcalc & (KC_MDKS|KC_RMKS|KC_ROKS|KC_EPKS|KC_UPKS)) {
      /* N.B.  Code in xxks1sx assumes ppBM and Cog arrays are
      *  pairwise adjacent and in the order established here.
      *  These usages are marked with the IXb block index.
      *  Set up dim info.  Use formula for sum of a geometric
      *  series to generate the mask.  */
      size_t tmask = (((size_t)1 << td*k) - 1)/(nq - 1);
      phd->pDim0 = pdim = (Dim *)pwk, pwk += litm[jdim];
      for (jk=0; jk<k; ++pdim,++jk) {
         pdim->oadd = (~((size_t)1 << jk) + 1) & (~tmask + 1);
         pdim->mask = tmask;
         tmask <<= 1;
         }
      /* Set up level and brick count info */
      phd->pLvl0 = plvl = (Lvl *)pwk, pwk += litm[jlvl];
      for (jd=1; jd<=td; ++plvl,++jd) {
         int jq;
         tsz = AlignUp(sizeof(Brick) << jd*k);
         for (jq=0; jq<kq; ++jq)
            plvl->pbrk[jq] = (Brick *)pwk, pwk += tsz;
         }
      phd->lbrks = litm[jlbr];
      /* Set up brick membership list info */
      phd->lbml = litm[jlbm];
      if (kcalc & (KC_MDKS|KC_RMKS)) {
         tsz = litm[jlbm] >> 1;
         phd->ppBm0[IX1] = (Link **)pwk, pwk += tsz;
         phd->ppBm0[IX2] = (Link **)pwk, pwk += tsz;
         }
      /* Set up ribbon info */
      if (kcalc & (KC_RMKS|KC_ROKS|KC_UPKS))
         phd->pRib0 = (Ribn *)pwk, pwk += litm[jrib];
      if (kcalc & KC_UPKS)
         phd->pQfr0 = (Prec *)pwk, pwk += litm[jqfr];
      /* Set up center-of-gravity info */
      if (kcalc & KC_CGKS) {
         tsz = litm[jcog] >> 1;
         phd->pCog0[IX1] = (Xdat *)pwk, pwk += tsz;
         phd->pCog0[IX2] = (Xdat *)pwk, pwk += tsz;
         phd->lcog = litm[jcog];
         }
      } /* End if k > 1 and not just brute force */
   phd->lallo = lwk;
   phd->sqrtn[IX1] = sqrt((double)n1);
   phd->sqrtn[IX2] = sqrt((double)n2);
   phd->n[IX1] = n1;
   phd->n[IX2] = n2;
   phd->d = td;
   phd->kcalc = kcalc;
   phd->k = k;
   phd->repct = 0;
   phd->hdrlbl = HLBL;

   /* Determine whether machine is Big-endian or Little-endian.  Doing
   *  it this way is very fast and avoids dependence on how this might
   *  be defined in system header files.  */
   {  union { int qlint; char qlch; } qlu;
      qlu.qlint = 1; phd->qle = (int)qlu.qlch; }

   return 0;

   } /* End mdksallox() */

