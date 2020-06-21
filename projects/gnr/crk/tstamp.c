/* (c) Copyright 1992-2015, The Rockefeller University *11115* */
/* $Id: tstamp.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               tstamp                                 *
*                                                                      *
*  Format the current system time for printing.                        *
*                                                                      *
************************************************************************
*  Version 1, 05/22/92, ROZ                                            *
*  Rev, 09/30/92, GNR - Add zero padding to all fields                 *
*  Rev, 08/19/00, GNR - Fix Y2K problem                                *
*  ==>, 12/07/08, GNR - Last date before committing to svn repository  *
*  Rev, 08/22/09, GNR - Replace ibcdwt w/wbcdwt for 64-bit compilation *
*  Rev, 10/04/15, GNR - Use local i2a instead of wbcdwt for rksubs use *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <time.h>
#include "sysdef.h"
#if defined(ACC)
#include <sys/types.h>
#endif
#include "rksubs.h"

static char *pts;                /* Ptr to output */

static void Iout2(int iv) {      /* Output a two-digit integer */

   if (iv < 0 || iv > 99) {
      *pts++ = '*';
      *pts++ = '*';
      }
   else {
      div_t tu = div(iv, 10);
      *pts++ = '0' + tu.quot;
      *pts++ = '0' + tu.rem;
      }

   } /* End Iout2() */
      

void tstamp(char *pstamp) {

   struct tm *tmstmp;
   time_t tp;

   time(&tp);
   tmstmp = localtime(&tp);

   pts = pstamp;
   Iout2(tmstmp->tm_year%100);
   Iout2(tmstmp->tm_mon+1);
   Iout2(tmstmp->tm_mday);
   Iout2(tmstmp->tm_hour);
   Iout2(tmstmp->tm_min);
   Iout2(tmstmp->tm_sec);
   }
