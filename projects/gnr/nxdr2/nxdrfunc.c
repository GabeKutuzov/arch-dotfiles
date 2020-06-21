/* (c) Copyright 1999-2016, The Rockefeller University *11113* */
/***********************************************************************
*                             nxdrfunc.c                               *
*                                                                      *
*          Miscellaneous service functions for nxdr program            *
*                                                                      *
*----------------------------------------------------------------------*
*  V2D, 02/05/16, GNR - Add '-m32' and '-m64' options.                 *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "rksubs.h"
#include "nxdr2.h"
#ifdef UNIX
#include <sys/wait.h>
#endif

/*---------------------------------------------------------------------*
*                             allotype()                               *
*                                                                      *
*  Allocate and initialize a type record.                              *
*                                                                      *
*  Synopsis:  type *allotype(int keys);                                *
*                                                                      *
*  Argument:                                                           *
*     keys     One or more codes to be stored in key field as defined  *
*              in nxdr2.h.                                             *
*                                                                      *
*  Return Value:                                                       *
*     A pointer to an empty type record.                               *
*                                                                      *
*  Errors:                                                             *
*     All errors are fatal, none returned.                             *
*                                                                      *
*  Side effects:                                                       *
*     The new type is added at the end of the headtyprec list.  This   *
*     makes the generation of output from the list easier, as types    *
*     are always preceded in the list by their embedded components.    *
*---------------------------------------------------------------------*/

type *allotype(int keys) {

   type *p;
   int i;

   /* If no free type records are available, make some */
   if (!pavtp) {
      pavtp = p = (type *)callocv(ITEMINCR, sizeof(type),
         "Block of type info records");
      for (i=0; i<(ITEMINCR-1); ++i,++p) {
         p->pntp = p + 1;
         }
      }

   /* Take first available type record */
   p = pavtp, pavtp = p->pntp, p->pntp = NULL;

   p->offset = -1;            /* Indicate not assigned yet */
   p->nel = 1;                /* For the sizeof() entry */
   p->key = (short)keys;      /* Indicate category of object */

   *tailtyprec = p;           /* Link into list */
   tailtyprec = &p->pntp;
   return p;
   } /* End allotype() */

/*---------------------------------------------------------------------*
*                               additem                                *
*                                                                      *
*  Add a data conversion entry to a type record.  If the item is of    *
*  the same type as the last item in the record, the count of that     *
*  item is simply increased.  Otherwise, the new item is added at      *
*  the end of any existing chain of items.  If there are no free item  *
*  records available, allocate another block of ITEMINCR item records  *
*  and use one of them.                                                *
*                                                                      *
*  Synopsis: item *additem(type *ptyp, type *pinc, long count,         *
*                 int itype)                                           *
*                                                                      *
*  Arguments:                                                          *
*     ptyp        A ptr to the type record that is to contain the      *
*                 new item.                                            *
*     pinc        A ptr to the type record for the included type       *
*                 if such there is, otherwise NULL.                    *
*     count       The number of items of this type.                    *
*     itype       Index of the item in the intrtypes list.             *
*                                                                      *
*  Return value:                                                       *
*     A pointer to the added item structure.                           *
*                                                                      *
*  Errors:                                                             *
*     All errors (e.g. unable to allocate memory for a block of        *
*     item records) are fatal, no error conditions are returned.       *
*                                                                      *
*  Side effects:                                                       *
*     Updates length, mxtlen, and nel in the parent type (updating     *
*     of parent length from embedded structures or unions is done      *
*     in WriteTTable).  May result in allocation of a new block of     *
*     item structures at *pavit.                                       *
*                                                                      *
*  Rev, 02/06/16, GNR - Use itype to access code and length            *
*---------------------------------------------------------------------*/

