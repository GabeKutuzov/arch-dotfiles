/* (c) Copyright 1988-2018, The Rockefeller University *11116* */
/* $Id: bcdout.c 67 2018-05-07 22:08:53Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                              bcdout.c                                *
*    ROCKS numerical output conversion for floating-point numbers      *
*                                                                      *
*  V3A has the following features:                                     *
*  (1) Can predict overflow with near-certainty early in the           *
*      algorithm.  Improved way to deal with overflows.                *
*  (2) Explicitly allows overflow on rounding into blank at far        *
*      left of output field (rare), but forced E fmt maintains a       *
*      blank in left column to avoid confusion in rows of numbers.     *
*  (3) Outputs directly to user field, saving a memcpy().              *
*  (4) Internal function calls are eliminated to save time.            *
*  (5) All long types changed to si32,ui32 in anticipation of 64-bit   *
*      computing.                                                      *
*                                                                      *
*  Note added 09/04/11:  Policy regarding protective space at left of  *
*  output field:  Ordinary conversions may go against left edge and    *
*  left-justified conversions always do.  A blank is inserted if there *
*  is no sign and (i) RK_LSPC bit is on, (ii) RK_AUTO bit is on, or    *
*  (iii) E format is forced.  In all these cases, the blank may be     *
*  overwritten if roundoff during decimal insertion overflows into     *
*  the protective blank.                                               *
*                                                                      *
************************************************************************
*  V1A, 10/17/88, GNR - Convert from my IBM Assembler version          *
*  V2A, 02/12/89, GNR - Move BCDIN, IBCDIN into separate file          *
*  Rev, 04/19/89, GNR - Incorporate rkxtra.h                           *
*  V2B, 06/22/89, GNR - Permit hexout width < length if zeros          *
*  Rev, 03/11/93, GNR - Correct bug in 6/22/89 hexout fix              *
*  Rev, 01/27/97, GNR - Add decout(), finout(), ubcdwt()               *
*  Rev, 07/08/03, GNR - Autoscale (not EFMT forced) inserts a blank    *
*  Rev, 09/13/03, GNR - Decimal with autoscale is max decimals         *
*  V3A, 05/20/07, GNR - All fixpt algs for ibcdwt/ubcdwt, in sep file, *
*                       remove never-used RK_INT facility              *
*  ==>, 05/29/07, GNR - Last date before committing to svn repository  *
*  Rev, 01/03/09, GNR - Make 64-bit bin->dec agree with 32-bit method  *
*  Rev, 01/04/09, GNR - Parm checks for 64-bit compilation, optimiz's  *
*  Rev, 05/23/09, GNR - If EFMT, use larger of spec or needed expwid   *
*  Rev, 12/28/09, GNR - Deal with rounding (floor()) failure with -O2  *
*  Rev, 08/20/10, GNR - Autodec controls max dec even in force state   *
*  Rev, 11/19/10, GNR - Reassert (2) above (not autodec or forced)     *
*                       (this is used e.g. in cryout to print time).   *
*  V3B, 08/23/11, GNR - Add detection and flagging of NaNs             *
*  V3C, 08/31/11, GNR - Move expwid to RK, add RK_PLUS, RK_LSPC        *
*  Rev, 09/03/11, GNR - Use RK_NPAD0, not RK_PAD0 for zero padding,    *
*                       call wbcdwt for hex/oct out.                   *
*  R67, 04/02/18, GNR - Correct values of constants in comments        *
***********************************************************************/

/*---------------------------------------------------------------------*
*        Usage:                                                        *
*---------------------------------------------------------------------*/

