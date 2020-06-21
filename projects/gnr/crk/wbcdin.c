/* (c) Copyright 2009-2013, The Rockefeller University *11115* */
/* $Id: wbcdin.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                              wbcdin.c                                *
*                                                                      *
*  Routine wbcdin is designed to replace "traditional" rocks numeric   *
*  input routines ibcdin and ubcdin.  It implements 64-bit fixed-point *
*  input along with narrower variable types in either a 32-bit or a    *
*  64-bit compilation environment.  Calculations are performed with    *
*  fixed-point arithmentic to ensure consistent results independent    *
*  of floating-point representation.  Results are returned via a       *
*  pointer so all types can be handled with a single routine.          *
*                                                                      *
*  Design note:  The working accumulator is 6*32 = 192 bits.  This     *
*  handles the most extreme possible case of 32 decimal (or hex)       *
*  digits in (106 or 128 bits) with 63 fraction bits, or 191 bits      *
*  max before scaling.  This avoids the need for overflow checking     *
*  (except with positive exponents) and there is no roundoff error     *
*  because scaling is done with just one division.  Speed is not       *
*  much of an issue--expected use is just reading input parameters.    *
************************************************************************
*  V1A, 09/27/08, GNR - Derived from bcdin+ibcdin                      *
*  ==>, 05/25/09, GNR - Last date before committing to svn repository  *
*  Rev, 10/25/13, GNR - Replace MINUX0 with UI32_SGN                   *
***********************************************************************/

/*---------------------------------------------------------------------*
*        Usage:                                                        *
*---------------------------------------------------------------------*/

/*
*  Synopsis: void wbcdin(const char *field, void *pitem, ui32 ic)
*
*  'field' is an array containing the string to be converted.
*  'pitem' is a pointer to the item to be returned.
*  'ic' is the processing code described below.
*
*  Prototyped and 'ic' codes defined in rkxtra.h
*
*  The operation code 'ic' is the sum of five quantities:
*       ic = scale + dec + op + size + width - 1
*
*    'scale' is used to specify noninteger fixed-point variables.
*       The scale code is (s << RK_BS), where RK_BS is 24 and 's'
*       is the number of bits to the right of the binary point in
*       the result (maximum, 63).
*
*     'dec' specifies decimal scaling. If the RK_DSF bit (=0x00800000)
*        is set, scaling is forced, i.e. the value entered by the user
*        is always multipled by the scale.  If RK_DSF is not set, sca-
*        ling only occurs when no decimal point is found in the input.
*        'dec' is equal to (d << RK_DS), where RK_DS is 16.  The scale
*        is (10**(-d)) with (-64 < d <= 63) (i.e. 'dec' on input is 7
*        bits including the 3 bits used for the exponent width by the
*        output routines--but note that if the magnitude of the net of
*        'd' and any input exponent or decimal is too large the result
*        may report an out-of-range or zero error).  Thus, positive
*        values of 'd' correspond to the number of places to the right
*        of the inserted decimal when no decimal is found in the input.
*        Users normally expect 'd' to be 0.
*
*    'op' specifies the operation to be performed.  It is made
*       up by adding the desired codes from the following table.
*       Most of these are the same as the old ibcdin/ubcdin codes.
*       Codes that are different have names starting with RK_N.
*       Exponential format input is always accepted even if the
*       RK_IORF (originally I or F format) code is entered.
*
*       Hexadecimal Defined
*         Value      Name    Function
*       ----------  -------  ------------------------------------------
*       0x00008000  RK_NUNS  Item is unsigned.
*       0x00004000  RK_ZTST  Zero Test.  Generate an error if the
*                            numerical value of the result is zero.
*       0x00002000  RK_CTST  Character Test.  Generate an error if non-
*                            numeric characters are found in the input.
*       0x00001000  RK_QPOS  Query Positive.  Generate an error if the
*                            numerical value of the result is negative.
*       0x00000600  RK_IORF  Numbers are entered to base 10.  This is
*                            the default, this code is optional.
*       0x00000400  RK_OCTF  O (octal) conversion.  Numbers are
*                            entered to base 8.  No exponents.
*       0x00000200  RK_HEXF  H (hexadecimal) conversion.  Numbers are
*                            entered to base 16.  No exponents.
*
*    'size' specifies the size of the result to be returned.  In
*       general, an error is generated if the result will not fit
*       in a variable of the specified size.
*
*       Hexadecimal Defined
*         Value      Name    Function
*       ----------  -------  ------------------------------------------
*       0x00000000  RK_NDFLT Default:  item is an int as in K&R C.
*       0x00000040  RK_NBYTE Item is a single byte (8 bits).
*       0x00000080  RK_NHALF Item is a halfword (16 bits).
*       0x000000c0  RK_NINT  Item is an int (16 or 32 bits depending
*                            on the compilation mode).
*       0x00000100  RK_NI32  Item is a 32-bit fixed-point value.
*       0x00000140  RK_NLONG Item is a long (32 or 64 bits depending
*                            on the compilation mode).
*       0x00000180  RK_NI64  Item is a 64-bit fixed-point value.
*       0x000001c0  RK_NI128 Reserved for future 128-bit values,
*                            not implemented here.
*
*    'width' is the number of characters in the input field to be
*       examined, less one.  (The maximum width is RK_MXWD+1 = 32.)
*/