item *additem(type *ptyp, type *pinc, long count, int ityp) {

   item *p;
   int i;
   char tlen;

   /* Consolidate same-type entries */
   if ((p = ptyp->plit) &&    /* Assignment intended */
       p->pinctbl == pinc &&
       (int)p->itype == ityp) {
      p->count += count;
      /* Because pitlens for structures and unions are 0, there
      *  is no need to test ityp for those cases here.  */
      ptyp->length += count*pitlens[ityp];
      return p;
      }

   /* If no free item records are available, make some */
   if (!pavit) {
      pavit = p = (item *)callocv(ITEMINCR, sizeof(item),
         "Block of item conversion records");
      for (i=0; i<(ITEMINCR-1); ++i,++p) {
         p->pnit = p + 1;
         }
      }

   /* Take first available item record */
   p = pavit, pavit = p->pnit;

   p->pnit    = NULL;
   p->pinctbl = pinc;
   p->count   = count;
   p->itype   = ityp;

   /* Link it into the specified parent type.
   *  (A ptr to the last item is used, rather than a ptr to the ptr
   *  to the next item, even though it necessitates an extra 'if'
   *  here, to allow access to the last item for dup checking.)  */
   if (ptyp->plit)
      ptyp->plit->pnit = p;
   else
      ptyp->pfit = p;
   ptyp->plit = p;
   ptyp->nel += (ityp == IT_STRUC || ityp == IT_JUMP) ?
      (++pinc->nref, 2) : 1;

   /* Pick up length and mxtlen of item and update parent */
   if (ityp == IT_JUMP || ityp == IT_STRUC)
      tlen = pinc ? pinc->mxtlen : pitlens[ityp];
   else {
      tlen = pitlens[ityp];
      ptyp->length = keyszup(ptyp,tlen) + count*tlen;
      }
   if (tlen > ptyp->mxtlen) ptyp->mxtlen = tlen;

   return p;
   } /* End additem() */

/*---------------------------------------------------------------------*
*                               addtype                                *
*                                                                      *
*  Add a name to a type record and hash it, unless it is already       *
*  defined.  This prevents user entries from overriding base types     *
*  already in the hash table (specifically, the definitions of si64    *
*  and ui64 as long longs are protected on systems where they are      *
*  implemented as structs).                                            *
*                                                                      *
*  Synopsis:  void addtype(idfr typname, type *ptyp)                   *
*                                                                      *
*  Arguments:                                                          *
*     typname     The name of the new type.                            *
*     ptyp        A pointer to the type record to which the name is    *
*                 being added.                                         *
*                                                                      *
*  Return value:                                                       *
*     None                                                             *
*                                                                      *
*  Errors:                                                             *
*     All errors (e.g. unable to allocate memory for a tynrec) are     *
*     fatal, none returned.                                            *
*---------------------------------------------------------------------*/

void addtype(idfr typname, type *ptyp) {

   tynrec *ptyn;

   if (!ptyp)
      abexit(358);      /* Bug trap--no message needed */

   if (tyrlkup(typname))
      return;           /* Exit if type already defined */

   ptyn = (tynrec *)callocv(1, sizeof(tynrec) +
      strlen(typname), "Type Definition Record");
   ptyn->ptype = ptyp;
   ptyp->ptnm  = ptyn;
   strcpy(ptyn->typname, typname);

   hashadd(htynr, ptyn);

   } /* End addtype() */

/*---------------------------------------------------------------------*
*                              savedname                               *
*                                                                      *
*  Save the name of the first declarator of an anonymous struct or     *
*  union in a temporary tynrec block, prefixed with a '#' to indicate  *
*  that it is a partial name.  This name will be used later to build   *
*  a full name for the anonymous object based on the concatenated      *
*  names of all its parents with the declarator name.  Create a        *
*  tynrec to hold the name and insert a pointer to it in the type      *
*  record defining the type, but do not hash the name.                 *
*                                                                      *
*  Synopsis:  void savedname(type *ptp, idfr dname)                    *
*                                                                      *
*  Arguments:                                                          *
*     ptp         A pointer to the type structure defining the         *
*                 anonymous type that needs to be named.               *
*     dname       The name of an item that is an instance of the type. *
*                                                                      *
*  Return value:                                                       *
*     None                                                             *
*                                                                      *
*  Errors:                                                             *
*     All errors (e.g. unable to allocate memory for a tynrec) are     *
*     fatal, none returned.                                            *
*---------------------------------------------------------------------*/

