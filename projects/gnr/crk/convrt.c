/* (c) Copyright 1989-2017, The Rockefeller University *11115* */
/* $Id: convrt.c 66 2018-03-15 19:01:23Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                  INFORM, SINFORM, CONVRT, SCONVRT                    *
*                                                                      *
************************************************************************
*  V1A, 01/13/89, GNR - Convrted from System 370 Assembler version     *
*  V1B, 02/24/89, GNR - Change ^ code to give absolute item size       *
*  V1C, 03/06/89, GNR - Make inform not input without '/' code         *
*  Rev, 03/10/89, GNR - Require width with 'A' format, allow #^        *
*  Rev, 04/03/89, GNR - Put subroutines first, allow for fact that     *
*     'toupper' may be a macro.                                        *
*  Rev, 04/19/89, GNR - Incorporate rkxtra.h                           *
*  V1D, 06/22/89, GNR - Terminate sconvrt strings with null            *
*  V1E, 11/20/91, GNR - Add 'O' code to kill null termination          *
*  Rev, 08/03/92, GNR - Use RK_CCL if no valid carriage control        *
*  Rev, 01/05/94, GNR - Trap format codes lacking item pointers        *
*  V1F, 05/22/96, GNR - Correct initial priority bug, add output       *
*     codes for subtitles, add '|' code for indented tables, pass      *
*     buffer length to fmtio, use RK.pgcls instead of LNSIZE.          *
*  V1G, 01/25/97, GNR - Add V+, V~, V>, V(, V), R~ codes, U for        *
*     unsigned integers, fix indexing bug on zero repeat count,        *
*  Rev, 02/09/97, GNR - Add RK.numcnv counter.                         *
*  Rev, 09/07/98, GNR - Allow scanned Aw width > DFLT_MAXLEN           *
*  Rev, 07/01/00, GNR - Flush output after 'W' code.                   *
*  Rev, 09/02/00, GNR - Fix bug: (S=An) and key=val sets arg to "key"  *
*  V1H, 08/25/01, GNR - Add T (input) format code                      *
*  V1I, 09/23/01, GNR - Add K code, return to caller if parens w/o V(  *
*  V1J, 02/01/03, GNR - Add metric unit multipliers ($ codes)          *
*  V1K, 09/13/03, GNR - Add .<n and Kw constructions                   *
*  V1L, 02/15/05, GNR - Add VA construction                            *
*  Rev, 03/24/07, GNR - Check RK.plevel, not just RK_INPARENS, add @   *
*  V1M, 12/30/07, GNR - Add generic input value checking               *
*  Rev, 05/14/08, GNR - Fix bug: (S=An) but len(key) > n in key=val    *
*  Rev, 05/18/08, GNR - Fix bug:  Init fplev with curplev, not plevel  *
*  ==>, 06/19/08, GNR - Last date before committing to svn repository  *
*  V1O, 02/15/09, GNR - Add svvadj/ckvadj mechanism, remove ICC fixes  *
*                       Add 'N(' input flag--parens not an error       *
*  Rev, 05/04/09, GNR - Add J code on input to call kwjreg routine     *
*  V2A, 08/16/09, GNR - Revise for 64-bit, use wbcdin, wbcdwt          *
*  Rev, 12/26/09, GNR - Add IS code for wseed random seeds             *
*  Rev, 01/18/11, GNR - On input scan, an empty field skips the item   *
*  Rev, 07/29/11, GNR - Check for bracket count finished on semicolon  *
*  Rev, 01/24/12, GNR - 'V' code for RK_NZTW with hex output           *
*  Rev, 05/12/13, GNR - Add 'IZ', 'UZ' codes for size_t variables      *
*  Rev, 05/31/13, GNR - Fix bug:  jfbl was not reset on new field      *
*  Rev, 08/31/13, GNR - Add 'M', 'B' type codes for integers           *
*  Rev, 09/02/13, GNR - Eliminate no-longer-needed immflgs PADWITH0    *
*  Rev, 02/15/16, GNR - Add 'R?' code, correct SKIP codes for 64-bit   *
*  Rev, 03/01/17, GNR - Add '-' code to call wbcdwt with RK_MZERO code *
*  R66, 12/15/17, GNR - Pickup scale from list if B with no digits,    *
*     use second binary scale in format if '?' and RK_BSQUEST is set.  *
***********************************************************************/

/*---------------------------------------------------------------------*
*        Usage                                                         *
*---------------------------------------------------------------------*/

