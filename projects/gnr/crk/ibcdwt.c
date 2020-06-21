/* (c) Copyright 1966-2011, The Rockefeller University *11115* */
/* $Id: ibcdwt.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                              ibcdwt.c                                *
*                                                                      *
*  ROCKS 32-bit fixed-point numerical output conversion routines       *
*  Adapted from IBM Assembler versions by G.N. Reeke                   *
*                                                                      *
*  V3A has the following features:                                     *
*  (1) Entire calculation is performed with fixed point arithmetic,    *
*      so decimal rounding can be exact.                               *
*  (2) Can predict overflow with near-certainty early in the           *
*      algorithm.                                                      *
*  (3) Outputs directly to user field, saving a memcpy().              *
*  (4) Internal function calls are eliminated to save time.            *
*  (5) All long types changed to si32,ui32 in anticipation of 64-bit   *
*      computing.                                                      *
*                                                                      *
*  Note:  This routine is retained for compatibility with old code.    *
*  It could not readily be expanded to handle 64-bit fixed-point       *
*  output because of the 32-bit third argument.  The replacement is    *
*  wbcdwt.c, which uses a pointer to handle all argument sizes.        *
************************************************************************
*  V00, xx/xx/66, GNR - IBM 360 Assembler version                      *
*  V1A, 10/17/88, GNR - Convert from my IBM Assembler version          *
*  V2A, 02/12/89, GNR - Move BCDIN, IBCDIN into separate file          *
*  Rev, 04/19/89, GNR - Incorporate rkxtra.h                           *
*  V2B, 06/22/89, GNR - Permit hexout width < length if zeros          *
*  Rev, 05/14/91, GNR - Make width int for NCUBE, revise GFP test      *
*  Rev, 11/09/91, GNR - Correct GFP bug when scale = 31                *
*  Rev, 08/26/92, GNR - Add octal output facilities                    *
*  Rev, 03/11/93, GNR - Correct bug in 6/22/89 hexout fix              *
*  Rev, 01/27/97, GNR - Add decout(), finout(), ubcdwt()               *
*  Rev, 02/23/03, GNR - Decimal in integer output inserts w/o scaling  *
*  Rev, 07/08/03, GNR - Autoscale (not EFMT forced) inserts a blank    *
*  Rev, 09/13/03, GNR - Decimal with autoscale is max decimals         *
*  V3A, 05/19/07, GNR - Separate file, new all-fixed-point algorithm,  *
*                       exact decimal rounding, [us]i32 arg types      *
*  ==>, 05/29/07, GNR - Last date before committing to svn repository  *
*  Rev, 01/04/09, GNR - Make compatible with 64-bit compilation        *
*  Rev, 09/01/11, GNR - Update comments relating to 'expwid'.          *
***********************************************************************/

/*---------------------------------------------------------------------*
*        Usage:                                                        *
*---------------------------------------------------------------------*/

