/* (c) Copyright 1966-2009, The Rockefeller University *11116* */
/* $Id: bcdin.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               bcdin.c                                *
*              ROCKS numerical input conversion routines               *
*                                                                      *
*  Note 05/24/09:  ibcdin, ubcdin and the ivckmsg error routines were  *
*  removed to separate files because in 64-bit usage they may be used  *
*  in varying combinations. ibcdin() and ubcdin() will be replaced by  *
*  wbcdin(). Routines hexin, decin, previously static, have been made  *
*  externally visible for use by ibcdin, etc.                          *
************************************************************************
*  V00, xx/xx/66, GNR - IBM 360 Assembler version                      *
*  V1A, 10/17/88, GNR - Convert from my IBM Assembler version          *
*  V1B, 01/08/89, GNR - Add RK_BYTE bit to IBCDIN call                 *
*  V2A, 02/12/89, GNR - Separate from BCDOUT, IBCDWT                   *
*  Rev, 04/19/89, GNR - Incorporate rkxtra.h                           *
*  Rev, 01/16/92, GNR - Correct GFP bug when scale = 31                *
*  Rev, 01/25/97, GNR - Add RK_ZTST and DECIN, UBCDIN functions        *
*  V2B, 01/25/03, GNR - Add forced decimal scaling                     *
*  V2C, 03/08/08, GNR - More detailed error messages                   *
*  ==>, 03/16/08, GNR - Last date before committing to svn repository  *
*  Rev, 02/16/09, GNR - Perform value adjustment before overflow test  *
*  Rev, 05/24/09, GNR - Remove ivckmsg, ibcdin, ubcdin to sep files    *
***********************************************************************/

/*---------------------------------------------------------------------*
*        Usage:                                                        *
*---------------------------------------------------------------------*/

/*
   For characters to double precision floating point:
         double bcdin(ui32 ic, char *field)

   'ic' is the processing code described below.
   'field' is an array containing the string to be converted.

   The operation code 'ic' is the sum of four quantities:
         ic = dec + op + width - 1

      'dec' specifies decimal scaling. If the RK_DSF bit (=0x00800000)
         is set, scaling is forced, i.e. the value entered by the user
         is always multipled by the scale.  If RK_DSF is not set, sca-
         ling only occurs when no decimal point is found in the input.
         'dec' is equal to (d << RK_DS & 0x007f0000), where RK_DS is
         16.  The scale is (10**(-d)) with (-64 < d <= 63) (i.e. 'dec'
         is 7 bits including the 3 bits used for other flags by the
         output routines).  Thus, positive values of 'd' correspond to
         the number of places to the right of the inserted decimal when
         no decimal is found in the input.  Users normally expect 'd'
         to be 0.

      'op' specifies the operation to be performed.  It is made
         up by adding the desired codes from the following table.
         The argument types and format codes are independent.

         Hexadecimal Defined
           Value      Name    Function
         ----------  -------  ------------------------------------------
         0x00004000  RK_ZTST  Zero Test.  Generate an error if the
                              numerical value of the result is zero.
         0x00002000  RK_CTST  Character Test. Generate an error if non-
                              numeric characters found in input string.
         0x00001000  RK_QPOS  Query Positive.  Generate an error if the
                              numerical value of the result is negative.
         0x00000600  RK_IORF  I (integer) format (IBCDIN,UBCDIN) or
                              F (decimal) format (BCDIN).  Exponential
                              format is also accepted when this format
                              code is used.
         0x00000200  RK_HEXF  H (hexadecimal) conversion.
         0x00000100  RK_SNGL  Single precision.  Checks whether over-
                              flow would occur if result truncated by
                              caller to a 32-bit real.  If input format
                              is hexadecimal, generates an error if
                              more digits are found than needed to
                              represent a number of single precision.

      'width' is the number of characters in the input field to be
         examined, less one.  (The maximum width is RK_MXWD+1 = 32.)

   Because portability is the goal, accuracy will be no better
   than can be achieved with ordinary double operations.
*/