/*
   These routines provide facilities for formatted input (inform,
sinform) and output (convrt, sconvrt).  They are essentially inter-
preters which generate calls to the low-level ROCKS interface
routines under control of a pseudo-FORMAT statement.  Subroutines
inform and convrt perform I/O through cryin and cryout, respectively,
while sinform and sconvrt operate on buffers in memory.  When using
sinform or sconvrt, it is an error to process more than one record
or to use the L, P, or Y format codes.

Input usage:
   void inform(char *format, void *item, ..., NULL)
   void sinform(char *card, char *format, void *item, ..., NULL)
Output usage:
   void convrt(char *format, void *item, ..., NULL)
   void sconvrt(char *line, char *format, void *item, ..., NULL)

Arguments: 'card' is a pointer to an input string to be converted
     by sinform.  Subroutine inform can be made to read a card
     internally by including a '/' in the format code.  Normally,
     however, inform will be used for scanning a card that has
     already been read by cryin.  In that case, inform will use
     RK.last as the card location.

         'line' is a buffer large enough to hold the output gene-
      rated by sconvrt.  A standard null end-of-string character
      is inserted unless the 'O' format code is specified.  Sub-
      routine convrt calls cryout internally to write each output
      record that is generated.

         'format' is the equivalent of a FORTRAN FORMAT statement
      (without the word "FORMAT") enclosed in parentheses and double
      quotes.  It may contain any of the FORTRAN format codes except
      C, G, L, or P.  (L and P have different meanings in convrt.)

         'item', ... represents a list of pointers to the variables
      or arrays to be converted.  Control variables used with the
      L, R, T, ^, and # codes are passed as pointers to integers.
      The list of items must be terminated with a null pointer.

   There are several major differences in usage between [s]inform
and [s]convrt, FORTRAN formatted I/O, and the C routines [s]scanf
and [s]printf, which they resemble.  Most important, the types and
sizes of all the 'item' arguments must be explicitly coded in the
format string.  This process is described in detail in the writeup.

   A comma between two format specifications is optional except when
needed to separate two numbers, e.g. (I2I4,3F6.2).  Commas in other
positions are ignored.  Blanks are treated as if they were commas.

   The largest valid width specification is (RK_MXWD+1 = 32), except
for H and T format codes, which may have widths up to 256 characters.

   Individual format codes are interpreted as follows (additional
information may be found in the writeup):
   Aw Alphanumeric.  Must be followed by 'w', the maximum number of
      characters to be transmitted.  On input, shorter strings are
      truncated by an end-of-string character.  Longer strings give
      an error message.  On output, shorter strings are padded with
      blanks to 'w' characters unless the 'A' code is preceded by
      'Jn', in which case the actual string length is used, followed
      by 'n' blanks.  'w' may be followed by a '.d' specification, in
      which case 'd' is the displacement to the next variable (in
      characters) for indexing purposes (same as ^ code).  If not
      specified, d=w.
   .A  Automatic decimal placement.
   B[n]  Binary scaling (with I, U, or M). May be followed by 'n', the
      number of bits to the right of the binary point (0 <= n < 63).
      'n' may consist of two integers separated by a '/', '|', or '?'.
      With '/', the second scale is used if the RK_BSSLASH bit was set
      by a previous call to bscompat().  With '|', the second scale is
      used if the RK_BSVERTB bit was so set.  And with '?', the second
      scale is used if the RK_BSQUEST bit was set.  If a value for 'n'
      is not given, the next list item, which must be a pointer to int,
      is used for 'n'.  
      The B specification follows the repeat count and any V code and
      applies only to the immediately following format code.
   D  Double precision exponential (same as FORTRAN).
   E  Exponential (same as FORTRAN).
   F  Floating (same as FORTRAN).
   H  Hollerith (same as FORTRAN).
   I[C|H|M|I|J|L|B|S|W|Z]  Integer.  The I code may be followed by a
      type modifier and a 'w.d' specification.  The type modifiers
      C,H,M,I,J,L,B,S,W, and Z specify character, short, short, integer,
      si32, long, sbig, wseed, si64, size_t, respectively.  The default
      is int.  The decimal is normally used only with binary scaling.
   J[n]  Input:  Calls routine 'n' (0-9) registered by a previous
      call to kwjreg.  Everything after the 'n' until the next comma
      is used as the first argument to the routine (max 7 chars).
   J[n]  Output:  Left-Justify.  If a value for 'n' is given, 'n'
      blanks are inserted to the right of the output field--the width
      of the field is data dependent and the width given in the format
      is only a maximum width (which does not include the 'n' blanks).
      If a value for 'n' is not given, enough blanks are inserted to
      fill out the width requested in the associated format item.
   K  Input:  Keys expected.  With scanned input (after an S code),
      indicates that nonnumeric keywords may appear after this point
      in the input.  When an nonnumeric data item is found, inform()
      calls scanagn() and then returns so the calling program can call
      kwscan().  Any remaining format items are ignored.  Implies N,
      so no error message is generated for missing fields--these
      should be set to default values before calling inform().
      Without the 'K' code, nonnumeric items are treated as data.
   K[C|H|I|J|L]w  Output:  Binary option Keys.  This format requires
      two arguments in the argument list.  The first is an ASCII string
      specifying the codes to be printed for each 1 bit in the second
      argument, which is a pointer to the data item to be converted.
      Characters other than uppercase letters and digits in the code
      specification indicate bit positions in the data that are not
      encoded in the output.  The K code may be followed by a type
      modifier C,H,I,J, or L to specify that the corresponding argument
      is an unsigned character, short, int, ui32, or long, respectively
      (default unsigned int).  This must be followed by 'w', the maxi-
      mum number of characters to be transmitted.  Shorter strings are
      padded with blanks to 'w' characters unless preceded by 'Jn', in
      which case the actual string length is used, followed by 'n'
      blanks.  Not usable inside <> brackets.   (L is a deprecated
      synonym for J for compatibility with old programs.)
   L[n]  Lines.  Force a page eject if the next 'n' lines of output
      would not all fit on the current page.  If a value for 'n' is
      not given, the next list item, which must be a pointer to int,
      is used for 'n'.  (Calls tlines, so print with convrt or cryout.)
   M  "Midi".  M is an alternative code for halfword (short) integers,
      identical to 'IH'.  This usage may be removed in a future version.
   N[n|(]  Input:  Not-required or call registered routine.  With scan
      input, if followed by a digit 'n' (0-9), it calls routine 'n'
      registered by a previous call to kwsreg.  (If variable is an
      array, use '^' to specify size.)  If 'n' is not present, 'N'
      prevents an error message if the end of the input card is reach-
      ed before values have been assigned to all the variables in the
      input list.  Additional input cards, other than continuations,
      are never read to fill in missing variables.  Aditionally, 'N('
      indicates that a left parenthesis found without a prior 'V('
      code returns to the user with no error.
   N  Output:  Not-forced.  Sets the RK_NFSUBTTL bit, so the output
      is interpreted as a subtitle which does not force a new page.
   O  Output:  Overwrite.  Omit putting '\0' at end of output string
      produced by sconvrt.  Ignored in other cases.
   Pn  Priority.  Set the priority of the printed output to 'n'.
   Q  "Quality".  Double precision floating point format.
   R[?][~] Repeat.  R may be used instead of a repeat count before (),
      <>, or a format code.  It indicates that a list item, assumed to
      be a pointer to int, is read and used for the corresponding
      repeat count.  Before format codes, but not with () or <>, the
      repeat count may be zero to skip the next format code.  If the
      list item is 0 or 1 and R is preceded by a number, that number
      is used as the number of items to skip or process, respectively.
      If R is followed by a '~', data are skipped on input and blanks
      are inserted on output corresponding to the skipped data items.
      If R is followed by a '?', the repeat count is saved but not
      used.  Any following R code without '?' (for example, repeating
      a parenthesized format group), uses the saved count and a list
      item is not consumed.  Both '?' and '~' modifiers cannot be used
      with the same 'R' code.
   S[[W]n]  Input:  Scan.  On input, S indicates that the input is to
      be scanned.  W, if present, indicates that 'n' is the number of
      Words to skip, otherwise 'n' is the number of columns to skip.
      inform() calls cdscan() unless 'n' is omitted, in which case it
      assumes that cdscan() was already called.  For each list item,
      one field is scanned and converted.  With A format, the field is
      truncated with 0 if shorter than the width specification.
      Longer fields give an error.  With numeric formats, the width
      specification is ignored and all characters read by scan() are
      included in the conversion (max 32).  Upon return from inform(),
      kwscan() may be called to interpret any keyword parameters that
      may follow the fields read by inform().
   S  Output:  Subtitle.  Sets the RK_SUBTTL bit, causing the output
      to be treated as a new subtitle.
   T[C|H|I|J|L]n  Input:  Text.  The value is a string of up to 'n'
      characters.  It is read in and saved in the text cache by a call
      to savetxt().  The text locator is converted to an unsigned char,
      short, int, ui32, or long if the T code is followed by C,H,I,J,
      or L, respectively (default: unsigned int).
   T[n]  Output:  Set the record poinTer to column 'n'.  The output
      buffer is not blanked, allowing output from more than one call
      to be combined.  Use nX to blank the buffer if necessary.  When
      the line is printed, everything up to the current pointer is
      included.  Some output may be lost if the pointer is backspaced.
      If 'n' is not given in the format, a list item, assumed to be a
      pointer to int, is read and used for the column number.  This
      feature is useful for making graphs on the printer.
   U[C|H|M|I|J|L|B|W]  Unsigned integer.  The U code may be followed
      by a type modifier and a 'w.d' specification.  U specifies an
      unsigned integer conversion.  The type modifiers C,H,M,I,J,L,B,
      and W specify byte, ui16, ui16, int, ui32, long, ubig, and ui64
      lengths, respectively (default unsigned int).  The former usage
      of U as a code for underflow control is no longer accepted.
   V[<[=][nnn]|~|>[=][nnn]]  Input:  Verify or Underflow control.
      Placed before a format code on numeric input, V<nnn or V<=nnn
      gives an error if items read by the following format spec are
      >=nnn or >nnn, bzw.  V>nnn or V>=nnn gives an error if the data
      are <=nnn or <nnn, bzw.  If 'nnn' is omitted, it defaults to 0.
      V~ gives an error if the value is zero.  Multiple tests may be
      combined in one code.  On alphabetic input, V causes the string
      to be converted to upper case.
   V[(|)]  Input:  Verify data in parentheses on scanned input.  When
      a set of format codes is placed between V( and V), the input data
      items (if more than one) must be enclosed in parentheses.  If
      there has not been an N or = code and ')' or end of scan is found
      before all the data items have been read, an error occurs.  If
      all the data items are satisfied before the ')' is found in the
      data, an error occurs and the excess data are skipped.  When
      parentheses are found and 'V(' has not been coded (or the condi-
      tions of the '=' or 'K' format codes are satisfied), control
      returns to the caller and the rest of the format is skipped.
   V  Output:  If decimal, causes output generated by the following
      format specification to use E format if all precision otherwise
      would be lost.  If hex, causes output to use the number of
      characters implied by the item width.  (Code V+, never used,
      has been removed.)
   W  Input:  Warning.  Same as 'V' except gives a warning rather than
      an error for data out of range.  The value is replaced with a
      value just inside the allowed range.  If followed directly by
      a comma, calls cdprt1().
   W  Output:  Write.  Causes current line to be flushed.
   X  Blanks, same as FORTRAN.
   Y[n]  Output:  Spout the next 'n' lines of output.  If 'n' is not
      present, a value of 1 is assumed.
   Z  Hexadecimal, same as FORTRAN.
   @  Pad with zeros rather than blanks on output.
   #[n]  Gives dimension of next array as a value or list item.
   '  Output:  Literal strings may be given, enclosed in single quotes.
      A quotation mark within such a string must be written as two
      quotation marks.  Same as FORTRAN.
   ()  Format codes enclosed in parentheses are repeated, as in FOR-
      TRAN.  If the end of the I/O list is reached, interpretation
      of the format ends at the next right parenthesis and following
      literals are not printed.  Only one level of nesting of paren-
      theses is allowed.
   *  Indicates repeat count applies to separate list items.
   -  Perform fixed-point output with RK_MZERO code so most-negative
      number is printed as '-0'.
   +  A + code appearing before an array specification following an N
      or = format code indicates that values must be provided on the
      input card for either all members of the array or none.  (With-
      out the N or =, all values must be provided.  With N or = but
      not +, any number of values can be provided.)
   ,  Format separator.  Optional except to separate numbers or
      ambiguous code combinations.
   /  New record, same as FORTRAN.  Calls cryin() for input.
   :  Same as ;
   ;  On output, causes interpretation of the format to terminate if
      there are no more list items to be converted.  It may be used
      to turn off the conversion of following literals when paren-
      theses are not involved.
   <>  See discussion in writeup.
   .<n Variant of automatic decimal in which the decimal parameter is
      the lesser of 'n' and the automatic value needed to just fit
      the value in the given width with one leading blank.
   =  With scanned input (after an S code), indicates that keyword
      parameters ('key=value') may appear after this point in the
      input.  When an equals sign is found, inform() calls scanagn()
      and then returns so the calling program can call kwscan().  Any
      remaining format items are ignored.  Implies N, so no error
      message is generated for missing fields--these should be set
      to default values before calling inform().  Without the = code,
      an equals sign in the input does generate an error message.
      In situations other than scanned input, the = code gives the
      index increment for arrays in <> repeats.
   ^[n]  Overrides item size for next format item.  n is in bytes
      as integer value or list item.
   |  Indent.  On output, records the current column location.  Fol-
      lowing this code, when scanning returns to a left parenthesis
      on a new line, a sufficient number of blanks is inserted to
      cause the output to be indented to the recorded position.

   Codes I,U,F,Q,D may be followed, after any C,H,I,J,L,W or .d speci-
fication, by a '$' and a metric units specification as defined in
muscan.c.  Also, variables read with these codes (numeric values)
may be followed in the input by a '+' or '-' sign and the name of
an adjustment value.  If a value with that name was stored by a
previous call to svvadj(), the adjustment is added to the input
value before it is returned.

*/

