/* (c) Copyright 1993-2018, The Rockefeller University *21116* */
/* $Id: d3perr.c 78 2018-08-02 18:36:58Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                              d3perr.c                                *
*              Print error messages for CNS termination                *
*                                                                      *
*  This program separates the printing of error messages, which will   *
*  now occur in the driver program in parallel versions of CNS, from   *
*  the other shutdown activities in d3abort, which will occur on comp  *
*  nodes.                                                              *
*                                                                      *
*  Conditional compilations for messages dependent on comp node type   *
*  have been removed--makes d3perr compatible with various drivers.    *
*                                                                      *
************************************************************************
*  V6B, 07/29/93, ABP - Initial version, derived from d3abort.c        *
*  Rev, 12/11/93, GNR - Kludge out ROCKS so won't print default title  *
*  V8A, 04/24/96, GNR - Break down overflow into several subtypes      *
*  Rev, 11/28/96, GNR - Remove support for non-hybrid version          *
*  V8D, 08/17/05, GNR - Add conductances and ions, new reset options   *
*  ==>, 12/28/07, GNR - Last mod before committing to svn repository   *
*  Rev, 04/28/08, GNR - Revise overflow msg to include cell number     *
*  V8F, 02/21/10, GNR - Remove VDR messages, add PREPROC_ERR           *
*  V8H, 11/08/10, GNR - Two EMSG->iv to &EMSG->iv in convrt calls      *
*  Rev, 06/19/12, GNR - Remove LEIFAMP_ERR                             *
*  R67, 04/27/16, GNR - Remove Darwin 1 support                        *
*  R78, 04/20/18, GNR - Add IMTOOSM_ERR                                *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "sysdef.h"
#include "d3global.h"
#include "savblk.h"
#include "rocks.h"
#include "rkxtra.h"
#ifdef UNIX
#include <errno.h>
#ifndef GCC
extern char *sys_errlist[];   /* For some reason, not in errno.h */
#endif
#endif

/***********************************************************************
*                              d3perr()                                *
***********************************************************************/

