/* (c) Copyright 2000-2013, The Rockefeller University *11115* */
/* $Id: ilstiter.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                             ilstiter.c                               *
*                                                                      *
*  This collection of routines allows the user to set a starting       *
*  position (which may be different on different nodes in a parallel   *
*  computer) and iterate through the entries in a ROCKS iteration      *
*  list.  The state of the iteration is kept in a separate data        *
*  structure from the list of items so that multiple iterators can     *
*  access the same list.  The list is created by a call to ilstread(), *
*  which guarantees that (1) the list is sorted, (2) ranges do not     *
*  overlap, (3) if there is a range increment, it is stored before     *
*  the range end, and (4) if there is an EVERY parameter, all values   *
*  and ranges are contained in the first batch.                        *
*                                                                      *
*  Warning:  Because the first two bits of each list item may be used  *
*  for flags, the largest item that can be accommodated in a list is   *
*  2**30 - 1 (32-bit) or 2**62 - 1 (64-bit).                           *
*                                                                      *
************************************************************************
*  V1A, 03/19/00, GNR - New routines                                   *
*  V2A, 11/16/08, GNR - Revise for 64-bit operation                    *
*  ==>, 11/16/08, GNR - Last date before committing to svn repository  *
*  Rev, 10/24/13, GNR - SRA() for signed '>>'                          *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "rkilst.h"

/*---------------------------------------------------------------------*
*                              ilsttest                                *
*                                                                      *
*  This routine determines whether a given test value is included in   *
*  an iteration list, either as an enumerated value or in a range.     *
*  A NULL list is OK.                                                  *
*---------------------------------------------------------------------*/

int ilsttest(ilst *pil, ilstitem item) {

   ilstitem t, *p;
   long i, ievry;

   if (!pil)
      return FALSE;

   p = pil->pidl;

   if (ievry = pil->evry & IL_MASK) {     /* Assignment intended */
      long ibase = SRA(pil->evry,IL_BITS);
      item = ibase + (item - ibase) % ievry;
      }

   i = ilstsrch(pil, item);

   if (i < 0)                       /* Below bottom of list */
      return FALSE;

   if (p[i] == item)                /* Found exactly */
      return TRUE;

   if (i >= pil->nusm1)             /* At top, can't be a range */
      return FALSE;

   if (t = p[i+1] & IL_REIN) {      /* Assignment intended */
      if (t == IL_INCR) {           /* Range with increment */
         if (item <= (p[i+2] & IL_MASK) &&
            (item - p[i]) % (p[i+1] & IL_MASK) == 0)
            return TRUE;
         }
      else if (item <= (p[i+1] & IL_MASK))
         return TRUE;
      }

   return FALSE;

   } /* End ilsttest() */

/*---------------------------------------------------------------------*
*                               ilstset                                *
*                                                                      *
*  Set the starting position of an iteration to the smallest value     *
*  that is greater than or equal to the argument value.  Typically,    *
*  this is used to start an iteration at the first item on the         *
*  current node in a parallel computer.  A NULL list is OK.            *
*---------------------------------------------------------------------*/

void ilstset (iter *pit, ilst *pil, ilstitem item) {

   pit->pilst = pil;
   if (pil && pil->nusm1 >= 0) {

      long i, ievry = pil->evry & IL_MASK;
      ilstitem t, *p = pil->pidl;

      /* Work in base range of EVERY batches */
      if (ievry && item > 0) {
         long ibase = SRA(pil->evry,IL_BITS);
         pit->iadj = item - (ibase + (item - ibase) % ievry);
         item -= pit->iadj;
         }
      else
         pit->iadj = 0;

      i = ilstsrch(pil, item);

      /* If item is below bottom of list, use first item in list */
      if (i < 0) i = 0;

      /* If item is above value returned by search, check next */
      while (1) {
         if (i < pil->nusm1 &&
               (t = p[i+1] & IL_REIN)) {  /* Assignment intended */
            if (t == IL_INCR) {           /* Range with increment */
               register ilstitem iend = p[i+2] & IL_MASK;
               /* (ilstread guarantees iend is in the range) */
               if (item > iend)
                  i += 3;
               else {
                  register ilstitem incr = p[i+1] & IL_MASK;
                  register ilstitem inow;
                  if (item > p[i]) {
                     inow = item + incr - 1;
                     inow -= (inow - p[i])%incr;
                     }
                  else
                     inow = p[i];
                  pit->iend = iend;
                  pit->incr = incr;
                  pit->inow = inow;
                  break;
                  }
               }
            else {                        /* Range with unit step */
               register ilstitem iend = p[i+1] & IL_MASK;
               if (item > iend)
                  i += 2;
               else {
                  pit->iend = iend;
                  pit->incr = 1;
                  pit->inow = max(item,p[i]);
                  break;
                  }
               }
            }
         else if (i <= pil->nusm1) {      /* Single list item */
            if (item > p[i])
               i += 1;
            else {
               pit->iend = p[i];
               pit->incr = 1;
               pit->inow = p[i];
               break;
               }
            }
         else {
            pit->iidl = -1L;
            return;
            }
         } /* End checking loop */

      pit->iidl  = i;
      }

   else                          /* List is empty */
      pit->iidl = -1L;

   } /* End ilstset() */

/*---------------------------------------------------------------------*
*                              ilstiter                                *
*                                                                      *
*  Returns current item in an iteration and advances to the next item. *
*  Returns -1 if the list is empty or all items have been processed.   *
*  Results are undefined if ilstset() has not been called.             *
*---------------------------------------------------------------------*/

long ilstiter(iter *pit) {

   static long ki[3] = { 1,3,2 };   /* Range size by terminal code */

   if (pit->iidl < 0)
      return -1L;                   /* List is empty or exhausted */

   else {
      long ir = (long)(pit->inow + pit->iadj);

      /* Advance to next list item */
      pit->inow += pit->incr;
      if (pit->inow > pit->iend) {
         ilst *pil = pit->pilst;
         ilstitem t, *p = pil->pidl;
         long i = pit->iidl;
         if (i < pil->nusm1)
            i += ki[p[i+1] >> IL_BITS];
         else
            i += 1;

         if (i > pil->nusm1) {               /* At end of list? */
            long ievry = pil->evry & IL_MASK;
            if (ievry) {
               pit->iadj += ievry;
               i = 0; }
            else {
               pit->iidl = -1L;
               return ir; }
            }

         pit->iidl = i;
         pit->inow = p[i];
         if (i < pil->nusm1 &&
               (t = p[i+1] & IL_REIN)) {     /* Assignment intended */
            if (t == IL_INCR) {              /* Range with increment */
               pit->incr = p[i+1] & IL_MASK;
               pit->iend = p[i+2] & IL_MASK;
               }
            else {                           /* Range with unit step */
               pit->incr = 1;
               pit->iend = p[i+1] & IL_MASK;
               }
            }
         else {                              /* Single list item */
            pit->incr = 1;
            pit->iend = p[i];
            }
         }

      return ir;
      }

   } /* End ilstiter() */


/*---------------------------------------------------------------------*
*                               ilstnow                                *
*                                                                      *
*  Returns current item in an iteration without advancing.             *
*  Returns -1 if the list is empty or all items have been processed.   *
*  Results are undefined if ilstset() has not been called.             *
*---------------------------------------------------------------------*/

long ilstnow (iter *pit) {

   return (pit->iidl < 0) ? -1L : (long)(pit->inow + pit->iadj);

   } /* End ilstnow() */
