/* (c) Copyright 1997-2008, The Rockefeller University *11116* */
/* $Id: reallocv.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                              REALLOCV                                *
*                           Verbose realloc                            *
*                                                                      *
*  prototype:  void *reallocv(void *ptr, size_t length, char *msg)     *
*                                                                      *
*  input:      ptr    = location of existing storage block, which      *
*                       must have been previously allocated by malloc, *
*                       calloc, etc., or NULL                          *
*              length = number of bytes of storage to allocate.  This  *
*                       value may be larger or smaller than the ori-   *
*                       ginal size of the block.                       *
*              msg    = string to incorporate in message that is       *
*                       written if reallocation fails (max 48 chars).  *
*                                                                      *
*  returns:    Pointer to a contiguous block of memory of requested    *
*              size, rounded up to nearest ROUNDSIZE byte block.  If   *
*              the block is in a different location than that pointed  *
*              to by ptr, then the old contents of the block will      *
*              have been copied to the new location.  If reallocation  *
*              fails and 'msg' is a NULL pointer, a NULL pointer is    *
*              returned as with standard realloc().  Otherwise, a      *
*              message incorporating the string 'msg' is printed and   *
*              the calling program is terminated via abexitm with      *
*              abexit code 32.  The 'msg' string should indicate what  *
*              was being reallocated.                                  *
*                                                                      *
*              Another use of this routine is to permit interception   *
*              of realloc calls in environments where the C library    *
*              realloc cannot be used (e.g. MATLAB mex routines).  In  *
*              these situations, a user-written reallocv can be used   *
*              in place of this one.  To facilitate this mechanism,    *
*              all ROCKS routines that are intended for use in such    *
*              environments call reallocv() instead of realloc().      *
*                                                                      *
*  N.B.:       Do not construct pointers either inside or outside the  *
*              allocated block that point to locations in the block,   *
*              or be prepared to update all such pointers when reallo- *
*              cation to a new location occurs.                        *
*                                                                      *
*  N.B.:       In parallel programs, be sure to provide a user-written *
*              version of abexitm that passes the error message to     *
*              the host node for printing.                             *
*                                                                      *
************************************************************************
*  V1A, 09/10/97, GNR - New routine                                    *
*  Rev, 11/05/97, GNR - Eliminate cryout call, use abexitm()           *
*  Rev, 09/19/08, GNR - Use %10ld in ssprintf call for error message   *
*  ==>, 09/19/08, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"

#define MXMEMMSG 48        /* Max size of msg arg */

void *reallocv(void *ptr, size_t length, char *msg) {

   void *newstorage;

   /* Deal with the fact that standard UNIX realloc is not
   *  guaranteed to work if the pointer passed to it is NULL.  */
   if ((newstorage = ptr ? (void *)realloc(ptr, length) :
         (void *)malloc(length)) == NULL && msg)
      abexitm(32, ssprintf(NULL, "Memory realloc failed for %48s "
         "(%10ld bytes requested).", msg, (long)length));
   return newstorage;
   } /* End reallocv */