void savedname(type *ptp, idfr dname) {

   tynrec *ptyn;
   if (ptp->key & Tk_BASE) return;
   ptyn = (tynrec *)callocv(1, sizeof(tynrec) + strlen(dname) + 1,
      "Anonymous Type Name Record");
   ptyn->typname[0] = '#';
   strcpy(ptyn->typname+1, dname);
   ptyn->ptype = ptp;
   ptp->ptnm = ptyn;

   } /* End savedname() */

/*---------------------------------------------------------------------*
*                              freeitem                                *
*                                                                      *
*  Return an unused item to the pool of free item records.             *
*                                                                      *
*  N.B.  Currently, the only time an item record is released is after  *
*  the program is finished scanning the source file, so the released   *
*  records never get reused.  However, that might change someday.      *
*  Do not be tempted to call free()--item records are allocated in     *
*  arrays and cannot be freed individually.                            *
*---------------------------------------------------------------------*/

void freeitem(item *pit) {

   memset((char *)pit, 0, sizeof(item));
   pit->pnit = pavit;
   pavit = pit;

   } /* End freeitem() */

/*---------------------------------------------------------------------*
*                              freetype                                *
*                                                                      *
*  Return an unused type record to the pool of free type records.      *
*  Remove the associated tynrec if there is one and delete the name    *
*  from the hash table.  There is no use keeping a pool of free        *
*  tynrecs, as the name lengths differ.                                *
*                                                                      *
*  N.B.  Currently, the only time a type record is released is after   *
*  the program is finished scanning the source file, so the released   *
*  records never get reused.  However, that might change someday.      *
*  Do not be tempted to call free()--type records are allocated in     *
*  arrays and cannot be freed individually.                            *
*---------------------------------------------------------------------*/

void freetype(type *ptp) {

   tynrec *ptyn;
   if (ptp->key & Tk_BASE) return;
   if (ptyn = ptp->ptnm) {       /* Assignment intended */
      if (ptyn->typname[0] != '#') hashdel(htynr, ptyn);
      free(ptyn);
      }
   memset((char *)ptp, 0, sizeof(type));
   ptp->pntp = pavtp;
   pavtp = ptp;

   } /* End freetype() */

/*---------------------------------------------------------------------*
*                               nexttkn                                *
*                                                                      *
*  Return the next token from the globally defined input file and      *
*  a code to indicate the type of the token.  If EOF is reached, the   *
*  token '#', which is not a C token after the cpp pass, is returned.  *
*                                                                      *
*  If the length of the token exceeds the current high-water-mark      *
*  token length, the token string is expanded, so the caller never     *
*  has to worry about tokens that are too long.                        *
*                                                                      *
*  Rev, 10/01/96, GNR - Fully rewrite for correct handling of hexa-    *
*     decimal and floating point constants (although it still cannot   *
*     detect incorrectly formed constants).                            *
*  V2A, 09/25/99, GNR - Return token and ttype in global variables.    *
*  Rev, 01/29/00, GNR - Expand token string if input is too long,      *
*     retain seek ptrs to last NCONTEXT lines for use in error msgs.   *
*---------------------------------------------------------------------*/

/* Macro to terminate current token, set class, and return */
#if 0         /* DEBUG version */
#define endtkn(class) \
   {  token[ltoken] = '\0'; \
      printf("Token >>%s<< type %d\n", token, class); \
      fflush(stdout); ttype = class; return; }
#else
#define endtkn(class) \
   {  token[ltoken] = '\0'; ttype = class; return; }
#endif

/* Function to check length, then move one character to token */
static void mvtkn(int rc) {
   if (ltoken >= mxltoken)
      token = reallocv(token, (mxltoken=ltoken+IDFRINCR)+1, "Token");
   token[ltoken++] = (char)rc;
   } /* End mvtkn() */