/*
   For double precision floating point to characters:
         void bcdout(ui32 ic, char *field, double arg)

   'ic' is the processing code described below.
   'field' is an array into which the result is placed.
   'arg' is the number to be converted.

   Note that arguments of float type will be automatically coerced to
   double under ANSI C.  The 'ic' argument is 32 bits by tradition--
   padding to 64-bits on the stack in a 64-bit environment cannot be
   avoided.

   The operation code 'ic' is the sum of three quantities:
         ic = dec + op + width - 1

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
         0x40000000  RK_FORCE Used to signal a force to E format.
         0x00400000  RK_NEGV  Used internally to signal a negative arg.
         0x00100000  RK_PLUS  Insert plus sign for positive numbers
                              (for printf '+' flag).
         0x00004000  RK_LFTJ  Left Justify.  Output is left justified.
         0x00002000  RK_UFLW  Underflow control.  Switch to E format if
                              result would have no significant digits.
         0x00001000  RK_AUTO  Automatic Decimal.  Position of decimal
                              point is adjusted to give greatest number
                              of significant figures that will fit in
                              the output field after a single blank,
                              but not more than dec-1 decimals if dec>0.
         0x00000800  RK_NPAD0 Pad with zeros.  Characters to left of
                              most significant digit are padded with
                              zeros.  Default is to pad with blanks.
         0x00000000  RK_EFMT  Generate E (exponential) format.
         0x00000600  RK_IORF  Generate F (decimal) format.
         0x00000400  RK_OCTF  Perform O (octal) conversion.
         0x00000200  RK_HEXF  Perform H (hexadecimal) conversion.
         0x00000100  RK_SNGL  Single precision.  Use with hex conver-
                              sion only.  Displays hex equivalent of
                              single-precision real internal format.
                              Suppresses output of high-order digits
                              of float argument.

      N.B.:  The following bits are used in ibcdwt/ubcdwt/wbcdwt and
         should not be assigned other purposes in this routine:
         ----------  -------  ------------------------------------------
         0x80000000  RK_MZERO Output -BIG as -0.
         0x3f000000           Binary scale.
         0x00800000  RK_DSF   Decimal scale forced.
         0x00200000  RK_LSPC  Left space (could be implemented here)
         0x00008000  RK_GFP,RK_NUNS   General Fixed Point, Unsigned.
         0x000000c0           Type specs with wbcdin,wbcdwt.
         0x00000080  RK_BYTE  Old byte precision with hex conversion.
         0x00000040  RK_PAD0  Old-style pad with zeros flag.

      'width' is the number of characters in the output field, less
         one.  (The maximum width is RK_MXWD+1 = 32.  However, the
         maximum number of digits is given by OUT_SIZE and the re-
         mainder of the width can only be filled with blanks, signs,
         decimal points, exponents, etc.)

      Additionally, the global RK.expwid can be set by the user before
         a call to bcdout().  'expwid' is used only for E format out-
         put, and specifies the number of digits in the printed expo-
         nent (including sign).  If 'expwid' is zero or larger than
         EXP_SIZE, the number of digits needed to express the largest
         exponent that can be represented on the machine, 'expwid' is
         replaced by EXP_SIZE.  If 'expwid' is too small to represent
         the result, a larger value is substituted.

   Because portability is the goal, accuracy will be no better
   than can be achieved with ordinary double operations.
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

      3) abexits: 40 if invalid combination of parameters specified,
         41 if internal check fails.
*/

/*---------------------------------------------------------------------*
*        Restrictions:                                                 *
*---------------------------------------------------------------------*/

/*
      1) No more than OUT_SIZE decimal digits can be handled.
         OUT_SIZE must be not more than (LONG_SIZE-1) if LSIZE
         is 8 bytes, otherwise 2*(UI32_SIZE-1).

      2) Exponents larger than EXP_SIZE can not be handled.

      3) Program assumes 8 bits per byte.
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

#define LONG_SIZE_M1 (LONG_SIZE-1)
#define UI32_SIZE_M1 (UI32_SIZE-1)
#define STT         16        /* Min size of powers of ten table--
                              *  code assumes this is a power of 2 */
#define SDT          5        /* Size of decades table */
#define MxbsMask 0x3f000000   /* Mask for maximum binary scale */
#define MxdsMask 0x000f0000   /* Mask for maximum decimal scale */
#define KpfxMask 0x00700000   /* Mask for prefix controls */
#define KtypMask 0x000001c0   /* Mask for variable type controls */
#define KpfxShft    20        /* Shift to make 0/1 from RK_PLUS bit */
#define ZpfxShft    12        /* Shift to make int from RK_NZ bits */

