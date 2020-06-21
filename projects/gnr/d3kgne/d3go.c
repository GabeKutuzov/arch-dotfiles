/* Copyright (c) 1985-2012, Neurosciences Research Foundation, Inc. */
/* $Id: d3go.c 52 2012-06-01 19:54:05Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                                d3go                                  *
*                                                                      *
************************************************************************
*                                                                      *
*     For one stimulus presentation, carries out 'ntc' cycles of       *
*  network simulation.  For all cells in all repertoires, calculates   *
*  group responses and any consequent synaptic changes.  Calculates    *
*  motor neuron outputs to be sent to effectors.  Accumulates stats.   *
*                                                                      *
*  Call: d3go(void)                                                    *
*  Returns: return code from newplt if series to be terminated,        *
*     otherwise zero.                                                  *
*                                                                      *
*  This program is arranged in four major sections:                    *
*     1) Initialization                                                *
*     2) Main loop                                                     *
*     3) Routines for particular DECAY and AMP rules                   *
*     4) Cell, layer, and trial termination.                           *
*                                                                      *
*  A note concerning overflow checking:  Prior to scaling, connection  *
*     sums are kept as 64-bit values, which cannot overflow as long    *
*     as number of connections per cell is restricted to UI32_MAX.     *
*     After scaling, an error occurs if the results do not fit into    *
*     an ordinary 32-bit long at (S20), which allows sums up to ~2V.   *
*     This all needs to be reexamined in the context of deciding       *
*     which of these calculations might be done in floating point,     *
*     and which code might be in Assembler, where overflow checking    *
*     is much easier.  -GNR                                            *
*                                                                      *
*  A note concerning dprnt and statistics:  In V8A, individual speci-  *
*     fication of H,Q,S modes for all afferences was added.  To avoid  *
*     excessive burgeoning of statistics, all afference distributions  *
*     and individual dprnts are made simply with the raw input sums    *
*     (so user can see what came in before any caps were applied),     *
*     but the dprnt class sums are those actually used, i.e. after     *
*     caps, decay, and multiplication by voltage if volt-dep. -GNR     *
*                                                                      *
*  Version 1A, 12/18/85, G.N. Reeke                                    *
*  V4A, 11/20/88, SCD - Translated to C from version V3F               *
*  Rev, 11/21/89, GNR - Stim noise code changed to incorporate snfr    *
*  Rev, 12/06/89, JWT - Broadcast of window offset values added        *
*  Rev, 12/19/89, JWT - ROCKS output calls removed for MIMD version,   *
*                       overflow promoted from warning to fatal error  *
*  Rev, 01/04/90, GNR - Fixed il->group(x,y) calculation               *
*  Rev, 02/23/90, GNR - Handle PAR and serial d3inhb calls             *
*  Rev, 03/06/90, GNR - Fixed determination of frozen trial            *
*  Rev, 03/22/90, GNR - Removed mean prev response decay (kind 'M')    *
*  Rev, 04/25/90, GNR - Correct cskip,cdist for OPT=STORAGE            *
*  Rev, 05/03/90, JWT - Parallelized hand vision, added arm/wdw bcst   *
*  Rev, 09/15/90,  MC - Added TV code                                  *
*  Rev, 11/01/90, GNR - Handle blue button (GUI) exits                 *
*  Rev, 02/05/91,  MC - Add phase calculations                         *
*  V5A, 04/28/91, GNR - Add generic effector code to trialfin          *
*  Rev, 07/12/91, GNR - Calc changes when s(i) on host after d3exch    *
*  V5C, 11/07/91, GNR - Correct fv usage, detail print end of list,    *
*                       improve overflow detection                     *
*  Rev, 01/15/92, GNR - Allow BW inputs from colored objects           *
*  V5E, 07/12/92, GNR - Revise to reenable GCONN's in serial version   *
*  Rev, 07/23/92, GNR - Revise for external Lij files                  *
*  Rev, 07/25/92, GNR - Add KINDB decay, baseline Cij, fix s_mticks    *
*                       bug, omit disallowed sign changes from NMOD    *
*  Rev, 08/08/92, GNR - Use SRA macro where needed                     *
*  Rev, 08/19/92, GNR - Random rounding for Cij0, further mticks fix,  *
*                       remove unneeded Lij checks, allow colored VT   *
*  Rev, 10/29/92, ABP - Fix cpp directive enclosing offcijhigh, and    *
*                       fix jm64sb prototype.                          *
*  V6A, 03/05/93, GNR - Add subarbors, correct lseed update            *
*  Rev, 03/24/93, GNR - Call d3woff for window offset calculation      *
*  Rev, 03/26/93, GNR - Add KAMDQ to use sbar for mti, add mdist       *
*  Rev, 05/20/93, GNR - Add persistence limit, RP_STEP to runflags     *
*  V6D, 02/02/94, GNR - Add DELAY, d3lplt, GCONN self-avoidance,       *
*                       spike, ctscll                                  *
*  V7B, 06/22/94, GNR - Correct subarbor counting when conns are skip- *
*                       ped, average Cij after amp, add explicit amp   *
*                       scales, amp modulated by LTP, delete ctscll,   *
*                       add ptscll, stscll, better rounding, correct   *
*                       bug in serial OPTZZ G,J,N,O,U Lij gen, correct *
*                       bug spiking when cellaffs < 0, delete rt rfrc  *
*  Rev, 09/16/94, GNR - Correct erroneous rseed fix of 12/10/93        *
*  V7C, 11/01/94, GNR - Add decay to MODULATE and GCONN connections    *
*  Rev. 01/06/95, GNR - Permit voltage-dep. conns to cells w/o phase   *
*  V8A, 05/06/95, GNR - H,X,Y,Z,C,F,U stats to CELLTYPE, modalities,   *
*                       A,G,Msums, allow self-input and decay w/or w/o *
*                       phasing, use depression(t), revise refractory  *
*                       period, sep. pos,neg ax for caps, sep. Q,S per *
*                       MOD or GCONN type, remove "LTP" and fhits[pn], *
*                       nhit[pn] stats, Aij scaled according to nbc,   *
*                       OVERFLOW_ERR, COMPAT=R for depr-thresh         *
*  Rev, 01/12/97, GNR - Get rid of once-only repertoire/celltype loop, *
*                       kcy,et2,NOPTR,kill_amp, add modality ntrs,ntcs *
*  Rev, 02/17/97, GNR - Updating of rseed, phseed moved from d3seed    *
*  Rev, 04/26/97, GNR - Set to always expect condensed connection data *
*  Rev, 08/27/97, GNR - Store AK and PHD data for GRAFDATA file        *
*  Rev, 09/28/97, GNR - Independent dynamic allocations per conntype   *
*  Rev, 12/27/97, GNR - Add ADECAY, deferred allocation of statistics  *
*  Rev, 01/06/98, GNR - Add PPF, s24tous8(), d3noise()                 *
*  Rev, 11/24/00, GNR - Separate RADIUS parameter, HV in IA coords     *
*  V8B, 01/07/01, GNR - Implement detailed print via ilst package      *
*  Rev, 09/08/01, GNR - Implement probes via ilst package              *
*  V8C, 04/08/03, GNR - Cell responses in millivolts                   *
*  V8D, 08/28/05, GNR - Add conductances and ions                      *
*  Rev, 07/01/06, GNR - Move Lij, Cij, Sj generation to new routines   *
*  Rev, 01/19/07, GNR - Reorganize phase calcs so raw data go to dprt  *
*  Rev, 04/08/07, GNR - MODBY block for NMOD, general modul inputs     *
*  Rev, 09/28/07, GNR - Change KAMFZ to OPTFZ, implement KAMDR, KAMFS  *
*  ==>, 02/28/08, GNR - Last mod before committing to svn repository   *
*  Rev, 04/23/08, GNR - Add ssck (sum sign check) mechanism            *
*  Rev, 12/27/08, GNR - Change mssw,msswe to msrswe (rounding, 96-bit) *
*  Rev, 12/30/08, GNR - jm64s[bsl]-->mssle or msrsle for ovflow check  *
*  Rev, 01/07/09, GNR - Change bitor to bitior (bitor is C++ keyword)  *
*  V8E, 01/23/09, GNR - Add Izhikevich response function               *
*  Rev, 02/10/09, GNR - Add probe ramps, phase info to pctpd PHASEDEF, *
*                       remove all explicit use of vrest.              *
*  Rev, 04/12/09, GNR - Test PSP bits on paxp,paxn,aeff sums, PRADIJ   *
*  Rev, 05/02/09, GNR - Remove mabrsj, OLDET, revise amp mnsj logic    *
*  Rev, 05/14/09, GNR - Incorporate MODCOM variable names              *
*  Rev, 07/01/09, GNR - Add PRBDEF cyc1                                *
*  V8F, 05/17/10, GNR - Add KRPGP statistic                            *
*  V8G, 08/13/10, GNR - Add AUTOSCALE and KAMZS                        *
*  V8H, 10/24/10, GNR - Add xn (once per conntype) variable allocation *
*  Rev, 01/27/11, GNR - Remove obsolete evaluation of CHANGES blocks   *
*  Rev, 02/28/11, GNR - Add KAMCG (supervised categorization) option   *
*  Rev, 03/30/11, GNR - Change KAMCG to use groupn rather than icell   *
*  Rev, 03/31/11, GNR - Add modulation, GCONN, and amplif deferral     *
*  Rev, 04/22/11, GNR - Add phifn and KAMKI                            *
*  Rev, 04/28/11, GNR - Add CTOPT=C for new-stim-group fast start      *
*  Rev, 03/30/12, GNR - Add distribution of Sj statistic               *
*  Rev, 04/02/12, GNR - Expand DAS stats to include delta Cij by case  *
*  Rev, 04/09/12, GNR - Add MOPT=F, mdefer in MODVAL is minimum        *
*  Rev, 05/09/12, GNR - Add AUTOSCALE OPT=T, add self-update check     *
*  Rev, 05/26/12, GNR - Store raw specific affsums as si64             *
***********************************************************************/

#define NEWARB          /* Use new EFFARB mechanism */
#define CLTYPE   struct CLBLK
#define LIJTYPE  struct LIJARG
#define OVFI     OVF_IZHI           /* Shorter overflow code */

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "armdef.h"
#include "wdwdef.h"
#ifdef D1
#include "d1def.h"
#endif
#include "celldata.h"
#include "clblk.h"
#include "simdata.h"
#include "rocks.h"
#include "rkxtra.h"
#include "rkarith.h"
#include "swap.h"
#include "bapkg.h"

/* Prototypes unique to d3go */
void d3autsc(struct CELLTYPE *);
void d3gfsvct(struct SDSELECT *, struct CELLTYPE *, ui32);
void d3pafs(void);
void d3valu(struct VBDEF *);
static void layerupd(struct CELLTYPE *, ui16);
#ifndef PAR0
static void cijclean(struct CONNTYPE *, struct CONNDATA *);
void d3assv(struct AUTSCL *, si32);
long d3cond(struct CELLTYPE *);
void d3inht(struct CELLTYPE *);
int  d3inhb(void);
void d3inhg(void);
void d3inhc(void);
long d3phase(struct CELLTYPE *);
static void sumaffs(struct CELLTYPE *, long *);
#endif
#ifndef PAR
void d3dprt(void);
#else
void d3pdpn(void);
void d3pdpz(void);
#endif
#ifndef PARn
void d3gfsv(struct SDSELECT *, ui32);
void d3vprt(void);
long allciter(iter *pit);
#endif

/* Values 'sequence' takes on: */
#define SEQ_I     1     /* No decay, amplification or statistics */
#define SEQ_II    2     /* Statistics only                       */
#define SEQ_III   3     /* Decay, amplification and statistics   */

/* A few globals used in functions called from d3go */

struct CELLDATA CDAT;
#ifndef PAR0
extern struct LIJARG Lija;
static short *pcontbl;        /* Ptr to consolidation tables */
#endif
const byte ctab[256] = {      /* Values of "phi" function */
         0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
         0xff,0xff,0xff,0xff,0xff,0xff,0xfe,0xfe,
         0xfe,0xfe,0xfd,0xfd,0xfd,0xfd,0xfc,0xfc,
         0xfc,0xfb,0xfb,0xfa,0xfa,0xf9,0xf9,0xf9,
         0xf8,0xf8,0xf7,0xf7,0xf6,0xf5,0xf5,0xf4,
         0xf4,0xf3,0xf2,0xf2,0xf1,0xf0,0xf0,0xef,
         0xee,0xee,0xed,0xec,0xeb,0xeb,0xea,0xe9,
         0xe8,0xe7,0xe6,0xe6,0xe5,0xe4,0xe3,0xe2,
         0xe1,0xe0,0xdf,0xde,0xdd,0xdc,0xdb,0xda,
         0xd9,0xd8,0xd7,0xd6,0xd5,0xd4,0xd3,0xd2,
         0xd0,0xcf,0xce,0xcd,0xcc,0xcb,0xc9,0xc8,
         0xc7,0xc6,0xc5,0xc3,0xc2,0xc1,0xc0,0xbe,
         0xbd,0xbc,0xba,0xb9,0xb8,0xb7,0xb5,0xb4,
         0xb2,0xb1,0xb0,0xae,0xad,0xac,0xaa,0xa9,
         0xa7,0xa6,0xa5,0xa3,0xa2,0xa0,0x9f,0x9d,
         0x9c,0x9a,0x99,0x97,0x96,0x94,0x93,0x91,
         0x90,0x8e,0x8d,0x8b,0x8a,0x88,0x87,0x85,
         0x84,0x82,0x81,0x7f,0x7e,0x7c,0x7b,0x79,
         0x78,0x76,0x75,0x73,0x71,0x70,0x6e,0x6d,
         0x6b,0x6a,0x68,0x67,0x65,0x64,0x62,0x61,
         0x5f,0x5e,0x5c,0x5b,0x59,0x57,0x56,0x54,
         0x53,0x51,0x50,0x4f,0x4d,0x4c,0x4a,0x49,
         0x47,0x46,0x44,0x43,0x41,0x40,0x3f,0x3d,
         0x3c,0x3a,0x39,0x38,0x36,0x35,0x34,0x32,
         0x31,0x30,0x2e,0x2d,0x2c,0x2b,0x29,0x28,
         0x27,0x26,0x24,0x23,0x22,0x21,0x20,0x1f,
         0x1e,0x1c,0x1b,0x1a,0x19,0x18,0x17,0x16,
         0x15,0x14,0x13,0x12,0x12,0x11,0x10,0x0f,
         0x0e,0x0d,0x0c,0x0c,0x0b,0x0a,0x0a,0x09,
         0x08,0x08,0x07,0x06,0x06,0x05,0x05,0x04,
         0x04,0x03,0x03,0x03,0x02,0x02,0x02,0x01,
         0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x00 };

/*=====================================================================*
*                        d3go executable code                          *
*=====================================================================*/

