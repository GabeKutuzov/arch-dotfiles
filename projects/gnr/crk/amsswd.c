/* (c) Copyright 2013, The Rockefeller University *11115* */
/* $Id: amsswd.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               amsswd                                 *
*                                                                      *
*  This function multiplies two signed integers, shifts the result,    *
*  and adds the shifted product into a 64-bit accumulator with full    *
*  overflow checking--see amssw for a version with no error checking.  *
*  This is the version that uses the error code stored by e64dec().    *
*                                                                      *
*  Synopsis: si64 amsswd(si64 sum, si32 x, si32 y, int s)              *
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

#undef amsswd  /* Get rid of the macro */

si64 amsswd(si64 sum, si32 x, si32 y, int s) {

   return jaswd(sum, jsswd(jmsw(x,y), s));

   } /* End amsswd() */
