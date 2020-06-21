/* (c) Copyright 1997-2016, The Rockefeller University *11115* */
/* $Id: shsortus.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                             shsortus()                               *
*                                                                      *
*  Performs a "shell short" (K&R p.62) on an array of unsigned short   *
*     integers.                                                        *
*  Arguments:                                                          *
*     us    Array of unsigned short integers to be sorted              *
*     n     Number of elements in array s                              *
*                                                                      *
************************************************************************
*  Initial version, 05/12/97, G.N. Reeke                               *
*  ==>, 07/01/02, GNR - Last date before committing to svn repository  *
*  Rev, 10/24/13, GNR - Add test for negative n                        *
*  Rev, 02/19/16, GNR - Add #include sysdef.h for abexit declaration   *
***********************************************************************/

#include "sysdef.h"

void shsortus(unsigned short *us, int n) {

   int gap,i,j;
   unsigned short temp;

   if (n <= 0) abexit(57);
   for (gap=n>>1; gap>0; gap>>=1)
      for (i=gap; i<n; i++)
         for (j=i-gap; j>=0 && us[j]>us[j+gap]; j-=gap) {
            temp = us[j];
            us[j] = us[j+gap];
            us[j+gap] = temp;
            }

   } /* End shsortus */

