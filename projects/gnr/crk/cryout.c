/* (c) Copyright 1988-2018, The Rockefeller University *11115* */
/* $Id: cryout.c 67 2018-05-07 22:08:53Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*             CRYOUT, SPOUT, and related output routines               *
*                                                                      *
************************************************************************
*  Design Notes: The RK_PF mode is passed to prnlin() via usecc,       *
*  allowing this new mode to be implemented with no changes to the     *
*  prnlin() call, older cryout(), or ermark().  Line counts for        *
*  paging are based only on counts in 'lcode' and not by counting      *
*  actual LFs (except when breaking a too-long line).  Pagination      *
*  will be in error if lcodes are not correct.                         *
************************************************************************
*  V1A, 11/14/88, GNR - Adapted from my IBM Assembler versions         *
*  V1B, 02/24/89, GNR - Use carriage controls to count lines           *
*  Rev, 04/03/89, GNR - Modify for compilation on NCUBE                *
*  Rev, 04/19/89, GNR - Incoporate rkxtra.h                            *
*  Rev, 06/22/89, GNR - Correct bug in multiline subtitles             *
*  Rev, 03/02/92, GNR - Do fflush after endprt on FLUSH call           *
*  Rev, 04/03/92, GNR - Use min(width,strlen) when width given         *
*  Rev, 06/25/92, ABP - Cast to char * in call to memcpy               *
*  Rev, 08/18/92, GNR - Don't spout comments, don't flush              *
*                       warnings, do fflush all internal flushes,      *
*                       spout width 132, fix time overflow, make       *
*                       lines() flush cryout buffer                    *
*  Rev, 06/04/94, GNR - Add spoutm routine for spout override          *
*  Rev, 05/22/96, GNR - Add RK_NTSUBTTL code for clearing subtitle     *
*  Rev, 08/15/97, GNR - Add nopage() to globally suppress pagination   *
*  Rev, 09/11/97, GNR - Return remaining columns in RK.rmcls           *
*  Rev, 09/12/98, GNR - Use rfwrite, correct stderr flushing, LNSIZE   *
*                       replaces PGLNS,TTLKNS, absorb blank lines at   *
*                       top of pg, initialize spout via rfqsame        *
*  ==>, 03/08/07, GNR - Last date before committing to svn repository  *
*  Rev, 08/05/09, GNR - Mods for 64-bit compilation                    *
*  Rev, 11/07/09, GNR - Eliminate spoutm, more logical SPOUT controls  *
*  Rev, 08/13/11, GNR - Add RK_PF bit for printf() output (no CC)      *
*  Rev, 11/14/11, GNR - Bug fix: Only omit LF on first line of subttl  *
*  Rev, 01/05/12, GNR - Bug fix: Protect against zero-width fields     *
*  Rev, 01/09/12, GNR - Bug fix: New page on rmlns <= 0, not just < 0  *
*  Rev, 04/13/13, GNR - Add cryocls()                                  *
*  Rev, 05/01/14, GNR - Flush spout output at end of every call        *
*  R60, 03/29/16, GNR - Write to debug file if RKC.PU.type is 2        *
*  R65, 09/29/17, GNR - Increase title space to 66, decrease pid to 30 *
*  R67, 03/30/18, GNR - Replace default title (ROCKS) w/NOT ENTERED    *
***********************************************************************/

/*---------------------------------------------------------------------*
*        Usage:                                                        *
*---------------------------------------------------------------------*/