int d3go(void) {

#ifndef PAR0
   /* Needed for zeroing si64 values outside initializers */
   static si64 si64zero = {0};
   /* Amplification case scales for specific traditional rule codes--
   *  these values will be indexed by ix->amptt as set in d3news().  */
   static char amp_case_scales[NTAC][8] = {
      {1,1,0,0,1,0,0,0},         /* Two-way on si */
      {1,0,1,0,1,0,0,0},         /* Two-way on Sj */
      {1,1,1,0,1,0,1,0},         /* Three-way     */
      {1,1,1,1,1,0,1,0},         /* Four-way      */
      {1,0,0,0,1,0,0,0} };       /* Pure Hebbian  */
   /* Table of cases affected by csc scaling */
   static char odd_way_cases[8] = {0,1,1,0,1,0,0,1};
   double  daxs;              /* Scale to reduce ax for output */
   /* General pointers */
   struct CONNDATA *ND;       /* Ptr to current connection data */
   struct MODVAL   *imv;      /* Ptr to modulation value block */
   struct MODBY    *imb;      /* Ptr to modulated-by block */
   struct PRBDEF   *pprb;     /* Ptr to probe definition */
#endif
   struct REPBLOCK *ir;
   struct CELLTYPE *il,*jl;
   struct CONNTYPE *ix;

   float plvx,plvy;           /* Repertoire plot x,y coords */

   int plot_cycle;            /* Cycle number for d3rplt */
#ifndef PAR0
   int  isqis;                /* Squared inhibition shift */
#endif

   /* Cycle controls */
   ui16  cyclestogo;          /* Cycles remaining in main loop */
   ui16  ntcm1;               /* = ntc - 1 */
   ui16  etnewt;              /* Test for OPT=E and new trial */

   char add_s_noise;          /* TRUE if adding noise to stimuli */
   char do_dprint;            /* TRUE if doing detailed print */
   char do_gfsv;              /* TRUE if recording SIMDATA */
   char sequence;             /* Flow control for inner loop */
   char statenbg;             /* Statistics enable (global) */
   char statenbr;             /* Statistics enable (repertoire) */
   char statenbc;             /* Statistics enable (celltype) */
   char statenbm;             /* Statistics enable (modality) */

/***********************************************************************
*                                                                      *
*        Section 1 - Initialization done on every call to d3go         *
*                                                                      *
***********************************************************************/

   CDAT.flags = 0;                  /* Clear all flags */
   RP->ovflow = 0;

/* Initialization relating to stimulus noise */

   add_s_noise = (RP->snsg | RP->snmn) && RP->snfr;

/* Set global statistics control switch */

   statenbg = (RP->CP.runflags & (RP_NOSTAT|RP_NOCYST)) == 0;

/* Implement the decision matrix relating to the selection of
*     SEQUENCE I:    No amplification or detailed amp stats
*     SEQUENCE II:   Detailed stats only, no amp or decay
*     SEQUENCE III:  Decay, amplification, and detailed stats.
*  (This done on every trial because SEQ_II changes to SEQ_III
*  after first cycle if IMMEDIATE AMPLIF option is not active.)  */

   ntcm1 = (ui16)(RP->ntc - 1);
   if (ntcm1 == 0) {
      if (RP->CP.runflags & RP_NOCHNG) sequence = SEQ_II;
      else if (RP->adefer == 0) sequence = SEQ_III;
      else sequence = SEQ_II;
      }
   else if (RP->CP.runflags & RP_NOCHNG) {
      if (RP->adefer == 0) sequence = SEQ_II;
      else sequence = SEQ_I;
      }
   else {
      if (RP->adefer == 0) sequence = SEQ_III;
      else sequence = SEQ_I;
      }

#ifndef PARn
/*---------------------------------------------------------------------*
*  Following initializations are not required on comp nodes            *
*---------------------------------------------------------------------*/

/* Clear the changes array */

   if (RP0->ljnwd) memset((char *)RP0->pjnwd, 0,
      RP0->ljnwd*sizeof(float));

/* Increment trials counters in modality statistics */

   if (statenbg) {
      struct MODALITY *pmdlt;
      ui16 ltrialsr = (ui16)RP->CP.trialsr;
      for (pmdlt=RP->pmdlt1; pmdlt; pmdlt=pmdlt->pnmdlt)
         if (ltrialsr < pmdlt->ntrs)
            ++pmdlt->um1.MS.ntract;
      }
#endif

#ifndef PAR0
/*---------------------------------------------------------------------*
*   Following initializations are not required on host                 *
*---------------------------------------------------------------------*/

/* Use the stimulus marking bits contained in the sensory broadcast
*  to fill in the pxxxx bits in each modality block indicating which
*  stimuli were actually encountered.  This info is needed in d3stat
*  to determine whether to proceed with XRM statistics, hence must be
*  available prior to GRPMSK broadcast where it might otherwise go.
*
*  (Prior to V8A, this was done in main pgm following RP broadcast,
*  using isn info in RP.  This scheme would now require broadcasting
*  modality blocks on every cycle.  It was technically wrong for two
*  other reasons: (1) could not handle more than one stimulus in a
*  presentation, and (2) was unnecessarily done during replays.  */

   if (RP->kxr & MXSTATS) {
      struct MODALITY *pmdlt;
      byte *pmbits = RP->pbcst + RP->ombits;
      for (pmdlt=RP->pmdlt1; pmdlt; pmdlt=pmdlt->pnmdlt)
         bitior(pmdlt->pxxxx, 0, pmbits,
            (int)pmdlt->mdltboff, (long)pmdlt->nds);
      } /* End pxxxx marking */

/* Set scales that depend on COMPAT=C mode */

   isqis = -(FBwk + RP->bsdc);
   daxs = 1.0/(double)(1 << (FBrs + RP->bsdc));

#endif

/*---------------------------------------------------------------------*
*      Perform new trial initialization loop over all cell types       *
*---------------------------------------------------------------------*/

   for (il=RP->pfct; il; il=il->pnct) {

      /* Clear cycle-dependent flags */
      il->ctf &= ~(DIDSTATS|CTFDPENB|OLDSILIM);

      /* Initialize for s(i) calculation freeze.  (This cannot be
      *  moved to d3news because RP->ntc can change after d3news.) */
      il->ctwk.go.ntca = (il->Ct.ctopt & OPTFZ) ? 0 : (si32)ntcm1;

#ifndef PAR0
      il->ctwk.go.fstanh = (float)il->Ct.stanh;
      /* [qs]dampc = 1.0 - [qs]damp ([qs]damp may change in d3chng) */
      il->ctwk.go.qdampc = S15 - il->Dc.qdamp;
      il->ctwk.go.sdampc = S15 - il->Dc.sdamp;

/* If ctopt OPTCFS requested, and there is a stimulus from a new
*  stimulus group, restart rbar and qbar averaging.  */

      if (il->Ct.ctopt & OPTCFS) {
         struct MODALITY *pmdlt;
         byte *pmbits = RP->pbcst + RP->ombits;
         for (pmdlt=RP->pmdlt1; pmdlt; pmdlt=pmdlt->pnmdlt) {
            if (bittst((unsigned char *)&il->amdlts,
                  (long)pmdlt->mdltno) &&
                  bittst(pmbits, (long)pmdlt->mdltnoff)) {
               il->ctf |= CTFDOFS;
               break;
               } /* End if new stimulus class found */
            } /* End modality scan */
         } /* End if OPTCFS */

/* Set enable flag for detail print to save two tests in cycle loop.
*  (If done in d3news, would require an extra il loop.)  */

      if ((RP->CP.runflags & RP_OKPRIN) &&
            il->ctclid[CTCL_DPRINT] && !(il->ctf & CTDPDIS))
         il->ctf |= CTFDPENB;

/* Set up for probes.  Check trial timer and set
*  whether and where to start in cell list.  */

      pprb = il->pctprb + il->ctclid[CTCL_PRBSL];
      if (pprb) {
         if (RP->kevtu & EVTPRB &&
               bittst(RP->CP.trtimer, (long)pprb->jprbt)) {
            pprb->cyce = pprb->cpp + pprb->cyc1 - 1;
            /* Set voltage for first cycle = sprb after ramp added */
            pprb->sprbcyc = pprb->sprb - pprb->ramp;
#ifdef PAR
            if (pprb->iprbcyc < pprb->myipc1)
               pprb->cyce = 0;
            else if (pprb->iprbcyc == pprb->myipc1) {
               pprb->ipptr   = pprb->myipt1;
               pprb->prob1st = il->locell;
               }
            else {
               pprb->ipptr   = pprb->pptr;
               pprb->prob1st = pprb->probnxt;
               }
#else
            if (pprb->iprbcyc == 0) {
               pprb->ipptr   = pprb->pptr;
               pprb->prob1st = 0;
               }
            else
               pprb->prob1st = pprb->probnxt;
#endif
            if (++pprb->iprbcyc == pprb->lprbcyc)
               pprb->iprbcyc = 0;
            }
         else
            pprb->cyce = 0;
         }
#endif

/*---------------------------------------------------------------------*
*                New-trial loop over connection types                  *
*---------------------------------------------------------------------*/

      for (ix=il->pct1; ix; ix=ix->pct) {

/* New trial initialization: new connection type--
*     Set number of cycles simulation should be active.
*     Set kgfl bit KGNBP for normal or bypass action.
*     For IA input, locate first stimulus byte at window.
*     For HV input, locate arm with respect to window.  */

/* Determine number of cycles needed to simulate until all
*  directly-connected inputs have quiesced.  (Since ntca
*  is set to (cycles-1), we get (ntcs+1) effectively.)  */

         if (il->Ct.ctopt & OPTFZ && ix->cnflgs & DIMDLT) {
            si32 lntcs = (si32)ix->psmdlt->ntcs;
            if (lntcs > il->ctwk.go.ntca)
               il->ctwk.go.ntca = lntcs;
            }

/* If topographic or scanned input from IA, set osrc = offset of first
*  stimulus byte to use = center of IA if 'T' or center of window if
*  'S' minus half arbor size.  (Done before bypass check in case
*  program tries to plot a bypassed connection type.)  */

         if (ix->kgfl & KGNIA)
            ix->osrc = (ix->kgen & KGNTP) ? d3woff(ix) : 0;

/* N.B.  KAMBY is the master control for bypassing a connection type.
*  KGNBP must accurately reflect KAMBY plus any other conditions that
*  cause bypassing, because it is used to maintain rseed and upphseed
*  updating in synch (1) at start of cell loop, correcting for cells
*  on lower nodes, (2) during cell loop, updating cells on current
*  node, and (3) at layerfin, correcting for cells on higher nodes.
*  KGNBP must be reset here on each trial because it can be set for
*  other reasons (ntcs < ntc) during trial cycling.  */

         ix->kgfl &= ~KGNBP;
         /* Bypass if requested by the user or if no input or if
         *  connected directly to a sensory modality and past ntrs
         *  trials since reset for that modality.  */
         if (!(ix->cssck & (PSP_POS|PSP_NEG)) ||
            ((ix->cnflgs & DIMDLT) &&
               ((ui16)RP->CP.trialsr >= ix->psmdlt->ntrs))) {
            ix->kgfl |= KGNBP;
            continue;
            }

/* Amplification setups previously done here moved to d3news,
*  02/20/97, GNR.  These cannot change during a trial series.  */

/* The following data change only at new trial time, not at
*  new cycle time, therefore, not needed to set in cycle loop. */

#ifndef PAR0

/* If hand-vision input, pick up and save relative hand location */

#ifdef BBDS
         if (ix->kgen & KGNBV) {    /* Distances from BBD device */
            if (ix->cnxc.tvmode == Col_R4) {
               /* Input is already floating-point.  This is reduced
               *  to a fraction using vcmin,vcmax as scale limits. */
               float *pvf = RP->paw + ix->osrc;
               ix->ul1.w.xwa = (pvf[    0   ] - ix->ul2.a.bvv1);
               ix->ul1.w.ywa = (pvf[ix->loff] - ix->ul2.a.bvv1);
               }
            else {
               /* Input is fixed-point (S8) */
               byte *pvb = RP->pbcst + ix->osrc;
               ix->ul1.w.xwa = (float)pvb[    0   ];
               ix->ul1.w.ywa = (float)pvb[ix->loff];
               }
            /* Convert distances to scale 0 to nux, 0 to nuy */
            ix->ul1.w.xwa *= ix->ul2.a.bvxdv;
            ix->ul1.w.ywa *= ix->ul2.a.bvydv;
            }
         else
#endif
         if (ix->kgen & KGNHV) {
            float dx,dy;
#ifdef PAR
            float *awdim;        /* Ptr to window dimensions */
            awdim = (float *)RP->paw + (ix->armi*BARM_VALS);
            dx = awdim[AWX];
            dy = awdim[AWY];
#else
            struct ARMDEF *pa = (struct ARMDEF *)(ix->psrc);
            dx = pa->armx;
            dy = pa->army;
#endif
            /* Correct origin for window being used--loff
            *  is nonzero only if a viewing through a window */
            if (ix->loff) {
#ifdef PAR
               /* Skip over arm and lower-numbered window data */
               awdim = (float *)RP->paw +
                  (RP->narms*BARM_VALS + ix->wdwi*BWDW_VALS);
               dx -= awdim[AWX];
               dy += awdim[WHT] - awdim[AWY];
#else
               /* Find the relevant window--it is known wdwi < nwdws */
               struct WDWDEF *pw = RP0->ppwdw[ix->wdwi - 1];
               dx -= pw->wx;
               dy += pw->wht - pw->wy;
#endif
               } /* End WINDOW if */
            ix->ul1.w.xwa = dx;
            ix->ul1.w.ywa = dy;
            } /* End if KGNHV */
#endif /* !PAR0 */

/* Stride and offset testing previously done here for VG inputs moved
*  to d3tchk, 11/16/91, GNR.  With removal of VF,VR inputs, the length
*  of a VG array can no longer change according to the stimulation.  */

         }  /* End loop over ix */

      if (il->ctwk.go.ntca > (si32)ntcm1)
         il->ctwk.go.ntca = (si32)ntcm1;

      }  /* End loop over il */

/***********************************************************************
*                                                                      *
*                         Section 2 - Main loop                        *
*                                                                      *
***********************************************************************/

/*---------------------------------------------------------------------*
*                                                                      *
*                           LOOP OVER CYCLES                           *
*                                                                      *
*     Begin new cycle--(cycles remaining in cyclestogo)                *
*        Set switches for stimulus cutoff, probing, and stats.         *
*        Calculate modulation from virtual groups.                     *
*        Contaminate stimuli with noise.                               *
*                                                                      *
*---------------------------------------------------------------------*/

   cyclestogo = RP->ntc;  /* Initialize cycle counter */
NEWCYCLE:
   CDAT.curr_cyc1 = RP->ntc - cyclestogo + 1;
   etnewt = CDAT.curr_cyc1 == 1 ? KAUT_ET : 0;
   RP->CP.effcyc += 1;

   /* Determine whether this cycle is to be recorded in SIMDATA */
   do_gfsv = RP->kevtu & EVTWSD && (RP->ksv & KSDNC ||
      cyclestogo == 1) && bittst(RP->CP.trtimer, (long)RP->jsdsvtm);

/* Contaminate input array stimuli with noise.
*     Note: Stimuli are stored in strips of nsy bytes.
*     Each strip is followed by nsy bytes used for assignments.
*     The noise-contaminated values are now stored in this space.
*     A special faster path is used if no noise is being added.  */

   if (!(RP->CP.runflags & RP_NOINPT)) {
      long lnsy = RP->nsy;
      stim_mask_t *pstm,*pstmlim;
      stim_mask_t *mxstim = RP->pstim + (RP->nsx << RP->ky1);
      if (add_s_noise) {         /* Adding noise */
         for (pstm=RP->pstim; pstm<mxstim; pstm+=lnsy) {
            pstmlim = pstm + lnsy;
            for ( ; pstm<pstmlim; pstm++) {
               register long work = *pstm << Ss2hi;   /* To (S24) */
               work += d3noise(RP->snmn,RP->snsg,RP->snfr,RP->snseed);
               *(pstm+lnsy) = (stim_mask_t)(work <= 0 ? 0 :
                  (work >= 0x00fe8000L ? 0xff :
                  ((work + 0x00008000L) >> Ss2hi)));
               } /* End column loop */
            } /* End row loop */
         }
      else {                     /* Not adding noise */
         for (pstm=RP->pstim; pstm<mxstim; pstm+=lnsy) {
            pstmlim = pstm + lnsy;
            memcpy((char *)pstmlim, (char *)pstm, lnsy);
            pstm = pstmlim;
            } /* End row loop */
         }
      } /* End if (input exists) */

#ifndef PARn      /* Host-only diagnostics */
   /* Update title to include cycle number */
   {  char cycln[14];
      sconvrt(cycln, "(7H Cycle J0UH5)", &CDAT.curr_cyc1, NULL);
      cryout(RK_P1, RP0->stitle, RK_NFSUBTTL+RK_LN2+RP0->lst,
         cycln, RK_CCL, NULL);
      }

   /* Record non-network data in SIMDATA */
   if (do_gfsv) {
      stattmr(OP_PUSH_TMR, GDFL_TMR);
      d3gfsv(RP0->pgds, ~(GFLIJ|GFDIJ|GFABCD));
      stattmr(OP_POP_TMR, 0);
      }

   /* Print value array if requested */
   if ((RP->kdprt & PRVAL) && (RP->nvblk > 0)) d3vprt();

   /* Print environment if requested.  This code moved here from
   *  main, 06/26/08, to allow printing IA after noise added and
   *  on every cycle.  */
   if (RP->kpr & KPRENV)
      envprt(RP0->Eptr, RP->pstim, (int)RP0->kiarr);
#endif

#ifndef PAR0

/*---------------------------------------------------------------------*
*   Following initializations are not required on host node            *
*---------------------------------------------------------------------*/

/* Evaluate modulation contributions from virtual groups.  Given that
*  mdgetfn() returns input on S14 (=S7 mV), it is now necessary to
*  perform 64-bit sum to avoid possible overflow.  Input from an IA
*  window requires a 2-D loop.  (There is a cute way to read the
*  full IA in a single loop with a subscript and a mask, but there
*  is less fuss treating it in two loops as a large window.)  Note
*  that if any MODBY using this MODVAL has decay, Mdc.mdefer is set
*  to 0 so summation is always done.  */

   for (imv=RP->pmvv1; imv; imv=imv->pmdv) {
      if (CDAT.curr_cyc1 < imv->Mdc.mdefer)
         imv->umds.mdsum = si64zero;
      else {
         s_type *pcell  = (s_type *)imv->amsrc; /* Ptr to src cells */
         gvin_fn getin = imv->mdgetfn;       /* Ptr to fn to get data */
         si64   wsum  = si64zero;            /* Working sum */
         si32   lmthi = (si32)imv->Mdc.mt;   /* Local copy of mthi */
         si32   lmtlo = (si32)imv->Mdc.mtlo; /* Local copy of mtlo */
         si32   lmjrv = (si32)imv->Mdc.mjrev;/* Local copy of Vrev */
         si32   lsmthi, lsmtlo;              /* Subtracted thresholds */
         si32   wksi;                        /* Working si */

         /* Set for STEP vs KNEE thresholds */
         if (imv->Mdc.mopt & IBOPKR)
            lsmthi = lmthi, lsmtlo = lmtlo;
         else
            lsmthi = lsmtlo = 0;
         /* Set to eliminate zero entries */
         if (lmthi) lmthi -= 1;
         if (lmtlo) lmtlo += 1;

         if (imv->mdsrctyp == IA_SRC) {   /* Input from IA */
            s_type *pcxe,*pcye;           /* Limiting pcell x,y vals */
            long lnsy = RP->nsy;          /* Offset to noisy input */
            long x1,x2,y1,y2;             /* Window x,y limits */
            long xinc,yeinc;              /* X,Yend increments */
            int  iw = imv->mdsrcndx;      /* Window number */
            if (iw > 0) {
               /* Looking through a window */
   #ifdef PAR
               /* Skip over arm and lower-numbered window data */
               float *wdim = (float *)RP->paw +
                  (RP->narms*BARM_VALS + (iw-1)*BWDW_VALS);
               x1 = (long)wdim[AWX];
               x2 = (long)(wdim[AWX] + wdim[WWD]);
               y1 = (long)(wdim[AWY] - wdim[WHT]);
               y2 = (long)wdim[AWY];
   #else
               /* Serial version--access WDWDEF block directly */
               struct WDWDEF *pw = RP0->ppwdw[iw - 1];
               x1 = (long)pw->wx;
               x2 = (long)(pw->wx + pw->wwd);
               y1 = (long)(pw->wy - pw->wht);
               y2 = (long)pw->wy;
   #endif
               if (x1 < 0) x1 = 0;
               if (x2 > RP->nsx) x2 = RP->nsx;
               if (y1 < 0) y1 = 0;
               if (y2 > RP->nsy) y2 = RP->nsy;
               }
            else {
               /* Looking at full input array */
               x1 = 0; x2 = RP->nsx;
               y1 = 0; y2 = RP->nsy;
               }
            pcxe  = pcell + (x2 << RP->ky1);
            /* Add lnsy to access noise-contaminated inputs */
            pcell += (x1 << RP->ky1) + lnsy;
            pcye  = pcell + y2;
            pcell += y1;
            yeinc = lnsy + lnsy;
            xinc  = yeinc - (y2 - y1);
            /* Sum inputs above thresh over the window */
            for ( ; pcell<pcxe; pcye+=yeinc,pcell+=xinc) {
               for ( ; pcell<pcye; ++pcell) {
                  wksi = getin(pcell, &imv->Mdc.mvxc) - lmjrv;
                  if      (wksi > lmthi)
                     wsum = jasl(wsum, wksi - lsmthi);
                  else if (wksi < lmtlo)
                     wsum = jasl(wsum, wksi - lsmtlo);
                  } /* End y loop */
               } /* End x loop */
            } /* End input from IA */
         else if (imv->mdsrctyp == VALSRC) { /* Input from Value */
            wksi = (*(long *)imv->amsrc << Sv2mV) - lmjrv;
            if      (wksi > lmthi) wsum = jesl(wksi - lsmthi);
            else if (wksi < lmtlo) wsum = jesl(wksi - lsmtlo);
            } /* End input from Value */
         else {                     /* Input not from IA or Value */
            s_type *pce;                  /* Limiting pcell value */
            long   pci = imv->mdincr;     /* Cell increment */
            pce = pcell + pci*imv->mdnel;
            /* Sum inputs above thresh */
            for ( ; pcell<pce; pcell+=pci) {
               wksi = getin(pcell, &imv->Mdc.mvxc) - lmjrv;
               if      (wksi > lmthi) wsum = jasl(wsum, wksi - lsmthi);
               else if (wksi < lmtlo) wsum = jasl(wsum, wksi - lsmtlo);
               }
            } /* End not IA or VAL */
         imv->umds.mdsum = wsum;
         } /* End not deferred */
      } /* End of virtual modulation sum loop */
#endif

/*---------------------------------------------------------------------*
*                                                                      *
*     Main simulation loop--process all repertoires                    *
*                                                                      *
*---------------------------------------------------------------------*/

   for (ir=RP->tree; ir; ir=ir->pnrp) {

      /* Set repertoire-level control for general statistics */
      statenbr = (ir->Rp.krp & KRPNS) ? FALSE : statenbg;

      /* Setup superposition plot for current repertoire */
      if (RP->kpl1 & (KPLSUP|KPLMFSP)) {
         /* Set to omit all plotting if not requested */
         if (!(ir->Rp.krp & (KRPPL|KRPBP)))
            plot_cycle = -1;
         /* Adjust coords each cycle for PLVTS plots.  The last
         *  plot ends up at the given coords aplx,aply.  This
         *  scheme enables d3lplt to locate source cells even
         *  when KP=V is active.  (A more accurate calc would use
         *  max(ntca) over il as first cyclestogo.)  */
         else if (RP->kplot & PLVTS && ir->Rp.krp & KRPVT) {
            float fcycle = (float)(cyclestogo - 1);
            plvx = ir->aplx - fcycle*ir->vxo;
            plvy = ir->aply - fcycle*ir->vyo;
            plot_cycle = (cyclestogo > 1) ? (int)CDAT.curr_cyc1 : 0;
            }
         else {      /* Normal superposition, no time series */
            plvx = ir->aplx;
            plvy = ir->aply;
            plot_cycle = (cyclestogo > 1) ? -1 : 0;
            }
         if (plot_cycle >= 0)
            d3rplt(ir, plot_cycle, plvx, plvy, ir->aplw, ir->aplh);
         } /* End if superposition plot */

/*---------------------------------------------------------------------*
*            Process all celltypes for current repertoire              *
*---------------------------------------------------------------------*/

      for (il=ir->play1; il; il=il->play) {
         iter dpit;              /* Iterator for detailed print */
#ifndef PAR0
         iter prbit;             /* Probe iteration control block */

         struct PPFDATA  *ppfd;  /* Ptr to PPF parameters */
         struct RFRCDATA *prf;   /* Ptr to refractory period params */
         struct PHASEDEF *ppd;   /* Ptr to phasing parameters */
         struct GPSTAT   *pgp0;  /* Ptr to KRPGP stats */
         rd_type *plsd;          /* Ptr to "ls" data for current cell */
         si64    *prbar;         /* Ptr to rbar info stored for stats */
         s_type  *psnew;         /* Ptr to new value of s(i) */
         long    *paxp,*paxn;    /* Ptrs to raw,processed affs */
         short   *pctway;        /* Ptr to wayset scale at ct level */
         short   *pdelc;         /* Ptr to saved delta Cij for dprnt */
         byte    *pizv,*pizu;    /* Ptrs to 32-bit Izhik. v,u data */
         byte    *pmark,*tmark;  /* Ptrs to modality marker bits */

         long celllim;           /* Cell counter limit */
         long cellaffs;          /* Sum of A+G+M afferent voltages and
                                 *  probes applied to a cell (S20) */
         long coff;              /* Offset to this cell in prd data */
         long hacell;            /* Hebb amp cell term (old smthm),
                                 *  S22 if (v-mtv)*(Si-mti), else S14 */
         long icell;             /* Cell counter */
         long laffb;             /* Number of bytes in laffs si64's */
         long lpptr;             /* Local probes/trial counter */
         long normpers;          /* Persistence included in NormSums */
         long refrcs16;          /* Negative refractory time (S16) */
         long response;          /* Response after nonlinearities
                                 *  (S20/27) */
         long rnd1,rnd2,rnd3;    /* Rounding values derived from rsd */
         long wkdst;             /* Working value of dst (mV S20) */
         long wkst;              /* Working value of st (mV S20) */

         si32 llohi;             /* Local high for lowest high stat */
         si32 lzseed;            /* Working value of izseed */
         si32 oldsi;             /* si(t-1) (mV S20) */

         int  hitht,hitpt,hitnt; /* TRUE if s(i) >= ht, >= pt, <= nt */
         int  ihighrb;           /* Highest irb */
         int  irb;               /* Response bin for statistics */
         int  laffs;             /* Number of terms in a sum */
         int  sovrinc;           /* sover stat increment */

         byte decay_bit;         /* Equal to DECAYED if omega2 != 0,
                                 *  otherwise zero.  */
         char didnorms;          /* TRUE if normal sums done */
         char marking;           /* TRUE if marking is being done */
#endif
         CDAT.cdlayr = il;
         CDAT.myngtht = 0;

         /* Prevent s1,s2 flip, stats at end if calc skipped */
         il->ctf &= ~(DIDSCALC|CTFSTENB);

         /* Set up statistical accumulation according to
         *     global, repertoire, and celltype controls */
         if (il->Ct.kctp & CTPNS)
            statenbc = statenbm = FALSE;
         else if (il->ctwk.go.ntca == 0)
            statenbc = statenbm = statenbr;
         else if (RP->CP.runflags & RP_ALLCYC)
            statenbc = statenbr, statenbm = FALSE;
         else
            statenbc = statenbm = FALSE;
         if (statenbc) il->ctf |= CTFSTENB;

         /* Omit entire layer if s(i) freeze and have done one
         *  cycle after all direct inputs have quiesced.  */
         if (il->ctwk.go.ntca < 0) goto ADVLAYER;

/* Set bypass bits now so e.g. d3kiji() will see them in time */

         for (ix=il->pct1; ix; ix=ix->pct) {
            if ((ix->cnflgs & DIMDLT) &&
                  (CDAT.curr_cyc1 > ix->psmdlt->ntcs))
               ix->kgfl |= KGNBP;
            }

/* Set flags to control autoscaling.  il->kautu is copied from
*  il->paut->kaut, then modified to reflect actions in this cycle.
*  KAUT_NOW bit is set to cause a new scale to be calculated now
*  (always with OPT=C, in first cycle of trial with OPT=T, other-
*  wise in last cycle of trial).  KAUT_[SGMA] bits are set only
*  when scale is to be applied to that input category.  */

         il->kautu = 0;
         if (il->pauts && il->pauts->ngtht > 0) {
            struct AUTSCL *paut = il->pauts;
            il->kautu = paut->kaut;
            if (il->ctf & CTFASFS)
               il->kautu |= KAUT_FSN;
            if (!(RP->CP.runflags & RP_NOUPAS) &&
                  !(il->kautu & KAUT_NU) && (il->ctf & CTFASFS ||
                  il->kautu & (KAUT_CYC|etnewt) ||
                  (!(il->kautu & KAUT_ET) && il->ctwk.go.ntca == 0)))
               il->kautu |= (KAUT_NOW|KAUT_UPD);
            if ((il->kautu & (KAUT_FSN|etnewt)) || paut->adamp == S15)
               il->kautu &= ~KAUT_UPD;
            if (il->kautu & (KAUT_IMM|KAUT_FSN|etnewt)) {
               il->kautu &= ~(KAUT_S|KAUT_G|KAUT_M|KAUT_A);
               il->kautu |= KAUT_APP; }
            /* If old scale is not being used, set it to 1.0 for
            *  neater printing in d3dprt.  */
            if (il->kautu & (KAUT_FSN|etnewt))
               RP->psclmul[il->oautsc] = S24;
            il->ctf &= ~CTFASFS;
            /* In KAUT_IMM mode, a second pass is made through all the
            *  cells to update s(i), sbar, and qbar.  A separate copy
            *  of rsd is used to maintain determinism if parallel.  */
            il->pauts->asrsd = il->rsd;
            }
         if (RP->compat & OLDRANGE && !(il->kautu & KAUT_APP))
            il->ctf |= OLDSILIM;

/* Setup for noise generation.
*  N.B.  One would just like to use nmodval as the noise switch,
*  but this will not work because mdsum is not known on host.  */

         if (il->ctf & CTFHN) {
#ifdef PARn
            /* Make seed correspond to first cell on this node */
            udevskip(&il->nsd, il->locell<<1);
#endif
            }
         else
            CDAT.noise = 0;

/* Turn detailed print on or off and start selection list iterator */

         do_dprint = (il->ctf & CTFDPENB) &&
            (il->ctwk.go.ntca == 0 || il->dpitem & DPVERB) &&
            bittst(RP->CP.trtimer, (long)il->jdptim);
         if (do_dprint)
            ilstset(&dpit, il->dpclb->pclil, il->locell);

/* The pointers, lengths, and consolidation tables calculated by
*  d3pafs do not change from cycle to cycle but may change at Group
*  III time.  These calculations are repeated in each cycle just to
*  save the space in CELLTYPE's that would be needed to store all the
*  results.  Needed on host for detail print.  */

         d3pafs();

#ifndef PAR0
/*---------------------------------------------------------------------*
*   Following initializations are not required on host node            *
*---------------------------------------------------------------------*/

/* Set length of sums going into cell totals */

         laffs = 1 << il->pdshft;
         laffb = sizeof(si64) * laffs;

         /* Clear highest response accumulator */
         ihighrb = 0;

/* Get pointer to rbar info (1 si64 per conntype) */

         prbar = CDAT.rsumaij + il->nct;

/* Get pointers to refractory and phase info */

         prf = il->prfrc;
         ppd = il->pctpd;

/* Setup to mark CDAT.events as DECAYED if there is decay */

         decay_bit = il->Dc.CDCY.kdcy ? DECAYED : 0;

/* If AUTOSCALE was requested, and there is self-input, adjust siscl
*  accordingly and store scale actually to be used in ix->sclu.
*  Because KAUT_A is by definition self-input, it is allowed
*  per user request without checking source.  */

         il->sisclu = (il->kautu & KAUT_A) ? mssle(il->Dc.siscl,
            RP->psclmul[il->oautsc], -FBsc, OVF_AUTS) : il->Dc.siscl;

/* Set up for geometric connections (apply autoscale, etc.) */

         if (il->pib1) d3inht(il);

/* Advance seed for Izhikevich neuron a,b,c,d randomization--only
*  needed if randomizing and OPTMEM  */

         if (il->ctf & CTFRANOS) {
            lzseed = ((struct IZHICOM *)il->prfi)->izseed;
#ifdef PAR
            udevskip(&lzseed, il->nizvar*il->locell);
#endif
            }

#ifdef PARn
/* Advance seed for phase option to low cell on this node if needed.
*  N.B.  phiseed advances once per cell.  This seed is advanced on
*  host at layer update time to account for all cells in the layer.
*  This whole scheme of course must be reexamined if host ever
*  participates in calcs.  */

         if (il->ctf && CTFNC)
            udevskip(&ppd->phiseed, il->locell);

/* Advance cell-level decay (random-rounding) seed.  At present, this
*  seed is used up to three times per cell per cycle to round qbar &
*  sbar, dst, and depression, but we advance it deterministically four
*  times.  This makes results independent of which options may be
*  turned on or off, and allows one additional use of rsd to be added
*  later without changing results obtained with the present version.
*/

         udevskip(&il->rsd, il->locell<<2);
#endif
/* Setup for geometrical connections:
*     Four routines must be called to implement geometric connections.
*     d3inhi() is called from main once per CYCLE card to initialize
*     internal variables.  d3inhb() is called from here once per cell
*     type per cycle.  In the serial version, it evaluates all gconns
*     and leaves results in bandsum arrays.  In the parallel version,
*     it only sets up layer totals for OPT=X.  d3inhg() is called once
*     per cell group. It completes the calc of everything that depends
*     only on the group and saves partial totals for d3inhc().  If
*     parallel, it does the boxsums.  In all cases, it calculates the
*     full scaled bandsums and the sum of capped bandsums (rnsums),
*     omitting the central group for INHIBBLKs with IBOPT=V.  Finally,
*     d3inhc() is called once per cell.  It adds the band 0 contribs
*     from any INHIBBLKs with IBOPT=V into the rnsums, omitting the
*     activity of the target cell.  Results are returned in the appro-
*     priate sums indexed by ib->ogsei.  In all cases, bandsums are
*     left in the INHIBBAND structures for statistics.  In the serial,
*     but not the parallel case, d3inhb can return nonzero indicating
*     that the user pressed the GUI "interrupt" button during d3bxvp
*     to terminate the trial series.  */

         if (il->pib1) {
#ifdef PAR
            d3inhb();
#else
            int rc = d3inhb();
            if (rc > 0) return rc;
#endif
            }

/* Evaluate specific modulation for this cell type by summing over
*  modval blocks pointed to by modby blocks. (The real modval blocks
*  are updated with the s(i), the virtual ones at the start of each
*  cycle.)  The ideal algorithm would be to multiply by the scale to
*  get a 94-bit S27 product, then divide by mdnel, then add S7>>1 to
*  round, then scale right 7.  But lacking 94-bit division, we use
*  multiply, round, scale, divide as a good approximation, noting
*  that an msrswe overflow cannot be rescued by dividing first now
*  that msswe,msrswe have 94-bit intermediates, so that 03/25/08
*  code has been removed.  The original treatment that mto was
*  applied knee-style to the average before scaling is preserved
*  formally when mopt bit IBOPKR is set, although the numerical
*  results will not be identical.
*
*  V8A, 04/02/96, GNR - Save afferences in CDAT.pModulAffs, omit
*                       phase convolution if no kernel defined.
*  Rev, 05/04/96, GNR - Add cap and decay.
*  Rev, 06/03/06, GNR - Remove phasing, always treat as PHASU
*  V8D, 04/09/07, GNR - Now handle noise modulation here with
*                       decay, stats, etc.
*  Rev, 03/25/08, GNR - Remove old-style scaling (divide before
*                       multiply), allowing mt and mto to coexist.
*                       Skip all calcs if mscl == 0, storage space
*                       is no longer allocated in this case.
*  Rev, 05/04/08, GNR - Introduce mopt IBOPKR subtraction control.
*  Rev, 12/27/08, GNR - Change msswe->msrswe, remove code to
*                       divide first if there is an overflow.
*  V8E, 05/05/09, GNR - Use the -1,+1 adjustment on the old tests.
*  V8G, 08/17/10, GNR - Apply scale multiplier to mscl.
*  V8H, 04/09/12, GNR - Add mdefer test, as MODVAL may have been
*                       calculated for a smaller mdefer.  Add new
*                       IBOPFS test.
*/

         CDAT.nmodval = il->No.nfr;
         if (CDAT.lModulSums > 0) {
            memset((char *)CDAT.pModulSums, 0, CDAT.lModulSums);
            for (imb=il->pmby1; imb; imb=imb->pmby) {
               struct MODVAL *imv;
               si64 nelmto,wmsum;
               long mdec;
               si32 work,wscl,lmxm;

               if (!(imb->mssck & (PSP_POS|PSP_NEG))) continue;
               if (CDAT.curr_cyc1 <= imb->Mc.mdefer &&
                  !imb->MDCY.omega) continue;
               imv = imb->pmvb;
               mdec = 0;
               paxp = CDAT.pAffData + imb->omafs;

               /* N.B.  If no tests are specified, mto = mtolo = 0
               *  and all nonzero sums will pass one or the other
               *  test and get used.  */
               /* Check high threshold */
               if (imb->mto) {
                  nelmto = jrsl(jmsw((si32)imb->mto, imv->mdnel),1);
                  wmsum = jrsw(imv->umds.mdsum, nelmto); }
               else
                  wmsum = imv->umds.mdsum;
               if (qsw(wmsum) > 0) goto ScaleModVal;
               /* Check low threshold */
               if (imb->mtolo) {
                  nelmto = jasl(jmsw((si32)imb->mtolo, imv->mdnel),1);
                  wmsum = jrsw(imv->umds.mdsum, nelmto); }
               else
                  wmsum = imv->umds.mdsum;
               if (qsw(wmsum) < 0) goto ScaleModVal;
               work = 0;
               goto ModvalBelowThresh;

ScaleModVal:   if (!(imb->Mc.mopt & IBOPKR)) wmsum = imv->umds.mdsum;
               /* Apply autoscale adjustment to mscl */
               wscl = (il->kautu & KAUT_M && !imb->Mc.modself) ?
                  mssle(imb->mscl, RP->psclmul[il->oautsc], -FBsc,
                     OVF_AUTS) : imb->mscl;
               /* S7 (mdsum) + S20 (mscl) - 7 ==> S20 */
               wmsum = msrswe(wmsum, wscl, -FBsi, OVF_MOD);
               work = jdswq(wmsum, imv->mdnel);

ModvalBelowThresh:
               /* Save "raw" mval for dprt and stats */
               *paxp++ = work;

               /* Apply cap */
               lmxm = imb->mxmod;
               if (labs(work) > lmxm) {
                  work = (work >= 0) ? lmxm : -lmxm;
                  }

               /* Apply decay if requested */
               /* N.B.  04/19/12, GNR - Here w/OPT=F we store current
               *  value as persistence for next time but do not add
               *  omega times it into itself--to give a more gentle
               *  startup.  */
               if (imb->MDCY.omega) {
                  if (imb->Mc.mopt & IBOPFS && CDAT.curr_cyc1 == 1)
                     *paxp++ = imb->umve.mvaleff = work;
                  else work = d3decay(&imb->MDCY, &imb->umve.mvaleff,
                     work, paxp++);
                  }

               /* Save final value for dprt */
               *paxp = work;

               if (CDAT.curr_cyc1 <= imb->Mc.mdefer) continue;
               if (work >= 0 && imb->Mc.mopt & MOPNEG) continue;

               if (imb->mct == 0) {
                  /* Perform noise modulation */
                  work = SRA(work, (FBwk + FBsi - FBsc));  /* To S24 */
                  if (imb->Mc.mopt & IBOPTQ) {  /* Squared? */
                     si32 wksq = msrsle(work, work, -FBsc, OVF_RFN);
                     work = (work < 0) ? -wksq : wksq;
                     }
                  CDAT.nmodval += work;
                  }
               else {
                  /* Perform cell response modulation.  If celltype has
                  *  phase, distribute modulation uniformly, otherwise
                  *  there is just one term.  */
                  long *pSums = CDAT.pAffData +
                     imb->omsei[(ui32)work >> (BITSPERUI32-1)];
                  int  p;
                  for (p=0; p<laffs; p++)
                     pSums[p] += work;
                  }

               }  /* End of loop over modby blocks */
            } /* End if lModulSums > 0 */

/* Setup for cross-response and history marking.  A mask is created in
*  the pmark array which has a field for each sensory modality that
*  contributes to this cell type.  Within each such field, bits are
*  set for all distinct stimuli that are visible to this cell type in
*  this cycle.  Marking of the XRM ('marking' switch) and Cdist data
*  for individual cells is later done from this mask (when requested).
*  However, the pmark data must be maintained even if this cell type
*  has no stats, for possible calculation of the responses of down-
*  stream cell types ('masking' switch).  Accumulation of response
*  history requires the delmk bit array, which records changes in
*  stimulation from trial to trial.  It is set up after pmark is
*  initialized in the conntype loop below.  */

         marking = statenbm && ctexists(il,XRM);
         il->ctwk.go.masking = statenbg && (il->ctf & CTFXD);
         if (il->ctwk.go.masking) {
            pmark = il->ps2 + spsize(il->mcells,il->phshft);
            memset((char *)pmark, 0, (size_t)il->lmm);
            }

/* Set negative refractory period (S16) for faster clock setting */

         if (prf) refrcs16 = -((long)prf->refrac << Ss2hi);

/* Set up for probes.  Note that cyce is 0 to skip probing,
*  e.g. if, in a PAR computer, lowest cell is not on this node.  */

         pprb = il->pctprb + il->ctclid[CTCL_PRBSL];
         lpptr = 0;
         if (pprb && CDAT.curr_cyc1 >= pprb->cyc1 &&
                     CDAT.curr_cyc1 <= pprb->cyce) {
            ilstset(&prbit, pprb->pclb->pclil, pprb->prob1st);
            pprb->sprbcyc += pprb->ramp;
            CDAT.probeval = pprb->sprbcyc;
            lpptr = pprb->ipptr;
            }
#endif

/*---------------------------------------------------------------------*
*  Initialize the individual connection types for new cycle            *
*                                                                      *
*     Set bypass bit if DIMDLT input and past ntcs cycles.             *
*     Store (1-rdamp) for running average Sj calculation               *
*     Count amplification cycles for stats.                            *
*     Advance random-number seeds.                                     *
*     Store (V-mtv),(V*RHO) for value-modulated amplification.         *
*     Store working amp scales for 8 cases based on rule.              *
*     Set ix->srcorg to point to array of source Sj.                   *
*     Store address for decaying afferent voltage.                     *
*     OR suitable contribution into modality mask from input.          *
*                                                                      *
*  Note:  Initializations relating to Lij,Cij,Dij generation and       *
*     Sj lookup have been moved to d3kiji().                           *
*---------------------------------------------------------------------*/

#ifndef PAR0
         pctway = CDAT.pglway;      /* Storage for amp scales */
#endif
         for (ix=il->pct1; ix; ix=ix->pct) {

/* Turn off bits that may change on each cycle */

            ix->cnflgs &= ~UPRSD;

/* This little setup is required on all nodes because it controls
*  detail print on host.  */

            ix->finbnow =
               (sequence == SEQ_III) ? ix->finbi : NO_FIN_AMP;

/* If bypassing, skip rest of setup */

            if (ix->kgfl & KGNBP) continue;

/* Value used for computing running average Sj */

            ix->cnwk.go.rdampc = S15 - ix->Cn.rdamp;

/* Count amplification cycles for this conntype.  (Count on
*  all nodes so is preserved through d3gsta() collection.)
*  Note:  DAS can be obtained even if there is no actual
*  amplification, but nampc is used only for the "Synaptic
*  Change" stats, which are meaningless if no actual amp.  */

            if (ix->finbnow >= NORMAMP) ++ix->CNST.nampc;

/* Advance random number-generating seeds as required.
*  N.B.  Rounding of Mij and Rbar uses bits of rseed that are not
*  used for Cij rounding, so more than two updates per synapse are
*  never needed.  However, this code was changed in V8D to do the
*  updating in SEQ_II if needed for Mij or Rbar updating and this
*  will produce small differences from old runs.  */

            /* Advance random-rounding seed if rounding is done */
            if (ix->upcij && (sequence == SEQ_III ||
                  sequence == SEQ_II && (ix->Cn.cnopt & NOPSJR ||
                  ix->Cn.kam & (KAMUE|KAMTS|KAMSA|KAMDR)))) {
               ix->cnflgs |= UPRSD;
#ifdef PARn
               udevskip(&ix->rseed, il->locell*ix->upcij);
#endif
               }

#ifdef PARn
            /* Advance phasing seed if phasing is on */
            if (ix->phopt & PHASR)
               udevskip(&ix->upphseed, il->locell*ix->nc);
#endif

/*---------------------------------------------------------------------*
*   Following initializations are not required on host node            *
*---------------------------------------------------------------------*/

#ifndef PAR0

/* If AUTOSCALE was requested, adjust input scale accordingly and
*  store scale actually to be used in ix->sclu.  wsmnax setting
*  moved here from d3news, 08/17/10, to coordinate with AUTOSCALE
*  and wsmxax added, 04/23/12.  */

            ix->sclu = (il->kautu & KAUT_S &&
               il != (struct CELLTYPE *)ix->psrc) ? mssle(ix->Cn.scl,
               RP->psclmul[il->oautsc], -FBsc, OVF_AUTS) : ix->Cn.scl;

            /* Calculate mnax divided by sclu (S20 - S24 + 27 = S23) so
            *  raw sums can be tested before scaling them (scl may be 0
            *  or results may exceed 32 bits, so divide carefully).  */
            if (ix->sclu) {
               ldiv_t qrm = ldiv(ix->Cn.mnax, ix->sclu);
               ix->wsmnax = jasl(jslsw(jesl(qrm.quot),27),
                  dsrswq(jesl(qrm.rem),27,ix->sclu));
               qrm = ldiv(ix->Cn.mxax, ix->sclu);
               ix->wsmxax = jasl(jslsw(jesl(qrm.quot),27),
                  dsrswq(jesl(qrm.rem),27,ix->sclu));
               }
            else                    /* JIC - avoid divide by 0 */
               ix->wsmnax = ix->wsmxax = si64zero;

            ix->cnwk.go.vmvt = 0;   /* So DPRINT prints 0 if no amp */
            if (sequence == SEQ_I) goto SKIP_CYCLE_AMP_SETUP;

/*                         (V - mtv) and sms
*
*  Note:  The first of two words stored for each value scheme is the
*  signed, unclipped value, used for amplification so that connection
*  strengths can be decreased when value is negative.  The second word
*  for each value scheme is the value clipped to the range 0 < VALUE <
*  1.0 for use as an input.  ix->cnwk.go.sms is the scale to convert
*  (Sj-wkmtj)*(si-mti)*(v-mtv)*way-scale to S16.
*/

            ix->cnwk.go.kamv = 0;   /* Amplif case at value level */
            if (ix->Cn.kam & KAMVNC) {
               si16 tval = ix->cnwk.go.vmvt =
                  (si16)RP->pvdat[ix->Cn.vi-1].fullvalue - ix->Cn.mtv;
               if      (tval <  0) ix->cnwk.go.kamv |= KAC_VLO;
               else if (tval == 0) ix->cnwk.go.kamv |= KAC_INELG;
               }
            ix->cnwk.go.sms = (ix->Cn.kam & KAMVNU) ? 28 : 20;

/*                         (V * RHO)
*
*  This variable is used for maintaining Cij0.  If Cij0 is not
*  allocated, set vrho = 0 to prevent access. */

            if (cnexists(ix,CIJ0)) {   /* Is Cij0 allocated? */
               if (ix->Cn.dvi) {       /* Yes, is value index used? */
                  ix->cnwk.go.vrho = msrsle(ix->Cn.rho,
                     RP->pvdat[ix->Cn.dvi-1].fullvalue, -FBsv, OVF_GEN);
                  }
               else ix->cnwk.go.vrho = ix->Cn.rho;
               }
            else ix->cnwk.go.vrho = 0;

/*                         mntsmxmp
*
*  In TICKAMP or ELJTAMP, this represents the maximum decrement in
*  Mij that is allowed.  When the Mij increment exceeds this value,
*  i.e. Mij is about to increase, the clock is (re)started.  */

            ix->cnwk.go.mntsmxmp = (ix->Cn.mxmij*ix->Cn.zetam) >> 10;
            if (ix->cnwk.go.mntsmxmp > ix->Cn.mxmp)
               ix->cnwk.go.mntsmxmp = ix->Cn.mxmp;

/*                         pctway
*
*  Set scale for each sign combination for requested subclass of
*  Darwin II-style amplification.  At the same time, combine the
*  new-style scales with any old-style csc shift that may have
*  been entered, so csc scaling is eliminated from the inner amp
*  loop.  (Until we have non-Hebb cases to worry about, we needn't
*  check for KAM=E:  g2amplif & d3chng make sure not more than one
*  amp rule is requested.)  Storage space for these scales is
*  allocated once-only by d3nset() according to the largest number
*  of connection types that occur for any cell type, so this space
*  must be reinitialized each time a new cycle begins.  Attempts
*  to place this space in ix->cnwk.go caused an nxdr failure,
*  which should be debugged someday.  */

            if (ix->Cn.kam & (KAM2I|KAM2J|KAM3|KAM4|KAMHB)) {
               int ik;
               char *prway = amp_case_scales[(int)ix->amptt];
               /* Expand the amplification scales to S8 values */
               for (ik=0; ik<MAX_WAYSET_CODES; ik++)
                  pctway[ik] = (short)prway[ik] << 8;
               } /* End if traditional kam case */
            else {
               /* Amp scales given explicitly by user:
               *  just copy them into the temp scales  */
               memcpy((char *)pctway, (char *)ix->Cn.wayset,
                  MAX_WAYSET_CODES*sizeof(short));
               }

            pctway[MAX_WAYSET_CODES] = 0;
            if (ix->Cn.csc) {    /* Apply old-style shift scale */
               int ik;
               for (ik=0; ik<MAX_WAYSET_CODES; ik++) {
                  if (odd_way_cases[ik] != (pctway[ik] < 0))
                     pctway[ik] = SRA(pctway[ik], ix->Cn.csc);
                  }
               } /* End csc adjustment */
            pctway += (MAX_WAYSET_CODES + 1);   /* Skipped if bypass */

SKIP_CYCLE_AMP_SETUP: ;

/* Set ix->srcorg to point to the array of source Sj.  This setting
*  is delayed until here because of cycle alternation for D1 input
*  or repertoire input.  N.B.  This setting cannot be done on host
*  node because some of these pointers point inside arrays and thus
*  cannot be translated by membcst().  */

            switch (ix->cnsrctyp) {
            case REPSRC:
               ix->srcorg = (s_type *)
                  ((struct CELLTYPE *)ix->psrc)->pps;
               break;
            case IA_SRC:
               /* If doing a replay, noise is not added, so
               *  there should be no offset to the odd columns.  */
               {  register s_type *tstim = RP->pstim;
                  if (!(RP->CP.runflags & RP_REPLAY))
                     tstim += RP->nsy;
                  ix->srcorg = tstim;
                  } /* End local scope */
               break;
            case VALSRC:
               ix->srcorg = NULL;
               break;
#ifdef D1
            case D1_SRC:
               ix->srcorg = ((struct D1BLK *)ix->psrc)->pd1s;
               break;
#endif
            case USRSRC:
               if (ix->cnxc.tvmode == Col_R4)
                  ix->srcorg = (s_type *)(RP->paw + ix->osrc);
               else
                  ix->srcorg = (s_type *)RP->pbcst + ix->osrc;
               break;

            default:
               ix->srcorg = (s_type *)RP->pbcst + ix->osrc;
               } /* End cnsrctyp switch */

/* Store working pointer to decaying afference */

            ix->cnwk.go.paeff = ix->aeff;

/* Enter contribution of this connection type into pmark array for
*  cross-response and Cdist marking.  Stimulus identification bits
*  are picked up from the sensory broadcast in the case of direct
*  inputs, and from the pps arrays in the case of inputs from other
*  repertoires.  In case of axonal delays, we OR together inputs
*  subject to all delays that might be suffered by any input to this
*  conntype.  A little time might be saved by replacing the bittst
*  calls with in-line ANDs with a shifting mask bit, but the need to
*  allow for missing modality numbers would add complexity.  */

            if (il->ctwk.go.masking && !(ix->cnflgs & NOMDLT)) {
               struct MODALITY *pmdlt;
               if (ix->cnflgs & DIMDLT) { /* Non-repertoire input */
                  pmdlt = ix->psmdlt;
                  bitior(pmark, ix->mdltmoff, RP->pbcst + RP->ombits,
                     (int)pmdlt->mdltboff, (long)pmdlt->nds);
                  } /* End direct input */
               else {                     /* Repertoire input */
                  struct CELLTYPE *jl = (struct CELLTYPE *)ix->psrc;
                  byte **ppmark = (byte **)jl->pps;
                  long jboff = jl->lspt;  /* Byte offset of mask */
                  long jlen = jl->lmm;    /* Length of mask */
                  long ioff=0, joff=0;    /* Offsets of mask bits */
                  long mbit;              /* Modality number */
                  int idly;               /* Delay loop index */
                  mdlt_type comdlts = il->rmdlts & jl->rmdlts;

                  /* Collect stimulus marks for all relevant delays */
                  switch (ix->Cn.kdelay) {
                  case DLY_NONE:
                     tmark = ppmark[0] + jboff;
                     break;
                  case DLY_CONST:
                     tmark = ppmark[ix->Cn.mxcdelay] + jboff;
                     break;
                  case DLY_UNIF:
                  case DLY_NORM:
                     tmark = CDAT.tmark0;
                     memcpy((char *)tmark,
                        (char *)ppmark[0] + jboff, (size_t)jlen);
                     for (idly=1; idly<=(int)ix->Cn.mxcdelay; idly++)
                        bytior(tmark, jlen, ppmark[idly]+jboff);
                     break;
                     } /* End delay type switch */

                  /* Now transfer bits from all common modalities */
                  for (pmdlt=RP->pmdlt1; pmdlt; pmdlt=pmdlt->pnmdlt) {
                     mbit = pmdlt->mdltno;
                     if (bittst((unsigned char *)&comdlts, mbit))
                        bitior(pmark, ioff, tmark, joff,
                           (long)pmdlt->nds);
                     if (bittst((unsigned char *)&il->rmdlts, mbit))
                        ioff += pmdlt->nds;
                     if (bittst((unsigned char *)&jl->rmdlts, mbit))
                        joff += pmdlt->nds;
                     } /* End modality loop */

                  } /* End repertoire input */
               } /* End collecting contribution to mask array */

#endif   /* not defined PAR0 */
            }  /* End of loop over connection types */

#ifndef PAR0
         /* Now that pmark has been initialized, can calc delmk.
         *  N.B.  Modality bits in delmk scan from right to left. */
         if (statenbm && il->Ct.kctp & KRPHR) {
            struct MODALITY *pmdlt;
            mdlt_type imnum = 1; /* Relative modality number */
            int ioff1 = 0,ioff2; /* Zero-base marking bit offsets */
            int ibyt1,ibyt2;     /* Byte offsets to marking bits  */
            register int i;      /* Byte index */
            register int qnew;   /* Nonzero if stimulus is new */
            byte bmask;          /* Byte mask  */

            il->ctwk.go.delmk = 0;
            memcpy((char *)CDAT.tmark0, (char *)pmark, (size_t)il->lmm);
            bytxor(CDAT.tmark0, (long)il->lmm, il->pps[0] + il->lspt);
            for (pmdlt=RP->pmdlt1; pmdlt; pmdlt=pmdlt->pnmdlt) {
               if (!bittst((unsigned char *)&il->rmdlts,
                  (long)pmdlt->mdltno)) continue;
               qnew = 0;
               /* Isolate bits for this modality */
               ioff2 = ioff1 + pmdlt->nds - 1;
#if BITSPERBYTE != 8
#error Rewrite code for odd byte size
#endif
               ibyt1 = ioff1 >> 3;
               ibyt2 = ioff2 >> 3;
               for (i=ibyt1; i<=ibyt2; i++) {
                  bmask = 0xff;
                  /* N.B.  Both tests on i can be true */
                  if (i == ibyt1) bmask &=
                     (byte)(0x00ffL >> (ioff1 & (BITSPERBYTE-1)));
                  if (i == ibyt2) bmask &=
                     (byte)(0x7f80L >> (ioff2 & (BITSPERBYTE-1)));
                  qnew |= CDAT.tmark0[i] & bmask;
                  }
               if (qnew) il->ctwk.go.delmk |= imnum;
               ioff1 = ioff2 + 1;
               imnum <<= 1;
               }
            }
#endif

/*---------------------------------------------------------------------*
*                                                                      *
*                           Loop over cells                            *
*                                                                      *
*---------------------------------------------------------------------*/

#ifdef PAR0
         /* Node zero has no cells, so detail print routine must
         *  be invoked outside cell loop (once per layer). */
         if (do_dprint) d3pdpz();
         if (statenbc) il->ctf |= DIDSTATS;  /* Trigger stats */

#else  /*** BEGIN NODE ZERO EXCLUSION FROM LOOP OVER RESIDENT CELLS ***/

         /* Clear for dprnt items that may not be set at all */
         CDAT.dst = CDAT.sbar = CDAT.qbar = 0;
         /* Initialize test for lowest high response */
         if (statenbc && il->Ct.kctp & (KRPGP|KRPMG)) {
            pgp0 = CDAT.pgstat + il->oGstat;
            llohi = -SI32_MAX;
            }

         celllim = il->locell + il->mcells;
         coff = 0;
         for (icell=il->locell; icell<celllim; ++icell,coff+=il->lel) {
            {  long relcell = icell - il->locell;
               psnew = il->ps2 + spsize(relcell, il->phshft); }
            plsd = il->prd + coff;

            /* Initialize CDAT structure, clear accumulators */
            CDAT.cdcell = icell;
            CDAT.noise  = 0;     /* Clear noise */
            CDAT.events = 0;     /* Say nothing happened yet */
            memset((char *)CDAT.pSpecSums, 0, CDAT.lSpecSums);
            memset((char *)CDAT.pTotSums, 0, CDAT.lTotSums);
            hitpt = hitnt = sovrinc = 0;

            /* Locate consolidation tables.
            *  Indicate normal sums not yet done for VDT */
            pcontbl = CDAT.ConsTables;
            didnorms = FALSE;

            /* Prepare for Lij acquisition and
            *  establish group-level variables */
            d3kiji(il, icell);
            if (Lija.new_group) {
               CDAT.groupn = Lija.groupn;
               CDAT.groupx = Lija.groupx;
               CDAT.groupy = Lija.groupy;

               /* Compute group-level input from all GCONNS */
               if (il->pib1) d3inhg();
               } /* End new group actions */

            /* Generate local rounding constants from rsd.  Seed rsd
            *  is deterministically updated four times per cell per
            *  cyle.  Currently three of these are used, to round sbar
            *  & qbar, dst, and depression.  The fourth one is a spare.
            *  Code must assure deterministic results regardless of
            *  which options using rsd may be switched on or off.  */
            rnd1 = udev(&il->rsd);
            rnd2 = udev(&il->rsd);
            rnd3 = udev(&il->rsd);
            udev(&il->rsd);   /* Spare */

            /* Pick up sbar, qbar, si, and phi.  Pick up phase
            *  even if it doesn't exist, adjusting offset to avoid
            *  exceeding array bounds--probably faster than an 'if'
            *  statement and there is no code that assumes it has any
            *  particular value if it is not actually used.
            *  N.B.  The term simavgn previously used in the calc
            *  of persistence used avgnoise on an incorrect scale
            *  (effectively divided by S16). In V8B, subtract the
            *  scaled resting potential instead.  In V8C, zero si
            *  IS resting potential, so subtraction is needed only
            *  in the COMPAT=C case.  */

            if (ctexists(il,SBAR)) {
               rd_type *psbr = plsd + il->ctoff[SBAR];
               d3gts2(CDAT.sbar, psbr);
               }
            if (ctexists(il,QBAR)) {
               rd_type *psbr = plsd + il->ctoff[QBAR];
               d3gts2(CDAT.qbar, psbr);
               }
            {  s_type *psi = il->pps[0] + spsize(icell,il->phshft);
               d3gts2(CDAT.old_siS7, psi);
               /* Scale s(i) to S20/27 for most working */
               CDAT.old_si = oldsi = CDAT.old_siS7<<(FBwk-FBsi);
               CDAT.old_phi = psi[1+il->phshft];
               } /* End psi local scope */

            /* Calculate self-input */
            if (il->sisclu) {
               si32 simet = oldsi - il->Dc.siet;   /* (mV S20) */
               if (simet > 0) {
                  /* S20 + S24 -24 = S20 */
                  simet = msrsle(simet, il->sisclu, -FBsc, OVF_RFN);
                  if (il->phshft) {          /* Cells have phase */
                     register int i,p;
                     if (ppd->pselfpcf) {    /* Convolution method */
                        long *pkern = ppd->pselfpcf->bins;
                        si32 *psi[NEXIN], tsi;
                        psi[EXCIT] = CDAT.pAffData + il->ossei[EXCIT];
                        psi[INHIB] = CDAT.pAffData + il->ossei[INHIB];
                        for (p=0; p<PHASE_RES; p++) {
                           i = (p-(int)CDAT.old_phi) & PHASE_MASK;
                           /* S20 + S28 - 28 = S20 */
                           tsi =
                              msrsle(simet, pkern[i], -FBkn, OVF_RFN);
                           psi[(ui32)tsi >> (BITSPERUI32-1)][p] = tsi;
                           }
                        } /* End self-input convolution */
                     else {                  /* Uniform method */
                        si32 *psi = CDAT.pAffData +
                           il->ossei[(ui32)simet >> (BITSPERUI32-1)];
                        for (p=0; p<PHASE_RES; p++) psi[p] = simet;
                        }
                     } /* End phased self-input */
                  else {                     /* Cells do not have phase */
                     CDAT.pAffData[il->ossei[(ui32)simet >>
                        (BITSPERUI32-1)]] = simet;
                     } /* End unphased self-input */
                  } /* End if simet > 0 */
               } /* End self-input */

            /* If old_si will be included in NormSums for VDTs,
            *  save scaled normpers now, is common to all CONNTYPEs */
            normpers = (il->ctf & INCLPERS) ?
               msrsle(il->Dc.CDCY.omega, oldsi, -FBod, OVF_VDEP) : 0;

            /* Refractory period control:  Pick up current dst and
            *  use it to adjust st.  dst < 0 is refrac timer.  */
            wkst = il->Ct.st;          /* mV S20 */
            if (prf) {
               rd_type *pdst = plsd + il->ctoff[DST];
               d3gth2(wkdst, pdst);    /* Fetch, retaining sign */
               if (wkdst > 0) {
                  wkdst >>= 3;         /* S7+16-3 --> S20 */
                  wkst += wkdst;
                  }
               CDAT.dst = wkdst;       /* For printing and stats */
               }
            else wkdst = 0;

            /* Calculate GCONNS (geometric connections, lateral
            *  inhibition), now may be used in NORMSUMS for VDTs */
            if (il->ctf & CTFGV) d3inhc();

/*---------------------------------------------------------------------*
*                     Loop over connection types                       *
*  Use alternative threading of the CONNTYPE blocks that assures all   *
*  normal connection types are processed before all voltage-dep types. *
*---------------------------------------------------------------------*/

            pctway = CDAT.pglway;   /* Locate global amp scales */
            pdelc  = CDAT.pdelcij;  /* Place to save delta Cij */
            pizv  = plsd + il->ctoff[IZVU];  /*  Harmless if  */
            pizu  = pizv + I32SIZE;          /* not IZHI,BREG */
            for (ix=il->pctt1; ix; ix=ix->pctt) {

/*---------------------------------------------------------------------*
*     N.B. Four levels of indenting suppressed to end of conn loop.    *
*---------------------------------------------------------------------*/

   struct CONNDATA *cijavg;/* Work space for cij averaging */
   si64 *pds0;             /* Ptr to start of detailed amp stats */
   si64 *raxp,*raxn;       /* Ptrs to pos/neg raw conntype sums */
   si64 *saxp,*saxn;       /* Ptrs to subarb & processed sums */
   long *pexSums;          /* Ptr to excitatory sums */
   long *pinSums;          /* Ptr to inhibitory sums */
   size_t lpaxcb;          /* Bytes in pax[pn] data after raw */
   size_t lraxb;           /* Bytes in nexin*lrax si64's */
   size_t lrax1b;          /* Bytes in nexin*lrax1 si64's */
   size_t lsaxb;           /* Bytes in nexin*lsax si64's */
   long Aij;               /* Cij * (Sj - sjrev) */
   long Cijppf;            /* Cij adjusted for PPF */
   long delCij;            /* Change in Cij (S16) */
   long Lij;               /* Connection source index */
   long Mij;               /* Modifying substance (S7/14) */
   long Mijincr,Mijdecr;   /* Mij increment and decrement */
   long Mijtest;           /* Test for Mij a clock */
   long lepet,lenet;       /* Local copies of effet,effnt */
   long lspet,lsnet;       /* Local copies of subet,subnt */
   long simmti,sjmmtj;     /* Values of (si - mti), (sj - mtj) */
   long mjmmtj;            /* (Mij-mtj) as used for amp (mV S7)  */
   long Sj;                /* Sj (mV S7) - sjrev */
   long Sjtest;            /* Sj adjusted to fail et test */
   long SjCijdst;          /* Cij distribution Sjtest value */
   long wSj;               /* (Sj - et) or (Sj - etlo) for Aij */
   long wkmtj;             /* Working mtj or (RBAR - sjrev) */
   long wksi;              /* Working si or sbar */
   ui32 lkam;              /* Local copy of ix->Cn.kam */
   int  gotex,gotin;       /* cssck switches for excit,inhib */
   int  gotsj;             /* TRUE if Sj exists */
   int  kami,kamj,kams;    /* Amplif, stats cases--this connection */
   int  lpax64;            /* Wides in raxp after scaling */
   int  lraxw;             /* Local copy of lrax (wides in raxp etc.) */
   int  lrax1w;            /* Local copy of lrax1 (wides in raw sums) */
   int  lrax0;             /* Offset of first saved rax[pn] entry */
   int  lrax12w;           /* Wides in raxp+raxn in synapse loop */
   int  lsax12w;           /* Wides in saxp+saxn */
   int  lsaxw;             /* Local copy of lsax (number of wides) */
   int  nexin;             /* Number of signs of input */
   int  Pj;                /* Phase of Sj */
   int  zbict;             /* Zero-based ict */
   si16 wkmnsj;            /* Working mnsj test--SHRT_MAX if
                           *  mnsi test failed */
   /* For Seq I the value of amp_switch is not relevant.
   *  For Seq II it should be NO_AMP.  For Seq III it
   *  should be the requested amp scheme.  */
   char amp_switch;        /* Amplification flow control */
   /* If there is decay, decay_switch picks out the decay code
   *  to execute (Seq III).  If there is just DAS, it skips to
   *  the DAS code (Seq II).  If there is no decay, amp, or DAS
   *  it skips over the decay, amp, and DAS code (Seq I).  */
   char decay_switch;      /* Decay flow control */
   /* For Seq I and Seq II the value of finamp_switch is
   *  NO_FIN_AMP.  For Seq III it should be the finish
   *  routine for the requested amplification scheme.  */
   char finamp_switch;     /* Final action flow control */
   char detstats;          /* TRUE for detailed amp stats */
   char doampavg;          /* TRUE if amplif and avg Cij */

/* Exit now if bypassing this conntype */

   if (ix->kgfl & KGNBP) continue;

/* Copy updated phasing seed to current seed (even if never
*  used) and perform remaining conntype initialization */

   /* Initialize Lij,Cij generation or look-up */
   d3kijx(ix);
   d3lijx(ix);
   d3cijx(ix);
   if (ix->Cn.kdelay) d3dijx(ix);
   ix->phseed = ix->upphseed;
   d3sjx(ix);

   /* Pick up local copies of kam and effective thresholds */
   lkam = ix->Cn.kam;
   lepet = ix->effet, lenet = ix->effnt;
   lspet = ix->subet, lsnet = ix->subnt;

   /* Need conntype number to index items that
   *  must be accessed in non-threaded order */
   zbict = (int)ix->ict - 1;

   /* Access working space for cij averaging */
   cijavg = (struct CONNDATA *)CDAT.psws;

/* Set pointers to storage for raw and subarbor or convolution Aij
*  data sums and their lengths.  pax[pn] hold raw sums, persistence,
*  and final sums across conntypes for detail print.  rax[pn] holds
*  raw sums and deferred convolution temps.  sax[pn] holds subarbor
*  and final sums.  rax[pn] and sax[pn] are reused per conntype.
*  N.B.  pax[pn] have room for (1) 32 entries if PHASJ|PHASR, else
*        1, (2) 1 entry if decay, (3) 32 entries if PHASJ|PHASR or
*        convolution, else 1.  lpax, except decay part, which can
*        change at Group III time, is set in d3tchk.
*  N.B.  Unphased sums are always at rax[pn][0], followed if PHASJ
*        or PHASR by phased sums at rax[pn][1-PHASE_RES].
*  N.B.  lpax64, lrax, lrax1, and lsax are set in d3tchk().  Each is
*        1 if no phase.  With phases, lpax64 is PHASE_RES to hold
*        just scaled afferents.  lrax1 is (PHASE_RES+1) to store
*        phased and unphased raw sums in synapse loop.  lrax is same
*        but also (PHASE_RES+1) to allow PHASDCNV scratch.  lsax is
*        PHASE_RES with any phase code, +1 if subarbors, so it can
*        be used for convolutions and to hold the phased result for
*        input to cell sums.
*  N.B. sax[pn] can be used for subarbors or convolutions, not both.
*        Early convolution is not allowed with subarbors--it would
*        make mnax test very difficult.
*/

   {  int lcssck = (int)ix->cssck;
      gotex = lcssck & PSP_POS;
      gotin = lcssck & PSP_NEG;
      nexin = (int)npsps(lcssck);   /* 1 for EXCIT + 1 for INHIB */
      } /* End lcssck local scope */
   lpax64 = (int)ix->lpax64;
   lraxw  = (int)ix->lrax;
   lrax1w = (int)ix->lrax1;
   lrax12w = nexin * lrax1w;        /* Terms in raw sums */
   lrax0  = (int)(lrax1w >> PHASE_EXP);   /* 0 or 1 */
   lsaxw  = (int)ix->lsax;
   lsax12w = nexin * lsaxw;         /* Terms in final sums */
#if WSIZE == 8
   lraxb  = nexin * lraxw << 3;   /* Size of raxp+raxn in bytes */
   lrax1b = nexin * lrax1w << 3;  /* Portion used in raw sums */
   lsaxb  = nexin * lsaxw << 3;   /* Size of saxp+saxn in bytes */
#else
   lraxb  = nexin*sizeof(si64)*lraxw;
   lrax1b = nexin*sizeof(si64)*lrax1w;
   lsaxb  = nexin*sizeof(si64)*lsaxw;
#endif
   lpaxcb = sizeof(long) * ((ix->Cn.ADCY.kdcy != NODECAY) + (ix->phopt &
      (PHASJ|PHASR|PHASECNV|PHASDCNV|PHASVDU) ? PHASE_RES : 1));

   /* Locate space for si64 working sums, expanded for subarbors and
   *  various phase options, arranged so where possible one or both
   *  of excit and inhib can be handled in single loops of length
   *  lrax12w wides.  However, offset of saxp from paxp must be based
   *  on lrax, not lrax1, so it will not be overwritten in case of
   *  deferred convolutions.  */
   raxp = prbar + il->nct;
   saxp = raxp + nexin * lraxw;
   if (nexin > 1)
      raxn = raxp + lrax1w, saxn = saxp + lsaxw;
   else
      raxn = raxp, saxn = saxp;

   /* Locate arrays for individual type detail */
   paxp = CDAT.pAffData + ix->oexafs;
   paxn = CDAT.pAffData + ix->oinafs;

   /* Locate arrays for summation results */
   pexSums = CDAT.pAffData + ix->oasei[EXCIT];
   pinSums = CDAT.pAffData + ix->oasei[INHIB];

   /* Locate PPF parameters, if any */
   ppfd = ix->PP;

   /* Constant for determining whether Mij is a clock */
   Mijtest = ix->Cn.mticks - S15;

/* Process hand vision--
*  It is necessary to calculate Sj only if this is the
*  first cell of this type on this node, or if a new group
*  has just been entered.  Otherwise, stored value is used.
*  Cell grp coords -> environ coords for distance calc.
*  Scaling:  1.0 corresponds to 128 mV (S7) --> 2^14.
*  This is like a virtual group, so KGNXR is set in d3tchk()
*  Rev, 05/29/09, GNR - Add NOPRS option -- just a test --
*     scales radius according to distance from center  */

   if ((ix->kgen & KGNHV) && (Lija.new_group)) {
      float hr;
      float dx = ix->ul1.w.xwa -
         ix->ul1.w.xrat*((float)CDAT.groupx + 0.5);
      float dy = ix->ul1.w.ywa -
         ix->ul1.w.yrat*((float)CDAT.groupy + 0.5);
      float effrm2 = ix->ul1.w.radm2;
      if (ix->Cn.cnopt & NOPRS) {
         float xwc = ix->ul1.w.xwa/ix->ul1.w.xwmx - 0.5;
         float ywc = ix->ul1.w.ywa/ix->ul1.w.ywmx - 0.5;
         hr = 0.5 + sqrt(xwc*xwc + ywc*ywc);
         effrm2 /= (hr*hr);
         }
      hr = 1.0 - effrm2*(dx*dx + dy*dy);
      ix->ul1.w.sj = (hr > 0.0) ? (long)(dS14*hr) : 0;
      }

/*---------------------------------------------------------------------*
*  Prepare decay, amplification, and amplification stats switches      *
*  for this connection type.  Calculate those amplification variables  *
*  that depend only on the cell, not the specific connection.          *
*---------------------------------------------------------------------*/

   finamp_switch = ix->finbnow;
   detstats = (lkam & KAMDS) ? statenbc : FALSE;
   if (sequence == SEQ_I) {
      decay_switch = SKIP_ALL;
      amp_switch = NO_AMP;
      }

   else {
      long savg;
      decay_switch = (sequence == SEQ_II) ? NO_DECAY : ix->dcybi;
      amp_switch = ix->ampbi;

/* Calculate hacell and kami for amplification.  KAMDQ option
*  revised, 08/24/94, to use average of sbar and mti and revised
*  again 12/26/07 to use average of qbar and mti.  Original version
*  was not preserved, was used only in tests.  Note that we pretend
*  Si and Sj are S14 (i.e. 128mV = 1.0 old-scale). Removed rounding
*  of hacell in V8C--more accurate without it.  wkmnsj is mnsj test,
*  but set to SHRT_MAX to kill this cell at m(ij) update time if
*  mnsi test fails here (KAMUE only).  */

      wkmnsj = ix->Cn.mnsj;
      kami = (int)ix->cnwk.go.kamv;
      wksi = (lkam & (KAMSA|KAMZS|KAMTNU)) ? CDAT.sbar : CDAT.old_siS7;
      simmti = (lkam & KAMKI) ? S14 : (wksi - ((lkam & KAMDQ) ?
         (savg = CDAT.qbar + ix->Cn.mti, SRA(savg,1)) : ix->Cn.mti));
      /* '<=' tests kill amp if term is 0 and using default 0 test */
      if (wksi < ix->Cn.mnsi) {
         if (lkam & KAMUE) wkmnsj = SHRT_MAX;
         else              kami |= KAC_INELG;
         }
      if ((hacell = simmti) < 0) kami |= KAC_SILO;
      else if (simmti == 0)      kami |= KAC_INELG;
      /* Set value if group number matches category */
      if (lkam & KAMCG) {
         struct MODALITY *pm = ix->psmdlt;
         si16 tval = ix->cnwk.go.vmvt =
            ((si16)bittst(RP->pbcst + RP->ombits,
            (long)pm->mdltgoff + CDAT.groupn%pm->ncs + 1) << FBsv) -
            ix->Cn.mtv;
            if      (tval <  0) kami |= KAC_VLO;
            else if (tval == 0) kami |= KAC_INELG;
         }
      if ((lkam & KAMVNU) && !(kami & KAC_INELG))
         /* Calc hacell = (si-mti)*(v-mtv) (S22) */
         hacell *= (long)ix->cnwk.go.vmvt;
      } /* End else (not SEQ_I) */

/* N.B.  As written here, this code should give same results on
*  parallel and serial computers, but these results will differ
*  from those obtained with the classical IBM serial version,
*  which incremented seeds only as required in the inner loop.
*  It is not easily possible to replicate these old results on
*  a parallel computer, but to get them on a serial computer,
*  just omit the lines that update upphseed and psvrseed when
*  a connection is skipped.   --12/10/93, GNR
*  Rev, 01/23/07, GNR
*/

   /* Save rseed for deterministic updating */
   ix->cnwk.go.psvrseed = ix->rseed;

   doampavg = (finamp_switch != NO_FIN_AMP) && (ix->Cn.saopt & SAOPTA);
   if (doampavg) {            /* Clear Cij accumulators */
      memset((char *)cijavg, 0, sizeof(struct CONNDATA)*ix->nsa);
      SjCijdst = LONG_MAX; }
   else {                     /* Force count = 1 for stats */
      cijavg->narb = 1;
      SjCijdst = statenbc ? -LONG_MAX : LONG_MAX; }

/*---------------------------------------------------------------------*
*                         Loop over synapses                           *
*---------------------------------------------------------------------*/

/* Clear rax[pn], sax[pn], and rbar totals.  If we want to consolidate
*  clearing of raxp,saxp, then must reallocate saxp later when length
*  of raxp might expand from lrax1b to lraxb.  */

   memset((char *)raxp, 0, lrax1b);
   memset((char *)saxp, 0, lsaxb);
   prbar[zbict] = si64zero;

/* This loop is driven by Lija.isyn so that in cases where Lij's
*  are being calculated, the loop counter is available to the Lij
*  routines.  For the same reason, the conntype number, subarbor
*  number, etc. are also maintained in Lija.  */

   for (Lija.isyn=0;Lija.isyn<Lija.nuk;++Lija.isyn,Lija.psyn+=ix->lc) {

/* Fetch or generate an Lij coefficient and break out if no
*  more connections will be generated for this cell.  */

      if (!Lija.lijbr(ix)) break;
      Lij = Lija.lijval;

/* If starting a new subarbor other than the first, check |net input|
*  against descaled threshold.  If above threshold, accumulate into
*  sax[pn].  This leaves the last contribution in rax[pn] to be
*  checked and summed at the end of the synapse loop, which must
*  be done anyway to handle sudden exits when nuk < nc.  See notes
*  at end of loop re: scaling and overflow checking.
*  N.B.  The present mnax test spec allows sums for excit & inhib
*  conns to be consolidated in a single loop over lrax12w terms.
*  One might instead want separate tests on the excit & inhib sums.
*  This would require separate PSP_POS & PSP_NEG loops.  */

      if (Lija.jsyn > 0 && Lija.jsyn == Lija.isas) {
         register int p;
         si64 tsum = jasw(raxp[0],raxn[0]);
         tsum = jrsw(jabs(tsum),ix->wsmnax);
         if (qsw(tsum) >= 0) for (p=0; p<lrax12w; p++)
            saxp[p] = jasw(saxp[p],raxp[p]);
         memset((char *)raxp, 0, lrax1b);
         } /* End first or new arbor */

/* Check Lij for out-of-bounds (input from IA only) and fetch Sj.
*  It is also necessary to fetch phase, whether or not needed, to
*  keep seed updated in case it is PHASR and connection is skipped.
*
*  Maintain running average Sj which may be used either to reduce Sj
*  to its deviation from average (OPT=I) or as substitute for mtj in
*  amplification (KAM=R).  The running average is random-rounded
*  using bits from rseed that are not used to round Cij or Mij.
*  There is no check for overflow because rdamp is checked at
*  input time and Sj is itself limited to two bytes.  The total
*  is accumulated in prbar[zbict] for use in statistics, whether
*  needed or not to save 'if' (statistics are almost always left
*  turned on, so it's better to save them every time than to test).
*
*  Once RBAR has been updated, Sj is set to Sj - sjrev for all
*  following calculations.  Working value of mtj is either mtj
*  from input or <Sj> (RBAR) - sjrev.  (Technically mtj is only
*  needed for detstats or amplif, but it's probably quicker to
*  calculate it every time than to test for these conditions.)
*
*  Rev, 09/29/07, GNR - If out-of-bounds (sjbr returns FALSE), re-
*     vised to treat as if Sj = 0.  Formerly, this code skipped to
*     next connection, omitting stuff like ppft update, Cij decay,
*     etc. and leaving incorrect count in nuk for later stats.
*  Rev, 05/07/09, GNR - Added gotsj for cases where setting Sjtest
*     to -LONG_MAX does not catch cases where Sj < etlo.
*  Rev, 03/30/12, GNR - Add Sj histo statistic before any manip.
*
*/

      gotsj = ix->sjbr(ix);
      if (gotsj) {
         Sj = Lija.sjval;
         if (statenbc) {
            ddist *pdd = il->pctddst + ix->ocnddst + OaSd;
            int ibin = 8 + SRA(Sj,FBsi+Sr2mV-3);
            if (ibin >= LDSTAT) ibin = LDSTAT-1;
            else if (ibin < 0)  ibin = 0;
            pdd->d[ibin] = jasl(pdd->d[ibin], 1);
            }
         if (cnexists(ix, RBAR)) {
            rd_type *prbr = Lija.psyn + ix->cnoff[RBAR];
            if (ix->cnflgs & CNDOFS)
               wkmtj = Sj;
            else {
               d3gts2(wkmtj, prbr);
               wkmtj = wkmtj*(long)ix->cnwk.go.rdampc +
                  Sj*(long)ix->Cn.rdamp + (ix->rseed & 0x00007fc0);
               wkmtj = SRA(wkmtj, FBdf); }
            d3ptl2(wkmtj, prbr);
            prbar[zbict] = jasl(prbar[zbict], wkmtj);
            if (ix->Cn.cnopt & NOPSJR) Sj -= wkmtj;
            wkmtj -= ix->Cn.sjrev;
            }
         if (!(lkam & KAMDR)) wkmtj = ix->Cn.mtj;
         Sjtest = Sj -= ix->Cn.sjrev;
         }
      else {
         /* Setting Sjtest = -INF here avoids need for gotsj
         *  tests when there is not a hi,lo threshold pair.  */
         Sj = 0, Sjtest = -LONG_MAX;
         wkmtj = ix->Cn.mtj;
         }
      Pj = ix->pjbr(ix);

/* Locate appropriate CONNDATA block for Cij processing.
*  Using these blocks for averaging assures that info on
*  old Cij and old Cij0 is available when cijclean gets
*  called after the first pass through the connections.  */

      ND = cijavg;
      if (doampavg) ND += Lija.jsyn - Lija.isas;

/* Pick up or generate Cij (S31), then shift to (S16).
*  Save old value as new to simplify stats if no change.
*  The fundamental reason shifting is necessary is to detect
*  overflow/underflow in a higher-level language when Cij is
*  decayed or amplified (added to).  */

      ix->cijbr(ix);
      ND->new_cij = ND->old_cij = Lija.cijsgn;
      Cijppf = ND->curr_cij = SRA(Lija.cijval, 15);

/* Update PPF based on Sj(t).
*  N.B.  ppfij and ppflim are r' = r - 1 so baseline is 0.
*  PPF and PPFT may be signed, so use care fetching them.
*  PPFT is negative while PPF is rising, positive while decaying.  */

      if (ppfd) {
         long ppfij, ppftij;
         byte *psppf  = Lija.psyn + ix->cnoff[PPF];  /* Ptrs to PPF, */
         byte *psppft = Lija.psyn + ix->cnoff[PPFT]; /* PPF clock */
         d3gts2(ppfij,  psppf);              /* Pick up PPF */
         d3gts2(ppftij, psppft);             /* Pick up PPF clock */
         if (ppfd->ppfopt & PPFABR) {        /* ABRUPT PPF */
            if (Sjtest > ppfd->ppft) {
               /* Got a new hit, always count it */
               ++ix->CNST.nppfh;
               /* If PPF is on, reset to full decay time */
               if (ppftij > 0)
                  ppftij = ppfd->htdn + ppfd->htup;
               /* If idling, start rise-time clock */
               else if (ppftij == 0)
                  ppftij = -(ppfd->htup);
               /* If already rising, ignore hit, keep counting.
               *  Since htup is S1, count two half times/cycle.  */
               else if ((ppftij += 2) >= 0) {
                  /* Half-rise time is up: set to begin decay */
                  ppfij = ppfd->ppflim;
                  ppftij = ppfd->htdn + ppfd->htup; }
               } /* End servicing a new hit */
            /* Rising from a previous hit, just keep counting.  (This
            *  is just one of those times code must be repeated...)  */
            else if (ppftij < 0) {
               if ((ppftij += 2) >= 0) {
                  /* Half-rise time is up: set to begin decay */
                  ppfij = ppfd->ppflim;
                  ppftij = ppfd->htdn + ppfd->htup; }
               }
            /* Otherwise, if decaying, count down the decay clock */
            else if (ppftij > 0 && (ppftij -= 2) <= 0) {
               /* Half-decay time is up: reset to idle state */
               ppfij = 0;
               ppftij = 0;
               }
            } /* End ABRUPT PPF */
         else {                                 /* Normal PPF */
            if (Sjtest > ppfd->ppft) {
               /* Above threshold, restart the clock.  Since htup
               *  is on scale S1, we can get the full time by
               *  adding one unit per cycle to the timer.  */
               ppftij = -(ppfd->htup);
               ++ix->CNST.nppfh; }
            else if (ppftij < 0) {
               /* Increasing after a hit.  When time is up,
               *  reset clock to count down two half decay times */
               if (++ppftij == 0) ppftij = ppfd->htdn;
               ppfij += msrsle(ppfd->upsp, ppfd->ppflim-ppfij,
                  -28, OVF_RFN);
               }
            else if (ppftij > 0) {
               /* In the declining phase */
               if (--ppftij == 0) ppfij = 0;
               else ppfij -= msrsle(ppfd->taup, ppfij, -28, OVF_RFN);
               }
            } /* End normal PPF */
         Cijppf *= (S12 + ppfij);
         Cijppf = SRA(Cijppf,12);
         d3ptl2(ppfij,  psppf);
         d3ptl2(ppftij, psppft);
         } /* End PPF update */

/* Accumulate Cij*(Sj-sjrev) (S23) if et,etlo threshold tests pass.
*  These sums are used for the subarbor mnax checks because they are
*  not affected by phase convolutions.  Aij and sums cannot overflow:
*  the two factors are each known to have 16 or fewer bits.  N.B.
*  Values of Sjtest and Sj are arranged to fail these tests if sjbr()
*  returned no Sj value.  */

/* Note added, 08/01/08 and 08/05/08, GNR:  Prior to Rev. 6, Sjtest
*  remained set from Sj(raw), not (Sj - Sjbar - Sjrev) and Sjtest was
*  tested against et but labs(Sj) against mabrsj.  Subtraction was
*  always STEP type.  KNEE type can be restored by use of COMPAT=K.
*  Subtraction is same as present if Sjrev = 0 and Sjbar not used.
*  Rev, 05/07/09, GNR - COMPAT=E and mabrsj eliminated.
*/

      Aij = 0;

      if (!gotsj) goto OmitAijSum;
      if      (Sj > lepet) wSj = Sj - lspet;
      else if (Sj < lenet) wSj = Sj - lsnet;
      else goto OmitAijSum;

      Aij = Cijppf*wSj;    /* wSj = sjraw - sjrev - (et or etlo) */
      /* Sum Aij into raxp,raxn according to sign */
      if (Aij >= 0)
         raxp[0] = jasl(raxp[0], Aij);
      else
         raxn[0] = jasl(raxn[0], Aij);

/* If cells have phase, accumulate the extrinsic inputs separately
*  by phase bin.  In V8A, this code was changed to provide temporary
*  summation arrays in pax[pn], thus allowing convolutions to be
*  deferred until all connections of this type have been processed.
*  This is always done if connections are voltage-dependent or have
*  subarbors, otherwise only when nc > PHASE_RES, when it will save
*  time.
*  In V8C, these sums were changed to si64 and moved to rax[pn].
*
*  1) If Si has no phase, phase of Sj is ignored (ix->phopt==0).
*  2) If PHASU or C, can construct dist later based on rax[pn][0]
*     (d3tchk() will force PHASU if input not phased and PHASR or
*     PHASC not specified), so ignore incoming phase as in 1).
*  3) If PHASR, pjbr() will assign a random phase.
*  4) Otherwise, store input according to its assigned phase.  */

      if (ix->phopt & (PHASJ|PHASR)) {
         if (Aij >= 0) {                  /* Input is excitatory */
            raxp[Pj+1] = jasl(raxp[Pj+1], Aij);
            if (ix->phopt & PHASECNV) {   /* Apply convolution now */
               long *pkern = ix->Cn.ppcf->bins;
               register int i,p;
               for (p=0; p<PHASE_RES; p++) {
                  i = (p - Pj) & PHASE_MASK;
                  /* S20 + S28 - 28 leaves S20 result */
                  saxp[p] = amrssw(saxp[p], Aij, pkern[i], FBkn);
                  }
               } /* End convolution */
            } /* End excitatory input */
         else {                           /* Input is inhibitory */
            raxn[Pj+1] = jasl(raxn[Pj+1], Aij);
            if (ix->phopt & PHASECNV) {   /* Apply convolution now */
               long *pkern = ix->Cn.ppcf->bins;
               register int i,p;
               for (p=0; p<PHASE_RES; p++) {
                  i = (p - Pj) & PHASE_MASK;
                  saxn[p] = amrssw(saxn[p], Aij, pkern[i], FBkn);
                  }
               } /* End convolution */
            } /* End inhibitory input */
         } /* End phasing */
OmitAijSum: ;

/***********************************************************************
*                                                                      *
*            Section 3.  Handle Cij decay and amplification            *
*                                                                      *
*     Execute appropriate coefficient decay and amplification code     *
*                                                                      *
*     Sequence   I: Skip decay, amplification, DAS and amplification   *
*                   completion.                                        *
*                                                                      *
*     Sequence  II: DAS only.                                          *
*                                                                      *
*     Sequence III: Do decay, amplification, DAS and amplification     *
*                   completion.                                        *
*                                                                      *
***********************************************************************/

/*---------------------------------------------------------------------*
*                          Perform Cij Decay                           *
*---------------------------------------------------------------------*/

      switch (decay_switch) {

case NO_DECAY: /* Skip decay, but go on to amplification and DAS.
               *  (occurs in Sequence II or Sequence III).  */
         break;

case DECAYE:   /* Simple exponential decay */
         ND->curr_cij +=
            msrsle(ix->Cn.target-ND->curr_cij, ix->Cn.gamma,
               -FBsc, OVF_DCY);
         break;

case DECAYB:   /* Exponential decay to baseline Cij */
         d3gthn(ND->old_cij0, Lija.psyn+ix->cnoff[CIJ0], ix->cijlen);
         /* Isolate nbc bits and check for special -0 case */
         if ((ND->old_cij0 &= ix->cmsk) == MINUS0) ND->old_cij0 = 0;
         ND->curr_cij0 = SRA(ND->old_cij0, 15);
         ND->curr_cij +=
            msrsle(ND->curr_cij0-ND->curr_cij, ix->Cn.gamma,
               -FBsc, OVF_DCY);
         break;

case DECAYI:   /* Decay modified by individual mean s(i) */
         /* S16 + S14 + S24 - 38 (128mV taken as 1.0) --> S16 */
         ND->curr_cij += msrsle((ix->Cn.target-ND->curr_cij)*
            CDAT.sbar, ix->Cn.gamma, -38, OVF_DCY);
         break;

case SKIP_ALL: /* Skip decay, amplification and DAS and amplification
               *  completion all at once (occurs in Sequence I).  */
         goto AMP_SKIP_ALL;

         } /* End of decay switch */

/*---------------------------------------------------------------------*
*                        Perform Amplification                         *
*                                                                      *
* V8E, 05/08/09, GNR - psiway,psjway replaced by kamv,kami,kamj,kams   *
*---------------------------------------------------------------------*/

/* The code in the following switch performs those aspects of ampli-
*  fication that are necessary to prepare variables for DAS even if
*  amplification per se is being skipped.  sjmmtj is always (Sj-wkmtj)
*  for statistics, while mjmmtj is set to (Mij-wkmtj) if Mij exists,
*  else remains same as sjmmtj.  Note that Mij must be kept updated
*  even if the synapse is now ineligible for amplification. */

/* Note:  In V7B, rounding of Mij was changed to use low-order bits of
*  rseed instead of constant.  This prevents Mij from converging on
*  stationary values.  Mijincr and Mijdecr are computed on scale S20
*  so bits are available for rounding.  It is assumed that the low-
*  order bits of rseed are sufficiently decorrelated from the high-
*  order bits used to round Cij--this is purely to save the time of
*  another udev call.  If averaging Cij over a subarbor, the same
*  rounding value will be used for all conns of a cell during one time
*  step.  This case should not occur in practice, and even if it does,
*  we should still be better off than before.  -GNR */

      sjmmtj = Sj - wkmtj;       /* S7/14 for Mij calc. and stats */
      /* Set 'way' cases for stats and amplif--initially same, but amp
      *  case may change if KAMWM, KAC_INELG handled differently,  */
      kams = kami;
      if (sjmmtj < 0) kams |= KAC_SJLO;
      kamj = kams & ~KAC_INELG;
      if (kams & KAC_INELG) kams = KAC_INELG;

      switch (amp_switch) {

/*---------------------------------------------------------------------*
*                               ELJTAMP                                *
*  Eligibility trace based on product of pre-and post-synaptic Hebbian *
*  terms.  This will always be value-based and may optionally invoke   *
*  codes Q (dynamic mti) and/or T (timed Mij decay).  Pass Mij (non-   *
*  clock) without subtracting mtj to ELJTFIN in mjmmtj.                *
*                                                                      *
*  Rev, 05/01/09, GNR - mnsi,mnsj tests affect Mij incr, not amp elg.  *
*---------------------------------------------------------------------*/

case ELJTAMP:
         d3gts2(Mij, Lija.psyn+ix->cnoff[MIJ]);   /* Pick up old Mij */
         Mijincr = (Sjtest < wkmnsj) ? 0 :
            /* simmti and sjmmtj are effectively S14 and their 32-bit
            *  product cannot overflow.  upsm is S16 and wayscale is
            *  S8, so their 32-bit product is unlikely to overflow.
            *  The quadruple product is 64 bits at S(14+14+8+16) =
            *  S52, or S20 after rounding and scaling.  kamj can be
            *  inelg here due to vmvt == 0, but calc Mijincr anyway. */
            msrsle(simmti*sjmmtj, ix->Cn.upsm*pctway[kamj &
               (KAC_SILO|KAC_SJLO)], -BITSPERUI32, OVF_AMP);

         /* Is Mij now acting as a timer?  If net delta Mij > 0,
         *  restart clock, otherwise, tick clock down, set mjmmtj
         *  back to zero if time is up.  */
         if (Mij <= Mijtest) {
            mjmmtj = ix->Cn.mxmij;
            if (Mijincr > ix->cnwk.go.mntsmxmp) Mij = Mijtest;
            else if (--Mij <= -S15) mjmmtj = Mij = 0;
            goto AMP_SAVE_Mij;
            }

         /* Mij is not acting as a timer, it is really Mij */
         Mijdecr = Mij * ix->Cn.zetam, Mijdecr = SRA(Mijdecr, 10);
         /* If zetam*m exceeds max pumping rate, use latter */
         if (labs(Mijdecr) > ix->Cn.mxmp) {
            ix->CNST.mpover += 1;
            Mijdecr = (Mijdecr >= 0) ? ix->Cn.mxmp : -ix->Cn.mxmp; }
         /* Calc net delta Mij (S20), round, shift to S14, apply */
         Mijincr = Mijincr - Mijdecr + (ix->rseed & 0x0000003fL);
         Mij += SRA(Mijincr, 6);
         /* When Mij exceeds mxmij, count instances, start ticking
         *  if KAMTS, otherwise just reset it.  */
         if (Mij > ix->Cn.mxmij) {
            ix->CNST.mover += 1;
            if (lkam & KAMTS) {
               mjmmtj = ix->Cn.mxmij;
               Mij = Mijtest;
               goto AMP_SAVE_Mij;
               }
            Mij = ix->Cn.mxmij;
            }
         mjmmtj = Mij;
         goto AMP_LABEL4;

/*---------------------------------------------------------------------*
*                               TICKAMP                                *
*  Darwin2-style amplification with Sj replaced by "modifying          *
*  substance" (Mij).  The decay of Mij is controlled by a clock.       *
*  The time remaining is coded as a negative integer value.            *
*                                                                      *
*  Rev, 05/01/09, GNR - mnsj test affects Mij incr, not amp elg.       *
*---------------------------------------------------------------------*/

case TICKAMP:
         d3gts2(Mij, Lija.psyn+ix->cnoff[MIJ]);   /* Pick up old Mij */
         if (Sjtest >= wkmnsj)
            /* Sj (S14) * upsm (S16) - 10 yields Mijincr (S20) */
            Mijincr = Sj*ix->Cn.upsm, Mijincr = SRA(Mijincr, 10);
         else Mijincr = 0;

         /* Is Mij now acting as a timer?  If net delta Mij > 0,
         *  restart clock, otherwise, tick clock down, set Sj
         *  back to zero if time is up.  */
         if (Mij <= Mijtest) {
            mjmmtj = ix->Cn.mxmij - wkmtj;
            if (Mijincr > ix->cnwk.go.mntsmxmp) Mij = Mijtest;
            else if (--Mij <= -S15) Mij = 0, mjmmtj = -wkmtj;
            goto AMP_SAVE_Mij;
            }

         /* Mij is not acting as a timer, it is really Mij */
         Mijdecr = Mij*ix->Cn.zetam, Mijdecr = SRA(Mijdecr, 10);
         /* If zetam*m exceeds max pumping rate, use latter */
         if (labs(Mijdecr) > ix->Cn.mxmp) {
            ix->CNST.mpover += 1;
            Mijdecr = (Mijdecr >= 0) ? ix->Cn.mxmp : -ix->Cn.mxmp; }
         /* Calc net delta Mij (S20), round, shift to S14, apply */
         Mijincr = Mijincr - Mijdecr + (ix->rseed & 0x0000003fL);
         Mij += SRA(Mijincr, 6);
         /* When Mij exceeds mxmij, start ticking */
         if (Mij > ix->Cn.mxmij) {
            Mij = Mijtest;
            mjmmtj = ix->Cn.mxmij - wkmtj;
            ix->CNST.mover += 1;
            goto AMP_SAVE_Mij;
            }
         goto AMP_LABEL3;

/*---------------------------------------------------------------------*
*                               SLOWAMP                                *
*  Darwin2-style amplification with Sj replaced by "modifying          *
*  substance" (Mij).  The decay of Mij is exponential.                 *
*                                                                      *
*  Rev, 05/01/09, GNR - mnsj test affects Mij incr, not amp elg.       *
*---------------------------------------------------------------------*/

case SLOWAMP:
         d3gts2(Mij, Lija.psyn+ix->cnoff[MIJ]);   /* Pick up old Mij */
         if (Sjtest >= wkmnsj)
            /* Sj (S14) * upsm (S16) - 10 yields Mijincr (S20) */
            Mijincr = Sj*ix->Cn.upsm, Mijincr = SRA(Mijincr, 10);
         else Mijincr = 0;

         Mijdecr = Mij*ix->Cn.zetam, Mijdecr = SRA(Mijdecr, 10);
         /* If zetam*m exceeds max pumping rate, use latter */
         if (labs(Mijdecr) > ix->Cn.mxmp) {
            ix->CNST.mpover += 1;
            Mijdecr = (Mijdecr >= 0) ? ix->Cn.mxmp : -ix->Cn.mxmp; }
         /* Calc net delta Mij (S20), round, shift to S14, apply */
         Mijincr = Mijincr - Mijdecr + (ix->rseed & 0x0000003fL);
         Mij += SRA(Mijincr, 6);
         /* When Mij exceeds mxmij, cap it and count instances */
         if (Mij > ix->Cn.mxmij) {
            Mij = ix->Cn.mxmij;
            ix->CNST.mover += 1;
            }
AMP_LABEL3:
         mjmmtj = Mij - wkmtj;
         if (lkam & KAMWM) kamj ^= (kamj & KAC_SJLO) ^ (mjmmtj < 0);
AMP_LABEL4:
         /* Save new Mij, not to exceed two bytes */
         if (Mij > (S15-1)) Mij = (S15-1);
         else if (Mij <= Mijtest) Mij = Mijtest + 1;
AMP_SAVE_Mij:
         d3ptl2(Mij, Lija.psyn+ix->cnoff[MIJ]);
         break;

/*---------------------------------------------------------------------*
*                         HEBBAMP and NO_AMP                           *
*  Normal modified Hebb-style amplification or just decaying.          *
*  Set mjmmtj = sjmmtj and continue to DAS code.                       *
*---------------------------------------------------------------------*/

case HEBBAMP:
case NO_AMP:
         mjmmtj = sjmmtj;
         if (Sjtest < wkmnsj) kams = KAC_INELG;
         break;
         }  /* End of amplification switch */

/*---------------------------------------------------------------------*
*                 Detailed amplification statistics.                   *
*  Rev, 06/10/03, GNR - DAS changed to si64 type.                      *
*  Rev, 07/17/07, GNR - Introduce sjmmtj, Mij, value switch, move      *
*     count to first row, value & Mij to last.                         *
*  V8E, 05/08/09, GNR - Replace psiway,psjway with kami,kamj.  Make    *
*     inelg for stats if !gotsj, but still allow amplif if RULE S,T,U. *
*     Use mjmmtj for the Mij stat to be consistent with other stats    *
*     and noting that Mij before coercion to +/-S15 is now not saved.  *
*  Rev, 04/02/12, GNR - Use defined constants for pdas offsets.        *
*---------------------------------------------------------------------*/

      if (mjmmtj == 0) kams = KAC_INELG;
      if (detstats) {
         si64 *pdws;
         pdws = pds0 = ix->pdas + (gotsj ? kams : KAC_INELG);
         pdws[OdasCt] = jasl(pdws[OdasCt], 1);       /* Case counts */
         pdws[OdasSi] = jasl(pdws[OdasSi], simmti);     /* (Si-mti) */
         pdws[OdasSj] = jasl(pdws[OdasSj], sjmmtj);     /* (Sj-mtj) */
         if (lkam & (KAMCG|KAMVM|KAMUE)) {              /* (v -mtv) */
            pdws[OdasVM] = jasl(pdws[OdasVM], (long)ix->cnwk.go.vmvt);
            pdws += LDR; }
         if (lkam & (KAMSA|KAMTS|KAMUE))                /* Sj = Mij */
            pdws[OdasVM] = jasl(pdws[OdasVM], mjmmtj);
         } /* End of detailed amplification statistics */

/*---------------------------------------------------------------------*
*               Finally the actual delCij calculation.                 *
*       Rounding and Clean-Up Following Amplification and Decay        *
*---------------------------------------------------------------------*/

      switch (finamp_switch) {

/*---------------------------------------------------------------------*
*                               ELJTFIN                                *
*          KAMUE eligibility trace amplification completion            *
*                                                                      *
*  Note:  Sj here is actually Mij before range check.  Do not subtract *
*  mtj or use wayscales here because both were already done as Mij was *
*  being accumulated over time.                                        *
*---------------------------------------------------------------------*/

case ELJTFIN:
         if (kams & KAC_INELG) goto AMP_LABEL6;
         /* Calc delCij = scaled and rounded delta Cij
         *     = Mij*(v-mtv)  (S14+S8-6 = S16)  */
         delCij = mjmmtj*(long)ix->cnwk.go.vmvt + 32;
         delCij = SRA(delCij, 6);
         goto AMP_LABEL5;

/*---------------------------------------------------------------------*
*                               NORMAMP                                *
*          Completion of normal, slow, or tick amplification           *
*---------------------------------------------------------------------*/

case NORMAMP:
         if (kams & KAC_INELG) goto AMP_LABEL6;
         /* On entry here, mjmmtj contains Mij-wkmtj if KAMSA or KAMTS.
         *  Calc delCij = scaled and rounded delta Cij
         *     = ((Sj or Mij)-wkmtj)*(Si-mti)*(v-mtv)*wayscale.
         *  If KAMVM:  S14+S14+S8+S8-28 = S16,
         *  otherwise: S14+S14+S8-20 = S16.  */
         delCij = msrsle(mjmmtj*(si32)pctway[kamj], hacell,
            (int)ix->cnwk.go.sms, OVF_AMP);

AMP_LABEL5:    /* Code common to ELJTFIN and NORMAMP */
         {  long Cijterm, phiarg;
            switch (ix->Cn.phifn) {
            case FFN_DFLT:
               if ((delCij ^ ND->curr_cij) >= 0) {
                  if ((phiarg = labs(ND->curr_cij)) >= ix->bigc16)
                     goto AMP_LABEL6;
                  phiarg = (phiarg>>11) & 0x1f;  /* To (S7)/4 */
                  }
               else phiarg = 0;
               break;
            case FFN_UNIT:
               phiarg = 0;
               break;
            case FFN_ABSCIJ:
               if ((phiarg = labs(ND->curr_cij)) >= ix->bigc16)
                  goto AMP_LABEL6;
               phiarg = (phiarg>>11) & 0x1f;  /* To (S7)/4 */
               break;
               } /* End phifn switch */
            /* Form delta*phi*(si-mti)*((Sj or Mij)-wkmtj) (S16):
            *  delCij is S16, phitab is S28, 28+16-28 = 16 */
            Cijterm = msrsle(delCij, ix->phitab[phiarg], -28, OVF_AMP);
            /* If there is phase, multiply Cijterm by a cosine-like
            *  function of the phase difference. S16 + S28 - 28 = S16.
            *  d3tchk() sets pamppcf to NULL if there is no phase.  */
            if (ix->Cn.pamppcf) {
               int dif = ((int)CDAT.old_phi-Pj) & PHASE_MASK;
               Cijterm = msrsle(Cijterm, ix->Cn.pamppcf->bins[dif],
                  -28, OVF_AMP);
               } /* End phasing */
            ND->curr_cij += Cijterm;   /* Apply delta to Cij */

#ifndef PAR
/* Print detailed amplification information if requested--
*  this is basically a permanent debugging option.  */

            if (icell == ix->pradi && Lija.jsyn == ix->pradj) {
               convrt("(P2,' For ',J0A" qLXN ",', cell ',J0IL8,"
                  "', ct ',J0UH4,', j ',J0IL8/'    terms=',2*B14IL7."
                  "<4,B8IH7.4,', way=',B8IH7.<4,', prod=',B16IL7.<4,"
                  "', delCij=',B16IL7.<4)", fmtlrlnm(il), &icell,
                  &ix->ict, &Lija.jsyn, &simmti, &mjmmtj, &ix->cnwk.
                  go.vmvt, pctway+kamj, &delCij, &Cijterm, NULL);
               }
#endif

/* Accumulate synaptic change statistics.
*     (In Darwin2, these were counted only in statistical cycles.
*  Rev, 08/01/92, GNR - Omit disallowed sign changes.
*  V8D, 07/25/07, GNR - Now accumulate change statistics based on
*     Cijterm alone, regardless of any decay, random rounding, or
*     disallowed sign changes.  These statistics should be more
*     useful for tuning amplification.
*  Rev, 04/02/12, GNR - Add DeltaCij to detailed amp stats */

            if (Cijterm > 0) {
               ix->CNST.sdcij[EXCIT] =
                  jasl(ix->CNST.sdcij[EXCIT], Cijterm);
               ix->CNST.nmod[EXCIT] += 1; }
            else if (Cijterm < 0) {
               ix->CNST.sdcij[INHIB] =
                  jasl(ix->CNST.sdcij[INHIB], Cijterm);
               ix->CNST.nmod[INHIB] += 1; }
            if (detstats)
               pds0[OdasDC] = jasl(pds0[OdasDC], Cijterm);

            } /* End local scope ... fall through to NULLAMP */

/*---------------------------------------------------------------------*
*                               NULLAMP                                *
*  Here when there is no amplification, but nonetheless the Cij must   *
*  be checked for overflow, random-rounded, and checked for sign       *
*  change because decay to a target may cause changes.  All relevant   *
*  values are in NDAT struct for passing to cijclean().  If there is   *
*  neither decay nor amplification, this code is skipped altogether.   *
*                                                                      *
*  Note, 06/23/94, GNR:  Code previously located here has been removed *
*  to cijclean() function so it can be executed after averaging when   *
*  NOPSAA is used.  Former goto from above code into interior of this  *
*  block on Cij overflow has been eliminated.                          *
*---------------------------------------------------------------------*/

case NULLAMP:
AMP_LABEL6:
         if (doampavg) {
            /* Averaging subarbor Cij.  Accumulate sums. */
            ND->new_cij += ND->curr_cij;
            ++ND->narb;
            } /* End Cij averaging */
         else {
            /* Here when not averaging subarbor Cij */
            cijclean(ix, ND);
            /* Need to store Cij, Cij0 only if changed */
            if (ND->new_cij != ND->old_cij) d3pthn(ND->new_cij,
               Lija.psyn+ix->cnoff[CIJ], ix->cijlen);
            if (ix->cnwk.go.vrho && ND->new_cij0 != ND->old_cij0)
               d3pthn(ND->new_cij0, Lija.psyn+ix->cnoff[CIJ0],
               ix->cijlen);
            /* Save change for possible detail print--skip the
            *  'if' statement to save time */
            *pdelc++ = (ND->new_cij - ND->old_cij) >> Ss2hi;
            } /* End no Cij averaging */
         break;

case NO_FIN_AMP:  /* No change in Cij--don't do anything except
         *  update rseed if needed for Mij, Rbar rounding */
         if (ix->cnflgs & UPRSD) udevskip(&ix->rseed, ix->upcij1s);
         break;
         } /* End of amplification completion switch */
AMP_SKIP_ALL:

/* Accumulate distribution of Cij if in a statistical cycle (and not
*  averaging over subarbors).  Hold off until later if averaging,
*  because result is not known yet.  If averaging is requested, but
*  amplification is off, cdist will be handled normally here.
*  V8E, 04/12/09, GNR - Correctly handle -0 case, use full new_cij,
*                       not just offcijhigh byte as before.
*/

      if (Sjtest > SjCijdst) {
         ddist *pCdist = il->pctddst + ix->ocnddst + OaCd;
         int cijbin = (ND->new_cij == MINUS0) ? (LDSTAT-1) :
            (ui32)ND->new_cij >> (BITSPERUI32-4);
         pCdist->d[cijbin] = jasl(pCdist->d[cijbin], 1);
         }

/*---------------------------------------------------------------------*
*                 Finished processing one connection                   *
*---------------------------------------------------------------------*/

      } /* End of loop over connections (synapses) */

/*---------------------------------------------------------------------*
*          Finished processing inputs to one connection type           *
*---------------------------------------------------------------------*/

/* Perform Cij averaging if necessary */

   if (finamp_switch != NO_FIN_AMP) {
      if (doampavg) {
         register struct CONNDATA *pcijavg;
         register rd_type *pavg = ix->psyn0 + coff + ix->lxn;
         rd_type *psynx;         /* Ptr to skipped synapses */
         struct CONNDATA *cijavgl = cijavg + ix->nsa;
         long iskp,isub,isyn,skipndx;
         /* Average Cij over equivalent amplified values */
         for (pcijavg=cijavg; pcijavg<cijavgl; pcijavg++)
               if (pcijavg->narb) {
            pcijavg->curr_cij = pcijavg->new_cij/pcijavg->narb;
            /* Note that data needed by cijclean, other than
            *  new curr_cij calculated here, are left in CONNDATA
            *  struct as a result of first connection loop above. */
            cijclean(ix, pcijavg);
            /* Accumulate distribution of Cij omitted earlier.
            *  Here we can accumulate all equivalent Cij at once. */
            if (statenbc) {
               ddist *pCdist = il->pctddst + ix->ocnddst + OaCd;
               int cijbin = (pcijavg->new_cij == MINUS0) ? (LDSTAT-1) :
                  (ui32)pcijavg->new_cij >> (BITSPERUI32-4);
               pCdist->d[cijbin] =
                  jasl(pCdist->d[cijbin], pcijavg->narb);
               } /* End Cdist incrementing */
            } /* End subarbor loop */

         /* Now it is necessary to loop over all the connections
         *  again, picking up and saving relevant averaged Cij.
         *  Setup in d3allo and d3genr assures that a skip list
         *  exists.  */
         if ((iskp = ix->nc - Lija.nuk) > 0) {
            psynx = Lija.pxnd + ix->lct - ix->lc;
            d3gtln(skipndx, psynx, ix->nuklen); }
         else
            skipndx = -1;
         for (isyn=isub=0; isyn<ix->nc; ++isyn,++isub) {
            /* Maintain connection index within current subarbor */
            if (isub >= ix->nsa) isub = 0;
            /* Skip over connections that were skipped above.  Use
            *  counts established above, as Lij may not be stored. */
            if (isyn == skipndx) {
               if (--iskp > 0) {
                  psynx -= ix->lc;
                  d3gtln(skipndx, psynx, ix->nuklen); }
               else
                  skipndx = -1;
               continue;   /* Skip rest of connection processing */
               } /* End skipped connection processing */
            /* Locate averaged data for this connection */
            ND = cijavg + isub;
            /* Need to store Cij, Cij0 only if changed */
            if (ND->new_cij != ND->old_cij)
               d3pthn(ND->new_cij, pavg+ix->cnoff[CIJ], ix->cijlen);
            if (ix->cnwk.go.vrho && ND->new_cij0 != ND->old_cij0)
               d3pthn(ND->new_cij0, pavg+ix->cnoff[CIJ0], ix->cijlen);
            pavg += ix->lc;
            /* Save change for possible detail print--skip the
            *  'if' statement to save time */
            *pdelc++ = (ND->new_cij - ND->old_cij) >> Ss2hi;
            } /* End connection loop */
         } /* End if doampavg */
      } /* End if finamp_switch */
   pctway += (MAX_WAYSET_CODES + 1);

/* Advance rseed over nc connections if did rounding */

   ix->rseed = ix->cnwk.go.psvrseed;
   if (ix->cnflgs & UPRSD)
      udevskip(&ix->rseed, ix->upcij);

/* Update phase seed for next cell if skipped connections */

   if (ix->phopt & PHASR) {
      long nskip = ix->nc - Lija.nuk;
      if (nskip) udevskip(&Lija.wpseed, nskip);
      ix->upphseed = Lija.wpseed;
      }

/* If doing subarbors:  Accumulate last (or only) subarbor
*  contribution if it exceeds mnax.  Copy totals back to rax.  */

   if (ix->Cn.saopt & DOSARB) {
      register int p;
      si64 tsum = jasw(raxp[0],raxn[0]);
      tsum = jrsw(jabs(tsum),ix->wsmnax);
      if (qsw(tsum) >= 0) for (p=0; p<lrax12w; p++)
         saxp[p] = jasw(saxp[p],raxp[p]);
      memcpy((char *)raxp, (char *)saxp, lrax1b);
      }

/*---------------------------------------------------------------------*
*  Final scaling, voltage-dependence or phase convolution, capping,    *
*                        decay, and summation                          *
*---------------------------------------------------------------------*/

/* N.B. Before this code, unphased sums are in rax[pn][0], phased
*  contributions in rax[pn][1-32], and early convolution results,
*  if any, in sax[pn][0-31].  Raw contributions can now be scaled
*  and reduceed to S20.  Results are stored back in rax[pn][0-31],
*  and in *pAffD64 for possible use in detail print (printed even
*  if the contribution is below mnax).  Then, if ADCY decay exists,
*  persistence term (32-bits) goes into paxp,paxn, followed by the
*  final (capped, scaled, convoluted, volt-dep, etc.) values.
*  AffD64,paxp,paxn,aeff are allocated only when corresponding
*  cssck bit is set, unlike rax[pn],sax[pn], which always exist.
*
* As of 12/27/08, code previously included here to avoid overflow of
*  64-bit intermediate scaling product by prescaling based on bitsz
*  of sum has been removed, and msswe replaced by msrswe.  This is
*  possible because msswe,msrswe now keep a 94-bit intermediate
*  product before shifting.  The total scaling shift is 27 bits
*  = 24 for scl (S24) plus 3 for S23 to S20 conversion.
*  Unscaled, unphased sums are saved in CDAT.rsumaij (by conntype)
*  for mnax check and used later for afference stats, then scaled
*  for output to SIMDATA file.  Keeping si64 here makes overflow
*  during these operations very unlikely and so no checking is
*  necessary.
*
* Rev, 04/11/09, GNR - Added cssck tests where needed to avoid storing
*  into nonexistent paxp,paxn,aeff array elements, and elsewhere where
*  it might save a little time (e.g. multiple msrswe calls).
*
* Rev, 04/23/12, GNR - Now perform mxax checks on [rs]ax[pn] at 64-bit
*  precision in order to eliminate overflow error when reducing results
*  to 32 bits for pax[pn].  This moves mxax checks before convolution
*  and volt-dep calcs, making it more useful to avoid overflows.
*
* Rev, 05/23/12, GNR - Add CNOPT=C option to combine pos/neg terms
*  before mxax checking for better cancellation of large inputs.
*  Further revised, 05/27/12, to save scaled contribs before
*  combining in new 64-bit pAffD64 area.  */

   CDAT.rsumaij[zbict] = jasw(raxp[0],raxn[0]);
   {  si64 tsum, *pa64 = CDAT.pAffD64 + ix->oex64;
      int ks = -(FBsc + FBrs - FBwk);     /* Total Aij scale */
      if (ix->cnflgs & DOCEI) {           /* Combine before mxax test */
         /* In this case, d3news guarantees that both excit and
         *  inhib storage areas are present, so no need to test  */
         register int p,q;
         for (p=lrax0; p<lrax1w; p++) {   /* Loop over 0 only or 1-32 */
            q = p - lrax0;
            pa64[q]        = msrswe(raxp[p], ix->sclu, ks, OVF_SPEC);
            pa64[q+lpax64] = msrswe(raxn[p], ix->sclu, ks, OVF_SPEC);
            tsum = jasw(pa64[q], pa64[q+lpax64]);
            if (qsw(tsum) >= 0) {
               si64 tmx = jrsl(tsum, ix->Cn.mxax);
               raxp[q] = (qsw(tmx) > 0) ? jesl(ix->Cn.mxax) : tsum;
               raxn[q] = si64zero; }
            else {
               si64 tmx = jasl(tsum, ix->Cn.mxax);
               raxn[q] = (qsw(tmx) < 0) ? jesl(-ix->Cn.mxax) : tsum;
               raxp[q] = si64zero; }
            } /* End phase loop */
         } /* End "combine before test" */
      else {                              /* Normal case, test first */
         if (gotex) {
            register int p,q;
            /* Loop over 0 only or 1-32 */
            for (p=lrax0; p<lrax1w; pa64++,p++) {
               q = p - lrax0;
               *pa64 = msrswe(raxp[p], ix->sclu, ks, OVF_SPEC);
               tsum = jrsl(*pa64, ix->Cn.mxax);
               raxp[q] = (qsw(tsum) > 0) ? jesl(ix->Cn.mxax) : *pa64;
               }
            }
         if (gotin) {
            register int p,q;
            /* Loop over 0 only or 1-32 */
            for (p=lrax0; p<lrax1w; pa64++,p++) {
               q = p - lrax0;
               *pa64 = msrswe(raxn[p], ix->sclu, ks, OVF_SPEC);
               tsum = jasl(*pa64, ix->Cn.mxax);
               raxn[q] = (qsw(tsum) < 0) ? jesl(-ix->Cn.mxax) : *pa64;
               }
            }
         } /* End else not CODEI */
      } /* End pa64, tsum local scope */

/* The mnax test can now be applied, always to the raw sums, because
*  if it fails, no more calculations are needed.  This is done on the
*  unscaled data so same test can be used on subarbors before scaling.
*  As of V8C, omit if unphased total is below descaled mnax whether
*  or not there are phases or subarbors.  But even in this case, any
*  decay contribution--none is possible if there are phases--should
*  be updated and saved for dprt and conductance activation.  */

   {  si64 tsum = jrsw(jabs(CDAT.rsumaij[zbict]),ix->wsmnax);
   if (qsw(tsum) < 0) {
      long pnsum = 0;
      if (gotex) {
         if (ix->Cn.ADCY.kdcy) {
            long capex = d3decay(&ix->Cn.ADCY, ix->cnwk.go.paeff++,
               0, paxp++);
            pexSums[0] += paxp[0] = capex;
            pnsum += capex;
            }
         else
            memset((char *)paxp, 0, lpaxcb);
         }
      if (gotin) {
         if (ix->Cn.ADCY.kdcy) {
            long capin = d3decay(&ix->Cn.ADCY, ix->cnwk.go.paeff++,
               0, paxn++);
            pinSums[0] += paxn[0] = capin;
            pnsum += capin;
            }
         else
            memset((char *)paxn, 0, lpaxcb);
         }
      CDAT.psumaij[zbict] = pnsum;
      goto SKIP_CONNTYPE_CONTRIBUTION;
      }}

/* If early convolution was done, the convolved sums are now scaled
*  and the results stored back in sax[pn][0-31].  Voltage-dependent
*  scaling and late convolution can be skipped without further tests.
*  In this case, it is guaranteed that there are no subarbs, so size
*  of saxp and saxn is just PHASE_RES (32), i.e. they are adjacent.
*/

   if (ix->phopt & PHASECNV) {
      int ks = -(FBsc + FBrs - FBwk);  /* Total Aij scale */
      register int p;
      for (p=0; p<lsax12w; p++) {
         si64 tsum = jrsw(jabs(saxp[p]), ix->wsmxax);
         if (qsw(tsum) > 0) saxp[p] =
            jesl(qsw(saxp[p]) >= 0 ? ix->Cn.mxax : -ix->Cn.mxax);
         else
            saxp[p] = msswe(saxp[p], ix->sclu, ks, OVF_SPEC);
         }
      } /* End scaling early convolution */
   else {

/* If input had constant or uniform phase, distribute accordingly.
*  (Because subarbors are allowed in PHASC|PHASU cases, sax[pn] may
*  have subarbor sums that now must be cleared.  saxp and saxn parts
*  may not be adjacent.  */

      if (ix->phopt & (PHASC|PHASU)) {
         si64 raxp0 = raxp[0], raxn0 = raxn[0];
         register int p;
         if (ix->phopt & PHASC) {            /* Constant phase */
            memset((char *)saxp, 0, lsaxb);  /* (Maybe 2 extra) */
            p = (int)ix->Cn.phi;
            /* N.B.  If nexin == 1, saxp = saxn and this code
            *  harmlessly does the same copy twice.  */
            saxp[p] = raxp0, saxn[p] = raxn0;
            }
         else {                              /* Uniform phase */
            /* Need sep loops for saxp,saxn because different RHS */
            if (gotex) for (p=0; p<PHASE_RES; p++)
               saxp[p] = raxp0;
            if (gotin) for (p=0; p<PHASE_RES; p++)
               saxn[p] = raxn0;
            }
         }

/* Finally, if input was PHASJ or PHASR without early convolution,
*  or simply no phase at all, now just copy the scaled raw sums to
*  the scaled sums where final action will occur.  */

      else if (ix->phopt & (PHASJ|PHASR)) {
         if (gotex) memcpy((char *)saxp, (char *)(raxp), laffb);
         if (gotin) memcpy((char *)saxn, (char *)(raxn), laffb);
         }

      else {
         /* Harmless duplicate copy if nexin == 1 */
         saxp[0] = raxp[0];
         saxn[0] = raxn[0];
         }

/* Handle voltage-dependent connections.  Multiply voltage-dependent
*  afferents binwise by thresholded input from normal afferents.
*  Afference and output from this calculation are scaled and S20.
*  (Phase convolution, if any, is always deferred in this situation.)
*
*  N.B.  CONNTYPE list is now rethreaded so all non-voltage-dependent
*  CONNTYPEs are processed first.  That means that when the first VDT
*  is encountered, we can just add up the data for all the previous
*  CONNTYPEs, and that gives us the bin multipliers we need.  Old-
*  style persistence is recomputed here for COMPAT=V.  */

      if (ix->Cn.cnopt & NOPVA) {
         long tns;               /* Thresholded NormSums, S20 */
         register int p;         /* Phase index */

         if (!didnorms) {
            /* Form normal afferent sums (including self-input).
            *  (This must be done even if sums not used in NOPVP case
            *  because of effect on other conntypes and final sums.
            *  When all conntypes are done, another call to sumaffs
            *  will add the volt-deps into these totals.)  */
            sumaffs(il, CDAT.pTotSums);
            /* Combine excitatory and hyperpolarizing components.
            *  If compatibility option V, and no phases, and no self
            *  input, include precalculated persistence in NormSums.
            *  Replace persistence with self-input in new runs.  */
            if (il->ctf & CTFVD) {
               sumaffs(il, CDAT.pNormSums);
               CDAT.pNormSums[0] += normpers; }
            didnorms = TRUE;
            }

         /* Alternative method using old Si */
         if (ix->Cn.cnopt & NOPVP) {
            tns = oldsi + normpers - ix->Cn.vdt;
            if (tns <= 0)  /* Nullify if negative v-vdt */
               memset((char *)saxp, 0, lsaxb);
            else {
               /* tns/vdha:  S20+32-8-S20-1 ==> S24, extra right
               *  shift is because vdha is half-effective level
               *  (vdha is guaranteed to be nonzero).  */
               tns = ds64nq(tns, 0, -IBsc-1, ix->Cn.vdha);
               /* Adjust tns using sigmoidal method */
               if (ix->Cn.cnopt & NOPVC) tns = (long)(dS24 *
                  (1.0 - 1.0/cosh((double)tns*VDHAMULT)));
               if (gotex) for (p=0; p<laffs; p++)
                  saxp[p] = msrswe(saxp[p], tns, -FBsc, OVF_VDEP);
               if (gotin) for (p=0; p<laffs; p++)
                  saxn[p] = msrswe(saxn[p], tns, -FBsc, OVF_VDEP);
               } /* End tns > 0 */
            } /* End NOPVP method */

         /* Original method, using normal sums in each bin */
         else for (p=0; p<laffs; p++) {
            tns = CDAT.pNormSums[p] - ix->Cn.vdt;
            if (tns <= 0)  /* Nullify if negative v-vdt */
               saxp[p] = saxn[p] = jesl(0);
            else {
               /* tns/vdha:  S20+32-8-S20-1 ==> S24, extra right
               *  shift is because vdha is half-effective level
               *  (vdha is guaranteed to be nonzero).  */
               tns = ds64nq(tns, 0, -IBsc-1, ix->Cn.vdha);
               /* Adjust tns using sigmoidal method */
               if (ix->Cn.cnopt & NOPVC) tns = (long)(dS24 *
                  (1.0 - 1.0/cosh((double)tns*VDHAMULT)));
               /* Multiply using linear or sigmoidal method.  Here
               *  we do the gotex,gotin test inside the loop to avoid
               *  duplicating all the code above for the two cases. */
               if (gotex) saxp[p] =
                  msrswe(saxp[p], tns, -FBsc, OVF_VDEP);
               if (gotin) saxn[p] =
                  msrswe(saxn[p], tns, -FBsc, OVF_VDEP);
               }
            } /* End phase bin multiplication loop */

         } /* End voltage-dependent adjustment */

/* Finally, if convolution was deferred, apply it now.  Convolution
*  is deferred if nc >=32 or PHASU or PHASC or volt-dep or subarbors.
*  This should save significant time if nc is large.  In other cases,
*  convolution continues to be performed early, inside the connection
*  loop as originally coded by MC.  rax[pn] will have been allocated
*  for use as temp space.  Loop order is reversed relative to PHASECNV
*  code to allow one test on kernel to save amssw for both saxp and
*  saxn.  Both input and output are in sax[pn][0-31].  */

      if (ix->phopt & PHASDCNV) {
         long kerj, *pkern = ix->Cn.ppcf->bins;
         int j,p,pmj;
         if (gotex) {
            memset((char *)raxp, 0, laffb);
            for (j=0; j<PHASE_RES; j++) {
               if (kerj = pkern[j])          /* Assignment intended */
                     for (p=0; p<PHASE_RES; p++) {
                  si64 tsum;
                  pmj = (p - j) & PHASE_MASK;
                  /* S20 + S28 - 28 leaves S20 result */
                  tsum = msrswe(saxp[pmj], kerj, -FBkn, OVF_PHAS);
                  raxp[p] = jasw(raxp[p], tsum);
                  }  /* End convolving one kernel bin */
               } /* End loop over kernel bins */
            memcpy((char *)(saxp), (char *)raxp, laffb);
            }
         if (gotin) {
            memset((char *)raxn, 0, laffb);
            for (j=0; j<PHASE_RES; j++) {
               if (kerj = pkern[j])          /* Assignment intended */
                     for (p=0; p<PHASE_RES; p++) {
                  si64 tsum;
                  pmj = (p - j) & PHASE_MASK;
                  /* S20 + S28 - 28 leaves S20 result */
                  tsum = msrswe(saxn[pmj], kerj, -FBkn, OVF_PHAS);
                  raxn[p] = jasw(raxn[p], tsum);
                  }  /* End convolving one kernel bin */
               } /* End loop over kernel bins */
            memcpy((char *)(saxn), (char *)raxn, laffb);
            }
         } /* End deferred convolution */

      } /* End not early convolution */

/* Perform capping, and decay.
*  Save final sums in paxp,paxn (but if constant or uniform phase
*     and no convolution or volt-dep, just save the one unique
*     value) and accumulate in pexSums,pinSums.
*  Save fully processed contribution for conductance activation.
*  Rev, V7C - Test against mnax removed because it was redundant
*             with tests applied individually to subarbor sums.
*  Rev, V8C - Test against mnax now also applied to total of all
*             subarbors, as it should have been since V7C.  Test
*             for overflow in pexSums,pinSums removed because it
*             is considered unnecessary now that sums are S20 (and
*             and in any event can be prevented with suitable mxax).
*  Rev, V8E - Bug fix:  ix->aeff only exists if cssck bits set.
*  Rev, V8H - mxax test now performed before convolution & voltdep.
*/

   if (ix->phopt & (PHASJ|PHASR|PHASECNV|PHASDCNV|PHASVDU)) {
      /* Capping, no decay, store all 32 sums */
      long pnsum = 0;
      if (gotex) {                   /* Excitation */
         register int p;
         for (p=0; p<PHASE_RES; p++) {
            paxp[p] = swlo(saxp[p]);
            pexSums[p] += paxp[p];
            pnsum += paxp[p];
            }
         }
      if (gotin) {                   /* Inhibition */
         register int p;
         for (p=0; p<PHASE_RES; p++) {
            paxn[p] = swlo(saxn[p]);
            pinSums[p] += paxn[p];
            pnsum += paxn[p];
            }
         }
      CDAT.psumaij[zbict] = pnsum;
      } /* End capping, summing inputs with phase */

   else if (ix->phopt & PHASU) {
      /* PHASU, no volt-dep or convolution,
      *  all phase bins are the same */
      long pnsum = 0;
      if (gotex) {                   /* Excitation */
         register int p;
         paxp[0] = swlo(saxp[0]);
         for (p=0; p<PHASE_RES; p++)
            pexSums[p] += paxp[0];
         pnsum += paxp[0];
         }
      if (gotin) {                   /* Inhibition */
         register int p;
         paxn[0] = swlo(saxn[0]);
         for (p=0; p<PHASE_RES; p++)
            pinSums[p] += paxn[0];
         pnsum += paxn[0];
         }
      CDAT.psumaij[zbict] = pnsum << PHASE_EXP;
      } /* End case: all phase bins the same */

   else if (ix->phopt & PHASC) {
      /* PHASC, result just goes into one bin */
      long pnsum = 0;
      register int p = (int)ix->Cn.phi;
      if (gotex) {                   /* Excitation */
         paxp[0] = swlo(saxp[p]);
         pexSums[p] += paxp[0];
         pnsum += paxp[0];
         }
      if (gotin) {                   /* Inhibition */
         paxn[0] = swlo(saxn[p]);
         pinSums[p] += paxn[0];
         pnsum += paxn[0];
         }
      CDAT.psumaij[zbict] = pnsum;
      } /* End case: all phase bins the same */

   else {
      /* Otherwise (no phase), cap sums at mxax and apply decay. */
      long pnsum = 0;
      if (gotex) {                   /* Excitation */
         long capex = swlo(saxp[0]);
         if (ix->Cn.ADCY.kdcy)
            capex = d3decay(&ix->Cn.ADCY, ix->cnwk.go.paeff++,
               capex, paxp++);
         pexSums[0] += paxp[0] = capex;
         pnsum += capex;
         }
      if (gotin) {                   /* Inhibition */
         long capin = swlo(saxn[0]);
         if (ix->Cn.ADCY.kdcy)
            capin = d3decay(&ix->Cn.ADCY, ix->cnwk.go.paeff++,
               capin, paxn++);
         pinSums[0] += paxn[0] = capin;
         pnsum += capin;
         }
      CDAT.psumaij[zbict] = pnsum;
      } /* End cap, decay, save */

SKIP_CONNTYPE_CONTRIBUTION: ;

/* If requested, save ak (net ax) for SIMDATA.  Intermediate
*  storage in prd data is necessary to avoid serializing d3go
*  in order to send data to host cell-by-cell for output.
*  si64 data are reduced here to floats for SIMDATA convenience.
*  N.B. d3allo() guarantees that pak points to an appropriate
*  storage boundary */

   if (ix->cnflgs & NSVAK) {
      float *pak = (float *)(Lija.pxnd + ix->xnoff[AK]);
      *pak = (float)(daxs*swdbl(CDAT.rsumaij[zbict]));
      }

   }  /* End of loop over connection types */

/***********************************************************************
*                                                                      *
*                 Section 4.  Complete cell processing                 *
*                                                                      *
*  Beginning of what was cellfin routine in original IBM version--     *
*  Now that all inputs have been processed, finish up calculation      *
*     of cell response, leaving net afferent voltage in cellaffs.      *
*  Terms in cellaffs:                                                  *
*     Input from normal and voltage-dependent connection types.        *
*     Add in all remaining afference terms.                            *
*     Evaluate si and phi if there is phasing.                         *
*     Apply probe stimulation.                                         *
*  Additional transformations to get cell response:                    *
*     Apply any nonlinearities, such as a sigmoidal response           *
*        function (not yet implemented), spiking, or step vs           *
*        knee function.                                                *
*     Apply noise, refractory period, and depression.                  *
*  Accumulate statistics.                                              *
*  Advance topographic map origins as needed.                          *
*  (Unrealistic "LTP" option removed in V8A.)                          *
*                                                                      *
*  (This could be broken out into a separate source file)              *
*----------------------------------------------------------------------*
*  Two levels of indenting suppressed to end of cell loop              *
***********************************************************************/

/* Combine afferent inputs for all relevant afferent types.  As of
*  V8A, this is now fully checked for overflow.  Squared and shunting
*  inhibition are applied individually to all phase bins.  */

      sumaffs(il, CDAT.pTotSums);
      sumaffs(il, CDAT.pCellResp);

/* Handle squared inhibitory types.  The scaling of the squared sums
*  is: S20 + S20 - 20 = S20 returned by mssle, but need to divide by
*  an additional S7 in COMPAT=C mode to make 128mV = 1.0 multiplier.
*  This is set up in isqis on each call to d3go.  */

      if (il->kaff & AFF_Square<<SGM_Total) {
         register int p;
         for (p=0; p<laffs; p++) CDAT.pCellResp[p] -=
            msrsle(CDAT.pSqSums[p], CDAT.pSqSums[p], isqis, OVF_RFN);
         } /* End squared inhibition */

/* Handle shunting inhibition.  In looking up phi function,
*  use our usual mapping that 128mV = 1.0 (old activity scale).
*  This is equavalent to S27, so shift 19 to get 8 bit arg to
*  look up phi.  Then S20+8-8 = S20 in bits returned by msrsle.  */

      if (il->kaff & AFF_Shunt<<SGM_Total) {
         register int p;
         for (p=0; p<laffs; p++) {
            long abv = labs(CDAT.pShSums[p]);
            CDAT.pCellResp[p] = (abv >= 1L<<27) ? 0 :
               msrsle((si32)(ui32)ctab[abv>>19],
                  CDAT.pCellResp[p], -8, OVF_RFN);
            }
         } /* End shunting inhibition */

/* If cells have phase, call d3phase to calculate final activity and
*  phase according to phimeth and simeth controls.  Then save final
*  phase distribution for SIMDATA if requested (this is necessary to
*  avoid serializing d3go to send pCellResp to host for output). Note
*  that phase distribution is stored in host order, not memacc order.
*  If no phase, s(i) is just the result of the above calculations.  */

      if (il->phshft) {
         cellaffs = d3phase(il);
         if (il->ctf & CTFPD) memcpy((char *)plsd+il->ctoff[PHD],
            (char *)CDAT.pCellResp, sizeof(long)*PHASE_RES);
         }
      else
         cellaffs = *CDAT.pCellResp;

/* Process probe option - add input to selected cells.
*  N.B.  Currently, probes are added into final cellaffs with no
*     particular phase.  It may be desirable to change this.  */

      if (lpptr > 0 && ilstnow(&prbit) == icell) {
         /* If there is a user function, call it now */
         if (pprb->pupsprb) CDAT.probeval =
            (long)((float)S20*pprb->pupsprb((long)il->seqvmo,
            icell, RP->CP.ser, RP->CP.trial,
            (long)CDAT.curr_cyc1, (float)pprb->sprb/(float)S20));
         cellaffs += CDAT.probeval;
         CDAT.events |= PROBED;
         /* Advance to next cell and save cell number for restarting
         *  iterator on next trial.  (This is now done redundantly
         *  in each cycle in case cycle loop terminates early.)  */
         ilstiter(&prbit);
         if (--lpptr == 0) pprb->probnxt = ilstnow(&prbit);
         } /* End of probe */

/*---------------------------------------------------------------------*
*             That completes evaluation of cell afferents              *
*        Now apply various nonlinearities to get cell response         *
*---------------------------------------------------------------------*/

      response = cellaffs;       /* Initially, response = affs */

/* Perform KNEE or STEP function evaluation of net afference.
*  Check for hits (positive and negative) and set triggers to
*     count them later if statistics being done.
*  (In Darwin II, positive hits were counted before inhib.)
*  Really should check pt,nt against excit,inhib sums separately.
*/

      if (il->ctf & CTFDR) {
         /* STEP response */
         if (response >= il->Ct.pt)       hitpt = 1;
         else if (response <= il->Ct.nt)  hitnt = 1;
         else response = 0;
         }
      else {
         /* KNEE response */
         if (response >= il->Ct.pt)
            response -= il->Ct.pt,        hitpt = 1;
         else if (response <= il->Ct.nt)
            response -= il->Ct.nt,        hitnt = 1;
         else response = 0;
         }

/* V8A, 04/15/96, GNR - This code was heavily revised to give each
*  cell an individual dynamically variable 'st' firing threshold
*  and to couple refractory periods to 'st' levels rather than to
*  depression.  Redundant code was introduced to improve clarity.
*  Depression is now applied based on D(t) rather than on D(t-1).
*  Former "LTP" calculations and "hit" statistics were removed.
*  Old-style ('rt') refractory period was already removed in V7B.  */

/* If in an absolute refractory period, step timer and either set
*  response to zero or let it decay.  It might be possible to save
*  some time by omitting calculation of cell afferents during absolute
*  refractory periods, however, great care would be needed to assure
*  deterministic updating of all random number seeds and pointers in
*  the connection loops.  */

      if (wkdst < 0) {  /* Guaranteed 0 if prf is NULL */
         /* Set event bit and count refractory events */
         CDAT.events |= REFRAC;
         /* Advance noise seed deterministically */
         if (il->ctf & CTFHN) udevskip(&il->nsd, 2);
         /* Tick clock.  At end of refractory interval,
         *  set to resume normal response processing. */
         if ((wkdst += S16) >= 0) wkdst = prf->psdst;
         /* If refractory period is not abrupt, decay down
         *  from previous activity towards baseline + AHP.
         *  This is not handled by d3decay(), as rate is not
         *  omega2 and limiting/saturating does not apply. */
         response = prf->upsahp ? (oldsi - msrsle(prf->upsahp,
            oldsi-prf->ahp, -28, OVF_RFN)) : prf->ahp;
         } /* End handling absolute refractory period */

/* Not in an absolute refractory period.  Calculate and add noise
*  (S20/27).  If afference plus noise exceeds wkst and an absolute
*  refractory period exists, set to enter one on next cycle, other-
*  wise replace dst with psdst ("post-spike dst") on next cycle.
*  Note:  NMOD can force nmodval < 0, which is OK, gives 0 noise.
*  Note:  Before V8A, noise could not initiate a spike.
*  Rev, 05/05/08, GNR - Restore checking for refractory period for
*     all response functions, not just RF_SPIKE.  For a time in V8D,
*     this was not done, but the doc specifies it, and it is easily
*     turned off if not wanted.
*/

      else {
         long tsimr, respnois = response;
         if (il->ctf & CTFHN) {
            CDAT.noise = d3noise(
               il->No.nmn, il->No.nsg, CDAT.nmodval, il->nsd);
            respnois += CDAT.noise; }
         if (prf) {
            if (respnois >= wkst)
               wkdst = prf->refrac ? refrcs16 : prf->psdst;
            else
               CDAT.events |= UPDTDST;
            }

/* Switch according to response function */

         switch ((enum RespFunc)il->rspmethu) {

/* IZHIKEVICH function--evaluate response as described by MATLAB
*  code in "Simple Model of Spiking Neurons" by E.M. Izhikevich,
*  IEEE Trans. Neural Networks, 14:1569-1572 (2003) (RF_IZHI3)
*  and in his book "Dynamical Systems in Neuroscience, MIT Press
*  (2007) (RF_IZHI7), p. 274.
*
*  One problem is that when a spike occurs, Izhikevich sets the
*  response from the previous time step to the peak value, just
*  to make all the spikes the same size ("to avoid amplitude jitter
*  associated with the finite simulation time step" as he puts it).
*  This is impossible in CNS because the data for the previous time
*  step will already have been graphed, written to SIMDATA, sent to
*  other cells, etc.  Accordingly, RF_IZH7X code caps the spike to
*  vPeak + u*umvp when it occurs, signals this fact by storing '1'
*  in the low-order bit of u, and performs the reset on the next
*  time step. This signal avoids having to calculate vPeak + u*umvp
*  twice on each time step (once with new u, once with old u).
*
*  Another problem is that 16-bit s(i) and u(i) values do not preserve
*  enough precision to replicate E.I.'s examples.  Accordingly, for
*  these response functions, dynamic variable IZVU is allocated, where
*  both s(i) and u(i) can be stored as si32 values.  This is forced
*  on a word boundary by d3allo.  The value of s(i) of course is
*  truncated to 16 bits for transmission to other cells.
*
*  A third problem is that when OPTMEM is in effect and any of a,b,c,d
*  are variable, the seed updates must occur in a,b,c,d order, even
*  though only one pair of a,b or c,d is accessed in any one cycle.
*  Retain only the same number of bits as the si16 stored values have.
*
*  d3news() sets il->rspmethu according to how much calculation is
*  actually needed.  Cases RF_IZH3,RF_IZH7 are the simplest, fastest
*  implementations with no extra features.  Case RF_IZH3X adds vari-
*  able a,b,c,d to the basic RF_IZH3.  Case RF_IZHIX handles all the
*  variants described in the book except multicompartment cells.
*
*  In all cases, the CDAT.events 'SPIKED' bit is set on the down-
*  stroke.  This is used by d3dprt to print c,d rather than a,b
*  and also causes a spike to be counted in the statistics.
*
*  Many aspects of this function are redundant with the depression
*  and variable spike threshold functions also coded here--normally
*  those options should be left turned off.  */

case RF_IZH7X:  {          /* Complete 2007 version */
            struct IZ07DEF *pi7 = (struct IZ07DEF *)il->prfi;
            d3gti4(CDAT.old_si, pizv);    /* S20/27 */
            d3gti4(CDAT.izhu, pizu);      /* S22/29 */
            /* Perform reset if a spike was stored last time */
            if (CDAT.izhu & IZ_SPIKED) {
               si32 tt, c = pi7->Z.izc, d = pi7->Z.izd;
               CDAT.events |= SPIKED;
               if (il->ctf & CTFRANOS)
                  udevskip(&lzseed, (si32)il->nizvab);
               if (il->nizvcd) {
                  if (il->Ct.ctopt & OPTMEM) {
                     /* N.B.  Must discard bits to bring precision to
                     *  same number of bits saved when not OPTMEM.  */
                     if (pi7->Z.izrc) {   /* Use individual cell c */
                        tt = udev(&lzseed) << 1;
                        tt = mssle(tt, abs32(tt), -31, OVFI);
                        tt = mssle(tt, (si32)pi7->Z.izrc, -31, OVFI);
                        /* S31 + 7/14 - 31 + (20-7) ==> S20/27 */
                        c += tt << (FBwk-FBsi); }
                     if (pi7->Z.izrd) {   /* Use individual cell d */
                        tt = udev(&lzseed) << 1;
                        tt = mssle(tt, abs32(tt), -31, OVFI);
                        tt = mssle(tt, (si32)pi7->Z.izrd, -31, OVFI);
                        /* S31 + 7/14 - 31 + (22-7) ==> S22/29 */
                        d += tt << (FBIu-FBsi); }
                     }
                  else {                  /* Variable c,d in mem */
                     rd_type *prr = plsd + il->ctoff[IZRA] +
                        il->nizvab * SHSIZE;
                     if (pi7->Z.izrc) {   /* Use individual cell c */
                        d3gth2(tt, prr); prr += SHSIZE;
                        /* S7/14 + 16 (d3gth2) - 3 ==> S20/27 */
                        c += SRA(tt,FBrs-FBwk); }
                     if (pi7->Z.izrd) {   /* Use individual cell d */
                        d3gth2(tt, prr);
                        /* S7/14 + 16 (d3gth2) - 1 ==> S22/29 */
                        d += SRA(tt, 1); }
                     }
                  }
               if (pi7->umc) c +=
                  msrsle(pi7->uumctCm, CDAT.izhu, FBwk-FBIu-24, OVFI);
               response = CDAT.izhc = c;
               CDAT.izhu += CDAT.izhd = d;
               if (CDAT.izhu > pi7->uumaxoCm)
                  CDAT.izhu = pi7->uumaxoCm;
               CDAT.izhu &= ~IZ_SPIKED;
               } /* End spike termination action */
            else {                     /* Not a spike last time */
               si64 t64 = jmsw(CDAT.old_si, CDAT.old_si - pi7->izvt);
               si32 a,b,tt,wuv3,simvur = CDAT.old_si - pi7->Z.vur;
               /* If calc si*(si-vt) as only 32 bits, must shift away
               *  an extra 5 to avoid overflow on max pos si, then lose
               *  most precision if si is very small, so calc 64.
               *  Here S20/27 + 20/27 + 30/23 - 50 ==> S20/27 */
               t64 = msrswe(t64, pi7->ukoCm, -50, OVFI);
               tt = (si32)(swlo(t64));
               if (swhi(t64) != SRA(tt,(BITSPERUI32-1)))
                  e64act("d3go", OVFI);
               response = CDAT.old_si + tt -
                  SRA(CDAT.izhu, FBIu-FBwk) + respnois;
               a = pi7->Z.iza;
               /* Note:  Even if using bvlo, still must update
               *  lzseed if b is variable and in OPTMEM mode,
               *  so do not jump around update coede below  */
               if (simvur < 0)      /* Use b,uv3 for negative s(i) */
                  b = pi7->ubvlooCm, wuv3 = pi7->uuv3looCm;
               else                 /* Use normal b,uv3 */
                  b = pi7->Z.izb, wuv3 = pi7->uuv3oCm;
               if (il->nizvab) {
                  if (il->Ct.ctopt & OPTMEM) {
                     /* N.B.  Must discard bits to bring precision to
                     *  same number of bits saved when not OPTMEM.  */
                     if (pi7->Z.izra) {   /* Use individual cell a */
                        tt = udev(&lzseed) << 1;
                        tt = mssle(tt, (si32)pi7->Z.izra, -31, OVFI);
                        /* S31 + 14 - 31 + 14 ==> S28 */
                        a += tt << 14; }
                     if (pi7->Z.izrb) {   /* Use individual cell b */
                        tt = udev(&lzseed) << 1;
                        if (simvur >= pi7->usmvtest) {
                           tt = mssle(tt,(si32)pi7->Z.izrb,-31,OVFI);
                           /* S31 + 14 - 31 + 14 ==> S28 */
                           b += tt << 14; }
                        }
                     }
                  else {
                     rd_type *prr = plsd + il->ctoff[IZRA];
                     if (pi7->Z.izra) {   /* Use individual cell a */
                        d3gth2(tt, prr); prr += SHSIZE;
                        /* S14 + 16 (d3gth2) - 2 ==> S28 */
                        a += SRA(tt,2); }
                     if (simvur >= pi7->usmvtest) {
                        /* Use individual cell b */
                        d3gth2(tt, prr);
                        /* S14 + 16 (d3gth2) - 2 ==> S28 */
                        b += SRA(tt,2); }
                     }
                  }
               if (il->ctf & CTFRANOS)
                  udevskip(&lzseed, (si32)il->nizvcd);
               CDAT.izha = a;
               CDAT.izhb = b;
               /* E.I. uses oldsi here, not new response */
               tt = mssle(b, simvur, FBIu-FBIab-FBwk, OVFI) - CDAT.izhu;
               if (wuv3) {             /* Need v-cubed term */
                  si64 t64 = jmsw(simvur, simvur);
                  t64 = msswe(t64, simvur, 2*isqis, OVFI);
                  t64 = msswe(t64, wuv3, FBIu-FBwk-36, OVFI);
                  t64 = jasl(t64, tt);
                  t64 = msrswe(t64, a, -FBIab, OVFI);
                  tt = (si32)(swlo(t64));
                  if (swhi(t64) != SRA(tt,(BITSPERUI32-1)))
                     e64act("d3go", OVFI);
                  CDAT.izhu += tt;
                  }
               else
                  CDAT.izhu += msrsle(tt, a, -FBIab, OVFI);
               CDAT.izhu &= ~IZ_SPIKED;
               /* Now cap the response as commented above */
               tt = pi7->Z.vpeak;
               if (pi7->uumvptCm) tt += msrsle(pi7->uumvptCm,
                  CDAT.izhu, FBwk-FBIu-24, OVFI);
               if (response > tt) {
                  CDAT.izhu |= IZ_SPIKED;
                  response = tt;
                  }
               }
            break;   /* Skip old-style decay */
            } /* End RF_IZHIX local scope */

case RF_IZH3X:  {          /* 2003 version with variable a,b,c,d */
            struct IZ03DEF *pi3 = (struct IZ03DEF *)il->prfi;
            si32 aseed,bseed,b,tt,wksi;
            d3gti4(CDAT.old_si, pizv);    /* S20/27 */
            d3gti4(CDAT.izhu, pizu);      /* S22/29 */

            if (il->ctf & CTFRANOS) {     /* Save a,b seeds */
               if (pi3->Z.izra) aseed = udev(&lzseed);
               if (pi3->Z.izrb) bseed = udev(&lzseed);
               }

            /* We (may) need b to calculate i3vr */
            b = pi3->Z.izb;
            if (pi3->Z.izrb) {   /* Cells have individual b values */
               if (il->Ct.ctopt & OPTMEM) {  /* Not in memory */
                  /* N.B.  Must discard bits to bring precision to
                  *  same number of bits saved when not OPTMEM.  */
                  tt = mssle(bseed<<1, (si32)pi3->Z.izrb, -31, OVFI);
                  /* S31 + 14 - 31 + 14 ==> S28 */
                  b += tt << 14; }
               else {                        /* Indiv b in memory */
                  int qzra = pi3->Z.izra != 0;
                  rd_type *prb = plsd + il->ctoff[IZRA] + SHSIZE*qzra;
                  d3gth2(tt, prb);
                  /* S14 + 16 (d3gth2) - 2 = S28 */
                  b += SRA(tt,2); }
               } /* End getting individual b */

            /* Adjust CNS s(i)(t-1) for 2003 model Vrest scale */
            if (il->ctf & CTFI3IVC)
               CDAT.i3vr = d3i3vr(pi3, b);
            else if (il->ctf & CTFI3IVR) {
               rd_type *pivr = pizu + I32SIZE;
               d3gti4(CDAT.i3vr, pivr); }
            else
               CDAT.i3vr = pi3->i3vr;
            wksi = CDAT.old_si + CDAT.i3vr;

            /* Perform reset if a spike was stored last time */
            if (wksi >= pi3->Z.vpeak) {
               si32 c = pi3->Z.izc, d = pi3->Z.izd;
               CDAT.events |= SPIKED;
               if (il->nizvcd) {
                  if (il->Ct.ctopt & OPTMEM) {
                     /* N.B.  Must discard bits to bring precision to
                     *  same number of bits saved when not OPTMEM.  */
                     if (pi3->Z.izrc) {   /* Use individual cell c */
                        tt = udev(&lzseed) << 1;
                        tt = mssle(tt, abs32(tt), -31, OVFI);
                        tt = mssle(tt, (si32)pi3->Z.izrc, -31, OVFI);
                        /* S31 + 7/14 - 31 + (20-7) ==> S20/27 */
                        c += tt << (FBwk-FBsi); }
                     if (pi3->Z.izrd) {   /* Use individual cell d */
                        tt = udev(&lzseed) << 1;
                        tt = mssle(tt, abs32(tt), -31, OVFI);
                        tt = mssle(tt, (si32)pi3->Z.izrd, -31, OVFI);
                        /* S31 + 7/14 - 31 + (22-7) ==> S22/29 */
                        d += tt << (FBIu-FBsi); }
                     }
                  else {                  /* Variable c,d in mem */
                     rd_type *prr = plsd + il->ctoff[IZRA] +
                        il->nizvab * SHSIZE;
                     if (pi3->Z.izrc) {   /* Use individual cell c */
                        d3gth2(tt, prr); prr += SHSIZE;
                        /* S7/14 + 16 (d3gth2) - 3 ==> S20/27 */
                        c += SRA(tt,FBrs-FBwk); }
                     if (pi3->Z.izrd) {   /* Use individual cell d */
                        d3gth2(tt, prr);
                        /* S7/14 + 16 (d3gth2) - 1 ==> S22/29 */
                        d += SRA(tt, 1); }
                     }
                  }
               response = CDAT.izhc = c;
               CDAT.izhu += CDAT.izhd = d;
               } /* End spike termination action */
            else {                     /* Not a spike last time */
               si32 a = pi3->Z.iza;
               /* S20/27+S30/23-26 = S24 */
               tt = mssle(wksi, pi3->izcv2,
                  FBsc-30-FBwk, OVFI) + pi3->izcv1p1;
               tt = msrsle(wksi, tt, -FBsc, OVFI) + pi3->izcv0;
               /* Vm(t-1) is in tt via izcv1p1, unlike IZHI7 code */
               response = tt - SRA(CDAT.izhu, FBIu-FBwk) + respnois;
               if (pi3->Z.izra) {
                  if (il->Ct.ctopt & OPTMEM) {
                     /* N.B.  Must discard bits to bring precision to
                     *  same number of bits saved when not OPTMEM.  */
                     tt = mssle(aseed<<1, (si32)pi3->Z.izra, -31, OVFI);
                     /* S31 + 14 - 31 + 14 ==> S28 */
                     a += tt << 14; }
                  else {
                     rd_type *prr = plsd + il->ctoff[IZRA];
                     d3gth2(tt, prr); prr += SHSIZE;
                     /* S14 + 16 (d3gth2) - 2 ==> S28 */
                     a += SRA(tt,2); }
                     }
               if (il->ctf & CTFRANOS)
                  udevskip(&lzseed, (si32)il->nizvcd);
               CDAT.izha = a;             /* For detail print */
               CDAT.izhb = b;
               /* E.I. uses newsi here in 2003, oldsi in 2007 */
               tt = mssle(b, response - pi3->Z.vur,
                  FBIu-FBIab-FBwk, OVFI) - CDAT.izhu;
               CDAT.izhu += msrsle(tt, a, -FBIab, OVFI);
               /* Now cap the response as commented above */
               if (response > pi3->Z.vpeak) response = pi3->Z.vpeak;
               }
            response -= CDAT.i3vr;        /* Back to CNS scale */
            break;   /* Skip old-style decay */
            } /* End RF_IZH3X local scope */

case RF_IZH7:  {           /* Faster version, no extras (except vur) */
            struct IZ07DEF *pi7 = (struct IZ07DEF *)il->prfi;
            d3gti4(CDAT.old_si, pizv);    /* S20/27 */
            d3gti4(CDAT.izhu, pizu);      /* S22/29 */
            /* Perform reset if a spike was stored last time */
            if (CDAT.old_si >= pi7->Z.vpeak) {
               CDAT.events |= SPIKED;
               response = pi7->Z.izc;
               CDAT.izhu += pi7->Z.izd;
               } /* End spike termination action */
            else {                        /* Not a spike last time */
               si64 t64;
               si32 tt,simvur = CDAT.old_si - pi7->Z.vur;
               /* S20/27 + 20/27 + 30/23 - 50 ==> S20/27 */
               t64 = jmsw(CDAT.old_si, CDAT.old_si - pi7->izvt);
               t64 = msrswe(t64, pi7->ukoCm, -50, OVFI);
               tt = (si32)(swlo(t64));
               if (swhi(t64) != SRA(tt,(BITSPERUI32-1)))
                  e64act("d3go", OVFI);
               response = CDAT.old_si + tt -
                  SRA(CDAT.izhu, FBIu-FBwk) + respnois;
               /* E.I. uses oldsi here, not new response */
               tt = mssle(pi7->Z.izb, simvur,
                  FBIu-FBIab-FBwk, OVFI) - CDAT.izhu;
               CDAT.izhu += msrsle(tt, pi7->Z.iza, -FBIab, OVFI);
               /* Now cap the response as commented above */
               if (response > pi7->Z.vpeak) response = pi7->Z.vpeak;
               }
            break;   /* Skip old-style decay */
            } /* End RF_IZH7 local scope */

case RF_IZH3:  {           /* 2003 version of the model */
            struct IZ03DEF *pi3 = (struct IZ03DEF *)il->prfi;
            si32 wksi;
            d3gti4(CDAT.old_si, pizv);    /* S20/27 */
            d3gti4(CDAT.izhu, pizu);      /* S22/29 */
            /* Adjust CNS s(i) for 2003 model Vrest scale, which,
            *  in RF_IZH3 case, is constant in pi3->i3vr.  */
            wksi = CDAT.old_si + pi3->i3vr;
            /* Perform reset if a spike was stored last time */
            if (wksi >= pi3->Z.vpeak) {
               CDAT.events |= SPIKED;
               response = pi3->Z.izc;
               CDAT.izhu += pi3->Z.izd;
               } /* End spike termination action */
            else {
               /* S20/27+S30/23-26 = S24 */
               si32 tt = mssle(wksi, pi3->izcv2,
                  FBsc-FBod-FBwk, OVFI) + pi3->izcv1p1;
               tt = msrsle(wksi, tt, -FBsc, OVFI) + pi3->izcv0;
               /* Vm(t-1) is in tt via izcv1p1, unlike IZHI7 code */
               response = tt - SRA(CDAT.izhu, FBIu-FBwk) + respnois;
               /* E.I. uses newsi here in 2003, oldsi in 2007 */
               tt = mssle(pi3->Z.izb, response - pi3->Z.vur,
                  FBIu-FBIab-FBwk, OVFI) - CDAT.izhu;
               CDAT.izhu += msrsle(tt, pi3->Z.iza, -FBIab, OVFI);
               /* Now cap the response as commented above */
               if (response > pi3->Z.vpeak) response = pi3->Z.vpeak;
               }
            response -= pi3->i3vr;     /* Back to CNS scale */
            break;   /* Skip old-style decay */
            } /* End RF_IZH3 local scope */

/* Brette-Gerstner (aEIF) function -- taken from R. Brette and W.
*  Gerstner, J. Neurophysiol. 94:3637-3642 (2005).  The Izhikevich
*  IZVU variables are also used with this function.  The exponential
*  is approximated by exp(y) ~ 2^k * (1 + f - c) where k = int(y/ln2)
*  and f = frac(y/ln2) as suggested by N. N. Schraudolph */

case RF_BREG:  {
            struct BREGDEF *pbg = (struct BREGDEF *)il->prfi;
            d3gti4(CDAT.old_si, pizv);
            d3gti4(CDAT.izhu, pizu);
            /* Determine whether a spike was stored last time */
            if (CDAT.old_si >= pbg->vPeak) {
               CDAT.events |= SPIKED;
               response = pbg->vReset;
               CDAT.izhu += pbg->ubgb;
               } /* End spike termination action */
            else {
               /* Calculate new response = s(i) and cap it.  If the
               *  exponential term gets big enough to overflow,
               *  declare a spike.  */
               si32 tt = CDAT.old_si - pbg->vT;
               if (tt > pbg->uspthr) response = pbg->vPeak;
               else {
#if 0    /* Test code--use library exponential */
                  float x = expf((float)tt/(float)pbg->delT);
                  x *= (float)pbg->ugLdToCm;
                  tt = (si32)x;
#else
                  si32 y,k;
                  /* S20 + 28 - 24 = S24 */
                  y = msrsle(tt, pbg->u1oln2dT, -24, OVF_BREG);
                  k = SRA(y,24);    /* Exponent of 2 */
                  /* Add 1 (S24) - (c = 45799 (S20)) to the fraction,
                  *  then multiply by gL*delT/Cm before scaling.  The
                  *  scaling shift and k shift can then be combined,
                  *  assuring that the shift is always to the right
                  *  (if uspthr test passed).  msrsle does not detect
                  *  shift > 64, so test for it here.  */
                  if (k <= -40) tt = 0;
                  else {
                     y = (y & (S24-1)) + 16044432;
                     tt = msrsle(y, pbg->ugLdToCm, -(24-k), OVF_BREG);
                     }
#endif
                  /* ugLoCm = 1-gL/Cm, so includes old_si in response */
                  response = tt - CDAT.izhu + respnois +
                     msrsle(CDAT.old_si, pbg->ugLoCm, -FBIab, OVF_BREG);
                  if (response > pbg->vPeak) response = pbg->vPeak;
                  }
               /* Calculate new w */
               tt = mssle(CDAT.old_si, pbg->ubga, -FBIab, OVF_BREG) -
                  CDAT.izhu;
               CDAT.izhu += msrsle(tt, pbg->u1otW, -FBIab, OVF_BREG);
               }
            break;   /* Skip old-style decay */
            } /* End RF_BREG local scope */

/* SPIKE function--fire a spike if afference plus noise is above
*  firing threshold  */

case RF_SPIKE:
            if (respnois >= wkst) {
               response = il->Ct.sspike;
               CDAT.events |= SPIKED;  /* Not currently used */
               } /* End spike threshold exceeded */
            /* Input does not exceed firing threshold */
            else {
               tsimr = oldsi;          /* Protect oldsi */
               response = d3decay(&il->Dc.CDCY, &tsimr,
                  respnois, &CDAT.pers);
               CDAT.events |= decay_bit;
               } /* End spike threshold not exceeded */
            break;

/* Hyperbolic tangent function.  Note that separate scales for map-
*  ping +/- afferences to tanh args are not provided--user can use
*  connection-type and noise scales for this purpose. Also Note that
*  because a transcendental function is involved, results are not
*  guaranteed to be identical on all machine architectures.  */

case RF_TANH:
            respnois = (long)(il->ctwk.go.fstanh *
            tanhf((float)respnois/dS27));

            /* Fall through to traditional case...no break */

/* Traditional piecewise-linear response */

case RF_STEP:
case RF_KNEE:
            tsimr = oldsi;             /* Protect oldsi */
            response = d3decay(&il->Dc.CDCY, &tsimr,
               respnois, &CDAT.pers);
            CDAT.events |= decay_bit;
            break;

            } /* End rspmeth switch */
         } /* End not refractory */

/* Generate and apply depression term.  Note that a spike can
*  now be subject to depression--prior to V8A, it could not.
*  This allows simulation of long-term homosynaptic depression
*  similar to that observed in the Aplysia system.  Also note
*  that resting potential is not included in calc of depression.
*
*  V8C, 08/03/03, GNR - To save significance, scale tsimrt*upsd
*  rather than tsimrt before multiplying.  With new or old scale,
*  we have S20/27 (tsimrt) + S27/20 (upsd) + (9 - 32) (mssle)
*  - 16 (final scale) --> S8 (depression).  OLDNSFRT will not
*  give same answer, because shift in jm64sh is SRA type.
*
*  Rev, 12/31/08, GNR - Don't round twice--shift depression*omega
*  only to S24 and include in random rounding term.  */

      if (il->ctf & CTFDN) {
         long depression,tsimrt;
         /* Fetch depression from data structure */
         d3gtl1(depression, plsd+il->ctoff[DEPR]);
         /* Update depression with random rounding and store
         *  in CDAT for stats before capping.
         *  Note:  Old NSF proposal used (oldsi-rt) in place
         *  of oldsi > 0 here.  This formula was removed:
         *  it was incorrect in that it failed to test for
         *  negative s(i)-rt or use SRA.  */
         tsimrt = (oldsi > 0) ? oldsi : 0;
         CDAT.depr = depression = (
            mssle(depression, il->Dp.omegad, -14, OVF_RFN) +
            mssle(tsimrt, il->Dp.upsd, -23, OVF_RFN) +
            (rnd3>>15)) >> Ss2hi;
         /* Don't let stored value exceed 255--but leave it in
         *  CDAT.depr for printing so user can see the problem */
         if (depression > 255) depression = 255;
         /* Depress cell response, s(depr) = s(calc)*phi(D).
         *  S20+S8=S28 and scale back to S20. */
         if (depression) response = msrsle(response,
            (si32)(ui32)ctab[depression], -8, OVF_RFN);
         /* Store new value back into data structure */
         d3ptl1(depression, plsd+il->ctoff[DEPR]);
         }  /* End of depression */

/* If dst exists and update was requested above, update, round, scale
*  (first to S20, so large values can be capped, then to S23), treating
*  128 mV as 1.0 old-style, then store hi result, giving effective S7
*  in memory.  The previous value, which is the one used to evaluate
*  new_si, is left in CDAT.dst for dprnt.  rnd2 is shifted to amount
*  to (1/2) when wkdst is shifted and high order stored.
*
*  Rev, 12/31/08, GNR - Values large enough to set the sign bit are
*  now trapped as overflows--old code would have incorrectly stored
*  them as timers.  */

      if (prf) {
         if (CDAT.events & UPDTDST) {
            wkdst = mssle(wkdst, prf->omegadst, -FBod, OVF_RFN) +
               ((response > 0) ? mssle(response, prf->upsdst,
                  -Ss2hi, OVF_RFN) : 0) + (rnd2>>18);
            if (wkdst >= S28) wkdst = (S28-1);
            }
         if (wkdst > 0) wkdst <<= 3;   /* Not a timer */
         d3pth2(wkdst, plsd+il->ctoff[DST]);
         } /* End dst update */

/* If this is an Izhikevich or Brette-Gerstner cell type, save 32-bit
*  values of response and u for next cycle.  This is done after
*  depression is applied.  */

      if (il->Ct.rspmeth >= RF_BREG) {
         d3pti4(response, pizv);
         d3pti4(CDAT.izhu, pizu);
         }

/* If doing autoscaling, find the ngtht highest responses.  If this
*  is a parallel computer, must do this on all nodes (conceivably,
*  all could end up on the same node) and collect on node 0 later.
*  Work space is allocated for this at CDAT.prgtht by d3nset.  */

      if (il->kautu & KAUT_NOW) d3assv(il->pauts, (si32)response);

/* Round, scale to S7, and store final s(i) value.  If necessary,
*  replace response by max response that will fit in two bytes.
*  If using old s(i) range, negative s(i) is not allowed, but
*  we now allow s(i) == 1 (S14), previously not representable. */

      if (il->ctf & OLDSILIM) {
         if (response > S27)     response = S27, sovrinc = 1;
         else if (response < 0)  response = 0;
         }
      CDAT.new_si = response;    /* Save S20 response */

      {  long respS7 = (labs(response) +
            (1L << (FBwk-FBsi-1))) >> (FBwk-FBsi);
         if (respS7 > SHRT_MAX)  respS7 = SHRT_MAX, sovrinc = 1;
         if (response < 0) respS7 = -respS7;
         d3ptl2(respS7, psnew);

/* Calculate and store updated mean response (qbar and sbar).  In V7B,
*  this code was changed to perform random-rounding of the new sbar
*  with il->rsd.  This avoids stationary sbar values that arise with
*  fixed rounding, e.g.  with no input, integer sbar can never decay
*  below 1/(2*sdamp).  Changed again at V8D to use simpler signed
*  rounding algorithm.  There is no check for overflow because sdamp
*  is checked at input time and respS7 is limited to two bytes in code
*  just above.  In V8G, these updates are deferred in KAUT_APP mode
*  to the rescaling in d3autsc().  */

         if (il->ksqbup & KSQUP_GO) {
            if (ctexists(il,SBAR)) {
               long sbar1;
               if (il->ctf & CTFDOFS)
                  sbar1 = respS7;
               else {
                  sbar1 = CDAT.sbar*(long)il->ctwk.go.sdampc +
                     respS7*(long)il->Dc.sdamp + (rnd1 >> Ss2hi);
                  sbar1 = SRA(sbar1, FBdf);
                  }
               d3ptl2(sbar1, plsd+il->ctoff[SBAR]);
               }
            if (ctexists(il,QBAR)) {
               long qbar1;
               if (il->ctf & CTFDOFS)
                  qbar1 = respS7;
               else {
                  qbar1 = CDAT.qbar*(long)il->ctwk.go.qdampc +
                     respS7*(long)il->Dc.qdamp + (rnd1 & (S15-1));
                  qbar1 = SRA(qbar1, FBdf);
                  }
               d3ptl2(qbar1, plsd+il->ctoff[QBAR]);
               }
            } /* End if KSQUP_GO */
         } /* End scope of respS7 */

      if (il->phshft) psnew[LSmem] = CDAT.new_phi;
      hitht = response >= il->Ct.ht;

/*---------------------------------------------------------------------*
*  Accumulate response statistics if this is a statistical cycle       *
*---------------------------------------------------------------------*/

      if (statenbc) {
         ddist *pdd = il->pctddst;  /* Ptr to double stats */

         /* Accumulate hit and truncation sums.  (This method
         *  eliminates multiple 'if (statenbc)'s in code above.) */
         il->CTST->hitpos += hitpt;
         il->CTST->hitneg += hitnt;
         il->CTST->sover  += sovrinc;
         if (CDAT.events & REFRAC) il->CTST->nrfrc++;
         if (CDAT.events & SPIKED) il->CTST->nspike++;

         if (cellaffs >= 0) {
            /* Positive afference--accumulate sum and count */
            il->CTST->sumpos = jasl(il->CTST->sumpos, cellaffs);
            il->CTST->numpos++;
            /* Record highest score */
            if (cellaffs > il->CTST->hiscor)
               il->CTST->hiscor = cellaffs;
            }
         else {
            /* Negative - accumulate sum and count */
            il->CTST->sumneg = jasl(il->CTST->sumneg, cellaffs);
            il->CTST->numneg++;
            /* Record lowest score */
            if (cellaffs < il->CTST->loscor)
               il->CTST->loscor = cellaffs;
            }

         /* Accumulate average response and hit data for
         *  sharpness and cross-response statistics.  */
         il->CTST->sumrsp = jasl(il->CTST->sumrsp, response);
         if (hitht) {
            il->CTST->sharp = jasl(il->CTST->sharp, response);
            il->CTST->nsharp++;
            if (marking) bytior(plsd + il->ctoff[XRM],
               (long)il->lmm, pmark);
            }

         /* Record response history data by modality */
         if (il->Ct.kctp & KRPHR) {
            hist *phist = il->phrs;
            int imnum,himnum = 1 << il->nmdlts;
            int khist = (1 - hitht) +
               ((oldsi < il->Ct.ht) << 1);
            for (imnum=1; imnum<himnum; imnum<<=1,++phist)
               ++phist->h[khist +
                  (((il->ctwk.go.delmk & imnum) != 0) << 2)];
            }

         /* Accumulate distribution of s(i) and variables keyed to
         *  s(i).  Due to the larger possible range of s(i) starting
         *  with V8C, these histograms are now based on a log scale,
         *  separately for hyperpolarizing and depolarizing responses.
         *  N.B. In KAUT_IMM mode, statistics are done on the unscaled
         *  responses, basically because S,G,M connection sums are not
         *  saved across cells, and in OLDRANGE mode, responses are
         *  left untruncated for more accurate scaling later.  Ergo,
         *  we still must correct negative irb in OLDRANGE mode.  */
         if (RP->compat & OLDRANGE) {
            irb = response >> (FBwk+3);
            if (irb > (LDSTAT-1)) irb = LDSTAT-1;
            else if (irb < 0) irb = 0;
            }
         else if (response >= 0) {
            irb = (LDSTAT/2) + bitsz((response>>(FBwk-1))+1);
            if (irb >= LDSTAT) irb = LDSTAT - 1;
            }
         else {
            irb = (LDSTAT/2) - bitsz((labs(response)>>(FBwk-1))+1);
            if (irb < 0) irb = 0;
            }

         if (irb > ihighrb) ihighrb = irb;
         il->CTST->sdist[irb] += 1;

         /* Accumulate s(i) dist by stimulus.  One distribution
         *  is allocated (see d3nset) for each distinct stimulus
         *  in each modality the celltype responds to.  The code
         *  here must enter a count in the distribution for each
         *  stimulus that is present in this cycle, as recorded
         *  in pmark.  Most bits in pmark are 0, so a plain loop
         *  is quite inefficient.  One could make a list of stim
         *  at celltype setup time, or use a machine instruction
         *  to find next nonzero bit.  The compromise code here
         *  skips over empty bytes and needs no setup.  */
         if (il->Ct.kctp & KRPCL) {
            dist *pCdist = il->pctdist + il->oCdist;
            int ibyt,ibit,ibit2;
            for (ibyt=ibit=0; ibyt<(int)il->lmm; ibyt++) {
               ibit2 = ibit + 8;
               if (pmark[ibyt]) for ( ; ibit<ibit2; ibit++) {
                  if (bittst(pmark, ibit+1))
                     pCdist[ibit].d[irb] += 1;
                  } /* End bit loop */
               else ibit = ibit2;
               } /* End byte loop */
            } /* End recording Cdist stats */

         /* Accumulate "additional group statistics" (actually min
         *  and max responses by stimulus number).  One GPSTAT is
         *  allocated for each distinct stimulus in each modality
         *  the celltype responds to.  The code here does not need
         *  to know which modality is which--it just needs to check
         *  all the pmark bits in turn and add into the stats when
         *  a stimulus is found.  See comments above re KRPCL.  */
         if (il->Ct.kctp & (KRPGP|KRPMG)) {
            struct GPSTAT *pgp;
            int ibyt,ibit,ibit2;
            for (ibyt=ibit=0; ibyt<(int)il->lmm; ibyt++) {
               ibit2 = ibit + 8;
               if (pmark[ibyt]) for ( ; ibit<ibit2; ibit++) {
                  if (bittst(pmark, ibit+1)) {
                     pgp = pgp0 + ibit;
                     pgp->nhits += hitht;
                     pgp->hiresp = max(pgp->hiresp, response);
                     pgp->loresp = min(pgp->loresp, response);
                     llohi = max(llohi, response);
                     }
                  } /* End bit loop */
               else ibit = ibit2;
               } /* End byte loop */
            } /* End recording GPSTAT stats */

         /* Accumulate phasing statistics */
         if (il->phshft) {
            dist *pFdist = il->pctdist + il->oFdist;
            int dif = abs((int)CDAT.new_phi-(int)CDAT.old_phi);
            pFdist[OcFd].d[(CDAT.new_phi>>1)&15] += 1;
            /* Scrunch into 16 bins.  We want distance from zero
            *  on the circle, i.e. bins are labelled 0,+-1,+-2,
            *  ...,+-15,16.  Since we lack the 17th bin, we lump
            *  16 with +-15, giving:  0 1,31 2,30 ...  15-17 */
            if (dif == 16) dif = 15;
            dif = (dif > 15) ? PHASE_RES - dif : dif;
            pFdist[OcDFd].d[dif] += 1;
            }

         /* Accumulate distribution of Breg w = Izhi u variable */
         if (il->Ct.rspmeth >= RF_BREG) {
            pdd->d[irb] = jasl(pdd->d[irb], SRA(CDAT.izhu, FBIu-FBsi));
            ++pdd;
            }

         /* Accumulate distribution of persistence term (not IZHI) */
         else if (il->Dc.CDCY.kdcy) {
            if (CDAT.events & decay_bit)
               pdd->d[irb] = jasl(pdd->d[irb], CDAT.pers);
            ++pdd;
            }

         /* Accumulate depression statistics (S8) */
         if (il->ctf & CTFDN) {
            pdd->d[irb] = jasl(pdd->d[irb], CDAT.depr);
            ++pdd;
            }

         /* Accumulate dst (delta-spike-thresh) statistics.
         *  Use psdst when dst < 0 (acting as a clock).
         *  N.B.  Prior to V8C, this statistic was based on the
         *  newly calculated value of DST (wkdst). It is better
         *  to use the old dst (CDAT.dst), because that is the
         *  value actually used to calculate cell response.  */
         if (prf) {
            long tdst = CDAT.dst;
            if (tdst < 0) tdst = prf->psdst;
            pdd->d[irb] = jasl(pdd->d[irb], tdst);
            /* Increment pdd here if more stats added later... */
            }

         /* One last loop over connection types to accumulate
         *  distributions of afferent voltages, average Sj,
         *  connection decay, and s(i) vs FDM category.  */
         for (ix=il->pct1; ix; ix=ix->pct) {
            int zbict = (int)ix->ict - 1;
            pdd = il->pctddst + ix->ocnddst + OaVd;
            pdd->d[irb] = jasw(pdd->d[irb], CDAT.rsumaij[zbict]);
            if (ix->Cn.ADCY.kdcy) {
               long tax = 0;
               if (ix->cssck & PSP_POS)
                  tax += CDAT.pAffData[ix->oexafs];
               if (ix->cssck & PSP_NEG)
                  tax += CDAT.pAffData[ix->oinafs];
               ++pdd;
               pdd->d[irb] = jasl(pdd->d[irb], tax);
               }
            if (cnexists(ix, RBAR)) {
               rd_type *pnuk = ix->psyn0 + Lija.lcoff + ix->xnoff[NUK];
               long nuk;
               d3gtln(nuk, pnuk, ix->nuklen);
               if (nuk > 0) {
                  ++pdd;
                  pdd->d[irb] = jasl(pdd->d[irb],
                     jdswq(prbar[zbict], nuk));
                  }
               }
            if (ix->cnflgs & DOFDIS) {
               int iFd = ix->ucij.m.oFdist + icell%ix->ucij.m.nkt;
               il->pctdist[iFd].d[irb] += 1;
               }
            } /* End ix loop */

         /* Loop over GCONN blocks--Accumulate
         *  distributions of bandsums and persistence */
         {  struct INHIBBLK *ib;       /* Ptr to inhibition block */
         for (ib=il->pib1; ib; ib=ib->pib) {
            struct INHIBBAND *bnde, *bnd = ib->inhbnd;
            pdd = il->pctddst + ib->oidist;
            for (bnde=bnd+ib->nib; bnd<bnde; bnd++,pdd++) {
               pdd->d[irb] = jasl(pdd->d[irb],
#ifdef PAR
                  bnd->bandsum);
#else
                  bnd->bandsum[CDAT.groupn]);
#endif
               }  /* End loop over inhbbands */
            /* If there is decay, accumulate persistence */
            if (ib->GDCY.kdcy) {
               long *pgx = CDAT.pAffData + ib->ogafs;
               if (ib->gssck & PSP_POS) {
                  pdd->d[irb] = jasl(pdd->d[irb], pgx[1]);
                  pgx += 3; }
               if (ib->gssck & PSP_NEG)
                  pdd->d[irb] = jasl(pdd->d[irb], pgx[1]);
               }
            }} /* End ib loop and local scope */

         /* Accumulate distribution of modulation values and
         *  persistence.  For consistency with GCONNs, where
         *  decay is undefined for individual bands, use the
         *  value before decay also for this statistic.  */
         for (imb=il->pmby1; imb; imb=imb->pmby) {
            long  *pmx = CDAT.pAffData + imb->omafs;
            pdd = il->pctddst + imb->omdist;
            pdd[OmVd].d[irb] = jasl(pdd[OmVd].d[irb], pmx[0]);
            if (imb->MDCY.kdcy) pdd[OmWd].d[irb] =
               jasl(pdd[OmWd].d[irb], pmx[1]);
            }

         } /* End statistical section */

/* Record firing of cell for regeneration whether or not there
*  are any statistics.  */

      if ((il->Ct.ctopt & OPTRG) && hitht)
         *(plsd + il->ctoff[RGH]) |= 1;

/* If current cell has been listed for detailed print,
*  call d3dprt or d3pdpn to print the selected data.  */

      if (do_dprint && ilsttest(il->dpclb->pclil, icell)) {
#ifdef PAR
         d3pdpn();
#else
         d3dprt();
#endif
         }  /* End detail print */

/* Abort if any calculation overflowed for the current cell */

      if (RP->ovflow)
         d3exit(OVERFLOW_ERR, fmturlcn(il, icell), RP->ovflow);

      }  /* End loop over cells */

/* Accumulate number of cycles and distribution of highest responses */

   if (statenbc) {
      il->CTST->nstcap += 1;
      il->CTST->hdist[ihighrb] += 1;   /* Accumulate dist of
                                       *  highest responses */
      if (il->Ct.kctp & (KRPGP|KRPMG)) {
         struct GPSTAT *pgp;
         int ibyt,ibit,ibit2;
         for (ibyt=ibit=0; ibyt<(int)il->lmm; ibyt++) {
            ibit2 = ibit + 8;
            if (pmark[ibyt]) for ( ; ibit<ibit2; ibit++) {
               if (bittst(pmark, ibit+1)) {
                  pgp = pgp0 + ibit;
                  pgp->nstim += 1;
                  pgp->lohiresp = min(pgp->lohiresp, llohi);
                  }
               } /* End bit loop */
            else ibit = ibit2;
            } /* End byte loop */
         } /* End recording GPSTAT stats */
      il->ctf |= DIDSTATS;             /* This signal triggers
                                       *  stat printing */
      } /* End layerfin stats */

#endif  /*** END NODE ZERO EXCLUSION FROM LOOP OVER RESIDENT CELLS ***/

/*---------------------------------------------------------------------*
*                                                                      *
*        layerfin - All nodes layer finish routine                     *
*                                                                      *
*----------------------------------------------------------------------*
*  Resume normal indenting                                             *
*---------------------------------------------------------------------*/

/* Set flags that s(i) now done and Lij detail info printed.
*  Clear flags that request fast start rbar,sbar,qbar calculations.
*  (The extra ix loop here saves a double test in the inner loop.)  */

         il->ctf = (il->ctf & ~CTFDOFS) | DIDSCALC;
         if (do_dprint) il->dpitem |= DPDIDL;
         for (ix=il->pct1; ix; ix=ix->pct)
            ix->cnflgs &= ~CNDOFS;

#ifdef PAR
         /* Advance random num seeds to allow for cells on later nodes.
         *  Note: Following udevskips must execute on ALL nodes to keep
         *  node zero in step with comp nodes (for re-broadcast of tree
         *  at next CYCLE card).  Skip updating if processing was skip-
         *  ped by a jump to ADVLAYER, as was locell skipping above.
         *
         *  Rev, 02/17/97, GNR - phseed,rseed updates formerly in
         *  d3seed() were moved here to simplify code and assure con-
         *  sistent updating under all skip conditions, at cost of
         *  updating on each trial instead of once per CYCLE card.  */

         {  long nskip = il->nelt
#ifndef PAR0
               - celllim
#endif
               ;

            if (il->ctf & CTFHN) udevskip(&il->nsd, nskip<<1);
            udevskip(&il->rsd, nskip<<2);

            for (ix=il->pct1; ix; ix=ix->pct) {
               if (ix->kgfl & KGNBP) continue;
               if (ix->cnflgs & UPRSD)
                  udevskip(&ix->rseed, nskip*(long)ix->upcij1s);
               if (ix->phopt & PHASR)
                  udevskip(&ix->upphseed, nskip*ix->nc);
               } /* End updating conntype seeds */

            if (il->ctf & CTFNC)
               udevskip(&ppd->phiseed, nskip);

            } /* End scope of nskip */
#endif

         /* Determine and/or apply autoscale for next trial while
         *  response data are still available in CDAT.prgtht */
         if (il->kautu & (KAUT_NOW|KAUT_APP)) d3autsc(il);

#ifndef PAR0
         /* Add contribution of this celltype to superposition plot */
         if ((RP->kpl1 & (KPLSUP|KPLMFSP)) && (plot_cycle >= 0))
            d3lplt(il,il->ps2,plot_cycle,plvx,plvy,ir->aplw,ir->aplh);
#endif

/* End of what was layerfin routine */

/* This completes processing of one cell type--
*  Interchange s1,s2 for this and any 'UPWITH' layers.  */

ADVLAYER:                     /* Here if layer skipped altogether */

/* Write network data to SIMDATA file even if not updated */

         if (do_gfsv) {
            stattmr(OP_PUSH_TMR, GDFL_TMR);
#ifndef PARn
            d3gfsvct(RP0->pgds, il, ~(GFLIJ|GFDIJ|GFABCD));
#else
            if (RP->pgdcl) d3gfsvct(NULL, il, ~(GFLIJ|GFDIJ|GFABCD));
#endif
            stattmr(OP_POP_TMR, 0);
            }

         if (!(il->ctf & CTFDU)) {  /* If this layer is updated by
                                    *  another one, wait until it
                                    *  comes up. */
            /* Not updated by another layer. Run layerupd
            *  on any layers updated by this one. */
            for (jl=il; jl; jl=jl->pusrc)
                 layerupd(jl, cyclestogo);
            } /* End updating UPWITH layers */
         il->ctwk.go.ntca -= 1;
         }  /* End of CELLTYPE processing */
      }  /* End of processing loop over REPERTOIRES */

