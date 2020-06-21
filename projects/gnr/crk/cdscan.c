/* (c) Copyright 1988-2018, The Rockefeller University *11117* */
/* $Id: cdscan.c 68 2018-08-15 19:33:46Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*               CDSCAN, SCAN, SCANCK, SCANAGN, SCANLEN,                *
*                SKIP2END, THATSALL, VSLOOKUP, CURPLEV                 *
*                   Control card scanning routines                     *
*                                                                      *
*  These routines are used to extract fields (tokens) from control     *
*  cards read by cryin.  cdscan must be called first to initialize     *
*  scanning.  scan must be called once for each field.  Fields are     *
*  read left-to-right sequentially and are moved to an array speci-    *
*  fied by the user.  Continuation lines are read as required.         *
*  scanck combines scanning and punctuation checking.                  *
*                                                                      *
************************************************************************
*  V1A, 11/06/88, GNR - Convert from my IBM 370 Assembler version      *
*  V1B, 03/08/89, GNR - Make asterisks behave by default as data       *
*     characters, add RK_ASTDELIM flag, allow '),' on RK_NEWKEY,       *
*     allow RK_PMDELIM and RK_ASTDELIM in cdscan calls                 *
*  Rev, 03/17/89, GNR - Mark continuations with ACCPT_CS               *
*  Rev, 04/03/89, GNR - Put subroutines first, add erprnt              *
*     routine from cryout to eliminate recursive prototype             *
*  Rev, 04/19/89, GNR - Incorporate rkxtra.h                           *
*  Rev, 06/22/89, GNR - Correct bug in cdscan search for ':'           *
*  V1C, 07/24/89, GNR - Stop at newline, add okmark routine            *
*  V1D, 04/04/90, GNR - Correct scanagn bug, push/pop RK.length        *
*  V1E, 04/15/90, GNR - Add RK_IMMLFTP code, clarify code 4 usage      *
*  V1F, 04/19/90, GNR - Have scanagn save long fields via malloc       *
*  Rev, 07/10/92, GNR - Bug fix--scan() was storing into absolute      *
*     location zero when called with NULL field by cdscan for skip     *
*  Rev, 08/08/92, GNR - Make ercode unsigned for proper right shift    *
*  Rev, 08/26/92, GNR - Treat tabs as whitespace                       *
*  Rev, 05/28/94, GNR - Add scanck routine                             *
*  Rev, 10/10/96, GNR - Add skip2end routine, fix bug that would       *
*     cause special cards to be unrecognized after continuations       *
*  Rev, 06/03/97, GNR - Bug fix, looking at RK.last data when NULL     *
*  Rev, 08/20/97, GNR - Accept any whitespace as continuation marker   *
*  Rev, 10/31/97, GNR - Add thatsall routine                           *
*  Rev, 08/23/98, GNR - Add &SYS symbols and environment variables,    *
*     slashes and file names, CS->RKC, ermark out, backslash as        *
*     general escape, continuation of quoted string now needs just     *
*     one tab or blank                                                 *
*  Rev, 08/20/99, GNR - Do not pass +/- into parens in e.g. '-(xyz)'   *
*  Rev, 09/04/99, GNR - Remove support for NCUBE                       *
*  Rev, 07/03/00, GNR - Better error marking for skpblnks delimiters   *
*  Rev, 10/24/01, GNR - New return code defs, same as RK.scancode      *
*  Rev, 10/18/03, GNR - Left paren cancels NEWKEY error                *
*  Rev, 02/04/07, GNR - Add CrkParse facility, accept double quotes    *
*  Rev, 05/18/08, GNR - Add curplev function                           *
*  ==>, 05/18/08, GNR - Last date before committing to svn repository  *
*  Rev, 01/07/09, GNR - Add PMEQPM, NOPMEQ modes, 02/14/09 RK_PMVADJ   *
*  Rev, 08/10/09, GNR - Replace cuserid() with getpwuid(getuid())      *
*  Rev, 03/17/10, GNR - Check for '+=' or '-=' any time RK_NOPMEQ is   *
*                       off and before checking single +|- delimiters  *
*  Rev, 12/23/15, GNR - Handle '+', '-', '()' as data in filenames     *
*  Rev, 03/25/16, GNR - Correct bug trying ACCPT_AGN on new cdscan()   *
*  R65, 09/25/17, GNR - Expand symbols to hold up to MXL_SYMBVAL chars *
*     Always return RK.length = length(symbol) - 1                     *
*  R68, 08/04/18, GNR - Remove SUNOS support                           *
***********************************************************************/

/*---------------------------------------------------------------------*
*        Usage:                                                        *
*---------------------------------------------------------------------*/