/*
         Subroutine cryout is used to prepare printed output with
         page titles and subtitles.  Output is written to stdout and
         repeated to stderr if spout has been called.  Each cryout
         call may specify any number of text strings to be combined
         into one or more lines of output.  By default, there must be
         an ANSI carriage control character at the beginning of each
         line of output--these characters are internally converted to
         whatever control characters are needed in a particular im-
         plementation.  Alternatively, if the RK_PF bit is set in the
         'sprty' argument, UNIX-style text is expected, with LF in
         the data to start a new line and CR to overlay lines.  In
         either case, the line count is given in the 'lcode' argument
         allowing the user to make room on the page for a block of
         output that may be supplied in multiple cryout calls.

         To print output:
            void cryout(int sprty, char *field, int lcode, ..., NULL)

         'sprty' is the sum of values indicating the method of line
            control, the iexit setting (times 2^12), priority class
            (times 2^8), and spout count (unshifted) of the output.
            The RK_PF bit (0x800) indicates that ASCII carriage con-
            trols are used.  The named constants RK_P1, ...  , RK_P5
            can be used to indicate priorities 1 (highest) through 5
            (lowest), respectively, and RK_E1, ..., RK_E8 can be used
            to indicate error messages with the indicated iexit value
            and priority 1.  Only outputs with priorities not less
            than that given on the most recent OUTPRTY card are
            actually printed.  A nonzero spout value has the same
            effect as a call to spout with the same argument.  The
            spout number adds up across cryout and spout calls.

         The remaining arguments must come in field,lcode pairs.
            The list must be ended by a null field pointer.

         'field' is a text string to be printed.
         'lcode' is a code made up by adding entries from the follow-
            ing table indicating the operations desired plus possibly
            the length of the field.  If a field length of 1 to 254 is
            added to the code, the lesser of that value or the actual
            length of the text string will be used.  A count of 255
            (RK_SKIP) will cause the field to be skipped.  A zero count
            will cause the actual length of the text string to be used.

         Code Name   Value  Operation
         RK_CCL          0  Continue current line.  The text string
                            should NOT begin with a carriage control.
         RK_SKIP       255  Omit this field--ignore other codes.
         RK_LNk   2048+256*k  Begin new line.  If there are fewer than
                            k lines remaining on the current page, a
                            page eject will occur (k <= 6).
         RK_NEWPG     3840  Begin new page unless in RK_PGNONE mode.
                            The text string should begin with a blank
                            carriage control to follow the page title.
         RK_OMITLN    4096  Omit this line if in RK_PGNONE mode.
         RK_SUBTTL    8192  This field and following fields define a
                            new subtitle.  The next cryout call begins
                            a new page unless in RK_PGNONE mode.  The
                            subtitle is not printed unless an ordinary
                            line follows before the next subtitle call.
                            Converted to RK_NFSUBTTL in RK_PGNONE mode.
         RK_NTSUBTTL 16384  Same as RK_SUBTTL except printing of this
                            subtitle is not triggered on next cryout
                            call nor is a new page forced.  Useful for
                            quietly clearing an existing subtitle.
         RK_NFSUBTTL 24576  Same as RK_SUBTTL except a new page is not
                            forced.
         RK_FLUSH    32768  Flush output buffer (see discussion below).

         The ANSI carriage control codes are: '+' to overstrike the
            previous line, ' ' to single space, '0' to double space,
            and '-' to triple space.  However, if RK_PGNONE mode is
            in effect, all overstrikes are changed to single lines.

         Buffer Flushing:  Normally, each output line is held pending
            until the next call that starts a new line (RK_LNk code).
            This permits single lines to be constructed from fields
            passed in multiple cryout calls (and is necessary for the
            implementation of ANSI carriage control characters).  An
            option code, RK_FLUSH, is provided to force a completed
            line to be written at once rather than held.  This code
            may be used with any line that needs to be written at once
            (e.g. an interactive prompt) and MUST be used with the
            last line of a run so that an incomplete line is not left
            in the buffer when execution terminates.  To flush with-
            out writing anything, execute
               cryout(RK_P1,"\0",RK_LN0+RK_FLUSH+1,NULL);
            Error messages and warnings are automatically flushed.
            The line following a flushed line cannot be an overstrike.

         RK variables:  RK.pglns contains the number of lines per
            printed page.  RK.pgcls and RK.ttcls contain the number
            of columns per line on printed and terminal output, bzw.
            These variables may be changed before any call to cryout
            is made, but not thereafter.  RK.rmlns and RK.rmcls con-
            tain, respectively, the number of lines remaining on the
            current page and the number of columns remaining in the
            current line.  RK.pgno contains the current page number.
            These variables may be read but never changed explicitly
            by the application program.

         64-bit compilations:  The rule is that fixed-point arguments
            are promoted to int if smaller than int, otherwise kept
            at the larger size.  cryout() expects ints for the lcodes.
            Therefore, an lcode that is a long or long long should
            never be passed.
*/

/*---------------------------------------------------------------------*
*        Error Procedures                                              *
*---------------------------------------------------------------------*/

/*
         1) If a line overflows, it is continued on the next line.
         2) If a subtitle is longer than the internal buffer (length
            set by parameter SBHDCH) the program is terminated with
            error code 050.
         3) If pagination occurs during subhead processing, the
            program is terminated with error code 051.
         4) If a line continuation calls for a new page, the new page
            will occur and will interrupt the continued line.
         5) I/O errors terminate execution via rfwrite protocols.
*/

/*---------------------------------------------------------------------*
*        Global definitions                                            *
*---------------------------------------------------------------------*/

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "rockv.h"

