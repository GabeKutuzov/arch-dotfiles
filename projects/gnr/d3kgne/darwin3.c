/* (c) Copyright 1991-2011, Neurosciences Research Foundation, Inc. */
/* $Id: darwin3.c 46 2011-05-20 20:14:09Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               darwin3                                *
*             Cortical Network Simulator--Main Procedure               *
*                                                                      *
*----------------------------------------------------------------------*
*  N.B.  CNS uses the parallel-computer memory management routines     *
*  declared in memshare.h and documented in nsitools/membcst.c.        *
*  Always use these routines to allocate, free, and broadcast memory.  *
*----------------------------------------------------------------------*
*  V1A - G. N. Reeke - 08/21/85                                        *
*  V4A, 06/13/88, GNR et al. - Translated to 'C' from V3E              *
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
*  Rev, 06/12/11, GNR - Major rewrite of RESET controls                *
*  Rev, 10/12/11, GNR - Remove setmeta(), setmfd(), use setmf()        *
*                                                                      *
*  The list of modules needed for linking darwin3 formerly located     *
*     in this comment box has been deleted.  The definitive source     *
*     of this information is now the 'make' file--GNR 03/16/90.        *
*                                                                      *
***********************************************************************/

/*=====================================================================*
*                     Define version number here                       *
*=====================================================================*/
#define CNS_VERSION "V8H"

#define MAIN
#define NEWARB          /* Use new EFFARB mechanism */
#define CLTYPE  void
#define SDSTYPE struct SDSELECT
#define GSTYPE  struct GSDEF
#define TVTYPE  struct TVDEF
#define PPTYPE  struct PREPROC

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rkarith.h"
#include "normtab.h"
#ifdef UNIX
#include <signal.h>
#endif
#include "d3global.h"
#include "jntdef.h"
#include "armdef.h"
#include "wdwdef.h"
#include "d3fdef.h"
#include "allo_tbl.h"      /* To define static tables */
#ifdef D1
#include "d1def.h"
#endif
#include "simdata.h"
#include "statdef.h"
#include "bapkg.h"
#ifndef PARn
#include "rocks.h"
#include "rkxtra.h"
#include "savblk.h"        /* To define static tables */
#ifdef BBDS
#include "bbd.h"
#endif
#ifdef VVTV
#include "vvuser.h"
#endif
#endif /* !PARn */
#include "tvdef.h"
#include "plots.h"
#ifdef INMOS8
#include "xpnio.h"      /* Define global channels array */
#endif

#ifdef PARn
#include "rksubs.h"
/* System calls not prototyped elsewhere */
extern int sleep(unsigned int);
#endif

/* Prototypes unique to CNS main program */

#ifndef PARn      /* Not used on comp nodes */
void d3allo(void);
void d3bgis(void);
void d3cchk(void);
void d3clsm(void);
void d3cnmd(struct VDTDEF *);
void d3dflt(void);
void d3dflt2(void);
void d3echo(void);
void d3epsi(void);
void d3evt1(void);
void d3evt2(void);
void d3fchk(void);
void d3fix1(void);
void d3gfcl(int kg1);
void d3gfhd(struct SDSELECT *pgd);
void d3gfsh(struct SDSELECT *, ui32);
void d3gfsv(struct SDSELECT *, ui32);
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
void d3resetz(int kns);
void d3schk(void);
void d3snsa(void);
void d3snsprt(void);
void d3tchk(void);
void d3valu(struct VBDEF *);
void d3vitp(struct IMGTPAD *);
void d3vjnt(struct VCELL *);
void d3vwdw(struct VCELL *);
void d3vtch(struct VCELL *, stim_mask_t *);
void envdflt(void);
void prtfoff(void);
void rfdmtx(void);
void shutdown(int signum);
#endif

/* Routines used on all nodes */
void d3abort(void);
void d3gfsvct(struct SDSELECT *, struct CELLTYPE *, ui32);
void d3genr(void);
int  d3go(void);
void d3gsta(void);
int  d3imgt(int freq);
void d3inhi(void);
void d3msgl(void);
void d3msgr(void);
rd_type *d3nset(rd_type *);
int  d3plot(void);
void d3regenr(void);
void d3resetn(int rl);
int  d3rpgt(struct RFdef *);
void d3rstr(void);
void d3save(int svstage);
void d3stat(void);
void d3tsvp(int kblk);
void d3zsta(void);

/*=====================================================================*
*      Determine changes in reset action for a given event type        *
*                                                                      *
*  This routine implements the change in V8H that when multiple reset  *
*  types are called for at the same time, the highest action level and *
*  OR of the action kinds are performed.                               *
*=====================================================================*/

static void updreset(int rop) {
   RP->kra[RLact].kract |= RP->kra[rop].kract;
   if (RP->kra[rop].rlev > RP->kra[RLact].rlev)
      RP->kra[RLact].rlev = RP->kra[rop].rlev;
   } /* End updreset() */


/***********************************************************************
*                             CNS main()                               *
***********************************************************************/

