/* (c) Copyright 1989-2017, The Rockefeller University *21115* */
/* $Id: d3parm.c 74 2017-09-18 22:18:33Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3parm                                 *
*                                                                      *
*  Analyze run-time parameters and control cards that must come before *
*  anything else (currently BBDHOST and PARMS).                        *
*                                                                      *
*  In the original design, this routine was called twice:  the first   *
*  time to process parameters that must take effect before Group 0 and *
*  Group I control cards are read, the second time to get those that   *
*  override Group I parameters.  However, with the addition of the     *
*  PARMS control card, all the parameters must be processed at once    *
*  and their effects saved in RP0 for action after d3grp1().  These    *
*  actions are carried out by a separate call to d3par2().             *
*                                                                      *
*  The BBDHOST card is placed at the start of the deck by a BBD        *
*  client.  It sets up socket information.  The PARMS card follows     *
*  because it may arrive in the stream from the BBD client.  It may    *
*  have any of the same parameters as the command line, and is         *
*  parsed by the same code (d3pm1p).                                   *
*                                                                      *
*  COMPAT is not a parm because it is an essential feature of a given  *
*  model, it is not sensible to change it at run time.                 *
************************************************************************
*  Written by George N. Reeke                                          *
*  V4A, 05/07/89 Translated to C from version V2G                      *
*  Rev, 05/30/90, JWT - Input file name no longer an argument          *
*  Rev, 10/01/90, GNR - Add MATRIX, BEEPER, NEXP parameters            *
*  Rev, 02/21/92, GNR - Read dbgflags from DEBUG parameter             *
*  Rev, 04/08/92, ABP - Add NOVDRD, NOVDRE parameters.                 *
*  Rev, 07/02/92, ABP - Add HIMSG, split into 2 calls--see above       *
*  V5E, 08/04/92, GNR - Add support for NOGDEV, NOMETA, XG, NOXG       *
*  Rev, 08/18/92, GNR - Move NOPRINT to first group so can stop cdprnt *
*  Rev, 01/31/93, ABP - Fix bug                                        *
*  Rev, 06/04/94, GNR - Make parms enum, restore SPOUT, NOSPOUT        *
*  V7B, 08/06/94, GNR - Use code CMDLINE_ERR for distinctive exit msg  *
*  V8A, 11/27/96, GNR - Remove GDEV (support for NCUBE graphics)       *
*  Rev, 12/07/96, GNR - Remove HIMSG and SUPIP mechanisms              *
*  Rev, 09/12/97, GNR - Add NOPG, delete BEEPER and MATRIX             *
*  Rev, 01/02/98, GNR - Add TEST mode, disallow number after REPLAY    *
*  Rev, 09/21/98, GNR - Add OFFLINE                                    *
*  Rev, 08/13/00, GNR - Add NOAMP, NOSAVE                              *
*  V8B, 04/01/01, GNR - Add NOSD                                       *
*  V8D, 10/31/07, GNR - Add BBDHOST and PARAMS cards, no second pass   *
*  ==>, 01/06/08, GNR - Last mod before committing to svn repository   *
*  Rev, 09/02/08, GNR - Add VMSG parameter to BBDHOST card             *
*  V8E, 04/14/09, GNR - Add BMODE parameter, 11/07/09 Add SS           *
*  V8F, 02/21/10, GNR - Remove NOVDRD, NOVDRE                          *
*  V8H, 11/25/10, GNR - Turn off ONLINE if BBD server (allow override) *
*  V8I, 09/18/14, GNR - Add MFDDBG for mfdraw debug parameters         *
*  R67, 09/23/16, GNR - Add MPIDBG for mpitools debug parameters       *
*  R74, 08/01/17, GNR - Add pltbufl as a parameter of PLOTS key        *
***********************************************************************/

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "rocks.h"
#include "rkxtra.h"

/*---------------------------------------------------------------------*
*                               d3pm1p                                 *
*                                                                      *
*  Call from d3parm() when a parameter has been accessed either from   *
*  the command line or from a PARMS card.  Because some parameters may *
*  use a following parameter as an operand, two parameters are always  *
*  passed, except the second is NULL when there are no more.  d3pm1p() *
*  returns 0 if only 1 parameter was used, otherwise 1.                *
*                                                                      *
*  Note that when saving info to turn off a flag bit in d3par2, it is  *
*  necessary to erase it both from the AND mask and the OR mask in     *
*  order that a command-line parameter can in fact override a PARMS    *
*  card parameter read earlier.                                        *
*                                                                      *
*  Policy:  Keywords that turn on features that are not compiled will  *
*  not even be recognized, but those that turn such features off will  *
*  be recognized but ignored.  (No current examples)                   *
*---------------------------------------------------------------------*/