/* Be sure one ui64 or two ui32's are enough to handle double
*  output conversion */
#if LSIZE == 8
#if OUT_SIZE > LONG_SIZE_M1
#error "A double mantissa must fit in one 64-bit integer"
#endif
#else
#if OUT_SIZE > 2*UI32_SIZE_M1
#error "A double mantissa must fit in two 32-bit integers"
#endif
#endif

/* Table of powers of ten.  This is made available globally for use,
*  e.g. by the number() plot routine.  The extra entries beyond STT+1
*  currently go out to 2*UI32_SIZE_M1.  Check code below carefully if
*  STT, OUT_SIZE, UI32_SIZE, or LONG_SIZE change.  */

   double tens[STT+3] = {1E0, 1E1, 1E2, 1E3, 1E4, 1E5, 1E6,
      1E7, 1E8, 1E9, 1E10, 1E11, 1E12, 1E13, 1E14, 1E15, 1E16,
      1E17, 1E18 };
#if OUT_SIZE > STT
#error "Tens table must have at least OUT_SIZE+1 entries"
#endif
#if !defined(HAS_I64) && 2*UI32_SIZE_M1 > STT+2
#error "Tens table must have at least 2*UI32_SIZE entries"
#endif

/***********************************************************************
*                                                                      *
*                          Subroutine BCDOUT                           *
*                                                                      *
*  Note that RK_NEGV and RK_FORCE bits may be passed from [iuw]bcdwt() *
*  to indicate negative argument or E format force, bzw.               *
***********************************************************************/