int main(int argc, char *argv[]) {

#ifndef PARn
   /* Title for printed output -- SVNVRS is subversion revision
   *  number passed from gnrversn call in makefile */
#ifdef LINUX
   static char ttl[81] =
      "      CNS - Version " CNS_VERSION SVNVRS " - Compiled " __DATE__;
#else
   static char ttl[81] =
      "      CNS - Version " CNS_VERSION " - Compiled" __DATE__;
#endif
#endif

   rd_type *myprd;            /* Repertoire data pointer */
#ifdef D1
   struct VBDEF *ivb;
   char *mypd1;               /* D1 data pointer */
   byte *pd1wk;               /* Ptr to Darwin I work space */
#endif

#ifndef PARn
   ENVIRONMENT *E;            /* Ptr to environment */
   struct ARMDEF *pa;         /* Ptr to current arm */
   struct JNTDEF *pj;         /* Ptr to current joint */
   struct RFdef  *psvf;       /* Ptr to current savenet file */
   struct RFdef  *iru;        /* Pointer to replay file structure */
   struct TVDEF  *cam;        /* Ptr to current video camera */
   struct VCELL  *pvc;        /* Ptr to current VCELL block */
   struct WDWDEF *pw;         /* Ptr to current window */

   double druntime;           /* Run time (ms for output) */
   float rtime;               /* Real elapsed time */
   int ia;                    /* Arm index */
   int rc;                    /* Return code */
#endif

   long itrsi;                /* Relative reset interval  */
   long its,lts;              /* Series loop counter */
   long jfrz;                 /* Relative freeze interval */

#ifdef PAR0
   int  supip;       /* Descriptor for setup pipe, from cmd line */
#endif
#ifndef PARn
   byte lkd;         /* Print control for effectors, changes, senses */
#endif

/* End of main() local declarations */

/*=====================================================================*
*                                                                      *
*                       General Initialization                         *
*                                                                      *
*=====================================================================*/

#ifdef PAR
/* Prior to any other activity, must initialize the nio sub-system,
*  which provides NCUBE-like communications for all processes run-
*  ning on parallel machines.  Note that the same routine name--
*  nioinit()--is used for this purpose on all supported systems, but
*  the interface hidden inside is different in each architecture. */

#ifdef INMOS8
   /* Initialize nio for transputers */
   nioinit(argv);
#endif

#ifdef PAR0
   /* Initialize nio for hybrid host.  The last argument in the
   *  command line is the descriptor for a setup pipe that will
   *  be used to read the global structure NC.  After copying
   *  this argument, truncate argv so that "proper" cns code will
   *  never know about this argument.  */
   supip = atoi(argv[--argc]);
   argv[argc] = NULL;
   nioinit(supip);
#endif
#else  /* Serial */
   /* On serial machines, a negative coprocessor dimension will
   *  allow dual-mode library routines to tell what is going on.  */
   NC.dim = -1;
#endif /* PAR */

#ifndef PARn
/*=====================================================================*
*                                                                      *
*  Following initializations done only on serial systems and host      *
*     node of parallel systems:                                        *
*  Initialize signals, data structures and I/O systems.                *
*  Interpret command-line parameters.                                  *
*  Interpret Group 0 and Group 1 control cards.                        *
*                                                                      *
*=====================================================================*/

/* Ignore signals that may be issued to this process unintentionally.
*  These include all signals that are generated from the keyboard,
*  which are always sent to all the processes in the current process
*  group, and the notorious SIGWINCH, which is sent by OpenWindows
*  whenever the window status changes.  */

#ifdef UNIX
#ifdef PAR0
   signal(SIGINT, SIG_IGN);
#endif
   signal(SIGTSTP, SIG_IGN);
   signal(SIGQUIT, SIG_IGN);
   signal(SIGWINCH, SIG_IGN);

   /* Install handler to SIGTERM.  Presently, all this handler does
   *  is call exit(), to flush output buffers.  It is planned to make
   *  it aware of cryout() calls, and flush rocks output buffers as
   *  well.  */
   signal(SIGTERM, shutdown);
#endif

/* Start ROCKS I/O and the clock, check above allocations */

   RK.iexit = 0;              /* Clear ROCKS error flag */
   accwac();                  /* Accept whitespace as comments */
   sprmpt("CNS> ");           /* Set prompt for online operation */
   settit(ttl);               /* Set default output page title */
   cryout(RK_P1, "    ", RK_LN1, NULL);
   setpid(&ttl[6]);
#ifdef BBDS
   cryout(RK_P1, "    Using ", RK_LN2, crkvers(), RK_CCL, ", ", 2,
      plotvers(), RK_CCL, ", ", 2, envvers(), RK_CCL, ",", 1,
      "          ", RK_LN0+10, nsitvers(), RK_CCL, ", ", 2,
      bbdvers(), RK_CCL, NULL);
#else
   cryout(RK_P1, "    Using ", RK_LN2, crkvers(), RK_CCL, ", ", 2,
      plotvers(), RK_CCL, ", ", 2, envvers(), RK_CCL, ",", 1,
      "          ", RK_LN0+10, nsitvers(), RK_CCL, NULL);
#endif
#ifdef HAS_I64
   cryout(RK_P3, "    Compiled with hardware 64-bit arithmetic",
      RK_LN1, NULL);
#endif


/* Register routines to handle kwscan type J,N conversion codes */

   kwsreg((void (*)(void *))d3rlnm,   0);
   kwsreg((void (*)(void *))getdecay, 1);
   kwsreg((void (*)(void *))gettimnw, 2);
   kwsreg((void (*)(void *))getnapc,  3);
   kwsreg((void (*)(void *))getcsel,  5);
   kwsreg((void (*)(void *))getcsel3, 6);
   kwjreg((void (*)(char *, void *))getthr, 1);
   kwjreg((void (*)(char *, void *))g1arb,  2);

#ifdef PAR
   /* Parallel machine must have at least one comp node */
   if (NC.totalm1 <= 0) d3exit(NODENO_ERR, NULL, 0);
#endif /* PAR */

/* Allocate the global parameter blocks RP and RP0 */

   RP = (struct RPDEF *)allocpcv(Static, 1, IXstr_RPDEF,
      "RP global parameters");
   RP0 = (struct RPDEF0 *)allocpcv(Host, 1, sizeof(struct RPDEF0),
      "RP0 host parameters");

/* Set general CNS defaults and interpret BBDHOST card, PARMS card,
*  and command-line parameters that may override the PARMS card.  */

   d3dflt();
   d3parm(argc, argv);
   /* Wait for a debugger to be attached */
   while (RP->CP.dbgflags & DBG_CNS)
      rksleep(1,0);

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
   d3cchk();                  /* Link conductances and ions */
   d3mchk();                  /* Set modalities of all cell types */
   d3bgis();                  /* Set derived params for some RFs */
   d3gfcl(TRUE);              /* Check SIMDATA items and lists */

/* Exit after all possible input errors have been found. */

   if (RK.iexit) d3exit(ROCKIO_ERR, NULL, (long)RK.iexit);

   d3lafs();                  /* Calc length of afferent sums--
                              *  must do this before d3allo().  */
   d3allo();                  /* Allocate storage */
   if (RP->CP.runflags & RP_OKPRIN) d3echo();

/* N.B. Shared memory allocated after the d3echo call will be counted
*  in memsize(MPS_Static), but of course not included in the printed
*  total.  But it's OK, because all such memory (see list in box below)
*  is included in other totals that are also printed by d3echo. */

/*=====================================================================*
*  Final preparations for running the model--                          *
*     Allocate space for input array, TV images, Darwin I inputs,      *
*        kinesthetic and touch cells, and effector positions.          *
*     Allocate space for values and match MODVAL blocks with sources.  *
*     Establish communications with real-world BBD.                    *
*     Setup video disk recorders.                                      *
*     Set up stimulated environment.                                   *
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

#ifdef BBDS
/* If BBD exists, check that the client(s) can provide any requested
*  user-defined senses and output effectors.  Then, if requested by
*  the BBDHOST startup card, send version info to specified client(s).
*  Note: Currently, bbdschk() abexits in case of error, normally
*  returns 0, not checked here.  Really should give an error return,
*  use here to call d3exit in due course.  */

   bbdschk();
   if (RP0->vmsgwho)
      bbdsputm(ssprintf(NULL, "CNS " CNS_VERSION ", Rev " SVNVRS
         ", %s, %s\n", bbdvers(), plotvers()), (int)RP0->vmsgwho);
#endif

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

   if (RK.iexit) d3exit(ROCKIO_ERR, NULL, (long)RK.iexit);

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

#ifdef D1
/* Re-open Darwin 1 input files if needed.  It is already known
*  from success of d3fchk that these can be opened successfully,
*  so error checking can be minimal.  */

   if (RP0->cumnd1) {
      struct RFdef *pdf;
      for (pdf=d3file[DIGINPUT]; pdf; pdf=pdf->nf) {
         rfopen(pdf, NULL, SAME, SAME, SAME, SAME, SAME,
            SAME, SAME, SAME, SAME, SAME, ABORT);
         rfseek(pdf, pdf->userdata, SEEKABS, ABORT);
         } /* End loop over D1 input files */
      RP->kevtb |= EVTD1;
      } /* End D1 input opening */
#endif

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
#endif  /* PAR */

/* Set method for handling fixed-point arithmetic overflows */

   e64set(E64_FLAGS, &RP->ovflow);

#if !defined(PAR0) && defined(D1)

/* Allocate workspace for d1go.  N.B.  Don't put pd1wk in RP--
*  is undefined on host and broadcast will bash it elsewhere. */

   if (RP->md1byt)
      pd1wk = mallocv(RP->md1byt, "D1 work area");
#endif

/* Set node-dependent memory pointers.  This first call to d3nset
*  will use the initial ssck information stored by d3lafs above.  */

#ifdef D1
   if (RP->nd1s) mypd1 = d1nset(NULL);
#endif
   myprd = d3nset(NULL);

/*=====================================================================*
*                 Generate or restore all repertoires                  *
*                                                                      *
*  Do final video, graphics, and file initialization that comes after  *
*  repertoire generation for timing or dependency reasons.             *
*=====================================================================*/

   if (RP->ksv & KSVRR) d3rstr();
#ifdef D1
   if (RP->nd1s) d1genr();
#endif
   d3genr();

#ifndef PARn

#ifdef VVTV
/* Initialize for VVTV input--cameras having physical interface cards.
*  This code must come after tree broadcast and d3genr() but before
*  the GRAND LOOP.
*
*  Changed 11/27/96 to omit any driver calls on comp nodes.
*  Changed 02/15/10 to loop over cameras, use specified parameters.
*     However, note that vvgrab,vvgchk are defined over all cameras.
*/

   for (cam=RP->pftv; cam; cam=cam->pnxtv) {
      if ((cam->utv.tvsflgs & (TV_ON|TV_VVTV)) == (TV_ON|TV_VVTV)) {
         int isa  = (int)cam->utv.tvsa;
         int tvx0 = (int)cam->utv.tvxo;
         int tvy0 = (int)cam->utv.tvyo;
         int tvxe = tvx0 + isa*(int)cam->utv.tvx;
         int tvye = tvy0 + isa*(int)cam->utv.tvy;
         if ((rc = vvload((int)cam->utv.islot)) != 0)
            d3exit(VIDEO_ERR, "VVLOAD", rc);
         if ((rc = vvinit((int)cam->utv.islot,
               tvx0, tvxe-1, tvy0, tvye-1,
               (VV_233 | VV_TV1 | VV_ATD), isa, 0)) != 0)
            d3exit(VIDEO_ERR, "VVINIT" , rc);
         }
      } /* End camera loop */
#endif

/* Initialize graphics unless doing NOPLOT run */

   if (RP->CP.runflags & RP_OKPLOT) {
      long bufl = 4092;          /* Minimum buffer length */
      char *mfnm = NULL;         /* Metafile name */
      char *stnn = NULL;         /* X graphics station name */

      /* Initiate graphics metafile if METAFILE card entered */
      if (!(RP->CP.runflags & RP_NOMETA) && d3file[METAFILE]) {
         mfnm = d3file[METAFILE]->fname;
         bufl = max(bufl, d3file[METAFILE]->blksize);
         }

      /* Initiate X graphics unless NOXG entered on command line */
      if (!(RP->CP.runflags & RP_NOXG))
         stnn = RP0->hxterm ? getrktxt(RP0->hxterm) : "";

      setmf(mfnm, stnn, "CNS " CNS_VERSION, NULL, bufl,
         RP->CP.dbgflags, '8', 0, 0);
      }

/* Perform tree-structure-verify-print if requested */

   if (RP0->n0flags & N0F_TSVP) d3tsvp(7);

#endif /* !PARn */

/* Allocate working storage for cross-response stats--all nodes */

   d3msgl();

/* Clear all values */

   if (RP->nvblk > 0) d3zval(0);

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

   RP->kra[RLnow].rlent = 0;  /* Indicate RESET NOW not entered */
   RP->CP.outnow = d3grp3();
   RP->CP.trial = 0;          /* Kill stats output if no trials done */

/* Out of some superstition, this call is here, rather than in the
*  big initialization block below, because it might affect variables
*  in rpdef needed for d3save() or other node code between the RP
*  broadcast here and the big broadcast after the next section.  */

   d3bgis();                  /* Derived params for certain RFs */
   d3news();                  /* General initialization for d3go() */
   if (RK.iexit) RP->CP.outnow = OUT_USER_QUIT;

#endif
#ifdef PAR
/* Broadcast RP data with any changes */

   blkbcst(RP, ttloc(IXstr_RPDEF), 1, INTERRUPT_MSG);

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
*     Prepare falloff and normalization tables for gconns.             *
*     Set up for freeze options.                                       *
*     Print cycle parameters for this run.                             *
*---------------------------------------------------------------------*/

#ifndef PARn
   /* So old inputs will work, if user did not enter a TRIAL level,
   *  and ntc != 1, use omega1-style decay only.  */
   RP->kra[RLtuse] = RP->kra[RLtent];
   if (!RP-kra[RLtent].rlent) RP->kra[RLtuse].rlev =
      (RP->ntc == 1) ? KRLNONE : KRLTRIAL;
   /* Set for NOW reset if requested, then clear request */
   if (RP->kra[RLnow].rlent) {
      RP->kra[RLnow].rlent = 0;
      RP->kra[RLact] = RP->kra[RLnow];
      }

   if (RP0->n0flags & N0F_MKTBLS)
      d3ctbl(ON);             /* Store conductance activation tables */
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
*  kpr = preset for repertoire and environment print.
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

      /* Set for superposition plots */
      if (RP->kpl & KPLSUP) d3epsi();

#endif /* !PARn */
      } /* End if plotting enabled */