static int d3pm1p(char *p1, char *p2) {

   ui32  tdbgfl;           /* Temp to deal w/volatile warning */
   int nused = 0;          /* This will be the return value */

/* Table of recognized parameter keywords and enum values--the two
*  sets must be in the same order, obviously.  (Some defaults, e.g.
*  PLOTS, can be entered as parms for convenience, but not, for
*  example, those that depend on a file, e.g. META.)  */

   static char *cnsparms[] = {
      "DEBUG",
      "MFDDBG",
      "MPIDBG",
      "NOPRINT",
      "NOSPOUT",
      "BMODE",
      "MMODE",
      "SMODE",
      "NEXP",
      "OFFLINE",
      "ONLINE",
      "SPOUT",
      "TEST",
      "XG",
      "NOXG",
      /* Keys beyond this point are implemented via d3par2() */
      "NOAMP",
      "NOMETA",
      "PLOTS",
      "NOPLOTS",
      "SAVE",
      "NOSAVE",
      "NOSD",
      "NOSS",
      "NOSTATS",
      "REPLAY",
      "SS" } ;

#define NUM_KEYS (sizeof(cnsparms)/sizeof(char *)) /* Number of keys */

   enum D3PARM {
      D3PARM_DEBUG = 1,
      D3PARM_MFDDBG,
      D3PARM_MPIDBG,
      D3PARM_NOPRINT,
      D3PARM_NOSPOUT,
      D3PARM_BMODE,
      D3PARM_MMODE,
      D3PARM_SMODE,
      D3PARM_NEXP,
      D3PARM_OFFLINE,
      D3PARM_ONLINE,
      D3PARM_SPOUT,
      D3PARM_TEST,
      D3PARM_XG,
      D3PARM_NOXG,
      /* Keys beyond this point are implemented via d3par2() */
      D3PARM_NOAMP,
      D3PARM_NOMETA,
      D3PARM_PLOTS,
      D3PARM_NOPLOTS,
      D3PARM_SAVE,
      D3PARM_NOSAVE,
      D3PARM_NOSD,
      D3PARM_NOSS,
      D3PARM_NOSTATS,
      D3PARM_REPLAY,
      D3PARM_SS } ;

   switch ((enum D3PARM)smatch(RK_NMERR, p1, cnsparms, NUM_KEYS)) {

/*---------------------------------------------------------------------*
*  Parameters that are acted upon immediately (i.e. before Group 1)    *
*---------------------------------------------------------------------*/

case D3PARM_DEBUG:      /* 'DEBUG' */
      RP->CP.dbgflags = 1; /* Default to 1 if no number found */

      /* Break out if no more args or another parm found */
      if (!p2 || smatch(RK_NMERR, p2, cnsparms, NUM_KEYS) > 0)
         break;

      wbcdin(p2, &tdbgfl, (RK_NUNS|RK_CTST|RK_HEXF|RK_NI32)+SCANLEN-1);
      RP->CP.dbgflags = tdbgfl;
      ++nused;
      break;

case D3PARM_MFDDBG:     /* 'MFDDBG' */
      RP->CP.mfddbgflags = 1; /* Default to 1 if no number found */

      /* Break out if no more args or another parm found */
      if (!p2 || smatch(RK_NMERR, p2, cnsparms, NUM_KEYS) > 0)
         break;

      wbcdin(p2, &tdbgfl, (RK_NUNS|RK_CTST|RK_HEXF|RK_NI32)+SCANLEN-1);
      RP->CP.mfddbgflags = tdbgfl;
      ++nused;
      break;

case D3PARM_MPIDBG:     /* 'MPIDBG' */
      RP->CP.mpidbgflags = 1; /* Default to 1 if no number found */

      /* Break out if no more args or another parm found */
      if (!p2 || smatch(RK_NMERR, p2, cnsparms, NUM_KEYS) > 0)
         break;

      wbcdin(p2, &tdbgfl, (RK_NUNS|RK_CTST|RK_HEXF|RK_NI32)+SCANLEN-1);
      RP->CP.mpidbgflags = tdbgfl;
      ++nused;
      break;

case D3PARM_NOPRINT:    /* 'NOPRINT' */
      RP->CP.runflags &= ~RP_OKPRIN;
      RP->CP.runflags |= RP_NOSTAT;
      break;

case D3PARM_NOSPOUT:    /* 'NOSPOUT' */
      spout(SPTM_GBLOFF);
      break;

case D3PARM_BMODE:      /* "Batch Mode */
      RP0->n0flags |= N0F_MPARME;
      RP0->mmode = MM_BATCH;
      break;

case D3PARM_MMODE:      /* "Movie Mode" */
      RP0->n0flags |= N0F_MPARME;
      RP0->mmode = MM_MOVIE;
      break;

case D3PARM_SMODE:      /* "Still Mode" */
      RP0->n0flags |= N0F_MPARME;
      RP0->mmode = MM_STILL;
      break;

case D3PARM_NEXP:       /* Number of exposures/frame */
      RP0->nexp = 1; /* Default to 1 if no number found */

      /* Break out if no more args or next arg is not numeric */
      if (!p2 || cntrl(p2)) break;

      wbcdin(p2, &RP0->nexp, RK_CTST|RK_IORF|RK_NHALF|SCANLEN-1);
      ++nused;
      break;

case D3PARM_OFFLINE:    /* 'OFFLINE' */
      RP->CP.runflags &= ~RP_ONLINE;
      break;

case D3PARM_ONLINE:     /* 'ONLINE' */
      RP->CP.runflags |= RP_ONLINE;
      break;

case D3PARM_SPOUT:      /* 'SPOUT' */
      spout(SPTM_LCLON);
      break;

case D3PARM_TEST:       /* 'TEST' (dry run) */
      RP->CP.runflags |= RP_TEST;
      break;

case D3PARM_XG:      /* 'XG' (XGraphics unit) */
      RP->CP.runflags &= ~RP_NOXG;
      /* Break out if no more args or another parm found */
      if (!p2 || smatch(RK_NMERR, p2, cnsparms, NUM_KEYS) > 0)
         break;
      /* Location of terminal name to RP0 structure */
      RP0->hxterm = savetxt(p2);
      ++nused;
      break;

case D3PARM_NOXG:    /* Process 'NOXG' parameter */
      RP->CP.runflags |= RP_NOXG;
      break;

/*---------------------------------------------------------------------*
*            Parameters that are acted upon after Group 1              *
*---------------------------------------------------------------------*/

case D3PARM_NOAMP:      /* 'NOAMP' */
      RP0->ORrfl |= RP_NOAMP;
      break;

case D3PARM_NOMETA:     /* 'NOMETA' */
      RP0->ORrfl |= RP_NOMETA;
      break;

case D3PARM_PLOTS:      /* 'PLOTS' */
      RP0->ORrfl |= RP_OKPLOT;
      /* Break out if no more args or another parm found */
      if (!p2 || smatch(RK_NMERR, p2, cnsparms, NUM_KEYS) > 0)
         break;
      wbcdin(p2, &RP0->pltbufl, (RK_NUNS|RK_ZTST|RK_CTST|RK_IORF|
         RK_NLONG)+SCANLEN-1);
      ++nused;
      break;

case D3PARM_NOPLOTS:    /* 'NOPLOTS' */
      RP0->ANDrfl &= ~RP_OKPLOT;
      RP0->ORrfl &= ~RP_OKPLOT;
      break;

case D3PARM_SAVE:       /* 'SAVE' */
      RP0->ORksv |= (KSVOK|KSVSR);
      break;

case D3PARM_NOSAVE:     /* 'NOSAVE' */
      RP0->ANDksv &= ~KSVOK;
      RP0->ORksv &= ~KSVOK;
      break;

case D3PARM_NOSD:       /* 'NOSD' (no SIMDATA) */
      RP0->ORksv |= KSDNOK;
      break;

case D3PARM_NOSS:       /* 'NOSS' (no SPOUT statistics) */
      RP0->ANDrfl &= ~RP_SSTAT;
      RP0->ORrfl &= ~RP_SSTAT;
      break;

case D3PARM_NOSTATS:    /* 'NOSTATS' */
      RP0->ORrfl |= RP_NOSTAT;
      break;

case D3PARM_REPLAY:     /* Set for REPLAY and kill stats */
      RP0->ORrfl |= (RP_REPLAY|RP_NOSTAT);
      break;

case D3PARM_SS:         /* SPOUT statistics */
      RP0->ORrfl |= RP_SSTAT;
      break;

default: /* Unidentified parameter */
      cryout(RK_P1,"0***UNIDENTIFIABLE COMMAND LINE PARAMETER: ",
         RK_LN2, p1, RK_CCL, NULL);
      RK.iexit |= CMDLINE_ERR;
      break;

      } /* End of switch over options */

   return nused;
   } /* End d3pm1p() */