/* Function to get the next character and save newline locations */
static int getcsnl(void) {
   int i,rc = getc(pif);
   if (ferror(pif))
      abexitme(350, "Error reading preprocessor output file");
   if (rc == '\n') {
      for (i=0; i<(NCONTEXT-1); ++i) siln[i] = siln[i+1];
      siln[NCONTEXT-1] = ftell(pif);
      }
   return rc;
   }

/* Macro to get the next character and its uppercase equivalent */
#define getnc(rc, ucc) \
   (rc = getcsnl(), ucc = toupper(rc))

void nexttkn(void) {

   int rc;
   int kfloat = 0;   /* True if floating point number */
   char ucc;

   /* Get first character of new token, skipping any whitespace */
   do {
      rc = getcsnl();
      } while (rc != EOF && isspace(rc));
   ucc = toupper(rc);
   ltoken = 0;

   /* Handle end-of-file */
   if (rc == EOF) {
      mvtkn('#');
      endtkn(TKN_EOF);
      }

   /* Handle plus or minus sign not followed by a digit.  */
   if (ucc=='-' || ucc=='+') {
      mvtkn(rc);
      getnc(rc, ucc);
      if (isdigit(rc) || ucc=='.') goto StartConst;
      ungetc(rc, pif);
      endtkn(TKN_CHR);
      }

   /* Handle numbers, octal, hex or decimal, positive or negative.
   *  Numbers may contain decimal points or exponents. */
   if (isdigit(rc) || ucc=='.') {
StartConst:    /* Enter here when number has a leading sign */
      if (ucc == '0') {
         mvtkn(rc);
         getnc(rc, ucc);
         if (ucc == 'X') {
            /* Got a hexadecimal constant */
            do {
               mvtkn(rc);
               getnc(rc, ucc); }
               while (isxdigit(rc));
            if (ucc=='U' || ucc=='L') {
               mvtkn(rc); }
            else
               ungetc(rc, pif);
            endtkn(TKN_NUM);
            } /* End hex number */
         } /* End leading zero */
      /* Handle digits before a decimal point */
      while (isdigit(rc)) {
         mvtkn(rc);
         getnc(rc, ucc);
         }
      /* Handle decimal point and following fraction */
      if (ucc == '.') {
         kfloat = TRUE;
         do {
            mvtkn(rc);
            getnc(rc, ucc); }
            while (isdigit(rc)) ;
         }
      /* Handle suffixes allowed with fixed-point numbers */
      if (ucc=='U' || ucc=='L') {
         mvtkn(rc);
         endtkn(TKN_NUM);
         }
      /* Handle exponents */
      if (ucc == 'E') {
         kfloat = TRUE;
         mvtkn(rc);
         getnc(rc, ucc);
         if (ucc=='-' || ucc=='+') {
            mvtkn(rc);
            getnc(rc, ucc);
            }
         while (isdigit(rc)) {
            mvtkn(rc);
            getnc(rc, ucc); }
         } /* End exponent */
      /* Handle suffixes allowed with floating point numbers */
      if (kfloat && (ucc=='F' || ucc=='L')) {
         mvtkn(rc);
         endtkn(TKN_NUM); }
      /* Got something that's not part of this number token--
      *  push it back and terminate */
      ungetc(rc, pif);
      endtkn(TKN_NUM);
      } /* End numeric constant */

   /* Handle identifiers */
   if (isalpha(rc) || ucc=='_') {
      do {
         mvtkn(rc);
         getnc(rc, ucc); }
         while (isalpha(rc) || ucc=='_' || isdigit(rc));
      ungetc(rc, pif);

      /* Stop for debugging if token matches dbgtoken.  (tt is
      *  arg to sleep just to kill unrefd variable warning.) */
      if (dbgtoken) {
         token[ltoken] = '\0';
         if (strcmp(token, dbgtoken) == 0) {
            volatile int tt = 1;
            while (tt)
               sleep(tt);
            }
         } /* End dbgtoken wait */

      endtkn(TKN_IDF);
      } /* End identifier */

   /* All tokens other than a word (keyword or identifier) or
   *  number are one character long and may be returned now. */
   mvtkn(rc);
   endtkn(TKN_CHR);

   } /* End nexttkn() */

