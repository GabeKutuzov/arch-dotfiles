/* (c) Copyright 1989-2017, The Rockefeller University *11115* */
/* $Id: kwscan.c 66 2018-03-15 19:01:23Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*       EQSCAN, KWSCAN, KWSREG, KWJREG, KWSREGFN, and KWJREGFN         *
*                                                                      *
************************************************************************
*  V1A, 01/09/89, GNR - Convert IBM Assembler version to C             *
*  V1B, 02/10/89, GNR - Exact match always accepted                    *
*  Rev, 03/06/89, GNR - Correct bug in A-format length testing         *
*  Rev, 04/03/89, GNR - Reorganization needed for NCUBE                *
*  Rev, 04/19/89, GNR - Incorporate rkxtra.h                           *
*  Rev, 06/23/89, GNR - Correct bugs in BxxVI and lower-case x         *
*  V1C, 01/25/97, GNR - Add V+, V~, V> codes, U conversion type        *
*  Rev, 10/31/97, GNR - Unique error if key is unmatched numeric       *
*  V1D, 09/06/98, GNR - Add N, O format codes, kwsreg(), big A widths  *
*  Rev, 07/03/00, GNR - In eqscan, don't set value if field is missing *
*  V1E, 07/25/00, GNR - Add R code to indicate state requirement       *
*  Rev, 12/23/00, GNR - Add RK_RTPCK code to check for right parens    *
*  V1F, 08/22/01, GNR - Add T format code, RK_PNCHK                    *
*  V1G, 10/17/01, GNR - Add 4 flavors of K code                        *
*  V1H, 04/27/02, GNR - Return codes for items found in ic             *
*  V1I, 01/29/03, GNR - Add metric unit multipliers, bscompat          *
*  V1J, 02/15/05, GNR - Add VA construction                            *
*  V1K, 12/16/07, GNR - Increase to 10 kwsreg slots                    *
*  V1M, 12/30/07, GNR - Add generic input value checking               *
*  V1N, 05/21/08, GNR - Add kwsregfn(), new '|' scale selector         *
*  ==>, 05/23/08, GNR - Last date before committing to svn repository  *
*  V1O, 01/08/09, GNR - Add RK_PMEQPM flag to scan '+=','-=' switches  *
*  Rev, 02/15/09, GNR - Add svvadj/ckvadj mechanism                    *
*  Rev, 05/02/09, GNR - Add kwjreg/kwjregfn mechanism, allow '(' if    *
*                       eqscan() called from inform 'J' code routine   *
*  V2A, 08/12/09, GNR - New O,S,I length codes for 64-bit, use wbcdin  *
*  Rev, 12/26/09, GNR - Add IS code for wseed random seeds             *
*  Rev, 03/19/10, GNR - Add RK_BPMCK error check code                  *
*  V2B, 01/01/11, GNR - Correct default action, documented 12/11/10    *
*  Rev, 06/15/12, GNR - Expand O code to allow unsetting a flag        *
*  Rev, 08/31/13, GNR - Add 'M', 'B' type codes for integers           *
*  R66, 12/15/17, GNR - Use 2nd scale if '?' and RK_BSQUEST is set.    *
***********************************************************************/

/*---------------------------------------------------------------------*
*     Usage of kwscan:                                                 *
*---------------------------------------------------------------------*/