void d3perr(struct exit_msg *EMSG, int source) {

/* The following length limit is used as a safety net when printing
*  undefined information received from CNS.  It is also used in a
*  few cases where repertoire and layer names are printed, the max
*  length of which is known in d3global.h, just to avoid bringing
*  a full collection of CNS structure definitions into this file.  */
#define MX_ETXT_LEN  80 /* Maximum length of unknown error text */
#define qMEL   "80"     /* Quoted version of MX_ETXT_LEN */

   char acode[4];       /* ASCII text of error code */
#ifdef PAR
   char anode[16];      /* ASCII text of error node */
#endif
   char SPECMSG[MX_ETXT_LEN+32]; /* Msg text for development errors */

/* If no exit_msg was received, take a shortcut exit.  This is
*  really a development error, but there is no info to print.  */

   if (!EMSG) {
      cryout(RK_P1, "0***Program terminated with no error info.",
         RK_LN2+RK_FLUSH,NULL);
      return;
      }

/* Here to send appropriate message to stdout */

   switch (EMSG->ec) {
      case D3_NORMAL:
         if (RP->CP.outnow & OUT_USER_QUIT)
            cryout(RK_P1, "0CNS terminated by graphics or image "
               "interface", RK_LN2+RK_FLUSH, NULL);
         else
            cryout(RK_P1, "0Normal CNS termination",
               RK_LN2+RK_FLUSH, NULL);
         break;
      case ABEXIT_ERR:
         if (EMSG->pit) cryout(RK_P1, EMSG->pit, RK_LN2+LNSIZE-1,
            NULL);
         EMSG->ec = EMSG->iv;
         break;
      case LIMITS_ERR:
         convrt("(P1,'0***INPUT HAS MORE ',J1A20,'OBJECTS THAN "
            "ALLOWED BY CURRENT PROGRAM LIMITS (',J0I8,2H).)",
            EMSG->pit, &EMSG->iv, NULL);
         break;
      case ASIZE_ERR:
         convrt("(P1,'0***',J1A20,'IS LARGER THAN ALLOWED BY "
            "CURRENT PROGRAM LIMITS", EMSG->pit, NULL);
         break;
      case SAVEXF_ERR:
         cryout(RK_P1,"0***RAN OUT OF SAVE FILES.",RK_LN2,NULL);
         break;
      case LIJXF_ERR:
         cryout(RK_P1,"0***RAN OUT OF CONNLIST FILES.",RK_LN2,NULL);
         break;
      case FILECK_ERR:
         cryout(RK_P1,"0***ERROR CHECKING INPUT FILES.",RK_LN2,NULL);
         break;
      case REPLAY_ERR:
         cryout(RK_P1,"0***REPLAY FILE FORMAT WRONG.", RK_LN2, NULL);
         break;
      case VALMAG_ERR:
         convrt("(P1, '0***BBD VALUE ',J1I4,'IS OUTSIDE -255 TO "
            "+255 RANGE.')", &EMSG->iv, NULL);
         break;
      case LIJBDV_ERR:
         convrt("(P1,'0***CONNLIST Lij > SOURCE SIZE FOR ',J0A20,"
            "', CONNTYPE ',J0I4)", EMSG->pit, &EMSG->iv, NULL);
         break;
      case RESETCIJ_ERR:
         convrt("(P1, '0***TRYING TO RESET CIJ TO CIJ0 FOR ',J0A20,"
            "', CONNTYPE ',J0I4,1H,/,'    BUT CIJ0 WAS NOT SAVED.')",
            EMSG->pit, &EMSG->iv, NULL);
      case ROCKIO_ERR:
         if (EMSG->iv & 1) cryout(RK_P1,
            "0***ERROR IN CONTROL FILE.", RK_LN2, NULL);
         if (EMSG->iv & ENV_ERR) cryout(RK_P1,
            "0***ERROR IN ENVIRONMENT MODULE.", RK_LN2, NULL);
         if (EMSG->iv & FILE_ERR) cryout(RK_P1,
            "0***ERROR INTERPRETING DATA FILE.", RK_LN2, NULL);
         if (EMSG->iv & CMDLINE_ERR) cryout(RK_P1,
            "0***BAD COMMAND-LINE PARAMETER.", RK_LN2, NULL);
         if (EMSG->iv & EXTDEV_ERR) cryout(RK_P1,
            "0***ERROR ACCESSING EXTERNAL DEVICE.", RK_LN2, NULL);
         break;
      case NODENO_ERR:
         cryout(RK_P1,"0***PARALLEL CNS REQUIRES >= 2 NODES.",
            RK_LN2,NULL);
         break;
      case MAXISN_ERR:
         cryout(RK_P1,"0***CNS CANNOT HANDLE > 32767 UNIQUE STIMULI"
            "PER TRIAL SERIES.", RK_LN2, NULL);
         break;
      case GCONNSZ_ERR:
         convrt("(P1, '0***GCONN AREA EXCEEDS LAYER SIZE FOR ',J0A20,"
            "', GCONN ',J0I4)", EMSG->pit, &EMSG->iv, NULL);
         break;
      case REPLAY_EOF_ERR:
         cryout(RK_P1,"0***PREMATURE EOF ON REPLAY FILE.",
            RK_LN2, NULL);
         break;
      case VMAMPL_ERR:
         convrt("(P1,'0***VALUE-MODULATED AMP VALUE INDEX OUT OF "
            "BOUNDS FOR ',J0A20,', CONNTYPE ',J0I4)", EMSG->pit,
            &EMSG->iv, NULL);
         break;
      case OVERFLOW_ERR:
         {  long ibit = 1;
            int it;
            char ovftype[80];
            static char *ovtypes[] = { "GENERAL,", "MEM_ALLO,",
               "GCONN,", "DECAY,", "IZHIK,", "BREG,", "TER_SCALE,",
               "STATS", "PROBE", "SVFILHDR", "PREPROC", };
            strcpy(ovftype, ", TYPE ");
            for (it=0; it<OVT_END; it++,ibit<<=1)
               if (EMSG->iv & ibit) strcat(ovftype, ovtypes[it]);
            /* Delete extra comma at end */
            it = strlen(ovftype); ovftype[it-1] = '\0';
            cryout(RK_P1,"0***OVERFLOW IN ", RK_LN2, EMSG->pit,
               RK_CCL, ovftype, RK_CCL, NULL);
            } /* End overflow local scope */
         break;
      case PREPROC_ERR:
         convrt("(P1,'0***PREPROCESSOR ERROR ',J1I4,'IN ',A8)",
            &EMSG->iv, EMSG->pit, NULL);
         break;
      case GFSVRL_ERR:
         convrt("(P1,'0***SIMDATA RECLEN > 2GB/CELL FOR ',J0A20)",
            EMSG->pit, NULL);
         break;
      case VIDEO_ERR:
         convrt("(P1,'0***VIDEO ERROR ',J1I4,'IN ',A8)",
            &EMSG->iv, EMSG->pit, NULL);
         break;
      case TV_SCALE_ERR:
         convrt("(P1,'0***IMAGE TOO BIG IN CAMERA ',J0I4)",
            &EMSG->iv, NULL);
      case PAUSEMSG_ERR:
         convrt("(P1,'0***PAUSE CONTROL ERROR ',JI4)",
            &EMSG->iv, NULL);
         break;
      case BADCELL_ERR:
         convrt("(P1,'0***CELL ',J1I6,'DOES NOT EXIST ON ',J0A20)",
            &EMSG->iv, EMSG->pit, NULL);
         break;
      case RESITM_ERR:
         cryout(RK_P1,"0***RESTORE FILE IS MISSING DATA FOR KEY ",
            RK_LN2, EMSG->pit, RK_CCL, NULL);
         break;
      case RESTYP_ERR:
         convrt("(P1,'0***RESTORE FILE ITEM KEY = A" qqv(SFKEY_LEN)
            ",9H, TYPE = J0Z8)", EMSG->pit, &EMSG->iv, NULL);
         break;
      case UPROG_ERR:
         cryout(RK_P1,"0***USER PROGRAMS NOT SUPPORTED "
            "ON THIS COMPUTER.", RK_LN2, NULL);
         break;
      case BBD_ERR:
         convrt("(P1,'0***BBD ERROR ',JI4,"
            "' MSG ',J0A" qMEL ")", &EMSG->iv, EMSG->pit, NULL);
         break;
      case IMTOOSM_ERR:
         cryout(RK_E1, "0***PREPROC ", RK_LN2, EMSG->pit, RK_CCL,
            " KERNEL DOES NOT FIT ON INPUT IMAGE.", RK_CCL, NULL);
         break;
      case NREAD_ERR:
         convrt("(P1,'0***NODE-NODE READ ERROR ',JI4,"
            "' FOR: ',J0A" qMEL ")", &EMSG->iv, EMSG->pit, NULL);
         break;
      case NWRITE_ERR:
         convrt("(P1,'0***NODE-NODE WRITE ERROR ',JI4,"
            "' FOR: ',J0A" qMEL ")", &EMSG->iv, EMSG->pit, NULL);
         break;
      case PREV_GREX_ERR:
         cryout(RK_P1,"0CNS clean exit, error reported above",
            RK_LN2+RK_FLUSH, NULL);
         break;
      case QUICKEXIT_RQST:
         cryout(RK_P1,"0***IMMEDIATE EXIT REQUESTED", RK_LN2, NULL);
         break;

      /* Handle development errors -- create
      *  string with minimal error info */
      case NYIMPL_ERR:
         sconvrt(SPECMSG+12, "('OPTION ', J1A20,"
            "'NOT YET IMPLEMENTED')", EMSG->pit, NULL);
         break; 
      case GETTHR_ERR:
         sconvrt(SPECMSG+12, "('BAD GETTHR FORMAT: ',J0A12)",
            EMSG->pit, NULL);
         break;
      case VALDAT_ERR:
         strcpy(SPECMSG+12, "'VALUE DATA ARRAY WAS NOT ALLOCATED'");
         break;
      case COLOR_ERR:   /* Bad color selector */
         sconvrt(SPECMSG+12, "(7HCSEL = J0I8)", &EMSG->iv, NULL);
         break;
      case LSRSTR_ERR:  /* Tried to save lsrstr count at wrong time */
         SPECMSG[12] = '\0';
         break;
      case IJPLEND_ERR: /* Wrong starting cell in cijplot stream */
         sconvrt(SPECMSG+12, "(3HAT J0A38,9H, CELL = J0I6)",
            EMSG->pit, &EMSG->iv, NULL);
         break;
      case IONREG_ERR:  /* ION was attached to CELLTYPE, no REGION */
      case INITSI_ERR:  /* Tried to init more s(i) than delay rows */
         sconvrt(SPECMSG+12, "(3HIN J0A" qLXN ")", EMSG->pit, NULL);
         break;
      case DPLLEN_ERR:  /* Detail print line too long */
      case LIJBND_ERR:  /* Lij out-of-bounds entering lb2b */
      case CONDCT_ERR:  /* Conductance refs nonexistent CONNTYPE */
      case LIJGEN_ERR:  /* Unrecognized KGEN option */
      case DPSYNC_ERR:  /* Detail print out of synch */
         sconvrt(SPECMSG+12, "(3HIN J0A" qLXN ",11H, CONNTYPE J0I8)",
            EMSG->pit, &EMSG->iv, NULL);
         break;
      case CSELJSYN_ERR:   /* CSELPLOT Node 0 got negative jsyn */
         sconvrt(SPECMSG+12, "'JSYN ',J0I6,' <0')", &EMSG->iv, NULL);
         break;
      default: {
         /* We should only get here if a new error code was inserted
         *  in CNS (or one of its libraries) but the corresponding
         *  error message text was not added to this program.  If
         *  there is no message text, it is a development error.
         *  Otherwise, just print what there is.  */
         if (EMSG->pit) {
            int ll = 0;
            while (isprint(EMSG->pit[ll]) && ll < MX_ETXT_LEN) ll++;
            if (ll) cryout(RK_P1, "0***", RK_LN2, EMSG->pit, ll, NULL);
            convrt("(P1,' ***DETAIL CODE = ',JI8)", &EMSG->iv, NULL);
            }
         else {
            sconvrt(SPECMSG+12,"(8HVALUE = JI8)", &EMSG->iv, NULL);
            EMSG->ec = FIRST_DEVERR;
            }
         } /* End default */

      } /* End error message switch */

   if (EMSG->ec >= FIRST_DEVERR) { /* Handle development errors */

/* Development errors are those errors that should never occur outside
*  a development environment.  They are handled differently, to assure
*  that if such an error slips into a released version, the user will
*  be prompted to contact the developers.  The full meaning of the error
*  is not given to the user, as in "normal" errors, but an error message
*  containing the error code and some useful data is generated, to
*  be submitted as part of the report. */

      sconvrt(SPECMSG,"(O5HErrorI5,2H, )", &EMSG->ec, NULL);
      cryout(RK_P1,
       "0***FATAL CNS ERROR***",RK_LN6,
       " ***An internal error in CNS caused termination.",RK_LN0,
       " ***   Please submit a problem report to George Reeke,",RK_LN0,
       " ***   including copies of files used, description",RK_LN0,
       " ***   of system, and the following error summary: ",RK_LN0,
       " ", RK_LN1+1, SPECMSG, RK_CCL, NULL);
      }

/* Final message includes error code and source node.  ROCKS
*  buffers are flushed--this now also flushes library buffers. */

   if (EMSG->ec != 0) { /*DEBUG: subst with symbol for normal term */
      wbcdwt(&EMSG->ec, acode, RK_IORF|RK_LFTJ|RK_NINT|3);
#ifdef PAR
      if (source == NC.hostid)
         ssprintf(anode, "ON HOST NODE.");
      else
         ssprintf(anode, "ON NODE %6d.", source);
      cryout(RK_P1, "0***PROGRAM TERMINATED WITH ABEND CODE ", RK_LN2,
            acode, RK_CCL+4, anode, RK_CCL+RK_FLUSH, NULL);
#else
      cryout(RK_P1, "0***PROGRAM TERMINATED WITH ABEND CODE ", RK_LN2,
            acode, RK_CCL+RK_FLUSH+4, NULL);
#endif
      }
   return;
   } /* End d3perr() */
