/* (c) Copyright 1985-2018, The Rockefeller University *21116* */
/* $Id: darwin3.c 78 2018-08-02 18:36:58Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               darwin3                                *
*             Cortical Network Simulator--Main Procedure               *
*                                                                      *
*----------------------------------------------------------------------*
*  N.B.  CNS uses the parallel-computer memory management routines     *
*  declared in memshare.h and documented in mpitools/membcst.c.        *
*  Always use these routines to allocate, free, and broadcast memory.  *
*----------------------------------------------------------------------*
*  V1A - G. N. Reeke - 08/21/85                                        *
*  V4A, 06/13/88, GNR et al. - Translated to 'C' from V3E              *
*  Rev, 03/16/90, GNR - List of modules for linking moved to make file *
*  Rev, 04/26/90, GNR - PRINT/PLOT cycles, clean-up of KPS array       *
*  Rev, 05/30/90, GNR - Input file name no longer command line arg     *
*  Rev, 10/01/90, GNR - Add MOVIE implementation                       *
*  Rev, 10/09/90, MC  - Add calls to vinit, nsigrab                    *
*  Rev, 12/27/90, MC  - Add calls to d3save, d3savn                    *
*  Rev, 01/30/91, GNR - Correct arm/window broadcast bug               *
*  Rev, 04/23/91, GNR - Break rpdef into rpdef and rpdef0              *
*  V5A, 05/21/91, GNR - Add platform calls                             *
*  Rev, 08/19/91, GNR - Avoid TV auto time delay if platform in use    *
*  V5C, 11/16/91, GNR - Add D1, multifile save/restore, many fixes     *
*  Rev, 03/05/92, GNR - Use rf io routines                             *
*  Rev, 04/21/92, ABP - Add d3vdsu, d3vdst, d3vdcl for VDR             *
*                       Replace calls to ngraph with calls to d3ngph   *
*  Rev, 05/09/92, GNR - Use new ROCKS plot calls for atomization       *
*  Rev, 05/20/92, ABP - Add stattmr() calls                            *
*  V5E, 06/06/92, GNR - Add plot metafile, break out d3plot routine    *
*  Rev, 07/02/92, ABP - Split d3parm() call to two, move bkpt()        *
*  Rev, 08/18/92, GNR - Preserve pgno, rmlns, oprty, spend across ovly *
*                       Improve implementation of NOPRINT option       *
*  Rev, 02/27/93, GNR - Remove snapshot support, replaced by metafile  *
*  V6A, 03/02/93, GNR - Add subarbors and platform senses              *
*  Rev, 04/19/93, GNR - Always call d3pxq, check for OUT_QUICK_EXIT    *
*  V6C, 08/14/93, GNR - Remove 2**n dependency, add findcell routine   *
*  Rev, 12/17/93, GNR - New rseed handling, - Page 1, + NL at end      *
*  V6D, 01/10/94, GNR - GCONN OPT=V, DELAY, spiking cells              *
*  Rev, 01/31/94,  LC - Added sprmpt                                   *
*  V7A, 04/13/94, GNR,PJM - Support NOMAD on T/NM, user-defined senses *
*  V7B, 06/22/94, GNR - Strict compatibility relaxed, correct subarbor *
*                       indexing & amp, add explicit amp scales, "LTP" *
*                       fancified (removed in V8A), better rounding,   *
*                       correct bug spiking when cell_resp < 0, output *
*                       contrls via OUTCTL, regeneration, delete KGN1D *
*  V7C, 11/12/94, GNR - Divide ngr into ncs,nds                        *
*  V8A, 04/15/95, GNR - Add sensory modalities, d3ajw, d3schk, d3mchk, *
*                       d3mset, d3mark, d3oafs, KP to CELLTYPE, handle *
*                       VJ,VW,VT as VCELLs, CYCLE KP=U, remove LTP and *
*                       D1ENV value, clear value on new trial series   *
*  Rev, 11/22/96, GNR - Bug fix: preserve serial ps2 after d3nset call *
*  Rev, 11/28/96, GNR - Remove support for non-hybrid version          *
*  Rev, 03/01/97, GNR - Allocate for all s(i) on host to save d3exch() *
*  Rev, 08/25/97, GNR - Implement version 3 GRAFDATA                   *
*  Rev, 12/08/97, GNR - Add trials since start of run to stitle        *
*  Rev, 04/24/98, GNR - Revise outnow procedures to handle BUT_QUIT    *
*  Rev, 09/22/98, GNR - Use kwscan N (calls d3rlnm) and O type codes   *
*  V8B, 12/03/00, GNR - Move to new memory management routines         *
*  V8C, 02/22/03, GNR - Cell responses in millivolts, COMPAT=C         *
*  V8D, 02/23/04, GNR - Add conductances and ions, new reset options   *
*  Rev, 02/08/07, GNR - Remove PNS, add VVTV and BBD support           *
*  Rev, 11/22/07, GNR - Event resets, SAVE NEWSERIES, CREATE, ROTATE   *
*  Rev, 12/22/07, GNR - Preserve new ENV, BBD values on value reset    *
*  ==>, 02/28/08, GNR - Last mod before committing to svn repository   *
*  Rev, 03/12/08, GNR - Add subvsn to ttl, bbd and oldplot versions    *
*  Rev, 03/23/08, GNR - Add FRZTRIALS, EVTRIAL, FRZCYC default = ntc1  *
*  Rev, 09/26/08, GNR - Minor type changes for 64-bit compilation      *
*  V8E, 01/26/09, GNR - Add Izhik cells, call d3resetn() on all nodes  *
*  V8F, 02/15/10, GNR - Add UPGM TV, serial RENDEZVOUS[12]             *
*  Rev, 02/20/10, GNR - Remove PLATFORM and VDR support                *
*  Rev, 04/24/10, GNR - Add image preprocessing                        *
*  V8H, 10/20/10, GNR - Add xn (once per conntype) variable allocation *
*  Rev, 11/11/10, GNR - Add CYCLE KP=Q to print stats on mfdraw quit   *
*  Rev, 04/25/11, GNR - Allow SIMDATA file to start at Group III time. *
*  Rev, 10/12/11, GNR - Remove setmeta(), setmfd(), use setmf()        *
*  Rev, 04/13/13, GNR - Move cryout flush to d3term, cry[io]cls calls  *
*  Rev, 05/11/13, GNR - Remove gconn falloff                           *
*  R64, 12/10/15, GNR - Move bbdschk call after d3news (first series)  *
*  R67, 04/27/16, GNR - Remove Darwin 1 support                        *
*  Rev, 08/13/16, GNR - Revisions for MPI                              *
*  R78, 03/25/18, GNR - Bug fix:  Return error code after RK.iexit err *
*  R78, 03/29/18, GNR - Add version file output for project recording  *
*  R78, 03/31/18, GNR - Break out titles, version, runno to d3start()  *
*  R79, 08/07/18, GNR - Call envin() to redirect stdin, calc omegau,   *
*                       turn off OKPLOT if NOXG and no METAFILE        *
***********************************************************************/

/*=====================================================================*
*    If revision number is not defined in makefile, define it here     *
*=====================================================================*/
#ifndef SVNVRS
#define SVNVRS "78"
#endif

#define MAIN

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rkarith.h"
#include "normtab.h"
#ifdef UNIX
#include <signal.h>
#include <unistd.h>
#endif
#include "d3global.h"
#include "jntdef.h"
#include "armdef.h"
#include "wdwdef.h"
#include "d3fdef.h"
#include "allo_tbl.h"      /* To define static tables */
#include "simdata.h"
#include "statdef.h"
#include "bapkg.h"
#include "collect.h"
#ifndef PARn
#include "rocks.h"
#include "rkxtra.h"
#include "savblk.h"        /* To define static tables */
#ifdef VVTV
#include "vvuser.h"
#endif
#endif /* !PARn */
#include "tvdef.h"
#include "plots.h"

#ifdef PARn
#include "rksubs.h"
#endif

/* Prototypes unique to CNS main program */

#ifndef PARn      /* Not used on comp nodes */
void d3allo(void);
void d3aprt(void);
void d3asck(void);
void d3bgis(void);
void d3cchk(void);
void d3clsm(void);
void d3cnmd(struct VDTDEF *);
void d3dflt(void);
void d3dflt2(void);
void d3echo(void);
void d3evt1(void);
void d3evt2(void);
void d3fchk(void);
void d3fix1(void);
void d3gfcl(int kg1);
void d3gfhd(struct SDSELECT *pgd);
ui32 d3gfrl(struct CELLTYPE *, ui32);
void d3gfsh(struct SDSELECT *, ui32);
void d3gfsv(struct SDSELECT *, ui32);
void d3grout(void);
int  d3grp0(void);
int  d3grp1(void);
void d3grp2(void);
int  d3grp3(void);
void d3ijpi(void);
void d3lafs(void);
void d3mark(void);
void d3mchk(void);
void d3mclr(void);
void d3news(void);
void d3oafs(void);
void d3parm(int, char **);
void d3par2(void);
/*** KLUDGE NOTE: kns is TRUE if really new series.
*    THIS WILL BE REMOVED WHEN NEW RESET MECH IS READY ***/
void d3resetz(int rl, int kns);
void d3schk(void);
void d3snsa(void);
void d3snsprt(void);
void d3start1(char *vfnm, char *version);
void d3start2(char *vfnm);
void d3tchk(void);
void d3valu(struct VBDEF *);
void d3vchk(void);
void d3vitp(struct IMGTPAD *);
void d3vjnt(struct VCELL *);
void d3vwdw(struct VCELL *);
void d3vtch(struct VCELL *, stim_mask_t *);
void envdflt(void);
void rfdmtx(void);
#endif