/* Establish base bits in kevtb.  */

   RP->kevtb &= ~(EVTWSD|EVTPRB);
   if (RP->pfprb) RP->kevtb |= EVTPRB;

#ifndef PARn
/* Prepare for SIMDATA file by making general header and optional
*  segment 0 header (once only).  If d3gfhd() cannot open the output
*  file, it sets iexit but also sets KSDNOK to disable other activity
*  for SIMDATA until the exit occurs.  Because d3gfsh() edits the cell
*  and connection-level svitms codes, it must be called before the
*  tree broadcast, but d3gfsv[ct] must be called afterwards.  */

   if (!(RP->ksv & (KSDNE|KSDNOK|KSDOFF))) {
      d3gfcl(FALSE);             /* Check cell lists for SIMDATA */
      RP->ksv &= ~KSDSG;         /* Kills extra write on new series */
      if (!(RP->ksv & KSDHW)) {
         d3gfhd(RP0->pgds);
         /* Record header for SIMDATA segment 0 (Lij, Dij, and ABCD)
         *  if needed.  But test whether d3gfhd set the KSDNOK bit */
         if (!(RP->ksv & KSDNOK))
            d3gfsh(RP0->pgds, (GFLIJ|GFDIJ|GFABCD));
         } /* End !KSDHW */
      } /* End if (RP0->pgds) */