/* Perform KP=E (every cell) output in every cycle.  This
*  requires a separate repertoire loop so all deferred
*  (UPWITH) updates are completed before calling d3rpsi().  */

   if (RP->kpr & KPRREP)
      for (il=RP->pfct; il; il=il->pnct)
         if (il->Ct.kctp & KRPEP &&
               bittst(RP->CP.trtimer, (long)il->Ct.jketim))
            d3rpsi(il, (int)CDAT.curr_cyc1);

/* Collect categorization statistics */

#ifndef PARn
   if (RP0->n0flags & N0F_ANYKCG)
         for (il=RP->pfct; il; il=il->pnct) {
      if (il->ctf & CTFSTENB && il->orkam & KAMCG) {
         s_type *psi = il->pps[0];
         long icell,hii = 0;
         si32 wksi,hisi = -SI32_MAX;
         int  llsp = (int)il->lsp;
         for (icell=0; icell<il->nelt; psi+=llsp,++icell) {
            d3gts2(wksi, psi);
            if (wksi > hisi) hisi = wksi, hii = icell;
            }
         hii /= il->nel;   /* Get group number */
         for (ix=il->pct1; ix; ix=ix->pct) if (ix->Cn.kam & KAMCG) {
            struct MODALITY *pm = ix->psmdlt;
            if (bittst(RP->pbcst + RP->ombits,
                  (long)pm->mdltgoff + (hii % pm->ncs) + 1))
               ix->nccat += 1;
            }
         }
      } /* End categorization correct counting */
