/* (c) Copyright 2013-2014, The Rockefeller University */
/* $Id: mdksallo.c 6 2014-06-09 19:19:58Z  $ */
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
*  used by the other routines in the mdks package.  Work area manage-  *
*  ment is performed by a separate routine so the same work area can   *
*  be reused for multiple MDKS calculations with the same parameters.  *
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
*  precision data must be called before calls to any of the other      *
*  routines in the MDKS package or any time one of the parameters      *
*  changes.  The value returned is a pointer to a work area that is    *
*  an argument to the other functions.  Once allocated, the same work  *
*  area can be used for multiple calculations as long as all the       *
*  parameters remain unchanged.                                        *
*                                                                      *
*     The default depth of bisection in the brick hierarchy has been   *
*  established by a polynomial fit to the optimum depth found in tests *
*  with k=2,...,5 and n={200,1000,5000,25000,125000}.  This polynomial *
*  may not be optimal for other data set sizes and may be modified in  *
*  future versions of this program.  Letting maxn = max(n1,n2), lmxn = *
*  log2(maxn), and lnok = lmxn/k, the default depth is:                *
*  (int)round(1.9287E-6*maxn*lnok + 1.0305*lnok + 0.0101*lmxn +        *
*  0.3616*k - 2.6180)                                                  *
*                                                                      *
*     The size of the work area allocated by this routine depends on   *
*  the problem parameters and the pointer (PSIZE) and data count (int  *
*  or long, NSIZE) sizes.  Let n=n1+n2, d=hypercube hierarchy depth,   *
*  which is either an input value or the default described above,      *
*  b=number of bricks=Sum(j=1..d)(2^(j*k)) and bb=number of bottom-    *
*  level bricks=2^(d*k).  Then when k > 1, the work area size is       *
*  approximately max(512*PSIZE, 2*2^k*NSIZE) + (20+3n+2k+6d+2bb)*PSIZE *
*  + 4b*NSIZE plus a small fixed amount of overhead.  When k == 1,     *
*  the work area size is (532+3n)*PSIZE plus overhead.                 *
*                                                                      *
*  MATLAB Synopsis:                                                    *
*     MDKSCOM = mdksallox(MDKSCOM, g1, g2, n1, n2, k)                  *
*        where g1, g2, n1, n2, and k are MATLAB double-precision       *
*        scalars (1 x 1 matrices).                                     *
*  C or C++ Synopsis:                                                  *
*     #include "mdks.h"                                                *
*     int mdksallox(void **ppwk, int *g1, int *g2, NDAT_T n1,          *
*        NDAT_T n2, int k)                                             *
*                                                                      *
*  Arguments:                                                          *
*     k        Number of dimensions of the data points, 1 <= k < MXK,  *
*              where MXK is 8*PSIZE - 4 - KTN.                         *
*     n1       Number of points in distribution X1, k < n1 < MXN.      *
*     n2       Number of points in distribution X2, k < n2 < MXN.      *
*              (NDAT_T represents the type of 'n1' and 'n2' for which  *
*              the particular version of mdksallox was compiled.  The  *
*              type NDAT_T is determined by the compile-time variable  *
*              'KTN', which is 0 for 'int' and 1 for 'long'.  The      *
*              actual data are passed as arguments to mdks1sx() or     *
*              mdks2sx().  MXN depends on the memory available in the  *
*              system.                                                 *
*     g1,g2    These arguments are pointers for compatibility with an  *
*              earlier version of mdksallox().  g1 overrides the depth *
*              of the calculation with a user-specified value,         *
*              0 <= g1 < MXK.  g1 may be 0 or a NULL pointer (empty    *
*              MATLAB array) to use the default depth.  g2 is ignored. *
*     ppwk     Pointer to a location where mdksallox() will return a   *
*              pointer to the work area described above.  The pointer  *
*              at *ppwk must be initialized to NULL for the first call *
*              to mdksallox()--a non-NULL value indicates that this    *
*              call is intended to update an existing work area at     *
*              that location for data with different size parameters.  *
*              The pointer at *ppwk must be passed as the first argu-  *
*              ment in each following call to mdks1sx(), mdks2sx(), or *
*              mdksfree().                                             *
*     MDKSCOM  MATLAB equivalent of pwk.  This should be an empty      *
*              array on the first call, otherwise a value returned     *
*              by a previous call if the intent is to modify an        *
*              existing work area.                                     *
*                                                                      *
*  Return values:                                                      *
*     mdksallox() returns (at *ppwk) a pointer to a work area that     *
*  will be used by mdks1sx() or mdks2sx() during its calculations.     *
*  The contents of this work area are not of interest to the calling   *
*  program.  Care should be taken not to use stale pointers if multi-  *
*  ple calls are made to mdksallox() with different 'ppwk', as the     *
*  work area may be moved to a new location if it needs to be expanded.*
*  In this case, the pointer at *ppwk will be modified.  The function  *
*  return from mdksallox() is an integer which is zero in case of      *
*  success.  Nonzero values indicate errors as follows:                *
*     (1) On an attempt to modify the work area, the value at *ppwk    *
*         did not appear to point to a previously allocated work area. *
*     (2) 'k' is negative or zero.                                     *
*     (3) Quadrant sums will not fit in memory.                        *
*     (4) 'n1' is not larger than 'k'.                                 *
*     (5) 'n2' is not larger than 'k'.                                 *
*     (6) A cube depth specified in 'g1' is larger than 8*PSIZE/k.     *
*     (7) [No longer used].                                            *
*     (8) Size of work space exceeds system address space.  If running *
*         in a 32-bit system, problem might fit in a 64-bit system.    *
*     (9) The required amount of storage was not available.  In        *
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
*                                                                      *
*  Arguments:                                                          *
*     pwk      This must be the pointer stored at *ppwk by a previous  *
*              call to mdksallox().                                    *
*     MDKSCOM  MATLAB equivalent of pwk.                               *
*                                                                      *
*  Return values:                                                      *
*     mdksfree() returns 0 on success and 1 if the argument 'pwk'      *
*  apparently did not point to an area allocated by an earlier call    *
*  to mdksallox().  The MATLAB mex file gives a terminal error if      *
*  the operation was not successful.                                   *
*                                                                      *
*  Design Note:                                                        *
*     Using a variant of mdksallox() as the MATLAB call to release a   *
*  work area allows it to access the location of the work area stored  *
*  in the static mxArray at pmxwk so that this array can be destroyed  *
*  and the pointer set NULL so as to inactivate the mexAtExit routine  *
*  that otherwise would be called and would cause an error attempting  *
*  to release storage that was already released.                       *
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
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#ifdef I_AM_MATLAB
#include "mex.h"
#endif
#include "ank_mdks.h"
#include "ank_mdksint.h"

/*=====================================================================*
*  N.B.  This program is intended to be included in a wrapper program  *
*  that defines the type specifiers KTX and KTN and the version iden-  *
*  tifier VL.  Alternatively, these variables can be defined in a      *
*  makefile.  If compiled without external definitions, mdksallo will  *
*  default to double-precision data with integer size parameters and   *
*  no name terminal letter.                                            *
*  (This indirect method is used because #if cannot test strings.)     *
*=====================================================================*/

#ifdef I_AM_MATLAB
#define MyName vfnmex("ank_mdksallo",VL)

static mxArray *pmxwk = NULL;    /* Ptr to mxArray that contains pwk */
enum rhsargs { jwk, jd1, jd2, jn1, jn2, jk };

/*=====================================================================*
*                              mdksexit                                *
*                                                                      *
*  This is the exit function that gets run when the user exits MATLAB  *
*  or issues 'clear mex'.  It releases the work area storage and the   *
*  mxArray that points to it if that was not already done.  The free-  *
*  standing mdksfree() cannot be used for this purpose because it has  *
*  no way to access the static pmxwk (other than adding code to store  *
*  a pointer to it in system shared memory).                           *
*=====================================================================*/

static void mdksexit(void) {

   if (pmxwk) {
      Wkhd *phd = *(Wkhd **)mxGetData(pmxwk);
      if (!phd || phd->hdrlbl != HLBL) return;
      /* Kill the hdrlbl in case user tries to reuse freed area */
      phd->hdrlbl = 0;
      /* Free the storage */
      free(phd);
      mxDestroyArray(pmxwk);
      /* Indicate it is gone */
      pmxwk = NULL;
      }

   } /* End mdksexit() */

/*=====================================================================*
*                   mdksallox mex function wrapper                     *
*=====================================================================*/

void InnerAllo(int nlhs, mxArray *plhs[],
   int nrhs, const mxArray *prhs[]) {

   void *pwk;                    /* Ptr to mdks work area */
   void *pmxptr;                 /* Ptr to data in pmxwk */
   double darg;                  /* Double args for overflow checks */
   Nint  n1,n2;                  /* Array sizes converted to ints */
   int   d1,d2;                  /* Depths */
   int   k;                      /* Dimensionality */
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

   /* Be sure it is the same as the pointer stored at pmxwk */
   if (!pmxwk)
      mexErrMsgTxt(MyName " attempt to free work area already freed.");
   if (pwk != *(void **)mxGetData(pmxwk))
      mexErrMsgTxt(MyName " MDKSCOM arg is not current work area.");

   /* Free the work area */
   mdksexit();
   return;

/* MATLAB call to allocate or modify mdks work area */

MdksDoAllo:

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

   d2 = 0;

   /* Allocate or update the work area */
   rc = vfnname(ank_mdksallo,VL)(&pwk, &d1, &d2, n1, n2, k);
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

   /* If not already there, allocate a pmxwk array to store pwk
   *  for possible use later for releasing the work area.  */
   if (!pmxwk) {
      pmxwk = mxCreateNumericMatrix(sizeof(void *), 1,
         mxUINT8_CLASS, mxREAL);
      mexMakeArrayPersistent(pmxwk);
      mexAtExit(mdksexit);
      }
   pmxptr = mxGetData(pmxwk);
   memcpy(pmxptr, &pwk, sizeof(void *));

   /* Now allocate a second copy to return to the caller */
   plhs[0] = mxCreateNumericMatrix(sizeof(void *), 1,
      mxUINT8_CLASS, mxREAL);
   pmxptr = mxGetData(plhs[0]);
   memcpy(pmxptr, &pwk, sizeof(void *));

   } /* End mexFunction */

/*** DEBUG - WRAPPER FOR THE WRAPPER ***/
void mexFunction(int nlhs, mxArray *plhs[],
   int nrhs, const mxArray *prhs[]) {

   InnerAllo(nlhs, plhs, nrhs, prhs);

   }
/*** ENDDEBUG ***/

#endif /* I_AM_MATLAB */

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
#define NADIM 2

/* Indexes of items to be included in lwk when k > 1. */
enum jlwk { jlbr, jlbm, jlvl, jdim, jrib, jln };

int vfnname(ank_mdksallo,VL)(void **ppwk, int *g1, int *g2,
      Nint n1, Nint n2, int k) {
   char *pwk;
   Wkhd *phd;
   Dim  *pdim;
   Lvl  *plvl;
   Prec *ptqt;
   size_t lwk,llal,lmxtb,mxszt,nbrks,nq,tsz;
   size_t lhdr,lbins,ltots,llnkj[NDSET],litm[jrib];
   Nint mxn,totn;
   int jk,jd,jx,td;

   /* Sanity checks on args */
   if (k <= 0)    return MDKSA_ERR_KLOW;
   if (k >= MXK)  return MDKSA_ERR_KBIG;
   if (n1 <= k)   return MDKSA_ERR_N1LOW;
   if (n2 <= k)   return MDKSA_ERR_N2LOW;

   /* Read depth or compute default depth.  The calculation given here
   *  was established as described in the comments above.  It has no
   *  theoretical basis and may be revised upon further study.  */
   mxn = n1 > n2 ? n1 : n2;
   if (g1 && *g1 > 0)
      td = *g1;
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
      if (k > 1 && td != phd->d)
         goto AllocArgsNotSame;
      /* The work area is OK as is */
      return 0;
      }

AllocArgsNotSame:
   /* Compute size of address space.  This must work regardless of
   *  whether size_t is a signed or an unsigned type...  */
   mxszt = ~(size_t)0 ^ (size_t)1 << (NBPB*sizeof(size_t)-1);
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
   /* Next two lines here to avoid warning, really for k > 1 case */
   memset(litm, 0, jln*sizeof(size_t));
   nbrks = 0;                 /* Count of bricks for one data set */
   nq = (size_t)1 << k;
   if (k > 1) {
      size_t lbrkj, nbrkj = 0 /* To remove warning */;
      for (jd=1; jd<=td; ++jd) {
         nbrkj = (size_t)1 << jd*k;
         if (nbrkj > mxszt/(2*sizeof(Brick))) return MDKSA_ERR_ADSPCE;
         if (jckzs(nbrks, nbrkj)) return MDKSA_ERR_ADSPCE;
         nbrks += nbrkj;
         lbrkj = AlignUp(nbrkj*sizeof(Brick));
         if (jckzs(litm[jlbr],lbrkj)) return MDKSA_ERR_ADSPCE;
         litm[jlbr] += lbrkj;
         } /* End loop over depth */
      if (nbrkj > mxszt/(2*sizeof(void *))) return MDKSA_ERR_ADSPCE;
      litm[jlbm] = AlignUp(2*nbrkj*sizeof(void *));
      litm[jlvl] = AlignUp(td*sizeof(Lvl));
      litm[jdim] = AlignUp(k*sizeof(Dim));
      ltots = sizeof(Prec) << (k+1);
      litm[jrib] = AlignUp((2 << td) * sizeof(ORibn));
      /* Safely compute the grand total */
      for (jx=0; jx<jln; ++jx) {
         if (jckzs(lwk,litm[jx])) return MDKSA_ERR_ADSPCE;
         lwk += litm[jx];
         }
      }
   else
      ltots = 0;
   /* Following items are used for all values of k */
   llnkj[IX1] = AlignUp(n1*sizeof(Link));
   if (jckzs(lwk,llnkj[IX1]))  return MDKSA_ERR_ADSPCE;
   lwk += llnkj[IX1];
   llnkj[IX2] = AlignUp(n2*sizeof(Link));
   if (jckzs(lwk,llnkj[IX2]))  return MDKSA_ERR_ADSPCE;
   lwk += llnkj[IX2];
   lbins = AlignUp(NBINS*sizeof(Bin));
   lmxtb = ltots > lbins ? ltots : lbins;
   if (jckzs(lwk,lmxtb))       return MDKSA_ERR_ADSPCE;
   lwk += lmxtb;

   /* If there is no existing work area, allocate one now.  If there
   *  is an existing work area and the new one will fit in the allo-
   *  cation of the old one, just reuse it.  Otherwise, use realloc()
   *  to make it big enough and replace the contents that differ.  */
   if (!phd) {
      if (!(*ppwk = malloc(lwk))) return MDKSA_ERR_NOMEM;
      }
   else if (lwk > phd->lallo) {
      if (!(*ppwk = realloc(*ppwk, lwk))) return MDKSA_ERR_NOMEM;
      }
   phd = (Wkhd *)(pwk = *ppwk);

   /* Fill in the Wkhd */
   pwk += lhdr;
   phd->pLnk0[IX1] = (Link *)pwk, pwk += llnkj[IX1];
   phd->pLnk0[IX2] = (Link *)pwk, pwk += llnkj[IX2];
   phd->pBin0 = (Bin *)pwk, ptqt = (Prec *)pwk;
   pwk += lmxtb;
   if (k > 1) {
      /* N.B.  Code in mdks1sx assumes pbct, ppBM, and Qtot arrays
      *  are pairwise adjacent and in the order established here.
      *  These usages are marked with the IXb block index.  */
      phd->d = td;
      /* Set up dim info.  Use formula for sum of a geometric
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
         tsz = AlignUp(sizeof(Brick) << jd*k);
         plvl->pbrk1[IX1] = (Brick *)pwk, pwk += tsz;
         plvl->pbrk1[IX2] = plvl->pbrk1[IX1] + nbrks;
         }
      /* Account for 2nd set of Bricks and total length of all Bricks */
      pwk += (phd->lbrks = litm[jlbr]);
      /* Set up brick membership list info */
      tsz = (phd->lbml = litm[jlbm]) >> 1;
      phd->ppBm0[IX1] = (Link **)pwk, pwk += tsz;
      phd->ppBm0[IX2] = (Link **)pwk, pwk += tsz;
      /* Set up quadrant count info */
      phd->pQtot[IX1] = ptqt, ptqt += nq;      
      phd->pQtot[IX2] = ptqt;
      /* Set up ribbon info */
      phd->pORibn_dat = (ORibn *)pwk, pwk += litm[jrib];
      } /* End if k > 1 */
   phd->lallo = lwk;
   phd->sqrtn[IX1] = sqrt((double)n1);
   phd->sqrtn[IX2] = sqrt((double)n2);
   phd->n[IX1] = n1;
   phd->n[IX2] = n2;
   phd->k = k;
   phd->hdrlbl = HLBL;

   
   /* Determine whether machine is Big-endian or Little-endian.  Doing
   *  it this way is very fast and avoids dependence on how this might
   *  be defined in system header files.  */
   {  union { int qlint; char qlch; } qlu;
      qlu.qlint = 1; phd->qle = (int)qlu.qlch; }

   return 0;

   } /* End mdksallox() */