/*--------------------------------------------------------------------*
*        Error actions:                                               *
*--------------------------------------------------------------------*/

/*
   The application is terminated with the indicated code if any of
the following errors occurs in the construction of a format string:

   011   Format does not begin with '('.
   012   Unrecognized format character.
   013   End of I/O list reached while trying to pick up 'n' for
            L, R, T, #, or ^ format code, or for any conversion.
   014   Digit not found when required (e.g. width of an item or
            after B code).
   015   Scanning attempted on output, justify or indent attempted
            on input.  Input to a Hollerith string attempted.
   016   Buffer length exceeded.
   017   Negative or zero repeat count.
   018   Attempted I/O from sconvrt or sinform call.
   019   Unmatched quotes in output format or V() in any format.
   020   Invalid decimal point specification.
   043   Attempt to call an unregisterd kwsreg routine.
*/

/*--------------------------------------------------------------------*
*        Global definitions                                           *
*--------------------------------------------------------------------*/

#include <ctype.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rocks.h"
#include "rockv.h"
#include "rkxtra.h"

/* Define the mode bits passed to the fmtio routine */

#define  OUT   0              /* Output mode */
#define  INM   1              /* Input mode */
#define  RDW   2              /* Force write, allow read */

/* Define the values CVC.indexsw may take */

#define  INDEXNONE   0        /* No indexing in effect */
#define  INDEXING    1        /* Now indexing */
#define  INDEXPEND   2        /* Index loaded and pending */
#define  INDEXBKT    4        /* Bracket indexing done */

/* Define the values CVC.flags may take */

#define  LPARENOK    2        /* Terminate scan on left paren */
#define  ALLFLDRQ    4        /* All fields required */
#define  EQSIGNXX    8        /* Equals sign not allowed */
#define  KEYXPECT   16        /* Terminate scan on nonnumerics */
#define  SCANNING   32        /* Scanning input */
#define  NULLTERM   64        /* Add null terminator */
#define  GOTRQUES  128        /* Repeat count entered as 'R?' */

/* Define the values CVC.immflgs may take */
#define  AINUPPER    2        /* Uppercase alphanumeric input */

/* Define the values CVC.vparens may take */

#define  PARENREQ    1        /* Field must be in parens */
#define  PARENINS    2        /* Inside parenthesized set */
#define  PARENEND    3        /* Must reach end of parens */
#define  PARENOPT    4        /* Parens optional here */

/* Define bits used with rep to indicate skipping data.  (These
*  bits interpreted as sbig integers must be negative numbers.)  */

#ifdef BIT64
#define  SKIPIDAT 0xc000000000000000L  /* Skip item and data */
#define  SKIPINOD 0x8000000000000000L  /* Skip item but not data */
#else
#define  SKIPIDAT 0xc0000000L          /* Skip item and data */
#define  SKIPINOD 0x80000000L          /* Skip item but not data */
#endif

/* Define jfbl code used with left-justification to indicate
*  that variable should be padded to full width in format code.  */

#define  JFULLWID 8192        /* About SHRT_MAX/2 */

/* Define maximum number of chars in a kwjregfn first arg */

#define  KWJMXARG 7           /* Arbitrary, should be big enough */

/* Define format conversion type codes.
*  The kco set is for output, the kci set for input.
*  Some of these are just placeholders so all corres-
*  ponding input and output items are equally spaced.
*  Warning:  Some tests depend on order of these items.  */

   enum kc_def {
      /* Cases in first row are unique to input or output */
      kcokchar, kcokshort, kcokui32, kcisreg, kcijreg,
      kcoafmt, kcotchar, kcotshort, kcotui32, kcotlong,
      kcowseed, kcofixpt, kcofloat, kcodouble,

      kciafmt, kcitchar, kcitshort, kcitui32, kcitlong,
      kciwseed, kcifixpt, kcifloat, kcidouble };

/* Common variables private to this program file */

   static struct {
      char *dptr;                /* Data pointer */
      char *fptr;                /* Format pointer */
      void *iptr;                /* Item pointer */
      char *lea;                 /* Scanning buffer--now uses
                                 *  high-water-mark allocation.  */
      char *line;                /* Convrt line buffer--initial value
                                 *  NULL assumed per C language std. */
      va_list lptr;              /* List pointer */
      va_list bklp;              /* Bracket initial list pointer */
      sbig dig;                  /* Value of format digit string */
      sbig dim;                  /* Current array dimension */
      sbig idim;                 /* Initial array dimension */
      sbig svrep;                /* Saved repeat from R? */
      int  isub;                 /* Subtitle control bits */
      int  llea;                 /* Current length of lea */
      int  prty;                 /* Print priority */
      byte indexsw;              /* Array indexing */
      byte flags;                /* Permanent flags defined above */
      byte immflgs;              /* Flags valid for one conversion */
      byte vparens;              /* State of 'V' input paren codes */
      } CVC;

/*--------------------------------------------------------------------*
*                                                                     *
*        digits() - pick up a decimal value from format string        *
*                                                                     *
*        Returns FALSE if no digits found, otherwise TRUE             *
*        Decimal value is in CVC.dig and fptr points to last digit    *
*                                                                     *
*--------------------------------------------------------------------*/

