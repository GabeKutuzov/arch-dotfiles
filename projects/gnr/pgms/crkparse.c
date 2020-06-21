/* (c) Copyright 2007, The Rockefeller University *11113* */
/***********************************************************************
*                              CrkParse                                *
*                                                                      *
*  Overview:                                                           *
*  ---------                                                           *
*  CrkParse is an application that reads a file containing control     *
*  statements constructed in accordance with ROCKS syntax, parses it   *
*  according to the ROCKS syntax rules, and writes the results to      *
*  stdout in the format described below.  This can be used by any      *
*  program that wants to interpret ROCKS-style input without needing   *
*  to replicate all the facilities of cryin/cdscan.                    *
*                                                                      *
*  Interface:                                                          *
*  ----------                                                          *
*  The client application should start CrkParse with the path name of  *
*  the file to be processed on the command line along with possible    *
*  options as described below.  It then communicates with CrkParse     *
*  by sending queries on stdin (possibly via a pipe) and receiving     *
*  replies via stdout.                                                 *
*                                                                      *
*  The dialog between the appliction and CrkParse is as follows:       *
*  (1) Application starts CrkParse.                                    *
*  (2) CrkParse writes "<FF>CrkParse\n" (<FF> = FormFeed) to stdout.   *
*  (3) Application requests next field by sending the string "?\n"     *
*      (not including the quotes) to CrkParse on its stdin.            *
*  (4) CrkParse sends on its stdout to the application one field       *
*      enclosed in curly braces.  A field may be either a single       *
*      value or a key=value pair.  Every field begins with a punc-     *
*      tuation character to indicate its type, as defined below.       *
*      The punctuation character will unambiguously indicate whether   *
*      just a key or a key=value pair follows.  If there is a value,   *
*      it will always be enclosed in double quote characters.          *
*  Repeat steps (3) and (4) until CrkParse reads an END card or        *
*      end-of-file and sends {$END}\n to the application.              *
*  (5) The application, upon receipt of {$END}\n, when ready to        *
*      quit will send to CrkParse the string {QUIT}\n and CrkParse     *
*      will termninate.  The application can send this string at       *
*      any time in place of "?\n", for example, when an error is       *
*      detected in the input.  In any event the application will       *
*      NOT send any error messages to Crkparse.                        *
*                                                                      *
*  CrkParse can produce error messages on stderr at any time.          *
*  All non-error messages will be enclosed in curly braces {}          *
*  to differentiate error from non-error messages.                     *
*                                                                      *
*  Format of CrkParse Messages:                                        *
*  ----------------------------                                        *
*                                                                      *
*  (In this section, apostrophes (') are used to delimit information   *
*  that will appear in the CrkParse output.  These characters do not   *
*  actually appear in the output.  Double quotes (") where shown do    *
*  appear in the output.  The curly braces that surround each field    *
*  are not shown here.)  The return types are as follows:              *
*                                                                      *
*  (1) Start of a ROCKS control card (or end-of-file).                 *
*                                                                      *
*  CrkParse returns '$key' where 'key' is the keyword at the start     *
*  of the card.  There is no value field.                              *
*                                                                      *
*  (2) Start of an application control card.                           *
*                                                                      *
*  CrkParse returns '>key' where 'key' is the keyword at the start of  *
*  the card.  There is no value field.                                 *
*                                                                      *
*  (3) A parameter from a control card not of form "keyword = value".  *
*                                                                      *
*  CrkParse returns '#nn="value"' where 'nn' is the position of the    *
*  parameter relative to the keyword at the start of the card or       *
*  relative to a left parenthesis.  (When a set of fields in paren-    *
*  theses occurs in the middle of a string of positional parameters,   *
*  the position in the outer list will be incremented by one for the   *
*  entire parenthesized string.)                                       *
*                                                                      *
*  (4) A keyword=value combination from a control card.                *
*                                                                      *
*  CrkParse returns '%key="value"' where 'key' is the key from the     *
*  control card and 'value' is the value following the equals sign on  *
*  the control card.  However, if the value is a collection of fields  *
*  in parentheses, the value here is replaced by the code '(' and the  *
*  individual fields follow on separate lines.  If the keyword is      *
*  followed by a qualifier in parentheses, the qualifier(s) are        *
*  returned first following the rules given below for values in        *
*  parentheses.  The value is then returned on the next prompt         *
*  following the last qualifier field, coded as '="value"'.            *
*                                                                      *
*  (5) A comment that is on a card by itself.                          *
*                                                                      *
*  CrkParse returns '*"this is the comment"'.  This type of comment is *
*  deemed to be associated with the following control card.            *
*                                                                      *
*  (6) A comment that is on a control card.                            *
*                                                                      *
*  CrkParse returns ';"this is the comment"'.  This type of comment is *
*  deemed to be associated with the data on the control card on which  *
*  it appears.  If the comment is continued with the backslash mecha-  *
*  nism, each continuation card is returned as a separate item.        *
*                                                                      *
*  (7) Start of a group of parameters in parentheses.                  *
*                                                                      *
*  CrkParse returns '('.  There are no data items associated with this *
*  code.                                                               *
*                                                                      *
*  (8) End of a group of parameters in parentheses.                    *
*                                                                      *
*  CrkParse returns ')'.  There are no data items associated with this *
*  code.                                                               *
*                                                                      *
*  (9) Abbreviation of a control card name by a colon.                 *
*                                                                      *
*  CrkParse returns ':'.  The application will need to identify the    *
*  previous control card key and omit skipping further positional      *
*  words that might be part of a multi-word card identifier.  The      *
*  next item returned will be the first data item following the colon. *
*                                                                      *
*  Notes:                                                              *
*  ------                                                              *
*                                                                      *
*  (1) CrkParse will have no knowledge of the parameters used in a     *
*  particular application and therefore it will have no knowledge of   *
*  what constitutes valid keyword abbreviations nor of how many words  *
*  at the start of a card must be skipped before the first data item   *
*  (unless omitted following a colon).  CrkParse will return the exact *
*  text of each key to the application, which will be responsible to   *
*  generate an error if a key is ambiguous or not recognized.          *
*                                                                      *
*  (2) Any card command that is normally processed by the CRK library  *
*  rather than by the application (e.g.  OUTPRTY, TITLE, etc.) may or  *
*  may not be parsed to its individual data fields by CrkParse.  In    *
*  particular, EXECUTE and DEFAULTS card will be parsed and returned   *
*  as individual fields, whether or not substitution of symbolic pa-   *
*  rameters is requested (see below).  (The reason for this is to be   *
*  able to deal in a consistent way with continuations and comments    *
*  that may occur on such cards.)  This should be of no concern to     *
*  the application, which will simply treat any set of data fields     *
*  beginning with a '$' key as comments.  Depending on the function    *
*  of the application, it may need to reconstruct the ROCKS control    *
*  cards from their parsed components and copy them to its own         *
*  output.                                                             *
*                                                                      *
*  (3) All values will be enclosed in double quote characters.  This   *
*  way CrkParse doesn't have to check whether the original data on     *
*  the card was in quotes or not.                                      *
*                                                                      *
*  (4) CrkParse has no way to know where an application might expect   *
*  an input field to be longer than DFLT_MAXLEN.  Accordingly, it      *
*  allows for fields up to an arbitrary MXFLDLEN (=1000) chars.        *
*                                                                      *
*  Filling in of variable parameters:                                  *
*  ----------------------------------                                  *
*                                                                      *
*  The ampersand ('&') character is used to indicate symbolic          *
*  parameters in an input file.  Accordingly, this character is not    *
*  used to indicate a CrkParse type.                                   *
*                                                                      *
*  There may be situations where variable parameters should be left    *
*  intact, i.e.  passed through CrkParse in the form the user wrote    *
*  them (e.g.  "&thresh1").  This will be the default behavior of      *
*  CrkParse.  However, there may also be situations where the user     *
*  might like the normal CRK parameter-replacement mechanism to work,  *
*  so the actual parameter values (from DEFAULT or EXECUTE cards       *
*  earlier in the input file) should be filled in.  (In this case,     *
*  the variable parameters are lost forever from that input file.)     *
*  To distinguish these situations, CrkParse will accept the           *
*  following command-line parameters:                                  *
*     -r    Leave variable parameters alone (the default).             *
*     +r    Replace variable parameters with their values.             *
*           (Note:  This substitution may generate error               *
*           messages if, for example, a value is not provided          *
*           for some parameter.)                                       *
*                                                                      *
*  Executable ("include") files:                                       *
*  -----------------------------                                       *
*                                                                      *
*  There may be cases where the user would like any included           *
*  ("EXECUTE") files to be read by CrkParse and merged into the        *
*  input file, or cases where this should not occur.  To handle        *
*  these cases, CrkParse will accept the following command-line        *
*  parameters:                                                         *
*     +e    Execute any EXECUTE cards and merge the included           *
*           input into the output stream (the default).                *
*     -e    Just copy the EXECUTE card itself to the stdout,           *
*           with the key {$EXECUTE}.  The contents will be             *
*           treated as any other data (except to read variable         *
*           parameter values if +r is specified).                      *
*  When +e is in effect and parsing comes to the end of an included    *
*  EXECUTE file, crkparse will respond to the next prompt with a       *
*  '$ENDEXEC' pseudocard to indicate to the calling application that   *
*  parsing will now resume from the earlier input file that was        *
*  interrupted by EXECUTE card processing.                             *
*                                                                      *
*  Parenthetical material:                                             *
*  -----------------------                                             *
*                                                                      *
*  Material in parentheses will be handled by returning the indivi-    *
*  dual fields in the normal manner as described above, with codes     *
*  indicating the beginning and end of the parenthetical material      *
*  as described above.  There is no limit in the ROCKS library to      *
*  the depth of nesting of parentheses, however, to keep the coding    *
*  simple, an arbitrary (compile-time) limit is set in CrkParse.       *
*                                                                      *
*  Abexit error codes 165-169 are assigned to this program             *
************************************************************************
*  V1A, 02/14/07, GNR - New program                                    *
*  Rev, 03/24/07, GNR - Add $ENDEXEC, reverse meaning of +r/-r options *
***********************************************************************/