/*---------------------------------------------------------------------+
|     To initialize:                                                   |
|        void cdscan(char *card, int displ, int maxlen, int csflags)   |
+----------------------------------------------------------------------+

   'card' is an array containing the card to be scanned.
   'displ' is the displacement of the first character to be inspected.
      It is measured in words if the 'csflags' RK_WDSKIP flag is set,
      otherwise in columns.  'displ' is used to skip over fixed infor-
      mation at the beginning of a card, but may be overridden with a
      colon by the user.  Word skipping permits users to abbreviate
      card identifiers without using a colon.  The colon is still
      needed, however, when the user omits some words in the card
      identifier.  Use of RK_WDSKIP is recommended for new programs.
   'maxlen' is the length in characters (not including the end-of-
      string character) of the longest field which the calling pro-
      gram is prepared to process, i.e. the length of the 'field'
      argument of the following scan calls.  If any field exceeds
      maxlen, an error message is issued and the value, truncated
      to maxlen characters, is returned to the scan caller.
         (Note:  Users are conditioned to expect 16 as the normal
      value of maxlen.  This is defined as DFLT_MAXLEN in rocks.h.
   'csflags' is the sum of various named constants denoting special
      options of the scanning routines.  At present, the following
      flags are defined:
         RK_AMPNULL  1  Ampersands are treated as null characters.
            Used by cryin to process variable symbols.
         RK_NEST     2  The parentheses nesting level is returned
            in RK.plevel after each scan call.  If this flag is
            not present, nested parentheses are an error.
         RK_NOCONT   4  Do not attempt to read continuation lines
            from cryin--'card' is a string, not an actual card.
         RK_NOPMEQ  64  The combinations '+=' and '-=' are never
            converted to '=+' or '=-'.
         RK_WDSKIP 128  'displ' is the number of words to skip rather
            than columns at the beginning of the control card.
      In addition, flags RK_PMDELIM, RK_ASTDELIM, and RK_SLDELIM may
      be passed through to scan during the initial word skip process.

+----------------------------------------------------------------------+
|     To parse a single field:                                         |
|         int scan(char *field, int scflags)                           |
+----------------------------------------------------------------------+

   'field' is an array of length 'maxlen'+1 into which the results
      are placed.  If 'field' is a NULL pointer, a field is skipped
      without returning anything.
   'scflags' contains option flags that control scanning options.
      It is the sum of the desired values from the following list:
         RK_NEWKEY   1  Signifies that a new keyword parameter is
            expected at this point.  A punctuation error is issued
            if the previous field was not delimited by a comma.
         RK_REQFLD   2  Indicates that a field is required at this
            point.  An error is issued if the end of the input
            record is reached during this scan call.
         RK_FENCE    4  Indicates that when the length of a field
            is equal to the maximum, a terminating NULL should not
            be appended.
         RK_PMDELIM  8  Indicates that plus and minus signs are to
            be interpreted as delimiters in this call, usually for
            purposes of interpreting algebraic formulas.  These
            delimiters are NOT stripped from the field, however,
            permitting them to act as both delimiters and signs.
         RK_ASTDELIM 16 Asterisks will be treated as delimiters,
            returning code RK_ASTERISK.  Unlike with the FORTRAN
            version, the default is to treat asterisks as data.
         RK_SLDELIM  32 Slashes will be treated as delimiters,
            returning code RK_SLASH.
         RK_SCANFNM  64 Indicates a file name is being parsed.
            Underscore, '+', '-', and '()' are treated as data.
            Slashes are treated as data or delimiters according to
            the state of the RK_SLDELIM flag, allowing path names
            to be separated or not.  This code quietly takes pre-
            cedence over RK_NEST and RK_PMxxxx family (i.e. a
            filename cannot be handled properly in parentheses--
            an unlikely need).  If support for Windows is added,
            this code should make single backslash be treated as
            slash according to the RK_SLDELIM flag.
         RK_WDSKIP  128 Used internally when called from cdscan()
            to perform initial word skip.
         RK_PMEQPM  256 Indicates that '+=' (entered as two consecu-
            tive characters) entered in the previous field is to be
            treated as if entered as '=+' and '-=' is to be treated
            as '=-'.  The RK_EQUALS code will have been returned on
            the previous call.  The plus or minus sign is prefixed
            to the data for the present field.  If this code is not
            entered, these combinations are treated as errors.
            (This is for the convenience of kwscan code 'K' input.)
         RK_PMVADJ  264 (=RK_PMDELIM|RK_PMEQPM)  Treats '+' and '-'
            as delimiters as for RK_PMDELIM, except that if one of
            these signs is followed by a digit, it is treated as
            data (to allow numbers with exponents in data).

      N.B.  The combinations '+=' and '-=' are treated as follows:
         (1) RK_PMDELIM, RK_PMVADJ, and RK_NOPMEQ all not set.
            This might be a keyword field ending a positional set.
            Data up to the sign returned with scancode RK_EQUALS.
            If RK_PMEQPM is set on next call, sign is returned at
            start of next field, otherwise there is an error.
         (2) RK_PMDELIM or RK_PMVADJ set and RK_NOPMEQ not set.
            Same as case (1) except scancode on the first call is
            RK_PMINUS|RK_EQUALS.
         (3) RK_PMDELIM and RK_PMVADJ not set, RK_NOPMEQ set.
            The sign is included in the data for the current call
            and the scancode is RK_EQUALS.
         (4) RK_PMDELIM or RK_PMVADJ set and RK_NOPMEQ set.  These
            are just separate delimiters.  Two fields are returned.
            The first contains the data up to the sign and returns
            scancode RK_PMINUS.  The second has the sign as the
            only data character and the scancode is RK_EQUALS.

      N.B.  I considered adding a code to check for equals sign at
         end of previous field, but this usually involves a special
         action to skip the key, so it must be a caller function.

      The scan function returns a code which is also stored in
         common cell RK.scancode.  The length of the field minus 1
         is stored in RK.length.  The possible return codes are:
            RK_BLANK        0  blank is only separator
            RK_PMINUS       1  RK_PMDELIM: plus or minus
            RK_COMMA        2  comma ended field
            RK_EQUALS       4  equals sign
            RK_ASTERISK     8  RK_ASTDELIM: asterisk
            RK_SLASH       16  RK_SLDELIM:  slash
            RK_LFTPAREN    32  left paren
            RK_INPARENS    64  in parens
            RK_RTPAREN    128  right paren
            RK_COLON      256  colon
            RK_SEMICOLON  512  semicolon comment (CrkParse)
            RK_ENDCARD   1024  card ended by blanks

+----------------------------------------------------------------------+
|     To parse a single field and check the delimiter                  |
|         int scanck(char *field, int scflags, int badpn)              |
+----------------------------------------------------------------------+

   'field', 'scflags', and return value are the same as for 'scan'.
   'badpn' is the sum of any of the RK.scancode punctuation codes.
      If any of these codes is returned by the scan() call, then
      ermark() is called to indicate a punctuation error.

+----------------------------------------------------------------------+
|     To "push back" a field that has been scanned:                    |
|        void scanagn(void)                                            |
+----------------------------------------------------------------------+

   The same field, scan code, and length will be returned again on
      the next call to scan.  Until then, the scan code and length
      from the previous field are visible.  A field that has been
      skipped over can not be pushed back.

+----------------------------------------------------------------------+
|     To determine the paren level at the current scan location:       |
|        int curplev(void)                                             |
+----------------------------------------------------------------------+

   Note that RK.plevel contains the paren level of the current data
      field.  This will not be the same as the current paren level
      if one or more right parens ended the current data field.

+----------------------------------------------------------------------+
|     To change the maximum field length for subsequent scan calls:    |
|        int scanlen(int maxlen)                                       |
+----------------------------------------------------------------------+

   The maxlen argument from the cdscan call is replaced by the
      new value, which may be larger or smaller than the original
      value.  The user must assure that the field argument to all
      subsequent scan calls is large enough to hold maxlen chars.
      The old length is returned so can be restored if desired.

+----------------------------------------------------------------------+
|     To skip over continuations to the next control card:             |
|        void skip2end(void)                                           |
|        void thatsall(void)                                           |
+----------------------------------------------------------------------+

   skip2end() reads and prints continuation cards until a card is
      encountered that is not a continuation card.  It then calls
      rdagn().  Its purpose is to eliminate unwanted "unexpected
      continuation card" messages when scanning is terminated early.
   thatsall() does the same thing except it prints an error message
      if any further fields are actually found.

*/

