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

/* (c) Copyright 2000, George N. Reeke, Jr. */
/***********************************************************************
*                             memthdrs.h                               *
*                                                                      *
*  This is the nxdr2 -f headers list file for the test program for the *
*  new shared memory allocation library.                               *
*                                                                      *
*  V1A, 03/05/00, GNR - New program                                    *
*  Rev, 07/03/16, GNR - Change nsitools to mpitools                    *
***********************************************************************/

#define NEWPOOLS
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rocks.h"
#include "mpitools.h"
#include "memshare.h"
#include "memtest.h"


long NXDRTT[] = {
   sizeof(struct mts1),              /* str_mts1 */
   ( 8 << 8 ) | 's',
   ( 1 << 8 ) | 'i',
   ( 1 << 8 ) | 'd',
   ( 2 << 8 ) | 'p',
   ( 1 << 8 ) | 'd',
   ( 1 << 8 ) | 'C',
   sizeof(struct mts2),              /* str_mts2 */
   ( 8 << 8 ) | 's',
   ( 1 << 8 ) | 'c',
   ( 1 << 8 ) | 'l',
   ( 1 << 8 ) | 'p',
   ( 1 << 8 ) | 'C',
   41,                               /* uni_mtu3_1 */
   ( 1 << 8 ) | 'X',
   ( 0 ),
   25,                               /* uni_mtu3_2 */
   ( 1 << 8 ) | 'X',
   ( 7 ),
   40,                               /* uni_mtu3_3 */
   ( 5 << 8 ) | 'L',
   sizeof(union mtu3),               /* uni_mtu3 */
   ( 1 << 8 ) | 'J',
   ( 0 << 8) | 8,
   sizeof(mttsu),                    /* mttsu */
   ( 8 << 8 ) | 's',
   ( 1 << 8 ) | 'i',
   ( 1 << 8 ) | 'J',
   ( 0 << 8) | 8,
   };

#ifdef PAR
   extern unicvtf NXFuni_mtu3_u;

unicvtf *NXDRUT[] = {
   NXFuni_mtu3_u,
   };
#else
unicvtf *NXDRUT[] = { NULL };
#endif

