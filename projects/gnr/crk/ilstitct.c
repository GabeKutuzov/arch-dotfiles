/* (c) Copyright 2001-2013, The Rockefeller University *11115* */
/* $Id: ilstitct.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                             ilstitct.c                               *
*                                                                      *
*  Find the number of items in an iteration list that are less than    *
*  a given value.  Counts iterations of EVERY blocks as necessary.     *
*                                                                      *
*  Synopsis:  long ilstitct(ilst *pil, ilstitem itmval)                *
*                                                                      *
*  Arguments:                                                          *
*     pil         Ptr to the iteration list to be examined.            *
*     itmval      Value of the item for which the count is desired.    *
*                                                                      *
*  Returns:                                                            *
*     Number of items in list at pil that are less than itmval.        *
*                                                                      *
************************************************************************
*  V1A, 09/15/01, GNR - New routine                                    *
*  ==>, 11/16/08, GNR - Last date before committing to svn repository  *
*  Rev, 10/24/13, GNR - SRA() for signed '>>'                          *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "rkilst.h"

long ilstitct(ilst *pil, ilstitem itmval) {

   ilstitem *pitl,*pitle;     /* Ptrs to item list and its end */
   ilstitem tag;              /* Tag bits from list item       */
   long ibase;                /* Base for EVERY looping        */
   long ievry;                /* EVERY block size              */
   long nitems = 0;           /* Total items less than itmval  */
   long ritem;                /* Cell number relative to base  */
   long rs,re,ri,rv;          /* Range start, end, incr, value */

   if (!pil) goto itct_done;
   ibase = SRA(pil->evry,IL_BITS);
   ievry = pil->evry & IL_MASK;
   pitle = pil->pidl + pil->nusm1;
   while ((ritem = itmval - ibase) > 0) {
      ri = re = rs = 0;       /* Flag non-pickup of start value and
                              *  kill warnings re undefined re,rs */
      for (pitl=pil->pidl; pitl<=pitle; pitl++) {
         rv = *pitl & IL_MASK;
         if (tag = *pitl & IL_REIN) {  /* Assignment intended */
            if (tag == IL_INCR)  /* Got range increment */
               ri = rv;
            else                 /* Got end of range */
               re = min(rv, itmval-1);
            }
         else {                  /* Got single item or range start */
            if (ri > 0)
               nitems += (re - rs)/ri + 1;
            if (rv >= itmval) goto itct_done;
            re = rs = rv, ri = 1;
            }
         } /* End loop over item list */

      if (ri > 0)             /* Count last item or range at end */
         nitems += (re - rs)/ri + 1;
      if (ievry == 0 || ritem < ievry) goto itct_done;
      nitems *= ritem/ievry;
      itmval  = ritem%ievry + ibase;
      ievry   = 0;            /* Terminate loop next time */
      } /* End while */

itct_done:
   return nitems;

   } /* End ilstitct() */