#ifdef PAR0
/* By now, any function that needs cell lists on comp nodes should
*  have set the appropriate request bits--so now assign cell lists
*  to Host or Dynamic memory pools.  */

   d3clsm();
#endif   /* PAR0 */

/* Set offsets into afferent sums and allocate phase distributions */

   d3lafs();
   d3oafs();

#endif /* !PARn */

/* Broadcast tree data for new run (new CYCLE card) */

#ifdef PAR
   membcst(MPS_Static|MPS_Dynamic);
#endif

/* Recalc node-dependent data */

   d3nset(myprd);
#ifdef D1
   d1nset(mypd1);
#endif

/* Allocate GRPNO-dependent storage for cross-response stats */

   d3msgr();

/* Prepare and print falloff and normalization tables for gconns */

#ifndef PAR0
   d3inhi();
#endif
#ifndef PARn
   if (RP->kdprt & PRFOFF) prtfoff();
#endif

/* If doing an online replay, allow it to cycle indefinitely */

   if ((RP->CP.runflags&(RP_REPLAY|RP_ONLINE)) == (RP_REPLAY|RP_ONLINE))
      RP->ntr1 = RP->ntr2 = 999999UL;

/* Set up for freeze options:
*  ifrz < 0: Extra series, freeze on last series
*  ifrz = 0: Amplify on all series
*  ifrz > 0: Freeze every ifrz'th series, no extra series.  */

   if (RP->ifrz < 0) {
#ifndef PARn
      jfrz = RP->nts;
#endif
      lts = 0;
      }
   else {
      lts = 1;
#ifndef PARn
      if (!RP->ifrz) jfrz = RP->nts + 1;
      else jfrz = RP->ifrz;
#endif
      }

