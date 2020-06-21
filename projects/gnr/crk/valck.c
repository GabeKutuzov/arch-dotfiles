/* (c) Copyright 2007-2009, The Rockefeller University *11115* */
/* $Id: valck.c 65 2017-10-13 18:53:31Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                getvalck, clrvalck and svvadj, ckvadj                 *
*                                                                      *
*  These routines are used by convrt, inform, eqscan, and kwscan.      *
*  getvalck() interprets value checking codes in an input format.      *
*  clrvalck() clears any tests from a previous input sequence.         *
*  svvadj() stores a given adjustment value in a global array and      *
*  indexes it with an arbitrary name.  ckvadj() checks whether user    *
*  input includes an adjustment construct (+/-name after the numeric   *
*  value) and, if so, stores the adjustment value for application by   *
*  one of the bcdin routines.  These perform the adjustment and over-  *
*  testing, then the valck routines perform any specified more         *
*  restrictive range tests on values of the type indicated by the      *
*  name of the routine.  (This is one of those cases where the C++     *
*  template facility would save some code duplication.)                *
*                                                                      *
*  Value checking on individual inputs is done by separately-compiled  *
*  routines dvalck, fvalck, ivalck, uvalck, wvalck because different   *
*  check sets may be needed for different applications.                *
*                                                                      *
*  The following encodings are understood by getvalck:                 *
*  The first letter is 'V' to give an error or 'W' to give just a      *
*  warning when the test fails.  In both cases, the numerical          *
*  argument is set to the limiting value to safeguard later code.      *
*  V~       The value must not be zero.                                *
*  V>nnn    The value must be greater than the given value 'nnn'.      *
*  V>=nnn   The value must be greater than or equal to 'nnn'.          *
*  V<nnn    The value must be less than 'nnn'.                         *
*  V<=nnn   The value must be less than or equal to 'nnn'.             *
*  In all four cases, if nnn is omitted, the test is against 0.        *
*  Two tests may be used with each occurrence of 'V' or 'W'--          *
*  a minimum test and a maximum test.                                  *
*                                                                      *
*  N.B.  When getvalck is called, any previous test is cancelled,      *
*  whether or not a new test is found.  This would seem to be          *
*  desirable given the way inform, eqscan, and kwscan work.  But       *
*  one test can be used to check multiple input values, for example,   *
*  members of an array.                                                *
*                                                                      *
*  Because the numeric format (fixed with binary scale or float)       *
*  is not known at the time the test is interpreted, 'nnn' is stored   *
*  as a string and interpreted using the code passed to the checker    *
*  routine.  This permits all the information about the tests to be    *
*  encoded in the format string, avoiding passing test values as       *
*  numeric arguments, which would be very difficult in certain cases.  *
************************************************************************
*  V1A, 12/30/07, GNR - New routine                                    *
*  ==>, 06/20/08, GNR - Last date before committing to svn repository  *
*  V2A, 02/16/09, GNR - Add svvadj(), ckvadj()                         *
*  V2B, 07/25/09, GNR - Break out [dfiuw]valck, add "NOT VALID" to msg *
***********************************************************************/

#include <ctype.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sysdef.h"
#include "rocks.h"
#include "rockv.h"
#include "rkxtra.h"


/*=====================================================================*
*                              valckmsg                                *
*                                                                      *
*  Routine to print messages for all the value check routines.         *
*  This code does not know the name of the variable that is bad,       *
*  but an ermark '$" and the input and limiting values can be          *
*  printed to help the user diagnose the problem.                      *
*  (Fortunately, the good value is available as a text string...)      *
*=====================================================================*/

void valckmsg(char *ttype, char *goodval) {

   char *fixt = goodval[0] ? goodval : "0";

   if (RKC.ktest & VCK_WARN) {
      ermark(RK_WARNING | RK_MARKFLD);
      cryout(RK_P1, "0-->WARNING: Numeric value ", RK_LN2,
         RKC.badval, BadDatSize, " is ", 4,
         ttype, RK_CCL, fixt, CkDatSize,
         ", reset to limiting value.", RK_CCL, NULL);
      }
   else {
      ermark(RK_MARKFLD);
      cryout(RK_E1, "0***NUMERIC VALUE ", RK_LN2,
         RKC.badval, BadDatSize, " IS ", 4,
         ttype, RK_CCL, fixt, CkDatSize,
         ", NOT VALID FOR THIS VARIABLE.", RK_CCL, NULL);
      }
   } /* End valckmsg() */


/*=====================================================================*
*                              clrvalck                                *
*                                                                      *
*  Synopsis:  void clrvalck(void)                                      *
*                                                                      *
*  This kills both value checks and adjusters (for bcdin safety)       *
*=====================================================================*/

void clrvalck(void) {

   RKC.ktest = 0;

   } /* End clrvalck() */


/*=====================================================================*
*                              getvalck                                *
*                                                                      *
*  Synopsis:  char *getvalck(char *fptr)                               *
*                                                                      *
*  Argument:                                                           *
*     fptr        Pointer to format control string at point where      *
*                 a 'V' or 'W' code is expected.  If character is      *
*                 anything else, or 'V' or 'W' is not followed by      *
*                 '<', '>', or '~', program returns, not an error.     *
*                                                                      *
*  Return value:  fptr updated to point to the next character after    *
*                 the test definition string.  The codes for the       *
*                 test are left in the RKC structure for use by        *
*                 later calls to one of the range check routines.      *
*                                                                      *
*  Errors:        (These are rare coding errors and therefore not      *
*                 accompanied by abexitm messages.)                    *
*     111         Test number was longer than CkDatSize characters.    *
*     112         Both GT and GE or LT and LE were specified.          *
*=====================================================================*/