/* Title and subtitle buffers.  The defined parameter SBHDCH gives
*  the number of characters to be reserved for the subtitle buffer.
*  This number must include space for a few LFs.  */

   static struct titdef {     /* Title buffer definition */
      short lttl;                /* Length of title */
      short lsubttl;             /* Length of subtitle */
      short shdlns;              /* Lines in current subtitles */
      byte usecc;                /* 1 if line has ANSI controls */
      char tff[1];               /* Form feed to start title */
      char pid[30];              /* Program id */
      char title[L_TITL];        /* User title */
      char date[24];             /* Date */
      char pgwd[11];             /* Page number */
      char subttl[SBHDCH];       /* Subtitles */
      } titbuf = {            /* Define initial title and subtitle */
         132, 2, 1, 0, '\f',
         "                                    ",
   "                        TITLE NOT ENTERED                         ",
         "                :       ",
         "Page      \n",
         "  ",
         };
   static char monthname[12][4] = { " JAN"," FEB"," MAR"," APR",
      " MAY"," JUN"," JUL"," AUG"," SEP"," OCT"," NOV"," DEC" };

/*=====================================================================*
*                      ansicc(buf, rem, cc, kfl)                       *
*                                                                      *
*  Analyze an ANSI control character. If the previous line was already *
*  terminated with CR|LF ("flushed" byte set), add any extra spacing   *
*  needed (but it's too late for overstrike). If the previous line was *
*  not already terminated, do so now and add any extra spacing that is *
*  needed.  Skip adding extra lines if last line was a subtitle.       *
*                                                                      *
*  Arguments:                                                          *
*     buf      Location where output can be stored.                    *
*     rem      Remaining space in buf.                                 *
*     cc       ANSI carriage control character to be analyzed.         *
*     kfl      Flush indication -- artificial for subtitles            *
*                                                                      *
*  Returns: Number of characters added to buf.                         *
*=====================================================================*/

static int ansicc(char *buf, int rem, char cc, char kfl) {

   int irem = rem;         /* Remaining count */
   switch (cc) {
      case '+' :           /* Overstrike */
         if (RKC.ck_omitln) goto treat_as_single_line;
         if (--irem < 0) abexit(50);
         *buf++ = '\r';
      case '\0' :          /* NULL used by flush calls */
         break;
      case '-' :           /* Triple space */
         if (RKC.PU.trigger == 2) goto treat_as_single_line;
         if (--irem < 0) abexit(50);
         *buf++ = '\n';
      case '0' :           /* Double space */
         if (RKC.PU.trigger == 2) goto treat_as_single_line;
         if (--irem < 0) abexit(50);
         *buf++ = '\n';
      case ' ' :           /* Plain new line */
      default :
treat_as_single_line:
         if (!kfl) {
            if (--irem < 0) abexit(50);
            *buf++ = '\n';
            }
      } /* End switch on ANSI control character */

   return (rem - irem);
   } /* End ansicc() */

/*=====================================================================*
*                         endprt(ublk, kffl)                           *
*                                                                      *
*  Flush the current print or spout buffer (unless already flushed).   *
*  Arguments are:                                                      *
*        ublk = pointer to unit_def structure defining unit            *
*        kffl = TRUE to call rflush for this unit                      *
*=====================================================================*/

static void endprt(struct unit_def *ublk, int kffl) {

   if (!ublk->flushed) {
      rfwrite(&ublk->rfd, "\n", 1, ABORT);   /* Flush the line */
      ublk->flushed = 1;                     /* So indicate */
      ublk->rmcls = ublk->pgcls; /* Reset remaining columns */
      }
   if (kffl) rfflush(&ublk->rfd, ABORT);
   } /* End endprt() */

/*=====================================================================*
*                       prnmov(ublk, pd, width)                        *
*                                                                      *
*  Move a line to the output, removing multiple LFs if ublk is SPOUT,  *
*  and breaking it up into overflow lines if necessary. Arguments are: *
*        ublk = pointer to unit_def structure defining unit            *
*        pd = pointer to data field to be printed                      *
*        width = width of data field                                   *
*=====================================================================*/

static void prnmov(struct unit_def *ublk, char *pd, int width) {

   char *wpd,*wpde = pd + width;    /* String, end of string */
   int  ww,rm = (int)ublk->rmcls;   /* Width,remaining width */
   while (pd < wpde) {
      /* If spout, skip multiple LFs at start of string */
      if (ublk->type) while (pd < wpde && *pd == '\n') ++pd;
      wpd = pd;
      /* Check for LF within the string */
      while (wpd < wpde) { if (*wpd++ == '\n') break; }
      ww = wpd - pd;
      while (ww > rm) {             /* Line overflow */
         rfwrite(&ublk->rfd, pd, rm, ABORT);    /* Write fragment */
         rfwrite(&ublk->rfd, "\n", 1, ABORT);   /* Add end of line */
         pd += rm, ww -= rm;        /* Update pointer,width */
         rm = ublk->pgcls;          /* Reset rmcls */
         RK.rmlns -= ublk->type ^ 1;/* Count line if !spout */
         } /* End overflow line processing */
      rfwrite(&ublk->rfd, pd, ww, ABORT); /* Write last fragment */
      pd += ww, rm -= ww;
      } /* End while (pd < wpde) */
   /* Indicate whether or not material is pending */
   ublk->rmcls = (ublk->flushed = wpde[-1] == '\n') ? ublk->pgcls : rm;
   } /* End prnmov() */