static int digits(void) {

   register sbig value;
   if (!isdigit((int)*(CVC.fptr+1))) return FALSE;
   value = (sbig)(*(CVC.fptr+1) - '0');
   while (isdigit(*(++CVC.fptr+1)))
      value = 10*value + (sbig)(*(CVC.fptr+1) - '0');
   CVC.dig = value;
   return TRUE;
   } /* End digits */

/*--------------------------------------------------------------------*
*                                                                     *
*        listitem() - check for more list items and update iptr       *
*                                                                     *
*        Returns with dim = 0 if at end of list, otherwise dim = 1    *
*           and CVC.iptr loaded from next list item                   *
*                                                                     *
*--------------------------------------------------------------------*/

static void listitem(void) {

   if (--CVC.dim) return;     /* If in array, return at once */
   if (CVC.indexsw != INDEXPEND) { /* End of array, kill indexing */
      CVC.indexsw = INDEXNONE; CVC.idim = 1; }
   /* If no more list items, return with dim = 0 */
   if ((CVC.iptr = va_arg(CVC.lptr,void *)) == NULL) return;
   /* Otherwise, return with dim = 1 and new iptr set */
   CVC.dim = 1; return;
   } /* End listitem */

/*--------------------------------------------------------------------*
*                                                                     *
*        pickitem() - pick up current item and advance iptr           *
*                                                                     *
*        Returns list item in CVC.dig                                 *
*                                                                     *
*--------------------------------------------------------------------*/

static void pickitem(void) {

   if (CVC.dim != 1) abexit(13); /* Error if array or no item */
   CVC.dig = (sbig)*((int *)CVC.iptr); /* Get item */
   listitem();                /* Advance list pointer */
   return;
   } /* End pickitem */

/*--------------------------------------------------------------------*
*                                                                     *
*        slash() - read or write a formatted record                   *
*                                                                     *
*        Arguments:  buf is the location of the current buffer        *
*                    mode is the fmtio mode control integer           *
*        Returns the location of the next buffer                      *
*                                                                     *
*--------------------------------------------------------------------*/

static char *slash(char *buf, int mode) {

   if (!(mode & RDW))         /* Error if sconvrt, sinform */
      abexit(18);
   if (mode & INM) {          /* New input record */
      CVC.dptr = cryin();        /* Read another card */
      CVC.flags &= ~SCANNING;    /* Reset input flags */
      CVC.flags |= (ALLFLDRQ|EQSIGNXX);
      } /* End new input record */
   else {                  /* New output record */
      int recl = CVC.dptr - buf; /* Compute record length */
      if (!recl) {               /* If zero, output blank line */
         *CVC.dptr = ' ';
         recl = 1; }

      switch (*(CVC.dptr = buf)) {  /* Check carriage control */
      case ' ' :                    /* ' ': Single space */
         recl |= RK_LN1; break;
      case '0' :                    /* '0': Double space */
         recl |= RK_LN2; break;
      case '1' :                    /* '1': New page */
         recl |= RK_NEWPG; break;
      case '-' :                    /* '-': Triple space */
         recl |= RK_LN3; break;
      case '+' :                    /* '+': Overstrike */
         recl |= RK_LN0; break;
      default: ;                    /* Default: Continue line */
         } /* End carriage-control switch */

      cryout(CVC.prty,buf,CVC.isub|recl,NULL); /* Print the line */
      } /* End output record */
   return CVC.dptr;
   } /* End slash */

/*--------------------------------------------------------------------*
*                                                                     *
*        fmtio() - Interpret a format string, convert data items      *
*                                                                     *
*--------------------------------------------------------------------*/

