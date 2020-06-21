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

/* (c) Copyright 2016, The Rockefeller University */
/***********************************************************************
*                             copthdrs.h                               *
*                                                                      *
*  This is the nxdr2 -f headers list file for the test program for the *
*  coppack statistics collecting library package.                      *
*                                                                      *
*  V1A, 08/31/16, GNR - New program                                    *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "rkarith.h"
#include "mpitools.h"
#include "copptest.h"


long CPTTT[] = {
   sizeof(struct ctst1),             /* str_ctst1 */
   ( 8 << 8 ) | 's',
   ( 1000 << 8 ) | 'd',
   ( 1200 << 8 ) | 'U',
   sizeof(struct ctst2),             /* str_ctst2 */
   ( 8 << 8 ) | 's',
   ( 801 << 8 ) | 'f',
   ( 1200 << 8 ) | 'w',
   ( 1200 << 8 ) | 'L',
   sizeof(struct bcstptrs),          /* str_bcstptrs */
   ( 2 << 8 ) | 'P',
   };

#ifdef PAR
unicvtf *CPTUT[] = {
   NULL
   };
#else
unicvtf *CPTUT[] = { NULL };
#endif