/***********************************************************************
*                   prnlin(ublk, pd, width, lcode)                     *
*                                                                      *
*  Deal with one field of normal or spout output.  Determine whether   *
*  a subtitle has been triggered, and if so, output it, then take care *
*  of the field itself.  Carriage control characters will be treated   *
*  appropriately depending on whether the call is for print or spout.  *
*  Arguments are:                                                      *
*        ublk = pointer to unit_def structure defining unit            *
*        pd = pointer to data field to be printed                      *
*        width = width of data field                                   *
*        lcode = line code from cryout call                            *
*  This routine must be visible from erprnt in ermark.c, so it is not  *
*  declared static.                                                    *
***********************************************************************/

void prnlin(struct unit_def *ublk, char *pd, int width, short lcode) {

/* If a subtitle has been triggered for this output unit,
*  go ahead and transfer the subtitle to the output */

   if (ublk->trigger && titbuf.lsubttl > 0) {
      endprt(ublk, NO);       /* Finish any pending line */
      if (ublk->type)         /* SPOUT skips multiple LFs */
         prnmov(ublk, titbuf.subttl, titbuf.lsubttl);
      else
         rfwrite(&ublk->rfd, titbuf.subttl, titbuf.lsubttl, ABORT);
      ublk->flushed = 0;
      if (!(titbuf.usecc)) endprt(ublk, NO);
      ublk->trigger = 2;      /* Skip extra blank lines */
      }

/* Proceed to current line.  If it is not a continued line, action
*  depends on whether ANSI control characters are present.
*  Using ANSI CC:  If this is spout, just terminate previous line
*     and output this with no extras.  If not spout, analyze CC,
*     and output requested LFs.  Then step over the CC.
*  Using UNIX LF:  Just output the data.  prnmov() will delete any
*     included LF if this is spout in either case.  */

   if ((lcode & RK_NEWPG) != RK_CCL) {
      if (ublk->type) endprt(ublk, NO);
      if (titbuf.usecc) {        /* Using ANSI CC */
         if (!ublk->type) {
            char accpfx[LLNCCH]; /* Space for extra LF from ansicc */
            int ncc = ansicc(accpfx, (int)LLNCCH, *pd, ublk->flushed);
            if (ncc > 0) {
               ublk->rmcls = ublk->pgcls;
               prnmov(ublk, accpfx, ncc);
               }
            }
         pd++;                   /* Step over cc character */
         width--;                /* Decrement width */
         }
      }

/* Process the field.  Test for line overflow.  If the current
*  item fits, just move it to the output.  If the line is too long,
*  print what will fit, then continue remainder on another line.
*  It is possible that the line overflow will cause a page overflow.
*  The definition of the cryout call makes it impossible in general
*  to anticipate this condition in time to start a new page BEFORE
*  the overflow occurs.  Accordingly, we just count lines in prnmov
*  and pick up the new page at the start of the next line. */

   prnmov(ublk, pd, width);   /* Line to output */
   ublk->trigger = 0;         /* Subtitle effects completed */
   } /* End prnlin() */

/***********************************************************************
*                                                                      *
*                cryout(sprty, field, code, ..., NULL)                 *
*                                                                      *
***********************************************************************/

