/* (c) Copyright 2009-2010, The Rockefeller University *11115* */
/* $Id: wseedin.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                              wseedin.c                               *
*                                                                      *
*  ROCKS library routine to read a 'wseed' (pair of 31- and 27-bit     *
*  random number seeds), called from inform() or kwscan()/eqscan()     *
*  when an 'IS' format code is encountered.  Formats recognized:       *
*     nnnnn    A single integer, 0= < nnnnn < 2**31-1, sets the seed31 *
*              component to nnnnn and the seed27 component to -1.      *
*     (mm,nn)  Two integers, mm <= 2**27 and 0= < nn < 2**31-1, sets   *
*              seed27 to mm and seed31 to nn.  mm < 0 sets compati-    *
*              bility with old udev(), nn == 0 causes all wdevxx calls *
*              to return 0 for tests.                                  *
*                                                                      *
*  Synopsis:   void wseedin(wseed *pwsd)                               *
*              (Assumes cdscan has been called and input scanned       *
*              such that the wseed to be read in is the next field.)   *
*                                                                      *
*  Prototyped in: wseed typedef in sysdef.h, routine in rkxtra.h       *
*----------------------------------------------------------------------*
*  V1A, 12/26/09, GNR - New routine                                    *
*  Rev, 02/04/10, GNR - Compatibility flag is seed27 == -1, too-large  *
*                       seeds are corrected modulo base, no ermark().  *
*  ==>, 02/04/10, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"
#define  WDEV_JDEF
#include "wdevcom.h"

void wseedin(wseed *pwsd) {

   int   ic,oldslen;
   short iopl;                /* Old paren level */
   char  lea[LONG_SIZE+2];    /* Space for scanning */

   oldslen = scanlen(LONG_SIZE+2);  /* Save current scan length */
   iopl = RK.plevel;          /* Save paren level */
   ic = scanck(lea, RK_REQFLD|RK_FENCE,
      ~(RK_COMMA|RK_BLANK|RK_INPARENS|RK_LFTPAREN));
   if (RK.plevel > iopl) {    /* Looks like two seeds in parens */
      wbcdin(lea, &pwsd->seed27,
         RK_IORF|RK_CTST|RK_NI32|LONG_SIZE);
      if (pwsd->seed27 < 0) pwsd->seed27 = -1;
      else pwsd->seed27 %= (w27+1);
      ic = scanck(lea, RK_REQFLD|RK_FENCE,
         ~(RK_COMMA|RK_BLANK|RK_INPARENS|RK_RTPAREN));
      if (!(ic & RK_RTPAREN)) ermark(RK_TOOMANY);
      }
   else                       /* One value, compatibility mode */
      pwsd->seed27 = -1;
   wbcdin(lea, &pwsd->seed31,
      RK_IORF|RK_QPOS|RK_CTST|RK_NI32|LONG_SIZE);
   if (pwsd->seed31 == w31) pwsd->seed31 -= 1;

   scanlen(oldslen);          /* Restore original scan length */
   return;
   } /* End wseedin() */
