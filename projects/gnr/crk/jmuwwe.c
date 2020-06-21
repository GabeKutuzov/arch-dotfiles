/* (c) Copyright 2012-2013, The Rockefeller University *11115* */
/* $Id: jmuwwe.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               jmuwwe                                 *
*                                                                      *
*  This function multiplies two unsigned 64 bit integers and returns   *
*  the unsigned 64-bit product with full overflow checking.            *
*                                                                      *
*  Synopsis: ui64 jmuwwe(ui64 x, ui64 y, int ec)                       *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.                                                   *
*                                                                      *
************************************************************************
*  V1A, 08/19/12, GNR - New routine                                    *
*  ==>, 08/19/12, GNR - Last date before committing to svn repository  *
*  Rev, 10/25/13, GNR - Use SI64_SGN as defined in sysdef.h            *
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

ui64 jmuwwe(ui64 x, ui64 y, int ec) {

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
   return jauwe(jmuw(xlo,ylo), ans << BITSPERUI32, ec);

#else    /* Do it with 32-bit arithmetic */

   /* Test for overflow of intermediate sum  */
   if (uwhi(ans) || (uwhi(xy1) | uwhi(xy2)) & SI32_SGN)
      goto JMUWWE_OVERFLOW;
   return jauwe(jmuw(xlo,ylo), jcuw(uwlo(ans), 0), ec);

#endif

/* Got overflow, report and return max result if e64act returns */
JMUWWE_OVERFLOW:
   e64act("jmuwwe", ec);
   return UI64_MAX;

   } /* End jmuwwe() */
