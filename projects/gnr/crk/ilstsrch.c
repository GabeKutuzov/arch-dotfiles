/* (c) Copyright 2000-2013, The Rockefeller University *11115* */
/* $Id: ilstsrch.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                             ilstsrch.c                               *
*                                                                      *
*  Perform a binary search to find the offset to the largest value in  *
*  an iteration list that is less than or equal to a given number.     *
*  This routine always returns the offset to a single item or to the   *
*  start of a range, never to an increment or range end item.  It      *
*  returns -1 if the request item is smaller than the first list item  *
*  or if the list is empty.                                            *
*                                                                      *
************************************************************************
*  V1A, 03/19/00, GNR - New routine                                    *
*  ==>, 07/01/02, GNR - Last date before committing to svn repository  *
*  Rev, 10/24/13, GNR - SRA() for signed '>>'                          *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "rkilst.h"

long ilstsrch(ilst *pil, ilstitem item) {

   ilstitem *p = pil->pidl;
   long ix, i = -1L, i1 = 0, i2 = pil->nusm1;

   static long ki[3] = { 1,3,2 };   /* Range size by terminal code */
   static long kd[4] = { 0,1,1,2 }; /* Decrement to start of range */

   if (p) while (i2 >= i1) {
      ix = i;
      i = SRA((i2 + i1),1);
      i -= kd[p[i] >> IL_BITS];     /* Locate start of range */
      if (item >= p[i]) {           /* Locate next range up */
         if (item == p[i] || i >= i2) break;
         i1 = i + ki[p[i+1] >> IL_BITS];
         }
      else {
         i2 = i - 1;
         i = ix; }
      } /* End search loop */

   return i;

   } /* End ilstsrch() */