void cryout(int sprty, ...) {

/* Local declarations: */

   va_list ap;                /* Argument pointer */
   char *pd;                  /* Pointer to current data */
   char *pt;                  /* Pointer to subtitle buffer--
                              *  NULL if not processing subtitle */
   int   lcode;               /* Code for current string */
   int   rmshc;               /* Remaining subhead characters */
   int   width;               /* Field width */
   short lincd;               /* Line code extracted from lcode */
   short linct;               /* Line count extracted from lcode */
   short scode;               /* Subhead code extracted from lcode */
   byte  flushtrig;           /* Flush trigger */
   byte  sttkfl;              /* kfl arg for subtitles */

/*---------------------------------------------------------------------*
*        Initialization                                                *
*---------------------------------------------------------------------*/

/* First time through:  initialize rfdefs, spouting,
*  get the current date and convert it for printing */

   if (!(RKC.PU.rfd.iamopen & IAM_ANYOPEN)) {
      time_t start_time;         /* Starting time value */
      struct tm *ptm;            /* Time pointer */
      int    fmtcode;            /* rfopen format code */
      int    tyr;                /* Temp for year % 100 */

      fmtcode = RKC.PU.type == 2 ? TEXT: SysOut;
      rfopen(&RKC.PU.rfd, NULL, WRITE, fmtcode, SEQUENTIAL, TOP,
         NO_LOOKAHEAD, REWIND, RELEASE_BUFF, IGNORE, IGNORE,
         IGNORE, ABORT);         /* Open print file */
      RKC.PU.type = 0;
      rfopen(&RKC.SU.rfd, NULL, WRITE, SysErr, SEQUENTIAL, TOP,
         NO_LOOKAHEAD, REWIND, RELEASE_BUFF, IGNORE, IGNORE,
         IGNORE, ABORT);         /* Open spout file */
      RKC.PU.pgcls = RK.pgcls;   /* Working copy of pgcls */
      RKC.SU.pgcls = RK.ttcls;
      if (rfqsame(&RKC.PU.rfd, &RKC.SU.rfd))
         spout(SPTM_GBLOFF);     /* Spout off and stays off */
      time(&start_time);         /* Get "calendar time" */
      ptm = localtime(&start_time); /* Break down time info */
      wbcdwt(&ptm->tm_mday, titbuf.date, RK_IORF|RK_NINT|3);
      memcpy(&titbuf.date[4], monthname[ptm->tm_mon], 4);
      titbuf.date[8] = ' ';
      tyr = ptm->tm_year%100;
      wbcdwt(&tyr, &titbuf.date[9], RK_IORF|RK_NINT|RK_NPAD0|1);
      } /* End once-only date processing */

/* If error codes have been registered, call erprnt, which calls
*  cryout recursively to print the messages */

   if (RKC.accpt & ACCPT_ERP) erprnt();

/* Decode the sprty argument, increment the spout count, then
*  skip to the end if the priority is below the current oprty. */

   {  int spct = sprty & 255;
      if (spct > 0) spout(spct); }
   RK.iexit |= ((unsigned)sprty >> 12);
   if ((((unsigned)sprty >> 8) & 7) > RKC.oprty) goto skip_print;

/* Set CC mode, clear subhead mode flag and flush trigger */

   {  byte notusecc = (sprty & RK_PF) != 0;
      if (notusecc && titbuf.usecc) {  /* Switching CC mode */
         endprt(&RKC.PU, NO);
         if (RKC.spend >= SPTM_NONE) endprt(&RKC.SU, NO); }
      titbuf.usecc = !notusecc;
      }  /* End notusecc local scope */
   pt = NULL;
   flushtrig = 0;

/*---------------------------------------------------------------------*
*        Scan output list                                              *
*---------------------------------------------------------------------*/

/* Skip items with RK_SKIP set or with RK_OMITLN set when in RK_PGNONE
*  mode.  Determine width of current item from code or via strlen.  */

   va_start(ap, sprty);
   while ((pd = va_arg(ap, char *)) != NULL) {
      lcode = va_arg(ap, int);
      if (lcode & RK_FLUSH) flushtrig = ON;
      if ((width = lcode & 255) == RK_SKIP) continue;
      if (lcode & RKC.ck_omitln)            continue;
      if (width == 0) width = strlen(pd);
      else            width = strnlen(pd, width);
      if (width == 0) continue;  /* In case pd is empty */

/* Determine whether line is a new subtitle entry */

      lincd = lcode & RK_NEWPG;     /* Pick up line code */
      linct = (lcode >> 8) & 7;     /* Pick up line count */
      scode = lcode & RK_NFSUBTTL;  /* Pick up subhead code */
      if (pt) goto titrec;          /* Add a subtitle line */
      if (scode) goto subtit;       /* Handle new subtitle */

/* Check for conditions that trigger a new page.  Since only print,
*  and not spout, output is paginated, this code is placed here and
*  not in prnlin, which is called for both outputs.  This placement
*  also removes need for linct, etc. to be global variables.
*     Subtitles, however, go into both outputs, and are handled in
*  prnlin.  */

      if (RKC.PU.nopaginate & RK_PGNONE) goto nwpg_return;
      if (lincd == RK_NEWPG) goto newpag; /* New page forced */
      if (!RKC.PU.nopaginate) {
         int trmln = RK.rmlns - linct;
         if (trmln < 0) goto newpag;
         if (RKC.PU.trigger && (trmln - titbuf.shdlns <= 4)) goto newpag;
         }
      goto nwpg_return;       /* Return to pending print line */

/*---------------------------------------------------------------------*
*        Begin a new page with a title line                            *
*---------------------------------------------------------------------*/

   newpag: {
      double elapsed;         /* Elapsed time */
      si32 minutes;           /* Elapsed minutes */

/* Get the elapsed time and convert it to minutes and seconds;
*  Increment and convert the page number */

      elapsed = second();
      minutes = (si32)(elapsed/60.0);
      wbcdwt(&minutes, titbuf.date+11, RK_IORF|RK_NI32|4);
      bcdout((3<<RK_DS)+RK_IORF+RK_NPAD0+4, titbuf.date+17,
         elapsed - 60.0*(double)minutes);
      RK.pgno += 1;
      wbcdwt(&RK.pgno, titbuf.pgwd+5, RK_LFTJ|RK_IORF|RK_NHALF|4);

/* Output the title line and set trigger for subtitle print */

      endprt(&RKC.PU, NO);       /* Be sure last line flushed */
      rfwrite(&RKC.PU.rfd, titbuf.tff, titbuf.lttl, ABORT);
      RKC.PU.rmcls = RKC.PU.pgcls;
      RKC.PU.flushed = RKC.PU.trigger = 1;

/* Reset the line count, using value possibly changed by user */

      RK.rmlns = RK.pglns - 1;
      } /* End newpag routine */
   nwpg_return:               /* newpag returns here */

/*---------------------------------------------------------------------*
*        Normal line processing                                        *
*---------------------------------------------------------------------*/

/* If line is an error message or warning, bump spout count.
*  If line is an error message, set flush trigger.  (It is
*     tempting here to set RK.iexit, but don't do it because
*     there is no way to know which bit should be set.  This
*     is now handled by RK_Ex codes in the sprty argument.)
*  Only test at start of line and with at least 3 chars.  */

      if (lincd != RK_CCL && width > 3) {
         char *pd1 = pd + titbuf.usecc; /* Ptr to text after any CC */
         if (!memcmp(pd1, "***", 3)) {
            flushtrig = 1; spout(1); }
         else if (!memcmp(pd1, "-->", 3))
            spout(1);
         } /* End if not continuation */

/* Process the line for print */

      prnlin(&RKC.PU, pd, width, lcode);

/* If spouting is to occur, reprocess the line for spout */

      if (RKC.spend > SPTM_NONE) prnlin(&RKC.SU, pd, width, lcode);

/* Proceed to next field */

      continue;

/*---------------------------------------------------------------------*
*        Store a new subtitle for later use                            *
*                                                                      *
*  Prepare to store a subtitle:                                        *
*  1) Enter subhead storage mode and set flags to trigger subhead      *
*     print and spout next time a normal line occurs.                  *
*  2) If the subtitle is not coded to occur immediately, trigger a     *
*     new page.                                                        *
*---------------------------------------------------------------------*/

   subtit:                    /* Initiate storing new subtitle */
      rmshc = SBHDCH;         /* Count spaces used */
      if (scode & RK_SUBTTL) RKC.PU.trigger = RKC.SU.trigger = 1;
                              /* Trigger print and spout subhead */
      if (scode == RKC.ck_subttl) RK.rmlns = -1;
                              /* Trigger new page unless immed */
      pt = titbuf.subttl;     /* Locate subtitle buffer; nonNULL
                              *  value is subtitle mode flag */
      titbuf.shdlns = (lincd == RK_CCL);  /* Count a line if
                              *  incorrectly coded as an RK_CCL */
      sttkfl = TRUE;          /* Makes ansicc omit one LF */
      /* Fall through to titrec... */

/* Store one field of a new subtitle */

   titrec:
      if (lincd == RK_NEWPG) abexit(51);     /* Bad pagination code */
      if (titbuf.usecc && lincd != RK_CCL) { /* Handle new lines */
         /* Convert ANSI CC to newlines in subhead buffer.
         *  (ncc = number of control chars) */
         int ncc = ansicc(pt, rmshc, *pd++, sttkfl);
         pt += ncc, rmshc -= ncc, width -= 1;
         } /* End counting for new lines */
      sttkfl = 0;             /* Resume normal LF counting */

/* Move the field into the subtitle buffer and do the bookkeeping */

      titbuf.shdlns += linct; /* Increment line count */
      if ((rmshc -= width) < 0) abexit(50);
      memcpy(pt, pd, width);
      pt += width, titbuf.lsubttl = SBHDCH - rmshc;

/*---------------------------------------------------------------------*
*        End list processing loop                                      *
*---------------------------------------------------------------------*/

      }
   va_end(ap);
   /* Indicate that last thing printed was not current control card */
   RKC.accpt &= ~(ACCPT_CLP|ACCPT_ACP);

/* If flush trigger was set, now flush both buffers.  Flushing is
*  deferred from the point of occurrence lest an error message with
*  RK_CCL continuations be split prematurely. */

   if (flushtrig) {
      endprt(&RKC.PU, YES);
      }
   if (RKC.spend >= SPTM_NONE) endprt(&RKC.SU, YES);

/* Finish up:  Store columns remaining in RK.rmcls, decrement spout
*  count and return.  The spout count must be decremented, but not
*  if it would change the sign, i.e. do an ON/OFF switch.  Decrement
*  even if output was skipped due to low priority to assure consistent
*  results across different values of OUTPRTY.  */

skip_print:
   RK.rmcls = RKC.PU.rmcls;
   if (RKC.spend > SPTM_NONE) RKC.spend--;
   return;
   } /* End cryout() */

