/* (c) Copyright 2017, The Rockefeller University *11115* */
/* $Id: jsrruj.c 67 2018-05-07 22:08:53Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               jsrruj                                 *
*                                                                      *
*  This function scales a 32-bit unsigned integer by a given amount    *
*  to the right with rounding.  Overflow is not possible, so not       *
*  checked.                                                            *
*                                                                      *
*  Synopsis: ui32 jsrruj(ui32 x, int s)                                *
*                                                                      *
*  Routine assumes without checking 0 < s < 32.                        *
*                                                                      *
*  Algorithm:  This program uses a shift of (s-1) followed by addition *
*  of the rounding bit followed by the final single shift.             *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.  Specifically in this case, access to a hardware  *
*  overflow flag would greatly simplify the logic.                     *
*                                                                      *
************************************************************************
*  V1A, 10/30/17, GNR - New routine, based on jsrruw                   *
*  ==>, 10/30/17, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stdlib.h>
#include "sysdef.h"
#include "rkarith.h"

ui32 jsrruj(ui32 x, int s) {

   if (s <= 0) abexit(74);    /* Rare, no message */

   x >>= (s-1);
   return ((x + 1) >> 1);

   } /* End jsrruj() */
