/* (c) Copyright 2014, The Rockefeller University */                   
/* $Id: strongmx.c 4 2014-03-17 21:12:51Z  $ */
/***********************************************************************
*            Strong et al. method for spike-train entropy              *
*                             strongmx.c                               *
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
*  This program is a Matlab mex file that performs the inner part of   *
*  the computation of the entropy of a sequence of neuronal firings    *
*  using the method given in the reference below and explicated in the *
*  comments.  It is written in C so that data "words" of any size can  *
*  be assembled and sorted with some ease.  It also contains code for  *
*  an alternative method suggested by a referee to a manuscript on     *
*  spike train entropy by N.D. Watters and G.N. Reeke, as interpreted  *
*  by G.N. Reeke.  In the alternative method, the "words" from which   *
*  the entropy is derived by the Shannon formula are constructed from  *
*  interspike-interval lengths expressed as multiples of the same bin  *
*  width used in the literature method.                                *
*                                                                      *
*  In this initial version of this program, the entropy is calculated  *
*  only for one word length.  It is assumed that the calling Matlab    *
*  program is responsible for organizing the data as an array of       *
*  interspike intervals (ISIs), for organizing the calculation over    *
*  some desired array of word lengths, for performing the extrapola-   *
*  tion to infinite word length described in the publication, and for  *
*  providing the results to the user as printed or graphical output.   *
*  A separate version callable from a C main program is not provided.  *
*                                                                      *
*  Matlab call synopsis:                                               *
*  H = strongmx(X, delta, N, wl, mxas)                                 *
*  where:                                                              *
*  X        is an array of interspike intervals in any desired units.  *
*  delta    is the bin width (> 0) for digitizing the data, in the     *
*           same units as X.                                           *
*  N        is the number of ISIs to use from the X array (which may   *
*           be larger than N samples).                                 *
*  wl       is the "word" length (number of bins in a "word" for the   *
*           literature method, number of ISIs in a "word" for the      *
*           alternative method, > 0).                                  *
*  mxas     if 0 indicates that the literature method should be        *
*           performed, otherwise is a positive integer giving the      *
*           "maximum alphabet size" for the alternative method.        *
*  H        is the result, in bits per bin of size delta.              *
*                                                                      *
*  All of the above arguments and results are standard Matlab double-  *
*  precision floating point scalars or arrays ('X').  'N', 'wl', and   *
*  'mxas' must be integers < 2^31 represented as doubles (any frac-    *
*  tions are silently discarded).                                      *
*----------------------------------------------------------------------*
*  Reference:  S.P. Strong, R. Koberle, R.R. de Ruyter van Steveninck, *
*  and W. Bialek, "Entropy and information in neural spike trains",    *
*  Phys. Rev. Letts. 80:197-200 (1997).                                *
************************************************************************
*  V1A, 02/28/14, G.N. Reeke - New program                             *
*  ==>, 02/28/14, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "mex.h"

/* Code assumes the following defined values--change only with care */
typedef unsigned char byte;
#define NBPB      8           /* Number of bits in a byte */
#define NBINS (2<<NBPB)       /* Number of bins needed for sorting */
#define MsgLbl  "Strong mex:" /* Function label for error messages */
/* Macro to align an item size to multiple of a type size */
#define AlignUp(sz,ll) (((sz + ll - 1)/ll) * ll)

/* Prototype for the inner C routine */
double strongmc(double *pX, double delta, int N, int wl, int mxas);

/* Structure used to hold a variable-length data "word" */
struct Word_t {
   struct Word_t *pnw;        /* Ptr to next word */
   int   npat;                /* Number of instances of a pattern */
   byte  bins[1];             /* Actually enough to hold a word */
   };
typedef struct Word_t Word;

/* Structure used to perform radix sort on data.
*  N.B.  As usual, the fiction is maintained that the head pointer
*  in a Bin is a complete Word.  This avoids extra tests inside the
*  sort loop but depends on pnw being the first item in the Word
*  struct.  If this ever changes, the sort code must also change.  */
struct Bin_t {
   Word *head;             /* Ptr to head of a sort bin */
   Word *tail;             /* Ptr to last entry in each bin */
   };