/*
   Function kwscan is used to search for a set of keys on a control
card and to read parameter values or set flags as directed by a series
of codes.  Before kwscan is called, the card must be read with cryin
and cdscan must be called to initialize scanning (this is often done
indirectly via an inform call with an 'S' format code).

Usage: int kwscan(ui32 *ic, char *key, void *arg, ..., NULL)

Arguments: 'ic' is a pointer to a ui32 integer.  kwscan sets the n'th
      bit (bitset/bittst counting method) of 'ic' when the n'th key
      (n <= BITSPERUI32) is encountered in the data.  The caller can
      use this information to determine which options were found in
      the data.  (The caller must zero 'ic' before the first call,
      thus allowing these bits to accumulate across '%X' exits.)  If
      there are more than BITSPERUI32 keys, the information for the
      excess ones is lost.

   'key' and 'arg' specify keywords, actions, and variables.  There is
      one 'key' argument for each keyword to be matched, consisting of
      a string (mixed case is acceptable) followed by a % separator,
      optionally the letter 'R', optionally the letter 'P' followed by
      an integer, and an action code.  The end of the key list must be
      marked by a NULL pointer.  When an 'R' is present, the next
      argument, assumed to be an int, is tested.  If the result is 0,
      an error is generated, otherwise conversion proceeds normally.
      When a 'P' (for 'prefix length') is present, the following
      integer is the minimum number of characters needed to match that
      key (this is needed when keys recognized by a default processor
      (see below) have a common prefix with one of the enumerated
      keys).  Following the 'R' and/or 'P', each 'key' is followed by
      0 (code X), 2 (codes J,O) or 1 (all others) 'arg' pointers.  The
      types of the args are implied by the codes.  The input card is
      scanned and each field is compared with all of the keys using
      ssmatch.  When a match is found, the action specified by the
      associated action code is performed.  There may also be a
      default action to be invoked if no key is matched.  This is
      coded by '%%' followed by an action as above.  It must follow
      the last explicit keyword.  If no match is found, and there is
      no default action, ermark error RK_IDERR is generated and
      another field is tested.  The possible codes and actions are as
      follows:

   Code  Action
     X      The program returns to the caller for whatever action is
            necessary to process the key.  The function value returned
            by kwscan is incremented by one for each X code encounter-
            ed up to and including the one matched, so a switch based
            on the return code can be used to select the appropriate
            action.  The delimiter after the keyword is not checked--
            the exit routine can use RK.scancode to check it if
            desired.  The exit routine can do anything necessary to
            process the keyword, including reading further fields
            with scan().  There is no corresponding 'arg' argument.
            When the exit routine is finished, it should branch back
            to the original kwscan call to resume scanning the card.
     J[n]   JName.  Executes a routine registered by a previous call
            to kwjreg() to process the data field.  'n' is an integer
            from 0 to 9 that specifies which of 10 possible routines
            should be called. Two arguments are passed to the routine.
            The first, presumably some sort of formatting information,
            is a pointer to the character following 'n' in the action
            code.  The second, presumably a pointer to the result,
            is passed from the next 'arg' to the routine.
     N[n]   Name.  Same as J except executes a routine registered by
            a previous call to kwsreg() instead of kwjreg() and only
            a single argument, presumably a pointer to the result,
            is passed from the next 'arg' to the routine.
     O[C|H|I|J|L|W][~] Or [now flag set/unset].  Two arguments must
            follow this code.  Both are pointers to type char, short,
            int, ui32, long, ui64, or int, according to whether the O
            code is followed by C,H,I,J,L,W, or nothing, bzw.  If the
            keyword is followed by a comma, or by an equal sign and
            one of '1', 'ON', 'TRUE', or 'YES', the second argument,
            typically an option flag, is OR'd into the first.  If the
            keyword is followed by an equals sign and one of '0',
            'OFF', 'FALSE', or 'NO', the bit represented by the second
            argument is cleared in the first argument.  If the width
            code is followed by '~', these actions are reversed.
     S[C|H|I|J|L|W]m Set.  The variable pointed to by 'arg' is set to
            the value 'm', which may be positive or negative.  The
            type of 'arg' defaults to int, but is taken to be char,
            short, int, si32, long, or si64 if 'S' is followed by
            C,H,I,J,L, or W, respectively.  If 'm' is not given, an
            abexit occurs.  The S option is used to set program flags
            or switches.  The keyword must be followed in the input by
            a comma, not an equals sign.  (For compatibility with
            earlier versions, 'm' may be followed by the type spec
            letter rather than preceded.)

     For the remaining codes, a field from the card is to be input
into 'arg'.  The card being scanned must contain the construction
'key = value', where 'value' is a value that is converted and stored
in 'arg'.  The type of conversion is specified by the code as follows:

     [V]An  Alphanumeric.  The value is a string, which is copied, up
            to a maximum of 'n' characters, to 'arg'.  If the string
            is longer than 'n' characters, ermark error RK_LENGERR is
            generated.  Option code 'VA' causes the string to be
            converted to upper case.
     [V]I[C|H|M|I|J|L|B|S|W]  Integer.  The value is an integer.  The
            type of 'arg' defaults to int, but is taken to be char,
            short, smed, int, si32, long, sbig, wseed, or si64 if the
            I code is followed by C,H,M,I,J,L,B,S, or W, respectively.
            Option code 'V' may be written 'V>[nnn]' or 'V>=[nnn]'
            (argument must be >nnn or >=nnn, bzw., nnn defaults to 0),
            'V<[nnn]' or 'V<=[nnn]' (argument must be <nnn or <=nnn,
            bzw., nnn defaults to 0), 'V' (argument must be >= 0), or
            'V~' (argument must be ~0).  'V' may be replaced by 'W' to
            generate a warning instead of an error when the range
            checks fail.  'V' is unnecessary with 'IS'.
     [V]BsI[C|H|M|I|J|L|B|W]  Fixed point number with binary scale 's'.
            The value is converted as described above for code I except
            that it may have an integral and a fractional part.  The
            value is stored with 's' fraction bits.  's' may consist
            of two integers separated by a '/', '|', or '?'.  With '/',
            the 2nd scale is used if the RK.bssel RK_BSSLASH bit was
            set by a previous call to bscompat(), with '|', the 2nd
            scale is used if RK_BSVERTB was set, and with '?', the
            2nd scale is used if RK_BSQUEST was set, otherwise the
            first scale is used.
     [V]U[C|H|M|I|J|L|B|W] or [V]BsU[C|H|M|I|J|L|B|W]  Unsigned
            integer.  Same as I codes except 'arg' is treated as an
            unsigned integer.
     [V]F[.d]  Floating-point.  A real value is converted and stored in
            an 'arg' of type float.  If the input string has no decimal
            point, 'd' digits to the right of the point are assumed.
     [V]Q[.d]  Double-precision.  A real value is converted and stored
            in an 'arg' of type double.  If the input string has no
            decimal point, 'd' digits to the right of the point are
            assumed.
     [V]D[.d]  Double-precision.  Same as Q code.
     T[C|H|I|J|L]n  Text.  The value is a string of up to 'n' charac-
            ters.  It is read in and saved in the text cache by a call
            to savetxt(). The text locator is converted to an unsigned
            integer of type byte, ui16, int, ui32, long, or int if the
            T code is followed by C,H,I,J,L, or nothing, bzw., and
            saved in 'arg'.  (I is optional.)  Note:  Text locators
            are consecutive integers starting at '1'.  Format 'TC'
            should be used only if the caller is certain that there
            can not be more than 255 different text strings in the
            application.
     K[C|H|I|J|L]codes  Codes.  'codes' represents a string of from one
            to 32 characters.  The argument is taken to be an unsigned
            integer of type char, short, int, ui32, ui32, or int accor-
            ding to whether the K code is followed by C, H, I, J, L, or
            nothing, bzw.  (I may be used to specify int type when the
            first code letter is one of the other type specifiers.  L
            is a deprecated synonym for J for compatibility with old
            programs.  There is no current provision to handle 64-bit
            option fields, but if such a feature is added, L and W will
            be used to specify long or ui64 arguments, bzw.)  The value
            must be a string comprised of a subset of the characters in
            'codes'.  Each character in the data string is matched to
            the codes and used to update the argument according to the
            rules of the mcodes() function.

   Codes I,U,F,Q,D may be followed, after any C,H,M,I,J,L,B,W, or .d
specification, by '$' and a metric units specification as defined in
muscan.  Also, variables read with these codes (numeric values) may be
followed in the input by a '+' or '-' sign and the name of an
adjustment value.  If a value with that name was stored by a previous
call to svvadj(), the adjustment is added to the input value before it
is returned.

   When kwscan reaches the end of the input card, it returns zero.
There is no guarantee that all or any of the keys were actually found
on the card--all variables ('argn') should be set to default values
before kwscan is called.

Restriction:  The internal scanning buffer in kwscan can hold no more
     than 32 characters.  Longer alphanumeric and text fields are OK.
*/