static void fmtio(int mode, char *buf, int bufl, char *fmt) {

   /* Tables of item sizes vs kc types */
   static int kcsize[kciafmt] = {
      sizeof(char), sizeof(short), sizeof(ui32), 0, 0,
      0, sizeof(char), sizeof(short), sizeof(ui32), sizeof(long),
      sizeof(wseed), 0, sizeof(float), sizeof(double) };
   char *gptr = fmt;          /* Group pointer */
   char *top = buf + bufl;    /* End of buffer signal */
   char *bkfp;                /* Bracket backup format pointer */
   void *bkip;                /* Bracket backup item pointer */
   char *mubkfp;              /* muscan backup format pointer */
   char *iokeys;              /* Ptr to keys for K formats */
   sbig bctr = 1;             /* Bracket count */
   sbig gct = 0;              /* Group count */
   sbig grp = 0;              /* Current group */
   sbig rep;                  /* Repeat count and skip flags */
   long incr = 1;             /* Index increment for brackets */
   long ndx = 0;              /* Bracket index */
   ui32 bs;                   /* Binary scale */
   ui32 code;                 /* bcdin/out conversion code */
   ui32 mucode;               /* muscan saved conversion code */
   enum kc_def kc;            /* Kind of conversion */
   int iadv;                  /* Item size override */
   int icol = 0;              /* Indent column */
   int csflags;               /* Scan flag word */
   int jfbl;                  /* Blanks after left-justify */
   int width;                 /* Field width */
   size_t itemsize;           /* Size of item */
   short fplev;               /* Format starting paren level */
   byte dimcheck;             /* TRUE to check array completion */
   byte liststep;             /* TRUE for list stepping (* code) */
   char kwjarg1[KWJMXARG+1];  /* Space for saving kwjregfn arg 1 */

/* Be sure format begins with left parens */

   if (*fmt != '(') abexit(11);

/* Initialize the CVC common block */

   RK.numcnv = 0;             /* Count total conversions */
   fplev = curplev();         /* Save initial parens level */
   CVC.dptr = buf;
   CVC.fptr = fmt;
   CVC.prty = RK_P2;
   CVC.isub = 0;
   CVC.flags = (ALLFLDRQ|EQSIGNXX|NULLTERM);
   CVC.vparens = 0;
   CVC.indexsw = INDEXNONE;
   CVC.dim = CVC.idim = 1;    /* Initial array dimension */
   listitem();                /* Set iptr if not end of list */

/* Here for new field */

newfield:                     /* Here when conversion completed */
   bs = 0;                       /* Clear binary scale */
   code = 0;                     /* Initialize BCD call */
   iadv = 0;                     /* Clear item size override */
   jfbl = JFULLWID;              /* Blanks after left-justify */
   CVC.immflgs = 0;              /* Clear one-conversion flags */
   liststep = FALSE;             /* Normal stepping */
   dimcheck = FALSE;             /* Omit array completion test */
   clrvalck();                   /* Clear value range check */

setrep:                       /* Here when Hollerith completed */
   rep = 1;                      /* Reset repeat count */

/* Here to examine new format code */

next: if (digits()) rep = CVC.dig;  /* Digits are repeat count */
   ++CVC.fptr;
   switch (toupper(*CVC.fptr)) {

   case ' ' :                 /* Blank: format separator */
      goto next;

   case '#' :                 /* Set array dimension */
      if (CVC.dim != 1) abexit(13); /* Error if in middle of array */
      if (!digits()) pickitem(); /* Get number or list item */
      CVC.idim = CVC.dig;        /* Set initial array dimension */
      CVC.indexsw = INDEXPEND;   /* Flag array indexing pending */
      goto next;

   case '\'' :                /* Quote: Hollerith string */
      if (mode & INM) abexit(15);
      {  char *nqt;              /* Next quote pointer */
         int dblqt;              /* Doubled quote flag */
         do {
            if ((nqt = strchr(++CVC.fptr,(int)'\'')) == 0) abexit(19);
            /* Test for doubled quotes; if found, include first but
            *  not second in memcpy and set flag to continue looping */
            if ((dblqt = (*(nqt+1) == '\'')) != 0) nqt++;
            if ((rep = nqt - CVC.fptr) != 0) {
               if (CVC.dptr + rep > top) abexit(16);
               memcpy(CVC.dptr,CVC.fptr,(size_t)rep);
               CVC.fptr += rep;  /* Update buffer and format ptrs */
               CVC.dptr += rep; }
            } while (dblqt) ;
         } /* End scope of nqt, dblqt */
      goto setrep;

   case '(' :                 /* Left Paren: Begin format group */
      if (rep <= 0) abexit(17);  /* Error if try to skip whole group */
      gct = grp = rep;           /* Set repeat count */
      gptr = CVC.fptr;           /* Save format pointer */
      goto setrep;

   case ')' :                 /* Right Paren: End format group */
      if (--grp) {
         int indent;             /* Number of columns to indent */
         if (grp < 0) {          /* End of whole format */
            if (!CVC.dim) break;    /* That's all, folks */
            buf = slash(buf,mode);  /* Perform read/write */
            grp = gct;              /* Restart group count */
            }
         if ((indent = buf + icol - CVC.dptr) > 0) {
            /* An indent was stored and we just moved before it.
            *  (No need to test for input mode, done when '|' found.) */
            memset(CVC.dptr,' ',indent);
            CVC.dptr += indent;
            }
         CVC.fptr = gptr;        /* Return to last '(' */
         } /* End format group, continue scanning */
      goto next;

   case '*' :                 /* Asterisk: Enter list mode */
      liststep = TRUE;
      goto next;

   case '-' :                 /* Minus:  Use RK_MZERO output format */
      code |= RK_MZERO;
      goto next;

   case '+' :                 /* Plus: Force array completion test */
      dimcheck = TRUE;
      goto next;

   case ',' :                 /* Comma: Format separator */
      goto next;

   case '/' :                 /* Slash: Read or write a record */
      buf = slash(buf,mode);
      goto next;

   case ':' :                 /* Colon: Conditional stop */
   case ';' :                 /* Semicolon: Conditional stop */
      if (bctr <= 1 && !CVC.dim) break;
      goto next;

   case '<' :                 /* Bra: Begin bracket group */
      if (rep <= 0) abexit(17);  /* Error if try to skip altogether */
      bctr = rep;                /* Rep count becomes group count */
      bkfp = CVC.fptr;           /* Save format backup address */
      bkip = CVC.iptr;           /* Save item pointer */
      memcpy((char *)&CVC.bklp,(char *)&CVC.lptr,sizeof(CVC.lptr));
                                 /* Save I/O list location */
      goto setrep;

   case '=' :                 /* Equals: Bracket incr or kwscan */
      if (CVC.flags & SCANNING)  /* If scanning, allow keywords */
         CVC.flags &= ~(EQSIGNXX|ALLFLDRQ);
      else {                     /* Otherwise, set index increment */
         if (!digits()) pickitem();
         incr = CVC.dig; }
      goto next;

   case '>' :                 /* Ket: End bracket group */
      if (--bctr) {           /* Still counting */
         ndx += incr;            /* Increment index */
         CVC.fptr = bkfp;        /* Backup format */
         CVC.iptr = bkip;        /* Backup item pointer */
         memcpy((char *)&CVC.lptr,(char *)&CVC.bklp,sizeof(CVC.lptr));
                                 /* Back up list pointer */
         CVC.idim = CVC.dim = 1; /* Reset array dimension */
         } /* End bracket group backup */
      else {                  /* Count exhausted */
         ndx = 0;                /* Restore normal indexing */
         incr = 1; }             /* FIXES BUG IN ASSEMBLER VERSION */
      goto next;

   case '@' :                 /* AT: Pad output with zeros */
      code |= RK_NPAD0;
      goto next;

   case '^' :                 /* Caret (EBCDIC not): Set item size */
      if (!digits()) pickitem(); /*** CAUTION: the item size is ***/
      iadv = (int)CVC.dig;       /*** taken to be in bytes!  ***/
      goto next;

   case '|' :                 /* Vertical bar: store indent column */
      if (mode & INM) abexit(15); /* Error if input */
      icol = CVC.dptr - buf;
      goto next;

   case 'A' :                 /* A: ASCII string */
      kc = kcoafmt;
      goto numeric;

   case 'B' : {               /* B: Binary point */
      /* Now we can get binary scale from list */
      if (!digits()) pickitem();
      bs = (ui32)CVC.dig;
      if (CVC.fptr[1] == '/') {
         ++CVC.fptr;
         if (!digits()) abexit(14);
         if (RK.bssel & RK_BSSLASH) bs = (ui32)CVC.dig;
         }
      else if (CVC.fptr[1] == '|') {
         ++CVC.fptr;
         if (!digits()) abexit(14);
         if (RK.bssel & RK_BSVERTB) bs = (ui32)CVC.dig;
         }
      else if (CVC.fptr[1] == '?') {
         ++CVC.fptr;
         if (!digits()) abexit(14);
         if (RK.bssel & RK_BSQUEST) bs = (ui32)CVC.dig;
         }
      code |= bs << RK_BS;
      goto next;
      } /* End 'B' local scope */

   case 'D' :                 /* D: Double-precision exponential */
      kc = kcodouble;
      code |= (RK_DBL|RK_EFMT);
      goto numeric;

   case 'E' :                 /* E: Single-precision exponential */
      kc = kcofloat;
      code |= (RK_SNGL|RK_EFMT);
      goto numeric;

   case 'F' :                 /* F: Single-precision floating */
      kc = kcofloat;
      code |= (RK_SNGL|RK_IORF);
      goto numeric;

   case 'H' :                 /* H: Hollerith */
      if (mode & INM) abexit(15); /* Illegal on input */
      width = (int)(rep & ~SKIPIDAT);
      /* N.B.  A switch here doesn't work on 16-bit machines */
      {  sbig repmode = rep & SKIPIDAT;
         if (repmode == 0) {              /* Process H data normally */
            if (CVC.dptr + width > top) abexit(16);
            memcpy(CVC.dptr, CVC.fptr+1, (size_t)width);
            CVC.dptr += width;
            }
         else if (repmode == SKIPIDAT) {  /* Skip item and data */
            if (CVC.dptr + width > top) abexit(16);
            memset(CVC.dptr, ' ', (size_t)width);
            CVC.dptr += width;
            }
         } /* End repmode local scope */
      CVC.fptr += width;         /* Skip over data in format */
      goto setrep;

   case 'U' :                 /* U: Unsigned fixed-point */
      code |= RK_NUNS;
      /* Drop through to case I intended ... */
   case 'I' :                 /* I: Integer or general fixed-point */
      ++CVC.fptr;
      if (toupper(*CVC.fptr) == 'S') {
         /* Special case: 'IS' is one or two random seeds */
         kc = kcowseed;
         code |= RK_IORF | RK_NI32;
         if (!iadv) iadv = sizeof(wseed);
         if (mode & INM) goto reginfn;
         }
      else if (toupper(*CVC.fptr) == 'Z') {
         kc = kcofixpt;
#if PSIZE == 8
         code |= RK_IORF | RK_NI64;
#else
         code |= RK_IORF | RK_NI32;
#endif
         if (!iadv) iadv = PSIZE;
         }
      else {
         kc = kcofixpt;
         code |= RK_IORF | wntype(&CVC.fptr);
         if (!iadv) iadv = wnclen(code);
         --CVC.fptr;          /* Backup fptr for digits() call */
         }
      goto numeric;

   case 'J' :                 /* J: kwjregfn or Left Justify */
      if (mode & INM) {
         char *pa1;
         if (!(CVC.flags & SCANNING) || !digits()) abexit(12);
         kc = kcijreg;
         for (pa1=kwjarg1; pa1<kwjarg1+KWJMXARG; ++pa1) {
            if (CVC.fptr[1] == ',' || CVC.fptr[1] == ')') break;
            *pa1 = *(++CVC.fptr); }
         *pa1 = '\0';
         goto reginfn;
         }
      code |= RK_LFTJ;           /* Insert code */
      jfbl = digits() ? (int)CVC.dig : JFULLWID;
      goto next;

   case 'K' :                 /* K: Keywords expected */
      if (mode & INM) {          /* K on input */
         if (CVC.flags & SCANNING) {
            CVC.flags |= KEYXPECT;
            CVC.flags &= ~ALLFLDRQ; }
         else
            abexit(12);
         goto next;
         }
      else {                     /* K on output */
         /* Check for length codes */
         switch (toupper(*(CVC.fptr+1))) {
         case 'C' :                 /* Character */
            ++CVC.fptr;
            kc = kcokchar;
            break;
         case 'H' :                 /* Halfword */
#if (ISIZE == HSIZE) && (ISIZE != I32SIZE)
         case 'I' :                 /* 16-bit int */
#endif
            ++CVC.fptr;
#if (ISIZE == HSIZE) && (ISIZE != I32SIZE)
         default:                   /* No code implies int type */
#endif
            kc = kcokshort;
            break;
#if ISIZE == I32SIZE
         case 'I' :                 /* 32-bit int */
#endif
         case 'J' :                 /* ui32 */
         case 'L' :                 /* Old 'long' = ui32 */
            ++CVC.fptr;
#if ISIZE == I32SIZE
         default:                   /* No code implies int type */
#endif
            kc = kcokui32;
            break;
            } /* End length code switch */
         iokeys = (char *)CVC.iptr;
         CVC.iptr = va_arg(CVC.lptr,void *);
         if (!iokeys) abexit(13);
         goto numeric;
         }

   case 'L' :                 /* L: Reserve lines */
      if (!digits()) pickitem(); /* Get number or list item */
      tlines((int)CVC.dig);
      goto next;

   case 'M' :                 /* M: Short integer */
      kc = kcofixpt;
      code |= RK_IORF | RK_NHALF;
      if (!iadv) iadv = sizeof(short);
      goto numeric;

   case 'N' :                 /* N: Not-required or not-forcing */
      if (mode & INM) {
         if (digits()) {
            if (!(CVC.flags & SCANNING)) abexit(12);
            kc = kcisreg;
            goto reginfn;
            }
         else {
            CVC.flags &= ~ALLFLDRQ; /* Next fields not required */
            if (*(CVC.fptr+1) == '(') {
               CVC.flags |= LPARENOK;
               ++CVC.fptr;
               }
            }
         }
      else
         CVC.isub ^= RK_NFSUBTTL;   /* Make non-forcing subtitle */
      goto next;

   case 'O' :                 /* O: Omit null terminator */
      CVC.flags &= ~NULLTERM;
      goto next;

   case 'P' :                 /* P: Set priority */
      if (!digits()) abexit(14); /* Value is required */
      CVC.prty = (int)CVC.dig << 8;
      goto next;

   case 'Q' :                 /* Q: Double (Quality) floating */
      kc = kcodouble;
      code |= (RK_DBL|RK_IORF);
      goto numeric;

   case 'R' :                 /* R: Repeat count */
      if (*(CVC.fptr+1) == '?') {
         pickitem();
         CVC.svrep = CVC.dig;    /* Save but do not use count */
         CVC.flags |= GOTRQUES;
         ++CVC.fptr;
         goto next;
         }
      if (CVC.flags & GOTRQUES)  /* Using saved repeat count */
         CVC.dig = CVC.svrep;
      else
         pickitem();             /* Get list item */
      if (CVC.dig > 1)       rep = CVC.dig;   /* Normal processing */
      else if (CVC.dig == 1) ;                /* Use rep, CVC.dig==1 */
      else if (CVC.dig == 0) rep |= SKIPINOD; /* Skip item, not data */
      else                   abexit(17);      /* Negative count error */
      if (*(CVC.fptr+1) == '~') {
         if (rep & SKIPINOD) rep |= SKIPIDAT; /* Skip item and data */
         ++CVC.fptr; }
      goto next;

   case 'S' :                 /* S: Input scan, output subtitle */
      if (mode & INM) {          /* S on input */
         CVC.flags |= SCANNING;  /* Set scanning mode */
         /* Check for word skip code */
         if (toupper(*(CVC.fptr+1)) == 'W') {
            csflags = RK_WDSKIP; ++CVC.fptr; }
         else csflags = 0;
         /* Skip cdscan call if no starting column given.
         *  Kill continuations if scanning a string.  */
         if (digits()) cdscan(buf,(int)CVC.dig,DFLT_MAXLEN,
            csflags | ((mode & RDW) ? 0 : RK_NOCONT));
         }
      else {                     /* S on output */
         CVC.isub ^= RK_SUBTTL;  /* Set subtitle mode */
         }
         goto next;

   case 'T' :                 /* Text Cache or Column Pointer */
      if (mode & INM) {          /* Input:  Read text into cache */
         /* Check for length codes */
         switch (toupper(*(CVC.fptr+1))) {
         case 'C' :                    /* Character */
            ++CVC.fptr;
            kc = kcotchar;
            break;
         case 'H' :                    /* Halfword */
#if (ISIZE == HSIZE) && (ISIZE != I32SIZE)
         case 'I' ;                    /* 16-bit int */
#endif
            ++CVC.fptr;
#if (ISIZE == HSIZE) && (ISIZE != I32SIZE)
         default:                      /* No code implies int type */
#endif
            kc = kcotshort;
            break;
#if ISIZE == I32SIZE
         case 'I' :                    /* 32-bit int */
#endif
         case 'J' :                    /* ui32 */
            ++CVC.fptr;
#if ISIZE == I32SIZE
         default:                      /* No code implies int type */
#endif
            kc = kcotui32;
            break;
         case 'L' :                    /* Long */
            ++CVC.fptr;
            kc = kcotlong;
            break;
            } /* End length code switch */
         goto numeric;
         }
      else {                     /* Output:  Set column pointer */
         if (!digits()) pickitem();          /* Get n or list item */
         CVC.dptr = buf + max(0,CVC.dig-1);  /* Reset data pointer */
         goto next;
         }

   case 'V' :                 /* Verify or Underflow control */
      if (mode & INM) {          /* Input:  Verify numeric value */
         /* Check for additional codes */
         switch (*(CVC.fptr+1)) {
         case '(' :              /* Value must be in parens */
            /* If in a previous parens construct that was not yet
            *  ended with 'V)', this will just be an escalation
            *  of fplev.  But if 'V)' was found but the input
            *  did not yet reach the ')', the excess data must
            *  be skipped with an error message.  */
            if (CVC.vparens == PARENEND && RK.plevel > fplev) {
               while (!(RK.scancode & RK_RTPAREN)) {
                  if (scan(NULL, 0) & RK_ENDCARD) break;
                  ermark(RK_TOOMANY);
                  }
               }
            CVC.vparens = PARENREQ;
            ++fplev;
            break;
         case ')' :              /* Values must be last in parens */
            CVC.vparens = PARENEND;
            --fplev;
            break;
         default:                /* New or original range test */
            CVC.fptr = getvalck(CVC.fptr) - 1;
            /* If did not advance beyond 'V', set flag to convert
            *  alphabetic input to uppercase.  */
            if (toupper(*CVC.fptr) == 'V') CVC.immflgs |= AINUPPER;
            goto next;           /* 'next' will increment fptr */
            } /* End V code switch */
         ++CVC.fptr;
         } /* End input mode */
      else                       /* Output:  Underflow control */
         code |= RK_UFLW;
      goto next;

   case 'W' :                 /* Write */
      if (mode & INM) {
         if (CVC.fptr[1] == ',')
            cdprt1(RK.last), ++CVC.fptr;
         else
            CVC.fptr = getvalck(CVC.fptr) - 1;
         }
      else
         CVC.isub |= RK_FLUSH;
      goto next;

   case 'X' :                 /* Blanks */
      if (CVC.flags & SCANNING) { /* Scanning input */
         while (rep-- > 0) {     /* Do nothing if SKIPIDAT|SKIPINOD */
            if (scan(NULL,RK_REQFLD) & RK_ENDCARD) break; }
         } /* End scanning field skip */
      else if (rep > 0) {        /* Normal formatting */
         if (CVC.dptr + rep > top) abexit(16);
         if (!(mode & INM)) memset(CVC.dptr,' ',(size_t)rep);
         CVC.dptr += rep;
         } /* End normal formatting */
      goto setrep;

   case 'Y' :                 /* SPOUT */
      spout((int)(digits() ? CVC.dig : 1)); /* Default to 1 line */
      goto next;

   case 'Z' :                 /* Z: Hex */
      kc = kcofixpt;
      ++CVC.fptr;
      /* Most apps probably want unsigned here--can call wbcdwt
      *  directly if want signed, or we could invent new codes
      *  'ZU' etc.  */
      code |= RK_NUNS | RK_HEXF | wntype(&CVC.fptr);
#if RK_NZTW != RK_UFLW
      if (code & RK_UFLW) code = code & ~RK_UFLW | RK_NZTW;
#endif
      if (!iadv) iadv = wnclen(code);
      --CVC.fptr;             /* Backup fptr for digits() call */
      goto numeric;

/* All numeric conversions executed here */

numeric:
      /* Determine if width entered and save in conversion code */
      if (digits()) {
         width = (int)CVC.dig;
         code |= (ui32)(width - 1); }
      else if ((kc<=kcotlong) || !(CVC.flags & SCANNING))
         abexit(14);
      if (*(CVC.fptr+1) == '.') {   /* Check for decimal spec */
         ++CVC.fptr;                   /* Absorb the decimal point */
         if (toupper(*(CVC.fptr+1)) == 'A') {
            ++CVC.fptr;                /* Absorb the 'A' */
            code |= RK_AUTO; }         /* Set autodecimal */
         else {
            if (*(CVC.fptr+1) == '<') {   /* Automatic with limit */
               ++CVC.fptr;                /* Absorb the less-than */
               code |= RK_AUTO; }         /* Set autodecimal */
            if (digits())                 /* Get explicit decimal */
               code |= (ui32)CVC.dig<<RK_DS;
            else abexit(20);
            } /* End not 'A' */
         } /* End decimal point code */
      /* Take care of weird case of an explicit 0 repeat in format.
      *  (V1G bug fix:  Maintain CVC.dim through following loop.)  */
reginfn:
      if (rep == 0) rep = SKIPINOD + 1;
      if (CVC.dim == 0) break;      /* FORTRAN action at end of list */
      if (CVC.indexsw==INDEXPEND) { /* Activate pending indexing */
         CVC.dim = CVC.idim;
         CVC.indexsw = INDEXING; }
      /* If no supervenient dimension, use repeat count as dim */
      else if (!(CVC.indexsw|liststep))
         CVC.dim = CVC.idim = (rep & ~SKIPIDAT);
      /* Set item size, allowing for Aw.d case and caret override */
      itemsize = iadv ? iadv : (kc!=kcoafmt ? kcsize[kc]:(int)CVC.dig);
      if (mode & INM && kc >= kcoafmt) /* Set in or out conversion */
         kc += (kciafmt - kcoafmt);
      /* Save information for repeated calls to muscan() */
      mubkfp = CVC.fptr + 1;

/* Here to loop over conversions until repeat count satisfied */

      while (CVC.dim) {       /* Break out at end of I/O list */
         if (!CVC.iptr) abexit(13); /* Error if no item pointer  */
         /* Skip if SKIPINOD mode.  Design note: (rep+SKIPIDAT) is
         *  positive only if SKIPINOD, but could produce overflow. */
         if ((rep & SKIPIDAT) == SKIPINOD) goto noconv;

/* Handle bracket indexing */

         if (ndx && (CVC.indexsw < INDEXBKT)) {
            CVC.iptr = (char *)CVC.iptr + ndx*CVC.idim*itemsize;
            CVC.indexsw |= INDEXBKT; } /* Inactivate until next pick */

/* If scanning, scan a data field and check punctuation codes.  If A
*  or T format input, handle case that user's data might be wider than
*  current length of lea, but do not scan directly into user's field,
*  because there might be a default value sitting there.  */

         if (CVC.flags & SCANNING) {
            if (kc >= kciafmt && kc <= kcitlong) {
               int twidth = width + 1;
               if (CVC.llea < twidth) {
                  CVC.llea = twidth;
                  CVC.lea = reallocv(CVC.lea, CVC.llea, "Scan Buffer");
                  }
               /* A following 'key=' might have len(key) > width,
               *  so must scan with no less than DFLT_MAXLEN, then
               *  check result against user's width only if no '='. */
               if (width > DFLT_MAXLEN) {
                  scanlen(width);         /* Use user's width */
                  scan(CVC.lea, 0);       /* Scan a field */
                  scanlen(DFLT_MAXLEN);
                  }
               else
                  scan(CVC.lea, 0);
               if (RK.length < 0) goto noconv;
               if (!(RK.scancode & RK_EQUALS) && RK.length >= width)
                  ermark(RK_LENGERR);
               }
            else {                        /* Not input A format */
               if (CVC.llea < DFLT_MAXLEN + 1) {
                  CVC.llea = DFLT_MAXLEN + 1;
                  CVC.lea = reallocv(CVC.lea, CVC.llea, "Scan Buffer");
                  }
               /* Scan a field.  Use scan width and set to check for
               *  illegal chars.  If this is an input number, check
               *  whether there is an input adjuster and scan it now
               *  so RK.scancode tests below reflect the full data.
               *  If 'IS' input, scan now for end checking, etc.,
               *  then push field back at wseedin() call below.  */
               code = code & ~RK_MXWD | RK_CTST;
               if (kc >= kcifixpt) {
                  scan(CVC.lea, RK_PMVADJ);
                  if (RK.length < 0) goto noconv;
                  code |= (ui32)RK.length;
                  /* It might be KEY+=X, not an input adjuster */
                  if (cntrl(CVC.lea) == NUMBRET) ckvadj();
                  }
               else {
                  scan(CVC.lea, 0);
                  if (RK.length < 0) goto noconv;
                  code |= (ui32)RK.length;
                  CVC.vparens = PARENOPT;
                  }
               }
            CVC.dptr = CVC.lea;           /* Use field as data */

/* End of card trumps all other considerations */

            if (RK.scancode & RK_ENDCARD) {  /* Hit the end */
               if (CVC.flags & ALLFLDRQ) ermark(RK_REQFERR);
               goto Endret;         /* Return to caller */
               }

/* Deal with parentheses.  This code is designed to allow user to omit
*  parens around a single field that is coded to be in parens, but to
*  generate an error if there are too many fields or if required fields
*  are missing--see flowchart in design notes.  */

            switch (CVC.vparens) {
            case PARENEND:       /* End of parens required now */
               if (RK.plevel < fplev)
                  ermark(RK_PNRQERR);
               /* If still in parens, give error and eat
               *  remaining data in parens */
               else while (RK.plevel > fplev) {
                  ermark(RK_TOOMANY);
                  if (scan(NULL, 0) & RK_ENDCARD) break;
                  } /* Drop through to next case OK ... */
            case PARENOPT:       /* No testing here */
               CVC.vparens = 0;
               break;
            case PARENINS:       /* Inside parens */
               /* If above correct parens level, there are too many
               *  items in parens.  If below correct level, and all
               *  fields are required, then some data are missing.  */
               if (RK.plevel != fplev) {
                  if (RK.plevel > fplev)        ermark(RK_TOOMANY);
                  else {
                     scanagn();
                     if (CVC.flags & ALLFLDRQ)  ermark(RK_REQFERR);
                     }
                  goto noconv;
                  }
               break;
            case PARENREQ:       /* First time seeking parens */
               /* Advance to inside parens state for next time */
               CVC.vparens = PARENINS;
               break;
            case 0:              /* No parens expected here */
               if (RK.plevel > fplev) {
                  if (!(CVC.flags & LPARENOK)) ermark(RK_NESTERR);
                  scanagn();           /* Push field back */
                  goto Endret;         /* Return to caller */
                  }
               } /* End parentheses mode switch */

/* Deal with termination conditions for scanned input */

            /* Hit an '=' sign, not after a right paren */
            if ((RK.scancode & (RK_RTPAREN|RK_EQUALS)) == RK_EQUALS) {
               /* Error if not allowed at this point */
               if (CVC.flags & EQSIGNXX) ermark(RK_PUNCERR);
               else {
                  if (dimcheck && (CVC.dim!=CVC.idim))
                     ermark(RK_REQFERR);
                  scanagn();        /* Push field back */
                  goto Endret;      /* Return to caller */
                  }}

            if (CVC.flags & KEYXPECT &&      /* Exit on nonnumerics */
                  cntrl(CVC.lea) != NUMBRET) {
               /* Ignored parens in this situation prior to V1I */
               scanagn();           /* Push field back */
               goto Endret;         /* Return to caller */
               }

            if (rep < 0) goto noconv;     /* Omit conversion */
            } /* End scanning and scan checking */

         else {                     /* Handle normal formatting */
            if (kc == kciwseed) abexit(15);
            if (CVC.dptr + width > top) abexit(16);
            if (rep < 0) {          /* Skipping with blank fill-in */
               if (!(mode & INM)) memset(CVC.dptr,' ',(size_t)width);
               CVC.dptr += width;
               goto noconv;
               }
            } /* End preparations for unscanned data formatting */

/* Perform actual data conversion */

         mucode = code;
         switch (kc) {
         case kcokchar:
            memcpy(CVC.dptr, mcodprt((ui32)*(byte *)CVC.iptr,
               iokeys, width), width);
            break;
         case kcokshort:
            memcpy(CVC.dptr, mcodprt((ui32)*(ui16 *)CVC.iptr,
               iokeys, width), width);
            break;
         case kcokui32:
            memcpy(CVC.dptr, mcodprt(*(ui32 *)CVC.iptr,
               iokeys, width), width);
            break;
         case kcisreg:
            scanagn();
            kwsregfn(CVC.iptr, CVC.dig);
            break;
         case kcijreg:
            scanagn();
            kwjregfn(kwjarg1, CVC.iptr, CVC.dig);
            break;
         case kcoafmt: {
            int twidth = strnlen(CVC.iptr, width);
            int uwidth = width - twidth;
            memcpy(CVC.dptr, CVC.iptr, twidth);
            if (jfbl < uwidth) uwidth = jfbl;
            if (uwidth > 0) memset(CVC.dptr+twidth,' ',uwidth);
            RK.length = twidth - 1;
            break;
            } /* End scope of twidth,uwidth */
         /* Cases kcotchar ... kcotlong are just placeholders
         *  for   kcitchar ... kcitlong -- no code needed here */
         case kcowseed:
            wseedout((wseed *)CVC.iptr, CVC.dptr, code);
            break;
         case kcofixpt:
            wbcdwt(CVC.iptr, CVC.dptr, code);
            break;
         case kcofloat:
            bcdout(code,CVC.dptr,(double)(*(float *)CVC.iptr));
            break;
         case kcodouble:
            bcdout(code,CVC.dptr,*(double *)CVC.iptr);
            break;
         case kciafmt:
            if (CVC.immflgs & AINUPPER) {
               char *pi = CVC.dptr, *po = CVC.iptr;
               char *pe = po + strnlen(pi, width);
               while (po < pe) *po++ = toupper(*pi++);
               if (po < (char *)CVC.iptr + width) *po = '\0';
               }
            else
               strncpy(CVC.iptr,CVC.dptr,width);
            break;
         case kcitchar:
            *(byte *)CVC.iptr = (byte)savetxt(CVC.dptr);
            break;
         case kcitshort:
            *(short *)CVC.iptr = (short)savetxt(CVC.dptr);
            break;
         case kcitui32:
            *(ui32 *)CVC.iptr = (ui32)savetxt(CVC.dptr);
            break;
         case kcitlong:
            *(long *)CVC.iptr = (long)savetxt(CVC.dptr);
            break;
         case kciwseed:
            scanagn();
            wseedin(CVC.iptr);
            break;
         case kcifixpt:
            muscan(&mubkfp, CVC.dptr, &mucode);
            wbcdin(CVC.dptr, CVC.iptr, mucode);
            wvalck(CVC.iptr, code);
            break;
         case kcifloat:
            muscan(&mubkfp, CVC.dptr, &mucode);
            *(float *)CVC.iptr =
               (float)fvalck((float)bcdin(mucode,CVC.dptr), code);
            break;
         case kcidouble:
            muscan(&mubkfp, CVC.dptr, &mucode);
            *(double *)CVC.iptr =
               dvalck(bcdin(mucode,CVC.dptr), code);
            break;
            } /* End conversion type switch */
         ++RK.numcnv;         /* Count conversions */

/* Here to step data field after each conversion by full or left-
*  justified width.  If scanned input, harmlessly steps through lea. */

         CVC.dptr += (code & RK_LFTJ) ?
            min(width,(jfbl+RK.length+1)) : width;

/* Here when conversion skipped.  Advance item pointer by item size.
*  Count repeat converts and go for new field when count hits 0.  */

noconv:  CVC.iptr = (char *)CVC.iptr + itemsize;
         CVC.fptr = mubkfp - 1;        /* Step format */
         listitem();                   /* Step list */
         if ((--rep & ~SKIPIDAT) == 0) goto newfield;
         } /* End while (CVC.dim) ... */
      break;                  /* Here at end of list (dim==0) */

   default:                   /* Invalid format code */
      abexit(12);
      } /* End format switch */

/* End of conversion.  Return to caller.
*  Note:  It is really impractical to check here for unmatched parens
*  in the format, because there are many ways to exit before reaching
*  the end.  */

Endret:
   if (mode == OUT+RDW) slash(buf,mode);
   return;
   } /* End fmtio */

