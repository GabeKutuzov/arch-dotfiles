/* (c) Copyright 1989-2008, The Rockefeller University *11115* */
/* $Id: callocv.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                      CALLOCV - Verbose calloc                        *
*                                                                      *
*  prototype:  void *callocv(size_t n,size_t size,char * msg)          *
*                                                                      *
*  input:      n     = number of objects to allocate.                  *
*              size  = number of bytes in each object.                 *
*              msg   = string to incorporate in message that is        *
*                      written if allocation fails (max 48 chars).     *
*                                                                      *
*  returns:    This routine calls calloc and returns the pointer       *
*              returned to it by calloc.  The allocated memory is      *
*              cleared by calloc.  If allocation fails and 'msg' is    *
*              a NULL pointer, a NULL pointer is returned to the user  *
*              as with standard calloc().  Otherwise, a message        *
*              incorporating the string 'msg' is printed and the       *
*              calling program is terminated via abexitm with abexit   *
*              code 32.  The 'msg' string should indicate what was     *
*              being allocated.                                        *
*                                                                      *
*              Another use of this routine is to permit interception   *
*              of calloc calls in environments where the C library     *
*              calloc cannot be used (e.g. MATLAB mex routines).  In   *
*              these situations, a user-written callocv can be used    *
*              in place of this one.  To facilitate this mechanism,    *
*              all ROCKS routines that are intended for use in such    *
*              environments call callocv() instead of calloc().        *
*                                                                      *
*  N.B.:       In parallel programs, be sure to provide a user-written *
*              version of abexitm that passes the error message to     *
*              the host node for printing.                             *
*                                                                      *
************************************************************************
*  Rev, 03/28/92, GNR - Edit comments, delete useless size_t check.    *
*  Rev, 11/05/97, GNR - Eliminate cryout call, use abexitm()           *
*  Rev, 09/19/08, GNR - Use %10ld in ssprintf call for error message   *
*  ==>, 09/19/08, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"

#define MXMEMMSG 48        /* Max size of msg arg */

void *callocv(size_t n, size_t size, char *msg) {

   void *newstorage;

   if ((newstorage=(void *)calloc(n, size)) == NULL && msg)
      abexitm(32, ssprintf(NULL, "Memory alloc failed for %48s "
         "(%10ld bytes requested).", msg, (long)(n*size)));
   return newstorage;

   } /* End callocv */