void bcdout(ui32 ic, char *field, double arg) {

   double absarg;             /* Absolute value of arg */
   double factor;             /* Decimal scaling factor */
   char *ld;                  /* Ptr to last/left output digit */
   char *rd;                  /* Ptr to rightmost output digit */
   char *upfx;                /* Used prefix */
   int  absexp;               /* Absolute value of exponent */
   int  autodec;              /* 1 if in RK_AUTO mode */
   int  davail;               /* Digits available */
   int  dec0,dec;             /* Original and working decimal */
   int  dgt0;                 /* Need space for 0 before decpt */
   int  exp,expwid;           /* Exponent, exponent width */
   int  forced;               /* TRUE if E format forced */
   int  kpfx;                 /* Kind of prefix */
   int  lpfx;                 /* Length of prefix additions */
   int  mode;                 /* Conversion mode */
   int  sadj;                 /* Scale adjustment for overflow */
   int  width;                /* Width minus 1 */
   char fill;                 /* Fill character */

   /* Table of hex (and decimal) output characters */
   static char hextab[16] = "0123456789ABCDEF";

   /* Table of max exponent vs expwid.  Note: The tricky value '0' for
   *  decades[1] forces 'expwid' to be expanded to 2 in this case.  */
   static ui32 decades[SDT+1] = { 1, 0, 10, 100, 1000, 10000 };
#if EXP_SIZE > SDT
#error "Decades table must have at least EXP_SIZE+1 entries"
#endif

   /* Possible number prefixes and offsets vs control bits */
   static char pfxo[] = { 1,2,0,2,4,4,4,4 };
   static char pfx0[] = { " 0+0-0" };

/* Error if inappropriate options requested */

   mode = (int)ic & RK_IORF;
   if (ic & (MxbsMask|RK_DSF|(KtypMask-RK_SNGL))) abexit(40);

/* Extract width and set to start with rightmost digit.
*  N.B.  On entry to common output code, ld should point one
*  char to left of leftmost char already written to output.  */

   width = (int)ic & RK_MXWD;    /* Width */
   ld = rd = field + width;      /* Start with rightmost digit */
   fill = (ic & RK_NPAD0) ? '0' : ' ';

/* Special short path for zero argument */

   if (arg == 0.0) {
      *ld-- = '0';
      goto finout;
      }

/*---------------------------------------------------------------------*
*                         Hexadecimal output                           *
*                                                                      *
*  Now calls wbcdwt to eliminate redundant, rarely used, code.  The    *
*  RK_SNGL flag (same as RK_NI32) is used by the caller to indicate    *
*  that the original argument was single precision.  This sets the     *
*  width of the conversion and causes the arg, which would have been   *
*  coerced to a double by the call, to be converted back to a single,  *
*  as the exponent lengths of the two kinds of floats may differ.  It  *
*  would be better for the user to just call wbcdwt and call it NI32.  *
*  (May call bcdout recursively if goes to forced E format.)           *
*---------------------------------------------------------------------*/

#if RK_SNGL != RK_NI32
#error Recode bcdout for case RK_SNGL != RK_NI32
#endif
   if (mode == RK_HEXF) {
      if (ic & RK_SNGL) {        /* Handle single precision float */
         float farg = (float)arg;   /* Coerce back to single */
         wbcdwt(&farg, field, ic); }
      else {                     /* Handle double precision float */
         wbcdwt(&arg, field, ic); }
      return;
      } /* End hexadecimal output */

/*---------------------------------------------------------------------*
*                            Octal output                              *
*                                                                      *
*  This is unlikely ever to be used, but by calling wbcdwt we can      *
*  supply this mode cheaply and make the mode menagerie complete.      *
*  For handling of float vs. double args, see comments above for HEXF  *
*  output.  (May call bcdout recursively if goes to forced E format.)  *
*---------------------------------------------------------------------*/

   if (mode == RK_OCTF) {
      if (ic & RK_SNGL) {        /* Handle single precision float */
         float farg = (float)arg;   /* Coerce back to single */
         wbcdwt(&farg, field, ic); }
      else {                     /* Handle double precision float */
         wbcdwt(&arg, field, ic); }
      return;
      } /* End octal output */

/*---------------------------------------------------------------------*
*  Extract sign, prefix, and decimal parameters and calc space         *
*  available for all digits.                                           *
*---------------------------------------------------------------------*/

   expwid = sadj = 0;
   forced = (ic & RK_FORCE) != 0;
   if (arg < 0.0) ic |= RK_NEGV;
   autodec = (ic & RK_AUTO) != 0;
   kpfx = (int)((ic & KpfxMask) >> KpfxShft);   /* Prefix controls */
   lpfx = kpfx != 0;
   upfx = pfx0 + pfxo[kpfx];
   dec = dec0 = ((int)(ic & MxdsMask) >> RK_DS);
   davail = width - lpfx;     /* Implicitly allows for decimal pt. */
   if (autodec) {    /* Simplify some later tests */
      if (dec0 == 0) dec0 = STT; }
   else davail -= (dec - 1);  /* Adds one back if dec = 0 (no pt.) */
   if (forced) goto force;

#ifndef IBM370
/*---------------------------------------------------------------------*
*                                 NaN                                  *
*                                                                      *
*  NaNs do not test large against tens table width limits, thus giving *
*  ld-- field underflows.  Web info says the isnan() test function is  *
*  not available with all compilers, and the test 'arg != arg' would   *
*  likely be removed by optimization, so we do a low-level bit test.   *
*---------------------------------------------------------------------*/

   {  union { double tdarg; ui16 tharg[sizeof(double)/2]; } tnan;
#if BYTE_ORDRE > 0
      int eoff = 0;
#else
      int eoff = sizeof(double)/2 - 1;
#endif
      tnan.tdarg = arg;
      if ((tnan.tharg[eoff] & 0x7ff0) == 0x7ff0) {
         if (width - lpfx < 2) goto fillstars;
         *ld-- = 'N'; *ld-- = 'a'; *ld-- = 'N';
         fill = ' ';
         goto insrtpfx;
         } /* End processing NaN */
      } /* End tnan local scope */
#endif

/*---------------------------------------------------------------------*
*                           Decimal Output                             *
*---------------------------------------------------------------------*/

   if (mode == RK_EFMT) goto doEfmt;

/* Force E format if data would overflow user's field */

   absarg = fabs(arg);
   if (davail < 0 || absarg >= tens[min(OUT_SIZE,davail)])
      goto force;

/* Handle automatic decimal placement (F format)--
*     If |arg| is less than 10, allow space for +/- x.xxxx.
*     Otherwise, use log(arg) to find number of integer digits.
*     Check against table to correct rounding errors in log.
*        (Don't test lower limit--it might yet get rounded up.)
*  Note:  Need to subtract 1 from davail for first leading digit,
*  but add one back to convert fraction digits to our d parameter,
*  so net result is to leave davail alone here.  */

   if (autodec) {
      if (absarg >= tens[1]) {
         int jd = (int)log10(absarg);
         /* jd+1 cannot exceed size of tens table due to test in block
         *  above that forces E format if absarg >= tens[OUT_SIZE]  */
         if (absarg >= tens[jd+1]) jd++;
         davail -= jd;
         }
      dec = min(davail,dec0);

      /* Force E format if there is not room for the integer part.
      *  Note that this test only fails if the integer would overflow
      *  into the reserved blank at the left of the field, otherwise
      *  the test above already goes to force.  Delete this line to
      *  make it OK for integer to overflow into the blank.  */
      if (dec < 0) goto force;
      }
   goto doFfmt;

/*---------------------------------------------------------------------*
*                              E Format                                *
*---------------------------------------------------------------------*/

force:
   forced = TRUE;
   mode = RK_EFMT;

/* Scaling for E format conversion.  Determine scale using the log10
*  function to avoid any assumptions about the number of bits in the
*  exponent.  Because a full table of powers of ten out to DBL_EXP_MAX
*  would be quite large, the scale factor is computed on the fly if
*  outside the range 10**(-16) to 10**(+16).  The rather curious loop
*  structure is used to avoid overflow when 'power' gets squared.
*
*  The scaled argument may be outside the range 1.0 <= arg < 10.0
*  near an exact power of ten, but this can also be caused (or cured)
*  by decimal rounding.  When overflow is detected, program restarts
*  at doEfmt with sadj = 1, thus taking care of the rare case that
*  the exponent width also needs to expand by 1.
*
*  The idea of a preliminary test here that would exit to print stars
*  if not enough room for one digit plus exponent was not implemented
*  given rarity of this case and complexity of getting it right.  */

doEfmt:
   absarg = fabs(arg);        /* In case restarted */
   exp = (int)floor(log10(absarg)) + sadj;
   absexp = abs(exp);
   if (absexp > STT) {        /* Need power of ten outside table */
      double power;           /* Temporary power of ten */
      register int taexp = absexp;
      factor = tens[taexp & (STT-1)];
      for (power = tens[STT]; ; power *= power) {
         if (taexp & STT) factor *= power;
         if ((taexp >>= 1) < STT) break; }
      }
   else factor = tens[absexp];

   /* Apply the scale factor */
   if (exp > 0) absarg /= factor;
   else if (exp < 0) absarg *= factor;

/* Calculate the width needed for the exponent.  The maximum width of
*  an exponent, including sign, is given by the variable EXP_SIZE in
*  sysdef.h and the program does not check for violations.  EXP_SIZE
*  is used if RK.expwid was not preset, otherwise, the larger of
*  RK.expwid or the size actually needed is used.
*
*  If E format was not forced, calculate expwid and check overflow.
*  Calculation of total width is based on one prefix character (blank
*  or sign(arg), 1 for integer part, 'dec' for decimals (unless
*  RK_AUTO), and 'expwid' for the exponent (including its sign).
*  Since 'width' is 1 less than the true width, one less is added.
*  The code allows us to have no exponent at all if 'expwid' is 1.
*  If overflow is predicted, switch to forced mode, which uses the
*  minimum possible width.  Otherwise, if autoscale was requested,
*  calculate final decimal scale, allowing one extra blank space.  */

   if (!forced) {
      expwid = (int)RK.expwid;
      while (absexp >= decades[expwid]) expwid++;
      if (expwid > EXP_SIZE) expwid = EXP_SIZE;
      if (davail - expwid < 1) forced = TRUE;
      else if (autodec) {
         dec = width - expwid - lpfx - 1;
         if (dec > dec0) dec = dec0;
         }
      }

/* If E format was forced, recalculate the width necessary for the
*  exponent, ignoring the 'expwid' parameter.  Total width will be
*  1 for a blank, 1 for sign(arg), 1 for integer part, and 'expwid'
*  for the exponent (including its sign).  Any space that is left
*  can be used for decimal data.  */

   if (forced) {
      for (expwid = 0; absexp >= decades[expwid]; expwid++) ;
      dec = width - expwid - lpfx - 1;
      /* If dec < 0, all is lost, output asterisks */
      if (dec < 0) goto fillstars;
      if (autodec && dec > dec0) dec = dec0;
      }

/* Now drop into F format with argument scaled */

/*---------------------------------------------------------------------*
*                              F Format                                *
*---------------------------------------------------------------------*/

doFfmt:

/* Scaling and rounding.  (May reenter here with dec reduced by 1 if
*  rounding overflow occurs in RK_AUTO mode.)  When originally tested
*  with HAS_I64, bcdout was found to give different answers from the
*  32-bit code in certain cases where a fraction was very close to 1
*  after scaling and rounding.  This occurred when an 80-bit floating
*  register was cast to a 64-bit unsigned long integer, e.g.  53/160 =
*  0.33125 (exact in base 10) = 0x3fd5333333333333 (double) =>
*  0x400acf0ffffffffffeoc (reg) after scaling by 10000 for d=5 and
*  adding 0.5 (gdb prints this number as 3312.999999999999889 base
*  10).  Curiously, this number casts to 3312 as an unsigned long, but
*  to 3313 as an unsigned long long.  Although 3313 is more correct as
*  the rounded decimal result, 3312 is more correct given the original
*  truncated representation of 0.33125.  It was found that explicit
*  use of the floor() function seems to give the same (lower) results
*  in both cases.
*
*  Note added, 12/29/09, GNR:  It was found that the rounding line
*  using the floor() function gives different results as documented
*  above depending on whether gcc compilation is with -g (round up)
*  or -O2 (round down).  Attempts to use the following all failed:
*  pragma GCC optimize ("0")
*  pragma GCC optimize ("float-store")
*  __attribute__ ((optimize(0)))  [This ignored with a warning].
*  (SunOS acc compilation rounds up in both cases.)
*  After much testing, it was found that using trunc() in place
*  of floor() gave the same result (rounded up) in all cases
*  with gcc, but acc does not have this function.
*
*  Further note added, 09/05/11, GNR:  After some other changes, the
*  difference between -g and -O2 returned (Cove Rd older system).
*  I replaced "trunc(absarg + 0.5)" with "round(absarg)" and now
*  both compilation options gave the correct rounding behavior.
*/

   if (dec > STT) dec = STT;        /* Avoid index error */
   dgt0 = (dec > 0);
   absarg *= tens[dec-dgt0];
#ifdef GCC
   absarg = round(absarg);          /* Round */
#else
   absarg = floor(absarg + 0.5);    /* Round */
#endif

/* Now finally can check for underflow if RK_UFLW mode requested */

   if (ic & RK_UFLW && absarg < 1.0) goto force;

/* Check for overflow of the integer part past the start of the output
*  field.  Here the documented restriction is enforced that no more
*  than OUT_SIZE decimal digits can be produced (anything to the right
*  of that would be meaningless anyway).  To lift this restriction,
*  the tens table must be expanded at least enough to implement the
*  absarg test in the 'else' clause below, and the binary-to-decimal
*  conversion modified accordingly.  */

   davail = width + (1-dgt0) - lpfx - expwid - (autodec & ~forced);
   if (davail <= 0) goto force;     /* Just in case */
   if (mode == RK_EFMT) {
      if (absarg >= tens[dec+(1-dgt0)]) {
         if (sadj) abexit(41);
         sadj = 1; goto doEfmt; }
      }
   else {
      if (absarg >= tens[min(davail, (OUT_SIZE+1))]) {
         if (autodec && dec > 1) {
            dec -= 1; absarg = fabs(arg); goto doFfmt; }
         else
            goto force;
         }
      }

/*---------------------------------------------------------------------*
*                      Convert binary to decimal                       *
*---------------------------------------------------------------------*/

/* Convert the exponent and store right-to-left */

   if (expwid > 0) {
      char *pse = rd - expwid + 1;     /* Ptr to sign of exponent */
      if (pse <= field) abexit(41);    /* Just in case */
      while (ld > pse) {
         div_t erq = div(absexp, 10);
         *ld-- = hextab[erq.rem];
         absexp = erq.quot;
         }
      *ld-- = (exp >= 0) ? '+' : '-';
      }

/* In the absence of 64-bit arithmetic, this conversion is done in one
*  or two pieces according to whether the number will fit in one or
*  two ui32 integers.  It would not be too hard to generalize this
*  code to eliminate the restriction OUT_SIZE <= 2*(LONG_MAX-1), but
*  there is no clear need to do so.  Because absarg has been forced to
*  an integer, it is assumed that there will be no rounding errors in
*  the breakdown to high and low pieces.  */

#ifdef HAS_I64
   /* ui64 may be unsigned long or unsigned long long */
   {  ui64 quot, warg = (ui64)absarg;
      do {
         quot = warg/10;
         *ld-- = hextab[warg - 10*quot];
         warg = quot;
         } while (warg);
      } /* End warg local scope */
#else
   {  ui32 high, low;
      if (absarg >= tens[UI32_SIZE_M1]) {
         char *rde = rd - expwid - UI32_SIZE_M1;
         high = (ui32)(absarg/tens[UI32_SIZE_M1]);
         low  = (ui32)(absarg - (double)high*tens[UI32_SIZE_M1]);
         do {
            ldiv_t lrq = ldiv(low, 10);
            *ld-- = hextab[lrq.rem];
            low = lrq.quot;
            } while (low);
         while (ld > rde) *ld-- = '0';
         }
      else
         high = (ui32)absarg;
      do {
         ldiv_t vrq = ldiv(high, 10);
         *ld-- = hextab[vrq.rem];
         high = vrq.quot;
         } while (high);
      } /* End high,low local scope */
#endif

/*---------------------------------------------------------------------*
*            Insert decimal place and minus sign in result             *
*                                                                      *
*  There are three cases:                                              *
*     1) No decimal                                                    *
*     2) Decimal goes to left of MSD--add leading zeros.               *
*     3) Decimal goes at or to right of MSD (most signif digit)--      *
*        must move everybody left to make room for decimal.            *
*---------------------------------------------------------------------*/

   if (dec) {                       /* Decimal must be inserted */
      char *iloc = rd-expwid-dec;   /* LSD integer location */
      if (ld > iloc) {              /* Decimal goes left of MSD */
         while (ld >= iloc) *ld-- = '0';
         }
      else {                        /* Decimal goes right of MSD */
         char *tld;
         for (tld=ld--; tld<=iloc; ++tld) *tld = *(tld+1);
         }
      *(iloc+1) = '.';           /* Stick the decimal in */
      }

/*---------------------------------------------------------------------*
*                      Final alignment and fill                        *
*                                                                      *
*  The above code now places the result right-aligned in the user's    *
*  data field.  If left alignment is requested, now move data left     *
*  to final position and fill to right.  (Zero padding is used if      *
*  requested, even though this may change the magnitude.)  Otherwise,  *
*  fill to left with designated fill character.                        *
*---------------------------------------------------------------------*/

/* If the argument is negative, insert minus sign in result */

insrtpfx:

   while (lpfx--) *ld-- = *upfx--;

/* Store length of result */

finout:
   RK.length = rd - (++ld);
   if (RK.length < 0) abexit(41);

/* Perform left-justification if requested */

   if (ic & RK_LFTJ) {
      if (ld > field) while (ld <= rd) *field++ = *ld++;
      while (field <= rd) *field++ = fill;
      }

/* Data are right-justified, fill on left as needed */

   else while (ld > field) *(--ld) = fill;

   return;

fillstars:
   memset(field, '*', width+1);
   RK.length = width;
   return;

   } /* End bcdout() */
