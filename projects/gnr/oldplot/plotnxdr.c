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

/* (c) Copyright 2016, The Rockefeller University *11114* */
/* $Id: plothdrs.c 30 2017-01-16 19:30:14Z  $ */
/***********************************************************************
*                             plothdrs.h                               *
*                                                                      *
*  This is the nxdr2 -f headers list file for the old plot library.    *
*                                                                      *
************************************************************************
*  V1A, 07/20/16, GNR - New program                                    *
*  ==>, 07/20/16, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rksubs.h"
#include "glu.h"
#include "mpitools.h"
#include "memshare.h"
#include "swap.h"

long OPLTTT[] = {
   sizeof(struct RKGSdef),           /* str_RKGSdef */
   ( 4 << 8 ) | 's',
   ( 1 << 8 ) | 'f',
   ( 4 << 8 ) | 'u',
   ( 2 << 8 ) | 'H',
   };

#ifdef PAR
unicvtf *OPLTUT[] = {
   NULL
   };
#else
unicvtf *OPLTUT[] = { NULL };
#endif