/*---------------------------------------------------------------------*
*        Error Procedures:                                             *
*---------------------------------------------------------------------*/

/*
      1) All control parameters are treated modulo their maximum
         allowed values.

      2) Input errors generate calls to ivckmsg.  This calls ermark
         to mark the error if input was scanned, then calls cryout to
         print an appropriate message.  If an input value overflows,
         it is set to the largest value that can be expressed in the
         requested numeric type.  If an input value is zero and the
         RK_ZTST bit is set, it is set to a small positive value,
         possibly preventing a divide-by-zero error in the caller.
*/

/*---------------------------------------------------------------------*
*        Restrictions:                                                 *
*---------------------------------------------------------------------*/

/*
      1) Exponents larger than EXP_SIZE can not be handled.

      2) Program assumes 8 bits per byte.

      3) The representations of the characters '0' through
         '9' and 'A' through 'F' must be consecutive codes.
*/

/*---------------------------------------------------------------------*
*        Global definitions:                                           *
*---------------------------------------------------------------------*/

#include <ctype.h>
#include <math.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include "sysdef.h"
#include "rocks.h"
#include "rockv.h"
#include "rkxtra.h"

/* Copy of tens table so don't have to load bcdout for it */
static double tens[17] = {1E0,  1E1,  1E2,  1E3,  1E4,  1E5,  1E6,
   1E7, 1E8, 1E9, 1E10, 1E11, 1E12, 1E13, 1E14, 1E15, 1E16};
extern ui16 RK_Inflags;       /* Various flags */


/*=====================================================================*
*                                                                      *
*            Subroutine HEXIN -- Handle hexadecimal input              *
*                                                                      *
*=====================================================================*/

void hexin(ui32 ic, char *field, byte *value, int maxlen) {

   char *pd;                     /* Digits pointer */
   char *pf,*pfe;                /* Field pointer, field end */
   int digct = 0;                /* Number of digits found */
   int gotspace = FALSE;         /* TRUE when a space is found */
   int maxdig = 2*maxlen;        /* Maximum digits allowed */
   char bcdigits[2*DSIZE+4];     /* Work area--DSIZE is max(maxlen) */

/* First find out if there is an end-of-string signal.
*  This is basically for two reasons:
*  1) So that it is convenient to scan from right to left,
*     making it easy to insert zeros on the left when
*     there are not enough input characters.
*  2) So that if there is an overflow, the digits on the
*     left will be the ones not used.  */

   pd = bcdigits;
   pfe = field + (int)(ic & RK_MXWD);
   for (pf=field; pf<=pfe; pf++) {
      if (*pf == 0) break; }  /* Stop at end of scanned field */

/* Now scan right-to-left for digits and store them backwards
*  in bcdigits.  */

   for (pf--; pf>=field; pf--) {
      int digit = (int)*pf;   /* Pick up next digit */
      if (isxdigit(digit)) {
         if (gotspace) {
            ivckmsg0(field, IVCK_HEXFMT); break; }
         if (++digct > maxdig) {
            memcpy(bcdigits, "0x1", 3);
            memset(bcdigits+3, '0', maxdig);
            bcdigits[maxdig+3] = '\0';
            ivckmsg1(field, bcdigits);
            break; }
         else if (isdigit(digit)) *pd++ = digit - '0';
         else *pd++ = toupper(digit) - ('A' - 10);
         }
      else if (digit == ' ') gotspace = TRUE;
      else if (ic & RK_CTST) {
         ivckmsg0(field, IVCK_HEXFMT); break; }
      } /* End scan for hex characters */

/* If there weren't enough digits, finish it with zeros */

   while (++digct <= maxdig) *pd++ = 0;

/* Now move the digits into the value in the correct direction */

#if BYTE_ORDRE > 0            /* Handle high-order-first machines */
   while (pd > bcdigits) {
      *value = *--pd << 4;
      *value++ |= *--pd; }
#else                         /* Handle low-order-first machines */
   pd = bcdigits;
   while (pd<bcdigits+maxdig) {
      *value = *pd++;
      *value++ |= *pd++ << 4; }
#endif

   return;
   } /* End hexin() */