/*
   For signed 32-bit integer (or general fixed point) to characters:
         void ibcdwt(ui32 ic, char *field, si32 iarg)
   For unsigned 32-bit integer (or general fixed point) to characters:
         void ubcdwt(ui32 ic, char *field, ui32 uarg);

   'ic' is the processing code described below.
   'field' is an array into which the result is placed.
   'iarg' or 'uarg' is the number to be converted.

   The operation code 'ic' is the sum of four quantities:
         ic = scale + dec + op + width - 1

      'scale' is required only for noninteger fixed-point variables.
         The scale code is (s << RK_BS), where RK_BS is 24 and 's'
         is the number of bits to the right of the binary point in
         the argument (maximum, 31).  Use the RK_GFP bit to obtain
         decimal-point insertion for integers (s == 0).

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
         0x80000000  RK_NEGV  Used internally to signal a negative arg.
         0x40000000  RK_FORCE Used to signal a force to E format.
         0x00008000  RK_GFP   General Fixed Point.  Argument is a
                              general fixed point value with 'scale'
                              fraction bits (only required if s==0).
         0x00004000  RK_LFTJ  Left Justify.  Output is left justified.
         0x00002000  RK_UFLW  Underflow control.  Switch to E format if
                              result would have no significant digits.
         0x00001000  RK_AUTO  Automatic Decimal.  Position of decimal
                              point is adjusted to give greatest number
                              of significant figures that will fit in
                              the output field after a single blank,
                              but not more than dec-1 decimals if dec>0.
         0x00000000  RK_EFMT  Generate E (exponential) format.
         0x00000600  RK_IORF  Generate I (integer) or F (decimal) for-
                              mat (I format = F format with 'd' = 0.)
         0x00000400  RK_OCTF  Perform O (octal) conversion of integers
                              (error if used with RK_GFP or s != 0).
         0x00000200  RK_HEXF  Perform H (hexadecimal) conversion
                              (error if used with RK_GFP or s != 0).
         0x00000100  RK_SNGL  Single precision.  With hex conversion,
                              suppresses output of high-order digits
                              of short integer arguments, otherwise
                              ignored.
         0x00000080  RK_BYTE  Byte precision.  With hex conversion,
                              suppresses output of high-order digits
                              of byte arguments, otherwise ignored.
         0x00000040  RK_PAD0  Pad with zeros.  Characters to left of
                              most significant digit (right of least-
                              significant digit if left-justified) are
                              padded with zeros.  Default is to pad
                              with blanks.

      'width' is the number of characters in the output field, less
         one.  (The maximum width is RK_MXWD+1 = 32.  However, the
         maximum number of digits is given by OUT_SIZE and the re-
         mainder of the width can only be filled with blanks, signs,
         decimal points, exponents, etc.)

      Additionally, the global RK.expwid can be set by the user before
         a call to ibcdwt().  'expwid' is used only for E format out-
         put, and specifies the number of digits in the printed expo-
         nent (including sign).  If 'expwid' is zero or larger than
         EXP_SIZE, the number of digits needed to express the largest
         exponent that can be represented on the machine, 'expwid' is
         replaced by EXP_SIZE.  If 'expwid' is too small to represent
         the result, a larger value is substituted.

   Note that arguments of shorter types will be automatically
   coerced to the type required for conversion under ANSI C.
*/

/*---------------------------------------------------------------------*
*        Error Procedures                                              *
*---------------------------------------------------------------------*/

/*
      1) All control parameters are treated modulo their maximum
         allowed values.

      2) Output is automatically converted to E format when the number
         to be converted overflows the alloted field or when the deci-
         mal parameter exceeds the field width, provided the field has
         room for one digit plus signed exponent.  When this happens,
         the decimal is selected to give the largest number of signi-
         ficant figures that will fit in the field.  If the width is
         too small for even that, the field is filled with asterisks.

      3) Abexits: 40 if invalid combination of parameters specified,
         e.g. hex or octal conversion with GFP, 41 if internal check
         fails.
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
#include "rocks.h"
#include "rkxtra.h"
#include "rkarith.h"

#if UI32_SIZE > 10
#error "Bigs table must have at least UI32_SIZE entries"
#endif

/*=====================================================================*
*                               ubcdwt                                 *
*=====================================================================*/

