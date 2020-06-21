/* (c) Copyright 2009, The Rockefeller University *11115* */
/* $Id: wseedout.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                             wseedout.c                               *
*                                                                      *
*  ROCKS library routine to output a 'wseed' (pair of 31- and 27-bit   *
*  random number seeds) to an alphanumeric filed (typically a cryout   *
*  buffer).  It may be called from user code or from convrt() when an  *
*  'IS' format code is encountered.  The format of the output is:      *
*     "(mm,nn)"  where -1 <= mm <= 2**27 and 0 <= nn < 2**31-1.        *
*                                                                      *
*  Synopsis:  void wseedout(wseed *pwsd, char *field, ui32 ic)         *
*                                                                      *
*  Arguments:                                                          *
*     pwsd        Pointer to the wseed to be printed.                  *
*     field       Pointer to the string where output is to be placed.  *
*     ic          wbcdwt-style control code.  Only the width and       *
*                 left-justify codes are used.                         *
*                                                                      *
*  If the output will not fit in the specified width, the field is     *
*  filled with asterisks.  RK.length is set to the number of nonblank  *
*  characters writte, minus one.                                       *
*                                                                      *
*  Prototyped in: wseed typedef in sysdef.h, routine in rkxtra.h       *
*----------------------------------------------------------------------*
*  V1A, 12/28/09, GNR - New routine                                    *
*  ==>, 12/28/09, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <string.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"

void wseedout(wseed *pwsd, char *field, ui32 ic) {

   int   blen, dlen, dlm1;    /* Length of blanks, data, data - 1 */
   int   mlm1 = ic & RK_MXWD; /* Max width - 1 */
   char  tfld[2*LONG_SIZE+4]; /* Temp space for output */

   tfld[0] = '(';
   wbcdwt(&pwsd->seed27, tfld+1,
      RK_IORF|RK_LFTJ|RK_NI32|LONG_SIZE);
   dlm1 = RK.length + 2;
   tfld[dlm1++] = ',';
   wbcdwt(&pwsd->seed31, tfld+dlm1,
      RK_IORF|RK_LFTJ|RK_NUNS|RK_NI32|LONG_SIZE);
   dlm1 += RK.length + 1;
   tfld[dlm1] = ')';
   blen = mlm1 - dlm1;
   if (blen < 0)
      memset(field, '*', (dlm1 = mlm1)+1);
   else {
      dlen = dlm1 + 1;
      if (ic & RK_LFTJ) {
         memcpy(field, tfld, dlen);
         if (blen > 0) memset(field+dlen, ' ', blen);
         }
      else {
         if (blen > 0) memset(field, ' ', blen);
         memcpy(field+blen, tfld, dlen);
         }
      }
   RK.length = dlm1;
   return;
   } /* End wseedout() */