#endif

/* This completes one cycle--
*     Compute adaptive value of response
*     Allow decay and amplification to begin in 2nd cycle.  */

   if (RP->pvblkr) d3valu(RP->pvblkr);

   /* Resume decay, amplification, stats if adefer cycles have been
   *  completed.  If statistical freeze is on, use sequence II,
   *  otherwise activate full decay/amplification (sequence III).  */
   if (CDAT.curr_cyc1 >= RP->adefer) {
      if (RP->CP.runflags & RP_NOCHNG) sequence = SEQ_II;
      else sequence = SEQ_III;
      }

   if (--cyclestogo > 0) goto NEWCYCLE;

/*---------------------------------------------------------------------*
*             All cycles completed.  Finish up and return.             *
*---------------------------------------------------------------------*/

   return D3_NORMAL;
   }  /* End d3go() */


/***********************************************************************
*                                                                      *
*                    Miscellaneous local subroutines                   *
*                                                                      *
***********************************************************************/

/*---------------------------------------------------------------------*
*  cijclean routine--                                                  *
*     This routine incorporates actions required to clean up           *
*  amplified Cij values for storage, previously located at NULLAMP.    *
*  These actions have been moved to a subroutine so they can be used   *
*  on individual conns or on average Cij when NOPSAA in effect.        *
*  These actions include:                                              *
*     If new Cij is not out-of-bounds, perform random rounding.        *
*     If out-of-bounds, count overflows and truncate.                  *
*     Count sign change attempts and changes in value.                 *
*     Scale to S31 for storage.                                        *
*     Perform corresponding actions relating to Cij0.                  *
*  Location ND->narb contains a count of the number of equivalent      *
*  Cij being affected by this cleanup call.  This value is used to     *
*  increment relevant statistics.                                      *
*  V8D, 07/25/07, GNR - Remove nmod to pre-avg code in preparation     *
*    for complete rewrite of averaging.                                *
*---------------------------------------------------------------------*/

