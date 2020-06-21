/* (c) Copyright 1989-2018, The Rockefeller University *21115* */
/* $Id: d3exit.c 79 2018-08-21 19:26:36Z  $ */
/***********************************************************************
*                             CNS Program                              *
*          Routines relating to exits and message formatting           *
*                                                                      *
*    d3exit - Shutdown computation with message info                   *
*    abexit - Intercept generic ROCKS errors, convert to d3exit        *
*   abexitq - Same as abexit() except returns if this is a test run    *
*   abexitm - Intercept generic ROCKS errors with messages             *
*  abexitmq - Same as abexitm() except returns if this is a test run   *
*  abexitme - Intercept generic ROCKS errors with messages and errno   *
*  fmtlrlnm - Format lowercase rep & layer names for passing to host   *
*  fmturlnm - Format uppercase rep & layer names for passing to host   *
*  fmturlcn - Format uppercase rep & layer names with cell number      *
*  fmtsrcnm - Format source name                                       *
*                                                                      *
*  These routines are part of the standard shutdown procedure for CNS. *
*  They are called by any node that wants to terminate execution with  *
*     an error message to the user.                                    *
*  Arguments to d3exit are:                                            *
*     (1) A pointer to a string with error information, or NULL.       *
*         Meaning is specific to the particular error.                 *
*         Usually points to an identifying string that will be         *
*         incorporated in the text of the error message.               *
*     (2) An integer error code (documented in d3global.h)             *
*     (3) An integer containing information specific to the            *
*         particular error.                                            *
*                                                                      *
*  CNS now has provision for two kinds of error exits:                 *
*  -- d3grex can be called when a graceful exit is possible, that is,  *
*     the nature of the error does not preclude continuing to the      *
*     next rendezvous point, where the error is recognized and CNS     *
*     is terminated with cleanup.                                      *
*  -- d3exit (this code) should be called when the state of CNS may    *
*     not support continuing to rendezvous.  In this case, termina-    *
*     tion is immediate.  In serial versions, d3exit() completes the   *
*     error exit after calling d3term() to perform file and graphics   *
*     cleanup.  On a parallel computer, MPI_Abort is called and no     *
*     attempt is made to perform cleanup.                              *
*                                                                      *
*  Arguments to abexit[m][e][q]() are documented in apndx5.crk         *
************************************************************************
*  V1A, 08/31/89, G. N. Reeke                                          *
*  Rev, 07/26/90, JMS - Make node 0 d3exit call new function d3term()  *
*                       Make a local abexit() to intercept ROCKS errs  *
*  V2A, 12/29/92, ABP - New mechanism, new d3exit() interface          *
*  V8A, 11/27/96, GNR - Remove support for non-hybrid version          *
*  Rev, 11/06/97, GNR - Add support for abexitm                        *
*  Rev, 09/19/99, GNR - Add abexitme                                   *
*  Rev, 09/03/06, GNR - Move fmtlrlnm,fmturlnm from d3rlnm for PAR     *
*  V8D, 02/29/08, GNR - Eliminate abexitme loops                       *
*  ==>, 02/29/08, GNR - Last mod before committing to svn repository   *
*  Rev, 04/29/08, GNR - Add fmturlcn                                   *
*  Rev, 01/06/13, GNR - Add fmtsrcnm                                   *
*  R67, 04/25/16, GNR - Change longs in call to ints after mtxt        *
*  R67, 04/27/16, GNR - Remove Darwin 1 support                        *
*  R67, 09/16/16, GNR - Add abexitm?q to replace crk library version   *
*  R70, 01/17/17, GNR - Complete rewrite, separated d3grex for use     *
*                       when controlled cleanup is possible            *
*  R78, 03/31/18, GNR - Remove SUNOS/SOLARIS support                   *
*  R79, 08/10/18, GNR - Improve abexloop setting/testing               *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "sysdef.h"
#ifndef PARn
#include "rocks.h"
#include "rockv.h"
#include "rkxtra.h"
#else
#include "rksubs.h"
#endif
#include "d3global.h"
#include "plots.h"

void d3term(struct exit_msg *, int);

int abexloop;        /* Initially zero by rules of C language */

/*=====================================================================*
*                               d3exit                                 *
*=====================================================================*/

