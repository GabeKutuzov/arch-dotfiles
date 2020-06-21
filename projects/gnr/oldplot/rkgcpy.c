/* (c) Copyright 2011-2013, The Rockefeller University *11114* */
/* $Id: rkgcpy.c 30 2017-01-16 19:30:14Z  $ */
/***********************************************************************
*                              rkgcpy()                                *
*                                                                      *
*  This routine is part of the NSI plot library (pic versions only)    *
*  for use in a dynamically loaded environment (usually Matlab mex     *
*  files).  It is one of a set of four routines that allow one to      *
*  preserve the _RKG common block and storage it points to across      *
*  calls when new copies of the library routines, and hence of the     *
*  statically linked _RKG block, are loaded on each call.              *
*                                                                      *
*  The four routines are:                                              *
*     rkgcpy   Make a copy of the _RKG block in persistent (mallocv)   *
*              storage and return a pointer to it.  This should be     *
*              called after the initialization call to setmf().        *
*              Only the pointer returned by this routine needs to be   *
*              kept by the parent code across subsequent mex calls     *
*              for use in calls to the other three routines.           *
*     rkgget   Copy from the block obtained by rkgcpy() to the static  *
*              copy currently in memory.  This should be used at the   *
*              start of any dynamically loaded routine that will       *
*              use NSI plot calls.                                     *
*     rkgput   Copy from the static copy in memory to the persistent   *
*              copy obtained by rkgcpy() .  This should be used after  *
*              a series of plot calls to preserve the current state    *
*              of the plot package.                                    *
*     rkgcls   Release the memory obtained by rkgcpy().  Possibly      *
*              useful in an atexit() routine.                          *
*                                                                      *
*  SYNOPSIS:                                                           *
*     RKGHOLD *rkgcpy(void)                                            *
*                                                                      *
*  DESCRIPTION:                                                        *
*     See above.                                                       *
*                                                                      *
*  RETURN VALUE:                                                       *
*     Pointer to the persistent copy of _RKG.  This is declared as     *
*     a pointer to an incomplete RKGHOLD type because the caller need  *
*     not know anything about the contents that are pointed to.  The   *
*     caller only needs to save this value for later calls to the      *
*     other three routines.                                            *
************************************************************************
*  V1A, 10/13/11, GNR - New routine                                    *
*  ==>, 10/13/11, GNR - Last mod before committing to svn repository   *
*  Rev, 03/24/13, GNR - Add include stddef.h to get size_t for rksubs  *
***********************************************************************/

#include <stddef.h>
#include <ctype.h>
#include "sysdef.h"
#include "rksubs.h"
#include "plots.h"
#include "glu.h"

RKGHOLD *rkgcpy(void) {

   struct RKGdef *prkgh = (struct RKGdef *)mallocv(
      sizeof(struct RKGdef), "RKGHOLD");
   *prkgh = _RKG;
   return (RKGHOLD *)prkgh;

   } /* End rkgcpy() */