/*---------------------------------------------------------------------*
*        Parsing rules                                                 *
*---------------------------------------------------------------------*/

/*
   Fields are terminated by commas, equals signs, left parentheses,
      blanks, and by asterisks if the RK_ASTDELIM flag is on, by
      slashes if the RK_SLDELIM flag is on, by plus or minus signs
      if the RK_PMDELIM flag is on.  '+=' or '-=' (adjacent only)
      are handled as described above.  Any number of blanks or tabs
      is equivalent to a single blank.
   These delimiters are ignored if escaped with a backslash or if
      part of a field enclosed in single or double quotes.
   Cards being scanned may be continued on as many further cards
      as needed.  To continue, break off at a comma, (, or = sign,
      or put a backslash at the end of the record, and begin the
      next line with one or more whitespace characters.  When the
      backslash occurs in a quoted string, the field continues at
      the second character of the following card.  Comments are
      allowed on any card, separated from text by a semicolon.
   Scanning is terminated by a '\0' (cryin) or a '\n' (fgets).
   More detailed rules are given in the file ccrules.crk.
*/

/*---------------------------------------------------------------------*
*        Global definitions                                            *
*        (Includes items that must survive across calls)               *
*---------------------------------------------------------------------*/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "sysdef.h"
#include "rocks.h"
#include "rockv.h"
#include "rkxtra.h"
#ifdef UNIX
#ifdef LINUX
#define __USE_XOPEN
#endif
#include <sys/utsname.h>      /* Proto for uname() */
#include <unistd.h>           /* Proto for gethostname(), getcwd */
#include <sys/types.h>
#include <pwd.h>
#endif

/*=====================================================================*
*                        Macro or Function Tc                          *
*                                                                      *
*        For dealing with filename parsing.  Under UNIX, the argument  *
*        is returned unchanged.  Support for DOS removed, 08/18/07     *
*        Note, if rules are not UNIX rules, this code must convert     *
*        directory separator to slashes on the fly, because we cannot  *
*        change the user's input string and the cases in the main      *
*        switch must be constants.                                     *
*=====================================================================*/

#define Tc(c) (c)

/*=====================================================================*
*                          Function ISCONTNU                           *
*                                                                      *
*        Returns TRUE if argument string is a valid continuation       *
*        card (begins with at least one tab or blank), otherwise       *
*        FALSE.  This rather trivial test is put in a separate         *
*        function to localize knowledge of the continuation rule.      *
*=====================================================================*/

int iscontnu(char *card) {

   return (card && (*card == ' ' || *card == '\t')) ? TRUE : FALSE;

   } /* End iscontnu() */


/*=====================================================================*
*                          Function CONTEST                            *
*                                                                      *
*        Determines whether a continuation card is present.            *
*        If so, reads it, prints it if the previous card was           *
*           printed, and sets pc to point to the new card.             *
*        Returns YES if a continuation was found, otherwise NO.        *
*        The caller is assumed to have a reason to expect a            *
*           continuation, e.g. CEPA bit on or backslash found.         *
*        This routine is visible only inside this source file.         *
*=====================================================================*/

static int contest(void) {

   unsigned short taccpt;           /* Value of accpt before cryin */

/* If RK_NOCONT flag is set, a string is being scanned
*  and continuation is not possible.  */

   if (RKC.qcsflags & RK_NOCONT) {
      RKC.accpt &= ~(ACCPT_CS | ACCPT_MKOK | ACCPT_MKNO);
      return NO; }

/* Read a card and see if it is a valid continuation.  If not, flag an
*  error message and signal reread.  This was changed from a straight
*  cryout() call to an ermark() to allow unpaired quotes or parens to
*  be detected and '$' marked before it's too late.  (cryin() must not
*  clear ACCPT_CS when called with ACCPT_CE set, do it here.)  */

   taccpt = RKC.accpt;              /* Save old value of accpt */
   RKC.accpt |= ACCPT_CE;           /* Expect continuation */
   RKC.cdsave = RKC.pc = cryin();   /* Read another card */
   if (!iscontnu(RKC.cdsave)) {
      RKC.accpt |= ACCPT_RR;        /* No good, shove back */
      /* Indicate not being scanned, thereby killing '$' mark */
      RKC.accpt &= ~(ACCPT_CS | ACCPT_MKOK | ACCPT_MKNO);
      ermark(RK_ECNFERR);
      return NO; }                  /* Leave ACCPT_CE set! */
   RKC.accpt &= ~ACCPT_CE;          /* Shut off continuation switch */

/* Got one, print it if parent was printed and skip to start of data */

   if (taccpt & ACCPT_CEP) cdprt1(RKC.cdsave);
   /* If in quotes, skip over just one blank or tab */
   if (RKC.f & (SQT|DQT)) RKC.pc += 1;
   /* Otherwise, new policy, skip any number of white spaces */
   else {
      while ((*RKC.pc==' ') || (*RKC.pc=='\t')) RKC.pc++ ;
      if (*RKC.pc == '\0') {        /* End of card, no contents */
         RKC.accpt &= ~(ACCPT_CS | ACCPT_MKOK | ACCPT_MKNO);
         ermark(RK_BCNFERR);        /* Request error message */
         return NO; }
      } /* End case of not in quotes */

   return YES;             /* Normal continuation found */

   } /* End contest() */