/*---------------------------------------------------------------------*
*     Usage of kwjreg and kwsreg:                                      *
*---------------------------------------------------------------------*/

/*
   Function kwjreg, bzw. kwsreg saves a pointer to an application-
   specific conversion routine that is called when the action code
   is Jn, bzw. Nn.

   Usage:  void kwjreg(void (*kwjfn)(char *, void *), int n)
           void kwsreg(void (*kwnfn)(void *), int n)

   Arguments:  'kwjfn', bzw. 'kwnfn' are functions that kwscan should
      call when a key with code Jn, bzw. Nn is matched.  'n' must be
      an integer from 0 to MAX_KWSREG_N (9).

   The kwjfn and kwnfn functions form entirely separate sets.
*/

/*---------------------------------------------------------------------*
*     Usage of kwjregfn and kwsregfn:                                  *
*---------------------------------------------------------------------*/

/*
   Function kwjregfn, bzw. kwsregfn calls the n'th function previously
   registerd with kwjreg, bzw. kwsreg.  This is mainly for use by
   inform().

   Usage:  void kwjregfn(char *pfmt, void *pvar, int n)
           void kwsregfn(void *pvar, int n)

   Arguments:  'pfmt' (kwjregfn only) and 'pvar' are pointers to the
      arguments expected by the registered routines.  'pfmt' is norm-
      ally some sort of format code and 'pvar' is normally a pointer
      to the value to be stored by the registered function number 'n'.
      'n' must be an integer from 0 to MAX_KWSREG_N (9).
*/