/***********************************************************************
*                              cryocls()                               *
*                                                                      *
*  Call cryocls() to flush any pending output, close stdout and stderr *
*  (if used), and release any dynamic storage associated with cryout().*
***********************************************************************/

void cryocls(void) {

   cryout(RK_P1, "\0", RK_LN0+RK_FLUSH+1, NULL);
   rfclose(&RKC.PU.rfd, NO_REWIND, RELEASE_BUFF, NOMSG_ABORT);
   rfclose(&RKC.SU.rfd, NO_REWIND, RELEASE_BUFF, NOMSG_ABORT);
   
   } /* End cryocls() */

/***********************************************************************
*                              tlines(n)                               *
*                                                                      *
*  To guarantee that a group of n lines ends up on the same page,      *
*  use the cryout lines code 'k' if k<7.  Otherwise, use function      *
*  tlines.                                                             *
*                                                                      *
*  Usage: int tlines(int n)                                            *
*                                                                      *
*  'n' is the number of lines to be kept together.  Returns the        *
*     number of lines left on the current page.                        *
***********************************************************************/

int tlines(int n) {

   int lleft = RK.rmlns - n;  /* Number of lines left */
   if (lleft < 0) RK.rmlns = -1, lleft = 0;
   return lleft;
   } /* End tlines() */

