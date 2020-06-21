/* (c) Copyright 1992-2008, The Rockefeller University *11116* */
/* $Id: freev.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                FREEV                                 *
*                            Verbose free                              *
*                                                                      *
*  prototype:  void freev(void *freeme, char *msg);                    *
*                                                                      *
*  input:      freeme = ptr to memory block to be freed.               *
*              msg    = identifying string to be included in message   *
*                       that is printed upon error (max 48 chars).     *
*                                                                      *
*  output:     Terminates program with abexit code 33 upon error.      *
*              (The standard C library routine does not detect         *
*              erroneous pointers, therefore, in cases where this      *
*              routine is implemented by a call to free(), errors      *
*              will not be detected.)                                  *
*                                                                      *
*              Another use of this routine is to permit interception   *
*              of free() calls in environments where the C library     *
*              free() cannot be used (e.g. MATLAB mex routines).  In   *
*              these situations, a user-written freev() can be used    *
*              in place of this one.  To facilitate this mechanism,    *
*              all ROCKS routines that are intended for use in such    *
*              environments call freev() instead of free().            *
*                                                                      *
*  N.B.:       In parallel programs, be sure to provide a user-written *
*              version of abexitm that passes the error message to     *
*              the host node for printing.                             *
*                                                                      *
*  N.B.:       Unlike free(), this routine will flag the error where   *
*              parameter 'freeme' is not the return value of malloc(), *
*              mallocd, calloc(), callocd, or realloc() on systems     *
*              where this functionality can be supported.              *
*                                                                      *
************************************************************************
*  Rev, 03/28/92, GNR - Broken out from verbs.c, terminate on error.   *
*  Rev, 11/05/97, GNR - Eliminate cryout call, use abexitm()           *
*  Rev, 09/04/99, GNR - Remove NCUBE support                           *
*  ==>, 07/01/02, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rocks.h"

#define MXMEMMSG 48        /* Max size of msg arg */

void freev(void *freeme, char *msg) {

/* Return at once if argument is NULL pointer */

   if (!freeme) return;

/* No error test:  normal C library has no checking facilities */

   free(freeme);

   } /* End freev() */