/*---------------------------------------------------------------------*
*     Usage of eqscan:                                                 *
*---------------------------------------------------------------------*/

/*
   Function eqscan performs the following sequence of actions that are
often used to read a value from a control card into a variable:
   (a) Check that the previous delimiter was an equals sign or at
       least not a right paren (optional),
   (b) Call scan() to get the next input field,
   (c) Convert the field according to a specified format.

Usage: int eqscan(void *item, char *code, int ierr)

Arguments:
      'item' is a pointer to an item to be converted.  Its type is
         determined by 'code'.
      'code' determines the type of the conversion.  The codes for
         eqscan and kwscan are the same, and are described above.
         The J, N, O, S, and X codes cannot be used with eqscan.
      'ierr' controls error checking.  If 'ierr' is zero, the
         delimiter following the previous field is not checked.  If
         RK_EQCHK is set and the previous field did not end with an
         equals sign, an RK_EQRQERR is generated.  If RK_BEQCK is set
         and the previous field did not end with either a blank or an
         equals sign, an RK_PUNCERR error is generated.  If both of
         these bits are set, use RK_BEQCK if called from (s)inform(),
         otherwise use RK_EQCHK.  If RK_PNCHK is set and the data are
         not in parentheses, RK_PNRQERR error is generated.  If
         RK_BPMCK is set, test is same as RK_EQCHK except added
         RK_PMINUS code is acceptable.  In all cases, RK.iexit is
         made nonzero, and eqscan returns with an error code.

Return value: eqscan returns 0 if there was no error, otherwise an
      error code equal to the ermark code of the error.

Restriction:  The internal scanning buffer in eqscan can hold no
      more than 32 characters.  Longer alphanumeric fields are OK.
*/

/*--------------------------------------------------------------------*
*        Global definitions:                                          *
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
#include "bapkg.h"

/* Maximum length of scan field--enough for maximal mcodes  */
#define LFLD ((DFLT_MAXLEN > MXKEYLEN) ? DFLT_MAXLEN : MXKEYLEN)

static struct {               /* Static shared storage */
   /* Ptrs to registered J,N functions */
   void (*kwjfn[MAX_KWSREG_N+1])(char *, void *);
   void (*kwnfn[MAX_KWSREG_N+1])(void *);
   char *pc;                     /* Code pointer */
   char *lea;                    /* Scanning buffer (dynamic) */
   int  llea;                    /* Current size of lea */
   int  oldsclen;                /* Old scan length */
   } KW;

/*=====================================================================*
*               getld - get length or decimal parameter                *
*                                                                      *
*  This routine is used to pick up decimal numbers in format codes,    *
*  e.g. length, scale, or decimal parameters.  It uses the static      *
*  format pointer KW.pc in order that the pointer may be updated.      *
*  Initially, pc points to the character just before the expected      *
*  decimal number; on return, pc points to the next character to be    *
*  scanned by the caller.  Gives abexit(24) if no digits are found.    *
*=====================================================================*/