void d3exit(char *mtxt, int ierr, int ival) {

#ifdef PAR           /*** PARALLEL d3exit ***/
   /* By definition (otherwise d3grex would have been called), there
   *  is no way to continue to a rendezvous on the current node and
   *  d3exit has attribute noreturn.  Send the message to andmsg to
   *  convey to user via stderr and terminate with MPI_Abort.  */
   struct {
      struct ErrMsg Err;
      char msg[MAX_EMSG_LENGTH];
      } emsg;
   volatile int waittest = 1;    /* To avoid optimization */
   emsg.Err.ec = ierr;
   emsg.Err.src = NC.node;
   emsg.Err.iv = ival;
   if (mtxt) {
      emsg.Err.ltxt = strlen(mtxt) + 1;
      strncpy(emsg.msg, mtxt, MAX_EMSG_LENGTH);
      }
   else {
      emsg.Err.ltxt = 0;
      emsg.msg[0] = '\0';
      }
   MPI_Send(&emsg, sizeof(struct ErrMsg) + emsg.Err.ltxt,
      MPI_UNSIGNED_CHAR, NC.dmsgid, INIT_ABORT_MSG, NC.commd);
   /* And wait for andmsg to abort everything */
   while (waittest)
      sleep(1);
   exit(emsg.Err.ec);   /* Should never get here */

#else                /*** SERIAL d3exit ***/
   /* Prepare an exit_msg in safe memory */
   struct exit_msg d3ex;
   char tmsg[MAX_EMSG_LENGTH+1];
   if (abexloop) abort();
   abexloop = TRUE;
   if (mtxt) {
      strncpy(tmsg, mtxt, MAX_EMSG_LENGTH);
      d3ex.pit = tmsg; }
   else d3ex.pit = NULL;
   d3ex.ec = ierr;
   d3ex.iv = ival;

   d3term(&d3ex, 0); /* Cleanup and print message */
   /* Implement "ERROR DUMP REQUESTED" */
   if (RKC.kparse & RKKP_EDMPRQ) abort();
   else exit(ierr);

#endif

   } /* End d3exit */

/*=====================================================================*
*          abexit, abexitq, abexitm, abexitmq, and abexitme            *
*  This code replaces the ROCKS abexit(), abexitq(), abexitm(),        *
*  abexitmq(), and abexitme() functions for CNS.  It diverts these     *
*  calls from the library routines, which know nothing about parallel  *
*  termination arrangements for CNS, to d3exit.                        *
*                                                                      *
*  Note:  Do not use mallocv in these routines because may be handling *
*  a mallocv error message.  Do not use ssprintf internal buffer with  *
*  the abexitm varieties, because caller message may be there, but it  *
*  is OK to use ssprintf with a supplied buffer.                       *
*=====================================================================*/

void abexit(int code) {

   if (abexloop) {
#ifndef PARn
      if (RKC.kparse & RKKP_EDMPRQ) abort(); else
#endif
      exit(code);
      }
   abexloop = TRUE;
   d3exit(NULL, ABEXIT_ERR, code);

   } /* End abexit() */

void abexitq(int code) {

   if (e64qtest()) {
      /* This is a test.  Print message and return.  */
      char *pmsg,tmsg[LNSIZE];
      pmsg = ssprintf(tmsg,"==>Exit message %d intercepted for test.",
         code);
#ifdef PARn
      dbgprt(" ");
      dbgprt(pmsg);
#else
      cryout(RK_P1, "0", RK_LN2+1, pmsg, RK_CCL+RK_FLUSH+1, NULL);
#endif
      return;
      }
   else
      abexit(code);
   } /* End abexitq() */

void abexitm(int code, char *emsg) {
   char tmsg[LNSIZE];

   if (abexloop) {
#ifndef PARn
      if (RKC.kparse & RKKP_EDMPRQ) abort(); else
#endif
      exit(code);
      }
   abexloop = TRUE;
   strcpy(tmsg, "0***");
   strncat(tmsg, emsg, LNSIZE-4);
   d3exit(tmsg, ABEXIT_ERR, code);

   } /* End abexitm() */

void abexitmq(int code, char *emsg) {

   if (e64qtest()) {
      /* This is a test.  Print message and return.  */
      char *pmsg,tmsg[LNSIZE];
      pmsg = ssprintf(tmsg, "==>Exit message %d intercepted for test:",
         code);
#ifdef PARn
      dbgprt(" ");
      dbgprt(pmsg);
      strcpy(tmsg, "***");
      strncat(tmsg, emsg, LNSIZE-4);
      dbgprt(tmsg);
#else
      cryout(RK_P1, "0", RK_LN3+1, pmsg, RK_CCL,
         " ***", RK_LN0+4, emsg, RK_CCL+RK_FLUSH, NULL);
#endif
      return;
      }
   else
      abexitm(code, emsg);
   } /* End abexitmq() */

void abexitme(int code, char *emsg) {

#ifndef PARn
   int sverr = errno;
   char ssmsg[LNSIZE];     /* Buffer for system error msg */
#endif
   /* The message has to be copied before using ssprintf(), because
   *  it is highly likely that the msg argument is itself a pointer
   *  to the ssprintf buffer... Don't malloc this space, because we
   *  may be handling a malloc error or already out of heap space.  */
   char tmsg[LNSIZE];

   if (abexloop) {
#ifndef PARn
      if (RKC.kparse & RKKP_EDMPRQ) abort(); else
#endif
      exit(code);
      }
   abexloop = TRUE;
   strcpy(tmsg, "0***");
   strncpy0(tmsg+4, emsg, LNSIZE-5);
#ifdef PARn             /* No errno on comp nodes */
   d3exit(tmsg, ABEXIT_ERR, code);
#elif defined(LINUX)
   strcpy(ssmsg, " ***");
   /* It would seem logical to use strerror_r here, but that
   *  function only seems to work when compiled for multithread */
   strncpy0(ssmsg+4, strerror(sverr), LNSIZE-5);
   cryout(RK_P1, tmsg, RK_LN3+LNSIZE-1,
      ssmsg, RK_LN0+RK_FLUSH, NULL);
   d3exit(NULL, ABEXIT_ERR, code);
#else
   d3exit(tmgs, ABEXIT_ERR, code);
#endif

   } /* End abexitme() */