/*---------------------------------------------------------------------*
*                               d3parm                                 *
*                                                                      *
*  Read parameters from BBDHOST and PARMS cards and any overriding     *
*  command-line parameters and either execute at once or store flags   *
*  for later action by d3par2() if delay needed to override Group I.   *
*---------------------------------------------------------------------*/

void d3parm(int argc, char **argv) {

   char *p1,*p2,*p3;          /* Ptrs to parameters */
   int iarg;                  /* Argument counter */

/* Initialize controls for d3par2 */

   RP0->ANDrfl = ~(RP0->ORrfl = 0);
   RP0->ANDksv = ~(RP0->ORksv = 0);

/* Echo args--this program now expects only to be called once.
*  (Conversion to upper case removed for XG, 08/04/92, GNR.) */

   if (argc > 1) {
      cryout(RK_P1, "0RUN-TIME PARAMETERS:", RK_LN2, NULL);
      for (iarg=1; iarg<argc; iarg++)
         cryout(RK_P1, " ", RK_CCL, argv[iarg], RK_CCL, NULL);
      }

#ifdef BBDS
/* If program is running as a BBD server, BBDHOST card will be
*  inserted at start of control file by a BBD client in order to
*  provide parameters for establishment of a neural data socket
*  connection.  The syntax is:
*  "BBDHOST [VMSG who] lhn hostname:port" */

   if (cryin()) {
      static char *pbbdh[1] = { "BBDHOST" };
      if (smatch(RK_NMERR, RK.last, pbbdh, 1)) {
         char *phn;           /* Ptr to host name */
         int lhn,oldlen;      /* Scan lengths */
         char lea[SCANLEN1];
         cdscan(RK.last, 1, SCANLEN, RK_WDSKIP);
         scanck(lea, RK_REQFLD, ~RK_BLANK);
         if (ssmatch(lea, "VMSG", 4))
            inform("(SUHV>0I)", &RP0->vmsgwho, &lhn, NULL);
         else 
            wbcdin(lea, &lhn, RK_CTST|RK_IORF|RK_NINT|SCANLEN-1);
         phn = mallocv(lhn+1, "BBD host name");
         oldlen = scanlen(lhn);
         scanck(phn, RK_NEWKEY|RK_REQFLD, ~(RK_BLANK|RK_COMMA));
         bbdsfhnm(phn);
         scanlen(oldlen);
         freev(phn, "BBD host name");
         /* Prevent online requests for users to enter cards */
         RP->CP.runflags &= ~RP_ONLINE;
         }
      else
         rdagn();
      }
#endif

/* Next check for and read any parameters on a PARMS card in the
*  input file.  These will in turn be overridden below by actual
*  command-line parameters.  The idea of this is to let a BBD or
*  GUI client send command-line parameter via the control file
*  because you must be root to change parms sent by inetd/xinetd.  */

   if (cryin()) {
      char lea1[SCANLEN1], lea2[SCANLEN1];
      static char *pparms[2] = { "PARMS", "PARAMS" };
      if (smatch(RK_NMERR, RK.last, pparms, 2)) {
         cdprt1(RK.last);
         cdscan(RK.last, 1, SCANLEN, RK_WDSKIP);
         p1 = lea1, p2 = lea2;
PCScan2: if (scanck(p1, 0, ~(RK_BLANK|RK_ENDCARD)) == RK_ENDCARD)
            goto EndPCParms;
PCScan1: if (scanck(p2, 0, ~(RK_BLANK|RK_ENDCARD)) == RK_ENDCARD)
            p2 = NULL;
         if (d3pm1p(p1, p2)) goto PCScan2;      /* Used 2, scan 2 */
         else if (p2 == NULL) goto EndPCParms;  /* Used last one  */
         else {             /* Swap fields and scan just one more */
            p3 = p1; p1 = p2; p2 = p3; goto PCScan1; }
         } /* End scanning fields on PARMS card */
      else
         rdagn();
      }
EndPCParms: ;

/* Loop over command line parameters */

   for (iarg=1; iarg<argc; ) {      /* Empty loop if no parms */
      int jarg = iarg + 1;
      p1 = argv[iarg];
      p2 = (jarg < argc) ? argv[jarg] : NULL;
      iarg += d3pm1p(p1, p2) + 1;
      } /* End scanning command-line parameters */

   }  /* End of d3parm() */

/*---------------------------------------------------------------------*
*                               d3par2                                 *
*                                                                      *
*  Call from main after d3grp1() has run to override Group I controls  *
*  from stored command-line parameters.                                *
*---------------------------------------------------------------------*/

void d3par2(void) {

   RP->CP.runflags = RP->CP.runflags & RP0->ANDrfl | RP0->ORrfl;
   RP->ksv = RP->ksv & RP0->ANDksv | RP0->ORksv;

   } /* End d3par2() */
