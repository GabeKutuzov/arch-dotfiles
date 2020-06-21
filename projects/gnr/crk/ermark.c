/* (c) Copyright 1988-2010, The Rockefeller University *11116* */
/* $Id: ermark.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                           ERMARK, OKMARK                             *
*                       Error marking routines                         *
*                                                                      *
*----------------------------------------------------------------------*
*                              ermark()                                *
*                                                                      *
*  This routine is used to report errors that arise while scanning     *
*     control cards.  If the card has not already been printed, it     *
*     it is printed before the messages.  The location of each error   *
*     is marked by a '$' printed below the control card at the current *
*     scan location.  Multiple messages can be produced by separate    *
*     calls or by adding codes in a single call.  ermark() records     *
*     the error codes in RK.erscan and RKC.erdmrk, sets the 1 bit      *
*     of RK.iexit, and sets RK.highrc to a minimum return code of 4.   *
*                                                                      *
*  Synopsis:                                                           *
*     void ermark(ui32 mcode)                                          *
*                                                                      *
*  Argument:                                                           *
*     'mcode' is a message code made by summing error codes from the   *
*     list defined in rocks.h (many of these messages are usually      *
*     generated internally by the scanning routines).  High-order      *
*     code bits indicate need to print card and possibly mark error    *
*     locations without printing a specific message.                   *
*                                                                      *
*  Note:  To avoid duplicate error messages, ermark does not in        *
*     fact print anything when called, but rather sets ACCPT_ERP       *
*     bit in RKC.accpt, causing the requested messages to be printed   *
*     later.  Accordingly, to assure that all error messages are       *
*     printed when a run terminates, a cryout FLUSH call must be       *
*     made following the last ermark call.                             *
*                                                                      *
*----------------------------------------------------------------------*
*                              okmark()                                *
*                                                                      *
*  This routine is used to indicate whether scanning columns are valid *
*     for ermark.  ermark normally uses scan column information from   *
*     scan to position a '$' under the item in error.  This procedure  *
*     becomes invalid if the user scans a string not related to the    *
*     current control card.  To indicate to ermark that this is the    *
*     case, call okmark.                                               *
*                                                                      *
*  Synopsis:                                                           *
*     void okmark(int dmflag)                                          *
*                                                                      *
*  Argument:                                                           *
*     'dmflag' is FALSE if subsequent calls to ermark should not       *
*     generate '$' marks, TRUE to restore this function.  Every        *
*     call to cryin implicitly calls okmark(TRUE).                     *
*                                                                      *
************************************************************************
*  V1A, 11/06/88, GNR - Convert from my IBM 370 Assembler version      *
*  Rev, 04/03/89, GNR - Move erprnt() from cryout to eliminate         *
*                       recursive prototyping that fails on NCUBE      *
*  V1C, 07/24/89, GNR - Add okmark routine                             *
*  Rev, 08/08/92, GNR - Make ercode unsigned for proper right shift    *
*  Rev, 08/23/98, GNR - Break out from cdscan.c, add mord controls,    *
*                       add continuation, exclusive, and nest errors   *
*  Rev, 09/13/98, GNR - Add RK_WARNING, RK_MARKFLD, RK_MARKDLM         *
*  Rev, 12/24/00, GNR - Add RK_PNRQERR                                 *
*  Rev, 08/21/01, GNR - Add RK_EQRQERR                                 *
*  Rev, 01/27/03, GNR - Add RK_UNITERR, expand erscan to long          *
*  Rev, 03/16/07, GNR - Add RK_SYMBERR                                 *
*  ==>, 03/24/07, GNR - Last date before committing to svn repository  *
*  Rev, 02/14/09, GNR - Add RK_VANSERR                                 *
*  Rev, 08/06/09, GNR - Change arg to ui32 for 64-bit compilation      *
*  Rev, 06/11/10, GNR - Introduce ACCPT_MKOK and ACCPT_MKNO            *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rocks.h"
#include "rockv.h"
#include "rkxtra.h"

/* Mask indicating which error codes can create '$' marks */
static ui32 ERMdm =        ( RK_PUNCERR | RK_IDERR   | RK_LENGERR |
   RK_PNQTERR | RK_REQFERR | RK_NUMBERR | RK_CHARERR | RK_ABBRERR |
   RK_TOOMANY | RK_EXCLERR | RK_NESTERR | RK_PNRQERR | RK_EQRQERR |
   RK_UNITERR | RK_MULTERR | RK_SYMBERR | RK_VANSERR | RK_MARKDLM |
   RK_MARKFLD );
