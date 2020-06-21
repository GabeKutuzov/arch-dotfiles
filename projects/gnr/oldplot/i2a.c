/* (c) Copyright 1992-2011, The Rockefeller University *21114* */
/* $Id: i2a.c 30 2017-01-16 19:30:14Z  $ */
/***********************************************************************
*                                i2a()                                 *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void i2a(unsigned char *vstr,int inp,int size,int type)          *
*                                                                      *
*  DESCRIPTION:                                                        *
*     This function converts an integer to a string of digits.         *
*     'vstr' pointer to the destination string.                        *
*     'inp'  the integer to be converted.                              *
*     'size' the maximum size of the integer.                          *
*     'type' type of conversion.                                       *
*                                                                      *
*  RETURN VALUES:                                                      *
*     None                                                             *
************************************************************************
*  Version 1, 06/09/92, ROZ                                            *
*  Rev, 07/20/93, GNR - Use max value if number doesn't fit            *
*  Rev, 02/29/08, GNR - Add type == FORCESZ, use div()                 *
*  ==>, 03/01/08, GNR - Last mod before committing to svn repository   *
*  Rev, 10/11/11, GNR - _NCG to _RKG, gmf.h now in glu.h, etc.         *
***********************************************************************/

#include <stdlib.h>
#include "glu.h"

void i2a(unsigned char *vstr, int inp, int size, int type) {

   div_t q;
   int   sz;
   char  str[32];

   /* If negative number put '-' sign at beginning of vstr */
   if (inp < 0) {
      *vstr++ = '-';
      inp = (-inp);
      --size;
      }

   /* Make sure there is room for at least one digit */
   if (size <= 0) goto update;

   /* Calculate and count decimal digits.
   *  Note that str contains digits in reverse order. */
   for (sz=0; sz<size; sz++) {
      q = div(inp, 10);
      str[sz] = q.rem;
      if ((inp = q.quot) == 0) goto stage2;
      }
   /* Number is too long--replace with all nine's */
   --sz;
   while (sz--) *vstr++ = '9';
   *vstr++ = 'j';
   goto update;

   /* Store digits in output field */
stage2:
   if (type == FORCESZ) {
      --size; while (sz < size) str[++sz] = 0; }
   while (sz) *vstr++ = str[sz--] + '0';
   *vstr++ = str[0] + 'a';
update:
   _RKG.MFCurrPos = vstr;
   return;
   } /* End i2a() */