/*---------------------------------------------------------------------*
*                    fmtlrlnm, fmturlnm, fmturlcn                      *
*  These little functions format repertoire and layer names for in-    *
*  clusion in a warning message or an error message.  The action is    *
*  very simple, but is always done in this one place so all messages   *
*  will be formatted consistently.  These routines are available on    *
*  comp nodes.                                                         *
*  fmtlrlnm returns, for a celltype il:                                *
*     Ptr to lowercase formatted string "Celltype (sname,lname)".      *
*  fmturlnm returns, for a celltype il:                                *
*     Ptr to uppercase formatted string "CELLTYPE (sname,lname)".      *
*  fmturlcn returns, for celltype il and cell icell:                   *
*     Ptr to formatted string "CELLTYPE (sname,lname), CELL icell".    *
*  Result string is in static storage, and remains valid until next    *
*     call to any of these routines.                                   *
*---------------------------------------------------------------------*/

char fmtrlmsg[20+2*LSNAME+LONG_SIZE];
#if LXRLNM < 14 + 2*LSNAME
#error "LXRLNM (d3global.h) is too small."
#endif

char *fmtlrlnm(struct CELLTYPE *il) {

   ssprintf(fmtrlmsg, "Celltype (%*s,%*s)",
      (int)LSNAME, il->pback->sname, (int)LSNAME, il->lname);
   return fmtrlmsg;
   } /* End fmtlrlnm() */

char *fmturlnm(struct CELLTYPE *il) {

   ssprintf(fmtrlmsg, "CELLTYPE (%*s,%*s)",
      (int)LSNAME, il->pback->sname, (int)LSNAME, il->lname);
   return fmtrlmsg;
   } /* End fmturlnm() */

char *fmturlcn(struct CELLTYPE *il, int icell) {

   ssprintf(fmtrlmsg, "CELLTYPE (%*s,%*s), CELL %*d",
      (int)LSNAME, il->pback->sname, (int)LSNAME, il->lname,
      (int)UI32_SIZE, icell);
   return fmtrlmsg;
   } /* End fmturlnm() */

/*---------------------------------------------------------------------*
*                              fmtsrcnm                                *
*  This function returns a pointer to a formatted message describing   *
*  the source and type fields in a given rlnm for use in error or      *
*  warning messages.  The format depends on the type of source.  The   *
*  result string is in static storage and remains valid only until the *
*  next call to this routine.                                          *
*---------------------------------------------------------------------*/

#ifndef PARn
char *fmtsrcnm(rlnm *srcnm, int ityp) {

   static char fmtsrmsg[32];

   strcpy(fmtsrmsg, "from ");
   switch (ityp) {
   case REPSRC:
      ssprintf(fmtsrmsg+5, "celltype (%*s,%*s)",
         (int)LSNAME, srcnm->rnm, (int)LSNAME, srcnm->lnm);
      break;
   case IA_SRC:
      strcpy(fmtsrmsg+5, "input array");
      break;
   case VALSRC:
      ssprintf(fmtsrmsg+5,"value %4d", (int)srcnm->hbcnm);
      break;
   case VJ_SRC:
      ssprintf(fmtsrmsg+5,"joint %4d", (int)srcnm->hbcnm);
      break;
   case VH_SRC:
      ssprintf(fmtsrmsg+5,"handvis %4d", (int)srcnm->hbcnm);
      break;
   case VW_SRC:
      ssprintf(fmtsrmsg+5,"window %4d", (int)srcnm->hbcnm);
      break;
   case VS_SRC:
      ssprintf(fmtsrmsg+5,"scan window %4d", (int)srcnm->hbcnm);
      break;
   case VT_SRC:
      ssprintf(fmtsrmsg+5,"touch %4d", (int)srcnm->hbcnm);
      break;
   case TV_SRC:
      ssprintf(fmtsrmsg+5, "TV %15s", getrktxt(srcnm->hbcnm));
      break;
   case PP_SRC:
      ssprintf(fmtsrmsg+5, "img prep %15s", getrktxt(srcnm->hbcnm));
      break;
   case ITPSRC:
      ssprintf(fmtsrmsg+5,"img touch %4d", (int)srcnm->hbcnm);
      break;
   case USRSRC:
      ssprintf(fmtsrmsg+5, "userdef %15s", getrktxt(srcnm->hbcnm));
      break;
   case BADSRC:
      strcpy(fmtsrmsg+5, "unknown source");
      break;
      } /* End ityp switch */

   return fmtsrmsg;
   } /* End fmtsrcnm() */
#endif
