/* (c) Copyright 2010, The Rockefeller University *11115* */
/* $Id: jmule.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                jmule                                 *
*                                                                      *
*  This function multiplies two unsigned 32 bit integers and returns   *
*  the unsigned 32-bit product with full overflow checking.            *
*                                                                      *
*  Synopsis: ui32 jmule(ui32 x, ui32 y, int ec)                        *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.                                                   *
*                                                                      *
************************************************************************
*  V1A, 10/02/10, GNR - New routine                                    *
*  ==>, 10/02/10, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

ui32 jmule(ui32 x, ui32 y, int ec) {

   ui64 rv = jmuw(x,y);

   /* Check for overflow.  If e64act returns,
   *  set result to max value.  */
   if (uwhi(rv)) {
      e64act("jmule", ec);
      return UI32_MAX;
      }
   return uwlo(rv);

   } /* End jmule() */
