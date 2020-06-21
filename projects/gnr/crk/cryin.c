/* (c) Copyright 1988-2018, The Rockefeller University *11115* */
/* $Id: cryin.c 68 2018-08-15 19:33:46Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*      CRYIN, CDUNIT, CDPRNT, CDPRT1, GTUNIT, RDAGN, and ACCWAC        *
*                                                                      *
************************************************************************
*  V1A, 12/07/88, GNR - Adapted from my IBM Assembler versions         *
*  Rev, 04/19/89, GNR - Incorporate rkxtra.h                           *
*  Rev, 05/03/89, GNR - Return NULL if card is NULL                    *
*  V1B, 06/22/89, GNR - Remove conversion of input to upper case       *
*  V1C, 04/05/90, GNR - Correct bug reading input of length CDSIZE     *
*  Rev, 11/06/90, GNR - Make cdunit(0) a no-op if at top level         *
*  Rev, 06/25/92, ABP - Cast REWIND to type on assig. to exrsw         *
*  Rev, 08/19/92, GNR - Fix flush call, do it only on final eof        *
*  Rev, 09/30/92, GNR - Clear ACCPT_CLP when new card is read          *
*  Rev. 01/31/94,  LC - Added OL bit to RKC.accpt                      *
*  Rev, 04/24/94, GNR - Call erprnt before new card changes            *
*                       RK.last, accept null lines as comments         *
*  Rev, 08/15/97, GNR - Add NOPG option on OUTPRTY card, add accwac    *
*  Rev, 09/13/97, GNR - Eliminate need for MXSYM parameter             *
*  Rev, 10/31/97, GNR - Add ACCPT_ACP control to cdprt1                *
*  Rev, 08/20/98, GNR - Read input using rfgets instead of fgets,      *
*                       Add comment and TITLE symbols, NOREUSE, PROMPT *
*  Rev, 02/15/03, GNR - Add accwad                                     *
*  Rev, 02/03/07, GNR - Add CrkParse mode and setcpm routine           *
*  Rev, 03/16/08, GNR - Accept QUIT as a synonym for END               *
*  ==>, 03/16/08, GNR - Last date before committing to svn repository  *
*  Rev, 08/22/09, GNR - Use eqscan() for OUTPRTY for valck, no ibcdin  *
*  Rev, 11/07/09, GNR - Eliminate spoutm, more logical SPOUT controls  *
*  Rev, 06/11/10, GNR - Introduce ACCPT_MKOK and ACCPT_MKNO            *
*  Rev, 04/13/13, GNR - Add cryicls()                                  *
*  Rev, 04/24/14, GNR - Add accmeth ON_LINE switch                     *
*  Rev, 04/22/16, GNR - Add SETENV card and '#' comments               *
*  R65, 09/25/17, GNR - Expand symbols to hold up to L_SYMBVAL chars   *
*  R68, 08/04/18, GNR - Remove SUNOS support                           *
***********************************************************************/

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "sysdef.h"           /* Must come before any ifdefs */
#include "rocks.h"
#include "rkxtra.h"
#include "rockv.h"

#define EXEC   0              /* EXECUTE call to execdflt */
#define DFLT   1              /* DEFAULTS call to execdflt */

/***********************************************************************
*                               ACCWAC                                 *
*                                                                      *
*  Subroutine accwac() is called to make cryin() treat whitespace      *
*     cards as comments.  The call is:                                 *
*                                                                      *
*     void accwac(void)                                                *
***********************************************************************/

void accwac(void) {

   RKC.kparse |= RKKP_WC;
   return;
   } /* End accwac() */

/***********************************************************************
*                               ACCWAD                                 *
*                                                                      *
*  Subroutine accwad() is called to control whether cryin() treats     *
*     subsequent cards beginning with whitespace as data (TRUE) or     *
*     as continuations (FALSE).  The default is FALSE.  The call is:   *
*                                                                      *
*     void accwad(int kaccw)                                           *
***********************************************************************/

void accwad(int kaccw) {

   if (kaccw)
      RKC.kparse |= RKKP_WD;
   else
      RKC.kparse &= ~RKKP_WD;
   return;
   } /* End accwad() */

/***********************************************************************
*                               SETCPM                                 *
*                                                                      *
*  Subroutine setcpm() is called to control whether cryin() and scan() *
*     operate in the special CrkParse mode, in which an input file is  *
*     parsed for use by a GUI, built-in control cards and comments are *
*     treated as data, and variable symbols may or may not be replaced.*
*  It must be called before the first call to cryin() and once set,    *
*     CrkParse mode cannot be turned off.  The call is:                *
*                                                                      *
*     void setcpm(int kparse)                                          *
*                                                                      *
*     where kparse may contain any of the RKKP flags (see rockv.h).    *
***********************************************************************/

void setcpm(int kparse) {

   RKC.kparse |= (byte)kparse;
   RKC.oprty = 1;
   nopage(RK_PGNONE);
   spout(SPTM_GBLOFF);
   return;
   } /* End setcpm() */

/***********************************************************************
*                               RDAGN                                  *
*                                                                      *
*  Subroutine rdagn() is called to make cryin() read the current card  *
*     again when it is next called.  The call is:                      *
*                                                                      *
*     void rdagn(void)                                                 *
***********************************************************************/

void rdagn(void) {

   RKC.accpt |= ACCPT_RR;
   return;
   } /* End rdagn() */

