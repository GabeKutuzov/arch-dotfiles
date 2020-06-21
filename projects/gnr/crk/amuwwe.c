/* (c) Copyright 2012, The Rockefeller University *11115* */
/* $Id: amuwwe.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               amuwwe                                 *
*                                                                      *
*  This function multiplies two unsigned 64-bit integers and adds the  *
*  unshifted product into a 64-bit accumulator with full overflow      *
*  checking.                                                           *
*                                                                      *
*  Synopsis: ui64 amuwwe(ui64 sum, ui64 x, ui64 y, int ec)             *
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
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

#undef amuwwe  /* Get rid of the macro */

ui64 amuwwe(ui64 sum, ui64 x, ui64 y, int ec) {

   return jauwe(sum, jmuwwe(x,y,ec), ec);

   } /* End amuwwe() */
