/* (c) Copyright 1999-2008, The Rockefeller University *11115* */
/* $Id: amssw.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                amssw                                 *
*                                                                      *
*  This function multiplies two signed integers, shifts the result,    *
*  and adds the shifted product into a 64-bit accumulator.  There is   *
*  no error checking--see amsswe for version with full error checking. *
*                                                                      *
*  Synopsis: si64 amssw(si64 sum, si32 x, si32 y, int s)               *
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

#undef amssw   /* Get rid of the macro */

si64 amssw(si64 sum, si32 x, si32 y, int s) {

   return jasw(sum, jssw(jmsw(x,y),s));

   } /* End amssw() */
