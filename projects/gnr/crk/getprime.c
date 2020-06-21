/* (c) Copyright 1998-2008, The Rockefeller University *11115* */
/* $Id: getprime.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                              getprime                                *
*                                                                      *
*     Synopsis:  static unsigned long getprime(unsigned long minval)   *
*                                                                      *
*     Argument:  Any integer--normally twice the number of entries     *
*                to be stored in some hash table.                      *
*                                                                      *
*     Returns:   The lowest prime number greater than or equal to      *
*                the argument.                                         *
*                                                                      *
*     Errors:    Abend 78 if the next larger prime would exceed the    *
*                number of bits allocated for the result (32 or 64).   *
*                If minval < 3, it is returned unchanged.              *
*                                                                      *
*     Algorithm: Sort of brute-force with a few refinements.  It       *
*                is probably suitable only for occasional use...       *
*                The inner loop is 2x unrolled and sometimes does      *
*                two divisions when only one is needed, but the        *
*                number of saved loop tests should make up for it.     *
*                                                                      *
************************************************************************
*  V1A, 08/12/98, GNR - New routine                                    *
*  Rev, 11/15/08, GNR - Add overflow test                              *
*  ==>, 11/15/08, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <math.h>
#include "rkarith.h"

unsigned long getprime(unsigned long minval) {

   unsigned long i,imax,imxnxtsq;

   if (minval > 3) {       /* Avoid infinite loop */
      imax = (unsigned long)sqrt((double)(minval |= 1));
      { register unsigned long i1 = imax + 1; imxnxtsq = i1*i1; }
      for (;;) {
         if (imxnxtsq < imax) abexit(78);
         /* 3 is not tested by loop below */
         if ((minval % 3) == 0) goto NewMinVal;
         /* Only 6n+5 and 6n+7 can be primes */
         for (i=5; i<=imax; i+=6) {
            if ((minval % i) == 0) goto NewMinVal;
            if ((minval % (i+2)) == 0) goto NewMinVal;
            }
         goto ReturnPrime;
NewMinVal: minval += 2;
         if (minval < imax) abexit(78);
         if (imxnxtsq <= minval)
            imax += 1, imxnxtsq += imax + imax + 1;
         } /* End divisor test loop */
      }
ReturnPrime: return minval;

   } /* End getprime() */