/* Mask indicating which '$' marks should be backed up into data */
static ui32 ERMbm =          ( RK_IDERR | RK_LENGERR | RK_NUMBERR |
   RK_CHARERR | RK_ABBRERR | RK_TOOMANY | RK_EXCLERR | RK_PNRQERR |
   RK_UNITERR | RK_MULTERR | RK_SYMBERR | RK_VANSERR | RK_MARKFLD );

/*=====================================================================*
*                          Subroutine ERPRNT                           *
*                                                                      *
*  This routine is called from cryout to print and spout error         *
*  messages registered by ermark().  This necessitates that routine    *
*  prnlin in cryout be visible externally and callable from here,      *
*  and that the SU and PU structures be global.  This incidentally     *
*  escapes the oprty of the original cryout call.                      *
*=====================================================================*/

void erprnt(void) {

/* Text of numbered ermark messages.
*  Note:  RK_CHARERR has been replaced by calls to vckmsg().
*  This code is available for reassignment to a new message.  */

   static char *emsg[] = {
      "Incorrect punctuation",
      "Keyword unrecognized or invalid in context",
      "Field is too long",
      "Unbalanced parens or quotes",
      "Required field missing",
      "Above line not recognized",
      "Invalid or unexpected numeric value",
      "Bad character in numeric field",
      "Ambiguous abbreviation",
      "Too many data fields of kind marked",
      "Expected continuation not found, scan end forced",
      "Blank continuation found, scan end forced",
      "More than one of exclusive set",
      "Unexpected left parenthesis",
      "Parentheses required",
      "Equals sign expected",
      "Unit spec invalid or not the type expected",
      "Unrecognized metric unit multiplier prefix",
      "Undefined variable symbol",
      "Value for variable adjustment not set",
      };
   static char stars[] = " ***";
   static char atmsg[] = " at '$`";

   /* Define order in which these messages should be printed */
   static byte mord[] = { 0, 15, 13, 14, 1, 2, 8, 9, 12, 16, 17,
      18, 19, 7, 6, 10, 11, 3, 4, 5 };

   ui32 km;                   /* Message selector bit */
   int im,jm;                 /* Message indexes */
   int kdmrk;                 /* Marker message control */
   int width;                 /* Width of text */

/* Make local copies of error codes and zero the originals to avoid
*  infinite recursion when cryout is called to print the messages.
*  Remove any undefined bits in ecodes to avoid infinite loop below. */

   ui32 ecodes = RK.erscan & (((ui32)1<<sizeof mord)-1);
   ui32 dcodes = RKC.erdmrk;
   RK.erscan = 0;
   RKC.erdmrk = 0;
   RKC.accpt &= ~ACCPT_ERP;

/* If spout is active, spout the last control card.  Because cdprnt
*  doesn't normally spout control cards, we don't bother to keep a
*  separate "spouted" flag--just spout always and accept the chance
*  of occasional double spouting.  On the other hand, printing will
*  normally have already been done, so we print only if ACCPT_CLP is
*  not already set.  This necessitates using the lower-level prnlin
*  routine to separate the printing and spouting functions.  */

   if (RK.last) {             /* Omit if no card read yet */
      width = strlen(RK.last);
      if (RKC.spend >= SPTM_NONE) {
         prnlin(&RKC.SU, "    ", 4, RK_LN1);
         prnlin(&RKC.SU, RK.last, width, RK_CCL); }

/* If card was not yet printed, print it now */

      if (!(RKC.accpt & ACCPT_CLP)) {
         prnlin(&RKC.PU, "    ", 4, RK_LN1);
         prnlin(&RKC.PU, RK.last, width, RK_CCL); }
      } /* End printing of last control card */

/* Print the "$" marker message with spouting.  Even when empty, it
*  serves as a spacer above the first error message.  It is not
*  necessary to flush, as subsequent error messages will do so
*  automatically.  Then blank it out for the next time.  */

   if (RKC.dmsg) {
      cryout(RK_P1+1, "    ", RK_LN1+4, RKC.dmsg, RK_CCL+CDSIZE, NULL);
      memset(RKC.dmsg, ' ', CDSIZE); }

/* Scan the possible error bits and print messages for the ones that
*  have been set.  Append "at $" message when so flagged.  Because
*  these are error messages, a second cryout call cannot be used.  */

   for (im=0; ecodes; im++) {
      jm = mord[im], km = (ui32)1<<jm;
      if (ecodes & km) {      /* If bit is set, print message */
         kdmrk = (dcodes & ERMdm & km) ? sizeof(atmsg) : RK_SKIP;
         cryout(RK_P1, stars, RK_LN1+4, emsg[jm], RK_CCL,
            atmsg, kdmrk, ".", 1, NULL);
         ecodes ^= km; }      /* Clear request bit */
      }

   return;                    /* All done */

   } /* End erprnt() */