/* At this point, KSDSG bit can only be set if write of segment 0 is
*  pending.  This needs to be done on all nodes, then d3gfsh can be
*  done for the regular data segment.  An isynch() is not needed
*  because d3gfsvct() effectively does one.  */

   if (RP->ksv & KSDSG) {
      struct CELLTYPE *il;
#ifdef PARn
      for (il=RP->pfct; il; il=il->pnct) {
         d3gfsvct(NULL, il,  (GFLIJ|GFDIJ|GFABCD));
#else
      d3gfsv(RP0->pgds, (GFLIJ|GFDIJ|GFABCD));
      for (il=RP->pfct; il; il=il->pnct)
         d3gfsvct(RP0->pgds, il, (GFLIJ|GFDIJ|GFABCD));
#endif
      }

#ifndef PARn
   if (!(RP->ksv & (KSDNE|KSDNOK|KSDOFF))) {
      ++RP0->pgds->gdseg;
      d3gfsh(RP0->pgds, ~(GFLIJ|GFDIJ|GFABCD));
      if (RP->ksv & KSDSG) RP->kevtb |= EVTWSD;
      } /* End if (RP0->pgds) */

/* Last chance to exit before beginning trials */

   if (RK.iexit) d3exit(ROCKIO_ERR, NULL, (long)RK.iexit);

/* Print cycle parameters for this run: */

   if (RP->CP.runflags & RP_OKPRIN) {
      ldiv_t tsrq;
      long szdmem = memsize(Dynamic);
      cryout(RK_P2, "0Cycle parameters for this run:",
         RK_NFSUBTTL+RK_LN2, NULL);
      convrt("(4X'Number of series',IL14,4X'Freeze interval',I15/"
         "4X'Trials/normal series',IL10,4X'Trials/freeze series',IL10/"
         "4x'Cycles/normal trial',IH11,4X'Cycles/freeze trial',IH11)",
         &RP->nts, &jfrz, &RP->ntr1, &RP->ntr2, &RP->ntc1, &RP->ntc2,
         NULL);
      convrt("(4X'Mean stim noise',B24IJ15.4,"
         "4X'Std dev of noise',B28IJ14.4"
         "/4X'Stim noise fraction',B24IJ11.4,4X'Stim noise seed',IL15)",
         &RP->snmn, &RP->snsg, &RP->snfr, &RP->snseed, NULL);
      /* Step time is stored internally in microseconds so we can
      *  express decimal fractions of msec exactly.  Revised
      *  ubcdwt requires doing this explicitly.  */
      tsrq = ldiv(RP->timestep,1000);
      convrt("(W4X'Step time (msec)',UL10,H.@UL3,4X,'Dynamic "
         "storage',UL15)", &tsrq.quot, &tsrq.rem, &szdmem, NULL);
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
      d3exit(VIDEO_ERR, "VVGRAB", rc);
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
         cryout(RK_P3, "0-->This is a freeze series", RK_LN2, NULL);
         }

/* If there was a SAVE NEWSERIES card, do it now */

      if ((RP->ksv & (KSVNS|KSVOK)) == (KSVNS|KSVOK))
         d3save(D3SV_NEWSER);

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

/* Establish new series reset mode, except there is no reset on the
*  first trial of a run.  The special call to d3resetz with kns == 1
*  is to reset cameras (if requested) before the trial loop actually
*  reads an image.
*  N.B.  A new series is also a new trial.
*/

      if (RP->CP.effcyc > 0) {
         updreset(RLser);
         updreset(Rltuse);
         d3resetz(RL_SERIES, 1);
         }

/* If any series-time images are called for, get and preprocess now */

      if (RP->ortvfl & (TV_SerVVTV|TV_SerBBD|TV_SerUPGM)) {
         rc = d3imgt((int)UTVCFr_SERIES);
         if (rc) d3exit(VIDEO_ERR, "D3IMGT", rc);
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

#ifndef PARn
#ifdef VM
         int ipxq;            /* pxq() return */
#endif
         ++RP0->trials0;      /* Increment global trial number */
         ++RP0->trialse;      /* Increment trials since "event" */

         if (RP->CP.runflags & RP_REPLAY) {
            /* Locate to start of data for this trial */
            rfseek(iru, RP->CP.rpirec*RP0->rprecl, SEEKABS, ABORT);
            /* Read series and trial numbers (adjacent in RP) */
            if (!rfread(iru, &RP->CP.ser, sizeof(long), ABORT)) {
               RP->CP.outnow = OUT_REPLAY_RANGE;
               goto RENDEZVOUS_1;
               }
            /* Remove the new format marker from the series number */
            RP->CP.ser &= ~MINUS0;
            }

/* Set print subtitle with current trial number.  This must be done
*  after reading replay header, but before calling envget or envcyc,
*  which may print error messages.  This same title string will be
*  used to label superposition plots, envplots, and SAVENET files.
*  d3rpsi and d3dprt output must add cycle number if needed, as room
*  for this was not reserved in stitle for plots and SAVENET headers.
*/

         sconvrt(RP0->stitle, "(8H0Series J0IL8,8H  Trial J0IL8)",
            &RP->CP.ser, &RP->CP.trial, NULL);
         if (RP0->trials0 != RP->CP.trial)
            sconvrt(RP0->stitle + strnlen(RP0->stitle, LSTITL),
               "(2H (J0IL8H))", &RP0->trials0, NULL);
         /* When codes for ui64 O/P are added to sconvrt, this
         *  conversion to double will no longer be needed.  */
         druntime = 1.0E-3*uwdbl(RP->CP.runtime);
         sconvrt(RP0->stitle + strnlen(RP0->stitle, LSTITL),
            "(2H (J1Q12.4,3Hms))", &druntime, NULL);
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
               int ii = envget(E, RP->pstim);
               if (ii == D3_PREM_EOF) {
                  RP->CP.outnow = OUT_REPLAY_RANGE;
                  goto RENDEZVOUS_1;
                  }
               if (ii == D3_FATAL) d3exit(REPLAY_ERR, NULL, 0);
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
            RP->CP.outnow = OUT_USER_QUIT;
            goto RENDEZVOUS_1;
            }
#endif

/* Collect trial-time UPGM images unless disabled for event trial.
*  (Unnecessary to check ortvfl--off bits are off in kevtu)  */

         if (RP->kevtu & (EVTBTV|EVTVTV|EVTUTV) &&
               d3imgt((int)UTVCFr_TRIAL)) {
            RP->CP.outnow = OUT_USER_QUIT;
            goto RENDEZVOUS_1;
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
         if (RP->CP.rlnow > RL_CYCLE) d3resetz(0);

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
            RP->CP.outnow = OUT_USER_QUIT;
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

#ifdef D1
/* Read Darwin 1 data if defined and requested */

         if (RP->kevtu & EVTD1) {
            struct RFdef *pdf;
            byte *pd = RP->pbcst + RP0->od1in;
            for (pdf=d3file[DIGINPUT]; pdf; pdf=pdf->nf) {
               rfread(pdf, pd, pdf->lrecl, ABORT);
               pd += pdf->lrecl; }
            } /* End D1 reading */
#endif /* D1 */

#endif      /* Not PARn */

/* Broadcast CP, IA, TV, D1, value, sense, modality, effector data */

#ifdef PAR
RENDEZVOUS_1:     /* (Here to notify nodes of errors) */
         blkbcst(&RP->CP, ttloc(IXstr_CPDEF), 1, CP_CYCLE_MSG);
         if (RP->CP.outnow > 0) goto END_TRIAL_LOOP;
         membcst(MPS_Shared);
#endif /* PAR */

/* Complete network data portion of reset now that runflags have been
*  broadcast to all nodes.  Implement ALLSERIES value reset also.
*  Even if no reset is scheduled, call d3resetn() because it may be
*  necessary to update Izhikevich 'u' or Brette-Gerstner 'w' due to a
*  change in parameters by d3chng().  */

         d3resetn(RP->CP.rlnow);
         if (RP->CP.runflags & RP_RVALL && RP->CP.trial == 1)
            d3zval(VBVOPBR);

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
            int iw;
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
            int kout = 0;
            if (!(RP->kpl1 & KPLSUP))  kout |= SKP_XG;
            if (!(RP->kpl1 & KPLMFSP)) kout |= SKP_META;
            if ((RP->CP.outnow = d3ngph(RP->spwid, RP->sphgt, 0, 0,
                  "DEFAULT", RP->colors[CI_BOX], "CLIP", kout)) > 0)
               goto RENDEZVOUS_2;
            /* Plot TV images, if any */
            if (RP->ortvfl & TV_ON && !(RP->CP.runflags & RP_REPLAY))
               tvplot();
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
            if (ii == D3_PREM_EOF) {
               RP->CP.outnow = OUT_REPLAY_RANGE;
               goto RENDEZVOUS_2;
               }
            if (ii == D3_FATAL) d3exit(REPLAY_ERR, NULL, 0);
#ifndef PARn
            /* Read changes array */
            if (RP0->ljnwd > 0) {
               if (!rfread(iru, RP0->pjnwd,
                     sizeof(float)*RP0->ljnwd, ABORT)) {
                  RP->CP.outnow = OUT_REPLAY_RANGE;
                  goto RENDEZVOUS_2;
                  }
               }

            /* Read the value array */
            if (RP->nvblk > 0) {
               if (!rfread(iru, RP->pvdat, RP0->cumval, ABORT)) {
                  RP->CP.outnow = OUT_REPLAY_RANGE;
                  goto RENDEZVOUS_2;
                  }
               }
#endif
            } /* End reading replay info */

/*---------------------------------------------------------------------*
*       Otherwise, perform ntc cycles (one trial) of simulation        *
*---------------------------------------------------------------------*/


         else {
#ifdef D1
            /* Evaluate Darwin 1 repertoires */
            if (RP->pd1b1) d1go(pd1wk);
#endif
            stattmr(OP_PUSH_TMR, CALC_TMR);

            RP->CP.outnow = d3go();

#ifndef PARn
#ifdef BBDS
            /* If there are any outputs to BBDs, d3go will have
            *  put the requisite values in the changes array,
            *  so now just transmit them */
            if (RP->kevtu & EVTEFF) bbdspute(FALSE);
#endif
#endif /* Not PARn */

            stattmr(OP_POP_TMR, 0);
            if (RP->CP.outnow > 0)
               goto RENDEZVOUS_2;
            } /* End if replay else simulation */

/*---------------------------------------------------------------------*
*Print/plot output that occurs for either replay or current simulation.*
*---------------------------------------------------------------------*/

         if ((RP->CP.outnow = d3plot()) > 0)
            goto RENDEZVOUS_2;

#ifndef PARn         /*** OPEN COMP NODE EXCLUSION BLOCK ***/

/* Print changes array if requested */

#if LSIZE == 8
         if (lkd & PRCHG && RP0->ljnwd < INT_MAX)
#else
         if (lkd & PRCHG)
#endif
            {  /* Open brace here not above so pair-matching works */
            int lnch = (int)RP0->ljnwd;   /* Need int for convrt */
            if (RP->kdprt & PRSPT) spout((lnch + 9)/10);
            convrt("('0Changes array: '#|(10F8.2;/))",
               &lnch, RP0->pjnwd, NULL);
            }

/* See if value conditions are satisfied for canonical trace
*  or grip modes.  Required also in replays. */

         d3cnmd(RP->pvdat);

/*---------------------------------------------------------------------*
*             End-of-trial updating and breakout checking              *
*---------------------------------------------------------------------*/

/* Advance run time */

         RP->CP.runtime = jaul(RP->CP.runtime, RP->timestep);

/* Restore trial-level reset in case event not found */

         RP->CP.rlnow = RP0->rlvl[lvnow] = RP0->rlvl[lvtuse];

/* If a special event trial just completed, resume normal cycling */

         if (RP->CP.runflags & RP_EVTRL) {
            RP->kevtu = RP->kevtb;
            RP->CP.runflags &= ~(RP_EVTRF|RP_EVTRL);
            RP0->rlvl[lvnow] = RP0->rlvl[lvtdef];
            RP0->trialse = 0;
            }

/* Check for user intervention (currently VM opsys only) */

#ifdef VM
         else if (ipxq = d3pxq(FALSE)) /* Assignment intended */
            RP->CP.outnow = ipxq;
#endif

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
                  RP->CP.outnow = OUT_EVENT_TRIAL;
                  } /* End setting for special event trial */
               } /* End got any BBD event */
            } /* End checking for BBD events */
