/* (c) Copyright 2000 Neurosciences Research Foundation, Inc. */
/* $Id: d3salf.c 3 2008-03-11 19:50:00Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               D3SALF                                 *
*                                                                      *
*  This file provides wrappers for allocpcv, allocprv, and freepv with *
*  the calling arguments of callocv, reallocv, and freev, respectively.*
*  They are for use by the ROCKS ilstsalf routine, to cause iteration  *
*  lists to be stored in Host pool rather than ordinary malloc memory. *
*  Lists are moved from Host to Dynamic memory by chngpool() if needed.*
************************************************************************
*  ==>, 02/17/05, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "d3global.h"

/***********************************************************************
*                              D3ALFPCV                                *
*                 Host Pool Dynamic Verbose calloc()                   *
***********************************************************************/
 
void *d3alfpcv(size_t n, size_t size, char *msg) {

   void * p = (size == sizeof(ilst)) ?
      allocpcv(Host, n, IXilst, msg) :
      allocpcv(Host, n, IXilstitem, msg);
   return p;

   } /* End d3alfpcv() */


/***********************************************************************
*                              D3ALFPRV                                *
*                     Host Pool Verbose realloc()                      *
***********************************************************************/

void *d3alfprv(void *ptr, size_t length, char *msg) {

   return allocprv(ptr, length/sizeof(ilstitem), IXilstitem, msg);

   } /* End d3alfprv() */


/***********************************************************************
*                              D3ALFREE                                *
*                      Host Pool Verbose free()                        *
***********************************************************************/

void d3alfree(void *block, char *msg) {

   freepv(block, msg);

   } /* End d3alfree() */
