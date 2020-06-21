/* (c) Copyright 2009, The Rockefeller University *11115* */
/* $Id: uvalck.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               uvalck                                 *
*                                                                      *
*  Synopsis: ui32 uvalck(ui32 value, ui32 ccode)                       *
*                                                                      *
*  This is the routine for unsigned integer input value checking used  *
*  by inform, eqscan, kwscan.  Test conditions set up by calls to      *
*  getvalck are applied here to variables read in with ubcdin.         *
*                                                                      *
*  Arguments:                                                          *
*     value       Unsigned 32-bit fixed-point value to be checked.     *
*     ccode       ubcdin conversion code to convert test value.        *
*                                                                      *
*  Return value:  value updated to range enforced by the tests.        *
*                                                                      *
*  Errors:        Conversion of the test value may generate errors.    *
*     113         A maximum for unsigned value was specified as 0.     *
*                                                                      *
*  Notes:         This design allows checking of short and char        *
*                 values via argument coercion.  Test are ordered      *
*                 according to likelihood, not greatest simplicity.    *
************************************************************************
*  V2B, 07/25/09, GNR - Broken out from valck.c                        *
*  ==>, 07/25/09, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <ctype.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rocks.h"
#include "rockv.h"
#include "rkxtra.h"

ui32 uvalck(ui32 value, ui32 ccode) {

   ui32 tval;              /* Test value */
   ui32 bvcode;            /* Modified bcdout code for bad value */
   ui32 tvcode;            /* Modified bcdin code for test value */

   /* Quick exit if no tests */
   if (!(RKC.ktest & ~VCK_ADJ)) return value;

   tvcode = ccode & ~(RK_ZTST|RK_HEXF|RK_SNGL|RK_BYTE|RK_MXWD) |
      (RK_QPOS|(CkDatSize-1));
   bvcode = ccode & ~(RK_BSCL-RK_D|RK_SNGL|RK_BYTE|RK_MXWD) |
      (RK_LFTJ|RK_UFLW|RK_AUTO|(BadDatSize-1));

   /* Minimum tests.  Note:  If both '>' and '>=' specified,
   *  getvalck abexits, so need not do both tests here.  */
   if (RKC.ktest & (VCK_GTHAN|VCK_GTEQ)) {
      tval = RKC.cmntest[0] ? ubcdin(tvcode, RKC.cmntest) : 0;
      if (RKC.ktest & VCK_GTHAN) {
         if (value <= tval) {
            ubcdwt(bvcode, RKC.badval, value);
            valckmsg("<= ", RKC.cmntest);
            return tval + 1;
            }
         }
      else {
         if (tval == 0) abexit(113);
         if (value < tval) {
            ubcdwt(bvcode, RKC.badval, value);
            valckmsg("< ", RKC.cmntest);
            return tval;
            }
         }
      }

   /* Maximum tests.  Note:  If both '<' and '<=' specified,
   *  getvalck abexits, so need not do both tests here.  */
   if (RKC.ktest & (VCK_LTHAN|VCK_LTEQ)) {
      tval = RKC.cmxtest[0] ? ubcdin(tvcode, RKC.cmxtest) : 0;
      if (RKC.ktest & VCK_LTHAN) {
         if (tval == 0) abexit(113);
         if (value >= tval) {
            tvcode = ccode & ~(RK_BSCL-RK_D|RK_SNGL|RK_BYTE|RK_MXWD) |
               (RK_LFTJ|RK_UFLW|RK_AUTO|RK_IORF|(BadDatSize-1));
            ubcdwt(tvcode, RKC.badval, value);
            valckmsg(">= ", RKC.cmxtest);
            return tval - 1;
            }
         }
      else {
         if (value > tval) {
            tvcode = ccode & ~(RK_BSCL-RK_D|RK_SNGL|RK_BYTE|RK_MXWD) |
               (RK_LFTJ|RK_UFLW|RK_AUTO|RK_IORF|(BadDatSize-1));
            ubcdwt(tvcode, RKC.badval, value);
            valckmsg("> ", RKC.cmxtest);
            return tval;
            }
         }
      }

   if (RKC.ktest & VCK_NZERO) {
      if (value == 0) {
         strcpy(RKC.badval, "0");
         valckmsg("==", "0");
         return 1;
         }
      }

   return value;

   } /* End uvalck() */