/*---------------------------------------------------------------------*
*        Error Procedures:                                             *
*---------------------------------------------------------------------*/

/*
*    Input errors generate calls to ivckmsg[012].  This calls ermark
*       to mark the error if input was scanned, then calls cryout to
*       print an appropriate message.  If an input value overflows,
*       it is set to the largest value that can be expressed in the
*       requested numeric type.  A special dispensation is made for
*       short and byte types:  If the overflow is exactly to the next
*       higher integer, it is set to the maximum value but no error
*       is flagged.  If an input value is zero and the RK_ZTST bit is
*       set, the value is set to a small positive value, possibly
*       preventing a divide-by-zero error in the caller.
*/

/*---------------------------------------------------------------------*
*        Restrictions (not checked in the code):                       *
*---------------------------------------------------------------------*/

/*
      1) Program assumes 8 bits per byte.

      2) The representations of the characters '0' through
         '9' and 'A' through 'F' must be consecutive codes.

      3) Assumes fixed-point negatives are two's complements
         with the sign in the high-order bit.
*/

/*---------------------------------------------------------------------*
*        Global definitions:                                           *
*---------------------------------------------------------------------*/

#include <ctype.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "rockv.h"
#include "rkarith.h"

extern ui16 RK_Inflags;       /* Various flags */

#define BaseMask       3      /* Base code mask */
#define BaseShft       9      /* Shift to extract base code */
#define BsclMask      63      /* Binary scale mask */
#define BsclShft      24      /* Shift to extract binary scale */
#define Bscl2Wds       5      /* Shift to extract words from bscl */
#define DtosShft       9      /* Shift high decimal to sign bit */
#define Dto1Shft      25      /* Shift signed decimal to ones bit */
#define BPL  BITSPERUI32      /* 32 bits */
#define BPS BITSPERSHORT      /* 16 bits */
#define LAC            6      /* 192 bits/32 bits */
#define LAC16         12      /* 192 bits/16 bits */
#define NIM            4      /* HAS_I64 input multiplies */
#define NIM16          8      /*  NO_I64 input multiplies */
#define STT    UI32_SIZE      /* Size of itens table (==10) */
#define STT16          5      /* Usable itens table if NO_I64 */
#define TWO64   1.84467440E19 /* Conservative overflow test */
#define TWO32   4294967296.0  /* Two to 32nd power */

/*=====================================================================*
*                               getdval                                *
*                                                                      *
*  This little internal function returns the numeric value of a digit  *
*  in any of the number bases decimal, hexadecimal, octal, or -1 if    *
*  the argument is not a digit in that base.                           *
*=====================================================================*/

