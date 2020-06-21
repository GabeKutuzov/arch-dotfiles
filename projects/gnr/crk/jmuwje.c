/* (c) Copyright 2013, The Rockefeller University *11115* */
/* $Id: jmuwje.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               jmuwje                                 *
*                                                                      *
*  This function multiplies a 64 bit unsigned integer by a 32-bit      *
*  unsigned integer and returns the unsigned 64-bit product with full  *
*  overflow checking.                                                  *
*                                                                      *
*  Synopsis: ui64 jmuwje(ui64 x, ui32 y, int ec)                       *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.                                                   *
*                                                                      *
************************************************************************
*  V1A, 08/26/13, GNR - New routine                                    *
*  ==>, 08/26/13, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

ui64 jmuwje(ui64 x, ui32 y, int ec) {

   ui64 xyhi,xylo,tw;
   ui32 xhi = uwhi(x), xlo = uwlo(x);

   tw = jmuw(xhi,y);
   if (uwhi(tw)) goto JMUWJE_OVERFLOW;
   xyhi = jsluw(tw, BITSPERUI32);
   xylo = jmuw(xlo,y);
   /* Note:  Any bit in high-order (uwhi(xyhi)) is an overflow.
   *  Even if this is zero, there could be a rare overflow if
   *  high-order bit was set in either summand.  */
   tw = jauw(jsruw(xylo, BITSPERUI32), jsruw(xyhi, BITSPERUI32));
   if (uwhi(tw)) goto JMUWJE_OVERFLOW;
   return jauw(xyhi, xylo);

   /* Got overflow, report and return max result if e64act returns */
JMUWJE_OVERFLOW:
   e64act("jmuwje", ec);
   return UI64_MAX;

   } /* End jmuwje() */