/* Routines used on all nodes */
void d3gfsvct(struct SDSELECT *, struct CELLTYPE *, ui32);
void d3genr(void);
int  d3go(void);
void d3gsta(void);
int  d3imgt(int freq);
void d3inhi(void);
void d3msgl(void);
void d3msgr(void);
rd_type *d3nset(rd_type *);
void d3oflw(void);
int  d3plot(void);
void d3regenr(void);
void d3resetn(int rl);
int  d3rpgt(struct RFdef *);
void d3rstr(void);
void d3stat(void);
void d3term(struct exit_msg *, int);
void d3tsvp(int kblk);
void d3zsta(void);

/***********************************************************************
*                             CNS main()                               *
***********************************************************************/

/* This is the origin pointer to all the repertoire data.  In a
*  parallel computer it must never be broadcast because it is
*  different on every node.  It is placed here as a global so it
*  can be accessed from the d3nset() call in d3term().  */
rd_type *myprd;

int main(int argc, char *argv[]) {

#ifndef PARn
   ENVIRONMENT *E;            /* Ptr to environment */
   struct ARMDEF *pa;         /* Ptr to current arm */
   struct JNTDEF *pj;         /* Ptr to current joint */
   struct RFdef  *psvf;       /* Ptr to current savenet file */
   struct RFdef  *iru;        /* Pointer to replay file structure */
   struct VCELL  *pvc;        /* Ptr to current VCELL block */
   struct WDWDEF *pw;         /* Ptr to current window */
   char  *vfnm;               /* Ptr to version file name */
   float rtime;               /* Real elapsed time */
   int rc;                    /* Return code */
#endif

   struct exit_msg finex;     /* Used to control final exit msg */

   long itrsi;                /* Relative reset interval  */
   long its,lts;              /* Series loop counter */
   ui32 jfrz;                 /* Relative freeze interval */

#ifndef PARn
   byte lkd;         /* Print control for effectors, changes, senses */
#endif

/* End of main() local declarations */

/*=====================================================================*
*                                                                      *
*                       General Initialization                         *
*                                                                      *
*=====================================================================*/

   /* Give allocp routines access to NXDRTT,NXDRUT tables */
   nxtupush(NXDRTT, NXDRUT);

   /* Set for normal termination exit */
   finex.pit = NULL;
   finex.ec = finex.iv = 0;

#ifndef PARn
/*=====================================================================*
*                                                                      *
*  Following initializations done only on serial systems and host      *
*     node of parallel systems:                                        *
*                                                                      *
*=====================================================================*/

#if 0
   if (1) {    /*** EMERGENCY EARLY DEBUG ***/
      volatile int wct = 1;
      while (wct)    /* Wait for debugger to set wct = 0 */
         sleep(1);
      }
#endif

   /* Clear ROCKS error flag */
   RK.iexit = 0;

#ifdef DBGOUT
   /* Employ special method to divert stdout to a file--this method
   *  requires recompilation but is usable when Cns is run as a server
   *  where command-line output redirection is not available.  */
   dbgout(DBGOUT);
#endif
   /* Or it can be done by an environment variable--convenient when
   *  CNS is run from a script, e.g. under mpi, where redirection
   *  syntax is not straightforward.  Likewise for input.  */
   envout("CNS_OUTFILE");
   envin("CNS_INFILE");

/* Ignore signals that may be issued to this process unintentionally.
*  This code modified in R71 as follows:  We do not want to assume
*  that keyboard interaction or error shutdown originates on same
*  processor as host node.  Therefore, signals cannot be used for
*  these functions as in the old hybrid system.  Accordingly, just
*  ignore those and other signals that may come along that this
*  program cannot deal with.  */
#ifdef UNIX
   {  struct sigaction actdefs;
      actdefs.sa_handler = SIG_IGN;
      sigemptyset(&actdefs.sa_mask);
      actdefs.sa_flags = SA_RESTART;
      sigaction(SIGINT, &actdefs, NULL);
      sigaction(SIGWINCH, &actdefs, NULL);
      sigaction(SIGUSR1, &actdefs, NULL);
      sigaction(SIGUSR2, &actdefs, NULL);
      }
#endif

/* Allocate the global parameter blocks RP and RP0 */

   RP = (struct RPDEF *)allocpcv(Static, 1, IXstr_RPDEF|MBT_Unint,
      "RP global parameters");
   RP0 = (struct RPDEF0 *)allocpcv(Host, 1, sizeof(struct RPDEF0),
      "RP0 host parameters");

/* Part of overflow error handling we can set before RP alloc */

   e64set(E64_CALL, d3lime);

/* Start ROCKS I/O and the clock, print title & version information.
*  Note:  TITLE info is only available after d3parm is called, but we
*  want to print version info before any control cards are printed.
*  Solution:  Break d3start into two pieces, store version info for
*  later version file output.  */

   vfnm = getenv("CNS_VERSFILE");
   /* Print default title with compile date and run number,
   *  print version numbers of subroutine libraries.  */
   d3start1(vfnm, SVNVRS);
   /* Set general CNS defaults */
   d3dflt();
   /* Interpret BBDHOST card, TITLE card if any, PARMS card, and
   *  command-line parameters that may override the PARMS card.  */
   d3parm(argc, argv);

/* Register routines to handle kwscan type J,N conversion codes */

   kwsreg((void (*)(void *))d3rlnm,   0);
   kwsreg((void (*)(void *))getdecay, 1);
   kwsreg((void (*)(void *))gettimnw, 2);
   kwsreg((void (*)(void *))getnapc,  3);
   kwsreg((void (*)(void *))getpspov, 4);
   kwsreg((void (*)(void *))getcsel,  5);
   kwsreg((void (*)(void *))getcsel3, 6);
   kwjreg((void (*)(char *, void *))getthr, 1);
   kwjreg((void (*)(char *, void *))g1arb,  2);

   /* Wait for a debugger to be attached */
   while (RP->CP.dbgflags & DBG_CNS)
      rksleep(1,0);

#endif   /* !PARn */

#ifdef PAR
/* Initialize the NC data structure, which sets up for NCUBE-like
*  communications for all processes running on parallel machines,
*  and start the andmsg (debug output) and mfsr (graphics command
*  collector) if requested.  aninit() is the version for systems
*  running MPI tools and must be called on all nodes (but it
*  broadcasts the parameters from PAR0, can code zeros on PARn).  */

   {  ui32 initops = ANI_DBMSG;
#ifdef PAR0
      if (RP0->ANDrfl & RP_OKPLOT) initops |= ANI_MFSR;
      aninit(initops, RP->CP.mpidbgflags, RP->CP.mfddbgflags);
#else
      aninit(initops, 0, 0);
#endif
      }
   /* Parallel machine must have at least one comp node */
   if (NC.dim < 0) d3exit(NULL, NODENO_ERR, 0);
   /* Restore nxdr table stack written over by aninit */
   NC.jnxtt = 0;
   nxtupush(NXDRTT, NXDRUT);
#else  /* Serial */
   /* On serial machines, a negative coprocessor dimension will
   *  allow dual-mode library routines to tell what is going on.  */
   NC.dim = -1;
#endif /* PAR */

   /* Now that everybody has RP, can count overflows there
   *  (RP->ovflow is initially zero from callocpv())  */
   e64set(E64_FLAGS, &RP->ovflow);

#ifndef PARn         /* Back to Node 0 startup */

/* Copy title, version, and run number info to CNS_VERSFILE.  */
   d3start2(vfnm);

#ifdef BBDS
/* Initialize BBDS package */
   bbdsinit();
#endif

/* Interpret Group 0 and Group 1 control cards */

   d3grp0();                     /* Process Group 0 control cards */
   if (RP->nfdh > 0) rfdmtx();   /* Read feature-detection matrices */
   envdflt();                    /* Set environment defaults */
   d3grp1();                     /* Process Group 1 control cards */

   /* Implement command-line parameters that may override options
   *  from d3grp0() or d3grp1().  */
   d3par2();
   /* Set cell- and connection-type defaults that may depend on
   *  COMPAT=C mode.  */
   d3dflt2();

/* Calculate a few derived quantities and reorganize
*  arm and window chains into arrays for quick access.  */

   d3fix1();

/*=====================================================================*
*                                                                      *
*  Interpretation of group II control cards--                          *
*     Establish tree structure of repertoires and cell types           *
*     Link connections to their sources                                *
*     Locate excitatory and inhibitory output cells                    *
*     Link cells to conductances and conductances to ions              *
*     Allocate storage for repertoire and s(i) data                    *
*     Report repertoire structure                                      *
*                                                                      *
*=====================================================================*/

   d3grp2();                  /* Process group II control cards */
   d3schk();                  /* Sort and check sense blocks */
   d3tchk();                  /* Match connections to sources */
   d3vchk();                  /* Check values, senses, effectors */
   d3cchk();                  /* Link conductances and ions */
   d3mchk();                  /* Set modalities of all cell types */
   d3bgis();                  /* Set derived params for some RFs */
   d3gfcl(TRUE);              /* Check SIMDATA items and lists */
   d3lafs();                  /* Calc length of afferent sums--
                              *  must do this before d3allo().  */
   d3asck();                  /* Initialize for autoscaling */

/* Exit after all possible input errors have been found. */

   if (RK.iexit) d3exit(NULL, ROCKIO_ERR, (int)RK.iexit);

   d3allo();                  /* Allocate storage */
   if (RP->CP.runflags & RP_OKPRIN) d3echo();

/* N.B. Shared memory allocated after the d3echo call will be counted
*  in memsize(MPS_Static), but of course not included in the printed
*  total.  But it's OK, because all such memory (see list in box below)
*  is included in other totals that are also printed by d3echo. */

/*=====================================================================*
*  Final preparations for running the model--                          *
*     Allocate space for input array, TV images, kinesthetic and touch *
*        cells, and effector positions.                                *
*     Allocate space for values and match MODVAL blocks with sources.  *
*     Establish communications with real-world BBD.                    *
*     Set up stimulated environment.                                   *
*     Initialize graphics unless doing NOPLOT run                      *
*     Open replay, Darwin 1 input, and graph data files.               *
*=====================================================================*/

/* Allocate space for sensory broadcast area and establish pointers
*  to the components contained within it--input array, TV images,
*  Darwin 1 inputs, kinesthetic and touch sensors, and effector
*  positions.  This space is needed even in the serial version, it
*  just isn't broadcast in that case.  d3snsa also allocates space
*  for value and autoscale multipliers and matches virtual MODVAL
*  blocks with sources.  */

   d3snsa();

/* If restoring from SAVENET files, open them and check that headers
*     match current input.  If reading Darwin 1 data, open files and
*     check headers.  If reading a replay file, calculate record
*     length.  This call must follow d3snsa call so buffers can be
*     assigned.  */

   d3fchk();

/* Initialize environment--but only if needed */

   if (RP->nst) {
      E = RP0->Eptr = envset(d3file[SHAPELIB], d3file[REPLAY],
         d3file[PIXSTIM], RP0->parm1, RP0->pwdw1, RP->kx, RP->ky,
         RP0->mxobj, 0 /* Obsolete mxref */, RP->kcolor, (RP0->pgds &&
         RP0->pgds->svitms & GFIA) ? ENV_SAVALL : ENV_SAVOBJ);
      RP->kevtb |= EVTENV;
      } /* End if not NO_INPUT */

/* Catch any errors from above setups, including envset */

   if (RK.iexit) d3exit(NULL, ROCKIO_ERR, (int)RK.iexit);

/* Initialize graphics unless doing NOPLOT run
*  R79, 08/08/18, GNR - Bug fix:  If we had command-line NOXG and
*     there was also no metafile specified, it is really a NOPLOT
*     situation but newplt would try to do some plotting.  We can
*     turn off RP_OKLPLOT even this late and endplt will kill mfsr.
*     Code moved before membcst so comp nodes get NOPLOT state.
*/

   if (RP->CP.runflags & RP_OKPLOT) {
      char *mfnm = NULL;      /* Metafile name */
      char *stnn = NULL;      /* X graphics station name */
      int  itl;               /* For title truncation */
      int  gotmeta;           /* TRUE if !RP_NOMETA && have METAFILE */
      char tbuf[L_TITL+1];    /* Space for terminated title */

      /* Turn off graphics if RP_NOXG and also RP_NOMETA or just
      *  no metafile specified.  */
      gotmeta = !(RP->CP.runflags & RP_NOMETA) && d3file[METAFILE];
      if (RP->CP.runflags & RP_NOXG && !gotmeta) {
         RP->CP.runflags &= ~RP_OKPLOT;
         }
      else {
         /* Initiate graphics metafile if METAFILE card entered */
         if (gotmeta) {
            mfnm = d3file[METAFILE]->fname;
            RP0->pltbufl = max(RP0->pltbufl, d3file[METAFILE]->blksize);
            }
         /* Initiate X graphics unless NOXG entered on command line */
         if (!(RP->CP.runflags & RP_NOXG))
            stnn = RP0->hxterm ? getrktxt(RP0->hxterm) : "";
         /* Get run title to graphics */
         strncpy(tbuf, gettit(), L_TITL);
         for (itl=L_TITL-1; itl>0; --itl) {
            if (tbuf[itl] != ' ' && tbuf[itl] != '\0') break;
            }
         tbuf[itl+1] = '\0';
         setmf(mfnm, stnn, tbuf, NULL, RP0->pltbufl,
            RP->CP.mfddbgflags, '8', 0, 0);
         }  /* End else (graphics is on) */
      }

/* Initialize for savenet output */

   psvf = d3file[SAVREP_OUT];

/* Open replay file if needed */

   if (RP->CP.runflags & RP_REPLAY) {
      if (!rfopen(d3file[REPLAY], NULL,
            READ, BINARY, DIRECT, TOP,
            LOOKAHEAD, REWIND, RETAIN_BUFF, IGNORE,
            RP0->rprecl, IGNORE, NO_ABORT)) {
         cryout(RK_P1, "0***UNABLE TO OPEN REPLAY FILE.",
            RK_LN2, NULL);
         RK.iexit |= FILE_ERR;
         }
      iru = d3file[REPLAY];
      } /* End replay file open */

/* Wait for a debugger to be attached */
   while (RP->CP.dbgflags & DBG_ENDSET)
      rksleep(1,0);

#endif /* !PARn */

/*=====================================================================*
*      Comp nodes skip above setup code and resume working here        *
*=====================================================================*/

#ifdef PAR

/* Broadcast all shared data structures */

   RP = membcst(MPS_Static|MPS_Dynamic);
   if (RP->CP.dbgflags & DBG_MEMD) memdump(MPS_AllPools);
#endif  /* PAR */

/* Set node-dependent memory pointers.  This first call to d3nset
*  will use the initial ssck information stored by d3lafs above.  */

   myprd = d3nset(NULL);

/*=====================================================================*
*                 Generate or restore all repertoires                  *
*                                                                      *
*  Do final video, graphics, and file initialization that comes after  *
*  repertoire generation for timing or dependency reasons.             *
*=====================================================================*/

   if (RP->ksv & KSVRR) d3rstr();

   d3genr();

#ifndef PARn

#ifdef VVTV
/* Initialize for VVTV input--cameras having physical interface cards.
*  This code must come after tree broadcast and d3genr() but before
*  the GRAND LOOP.  Note:  This code should be moved to d3g1imin if
*  ever reactivated.  It was here to obtain execution overlap always
*  capturing next image while processing current one.
*
*  Changed 11/27/96 to omit any driver calls on comp nodes.
*  Changed 02/15/10 to loop over cameras, use specified parameters.
*     However, note that vvgrab,vvgchk are defined over all cameras.
*/

   {  struct TVDEF  *cam;        /* Ptr to current video camera */
      for (cam=RP->pftv; cam; cam=cam->pnxtv) {
         if ((cam->utv.tvsflgs & (TV_ON|TV_VVTV)) == (TV_ON|TV_VVTV)) {
            int isa  = (int)cam->utv.tvsa;
            int tvx0 = (int)cam->utv.tvrxo;
            int tvy0 = (int)cam->utv.trvyo;
            int tvxe = tvx0 + isa*(int)cam->utv.tvrx;
            int tvye = tvy0 + isa*(int)cam->utv.tvry;
            if ((rc = vvload((int)cam->utv.islot)) != 0)
               d3exit("VVLOAD", VIDEO_ERR, rc);
            if ((rc = vvinit((int)cam->utv.islot,
                  tvx0, tvxe-1, tvy0, tvye-1,
                  (VV_233 | VV_TV1 | VV_ATD), isa, 0)) != 0)
               d3exit("VVINIT", VIDEO_ERR, rc);
            }
         } /* End camera loop */
      } /* End cam local scope */
#endif

/* Perform tree-structure-verify-print if requested */

   if (RP0->n0flags & N0F_TSVP) d3tsvp(7);

#endif /* !PARn */

/* Allocate working storage for cross-response stats--all nodes */

   d3msgl();

/* Clear all values */

   if (RP->nvblk > 0) {
      d3zval(0);
      }

/* Quit now if found overflow errors during setup */

   if (RP->ovflow)
      d3exit("Setup", OVERFLOW_ERR, RP->ovflow);

/* Report time for end of repertoire generation */

#ifdef PAR
   isynch();  /* Synchronize */
#endif
#ifndef PARn
   rtime = (float)second();
   convrt(
      "(YW'0Network generation complete in ',J1F12.5,'seconds')",
      &rtime, NULL);
#endif

/*=====================================================================*
*                                                                      *
*     The grand loop.  Simulate the activity of the system.            *
*                                                                      *
*=====================================================================*/

/* Read and interpret group III control cards until cycle found */
TOP_OF_GRAND_LOOP:

#ifndef PARn
/* Check input deck and set termination flag */

   RP->CP.outnow = (outnow_type)d3grp3();
   RP->CP.trial = 0;          /* Kill stats output if no trials done */

/* Out of some superstition, this call is here, rather than in the
*  big initialization block below, because it might affect variables
*  in rpdef needed for d3save() or other node code between the RP
*  broadcast here and the big broadcast after the next section.  */

   d3bgis();                  /* Derived params for certain RFs */
   d3lafs();                  /* Calculate length of afferent sums */
   d3asck();                  /* Initialize for autoscaling */
   d3news();                  /* General initialization for d3go() */

#ifdef BBDS
/* If BBD exists, and this is first trial series, check that the
*  client(s) can provide any requested user-defined senses and output
*  effectors.  Then, if requested by the BBDHOST startup card, send
*  version info to specified client(s).  Note:  This code must follow
*  d3news() call where final ldat is set, hence the effcyc check.
*  Currently, bbdschk() abexits in case of error, normally returns 0,
*  not checked here.  Really should give an error return, use here to
*  call d3exit in due course.  */

   if (!RP->CP.effcyc) {
      bbdschk();
      if (RP0->vmsgwho)
         bbdsputm(ssprintf(NULL, "CNS Revision " SVNVRS
            ", %s, %s\n", bbdvers(), plotvers()), (int)RP0->vmsgwho);
      }
#endif

   if (RK.iexit) RP->CP.outnow |= OUT_ERROR_EXIT;
#endif /* !PARn */

#ifdef PAR
/* Broadcast RP data with any changes.  Actually, this has to be
*  a membcst because d3grp3 may have added new pointers in RP that
*  have not yet been hashed on the comp nodes.  First call d3clsm()
*  to be sure all cell lists are in the dynamic pool.  */

#ifdef PAR0
   d3clsm();
#endif
   membcst(MPS_Static|MPS_Dynamic);
#endif

/* If there was a Group III SAVE NETWORK card (but no SAVE NEWSERIES
*  card) do it now */

   if ((RP->ksv & (KSVS3|KSVNS|KSVOK)) == (KSVS3|KSVOK)) {
      d3save(D3SV_FMGRP3);
      RP->ksv &= ~KSVS3;
      }

/* All nodes quit if outnow set (e.g. by d3grp3() getting END card) */

   if (RP->CP.outnow > 0) goto END_TRIAL_LOOP;

/*---------------------------------------------------------------------*
*  Initialization that can change when new Group III cards read:       *
*     Store conductance activation tables.                             *
*     Set up for Cij, Mij, etc. plots.                                 *
*     Set environment printing and plotting options.                   *
*     Set up control variables for printing and plotting.              *
*     Set for superposition plots, movies, and grafdata files.         *
*     Check cell lists for SIMDATA, write segment 0 if needed.         *
*     Determine offsets to afferent sums and storage for phase dists.  *
*     Broadcast and recalc tree data for new run (new CYCLE card).     *
*     Allocate group mask arrays.                                      *
*     Prepare normalization tables for gconns.                         *
*     Set up for freeze options.                                       *
*     Print cycle parameters for this run.                             *
*---------------------------------------------------------------------*/

#ifndef PARn
   if (RP0->n0flags & N0F_MKTBLS) {
      d3ctbl(ON);             /* Store conductance activation tables */
      }
   d3ijpi();                  /* Set up for Cij.Mij,etc. plots */

   /* Set env variables that can be changed by d3grp3 or d3chng */
   if (RP->nst) {
      int egrid = (RP->kplot & PLGRD) ? (int)RP0->grids : 0;
      envopt(E, (double)RP0->epr, (double)RP->stdlht,
         RP0->ept, RP0->eilo, RP0->eihi, RP0->kux, RP0->kuy,
         egrid, RP0->ecolgrd, RP0->ecolstm,
         RP->kplot & PLSQR ? ENV_SQUARE : ENV_CIRCLE,
         RP->kplot & PLCOL ? TRUE : FALSE,
         RP->kplot & PLRTR ? TRUE : FALSE,
         RP->kplot & PLNOT ? TRUE : FALSE,
         RP->kdprt & PROBJ ? (RP->kdprt & PRSPT ? 2 : 1) : FALSE);
      /* Check whether object reference numbers will overflow */
      if (RP0->n0flags & N0F_VTCHSU && envgmxr(E) > 255)
         cryout(RK_E1, "0***MXREF > 255 WITH TOUCH.", RK_LN2, NULL);
      envser(E);
      }
#endif

/* Set up control variables for printing and plotting:
*  lkd = kdprt filtered by global print switch and
*        existence of requested items.
*  kpl = preset for environment, superposition, dynamic
*        configuration, and individual repertoire plots
*        (kpl1 is same thing filtered by plot timers).
*  kpr = preset for environment print.
*/

   RP->kpr = RP->kpl = OFF;
   if (RP->CP.runflags & RP_OKPRIN) {
#ifndef PARn
      lkd = RP->kdprt;
      if (!(RP->narms|RP->nwdws)) lkd &= ~PRREP;
      if (!RP0->ljnwd)            lkd &= ~PRCHG;
      if (!RP0->pvc1)             lkd &= ~PRSNS;
      if (!RP0->nautsc)           lkd &= ~PRAUS;
#endif
      if (RP->CP.runflags & RP_REPPR) RP->kpr |= KPRREP;
      }
#ifndef PARn
   else {
      lkd = 0;
      }
#endif

   if (RP->CP.runflags & RP_OKPLOT) {
      RP->kpl = (RP->kplot & (PLSUP|PLVTS) ?
         (KPLSUP|KPLMFSP) : (KPLIRP|KPLVCP)) |
         (RP->kplot & PLDYN ? KPLDYN : 0) |
         (RP->kplot & PLEDG ? KPLEDG : 0);

#ifndef PARn
      /* Set for movie (unless plotting disabled).
      *  If a movie mode was entered as a PARM or on a Group III
      *  MOVIE card, use that.  If nothing was entered, use MM_MOVIE
      *  if online and MM_BATCH if offline.  Since nexp may change
      *  at Group III time, issue setmovie() call in all cases.
      *  V8E, 04/14/09, GNR - Allow user to request BATCH mode */
      if (RP0->mmode == MM_NOCHG) RP0->mmode =
         (RP->CP.runflags & RP_ONLINE) ? MM_BATCH : MM_MOVIE;
      setmovie((int)RP0->mmode, (int)RP0->mmdev, (int)RP0->nexp);
      RP0->n0flags &= ~N0F_MPARME;
#endif /* !PARn */
      } /* End if plotting enabled */

/* Establish base bits in kevtb.  */

   RP->kevtb &= ~(EVTWSD|EVTPRB);
   if (RP->pfprb) RP->kevtb |= EVTPRB;

#ifndef PARn
/* Prepare for SIMDATA file by making general header and optional
*  segment 0 header (once only).  If d3gfhd() cannot open the output
*  file, it sets iexit but also sets KSDNOK to disable other activity
*  for SIMDATA until the exit occurs.  Because d3gfrl() edits the cell
*  and connection-level svitms codes, it must be called before the
*  tree broadcast, but d3gfsv[ct] must be called afterwards.  */

   if (!(RP->ksv & (KSDNE|KSDNOK|KSDOFF))) {
      struct CELLTYPE *il;
      d3gfcl(FALSE);                /* Check cell lists for SIMDATA */
      RP->CP.runflags &= ~RP_KSDSG; /* Kill SIMDATA write if no data */
      if (!(RP->ksv & KSDHW)) {
         d3gfhd(RP0->pgds);
         /* Record header for SIMDATA segment 0 (Lij, Dij, and ABCD)
         *  if needed.  But test whether d3gfhd set the KSDNOK bit */
         if (!(RP->ksv & KSDNOK)) {
            /* Calc and store record lengths for segment0 */
            for (il=RP->pfct; il; il=il->pnct)
               il->lsd1c0 = d3gfrl(il, (GFLIJ|GFDIJ|GFABCD));
            d3gfsh(RP0->pgds, (GFLIJ|GFDIJ|GFABCD));
            } /* End !KSDNOK */
         } /* End !KSDHW */
      /* Calc and store record lengths for later segments */
      for (il=RP->pfct; il; il=il->pnct)
         il->lsd1c = d3gfrl(il, ~(GFLIJ|GFDIJ|GFABCD));
      } /* End !(KSDNE|KSDNOK|KSDOFF) */

/* Set offsets into afferent sums and allocate phase distributions */

   d3oafs();

#endif /* !PARn */

/* Broadcast tree data for new run (new CYCLE card) */

#ifdef PAR
#ifdef PAR0
   d3clsm();
#endif
   membcst(MPS_Static|MPS_Dynamic);
   if (RP->CP.dbgflags & DBG_MEMD) memdump(MPS_AllPools);
#endif

/* Recalc node-dependent data */

   d3nset(myprd);

/* Allocate GRPNO-dependent storage for cross-response stats */

   d3msgr();

/* Prepare normalization tables for gconns (must come after
*  d3nset() call or separate items in ib->iu1).  */

   d3inhi();

/* If doing an online replay, allow it to cycle indefinitely */

   if ((RP->CP.runflags&(RP_REPLAY|RP_ONLINE)) == (RP_REPLAY|RP_ONLINE))
      RP->ntr1 = RP->ntr2 = 999999U;

/* Set up for freeze options:
*  ifrz < 0: Extra series, freeze on last series
*  ifrz = 0: Amplify on all series
*  ifrz > 0: Freeze every ifrz'th series, no extra series.  */

   if (RP->ifrz < 0) {
      jfrz = RP->nts;
      lts = 0;
      }
   else {
      lts = 1;
      if (!RP->ifrz) jfrz = RP->nts + 1;
      else jfrz = (ui32)RP->ifrz;
      }

/* At this point, RP_KSDSG bit can only be set if write of segment 0
*  is pending.  This needs to be done on all nodes, then d3gfsh can
*  be done for the regular data segment.  An isynch() is not needed
*  because d3gfsvct() effectively does one.  */

   if (RP->CP.runflags & RP_KSDSG) {
      struct CELLTYPE *il;
#ifdef PARn
      for (il=RP->pfct; il; il=il->pnct)
         d3gfsvct(NULL, il, (GFLIJ|GFDIJ|GFABCD));
#else
      d3gfsv(RP0->pgds, (GFLIJ|GFDIJ|GFABCD));
      for (il=RP->pfct; il; il=il->pnct)
         d3gfsvct(RP0->pgds, il, (GFLIJ|GFDIJ|GFABCD));
#endif
      }

   if (!(RP->ksv & (KSDNE|KSDNOK|KSDOFF))) {
#ifndef PARn
      ++RP0->pgds->gdseg;
      d3gfsh(RP0->pgds, ~(GFLIJ|GFDIJ|GFABCD));
#endif
      } /* End if (RP0->pgds) */

#ifndef PARn
/* Last chance to exit before beginning trials */

   if (RK.iexit) d3exit(NULL, ROCKIO_ERR, (int)RK.iexit);

/* Print cycle parameters for this run: */

   if (RP->CP.runflags & RP_OKPRIN) {
      ldiv_t tsrq;
      size_t szdmem = memsize(Dynamic);
      cryout(RK_P2, "0Cycle parameters for this run:",
         RK_NFSUBTTL+RK_LN2, NULL);
      convrt("(4X'Number of series',UJ14,4X'Freeze interval',UJ15/"
         "4X'Trials/normal series',IJ10,4X'Trials/freeze series',IJ10/"
         "4x'Cycles/normal trial',UH11,4X'Cycles/freeze trial',UH11)",
         &RP->nts, &jfrz, &RP->ntr1, &RP->ntr2, &RP->ntc1, &RP->ntc2,
         NULL);
      convrt("(4X'Mean stim noise',B24IJ15.4,"
         "4X'Std dev of noise',B28IJ14.4"
         "/4X'Stim noise fraction',B24IJ11.4,4X'Stim noise seed',IJ15)",
         &RP->snmn, &RP->snsg, &RP->snfr, &RP->snseed, NULL);
      /* Step time is stored internally in microseconds so we can
      *  express decimal fractions of msec exactly.  */
      tsrq = ldiv(RP->timestep,1000);
      convrt("(4X'Step time (msec)',UL10,H.@UL3,4X,'Dynamic "
         "storage',UZ15)", &tsrq.quot, &tsrq.rem, &szdmem, NULL);
      convrt("(W4X'Shared work area',UL14)", &RP0->cumsws, NULL);
      cryout(RK_P2, "    ", RK_NTSUBTTL+RK_LN1+4, NULL);
      } /* End if OK_PRIN */
#endif   /* Not PARn */

/*=====================================================================*
*  Perform nts series                                                  *
*  Note: The general scheme during a replay is that the series and     *
*     trial for loops are allowed to play out.  If we are offline,     *
*     the replay will remain in synch with the control cards.  The     *
*     only use I can imagine for this is to make hard-copy plots from  *
*     a previous run.  If we are online, ntr is made very large so we  *
*     effectively have one long series.  CP.ser and trial are loaded   *
*     from the replay file and used to label all output.  Thus, if we  *
*     want to change any plot options, etc., we must interrupt with a  *
*     PX command (or blue button) and enter appropriate commands.      *
*  Changes to CP only need be done on host, as whole thing is broad-   *
*     cast to all nodes once per trial later on.                       *
*=====================================================================*/

#if defined(VVTV) && !defined(PARn)
/* Code for VVTV video and senses:  Grab first set of TV images before
*  loop starts.  Next grab will occur as soon as data have been trans-
*  ferred to CNS, thereby maximizing overlap of data acquisition and
*  neural simulation.  Video grabbing is done regardless of which VVTV
*  cameras may have TV_ON flags set because auto time averaging must
*  be the same for all cameras and is calculated based on the first
*  camera.  The dosynch flag is off, because no nodes are yet getting.
*  */

   if (RP->ortvfl & TV_VVTV &&
         (rc = vvgrab((int)RP->pftv->utv.tvta, OFF)) != 0))
      d3exit("VVGRAB", VIDEO_ERR, rc);