#endif

         if ((RP0->rlvl[lvnow] & ~RL_ISKIP) >= RL_NEWSERIES)
            RP->CP.outnow = OUT_EVENT_NEWSER;

#endif               /*** CLOSE COMP NODE EXCLUSION BLOCK ***/

#ifdef PAR
/* If PAR, make another rendezvous, exchanging the outnow flag
*  so all nodes may terminate the trial loop if necessary */

RENDEZVOUS_2:
         blkbcst(&RP->CP.outnow, ttloc(IXoutnow_type), 1,
            LOOP_EXIT_MSG);
#endif /* PAR */
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
*---------------------------------------------------------------------*/

#ifndef PAR
RENDEZVOUS_1:
RENDEZVOUS_2:
#endif
END_TRIAL_LOOP:
      switch (RP->CP.outnow) {

case OUT_EVENT_NEWSER:
      break;

case OUT_USER_QUIT:
TREAT_AS_OUT_USER_QUIT:
      if (RP->CP.trial > 0 && RP->kpsta & KPSQS)
            goto END_ALL_SERIES_WITH_STATS;
      else  goto END_ALL_SERIES_NO_STATS;

case OUT_QUICK_EXIT:
      d3exit(QUICKEXIT_RQST, NULL, 0);

/* Here when replay file goes out of range
*  If online, let user go elsewhere, otherwise, kill the job.  */

case OUT_REPLAY_RANGE:
#ifndef PARn
      cryout(RK_P1, "0***REPLAY REQUEST IS OUT OF STORED RANGE.",
         RK_LN2, NULL);
#endif
      if (!(RP->CP.runflags & RP_ONLINE))
         d3exit(REPLAY_ERR, NULL, 0);
      goto REQUEST_GROUP_III_INPUT;

/* GUI Interrupt button (or D3PXQ) exits to here.
*  All code that comes here must have RP->CP.outnow already set,
*     so value can be preserved.  This is unlike the IBM version.
*  Pop card unit to force on-line read, then terminate series loop
*     so program ends up calling d3grp3 to read control cards.
*  When STATS|NOSTATS implemented on PX card, correct setting of
*     RP_NOCYST bit in following line.  */

case OUT_USER_PAUSE:
      RP->CP.runflags |= RP_NOCYST;
      /* Drop through to OUT_USER_PSTAT case ... */
case OUT_USER_PSTAT:
      if (!(RP->CP.runflags & RP_ONLINE))
         goto TREAT_AS_OUT_USER_QUIT;
REQUEST_GROUP_III_INPUT: ;
#ifndef PARn
      cdunit(0);
      cryout(RK_P1+1, "0--->Enter group III control cards:",
         RK_LN2+RK_FLUSH, NULL);
#endif

         } /* End outnow switch */

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
      druntime = 1.0E-3*uwdbl(RP->CP.runtime);
      convrt("(YW' END SERIES ',J1IL6,'at ',J1Q12.4,'ms (sim), ',"
         "J1F10.4,'sec (CPU)')",
         &RP->CP.ser, &druntime, &rtime, NULL);