/***********************************************************************
*                          CDPRNT and CDPRT1                           *
*                                                                      *
*  Subroutine cdprnt prints the current control card with double       *
*     spacing and cdprt1 prints it with single spacing.  Both calls    *
*     set flag bits signalling that card has been printed.  ACCPT_CEP  *
*     signifies that the card has "ever" been printed and enables      *
*     automatic printing of subsequent continuation lines.  This bit   *
*     remains set even if other information is printed before the      *
*     continuation is read.  ACCPT_CLP signifies that the card was     *
*     the "last" thing printed and disables printing of the card when  *
*     erprnt is called to print error messages with column marks.      *
*     ACCPT_ACP indicates that the previous item printed was any       *
*     control card.  When this is off, cdprt1 is treated as cdprnt.    *
*     N.B.  The setting of these bits must be conditioned on the       *
*     print priority, because erprnt must know if card was actually    *
*     printed, not just requested to be printed.  Having done the      *
*     test, we might as well save a little time by conditioning the    *
*     cryout call on the result as well.  The calls are:               *
*                                                                      *
*     void cdprnt(char *card)                                          *
*     void cdprt1(char *card)                                          *
*                                                                      *
*     In both cases, 'card' is the card image to be printed.           *
***********************************************************************/

void cdprnt(char *card) {

   if (RKC.oprty >= 3) {
      cryout(RK_P3, "0   ", RK_LN2+4, card, RK_CCL, NULL);
      RKC.accpt |= (ACCPT_CEP|ACCPT_CLP|ACCPT_ACP);
      }
   return;
   } /* End cdprnt() */

void cdprt1(char *card) {

   if (RKC.oprty >= 3) {
      if (RKC.accpt & ACCPT_ACP)
         cryout(RK_P3, "    ", RK_LN1+4, card, RK_CCL, NULL);
      else
         cryout(RK_P3, "0   ", RK_LN2+4, card, RK_CCL, NULL);
      RKC.accpt |= (ACCPT_CEP|ACCPT_CLP|ACCPT_ACP);
      }
   return;
   } /* End cdprt1() */

/*=====================================================================*
*                              cry_mkiu                                *
*                                                                      *
*  Function cry_mkiu() is used internally by cryin() and cdunit()      *
*     and externally by crkparse to make or locate an RKCDU block      *
*     matching a given filename.  It first searches on the reserve     *
*     list for an existing RKCDU block matching the given filename.    *
*     If one is found, it is removed from the reserve list and its     *
*     address is returned.  Otherwise, a new RKCDU block is allocated  *
*     and initialized.  A card buffer of length CDSIZE+2 is allocated. *
*     This makes it possible to detect a line that is too long.        *
*                                                                      *
*     The argument is a pointer to a string containing the name        *
*     of the file.  This string is copied into the RFdef.  A NULL      *
*     argument (not a pointer to a NULL string) is taken to mean       *
*     that input should be read from stdin.  In this case, there       *
*     is no reserve list search, because stdin is always active.       *
*                                                                      *
*     A pointer to the new RKCDU struct is returned.                   *
*                                                                      *
*  Note:  The RKdef block must be the first thing in the RKCDU         *
*     struct in order that the rfd.nf pointer may be cast to a         *
*     pointer to the enclosing RKCDU block.  The active and reserve    *
*     RKCDU lists are single- rather than double-linked because a      *
*     pointer to the parent is always available when an item is        *
*     being unlinked.                                                  *
*=====================================================================*/

struct RKCDU *cry_mkiu(char *fnm) {

   register struct RKCDU *pcu;   /* Ptr to current RKCDU */
   struct RKCDU **ppncu;         /* Ptr to ptr to next RKCDU */

   if (fnm) for (ppncu=&RKC.cdures; *ppncu; ) {
      /* pcu->rfd.fname cannot ever be a NULL pointer */
      if (strcmp(fnm, (pcu = *ppncu)->rfd.fname) == 0) {
         *ppncu = (struct RKCDU *)pcu->rfd.nf;
         return pcu; }
      /* GCC won't accept ppncu = &(struct RKCDU *)pcu->rfd.nf! */
      ppncu = (struct RKCDU **)(&pcu->rfd.nf);
      }
   pcu = (struct RKCDU *)calloc(1, sizeof(struct RKCDU));
   if (!pcu) abexit(62);
   if (fnm) {                    /* Copy file name to RFdef */
      pcu->rfd.fname = (char *)malloc(strlen(fnm)+1);
      if (!pcu->rfd.fname) abexit(62);
      strcpy(pcu->rfd.fname, fnm);
      pcu->rfd.fmt = TEXT;       /* Set input mode */
      }
   else
      pcu->rfd.fmt = SysIn;
   pcu->pc = (char *)malloc(CDSIZE+2); /* Allocate card buffer */
   if (!pcu->pc) abexit(62);
   return pcu;
   } /* End cry_mkiu() */

/*=====================================================================*
*                              cry_opiu                                *
*                                                                      *
*  Subroutine cry_opiu() is used internally by cdunit() and execdflt() *
*     and externally by crkparse to open an RKCDU block if it is not   *
*     already open & push it onto the stack of input files, saving     *
*     accpt status of previous unit.  This results in the new file's   *
*     becoming the current active file.                                *
*                                                                      *
*     The argument is a pointer to the RKCDU to be used.               *
*=====================================================================*/