#endif

/* Start gathering timer statistics */

   stattmr(OP_RESET_ALL, 3);
   stattmr(OP_PUSH_TMR, DFLT_TMR);

/* Now perform the series loop */

   RP->CP.outnow = 0;
   for (its=lts; its<=RP->nts; its++) {
      RP->CP.ser++;

/* Determine whether this will be a freeze series */

      itrsi = 0;
      if (its % jfrz != 0) {
         RP->CP.runflags &= ~RP_FREEZE;
         RP->ntr = RP->ntr1, RP->ntc = RP->ntc1;
         }
      else {
         RP->CP.runflags |= RP_FREEZE;
         RP->ntr = RP->ntr2, RP->ntc = RP->ntc2;
#ifndef PARn
         cryout(RK_P3, "0-->This is a freeze series", RK_LN2, NULL);
#endif
         }

/* Set persistence parameter for Euler integration, but turn integ'n
*  off if ntc == 1.  (No routine is called between here where ntc is
*  set and the beginning of the trial loop, so this extra loop over
*  celltypes just has to go here (or could be done redundantly at
*  start of d3go).)  */

      RP->CP.runflags &= ~RP_DOINT;
      {  struct CELLTYPE *il;
         si32 doint, tt = (si32)max(RP->ntc,1);
         RP->eudt = (Sod + (tt >> 1))/tt;
         doint = tt > 1 && RP->kinteg & KIEUL;
         if (doint) RP->CP.runflags |= RP_DOINT;
         for (il=RP->pfct; il; il=il->pnct)
               il->Dc.omegau = doint ?
            Sod - mrsrsl(RP->eudt, il->Dc.epsilon, FBod) :
            il->Dc.omega;
         } /* End if Eular integration */

/* If there was a SAVE NEWSERIES card, do it now */

      if ((RP->ksv & (KSVNS|KSVOK)) == (KSVNS|KSVOK)) {
         d3save(D3SV_NEWSER);
         }

/* Environment printing and plotting flags */

      if (((RP->CP.runflags & (RP_OKPLOT|RP_NOINPT)) == RP_OKPLOT) &&
            ((RP->kplot & PLALL) ||
            ((RP->kplot & PLFRZ) && (RP->CP.runflags & RP_FREEZE))))
         RP->kpl |=  KPLENV;
      else
         RP->kpl &= ~KPLENV;

#ifndef PARn
      if (((RP->CP.runflags & (RP_OKPRIN|RP_NOINPT)) == RP_OKPRIN) &&
            ((RP0->kiarr & PIAALL) ||
            ((RP0->kiarr & PIAFRZ) && (RP->CP.runflags & RP_FREEZE))))
         RP->kpr |=  KPRENV;
      else
         RP->kpr &= ~KPRENV;
      RP0->exisn = 0;

/* Establish reset mode (preliminary) */

      /* So old inputs will work, if user did not enter a TRIAL
      *  level, and ntc != 1, use omega1-style decay only.  */
      RP0->rlvl[lvtuse] = (RP0->rlvl[lvtent] >= RL_NONE) ?
         RP0->rlvl[lvtent] : ((RP->ntc == 1) ? RL_NONE : RL_OLDTRIAL);
      RP->CP.rlnow = RP0->rlvl[lvnow];
      if (RP0->rlvl[lvser] > RP->CP.rlnow)
         RP->CP.rlnow = RP0->rlvl[lvser];
      if (RP0->rlvl[lvtuse] > RP->CP.rlnow)
         RP->CP.rlnow = RP0->rlvl[lvtuse];

      /*** TEMPORARY CODE:  Do series reset (to reset cameras)
      **** before trial loop actually reads an image  ****/
      d3resetz(RL_SERIES, 1);

/* If any series-time images are called for, get and preprocess now */

      if (RP->ortvfl & (TV_SerVVTV|TV_SerBBD|TV_SerUPGM)) {
         rc = d3imgt((int)UTVCFr_SERIES);
         if (rc) d3exit("D3IMGT", VIDEO_ERR, rc);
         }

/* Clear previous page subtitle */

      cryout(RK_P1, "0   ", RK_NFSUBTTL+RK_LN2, NULL);
#endif

/* Restart all probe cycles */

      {  struct PRBDEF *pprb;
         for (pprb=RP->pfprb; pprb; pprb=pprb->pnprb)
            pprb->ipptr = pprb->iprbcyc = 0;
         } /* End pprb local scope */

/* Clear statistics for this group of trials (all nodes)  */

      d3zsta();

/*---------------------------------------------------------------------*
*  Perform ntr trials                                                  *
*----------------------------------------------------------------------*
*  Set a triggered subtitle for the stimulus number                    *
*     (do now so any error from envget will be labelled).              *
*  Evaluate all trial timers.                                          *
*  Update contents of the environment and read environmental senses.   *
*  Reset all the arms, windows, values, and cells if reset time.       *
*  Set modality sense mark bits according to objects on input array.   *
*     If no GRPNOs entered, record in GRPDEF arrays from object ign.   *
*     Record highest value of any object in modality block for ENVVAL. *
*  Update virtual senses, Darwin 1, TV, and BBD data, and value.       *
*                                                                      *
*  Notes:  All code that references isn checks for and ignores out-of- *
*     bounds values, but all ign's are valid (unless used for KAMCG)-- *
*     code in d3stat works on number of unique values of ign, which    *
*     need not be < pmdlt->ncs.                                        *
*  If a series is to be terminated prematurely, RP->CP.outnow must     *
*     be set to a reason code so the termination can be communicated   *
*     to all nodes.  This same flag is used to break out of the nested *
*     trial and series loops.                                          *
*  A slightly subtle performance note:  stuff done on host that        *
*     does not participate in the RP+Input broadcast (e.g. the PRREP   *
*     output) should be postponed until after the broadcast so it      *
*     can execute in parallel with the slow comp node stuff.           *
*---------------------------------------------------------------------*/

      RP->kevtu = RP->kevtb;
      for (RP->CP.trial=1; RP->CP.trial<=RP->ntr; RP->CP.trial++) {

         RP->ovflow = 0;      /* Clear overflow flags for this trial */
#ifndef PARn
         ++RP0->trials0;      /* Increment global trial number */
         ++RP0->trialse;      /* Increment trials since "event" */

         if (RP->CP.runflags & RP_REPLAY) {
            /* Locate to start of data for this trial */
            rfseek(iru, RP->CP.rpirec*RP0->rprecl, SEEKABS, ABORT);
            /* Read series and trial numbers */
            RP->CP.ser = rfri4(iru);
            RP->CP.trial = rfri4(iru);
            if (iru->lbsr == ATEOF) {
               d3grex(NULL, REPLAY_EOF_ERR, 0);
               RP->CP.outnow |= OUT_GRACEFUL_EXIT;
               goto RENDEZVOUS_1;
               }
            /* Remove the new format marker from the series number */
            RP->CP.ser &= SI32_MAX;
            }

/* Set print subtitle with current trial number.  This must be done
*  after reading replay header, but before calling envget or envcyc,
*  which may print error messages.  This same title string will be
*  used to label superposition plots, envplots, and SAVENET files.
*  d3rpsi and d3dprt output must add cycle number if needed, as room
*  for this was not reserved in stitle for plots and SAVENET headers.
*/

         sconvrt(RP0->stitle, "(8H0Series J0UJ8,8H  Trial J0UJ8)",
            &RP->CP.ser, &RP->CP.trial, NULL);
         if (RP0->trials0 != RP->CP.trial)
            sconvrt(RP0->stitle + strnlen(RP0->stitle, LSTITL),
               "(2H (J0UJ8H))", &RP0->trials0, NULL);
         /* Add 128 to decimal code to turn on RK_DSF bit */
         sconvrt(RP0->stitle + strnlen(RP0->stitle, LSTITL),
            "(2H (J1UW12.132,3Hms))", &RP->CP.runtime, NULL);
         RP0->lst = (short)strnlen(RP0->stitle, LSTITL);
         cryout(RK_P1, RP0->stitle, RK_NFSUBTTL+RK_LN2+RP0->lst, NULL);

/* Evaluate timers based on trial number, as these
*  may be used to control creation of stimuli.  */

         d3evt1();

/* Initialize mdltvalu values and clear modality stimulus sense
*  bits as we begin to collect data for new sensory broadcast.  */

         d3mclr();

/* If the simulated environment exists, update it now (host only) */

         if (RP->nst) {
            if (RP->CP.runflags & RP_REPLAY) {
               if (envget(E, RP->pstim) == ENV_FATAL)
                  d3exit(NULL, REPLAY_ERR, 0);
               } /* End if replay */
            else if (RP->kevtu & EVTENV &&
                  envcyc(E,RP->pstim,RP0->pjnwd) & RP0->krenvev &&
                  RP0->rlvl[lvevnt] > RP->CP.rlnow)
               RP->CP.rlnow = RP0->rlvl[lvevnt];
            /* Mark objects seen on input array */
            d3mark();
            }

#ifdef BBDS
/* Read values from BBDs now, as protocol dictates these are read
*  before BBD trial cameras.  Below, these may be used to trigger
*  a reset via VALUE criteria.  */

         if (RP->kevtu & EVTVAL && bbdsgetv()) {
            RP->CP.outnow |= OUT_USER_QUIT;
            goto RENDEZVOUS_1;
            }
#endif

/* Collect trial-time UPGM images unless disabled for event trial.
*  (Unnecessary to check ortvfl--off bits are off in kevtu)  */

         if (RP->kevtu & (EVTBTV|EVTVTV|EVTUTV) &&
               d3imgt((int)UTVCFr_TRIAL)) {
            RP->CP.outnow |= OUT_USER_QUIT;
            goto RENDEZVOUS_1;
            }

/* Mark stimulus info for cameras.  Code formerly in d3imgt to do this
*  was moved here so info can be captured for Series-frequency cameras
*  that do not execute d3imgt on every trial.  */

         if (RP->ortvfl & TV_ON) {
            struct TVDEF *cam;      /* Ptr to a camera */
            for (cam=RP->pftv; cam; cam=cam->pnxtv) {
               struct UTVDEF *putv = &cam->utv;
               if (!(putv->tvsflgs & TV_ON)) continue;
               if (cam->ptvmdlt) d3mark1(cam->ptvmdlt,
                  putv->isn, putv->ign, putv->iadapt);
               }
            }

/* Evaluate timers which may depend on stimuli presented.  Check
*  whether a timer reset is to be initiated.  If the programmed
*  reset timer goes off, reset the trial timer so its reset doesn't
*  happen at once.  */

         d3evt2();
         if ((RP->CP.runflags & (RP_TMRSET|RP_EVTRL)) == RP_TMRSET &&
               (bittst(RP->CP.trtimer, (long)RP->jrsettm) ||
               (RP0->ntrsi > 0 && --itrsi <= 0))) {
            itrsi = RP0->ntrsi;
            if (RP0->rlvl[lvtimr] > RP->CP.rlnow)
               RP->CP.rlnow = RP0->rlvl[lvtimr];
            }

/* Evaluate environment-dependent value schemes.  Must be done
*  after environment update but before value reset checking.  */

         if (RP0->pvblke) d3valu(RP0->pvblke);

/* Determine whether a value reset event has occurred, but don't
*  bother if reset level is already that high (it always will be
*  RL_ISKIP if this is already a special event trial.  */

         if (RP0->rlvl[lvevnt] > RP->CP.rlnow) {
            struct VALEVENT *pvev;
            struct VDTDEF   *val;
            for (pvev=RP0->pvev1; pvev; pvev=pvev->pnve) {
               val = RP->pvdat + pvev->ivdt - 1;
               if (pvev->veops & VEV_GE) {
                  if (val->vcalcnow >= pvev->evtrig)
                     RP->CP.rlnow = RP0->rlvl[lvevnt];
                  }
               else {
                  if (val->vcalcnow <= pvev->evtrig)
                     RP->CP.rlnow = RP0->rlvl[lvevnt];
                  }
               } /* End VALEVENT loop */
            } /* End checking for value event reset */

/* Reset if not replay and a timed or environment reset has been
*  requested or it is a new series.  (Do d3resetz now, comp nodes
*  will call d3resetn() after RP->CP is broadcast below).  */

         if (RP->CP.rlnow == RL_ISKIP) RP->CP.rlnow = RL_NONE;
         if (RP->CP.rlnow > RL_CYCLE) d3resetz(RP->CP.rlnow, 0);

#ifdef PAR0
/* Store arm and window positions for broadcast */

         if (RP0->cumnaw) {
            float *fptr = (float *)RP->paw;
            pw = RP0->pwdw1;
            /* Gather positional data for all arms and windows. */
            /* Note: contiguity of x,y,w,h data is assumed here. */
            for (pa=RP0->parm1; pa; pa=pa->pnxarm,fptr+=BARM_VALS)
               memcpy((char *)fptr, (char *)&pa->armx, BARM_SIZE);
            for (pw=RP0->pwdw1; pw; pw=pw->pnxwdw,fptr+=BWDW_VALS)
               memcpy((char *)fptr, (char *)&pw->wx, BWDW_SIZE);
            } /* End if cumnaw */
#endif /* PAR0 */

/* Update any BBD senses */

#ifdef BBDS
         if (RP->kevtu & EVTSNS && bbdsgets()) {
            RP->CP.outnow |= OUT_USER_QUIT;
            goto RENDEZVOUS_1;
            }
#endif

/* Update virtual senses according to type.
*  Rev, 11/09/10, GNR - Add mark1 call for USRSRC types
*                       (Done for others in d3vtch, etc.)
*/

         for (pvc=RP0->pvc1; pvc; pvc=pvc->pnvc) {
            switch (pvc->vtype) {

            case VJ_SRC:      /* Joint kinesthesia */
               d3vjnt(pvc);
               break;
            case VW_SRC:      /* Window kinesthesia */
               d3vwdw(pvc);
               break;
            case VT_SRC:      /* Touch */
               d3vtch(pvc, RP->pstim);
               break;
            case ITPSRC:      /* Image touchpad */
               d3vitp((struct IMGTPAD *)pvc);
               break;
            case USRSRC:      /* User-programmed (currently BBD) */
               /* Note:  value not currently implemented here */
               if (pvc->pvcmdlt) d3mark1(pvc->pvcmdlt,
                  pvc->isn, pvc->ign, 0);
               if (RP->kdprt & PROBJ) convrt("(P4X,J1A27,"
                  "'RETURNED OBJECT ',J1UH6,'FROM CLASS ',J0UH6)",
                  fmtvcn(pvc), &pvc->isn, &pvc->ign, NULL);
               break;
               } /* End sense switch */
            } /* End sense block loop */

#endif      /* Not PARn */

/* Broadcast CP, IA, TV, value, sense, modality, effector data */

#ifdef PAR
#ifndef PARn
RENDEZVOUS_1:     /* (Here to notify nodes of errors) */
#endif
         blkbcst(&RP->CP, ttloc(IXstr_CPDEF), 1, CP_CYCLE_MSG);
         if (RP->CP.outnow > 0) goto END_TRIAL_LOOP;
         membcst(MPS_Shared);
#else             /* Serial test for exit request */
         if (RP->CP.outnow > 0) goto END_TRIAL_LOOP;
#endif /* PAR */

/* Complete network data portion of reset now that runflags have been
*  broadcast to all nodes.  Implement ALLSERIES value reset also.
*  Even if no reset is scheduled, call d3resetn() because it may be
*  necessary to update Izhikevich 'u' or Brette-Gerstner 'w' due to a
*  change in parameters by d3chng().  */

         d3resetn(RP->CP.rlnow);
         if (RP->CP.runflags & RP_RVALL && RP->CP.trial == 1)
            d3zval(VBVOPBR);

/* Activate SIMDATA saving based on whether segment has contents */

         if (RP->CP.runflags & RP_KSDSG) RP->kevtu |= EVTWSD;

/* Determine whether superposition plot, configuration plot,
*  and/or separate environmental plot should occur for this
*  stimulus */

         RP->kpl1 = RP->kpl;
         if (!bittst(RP->CP.trtimer, (long)RP->jsptim))
            RP->kpl1 &= ~KPLSUP;
         if (!bittst(RP->CP.trtimer, (long)RP->jspmftim))
            RP->kpl1 &= ~KPLMFSP;
         if (!bittst(RP->CP.trtimer, (long)RP->jdcptim))
            RP->kpl1 &= ~KPLDYN;
         if (!bittst(RP->CP.trtimer, (long)RP->jeptim))
            RP->kpl1 &= ~KPLENV;
         if (!bittst(RP->CP.trtimer, (long)RP->jvcptim))
            RP->kpl1 &= ~KPLVCP;

#ifndef PARn
/* Print arm and window coordinates if requested  */

         if (lkd & PRREP) {
            int ia,iw;
            char hpwa = '0';
            if (RP->kdprt & PRSPT) spout(2*RP->narms + RP->nwdws);
            for (pa=RP0->parm1,ia=1; pa; pa=pa->pnxarm,ia++) {
               int ljntb = sizeof(struct JNTDEF)/sizeof(float);
               int lnjnts = pa->njnts; /* Convrt expects int R item */
               pj = pa->pjnt;
               convrt("(A1,'Arm',I2,' joint angles:',R=<F16.2>/6X,'"
                  " X,Y coords:  ',R=<F8.2,F8.2>)",
                  &hpwa, &ia, &lnjnts, &ljntb, &pj->u.jnt.ja,
                  &lnjnts, &ljntb, &pj->u.jnt.jx, &pj->u.jnt.jy, NULL);
               hpwa = ' ';
               }  /* End arm loop */

            for (pw=RP0->pwdw1,iw=1; pw; pw=pw->pnxwdw,iw++) {
               convrt("(A1,'Window',I2,' coords:   ',4*F8.2)",
                  &hpwa, &iw, &pw->wx, &pw->wy, &pw->wwd, &pw->wht,
                  NULL);
               hpwa = ' ';
               } /* End window loop */

            } /* End if lkd & PRREP */

/* Print sense and autoscale data if requested */

         if (lkd & PRSNS) d3snsprt();
         if (lkd & PRAUS) d3aprt();

#endif /* ndef PARn */

/* Initialize plotting package to prepare for superposition plot.
*  This must be done before d3go, which may invoke d3rplt.  */

         if (RP->kpl1 & (KPLSUP|KPLMFSP)) {
            int ngc, kout = 0;
            if (!(RP->kpl1 & KPLSUP))  kout |= SKP_XG;
            if (!(RP->kpl1 & KPLMFSP)) kout |= SKP_META;
            ngc = d3ngph(RP->spwid, RP->sphgt, 0, 0, "DEFAULT",
                  RP->colors[CI_BOX], "CLIP", kout);
            RP->CP.outnow |= ngc;
            if (ngc >= OUT_USER_QUIT) goto RENDEZVOUS_2;
#ifndef PARn
            /* Plot TV images, if any */
            if (RP->ortvfl & TV_ON && !(RP->CP.runflags & RP_REPLAY))
               tvplot();
#endif
            } /* End if (KPLSUP|KPLMFSP) */

/*---------------------------------------------------------------------*
*              If doing a replay, read replay information              *
*---------------------------------------------------------------------*/

         if (RP->CP.runflags & RP_REPLAY) {
            int ii;

/* Read a transcript of the current trial from a replay file.
*  Note:  When seriously implementing replays, be sure input
*     virtual cells are read before the general broadcast above.
*  Note: It is assumed here that d3rpgt exchanges the data.
*     The argument will eventually be the file number. */

            /* Read repertoires, increment record number */
#ifndef PARn
            ii = d3rpgt(iru);
            RP->CP.rpirec += RP0->rprecp;
#else
            ii = d3rpgt(NULL);
#endif
            /* d3rpgt stub calls d3exit, but this is JIC */
            if (ii != 0) d3exit(NULL, REPLAY_ERR, 0);
#ifndef PARn
            /* Read changes array */
            if (RP0->ljnwd > 0) {
               if (!rfread(iru, RP0->pjnwd,
                     sizeof(float)*RP0->ljnwd, ABORT)) {
                  RP->CP.outnow = OUT_ERROR_EXIT;
                  goto RENDEZVOUS_2;
                  }
               }

            /* Read the value array */
            if (RP->nvblk > 0) {
               if (!rfread(iru, RP->pvdat, RP0->cumval, ABORT)) {
                  RP->CP.outnow = OUT_ERROR_EXIT;
                  goto RENDEZVOUS_2;
                  }
               }
#endif
            } /* End reading replay info */

/*---------------------------------------------------------------------*
*       Otherwise, perform ntc cycles (one trial) of simulation        *
*---------------------------------------------------------------------*/


         else {
            stattmr(OP_PUSH_TMR, CALC_TMR);
            RP->CP.outnow |= d3go();
#if 0    /*** DEBUG d3grex calls ***/
         if (RP->CP.trial == 5) d3grex("Trial 5 test", 497, 1);
#endif

#ifndef PARn
#ifdef BBDS
            /* If there are any outputs to BBDs, d3go will have
            *  put the requisite values in the changes array,
            *  so now just transmit them */
            if (RP->kevtu & EVTEFF) bbdspute(FALSE);
#endif
#endif /* Not PARn */

            stattmr(OP_POP_TMR, 0);
            if (RP->CP.outnow >= OUT_USER_QUIT)
               goto RENDEZVOUS_2;
            } /* End if replay else simulation */

/*---------------------------------------------------------------------*
*Print/plot output that occurs for either replay or current simulation.*
*---------------------------------------------------------------------*/

         {  int plrc = d3plot();
            RP->CP.outnow |= plrc;
            if (plrc >= OUT_USER_QUIT)
               goto RENDEZVOUS_2;
            }

#ifndef PARn         /*** OPEN COMP NODE EXCLUSION BLOCK ***/

/* Print changes array if requested */

         if (lkd & PRCHG) {
            if (RP->kdprt & PRSPT) spout((RP0->ljnwd + 9)/10);
            convrt("('0Changes array: '#|(10F8.2;/))",
               &RP0->ljnwd, RP0->pjnwd, NULL);
            }

/* See if value conditions are satisfied for canonical trace
*  or grip modes.  Required also in replays. */

         d3cnmd(RP->pvdat);

/*---------------------------------------------------------------------*
*             End-of-trial updating and breakout checking              *
*---------------------------------------------------------------------*/

/* Advance run time */

         RP->CP.runtime = jaul(RP->CP.runtime, RP->timestep);

/* Check for user intervention (currently VM opsys only) */

#ifdef VM
         {  int ipxq = d3pxq(FALSE);
            RP->CP.outnow |= ipxq;
            if (ipxq > 0)
               goto RENDEZVOUS_2;
            }
#endif

/* Restore trial-level reset in case event not found */

         RP->CP.rlnow = RP0->rlvl[lvnow] = RP0->rlvl[lvtuse];

/* If a special event trial just completed, resume normal cycling */

         if (RP->CP.runflags & RP_EVTRL) {
            RP->kevtu = RP->kevtb;
            RP->CP.runflags &= ~(RP_EVTRF|RP_EVTRL);
            RP0->rlvl[lvnow] = RP0->rlvl[lvtdef];
            RP0->trialse = 0;
            }

/* BBD events may be used to cue input of BBDCFr_EVENT images, invoke
*  special event trials (basically for recording SIMDATA and METAFILE
*  data after an event) and/or force a network reset.  In all these
*  cases, now, at the end of a trial, is the time to check whether an
*  event has been signalled.  If so, download any BBDCFr_EVENT images
*  first.  Then, if a special event trial is being started, defer the
*  event reset until the next trial.  Otherwise, if a BBD event reset
*  is enabled, set rlnow immediately.  If it is an RL_NEWSERIES reset,
*  exit the trial loop--the reset will be performed at the beginning
*  of the new series.
*  It is assumed that RENDEZVOUS_2 immediately follows this block.  */

#ifdef BBDS
         else if (RP0->needevnt) {
            ui32 ecode;
            /* Assignment intended in next line */
            if (ecode = bbdsevnt(RP0->evhosts)) {
               if (ecode & BBDSigQuit || ecode & BBDSigImage &&
                     RP->ortvfl & (TV_EvtVVTV|TV_EvtBBD|TV_EvtUPGM) &&
                     d3imgt(UTVCFr_EVENT)) {
                  RP->CP.outnow = OUT_USER_QUIT;
                  goto RENDEZVOUS_2;
                  }
               if (ecode & BBDSigReset &&
                     RP0->rlvl[lvevnt] > RP0->rlvl[lvnow])
                  RP0->rlvl[lvnow] = RP0->rlvl[lvevnt];
               if (RP->kevtr) {
                  /* Initiate special event trial */
                  RP0->rlvl[lvtdef] = RP0->rlvl[lvnow];
                  RP0->rlvl[lvnow] = RL_ISKIP;
                  RP->CP.runflags |= RP_EVTRL;
                  if (RP->kevtr & EVTFRZ) RP->CP.runflags |= RP_EVTRF;
                  RP->kevtu = RP->kevtb & RP->kevtr;
                  RP->CP.outnow |= OUT_EVENT_TRIAL;
                  } /* End setting for special event trial */
               } /* End got any BBD event */
            } /* End checking for BBD events */
#endif

         if ((RP0->rlvl[lvnow] & ~RL_ISKIP) >= RL_NEWSERIES)
            RP->CP.outnow |= OUT_EVENT_NEWSER;

#endif            /*** CLOSE COMP NODE EXCLUSION BLOCK ***/

#ifdef PAR
/* If PAR, make another rendezvous, exchanging the outnow flag
*  so all nodes may terminate the trial loop if necessary */

RENDEZVOUS_2:
         {  static COPD ovfoutcop[2] =
               {{ LOR|UI32V, 1 }, { LOR|SHORT, 1 }};
            struct {
               ui32        myovflow;
               outnow_type myoutnow;
               } myovfout;
            myovfout.myovflow = RP->ovflow;
            myovfout.myoutnow = RP->CP.outnow;
            vecollect((char *)&myovfout, ovfoutcop, 2, OUTOVFPK_MSG,
               sizeof(myovfout));
#ifdef PAR0
            RP->ovflow =    myovfout.myovflow;
            RP->CP.outnow = myovfout.myoutnow;
#endif
            } /* End ovfoutcop local scope */
         blkbcst(&RP->CP.outnow, ttloc(IXoutnow_type), 1,
            LOOP_EXIT_MSG);
#endif /* PAR */
         if (RP->ovflow) {
            d3grex(NULL, OVERFLOW_ERR, RP->ovflow);
            RP->CP.outnow |= OUT_GRACEFUL_EXIT;
            }
         /* Mechanism to get ntr incrmented on all nodes... */
         if (RP->CP.outnow == OUT_EVENT_TRIAL) {
            RP->ntr += 1;
            RP->CP.outnow = 0;
            }
         else if (RP->CP.outnow) break;
         RP->CP.trialsr++;    /* Count cycles since reset */
         }  /* End trial loop */

/*---------------------------------------------------------------------*
*                           End trial loop                             *
*                                                                      *
*  Switch and take action according to code stored in RP->CP.outnow    *
*---------------------------------------------------------------------*/

#ifndef PAR
RENDEZVOUS_1:
RENDEZVOUS_2:
#endif
END_TRIAL_LOOP:
      if (RP->CP.outnow & OUT_QUICK_EXIT) {
         /* Severe errors where should not attempt d3term() */
         d3exit(NULL, QUICKEXIT_RQST, 0);
         }

      else if (RP->CP.outnow & (OUT_ERROR_EXIT|OUT_USER_QUIT)) {
#ifndef PARn
         if (RK.iexit) finex.ec = ROCKIO_ERR, finex.iv = (int)RK.iexit;
#endif
         /* Error exit or user requested quit.
         *  N.B.  With current design, d3exit() is probably never
         *  going to return, but this is sort of an experiment to
         *  see whether we can still get stats after a fatal error. */
         if (RP->CP.trial > 0 && RP->kpsta & KPSQS)
               goto END_ALL_SERIES_WITH_STATS;
         else  goto END_ALL_SERIES_NO_STATS;
         }

      else if (RP->CP.outnow & OUT_GRACEFUL_EXIT) {
         /* Errors where it is OK to complete current trial--which
         *  has now been done by code above.  Modify finex to make
         *  suitable exit message.  If parallel, call d3grout to
         *  print messages, then terminate.  */
         finex.ec = PREV_GREX_ERR;
#ifdef PAR0
         d3grout();
#endif
         goto END_ALL_SERIES_WITH_STATS;
         }

      else if (RP->CP.outnow & OUT_END_INPUT) {
         /* d3grp3 came to end of input, no serious errors,
         *  and stats were already done */
         goto END_ALL_SERIES_NO_STATS;
         }

      else if (RP->CP.outnow & (OUT_USER_PSTAT|OUT_USER_PAUSE)) {
         /* User pause with/without stats.
         *  Pop card unit to force on-line read, then terminate series
         *  loop so program ends up calling d3grp3 to read control
         *  cards.  When STATS|NOSTATS implemented on PX card, correct
         *  setting of RP_NOCYST bit in following line.  */
         if (RP->CP.outnow & OUT_USER_PAUSE)
            RP->CP.runflags |= RP_NOCYST;
#ifndef PARn
         if (RP->CP.runflags & RP_ONLINE) {
            cdunit(0);
            cryout(RK_P1+1, "0--->Enter group III control cards:",
               RK_LN2+RK_FLUSH, NULL);
            }
#endif
         }

/* End of series.  Regenerate unresponsive cells, print warnings,
*  headers, and statistics.  */

      /* If any cell type requested regeneration, do it now
      *  (under control of RP->kregen code).  This call also
      *  advances cseed, dseed, lseed for regen.  */
      if (RP->CP.runflags & RP_REGENR) {
         int mrgn = (RP->CP.runflags & RP_NOCHNG) ? 1 : 2;
         if ((RP->kregen & mrgn) == 0) d3regenr();
         } /* End if RP_REGENR */

END_ALL_SERIES_WITH_STATS:
#ifndef PARn
      if (RP0->exisn > 0)
         convrt("(YW'0-->WARNING:',I4,' stimulus nos. exceeded NDS.')",
            &RP0->exisn, NULL);

      rtime = (float)second();
      cryout(RK_P1, "    ", RK_SUBTTL+RK_LN1, NULL);
      convrt("(YW' END SERIES ',J1UJ6,'at ',J1UW12.132,'ms (sim), ',"
         "J1F10.4,'sec (CPU)')",
         &RP->CP.ser, &RP->CP.runtime, &rtime, NULL);
#endif

      /* Gather and print statistics */
      d3oflw();
      if (!(RP->CP.runflags & (RP_NOSTAT|RP_NOCYST))) {
#ifdef PAR
         d3gsta();
#endif
         d3stat();
         }

/* End of series, go on to another, unless breaking */

      if (RP->CP.outnow > 0) break;
      } /* End series loop */

/*---------------------------------------------------------------------*
*                           End series loop                            *
*---------------------------------------------------------------------*/

END_ALL_SERIES_NO_STATS:
   stattmr(OP_POP_TMR, 0);   /* Pop the default timer */
   stattmr(OP_PRINT_ALL, 0);

/* If there is VVTV, acknowledge last vvgrab */

#if defined(VVTV) && !defined(PARn)
   if (RP->ortvfl & TV_VVTV && (rc = vvgchk(OFF)) != 0)
      d3exit("VVGCHK", VIDEO_ERR, rc);
#endif

   if (RP->CP.outnow < OUT_END_INPUT) goto TOP_OF_GRAND_LOOP;

/*=====================================================================*
*        End of normal run.  Perform cleanup actions and stop.         *
*  (Save repertoires if requested, close other files and graphics.)    *
* ====================================================================*/

   d3term(&finex, NC.node);

/* Final normal termination */

#ifdef PAR
   appexit(NULL, 0, 0);
#else
   exit(finex.ec);
#endif

   } /* End of CNS main() */
