/* (c) Copyright 2009, The Rockefeller University *11115* */
/* $Id: wvalck.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               wvalck                                 *
*                                                                      *
*  This is the routine for fixed-point input value checking used       *
*  by inform, eqscan, kwscan.  Test conditions set up by calls to      *
*  getvalck are applied here to variables read in with wbcdin.         *
*                                                                      *
*  Synopsis: void wvalck(void *val, ui32 ccode)                        *
*                                                                      *
*  Arguments:                                                          *
*     val         Pointer to value to be checked.  The length and      *
*                 whether signed or unsigned is specified by the       *
*                 ccode argument.                                      *
*     ccode       wbcdin conversion code to convert test value.        *
*                                                                      *
*  Return value:  None.  If a value check fails, the value is replaced *
*                 via the pointer with the nearest legal value.        *
*                                                                      *
*  Errors:        Conversion of the test value may generate errors.    *
************************************************************************
*  V1A, 07/25/09, GNR - New routine, based on ivalck()                 *
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

void wvalck(void *val, ui32 ccode) {

   byte *pv;                  /* Access val as byte string */
   ui32 bvcode;               /* wbcdwt code for bad value */
   int  i,itest;              /* Test index, result */
   int  nisz;                 /* Size of the variable */
   byte tval[sizeof(ui64)];   /* Space for test value */
   byte sinv;                 /* Sign inverter */

   /* Quick exit if no tests */
   if (!(RKC.ktest & ~VCK_ADJ)) return;

   /* Items used with both max and min tests */
   pv = (byte *)val;
   bvcode = (ccode & ~((RK_BSCL-RK_D)|RK_INT|RK_MXWD)) |
      (RK_LFTJ|RK_UFLW|RK_AUTO|(BadDatSize-1));
   nisz = wnclen(ccode);
   sinv = (ccode & RK_NUNS) ? 0 : 0x80;

/*---------------------------------------------------------------------*
*                            Minimum tests                             *
*---------------------------------------------------------------------*/

   if (RKC.ktest & (VCK_GTHAN|VCK_GTEQ)) {

      if (RKC.cmntest[0])        /* Get test value */
         wbcdin(RKC.cmntest, tval, (ccode & ~((RK_BSCL-RK_D)|
            RK_ZTST|RK_QPOS|RK_INT|RK_MXWD)) | (CkDatSize-1));
      else
         memset(tval, 0, nisz);

#if BYTE_ORDRE > 0               /* Big-endian comparison */
      pv[0] ^= sinv; tval[0] ^= sinv;
      for (itest=i=0; i<nisz; ++i) {
         if (pv[i] > tval[i]) { itest =  1; break; }
         if (pv[i] < tval[i]) { itest = -1; break; }
         }
      pv[0] ^= sinv; tval[0] ^= sinv;
#else                            /* Little-endian comparison */
      pv[nisz-1] ^= sinv; tval[nisz-1] ^= sinv;
      for (itest=0,i=nisz-1; i>=0; --i) {
         if (pv[i] > tval[i]) { itest =  1; break; }
         if (pv[i] < tval[i]) { itest = -1; break; }
         }
      pv[nisz-1] ^= sinv; tval[nisz-1] ^= sinv;
#endif

      /* Note:  If both '>' and '>=' specified, getvalck
      *  abexits, so no need to do both tests here.  */
      if (RKC.ktest & VCK_GTHAN) {
         if (itest <= 0) {
            unsigned int tinc = 1;
            wbcdwt(val, RKC.badval, bvcode);
            valckmsg("<= ", RKC.cmntest);
            /* Set user value to test value plus 1 and return */
#if BYTE_ORDRE > 0               /* Big-endian */
            for (i=nisz-1; i>=0; --i)
#else                            /* Little-endian */
            for (i=0; i<nisz; ++i)
#endif
               {  tinc += (unsigned int)tval[i];
                  pv[i] = (byte)tinc;
                  tinc >>= BITSPERBYTE;
                  }
            return;
            }
         }
      if (RKC.ktest & VCK_GTEQ) {
         if (itest < 0) {
            wbcdwt(val, RKC.badval, bvcode);
            valckmsg("< ", RKC.cmntest);
            memcpy(pv, tval, nisz);
            return;
            }
         }
      }

/*---------------------------------------------------------------------*
*                            Maximum tests                             *
*---------------------------------------------------------------------*/

   if (RKC.ktest & (VCK_LTHAN|VCK_LTEQ)) {

      if (RKC.cmxtest[0])        /* Get test value */
         wbcdin(RKC.cmxtest, tval, (ccode & ~((RK_BSCL-RK_D)|
            RK_ZTST|RK_QPOS|RK_INT|RK_MXWD)) | (CkDatSize-1));
      else
         memset(tval, 0, nisz);

#if BYTE_ORDRE > 0               /* Big-endian comarison */
      pv[0] ^= sinv; tval[0] ^= sinv;
      for (itest=i=0; i<nisz; ++i) {
         if (pv[i] > tval[i]) { itest =  1; break; }
         if (pv[i] < tval[i]) { itest = -1; break; }
         }
      pv[0] ^= sinv; tval[0] ^= sinv;
#else                            /* Little-endian comparison */
      pv[nisz-1] ^= sinv; tval[nisz-1] ^= sinv;
      for (itest=0,i=nisz-1; i>=0; --i) {
         if (pv[i] > tval[i]) { itest =  1; break; }
         if (pv[i] < tval[i]) { itest = -1; break; }
         }
      pv[nisz-1] ^= sinv; tval[nisz-1] ^= sinv;
#endif

      /* Note:  If both '<' and '<=' specified, getvalck
      *  abexits, so need not do both tests here.  */
      if (RKC.ktest & VCK_LTHAN) {
         if (itest >= 0) {
            ui16 tdec = 1;
            wbcdwt(val, RKC.badval, bvcode);
            valckmsg(">= ", RKC.cmxtest);
            /* Set user value to test value minus 1 and return */
#if BYTE_ORDRE > 0               /* Big-endian */
            for (i=nisz-1; i>=0; --i)
#else                            /* Little-endian */
            for (i=0; i<nisz; ++i)
#endif
               {  tdec = (ui16)tval[i] - tdec;
                  pv[i] = (byte)tdec;
                  tdec >>= (BITSPERSHORT - 1);
                  }
            return;
            }
         }
      if (RKC.ktest & VCK_LTEQ) {
         if (itest > 0) {
            wbcdwt(val, RKC.badval, bvcode);
            valckmsg("> ", RKC.cmxtest);
            memcpy(pv, tval, nisz);
            return;
            }
         }
      }

/*---------------------------------------------------------------------*
*                            Nonzero test                              *
*---------------------------------------------------------------------*/

   if (RKC.ktest & VCK_NZERO) {
      for (itest=i=0; i<nisz; ++i)
         if (pv[i]) { itest = 1; break; }
      if (itest == 0) {
         strcpy(RKC.badval, "0");
         valckmsg("== ", "0");
         /* Set user value to 1 and return */
         memset(pv, 0, nisz);
#if BYTE_ORDRE > 0               /* Big-endian */
         pv[nisz-1] = 1;
#else                            /* Little-endian */
         pv[0] = 1;
#endif
         return;
         }
      }

   } /* End wvalck() */
