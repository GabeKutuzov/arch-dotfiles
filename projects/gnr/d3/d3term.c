/* (c) Copyright 1989-2017, The Rockefeller University *21115* */
/* $Id: d3term.c 79 2018-08-21 19:26:36Z  $ */
/***********************************************************************
*                             CNS Program                              *
*          d3term - actions needed at normal and error exits           *
*                                                                      *
*  Synopsis:  void d3term(struct exit_msg *EMSG, int source)           *
*                                                                      *
*  Action:  This routine performs all available cleanup actions when   *
*     CNS terminates, either normally or due to an error, including    *
*     calling d3perr() to print a final message.  But it leaves it     *
*     to the caller to perform final serial or parallel termination    *
*     procedure.  See comments below for list of actions performed.    *
*                                                                      *
*  Any node which wants to initiate a shutdown should call d3exit.     *
*     In a serial computer, this just results in calling d3term() and  *
*     exiting to the OS.  In a parallel computer, d3exit() sends an    *
*     INIT_ERROR_MSG to andmsg.  The text of this message is an        *
*     exit_msg struct containing the message number and possibly       *
*     additional information for constructing the error message text.  *
************************************************************************
*  ==>, 02/17/05, GNR - Last mod before committing to svn repository   *
*  R70, 01/16/17, GNR - New routine under old d3term name, see above   *
***********************************************************************/

/* Include standard library functions */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#ifdef UNIX
#include <errno.h>
#include <signal.h>
#endif
#include "d3global.h"
#include "plots.h"
#ifndef PARn
#include "d3fdef.h"
#include "simdata.h"
#include "tvdef.h"
#include "rocks.h"
#include "rockv.h"
#include "rkxtra.h"
#endif

/* Prototypes for d3perr and d3nset */
void d3perr(struct exit_msg *EMSG, int source);
rd_type *d3nset(rd_type *);
/* Global data origin needed for d3nset() call */
extern rd_type *myprd;

/*--------------------------------------------------------------------*/
/*                                                                    */
/*                              d3term                                */
/*                                                                    */
/*--------------------------------------------------------------------*/

void d3term(struct exit_msg *EMSG, int source) {

   /* If no EMSG is provided, create a fake one so
   *  d3perr() will print normal end message */
   struct exit_msg dfltemsg = { NULL, 0, 0 };
   if (!EMSG) EMSG = &dfltemsg;

   /* Finishing and closing SAVENET and SIMDATA files is done first
   *  so any errors in less important cleanups do not have a chance
   *  to prevent these important data save operations.  */

   /* Save networks if requested.
   *  N.B.  If we are here after all trial series, il->llt and il->prd
   *  are 0 from membcst--must call d3nset again to restore.  */
   if ((RP->ksv & (KSVSR|KSVOK)) == (KSVSR|KSVOK)) {
#ifdef PARn
      d3nset(myprd);
#endif
      d3save(D3SV_ENDXEQ);
      }

   /* Finish all plotting.  If parallel, need to call endplt()
   *  even if plotting off in order to terminate mfsr if on.  */
#ifdef PAR
   endplt();
#else
   if (RP->CP.runflags & RP_OKPLOT) endplt();
#endif

#ifndef PARn
   /* Write end mark and close SIMDATA file */
   if (!(RP->ksv & (KSDNE|KSDNOK))) {
      struct RFdef *pgdf = d3file[RP0->pgds->gdfile];
      if (pgdf->iamopen & (IAM_OPEN|IAM_FOPEN)) {
         rfwi4 (pgdf, (si32)EndMark);
         rfclose (pgdf, NO_REWIND, RELEASE_BUFF, ABORT);
         }
      }

   /* Close all UPGM camera routines--ignore errors at this late date */
   {  TVTYPE *pcam;
      if (RP0->ptvrsi) freev(RP0->ptvrsi, "Rescale info");
      for (pcam=RP->pftv; pcam; pcam=pcam->pnxtv) {
         struct PREPROC *pip;
         if (pcam->utv.tvsflgs & TV_UPGM && pcam->tvinitfn)
            pcam->tvinitfn(&pcam->utv, getrktxt(pcam->hcamnm),
               NULL, NULL, TV_CLOSE);
         for (pip=pcam->pmyip; pip; pip=pip->pnipc)
            if (pip->ipsflgs & PP_UPGM && pip->pipfn)
               pip->pipfn(&pip->upr, NULL, NULL);
         }
      } /* End pcam local scope */

   /* Close any user libraries */
   d3upcl();

   /* Close any replay file */
   if (RP->CP.runflags & RP_REPLAY)
      rfclose(d3file[REPLAY], REWIND, RELEASE_BUFF, ABORT);

   /* Disconnect from any BBDs */
#ifdef BBDS
   bbdssvrc(EMSG->ec);
   bbdsquit();
#endif

   /* Print final message and close cryin/cryout files */
   d3perr(EMSG, source);
   cryicls();
   cryocls();
#endif

   } /* End d3term */

