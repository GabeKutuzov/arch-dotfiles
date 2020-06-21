/* (c) Copyright 2018, The Rockefeller University *21116* */
/* $Id: d3start.c 79 2018-08-21 19:26:36Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                         d3start1, d3start2                           *
*    Cortical Network Simulator--Startup I/O, Version, Run number      *
*                                                                      *
*----------------------------------------------------------------------*
*  This file contains two functions:                                   *
*  void d3start1(char *vfnm, char *version)                            *
*     Arg1: "vfnm" is name of version information file to be written,  *
*        here used just as a run number update switch.                 *
*     Arg2: "version" is string giving current CNS revision number.    *
*     Call before d3parm so default title and version info is printed  *
*     on stdout before any control cards are read and printed.         *
*  void d3start2(char *vfnm)                                           *
*     Arg:  "vfnm" is name of version information file to be written.  *
*     This must be called after d3parm so title card, if any, has been *
*     read by cryin and is available via gettit call.                  *
*     The idea is that this file can be saved in a subversion archive  *
*     along with the control file, so software versions used in every  *
*     run are saved even if the large stdout "print" file is deleted.  *
*                                                                      *
*  New to R78 is output of CNS_VERSFILE file with CNS compilation and  *
*  library version info, maintenance of run nunber in .cnsrunno file.  *
*  Design Decision:  The run number is only incremented when the vfnm  *
*  version file is written.  This is to avoid incrementing it for test *
*  or debug runs that are not being recorded.                          *
*----------------------------------------------------------------------*
*  R78, 03/31/18, GNR - Code broken out from darwin3.c main program    *
*  ==>, 08/02/18, GNR - Last mod before committing to svn repository   *
*  R79, 08/09/18, GNR - Add host name and number of nodes to vfnm info *
*  R79, 08/21/18, GNR - Add WAI ("where am I") envvar to runno output  *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "sysdef.h"
#ifdef UNIX
#include <signal.h>
#include <unistd.h>
#endif
#include "d3global.h"
#include "rocks.h"
#include "rkxtra.h"

#ifdef PARn
#error d3start should not be compiled on PARn nodes
#endif

/* Number of decimal digits in stored run number */
#define LSRNO     6

/* Prototypes unique to this program */
char *compdate(void);

/* Info saved from start1 to start2 --
*  maybe not worth malloc overhead */
static char vline1[L_TITL], vline2[LNSIZE], vline3[LNSIZE];

/*---------------------------------------------------------------------*
*                             d3start1()                               *
*---------------------------------------------------------------------*/

