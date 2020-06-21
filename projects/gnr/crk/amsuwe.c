/* (c) Copyright 1999-2008, The Rockefeller University *11115* */
/* $Id: amsuwe.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               amsuwe                                 *
*                                                                      *
*  This function multiplies two unsigned integers, shifts the result,  *
*  and adds the shifted product into a 64-bit accumulator with full    *
*  overflow checking--see amsuw for a version with no error checking.  *
*                                                                      *
*  Synopsis: ui64 amsuwe(ui64 sum, ui32 x, ui32 y, int s, int ec)      *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.                                                   *
*                                                                      *
************************************************************************
*  V1A, 06/19/99, GNR - New routine                                    *
*  ==>, 10/02/08, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

#undef amsuwe  /* Get rid of the macro */

ui64 amsuwe(ui64 sum, ui32 x, ui32 y, int s, int ec) {

   return jauwe(sum, jsuwe(jmuw(x,y),s,ec), ec);

   } /* End amsuwe() */
