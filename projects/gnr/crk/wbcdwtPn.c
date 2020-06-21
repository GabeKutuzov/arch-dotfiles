/* (c) Copyright 2016, The Rockefeller University *11115* */
/* $Id: wbcdwtPn.c 65 2017-10-13 18:53:31Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                             wbcdwtPn.c                               *
*                                                                      *
*  This is a version of wbcdwt that does not use any ROCKS card input/ *
*  print routines so it can be used in MATLAB mex files and parallel   *
*  computer PARn executables.  It performs 8-16-32-64-bit fixed-point  *
*  numerical output conversion with the following inherited features:  *
*  (1) Entire calculation is performed with fixed point arithmetic,    *
*      so decimal rounding can be exact.                               *
*  (2) Can predict overflow with near-certainty early in the algorithm.*
*  (3) Outputs directly to user field, saving a memcpy().              *
*  (4) Internal function calls are eliminated to save time.            *
*                                                                      *
*  Note added 09/04/11:  Policy regarding protective space at left of  *
*  output field:  Ordinary conversions may go against left edge and    *
*  left-justified conversions always do.  A blank is inserted if there *
*  is no sign and (i) RK_LSPC bit is on, (ii) RK_AUTO bit is on, or    *
*  (iii) E format is forced.  In all these cases, the blank may be     *
*  overwritten if roundoff during decimal insertion overflows into     *
*  the protective blank.                                               *
*                                                                      *
*  Program is prototyped in rksubs.h along with all defined constants. *
************************************************************************
*  V1A, 10/21/16, GNR - New program, based on wbcdwt() and bcdout().   *
*  ==>, 10/21/16, GNR - Last date before committing to svn repository  *
*  Rev, 02/28/17, GNR - Add RK_MZERO op code                           *
*  R65, 09/26/17, GNR - Bug fix -- abend 199 when neg just fits field  *
***********************************************************************/

/*---------------------------------------------------------------------*
*            Usage (same as wbcdwt except error handling):             *
*---------------------------------------------------------------------*/