/***********************************************************************
*                          Function VSLOOKUP                           *
*                                                                      *
*        Looks up a variable symbol passed as a NULL-terminated        *
*        string.  The second argument is the length of the symbol      *
*        (redundant but caller already knows it, saves a strlen()).    *
*        Returns a pointer to the result and the length minus 1 of     *
*        the result in RK.length.  The symbol argument should be long  *
*        enough to contain the result of up to L_SYMBVAL (=255) chars. *
*        (The arg string may be changed to uppercase or may or may not *
*        be replaced by the result symbol value on return.             *
*        The result is valid only until the symbol table is changed    *
*        or deallocated (EXECUTE card).                                *
*                                                                      *
*        If the symbol is not found, NULL is returned.                 *
***********************************************************************/

char *vslookup(char *symbol, int ct) {

   time_t curtime;            /* Current time */
   struct tm *ptm;            /* Time pointer */
   struct RKVS *pst,*pste;    /* Pointer to symbol table */
   char   *pval;              /* Ptr to symbol value */
#ifdef UNIX
   struct passwd *pupw;       /* Ptr to user's password entry */
#endif

   static char *syssym[] = { "USER", "HOST", "DATE", "TIME",
      "CPU", "OS", "CMND", "CWD", "NDX" };
#define NSysSym (sizeof syssym/sizeof syssym[0])

   /* Case I:  Search for user symbol.  Be sure name is not
   *  too long and a symbol table exists.  There are probably
   *  never enough symbols used to justify a hash table...  */

   if ((ct <= L_SYMBNM) && (RKC.cduptr->nvs > 0)) {
      pste = RKC.cduptr->pfvs + RKC.cduptr->nvs;
      for (pst=RKC.cduptr->pfvs; pst<pste; pst++)
            if (strncmp(symbol, pst->vname, L_SYMBNM) == 0) {
         RK.length = (short)pst->vsize - 1;
         return pst->pval;
         }
      }

   /* Case II:  Search for system symbol.  */

   if (ssmatch("SYS", symbol, 3))
      switch (smatch(RK_NMERR, symbol+3, syssym, NSysSym)) {
   case 1:                       /* &SYSUSER */
#if defined(UNIX)
      if ((pupw = getpwuid(getuid())) != NULL) {
         strncpy(symbol, pupw->pw_name, L_SYMBVAL);
         RK.length = strlen(symbol) - 1;
         return symbol;
         }
#endif
      RK.length = 4;
      return "?USER";

   case 2:                       /* &SYSHOST */
#if defined(UNIX)
      if (gethostname(symbol, L_SYMBVAL) == 0) {
         RK.length = strlen(symbol) - 1;
         return symbol;
         }
#endif
      RK.length = 4;
      return "?HOST";

   case 3:                       /* &SYSDATE */
      time(&curtime);
      ptm = localtime(&curtime);
      /* Use low-level routines here--
      *  may have been called from inside inform/convrt */
      {  int tym;                /* Temp for year and month */
         tym = ptm->tm_mon + 1;
         wbcdwt(&tym, symbol, RK_IORF|RK_NINT|RK_NPAD0|1);
         wbcdwt(&ptm->tm_mday, symbol+3, RK_IORF|RK_NINT|RK_NPAD0|1);
         tym = ptm->tm_year + 1900;
         wbcdwt(&tym, symbol+6, RK_IORF|RK_NINT|RK_NPAD0|3);
         } /* End tym local scope */
      symbol[2] = symbol[5] = '/';
      RK.length = 9;
      return symbol;

   case 4:                       /* &SYSTIME */
      time(&curtime);
      ptm = localtime(&curtime);
      wbcdwt(&ptm->tm_hour, symbol,  RK_IORF|RK_NINT|RK_NPAD0|1);
      wbcdwt(&ptm->tm_min, symbol+3, RK_IORF|RK_NINT|RK_NPAD0|1);
      wbcdwt(&ptm->tm_sec, symbol+6, RK_IORF|RK_NINT|RK_NPAD0|1);
      symbol[2] = symbol[5] = ':';
      RK.length = 7;
      return symbol;

   case 5:                       /* &SYSCPU  */
#if defined(UNIX)
      {  struct utsname utsn;
         if (uname(&utsn) >= 0) {
            strncpy(symbol, utsn.machine, L_SYMBVAL);
            RK.length = strlen(symbol) - 1;
            return symbol;
            }
         else {
            RK.length = 3;
            return "?CPU";
            }
         }
#else
      RK.length = 3;
      return "?CPU";
#endif

   case 6:                       /* &SYSOS   */
#if defined(UNIX)
      {  struct utsname utsn;
         if (uname(&utsn) >= 0) {
            strncpy(symbol, utsn.sysname, L_SYMBVAL);
            RK.length = strlen(symbol) - 1;
            return symbol;
            }
         else {
            RK.length = 3;
            return "UNIX";
            }
         }
#elif defined(MVS)
      RK.length = 2;
      return "MVS";
#elif defined(VM)
      RK.length = 1;
      return "VM";
#else
      RK.length = 2;
      return "?OS";
#endif

   case 7:                       /* &SYSCMND  */
      /* This code knows too much about cryout innards */
      pval = gettit() - (ct = 36);
      while (ct && pval[ct-1] == ' ') --ct;
      RK.length = ct - 1;
      return pval;

   case 8:                       /* &SYSCWD  */
#if defined(UNIX)
      if ((pval = getcwd(symbol, L_SYMBVAL)) != NULL) {
         RK.length = strnlen(pval, L_SYMBVAL) - 1;
         return pval;
         }
#endif
      RK.length = 3;
      return "?CWD";

   case 9:                       /* &SYSNDX  */
      RKC.sysindex += 1;
      wbcdwt(&RKC.sysindex, symbol, RK_IORF|RK_NI32|RK_LFTJ|LONG_SIZE);
      return symbol;
      } /* End system symbol name switch */

   /* Case III:  Environment variable.  */
   if ((pval = getenv(symbol)) != 0) {
      RK.length = strnlen(pval, L_SYMBVAL) - 1;
      return pval;
      }

   /* Error - symbol not matched */
   return NULL;

   } /* End vslookup() */


/*=====================================================================*
*                           Function UPDATE                            *
*                                                                      *
*     Moves data from arg to a result field and updates remct.         *
*     Replaces variable symbols with their values.                     *
*     Advances RKC.pc to point to next character.                      *
*                                                                      *
*     Argument:                                                        *
*        pn       Ptr to character following last one to be copied.    *
*=====================================================================*/

