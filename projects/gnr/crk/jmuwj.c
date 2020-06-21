/* (c) Copyright 2016, The Rockefeller University *11115* */
/* $Id: jmuwj.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                jmuwj                                 *
*                                                                      *
*  This function multiplies a 64 bit unsigned integer by a 32-bit      *
*  unsigned integer and returns the unsigned 64-bit product with no    *
*  overflow checking.  This routine should be used only when there     *
*  is no possibility of overflow.                                      *
*                                                                      *
*  Synopsis: ui64 jmuwj(ui64 x, ui32 y)                                *
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
*  R60, 05/15/16, GNR - New routine                                    *
*  ==>, 05/15/16, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

#ifdef HAS_I64
#error Routine jmuwj should not be compiled if system HAS_I64
#else

ui64 jmuwj(ui64 x, ui32 y) {

   ui64 xyhi,xylo;
   ui32 xhi = uwhi(x), xlo = uwlo(x);

   xyhi = jsluw(jmuw(xhi,y), BITSPERUI32);
   xylo = jmuw(xlo,y);
   return jauw(xyhi, xylo);

   } /* End jmuwj() */

#endif /* !HAS_I64 */
