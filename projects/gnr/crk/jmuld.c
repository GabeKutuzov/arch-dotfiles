/* (c) Copyright 2013, The Rockefeller University *11115* */
/* $Id: jmuld.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                jmuld                                 *
*                                                                      *
*  This function multiplies two unsigned 32 bit integers and returns   *
*  the unsigned 32-bit product with full overflow checking.            *
*  This is the version that uses the error code stored by e64dec().    *
*                                                                      *
*  Synopsis: ui32 jmuld(ui32 x, ui32 y)                                *
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

ui32 jmuld(ui32 x, ui32 y) {

   ui64 rv = jmuw(x,y);

   /* Check for overflow.  If e64dac returns,
   *  set result to max value.  */
   if (uwhi(rv)) {
      e64dac("jmuld");
      return UI32_MAX;
      }
   return uwlo(rv);

   } /* End jmuld() */