static void update(char *pn) {

   char *tval = RKC.pc;       /* Ptr to start of data to be moved */
   int ct;                    /* Number of characters to move */

/* Update source pointer at once so ermark() will mark correct spot */

   RKC.pc = pn + 1;           /* Update source pointer */

/* Exit at once if nothing to move.
*  Note:  Order of these tests is significant.  */

   if ((ct = pn - tval) <= 0) goto noupd;
   if (!RKC.pfield) goto skipupd;

/* If last field was a variable symbol, process it now */

   if (RKC.f & AMP) {

      /* Make a NULL-terminated copy of the symbol name for matching
      *  purposes.  If it is being continued with a backslash, go on
      *  scanning.  This maneuver also allows system and environment
      *  symbol names to be longer than L_SYMBNM or DFLT_MAXLEN (up
      *  to a max of L_SYMBVAL).  */
      if (ct > RKC.remsym) {           /* Error--symbol too long */
         ermark(RK_LENGERR);
         goto skipupd; }
      memcpy(RKC.psymbol, tval, ct);   /* Save symbol for lookup */
      *(RKC.psymbol += ct) = '\0';
      RKC.remsym -= ct;
      if (Tc(*pn) == '\\') {           /* Return if continued */
         RKC.f &= ~(IN|CEPA);
         goto noupd; }
      if ((tval = vslookup(RKC.symbol, ct)) != NULL)
         ct = RK.length + 1;
      else {
         ermark(RK_SYMBERR);
         goto skipupd;
         }
      }

/* Check that there is enough space left, then move the current
*  value to the user's field, omitting end fence if requested.  */

   if (ct > RKC.remfld) {     /* Error if field length exceeded */
      ermark(RK_LENGERR);
      if ((ct = RKC.remfld) <= 0) goto skipupd; }
   memcpy(RKC.pfield, tval, ct);    /* Move the data */
   RKC.pfield += ct;                /* Update field pointer */
   if ((RKC.remfld -= ct) > 0 || !(RKC.qscflags & RK_FENCE))
      *RKC.pfield = '\0';           /* Terminate field */
skipupd:                      /* Here when skipping nonzero update */
   RKC.f &= ~(IN|CEPA|AMP);   /* Clear initial, continue, symbol */
noupd: ;                      /* Here when nothing to move */

   } /* End update() */


/***********************************************************************
*                                                                      *
*                            Function SCAN                             *
*                                                                      *
***********************************************************************/