#endif

      /* Gather statistics */
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
      d3exit(VIDEO_ERR, "VVGCHK", rc);
#endif

   if (RP->CP.outnow < OUT_USER_QUIT) goto TOP_OF_GRAND_LOOP;

/*=====================================================================*
*        End of run.  Save repertoires if requested and stop.          *
*=====================================================================*/

   /* If there was a SAVE NETWORKS card, do it most urgently */
   if ((RP->ksv & (KSVSR|KSVOK)) == (KSVSR|KSVOK))
      d3save(D3SV_ENDXEQ);

   /* Finish all plotting */
   if (RP->CP.runflags & RP_OKPLOT) endplt();

#ifndef PARn
   /* Close all UPGM camera routines--ignore errors at this late date */
   if (RP0->ptvrsh) freev(RP0->ptvrsh, "Rescale histo");
   for (cam=RP->pftv; cam; cam=cam->pnxtv) {
      struct PREPROC *pip;
      if (cam->utv.tvsflgs & TV_UPGM)
         cam->tvinitfn(&cam->utv, getrktxt(cam->hcamnm),
            NULL, NULL, TV_CLOSE);
      for (pip=cam->pmyip; pip; pip=pip->pnipc)
         if (pip->ipsflgs & PP_UPGM)
            pip->pipfn(&pip->upr, NULL, NULL);
      }
   d3upcl();                  /* Close any user libraries */
   if (RP->CP.runflags & RP_REPLAY)
      rfclose(d3file[REPLAY], REWIND, RELEASE_BUFF, ABORT);
   if (!(RP->ksv & (KSDNE|KSDNOK))) {
      struct RFdef *pgdf = d3file[RP0->pgds->gdfile];
      rfwi4 (pgdf, (si32)EndMark);
      rfclose (pgdf, NO_REWIND, RELEASE_BUFF, ABORT);
      }
   cryout(RK_P1, "\0", RK_LN0+RK_FLUSH+1, NULL);
   fflush(stdout);
   fflush(stderr);
#endif

/* Final normal termination */

#ifdef PARn
   /* Comp nodes wait to receive termination message via d3exit,
   *  otherwise, connection with host could be terminated too
   *  soon, causing driver to fail before it gets the normal
   *  exit synchronization from the other nodes.  */
#ifdef INMOS8
   for (;;) ProcWait(100000);
#else
   for (;;) sleep(100);
#endif
#else /* !PARn */
/* BUG:  May want to put in a sleep() or other mechanism to let
*  processes such as 'tee' catch up before terminating the group */
   d3exit(D3_NORMAL, NULL, 0);
#endif

   } /* End of CNS main() */