static int getld(void) {

   register int value = 0;
   if (!isdigit(KW.pc[1])) abexit(24);
   while (isdigit(*++KW.pc))
      value = (((value<<2) + value)<<1) + (*KW.pc - '0');
   return value;
   } /* End getld() */

/*=====================================================================*
*                       getfld - get data field                        *
*                                                                      *
*  This routine increments the dynamic buffer size if necessary,       *
*  scans the data field, and generates an error and returns zero       *
*  if no data are found.  It relies on reallocv() to do a mallocv()    *
*  if the current buffer length is 0.                                  *
*                                                                      *
*  Arguments:                                                          *
*     w        Max width allowed for this field                        *
*     iscf     Scan flags for this call                                *
*=====================================================================*/

static int getfld(int w, int iscf) {

   if (w > KW.llea) KW.lea = reallocv(KW.lea, (KW.llea = w) + 1,
      "Scan buffer");
   KW.oldsclen = scanlen(w);     /* Allow this scan length */
   scan(KW.lea, iscf);           /* Pick up the data field */
   return (RK.scancode & RK_ENDCARD || RK.length < 0) ? 0 : 1;
   } /* End getfld() */

/*=====================================================================*
*            getnc - advance to next comma or end of card              *
*                                                                      *
*  This routine is used after a keyword error to skip to the next      *
*  keyword, allowing for possible absence of a value field.            *
*=====================================================================*/

static void getnc(void) {

   while (!(RK.scancode & (RK_COMMA|RK_ENDCARD)))
      scan(NULL, 0);    /* Skip over data */
   } /* End getnc() */

/***********************************************************************
*                                                                      *
*                          Subroutine EQSCAN                           *
*                                                                      *
***********************************************************************/

