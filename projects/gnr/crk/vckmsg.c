/* (c) Copyright 2009, The Rockefeller University *11115* */
/* $Id: vckmsg.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                    IVCKMSG0, IVCKMSG1, IVCKMSG2                      *
*                                                                      *
*  Input variable error message generators for bcdin, ibcdin, wbcdin   *
*  (These were taken out of bcdin.c and put in this new file because   *
*  input routines may be used in different combinations in different   *
*  applications--need only one copy of these error routines for all    *
*  of them.)                                                           *
*                                                                      *
************************************************************************
*  V1A, 05/24/09, GNR - Split out from bcdin.c for 64-bit              *
*  ==>, 05/24/09, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include "sysdef.h"
#include "rocks.h"
#include "rockv.h"
#include "rkxtra.h"
#include "rkarith.h"

/*=====================================================================*
*                                                                      *
* Subroutines IVCKMSG0, IVCKMSG1, IVCKMSG2 -- Generate error message   *
*                                                                      *
*  IVCKMSG0 - The message is entirely contained in this routine.       *
*  IVCKMSG1 - The caller passes a literal limit for the message.       *
*  IVCKMSG2 - The caller passes number integer bits for the msg.       *
*=====================================================================*/

void ivckmsg0(const char *udat, int ierr) {

   static char *etxt[] = {
      "<== IS NOT A WELL FORMED NUMBER.",
      "<== IS NOT A VALID HEX/OCTAL NUMBER.",
      "<== HAS AN ILL-FORMED EXPONENT.",
      "<== MUST BE >= 0.",
      "<== MUST BE NONZERO AFTER ROUNDING.",
      "<== MUST BE AN INTEGER.",
      "<== OVERFLOWS DURING INPUT CONVERSION.",
      "<== UNDERFLOWS DURING INPUT CONVERSION.",
      };

   if (ierr > IVCK_HIGHCODE) abexit(114);
   if (RK_Inflags & (IVCK_qERR << ierr)) return;
   ermark(RK_MARKFLD);
   if (RKC.ktest & VCK_ADJ && ierr == IVCK_APPGE0) {
      char vadj[10];
      bcdout(RK_UFLW|RK_AUTO|RK_IORF|9, vadj, RKC.dvadj);
      cryout(RK_E1, "0***AFTER ADJUSTMENT OF", RK_LN3, vadj, 10,
         ",", 1, " ***NUMERIC VALUE ==>", RK_LN0,
         udat, RK_MXWD+1, etxt[ierr-1], RK_CCL, NULL);
      }
   else
      cryout(RK_E1, "0***NUMERIC VALUE ==>", RK_LN2,
         udat, RK_MXWD+1, etxt[ierr-1], RK_CCL, NULL);
   RK_Inflags |= (IVCK_qERR << ierr);

   } /* End ivckmsg0() */

void ivckmsg1(const char *udat, char *vlim) {

   if (RK_Inflags & IVCK_qERR) return;
   ermark(RK_MARKFLD);
   cryout(RK_E1, "0***MAGNITUDE OF NUMERIC VALUE ==>|", RK_LN2,
      udat, RK_MXWD+1, "|<== MUST BE < ", RK_CCL, vlim, RK_CCL,
      ".", 1, NULL);
   RK_Inflags |= IVCK_qERR;

   } /* End ivckmsg1() */

void ivckmsg2(const char *udat, int iint) {

   ui64 mxval = jeul(1);
   char vlim[WIDE_SIZE+1];

   if (RK_Inflags & IVCK_qERR) return;
   ermark(RK_MARKFLD);
   if (iint >= BITSPERUI64) {
      ivckmsg1(udat, "1.84467E19 (64 BITS)"); return; }
   else if (iint >= 0) {
      mxval = jsluw(mxval, iint);
      wbcdwt(&mxval, vlim, RK_IORF|RK_LFTJ|RK_NUNS|RK_NI64|OUT_SIZE-1);
      }
   else wbcdwt(&mxval, vlim, (abs(iint)<<RK_BS)|
      (6<<RK_DS|RK_EFMT|RK_LFTJ|RK_NI64|OUT_SIZE-1));
   vlim[RK.length+1] = '\0';
   ivckmsg1(udat, vlim);

   } /* End ivckmsg2() */