void ubcdwt(ui32 ic, char *field, ui32 uarg) {

   char *pd;                  /* Ptr to last output digit */
   char *pde;                 /* Limiting value of pd in a loop */
   char *prd;                 /* Always ptr to rightmost digit */
   int  bs;                   /* Binary scale */
   int  mode;                 /* Conversion mode */
   int  negv;                 /* Variable will be marked negative */
   int  width;                /* Width minus 1 */
   char fill;                 /* Fill character */

   /* Table of hex (and decimal) output characters */
   static char hextab[16] = "0123456789ABCDEF";
   /* Table of powers of ten */
   static ui32 Bigs[UI32_SIZE] = { 9, 99, 999, 9999, 99999, 999999,
      9999999, 99999999, 999999999, UI32_MAX };

/* Extract sign and width and set to start with rightmost digit.
*  N.B.  On entry to common output code, pd should point one
*  char to left of leftmost char already written to output.  */

   negv = (int)(ic >> (BITSPERUI32 - 1)); /* Sign */
   width = (int)ic & RK_MXWD; /* Width */
   pd = prd = field + width;  /* Start with rightmost digit */

/* Special short path for zero argument */

   if (uarg == 0) {
      *pd-- = '0';
      goto finout;
      }

/* Extract bit scale and conversion mode */

   bs = (int)(ic >> RK_BS) & 31;
   mode = (int)ic & RK_IORF;

/* Switch according to conversion format by decreasing frequency */

   if (mode == RK_IORF) {

/*---------------------------------------------------------------------*
*                    I or F Format Decimal Output                      *
*---------------------------------------------------------------------*/

      ui32 targ;              /* Whole integer part of uarg */
      int  dec;               /* Decimal parameter */
      int  texp;              /* Power of ten for overflow checking */

/* Force exponential format if data would overflow user's field */

      dec = ((int)(ic >> RK_DS) & 15);
      targ = uarg >> bs;
      texp = width - negv - (ic & RK_AUTO ? 0 : dec);
      if (texp < 0 || targ > Bigs[min((UI32_SIZE-1),texp)])
         goto force;

/* Handle fixed-point data with fraction bits or decimal output.
*  N.B.  It is valid to enter here with binary scale 0 when 'ic'
*  specifies converting an integer with some decimals.  Because
*  left shift may operate modulo 32 on some machines (Intel),
*  a special test is needed for the case bs == 0.  */

      if (ic & (0x7F0F0000 | RK_GFP | RK_AUTO)) {

         ui32 frac = bs ? uarg << (BITSPERUI32 - bs) : 0;
         ui32 nosignif = targ ? 0 : RK_UFLW;

         /* If automatic decimal placement requested, now calculate
         *  the dec parameter, but make it not larger than the value
         *  supplied by the caller.  Must omit the log10 call if targ
         *  is 0, which can happen if there is a fraction but no whole
         *  number.  l10 can be one short at exact powers of ten, but
         *  presumably it will never be high at a power of ten minus
         *  one.  */
         if (ic & RK_AUTO) {
            int  l10;         /* No. of integer digits minus 1 */
            int  tdec;        /* Calculated new value of dec */
            if (targ) {
               l10 = (int)log10((double)targ);
               if (targ > Bigs[l10]) l10 += 1;
               }
            else
               l10 = 0;
            tdec = width - negv - l10 - 1;
            /* This is a test for a coding error:  Negative tdec should
            *  be impossible at this point because of the earlier test
            *  for exponential forcing.  */
            if (tdec < 0) abexit(199);
            if (tdec == 1) tdec = 0;
            if (dec == 0 || tdec < dec) dec = tdec;
            } /* End auto-decimal setting */

         /* Insert decimal point and fraction digits */
         pd = pde = prd + 1 - dec;  /* Location of decimal */
         if (dec > 0) {
            *pd++ = '.';            /* Insert decimal point */
            while (pd <= prd) {
               ui64 prod = jmuw(frac, 10);
               ui32 newd = uwhi(prod);
               if (newd) nosignif = 0;
               *pd++ = hextab[newd];
               frac = uwlo(prod);
               } /* End converting fraction */
            }

         /* Perform rounding based on value of remaining fraction.
         *  N.B.  If dec < 2, above loop was skipped, pd is already
         *  <= pde, and fraction rounding will be skipped.  */
         if (frac & 0x80000000) {
            nosignif = 0;
            while (--pd > pde) {
               if ((*pd += 1) <= '9') goto DoneRounding;
               *pd = '0';
               }
            /* Carry rounding to integer part.  This provides one
            *  small chance it may be necessary to force EFMT.  */
            targ += 1;
            texp = pde - 1 - field;
            if (targ > Bigs[min((UI32_SIZE-1),texp)])
               goto force;
DoneRounding: ;
            } /* End decimal rounding */

         /* If RK_UFLW and no significant digits, force E format */
         if (ic & nosignif) goto force;

         pd = prd - dec;            /* Setup for integer output */
         } /* End general-fixed-point */

/* Convert integer portion to decimal by usual algorithm.
*  pd should point to rightmost character to be written.
*  It is not clear an unsigned 32-bit version of div() is
*  always available, so do the division explicitly.  */

      do {
         ui32 q = targ/10;
         if (pd < field) abexit(199);  /* Just in case */
         *pd-- = hextab[targ - q*10];
         targ = q;
         } while (targ);

      } /* End I or F format */

/*---------------------------------------------------------------------*
*                         Hexadecimal output                           *
*                                                                      *
*  Hex output treats all values as unsigned--sign bit of negatives     *
*  appears as an ordinary digit--unless RK_NEGF passed from ibcdwt().  *
*---------------------------------------------------------------------*/

   else if (mode == RK_HEXF) {

      int  nhex;                    /* Number of bytes */

      /* Check for invalid combinations of options */
      if (ic & (RK_BSCL - RK_D | RK_GFP | RK_UFLW | RK_AUTO))
         abexit(40);

      /* Figure number of nibbles to convert and force E format
      *  if not enough space for result  */
      nhex = (ic & RK_BYTE) ? (2*BSIZE) :
         ((ic & RK_SNGL) ? (2*HSIZE) : (2*I32SIZE));
      if (nhex > width + 1 - negv) goto force;

      while (nhex--) {
         *pd-- = hextab[uarg & 15];
         uarg >>= 4;
         }

      } /* End hex conversion */

/*---------------------------------------------------------------------*
*                            Octal output                              *
*                                                                      *
*  Octal output treats all values as unsigned--sign bit of negatives   *
*  appears as an ordinary digit--unless RK_NEGF passed from ibcdwt().  *
*---------------------------------------------------------------------*/

   else if (mode == RK_OCTF) {

      int avdig;                    /* Available digits */

      /* Check for invalid combinations of options */
      if (ic & (RK_BSCL - RK_D | RK_GFP | RK_UFLW | RK_AUTO))
         abexit(40);

      /* Force E format if not enough space for result */
      avdig = width - negv;
      if (avdig < 0 || (avdig < 11 && uarg >= 8 << (3*avdig)))
         goto force;

      do {
         *pd-- = hextab[uarg & 7];
         uarg >>= 3;
         } while (uarg);

      } /* End octal conversion */

/*---------------------------------------------------------------------*
*         Produce exponential format if explicitly requested           *
*---------------------------------------------------------------------*/

   else
      goto doexp;

/*---------------------------------------------------------------------*
*  Final details common to decimal, hex, octal, and zero conversions.  *
*  On entry to this code, pd should always point one char to left of   *
*  last char already written to output.                                *
*---------------------------------------------------------------------*/

finout:

/* Put in minus sign */

   if (ic & RK_NEGV) {
      if (pd < field) abexit(199);     /* Just in case */
      *pd-- = '-';
      }

/* Store length of result */

   RK.length = prd - (pd += 1);

/* Perform left-justification if requested */

   fill = (ic & RK_PAD0) ? '0' : ' ';
   if (ic & RK_LFTJ) {
      if (pd > field) while (pd <= prd) *field++ = *pd++;
      while (field <= prd) *field++ = fill;
      }

/* Data are right-justified, fill on left as needed */

   else while (pd > field) *(--pd) = fill;

   return;

/*---------------------------------------------------------------------*
*  Here to generate E format if requested or force it on overflow.     *
*  N.B.  When RK_FORCE bit is set, bcdout() will allow auto-decimal    *
*        to fill leftmost byte (no blank space).                       *
*---------------------------------------------------------------------*/

force:
      ic |= (RK_FORCE | RK_AUTO);
doexp:   {
      double darg = (double)uarg;
      if (bs) darg /= (double)(1UL << bs);
      bcdout(ic & ~((BITSPERUI32-1) << RK_BS | RK_GFP | RK_IORF),
         field, darg);
      } /* End doexp local scope */

   return;

   } /* End ubcdwt() */

/*=====================================================================*
*                               ibcdwt                                 *
*=====================================================================*/

void ibcdwt(ui32 ic, char *field, si32 iarg) {

   if (iarg < 0)
      ubcdwt(ic | RK_NEGV, field, (ui32)(-iarg));
   else
      ubcdwt(ic, field, (ui32)iarg);

   } /* End ibcdwt() */