/*
   To convert an integer (or general fixed point) variable to
   an ASCII character string:
         void wbcdwtPn(void *pitm, char *field, ui32 ic)

   'pitm'  is a pointer to the number to be converted.
   'field' is an array into which the result is placed.
   'ic'    is the processing code described below.

   The operation code 'ic' is the sum of four quantities:
         ic = scale + dec + op + width - 1

      'scale' is required only for noninteger fixed-point variables.
         The scale code is (s << RK_BS), where RK_BS is 24 and 's'
         is the number of bits to the right of the binary point in
         the argument (maximum, 63).

      'dec' specifies the location of the printed decimal point.
         'dec' is equal to (d << RK_DS), where RK_DS is 16 and 'd'
         is one more than the number of places to the right of the
         decimal point (max 15).  'd' = 0 corresponds to integer
         format (no decimal inserted in the output).

      'op' specifies the operation to be performed.  It is made
         up by adding the desired codes from the following table.
         The argument types and format codes are independent.

         Hexadecimal Defined
           Value      Name    Function
         ----------  -------  ------------------------------------------
         0x80000000  RK_MZERO Output -BIG as -0.
         0x40000000  RK_FORCE Used to signal a force to E format.
         0x00800000  RK_DSF   Decimal scale forced.  On decimal output,
                              inserts decimal without scaling.
         0x00800000  RK_NZLC  Lower-case a-f and x for hex output.
         0x00400000  RK_NEGV  Used internally to signal a negative arg.
                              May be used to insert a minus sign in the
                              converted value of an unsigned argument.
         0x00200000  RK_LSPC  Protect unsigned output on left with at
                              least one blank (for printf ' ' flag).
         0x00100000  RK_PLUS  Insert plus sign for positive numbers
                              (for printf '+' flag).
         0x00008000  RK_NUNS  Unsigned.  Argument is an unsigned value.
         0x00004000  RK_LFTJ  Left Justify.  Output is left justified.
         0x00002000  RK_UFLW  Underflow control.  Switch to E format if
                              result would have no significant digits.
                              (Not valid with hex or octal output.)
         0x00001000  RK_AUTO  Automatic Decimal.  Position of decimal
                              point is adjusted to give greatest number
                              of significant figures that will fit in
                              the output field after a single blank,
                              but not more than dec-1 decimals if dec>0.
                              (Not valid with hex or octal output.)
         0x00002000  RK_NZTW  Output number of characters dictated by
                              item type, including leading zeros
                              (hex output only).
         0x00001000  RK_NZ0X  Prepend hex values with '0x' or '0X'.
         0x00001000  RK_Oct0  Prepend octal values with '0'.
         0x00000800  RK_NPAD0 Pad output to left (or right) with zeros.
                              Default is to pad with blanks.  Ignored
                              if RK_EFMT or E format forced.
            --The next two bits specify the mode of conversion--
         0x00000000  RK_EFMT  Generate E (exponential) format.
         0x00000200  RK_HEXF  Perform H (hexadecimal) conversion
                              (error if used with s or e != 0).
         0x00000400  RK_OCTF  Perform O (octal) conversion of integers
                              (error if used with s or e != 0).
         0x00000600  RK_IORF  Generate I (integer) or F (decimal) for-
                              mat (I format = F format with 'd' = 0.)
            --The next three bits specify the size of the argument--
         0x00000000  RK_NDFLT Default: int (16 or 32 bits).
         0x00000040  RK_NBYTE byte (8 bits).
         0x00000080  RK_NHALF short (half-word, 16 bits).
         0x000000c0  RK_NINT  int (16 or 32 bits).
         0x00000100  RK_NI32  si32 or ui32 (always 32 bits).
         0x00000140  RK_NLONG long (32 or 64 bits).
         0x00000180  RK_NI64  si64 or ui64 (always 64 bits).
         0x000001c0  RK_NI128 Reserved for future 128-bit values.

      'width' is the number of characters in the output field, less
         one.  (The maximum width is RK_MXWD+1 = 32.  However, the
         maximum number of digits is given by WIDE_SIZE and the re-
         mainder of the width can only be filled with blanks, signs,
         decimal points, exponents, etc.)

      Additionally, the globals RK.expwid and RK.length are accessed
         in wbcdwt().  The RK structure is replaced in wbcdwtPn and
         bcdoutPn with the small structure RKPn, with variables
         RKPn.expwid and RKPn.length.  RKPn.expwid can be set by the
         user before a call to wbcdwt().  'expwid' is used only for E
         format out-put, and specifies the number of digits in the
         printed expo-nent (including sign).  If 'expwid' is zero or
         larger than EXP_SIZE, the number of digits needed to express
         the largest exponent that can be represented on the machine,
         'expwid' is replaced by EXP_SIZE.  If 'expwid' is too small
         to represent the result, a larger value is substituted.

*/

/*---------------------------------------------------------------------*
*        Error Procedures                                              *
*---------------------------------------------------------------------*/

/*
     (1) Output is automatically converted to E format when the number
         to be converted overflows the alloted field or when the deci-
         mal parameter exceeds the field width, provided the field has
         room for one digit plus signed exponent.  When this happens,
         the decimal is selected to give the largest number of signi-
         ficant figures that will fit in the field.  If the width is
         too small for even that, the field is filled with asterisks.

      (2) Abexits: 40 if invalid combination of parameters specified,
         e.g. hex or octal conversion with binary scale, 41 if internal
         check fails.
*/

/*---------------------------------------------------------------------*
*        Restrictions:                                                 *
*---------------------------------------------------------------------*/

/*
      1) Program assumes 8 bits per byte.

      2) Program assumes sign is stored in high-order bit.
*/

/*---------------------------------------------------------------------*
*        Global definitions:                                           *
*---------------------------------------------------------------------*/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "sysdef.h"
#include "rksubs.h"
#include "rkarith.h"