void cry_opiu(struct RKCDU *pcu) {

   if (!(pcu->rfd.iamopen & IAM_ANYOPEN)) /* Open file if new */
      rfopen(&pcu->rfd, NULL, READ, SAME, SEQUENTIAL, TOP, LOOKAHEAD,
      REWIND, RETAIN_BUFF, IGNORE, IGNORE, IGNORE, ABORT);
   if (RKC.cduptr)                     /* If not first file, */
      RKC.cduptr->accpt = RKC.accpt;   /*    save current status */
   pcu->rfd.nf =                       /* Insert at head of stack */
      (struct RFdef *)RKC.cduptr;
   RKC.cduptr = pcu;
   /* Clear reread, eof, on-line, and noreuse bits */
   RKC.accpt &= ~(ACCPT_RR|ACCPT_EF|ACCPT_OL|ACCPT_NOR);
   } /* End ounnit() */

/*=====================================================================*
*                               POPUNIT                                *
*                                                                      *
*  Subroutine popunit() is used internally by cdunit() and cryin() to  *
*     stop using the current input unit and resume the previous one.   *
*     If current file is marked ACCPT_NOR, close it and release all    *
*     storage, otherwise, move it to the reserve stack so it can be    *
*     used later without a close/open sequence.                        *
*=====================================================================*/

static void popunit(void) {

   register struct RKCDU *pcu =  /* Ptr to old RKCDU */
      RKC.cduptr;

   RKC.cduptr =                  /* Pop the stack */
      (struct RKCDU *)pcu->rfd.nf;
   if (pcu->nvsa > 0)            /* Free variable symbols */
      free(pcu->pfvs);
   if (pcu->accpt & ACCPT_NOR) { /* No reuse--delete file */
      free(pcu->pc);             /* Free card buffer */
      rfclose(&pcu->rfd, SAME, RELEASE_BUFF, ABORT);
      free(pcu);                 /* Delete the old unit block */
      }
   else {                        /* Retain file for reuse */
      pcu->pfvs = NULL;          /* Clean up for reuse */
      pcu->nvs = pcu->nvsa = pcu->nvse = 0;
      pcu->rfd.nf = (struct RFdef *)RKC.cdures;
      RKC.cdures = pcu;
      }
   if ((pcu=RKC.cduptr) != NULL) {  /* Any previous unit? */
      RKC.accpt = pcu->accpt;    /* Yes, restore accpt status */
      RK.last = pcu->pc; }       /* Restore old card pointer */
   else RKC.accpt |= ACCPT_EF;   /* No, grand final eof time */
   } /* End popunit() */

/***********************************************************************
*                               CDUNIT                                 *
*                                                                      *
*  Subroutine cdunit changes the file from which control cards are     *
*     read by cryin.  It retains the status of all previous input      *
*     files on a push-down stack.  This stack is popped up whenever    *
*     an end-of-file or END card is read by cryin.  This facility      *
*     permits a return to normal processing following invocation of    *
*     stored control cards by an EXECUTE control card.  Usage:         *
*                                                                      *
*     void cdunit(char *Fname)                                         *
*                                                                      *
*     Argument: 'Fname' is a string giving the name of the new input   *
*     file.  If Fname==NULL, the stack is popped up, that is, card     *
*     reading continues on the most recently used previous file.       *
*     This call is a no-op if at the top level, unlike the IBM ver-    *
*     sion.  There is no checking for recursive use of EXECUTE files.  *
*     DDnames, as used in MVS or CMS, are not currently supported.     *
***********************************************************************/

void cdunit(char *Fname) {

   register struct RKCDU *pcu;   /* Ptr to current RKCDU */

   if (Fname) {                  /* Non-null filename: push stack */

      pcu = cry_mkiu(Fname);     /* Locate new or reserve RKCDU */
      cry_opiu(pcu);             /* And use it as current input */
      }

   else {                         /* NULL filename:  pop stack */

      /* Ignore if no unit yet or no previous unit and not eof */
      if (((pcu = RKC.cduptr) != NULL) &&
            (pcu->rfd.nf != NULL || (pcu->accpt & ACCPT_EF) != 0))
         popunit();
      }

   return;
   } /* End cdunit() */

/***********************************************************************
*                               GTUNIT                                 *
*                                                                      *
*  Function gtunit is used to retrieve the name of the current         *
*     input file.  Usage:                                              *
*                                                                      *
*     (char *)gtunit(void)                                             *
*                                                                      *
*     The value returned is a pointer to the name of the current       *
*     input file.  This pointer should be considered valid only        *
*     until the next cryin call.  If the name cannot be obtained,      *
*     NULL will be returned.                                           *
***********************************************************************/

char *gtunit(void) {

   if (!RKC.cduptr) return NULL;
   return RKC.cduptr->rfd.fname;
   } /* End gtunit() */

/*=====================================================================*
*                              FREEPFVS                                *
*                                                                      *
*  Free memory for a table of user-defined symbols.  Formerly this     *
*  was just pncu->pfvs, but now we have separately allocated space     *
*  for each symbol value.                                              *
*=====================================================================*/

void freepfvs(struct RKCDU *pncu) {

   int ivs;                   /* Variable symbol counter */

   if (!pncu->pfvs) return;
   for (ivs=0; ivs<pncu->nvs; ++ivs)
      free(pncu->pfvs[ivs].pval);
   free(pncu->pfvs);
   pncu->pfvs = NULL;
   pncu->nvs = pncu->nvsa = pncu->nvse = 0;
   } /* End freepfvs */