/*---------------------------------------------------------------------*
*                              findmatch                               *
*                                                                      *
*  This function finds the end of a block enclosed within a pair of    *
*  curly, round, or square brackets.  It is called after the opening   *
*  bracket has been read in and returns with the input positioned at   *
*  the next token after the closing token.  Like all other functions   *
*  in nxdr2, it is designed for a C file that has been preprocessed    *
*  by cpp and therefore contains no comments.                          *
*                                                                      *
*  Terminates execution with an error if the match is not found.       *
*  Saves position at start of search in order to give relevant         *
*  context when this happens.                                          *
*---------------------------------------------------------------------*/

void findmatch(int kbra) {

   long svsiln = siln[0];
   int instring = FALSE;   /* TRUE if scanning in a quoted string */
   int esc = FALSE;        /* TRUE if last char was a backslash */
   /* Order of brackets in lbra,rbra must match FM_xxx defs */
   static char *lbra[] = { "{", "(", "[" };
   static char *rbra[] = { "}", ")", "]" };

GetAnotherToken:
   nexttkn();
   while (1) {
      if (ttype == TKN_EOF) {
         siln[0] = svsiln;
         show_context();
         abexitm(343, ssprintf(NULL, "Reached end-of-file "
            "while scanning for match for %s", lbra[kbra]));
         }
      if (ttype != TKN_CHR) {
         esc = FALSE;
         goto GetAnotherToken; }
      switch (token[0]) {

      case '\"':
         if (!esc)
            instring = !instring;
         esc = FALSE;
         goto GetAnotherToken;

      case '\\':
         if (instring)
            esc = TRUE;
         goto GetAnotherToken;

      case '(':
         esc = FALSE;
         if (instring)
            goto GetAnotherToken;
         findmatch(FM_ROUND);
         continue;

      case '[':
         esc = FALSE;
         if (instring)
            goto GetAnotherToken;
         findmatch(FM_SQUARE);
         continue;

      case '{':
         esc = FALSE;
         if (instring)
            goto GetAnotherToken;
         findmatch(FM_CURLY);
         continue;

      case ')':
      case ']':
      case '}':
         esc = FALSE;
         if (instring)
            goto GetAnotherToken;
         if (token_is(rbra[kbra])) {
            nexttkn(); return; }
         else {
            siln[0] = svsiln;
            show_context();
            abexitm(343, ssprintf(NULL, "Reached a non-"
               "matching %s while scanning for match for %s",
               token, lbra[kbra]));
            }
      default:
         goto GetAnotherToken;
         } /* End switch */
      } /* End while */
   } /* End findmatch() */

/*---------------------------------------------------------------------*
*                              ignqualif                               *
*                                                                      *
*  Ignore qualifiers (const, volatile, auto, register, static, extern) *
*  that do not change the type or length of a declaration.  For the    *
*  purposes of this program, it is not necessary to be too picky about *
*  just where these qualifiers may or may not appear.                  *
*---------------------------------------------------------------------*/

void ignqualif(void) {

   int i;

#define NQUALS  6
   static char *quals[] = { "const", "volatile", "auto",
      "register", "static", "extern" };

CheckAnotherToken:
   if (ttype == TKN_IDF) for (i=0; i<NQUALS; i++) {
      if (token_is(quals[i])) {
         nexttkn();
         goto CheckAnotherToken;
         }
      }

   } /* End ignqualif() */

/*---------------------------------------------------------------------*
*                              skipstmt                                *
*                                                                      *
*  Skip to end of current compound statement:  if at a left curly      *
*  brace, skip everything up to the matching right curly brace, then   *
*  skip to next semicolon and then to the next token after that.       *
*---------------------------------------------------------------------*/

void skipstmt(void) {

   if (token_is("{"))
      findmatch(FM_CURLY);
   while (token_is_not(";"))
      nexttkn();
   nexttkn();

   } /* End skipstmt() */

