/* (c) Copyright 1999-2008, The Rockefeller University *11115* */
/* $Id: amsuw.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                amsuw                                 *
*                                                                      *
*  This function multiplies two unsigned integers, shifts the result,  *
*  and adds the shifted product into a 64-bit accumulator.  There is   *
*  no error checking--see amsuwe for version with full error checking. *
*                                                                      *
*  Synopsis: ui64 amsuw(ui64 sum, ui32 x, ui32 y, int s)               *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.                                                   *
*                                                                      *
************************************************************************
*  V1A, 07/03/99, GNR - New routine                                    *
*  ==>, 03/12/06, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

#undef amsuw   /* Get rid of the macro */

ui64 amsuw(ui64 sum, ui32 x, ui32 y, int s) {

   return jauw(sum, jsuw(jmuw(x,y),s));

   } /* End amsuw() */
