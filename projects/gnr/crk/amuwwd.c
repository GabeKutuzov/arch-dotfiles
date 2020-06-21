/* (c) Copyright 2013, The Rockefeller University *11115* */
/* $Id: amuwwd.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               amuwwd                                 *
*                                                                      *
*  This function multiplies two unsigned 64-bit integers and adds the  *
*  unshifted product into a 64-bit accumulator with full overflow      *
*  checking.                                                           *
*  This is the version that uses the error code stored by e64dec().    *
*                                                                      *
*  Synopsis: ui64 amuwwd(ui64 sum, ui64 x, ui64 y)                     *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.                                                   *
*                                                                      *
************************************************************************
*  V1A, 10/21/13, GNR - New routine                                    *
*  ==>, 10/21/13, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

#undef amuwwd  /* Get rid of the macro */

ui64 amuwwd(ui64 sum, ui64 x, ui64 y) {

   return jauwd(sum, jmuwwd(x,y));

   } /* End amuwwd() */