#ifndef PAR0
static void cijclean(struct CONNTYPE *ix, struct CONNDATA *ND) {

   register long work;           /* Various temporary values */
   long curr_cij = ND->curr_cij; /* Value of Cij being adjusted */
   long curr_cij0;               /* Value of Cij0 being adjusted */
   long trun_new_cij;            /* Truncated new Cij */
   long worksign;                /* Holds sign of curr_cij */
   int  nbcshft = ix->Cn.nbc+14; /* Shift for new Cij */

/* Random-round or truncate accord to whether new Cij is in bounds */

   if ((work=labs(curr_cij)) < ix->bigc16) {
      /* The new Cij is within truncation limits.
      *  Apply random rounding because Cij is highly quantized.
      *  A separate rounding seed is used to avoid changing any
      *  other random quantities.  Note that the rounding term
      *  is always positive because truncation always goes toward
      *  algebraically lower values.  A positive value just below
      *  bigc16 can be incremented to equal, but not exceed, it.  */
      curr_cij += ((unsigned long)udev(&ix->rseed)) >> nbcshft;
      }
   else {
      if (work > ix->bigc16)
         /* New Cij is absolutely out-of-bounds, count overflows */
         ix->CNST.cover += ND->narb;
      curr_cij = (curr_cij < 0) ? -ix->bigc16 : ix->bigc16;
      }

/* Handle sign changes--
*  Count all attempts to change sign of Cij
*  If sign change disallowed, set result to zero--
*     will get sign when stored below.  */

   worksign = curr_cij;    /* Hold sign in case set to zero */
   if ((ND->old_cij ^ curr_cij) < 0) {
      ix->CNST.cschg += ND->narb;
      if (ix->Cn.kam & KAMNC)
         { curr_cij = 0; worksign = ND->old_cij; }
      }

/* Convert new Cij back to S31 and return in ND structure.
*     If magnitude is zero, give it sign of new or old Cij
*        according to whether sign changes are allowed.
*     Code assumes overflowing Cij have already been reset.  */

   work = (curr_cij << 15) & ix->cmsk;
   trun_new_cij = SRA(work,15);
   if (work == 0) work = MINUS0 & worksign;
   ND->new_cij = work;

/* If baseline Cij is being maintained, update it now (S16).
*  (Setup code sets vrho to 0 if Cij0 not allocated.)  */

   if (ix->cnwk.go.vrho) {
      /* To avoid bias, use the rounded and truncated new Cij */
      curr_cij0 = ND->curr_cij0;
      curr_cij0 += msrsle(trun_new_cij - curr_cij0,
         ix->cnwk.go.vrho, -FBsc, OVF_AMP);
      /* Make sure |base_cij| < 1.0 (this is necessary
      *  because rho can in principle be > 1.0).  */
      if ((work=labs(curr_cij0)) < ix->bigc16)
         /* Now have the new base_cij (S16) and it did not
         *  overflow.  Apply random rounding as for curr_cij. */
         curr_cij0 += ((unsigned long)udev(&ix->rseed)) >> nbcshft;
      else
         /* base_cij did overflow.  Set to limiting value. */
         curr_cij0 = (curr_cij0 < 0) ? -ix->bigc16 : ix->bigc16;

      /* Enforce same sign change restriction as for Cij,
      *  except we don't have to worry about minus 0. */
      if (((ND->old_cij ^ curr_cij0) < 0) &&
         (ix->Cn.kam & KAMNC)) ND->new_cij0 = 0;
      else
         ND->new_cij0 = (curr_cij0 << 15) & ix->cmsk;
      }

   } /* End cijclean() */

