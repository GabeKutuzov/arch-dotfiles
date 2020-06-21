/* (c) Copyright 1998-2009, The Rockefeller University *11115* */
/* $Id: freep.c 3 2017-01-16 19:29:37Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                                freep                                 *
*                                                                      *
*  This routine is the free() analog in a shared memory management     *
*  package for parallel computers.  It should not be called on comp    *
*  nodes or to free memory in the Static pool.  It should be used      *
*  on serial computers to free memory allocated with this package.     *
*                                                                      *
*  Synopsis:                                                           *
*     void freep(void *ptr)                                            *
*                                                                      *
*  Arguments:                                                          *
*     ptr      pointer to data object to be released.                  *
*                                                                      *
*  Return value:                                                       *
*     none     The program is terminated if an attempt is made to      *
*              free memory that was not allocated with the shared      *
*              memory package.  Generally freepv() is used to get      *
*              a more informative error message in this situation.     *
*                                                                      *
************************************************************************
*  V1A, 06/27/98, GNR - Newly written                                  *
*  V1B, 03/04/00, GNR - Add HostString, use same calls for serial and  *
*                       parallel systems.                              *
*  V1D, 06/10/00, GNR - Call freepv() with null emsg, allowing fancier *
*                       error checking with signal handler in freepv().*
*  ==>, 04/23/16, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "memshare.h"
#include "mempools.h"

void freep(void *ptr) {

#ifdef PARn

   abexit(MP_COMP_ALLOC);     /* Don't call on comp nodes */

#else

/* Call freepv() with a NULL error message string */

   freepv(ptr, NULL);

#endif

   } /* End freep() */
