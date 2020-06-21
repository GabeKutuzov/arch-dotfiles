/* (c) Copyright 2000-2016, The Rockefeller University *11115* */
/* $Id: rkilst.h 59 2017-01-13 20:34:16Z  $ */
/***********************************************************************
*                           ROCKS C LIBRARY                            *
*                        rkilst.h header file                          *
*                                                                      *
************************************************************************
*  V1A, 03/18/00, GNR - New header file                                *
*  V1B, 09/15/01, GNR - Add ilstitct()                                 *
*  V1D, 04/13/02, GNR - Add IL_OFFFLG                                  *
*  ==>, 03/22/08, GNR - Last date before committing to svn repository  *
*  V2A, 11/16/08, GNR - Revise for 64-bit and C++ operation            *
*  Rev, 02/17/16, GNR - Convert seed to si32 in calls and structs      *
***********************************************************************/

#ifndef RKILST_HDR_INCLUDED
#define RKILST_HDR_INCLUDED

#include "sysdef.h"

typedef unsigned long ilstitem;  /* Define an iteration list item */

typedef struct ILST {            /* Structure of an iteration list */
   ilstitem *pidl;               /* Ptr to iteration data list */
   size_t   nallo;               /* Number of words allocated */
   long     nusm1;               /* Number of items in list - 1 */
   long     evry;                /* EVERY block repeat interval
                                 *  plus (ibase << IL_BITS) */
   long     frst;                /* Value from FIRST option or
                                 *  IL_ALL for ALL option */
   long     last;                /* Value from LAST option */
   long     rand;                /* Value from RANDOM option */
   si32     seed;                /* Value from SEED option */
   } ilst;

typedef struct ITER {            /* Structure of an iterator */
   struct ILST *pilst;           /* Ptr to controlling ilst */
   long     iidl;                /* Current offset in data list */
   ilstitem inow;                /* Current value of iteration */
   ilstitem incr;                /* Increment for current loop */
   ilstitem iend;                /* End value of current loop */
   ilstitem iadj;                /* Base of current EVERY block */
   } iter;

#define IL_ILSZ 16               /* Initial list size */
#define IL_BITS (BITSPERLONG-2)  /* Bits not used by range codes */
#if LSIZE == 8
#define IL_ALLI 0x4000000000000000L /* Use all items in set */
#define IL_INCR 0x4000000000000000L /* Marker for range increment */
#define IL_REND 0x8000000000000000L /* End range with unit increment */
#define IL_REIN 0xC000000000000000L /* End range with specified incr */
#define IL_MASK 0x3FFFFFFFFFFFFFFFL /* Mask to isolate numeric value */
#else
#define IL_ALLI 0x40000000L      /* Use all items in set */
#define IL_INCR 0x40000000L      /* Marker for range increment */
#define IL_REND 0x80000000L      /* End range with unit increment */
#define IL_REIN 0xC0000000L      /* End range with specified incr */
#define IL_MASK 0x3FFFFFFFL      /* Mask to isolate numeric value */
#endif
#define IL_DFLTSEED      97      /* Default random number seed */
#define IL_OFFFLG         1      /* Value in RK.highrc if list OFF */

/* Prototype routines for managing iteration lists */
#ifdef __cplusplus
extern "C" {
#endif
void  ilstsalf(
   void * (*ilallocv)(size_t nitm, size_t size, char *msg),
   void * (*ilallorv)(void *block, size_t size, char *msg),
   void   (*ilfreev) (void *block, char *msg));
ilst *ilstread(ilst *poldilst, int idop, int ibase, si32 seed);
#define IL_NEW    1              /* Values for idop argument */
#define IL_ADD    2              /* Must match order of key- */
#define IL_DEL    3              /* words in ilstread().     */
#define IL_OFF    4              /* Must be a testable bit.  */
#define IL_ELOK  16              /* Empty list is OK  */
#define IL_ESOK  32              /* End-of-scan is OK */
void  ilstreq (void);
int   ilstchk (ilst *pil, long nmax, char *msg);
long  ilsthigh(ilst *pil);
long  ilstitct(ilst *pil, ilstitem item);
long  ilstsrch(ilst *pil, ilstitem item);
int   ilsttest(ilst *pil, ilstitem item);
void  ilstset (iter *pit, ilst *pil, ilstitem item);
long  ilstiter(iter *pit);
long  ilstnow (iter *pit);
void  freeilst(ilst *pil);
#ifdef __cplusplus
}
#endif

#endif /* ifndef RKILST_HDR_INCLUDED */