/*---------------------------------------------------------------------*
*  sumaffs routine--                                                   *
*     Performs sum over relevant connection classes (specific, self,   *
*     voltage-dependent, geometric, modulatory) for one or more types  *
*     of connections (excitatory, hyperpolarizing, squared, or shunt)  *
*     as directed by a consolidation table prepared earlier.  Sums     *
*     will be over phase distributions if cells have phase, otherwise  *
*     over just single input accumulators.  Check for overflow.        *
*---------------------------------------------------------------------*/

static void sumaffs(struct CELLTYPE *il, long *totals) {

   if (il->phshft) {          /* Cells have phase */
      for ( ; *pcontbl > EndAllConsTbl; pcontbl++) {
         if (*pcontbl == EndConsTbl)
            totals += PHASE_RES;
         else {
            long *fa = CDAT.pAffData + *pcontbl;
            register int p;      /* Phase index */
            for (p=0; p<PHASE_RES; p++)
               addck(totals[p], fa[p], OVF_RFN);
            }
         } /* End loop over consolidation tables */
      } /* End sums for cells with phases */
   else {                     /* Cells do not have phase */
      for ( ; *pcontbl > EndAllConsTbl; pcontbl++) {
         if (*pcontbl == EndConsTbl)
            totals += 1;
         else {
            long *fa = CDAT.pAffData + *pcontbl;
            addck(*totals, *fa, OVF_RFN);
            }
         } /* End loop over consolidation tables */
      } /* End sums for cells with no phases */
   pcontbl++;              /* Advance to next consol. table */
   } /* End sumaffs */

