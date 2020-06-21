/* (c) Copyright 2015, The Rockefeller University *11116* */
/* $Id: sort2.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                SORT2                                 *
*                 Perform radix sort on a linked list                  *
*                                                                      *
*  This is a revised and expanded version of CRK sort().  It performs  *
*  radix sorting in increasing or decreasing order on data structures  *
*  organized in a linked list.  Keys may consist of character strings, *
*  fixed- or floating-point, positive or negative numeric data of any  *
*  byte length.                                                        *
*                                                                      *
*  Usage:  void *sort2(void *pdata, void *work, int okeys,             *
*     int lkeys, int ktype)                                            *
*                                                                      *
*  Prototyped in:                                                      *
*     rocks.h, rksubs.h                                                *
*                                                                      *
*  Arguments:                                                          *
*     'pdata' points to a linked list of structures containing         *
*  the data to be sorted.  It is assumed that the first element of     *
*  each structure is a pointer to the next structure, and that the     *
*  list is terminated by a NULL pointer.  The pointers, and only       *
*  the pointers, are modified by sort2.                                *
*                                                                      *
*     'work' is a pointer to a work area large enough to contain       *
*  (2 << BITSPERBYTE) pointers of size PSIZE (defined in sysdef,h).    *
*  The contents are modified during the operation of this routine.     *
*                                                                      *
*     'okeys' is the offset in bytes from the beginning of each        *
*  structure to the beginning of the key.  The remainder of each       *
*  structure may contain data of any length.  Normally, the keys       *
*  are placed immediately after the linked-list pointers and           *
*  okeys = sizeof(void *).  However, note that in a 32-bit system,     *
*  if keys are doubles, padding is inserted after the pointer and      *
*  okeys will be 8, not 4.                                             *
*                                                                      *
*     'lkeys' is the length of the keys, in bytes.                     *
*                                                                      *
*     'ktype' indicates the type of the keys and is the sum of any     *
*  relevant bits from the following list (defined in rocks.h and       *
*  rksubs.h):                                                          *
*  KST_CHAR    1  Keys are character strings, to be sorted from left   *
*                 to right regardless of endian order of the machine.  *
*                 KST_APOS and KST_FLOAT are ignored.  (Default: Keys  *
*                 are numbers, high-order byte first in big-endian,    *
*                 low-order byte first in little-endian machines.)     *
*  KST_APOS    2  All keys are known to be positive numeric values.    *
*                 Sorting is slightly faster than with mixed signs.    *
*                 (Default:  Keys may include negative values.)        *
*  KST_FLOAT   4  Keys are floating-point numbers, that is, negative   *
*                 values are stored as sign and magnitude.  (Default:  *
*                 Negative numbers (KST_APOS bit off) are stored as    *
*                 two complements.)                                    *
*  KST_DECR    8  Sort in decreasing order of key values.              *
*                 (Default:  Sort increasing order.)                   *
*                                                                      *
*  Value returned:  sort2 returns a pointer to the first structure     *
*     in the sorted list.  This pointer typically must be cast to      *
*     the appropriate type.                                            *
*                                                                      *
*  Performance:  Execution time is proportional to lkeys*N, where N is *
*     the number of items to be sorted.  There is additional time pro- *
*     portional to lkeys but not N which may be significant when N is  *
*     very small.  To eliminate sign testing in the inner loop, there  *
*     is separate code for the case that all keys are positive and     *
*     sort is ascending.                                               *
*                                                                      *
************************************************************************
*  V1A, 12/24/15, GNR - New routine, based on sort.c                   *
*  ==>, 12/25/15, GNR - Last date before committing to svn repository  *
***********************************************************************/

/*---------------------------------------------------------------------*
*     Global declarations                                              *
*---------------------------------------------------------------------*/

#define NBINS (1<<BITSPERBYTE)         /* Do not change! */
#define SBIT  (1 << (BITSPERBYTE-1))   /* Sign bit of a byte */
#include <stddef.h>
#include <stdio.h>
#include "sysdef.h"
#include "rksubs.h"