static int getdval(int inchar, int kbase) {
   switch (kbase) {
   case (RK_EFMT >> BaseShft):   /* Decimal input */
   case (RK_IORF >> BaseShft):
      if (isdigit(inchar)) return (inchar - '0');
      break;
   case (RK_HEXF >> BaseShft):   /* Hexadecimal input */
      if (isxdigit(inchar)) {
         if (isdigit(inchar)) return (inchar - '0');
         else return (toupper(inchar) - ('A' - 10));
         }
      break;
   case (RK_OCTF >> BaseShft):   /* Octal input */
      if (inchar >= '0' && inchar < '8') return (inchar - '0');
      break;
      }
   return -1;
   } /* End getdval() */


/*=====================================================================*
*                                                                      *
*                               wbcdin                                 *
*                                                                      *
*=====================================================================*/

void wbcdin(const char *field, void *pitem, ui32 ic) {

   ui32 wval[LAC];                     /* 192-bit value accumulator */
   const char *pf,*pfe;                /* Ptrs for scanning field */
#ifndef HAS_I64
   ui16 *pwv = (ui16 *)wval;
#endif
   int bscl =                          /* Binary scale */
      (int)(ic >> BsclShft & BsclMask);
   int decloc = 0;                     /* Decimal location */
   int digct = 0;                      /* Digit count */
   int exp = 0;                        /* Exponent */
   int base,kbase =                    /* Code for number base */
      (int)(ic >> BaseShft & BaseMask);
   int nisz;                           /* Size of variable */
   int qzero = TRUE;                   /* TRUE if result is zero */
   int width = (int)(ic & RK_MXWD);    /* Max width of input - 1 */
   /* Define low, middle, high words to shorten some code below */
#if BYTE_ORDRE > 0
   enum evw { XV3=0, XV2, XV1, WVH, WVM, WVL };
#else
   enum evw { WVL=0, WVM, WVH, XV1, XV2, XV3 };
#endif

   /* Powers of ten for decimal scaling */
   static const ui32 itens[STT] = { 1, 10, 100, 1000, 10000, 100000,
      1000000, 10000000, 100000000, 1000000000 };
   /* Arithmetic base as function of kbase */
   static const int base0[4] = { 10, 16, 8, 10 };

   base = base0[kbase];
   nisz = wnclen(ic);
   if (nisz <= 0) abexit(40);          /* Unimplemented size */
   RK_Inflags = 0;
   memset((char *)wval, 0, LAC*sizeof(ui32));

/*---------------------------------------------------------------------*
*          Scan and accumulate numeric value of input field            *
*---------------------------------------------------------------------*/

   pfe = field + width;
   for (pf=field; pf<=pfe; ++pf) {
      int digit, inchar = (int)*pf;
      if (inchar == '\0') break;       /* End of input string */
      if ((digit = getdval(inchar, kbase)) >= 0) {
#ifdef HAS_I64                         /*--------------------*/
         ui64 tt = (ui64)digit;        /* Accumulate: 64-bit */
         int i;                        /*--------------------*/
         if (digit > 0) qzero = FALSE;
#if BYTE_ORDRE > 0                     /* Big-endian */
         for (i=(LAC-1); i>=(LAC-NIM); --i) {
            tt += base*(ui64)wval[i];
            wval[i] = (ui32)tt;
            tt >>= BPL;
            }
#else                                  /* Little-endian */
         for (i=0; i<NIM; ++i) {
            tt += base*(ui64)wval[i];
            wval[i] = (ui32)tt;
            tt >>= BPL;
            }
#endif
#else /* NO_I64 */                     /*--------------------*/
         ui32 tt = (ui32)digit;        /* Accumulate: 32-bit */
         int i;                        /*--------------------*/
         if (digit > 0) qzero = FALSE;
#if BYTE_ORDRE > 0                     /* Big-endian */
         for (i=(LAC16-1); i>=(LAC16-NIM16); --i) {
            tt += base*(ui32)pwv[i];
            pwv[i] = (ui16)tt;
            tt >>= BPS;
            }
#else                                  /* Little-endian */
         for (i=0; i<NIM16; ++i) {
            tt += base*(ui32)pwv[i];
            pwv[i] = (ui16)tt;
            tt >>= BPS;
            }
#endif
#endif
         ++digct;
         }
         /* Unlikely to be needed, but cheap--allow decimals with
         *  hex and octal input, but exponents give error below */
      else if (inchar == '.') {        /* Decimal point found */
         if (RK_Inflags & DP_FOUND) ivckmsg0(field, IVCK_FORMAT);
         else {
            decloc = digct;
            RK_Inflags |= DP_FOUND; }
         }
      else if (inchar == '+') {        /* Plus sign found */
         if (digct > 0) goto expscan;
         if (RK_Inflags & SIGN_FOUND) ivckmsg0(field, IVCK_FORMAT);
         RK_Inflags |= SIGN_FOUND;
         }
      else if (inchar == '-') {        /* Minus sign found */
         if (digct > 0) goto negexpscan;
         if (RK_Inflags & SIGN_FOUND) ivckmsg0(field, IVCK_FORMAT);
         RK_Inflags |= (SIGN_FOUND | MINUS_FOUND);
         }
      else if (toupper(inchar) == 'E') {  /* Exponent found */
         if (digct > 0) {              /* A sign may follow */
            pf++;
            if (*pf == (char)'+') goto expscan;
            if (*pf == (char)'-') goto negexpscan;
            pf--;                 goto expscan;
            }
         if (ic & RK_CTST) ivckmsg0(field, IVCK_FORMAT);
         }
      else if ((inchar != ' ') && (ic & RK_CTST)) {
         /* Bad char found */
         ivckmsg0(field, base == 10 ? IVCK_FORMAT : IVCK_HEXFMT);
         }
      } /* End digit scan */
   goto scale;                         /* Skip over exponent scan */

/*---------------------------------------------------------------------*
*            Scan the exponent field (decimal input only)              *
*---------------------------------------------------------------------*/

negexpscan: RK_Inflags |= NEG_EXP;
expscan:
   if (base == 10) for (pf++; pf<=pfe; pf++) {
      register int inchar = (int)*pf;
      if (inchar == 0) break;
      if (isdigit(inchar)) {           /* Digit found */
         exp = 10*exp + (inchar - '0');
         if (exp > (INT_MAX/10-10)) {  /* Just in case */
            ivckmsg0(field, IVCK_BADEXP);
            goto GotOverflow; }
         RK_Inflags |= EXP_DIG_FOUND;
         }
      /* Bad character in exponent */
      else if (toupper(inchar)=='E' || inchar=='+' || inchar=='-') {
         ivckmsg0(field, IVCK_BADEXP);
         if (RK_Inflags & EXP_DIG_FOUND) break;
         }
      else if (inchar != ' ' && ic & RK_CTST)
         ivckmsg0(field, IVCK_BADEXP);
      } /* End exponent digit scan */
   /* Exponent but no digits, or base is not 10 */
   if ((RK_Inflags & EXP_DIG_FOUND) == 0)
      ivckmsg0(field, IVCK_BADEXP);

/*---------------------------------------------------------------------*
*                         Apply binary scale                           *
*  The size of the working value accumulator has been chosen large     *
*  enough that no binary scale that is representable in the ic code    *
*  can possibly cause an overflow, so just a big left shift is done.   *
*  Shift of 32 is handled separately because NOP on some processors.   *
*  After this shift, code must assume all LAC words of wval are !=0.  *
*---------------------------------------------------------------------*/

scale:
   if (digct == 0) {                   /* No digits found */
      /* If the card is not being scanned, an empty field is an
      *  acceptable 0.  */
      if (RKC.accpt & ACCPT_CS)
         ivckmsg0(field, IVCK_FORMAT); /* Mark number error */
      goto ReturnZero;
      }
   if (qzero) goto CheckAdjust;        /* Result is zero */
   if (bscl == BITSPERUI32) {
      int i;
#if BYTE_ORDRE > 0                     /* Big-endian */
      for (i=1; i<(LAC-1); ++i) wval[i] = wval[i+1];
#else                                  /* Little-endian */
      for (i=(LAC-1); i>0; --i) wval[i] = wval[i-1];
#endif
      wval[WVL] = 0;
      }
   else if (bscl) {
      ui32 tt;                         /* Shift temp */
      int i;
      int nfw = bscl >> Bscl2Wds;      /* Full-word shifts */
      int nls = bscl & (BPL-1);        /* Left shifts */
      int nrs = BPL - nls;             /* Right shifts */
#if BYTE_ORDRE > 0                     /* Big-endian */
      for (tt=0,i=1; i<(LAC-1); ++i) {
         wval[i-nfw] = tt | wval[i+1] >> nrs;
         tt = wval[i+1] << nls;
         }
      wval[i] = 0;                     /* Written over if nfw == 0 */
      wval[i-nfw] = tt;
# else                                 /* Little-endian */
      for (tt=0,i=NIM; i>0; --i) {
         wval[i+nfw] = tt | wval[i-1] >> nrs;
         tt = wval[i-1] << nls;
         }
      wval[0] = 0;                     /* Written over if nfw == 0 */
      wval[nfw] = tt;
#endif
      }

/*---------------------------------------------------------------------*
*             Apply combined decimal and exponent scales               *
*---------------------------------------------------------------------*/

   if (RK_Inflags & NEG_EXP) exp = -exp; /* Set exponent sign */
   if (RK_Inflags & DP_FOUND)          /* Use decimal found in input */
      exp -= (digct - decloc);
   else
      ic |= RK_DSF;                    /* Otherwise, use default dec */
   if (ic & RK_DSF) {                  /* Use ic decimal (with sign) */
      si32 d = ic << DtosShft;
      exp -= (int)SRA(d, Dto1Shft);
      }

   if (exp > 0) {
      /* Apply positive exponent by multiplication.  Since there
      *  is no rounding, there is no error if multiplication is
      *  broken into 32 bit pieces when exp > 9.  Do precheck for
      *  overflow:  exp > 19 (value that when multiplied by 1
      *  would overflow 64 bits) or wval >= 2**64 must overflow.  */
      ui32 iscl;
      if (exp > 19) goto GotOverflow;
      if (wval[XV3] | wval[XV2]) goto GotOverflow;
      while (exp > 0) {
#ifdef HAS_I64                         /*-----------------------*/
         ui64 tt;                      /* Scale exp > 0: 64-bit */
         int i,ee = min(exp, (STT-1)); /*-----------------------*/
         iscl = itens[ee];
#if BYTE_ORDRE > 0                     /* Big-endian */
         for (tt=0,i=(LAC-1); i>=0; --i) {
            tt += iscl*(ui64)wval[i];
            wval[i] = (ui32)tt;
            tt >>= BPL;
            }
#else                                  /* Little-endian */
         for (tt=0,i=0; i<LAC; ++i) {
            tt += iscl*(ui64)wval[i];
            wval[i] = (ui32)tt;
            tt >>= BPL;
            }
#endif
#else /* NO_I64 */                     /*-----------------------*/
         ui32 tt;                      /* Scale exp > 0: 32-bit */
         int i;                        /*-----------------------*/
         int ee = min(exp, (STT16-1));
         iscl = itens[ee];
#if BYTE_ORDRE > 0                     /* Big-endian */
         for (tt=0,i=(LAC16-1); i>=0; --i) {
            tt += iscl*(ui32)pwv[i];
            pwv[i] = (ui16)tt;
            tt >>= BPS;
            }
#else                                  /* Little-endian */
         for (tt=0,i=0; i<LAC16; ++i) {
            tt += iscl*(ui32)pwv[i];
            pwv[i] = (ui16)tt;
            tt >>= BPS;
            }
#endif
#endif
         if (tt) goto GotOverflow;
         exp -= ee;
         } /* End while (exp > 0) */
      } /* End exp > 0 */

   else if (exp < 0) {
      /* Apply negative exponent by division after first constructing
      *  the divisor.  Unlike the old ibcdin/ubcdin, which used double
      *  floating-point arithmetic for this operation, here fixed-
      *  point division (vdivl) is used.  This minimizes round-off
      *  errors and underflow checking is exact.  The penalty is that
      *  the program will be a little slower.  */
      ui32 dscl[LAC];                  /* Scale factor */
      ui32 dtmp[6*LAC+2];              /* vdivl() scaling temp */
      int i;
#ifndef HAS_I64
      int ee;                          /* Working exponent */
#endif
      /* If RK_INT bit is set (old coding was RK_GFP not set), give
      *  an error that the result must be an integer.  This is based
      *  on a negative exponent even if scaling would incidentally
      *  leave an integer result--(a) user clearly didn't understand
      *  what was wanted here, (b) easier test.  */
      if (ic & RK_INT) ivckmsg0(field, IVCK_INTEGER);
      exp = abs(exp);
      /* The largest possible input number (10**32) (S63), divided
      *  by 10**52 gives a quotient of 0, hence any exponent this
      *  large is a guaranteed underflow and we can skip scaling.  */
      if (exp > 51) {
         ivckmsg0(field, IVCK_UNDRFLOW);
         goto ReturnZero;
         }
      /* Construct divisive scale factor */
                                       /*-----------------------*/
#ifdef HAS_I64                         /* Scale exp < 0: 64-bit */
                                       /*-----------------------*/
#if BYTE_ORDRE > 0                     /* Big-endian */
      memset((char *)dscl, 0, (LAC-1)*sizeof(ui32));
      dscl[LAC-1] = itens[exp % (STT-1)];
      while (exp >= (STT-1)) {
         ui64 tt, mul32 = (ui64)itens[STT-1];
         for (tt=0,i=(LAC-1); i>=0; --i) {
            tt += mul32*(ui64)dscl[i];
            dscl[i] = (ui32)tt;
            tt >>= BPL;
            }
         exp -= (STT-1);
         }
#else                                  /* Little-endian */
      dscl[0] = itens[exp % (STT-1)];
      memset((char *)(dscl+1), 0, (LAC-1)*sizeof(ui32));
      while (exp >= (STT-1)) {
         ui64 tt, mul32 = (ui64)itens[STT-1];
         for (tt=0,i=0; i<LAC; ++i) {
            tt += mul32*(ui64)dscl[i];
            dscl[i] = (ui32)tt;
            tt >>= BPL;
            }
         exp -= (STT-1);
         }
#endif
                                       /*-----------------------*/
#else /* NO_I64 */                     /* Scale exp < 0: 32-bit */
      pwv = (ui16 *)dscl;              /*-----------------------*/
      ee = min(exp, (STT-1));
#if BYTE_ORDRE > 0                     /* Big-endian */
      memset((char *)dscl, 0, (LAC-1)*sizeof(ui32));
      dscl[LAC-1] = itens[ee];
      while ((exp -= ee)) {            /* Assignment intended */
         ui32 tt,mul16;
         ee = min(exp, (STT16-1));
         mul16 = itens[ee];
         for (tt=0,i=(LAC16-1); i>=0; --i) {
            tt += mul16*(ui32)pwv[i];
            pwv[i] = (ui16)tt;
            tt >>= BPS;
            }
         }
#else                                  /* Little-endian */
      dscl[0] = itens[ee];
      memset((char *)(dscl+1), 0, (LAC-1)*sizeof(ui32));
      while ((exp -= ee)) {            /* Assignment intended */
         ui32 tt,mul16;
         ee = min(exp, (STT16-1));
         mul16 = itens[ee];
         for (tt=0,i=0; i<LAC16; ++i) {
            tt += mul16*(ui32)pwv[i];
            pwv[i] = (ui16)tt;
            tt >>= BPS;
            }
         }
#endif
#endif
      /* Scale with rounding */
      vdivl(wval, dscl, wval, NULL, dtmp, LAC, LAC, TRUE);
      } /* End exp < 0 */

/*---------------------------------------------------------------------*
*                  Adjust and store the final result                   *
*  The code here was dictated by a decision that even with RK_NUNS or  *
*  RK_QPOS, it should be possible to enter a negative value but get an *
*  acceptable positive value after applying variable adjustment.       *
**** From here to end, wval arithmetic is 3*32-bits, not 6*32-bits. ****
*---------------------------------------------------------------------*/

   /* Do second-level overflow check:  If any of top three words in
   *  wval is nonzero, or sign bit in next word if variable is signed,
   *  no amount of variable adjustment can help, so give overflow
   *  error now--simpler than testing later when wval is signed.  */
   if (wval[XV3] | wval[XV2] | wval[XV1]) goto GotOverflow;
   if (!(ic & RK_NUNS) && wval[WVH] & UI32_SGN) goto GotOverflow;

   /* Negate the input value if minus sign found, even if unsigned.  */
   if (RK_Inflags & MINUS_FOUND) {
      ui32 cc;                         /* Complementation carry */
      wval[WVL] = -wval[WVL];
      wval[WVM] = ~wval[WVM] + (cc = wval[WVL] == 0);
      wval[WVH] = ~wval[WVH] + (cc & wval[WVM] == 0);
      }

   /* Perform variable adjustment if requested:
   *  (1) Scale the adjustment value to the current bscl.
   *  (2) Convert double to fixed-point.  After scaling, this
   *      could extend beyond 64 bits, but low bits would be
   *      ill defined, so this is treated as an overflow.
   *  (3) Add it to or subtract it from wval (64 bits plus
   *      sign in 65th bit.  */
CheckAdjust:
   if (RKC.ktest & VCK_ADJ) {
      double dadj = fabs(RKC.dvadj);
      ui64   wadj;

#if LSIZE == 8
#define OneUL 1UL
#else
#define OneUL 1ULL
#endif
#ifdef HAS_I64                         /* 64-bit version */
#if BYTE_ORDRE > 0
      ui64 *pval = (ui64 *)(wval + WVM);
#else
      ui64 *pval = (ui64 *)(wval + WVL);
#endif
      dadj *= (double)(OneUL<<bscl);
      dadj += 0.5;
      if (dadj > TWO64) goto GotOverflow;
      wadj = (ui64)dadj;
      if (RKC.dvadj >= 0.0) {          /* Add adjustment */
         if (~wadj < *pval) wval[WVH] += 1;
         *pval += wadj;
         }
      else {                           /* Subtract adjustment */
         if (wadj > *pval) wval[WVH] -= 1;
         *pval -= wadj;
         }
#else                                  /* 32-bit version */
      if (bscl >= BITSPERUI32) dadj *= TWO32;
      dadj *= (double)(OneUL<<(bscl & (BITSPERUI32-1)));
      dadj += 0.5;
      if (dadj > TWO64) goto GotOverflow;
      wadj.hi = (ui32)(dadj / TWO32);
      wadj.lo = (ui32)fmod(dadj, TWO32);
      if (RKC.dvadj >= 0.0) {          /* Add adjustment */
         ui32 dloc = ~wval[WVL] < wadj.lo;
         wval[WVH] += ~wval[WVM] < dloc;
         wval[WVM] += dloc;
         wval[WVL] += wadj.lo;
         wval[WVH] += ~wval[WVM] < wadj.hi;
         wval[WVM] += wadj.hi;
         }
      else {
         ui32 dloc = wval[WVL] < wadj.lo;
         wval[WVH] -= wval[WVM] < dloc;
         wval[WVM] -= dloc;
         wval[WVL] -= wadj.lo;
         wval[WVH] -= wval[WVM] < wadj.hi;
         wval[WVM] -= wadj.hi;
         }
#endif

      /* Reset negative flag according to result of adjustment--
      *  used to set sign of overflow/underflow replacements.  */
      if (wval[WVH] & UI32_SGN) RK_Inflags |= MINUS_FOUND;
      else                      RK_Inflags &= ~MINUS_FOUND;
      } /* End applying variable adjustment */

   /* Now if the result is required to be positive (or unsigned)
   *  and it is still negative after any variable adjustment,
   *  give an error message and return 0.  Because WVH word is
   *  kept, the same test can be used for signed & unsigned. */
   if (ic & (RK_QPOS|RK_NUNS) && wval[WVH] & UI32_SGN) {
      ivckmsg0(field, IVCK_APPGE0);
      goto ReturnZero;
      }

   /* If a forbidden zero value was found, code at ReturnZero will
   *  return a value of +/- one.  */
   if (ic & RK_ZTST && !(wval[WVL] | wval[WVM] | wval[WVH])) {
      ivckmsg0(field, IVCK_APPNZ);
      goto ReturnZero;
      }

   /* Check whether result will overflow user's data type.  If
   *  result is signed, discarded bytes must match result sign.
   *  Return result to caller.  */
#if BYTE_ORDRE > 0                     /* Big-endian */
   pf = (char *)(wval+WVH), pfe = (char *)wval + sizeof(wval) - nisz;
   if (ic & RK_NUNS) {
      while (pf < pfe) if (*pf++) goto GotOverflow;
      }
   else {                              /* Signed */
      char btst = (pfe[0] & 0x80) ? BYTE_MAX : 0;
      while (pf < pfe) if (*pf++ != btst) goto GotOverflow;
      }
   memcpy((char *)pitem, pfe, nisz);
#else                                  /* Little-endian */
   pf = (char *)wval + nisz, pfe = (char *)wval + 3*sizeof(ui32);
   if (ic & RK_NUNS) {
      while (pf < pfe) if (*pf++) goto GotOverflow;
      }
   else {                              /* Signed */
      char btst = (pf[-1] & 0x80) ? BYTE_MAX : 0;
      while (pf < pfe) if (*pf++ != btst) goto GotOverflow;
      }
   memcpy((char *)pitem, (char *)wval, nisz);
#endif

   return;

/*---------------------------------------------------------------------*
*                      Quick path to return zero                       *
*---------------------------------------------------------------------*/

ReturnZero:
   if (ic & RK_ZTST) {                 /* Return value closest to 0 */
      if (RK_Inflags & MINUS_FOUND)
         memset((char *)pitem, BYTE_MAX, nisz);
      else {
#if BYTE_ORDRE > 0                     /* Big-endian */
         char *pz = (char *)pitem + nisz - 1;
#else                                  /* Little-endian */
         char *pz = (char *)pitem;
#endif
         memset((char *)pitem, 0, nisz);
         *pz = 1;
         }
      }
   else
      memset((char *)pitem, 0, nisz);
   return;

/*---------------------------------------------------------------------*
*                          Handle overflows                            *
*---------------------------------------------------------------------*/

GotOverflow: {
   int iint = BITSPERBYTE*nisz - bscl;
   if (ic & RK_NUNS) {                 /* Unsigned */
      ivckmsg2(field, iint);
      memset((char *)pitem, BYTE_MAX, nisz);
      }
   else {                              /* Signed */
      char *pit = (char *)pitem;
      int niszm1 = nisz - 1;
      ivckmsg2(field, iint-1);
      if (RK_Inflags & MINUS_FOUND) {
         /* Store maximum negative number */
#if BYTE_ORDRE > 0                     /* Big-endian */
         pit[0] = 0x80;
         if (niszm1 > 0) memset(pit+1, 0, niszm1);
#else                                  /* Little-endian */
         if (niszm1 > 0) memset(pit, 0, niszm1);
         pit[niszm1] = 0x80;
#endif
         }
      else {
         /* Store maximum positive number */
#if BYTE_ORDRE > 0                     /* Big-endian */
         pit[0] = SCHR_MAX;
         if (niszm1 > 0) memset(pit+1, BYTE_MAX, niszm1);
#else                                  /* Little-endian */
         if (niszm1 > 0) memset(pit, BYTE_MAX, niszm1);
         pit[niszm1] = SCHR_MAX;
#endif
         }
      }} /* End signed-number overflow and local scope */
   return;

   } /* End wbcdin() */