/*=====================================================================*
*                                                                      *
*       Subroutine DECIN -- Handle floating-point decimal input        *
*                                                                      *
*=====================================================================*/

double decin(ui32 ic, char *field) {

   register char *pf;               /* Field scan pointer */
   char *pfe;                       /* End of field */
   double value = 0.0;              /* Value accumulator */
   int decloc;                      /* Decimal location */
   int digct = 0;                   /* Digit count */
   int exp = 0;                     /* Exponent */
   int width = (int)(ic & RK_MXWD); /* Maximum width of input field */

   pfe = field + width;
   for (pf=field; pf<=pfe; pf++) {
      int digit = (int)*pf;
      if (digit == 0) break;     /* Stop at end of input string */
      if (isdigit(digit)) {      /* Digit found */
         value = 10.0*value + (double)(digit - '0');
         digct++;
         }
      else if (digit == '.') {   /* Decimal point found */
         if (RK_Inflags & DP_FOUND) ivckmsg0(field, IVCK_FORMAT);
         decloc = digct;
         RK_Inflags |= DP_FOUND;
         }
      else if (digit == '+') {   /* Plus sign found */
         if (digct > 0) goto expscan;
         if (RK_Inflags & SIGN_FOUND) ivckmsg0(field, IVCK_FORMAT);
         RK_Inflags |= SIGN_FOUND;
         }
      else if (digit == '-') {   /* Minus sign found */
         if (digct > 0) goto negexpscan;
         if (RK_Inflags & SIGN_FOUND) ivckmsg0(field, IVCK_FORMAT);
         if (ic & RK_QPOS) {
            ivckmsg0(field, IVCK_APPGE0); return 0.0; }
         RK_Inflags |= (SIGN_FOUND | MINUS_FOUND);
         }
      else if (toupper(digit) == 'E') {   /* Exponent found */
         if (digct > 0) {        /* A sign may follow */
            if (pf[1] == (char)'+') { ++pf; goto expscan; }
            if (pf[1] == (char)'-') { ++pf; goto negexpscan; }
            goto expscan;
            }
         if (ic & RK_CTST) ivckmsg0(field, IVCK_FORMAT);
         }
      else if ((digit != ' ') && (ic & RK_CTST)) /* Bad char found */
         ivckmsg0(field, IVCK_FORMAT);
      } /* End digit scan */
   goto scale;                   /* Skip over exponent scan */

/* Scan the exponent field */

negexpscan: RK_Inflags |= NEG_EXP;
expscan:
   for (pf++; pf<=pfe; pf++) {
      int digit = (int)*pf;
      if (digit == 0) break;
      if (isdigit(digit)) {      /* Digit found */
         exp = 10*exp + (digit - '0');
         RK_Inflags |= EXP_DIG_FOUND;
         }
      else if ((digit != ' ') && (ic & RK_CTST)) /* Bad char found */
         ivckmsg0(field, IVCK_BADEXP);
      } /* End exponent digit scan */
   if (!(RK_Inflags & EXP_DIG_FOUND))     /* Exponent but no digits */
      ivckmsg0(field, IVCK_BADEXP);
   if (RK_Inflags & NEG_EXP) exp = -exp;  /* Set exponent sign */

/* Apply combined decimal and exponent scales.  A precise method
*  of detecting floating-point overflow exceptions before they
*  can happen is difficult, and we content ourselves with a
*  reasonable approximation, which has the following properties:
*     1) The final exponent is estimated as the number of digits
*        in the value, minus the number after the decimal point,
*        plus any explicit exponent entered.  If this is less
*        than the exponent of the largest representable number,
*        the value is accepted.  Otherwise, an ivckmsg1 error
*        is issued.
*     2) The different ranges of floats and doubles are recognized.
*     3) The test is also applied to negative exponents to prevent
*        overflow during calculation of the scale factor.  This
*        effectively prevents one from making very small numbers
*        that would underflow.
*     4) The sign of the mantissa is ignored, i.e. it is assumed
*        that negative numbers have the same range as positive.
*  A few obvious shortcomings of this method are:
*     1) It will be fooled by leading zeros.
*     2) It won't permit legal numbers with the max exponent but
*        smaller than the largest representable value.
*     3) It doesn't catch overflows in the exponent itself (more
*        than UI32_SIZE digits).
*/

scale:
   if ((digct == 0) && (ic & RK_CTST)) /* No digits found */
      ivckmsg0(field, IVCK_FORMAT);    /* Mark number error */
   if (RK_Inflags & DP_FOUND)    /* Use decimal found in input */
      exp -= (digct - decloc);
   else
      ic |= RK_DSF;           /* Otherwise, use default dec */
   if (ic & RK_DSF) {         /* Use signed 7-bit ic decimal */
      si32 d = (si32)ic << 9;
      exp -= (int)SRA(d, 25); }
   if (exp) {                    /* Handle nonzero exponent */
      double factor;             /* Scale factor */
      int absexp = abs(exp);
      if (absexp > 16) {         /* Calculate scales > 10**16 */
         register double power;
         int expmax = (ic & RK_SNGL) ? FLT_EXP_MAX : DBL_EXP_MAX;
         int expmx0 = expmax;
         if (exp > 0) expmax -= digct;
         if (absexp > expmax) {  /* Handle potential overflow */
            char tmxer[EXP_SIZE+4];
            absexp = expmax;
            memcpy(tmxer, "1E", 2);
            wbcdwt(&expmx0, tmxer+2,
               RK_NUNS|RK_IORF|RK_LFTJ|EXP_SIZE-1);
            tmxer[RK.length+3] = '\0';
            ivckmsg1(field, tmxer); }
         factor = tens[absexp&15];
         /* This loop structure avoids power overflow */
         for (power = tens[16]; ; power *= power) {
            if (absexp & 16) factor *= power;
            if ((absexp>>=1) < 16) break; }
         }
      else factor = tens[absexp];
      if (exp < 0) {             /* Apply negative exponent */
         value /= factor;
         RK_Inflags |= NONZERO_FRAC; }
      else                       /* Apply positive exponent */
         value *= factor;
      } /* End exponent scaling */

/* If an input adjuster was specified, apply it now
*  (before fixed-point scaling and range testing).  */

   if (RKC.ktest & VCK_ADJ) {
      if (RK_Inflags & MINUS_FOUND) value -= RKC.dvadj;
      else                          value += RKC.dvadj;
      RKC.ktest &= ~VCK_ADJ;     /* Only use it once */
      }

   return value;
   } /* End decin() */