#if WIDE_SIZE > 20
#error "Bigs table must have at least WIDE_SIZE entries"
#endif
#define MxbsMask 0x3f000000   /* Mask for maximum binary scale */
#define MxdsMask 0x000f0000   /* Mask for maximum decimal scale */
#define KpfxMask 0x00700000   /* Mask for prefix controls */
#define KtypMask 0x000001c0   /* Mask for variable type controls */
#define NTypes       8        /* Number of defined type codes */
#define KaszMask     7        /* Size code mask */
#define KdsfShft    23        /* Shift to make 0/1 from RK_DSF bit */
#define KpfxShft    20        /* Shift to make 0/1 from RK_PLUS bit */

/* Globals replacing those in RK or RKC in full apps */
struct RKPndef {
   short length;
   byte expwid;
   } RKPn;

/*=====================================================================*
*                              wnclenPn                                *
*                                                                      *
*  Synopsis:  int wnclenPn(ui32 cc)                                    *
*                                                                      *
*  Returns the length of a variable encoded by one of the wbcdin/      *
*  wbcdwt fixed-point data type codes.                                 *
*=====================================================================*/

int wnclenPn(ui32 cc) {

   /* Size of each input variable type as a function of size code */
   static const int nisz0Pn[] = { sizeof(int), sizeof(byte),
      sizeof(short), sizeof(int), sizeof(ui32), sizeof(long),
      sizeof(ui64), 0 };

   return nisz0Pn[cc >> RK_TS & KaszMask];

   } /* End wnclenPn() */


/*=====================================================================*
*                              wbcdwtPn                                *
*=====================================================================*/