/**********************************************************************
*                               CONVRT                                *
**********************************************************************/

void convrt(char *format, ...) {

/* First time through, allocate a line buffer based on RK.pgcls */

   if (!CVC.line)
      CVC.line = mallocv(RK.pgcls+1,"convrt buffer");

   va_start(CVC.lptr,format);                /* Pick up item list */
   fmtio(OUT+RDW,CVC.line,RK.pgcls+1,format);/* Do all the work */
   va_end(CVC.lptr);                         /* Clean up the list */
   } /* End convrt */

/**********************************************************************
*                              SCONVRT                                *
**********************************************************************/

void sconvrt(char *line, char *format, ...) {

   va_start(CVC.lptr,format);             /* Pick up item list */
   fmtio(OUT,line,RK.pgcls+1,format);     /* Do all the work */
   va_end(CVC.lptr);                      /* Clean up the list */
   /* Terminate output string unless 'O' format code was entered */
   if (CVC.flags & NULLTERM) *CVC.dptr = '\0';
   } /* End sconvrt */

/**********************************************************************
*                               INFORM                                *
**********************************************************************/

void inform(char *format, ...) {

   va_start(CVC.lptr,format);             /* Pick up item list */
   RKC.accpt |= ACCPT_INF;                /* Signal for eqscan */
   fmtio(INM+RDW,RK.last,CDSIZE,format);  /* Do all the work */
   RKC.accpt &= ~ACCPT_INF;
   va_end(CVC.lptr);                      /* Clean up the list */
   } /* End inform */

/**********************************************************************
*                              SINFORM                                *
**********************************************************************/

void sinform(char *card, char *format, ...) {

   va_start(CVC.lptr,format);             /* Pick up item list */
   RKC.accpt |= ACCPT_INF;                /* Signal for eqscan */
   fmtio(INM,card,CDSIZE,format);         /* Do all the work */
   RKC.accpt &= ~ACCPT_INF;
   va_end(CVC.lptr);                      /* Clean up the list */
   } /* End sinform */
