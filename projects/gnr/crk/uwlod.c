/* (c) Copyright 2013, The Rockefeller University *11115* */
/* $Id: uwlod.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                uwlod                                 *
*                                                                      *
*  This function extracts the low order part of an unsigned 64-bit     *
*  value as an unsigned 32-bit value and signals overflow.             *
*  This is the version that uses the error code stored by e64dec().    *
*                                                                      *
*  Synopsis: ui32 uwlod(ui64 x)                                        *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.                                                   *
*                                                                      *
************************************************************************
*  V1A, 10/23/13, GNR - New routine                                    *
*  ==>, 10/23/13, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

ui32 uwlod(ui64 x) {

   if (jckulo(x)) { e64dac("uwlod"); return UI32_MAX; }
   return uwlo(x);

   } /* End uwlod() */
