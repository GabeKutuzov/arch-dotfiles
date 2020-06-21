/* (c) Copyright 1997-2008, The Rockefeller University *11115* */
/* $Id: sibcdin.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               SIBCDIN                                *
*                                                                      *
*  Subroutine sibcdin ("subset ibcdin") provides a subset of the       *
*  facilities available in the standard ROCKS library function ibcdin  *
*  in a completely self-contained unit.  It is intended mainly for use *
*  in environments (parallel computers, MATLAB, etc.) where the full   *
*  ROCKS card/page formatting facilities may require too much memory   *
*  or may be unavailable for other reasons.  It supports scanning or   *
*  fixed-width number conversion, but not floating-point or general    *
*  fixed-point numbers.                                                *
*                                                                      *
*  Usage:  long sibcdin(int ic, char *field)                           *
*                                                                      *
*  Prototyped in:  rkxtra.h, rksubs.h                                  *
*                                                                      *
*  ARGUMENTS:                                                          *
*     ic       is the sum of any of the following processing codes     *
*              RK_HEXF  (0x0200)    to get hexadecimal conversion,     *
*              RK_OCTF  (0x0400)    to get octal conversion,           *
*              RK_IORF  (0x0600)    (or no code) to get decimal conv,  *
*              RK_SSCN  (0x0800)    to perform simple scan, i.e. ter-  *
*                                   minate number at first whitespace, *
*              RK_QPOS  (0x1000)    to give an error if the input      *
*                                   value is negative,                 *
*              RK_CTST  (0x2000)    to give an error if the input      *
*                                   string contains nonnumeric or      *
*                                   nonwhitespace characters,          *
*              RK_ZTST  (0x4000)    to give an error if the input      *
*                                   value is zero.                     *
*           +  width - 1            one less than the maximum width    *
*                                   to be scanned (max RK_MXWD=31).    *
*     field    is the location of the input field to be scanned.       *
*              If the RK_SSCN bit is set, input conversion terminates  *
*              when a blank or tab or newline or null character is     *
*              found.  Otherwise, the full width is scanned.  The      *
*              location of the next character following the termi-     *
*              nating character is retained by sibcdin, and scanning   *
*              resumes there on the next call if 'field' is a NULL     *
*              pointer.                                                *
*                                                                      *
*  GLOBALS:    char *sibcdinf on return points to the next character   *
*              following the field that was converted.  If *sibcdinf   *
*              is zero, the scan reached a newline or the end of the   *
*              input string.                                           *
*                                                                      *
*  ERRORS:     sibcdin terminates with abend code 55 if the new and    *
*              previous field locations are both NULL pointers, with   *
*              56 if a nonnumeric character is found and the RK_CTST   *
*              flag was set, and 57 if the numeric value is invalid.   *
*                                                                      *
************************************************************************
*  V1A, 11/07/97, GNR - New routine                                    *
*  ==>, 09/21/08, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <ctype.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include "sysdef.h"
#include "rkxtra.h"

char *sibcdinf;               /* Saved location of field terminator */

long sibcdin(int ic, char *field) {

   register char *pc = field; /* Local copy of field arg */
   char *pce;                 /* End of field */
   long iv = 0;               /* Accumulator for return value */
   int  gotdigit = FALSE;
   int  gotdreck = FALSE;
   int  gotminus = FALSE;
   int  scanning = ic & RK_SSCN;

   if (!pc && !(pc = sibcdinf)) goto Abend55; /* Assignment intended */
   pce = pc + (ic & RK_MXWD); /* Locate end of field */

   /* State 1:  Skip over any initial whitespace */
   for (;;) {
      if (*pc == '\0'  || *pc == '\n') goto ReturnCode;
      if (*pc != ' ' && *pc != '\t') break;
      if (++pc > pce) goto ReturnCode;
      }

   /* State 2:  A minus sign is acceptable only here */
   if (*pc == '-') {
      gotminus = TRUE;
      if (++pc > pce) goto ReturnCode;
      }

   /* State 3:  Read digits */
   switch (ic & (RK_HEXF|RK_OCTF)) {
   case RK_HEXF:              /* Number is hexadecimal */
      do {
         register int digit = *pc;
         if (isxdigit(digit)) {
            iv <<= 4;
            if (isdigit(digit)) iv += digit - '0';
            else                iv += toupper(digit) - ('A' - 10);
            gotdigit = TRUE; }
         else if (digit == '\0' || digit == '\n')
            goto ValueCheck;
         else if (digit == ' ' || digit == '\t')
            { if (scanning) goto ValueCheck; }
         else
            gotdreck = TRUE;
         } while (++pc <= pce);
      break;
   case RK_OCTF:              /* Number is octal */
      do {
         register int digit = *pc;
         if (digit >= '0' && digit < '8') {
            iv = (iv<<3) + digit - '0';
            gotdigit = TRUE; }
         else if (digit == '\0' || digit == '\n')
            goto ValueCheck;
         else if (digit == ' ' || digit == '\t')
            { if (scanning) goto ValueCheck; }
         else
            gotdreck = TRUE;
         } while (++pc <= pce);
      break;
   default:                   /* Number is decimal */
      do {
         register int digit = *pc;
         if (isdigit(digit)) {
            iv = (iv<<3) + iv + iv + digit - '0';
            gotdigit = TRUE; }
         else if (digit == '\0' || digit == '\n')
            goto ValueCheck;
         else if (digit == ' ' || digit == '\t')
            { if (scanning) goto ValueCheck; }
         else
            gotdreck = TRUE;
         } while (++pc <= pce);
      } /* End switch */

ValueCheck:
   if (!gotdigit) {
      abexitm(57, "Expected a number, no digits found.");
      goto ReturnCode;        /* Gets here only in sibctest */
      }
   if (gotminus) iv = -iv;
   if (ic & RK_CTST && gotdreck)
      abexitm(56, "Nonnumeric character in numeric field.");
   if (ic & RK_QPOS && iv < 0)
      abexitm(57, "Value must be positive.");
   if (ic & RK_ZTST && iv == 0)
      abexitm(57, "Value must be nonzero.");

ReturnCode:
   sibcdinf = (pc && *pc && *pc != '\n') ? pc : (char *)0;
   return iv;

Abend55:
   abexit(55);
   goto ReturnCode;        /* Gets here only in sibctest */

   } /* End sibcdin */