int eqscan(void *item, char *code, int ierr) {

   ui32 cc, vcc;                    /* Conversion code */
   int  lcode;                      /* Argument length code */
   int  rc = 0;                     /* Return code */
   int  bs = 0;                     /* Binary scale */

   /* Assignment intended on next line */
   lcode = ierr & ~RK_PNCHK;
   if (lcode == (RK_EQCHK|RK_BEQCK))
      lcode = (RKC.accpt & ACCPT_INF) ? RK_BEQCK : RK_EQCHK;
   switch (lcode) {
   case RK_EQCHK:
      if ((RK.scancode & ~(RK_INPARENS|RK_RTPAREN)) != RK_EQUALS) {
         ermark(RK_EQRQERR); return RK_EQRQERR; }
      break;
   case RK_BEQCK:
      if (RK.scancode & ~(RK_INPARENS|RK_RTPAREN|RK_EQUALS)) {
         ermark(RK_PUNCERR); return RK_PUNCERR; }
      break;
   case RK_BPMCK:
      if ((RK.scancode & ~(RK_INPARENS|RK_RTPAREN|RK_PMINUS)) !=
         RK_EQUALS) { ermark(RK_EQRQERR); return RK_EQRQERR; }
      break;
      } /* End lcode switch */
   if (ierr & RK_PNCHK &&
         (RK.scancode ^ RK_INPARENS) & (RK_INPARENS|RK_RTPAREN)) {
      ermark(RK_PNRQERR); return RK_PNRQERR; }

   KW.pc = code;                    /* Make global copy of code */
   KW.pc = getvalck(KW.pc);         /* Check for 'V' code */
   switch (toupper(*KW.pc)) {       /* Convert according to code */
   case 'A' :                       /* Alphanumeric */
      KW.oldsclen = scanlen(getld());  /* Set max length */
      scan((char *)item, (RK_REQFLD|RK_FENCE));
      if (toupper(*code) == 'V') {  /* Force input to upper case */
         char *pi = (char *)item, *pe = pi + RK.length;
         while (pi <= pe) { *pi = toupper(*pi); ++pi; }
         }
      break;

   case 'K' :                       /* Keys */
      lcode = toupper(KW.pc[1]);    /* Interpret type code */
      if (strchr("LJIHC", lcode))
         ++KW.pc;
      else
         lcode = 'I';
      if (!getfld(MXKEYLEN, RK_REQFLD|RK_PMEQPM))
         goto ItemMissing;
      rc = mcodes(KW.lea, ++KW.pc, NULL);
      switch (lcode) {              /* Update according to code */
      case 'C' :                       /* Character */
         mcodopc((unsigned char *)item);
         break;
      case 'H' :                       /* Half-word */
         mcodoph((unsigned short *)item);
         break;
      case 'I' :                       /* Integer */
         mcodopi((unsigned int *)item);
         break;
      case 'J' :                       /* ui32 */
      case 'L' :                       /* Old 'long' = ui32 */
         mcodopl((ui32 *)item);
         break;
         } /* End lcode switch */
      break;

   case 'T' :                       /* Text cache fields */
      lcode = isdigit(KW.pc[1]) ? 'I' : toupper(*++KW.pc);
      if (!getfld(getld(), RK_REQFLD)) /* Pick up the data field */
         goto ItemMissing;
      bs = savetxt(KW.lea);
      switch (lcode) {              /* Store according to code */
      case 'C' :                       /* Byte */
         if (bs > BYTE_MAX) abexit(42);
         *(byte *)item = (byte)bs;
         break;
      case 'H' :                       /* Half-word */
         if (bs > UI16_MAX) abexit(42);
         *(ui16 *)item = (ui16)bs;
         break;
      case 'I' :                       /* Integer */
         *(unsigned int *)item = (unsigned int)bs;
         break;
      case 'J' :                       /* ui32 */
         *(ui32 *)item = (ui32)bs;
         break;
      case 'L' :                       /* Old long = ui32 */
         *(unsigned long *)item = (unsigned long)bs;
         break;
      default:
         abexit(12);
         } /* End lcode switch */
      break;

   case 'I' :                       /* Integer */
      if (toupper(KW.pc[1] == 'S')) {
         /* Special case: 'IS' is one or two random seeds */
         wseedin(item);
         return rc;
         } /* Fall through to default ... */
   default:                         /* Ordinary numeric field */
      /* Pick up the data field, allowing for an adjuster */
      if (!getfld(LFLD, RK_REQFLD|RK_PMVADJ))
         goto ItemMissing;
      /* Call for I or F fmt w/character test */
      cc = (RK_IORF|RK_CTST) | (ui32)RK.length;
      /* Pick up binary scale if any */
      if (toupper(*KW.pc) == 'B') {
         bs = getld();
         if (*KW.pc == '/') {
            int bs2 = getld();
            if (RK.bssel & RK_BSSLASH) bs = bs2;
            }
         else if (*KW.pc == '|') {
            int bs2 = getld();
            if (RK.bssel & RK_BSVERTB) bs = bs2;
            }
         else if (*KW.pc == '?') {
            int bs2 = getld();
            if (RK.bssel & RK_BSQUEST) bs = bs2;
            }
         cc |= ((ui32)bs << RK_BS); /* Set scale */
         } /* End processing B code */
      /* Check for value adjuster */
      ckvadj();

      switch (toupper(*KW.pc)) {    /* Convert according to code */
      case 'U' :                    /* Unsigned integer */
         cc |= RK_NUNS;
         /* Drop through to case 'I' ... */
      case 'I' :                    /* Signed integer */
         ++KW.pc;
         cc |= wntype(&KW.pc);    /* Which size? */
         muscan(&KW.pc, KW.lea, &cc);
         wbcdin(KW.lea, item, cc);
         wvalck(item, cc);
         break;

      case 'F' :                    /* Float */
         if (*++KW.pc == '.') cc |= (ui32)getld() << RK_DS;
         vcc = cc |= RK_SNGL;
         muscan(&KW.pc, KW.lea, &cc);
         *(float *)item =
            (float)fvalck((float)bcdin(cc, KW.lea), vcc);
         break;

      case 'D' :                    /* Double precision */
      case 'Q' :
         if (*++KW.pc == '.') cc |= (ui32)getld() << RK_DS;
         vcc = cc |= RK_DBL;
         muscan(&KW.pc, KW.lea, &cc);
         *(double *)item = dvalck(bcdin(cc, KW.lea), vcc);
         break;

      default:                      /* Unrecognized code */
         abexit(12);

         } /* End code switch */

      } /* End numeric conversions */

ItemMissing:
   scanlen(KW.oldsclen);            /* Restore previous scan length */
   lcode = RKC.accpt & ACCPT_INF ?
      ~(RK_COMMA|RK_RTPAREN|RK_INPARENS|RK_LFTPAREN) :
      ~(RK_COMMA|RK_RTPAREN|RK_INPARENS);
   if (RK.scancode & lcode) {
      ermark(RK_PUNCERR); return RK_PUNCERR; }
   return rc;

   } /* End eqscan() */