/*---------------------------------------------------------------------*
*  sumrmod routine--                                                   *
*     Perform real modulation sums for this cell type based on new     *
*     values of s(i).  These sums are not needed on host.  This is     *
*     now a separate routine to avoid needing same code in d3genr().   *
*                                                                      *
*  Rev, 03/30/03, GNR - Use mddelay, may matter at restore time.       *
*  Rev, 04/24/08, GNR - Remove MOPABS, add mtlo.                       *
*  V8E, 05/06/09, GNR - New routine, add mjrev, obey STEP/KNEE option  *
*  V8H, 03/31/11, GNR - Pass curr_cyc1 as a parameter for defer check  *
*  Rev, 04/09/12, GNR - Defer test is '<', not '<=' (Si already calcd) *
*---------------------------------------------------------------------*/

void sumrmod(struct CELLTYPE *il, ui16 curr_cyc1) {

   struct MODVAL *imv;
   static si64 si64zero = {0};

   for (imv=il->pmrv1; imv; imv=imv->pmdv) {
      if (curr_cyc1 < imv->Mdc.mdefer)
         imv->umds.mdsum = si64zero;
      else {
         si64   wsum = {0};
         s_type *psi = il->pps[imv->Mdc.mdelay];
         s_type *pse = psi + il->lspt;
         si32   lmthi = (si32)imv->Mdc.mt;
         si32   lmtlo = (si32)imv->Mdc.mtlo;
         si32   lmjrv = (si32)imv->Mdc.mjrev;
         si32   lsmthi,lsmtlo,wksi;

         /* Set for STEP vs KNEE thresholds */
         if (imv->Mdc.mopt & IBOPKR)
            lsmthi = lmthi, lsmtlo = lmtlo;
         else
            lsmthi = lsmtlo = 0;
         /* Set to eliminate zero entries */
         if (lmthi) lmthi -= 1;
         if (lmtlo) lmtlo += 1;

         for ( ; psi<pse; psi+=il->lsp) {
            d3gts2(wksi, psi);
            wksi -= lmjrv;
            if      (wksi > lmthi) wsum = jasl(wsum, wksi - lsmthi);
            else if (wksi < lmtlo) wsum = jasl(wsum, wksi - lsmtlo);
            }
         imv->umds.mdsum = wsum;
         } /* End not deferred */
      } /* End loop over imv */
   } /* End sumrmod() routine */