/*=====================================================================*
*                                                                      *
*                          Subroutine ERMARK                           *
*                                                                      *
*=====================================================================*/

void ermark(ui32 mcode) {

   int col;                            /* Column number */

/* Unless this is just a warning, set RK.iexit and RK.highrc */

   if (!(mcode & RK_WARNING)) {
      RK.iexit |= 1;                      /* Set exit flag for user  */
      if (RK.highrc < 4) RK.highrc = 4;   /* Record high-water error */
      }

/* Set signal for cryout to call erprnt on next call and save
*  codes indicating which messages are to be printed.  */

   RKC.accpt |= ACCPT_ERP;
   RK.erscan |= mcode;

/* If the card is being scanned, and the particular error can be
*  referred to a specific position, mark the current scan column. */

   if (mcode & ERMdm && !(RKC.accpt & ACCPT_MKNO) &&
         (RKC.accpt & (ACCPT_CS|ACCPT_MKOK))) {
      if (!RKC.dmsg) {                 /* Allocate and clear dmsg */
         RKC.dmsg = mallocv(CDSIZE, "Error marker");
         memset(RKC.dmsg, ' ', CDSIZE); }
      /* Calc column for error flag */
      col = (RKC.pcfd ? RKC.pcfd : RKC.pc) - RKC.cdsave - 1;
      if (col > 0 && col < CDSIZE) {   /* Do safety test */
         if (mcode & (ERMdm ^ ERMbm))
            RKC.dmsg[col] = '$';       /* Put '$' under delimiter */
         if (mcode & ERMbm)
            RKC.dmsg[col-1] = '$';     /* Put '$' under data field */
         /* Tell erprnt to append "at $" to marked messages */
         RKC.erdmrk |= mcode;
         }
      } /* End marking */

   } /* End ermark() */

/*=====================================================================*
*                          Subroutine OKMARK                           *
*                                                                      *
*  Note:  This code was revised, 06/11/10, to set ACCPT_MKxx bits      *
*  instead of directly modifying ACCPT_CS.  This leaves ACCPT_CS as    *
*  always a valid indicator of whether input is being scanned or not.  *
*=====================================================================*/

void okmark(int dmflag) {

   if (dmflag) RKC.accpt |= ACCPT_MKOK;
   else        RKC.accpt |= ACCPT_MKNO;

   } /* End okmark() */