char *getvalck(char *fptr) {

   char *tvar,*tvlim;
   RKC.ktest = 0;

/* First check for 'V' or 'W' and return if neither found */

   switch (toupper(*fptr)) {

   case 'W':
      RKC.ktest |= VCK_WARN;
      /* Fall through to 'V' case... */

   case 'V':

/* Loop over possibly more than one test */

      while (1) {

         /* Check which type of test it is */
         switch (*++fptr) {
         case '<':
            tvar = RKC.cmxtest;
            if (fptr[1] == '=')
               RKC.ktest |= VCK_LTEQ, ++fptr;
            else
               RKC.ktest |= VCK_LTHAN;
            break;      /* Read numeric string */
         case '>':
            tvar = RKC.cmntest;
            if (fptr[1] == '=')
               RKC.ktest |= VCK_GTEQ, ++fptr;
            else
               RKC.ktest |= VCK_GTHAN;
            break;      /* Read numeric string */
         case '~':
               RKC.ktest |= VCK_NZERO;
            continue;   /* Look for another test */
         default:
            if (!(RKC.ktest & ~VCK_ADJ)) {
               RKC.ktest |= VCK_GTEQ;
               RKC.cmntest[0] = '\0';
               return fptr;
               }
            goto ConsistencyCheck;
            } /* End test type switch */

         /* Read numeric string */
         tvlim = tvar + CkDatSize;
         while (isdigit(fptr[1]) || fptr[1] == '-' || fptr[1] == '.') {
            if (tvar >= tvlim) abexit(111);
            *tvar++ = *++fptr;
            }
         *tvar = '\0';
         } /* End while */

ConsistencyCheck:
      if ((RKC.ktest & (VCK_GTHAN|VCK_GTEQ)) == (VCK_GTHAN|VCK_GTEQ) ||
          (RKC.ktest & (VCK_LTHAN|VCK_LTEQ)) == (VCK_LTHAN|VCK_LTEQ))
         abexit(112);

      } /* End V,W switch */

   return fptr;

   } /* End getvalck() */


/*=====================================================================*
*                               svvadj                                 *
*                                                                      *
*  Synopsis:  void svvadj(double value, char *aname)                   *
*                                                                      *
*  This routine stores the given value and indexes it with the named   *
*  aname (max L_SYMBNM = 15 char) for later access by ckvadj.  There   *
*  is no way to delete an adjustor once stored, but its value can      *
*  be set to 0.0 (1.0 if we ever add multiplicative adjusters).        *
*=====================================================================*/

void svvadj(double value, char *aname) {

   struct RKVAV *pvav,**ppvav;

   /* First see if one is already stored with the same name */
   for (ppvav=&RKC.pfvav; (pvav = *ppvav); ppvav=&pvav->pnvav) {
      if (!strncmp(aname, pvav->vname, L_SYMBNM)) goto GotOne;
      }
   /* No previous entry, make a new one */
   pvav = *ppvav = mallocv(sizeof(struct RKVAV), "Adjustment value");
   pvav->pnvav = NULL;
   strncpy(pvav->vname, aname, L_SYMBNM);
GotOne:
   pvav->adjval = value;
   return;
   } /* End svvadj() */


/*=====================================================================*
*                               ckvadj                                 *
*                                                                      *
*  Synopsis:  void ckvadj(void)                                        *
*                                                                      *
*  Routine checks whether last scanned field was terminated with a     *
*  plus or minus sign.  If so, scans the next field and compares it    *
*  with the table of suffix names previously stored by svvadj.  If     *
*  a match is found, the named value is stored in RKC.dvadj for one    *
*  of the bcdin routines to apply (done this way because only bcdin    *
*  can apply the adjustment before binary scaling and overflow test).  *
*  If a match is not found, an ermark RK_VANSERR error is generated.   *
*  If there was no '+' or '-' delimiter, RKC.ktest bit VCK_ADJ is 0.   *
*=====================================================================*/

void ckvadj(void) {

   struct RKVAV *pvav;        /* List search ptr */
   int oldsclen;              /* Saved previous max field length */
   char lea[L_SYMBNM+1];      /* Scan space */

   RKC.ktest &= ~VCK_ADJ;     /* Kill adjustment if none found */
   if (!(RK.scancode & RK_PMINUS)) return;
   oldsclen = scanlen(L_SYMBNM+1);
   scan(lea, RK_REQFLD|RK_FENCE);   /* Error if no value found */
   scanlen(oldsclen);               /* Restore old max length */
   if (RK.scancode & RK_ENDCARD) return;

   /* Look for a matching stored modifier */
   for (pvav=RKC.pfvav; pvav; pvav=pvav->pnvav) {
      if (!strncmp(lea+1, pvav->vname, L_SYMBNM)) goto GotOne;
      }
   /* No match, this is a user error */
   ermark(RK_VANSERR);
   return;

/* Found a matching stored modifier, store it and turn on VCK_ADJ */
GotOne:
   RKC.dvadj = pvav->adjval;
   if (lea[0] == '-') RKC.dvadj = -RKC.dvadj;
   RKC.ktest |= VCK_ADJ;
   return;
   } /* End ckvadj() */