/***********************************************************************
*                              lines(n)                                *
*                                                                      *
*   Function lines is similar to tlines except it may be called when   *
*   cryout is not being used to print the lines in question.  It       *
*   updates RK.rmlns, then flushes the cryout buffer.  It actually     *
*   prints titles and subtitles when a new page is required, using     *
*   a magic call to cryout with a null carriage control character      *
*   and no data field.                                                 *
*                                                                      *
*   Usage: int lines (int n);                                          *
*                                                                      *
*   'n' is the number of lines to be kept together.  Returns the       *
*    number of lines left on the current page.                         *
***********************************************************************/

int lines(int n) {

   int kode = RK_LN0 + RK_FLUSH + 1;
   if ((RK.rmlns -= n) < 0) {
      RK.rmlns = -1;
      kode += RK_NEWPG; }
   cryout(RK_P1, "\0", kode, NULL);
   return RK.rmlns;
   } /* End lines() */

/***********************************************************************
*                              spout(n)                                *
*                                                                      *
*  Subroutine spout is used to control the summary print out feature   *
*  which duplicates selected lines of output to stderr for on-line     *
*  examination by the user.  Activation of the spout output is         *
*  controlled by the SPOUT control card interpreted by cryin.          *
*                                                                      *
*  Usage: void spout(int n)                                            *
*                                                                      *
*     If n == SPTM_GBLOFF, spouting is being globally turned off by    *
*           a command-line parameter, or because stderr == stdout,     *
*           and all later calls are ignored.                           *
*     If n == SPTM_LCLOFF, spouting is being locally turned off by     *
*           a cryin() SPOUT card or some application option.  Later    *
*           spout calls are ignored until SPTM_LCLON is received.      *
*           (This is the default situation at start up.)               *
*     If n == SPTM_LCLON, SPTM_LCLOFF is converted to SPTM_NONE.       *
*     If n == SPTM_NONE, the LCLOFF|LCLON status is not changed,       *
*           but the pending count is set to 0.                         *
*     If n > 0, and the pending count is >= 0, then the print from     *
*           the next 'n' cryout calls is sent to stderr in addition    *
*           to stdout.  All carriage controls are ignored.  Subtitles  *
*           are written only once, before the next output.             *
*     If n == SPTM_ALL, spouting is on for all subsequent cryout       *
*           calls until SPTM_NONE or below is received.                *
*     Note: If spout is active, then all error messages and warnings   *
*           (lines beginning with '***' or '-->') and all control      *
*           cards marked by ermark() are sent to spout automatically.  *
***********************************************************************/

