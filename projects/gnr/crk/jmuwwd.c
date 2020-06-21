/* (c) Copyright 2013, The Rockefeller University *11115* */
/* $Id: jmuwwd.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               jmuwwd                                 *
*                                                                      *
*  This function multiplies two unsigned 64 bit integers and returns   *
*  the unsigned 64-bit product with full overflow checking.            *
*  This is the version that uses the error code stored by e64dec().    *
*                                                                      *
*  Synopsis: ui64 jmuwwd(ui64 x, ui64 y)                               *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.                                                   *
*                                                                      *
************************************************************************
*  V1A, 10/24/13, GNR - New routine                                    *
*  ==>, 10/24/13, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

ui64 jmuwwd(ui64 x, ui64 y) {

   ui32 xhi = uwhi(x), xlo = uwlo(x);
   ui32 yhi = uwhi(y), ylo = uwlo(y);
   ui64 ans,xy1,xy2;

   if (xhi && yhi)
      goto JMUWWE_OVERFLOW;
   xy1 = jmuw(xhi,ylo);
   xy2 = jmuw(xlo,yhi);
   ans = jauw(xy1,xy2);

#ifdef HAS_I64

   /* Note on overflow test here:  Any bit in high-order (xy1+xy2)
   *  is an overflow.  Even if this is zero, there could be a rare
   *  overflow if high-order bit was set in either summand.  */
   if (uwhi(ans) || (xy1 | xy2) & SI64_SGN)
      goto JMUWWE_OVERFLOW;
   return jauwd(jmuw(xlo,ylo), ans << BITSPERUI32);

#else    /* Do it with 32-bit arithmetic */

   /* Test for overflow of intermediate sum  */
   if (uwhi(ans) || (uwhi(xy1) | uwhi(xy2)) & SI32_SGN)
      goto JMUWWE_OVERFLOW;
   return jauwd(jmuw(xlo,ylo), jcuw(uwlo(ans), 0));

#endif

/* Got overflow, report and return max result if e64dac returns */
JMUWWE_OVERFLOW:
   e64dac("jmuwwd");
   return UI64_MAX;

   } /* End jmuwwd() */
