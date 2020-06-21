/* (c) Copyright 2010, The Rockefeller University *11115* */
/* $Id: jmsle.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                jmsle                                 *
*                                                                      *
*  This function multiplies two signed 32 bit integers and returns     *
*  the signed 32-bit product with full overflow checking.              *
*                                                                      *
*  Synopsis: si32 jmsle(si32 x, si32 y, int ec)                        *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.                                                   *
*                                                                      *
************************************************************************
*  V1A, 10/01/10, GNR - New routine                                    *
*  ==>, 10/01/10, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stdlib.h>
#include "sysdef.h"
#include "rkarith.h"

si32 jmsle(si32 x, si32 y, int ec) {

   si64 rv = jmsw(x,y);

   /* Check for overflow.  If e64act returns,
   *  set result to max value of correct sign.  */
   if (qsw(jsrsw(abs64(rv),31))) {
      e64act("jmsle", ec);
      return swlo(jsrsw(rv,31)) ^ SI32_MAX;
      }
   return swlo(rv);

   } /* End jmsle() */