/*=====================================================================*
*                              CRICKSYM                                *
*                                                                      *
*  Subroutine cricksym() is used internally when processing TITLE or   *
*     comments cards to check for variable symbols and replace them    *
*     with their values.  The usual scan() rules do not apply because  *
*     (1) we want to preserve the user input except for the symbols,   *
*     and (2) we might get called while reading a scan continuation.   *
*  The rules are:  An '&' starts a symbol except the sequences '\&'    *
*     and '&&' are escapes which resolve to a single '&' in the out-   *
*     put.  A symbol is ended by any of the usual scan() punctuation   *
*     characters.  If the ending character is a '.', it is deleted.    *
*     (We could have a rule for escaping punctuation characters, but   *
*     there is currently no need for it.)  There are no continuations. *
*  There are no error messages because (1) comments are scanned before *
*     being printed, and (2) this keeps old input files compatible.    *
*     If result is longer than a standard control card, any excess     *
*     characters are discarded.  If a symbol is too long or undefined, *
*     it is kept unchanged.                                            *
*                                                                      *
*     The argument is a pointer to the card image to be processed.     *
*=====================================================================*/

static void cricksym(char *pcb) {

   char *pam;                       /* Ptr to an ampersand */
   char *po,*pn;                    /* Ptrs to old and new cards */
   int  lm,lr;                      /* Length of move, length rem */
   char cb2[CDSIZE+1];              /* Space for storing new card */
   char symbol[L_SYMBVAL+1];        /* Array to hold symbol name,
                                    *  but value after vslookup call */

   /* Do a quick check first, because usually there is no symbol */
   if (pam = strchr(pcb, '&')) {    /* Assignment intended */
      lr = CDSIZE;                  /* Max length of output */
      po = pcb, pn = cb2;           /* Set up scan pointers */
      do {                          /* Scan for symbols */
         /* Copy everything up to the beginning of the symbol */
         lm = min(pam - po, lr);
         if (lm > 0) memcpy(pn, po, lm), pn += lm, lr -= lm;
         po = pam + 1;
         /* Check kind of symbol--
         *  No need to check (pn-1 >= cb2), card id is there */
         if (*(pn-1) == '\\') {     /* Escaped symbol */
            *(pn-1) = '&'; }           /* Put back the ampersand */
         else if ((lm = strcspn(po, /* Normal symbol, */
               "\t \"%&\'()*+,-./:;<=>?[\\]`{|}")) > 0 &&
               lm <= L_SYMBVAL) {      /* and not too long */
            memcpy(symbol, po, lm), symbol[lm] = '\0';
            po += lm;
            if (*po == '.') ++po;      /* Eat period delimiter */
            if ((pam = vslookup(symbol, lm)) != NULL)
               lm = RK.length + 1;        /* Use symbol value */
            else                          /* Symbol not defined */
               pam = symbol;              /* Use name as value */
            lm = min(lm, lr);
            if (lm > 0) memcpy(pn, pam, lm), pn += lm, lr -= lm;
            } /* End translating symbol */
         else {                     /* Zero-length or long symbol */
            if (*po == '&') ++po;      /* Eat escaped ampersand */
            if (lr > 0) *pn = '&', pn += 1, lr -= 1;
            }
         } while (pam = strchr(po, '&')); /* End search loop */
      /* Append any remaining fragment after the last symbol */
      lm = strlen(po), lm = min(lm, lr);
      if (lm > 0) memcpy(pn, po, lm), pn += lm;
      /* Copy result back to input string */
      lm = pn - cb2;
      memcpy(pcb, cb2, lm);
      pcb[lm] = '\0';
      } /* End if '&' on card */
   } /* End cricksum() */

/*=====================================================================*
*                              EXECDFLT                                *
*                 Process EXECUTE and DEFAULTS cards                   *
*                                                                      *
*     Usage: int execdflt(char *pcb, int mode)                         *
*                                                                      *
*     Arguments: 'pcb' is a pointer to the card buffer.                *
*                'mode' indicates the type of call as follows:         *
*        mode == EXEC:  call to process EXECUTE card                   *
*        mode == DFLT:  call to process DEFAULTS card                  *
*                                                                      *
*     The value returned is nonzero if an error was detected.          *
*=====================================================================*/

