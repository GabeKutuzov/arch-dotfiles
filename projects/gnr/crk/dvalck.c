/* (c) Copyright 2007-2009, The Rockefeller University *11115* */
/* $Id: dvalck.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               dvalck                                 *
*                                                                      *
*  This is the routine for double-precision floating-point input value *
*  checking used by inform, eqscan, kwscan.  Test conditions set up by *
*  calls to getvalck are applied here to variables read in with bcdin. *
*                                                                      *
*  Synopsis: double dvalck(double value, ui32 ccode)                   *
*                                                                      *
*  Arguments:                                                          *
*     value       Double-precision floating-point value to be checked. *
*     ccode       bcdin conversion code to convert test value.         *
*                                                                      *
*  Return value:  value updated to range enforced by the tests.        *
*                                                                      *
*  Errors:        Conversion of the test value may generate errors.    *
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

double dvalck(double value, ui32 ccode) {

   double tval;            /* Test value */
   ui32 bvcode;            /* Modified bcdout code for bad value */
   ui32 tvcode;            /* Modified bcdin code for test value */

   /* Quick exit if no tests */
   if (!(RKC.ktest & ~VCK_ADJ)) return value;

   tvcode = ccode & ~(RK_ZTST|RK_QPOS|RK_HEXF|RK_SNGL|RK_BYTE|RK_MXWD) |
      (CkDatSize-1);
   bvcode = ccode & ~(RK_BSCL-RK_D|RK_SNGL|RK_BYTE|RK_MXWD) |
      (RK_LFTJ|RK_UFLW|RK_AUTO|(BadDatSize-1));

   /* Minimum tests.  Note:  If both '>' and '>=' specified,
   *  getvalck abexits, so need not do both tests here.  */
   if (RKC.ktest & (VCK_GTHAN|VCK_GTEQ)) {
      tval = RKC.cmntest[0] ? bcdin(tvcode, RKC.cmntest) : 0.0;
      if (RKC.ktest & VCK_GTHAN) {
         if (value <= tval) {
            bcdout(bvcode, RKC.badval, value);
            valckmsg("<= ", RKC.cmntest);
            return tval;
            }
         }
      else {
         if (value < tval) {
            bcdout(bvcode, RKC.badval, value);
            valckmsg("< ", RKC.cmntest);
            return tval;
            }
         }
      }

   /* Maximum tests.  Note:  If both '<' and '<=' specified,
   *  getvalck abexits, so need not do both tests here.  */
   if (RKC.ktest & (VCK_LTHAN|VCK_LTEQ)) {
      tval = RKC.cmxtest[0] ? bcdin(tvcode, RKC.cmxtest) : 0.0;
      if (RKC.ktest & VCK_LTHAN) {
         if (value >= tval) {
            bcdout(bvcode, RKC.badval, value);
            valckmsg(">= ", RKC.cmxtest);
            return tval;
            }
         }
      else {
         if (value > tval) {
            bcdout(bvcode, RKC.badval, value);
            valckmsg("> ", RKC.cmxtest);
            return tval;
            }
         }
      }

   if (RKC.ktest & VCK_NZERO) {
      if (value == 0.0) {
         strcpy(RKC.badval, "0.0");
         valckmsg("==", "0.0");
         return 0.0;
         }
      }

   return value;

   } /* End dvalck() */