#define MAIN

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"

#define MXINLEN     8         /* Max length of stdin prompt */
#define MXFLDLEN 1000         /* Maximum input field */
#define MXPLEVEL    5         /* Maximum paren levels (see above) */
#define MXPWIDTH    4         /* Max width of position index */

/* These routines in cryin.c are not prototyped for general use */
struct RKCDU *cry_mkiu(char *fnm);
void cry_opiu(struct RKCDU *pcu);

int main (int argc, char *argv[]) {

   struct RKCDU *plcu = NULL; /* Ptr to last card unit */
   struct RKCDU *pncu;        /* Ptr to new card unit */
   struct RKVS *pvs;          /* Ptr to symbol table */
   struct RKVS *pvz;          /* Ptr to end of symbol table */
   char *key;                 /* Buffer for user key */
   char *lea;                 /* Buffer for user data */
   char *newcd;               /* Ptr to new card */
   char *pvsn;                /* Ptr to variable symbol name */
   int iarg;                  /* Argument counter */
   int klen,vlen;             /* Length of key,value */
   int chkeq,goteq = 0;       /* TRUE if equals after right paren */
   int dovs = 0;              /* TRUE to do variable substitution */
   int dlev = 0;              /* Change in parens level */
   int jlev = 0;              /* Previous parenthesis level */
   int nfiles = 0;            /* Putative file counter */
   int npos[MXPLEVEL];        /* Positional parameter counters */
   int norew = 0, noreu = 0;  /* NOREWIND, NOREUSE EXEC flags */
   enum { ReadNew, TitleText, NextField, GotColon, ExecPart1,
      ExecPart2, DfltPart1, DfltPart2 } kstate = ReadNew;
   /* Cases must match keyword order in crkcds below */
   enum CARDS { Title=1, OutPrty, Spout, Prompt, Error, Execute,
      Defaults, End } kcard;
   char apos[MXPWIDTH];       /* Buffer for position number */
   char inbuf[MXINLEN];       /* Buffer for client prompt */
   char exfnm[LFILNM+1];      /* EXEC file name */
   /* Possible command-line parameters */
   static char *args[] = { "-e", "+e", "+r", "-r" };
   /* Possible ROCKS special control cards */
   static char *crkcds[] = { "TITLE", "OUTPRTY", "SPOUT",
      "PROMPT", "ERROR", "EXECUTE", "DEFAULTS", "END" };

/*---------------------------------------------------------------------*
*                        Start up formalities                          *
*---------------------------------------------------------------------*/

   key = malloc(MXFLDLEN);
   lea = malloc(MXFLDLEN);
   RKC.kparse = (RKKP_CRKP|RKKP_DOEX|RKKP_NOVS);
   setpid("CrkParse");
   nopage(RK_PGNONE);
   cryout(RK_P1, " \fCrkParse", RK_FLUSH+RK_LN1+10, NULL);
   exfnm[0] = '\0';           /* Flag no file name yet */

/* Interpret command-line parameters.
*  N.B.  This program does not support concatenation of parameters.  */

   for (iarg=1; iarg<argc; ++iarg) switch (smatch(RK_NMERR,
         argv[iarg], args, sizeof(args)/sizeof(char *))) {

   case 0:                    /* No match, putative file name */
      if (nfiles++ > 0)
         abexitm(165, "Only one file name allowed");
      /* Set cryin to use this file, with no reuse when done */
      cdunit(argv[iarg]);
      RKC.cduptr->accpt |= ACCPT_NOR;
      break;
   case 1:                    /* -e parameter */
      RKC.kparse &= ~RKKP_DOEX;
      break;
   case 2:                    /* +e parameter */
      RKC.kparse |= RKKP_DOEX;
      break;
   case 3:                    /* +r parameter */
      dovs = TRUE;
      break;
   case 4:                    /* -r parameter */
      dovs = FALSE;
      break;

      } /* End parameter switch */

   if (nfiles != 1)
      abexitm(165, "File name required");

/*---------------------------------------------------------------------*
*                              Main Loop                               *
*---------------------------------------------------------------------*/

   while (fgets(inbuf, MXINLEN, stdin)) {

/* Prompt from user should be either "?" or "{QUIT}" */

      if (strcmp(inbuf, "{QUIT}\n") == 0) goto StopNow;
      if (strcmp(inbuf, "QUIT\n") == 0)   goto StopNow;
      if (strcmp(inbuf, "?\n"))
         abexitm(166, "Unexpected prompt received");

ReadNoPrompt:
      switch (kstate) {

/*---------------------------------------------------------------------*
*        Read a new card and process it if it is a ROCKS card          *
*---------------------------------------------------------------------*/

/* When RKC.kparse RKKP_CRKP mode is set, cryin and cdscan pass
*  comments automatically to the output in the right format, so this
*  code does not need to worry about them.  */

      case ReadNew:

         /* Save EOF indication until after ENDEXEC check, in
         *  case EXECUTE was last card before end of deck.  */
         newcd = cryin();

         /* If cryin popped to a previous unit, push the card back,
         *  issue '$ENDEXEC', and wait for next prompt.  */
         if (plcu && plcu != RKC.cduptr) {
            rdagn();
            cryout(RK_P1, " {$ENDEXEC}", RK_FLUSH+RK_LN1+11, NULL);
            plcu = NULL;
            continue;
            }

         if (!newcd) goto NoMoreCards;

         /* Initiate scan and pick up first keyword */
         cdscan(RK.last, 0, MXFLDLEN, RK_WDSKIP|RK_NEST);
         scan(key, RK_REQFLD|RK_WDSKIP);
         npos[jlev = 0] = 0;     /* Make it outer parens */

         klen = RK.length + 1;
         kcard = (enum CARDS)smatch(RK_NMERR, RK.last, crkcds,
            sizeof(crkcds)/sizeof(char *));
         switch (kcard) {

         case 0:
            /* Got a card that is not a ROCKS card.  It must be
            *  an application card.  Just pass keyword to the
            *  caller.  If terminated by a colon, eat the colon
            *  and change the machine state to return a colon
            *  next time, otherwise enter NextField state.
            *  Also, even though it is not technically legal,
            *  we accept and handle a semicolon comment here.  */
            if (RK.scancode & RK_SEMICOLON)
               cryout(RK_P1, " {{;}=\"", RK_LN1+7, key, klen,
                  "\"}", RK_FLUSH+2, NULL);
            else {
               kstate = (RK.scancode & RK_COLON) ?
                  GotColon : NextField;
               cryout(RK_P1, " {>", RK_LN1+3, key, klen,
                  "}", RK_FLUSH+1, NULL);
               }
            continue;

         case Title:             /* Found a TITLE card */
            cryout(RK_P1, " {$TITLE}", RK_FLUSH+RK_LN1+9, NULL);
            kstate = TitleText;
            continue;

         case Error:             /* Found an ERROR DUMP card */
            /* This is like the others except set the dump
            *  request bit in CrkParse itself as well.  */
            RKC.kparse |= RKKP_EDMPRQ;
            /* ... Fall through to handle like other cards */

         case OutPrty:           /* Found an OUTPRTY card */
         case Spout:             /* Found a SPOUT card */
         case Prompt:            /* Found a PROMPT card */
            /* There is nothing on any of these that CrkParse
            *  needs to deal with, so just feed to the caller
            *  with any data fields.  */
            cryout(RK_P1, " {$", RK_LN1+3, key, klen,
               "}", RK_FLUSH+1, NULL);
            kstate = NextField;
            continue;

         case Execute:           /* Found an EXECUTE card */
            cryout(RK_P1, " {$EXECUTE}", RK_LN1+RK_FLUSH+11, NULL);
            kstate = ExecPart1;
            exfnm[0] = '\0';
            continue;

         case Defaults:          /* Found a DEFAULTS card */
            cryout(RK_P1, " {$DEFAULTS}", RK_LN1+RK_FLUSH+12, NULL);
            kstate = DfltPart1;
            exfnm[0] = '\0';
            continue;

         case End:
            goto NoMoreCards;

            } /* End kcard switch */
         continue;

      /* Got a title card, now send the text of the title */
      case TitleText:
         /* Code in cryin() deals with replacing symbols via
         *  a private routine, so not a problem here.
         *  N.B.:  Max length of title (=60) is hard coded
         *  in cryout, so same here.  */
         cryout(RK_P1, " {#1=\"", RK_LN1+6, RK.last+6, 60,
            "\"}", RK_FLUSH+2, NULL);
         kstate = ReadNew;
         continue;

/*---------------------------------------------------------------------*
*             Process a data field on an application card              *
*---------------------------------------------------------------------*/

      case NextField:
         if (dovs) RKC.kparse &= ~RKKP_NOVS;
         scan(key, 0);
         RKC.kparse |= RKKP_NOVS;
         dlev = RK.plevel - jlev;
         /* If paren level went down, push field back, issue
         *  code, wait for next prompt */
         if (dlev < 0) {
            --jlev;
            scanagn();
            cryout(RK_P1, " {)}", RK_LN1+RK_FLUSH+4, NULL);
            continue;
            }
         /* If paren level went up, increase count at current
         *  level by one and zero count at new level, issue
         *  code, wait for next prompt */
         if (dlev > 0) {
            if (RK.plevel >= MXPLEVEL)
               abexitm(167, "CrkParse max paren level exceeded");
            ++npos[jlev]; npos[++jlev] = 0;
            scanagn();
            cryout(RK_P1, " {(}", RK_LN1+RK_FLUSH+4, NULL);
            continue;
            }
         if (RK.scancode & RK_ENDCARD) {
            kstate = ReadNew;
            goto ReadNoPrompt; }
         klen = RK.length + 1;         /* Save key length */
         if (RK.scancode & RK_SEMICOLON) {
            /* Got a semicolon comment.  Do not count in npos.  */
            cryout(RK_P1, " {{;}=\"", RK_LN1+7, key, klen,
            "\"}", RK_FLUSH+2, NULL);
            continue;
            }
         ++npos[jlev];
         chkeq = RK.scancode & (RK_EQUALS|RK_RTPAREN);
         if (chkeq == RK_EQUALS) {
            /* Got a keyword=value field */
            if (dovs) RKC.kparse &= ~RKKP_NOVS;
            scan(lea, RK_REQFLD);      /* Get value field */
            RKC.kparse |= RKKP_NOVS;
            /* If entered parens, push field back, send code
            *  to user, update jlev so next prompt will
            *  behave normally in the new paren level.  */
            if (RK.plevel > jlev) {
               if (RK.plevel >= MXPLEVEL)
                  abexitm(167, "CrkParse max paren level exceeded");
               npos[++jlev] = 0;
               scanagn();
               cryout(RK_P1, " {%", RK_LN1+3, key, klen,
                  "=(}", RK_FLUSH+3, NULL);
               }
            else {
               cryout(RK_P1, " {%", RK_LN1+3, key, klen, "=\"", 2,
                  lea, RK.length+1, "\"}", RK_FLUSH+2, NULL);
               }
            }
         else if (goteq) {
            /* Previous field was ended by ")=" */
            cryout(RK_P1, " {=\"", RK_LN1+4, key, klen,
               "\"}", RK_FLUSH+2, NULL);
            goteq = FALSE;
            }
         else {
            /* Got a positional field--termination by left or
            *  right paren is of no concern until next field.  */
            ibcdwt(RK_LFTJ+RK_IORF+MXPWIDTH-1, apos, npos[jlev]);
            cryout(RK_P1, " {#", RK_LN1+3, apos, RK.length + 1,
               "=\"", 2, key, klen, "\"}", RK_FLUSH+2, NULL);
            }
         if (chkeq == (RK_EQUALS|RK_RTPAREN)) goteq = TRUE;
         continue;

      case GotColon:
         cryout(RK_P1, " {:}", RK_LN1+RK_FLUSH+4, NULL);
         kstate = NextField;
         continue;

/*---------------------------------------------------------------------*
*                  Handle EXECUTE and DEFAULTS cards                   *
*                                                                      *
* (Most of this code is copied, mutatis mutandis, directly from cryin) *
*---------------------------------------------------------------------*/

      case ExecPart1:
      case DfltPart1:
         if (scan(key, RK_NEWKEY) & RK_ENDCARD)
            goto EndExecDflt1;
         klen = RK.length + 1;         /* Save key length */

         /* Check for NOREUSE and set flag if found */
         if (ssmatch(key, "NOREUSE", 5)) {
            noreu = YES;
            ibcdwt(RK_LFTJ+RK_IORF+MXPWIDTH-1, apos, npos[jlev]);
            cryout(RK_P1, " {#", RK_LN1+3, apos, RK.length + 1,
               "=\"NOREUSE\"}", RK_FLUSH+11, NULL);
            continue; }

         /* Check for NOREWIND and set flag if found */
         if (ssmatch(key, "NOREWIND", 4)) {
            norew = TRUE;
            ibcdwt(RK_LFTJ+RK_IORF+MXPWIDTH-1, apos, npos[jlev]);
            cryout(RK_P1, " {#", RK_LN1+3, apos, RK.length + 1,
               "=\"NOREWIND\"}", RK_FLUSH+12, NULL);
            continue; }

         /* All other options are keyword=value forms--
         *  check now for punctuation error if no equals sign. */
         if (!(RK.scancode & RK_EQUALS)) {
            cryout(RK_P1, " ***BAD PUNCTUATION: EQUALS EXPECTED.",
               RK_LN1+RK_FLUSH, NULL);
            continue; }

         /* Check for FILE='name' option and store file name.
         *  N.B. cdunit() cannot be called at this time, because we
         *  must read any continuation cards first from the old unit.
         */
         if (ssmatch(key, "FILE", 4)) {
            int oldlen = scanlen(LFILNM);
            if (scan(exfnm, RK_REQFLD|RK_SCANFNM) & ~RK_COMMA)
               cryout(RK_P1, " ***BAD PUNCTUATION: COMMA EXPECTED.",
                  RK_LN1+RK_FLUSH, NULL);
            scanlen(oldlen);
            cryout(RK_P1, " {%FILE=\"", RK_LN1+9, exfnm, RK.length+1,
               "\"}", RK_FLUSH+2, NULL);
            if (kstate != ExecPart1)
               cryout(RK_P1, " ***FILE OPTION NOT ALLOWED HERE.",
                  RK_LN1+RK_FLUSH, NULL);
            continue; }

         /* Check for MAXSYM.  This is now an obsolete option--
         *  just skip over it and its value.  */
         if (ssmatch(key, "MAXSYM", 4)) {
            if (scan(NULL, RK_REQFLD) & ~RK_COMMA)
               cryout(RK_P1, " ***BAD PUNCTUATION: COMMA EXPECTED.",
                  RK_LN1+RK_FLUSH, NULL);
            goto ReadNoPrompt; }

/* Found a keyword that is not FILE, NOREUSE, NOREWIND, or MAXSYM--it
*  must be the first variable symbol.  Push it back for scanning later
*  and go set up RKCDU to receive symbol definitions.  */

         scanagn();              /* Push the field back */

EndExecDflt1:
         pncu = RKC.cduptr;            /* Default: use current file */
         if (kstate == ExecPart1) {    /* EXECUTE */
            if (exfnm[0]) {            /* Handle named file */
               if (RKC.kparse & RKKP_DOEX)
                  pncu = cry_mkiu(exfnm);
               }
            else {                     /* Handle current file */
               if (pncu->nvsa > 0) {   /* Free previous symbol table */
                  free(pncu->pfvs);
                  pncu->pfvs = NULL;
                  pncu->nvs = pncu->nvsa = pncu->nvse = 0; }
               } /* End setup for adding symbols to current file */
            if (norew) pncu->rfd.norew = (byte)NO_REWIND;
            if (noreu) pncu->accpt |= ACCPT_NOR;
            kstate = ExecPart2;
            }
         else {                        /* DEFAULTS */
            kstate = DfltPart2;
            }
         /* Fall through to Part 2 ... */

/* Pick up variable symbols and store values in the symbol table--
*  but only if there is not already a value there from the EXECUTE
*  card.  But if there are multiple definitions of the same symbol
*  on the same card, the LAST is used, consistent with normal ROCKS
*  practice.  */

      case ExecPart2:
      case DfltPart2:
         /* Get the key, check punctuation, save length */
         if (scan(key, RK_NEWKEY) & RK_ENDCARD)
            goto EndExecDflt2;
         if (key[0] == '&')
            pvsn = key + 1, klen = RK.length;
         else
            pvsn = key, klen = RK.length + 1;
         if (RK.scancode != RK_EQUALS) {
            cryout(RK_P1, " ***BAD PUNCTUATION: EQUALS EXPECTED.",
               RK_LN1+RK_FLUSH, NULL);
            continue; }

         /* Get the value, check punctuation, save length */
         scan(lea, RK_REQFLD);
         vlen = RK.length + 1;
         if (RK.scancode & ~RK_COMMA)
            cryout(RK_P1, " ***BAD PUNCTUATION: COMMA EXPECTED.",
               RK_LN1+RK_FLUSH, NULL);

         /* Omit saving symbol if not in dovs mode */
         if (!dovs) goto JustReportSymbolValue;

         /* See if the symbol already exists in the table.
         *  This could be replaced by a hash lookup some day.  */
         pvz = pncu->pfvs + pncu->nvs;   /* Table end fence */
         for (pvs=pncu->pfvs; pvs<pvz; pvs++)
            if (ssmatch(pvsn, pvs->vname, L_SYMBOL))
               goto ReplaceOldValue;
         /* No match, add new entry to table */
         if (pncu->nvs >= pncu->nvsa) {  /* Table size exceeded */
            if (pncu->nvsa == 0) {  /* Make initial allocation */
               pncu->pfvs = (struct RKVS *)malloc(
                  (pncu->nvsa = INIT_NVSA)*sizeof(struct RKVS));
               }
            else {               /* Expand existing table */
               pncu->pfvs = (struct RKVS *)realloc(pncu->pfvs,
                  (pncu->nvsa += pncu->nvsa)*sizeof(struct RKVS));
               }
            if (!pncu->pfvs) abexit(63);
            } /* End table allocation */
         pvs = pncu->pfvs + pncu->nvs++;
         /* Copy symbol name to table */
         strncpy(pvs->vname, pvsn, L_SYMBOL);

         /* Copy value to table unless this is DEFAULTS
         *  card and value was placed in table by EXECUTE card.  */
ReplaceOldValue:
         if (kstate == ExecPart2 || pncu->nvs > pncu->nvse) {
            strncpy(pvs->value, lea, DFLT_MAXLEN); /* Value to table */
            pvs->vsize = vlen;   /* Length to table */
            }
JustReportSymbolValue:
         cryout(RK_P1, " {%", RK_LN1+3, pvsn, klen, "=\"", 2,
            lea, vlen, "\"}", RK_FLUSH+2, NULL);
         continue;

EndExecDflt2:
         if (kstate == ExecPart1 || kstate == ExecPart2) {
            pncu->nvse = pncu->nvs;
            if (RKC.kparse & RKKP_DOEX && exfnm[0])
               cry_opiu(plcu = pncu);     /* Switch units */
            } /* End if ExecPart2 */
         kstate = ReadNew;
         goto ReadNoPrompt;

         } /* End state switch */
      } /* End main while loop switch */

/*---------------------------------------------------------------------*
*                              Finish up                               *
*---------------------------------------------------------------------*/

NoMoreCards:
   cryout(RK_P1, " {$END}", RK_LN1+RK_FLUSH+7, NULL);
   fgets(inbuf, MXINLEN, stdin);
   if (strncmp(inbuf, "{QUIT}\n", MXINLEN))
      abexitm(166, "Did not receive {QUIT} after EOF");
StopNow:
   free(key);
   free(lea);
   exit(0);

   } /* End CrkParse */