void wbcdwtPn(void *pitm, char *field, ui32 ic) {

   char *pd;                  /* Ptr to last output digit */
   char *pde;                 /* Limiting value of pd in a loop */
   char *prd;                 /* Always ptr to rightmost digit */
   const char *upfx;          /* Used prefix */
   ui64 uarg;                 /* Argument converted to unsigned wide */
   int  bs;                   /* Binary scale */
   int  dec;                  /* Decimal parameter */
   int  lpfx;                 /* Length of prefix additions */
   int  mode;                 /* Conversion mode */
   int  kasz;                 /* Kind of argument size */
   int  kpfx;                 /* Kind of prefix */
   int  width;                /* Width minus 1 */
   char fill;                 /* Fill character */

   /* Table of powers of ten less 1 */
#ifdef HAS_I64
#if LSIZE == 8
   static const ui64 BigsPn[WIDE_SIZE-1] = { 9UL, 99UL, 999UL, 9999UL,
      99999UL, 999999UL, 9999999UL, 99999999UL, 999999999UL,
      9999999999UL, 99999999999UL, 999999999999UL, 9999999999999UL,
      99999999999999UL, 999999999999999UL, 9999999999999999UL,
      99999999999999999UL, 999999999999999999UL, 9999999999999999999UL
      };
#else
   static const ui64 BigsPn[WIDE_SIZE-1] = { 9ULL, 99ULL, 999ULL,
      9999ULL, 99999ULL, 999999ULL, 9999999ULL, 99999999ULL,
      999999999ULL, 9999999999ULL, 99999999999ULL, 999999999999ULL,
      9999999999999ULL, 99999999999999ULL, 999999999999999ULL,
      9999999999999999ULL, 99999999999999999ULL,
      999999999999999999ULL, 9999999999999999999ULL };
#endif
#elif BYTE_ORDRE < 0
   static const ui64 BigsPn[WIDE_SIZE-1] = { {9,0}, {99,0}, {999,0},
      {9999,0}, {99999,0}, {999999,0}, {9999999,0}, {99999999,0},
      {999999999,0}, {1410065407,2}, {1215752191,23},
      {3567587327UL,232}, {1316134911,2328}, {276447231,23283},
      {2764472319UL,232830}, {1874919423,2328306},
      {1569325055,23283064}, {2808348671UL,232830643},
      {2313682943UL,2328306436UL} };
#else
   static const ui64 BigsPn[WIDE_SIZE-1] = { {0,9}, {0,99}, {0,999},
      {0,9999}, {0,99999}, {0,999999}, {0,9999999}, {0,99999999},
      {0,999999999}, {2,1410065407}, {23,1215752191},
      {232,3567587327UL}, {2328,1316134911}, {23283,276447231},
      {232830,2764472319UL}, {2328306,1874919423},
      {23283064,1569325055}, {232830643,2808348671UL},
      {2328306436UL,2313682943UL} };
#endif
   /* Table of hex (and decimal) output characters */
   static const char hextucPn[16] = "0123456789ABCDEF";
   static const char hextlcPn[16] = "0123456789abcdef";
   /* Possible number prefixes and offsets vs control bits */
   static const char pfx0Pn[] = { " 0X+0X-0X 0x+0x-0x" };
   static const char pfxoPn[] = { 1,3,0,3,6,6,6,6 };

/* Extract parameters and set to start with rightmost digit.
*  N.B.  On entry to common output code, pd should point one
*  char to left of leftmost char already written to output.  */

   bs  = (int)((ic & MxbsMask) >> RK_BS);       /* Binary scale */
   mode = (int)ic & RK_IORF;                    /* Mode */
   kasz = (int)(ic >> RK_TS) & (NTypes-1);      /* Item size */
   width = (int)ic & RK_MXWD;                   /* Width */
   pd = prd = field + width;     /* Start with rightmost digit */

/*---------------------------------------------------------------------*
*              Fetch data item and record sign if signed               *
*---------------------------------------------------------------------*/

   if (ic & RK_NUNS) {                          /*------------------*/
      switch (kasz) {                           /* ITEM IS UNSIGNED */
      case 1:                    /* byte */     /*------------------*/
#ifdef HAS_I64
         uarg = (ui64)(*(byte *)pitm);
#else
         uarg.hi = 0, uarg.lo = (ui32)(*(byte *)pitm);
#endif
         break;
      case 2:                    /* unsigned short */
#if ISIZE == 2
      case 0:                    /* default int */
      case 3:                    /* 16-bit unsigned int */
#endif
#ifdef HAS_I64
         uarg = (ui64)(*(ui16 *)pitm);
#else
         uarg.hi = 0, uarg.lo = (ui32)(*(ui16 *)pitm);
#endif
         break;
#if ISIZE == 4
      case 0:                    /* default int */
      case 3:                    /* 32-bit unsigned int */
#endif
      case 4:                    /* ui32 */
#if LSIZE == 4
      case 5:                    /* 32-bit unsigned long */
#endif
#ifdef HAS_I64
         uarg = (ui64)(*(ui32 *)pitm);
#else
         uarg.hi = 0, uarg.lo = *(ui32 *)pitm;
#endif
         break;
#if LSIZE == 8
      case 5:                    /* 64-bit unsigned long */
#endif
      case 6:                    /* ui64 */
         uarg = *(ui64 *)pitm;
         break;
      case 7:                    /* Reserved for 128-bit */
         abexit(40);             /* Not yet implemented */
         } /* End kasz switch */

   } else {
      ui32 icz = ic & RK_MZERO;                 /*----------------*/
      switch (kasz) {                           /* ITEM IS SIGNED */
      case 1:                    /* char */     /*----------------*/
      {  union {
            schr sitm;
            byte uitm;
            } u1;
         u1.sitm = *(schr *)pitm;
         if (u1.sitm < 0) {
            ic |= RK_NEGV;
            u1.sitm = u1.uitm == (icz>>24) ? 0 : -u1.sitm;
            }
#ifdef HAS_I64
         uarg = (ui64)u1.uitm;
#else
         uarg.hi = 0, uarg.lo = (ui32)u1.uitm;
#endif
         }
         break;
      case 2:                    /* short */
#if ISIZE == 2
      case 0:                    /* Default int */
      case 3:                    /* 16-bit signed int */
#endif
      {  union {
            si16 sitm;
            ui16 uitm;
            } u2;
         u2.sitm = *(si16 *)pitm;
         if (u2.sitm < 0) {
            ic |= RK_NEGV;
            u2.sitm = u2.uitm == (icz>>16) ? 0 : -u2.sitm;
            }
#ifdef HAS_I64
         uarg = (ui64)u2.uitm;
#else
         uarg.hi = 0, uarg.lo = (ui32)u2.uitm;
#endif
         }
         break;
#if ISIZE == 4
      case 0:                    /* Default int */
      case 3:                    /* 32-bit signed int */
#endif
      case 4:                    /* si32 */
#if LSIZE == 4
      case 5:                    /* 32-bit signed long */
#endif
      {  union {
            si32 sitm;
            ui32 uitm;
            } u4;
         u4.sitm = *(si32 *)pitm;
         if (u4.sitm < 0) {
            ic |= RK_NEGV;
            u4.sitm = u4.sitm == icz ? 0 : -u4.sitm;
            }
#ifdef HAS_I64
         uarg = (ui64)u4.uitm;
#else
         uarg.hi = 0, uarg.lo = (ui32)u4.uitm;
#endif
         }
         break;
#if LSIZE == 8
      case 5:                    /* 64-bit signed long */
#endif
      case 6:                    /* si64 */
      {  si64 sitm = *(si64 *)pitm;
#ifdef HAS_I64
         if (sitm < 0) {
            ic |= RK_NEGV;
            sitm = sitm == ((si64)icz<<32) ? 0 : -sitm;
            }
         uarg = (ui64)sitm;
#else
         if (sitm.hi < 0) {
            ic |= RK_NEGV;
            if (sitm.hi == icz && sitm.lo == 0)
               sitm.hi = 0; else sitm = jnsw(sitm);
            }
         uarg.hi = (ui32)sitm.hi, uarg.lo = sitm.lo;
#endif
         }
         break;
      case 7:                    /* Reserved for 128-bit */
         abexit(40);             /* Not yet implemented */
         } /* End kasz switch */
      } /* End signed item */

   /* Check for forced E format from some caller */
   if (ic & RK_FORCE) goto force;

   /* Setup for adding sign or blank prefix later */
   kpfx = (int)((ic & KpfxMask) >> KpfxShft);   /* Prefix controls */
   lpfx = kpfx != 0;                        /* Size of sign prefix */
   upfx = pfx0Pn + pfxoPn[kpfx];

/* Special short path for zero argument */

   if (quw(uarg) == 0) {
      *pd-- = '0';
      goto finout;
      }

/* Switch according to conversion format by decreasing frequency */

   if (mode == RK_IORF) {

/*---------------------------------------------------------------------*
*                    I or F Format Decimal Output                      *
*---------------------------------------------------------------------*/

      ui64 targ,frac;         /* Integer part of uarg, fraction */
      int  texp;              /* Power of ten for overflow checking */
      int  kdsf;              /* TRUE if RK_DSF bit set */

/* Force exponential format if data would overflow user's field.
*  Noting that texp is one less than the available space for digits,
*  there is no problem if texp >= (WIDE_SIZE-1).  */

      dec = (int)((ic & MxdsMask) >> RK_DS);
      kdsf = (int)(ic >> KdsfShft) & 1;
      targ = jsruw(uarg, bs);
      texp = width - lpfx - kdsf;
      if (!(ic & RK_AUTO)) texp -= dec;
      if (texp < (WIDE_SIZE-1)) {
#ifdef HAS_I64
         if (texp < 0 || targ > BigsPn[texp])
            goto force;
#else
         /* Once it is known that targ.hi is not all ones, it
         *  it OK to add the carry to it for the overflow test.  */
         if (texp < 0 || ~targ.hi == 0 ||
            targ.hi + (targ.lo > BigsPn[texp].lo) > BigsPn[texp].hi)
            goto force;
#endif
         }

/* Handle DSF ("decimal-scale-forced") option--this inserts the
*  decimal in a specified place without scaling the argument--
*  useful e.g. to print volts when the argument is in millivolts.  */

      if (kdsf) {
         if (dec < 1 || ic & (RK_UFLW|RK_AUTO)) abexit(40);
         /* If there are fraction bits, round now */
#ifdef HAS_I64
         if (bs > 0) targ += jsruw(uarg, bs-1) & 1;
#else
         if (bs > 0) {
            ui64 rnd = jsruw(uarg, bs-1);
            targ = jaul(targ, rnd.lo & 1);
            }
#endif
         /* Generate and store decimals */
         pde = prd - dec + 1;
         while (pd > pde) {
            ui32 digit;
            targ = jduwb(targ, 10, &digit);
            *pd-- = hextucPn[digit];
            }
         *pd-- = '.';         /* Insert decimal point */
         } /* End RK_DSF processing */

/* Handle fixed-point data with fraction bits or decimal output.
*  N.B.  It is valid to enter here with binary scale 0 when 'ic'
*  specifies converting an integer with some decimals.  Because
*  left shift may operate modulo 32|64 on some machines (Intel),
*  a special test is needed for the case bs == 0.  */

      else {
         if (ic & (MxbsMask | MxdsMask | RK_AUTO)) {

            ui32 nosignif = quw(targ) ? 0 : RK_UFLW;
            frac = bs ? jsluw(uarg, BITSPERUI64 - bs) : jeul(0);

            /* If automatic decimal placement requested, now calculate
            *  the dec parameter, but make it not larger than the value
            *  supplied by the caller.  Must omit the log10 call if
            *  targ is 0, which can happen if there is a fraction but
            *  no whole number.  l10 can be one short at exact powers
            *  of ten, but presumably it will never be high at a power
            *  of ten minus one.  */
            if (ic & RK_AUTO) {
               int  l10;         /* No. of integer digits minus 1 */
               int  tdec;        /* Calculated new value of dec */
               if (quw(targ)) {
                  l10 = (int)log10(uwdbl(targ));
#ifdef HAS_I64
                  if (targ > BigsPn[l10]) l10 += 1;
#else
                  if (~targ.hi == 0 || targ.hi +
                        (targ.lo > BigsPn[l10].lo) > BigsPn[l10].hi)
                     l10 += 1;
#endif
                  }
               else
                  l10 = 0;
               /* Bug fix, 09/26/17:  Make this test agree with stated
               *  policy:  Leave room for blank if arg >= 0, otherwise
               *  use that left space for negative sign.  */
               tdec = width - l10 - 1;
               /* This is a test for a coding error:  Negative tdec
               *  should be impossible at this point because of the
               *  earlier test for exponential forcing.  */
               if (tdec < 0) abexit(199);
               if (tdec == 1) tdec = 0;
               if (dec == 0 || tdec < dec) dec = tdec;
               } /* End auto-decimal setting */

            /* Insert decimal point and fraction digits.
            *  Design note:  Rounding would be easier if multiply arg
            *  by 10^(dec-1) and use division as above for RK_DSF case,
            *  but in worst case would need 128-bit arithmetic, plus
            *  division is slower, so we do it this way. */
            pd = pde = prd + 1 - dec;  /* Location of decimal */
            if (dec > 0) {
               *pd++ = '.';            /* Insert decimal point */
               while (pd <= prd) {
                  ui32 newd;
                  frac = jmuwb(frac, 10, &newd);
                  if (newd) nosignif = 0;
                  *pd++ = hextucPn[newd];
                  } /* End converting fraction */
               }

            /* Perform rounding based on value of remaining fraction.
            *  N.B.  If dec < 2, above loop was skipped, pd is already
            *  <= pde, and fraction rounding will be skipped.  */
            if (uwhi(frac) & 0x80000000) {
               nosignif = 0;
               while (--pd > pde) {
                  if ((*pd += 1) <= '9') goto DoneRounding;
                  *pd = '0';
                  }
               /* Carry rounding to integer part.  This provides one
               *  small chance it may be necessary to force EFMT.  */
               targ = jaul(targ, 1);
               texp = pde - 1 - lpfx - field;
               if (texp >= (WIDE_SIZE-1)) goto DoneRounding;
               if (texp < 0 ||
#ifdef HAS_I64
                     targ > BigsPn[texp]
#else
                     ~targ.hi == 0 || targ.hi +
                     (targ.lo > BigsPn[texp].lo) > BigsPn[texp].hi
#endif
                     ) {
                  /* Rounding flowed into prefix.  If a minus sign is
                  *  needed, force EFMT, otherwise see whether remov-
                  *  ing the prefix (plus or blank) makes it fit.  */
                  if (ic & RK_NEGV) goto force;
                  texp += lpfx, lpfx = 0;
                  if (texp < 0 ||
#ifdef HAS_I64
                        targ > BigsPn[texp]
#else
                        ~targ.hi == 0 || targ.hi +
                        (targ.lo > BigsPn[texp].lo) > BigsPn[texp].hi
#endif
                        ) goto force;
                  }
DoneRounding:  ;
               } /* End decimal rounding */

            /* If RK_UFLW and no significant digits, force E format */
            if (ic & nosignif) goto force;

            pd = prd - dec;            /* Setup for integer output */
            } /* End general-fixed-point */
         } /* End not RK_DSF */

/* Convert integer portion to decimal by usual algorithm.
*  pd should point to rightmost character to be written.  */

      do {
         ui32 digit;
         if (pd < field) abexit(199);  /* Just in case */
         targ = jduwb(targ, 10, &digit);
         *pd-- = hextucPn[digit];
         } while (quw(targ));

      } /* End I or F format */

/*---------------------------------------------------------------------*
*                         Hexadecimal output                           *
*                                                                      *
*  N.B.  In wbcdwtPn it is caller's choice whether to treat values     *
*  for hex output as signed or unsigned--use RK_NUNS bit to treat as   *
*  unsigned.  The default behavior is to print the number of digits    *
*  needed to display the item.  This can be overridden with RK_NZTW    *
*  to display leading zeros to fill out the size of the item type.     *
*---------------------------------------------------------------------*/

   else if (mode == RK_HEXF) {

      const char *hextab = (ic & RK_NZLC) ? hextlcPn : hextucPn;
      ui64 targ = uarg;       /* Might need uarg for force */
      int navl;               /* Number of bytes available */
      int nhex;               /* Number of nibbles */

      /* Check for invalid combinations of options */
      if (ic & (MxbsMask|MxdsMask)) abexit(40);

      /* Calculate length, force E format if not enough space
      *  for result, determine what prefix to use */
      if (ic & RK_NZLC) upfx += 9;
      if (ic & RK_NZ0X) lpfx += 2;
      navl = width + 1 - lpfx;
      nhex = 2*wnclenPn(ic);
      if (navl < nhex) {
         if (ic & RK_NZTW || navl <= 0) abexit(41);
         else nhex = navl;
         }

      /* Perform the hex conversion */
      do {
#ifdef HAS_I64
         *pd-- = hextab[targ & 15];
         targ >>= 4;
#else
         *pd-- = hextab[targ.lo & 15];
         targ = jsruw(targ, 4);
#endif
         } while (--nhex && (ic & RK_NZTW || quw(targ)));

      /* In case !RK_NZTW (but skip the 'if'), one final check
      *  for overflow is necessary ...  */
      if (quw(targ)) goto force;

      } /* End hex conversion */

/*---------------------------------------------------------------------*
*                            Octal output                              *
*                                                                      *
*  N.B.  It is caller's choice whether octal values are treated as     *
*  signed or unsigned--use RK_NUNS bit to treat as unsigned.  Unlike   *
*  w/hex output, octal behaves like decimal, generates just the number *
*  of digits needed to express the value.  The RK_NZTW bit is treated  *
*  as an error here, but could be implemented as for hex if needed.    *
*---------------------------------------------------------------------*/

   else if (mode == RK_OCTF) {

      ui64 targ = uarg;       /* Might need uarg for force */
      int navl;               /* Number of bytes available */

      /* Check for invalid combinations of options */
      if (ic & (MxbsMask|RK_DSF|RK_NZTW|MxdsMask)) abexit(40);

      /* Calculate available length */
      if (ic & RK_Oct0) lpfx += 1;
      navl = width + 1 - lpfx;

      /* Perform the octal conversion */
      do {
#ifdef HAS_I64
         *pd-- = hextucPn[targ & 7];
         targ >>= 3;
#else
         *pd-- = hextucPn[targ.lo & 7];
         targ = jsruw(targ, 3);
#endif
         } while (--navl && quw(targ));

      /* If output didn't fit in navl chars, force E format */
      if (quw(targ)) goto force;

      } /* End octal conversion */

/*---------------------------------------------------------------------*
*         Produce exponential format if explicitly requested           *
*                                                                      *
*  N.B.  The RK_NPAD0 bit is quietly ignored--bcdoutPn can't handle    *
*  it, and there is really no reason why it would be needed anyway.    *
*---------------------------------------------------------------------*/

   else {
      goto doexp;
      }

/*---------------------------------------------------------------------*
*  Final details common to decimal, hex, octal, and zero conversions.  *
*  On entry to this code, pd should always point one char to left of   *
*  last char already written to output.  (Code for inserting prefix    *
*  is duplicated in order to reduce number of 'if' statements.)        *
*---------------------------------------------------------------------*/

finout:

/* Point pd at data and check space one last time, just in case */

   if (++pd - lpfx < field) abexit(199);

/* Select fill character and store length of result */

   if (ic & RK_NPAD0) {
      fill = '0', RKPn.length = width;
      }
   else
      fill = ' ', RKPn.length = prd - pd + lpfx;

/* If left-justifying, start with prefix, if any, then data,
*  then fill on right (filling on left would give same output
*  as right-justifying, this way we get another, although
*  possibly  useless, possibility).  Note that the man page
*  for printf() etc. specifies that zero-filling is ignored
*  with left-justification.  */

   if (ic & RK_LFTJ) {
      int ipfx;
      for (ipfx=0; ipfx<lpfx; ++ipfx) *field++ = upfx[ipfx];
      if (pd > field) while (pd <= prd) *field++ = *pd++;
      while (field <= prd) *field++ = fill;
      }

/* Data are normally right-justified:  if padding with zeros,
*  padding goes to the right of the prefix, but if padding with
*  blanks, padding goes to the left of the prefix */

   else if (ic & RK_NPAD0) {
      int ipfx;
      for (ipfx=0; ipfx<lpfx; ++ipfx) *field++ = upfx[ipfx];
      while (pd > field) *(--pd) = fill;
      }

   else {
      while (lpfx) *(--pd) = upfx[--lpfx];
      while (pd > field) *(--pd) = fill;
      }

   return;

/*---------------------------------------------------------------------*
*  Here to generate E format if requested or force it on overflow.     *
*---------------------------------------------------------------------*/

force:
   ic &= ~(MxdsMask|RK_LSPC|RK_PLUS);  /* Get max decimals on force */
   ic |= (RK_FORCE | RK_AUTO);
doexp:   {
      double darg = uwdbl(uarg);
      if (bs) darg /= uwdbl(jsluw(jeul(1),bs));
      ic &= ~(MxbsMask|RK_IORF|KtypMask);
      bcdoutPn(ic, field, darg);
      } /* End doexp local scope */

   return;

   } /* End wbcdwtPn() */