int scan(char *field, int scflags) {

/* Local declarations: */

   char *pn;                  /* Pointer to next character */
   int rc;                    /* Return code */
   int tl,tlmax;              /* Length temps */

/* Check for new key if requested--
*  03/08/89 version allows blanks and keys in parens */

   if ((scflags & RK_NEWKEY) &&
         (RK.scancode & (RK_PMINUS|RK_ASTERISK|RK_SLASH|RK_EQUALS)))
      RKC.accpt |= ACCPT_NKP;

/* Check for previous call to scanagn.  Note that maxlen can change
*  between when the field was saved and now, so it and the RK_FENCE
*  bit must be checked--this bug actually happened once upon a time. */

   if (RKC.accpt & ACCPT_AGN) {
      int ct = RKC.lgth2sv + 1;  /* Get length of stored field */
      RKC.accpt &= ~ACCPT_AGN;   /* Clear rescan flag */
      if (ct > RKC.qmaxlen) {    /* Error if field length exceeded */
         ermark(RK_LENGERR);
         ct = RKC.qmaxlen; }
      if (field && ct > 0) {     /* Restore field */
         memcpy(field, RKC.fldsave, ct);
         if ((ct < RKC.qmaxlen) || !(scflags & RK_FENCE))
            field[ct] = '\0';       /* Terminate field */
         }
      rc = RKC.rc2save;          /* Restore scan code */
      RK.length = ct - 1;        /* Restore length */
      RK.plevel = RKC.plev2sv;   /* Restore paren level */
      goto act5a;
      } /* End action to repeat previous scan */
   RKC.rc1save = RK.scancode;    /* Save old code for scanagn */
   RKC.lgth1sv = RK.length;      /* Save old length for scanagn */
   RKC.plev1sv = RK.plevel;      /* Save old plevel for scanagn */

/* Prepare to scan */

   if (field) *field = '\0';     /* Make return field null */
   RKC.pfield = field;           /* Set start of field */
   RKC.remfld = RKC.qmaxlen;     /* Set field capacity */
   RK.plevel -= RKC.nlplp;       /* Lower paren level */
   RKC.nlplp = 0;
   if (RKC.f & TER) goto actv;   /* Exit if at end of scan */
   RKC.qscflags = (byte)scflags; /* Save flags for use in functions */
   RKC.pcfd = NULL;              /* Clear ptr to first delimiter */

/* If in CrkParse mode, deal with pending semicolon comments.
*  If last character is a backslash, we have a continued comment.
*  In this case, leave the HSCC bit on, so next scan() call will
*  repeat sending comments to user until end of backslashes.
*  Then continue previous scan if continuation was signalled,
*  otherwise terminate this card.  */

   if (RKC.f & HSCC) {
dohscc:
      /* Find end of card and send everything to user data */
      pn = RKC.pc;
      while (*++pn) ;
      update(pn);
      /* If backslash, look for continued comment.  If not found,
      *  contest() issues error.  Terminate card on next call.  */
      if (pn[-1] == '\\') {
         if (!contest()) RKC.f ^= (HSCC|TER);
         rc = RK_SEMICOLON;
         goto act2c;
         }
      /* End of this comment, check for continued data */
      RKC.f ^= (HSCC|TER);
      if (RKC.sccsf & CEPA && contest()) RKC.f &= ~TER;
      rc = RK_SEMICOLON;
      goto act2c;
      }

/* If there is a pending '+' or '-' from the end of the previous field
*  in RK_PMEQPM mode, move it to the start of the new output field and
*  perform necessary remfld check and bookkeeping (do not use update()
*  because no user data to skip over, no symbols to substitute).  */

   if (RKC.f & (PLEQ|MIEQ)) {
      if (!(scflags & RK_PMEQPM)) ermark(RK_PUNCERR);
      else if (RKC.pfield && RKC.remfld > 0 /* JIC */ ) {
         *RKC.pfield++ = (RKC.f & PLEQ) ? '+' : '-';
         if (--RKC.remfld > 0 || !(scflags & RK_FENCE))
            *RKC.pfield = '\0';
         }
      RKC.f &= ~(PLEQ|MIEQ);
      }

/*---------------------------------------------------------------------*
*        Scan and take action according to character found             *
*---------------------------------------------------------------------*/

   RKC.f |= IN;               /* Initial in field */
   rc = 0;                    /* Clear return codes */
sstart:  for (pn=RKC.pc; ; pn++) {     /* Loop until delimiter found */

/* The following group should contain only those characters that
*  are recognized even when inside a quoted string.  */

      if (*pn == '\0' || *pn == '\n') goto activ;
                              /* Terminate if end of card */

      if (Tc(*pn) == '\\') {  /* Backslash */
         if (RKC.f & BTE) goto act2b;  /* Terminate tentative end */
         update(pn);                   /* Stash any previous data */
         while (*++pn == ' ' || *pn == '\t') ;      /* Look ahead */
         /* If followed only by whitespace to end of card, induce
         *  continuation, whether or not in quotes.  */
         if (*pn == '\0' || *pn == '\n' || *pn == ';') {
            if (contest()) goto sstart;
            goto act4t; }
         /* Not continued: if in quotes, treat the backslash as data,
         *  otherwise, escape the character after the backslash.  */
         pn = RKC.pc + 1;              /* Don't examine next char */
         if (RKC.f & (SQT|DQT) && *RKC.pc != '\'')
            RKC.pc -= 1;               /* Retain the backslash */
         continue;
         } /* End processing backslash */

/* When in a quoted string, treat everything except end of record or
*  another quote character as data.  */

      if (RKC.f & SQT) {      /* In single quoted string */
         if (*pn != '\'') continue;    /* Not quote, treat as data */
         if (*(pn+1) == '\'')          /* Check for two in a row */
            pn++;                      /* Swallow the second one */
         else                          /* Only one, */
            RKC.f &= ~SQT;             /*    end string */
         update(pn);                   /* Save data up to here */
         continue;                     /*    and keep scanning */
         } /* End processing in single quoted string */

      if (RKC.f & DQT) {      /* In double quoted string */
         if (*pn != '\"') continue;    /* Not quote, treat as data */
         if (*(pn+1) == '\"')          /* Check for two in a row */
            pn++;                      /* Swallow the second one */
         else                          /* Only one, */
            RKC.f &= ~DQT;             /*    end string */
         update(pn);                   /* Save data up to here */
         continue;                     /*    and keep scanning */
         } /* End processing in double quoted string */

/* Normal case (not in a quoted string)--check for delimiters.
*  Note:  any time RKC.f&IN is tested, update must be called first. */

      switch (Tc(*pn)) {      /* Check next character */

   case ' ' :                 /* Blank */
   case '\t':                 /* Tab treated same as blank */
      goto skpblnks;                /* Skip rest of blanks in group */

   case ',' :                 /* Comma */
      rc |= RK_COMMA;               /* Set return code and */
      goto actii;                   /*    terminate field */

   case '=' :                 /* Equals sign */
      rc |= RK_EQUALS;              /* Set return code and */
      goto actii;                   /*    terminate field */

   case '(' :                 /* Left paren */
      if (scflags & RK_SCANFNM)
         goto symdelim;             /* In a filename, treat as data */
      update(pn);                   /* Data to user field, skip '(' */
      if (RKC.f & IN) {             /* If initial, bump level, */
         if (RK.plevel++ && !(RKC.qcsflags & RK_NEST))
            ermark(RK_NESTERR);     /*    error for illegal nesting, */
         RKC.accpt &= ~ACCPT_NKP;   /*    prevent NEWKEY error, */
         RKC.f |= CEPA;             /*    set continue flag, */
         break; }                   /*    and continue scanning */
      rc |= RK_LFTPAREN;            /* Not initial, is terminator */
      goto act2a;                   /* End field, reuse paren later */

   case ')' :                 /* Right paren */
      if (scflags & RK_SCANFNM)
         goto symdelim;             /* In a filename, treat as data */
      if ((short)++RKC.nlplp > RK.plevel) {  /* If unmatched right */
         ermark(RK_PNQTERR);                 /* paren, give error */
         goto skpblnks; }                    /* & treat as blanks */
      RKC.f &= ~(IN|CEPA);             /* Terminate field */
      rc |= (RK_INPARENS|RK_RTPAREN);  /* Set in & end parens codes */
      goto skpblnks;                   /* Skip any blanks */

   case '*' :                 /* Asterisk */
      if (scflags & RK_ASTDELIM) {  /* Treat as delimiter? */
         rc |= RK_ASTERISK;            /* Set return code and */
         goto actii; }                 /*    terminate field */
      goto symdelim;                /* No, treat as data */

   case '/' :                 /* Slash */
      if (scflags & RK_SLDELIM) {   /* Delimiter? */
         rc |= RK_SLASH;               /* Set return code and */
         goto actii; }                 /*    terminate field */
      goto symdelim;                /* No, treat as data */

   case '\'' :                /* Single Quote */
      if (RKC.f & BTE) goto act2c;  /* Terminate tentative end */
      update(pn);                   /* Data to user field */
      RKC.f |= SQT;                 /* Flag "in-single-quotes" */
      break;

   case '\"' :                /* Double Quote */
      if (RKC.f & BTE) goto act2c;  /* Terminate tentative end */
      update(pn);                   /* Data to user field */
      RKC.f |= DQT;                 /* Flag "in-double-quotes" */
      break;

   case '.' :                 /* Period--terminate variable symbol */
      if (RKC.f & BTE) goto act2b;  /* Terminate tentative end */
      if (RKC.f & AMP)              /* If vs, end it, ignore period */
         update(pn);
      break;                        /* Continue scanning */

   case '&' :                 /* Ampersand--variable symbol */
      if (RKC.f & BTE) goto act2b;  /* Terminate tentative end */
      if (RKC.kparse & RKKP_NOVS)   /* CrkParse: treat as data */
         break;
      update(pn);                   /* Swallow the ampersand */
      if (!(RKC.qcsflags&RK_AMPNULL)) { /* Unless called from cryin, */
         RKC.f |= AMP;                 /* flag it as a symbol, */
         RKC.psymbol = RKC.symbol;     /* set start of symbol, */
         RKC.remsym = L_SYMBVAL; }     /* set field capacity.  */
      break;

   case '_' :                 /* Underscore */
      if (RKC.f & BTE) goto act2b;  /* Terminate tentative end */
      if (!(RKC.f & AMP) &&         /* Unless inside a symbol */
         !(scflags & RK_SCANFNM))   /*    or parsing a filename, */
         update(pn);                /* Swallow this character */
      break;

   case '+' :                 /* Plus sign */
   case '-' :                 /* Minus sign */
      if (scflags & RK_SCANFNM)
         goto symdelim;             /* In a filename, treat as data */
      /* Rev, 03/19/10, GNR - Behave according to new documentation.
      *  First check cases '+=' and '-='.  If RK_NOPMEQ is not set,
      *  this is a compound delimiter.  Data up to the sign are
      *  returned with code RK_EQUALS, plus RK_PMINUS if RK_PMDELIM
      *  is set.  The sign is returned at the start of the next field
      *  if RK_PMEQPM is set.  If RK_NOPMEQ is set, these codes are
      *  not special.  The sign is a delimiter if RK_PMDELIM is set,
      *  otherwise just ordinary data--handled by normal code.  */
      if (pn[1] == '=' && !(RKC.qcsflags & RK_NOPMEQ)) {
         update(pn);
         RKC.f |= (*pn == '+') ? PLEQ : MIEQ;
         ++RKC.pc;
         rc |= scflags & RK_PMDELIM ? RK_EQUALS|RK_PMINUS : RK_EQUALS;
         goto act2b;
         }
      /* Not '+=' or '-=' (or one of those but RK_NOPMEQ set).
      *  The sign is a delimiter if RK_PMDELIM is set, or RK_PMVADJ
      *  is set and the sign is not followed by a digit.  Otherwise,
      *  the sign can terminate a symbol, but is treated as data.
      *  Note that we must test for initial before calling update so
      *  as not to skip over the sign if it IS initial--this in turn
      *  necessitates the double form of test for initial.  */
      if (scflags & RK_PMDELIM) {   /* Treat as delimiter? */
         if (!(RKC.f & IN) || (pn > RKC.pc)) {
            if (scflags & (RK_PMVADJ^RK_PMDELIM) &&
                  pn[1] >= '0' && pn[1] <= '9') goto symdelim;
            update(pn);                /* Not initial, move data to */
            rc |= RK_PMINUS;           /*    field, set return code, */
            goto act2a; }              /*    and terminate with reuse */
         update(pn+1);                 /* Initial, move to field, */
         RKC.f |= CEPA;                /*    anticipate continuation, */
         goto skpbnoup;                /*    and skip over blanks */
         }
symdelim:                           /* Common symbol delimiter code */
      if (RKC.f & BTE) goto act2b;  /* Terminate tentative end */
      if (RKC.f & AMP) {            /* Terminate symbol */
         update(pn);
         RKC.pc = pn; }             /*    and reuse delimiter */
      break;

   case ';' :                 /* Semicolon */
      /* If in CrkParse mode, treat like blank termination except
      *  set the HSCC bit so the comment will be returned as data
      *  on the next scan() call.  Save CEPA status in sccsf.  */
      if (RKC.kparse & RKKP_CRKP) {
         RKC.sccsf = RKC.f;
         update(pn);
         RKC.f |= HSCC;
         if (RKC.f & IN) goto dohscc;
         goto act2c;
         }
      goto activ;                   /* Terminate scan */

   case ':' :                 /* Colon */
      if (scflags & RK_WDSKIP) { /* If performing initial skip, */
         rc = RK_COLON; goto actii; }  /* terminate skipping */
                                    /* Otherwise, fall through ... */

   default :                  /* Data character found */
      if (RKC.f & BTE) goto act2b;  /* Terminate tentative end */
      if (RKC.accpt & ACCPT_NKP) {  /* Issue pending NEWKEY error */
         ermark(RK_PUNCERR); RKC.accpt &= ~ACCPT_NKP; }

      } /* End switch */
   } /* End scan loop */

/*---------------------------------------------------------------------*
*        SKPBLNKS                                                      *
*        Blanks, tabs, closing quotes, and right parens are considered *
*        nonterminating delimiters in the sense that they can be       *
*        followed by other delimiters, e.g. "),", etc.  SKPBLNKS       *
*        skips over these combinations until a terminating CEPA        *
*        delimiter is found.  The BTE (blank-tentative-end) flag       *
*        is set if non-initial, signalling that characters that        *
*        normally occur at the start of a new field (quote, left       *
*        paren, ampersand, plus, minus, or data) should terminate      *
*        the old field.  These are rescanned on the next call.         *
*---------------------------------------------------------------------*/

skpblnks:
         update(pn);
         if (!(RKC.f & IN)) {
            RKC.f |= BTE;
            /* Save location of first delim */
            if (!RKC.pcfd) RKC.pcfd = RKC.pc;
            }
skpbnoup:
         /* Skip over blanks or tabs */
         while (*++pn == ' ' || *pn == '\t') ;
         RKC.pc = pn;            /* Update field pointer */
         goto sstart;            /* Resume scanning */

/*---------------------------------------------------------------------*
*        Actions to end fields                                         *
*---------------------------------------------------------------------*/

/* Action II - Field ended by continuation-inducing delimiter--
*              gives nonblank return code, swallows delimiter
*  Action 2a - Field ended by starter of next field--
*              gives blank return code, backup scan pointer
*  Action 2b - Skpblnks encounters a field starter that must
*              be reused--gives blank return code, leaves pc
*  Action 2c - Field ended by semicolon or end-of-card--
*              gives blank return code, no CEPA, leaves pc */

actii:   update(pn);             /* Data to user field */
         goto act2b;
act2a:   RKC.pc--;               /* Restart on this delimiter */
act2b:   RKC.f |= CEPA;          /* Expect continuation */
act2c:   RKC.f &= ~BTE;          /* Clear tentative end */
         if (RK.plevel) rc |= RK_INPARENS; /* Signal if in parens */
         /* Calculate field length and save one smaller */
         RK.length = (tl = RKC.qmaxlen - RKC.remfld) - 1;
         /* Allocate enough space for longest field to date */
         if (RKC.lfldsave < (tlmax = max(tl,DFLT_MAXLEN) + 1)) {
            RKC.lfldsave = (tlmax + 15) & (-16);
            RKC.fldsave = reallocv(RKC.fldsave, RKC.lfldsave,
               "Scanagn buffer");
            }
         if (field)              /* Save field if there is one */
            strcpy(RKC.fldsave, field);
         else                    /* Otherwise, save NULL string */
            *RKC.fldsave = '\0';
         goto act5a;             /* Complete the return */

/* Action IV - End of field and card */

activ:   update(pn);             /* Data to user field */
         if ((RKC.f & CEPA) && contest()) goto sstart;
         while (RK.last && Tc(*(RK.last+strlen(RK.last)-1)) == '\\' &&
            (contest())) ;       /* Skip over continued comments */
act4t:   RKC.f |= TER;           /* Terminate at once next time */
         /* Error if unmatched parens or quotes */
         if (RK.plevel > (short)RKC.nlplp || RKC.f & (SQT|DQT))
            ermark(RK_PNQTERR);

         /* If there are data in the field, treat as a normal end of
         *  field.  Otherwise, fall through to Action V, which will
         *  give an error message if the user has indicated that this
         *  is a required field.  */

         if (!(RKC.f & IN)) goto act2c;

/* Action V - End of card while initial in field */

actv:    rc = RK_ENDCARD;         /* Normal end code */
         if (scflags & RK_REQFLD) ermark(RK_REQFERR);

         /* Enter here to pop previous field after scanagn */
act5a:   RK.scancode = rc;    /* Save return code */
         return rc;

   } /* End scan() */