typedef struct Bin_t Bin;

enum rhsargs { jX, jd, jN, jwl, jma, jend };  /* Argument locators */

/*=====================================================================*
*            strongmx = mex function wrapper for strongmc              *
*=====================================================================*/

void mexFunction(int nlhs, mxArray *plhs[],
   int nrhs, const mxArray *prhs[]) {

   /* Arguments extracted for passing to strongmc() */
   double *pX;
   double H;
   double delta;
   int   N;
   int    wl;
   int   mxas;

   int   nX;                  /* Actual size of X array */

   /* Standard MATLAB check for proper numbers of arguments */
   if (nlhs != 1 || nrhs != jend)
      mexErrMsgTxt(MsgLbl " expects 1 LH + 5 RH args.");

   /* Convert the N, wl, mxas args to ints */
   if (!mxIsDouble(prhs[jN]) || mxGetNumberOfElements(prhs[jN]) != 1)
      mexErrMsgTxt(MsgLbl " N must be double-prec scalar.");
   N = (int)mxGetScalar(prhs[jN]);

   if (!mxIsDouble(prhs[jwl]) || mxGetNumberOfElements(prhs[jwl]) != 1)
      mexErrMsgTxt(MsgLbl " wl must be double-prec scalar.");
   wl = (int)mxGetScalar(prhs[jwl]);
   if (wl <= 0)
      mexErrMsgTxt(MsgLbl " wl must be > 0.");
   if (N <= wl)               /* JIC */
      mexErrMsgTxt(MsgLbl " N must be > wl.");

   if (!mxIsDouble(prhs[jma]) || mxGetNumberOfElements(prhs[jma]) != 1)
      mexErrMsgTxt(MsgLbl " mxas must be double-prec scalar.");
   mxas = (int)mxGetScalar(prhs[jma]);
   if (mxas < 0 || mxas >= NBINS)
      mexErrMsgTxt(MsgLbl " 0 <= mxas < 256 is required.");

   /* Pick up delta */
   if (!mxIsDouble(prhs[jd]) || mxGetNumberOfElements(prhs[jd]) != 1)
      mexErrMsgTxt(MsgLbl " delta arg must be double-prec scalar.");
   delta = mxGetScalar(prhs[jd]);
   if (delta <= 0.0)
      mexErrMsgTxt(MsgLbl " delta must be > 0.");

   /* Get pointer to X and be sure array has at least N entries */
   if (!mxIsDouble(prhs[jX]))
      mexErrMsgTxt(MsgLbl " X must be a double-prec array.");
   if (mxGetNumberOfElements(prhs[jX]) < (size_t)N)
      mexErrMsgTxt(MsgLbl " X array must have at least N elements.");
   pX = (double *)mxGetData(prhs[jX]);

   /* Perform the calculation and return the result */
   H = strongmc(pX, delta, N, wl, mxas);
   plhs[0] = mxCreateDoubleScalar(H);

   } /* End mexFunction() */


/*=====================================================================*
*                              strongmc                                *
*=====================================================================*/

