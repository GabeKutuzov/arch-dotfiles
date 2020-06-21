/* (c) Copyright 2001-2008, The Rockefeller University *11115* */
/* $Id: scantxt.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                              scantxt.c                               *
*                                                                      *
*  This routine scans a text string using scanck, saves the string in  *
*  the cache, and returns a text locator that can be used to retrieve  *
*  the text later.  The maximum text length here is CDSIZE.            *
*                                                                      *
*  Syntax: int scantxt(int scflags, int badpn)                         *
*                                                                      *
*  Arguments:                                                          *
*     scflags     Flags passed to scanck() call.                       *
*     badpn       Mask indicating bits of RK.scancode that produce     *
*                 an error if set by the scanck() call.                *
*                                                                      *
*  Returns:                                                            *
*     Text locator or zero if no text is found.                        *
*                                                                      *
*  Errors:                                                             *
*     A required-field error is generated if no input field is found.  *
*     A punctuation error is generated if the badpn test fails.        *
*     An abexit is generated if all node 0 storage is exhausted.       *
*                                                                      *
************************************************************************
*  V8B, 08/12/01, GNR - New family of routines                         *
*  ==>, 04/14/02, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"

int scantxt(int scflags, int badpn) {

   int oldlen = scanlen(CDSIZE);
   char lea[CDSIZE+1];

   scanck(lea, scflags, badpn);
   scanlen(oldlen);

   if (RK.scancode == RK_ENDCARD) {
      ermark(RK_REQFLD);
      return 0;
      }
   else
      return savetxt(lea);

   } /* End scantxt() */
