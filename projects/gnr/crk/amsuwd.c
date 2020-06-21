/* (c) Copyright 2013, The Rockefeller University *11115* */
/* $Id: amsuwd.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               amsuwd                                 *
*                                                                      *
*  This function multiplies two unsigned integers, shifts the result,  *
*  and adds the shifted product into a 64-bit accumulator with full    *
*  overflow checking--see amsuw for a version with no error checking.  *
*  This is the version that uses the error code stored by e64dec().    *
*                                                                      *
*  Synopsis: ui64 amsuwd(ui64 sum, ui32 x, ui32 y, int s)              *
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

#undef amsuwd  /* Get rid of the macro */

ui64 amsuwd(ui64 sum, ui32 x, ui32 y, int s) {

   return jauwd(sum, jsuwd(jmuw(x,y), s));

   } /* End amsuwd() */