double strongmc(double *pX, double delta, int N, int wl, int mxas) {

   char  *pcwd,*pcwd0;        /* Ptrs to Word array as chars */
   Word  *pwd,*pwd1;          /* Ptrs to Words */
   Word  *pwdu,*pwdc;         /* Ptrs to unique words, unique counts */
   Bin   *pBin0;              /* Ptr to array of sorting bins */
   double dnorm;              /* 1.0/(double)nwds */
   double H;                  /* Entropy result */
   double Hscl;               /* H * Hscl = H for one bin_width */
   double ttot;               /* Total data length */
   size_t lall;               /* Total length to allocate */
   size_t llal;               /* Larger of pointer or double size */
   size_t l1wd;               /* Length of one word */
   size_t lBins;              /* Size of sorting bins array */
   size_t lWords;             /* Length of all data words */
   size_t nbpw;               /* Number of bytes/word */
   size_t nwds;               /* Number of Words to allocate */

#ifdef DEBUG         /*** DEBUG ***/
   int    nuniq;
   int    iprint;
#endif               /*** ENDDEBUG ***/

   /* Determine size needed for aligning Word pointers */
   llal = sizeof(void *);
   if (sizeof(double) > llal) llal = sizeof(double);

   /* Find total length of the data--needed for both methods */
   ttot = 0.0;
   {  double *pxi,*pxe = pX + N;
      for (pxi=pX; pxi<pxe; ++pxi) ttot += *pxi;
      }

   /* Determine size needed for allocating Word structs.
   *  N.B.  As described in the literature reference, words
   *  are nonoverlapping.  */
   l1wd = sizeof(void *) + sizeof(int);
   if (mxas > 0) {
      /* Alternative method */
      nwds = N/wl;
      nbpw = wl;
      l1wd += wl;
      Hscl = delta*(double)nwds/ttot;
      }
   else {
      /* Literature method */
      nwds = (size_t)(ttot/delta)/wl;
      nbpw = (wl + NBPB - 1)/NBPB;
      l1wd += nbpw;
      Hscl = 1.0/(double)wl;
      }
   if (nwds <= 0) return 0.0;    /* JIC */
      
   l1wd = AlignUp(l1wd, llal);
   lWords = nwds * l1wd;

   /* Size needed for sorting bins */
   lBins = NBINS*sizeof(Bin);

   /* Allocate and clear working space */
   pBin0 = (Bin *)mxMalloc(lall = lBins + lWords);
   pcwd0 = (char *)pBin0 + lBins;
   memset((char *)pBin0, 0, lall);

   pwd1 = pwd = (Word *)(pcwd = pcwd0);   /* Start word list */
   if (mxas > 0) {

/* Alternative method -- digitize ISIs instead of bins.  Loop
*  over ISI data, then over word length.  For each ISI, compute
*  its length in multiples of delta, apply max, store in data
*  structure.  */

      double *pxi,*pxe = pX + nwds*wl;
      double dmxas = (double)mxas;
      int iwo = 0;
      for (pxi=pX; pxi<pxe; ++pxi) {
         double dli = *pxi/delta;      /* ISI in bin units */
         if (dli > dmxas) dli = dmxas;
         pwd->bins[iwo] = (byte)dli;
         if (++iwo >= wl) {            /* Time for a new word? */
            pwd = pwd->pnw = (Word *)(pcwd += l1wd);
            iwo = 0;
            }
         }
      ((Word *)(pcwd - l1wd))->pnw = NULL;      /* Terminate list */
      }

   else {

/* Literature method -- digitize data into bins.  Because bin_width
*  is <= min ISI, each bin is either 0 or 1.  Therefore, bins are
*  implemented as single bits.  Loop over ISI data, dividing time
*  into bins, store in data structure.  This method avoids need to
*  generate cumulative sums first.  */

      double *pxi = pX;
      char   *pcwe = pcwd0 + lWords;
      byte   *pbyt;
      double Xrem = *pxi;     /* Remaining time in current ISI */
      int    mbits;
      byte   ibit,ibit0 = 1 << (NBPB-1);
      for (pcwd=pcwd0; pcwd<pcwe; ) {
         pwd = (Word *)pcwd;
         pbyt = pwd->bins;
         for (mbits=wl; ; ++pbyt) {
            for (ibit=ibit0; ibit>0; ibit>>=1) {
               if (Xrem <= delta) {
                  *pbyt |= ibit;
                  Xrem += *++pxi;
                  }
               Xrem -= delta;
               if (--mbits <= 0) goto WordDone;
               } /* End bit loop */
            } /* End byte loop */
WordDone: pwd->pnw = (Word *)(pcwd += l1wd);
         } /* End word loop */
      pwd->pnw = NULL;                 /* Terminate list */
      } /* End data setup for literature method */

/* To find unique words, sort the data list using a radix sort
*  (like an old card sorter) with time proportional to nwds*n1wd.
*  N.B.  Because this code is only looking for duplicate words, the
*  sort does not have to order them on magnitude and we do not need
*  to deal differently with big-endian vs little-endian machines.  */

#ifdef DEBUG         /*** DEBUG ***/
if (wl == 3) {
   mexPrintf("Before Sort, Word contents:\n");
   for (pwd = pwd1; pwd; pwd=pwd->pnw) {
      mexPrintf("  pwd = %p, npat = %d, bins = %x\n",
         pwd, pwd->npat, (int)pwd->bins[0]);
      }
   }
#endif               /*** ENDDEBUG ***/

   {  Bin *pbe = pBin0 + NBINS;
      Word **ppw;
      int ii;
      /* Loop over bytes in the radix sort */
      for (ii=0; ii<nbpw; ++ii) {
         Bin *pb;                /* Ptr to a sorting bin */

         /* Initialize sorting bins for this key digit */
         for (pb=pBin0; pb<pbe; ++pb) {
            pb->head = NULL;     /* Empty bin signal */
            pb->tail = (Word *)&pb->head;
            }

         /* Traverse linked list, extracting current key digit and
         *  assigning each record to top of appropriate bin.  */
         for (pwd=pwd1; pwd; pwd=pwd->pnw) {
            pb = pBin0 + pwd->bins[ii];
            pb->tail->pnw = pwd;
            pb->tail = pwd;
            } /* End loop over data records */

         /* End of pass, stack up the bins in order */
         for (ppw=&pwd1,pb=pBin0; pb<pbe; ++pb) if (pb->head) {
            *ppw = pb->head;
            ppw = &pb->tail->pnw;
            }
         *ppw = NULL;            /* Terminate linked list */
         } /* End loop over bytes in data words */
      } /* End pbe,ii local scope */

#ifdef DEBUG         /*** DEBUG ***/
if (wl == 3) {
   mexPrintf("After Sort, Word contents:\n");
   for (pwd = pwd1; pwd; pwd=pwd->pnw) {
      mexPrintf("  pwd = %p, npat = %d, bins = %x\n",
         pwd, pwd->npat, (int)pwd->bins[0]);
      }
   }
#endif               /*** ENDDEBUG ***/

/* Scan sorted list.  Count number of unique patterns and count
*  number of each different kind in the nuniq words in the list.  */

   pwdu = pwdc = pwd1, pwdc->npat = 1;

#ifdef DEBUG         /*** DEBUG ***/
   nuniq = 1;
#endif               /*** ENDDEBUG ***/

   for (pwd=pwd1->pnw; pwd; pwd=pwd->pnw) {
      if (memcmp(pwd->bins, pwdu->bins, nbpw)) {
         /* New word found */
         pwdu = pwd;
         pwdc = pwdc->pnw;
         pwdc->npat = 1;

#ifdef DEBUG         /*** DEBUG ***/
         ++nuniq;
#endif               /*** ENDDEBUG ***/

         }
      else
         /* Duplicate, just count */
         ++pwdc->npat;
      }
   /* Terminate list after last unique count */
   pwdc->pnw = NULL;

#ifdef DEBUG         /*** DEBUG ***/
if (wl == 3) {
   mexPrintf("After Pattern count, nuniq = %d, word contents:\n",
      nuniq);
   for (pwd = pwd1; pwd; pwd=pwd->pnw) {
      mexPrintf("  pwd = %p, npat = %d, bins = %x\n",
         pwd, pwd->npat, (int)pwd->bins[0]);
      }
   }
#endif               /*** ENDDEBUG ***/

/* Scan list of unique word counts, compute entropy */

   H = 0.0;
   dnorm = 1.0/(double)nwds;

#ifdef DEBUG         /*** DEBUG ***/
   iprint = 0;
#endif               /*** ENDDEBUG ***/

   for (pwdc=pwd1; pwdc; pwdc=pwdc->pnw) {
      double prob = dnorm*(double)pwdc->npat;
      H -= prob * log2(prob);

#ifdef DEBUG         /*** DEBUG ***/
if (++iprint <= 100) {
   mexPrintf("Word %d = %x, npat = %d, prob = %f, H = %f\n",
      iprint, (int)pwdc->bins[0], pwdc->npat, prob, H);
   }
#endif               /*** ENDDEBUG ***/

      }


   mxFree(pBin0);
   /* Convert to entopy per bin as specified above */
   return H*Hscl;

   } /* End strongmc() */