/***********************************************************************
*                                                                      *
*                          Subroutine CDSCAN                           *
*                                                                      *
***********************************************************************/

void cdscan(char *card, int displ, int maxlen, int csflags) {

/* Save parameters and clear flags */

   RK.scancode = 0;
   RK.plevel = 0;
   RKC.f = 0;                 /* Clear all flags */
   RKC.cdsave = card;         /* Save card location */
   RKC.qmaxlen = maxlen;      /* Save maxlen */
   RKC.nlplp = 0;             /* Kill parens from last card */
   RKC.qcsflags = csflags;    /* Save csflags */
   RKC.accpt &= ~ACCPT_AGN;   /* Don't try to rescan old card! */

/* Perform word or column skipping */

   if (csflags & RK_WDSKIP) {       /*** Skip words ***/
      register int iwd;       /* Local word counter */
      int ssfl = csflags &    /* Set scan flags */
         (RK_PMDELIM|RK_ASTDELIM|RK_SLDELIM|RK_WDSKIP);
      RKC.pc = card;          /* Start in col. 1 */
      for (iwd=0; iwd<displ; iwd++) /* Skip words */
         if (scan(NULL, ssfl) == RK_COLON) break;
      }
   else {                           /*** Skip columns ***/
      register char *ps;      /* Local skip pointer */
      if (displ >= strlen(card)) abexit(22);
                              /* Error if no stuff to scan */
      ps = card;
      while (ps < card+displ) /* Skip columns */
         if (*ps++ == ':') break;
      RKC.pc = ps;            /* Save data pointer */
      }

   RKC.accpt |= ACCPT_CS;     /* Signal that card is being scanned
                              *  (implies pc is valid for ermark) */

   } /* End cdscan() */