static int execdflt(char *pcb, int mode) {

#if L_SYMBNM > DFLT_MAXLEN    /* Length for symbol names */
#define EXMXSLEN  L_SYMBNM
#else
#define EXMXSLEN  DFLT_MAXLEN
#endif

   struct RKVS *pvs;          /* Ptr to symbol table */
   struct RKVS *pvz;          /* Ptr to current end of symbol table */
   struct RKCDU *pncu;        /* Ptr to new card unit */
   char *pvsn;                /* Ptr to variable symbol name */
   byte ederr = FALSE;        /* Error exit switch */
   byte exrsw = FALSE;        /* TRUE if get NOREWIND option */
   byte norsw = FALSE;        /* TRUE if got NOREUSE option */
   char lea[L_SYMBVAL+1];     /* Scan field (name or value) */
   char exfnm[LFILNM+1];      /* File name */

/* Print the card and prepare to scan it */

   cdprnt(pcb);
   cdscan(pcb, 1, EXMXSLEN, (RK_WDSKIP|RK_AMPNULL));
   exfnm[0] = '\0';           /* Flag no file name yet */

/* Scan for keyword parameters */

   while (!(scan(lea, RK_NEWKEY) & RK_ENDCARD)) {

      /* Check for NOREUSE and set flag if found */
      if (ssmatch(lea, "NOREUSE", 5)) {
         norsw = YES;
         continue; }

      /* Check for NOREWIND and set flag if found */
      if (ssmatch(lea, "NOREWIND", 4)) {
         exrsw = TRUE;
         continue; }

      /* All other options are keyword=value forms--
      *  check now for punctuation error if no equals sign. */
      if (!(RK.scancode & RK_EQUALS)) {
         ermark(RK_EQRQERR);
         goto ed_puncterr1E; }

      /* Check for FILE='name' option and store file name.
      *  N.B. cdunit() cannot be called at this time, because we
      *  must read any continuation cards first from the old unit. */
      if (mode == EXEC && ssmatch(lea, "FILE", 4)) {
         int oldlen = scanlen(LFILNM);
         if (scan(exfnm, RK_REQFLD|RK_SCANFNM) & ~RK_COMMA)
            goto ed_puncterr1;
         scanlen(oldlen);
         continue; }

      /* Check for MAXSYM.  This is now an obsolete option--
      *  just skip over it and its value.  */
      if (ssmatch(lea, "MAXSYM", 4)) {
         if (scan(NULL,RK_REQFLD) & ~RK_COMMA)
            goto ed_puncterr1;
         continue; }

/* Found a keyword that is not FILE, NOREUSE, NOREWIND, or MAXSYM--
*  it must be the first variable symbol.  Push it back for scanning
*  later and go set up RKCDU to receive symbol definitions.  */

      scanagn();              /* Push the field back */
      break;

/* Punctuation error while scanning preliminary keywords--
*  Mark the error and continue from ed_getfield */

ed_puncterr1:
      ermark(RK_PUNCERR);
ed_puncterr1E:
      ederr = TRUE;
      } /* End keyword scan loop */

/* If we have an EXECUTE card and a file name has been found, make
*  (or locate) an RKCDU block for it with no symbol table yet.  If
*  executing the present file, release any existing symbol table.
*  If we have a DEFAULTS card, leave any existing symbol table alone.
*/

   if (mode == EXEC) {        /* EXECUTE card */
      if (exfnm[0])              /* Handle named file */
         pncu = cry_mkiu(exfnm);
      else {                     /* Handle current file */
         pncu = RKC.cduptr;
         if (pncu->nvsa > 0)     /* Free previous symbol table */
            freepfvs(pncu);
         } /* End setup for adding symbols to current file */
      if (exrsw) pncu->rfd.norew = (byte)NO_REWIND;
      if (norsw) pncu->accpt |= ACCPT_NOR;
      } /* End EXECUTE card setup */
   else {                     /* DEFAULTS card */
      pncu = RKC.cduptr;
      } /* End DEFAULTS card setup */

/* Pick up variable symbols and store values in the symbol table--
*  but only if there is not already a value there from the EXECUTE
*  card.  But if there are multiple definitions of the same symbol
*  on the same card, the LAST is used, consistent with normal ROCKS
*  practice.  */

   while (!(scan(lea, RK_NEWKEY) & RK_ENDCARD)) {
      if (!(RK.scancode & RK_EQUALS)) {
         ermark(RK_EQRQERR);
         goto ed_puncterr2E; }

/* See if the symbol already exists in the table.
*  This could be replaced by a hash lookup some day.  */

      pvz = pncu->pfvs + pncu->nvs;    /* Table end fence */
      pvsn = lea + (lea[0] == '&');    /* Skip leading '&' */
      for (pvs=pncu->pfvs; pvs<pvz; pvs++)
         if (ssmatch(pvsn, pvs->vname, L_SYMBNM))
            goto ed_store;
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
      strncpy(pvs->vname, pvsn, L_SYMBNM);
      /* Get value and copy to table unless this is DEFAULTS
      *  card and value was placed in table by EXECUTE card.  */
ed_store:
      {  int oldlen = scanlen(L_SYMBVAL);
         if (scan(lea, RK_REQFLD) & ~RK_COMMA) goto ed_puncterr2;
         scanlen(oldlen);  }
      if (mode == EXEC || pncu->nvs > pncu->nvse) {
         pvs->pval = malloc(RK.length+2);
         strncpy(pvs->pval, lea, RK.length+1);  /* Value to table */
         pvs->vsize = RK.length + 1;            /* Length to table */
         }
      continue;
ed_puncterr2:                 /* Punct err in symbol scan */
      ermark(RK_PUNCERR);
ed_puncterr2E:
      ederr = 1;
      } /* End symbol scan loop */

/* Finished card scan.  If this is EXECUTE, save number of symbols.
*  Switch to new unit and return to caller.  */

   if (mode == EXEC) {
      pncu->nvse = pncu->nvs;
      if (exfnm[0]) cry_opiu(pncu);    /* Switch units */
      } /* End if EXECUTE */
   return (int)ederr;
   } /* End execdflt */

/***********************************************************************
*                                CRYIN                                 *
*                                                                      *
*  Function cryin reads the next line from the current input file      *
*     specified by the last call to cdunit.  The default input is      *
*     from stdin.  Cryin takes care of comments, TITLE, PROMPT, QUIT,  *
*     SPOUT, SETENV, ERROR DUMP REQUESTED, OUTPRTY, DEFAULTS, EXECUTE, *
*     continuation, and END cards.  These cards are identified by the  *
*     first 6 columns to avoid mistaking application cards for them.   *
*     (Note that if any of these cards is continued, a recursive call  *
*     to cryin is generated.)  If RDAGN has been called, the previous  *
*     line is reread.  The call is:                                    *
*                                                                      *
*     char *cryin(void)                                                *
*                                                                      *
*     A pointer to the input card is returned and is also placed in    *
*     RK.last.  This pointer should be considered valid only until     *
*     the next call to cryin.  When the end of input or an END or QUIT *
*     card has been reached, output is flushed and NULL is returned.   *
*     Note: most programs end when they get an eof exit from cryin.    *
*     These programs do not have to worry about flushing their last    *
*     print buffer, because cryin will do it for them at the time      *
*     the last attempt is made to read a card.                         *
*                                                                      *
*     When in CrkParse mode, the special cards listed above are        *
*     treated as ordinary data.  Substitution of variable symbols      *
*     is controlled by the RKKP_NOVS bit, and insertion of EXECUTE     *
*     files is controlled by the RKKP_DOEX bit.                        *
*                                                                      *
*     IMPLEMENTATION NOTE:  An RKdef block is set up for each file     *
*     requested by an EXECUTE card or cdunit() call.  If the file is   *
*     popped due to end-of-file being reached when NOREUSE was given,  *
*     the file is closed and all storage is released, reducing the     *
*     likelihood of running up against an OS limitation on the number  *
*     of simultaneously open files.  However, in all other cases, the  *
*     RKdef block is retained on a reserve list and the file is kept   *
*     open.  In this way, the NOREWIND parameter is honored even if    *
*     not implemented in the library.  NOREWIND is not really needed   *
*     now, because we don't rewind after an END card anyway.           *
*                                                                      *
*     CAUTION:  The length of the input buffer used for each card unit *
*     is set by the parameter CDSIZE in SYSDEF.H.  This should be set  *
*     large enough to contain the longest record that might reasonably *
*     be input under the operating system in use. It is very difficult *
*     (and maybe not wise) to allocate this buffer dynamically.        *
***********************************************************************/

