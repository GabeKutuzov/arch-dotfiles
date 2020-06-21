/* (c) Copyright 2016, The Rockefeller University *11115* */
/* $Id: jmswj.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                jmswj                                 *
*                                                                      *
*  This function multiplies a 64 bit signed integer by a 32-bit        *
*  signed integer and returns the signed 64-bit product with no        *
*  overflow checking.  This routine should be used only when there     *
*  is no possibility of overflow.                                      *
*                                                                      *
*  Synopsis: si64 jmswj(si64 x, si32 y)                                *
*                                                                      *
*  This routine is to be compiled only on systems that do not have     *
*  64-bit arithmetic -- implemented as a macro if HAS_I64              *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.                                                   *
*                                                                      *
************************************************************************
*  R67, 12/02/16, GNR - New routine                                    *
*  ==>, 12/02/16, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

#ifdef HAS_I64
#error Routine jmswj should not be compiled if system HAS_I64
#else

si64 jmswj(si64 x, si32 y) {

   union {
      ui64 uxty;
      si64 sxty;
      } uxy;
   ui64 xylo;
   ui32 axhi = (ui32)abs32(swhi(x));
   ui32 ay = (ui32)abs32(y);

   uxy.uxty = jsluw(jmuw(axhi,ay), BITSPERUI32);
   xylo = jmuwj(jeul(swlou(x)),ay);
   uxy.uxty = jauw(uxy.uxty, xylo);
   if (swhi(x) * y < 0) uxy.sxty = jnsw(uxy.sxty);
   return uxy.sxty;

   } /* End jmswj() */

#endif /* !HAS_I64 */
