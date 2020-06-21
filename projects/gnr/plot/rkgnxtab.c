/* This file is generated automatically by nxdr2.  It
*  contains data conversion tables for interprocessor
*  messaging.  Do not edit by hand!  */

/* When compiled with -DPAR, the application must
*  provide a definition of unicvtf, the interface
*  to union conversion routines, in some header
*  file that is included in the -f input file. */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

/* (c) Copyright 2015-2016, The Rockefeller University *11115* */
/* $Id: mfhdrs.c 51 2012-05-24 20:53:36Z  $ */
/***********************************************************************
*                     ROCKS metafile plot library                      *
*                              mfhdrs.c                                *
*                                                                      *
*  This file contains a list of all the header files that must         *
*  be read by nxdr2 to make broadcast conversion tables for the        *
*  graphics package.  They must be ordered such that symbols are       *
*  defined before use.  (Does not need to include files #included      *
*  from other files.)                                                  *
************************************************************************
*  V2A, 10/03/15, GNR - New file                                       *
*  ==>, 08/27/16, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stddef.h>
#include "sysdef.h"
#include "mfint.h"
#include "mpitools.h"
#include "memshare.h"
#include "swap.h"

long RKGTT[] = {
   sizeof(struct RKGSdef),           /* str_RKGSdef */
   ( 4 << 8 ) | 's',
   ( 2 << 8 ) | 'f',
   ( 2 << 8 ) | 'u',
   ( 1 << 8 ) | 'i',
   ( 4 << 8 ) | 'u',
   ( 1 << 8 ) | 'm',
   ( 5 << 8 ) | 'h',
   ( 1 << 8 ) | 'i',
   ( 15 << 8 ) | 'H',
   };

#ifdef PAR
unicvtf *RKGUT[] = {
   NULL
   };
#else
unicvtf *RKGUT[] = { NULL };
#endif

