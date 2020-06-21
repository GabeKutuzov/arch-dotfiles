/* (c) Copyright 2018, The Rockefeller University *11115* */
/* $Id: dsrswjqe.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                              dsrswjqe                                *
*                                                                      *
*  This function performs scaled and rounded division of a signed      *
*  64-bit dividend by an si32 divisor and returns a 64-bit quotient.   *
*  An error is detected when the initial shift would overflow (use     *
*  dsrswwqe to allow 96-bit intermediate dividends)--the following     *
*  division cannot overflow.  The routine first scales the dividend    *
*  by a specified shift amount, then performs rounding by adding       *
*  or subtracting abs(divisor)/2 to the dividend.  Then it performs    *
*  the division and returns the quotient.                              *
*  This is the version that uses the error code passed as an argument. *
*  Division with rounding is done with vdivl() on 32-bit machines.     *
*                                                                      *
*  This routine reports errors via e64act() except divide by 0 is a    *
*  terminal error.  There is no version to return the remainder,       *
*  which is of no interest after rounding has been applied.            *
*                                                                      *
*  Synopsis: si64 dsrswjqe(si64 x, int s, si32 y, int ec)              *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.                                                   *
*                                                                      *
************************************************************************
*  V1A, 08/30/18, GNR - New routine, based on dsrswqe()                *
*  ==>, 08/31/18, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <math.h>
#include <stdlib.h>
#include "sysdef.h"
#include "rkarith.h"

si64 dsrswjqe(si64 x, int s, si32 y, int ec) {

   si64 ux, uy, sgnq;

   /* Error if attempt to divide by zero */
   if (y == 0) { abexitmq(72, "Divide by zero."); return jesl(0); }

#ifdef HAS_I64
   /* Compute sign of quotient, which will also be overflow
   *  return value  */
   sgnq = (x ^ (si64)y) < 0 ? -SI64_MAX : SI64_MAX;

   /* Scale absolute value of x */
   ux = abs64(x);
   if (s > 0) {
      /* Error if scaling would overflow--no need to do this
      *  test if s == 0, because sign bit is now clear */
      if (ux >> (63-s)) goto ReportOvflow;
      ux <<= s;
      }
   else
      ux >>= -s;

   /* Round and detect overflow into sign bit */
   uy = (ui64)(abs32(y));
   ux += (uy >> 1);
   if (ux < 0) goto ReportOvflow;

   /* Divide and assign sign of result */
   ux /= uy;
   return (sgnq < 0) ? -ux : ux;

ReportOvflow:
   e64act("dsrswjqe", ec);
   return sgnq;

# else               /* System does not have I64 */
   
   /* Compute sign of quotient, which will also be overflow
   *  return value  */
   sgnq = (x.hi ^ y) < (si32)0 ? jcsw(SI32_SGN,1) : SI64_MAX;

   /* Scale absolute value of x */
   ux = jsswe(abs64(x), s, ec);
   uy = jesl(abs32(y));

   /* Divide with rounding and assign sign of result */
   {  ui32 work[12];
      vdivl((ui32 *)&ux, (ui32 *)&uy, (ui32 *)&ux, NULL,
         work, 2, 1, TRUE);
      }  /* End work local scope */

   return (sgnq.hi < (si32)0) ? jnsw(ux) : ux;

#endif

   } /* End dsrswjqe() */
