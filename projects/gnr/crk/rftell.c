/* (c) Copyright 1997-2008, The Rockefeller University *11115* */
/* $Id: rftell.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               rftell                                 *
*                                                                      *
*     This routine returns the current position of a specified file,   *
*  which can be used as a "bookmark".  If the value returned is used   *
*  as an 'offset' argument to rfseek() in SEEKABS mode, the file will  *
*  be returned to its present reading position.                        *
*                                                                      *
*     This routine trivially returns fd->aoff, but it provides a place *
*  where any future complexities can be implemented transparently.     *
*                                                                      *
*  INPUT:  Ptr to RFdef struct.                                        *
*  OUTPUT: Absolute offset in file of current reading position.        *
*                                                                      *
************************************************************************
*  V1A, 11/26/97, GNR - Newly written                                  *
*  Rev, 09/25/08, GNR - Minor type change for 64-bit compilation       *
*  ==>, 09/26/08, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "rfdef.h"

size_t rftell(struct RFdef *fd) {

   return fd->aoff;

   } /* End rftell() */
