/* (c) Copyright 1988-2008, The Rockefeller University *11116* */
/* $Id: mallocv.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               MALLOCV                                *
*                           Verbose malloc                             *
*                                                                      *
*  prototype:  void *mallocv(size_t length, char *msg)                 *
*                                                                      *
*  input:      length = number of bytes of storage to allocate.        *
*              msg    = string to incorporate in message that is       *
*                       written if allocation fails (max 48 chars).    *
*                                                                      *
*  returns:    This routine calls malloc and returns pointer returned  *
*              to it by malloc.  If allocation fails and 'msg' is a    *
*              NULL pointer, a NULL pointer is returned to the user    *
*              as with standard malloc().  Otherwise, a message        *
*              incorporating the string 'msg' is printed and the       *
*              calling program is terminated via abexitm with abexit   *
*              code 32.  The 'msg' string should indicate what was     *
*              being allocated.                                        *
*                                                                      *
*              Another use of this routine is to permit interception   *
*              of malloc calls in environments where the C library     *
*              malloc cannot be used (e.g. MATLAB mex routines).  In   *
*              these situations, a user-written mallocv can be used    *
*              in place of this one.  To facilitate this mechanism,    *
*              all ROCKS routines that are intended for use in such    *
*              environments call mallocv() instead of malloc().        *
*                                                                      *
*  N.B.:       In parallel programs, be sure to provide a user-written *
*              version of abexitm that passes the error message to     *
*              the host node for printing.                             *
*                                                                      *
************************************************************************
*  V1A, 06/27/88, GNR - New program                                    *
*  Rev, 03/28/92, GNR - Delete useless size_t check, edit comments.    *
*  Rev, 11/05/97, GNR - Eliminate cryout call, use abexitm()           *
*  Rev, 09/19/08, GNR - Use %10ld in ssprintf call for error message   *
*  ==>, 09/19/08, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"

#define MXMEMMSG 48        /* Max size of msg arg */

void *mallocv(size_t length, char *msg) {

   void *newstorage;

   if ((newstorage=(void *)malloc(length)) == NULL && msg)
      abexitm(32, ssprintf(NULL, "Memory alloc failed for %48s "
         "(%10ld bytes requested).", msg, (long)length));
   return newstorage;
   } /* End mallocv */
