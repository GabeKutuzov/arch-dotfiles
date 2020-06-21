/* (c) Copyright 1990-2013, The Rockefeller University *21114* */
/* $Id: number.c 21 2013-03-15 17:19:58Z  $ */
/***********************************************************************
*                              number()                                *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void number(float x, float y, float ht, double val, float angle, *
*        int ndec)                                                     *
*                                                                      *
*  DESCRIPTION:                                                        *
*     This function converts a floating point number to a decimal      *
*        string.                                                       *
*  Arguments:                                                          *
*     x,y    position on the screen.                                   *
*     ht     height of the drawn string.                               *
*     val    the floating point number to be converted.                *
*     angle  angle of the drawn string.                                *
*     ndec   is the number of places after the decimal to be           *
*            plotted. If ndec = 0, the integer portion of the          *
*            number is plotted with a decimal point, if ndec = -1,     *
*            it is plotted without a decimal, if ndec < -1,            *
*            abs(ndec) - 1 digits are dropped from the right of        *
*            the number before plotting.                               *
*                                                                      *
*  RETURN VALUES:                                                      *
*     None                                                             *
************************************************************************
*  Version 1, 06/19/90, JMS - Broken out of glu.c                      *
*  Rev,       09/29/92, ROZ - Changed it to use symbol()               *
*  ==>, 02/29/08, GNR - Last mod before committing to svn repository   *
*  Rev, 10/11/11, GNR - _NCG to _RKG, gmf.h now in glu.h, etc.         *
*  Rev, 10/17/11, GNR - Correct bug in bcdout ic decimal codes.        *
*  Rev, 01/05/13, GNR - Correct bug when ndec = -1.                    *
***********************************************************************/

#include "plots.h"   /* Must precede glu */
#include "glu.h"     /* Needed to define RKD macro */
#include "rocks.h"
#include "rkxtra.h"

int abs(int);

void number(float x, float y, float ht, double val, float angle,
      int ndec) {

   char printme[MAXSTRING];

   if (!_RKG.s.MFActive) return;
   if (ndec > 14) abexit(20);

   if (ndec >= 0)
      bcdout(RK_IORF+RK_LFTJ+((ndec+1)<<RK_DS)+(MAXSTRING-1),
         printme, val);
   else {
      int cutoff;
      bcdout(RK_IORF+RK_LFTJ+(MAXSTRING-1), printme, val);
      cutoff = RK.length + 2 - abs(ndec);
      if (cutoff > 0)
         printme[cutoff] = '\0';
      else
         printme[0] = '*', RK.length = 0;
      }

   symbol(x,y,ht,printme,angle,RK.length+1);

   } /* End number */