void spout(int n) {

   /* Ignore all calls if spouting is globally turned off */
   if (RKC.spend == SPTM_GBLOFF) return;

   switch (n) {
   case SPTM_GBLOFF:          /* Globally turn spout off */
   case SPTM_LCLOFF:          /* Locally turn spout off */
      RKC.spend = n;
      break;
   case SPTM_LCLON:           /* Locally turn spout on  */
      if (RKC.spend < SPTM_NONE) RKC.spend = SPTM_NONE;
      break;
   case SPTM_NONE:            /* Locally reset count to 0 */
   case SPTM_ALL:             /* Set to spout everything */
      if (RKC.spend > SPTM_LCLOFF) RKC.spend = n;
      break;
   default:
      if (RKC.spend <= SPTM_LCLOFF) break;
      if (RKC.spend == SPTM_LCLON) RKC.spend = 0;
      if (RKC.spend < SPTM_ALL)  RKC.spend += n;
      break;
      } /* End n switch */
   } /* End spout() */

/***********************************************************************
*                            nopage(kpage)                             *
*                                                                      *
*  Globally controls pagination and underlining of cryout output.      *
*                                                                      *
*  Usage: void nopage(int kpage)                                       *
*                                                                      *
*  If kpage == RK_PGNONE, all pagination is suspended.  All lines      *
*  with RK_OMITLN code are skipped.  All triggerable subtitles are     *
*  converted into immediate subtitles.  Lines with RK_NEWPG are        *
*  treated like ordinary single lines.                                 *
*                                                                      *
*  If kpage == RK_PGLONG, pagination is suspended, but the other       *
*  actions of RK_PGNONE do not happen.  This option is for making      *
*  deliberately long pages, as in maps of various kinds.               *
*                                                                      *
*  If kpage == RK_PGNORM, the RK_PGLONG bit is turned off, but the     *
*  RK_PGNONE bit is not affected.  There is currently no way to        *
*  exit from RK_PGNONE mode.  This would require a SPTM_LCLON analog.  *
*                                                                      *
*  For other values of kpage, the action is undefined, but is one      *
*  of the above.                                                       *
***********************************************************************/

void nopage(int kpage) {

   int kmode = kpage & (RK_PGNONE|RK_PGLONG);

   if (kpage & RK_PGNONE)
      RKC.ck_omitln = RK_OMITLN, RKC.ck_subttl = 0;
   if (kmode)
      RKC.PU.nopaginate |= kmode;
   else
      RKC.PU.nopaginate &= ~RK_PGLONG;

   } /* End nopage() */

/***********************************************************************
*                           settit(utitle)                             *
*                                                                      *
*  Change the current page title to a user-provided string.  This      *
*  call only saves the title for later use, it does not print it.      *
*  Normally, settit is called only once at the beginning of a run      *
*  to provide a default title--cryin handles user TITLE cards.         *
*                                                                      *
*  Usage: void settit(char *title)                                     *
*                                                                      *
*  'title' is an image of the TITLE card.  The title, beginning in     *
*  column 7 of the TITLE card and extending a maximum of L_TITL chars  *
*  (traditionally, L_TITL = 60, now 66), is transferred to the title   *
*  buffer, which is padded with blanks.                                *
***********************************************************************/

void settit(char *utitle) {

   size_t titlen;             /* Length of user title */

   utitle += 6;               /* Skip over the word "TITLE" */
   strncpy(titbuf.title, utitle, sizeof(titbuf.title));
   if ((titlen = strlen(utitle)) < sizeof(titbuf.title))
      memset(titbuf.title+titlen, ' ', sizeof(titbuf.title)-titlen);
   return;
   } /* End settit() */

/***********************************************************************
*                              gettit()                                *
*                                                                      *
*  This function returns a pointer to the current title string.        *
***********************************************************************/


char *gettit(void) {

   return titbuf.title;
   } /* End gettit() */

/***********************************************************************
*                             setpid(pid)                              *
*                                                                      *
*  Change the current program id to a user-provided string.  This      *
*  call only saves the pid for later use, it does not print titles.    *
*  Normally, setpid is called only once at the beginning of a run.     *
*                                                                      *
*  Usage: void setpid(char *pid)                                       *
*                                                                      *
*  'pid' is the program id.  The maximum length of 'pid' is 30.        *
***********************************************************************/

void setpid(char *upid) {

   size_t pidlen;             /* Length of user pid */

   strncpy(titbuf.pid, upid, sizeof(titbuf.pid));
   if ((pidlen = strlen(upid)) < sizeof(titbuf.pid))
      memset(titbuf.pid+pidlen, ' ', sizeof(titbuf.pid)-pidlen);
   return;
   } /* End setpid() */

/***********************************************************************
*                              getdat()                                *
*                                                                      *
*  This function returns a pointer to a string giving the date         *
*  at which execution began.                                           *
***********************************************************************/

char *getdat(void) {

   return titbuf.date;
   } /* End getdat() */

