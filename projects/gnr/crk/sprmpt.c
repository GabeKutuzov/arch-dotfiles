/* (c) Copyright 1994-2008, The Rockefeller University *11115* */
/* $Id: sprmpt.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               SPRMPT                                 *
*                                                                      *
*  Sets the on-line prompt used by cryin() to the argument string.     *
*  (CR-LF is prepended to the specified prompt internally.)            *
*                                                                      *
************************************************************************
*  V1A, 01/31/94,  LC - Initial version                                *
*  V1B, 08/29/98, GNR - Better length checking, no accpt switching     *
*  ==>, 07/01/02, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rocks.h"
#include "rockv.h"

void sprmpt(char *prompt) {

/* Store prompt string in RKC structure */

   if (prompt) {
      strncpy(RKC.prompt+2, prompt, L_PROMPT);
      RKC.plen = (short)strnlen(prompt, L_PROMPT) + 2;
      }
   else {
      RKC.plen = 0;
      }

   } /* End sprmpt() */