/***********************************************************************
*                                                                      *
*                          Function curplev                            *
*                                                                      *
***********************************************************************/

int curplev(void) {

   return RK.plevel - RKC.nlplp;

   } /* End curplev() */


/***********************************************************************
*                                                                      *
*                         Subroutine SCANAGN                           *
*                                                                      *
***********************************************************************/

void scanagn(void) {

   RKC.accpt  |= ACCPT_AGN;
   RKC.rc2save = RK.scancode;    /* Save new scan code */
   RKC.lgth2sv = RK.length;      /* Save new length */
   RKC.plev2sv = RK.plevel;      /* Save new plevel */
   RK.scancode = RKC.rc1save;    /* Restore old scan code */
   RK.length   = RKC.lgth1sv;    /* Restore old length */
   RK.plevel   = RKC.plev1sv;    /* Restore old plevel */

   } /* End scanagn() */


/***********************************************************************
*                                                                      *
*                           Function SCANCK                            *
*                                                                      *
***********************************************************************/

int scanck(char *field, int scflags, int badpn) {

   int ic = scan(field, scflags);
   if ((int)RK.scancode & badpn) ermark(RK_PUNCERR);
   return ic;

   } /* End scanck() */


/***********************************************************************
*                                                                      *
*                          Function SCANLEN                            *
*                                                                      *
***********************************************************************/

int scanlen(int maxlen) {

   int oldlen = RKC.qmaxlen;  /* Temporary old value */
   RKC.qmaxlen = maxlen;      /* Save maxlen */
   return oldlen;             /* Return old length */

   } /* End scanlen() */


/***********************************************************************
*                                                                      *
*                          Function SKIP2END                           *
*                                                                      *
***********************************************************************/

void skip2end(void) {

   int doprint = RKC.accpt & ACCPT_CEP;   /* Remember print status */
   RKC.accpt |= ACCPT_CE;   /* Signal cryin to expect continuation */

/* Read a card until one is found that is not a valid continuation.
*  When the card is a continuation, print it and read another.  */

   for (;;) {
      RKC.pc = cryin();       /* Read a card */
      if (!iscontnu(RKC.pc)) break;
      /* Print the card if its predecessor was printed */
      if (doprint) cdprt1(RKC.pc);
      }

   /* Not a continuation, signal reread (with ACCPT_CE still set) */
   RKC.accpt |= ACCPT_RR;     /* Shove the card back */
   /* Force termination is scan() is called again first */
   RKC.f |= TER;

   } /* End skip2end() */


/***********************************************************************
*                                                                      *
*                         Subroutine THATSALL                          *
*                                                                      *
***********************************************************************/

void thatsall(void) {

   if (scan(NULL, 0) != RK_ENDCARD) {
      ermark(RK_TOOMANY); skip2end(); }

   } /* End thatsall() */