/*---------------------------------------------------------------------*
*                                SORT2                                 *
*---------------------------------------------------------------------*/

void *sort2(void *pdata, void *work, int okeys, int lkeys, int ktype) {

   struct Dat_t {             /* Define (partial) data structure */
      struct Dat_t *pnxt;        /* Ptr to next data structure */
      byte key[1];               /* Data somewhere out here */
      } *ptl,**ppl;
   struct Bin_t {             /* Define sorting bins */
      struct Dat_t *head;        /* Ptr to data at head of sort bin */
      struct Dat_t *tail;        /* Ptr to data at tail of sort bin */
      } *pb,*pb0,*pbe;
   struct Dat_t *pd0;         /* Pointer to start of data chain */
   int i,ii,isz;              /* Indexes for loops over sort digits */
   unsigned int jsgn,jkey;    /* Offsets to sign byte, key byte */
   unsigned int doflip;       /* TRUE if flipping needed */
   unsigned int osgn;         /* Offset of high-order byte in data */
   unsigned int pflip,nflip;  /* Bit flippers for negs */
   unsigned int lipfl,linfl;  /* Last byte fliper flippers */

   /* Exit fast if nothing to do */
   if (!pdata || lkeys <= 0) return pdata;

   pd0 = (struct Dat_t *)pdata;
   pb0 = (struct Bin_t *)work;
   pbe = pb0 + NBINS;
   isz = lkeys - 1;
   okeys -= sizeof(void *);

   /* Set up controls according to ktype */

#if BYTE_ORDRE < 0
   osgn = (ktype & KST_CHAR) ? 0 : isz;
#else
   osgn = 0;
#endif
   jsgn = okeys + osgn;

   doflip = (ktype & KST_DECR) || !(ktype & (KST_CHAR|KST_APOS));
   if (doflip) {
      if (ktype & KST_FLOAT)
         pflip = linfl = 0, nflip = (NBINS-1), lipfl = SBIT;
      else
         pflip = nflip = 0, lipfl = linfl = SBIT;
      if (ktype & KST_DECR)
         pflip ^= (NBINS-1), nflip ^= (NBINS-1);
      }

   /* Loop over data in the radix sort */
   for (ii=0; ii<=isz; ++ii) {
      i = osgn ? ii : isz - ii;
      jkey = okeys + i;

      /* Initialize sorting bins for this key digit */
      for (pb=pb0; pb<pbe; ++pb) {
         pb->head = NULL;     /* Empty bin signal */
         pb->tail = (struct Dat_t *)&pb->head;
         }

      if (doflip) {
         /* Normal data, must deal with negatives */
         if (ii == isz)
            pflip ^= lipfl, nflip ^= linfl;

         for (ptl=pd0; ptl; ptl=ptl->pnxt) {
            unsigned int ibin = (unsigned int)ptl->key[jkey];
            ibin ^= (ptl->key[jsgn] & SBIT) ? nflip : pflip;
            pb = pb0 + ibin;
            pb->tail->pnxt = ptl;
            pb->tail = ptl;
            } /* End loop over data records */
         }

      else {
         /* Data are all postive, omit sign checking.  Traverse
         *  linked list, extracting current key digit and assigning
         *  each record to top of appropriate bin.  */
         for (ptl=pd0; ptl; ptl=ptl->pnxt) {
            unsigned int ibin = (unsigned int)ptl->key[jkey];
            pb = pb0 + ibin;
            pb->tail->pnxt = ptl;
            pb->tail = ptl;
            } /* End loop over data records */
         }

      /* End of pass, stack up the bins in order */
      ppl = &pd0;
      for (pb=pb0; pb<pbe; ++pb) if (pb->head) {
         *ppl = pb->head;
         ppl = &pb->tail->pnxt; }
      *ppl = NULL;            /* Terminate linked list */
      } /* End loop over bytes in data */

   return pd0;
   } /* End sort2 function */
