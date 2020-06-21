/* (c) Copyright 1992-2008, Neurosciences Research Foundation, Inc. */
/* $Id: d3exit.c 48 2012-04-23 18:40:15Z  $ */
/***********************************************************************
*                             CNS Program                              *
*    d3exit - Shutdown parallel computation with message info          *
*    abexit - Intercept generic ROCKS errors, convert to d3exit        *
*   abexitm - Intercept generic ROCKS errors with messages             *
*  abexitme - Intercept generic ROCKS errors with messages and errno   *
*  fmtlrlnm - Format lowercase rep & layer names for passing to host   *
*  fmturlnm - Format uppercase rep & layer names for passing to host   *
*  fmturlcn - Format uppercase rep & layer names with cell number      *
*                                                                      *
*  These routines are part of the standard shutdown procedure for CNS. *
*  They are called by any node that wants to terminate execution with  *
*     an error message to the user.                                    *
*  Arguments to d3exit are:                                            *
*     (1) A numerical error code (documented in d3global.h)            *
*     (2) A pointer to a string with error information, or NULL.       *
*         Meaning is specific to the particular error.                 *
*         Usually points to an identifying string that will be         *
*         incorporated in the text of the error message.               *
*     (3) Integer containing error information.  Meaning is specific   *
*         to the particular error.                                     *
*                                                                      *
*  In hybrid versions of CNS, the call to d3exit is translated to a    *
*  call to appexit(), a similar routine which is not CNS specific.     *
*  If running on a comp node, an INIT_ABORT_MSG containing all three   *
*  arguments is sent to host and the process goes into an infinite     *
*  loop, awaiting the cold embrace of an EXEC_ABORT_MSG.  In serial    *
*  version, d3term is called.                                          *
*                                                                      *
*  Arguments to abexit[m][e]() are documented in apndx5.crk            *
*                                                                      *
*  Bugs: if plotting, endplt() should be called on all nodes when      *
*     d3exit is called on any one node.  Currently no way to do this.  *
************************************************************************
*  REVISION HISTORY:                                                   *
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
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "sysdef.h"
#ifndef PARn
#include "rocks.h"
#endif
#include "d3global.h"
#include "plots.h"

/* Declarations not in header files--this all needs cleaning up */
void appexit(long err, char *txt, long ival);
#ifdef GCC
void d3term(struct exit_msg *, int) __attribute__ ((noreturn));
#else
void d3term(struct exit_msg *, int);
#endif

int abexloop;

/*=====================================================================*
*                               d3exit                                 *
*=====================================================================*/

void d3exit(long errorcode, char *idtext, long ival) {

#ifndef PAR
   struct exit_msg EMSG;      /* Structure to hold error parms */

/***KLUDGE:  Generate call to endplt() here if serial and not
*  a normal termination.  Parallel case requires careful work.
*  endplt() may itself create an abexit, so must check for loops.  */
   if (errorcode && !abexloop) {
      abexloop = TRUE; endplt(); }
#endif

#ifdef PAR
   /* Following call should never return */
   appexit(errorcode, idtext, ival);
   exit(1);
#else
   EMSG.ec  = errorcode;
   EMSG.pit = idtext;
   EMSG.iv  = ival;

   d3term(&EMSG, 0);
#endif
   } /* End d3exit */

/*=====================================================================*
*                    abexit, abexitm, and abexitme                     *
*  This code replaces the ROCKS abexit(), abexitm(), and abexitme()    *
*  functions for CNS.  It diverts these calls from library routines,   *
*  which know nothing about parallel termination arrangements for CNS, *
*  to d3exit.                                                          *
*=====================================================================*/

void abexit(int code) {

   d3exit(ABEXIT_ERR, NULL, code);

   } /* End abexit() */

void abexitm(int code, char *emsg) {

   char tmsg[133];   /* Don't malloc--may be handling malloc error */
   if (abexloop)
      d3exit(ABEXIT_ERR, NULL, code);
   strcpy(tmsg, "0***");
   strncat(tmsg, emsg, 128);
   d3exit(ABEXIT_ERR, tmsg, code);

   } /* End abexitm() */

void abexitme(int code, char *emsg) {

#if defined(SUNOS) || defined(SOLARIS) || defined(LINUX)
   int sverr = errno;
   char ssmsg[LNSIZE];     /* Buffer for system error msg */
#endif
   /* The message has to be copied before using ssprintf(), because
   *  it is highly likely that the msg argument is itself a pointer
   *  to the ssprintf buffer... Don't malloc this space, because we
   *  may be handling a malloc error or already out of heap space.  */
   char tmsg[LNSIZE];

   if (abexloop)
      d3exit(ABEXIT_ERR, NULL, code);

   strcpy(tmsg, "0***");
   strncpy0(tmsg+4, emsg, LNSIZE-5);
#ifdef PARn             /* No errno on comp nodes */
   d3exit(ABEXIT_ERR, tmsg, code);
#elif defined(SUNOS)
   cryout(RK_P1, tmsg, RK_LN3+LNSIZE-1, ssprintf(NULL,
      " ***The system error code is %d", sverr),
      RK_LN0+RK_FLUSH, NULL);
   d3exit(ABEXIT_ERR, NULL, code);
#elif defined(SOLARIS)
   if (sverr < sys_nerr) {
      strcpy(ssmsg, " ***");
      strncpy0(ssmsg+4, sys_errlist[sverr], LNSIZE-5); }
   else
      ssprintf(ssmsg, " ***The system error code is %6d", sverr);
   cryout(RK_P1, tmsg, RK_LN3+LNSIZE-1,
      ssmsg, RK_LN0+RK_FLUSH, NULL);
   d3exit(ABEXIT_ERR, NULL, code);
#elif defined(LINUX)
   strcpy(ssmsg, " ***");
   /* It would seem logical to use strerror_r here, but that
   *  function only seems to work when compiled for multithread */
   strncpy0(ssmsg+4, strerror(sverr), LNSIZE-5);
   cryout(RK_P1, tmsg, RK_LN3+LNSIZE-1,
      ssmsg, RK_LN0+RK_FLUSH, NULL);
   d3exit(ABEXIT_ERR, NULL, code);
#else
   d3exit(ABEXIT_ERR, tmsg, code);
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
*     Ptr to lowercase formatted string "celltype (sname,lname)".      *
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

   ssprintf(fmtrlmsg, "celltype (%*s,%*s)",
      (int)LSNAME, il->pback->sname, (int)LSNAME, il->lname);
   return fmtrlmsg;
   } /* End fmtlrlnm() */

char *fmturlnm(struct CELLTYPE *il) {

   ssprintf(fmtrlmsg, "CELLTYPE (%*s,%*s)",
      (int)LSNAME, il->pback->sname, (int)LSNAME, il->lname);
   return fmtrlmsg;
   } /* End fmturlnm() */

char *fmturlcn(struct CELLTYPE *il, long icell) {

   ssprintf(fmtrlmsg, "CELLTYPE (%*s,%*s), CELL %*ld",
      (int)LSNAME, il->pback->sname, (int)LSNAME, il->lname,
      (int)LONG_SIZE, icell);
   return fmtrlmsg;
   } /* End fmturlnm() */