/*---------------------------------------------------------------------*
*                             killstdout                               *
*                                                                      *
*  Redirects stdout to a null file for use when forking a compiler.    *
*---------------------------------------------------------------------*/

void killstdout(void) {
#ifdef _ISOC99_SOURCE
   int dummy = open(qKSONAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
#else
   int dummy = open(qKSONAME, O_CREAT | O_RDWR, S_IREAD | S_IWRITE);
#endif
   if (!dummy)
      abexitm(352, "Unable to open temp file for redirecting output");
   oldstdout = dup(1);
   if (oldstdout < 0)
      abexitm(352, "Unable to dup stdout--trying to redirect output");
   if (dup2(dummy, 1) < 0)
      abexitm(352, "Unable to redirect stdout to a dummy file");
   close(dummy);
   }

/*---------------------------------------------------------------------*
*                            regainstdout                              *
*                                                                      *
*  Regains normal usage of stdout after a killstdout() call.           *
*---------------------------------------------------------------------*/

void regainstdout(void) {
   if (dup2(oldstdout, 1) < 0)
      abexitm(352, "Unable to restore stdout after redirection");
   close(oldstdout);
   }

/*---------------------------------------------------------------------*
*                               getdim                                 *
*                                                                      *
*  This function is used by Scan_decls() to get the size of an array.  *
*  If the dimension expression consists of a single token (which must  *
*  be a number), its value is obtained directly, according to its      *
*  format (decimal, hex or octal).  Otherwise, the expression must be  *
*  evaluated according to C's rules.  For that purpose a small file    *
*  is written containing a C program to evaluate the expression and    *
*  that program is compiled and executed and the result retrieved.     *
*                                                                      *
*  The dname argument is used solely to construct more meaningful      *
*  error messages when something goes wrong.                           *
*---------------------------------------------------------------------*/

long getdim(idfr dname) {

   long mult = 1;          /* Dimension */
   int  tttype;            /* Type of look-ahead token */
   int  lnum;              /* Length of a number */

   while (token_is("[")) {

      nexttkn();

/* If got an immediate "]", the dimension is incomplete.
*  This case is inappropriate for our purposes.  */

      if (token_is("]")) {
         show_context();
         abexitm(346, ssprintf(NULL, "The identifier %s "
            "has an incomplete dimension", dname));
         }

/* Save the current token while we look ahead to see if it
*  is a single number.  (At this point, any defined constants
*  should already have been replaced by the preprocessor.)  */

      hwmidfr(cname, lcname, ltoken, "Array dimension");
      strcpy(cname, token);
      tttype = ttype;
      nexttkn();

/* If dimension is a numerical constant, evaluate it directly */

      if (token_is("]")) {
         if (tttype != TKN_NUM) {
            show_context();
            abexitm(346, ssprintf(NULL,
               "The identifier %s has a dimension that is not "
               "a constant expression", dname));
            }
         lnum = strlen(cname);
         if (lnum > 32) {
            show_context();
            abexitm(339, ssprintf(NULL,
               "The identifier %s has a dimension that is too "
               "long to process (>32 chars)", dname));
            }
         if (cname[0] == '0') {
            if (cname[1]=='x' || cname[1]=='X')
               mult *= sibcdin(RK_HEXF+RK_SSCN+RK_QPOS+RK_CTST+
                  RK_ZTST-1+lnum, cname+2);
            else
               mult *= sibcdin(RK_OCTF+RK_SSCN+RK_QPOS+RK_CTST+
                  RK_ZTST-1+lnum, cname+1);
            }
         else {
               mult *= sibcdin(RK_IORF+RK_SSCN+RK_QPOS+RK_CTST+
                  RK_ZTST-1+lnum, cname);
            }
         }

/* Otherwise, write, compile, and run a C program to evaluate it */

      else {

         FILE *pfc;     /* Temp .c file */
         FILE *pft;     /* Temp results file */
         static char tcfn[]  = "$nxdr$.c";
         static char resfn[] = "$nxdr$.t";

         static char xeqfn[] = "$nxdr$";
         char *nargv[5];
         int cpid;      /* Pid of child process */
         int tmpi;      /* Temp child status */

         /* Create a little C program to evaluate dimension */
         nargv[0] = ccmd;
         nargv[1] = tcfn;
         nargv[2] = "-o";
         nargv[3] = xeqfn;
         nargv[4] = NULL;

         if ((pfc=fopen(tcfn, "w")) == NULL) {
            show_context();
            abexitme(347, ssprintf(NULL, "Unable to create temporary "
               "file %s needed to evaluate dimension of array %s",
               tcfn, dname));
            }
         fputs("#include <stdio.h>\n", pfc);
         fputs("main(){\n", pfc);
         fputs("FILE *pft=fopen(\"", pfc);
            fputs(resfn, pfc);
            fputs("\",\"w\");\n", pfc);
         fputs("long d=", pfc);
            fputs(cname, pfc);
            while (token_is_not("]")) {
               if (token_is(";")) {
                  show_context();
                  abexitm(346, ssprintf(NULL, "The identifier %s "
                     "has an incomplete dimension", dname));
                  }
               fputs(token, pfc);
               nexttkn();
               }
            fputs(";\n", pfc);
         fputs("fprintf(pft,\"%d \",d);\n", pfc);
         fputs("fclose(pft);}\n", pfc);
         fclose(pfc);

         killstdout();

         /* Compile it */
         switch (cpid = fork()) {   /* Assignment intended */
         case -1:
            regainstdout();
            show_context();
            abexitm(347, ssprintf(NULL, "Unable to fork to run C "
               "compiler to evaluate dimension of array %s", dname));
         case 0:
            execvp(ccmd, nargv);
            regainstdout();
            show_context();
            abexitm(347, ssprintf(NULL, "Unable to compile temporary "
               "file %s needed to evaluate dimension of array %s",
               tcfn, dname));
         default:
            waitpid(cpid, &tmpi, 0);
            } /* End switch */

         /* Run it */
         switch (cpid = fork()) {   /* Assignment intended */
         case -1:
            regainstdout();
            show_context();
            abexitm(347, ssprintf(NULL, "Unable to fork to run "
               "temporary program %s needed to evaluate dimension "
               "of array %s", xeqfn, dname));
         case 0:
            execl(xeqfn, xeqfn, NULL);
            regainstdout();
            show_context();
            abexitm(347, ssprintf(NULL, "Unable to execute temporary "
               "program %s needed to evaluate dimension of array %s",
               xeqfn, dname));
         default:
            waitpid(cpid, &tmpi, 0);
            } /* End switch */

         regainstdout();

         /* Read back the result */
         if ((pft=fopen(resfn, "r")) == NULL) {
            show_context();
            abexitme(347, ssprintf(NULL, "Unable to open result "
               "file %s created to evaluate dimension of array %s",
               resfn, dname));
            }
         if (fgets(cname, lcname, pft) == NULL) {
            show_context();
            abexitme(347, ssprintf(NULL, "Unable to read dimension "
               "of array %s from temporary file %s", dname, resfn));
            }
         fclose(pft);

         lnum = strlen(cname);
         if (lnum > 32) {
            show_context();
            abexitm(339, ssprintf(NULL,
               "The identifier %s has a dimension that is too "
               "long to process (>32 chars)", dname));
            }
         mult *= sibcdin(RK_IORF+RK_SSCN+RK_QPOS+RK_CTST+
            RK_ZTST-1+lnum, cname);

         /* Clean up */
         unlink(tcfn);
         unlink(xeqfn);
         unlink(resfn);

         } /* End evaluation of dimension expression */

      nexttkn();
      } /* End loop over dimensions */

   return mult;
   } /* End getdim() */

/*---------------------------------------------------------------------*
*                              needname                                *
*                                                                      *
*  Determine whether a table needs to be constructed for the current   *
*  enum, struct, or union.  If the object has a definition and a name, *
*  and the name is already defined, then there is an error.  If the    *
*  name has already been defined, but there is not a new definition,   *
*  then return the old definition.  If there is a name with a new      *
*  definition, a table is created and added to the type chain and      *
*  its address is returned to the caller.  If there is no definition,  *
*  the Tk_INC bit is set.  If the object has no name and it is a       *
*  toplevel object, it is skipped and a NULL pointer is returned.      *
*  If it is embedded in a struct, union, or typedef, a new table is    *
*  created and its address is returned.                                *
*---------------------------------------------------------------------*/

type *needname(int level) {

   type *ptp;

   /* Save the first three chars of the type head ("enum", "struct",
   *  or "union") token in case needed later to build the object
   *  name, then swallow the type head.  */
   hwmidfr(cname, lcname, ltoken+4, "Object name");
   memcpy(cname, token, 3);
   nexttkn();

   /* If the object has a name, see if it is already defined */
   if (token_is("{")) {
      cname[0] = '\0';
      ptp = NULL; }
   else {
      /* There is a name */
      cname[3] = '_';
      strcpy(cname+4, token);
      nexttkn();
      {  tynrec *pon = (tynrec *)tyrlkup(cname);
         ptp = pon ? pon->ptype : NULL;
         } /* End local scope */
      }

   /* Distinguish the various cases as described above */
   if (ptp) {                 /* Previously encountered name */
      if (token_is("{")) {       /* But here's a definition */
         if (ptp->key & (Tk_DEF|Tk_IBD)) {
            show_context();
            abexitm(351, ssprintf(NULL, "The name %s is defined "
               "more than once", cname));
            }
         nexttkn();
         }
      else {                     /* Name is not defined here */
         if (level == TOPLEVEL) {   /* Must be a prototype */
            ptp = NULL;             /* or a global variable */
            skipstmt(); }
         else
            ptp->key |= Tk_INC;  /* So don't scan definition */
         }
      }
   else if (cname[0]) {       /* Named, not previously seen */
      ptp = allotype(keylkup(cname) ? Tk_KEY : 0);
      addtype(cname, ptp);
      if (token_is("{"))         /* There is a definition here */
         nexttkn();              /* Swallow the left bracket */
      else                       /* It's an incomplete type */
         ptp->key |= Tk_INC;     /* Skip parsing the definition */
      }
   /* Object is anonymous */
   else if (level == TOPLEVEL) {
      ptp = NULL;
      skipstmt();
      }
   else {
      if (token_is_not("{")) {
         show_context();
         abexitm(342, "The embedded anonymous structure does "
            "not have a type definition in curly braces");
         }
      nexttkn();              /* Swallow the left bracket */
      ptp = allotype(0);
      }

   return ptp;
   } /* End needname() */

/*---------------------------------------------------------------------*
*                            show_context                              *
*                                                                      *
*  Called when an error is found in the input file to show several     *
*  lines of context in the neighborhood of the error.                  *
*---------------------------------------------------------------------*/

void show_context(void) {

   int inl,il = 0;
   char incbuf[CDSIZE+1];

   /* Seek back the appropriate number of newlines */
   if (fseek(pif, siln[0], SEEK_SET))
      abexitme(349, "Unable to reposition preprocessor output file");

   /* Copy NCONTEXT lines to the output, breaking them up into
   *  segment no longer than CDSIZE in length if necessary.  */
   cryout(RK_P1, "0***At following context in input C program:",
      RK_LN2, NULL);
   while (il < NCONTEXT) {
      fgets(incbuf, CDSIZE+1, pif);
      if (ferror(pif))
         abexitme(350, "Error reading preprocessor output file");
      if (feof(pif))
         break;
      inl = strnlen(incbuf, CDSIZE) - 1;
      if (incbuf[inl] == '\n') {
         incbuf[inl] = '\0';
         il += 1; }
      cryout(RK_P2+1, " ", RK_LN1+1, incbuf, RK_CCL     , NULL);
      }
   } /* End show_context() */