void d3start1(char *vfnm, char *version) {

   FILE *prno;
   /* Default title for printed output */
   char dttl[L_TITL];

   accwac();                  /* Accept whitespace as comments */
   sprmpt("CNS> ");           /* Set prompt for online operation */
   if (vfnm) {                /* vfnm used as switch for runno update */
      char *pwai;             /* Ptr to WAI envvar */
      char arunno[LSRNO+2];
      if (!(prno = fopen(RUNNOFNM, "r+"))) {
         if (errno != ENOENT)
            abexitme(RUNNO_ERR, "Error opening " RUNNOFNM "(read)");
         prno = fopen(".cnsrunno", "w+");
         if (!prno)
            abexitme(RUNNO_ERR, "Error opening " RUNNOFNM "(write)");
         RP->CP.runno = 1;
         }
      else {
         if (!fread(arunno, LSRNO, 1, prno))
            abexitme(RUNNO_ERR, "Error reading " RUNNOFNM);
         if (fseek(prno, 0, SEEK_SET) < 0)
            abexitme(RUNNO_ERR, "Error rewinding " RUNNOFNM);
         wbcdin(arunno, &RP->CP.runno,
            RK_NUNS|RK_CTST|RK_QPOS|RK_IORF|RK_NI32|LSRNO-1);
         ++RP->CP.runno;
         }
      wbcdwt(&RP->CP.runno, arunno,
         RK_NUNS|RK_NPAD0|RK_IORF|RK_NI32|LSRNO-1);
      if (!fwrite(arunno, LSRNO, 1, prno))
         abexitme(RUNNO_ERR, "Error writing " RUNNOFNM);
      if (fclose(prno))
         abexitme(RUNNO_ERR, "Error closing " RUNNOFNM);
      /* Title formatting is done with ssprintf to control length.
      *  Version numbner is passed from the darwin3 main program,
      *  which obtains it from SVNVRS (the subversion revision number)
      *  passed from gnrversn call in makefile (or defined in main).
      *  The compdate() string gives the date of the most recent compi-
      *  lation of any CNS routine (we do not use __DATE__, which
      *  gives only the date of compilation of this routine.  The
      *  environment variable "WAI" (Where am I) gives a max 4 char
      *  abbrev for where the program is running, expected to be
      *  set in .cshrc so not needed in any scripts or input files. */
      pwai = getenv("WAI");
      ssprintf(vline1, "      CNS Release R%4s - Compiled %11s"
         " - Run %4s%6s", version, compdate(), pwai, arunno);
      }
   else {
      ssprintf(vline1, "      CNS Release R%4s - Compiled %11s",
         version, compdate());
      }
   settit(vline1);
   cryout(RK_P1, "    ", RK_LN1, NULL);
   ssprintf(dttl, "CNS Release R%4s", version);
   setpid(dttl);

   ssprintf(vline2, "    Using %30s, %30s, %30s,",
      crkvers(), plotvers(), envvers());
#ifdef BBDS
   ssprintf(vline3, "          %30s, %30s", mpitvers(), bbdvers());
#else
   ssprintf(vline3, "          %30s", mpitvers());
#endif
   cryout(RK_P1, vline2, RK_LN2, NULL);
   cryout(RK_P1, vline3, RK_LN0, NULL);
   }  /* End d3start1() */


/*---------------------------------------------------------------------*
*                             d3start2()                               *
*---------------------------------------------------------------------*/

void d3start2(char *vfnm) {

   FILE *pvf;
   char *uttl;
   char vline4[LNSIZE];
   char vuttl[L_TITL+1];
   int  lvln;

   /* Get hostname and number of nodes if parallel and format vline4.
   *  N.B.  Host name max length is set to make max length of vline4
   *  be around 96 chars.  Could be '%*s' with length = LNSIZE-43.  */
   lvln = gethostname(vuttl, L_TITL);
   if (lvln < 0) strcpy(vuttl, "unknown host");
#ifdef PAR
   ssprintf(vline4, "0   Running parallel on %8d nodes on host %64s",
      NC.total, vuttl);
#else
   ssprintf(vline4, "0   Running serial on host %64s", vuttl);
#endif
   cryout(RK_P1, vline4, RK_LN2, NULL);
   
   if (!vfnm) return;

   /* Write the version file with info saved in vlines.
   *  Delete trailing blanks from title, remove cryout controls */ 
   pvf = fopen(vfnm, "w");
   if (!pvf)
      abexitme(VFILE_ERR, "Error opening version file for write");
   uttl = gettit();              /* Line 0: Title */
   for (lvln=L_TITL; lvln && uttl[lvln-1] == ' '; --lvln) ;
   memcpy(vuttl, uttl, lvln);
   vuttl[lvln++] = '\n';
   fwrite(vuttl, lvln, 1, pvf);
   uttl = vline1 + 6;            /* Line 1: CNS release */
   lvln = strlen(uttl);
   uttl[lvln++] = '\n';
   fwrite(uttl, lvln, 1, pvf);
   uttl = vline2 + 4;            /* Line 2: crk, plot, env versions */
   lvln = strlen(uttl);
   uttl[lvln++] = '\n';
   fwrite(uttl, lvln, 1, pvf);
   uttl = vline3 + 10;           /* Line 3: mpitools, bbd versions */
   lvln = strlen(uttl);
   uttl[lvln++] = '\n';
   fwrite(uttl, lvln, 1, pvf);
   uttl = vline4 + 4;            /* Line 4: host and node count */
   lvln = strlen(uttl);
   uttl[lvln++] = '\n';
   fwrite(uttl, lvln, 1, pvf);
   fclose(pvf);
   } /* End d3start2() */