/*=====================================================================*
*                                                                      *
*                           Function BCDIN                             *
*                                                                      *
*  This routine differs from the IBM Assembler version in that input   *
*  values can be expressed in E format.  An E or sign after one or     *
*  more digits signifies E format.  With RK_CTST off, an E that is     *
*  legal junk may accidentally be taken as E format.                   *
*=====================================================================*/

double bcdin(ui32 ic, char *field) {

   double value;              /* Value to be returned */

   RK_Inflags = 0;

/* Handle hexadecimal input */

   if ((ic & RK_IORF) == RK_HEXF) {
      if (ic & RK_SNGL) {
         float spval;
         hexin(ic, field, (byte *)&spval, sizeof(float));
         value = (double)spval;
         } /* End single-precision hex input */
      else
         hexin(ic, field, (byte *)&value, sizeof(double));
      } /* End hex input */

/* Handle ordinary decimal formats */

   else {
      value = decin(ic, field);
      /* If minus sign was found, make result negative */
      if (RK_Inflags & MINUS_FOUND) value = -value;
      }

/* If a forbidden zero value was found, generate an error
*  message and replace it with a small nonzero value.  */

   if (ic & RK_ZTST && value == 0.0) {
      ivckmsg0(field, IVCK_APPNZ);
      value = 1.0E-30;
      }

   return value;
   } /* End bcdin() */