/***********************************************************************
*                                                                      *
*                     Subroutines KWJREG, KWSREG                       *
*                                                                      *
***********************************************************************/

void kwjreg(void (*kwjfn)(char *, void *), int n) {

   if (n < 0 || n > MAX_KWSREG_N) abexit(44);
   KW.kwjfn[n] = kwjfn;

   } /* End kwjreg() */

void kwsreg(void (*kwnfn)(void *), int n) {

   if (n < 0 || n > MAX_KWSREG_N) abexit(44);
   KW.kwnfn[n] = kwnfn;

   } /* End kwsreg() */

/***********************************************************************
*                                                                      *
*                   Subroutines KWJREGFN, KWSREGFN                     *
*                                                                      *
***********************************************************************/

void kwjregfn(char *pfmt, void *pvar, int n) {

   if (n < 0 || n > MAX_KWSREG_N || !KW.kwjfn[n]) abexit(43);
   KW.kwjfn[n](pfmt, pvar);

   } /* End kwjregfn() */

void kwsregfn(void *pvar, int n) {

   if (n < 0 || n > MAX_KWSREG_N || !KW.kwnfn[n]) abexit(43);
   KW.kwnfn[n](pvar);

   } /* End kwsregfn() */

/**********************************************************************
*                                                                     *
*        Subroutine KWSCAN                                            *
*                                                                     *
**********************************************************************/

