/* (c) Copyright 1998-2016, The Rockefeller University *11116* */
/* $Id: memshare.h 59 2017-01-13 20:34:16Z  $ */
/***********************************************************************
*                             memshare.h                               *
*  Prototypes and definitions for users of NSI shared memory library.  *
*  (Internal definitions not exposed to users are in mempools.h)       *
*                                                                      *
*  N.B.  In serial machines, memshare routines are equivalent to the   *
*  traditional C library routines except they accumulate pool sizes.   *
*  On comp nodes, only membcst() should be used--when called, it gets  *
*  a fresh copy of just those pools selected by its argument.          *
*                                                                      *
*  V1A, 06/27/98, GNR - New header file                                *
*  V1B, 03/04/00, GNR - Use same calls for serial and parallel systems *
*  V1C, 03/25/00, GNR - Use negative mbtype for structured data        *
*  V1F, 05/27/01, GNR - Add chngpool() routine                         *
*  ==>, 07/01/02, GNR - Last date before committing to svn repository  *
*  Rev, 02/18/16, GNR - Change alloc codes from longs to si32          *
*  Rev, 05/23/16, GNR - ifdefined(ZSIZE) a few changes for mpitools    *
***********************************************************************/

#ifndef __MEMSHARE__
#define __MEMSHARE__

/* Definitions of the five memory pools:
*  N.B.  Static must be 0 for some program logic and also
*  so membcst() will return pointer to root data area.  */
#define Npools   5         /* Number of defined memory pools */
#define Nshpools 4         /* Number of pools that are shared */
typedef enum mempool {
   Static=0, Dynamic, Shared, Private, Host } memtype;

#ifdef ZSIZE
typedef int mempoolspec;   /* For membcst, memsize calls */
#else
typedef long mempoolspec;  /* For membcst, memsize calls */
#endif
#define MPS_Static   (1<<(int)Static)
#define MPS_Dynamic  (1<<(int)Dynamic)
#define MPS_Shared   (1<<(int)Shared)
#define MPS_Private  (1<<(int)Private)
#define MPS_Host     (1<<(int)Host)
#define MPS_AllPools ((1<<Npools)-1)

/* Bit set in mbtype to indicate uninitialized data */
#define MBT_Unint 0x40000000
#define MBTMask   0x3fffffff /* Mask for isolating mbtype */

/* Prototypes for all nodes */
#ifdef USE_MPI
void blkbcst(void *msg, long *nxtbl, size_t n, int type);
size_t memsize(mempoolspec pools);
#else
void blkbcst(void *msg, long *mbtype, long n, int type);
long memsize(mempoolspec pools);
#endif
void *membcst(mempoolspec pools);
void *xlatptr(void *p);
void *xlatptrx(void *p);
#define XLAT_NOLKUP 1   /* Value returned if lookup fails */

/* Prototypes for memory management routines--host node */
#ifndef PARn
void *allocpm(memtype kmem, si32 mbtype);
void *allocpc(memtype kmem, size_t n, si32 mbtype);
void *allocpr(void *ptr, size_t n, si32 mbtype);
void *allocpmv(memtype kmem, si32 mbtype, char *emsg);
void *allocpcv(memtype kmem, size_t n, si32 mbtype, char *emsg);
void *allocprv(void *ptr, size_t n, si32 mbtype, char *emsg);
void chngpool(void *ptr, memtype newkmem);
void freep(void *ptr);
void freepv(void *ptr, char *emsg);
memtype whatpool(void *ptr);
#endif

#ifdef MAIN
#include "mempools.h"
#endif

#endif /* __MEMSHARE__ */