#endif

/*---------------------------------------------------------------------*
*                                                                      *
*  layerupd routine--                                                  *
*     Do any layer finish stuff delayed to update time.                *
*     Exchange new s(i) and push old values back in delay pipeline.    *
*     Copy modality marker bits into delay pipeline if any responses.  *
*     Evaluate any dependent modulation values.                        *
*     On last cycle of a trial:                                        *
*        If any KAM=C conntypes, count correct categorizations.        *
*        Evaluate contributions to effector arbors.                    *
*  Revised, 02/24/94, GNR - Handle OPTFZ with delay by replicating     *
*     previous s(i) into delay pipeline.  Also, call layerupd even     *
*     if s(i) calc skipped due to OPTFZ so can include in changes.     *
*                                                                      *
*---------------------------------------------------------------------*/

static void layerupd(struct CELLTYPE *il, ui16 cyclestogo) {

   int    mxsd;               /* Max s delay */

/* If this cell type feeds any connections with delay, push
*  all s(i) values in pipeline back one time step, making room
*  for newest s(i) in current slot.  This is done by rotating
*  the pointer array at *pps.  This allows the pointer for delay
*  jd to be accessed directly at il->pps[jd] w/o modulo arith.
*  The bits marking modality responses are also implicitly shifted.
*/

   if ((mxsd = il->mxsdelay1 - 1) > 0) {
      s_type *pend = il->pps[mxsd];
      while (mxsd--) il->pps[mxsd+1] = il->pps[mxsd];
      il->pps[0] = pend;
      } /* End applying axonal delays */

/* Update s(i) vector */

#ifdef PAR
   /* Parallel update:  Exchange s(i) to all nodes.  If OPTFZ is
   *  active, this will pick up ps2 from previous time step and
   *  effectively replicate s(i) into new time slot.  There is no
   *  point to copy the s(i) separately, because the d3exch must
   *  be done in any event to prepare for changes calc on host.  */
   d3exch(il);
#else
   /* Serial update:  If new s(i) were calculated, just swap
   *  pointers so the new values become visible.  Otherwise,
   *  if there was delay, the previous s(i) must be copied
   *  into the new time slot.  */
   if (il->ctf & DIDSCALC) {
      s_type *psi = il->pps[0];
      il->pps[0] = il->ps2;
      il->ps2 = psi; }
   else if (il->mxsdelay1 > 1)
      memcpy((char *)il->pps[0], (char *)il->pps[1], il->lspt);
#endif

#ifndef PAR0
/* If masking, transfer modality marker bits from ps2 array to ps[0].
*  Note (10/29/00):  Previous versions tested here whether any cells
*  responded above hit threshold in the last cycle, and, if not,
*  zeroed out the marker bits instead of copying them.  This code was
*  removed because there is no reason a cell responding below ht
*  could not influence a cell in a downstream repertoire.  If this
*  type of test is wanted, it needs to be much more sophisticated,
*  e.g. it must look at the connectivity matrix.  */

   if (il->ctwk.go.masking) {
#ifndef PAR
      if (!(il->ctf & DIDSCALC))
#endif
      memcpy((char *)il->pps[0] + il->lspt, (char *)il->ps2 +
         spsize(il->mcells,il->phshft), (size_t)il->lmm);
      } /* End if masking */

/* Evaluate modulation contributions from this layer */

   if (il->pmrv1 && il->ctf & DIDSCALC)
      sumrmod(il, CDAT.curr_cyc1);

#endif /* !PAR0 */

/* Handle EFFECTORs
*
*  Evaluate contribution of this layer to pjnwd changes array, which
*  provides "motor neuron" output to built-in (arm joints, windows)
*  and external (BBD) effectors.  An effector may draw excit and
*  inhib output from the same celltype (better be with different cell
*  lists)--this code must be prepared to do both sums in this case.
*
*  This used to be done at this time to avoid extra d3exch calls
*  since s(i) stayed on host only until next layer evaluated.  Now
*  that all s(i) are kept on host all the time, this placement is
*  unnecessary but has been kept.  Comments here relating to adding
*  up changes from multiple layers have been removed--it is not
*  clear this was ever actually done and certainly not since CHANGES
*  were replaced by EFFARBs.  */

#ifndef PARn
   if (cyclestogo == 1) {
      struct EFFARB  *pea;
      for (pea=il->pfea; pea; pea=pea->pnetct) {

         long (*iterator)(iter *);  /* Ptr to iterator function */
         float *pewd;               /* Ptr to output data storage */
         s_type *psi;               /* Ptr to Si data */
         iter wkitc;                /* Working iteration control */
         long icn;                  /* Current cell number */
         long ias,iase;             /* Arbor separation */
         long mxcn;                 /* Max cell number */
         long wksi;                 /* Unpacked value of s(i) */
         long weft;                 /* Effector action threshold */
         float wescl;               /* Effector output scale */
         int  iei;                  /* EXCIT or INHIB */

         /* Omit if frozen (output is zeroed above) */
         if (pea->keop & KEOP_FRZ) continue;

         /* Loop over excitation, inhibition lists */
         for (iei=EXCIT; iei<NEXIN; ++iei) {
            /* Omit if not coded for this celltype (or at all) */
            if (pea->pect[iei] != il) continue;

            /* Working constants for cell loop */
            pewd = RP0->pjnwd + pea->jjnwd[iei];
            weft = pea->eft[iei];
            wescl = pea->esclu[iei];

            /* Prepare loop over possible multiple chgs outputs.
            *  d3schk promises no pea entry here if nchgs == 0.  */
            iase = pea->nchgs * pea->arbs;
            for (ias=0; ias<iase; ias+=pea->arbs) {

               /* Prepare loop over cells.  Entire loop will be
               *  repeated for each iteration of ias loop, with
               *  ias added to cell number.  d3schk and d3news
               *  should check that nelt will not be exceeded.  */
               if (pea->pefil[iei]) {     /* User cell list */
                  iterator = ilstiter;
                  ilstset(&wkitc, pea->pefil[iei], 0);
                  mxcn = il->nelt;
                  }
               else {                     /* Synthetic iterator */
                  iterator = allciter;
                  wkitc.inow = 0;
                  mxcn = pea->arbw;
                  }

               if (pea->keop & KEOP_CELLS) {
                  /* Send data for individual cells to device.
                  *  Loop over cells contributing to this effector */
                  while ((icn=iterator(&wkitc)) >= 0 && icn < mxcn) {
                     icn += ias;
                     psi = il->pps[0] + spsize(icn,il->phshft);
                     d3gts2(wksi, psi);
                     if (wksi < weft) continue;
                     *pewd++ = wescl*(float)wksi;
                     } /* End of loop over cells */
                  } /* End KEOP_CELLS method */
               else {
                  /* Sending sums, not all cells */
                  si64 lsum;                 /* Layer sum */
                  memset((char *)&lsum, 0, sizeof(lsum));
                  /* Loop over cells contributing to this effector */
                  while ((icn=iterator(&wkitc)) >= 0 && icn < mxcn) {
                     icn += ias;
                     psi = il->pps[0] + spsize(icn,il->phshft);
                     d3gts2(wksi, psi);
                     if (wksi < weft) continue;
                     lsum = jasl(lsum, wksi);
                     } /* End of loop over cells */
                  /* Store the result */
                  *pewd = wescl*(float)(swdbl(lsum));
                  } /* End !KEOP_CELLS */

               } /* End changes loop */
            } /* End of loop over excit and inhib outputs */
         } /* End of loop over EFFECTOR blocks */

      } /* End if cyclestogo == 1 */
#endif   /* not PARn */

   } /* End layerupd() */