int kwscan(ui32 *ic, ...) {

   va_list ap;                /* Argument pointer */
   char *pkey;                /* Ptr to key */
   char *pfmt;                /* Ptr to format code */
   void *parg,*parg2;         /* Ptrs to numeric arguments */
   char *pbmf;                /* Ptr to best-match fmt */
   void *pbma,*pbma2;         /* Ptr to best-match args */
   long ikey,bkey;            /* Key counters (long for bitset) */
   int  lssm;                 /* Min prefix length */
   int  reqf,brqf;            /* Request flag, best request flag */
   short rc;                  /* Count X exits */
   short best_match;          /* Longest ssmatch */
   short exact_match;         /* TRUE if exact match */
   short this_match;          /* Current ssmatch */
   short match_exit;          /* Exit count at best match */
   char tie_match;            /* TRUE if tie match */

#define NTVALS  4
#define NTFVALS 8
   static char *tfvals[NTFVALS] =
      { "1", "YES", "ON", "TRUE", "0", "NO", "OFF", "FALSE" };

/* Make sure there is an input buffer */

   if (LFLD > KW.llea) KW.lea = reallocv(KW.lea,
      (KW.llea = LFLD) + 1, "Scan buffer");
   KW.oldsclen = scanlen(LFLD);     /* Allow this scan length */

/* Scan each control card field in turn */

   for (;;) {                       /* Do until end of card */
      scan(KW.lea, RK_NEWKEY);      /* Pick up a field */
      if (RK.scancode & RK_ENDCARD) break;   /* End exit */
      ikey = 0;                     /* Clear key counter */
      rc = 0;                       /* Clear return code */
      best_match = 0;               /* Clear longest match */
      va_start(ap, ic);             /* Initialize arg ptr */

/* Look for best match to this keyword */

      while ((pkey = va_arg(ap, char *)) != NULL) { /* Match keys */
         if ((pfmt = strrchr(pkey, '%')) == 0) abexit(12);
         pfmt++;                    /* toupper may be a macro */
         if (toupper(*pfmt) == 'R') {
            reqf = va_arg(ap, int); /* Save request flag */
            pfmt++;
            }
         else
            reqf = TRUE;            /* Indicate key is OK */
         lssm = 1;                  /* Deal with minimum match */
         if (toupper(*pfmt) == 'P') {
            KW.pc = pfmt; lssm = getld(); pfmt = KW.pc; }
         switch (toupper(*pfmt)) {
         case 'X' :                 /* User exit */
            rc++;
            break;
         case 'O' :                 /* OR, two arguments */
            parg2 = va_arg(ap, void *);   /* Fall through... */
         default:                   /* All others */
            parg =  va_arg(ap, void *);
            } /* End action code switch */
         ikey++;                    /* Count keys */
         this_match = ssmatch(KW.lea, pkey, -lssm);
         /* An exact match immediately terminates the search.
         *  If current key is default '%', ssmatch returns 0,
         *  this_match is 0, and exact_match is TRUE.  */
         exact_match = *(pkey+this_match) == '%';
         if ((this_match > 0 &&
               (exact_match || this_match > best_match)) ||
               ((this_match|best_match) == 0 && exact_match)) {
            best_match = max(this_match, exact_match);
            match_exit = rc;
            tie_match = FALSE;
            pbmf = pfmt;
            pbma = parg;
            pbma2= parg2;
            brqf = reqf;
            bkey = ikey;
            if (exact_match) break; }
         else if (this_match == best_match) tie_match = TRUE;
         } /* End key match loop */
      va_end(ap);
      if (!best_match || tie_match) {  /* Flag bad identifier */
         /* If both IDERR and tie match, IDERR has precedence.
         *  A matched numeric string is no error at all.  */
         if (!cntrl(KW.lea))   ermark(RK_NUMBERR);
         else if (!best_match) ermark(RK_IDERR);
         else                  ermark(RK_ABBRERR);
         getnc();
         }
      else if (!brqf) {
         ermark(RK_MARKFLD);
         cryout(RK_P1, " ***Option >>", RK_LN1, KW.lea, RK_CCL,
            "<< is incompatible with previously entered options.",
            RK_CCL, NULL);
         getnc();
         }
      else {                        /* Got one */
         if (bkey <= BITSPERUI32)
            bitset((byte *)ic, bkey);
         switch (toupper(*pbmf)) {  /* Switch according to format */

         case 'J' : {               /* Call named J function */
            int n = isdigit(pbmf[1]) ? pbmf[1] - '0' : 0;
            kwjregfn(pbmf+2, pbma, n);
            break; }                /* End case 'J' */

         case 'N' : {               /* Call named N function */
            int n = isdigit(pbmf[1]) ? pbmf[1] - '0' : 0;
            kwsregfn(pbma, n);
            break; }                /* End case 'N' */

         case 'O' : ++pbmf; {       /* Set or clear option flag */
            char *pbmc = (char *)pbma, *pbmc2 = (char *)pbma2;
            int im, ir, ll = wntlen(&pbmf);
            ir = *pbmf == '~';
            if (RK.scancode & RK_EQUALS) {
               char oval[6];
               eqscan(oval, "VA5", 0);
               im = smatch(0, oval, tfvals, NTFVALS);
               if (im <= 0) break;
               if (im > NTVALS) ir ^= 1;
               }
            if (ir) while (ll--) *pbmc2++ &= ~(*pbmc++);
            else    while (ll--) *pbmc2++ |= *pbmc++;
            break; }                /* End case 'O' */

         case 'S' : ++pbmf; {       /* Set constant value */
            /* Flexibility is the word here, not efficiency--
            *  this code will be used only rarely.  */
            char *pfend = pbmf + strlen(pbmf) - 1;
            ui32 cc = wntype(&pfend);
            if (cc == RK_NDFLT) cc = wntype(&pbmf);
            cc |= RK_IORF | strlen(pbmf) - 1;
            wbcdin(pbmf, pbma, cc);
            break; }                /* End case 'S' */

         case 'X' :                 /* Exit to caller */
            return (int)match_exit;

         default:                   /* All other formats */
            eqscan(pbma, pbmf, RK_BPMCK);

            } /* End format code switch */
         } /* End processing of one match */
      } /* End scan loop */

   scanlen(KW.oldsclen);            /* Restore previous scan length */
   return 0;

   } /* End kwscan() */