char *cryin(void) {

   struct RKCDU *pcu;         /* Pointer to card unit data */
   long lcard;                /* Length of current card */
#ifdef UNIX
   static int fd = 0;         /* Descriptor for writing prompt */
#endif
   char lea[DFLT_MAXLEN+1];   /* Scan field */
   char *pcb;                 /* Ptr to card buffer */

/* If there is not yet an RKCDU block, make one for stdin.
*  Try to determine whether an on-line prompt should be given.  */

   if (!RKC.cduptr) {
      RKC.cduptr = pcu = cry_mkiu(NULL);
      rfopen(&pcu->rfd, NULL, READ, SAME, SEQUENTIAL, TOP, LOOKAHEAD,
         NO_REWIND, RETAIN_BUFF, IGNORE, IGNORE, IGNORE, ABORT);
      if (qonlin()) {
         RKC.accpt |= ACCPT_OL;
         pcu->rfd.accmeth |= ON_LINE;
#ifdef UNIX
         fd = open("/dev/tty", O_RDWR);
#endif
         }
      }

/* Reenter here after eof pops card unit */

restart:
   pcu = RKC.cduptr;                            /* Locate RKCDU */
   if (!pcu || RKC.accpt & ACCPT_EF) {          /* EOF exit */
      if (!(RKC.kparse & RKKP_CRKP))            /* Flush output */
         cryout(RK_P1,"\0",RK_LN0+RK_FLUSH+1,NULL);
      pcb = NULL;
      goto cryin_return_new; }

/* Check for reread.  If the ACCPT_CE bit is off, this card has
*  already been checked and found not to be a special card, so we
*  can return it at once.  If the ACCPT_CE bit is on, this card
*  was rejected while looking for a continuation.  In that case,
*  it is now necessary to check whether it is a special card.
*  (We can not check for a special card while we are looking for
*  continuations, because we could recursively start a new cdscan
*  before the previous one had finished.)  It is not necessary to
*  call erprnt() here because RK.last does not change.  */

   if (RKC.accpt & ACCPT_RR) {
      RKC.accpt &= ~ACCPT_RR; /* Turn off the reread flag */
      pcb = RK.last;          /* Rereturn previous card */
      if (RKC.accpt & ACCPT_CE)
         goto check_specials;
      else
         goto cryin_return_new;
      }

/* Locate current unit block and card pointer */

   pcb = pcu->pc;             /* Point to the card buffer */

read_another:                 /* Here to read another card */

/* If there were any errors posted in connection with the last card,
*  which may have been an internal one that returns to read_another,
*  call erprnt() to print the error messages before buffer changes.
*  (Remember RK.last is just a copy of pcb, not a copy of the data.)
*/

   if (RKC.accpt & ACCPT_ERP) erprnt();

/* Print the prompt if online, then read next input line */

#if defined(UNIX)
   if (RKC.accpt & ACCPT_OL)  {
      cryout(RK_P1, "\0", RK_LN0+RK_FLUSH+1, NULL);
      if (RKC.accpt & ACCPT_CE)
         write(fd, "\n...?", 5);
      else if (RKC.plen > 0)
         write(fd, RKC.prompt, RKC.plen);
      }
#endif
   lcard = rfgets(&pcu->rfd, pcb, CDSIZE+2, ABORT) - 1;
   if (lcard < 0) goto eof1;
   if (lcard > CDSIZE)
      abexitm(60, "Input line too long.");

/* If in CrkParse mode, do things rather differently.  Because a
*  comment may occur between a card and its continuation, comments
*  have to be written directly to the output from here (and there
*  is no need to maintain the ACCPT_CC bit).  */

   if (RKC.kparse & RKKP_CRKP) {

      if (ssmatch(pcb,"END", 3)  && qwhite(pcb+3)) goto eof2;
      if (ssmatch(pcb,"QUIT", 4) && qwhite(pcb+4)) {
         RKC.accpt |= ACCPT_EF; goto eof2; }
      if (*pcb == '*') {         /* Got a comment */
         if (!(RKC.kparse & RKKP_NOVS)) cricksym(pcb);
         cryout(RK_P1, " {{*}=\"", RK_LN1+7, pcb, RK_CCL,
            "\"}", RK_CCL+2, NULL);
         goto read_another;
         }
      else if (*pcb == '\0' || qwhite(pcb)) {
         cryout(RK_P1, " {{*}=\" \"}", RK_LN1+10, NULL);
         goto read_another;
         }
      RK.last = pcb;
      /* Fairly normal continuation checking.  Do not clear ACCPT_CS,
      *  that is done in scan() only after an ermark() call. */
      if (RKC.accpt & ACCPT_CE) goto cryin_return;
      if (!(RKC.kparse & RKKP_WD) && iscontnu(pcb) && cntrl(pcb)) {
         cdprt1(pcb);
         cryout(RK_P1," -->Unexpected continuation ignored.",
            RK_LN1, NULL);
         goto read_another;
         }
      /* Deal with symbols on TITLE cards */
      if (ssmatch(pcb, "TITLE", 5)) {
         if (!(RKC.kparse & RKKP_NOVS)) cricksym(pcb);
         settit(pcb);
         }
      goto cryin_return_new;
      } /* End CrkParse mode */

/* If card is a comment, replace any variable symbols with their
*  values, then print the card with 8-column indent and read
*  another.  The print is double-spaced if the comment has text
*  and the previous line was not also a comment as indicated by
*  the ACCPT_CC flag.  Null lines are always taken as comments.
*  Whitespace lines are comments only if accwac() was called,
*  otherwise are treated as data for compatibility with old-
*  style formatted input.  A comment does not clear ACCPT_CEP,
*  because it may occur between a card and its continuation.   */

   if (*pcb == '*' || *pcb == '#') {   /* Comment with text */
      cricksym(pcb);          /* Deal with symbols */
      if (RKC.accpt & ACCPT_CC)
         cryout(RK_P3, "        ", RK_LN1+8, pcb, RK_CCL, NULL);
      else {
         cryout(RK_P3, "0       ", RK_LN2+8, pcb, RK_CCL, NULL);
         RKC.accpt |= ACCPT_CC; }
      goto read_another;
      }                       /* Comment with no text */
   if (*pcb == '\0' || RKC.kparse & RKKP_WC && qwhite(pcb)) {
      cryout(RK_P3, "    ", RK_LN1+4, NULL);
      RKC.accpt |= ACCPT_CC;
      goto read_another;
      }
   RKC.accpt &= ~ACCPT_CC;    /* Not a comment */

/* Check for an 'END' or 'QUIT' card, and if found, simulate an end-
*  of-file.  These cards are recognized even if a continuation is
*  expected.  They may be followed by whitespace, but nothing else.
*  QUIT differs from END in that it sets the ACCPT_EF bit, so program
*  quits even if in an EXECUTE file.  */

   if (ssmatch(pcb, "END", 3)  && qwhite(pcb+3)) goto eof2;
   if (ssmatch(pcb, "QUIT", 4) && qwhite(pcb+4)) {
      RKC.accpt |= ACCPT_EF; goto eof2; }

/* Store location of this card */

   RK.last = pcb;

/* If cryin was called from scan() while seeking a continuation, just
*  return and let scan() determine if this is one.  In this case, do
*  not clear the ACCPT_CS etc. bits because the continuation checker
*  may want to print an error message from the previous card.  If
*  accwad() was called, the card is considered data even if it looks
*  like a continuation (it may be old-fashioned formatted input).
*  Otherwise, check the card--if it is a continuation, it is an error,
*  but this case is demoted to a warning because a valid data card may
*  masquerade as a continuation.  */

   if (RKC.accpt & ACCPT_CE) goto cryin_return;
   if (RKC.kparse & RKKP_WD) goto check_specials;
   if (iscontnu(pcb)) {
      if (cntrl(pcb)) {
         cdprt1(pcb);
         cryout(RK_P1," -->Unexpected continuation ignored.",
            RK_LN1, NULL);
         goto read_another;
         }
      /* Numeric data masquerading as continue */
      goto cryin_return_new;
      } /* End continuation processing */

/* Card is new--clear scanned, last printed, and ever printed flags.
*  Note:  ACCPT_CLP logic changed 4/24/94.  ACCPT_CLP is cleared once
*  we are past all possibility of printing an error message relating
*  to the old card, indicating that the new card has not yet been
*  printed.  This must be done without reference to the print priority
*  (an earlier bug).  However, ACCPT_CEP is not cleared because it
*  controls printing of any continuations based on printing of the
*  parent.
*
*  Check for special cards.  As a small optimization, break this down
*  according to first/second half of alphabet.  */

check_specials:
   RKC.accpt &= ~(ACCPT_CEP | ACCPT_CLP | ACCPT_CS |
      ACCPT_MKOK | ACCPT_MKNO);
   if (toupper(*pcb) > 'M') {

/* Check for a 'TITLE' card and, if found, set a new title
*  after replacing any variable symbols with their values.
*  This card is printed directly in order to display any
*  symbols that were used and to give it priority 1 and to
*  be sure the application's default title displays at least
*  once before the new one takes effect.  */

      if (ssmatch(pcb, "TITLE", 5)) {
         cryout(RK_P1, "0   ", RK_LN2+4, pcb, RK_CCL, NULL);
         cricksym(pcb);          /* Deal with symbols */
         settit(pcb);
         RKC.accpt |= (ACCPT_CEP|ACCPT_CLP);
         goto read_another;
         } /* End TITLE card processing */

/* Check for an 'OUTPRTY' card and, if found, set output priority.
*  Look for 'NOPG' option and call nopage() if found.  (There is
*  currently no way to restore normal pagination.) */

      if (ssmatch(pcb, "OUTPRTY", 6)) {
         cdprnt(pcb);
         cdscan(pcb, 1, DFLT_MAXLEN, RK_WDSKIP);
         eqscan(&RKC.oprty, "V>0<=5IC", RK_BEQCK);
         if (!(scanck(lea,0,~(RK_ENDCARD|RK_COMMA)) & RK_ENDCARD)) {
            if (ssmatch(lea, "NOPG", 4)) nopage(RK_PGNONE);
            thatsall(); }
         goto read_another;
         } /* End OUTPRTY card processing */

/* Check for a 'SETENV' card and, if found, read and set environment
*  variables from input.  Values may in turn be variable symbols.  */

      if (ssmatch(pcb, "SETENV", 6)) {
         char evar[L_ENVSET+1],eval[L_ENVSET+1];
         cdprnt(pcb);
         cdscan(pcb, 1, L_ENVSET, RK_WDSKIP);
         while (scanck(evar, RK_NEWKEY, ~(RK_EQUALS|RK_ENDCARD)) <
               RK_ENDCARD) {
            if (RK.scancode != RK_EQUALS) break;
            scanck(eval, 0, ~(RK_BLANK|RK_COMMA|RK_ENDCARD));
            if (RK.scancode & ~(RK_BLANK|RK_COMMA|RK_ENDCARD)) break;
            if (*eval == '\0') unsetenv(evar);
            else setenv(evar, eval, 1);
            }
         goto read_another;
         } /* End SETENV card processing */

/* Check for a 'SPOUT' card, and if found, switch SPOUT on or off */

      if (ssmatch(pcb, "SPOUT", 5)) {
         cdprnt(pcb);
         cdscan(pcb, 1, DFLT_MAXLEN, RK_WDSKIP);
         if (scanck(lea, RK_REQFLD, ~(RK_ENDCARD|RK_COMMA)) <
               RK_ENDCARD) {
            if      (ssmatch(lea, "ON", 2)) spout(SPTM_LCLON);
            else if (ssmatch(lea, "OFF",3)) spout(SPTM_LCLOFF);
            thatsall();
            }
         goto read_another;
         } /* End SPOUT card processing */

/* Check for a 'PROMPT' card, and, if found, set prompt after
*  checking for variable symbols in the text string. */

      if (ssmatch(pcb, "PROMPT", 6)) {
         cdprnt(pcb);
         cdscan(pcb, 7, L_PROMPT, 0);
         scanck(RKC.prompt+2, 0, ~(RK_ENDCARD|RK_COMMA));
         RKC.plen = (RK.length >= 0) ? RK.length + 3 : 0;
         goto read_another;
         } /* End PROMPT card processing */

      } /* End second half-alphabet */
   else {

/* Check for an 'ERROR DUMP REQUESTED' card--
*  Sets 'RKKP_EDMPRQ' flag for abexit() */

      if (ssmatch(pcb, "ERROR DUMP", 10)) {
         cdprnt(pcb);
         RKC.kparse |= RKKP_EDMPRQ;
         goto read_another;
         } /* End ERROR DUMP REQUESTED card processing */

/* Check for an 'EXECUTE' card and, if found, process it */

      if (ssmatch(pcb, "EXECUTE", 6)) {
         if (execdflt(pcb, EXEC))   /* Scan it and exit if error */
            abexitm(64, "Error on EXECUTE card.");
         goto restart;
         } /* End EXECUTE card processing */

/* Check for a 'DEFAULTS' card and, if found, process it */

      if (ssmatch(pcb, "DEFAULTS", 6)) {
         if (execdflt(pcb,DFLT))    /* Scan it and exit if error */
            abexitm(64,"Error on DEFAULTS card.");
         goto read_another;
         } /* End DEFAULTS card processing */

      } /* End first half-alphabet */

/* None of the above special cards.  Return location to caller */

cryin_return_new:
   RKC.accpt &= ~(ACCPT_CEP | ACCPT_CLP | ACCPT_CS |
      ACCPT_MKOK | ACCPT_MKNO);
cryin_return:
   return pcb;

/* Physical end-of-file.  Rewind the file, unless NOREWIND was
*  specified, then treat same as logical end-of-file (END or QUIT
*  card).  (NOREWIND is set during initial setup for stdin.)  */

eof1:
   if (pcu->rfd.norew < (char)NO_REWIND)
      rfseek(&pcu->rfd, 0L, SEEKABS, ABORT);

/* Logical end-of-file.  Suspend reading from this file, but retain
*  its RKCDU block on the reserve list so can resume later.  This is
*  the way the user gets to execute a sequence of procedures loaded
*  in one file and separated by END cards.  The conditions tested at
*  cdunit(0) cannot happen here, so popunit() is called directly.  */

eof2:
#if 0    /* This code seems unnecessary */
   /* Clear printed, scanned flags */
   RKC.accpt &= ~(ACCPT_CEP | ACCPT_CLP | ACCPT_CS |
      ACCPT_MKOK | ACCPT_MKNO);
#endif
   popunit();                 /* Pop the file stack */
   goto restart;
   } /* End cryin() */

/***********************************************************************
*                              cryicls()                               *
*                                                                      *
*  Call cryicls() to close any active or pending input files and       *
*  release any dynamic storage associated with cryin().                *
***********************************************************************/

void cryicls(void) {

   /* Get rid of active units */
   while (RKC.cduptr) {
      RKC.cduptr->accpt |= ACCPT_NOR;
      popunit();
      }

   /* Get rid of reserve units by transferring the entire
   *  list to the active list and repeating the above.  */
   RKC.cduptr = RKC.cdures, RKC.cdures = NULL;
   while (RKC.cduptr) {
      RKC.cduptr->accpt |= ACCPT_NOR;
      popunit();
      }

   } /* End cryicls() */
