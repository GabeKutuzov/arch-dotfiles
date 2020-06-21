/* (c) Copyright 1985-2018, The Rockefeller University *11115* */
/* $Id: d3go.c 77 2018-03-15 21:08:14Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                                d3go                                  *
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
************************************************************************
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
*  Rev, 04/12/09, GNR - Test PSP bits on paxp,paxn,aeff sums           *
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
*  Rev, 06/05/12, GNR - Make AUTOSCALE OPT=T ignore RK_NOUPAS          *
*  Rev, 06/07/12, GNR - Add adefer per conntype, snapshot Cij dist.    *
*  Rev, 06/16/12, GNR - Move KAMBY to cnopt, add BCM, P amplif. rules  *
*  Rev, 07/28/12, GNR - Eliminate bit offsets and bitiors              *
*  Rev, 08/16/12, GNR - Use mdltbid to eliminate mdltno bit tests      *
*  Rev, 08/29/12, GNR - Add PSI amplification factor                   *
*  V8I, 12/05/12, GNR - New decay modes, fsdecay and overflow counts   *
*  Rev, 12/07/12, GNR - Convert ns[xy], ku[xy] to ui16, nst to ui32    *
*  Rev, 12/24/12, GNR - New overflow checking                          *
*  V8I, 03/17/13, GNR - Begin implementing d3xxj macros                *
*  Rev, 03/29/13, GNR - Add CNOPT=M (conntype yields max(Cij*Sj))      *
*  Rev, 04/03/13. GNR - Add mopt=M (same thing for modulation)         *
*  Rev, 04/10/13, GNR - Bug fix: wkmtj not scaled in rdamp update      *
*  Rev, 04/23/13, GNR - Add RP->knoup with KNUP_AS and KNUP_RB opts    *
*  Rev, 04/24/13, GNR - Add mxsi                                       *
*  Rev, 05/20/13, GNR - AUTOSCALE OPT=H, self inputs no longer exempt  *
*                       from autoscaling, scale before m[nx]ax tests   *
*  Rev, 06/27/13, GNR - Move autoscaling setup to new d3ascc() routine *
*  Rev, 08/21/13, GNR - Revise type-dep detail print to all 32-bit     *
*  Rev, 10/14/13, GNR - Add gsimax/min, rspmax/min mechanism           *
*  Rev, 10/15/13, GNR - Add sunder stat for negative truncation        *
*  Rev, 08/22/14, GNR - Si only EXP decay, remove DECAYDEF from DCYDFLT*
*  R63, 11/04/15, GNR - Move volt-dep options to vdopt, add 'A' code   *
*  R64, 12/12/15, GNR - Implement effector KEOP_NITS (multi-cycle O/P) *
*  Rev, 12/19/15, GNR - Change 'arbs' to 'arbsep[NEXIN]'               *
*  R67, 04/27/16, GNR - Remove Darwin 1 support                        *
*  R67, 10/08/16, GNR - Move amplif data to CONNDATA for PAR dprint    *
*  Rev, 11/12/16, GNR - Allow more than one probe per celltype         *
*  R70, 01/24/17, GNR - Use DST_CLOCK0 to allow negative dst range     *
*  R72, 02/03/17, GNR - Change shunting inhibition to divisive         *
*  R74, 08/18/17, GNR - Change vdopt code 'A' to 'I' (invert sign)     *
*  R75, 10/03/17, GNR - Add TERNARY test code                          *
*  R76, 10/14/17, GNR - Add volt-dep modified afference distributions  *
*  Rev, 10/30/17, GNR - Use Izhi v for oldsi if it exists, add VDOPS   *
*                       Eliminate early phase convolution              *
*  R76, 11/06/17, GNR - Remove vdopt=C, pctt, NORM_SUMS w/cons tables  *
*  Rev, 11/08/17, GNR - Reorder synapse loop, mxax test after volt-dep *
*  R76, 12/07/17, GNR - Add VMP, change IZVU to IZU + IZVr             *
*  R77, 01/30/18, GNR - Add vdmm parameter, save RAW for SVRAW         *
*  R77, 02/27/18, GNR - Change AK,RAW from ETYPE to FTYPE, S(20/27)    *
*  R78, 05/09/18, GNR - Add sslope multiplier for RF_STEP|RF_KNEE      *
*  R78, 06/21/18, GNR - Add VDOPT=V for old Vm, COMPAT=V so vdopt S->V *
*  R78, 07/05/18, GNR - Add INTEGRATION and first case EULER           *
*                       Bug fix:  Include persistence in tanh argument *
*  R78, 07/19/18, GNR - Introduce noise 'V' option, add to Vm or s(i), *
*                       Calc vmterms at S64 to reduce overflow checks  *
*  R79, 08/11/18, GNR - Move stim marks to lindv area.  Bug fix: Make  *
*                       KSTGP|KSTMG|KSTHR stats indep of num of nodes  *
*  R80, 08/30/18, GNR - Change Aij to si64 so TERNARY can overflow 32  *
***********************************************************************/

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "lijarg.h"
#include "armdef.h"
#include "wdwdef.h"
#include "celldata.h"
#include "clblk.h"
#include "simdata.h"
#include "rocks.h"
#include "rkxtra.h"
#include "rkarith.h"
#include "rksubs.h"
#include "swap.h"
#include "bapkg.h"

/* To activate debug output for high/low stats: */
/* #define DBGHLS */
/* #define DBGHLSD */
/* Also define DBGHLSD for more detail.  */
/*  To activate debug output for TERNARY modulation: */
#define DBGTER

/* Prototypes unique to d3go */
void d3ascc(struct CELLTYPE *);
void d3autsc(struct CELLTYPE *);
void d3gfsvct(struct SDSELECT *, struct CELLTYPE *, ui32);
void d3ghls(void);
void d3pafs(void);
void d3valu(struct VBDEF *);
void d3zhls(void);
static void layerupd(struct CELLTYPE *, si16);
#ifndef PAR0
static void cijclean(struct CONNTYPE *, struct CONNDATA *);
void d3assv(struct CELLTYPE *, si32);
long d3cond(struct CELLTYPE *);
void d3inht(struct CELLTYPE *);
int  d3inhb(void);
void d3inhg(void);
void d3inhc(void);
si64 d3phase(struct CELLTYPE *);
static void sumaffs(struct CELLTYPE *, si64 *);
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

   /* General pointers */
#ifndef PAR0
   struct MODVAL   *imv;      /* Ptr to modulation value block */
   struct MODBY    *imb;      /* Ptr to modulated-by block */
   struct PRBDEF   *pprb;     /* Ptr to probe definition */
   struct CONNDATA *pND;      /* Ptr to current connection data */
#endif
   struct REPBLOCK *ir;
   struct CELLTYPE *il,*jl;
   struct CONNTYPE *ix;
   float plvx,plvy;           /* Repertoire plot x,y coords */

   int plot_cycle;            /* Cycle number for d3rplt */
#ifndef PAR0
   int  tsqis;                /* Shift for simvur cubed */
#endif

   /* Cycle controls */
   si16  cyclestogo;          /* Cycles remaining in main loop--counts
                              *  down to 0 in only or last cycle */
   ui16  ntcm1;               /* = ntc - 1 */

   char add_s_noise;          /* TRUE if adding noise to stimuli */
   char do_dprint;            /* TRUE if doing detailed print */
   char do_gfsv;              /* TRUE if recording SIMDATA */
   char sequence;             /* Flow control for inner loop */
   char statenbg;             /* Statistics enable (global) */
#define STEG_ANYC  1             /* On for any or all cycles */
#define STEG_THIS  2             /* On for some celltype this cycle */
   char statenbr;             /* Statistics enable (repertoire) */
   char statenbc;             /* Statistics enable (celltype) */
   char statenbm;             /* Statistics enable (modality) */

/***********************************************************************
*                                                                      *
*        Section 1 - Initialization done on every call to d3go         *
*                                                                      *
***********************************************************************/

/* Initialization relating to stimulus noise */

   add_s_noise = (RP->snsg | RP->snmn) && RP->snfr;

/* Set global statistics control switch */

   ntcm1 = (ui16)(RP->ntc - 1);
   statenbg = (RP->CP.runflags & (RP_NOSTAT|RP_NOCYST)) == 0;

#ifndef PARn
/* Clear the changes array -- not required on comp nodes */

   if (RP0->ljnwd) memset((char *)RP0->pjnwd, 0,
      RP0->ljnwd*sizeof(float));
#endif

/* Keep track of information needed for modality statistics.
*  --Increment trials counters (all stats).
*  --Use the stimulus marking bits contained in the sensory broadcast
*  to fill in the pxxxx bits in each modality block indicating which
*  stimuli were actually encountered.  This info and the max isn are
*  needed in d3stat to determine whether to proceed with XRM stats,
*  hence must be available prior to GRPMSK broadcast where it might
*  otherwise go.
*
*  (Prior to V8A, this was done in main pgm following RP broadcast,
*  using isn info in RP.  This scheme would now require broadcasting
*  modality blocks on every cycle.  It was technically wrong for two
*  other reasons: (1) could not handle more than one stimulus in a
*  presentation, and (2) was unnecessarily done during replays.
*
*  R67, 11/30/16, GNR - This code was modified to compute mxisn from
*  available pxxxx bits.  This method, although clumsy, was considered
*  superior to adding a new broadcast to send mxisn from Node 0 at end
*  of trial series.  (It would not do to try to broadcast isn at start
*  of each trial and find max here, as there can be multiple isn even
*  for one modality.  Note that modality blocks are Static and there-
*  fore not updated on each trial.  If that changes, mxisn must be
*  moved to CELLDATA.  */

   if (statenbg) {
      struct MODALITY *pmdlt;
      for (pmdlt=RP->pmdlt1; pmdlt; pmdlt=pmdlt->pnmdlt) {
         if (RP->CP.trialsr < (ui32)pmdlt->ntrs)
            ++pmdlt->um1.MS.ntract;
         if (RP->kxr & MXSTATS) {
            byte *pxxbits = CDAT.pmbits + pmdlt->mdltboff;
            long jmxisn = (long)pmdlt->ndsbyt;
            bytior(pmdlt->pxxxx, jmxisn, pxxbits);
            while (--jmxisn >= 0) if (pxxbits[jmxisn]) {
               byte hibyt = pxxbits[jmxisn];
               byte jjbit = 1 << (BITSPERBYTE-1);
               jmxisn = Bytes2Bits(jmxisn) + 1;
               while (jjbit && !(hibyt & jjbit)) ++jmxisn, jjbit >>= 1;
               pmdlt->um1.MS.mxisn = jmxisn;
               break;
               } /* End search for mxisn */
            } /* End pxxxx marking */
         } /* End modality loop */
#ifndef PAR0
      if (RP->lmarks) memset(CDAT.pmarks, 0, RP->lmarks);
#endif
      } /* End if statenbg */

#ifndef PAR0
   /* Set scales that depend on COMPAT=C mode */
   tsqis = 2*(FBwk + RP->bsdc) - 4; /* 4 frac bits saved for rounding */
#endif

/*---------------------------------------------------------------------*
*      Perform new trial initialization loop over all cell types       *
*---------------------------------------------------------------------*/

   for (il=RP->pfct; il; il=il->pnct) {

      /* Clear cycle-dependent flags */
      il->ctf &= ~(DIDSTATS|CTFDPENB);

      /* Set enable flag for dprnt to save two tests in cycle loop.
      *  (If done in d3news, would require an extra il loop.)  */

      if ((RP->CP.runflags & RP_OKPRIN) &&
            il->ctclloc[CTCL_DPRINT].clnum && !(il->ctf & CTDPDIS))
         il->ctf |= CTFDPENB;

      /* Initialize for s(i) calculation freeze.  (This cannot be
      *  moved to d3news because RP->ntc can change after d3news.) */
      il->ctwk.go.ntca = (il->Ct.ctopt & OPTFZ) ? 0 : (si32)ntcm1;

#ifndef PAR0
      il->ctwk.go.fstanh = (float)il->Ct.stanh;
      il->ctwk.go.fvmscl = 1.0/(float)(1 << (RP->bsdc + FBwk));

/* If ctopt OPTCFS requested, and there is a stimulus from a new
*  stimulus group, restart rbar and qbar averaging.  */

      if (il->Ct.ctopt & OPTCFS) {
         struct MODALITY *pmdlt;
         for (pmdlt=RP->pmdlt1; pmdlt; pmdlt=pmdlt->pnmdlt) {
            if ((il->amdlts & pmdlt->mdltbid) &&
                  CDAT.pmbits[pmdlt->mdltnoff]) {
               il->ctf |= CTFDOFS;
               break;
               } /* End if new stimulus class found */
            } /* End modality scan */
         } /* End if OPTCFS */

/* Set up for probes.  Check trial timer and set
*  whether and where to start in cell list.  */

      for (pprb=il->pctprb; pprb; pprb=pprb->pnctp) {
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
               /* Can't overflow--design spec ncells < 2^31 */
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
*  (Compute quantities which do not change over cycles in a trial)
*     Set number of cycles simulation should be active.
*     Set kgfl bit KGNBP for normal or bypass action.
*     Adjust scale factor according to image max pixel.
*     For IA input, locate first stimulus byte at window.
*     For HV input, locate arm with respect to window.
*     Compute TERNARY modulation scale */

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

/* N.B.  NOPBY is the master control for bypassing a connection type.
*  KGNBP must accurately reflect NOPBY plus any other conditions that
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
         if (!(ix->sssck & (PSP_POS|PSP_NEG)) ||
            ((ix->cnflgs & DIMDLT) &&
               (RP->CP.trialsr >= (ui32)ix->psmdlt->ntrs))) {
            ix->kgfl |= KGNBP;
            continue;
            }

/* Amplification setups previously done here moved to d3news,
*  02/20/97, GNR.  These cannot change during a trial series.  */

/* The following data change only at new trial time, not at
*  new cycle time, therefore, not needed to set in cycle loop. */

/* If NOPNI (cnopt=N), use image or image preprocessor max pixel
*  information to set scale such that afference for max pixel after
*  scaling is mximg.  Scaling note and bug fix, 12/17/17:  Noting
*  that when Aij is scaled to Aff, we right shift by ix->scls = S27
*  (23 with mV scale and non-REPSRC Sj (S14)), we want
*  Aff(S20/27) = scl(S20?24) * Cij(S16) * pixel(S14) / S(23?27) and
*  scl(S24) = Fix*mximg(S27)*S27 / (S16*mxpix(S14)), Fix = S(24-24)=S0.
*  scl(S24) = Fix*mximg(S20)*S27 / (S16*mxpix(S7)),  Fix = S(24-24)=S0.
*  scl(S20) = Fix*mximg(S20)*S23 / (S16*mxpix(S14)), Fix = S(20-13)=S7.
*/
         if (ix->Cn.cnopt & NOPNI) {
            int Fix = (ix->sclb == FBsc) ? 0 : 7;
            ix->Cn.scl = dsrsjqe(ix->Cn.mximg, Fix,
               RP->pjdat[ix->omxpix], ix->iovec+CNO_RED);
#ifndef PARn
            Fix = (int)ix->sclb;
            convrt("(P2,' Scale for ',J1A" qLXN ",'conntype ',J1UH4,"
               "'set to ',J1BIJ8.<4,'for CNOPT=N.')",
               fmtlrlnm(il), &ix->ict, &Fix, &ix->Cn.scl, NULL);
#endif
            }

#ifndef PAR0

/* If TERNARY, compute denscl = denominator image scale = multiplier
*  for local image pixel to be used in cell loop below.  This is
*  given scale FBtm (=20) which gives plenty of precision and very
*  low probability of overflow (but do check for it).
*  Noise is constant over trial series so set in d3news().  */

         if (ix->pvpfb) {
            struct TERNARY *pvp = ix->pvpfb;
            si32 mxpix = pvp->mximage ? pvp->mximage :
               RP->pjdat[pvp->qvpict->omxpix];     /* (FBim) */
            pvp->denscl = dsrsjqe(pvp->modscl, FBtm-FBsc+FBim,
               mxpix, ix->iovec+CNO_TER);
            } /* End TERNARY scale calc */

/* If hand-vision input, pick up and save relative hand location */

#ifdef BBDS
         if (ix->kgen & KGNBV) {    /* Distances from BBD device */
            if (ix->cnxc.tvmode & BM_NONSTD) {
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

   cyclestogo = RP->ntc - 1;     /* Initialize cycle counter */
NEWCYCLE:
   CDAT.curr_cyc1 = RP->ntc - cyclestogo;
   RP->CP.effcyc += 1;

   /* Determine whether this cycle is to be recorded in SIMDATA */
   do_gfsv = RP->kevtu & EVTWSD && (RP->ksv & KSDNC ||
      !cyclestogo) && bittst(RP->CP.trtimer, (long)RP->jsdsvtm);

/* Contaminate input array stimuli with noise.
*     Note: Stimuli are stored in strips of nsy bytes.
*     Each strip is followed by nsy bytes used for assignments.
*     The noise-contaminated values are now stored in this space.
*     A special faster path is used if no noise is being added.  */

   if (RP->nst) {
      ui32 lnsy = RP->nsy;
      stim_mask_t *pstm,*pstmlim;
      stim_mask_t *mxstim = RP->pstim + ((ui32)RP->nsx << RP->ky1);
      if (add_s_noise) {         /* Adding noise */
         for (pstm=RP->pstim; pstm<mxstim; pstm+=lnsy) {
            pstmlim = pstm + lnsy;
            for ( ; pstm<pstmlim; pstm++) {
               register si32 work = *pstm << Ss2hi;   /* To (S24) */
               work += d3noise(RP->snmn,RP->snsg,RP->snfr,RP->snseed);
               *(pstm+lnsy) = (stim_mask_t)(work <= 0 ? 0 :
                  (work >= 0x00fe8000 ? 0xff :
                  ((work + 0x00008000) >> Ss2hi)));
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
         imv->umds.mdsum = jesl(0);
      else {
         s_type *pcell  = (s_type *)imv->amsrc; /* Ptr to src cells */
         gvin_fn getin = imv->mdgetfn;       /* Ptr to fn to get data */
         si64   wsum  = jesl(0);             /* Working sum */
         si32   lmthi = (si32)imv->Mdc.mt;   /* Local copy of mthi */
         si32   lmtlo = (si32)imv->Mdc.mtlo; /* Local copy of mtlo */
         si32   lmjrv = (si32)imv->Mdc.mjrev;/* Local copy of Vrev */
         si32   lsmthi, lsmtlo;              /* Subtracted thresholds */
         si32   wksi;                        /* Working s(i) */

         /* Set for STEP vs KNEE thresholds */
         if (imv->Mdc.mopt & IBOPKR)
            lsmthi = lmthi, lsmtlo = lmtlo;
         else
            lsmthi = lsmtlo = 0;
         /* Set to eliminate zero entries */
         if (lmthi) lmthi -= 1;
         if (lmtlo) lmtlo += 1;

         if (imv->mdsrctyp == IA_SRC || imv->mdsrctyp == VS_SRC) {
            s_type *pcxe,*pcye;           /* Limiting pcell x,y vals */
            /* Note:  x1, etc. left as longs, not ui32, to simplify
            *  tests below for negative or out-of-bounds coords.  */
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
                  if      (wksi > lmthi) wksi -= lsmthi;
                  else if (wksi < lmtlo) wksi -= lsmtlo;
                  else continue;
                  if (imv->Mdc.mopt & IBOPMX) {
                     if (qsw(jrsl(wsum, wksi)) < 0) wsum = jesl(wksi);
                     }
                  else
                     wsum = jasl(wsum, wksi);
                  } /* End y loop */
               } /* End x loop */
            } /* End input from IA */
         else if (imv->mdsrctyp == VALSRC) { /* Input from Value */
            wksi = (*(si32 *)imv->amsrc << Sv2mV) - lmjrv;
            if      (wksi > lmthi) wsum = jesl(wksi - lsmthi);
            else if (wksi < lmtlo) wsum = jesl(wksi - lsmtlo);
            } /* End input from Value */
         else {                     /* Input not from IA or Value */
            s_type *pce;                  /* Limiting pcell value */
            size_t pci = (size_t)imv->mdincr;   /* Cell increment */
            pce = pcell + pci*(size_t)imv->mdnel;
            /* Sum inputs above thresh */
            for ( ; pcell<pce; pcell+=pci) {
               wksi = getin(pcell, &imv->Mdc.mvxc) - lmjrv;
               if      (wksi > lmthi) wksi -= lsmthi;
               else if (wksi < lmtlo) wksi -= lsmtlo;
               else continue;
               if (imv->Mdc.mopt & IBOPMX) {
                  if (qsw(jrsl(wsum, wksi)) < 0) wsum = jesl(wksi);
                  }
               else
                  wsum = jasl(wsum, wksi);
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
            float fcycle = -(float)cyclestogo;
            plvx = fcycle*ir->Rp.vxo;
            plvy = fcycle*ir->Rp.vyo;
            plot_cycle = (cyclestogo > 0) ? (int)CDAT.curr_cyc1 : 0;
            }
         else {      /* Normal superposition, no time series */
            plvx = 0;
            plvy = 0;
            plot_cycle = (cyclestogo > 0) ? -1 : 0;
            }
         if (plot_cycle >= 0)
            d3rplt(ir, plot_cycle, ir->aplx+plvx, ir->aply+plvy,
               ir->aplw, ir->aplh);
         } /* End if superposition plot */

/*---------------------------------------------------------------------*
*            Process all celltypes for current repertoire              *
*---------------------------------------------------------------------*/

      for (il=ir->play1; il; il=il->play) {
         iter dpit;              /* Iterator for detailed print */
#ifdef PARn
         struct HILOSI *qhlsi;   /* Ptr to HILOSI for il and trial */
#endif
#ifndef PAR
         struct GPSTAT *pgp0;    /* Ptr to KSTGP|KSTMG stats (SER) */
         int  ihisi,ilosi;       /* Bins for [hl]dist (PAR in d3gsta) */
#endif
#ifndef PAR0
         struct RFRCDATA *prf;   /* Ptr to refractory period params */
         struct PHASEDEF *ppd;   /* Ptr to phasing parameters */
         byte    *pmark;         /* Ptr to modality marker bits */
         si64    *prbar;         /* Ptr to rbar info stored for stats */

         size_t coff;            /* Offset to this cell in prd data */
         size_t llel;            /* Length of data for one cell */

         long laffb;             /* Number of bytes in laffs si64's */
         ulng nhitspt;           /* Number of hits > pt in 1 cycle */
         ulng nhitsht;           /* Number of hits > ht in 1 trial */

         si32 lhisi,llosi;       /* High,low s(i) on this node */
         si32 lzseed;            /* Working value of izseed */
         si32 refrcs16;          /* DST_CLOCK0 - refrac period (S16) */

         int  icell;             /* Cell counter */
         int  ject;              /* Offset to overflow counts */
         int  laffs;             /* Number of terms in a sum */

         byte  decay_bit;        /* Equal to DECAYED if omegau != 0
                                 *  and !CLEAREDSI, otherwise 0.  */
#endif
         CDAT.cdlayr = il;

         /* Prevent s1,s2 flip, stats at end if calc skipped */
         il->ctf &= ~(DIDSCALC|CTFSTENB);

         /* Set up statistical accumulation according to
         *     global, repertoire, and celltype controls */
         if (il->Ct.kstat & KSTNS)
            statenbc = statenbm = FALSE;
         else if (il->ctwk.go.ntca == 0)
            statenbc = statenbm = statenbr;
         else if (RP->CP.runflags & RP_ALLCYC)
            statenbc = statenbr, statenbm = FALSE;
         else
            statenbc = statenbm = FALSE;
         if (statenbc)
            il->ctf |= CTFSTENB, statenbg |= STEG_THIS;

         /* Omit entire layer if s(i) freeze and have done one
         *  cycle after all direct inputs have quiesced.  */
         if (il->ctwk.go.ntca < 0) goto ADVLAYER;

         /* Set bypass bits now so e.g. d3kiji() will see them
         *  in time */
         for (ix=il->pct1; ix; ix=ix->pct) {
            if ((ix->cnflgs & DIMDLT) &&
                  (CDAT.curr_cyc1 > ix->psmdlt->ntcs))
               ix->kgfl |= KGNBP;
            }

         /* Set autoscaling controls */
         d3ascc(il);

/* Setup for noise generation.
*  N.B.  One would just like to use nmodval as the noise switch,
*  but this will not work because mdsum is not known on host.  */

         if (il->No.noikadd & NFL_GENN) {
            il->nsd0 = il->No.nsd;     /* For d3lplt noise calc */
#ifdef PARn
            /* Make seed correspond to first cell on this node */
            udevwskp(&il->No.nsd, jslsw(jesl(il->locell),1));
#endif
            }

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

#ifdef DBG_PHSEED
         if (il->ctf & CTFNC) {
dbgprt(ssprintf(NULL, "d3go Ser %d, Trial %d, Celltype %4s phiseed"
   " %jd", RP->CP.ser, RP->CP.trial, il->lname, il->pctpd->phiseed));
            }
#endif

/* Setup for cross-response and history marking.  A mask is created in
*  the pmarks array which has a field for each sensory modality that
*  contributes to this cell type.  Within each such field, bits are
*  set for all distinct stimuli that are visible to this cell type in
*  this trial.  Marking of the XRM ('marking' switch) and Cdist data
*  for individual cells is later done from this mask (when requested).
*  However, the pmarks data must be maintained even if this cell type
*  has no stats, for possible calculation of the responses of down-
*  stream cell types (CTFXD and 'masking' switch).  Accumulation of
*  response history requires the delmk bit array, which records
*  changes in stimulation from trial to trial.  It is set up after
*  pmarks & pmarkh are initialized in the conntype loop below.  */

         il->ctwk.go.marking = statenbm && ctexists(il,XRM);
         il->ctwk.go.masking = statenbg && (il->ctf & CTFXD);

#ifndef PAR0
/*---------------------------------------------------------------------*
*   Following initializations are not required on host node            *
*---------------------------------------------------------------------*/

/* Lookup offset to overflow counts.  Note:  We call e64dec() here
*  just to handle the case that a uw2zd() might catch an overflow on
*  a 32-bit system.  Most code uses ject explicitly with an offset
*  from the CTO_xxx series.  */

         e64dec(ject = (int)il->iovec);

/* Set length of sums going into cell totals */

         laffs = 1 << il->pdshft;
         laffb = sizeof(si64) * laffs;

/* Get ptrs to final affs and rbar info (1 si64 per conntype) */

         CDAT.pfaffs = CDAT.praffs + il->nct;
         prbar = CDAT.pfaffs + il->nct;

/* Get pointers to refractory and phase info */

         prf = il->prfrc;
         ppd = il->pctpd;

/* Get pointer to stimulus isn marking array (harmless
*  code if pmarks array does not exist, not used on PAR0).  */

         pmark = CDAT.pmarks + il->iMarks;

/* Setup to mark CDAT.events as DECAYED if there is a nonzero
*  persistence coefficient.  */

         decay_bit = (il->Dc.omegau && !(CDAT.flags & CLEAREDSI)) ?
            DECAYED : 0;

/* If there is self-input, store scale actually to be used in
*  il->sisclu.  If there is autoscaling with OPT=H|W, scale is
*  multiplied by the autoscale multiplier only if the self input
*  is inhibitory, otherwise for either sign.
*  d3ascc() sets CDAT.qasov to test USG_QIR bit only if
*  scale modification is to be done in this cycle.  */

         if (il->kaff[SGM_Self] & (AFS_Excit|AFS_Hyper)) {
            il->sisclu = il->Dc.siscl;
            if (CDAT.qasov & il->apspov) {
               int iasop = (il->Dc.siscl < 0 ? INHIB : EXCIT) + NSnS;
               iasop = CDAT.kasop[iasop];
               if (iasop != AS_NOP) il->sisclu = mrssle(il->sisclu,
                  il->pauts->asmul[iasop], FBsc,
                  (int)il->pauts->iovec);
               }
            }
         else il->sisclu = 0;

/* Set up for geometric connections (apply autoscale, etc.) */

         if (il->pib1) d3inht(il);

/* Advance seed for Izhikevich neuron a,b,c,d randomization--only
*  needed if randomizing and OPTMEM  */

         if (il->ctf & CTFRANOS) {
            lzseed = ((struct IZHICOM *)il->prfi)->izseed;
#ifdef PARn
            udevwskp(&lzseed, jmsw(il->nizvar,il->locell));
#endif
            }

#ifdef PARn
/* Advance seed for phase option to low cell on this node if needed.
*  N.B.  phiseed advances once per cell.  This seed is advanced on
*  host at layer update time to account for all cells in the layer.
*  This whole scheme of course must be reexamined if host ever
*  participates in calcs.  */

#ifdef DBG_PHSEED
         if (il->ctf & CTFNC) {
            udevskip(&ppd->phiseed, il->locell);
dbgprt(ssprintf(NULL, "  Adv by %d, resulting in %jd",
   il->locell, ppd->phiseed));
            }
#else
         if (il->ctf & CTFNC)
            udevskip(&ppd->phiseed, il->locell);
#endif

/* Advance cell-level decay (random-rounding) seed.  At present, this
*  seed is used up to three times per cell per cycle to round qbar &
*  sbar, dst, and depression, but we advance it deterministically four
*  times.  This makes results independent of which options may be
*  turned on or off, and allows one additional use of rsd to be added
*  later without changing results obtained with the present version.
*/

         udevwskp(&il->rsd, jslsw(jesl(il->locell),2));
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
               si32 *pafm;       /* Ptr to detail print aff data */
               si64 nelmto,wmsum;
               long mdec;
               si32 work,wscl,lmxm;

               if (!(imb->mssck & (PSP_POS|PSP_NEG))) continue;
               if (CDAT.curr_cyc1 <= imb->Mc.mdefer &&
                  !qdecay(&imb->MDCY)) continue;
               imv = imb->pmvb;
               mdec = 0;
               pafm = CDAT.pAffD32 + imb->omafs;

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
               /* Apply autoscale adjustment to mscl if this is not
               *  noise modulation.  If there is autoscaling with
               *  OPT=H|W, scale is multiplied by the autoscale multi-
               *  plier only if the input is inhibitory, otherwise for
               *  either sign.  Also must take into account whether to
               *  use scale for self or other input.  d3ascc() sets
               *  CDAT.qasov to test USG_QIR bit only if scale
               *  modification is to be done in this cycle.  */
               wscl = imb->mscl;
               if (imb->mct && CDAT.qasov & imb->Mc.mpspov) {
                  int iasop = imb->Mc.jself;
                  if (imb->mscl < 0) iasop += NSnS;
                  iasop = CDAT.kasop[iasop];
                  if (iasop != AS_NOP)
                     wscl = mrssle(wscl, il->pauts->asmul[iasop],
                        FBsc, (int)il->pauts->iovec);
                  }
               /* S7 (mdsum) + S20 (mscl) - 7 ==> S20 */
               wmsum = mrsrswe(wmsum, wscl, FBsi, (int)imv->iovec);
               work = (imb->Mc.mopt & IBOPMX) ?
                  swloe(wmsum, (int)imv->iovec) :
                  jdswq(wmsum, imv->mdnel);

ModvalBelowThresh:
               /* Save "raw" mval for dprt and stats */
               *pafm++ = work;

               /* Apply cap */
               lmxm = imb->mxmod;
               if (abs32(work) > lmxm) {
                  work = (work >= 0) ? lmxm : -lmxm;
                  }

               /* Apply decay if requested */
               if (qdecay(&imb->MDCY)) {
                  work = CDAT.pdecay[CDAT.curr_cyc1 == imb->mfscyc](
                     &imb->MDCY, imb->MDCY.peipv, pafm, work);
                  pafm += imb->MDCY.ndal;
                  }

               /* Save final value for dprt */
               *pafm = work;

               if (CDAT.curr_cyc1 <= imb->Mc.mdefer) continue;
               /* MOPNEG is deprecated -- use MUSAGE */
               if (work >= 0) {
                  if (imb->Mc.mopt & MOPNEG || imb->Mc.mpspov & USG_INH)
                     continue; }
               else if (imb->Mc.mpspov & USG_EXC) continue;

               if (imb->mct == 0) {
                  /* Perform noise modulation */
                  work = SRA(work, (FBwk + FBsi - FBsc));  /* To S24 */
                  if (imb->Mc.mopt & IBOPTQ) {  /* Squared? */
                     si32 wksq = mrsrsle(work,work,FBsc,ject+CTO_RFN);
                     work = (work < 0) ? -wksq : wksq;
                     }
                  CDAT.nmodval += work;
                  }
               else {
                  /* Perform cell response modulation.  If celltype has
                  *  phase, distribute modulation uniformly, otherwise
                  *  there is just one term.  */
                  si64 *pSums = CDAT.pAffD64 +
                     imb->omsei[(ui32)work >> (BITSPERUI32-1)];
                  int  p;
                  for (p=0; p<laffs; p++)
                     pSums[p] = jasl(pSums[p], work);
                  }

               }  /* End of loop over modby blocks */
            } /* End if lModulSums > 0 */

/* Set negative refractory period (S16) for faster clock setting */

         if (prf) refrcs16 = ((si32)(DST_CLOCK0-prf->refrac) << Ss2hi);

/* Set up for probes.  Note that cyce is 0 to skip probing,
*  e.g. if, in a PAR computer, lowest cell is not on this node.  */

         for (pprb=il->pctprb; pprb; pprb=pprb->pnctp) {
            if (CDAT.curr_cyc1 >= pprb->cyc1 &&
                  CDAT.curr_cyc1 <= pprb->cyce) {
               ilstset(&pprb->prbit, pprb->pclb->pclil, pprb->prob1st);
               pprb->sprbcyc += pprb->ramp;
               pprb->lpptr = pprb->ipptr;
               }
            else
               pprb->lpptr = 0;
            }
#endif

/*---------------------------------------------------------------------*
*  Initialize the individual connection types for new cycle            *
*                                                                      *
*     Set bypass bit if DIMDLT input and past ntcs cycles.             *
*     Determine now-conntype-dependent amplification sequence.         *
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

         for (ix=il->pct1; ix; ix=ix->pct) {

/* Turn off bits that may change on each cycle */

            ix->cnflgs &= ~UPRSD;

/* Implement the decision matrix relating to the selection of
*     SEQUENCE I:    No amplification or detailed amp stats
*     SEQUENCE II:   Detailed stats only, no amp or decay
*     SEQUENCE III:  Decay, amplification, and detailed stats.
*  (This done on every trial because SEQ_II changes to SEQ_III
*  after adefer cycle if amplif is deferred.)  */

            if (ntcm1 == 0) {
               if (RP->CP.runflags & RP_NOCHNG) sequence = SEQ_II;
               else if (ix->Cn.adefer == 0) sequence = SEQ_III;
               else sequence = SEQ_II;
               }
            else if (CDAT.curr_cyc1 > ix->Cn.adefer) {
               if (RP->CP.runflags & RP_NOCHNG) sequence = SEQ_II;
               else sequence = SEQ_III;
               }
            else
               sequence = SEQ_I;

/* This little setup is required on all nodes because it controls
*  detail print on host.  */

            ix->finbnow =
               (sequence == SEQ_III) ? ix->finbi : NO_FIN_AMP;

/* If bypassing, skip rest of setup */

            if (ix->kgfl & KGNBP) continue;

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
               udevwskp(&ix->rseed, jmsw(il->locell,ix->upcij));
#endif
               }

            /* Advance phasing seeds if phasing is on.
            *  Rev, 11/25/16, GNR - Updating upphseed moved here from
            *  layerupd to consolidate code.  Even though it is very
            *  unlikely that multiples of nc will overflow 32 bits, use
            *  udevwskp here to meet the spec that nelt < 2^31.  */
            if (ix->phopt & PHASR) {
               ix->phseed = ix->upphseed;
#ifdef PARn
               udevwskp(&ix->phseed, jmsw(il->locell,ix->nc));
#endif
               /* Advance for next cycle */
               udevwskp(&ix->upphseed, jmsw(il->nelt,ix->nc));
               }

/*---------------------------------------------------------------------*
*   Following initializations are not required on host node            *
*---------------------------------------------------------------------*/

#ifndef PAR0

/* If AUTOSCALE was requested, adjust input scale accordingly and
*  store scales actually to be used in ix->sclu.  wsmnax,wsmxax now
*  removed, scaling done before excit/inhib combination because the
*  two scales may be different with AUTOSCALE KAUT_WTA option.  */

            if (CDAT.qasov & ix->spspov) {
               struct AUTSCL *paut = il->pauts;
               int ias = CDAT.kasop[ix->jself];
               int jao = (int)paut->iovec;
               ix->sclu[EXCIT] = (ias == AS_NOP) ? ix->Cn.scl :
                  mrssle(ix->Cn.scl, paut->asmul[ias], FBsc, jao);
               ias = CDAT.kasop[ix->jself + NSnS];
               ix->sclu[INHIB] = (ias == AS_NOP) ? ix->Cn.scl :
                  mrssle(ix->Cn.scl, paut->asmul[ias], FBsc, jao);
                 }
            else if (ix->fdiopt) {     /* Perform fade-in scaling */
               ui32 ifade;
               switch ((enum FdiOps)ix->fdiopt) {
               case FDI_SBT:           /* Series by trial */
                  ifade = RP->CP.trial;
                  break;
               case FDI_SBC:           /* Series by cycle */
                  ifade = (ui32)RP->ntc*(RP->CP.trial - 1) +
                     (ui32)CDAT.curr_cyc1;
                  break;
               case FDI_TBC:           /* Trial by cycle */
                  ifade = (ui32)CDAT.curr_cyc1;
                  break;
                  }  /* End fade-in option switch */
               if (ifade > (ui32)ix->fdinum)
                  ix->sclu[EXCIT] = ix->sclu[INHIB] = ix->Cn.scl;
               else
                  ix->sclu[EXCIT] = ix->sclu[INHIB] =
                     mrssle(ix->Cn.scl, ix->fadein[ifade-1], FBfi,
                     ix->iovec+CNO_RED);
               }  /* End fade-in scaling */
            else
               ix->sclu[EXCIT] = ix->sclu[INHIB] = ix->Cn.scl;

            ix->cnwk.go.vmvt = 0;   /* So dprnt prints 0 if no amp */
            if (sequence == SEQ_I) goto SKIP_CYCLE_AMP_SETUP;

/*                         (V - mtv)
*
*  Note:  The first of two words stored for each value scheme is the
*  signed, unclipped value, used for amplification so that connection
*  strengths can be decreased when value is negative.  The second word
*  for each value scheme is the value clipped to the range 0 < VALUE <
*  1.0 for use as an input.  */

            ix->cnwk.go.kamv = 0;   /* Amplif case at value level */
            if (ix->Cn.kam & KAMVNC) {
               si16 tval = ix->cnwk.go.vmvt =
                  (si16)RP->pvdat[ix->Cn.vi-1].fullvalue - ix->Cn.mtv;
               if      (tval <  0) ix->cnwk.go.kamv |= KAC_VLO;
               else if (tval == 0) ix->cnwk.go.kamv |= KAC_INELG;
               }

/*                         (V * RHO)
*
*  This variable is used for maintaining Cij0.  If Cij0 is not
*  allocated, set vrho = 0 to prevent access. */

            if (cnexists(ix,CIJ0)) {   /* Is Cij0 allocated? */
               if (ix->Cn.dvi) {       /* Yes, is value index used? */
                  ix->cnwk.go.vrho = mrsrsle(ix->Cn.rho,
                     RP->pvdat[ix->Cn.dvi-1].fullvalue, FBsv,
                        ject+CTO_DCY);
                  }
               else ix->cnwk.go.vrho = ix->Cn.rho;
               }
            else ix->cnwk.go.vrho = 0;

/*                         mntsmxmp
*
*  In TICKAMP or ELJTAMP, this represents the maximum decrement in
*  Mij that is allowed.  When the Mij increment exceeds this value,
*  i.e. Mij is about to increase, the clock is (re)started.
*  Note: zetam is S16, by shifting only 10, we up the product
*  scale from 14 to 20.  */

            ix->cnwk.go.mntsmxmp = (ix->Cn.mxmij*ix->Cn.zetam) >> 10;
            if (ix->cnwk.go.mntsmxmp > ix->Cn.mxmp)
               ix->cnwk.go.mntsmxmp = ix->Cn.mxmp;

SKIP_CYCLE_AMP_SETUP: ;

/* Set ix->srcorg to point to the array of source Sj.  This setting is
*  delayed until here because of cycle alternation for repertoire
*  input.  N.B.  This setting cannot be done on host node because some
*  of these pointers point inside arrays and thus cannot be translated
*  by membcst().  */

            switch (ix->cnsrctyp) {
            case REPSRC:
               ix->srcorg = (s_type *)
                  ((struct CELLTYPE *)ix->psrc)->pps;
               break;
            case IA_SRC:
            case VS_SRC:
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
            case USRSRC:
               if (ix->cnxc.tvmode & BM_NONSTD)
                  ix->srcorg = (s_type *)(RP->paw + ix->osrc);
               else
                  ix->srcorg = (s_type *)RP->pbcst + ix->osrc;
               break;

            default:
               ix->srcorg = (s_type *)RP->pbcst + ix->osrc;
               } /* End cnsrctyp switch */

/* Store working pointer to decaying afference */

            ix->cnwk.go.paeff = ix->Cn.ADCY.peipv;

/* Enter contribution of this connection type into pmarks array for
*  cross-response and [CGM]dist masking.  Stimulus identification bits
*  are picked up from the sensory broadcast in the case of direct
*  inputs and from the source pmarks array in the case of inputs from
*  other regions.  CDAT.pmbits is space in broadcast area, should be
*  on all nodes.  A little time might be saved by replacing the bittst
*  calls with in-line ANDs with a shifting mask bit, but the need to
*  allow for missing modalities would add complexity.
*
*  Note added, 07/12/12:  I added the jl!=il test to avoid propagat-
*  ing marks from reentrant inputs from the same celltype, as with
*  ntc odd, these concatenate over the entire trial series.
*  Technically, it is correct to carry over this information,
*  but only if the reentrant connection contributed significantly
*  to the response, and there is no way to check that here.
*
*  Note added, 08/12/18:  This code was removed from PAR0 which only
*  collects data already identified by stim id.  Old code to OR
*  together inputs subject to different axonal delays was removed, as
*  these will be collected during multiple cycles of a trial, and are
*  now "forgotten" when a new trial starts, as is appropriate when
*  trials are assumed to be separated by suitable time intervals
*  (omega = 0 default).  */

            if (il->ctwk.go.masking && !(ix->cnflgs & NOMDLT)) {
               struct CELLTYPE *jl;
               struct MODALITY *pmdlt;
               if (ix->cnflgs & DIMDLT) { /* Non-repertoire input */
                  pmdlt = ix->psmdlt;
                  bytior(pmark + ix->mdltmoff, (long)pmdlt->ndsbyt,
                     CDAT.pmbits + pmdlt->mdltboff);
                  } /* End direct input */
#if 0    /* DEBUG */
               else
                  jl = (struct CELLTYPE *)ix->psrc;
#else
               else if ((jl = (struct CELLTYPE *)ix->psrc) != il)
#endif
                  {  /* Here so don't have two of these in #if above */
                  /* Repertoire input--transfer (OR)
                  *  bits from all common modalities */
                  byte *tsmk = CDAT.pmarks + jl->iMarks; /* Source */
                  byte *ttmk = pmark;                    /* Target */
                  mdlt_type comdlts = il->rmdlts & jl->rmdlts;
                  for (pmdlt=RP->pmdlt1; pmdlt; pmdlt=pmdlt->pnmdlt) {
                     if (comdlts & pmdlt->mdltbid)
                        bytior(ttmk, (long)pmdlt->ndsbyt, tsmk);
                     if (il->rmdlts & pmdlt->mdltbid)
                        ttmk += pmdlt->ndsbyt;
                     if (jl->rmdlts & pmdlt->mdltbid)
                        tsmk += pmdlt->ndsbyt;
                     } /* End modality loop */

                  } /* End repertoire input */
               } /* End collecting contribution to mask array */
#endif   /* Not defined PAR0 */

            }  /* End of loop over connection types */

#ifndef PAR0
         /* Now that pmark has been initialized, can calc delmk.
         *  N.B.  Modality bits in delmk scan from right to left. */
         if (statenbm && il->Ct.kstat & KSTHR && !cyclestogo) {
            struct MODALITY *pmdlt;
            /* tomk,tpmk = ptrs to old, current stim marks */
            char *tomk = (char *)CDAT.pmarkh + il->iMarkh;
            char *tpmk = (char *)pmark;
            mdlt_type imnum = 1; /* Relative modality number */

            il->ctwk.go.delmk = 0;
            for (pmdlt=RP->pmdlt1; pmdlt; pmdlt=pmdlt->pnmdlt) {
               if (!(il->rmdlts & pmdlt->mdltbid)) continue;
               if (memcmp(tomk, tpmk, pmdlt->ndsbyt))
                  il->ctwk.go.delmk |= imnum;
               tomk += pmdlt->ndsbyt;
               tpmk += pmdlt->ndsbyt;
               imnum <<= 1;
               }
            /* Now set the old marks from the current marks */
            memcpy((char *)CDAT.pmarkh+il->iMarkh, pmark, il->lmm);
            }
#endif   /* not defined PAR0 */

/*---------------------------------------------------------------------*
*                                                                      *
*                           Loop over cells                            *
*                                                                      *
*---------------------------------------------------------------------*/

#ifdef PAR0
         /* Node zero has no cells, so detail print routine must
         *  be invoked outside cell loop (once per layer). */
         if (do_dprint) d3pdpz();
         il->ctf |= DIDSTATS;       /* Trigger stats */

#else  /*** BEGIN NODE ZERO EXCLUSION FROM LOOP OVER RESIDENT CELLS ***/

         /* Clear for dprnt items that may not be set at all */
         CDAT.dst = CDAT.sbar = CDAT.qbar = 0;

         /* Initialize tests that may be accumulated over trials
         *  or cycles (high, low responses, numbers of hits)  */
         if (statenbc) {
            lhisi = -SI32_MAX, llosi = SI32_MAX;
            nhitspt = nhitsht = 0;
#ifdef PARn
            qhlsi = CDAT.phlsi + CDAT.ihlsi*RP->nhlpt + il->iHLsi;
#endif
#ifndef PAR
            /* Used for hdist,ldist */
            ihisi = 0; ilosi = LDSTAT;
            pgp0 = CDAT.pgstat + il->oGstat;
#endif
            }

         coff = 0;
         llel = uw2zd(il->lel);
         for (icell=il->locell; icell<il->hicell; ++icell,coff+=llel) {
            struct PPFDATA  *ppfd;  /* Ptr to PPF parameters */
            rd_type *plsd;          /* Ptr to data for current cell */
            s_type  *psnew;         /* Ptr to new value of s(i) */
            byte    *pizu;          /* Ptr to Izhik. u data */
            byte    *pvmp;          /* Ptr to Vm(t-1) (S20) */
            si64 psiacc;            /* Accumulator for PSI (S30) */
            si64 vmterms;           /* Terms in newvm (mV S20/27) */
            si32 hacell;            /* Hebb amp cell term, S22 if
                                    *  (v-mtv)*(Si-mti), else S14 */
            si32 hbcell;            /* BCM amp cell term, S22 if (v-mtv)
                                    *  included, else S14 */
            si32 oldsi;             /* si(t-1) (mV S20/27) */
            si32 oldvm;             /* Vm(t-1) (mV S20/27) */
            si32 newvm;             /* Vm(t) during sum (mV S20/27) */
            si32 oldvmsi_amp;       /* = oldsi or oldvm for amp */
            si32 oldvmsi_dcy;       /* = oldsi or oldvm for decay */
            si32 response;          /* Final response si(t) (S20/27) */
            si32 rnd1,rnd2,rnd3;    /* Rounding values from rsd */
            si32 wkdst;             /* Working value of dst (mV S20) */
            si32 wkst;              /* Working value of st (mV S20) */
            int  hitht,hitpt,hitnt; /* TRUE if s(i)>=ht, >=pt, <=nt */
            int  ntrunchi,ntrunclo; /* sover,sunder stat increments */
            int relcell = icell - il->locell;
            psnew = il->ps2 + spsize(relcell, il->phshft);
            plsd = il->prd + coff;
            pizu = plsd + il->ctoff[IZU];    /* Maybe not used */
            pvmp = plsd + il->ctoff[VMP];

            /* Initialize CDAT structure, clear accumulators */
            CDAT.cdcell = icell;
            CDAT.noise  = 0;     /* Clear noise */
            CDAT.events = 0;     /* Say nothing happened yet */
            memset((char *)CDAT.pSpecSums, 0, CDAT.lSpecSums);
            memset((char *)CDAT.pTotSums, 0, CDAT.lTotSums);
            memset((char *)prbar, 0, WSIZE*(size_t)il->nct);
            psiacc = jesl(0);    /* Whether needed or not */
            hitpt = hitnt = ntrunchi = ntrunclo = 0;

            /* Locate consolidation tables */
            pcontbl = CDAT.ConsTables;

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

            /* Pick up Vm, sbar, qbar, si, phi & psi.  Pick up phase
            *  even if it doesn't exist, adjusting offset to avoid
            *  exceeding array bounds--probably faster than an 'if'
            *  statement and there is no code that assumes it has any
            *  particular value if it is not actually used.  N.B.  The
            *  term simavgn previously used in the calc of persistence
            *  used avgnoise on an incorrect scale (effectively divided
            *  by S16).  In V8B, subtract the scaled resting potential
            *  instead.  In V8C, zero si IS resting potential, so sub-
            *  traction is needed only in the COMPAT=C case.  In R76,
            *  use Vm instead of old_si except w/COMPAT = A or C.  */

            d3gtjl4a(oldvm, pvmp);
            if (ctexists(il,SBAR)) {
               rd_type *psbr = plsd + il->ctoff[SBAR];
               d3gtjs2a(CDAT.sbar, psbr);
               }
            if (ctexists(il,QBAR)) {
               rd_type *psbr = plsd + il->ctoff[QBAR];
               d3gtjs2a(CDAT.qbar, psbr);
               }
            if (il->orkam & KAMLP) {
               /* 1.0 - psi is more useful for amplification */
               rd_type *psbr = plsd + il->ctoff[PSI];
               d3gtjs2a(CDAT.ompsi, psbr);
               CDAT.ompsi = S15 - CDAT.ompsi;
               }
            {  s_type *psi = il->pps[0] + spsize(icell,il->phshft);
               if (il->Ct.rspmeth >= RF_BREG) {
                  /* Use Izhi/Gerst v (S20/27) as oldsi */
                  CDAT.old_si = oldvm;
                  CDAT.old_siS7 = jsrrsj(CDAT.old_si, (FBwk-FBsi));
                  }
               else {
                  d3gtjs2(CDAT.old_siS7, psi);
                  /* Scale s(i) to S20/27 for most working */
                  CDAT.old_si = CDAT.old_siS7 << (FBwk-FBsi);
                  }
               oldsi = CDAT.old_si;
               oldvmsi_amp = (RP->compat & OLDAMP)  ? oldsi : oldvm;
               oldvmsi_dcy = (RP->compat & OLDPERS) ? oldsi : oldvm;
               CDAT.pers = mrsrsl(il->Dc.omegau, oldvmsi_dcy, FBod);
               if (CDAT.pers) CDAT.events |= decay_bit;
               CDAT.old_phi = psi[1+il->phshft];
               } /* End psi local scope */

            /* Calculate self-input.  Note that the code for phase con-
            *  volution depends on the convention documented in getpcf()
            *  that all convolution kernel elements are positive, i.e.
            *  whether psi[p] term is excit or inhib depends only on
            *  the sign of simet*sisclu and this is used to pick the
            *  relevant ossei AffData selector.  (simet could be < 0
            *  if we implemented step thresholding with siet < 0.)  */
            if (il->sisclu) {
               si32 simet = oldsi - il->Dc.siet;   /* (mV S20) */
               if (simet > 0) {
                  si64 *psi;
                  e64dec(ject + CTO_RFN);
                  /* S20 + S24 - 24 = S20 */
                  simet = mrsrsld(simet, il->sisclu, FBsc);
                  psi = CDAT.pAffD64 + il->ossei[simet < 0];
                  if (il->phshft) {          /* Cells have phase */
                     register int i,p;
                     if (ppd->pselfpcf) {    /* Convolution method */
                        si32 sk, *pkern = ppd->pselfpcf->bins;
                        for (p=0; p<PHASE_RES; p++) {
                           i = (p-(int)CDAT.old_phi) & PHASE_MASK;
                           /* S20 + S28 - 28 = S20 */
                           sk = mrsrsld(simet, pkern[i], FBkn);
                           psi[p] = jesl(sk);
                           }
                        } /* End self-input convolution */
                     else {                  /* Uniform method */
                        for (p=0; p<PHASE_RES; p++)
                           psi[p] = jesl(simet);
                        }
                     } /* End phased self-input */
                  else {                     /* Cells do not have phase */
                     *psi = jesl(simet);
                     } /* End unphased self-input */
                  } /* End if simet > 0 */
               } /* End self-input */

            /* Refractory period control:  Pick up current dst and use
            *  it to adjust st.  dst < DST_CLOCK0 is refrac timer.  */
            wkst = il->Ct.st;          /* mV S20 */
            if (prf) {
               rd_type *pdst = plsd + il->ctoff[DST];
               d3gtjh2a(wkdst, pdst);  /* Fetch, retaining sign */
               if (wkdst > DST_CLK0S16) {
                  wkdst = SRA(wkdst,3);/* S7+16-3 --> S20 */
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
*---------------------------------------------------------------------*/

            for (ix=il->pct1; ix; ix=ix->pct) {

/*---------------------------------------------------------------------*
*     N.B. Four levels of indenting suppressed to end of conn loop.    *
*---------------------------------------------------------------------*/

   struct CONNDATA *pcnd0; /* Ptr to first CONNDATA for this CONNTYPE */
   si64 *pds0;             /* Ptr to start of detailed amp stats */
   si64 *raxp,*raxn;       /* Ptrs to pos/neg raw conntype sums */
   si64 *saxp,*saxn;       /* Ptrs to subarb & convolution sums */
   si64 *pexSums;          /* Ptr to excitatory sums */
   si64 *pinSums;          /* Ptr to inhibitory sums */
   si32 *pafp;             /* Ptr to EXCIT aff data for detail print */
   si32 *pafn;             /* Ptr to INHIB aff data for detail print */
   si64 Aij;               /* Cij * (Sj - sjrev) (FBrs=S23/30) */
   size_t lraxb;           /* Bytes in NEXIN*lrax si64's */
   size_t lrax1b;          /* Bytes in NEXIN*lrax1 si64's */
   long Lij;               /* Connection source index */
   si32 Cijppf;            /* Cij adjusted for PPF */
   si32 lmxsj;             /* Local copy of emxsj */
   si32 lepet,lenet;       /* Local copies of effet,effnt */
   si32 lspet,lsnet;       /* Local copies of subet,subnt */
   ui32 lkam;              /* Local copy of ix->Cn.kam */
   si32 Mij;               /* Modifying substance (S7/14) */
   si32 Mijincr,Mijdecr;   /* Mij increment and decrement */
   si32 Mijtest;           /* Test for Mij a clock */
   si32 simmti;            /* (Si - mti) (mV S7) */
   si32 Sj;                /* Sj (mV S7) - sjrev */
   si32 Sjtest;            /* Sj adjusted to fail et test */
   si32 wSj;               /* (Sj - et) or (Sj - etlo) for Aij */
   si32 wkmtj;             /* Working mtj or (RBAR - sjrev) */
   si32 wksi;              /* Working si or sbar */
   int  gotex,gotin;       /* sssck switches for excit,inhib */
   int  gotsj;             /* TRUE if Sj exists */
   int  jecn;              /* Offset to overflow counts */
   int  kami,kamj;         /* Amplif cases--this cell, connection */
   int  lraxw;             /* Local copy of lrax (wides in rax[pn]) */
   int  lrax1w;            /* Wides in raw sums w/ or w/o phase */
   int  lsaxw;             /* Local copy of lsax (wides in sax[pn]) */
   int  nexin;             /* Number of signs of input */
   int  Pj;                /* Phase of Sj */
   int  zbict;             /* Zero-based ict, ict+nct */
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
   char dopsiup;           /* TRUE if KAMLP and oldvmsi_amp > mti */

/* Exit now if bypassing this conntype */

   if (ix->kgfl & KGNBP) continue;

/* Copy updated phasing seed to current seed (even if never
*  used) and perform remaining conntype initialization */

   /* Locate overflow counts */
   jecn = (int)ix->iovec;

   /* Initialize Lij,Cij generation or look-up */
   d3kijx(ix);
   d3lijx(ix);
   d3cijx(ix);
   if (ix->Cn.kdelay) d3dijx(ix);
   d3sjx(ix);

   /* Pick up local copies of kam and effective thresholds */
   lkam = ix->Cn.kam;
   lmxsj = ix->Cn.emxsj;
   lepet = ix->effet, lenet = ix->effnt;
   lspet = ix->subet, lsnet = ix->subnt;

   /* Need conntype number to index items that
   *  must be accessed in non-threaded order */
   zbict = (int)ix->ict - 1;

/* Set pointers to storage for raw and subarbor or convolution Aij
*  data sums and their lengths.  paf[pn] hold raw sums, persistence,
*  and final sums across conntypes for detail print.  rax[pn] holds
*  raw, later processed (volt-dep, phase convolution, etc.) sums.
*  sax[pn] holds subarbor, volt-dep, and phase convolution temps.
*  rax[pn] and sax[pn] are reused per conntype.
*  N.B.  paf[pn] have room for (1) npafraw = PHASE_RES entries for raw
*        sums if PHASJ|PHASR, plus 1, (2) 1 entry if decay, (3) npaffin
*        = PHASE_RES entries if PHASJ|PHASR or convolution, else 1.
*        npafraw,npaffin are set in d3tchk.  npaftot, sum of npafraw
*        and npaffin, including decay part, which can change at Group
*        III time, is computed in d3lafs().
*  N.B.  Unphased sums are always at rax[pn][0], followed if PHASJ
*        or PHASR by phased sums at rax[pn][OPhA..PHASE_RES].  With
*        PHASC or PHASU, raxn,saxn start at [rs]axp[1] and are moved
*        out to [rs]axp[PHASE_RES+1] when phase is expanded.
*  N.B.  lrax, lrax1, lsax are set in d3tchk().  lrax and lrax1 are
*        1 if no phase.  lrax is (PHASE_RES+1) if any phasing, lrax1
*        only with (PHASJ|PHASR) (because with (PHASU|PHASC), early
*        loops can ignore phase).  lsax is same as lrax if there are
*        subarbors, volt-deps, or phase convolution (which need temps),
*        otherwise 0.
*  N.B.  pexSums,pinSums,pafp,pafn memory for excit,inhib data is only
*        allocated when PSP_POS,PSP_NEG and thus gotex,gotin are on.
*        Implication:  When an operation changes the sign of a term
*        (e.g. volt-dep scaling), the modified data must be moved to
*        the other buckets and therefore both buckets must be there
*        when sign changes are possible (forced in d3lafs).
*  N.B.  Memory for [rs]ax[pn] is always present and zero if not used,
*        to simplify sum storage.
*/

   {  int lsssck = (int)ix->sssck;
      gotex = lsssck & PSP_POS;
      gotin = lsssck & PSP_NEG;
      nexin = (int)npsps(lsssck);   /* 1 for EXCIT + 1 for INHIB */
      } /* End lsssck local scope */
   lraxw  = (int)ix->lrax;             /* Entries in one of rax[pn] */
   lrax1w = (int)ix->lrax1;            /* Portion used in early sums */
   lsaxw  = (int)ix->lsax;
#if WSIZE == 8
   lraxb  = NEXIN * lraxw << 3;        /* Size of raxp+raxn in bytes */
   lrax1b = NEXIN * lrax1w << 3;       /* Portion used in early sums */
#else
   lraxb  = NEXIN*sizeof(si64)*lraxw;
   lrax1b = NEXIN*sizeof(si64)*lrax1w;
#endif

   /* Locate arrays for individual type detail */
   pafp = CDAT.pAffD32 + ix->odpafs;
   pafn = (nexin > 1) ? pafp + ix->npaftot : pafp;

   /* Locate space for si64 working sums, expanded for subarbors
   *  and various phase options (later for PHASU|PHASC)  */
   raxp = prbar + il->nct; raxn = raxp + lrax1w;
   /* Create a segfault if sax[pn] are used when not allocated */
   if (lsaxw) saxp = raxn + lrax1w, saxn = saxp + lrax1w;
   else       saxp = saxn = NULL;

   /* Locate arrays for summation results */
   pexSums = CDAT.pAffD64 + ix->oasei[EXCIT];
   pinSums = CDAT.pAffD64 + ix->oasei[INHIB];

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
   dopsiup = (lkam & KAMLP) != 0;
   detstats = (lkam & KAMDS) ? statenbc : FALSE;
   if (sequence == SEQ_I) {
      decay_switch = SKIP_ALL;
      amp_switch = NO_AMP;
      }

   else {
      decay_switch = (sequence == SEQ_II) ? NO_DECAY : ix->dcybi;
      amp_switch = ix->ampbi;

/* Calculate hacell and kami for amplification.  KAMDQ option
*  revised, 08/24/94, to use average of sbar and mti, revised again
*  12/26/07 to use (qbar+mti)/2, and again 06/30/12 to use qpull-
*  weighted average.  Original version was not preserved, was used
*  only in tests.  KAMPA added, 06/16/12, to replace mti with QBAR,
*  no averaging.  Note that we pretend Si and Sj are S14 (i.e. 128mV
*  = 1.0 old-scale).  Removed rounding of hacell in V8C--more accurate
*  without it.  wkmnsj is mnsj test, but set to SHRT_MAX to kill this
*  cell at m(ij) update time if mnsi test fails here (KAMUE only).
*  Bug Fix, 10/14/16, GNR - Moved KAMCG setting of kami first so it
*     correctly does not OR in kamv from cycle initialization.  */

      wkmnsj = ix->Cn.mnsj;
      /* Set value if group number matches category--ignore kamv */
      if (lkam & KAMCG) {
         struct MODALITY *pm = ix->psmdlt;
         si16 tval = ix->cnwk.go.vmvt =
            ((si16)bittst(CDAT.pmbits + pm->mdltgoff,
            (long)CDAT.groupn%pm->ncs + 1) << FBvl) - ix->Cn.mtv;
         kami = 0;
         if      (tval <  0) kami |= KAC_VLO;
         else if (tval == 0) kami |= KAC_INELG;
         }
      else
         kami = (int)ix->cnwk.go.kamv;
      wksi = (lkam & (KAMSA|KAMZS|KAMTNU)) ?
         CDAT.sbar : jsrrsj(oldvmsi_amp, (FBwk-FBsi));
      if (lkam & (KAMDQ|KAMPA|KAMBCM)) {  simmti = wksi - CDAT.qbar;
         if (lkam & KAMDQ) {
            si32 tmti = (si32)ix->Cn.qpull*(CDAT.qbar - ix->Cn.mti);
                                          simmti += SRA(tmti,FBdf); }
         }
      else if (lkam & KAMKI)              simmti = S14;
      else                                simmti = wksi - ix->Cn.mti;
      /* '<=' tests kill amp if term is 0 and using default 0 test */
      if (wksi < ix->Cn.mnsi || wksi > ix->Cn.mxsi) {
         if (lkam & KAMUE) wkmnsj = SHRT_MAX;
         else              kami |= KAC_INELG;
         }
      if ((hacell = simmti) < 0) kami |= KAC_SILO;
      else if (simmti == 0)      kami |= KAC_INELG;
      if (kami & (KAC_SILO|KAC_INELG)) dopsiup = 0;
      if ((lkam & KAMVNU) && !(kami & KAC_INELG))
         /* Calc hacell = (si-mti)*(v-mtv) (S22) */
         hacell *= (si32)ix->cnwk.go.vmvt;
      if (lkam & KAMBCM)
         hbcell = mrssle(hacell, wksi, FBsi+Sr2mV, jecn+CNO_AMP);
      } /* End else (not SEQ_I) */

   /* Save rseed for deterministic updating */
   ix->cnwk.go.psvrseed = ix->rseed;

   doampavg = (finamp_switch != NO_FIN_AMP) && (ix->Cn.saopt & SAOPTA);
   if (doampavg) {            /* Clear Cij accumulators */
      memset((char *)CDAT.pavgcd, 0, ix->nsa*sizeof(struct AVGCDATA));
      }

/*---------------------------------------------------------------------*
*                         Loop over synapses                           *
*---------------------------------------------------------------------*/

/* Clear rax[pn] and sax[pn] (if used) totals */

   memset((char *)raxp, 0, lsaxw ? 2*lrax1b : lrax1b);

/* This loop is driven by Lija.isyn so that in cases where Lij's
*  are being calculated, the loop counter is available to the Lij
*  routines.  For the same reason, the conntype number, subarbor
*  number, etc. are also maintained in Lija.  */

   pcnd0 = pND = CDAT.pconnd + ix->ocdat;    /* Amplification data */
   for (Lija.isyn=0;Lija.isyn<Lija.nuk;++Lija.isyn,Lija.psyn+=ix->lc) {

/* Fetch or generate an Lij coefficient and break out if no
*  more connections will be generated for this cell.  */

      if (!Lija.lijbr(ix)) break;
      Lij = Lija.lijval;
      e64dec(jecn+CNO_SUMS);

/* If starting a new subarbor other than the first, check scaled net
*  input against mnax threshold.  If above threshold, accumulate into
*  sax[pn].  This leaves the last contribution in rax[pn] to be checked
*  and summed at the end of the synapse loop, which must be done anyway
*  to handle sudden exits when nuk < nc.  See notes at end of loop re:
*  scaling and overflow checking.
*  N.B.  The present mnax test spec allows sums for excit & inhib
*  conns to be consolidated in a single loop over lrax1w terms.
*  One might instead want separate tests on the excit & inhib sums.
*  This would require separate PSP_POS & PSP_NEG loops but would
*  allow a return to the previous code where unscaled sums are
*  compared to an inversely scaled mnax (wsmnax).
*  Rev, 08/03/12, GNR - Add SAOPTM method with separate tests on
*  excitatory and inhibitory sums.
*  Rev, 06/20/13, GNR - Scale before testing, because EXCIT,INHIB
*  scales now can be different.  Unroll rax[pn] summation loop.
*  Rev, 11/09/17, GNR - Handle correctly when scales are negative.  */

      if (Lija.jsyn > 0 && Lija.jsyn == Lija.isas) {
         register int p;
         if (ix->Cn.saopt & SAOPTM) {
            for (p=0; p<lrax1w; p++) {
               si64 tsum = jrsw(absw(raxp[p]), absw(saxp[p]));
               if (qsw(tsum) > 0) saxp[p] = raxp[p];
               tsum = jrsw(absw(raxn[p]), absw(saxn[p]));
               if (qsw(tsum) > 0) saxn[p] = raxn[p];
               }
            }
         else {
            si64 srex = mrsswd(raxp[0], ix->sclu[EXCIT], ix->scls);
            si64 srin = mrsswd(raxn[0], ix->sclu[INHIB], ix->scls);
            si64 tsum = jasw(srex, srin);
            tsum = jrsl(absw(tsum), ix->Cn.mnax);
            if (qsw(tsum) >= 0) for (p=0; p<lrax1w; p++) {
               saxp[p] = jasw(saxp[p], raxp[p]);
               saxn[p] = jasw(saxn[p], raxn[p]);
               }
            }
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
*     to -SI32_MAX does not catch cases where Sj < etlo.
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
            if (RP->knoup & KNUP_RB) {
               d3gtjs2(wkmtj, prbr); }
            else {
               if (ix->cnflgs & CNDOFS)
                  wkmtj = Sj;
               else {
                  si32 dmtj;
                  d3gtjs2(wkmtj, prbr);
                  dmtj = (ix->rseed & 0x00007fc0) +
                     (si32)ix->Cn.rdamp*(Sj - wkmtj);
                  wkmtj += SRA(dmtj, FBdf);
                  }
               d3ptjl2(wkmtj, prbr);
               }
            prbar[zbict] = jasl(prbar[zbict], wkmtj);
            if (ix->Cn.cnopt & NOPSJR) Sj -= wkmtj;
            wkmtj -= ix->Cn.sjrev;
            }
         if (!(lkam & KAMDR)) wkmtj = ix->Cn.mtj;
         Sjtest = Sj -= ix->Cn.sjrev;
         }
      else {
         ++ix->CNST.ncskip;
         /* Setting Sjtest = -INF here avoids need for gotsj
         *  tests when there is not a hi,lo threshold pair.  */
         Sj = 0, Sjtest = -SI32_MAX;
         wkmtj = ix->Cn.mtj;
         }
      Pj = ix->pjbr(ix);

/* Pick up or generate Cij (S31), then shift to (S16).
*  Save old value as new to simplify stats if no change.
*  The fundamental reason shifting is necessary is to detect
*  overflow/underflow in a higher-level language when Cij is
*  decayed or amplified (added to).  */

      ix->cijbr(ix);
      pND->new_cij = pND->old_cij = Lija.cijsgn;
      Cijppf = pND->curr_cij = SRA(Lija.cijval, 15);

/* Update PPF based on Sj(t).
*  N.B.  ppfij and ppflim are r' = r - 1 so baseline is 0.
*  PPF and PPFT may be signed, so use care fetching them.
*  PPFT is negative while PPF is rising, positive while decaying.  */

      if (ppfd) {
         si32 ppfij, ppftij;
         byte *psppf  = Lija.psyn + ix->cnoff[PPF];  /* Ptrs to PPF, */
         byte *psppft = Lija.psyn + ix->cnoff[PPFT]; /* PPF clock */
         d3gtjs2(ppfij,  psppf);             /* Pick up PPF */
         d3gtjs2(ppftij, psppft);            /* Pick up PPF clock */
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
               ppfij += mrsrsle(ppfd->upsp, ppfd->ppflim-ppfij,
                  28, ject+CTO_RFN);
               }
            else if (ppftij > 0) {
               /* In the declining phase */
               if (--ppftij == 0) ppfij = 0;
               else ppfij -= mrsrsle(ppfd->taup,ppfij,28,ject+CTO_RFN);
               }
            } /* End normal PPF */
         Cijppf *= (S12 + ppfij);
         Cijppf = SRA(Cijppf,12);
         d3ptjl2(ppfij,  psppf);
         d3ptjl2(ppftij, psppft);
         } /* End PPF update */

/* Accumulate Cij*(Sj-sjrev) (S23) if et,etlo threshold tests pass.
*  These sums are used for the subarbor mnax checks because they are
*  not affected by phase convolutions.  Aij and sums cannot overflow:
*  the two factors are each known to have 16 or fewer bits.  N.B.
*  Values of Sjtest and Sj are arranged to fail these tests if sjbr()
*  returned no Sj value.  */

/* Note added, 08/01/08 and 08/05/08, GNR:  Prior to Rev. 6, Sjtest
*  remained set from Sj(raw), not (Sj - Sjbar - Sjrev) and Sjtest was
*  tested against et but abs(Sj) against mabrsj.  Subtraction was
*  always STEP type.  KNEE type can be restored by use of COMPAT=K.
*  Subtraction is same as present if Sjrev = 0 and Sjbar not used.
*  Rev, 05/07/09, GNR - COMPAT=E and mabrsj eliminated.
*/

      Aij = jesl(0);

      /* wSj = sjval - sjrev - (et or etlo) */
      if (!gotsj) goto OmitAijSum;
      if      (Sj > lepet) {
         wSj = Sj - lspet;
         if (wSj > lmxsj) wSj = lmxsj; }
      else if (Sj < lenet) wSj = Sj - lsnet;
      else goto OmitAijSum;

      /* Aij, [rs]ax[pn] etc. are FBrs, i.e. Cijppf (S16) * Sj (S7/14).
      *  Later these are scaled by sclu and become FBwk = S20/27.  */
      Aij = jmsw(Cijppf, wSj);

      /* If TERNARY modification, apply it now */
      if (ix->pvpfb) {
         struct TERNARY *pvp = ix->pvpfb;
#ifdef DBGTER
         si64 oldAij = Aij;
         ulng oldOvec = CDAT.povec[jecn+CNO_TER];
#endif
         si32 imgd;
         e64dec(jecn+CNO_TER);
         imgd = pvp->dennoi +
            mrsrsld(pvp->denscl, pvp->qvpict->sjcur, FBim);
         Aij = dsrswjqd(Aij, FBtm, imgd);
#ifdef DBGTER
         if (oldOvec < 20 && CDAT.povec[jecn+CNO_TER] > oldOvec) {
            char dmsg[132], toldaij[8], tdennoi[8],
               tdenscl[8], tsjcur[8], tnewaij[8];
            wbcdwtPn(&oldAij, toldaij, (23<<RK_BS)|(4<<RK_DS)|
               RK_AUTO|RK_IORF|RK_NI64|7);
            wbcdwtPn(&pvp->dennoi, tdennoi, (20<<RK_BS)|(4<<RK_DS)|
               RK_AUTO|RK_IORF|RK_NI32|7);
            wbcdwtPn(&pvp->denscl, tdenscl, (20<<RK_BS)|(4<<RK_DS)|
               RK_AUTO|RK_IORF|RK_NI32|7);
            wbcdwtPn(&pvp->qvpict->sjcur, tsjcur, (14<<RK_BS)|
               (4<<RK_DS)|RK_AUTO|RK_IORF|RK_NI32|7);
            wbcdwtPn(&Aij, tnewaij, (23<<RK_BS)|(4<<RK_DS)|
               RK_AUTO|RK_IORF|RK_NI64|7);
            ssprintf(dmsg, " Trial %4d, Cycle %4d, Cell %8d, "
               "OldAij %8s, Dennoi %8s, Denscl %8s, Sjcur %8s, "
               "NewAij %8s", (int)RP->CP.trial, (int)RP->CP.effcyc,
               icell, toldaij, tdennoi, tdenscl, tsjcur, tnewaij);
            dbgprt(dmsg);
            }
#endif
         e64dec(jecn+CNO_SUMS);
         }

/* Sum Aij into raxp[0],raxn[0] according to sign.
*  R76, 11/05/17, GNR - Bug fix, NOPMX case assumed Aij > 0.
*   Now put sums in bins for opposite sign if scale is negative
*   so scaling in place gives correct sign later.  */

      if ((swhi(Aij) ^ ix->Cn.scl) >= 0) {
         if (ix->Cn.cnopt & NOPMX) {
            if (qsw(jrsw(absw(raxp[0]), absw(Aij))) < 0)
               raxp[0] = Aij; }
         else
            raxp[0] = jasw(raxp[0], Aij);
         }
      else {
         if (ix->Cn.cnopt & NOPMX) {
            if (qsw(jrsw(absw(raxn[0]), absw(Aij))) < 0)
               raxn[0] = Aij; }
         else
            raxn[0] = jasw(raxn[0], Aij);
         }

/* If cells have phase, accumulate the extrinsic inputs separately
*  by phase bin.  In V8A, this code was changed to provide temporary
*  summation arrays in pax[pn], thus allowing convolutions to be
*  deferred until all connections of this type have been processed.
*  In V8C, these sums were changed to si64 and moved to rax[pn].
*  In R76, early convolution was eliminated, now always deferred.
*
*  1) If Si has no phase, phase of Sj is ignored (ix->phopt==0).
*  2) If PHASU or C, can construct dist later based on rax[pn][0]
*     (d3tchk() will force PHASU if input not phased and PHASR or
*     PHASC not specified), so ignore incoming phase as in 1).
*  3) If PHASR, pjbr() will assign a random phase.
*  4) Otherwise, store input according to its assigned phase.  */

      if (ix->phopt & (PHASJ|PHASR)) {
         if ((swhi(Aij) ^ ix->Cn.scl) >= 0)
            raxp[Pj+1] = jasw(raxp[Pj+1], Aij);
         else
            raxn[Pj+1] = jasw(raxn[Pj+1], Aij);
         } /* End if phasing */
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

      e64dec(jecn+CNO_CDCY);
      switch (decay_switch) {

case NO_DECAY: /* Skip decay, but go on to amplification and DAS.
               *  (occurs in Sequence II or Sequence III).  */
         break;

case DECAYE:   /* Simple exponential decay */
         pND->curr_cij +=
            mrsrsl(ix->Cn.target - pND->curr_cij, ix->Cn.gamma, FBsc);
         break;

case DECAYB:   /* Exponential decay to baseline Cij */
         d3gthn(pND->old_cij0, Lija.psyn+ix->cnoff[CIJ0], ix->cijlen);
         /* Isolate nbc bits and check for special -0 case */
         if ((pND->old_cij0 &= ix->cmsk) == SI32_SGN) pND->old_cij0 = 0;
         pND->curr_cij0 = SRA(pND->old_cij0, 15);
         pND->curr_cij += mrsrsl(pND->curr_cij0 - pND->curr_cij,
            ix->Cn.gamma, FBsc);
         break;

case DECAYI:   /* Decay modified by individual mean s(i) */
         /* S16 + S14 + S24 - 38 (128mV taken as 1.0) --> S16 */
         pND->curr_cij += mrsrsld((ix->Cn.target - pND->curr_cij)*
            CDAT.sbar, ix->Cn.gamma, 38);
         break;

case SKIP_ALL: /* Skip decay, amplification and DAS and amplification
               *  completion all at once (occurs in Sequence I).  */
         goto AMP_SKIP_ALL;

         } /* End of decay switch */

/*---------------------------------------------------------------------*
*                        Perform Amplification                         *
*                                                                      *
* V8E, 05/08/09, GNR - psiway,psjway replaced by kamv,kami,kamj        *
*---------------------------------------------------------------------*/

   e64dec(jecn+CNO_AMP);

/* The code in the following switch performs those aspects of ampli-
*  fication that are necessary to prepare variables for DAS even if
*  amplification per se is being skipped.  sjmmtj is always (Sj-wkmtj)
*  for statistics, while mjmmtj is set to (Mij-wkmtj) if Mij exists,
*  else remains same as sjmmtj.  Note that Mij must be kept updated
*  even if the synapse is now ineligible for amplification.
*  Scheme:
*     kamv holds way state based on value only (changes per cycle).
*     kami holds way state based on value and Si-mti (per cell).
*     kamj holds way state based on all three terms.
*
*  Note:  In V7B, rounding of Mij was changed to use low-order bits of
*  rseed instead of constant.  This prevents Mij from converging on
*  stationary values.  Mijincr and Mijdecr are computed on scale S20
*  so bits are available for rounding.  It is assumed that the low-
*  order bits of rseed are sufficiently decorrelated from the high-
*  order bits used to round Cij--this is purely to save the time of
*  another udev call.  Code was changed in R67 so that if averaging
*  Cij over a subarbor, rseed is nonetheless updated for use here.  */

      pND->simmti = simmti;      /* Save simmti and vmvt for dprnt */
      pND->vmvt = (si32)ix->cnwk.go.vmvt;
      pND->sjmmtj = Sj - wkmtj;  /* S7|S14 for Mij calc. and stats */
      kamj = kami;
      if (pND->sjmmtj < 0) kamj |= KAC_SJLO;

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
         d3gtjs2(Mij, Lija.psyn+ix->cnoff[MIJ]);  /* Pick up old Mij */
         Mijincr = (Sjtest < wkmnsj) ? 0 :
            /* simmti and sjmmtj are effectively S14 and their 32-bit
            *  product cannot overflow.  upsm is S16 and wayscale is
            *  S10, so their 32-bit product is unlikely to overflow.
            *  The quadruple product is 64 bits at S(14+14+10+16) =
            *  S54, or S20 after rounding and scaling.  kamj can be
            *  inelg here due to vmvt == 0, but calc Mijincr anyway. */
            mrsrsld(simmti*pND->sjmmtj, ix->Cn.upsm*ix->Cn.wayset[
               kamj & (KAC_SILO|KAC_SJLO)], 34);

         /* Is Mij now acting as a timer?  If net delta Mij > 0,
         *  restart clock, otherwise, tick clock down, set mjmmtj
         *  back to zero if time is up.  */
         if (Mij <= Mijtest) {
            pND->mjmmtj = ix->Cn.mxmij;
            if (Mijincr > ix->cnwk.go.mntsmxmp) Mij = Mijtest;
            else if (--Mij <= -S15) pND->mjmmtj = Mij = 0;
            goto AMP_SAVE_Mij;
            }

         /* Mij is not acting as a timer, it is really Mij */
         Mijdecr = Mij * ix->Cn.zetam, Mijdecr = SRA(Mijdecr, 10);
         /* If zetam*m exceeds max pumping rate, use latter */
         if (abs32(Mijdecr) > ix->Cn.mxmp) {
            ix->CNST.mpover += 1;
            Mijdecr = (Mijdecr >= 0) ? ix->Cn.mxmp : -ix->Cn.mxmp; }
         /* Calc net delta Mij (S20), round, shift to S14, apply */
         Mijincr = Mijincr - Mijdecr + (ix->rseed & 0x0000003f);
         Mij += SRA(Mijincr, 6);
         /* When Mij exceeds mxmij, count instances, start ticking
         *  if KAMTS, otherwise just reset it.  */
         if (Mij > ix->Cn.mxmij) {
            ix->CNST.mover += 1;
            if (lkam & KAMTS) {
               pND->mjmmtj = ix->Cn.mxmij;
               Mij = Mijtest;
               goto AMP_SAVE_Mij;
               }
            Mij = ix->Cn.mxmij;
            }
         pND->mjmmtj = Mij;
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
         d3gtjs2(Mij, Lija.psyn+ix->cnoff[MIJ]);  /* Pick up old Mij */
         if (Sjtest >= wkmnsj)
            /* Sj (S14) * upsm (S16) - 10 yields Mijincr (S20) */
            Mijincr = Sj*ix->Cn.upsm, Mijincr = SRA(Mijincr, 10);
         else Mijincr = 0;

         /* Is Mij now acting as a timer?  If net delta Mij > 0,
         *  restart clock, otherwise, tick clock down, set Sj
         *  back to zero if time is up.  */
         if (Mij <= Mijtest) {
            pND->mjmmtj = ix->Cn.mxmij - wkmtj;
            if (Mijincr > ix->cnwk.go.mntsmxmp) Mij = Mijtest;
            else if (--Mij <= -S15) Mij = 0, pND->mjmmtj = -wkmtj;
            goto AMP_SAVE_Mij;
            }

         /* Mij is not acting as a timer, it is really Mij */
         Mijdecr = Mij*ix->Cn.zetam, Mijdecr = SRA(Mijdecr, 10);
         /* If zetam*m exceeds max pumping rate, use latter */
         if (abs32(Mijdecr) > ix->Cn.mxmp) {
            ix->CNST.mpover += 1;
            Mijdecr = (Mijdecr >= 0) ? ix->Cn.mxmp : -ix->Cn.mxmp; }
         /* Calc net delta Mij (S20), round, shift to S14, apply */
         Mijincr = Mijincr - Mijdecr + (ix->rseed & 0x0000003f);
         Mij += SRA(Mijincr, 6);
         /* When Mij exceeds mxmij, start ticking */
         if (Mij > ix->Cn.mxmij) {
            Mij = Mijtest;
            pND->mjmmtj = ix->Cn.mxmij - wkmtj;
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
         d3gtjs2(Mij, Lija.psyn+ix->cnoff[MIJ]);  /* Pick up old Mij */
         if (Sjtest >= wkmnsj)
            /* Sj (S14) * upsm (S16) - 10 yields Mijincr (S20) */
            Mijincr = Sj*ix->Cn.upsm, Mijincr = SRA(Mijincr, 10);
         else Mijincr = 0;

         Mijdecr = Mij*ix->Cn.zetam, Mijdecr = SRA(Mijdecr, 10);
         /* If zetam*m exceeds max pumping rate, use latter */
         if (abs32(Mijdecr) > ix->Cn.mxmp) {
            ix->CNST.mpover += 1;
            Mijdecr = (Mijdecr >= 0) ? ix->Cn.mxmp : -ix->Cn.mxmp; }
         /* Calc net delta Mij (S20), round, shift to S14, apply */
         Mijincr = Mijincr - Mijdecr + (ix->rseed & 0x0000003f);
         Mij += SRA(Mijincr, 6);
         /* When Mij exceeds mxmij, cap it and count instances */
         if (Mij > ix->Cn.mxmij) {
            Mij = ix->Cn.mxmij;
            ix->CNST.mover += 1;
            }
AMP_LABEL3:
         pND->mjmmtj = Mij - wkmtj;
         if (lkam & KAMWM)
            kamj ^= (kamj & KAC_SJLO) ^ (pND->mjmmtj < 0);
AMP_LABEL4:
         /* Save new Mij, not to exceed two bytes */
         if (Mij > (S15-1)) Mij = (S15-1);
         else if (Mij <= Mijtest) Mij = Mijtest + 1;
AMP_SAVE_Mij:
         d3ptjl2(Mij, Lija.psyn+ix->cnoff[MIJ]);
         break;

/*---------------------------------------------------------------------*
*                         HEBBAMP and NO_AMP                           *
*  Normal modified Hebb-style amplification or just decaying.          *
*  Set mjmmtj = sjmmtj and continue to DAS code.                       *
*---------------------------------------------------------------------*/

case HEBBAMP:
case NO_AMP:
         pND->mjmmtj = pND->sjmmtj;
         if (Sjtest < wkmnsj) kamj |= KAC_INELG;
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
*  Bug Fix, 10/14/16, GNR - Modification of kamj at AMP_LABEL3 was     *
*     not being reflected in stats because had separate kams.          *
*---------------------------------------------------------------------*/

      if (pND->mjmmtj == 0) kamj = KAC_INELG;
      if (kamj & KAC_INELG) kamj = KAC_INELG;
      if (detstats) {
         si64 *pdws;
         pdws = pds0 = ix->pdas + (gotsj ? kamj : KAC_INELG);
         pdws[OdasCt] = jasl(pdws[OdasCt], 1);       /* Case counts */
         pdws[OdasSi] = jasl(pdws[OdasSi], simmti);     /* (Si-mti) */
         pdws[OdasSj] = jasl(pdws[OdasSj], pND->sjmmtj);/* (Sj-mtj) */
         if (lkam & (KAMCG|KAMVM|KAMUE)) {              /* (v -mtv) */
            pdws[OdasVM] = jasl(pdws[OdasVM], pND->vmvt);
            pdws += LDR; }
         if (lkam & (KAMSA|KAMTS|KAMUE))                /* Sj = Mij */
            pdws[OdasVM] = jasl(pdws[OdasVM], pND->mjmmtj);
         } /* End of detailed amplification statistics */

/*---------------------------------------------------------------------*
*               Finally the actual delCij calculation.                 *
*       Rounding and Clean-Up Following Amplification and Decay        *
*
*  At this point we should have all the individual factors and their   *
*  statistics, even though triple product will be 0 if KAC_INELG.      *
*---------------------------------------------------------------------*/

      pND->wayscl = ix->Cn.wayset[kamj];
      pND->Cijprod = pND->Cijterm = 0;
      switch (finamp_switch) {

/*---------------------------------------------------------------------*
*                               BCMAFIN                                *
*       Completion of Bienenstock, Cooper, Munro amplification         *
*---------------------------------------------------------------------*/

case BCMAFIN:
         if (kamj & KAC_INELG) goto AMP_LABEL6;
         /* On entry here, mjmmtj contains Mij-wkmtj if KAMSA or KAMTS.
         *  Calc Cijprod = scaled and rounded delta Cij
         *     = Si*((Sj or Mij)-wkmtj)*(Si-mti)*(v-mtv)*wayscale.
         *  If KAMVM:  S14+S14+S8+S10-30 = S16,
         *  otherwise: S14+S14+S10-22 = S16.  */
         pND->Cijprod = mrsrsld(pND->mjmmtj *
            (si32)ix->Cn.wayset[kamj], hbcell, (int)ix->sms);
         goto AMP_LABEL5;

/*---------------------------------------------------------------------*
*                               ELJTFIN                                *
*          KAMUE eligibility trace amplification completion            *
*                                                                      *
*  Note:  Sj here is actually Mij before range check.  Do not subtract *
*  mtj or use wayscales here because both were already done as Mij was *
*  being accumulated over time.                                        *
*---------------------------------------------------------------------*/

case ELJTFIN:
         if (kamj & KAC_INELG) goto AMP_LABEL6;
         /* Calc Cijprod = scaled and rounded delta Cij
         *     = Mij*(v-mtv)  (S14+S8-6 = S16)  */
         pND->Cijprod = pND->mjmmtj*pND->vmvt + 32;
         pND->Cijprod = SRA(pND->Cijprod, 6);
         goto AMP_LABEL5;

/*---------------------------------------------------------------------*
*                               NORMAMP                                *
*          Completion of normal, slow, or tick amplification           *
*---------------------------------------------------------------------*/

case NORMAMP:
         if (kamj & KAC_INELG) goto AMP_LABEL6;
         /* On entry here, mjmmtj contains Mij-wkmtj if KAMSA or KAMTS.
         *  Calc Cijprod = scaled and rounded delta Cij
         *     = ((Sj or Mij)-wkmtj)*(Si-mti)*(v-mtv)*wayscale.
         *  If KAMVM:  S14+S14+S8+S10-30 = S16,
         *  otherwise: S14+S14+S10-22 = S16.  */
         pND->Cijprod = mrsrsld(pND->mjmmtj *
            (si32)ix->Cn.wayset[kamj], hacell, (int)ix->sms);

AMP_LABEL5:    /* Code common to ELJTFIN and NORMAMP */
         {  si32 Cijterm, phiarg;
            switch (ix->Cn.phifn) {
            case FFN_DFLT:
               if ((pND->Cijprod ^ pND->curr_cij) >= 0) {
                  if ((phiarg = absj(pND->curr_cij)) >= ix->bigc16)
                     goto AMP_LABEL6;
                  phiarg = (phiarg>>11) & 0x1f;  /* To (S7)/4 */
                  }
               else phiarg = 0;
               break;
            case FFN_UNIT:
               phiarg = 0;
               break;
            case FFN_ABSCIJ:
               if ((phiarg = absj(pND->curr_cij)) >= ix->bigc16)
                  goto AMP_LABEL6;
               phiarg = (phiarg>>11) & 0x1f;  /* To (S7)/4 */
               break;
               } /* End phifn switch */
            /* Form mdelta*phi*(si-mti)*((Sj or Mij)-wkmtj) (S16):
            *  Cijprod is S16, phitab is S28, 28+16-28 = 16 */
            Cijterm = mrsrsld(pND->Cijprod, ix->phitab[phiarg], 28);
            /* If there is phase, multiply Cijterm by a cosine-like
            *  function of the phase difference. S16 + S28 - 28 = S16.
            *  d3tchk() sets pamppcf to NULL if there is no phase.  */
            if (ix->Cn.pamppcf) {
               int dif = ((int)CDAT.old_phi-Pj) & PHASE_MASK;
               Cijterm = mrsrsld(Cijterm,ix->Cn.pamppcf->bins[dif],28);
               } /* End phasing */
            /* If there is KAMLP (amplification refractoriness),
            *  multiply Cijterm by (1 - PSI). S16+S15-15 = S16.  */
            if (lkam & KAMLP) Cijterm =
               mrsrsld(Cijterm, CDAT.ompsi, FBdf);
            /* Apply delta to Cij */
            pND->curr_cij += (pND->Cijterm = Cijterm);

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
            struct AVGCDATA *pcsums = CDAT.pavgcd +
               (Lija.jsyn % (si32)ix->nsa);
            pcsums->sum_cij = jasl(pcsums->sum_cij, pND->curr_cij);
            ++pcsums->num_cij;
            /* Bug fix, 12/29/16: The cijclean() call below might do
            *  one or two udevs, but this code is still deterministic
            *  relative to number of comp nodes as doampavg has no
            *  dependence on the data.  */
            udev(&ix->rseed);    /* For Mij calcs above */
            } /* End Cij averaging */
         else {
            /* Here when not averaging subarbor Cij */
            pND->narb = 1;
            cijclean(ix, pND);
            pND->delCij = SRA((pND->new_cij - pND->old_cij), Ss2hi);
            /* Need to store Cij, Cij0 only if changed */
            if (pND->delCij) d3pthn(pND->new_cij,
               Lija.psyn+ix->cnoff[CIJ], ix->cijlen);
            if (ix->cnwk.go.vrho && pND->new_cij0 != pND->old_cij0)
               d3pthn(pND->new_cij0, Lija.psyn+ix->cnoff[CIJ0],
                  ix->cijlen);
            /* Accumulate sum of delCij^2 for KAMLP if needed */
            if (dopsiup) psiacc = jasw(psiacc,
               jmsw(pND->delCij, pND->delCij));
            } /* End no Cij averaging */
         break;

case NO_FIN_AMP:  /* No change in Cij--don't do anything except
         *  update rseed if needed for Mij, Rbar rounding */
         if (ix->cnflgs & UPRSD) udevskip(&ix->rseed, ix->upcij1s);
         break;
         } /* End of amplification completion switch */
AMP_SKIP_ALL:
      ++pND;

      } /* End of loop over connections (synapses) */

/*---------------------------------------------------------------------*
*       Finished processing inputs to this connection type, now        *
*              complete processing the connection type.                *
*---------------------------------------------------------------------*/

   e64dec(jecn+CNO_RED);         /* Set proper overflow recording */

   /* Save Sj for TERNARY or other use (nc == 1 enforced in d3tchk) */
   ix->sjcur = Lija.sjval;

/* Perform Cij averaging if necessary */

   if (finamp_switch != NO_FIN_AMP) {
      if (doampavg) {
         register rd_type *pacd = ix->psyn0 + coff + ix->lxn;
         rd_type *psynx;         /* Ptr to skipped synapses */
         struct AVGCDATA *pcija = CDAT.pavgcd;
         struct AVGCDATA *pcijae = pcija + ix->nsa;
         struct CONNDATA *pcnd, *pcndj = pcnd0;
         long iskp,isub,isyn,skipndx;
         /* Average Cij over equivalent amplified values */
         for ( ; pcija<pcijae; ++pcndj,++pcija) {
            pcndj->curr_cij = drswq(pcija->sum_cij, pcija->num_cij);
            pcndj->narb = pcija->num_cij;
            /* When cijclean() returns, adjusted average curr_cij
            *  has been copied to pcndj->new_cij.
            *  Note that data needed by cijclean, other than
            *  new curr_cij calculated here, are left in CONNDATA
            *  struct as a result of first connection loop above. */
            cijclean(ix, pcndj);
            } /* End subarbor loop */

         /* Now it is necessary to loop over all the connections
         *  again, picking up and saving relevant averaged Cij.
         *  Setup in d3allo and d3genr assures that a skip list
         *  exists.  */
         if ((iskp = (long)ix->nc - Lija.nuk) > 0) {
            psynx = Lija.pxnd + ix->lct - ix->nuklen;
            d3gtln(skipndx, psynx, ix->nuklen); }
         else
            skipndx = -1;
         for (isyn=0; isyn<(long)ix->nc; ++isyn) {
            si32 dcij;     /* S31 - S16 = S15 */
            /* Skip over connections that were skipped above.  Use
            *  counts established above, as Lij may not be stored. */
            if (isyn == skipndx) {
               if (--iskp > 0) {
                  psynx -= ix->nuklen;
                  d3gtln(skipndx, psynx, ix->nuklen); }
               else
                  skipndx = -1;
               continue;   /* Skip rest of connection processing */
               } /* End skipped connection processing */
            /* Maintain connection index within current subarbor */
            isub = isyn % (long)ix->nsa;
            /* Locate averaged data for this connection */
            pcndj = pcnd0 + isub;
            pcnd  = pcnd0 + isyn;
            pcnd->new_cij = pcndj->new_cij;
            dcij = SRA((pcnd->new_cij - pcnd->old_cij), Ss2hi);
            /* Save the change for possible detail print--skip the
            *  'if' statement to save time.  */
            pcnd->delCij = dcij;
            /* Bug fix, 12/29/16:  Was saving only if dcij != 0, but
            *  dcij does not have all the bits of new_cij.  */
            d3pthn(pcnd->new_cij, pacd+ix->cnoff[CIJ], ix->cijlen);
            if (ix->cnwk.go.vrho && pcnd->new_cij0 != pcnd->old_cij0)
               d3pthn(pcnd->new_cij0, pacd+ix->cnoff[CIJ0], ix->cijlen);
            pacd += ix->lc;
            /* Accumulate sum of dcij^2 for KAMLP if needed */
            if (dopsiup) psiacc = jasw(psiacc, jmsw(dcij, dcij));
            } /* End connection loop */
         } /* End if doampavg */
      } /* End if finamp_switch */

/* Advance rseed over nc connections if did rounding */

   ix->rseed = ix->cnwk.go.psvrseed;
   if (ix->cnflgs & UPRSD)
      udevskip(&ix->rseed, ix->upcij);

/* If doing subarbors:  Accumulate last (or only) subarbor
*  contribution if it exceeds mnax.  Copy totals back to rax.  */

   if (ix->Cn.saopt & DOSARB) {
      register int p;
      if (ix->Cn.saopt & SAOPTM) {
         for (p=0; p<lrax1w; p++) {
            si64 tsum = jrsw(absw(raxp[p]), absw(saxp[p]));
            if (qsw(tsum) > 0) saxp[p] = raxp[p];
            tsum = jrsw(absw(raxn[p]), absw(saxn[p]));
            if (qsw(tsum) > 0) saxn[p] = raxn[p];
            }
         }
      else {
         si64 srex = mrsswd(raxp[0], ix->sclu[EXCIT], ix->scls);
         si64 srin = mrsswd(raxn[0], ix->sclu[INHIB], ix->scls);
         si64 tsum = jasw(srex, srin);
         tsum = jrsl(absw(tsum), ix->Cn.mnax);
         if (qsw(tsum) >= 0) for (p=0; p<lrax1w; p++) {
            saxp[p] = jasw(saxp[p], raxp[p]);
            saxn[p] = jasw(saxn[p], raxn[p]);
            }
         }
      memcpy((char *)raxp, (char *)saxp, lrax1b);
      }

/*---------------------------------------------------------------------*
*   Final scaling, mnax, voltage-dependence, phase convolution,        *
*                   cappinjg, decay, and summation                     *
*---------------------------------------------------------------------*/

/* N.B.  Before this code, unphased sums are in rax[pn][0], and phased
*  contributions in rax[pn][OPhA..32] (OPhA=1 and is just used to label
*  these instances for easy grepping).  PHASC and PHASU phasing can be
*  handled as unphased until just before convolutions are done.  Raw
*  contributions can now be scaled and reduceed to S20.  Results are
*  now stored back in same locations (rax[pn][0..32]), and in *paf[pn]
*  for possible use in detail print (printed even if the contribution
*  is below mnax).  Then, if ADCY decay exists, persistence term
*  (32-bits) goes into *paf[pn], followed by the final (scaled, capped,
*  volt-dep, convoluted, etc.)  values.  paf[pn],peipv are allocated
*  only when corresponding sssck bit is set, unlike rax[pn],sax[pn],
*  which always exist.
*
* As of 12/27/08, code previously included here to avoid overflow of
*  64-bit intermediate scaling product by prescaling based on bitsz
*  of sum has been removed, and msswe replaced by msrswe.  This is
*  possible because msswe,msrswe now keep a 94-bit intermediate
*  product before shifting.  The total scaling shift is 27 bits
*  = 24 for scl (S24) plus 3 for S23 to S20 conversion.
*  (After 12/25/17, scl can be S20 with scls shift 23.)
*
* Scaled (after 06/21/13), unphased sums are saved in CDAT.praffs
*  (by conntype) for mnax check and used later for afference stats
*  and output to SIMDATA file.  Keeping si64 here makes overflow
*  during these operations very unlikely and so no checking is
*  necessary.
*
* Rev, 04/11/09, GNR - Added sssck tests where needed to avoid storing
*  into nonexistent paf[pn],peipv array elements, and elsewhere where
*  it might save a little time (e.g. multiple msrswe calls).
*
* Rev, 04/23/12, GNR - Now perform mxax checks on [rs]ax[pn] at 64-bit
*  precision in order to eliminate overflow error when reducing results
*  to 32 bits for paf[pn].  This moves mxax checks before convolution
*  and volt-dep calcs (11/08/17:  Moved back after volt-dep scaling).
*
* Rev, 05/23/12, GNR - Add CNOPT=C option to combine pos/neg terms
*  before mxax checking for better cancellation of large inputs.
*  Further revised, 05/27/12, to save scaled contribs before
*  combining in new 64-bit pAffD64 area.
*
* Rev, 06/20/13, GNR - Because a tested mode (KAUT_W) created a situ-
*  ation where excitatory and inhibitory terms could have different
*  scales, all scaling is now done before terms are combined (at some
*  cost in more arithmetic).  This eliminated wsmxax, wsmnax.
*
* Rev, 08/16/13, GNR - Eliminate redundant scaling of rax[pn][0]
*  when there is no phase, round as in phase bins.  Reorder tests
*  for faster path when no phasing.
*
* Rev, 08/21/13, GNR - Remove 64-bit raw sum storage for detail print.
*  This simplifies d3lafs() and d3pdpn() but requires swloem overflow
*  checking.  Values outside 32-bit range are ridiculous anyway, and
*  will be evident in the detail print.
*
* Rev, 11/08/17, GNR - Moved mxax testing after volt-dep scaling.
*  No longer move scaled phased inputs down to rax[pn][0].
*/

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*
*  N.B.  Before this code, if scales are negative, contributions in    *
*  raxp,raxn are reversed so they are correct after scaling in place.  *
*  N.B.  Here is where raxp,raxn S23/30 becomes S20/27.                *
*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/

   if (gotex) {
      register int p;
      for (p=0; p<lrax1w; p++) {
         raxp[p] = mrsrswd(raxp[p], ix->sclu[EXCIT], ix->scls);
         swlodm(pafp[p], raxp[p]);
         }
      pafp += lrax1w;
      }
   if (gotin) {
      register int p;
      for (p=0; p<lrax1w; p++) {
         raxn[p] = mrsrswd(raxn[p], ix->sclu[INHIB], ix->scls);
         swlodm(pafn[p], raxn[p]);
         }
      pafn += lrax1w;
      }

/* The raw (but scaled) excit+inhib sums are saved whether or not they
*  will be used for an 'R(x)' distribution to save an 'if'.
*  The mnax test can now be applied, always to the scaled sums before
*  vdep, convolution, and decay, because if it fails, no more calcula-
*  tions are needed. The same test is used earlier on subarbors before
*  including.  As of V8C, omit if unphased total is below mnax whether
*  or not there are phases or subarbors.  But even in this case, any
*  decay contribution--none is possible if there are phases--should
*  be updated and saved for dprt and conductance activation.  */

   {  si64 tsum, tspn = jasw(raxp[0], raxn[0]);
      CDAT.praffs[zbict] = tspn;
      tsum = jrsl(absw(tspn), ix->Cn.mnax);
      if (qsw(tsum) < 0) {
         si64 pnsum = jesl(0);
         size_t lpafcb = sizeof(si32) * ix->npaffin;
         if (gotex) {
            if (qdecay(&ix->Cn.ADCY)) {
               si32 capex = d3decay(&ix->Cn.ADCY,
                  ix->cnwk.go.paeff, pafp, 0);
               ix->cnwk.go.paeff += ix->Cn.ADCY.ndal;
               pafp += ix->Cn.ADCY.ndal;
               pafp[0] = capex;
               pexSums[0] = jasl(pexSums[0], capex);
               pnsum = jasl(pnsum, capex);
               }
            else
               memset((char *)pafp, 0, lpafcb);
            }
         if (gotin) {
            if (qdecay(&ix->Cn.ADCY)) {
               si32 capin = d3decay(&ix->Cn.ADCY,
                  ix->cnwk.go.paeff, pafn, 0);
               ix->cnwk.go.paeff += ix->Cn.ADCY.ndal;
               pafn += ix->Cn.ADCY.ndal;
               pafn[0] = capin;
               pinSums[0] = jasl(pinSums[0], capin);
               pnsum = jasl(pnsum, capin);
               }
            else
               memset((char *)pafn, 0, lpafcb);
            }
         CDAT.pfaffs[zbict] = pnsum;
         goto SKIP_CONNTYPE_CONTRIBUTION;
         }
      } /* End tsum local scope */

/* Handle voltage-dependent connections.  Multiply voltage-dependent
*  afferents binwise by thresholded input from previous activity.
*  Afference and output from this calculation are scaled and S20.
*  (Phase convolution, if any, now always comes after this calc.)
*
*  Note:  When volt-dep scaling changes the sign of an afference term,
*  and VDOPN is not specified to zero it, for adherence to rule that
*  gotex,gotin bzw always refer to pos,neg terms, the scaled sum must
*  be added into the opposite container.  N.B. this requires both
*  gotex and gotin to be set via code in d3lafs setting ix->sssck,
*  but these can be negated by spspov, so still must check here.
*
* Rev, 11/06/15, GNR - To allow for mix of vdopt=P,C, or S.  NormSums
*  now will not include pers term, just added to tns as needed.
* R76, 10/14/17, GNR - Add storage of output for distribution stat.
* Rev, 10/31/17, GNR - Divide by 2*(vdha - vdt), not just 2*vdha.
* Rev, 11/07/17, GNR - Eliminate old multiply by current inputs.
* Rev, 11/10/17, GNR - Result now stored back over rax[pn], includes
*                      unphased and PHASJ|PHASR terms.
* R78, 06/21/18, GNR - Now allow P, V, or S as sources
*/

/* Pound-define VDDBG to activate debug output */
/* #define VDDBG */

   if (ix->Cn.vdopt & VDANY) {
      si64 *svaxp,*svaxn;  /* Where to store results if sign change */
#ifdef VDDBG   /*** FOR DEBUG BELOW */
      si64 oldraxp;
      si32 num;
#endif
      si32 tns;            /* Thresholded prev activity, S20->S24 */
      si32 den = ix->Cn.vdha - ix->Cn.vdt;   /* Normalizer */
      register int p;                        /* Phase index */

      e64dec(jecn + CNO_VDEP);
      /* Select source according to priority V > P > S */
      if      (ix->Cn.vdopt & VDOPV)   tns = oldvm;
      else if (ix->Cn.vdopt & VDOPP)   tns = CDAT.pers;
      else if (RP->compat & OLDVDOPS)  tns = oldvm;
      else                             tns = oldsi;
      tns -= ix->Cn.vdt;
      if (ix->Cn.vdopt & VDOIN) tns = -tns;
#ifdef VDDBG
      oldraxp = raxp[0];
      num = tns;
#endif
      /* tns/den:  S(20+24-20-1) ==> S24, extra right
      *  shift is because den is half-effective level
      *  (den is guaranteed to be nonzero).  */
      tns = jdswqd(jslsw(jesl(tns),FBsc-1), den);
      if (tns < ix->Cn.vdmm) tns = ix->Cn.vdmm;
      /* Adjust tns using sigmoidal method */
      if (ix->Cn.vdopt & VDOPH) tns = (si32)(dS24 *
         (1.0 - 1.0/cosh((double)tns*VDHAMULT)));
      /* Setup to reverse results if sign changes.
      *  N.B.  If there is any chance that tns can be negative,
      *  gotex and gotin will both be set and there is no need
      *  to zero the temp storage in sax[pn].  */
      if (tns >= 0) svaxp = raxp, svaxn = raxn;
      else          svaxp = saxn, svaxn = saxp;
      if (gotex) for (p=0; p<lrax1w; p++)
         svaxp[p] = mrsrswdm(raxp[p], tns, FBsc);
      if (gotin) for (p=0; p<lrax1w; p++)
         svaxn[p] = mrsrswdm(raxn[p], tns, FBsc);
      /* Copy results back in case of sign change */
      if (tns < 0) memcpy(raxp, saxp, lrax1b);

#ifdef VDDBG    /*** DEBUG OUTPUT ***/
      {  si64 tsum = jasw(raxp[0], raxn[0]);
         int ibc, bcells[] = { 22806, 25349, 90740, 93812, 93823,
            500527, 500540, 503084, 505628, 505639 };
         char wnum[8],wden[8],wo[8],wp[8],ws[8];
         for (ibc=0; ibc<10; ++ibc) if (icell == bcells[ibc]) {
            wbcdwtPn(&num, wnum, 20<<RK_BS|4<<RK_DS|RK_IORF|RK_NI32|7);
            wbcdwtPn(&den, wden, 20<<RK_BS|4<<RK_DS|RK_IORF|RK_NI32|7);
            wbcdwtPn(&oldraxp,wo,20<<RK_BS|4<<RK_DS|RK_IORF|RK_NI64|7);
            wbcdwtPn(raxp,    wp,20<<RK_BS|4<<RK_DS|RK_IORF|RK_NI64|7);
            wbcdwtPn(&tsum,   ws,20<<RK_BS|4<<RK_DS|RK_IORF|RK_NI64|7);
            dbgprt(ssprintf(NULL," Trial %d, cell %d, ict %d, num %8s, "
               "den %8s, oldraxp %8s, raxp %8s, tsum %8s", RP->CP.trial,
               icell, (int)ix->ict, wnum, wden, wo, wp, ws));
            break;
            }
         }
#endif
      } /* End voltage-dependent adjustment */

/* If input had constant or uniform phase, distribute accordingly.
*  Advance [rs]axn ptrs to now head arrays with phase (same as
*  initial setup with PHASJ|PHASR).
*
*  R76, 11/05/17, GNR - Code to store scaled results of early con-
*  volution in sax[pn][0-31] was removed.  Code to copy everything
*  from rax[pn] to sax[pn] also removed--just keep working in rax[pn].
*/

   if (ix->phopt & (PHASC|PHASU)) {
      si64 srex = raxp[0], srin = raxn[0];
      register int p;
      raxn = raxp + lraxw;
      if (lsaxw) saxp = raxn + lraxw, saxn = saxp + lraxw;

      if (ix->phopt & PHASC) {            /* Constant phase */
         memset((char *)raxp, 0, lraxb);  /* (Maybe 2 extra) */
         p = (int)ix->Cn.phi + OPhA;
         /* Either one of these might be 0 if nexin == 1 */
         raxp[p] = raxp[0] = srex, raxn[p] = raxn[0] = srin;
         }
      else {                              /* Uniform phase */
         if (gotex) for (p=0; p<lraxw; p++)
            raxp[p] = srex;
         if (gotin) for (p=0; p<lraxw; p++)
            raxn[p] = srin;
         }
      }

/* Finally, apply phase convolution if requested (now always deferred
*  until after volt-dep--previously sometimes early if nc < 32 for
*  better speed).  sax[pn] will have been allocated for use as temp
*  space.  Both input and output are in rax[pn][OPhA..lraxw].
*  Rev, 08/17/13, GNR - Use msswe, not msrswe, so calc is same as
*     early convolution with no rounding.  */

   if (ix->phopt & PHASCNV) {
      si32 kerj, *pkern = ix->Cn.ppcf->bins;
      int j,p,pmj;
      e64dec(ppd->iovec);
      if (gotex) {
         memset((char *)saxp, 0, laffb);
         for (j=0; j<PHASE_RES; j++) {
            if (kerj = pkern[j])          /* Assignment intended */
                  for (p=0; p<PHASE_RES; p++) {
               si64 tsum;
               pmj = (p - j) & PHASE_MASK;
               /* S20 + S28 - 28 leaves S20 result */
               tsum = mrsswd(raxp[pmj+OPhA], kerj, FBkn);
               saxp[p] = jasw(saxp[p], tsum);
               }  /* End convolving one kernel bin */
            } /* End loop over kernel bins */
         memcpy((char *)(raxp+OPhA), (char *)saxp, laffb);
         }
      if (gotin) {
         memset((char *)saxn, 0, laffb);
         for (j=0; j<PHASE_RES; j++) {
            if (kerj = pkern[j])          /* Assignment intended */
                  for (p=0; p<PHASE_RES; p++) {
               si64 tsum;
               pmj = (p - j) & PHASE_MASK;
               /* S20 + S28 - 28 leaves S20 result */
               tsum = mrsswd(raxn[pmj+OPhA], kerj, FBkn);
               saxn[p] = jasw(saxn[p], tsum);
               }  /* End convolving one kernel bin */
            } /* End loop over kernel bins */
         memcpy((char *)(raxn+OPhA), (char *)saxn, laffb);
         }
      } /* End deferred convolution */

/* Now that all transformations on afferent data have been performed,
*  apply the mxax 'capping' test.
*  R76, 11/13/17, GNR - By keeping phased afferences at offset
*   OPhA = 1, cases with and without phasing can be handled by
*   same code with just different loop limits.
*/

   e64dec(jecn+CNO_RED);
   {  si64 tsum, tmxax = jesl(ix->Cn.mxax);
      if (ix->cnflgs & DOCEI) {     /* Combine before mxax test */
         /* In this case, d3news guarantees that both excit and
         *  inhib storage areas are present, so no need to test  */
         register int p;
         for (p=0; p<lraxw; p++) {
            tsum = jasw(raxp[p], raxn[p]);
            if (qsw(tsum) >= 0) {
               if (qsw(jrsw(tsum, tmxax)) > 0)
                  raxp[p] = tmxax, raxn[p] = jesl(0);
               }
            else {
               if (qsw(jasw(tsum, tmxax)) < 0)
                  raxn[p] = jnsw(tmxax), raxp[p] = jesl(0);
               }
            }/* End phase loop */
         }/* End "combine before test" */
      else {                        /* Test excit,inhib separately */
         /* Revised earlier code now guarantees that raxp has
         *  excitatory sums and raxn has inhibitory sumes.  */
         if (gotex) {
            register int p;
            for (p=0; p<lraxw; p++)
               if (qsw(jrsw(raxp[p], tmxax)) > 0) raxp[p] = tmxax;
            }
         if (gotin) {
            register int p;
            for (p=0; p<lraxw; p++)
               if (qsw(jasw(raxn[p], tmxax)) < 0) raxn[p] = jnsw(tmxax);
            }
         } /* End else not CODEI */
      } /* End tsum,tmxax local scope */

/* Perform decay and save final sums in pafp,pafn (but if constant
*     or uniform phase and no convolution or volt-dep, just save the
*     one unique value) and accumulate in pexSums,pinSums.
*  Save fully processed contribution for conductance activation.
*  Rev, V7C - Test against mnax removed because it was redundant
*             with tests applied individually to subarbor sums.
*  Rev, V8C - Test against mnax now also applied to total of all
*             subarbors, as it should have been since V7C.  Test
*             for overflow in pexSums,pinSums removed because it
*             is considered unnecessary now that sums are S20 (and
*             and in any event can be prevented with suitable mxax).
*  Rev, V8E - Bug fix:  ix->aeff only exists if sssck bits set.
*  Rev, V8H - mxax test now performed before convolution & voltdep.
*  R76, 11/13/17, GNR - mxax test moved back after conv & voltdep.
*/

   if (!(ix->phopt)) {
      /* No phase, currently only case that applies decay */
      si64 pnsum = jesl(0);
      if (gotex) {                   /* Excitation */
         si32 capex;
         swlodm(capex, raxp[0]);
         if (qdecay(&ix->Cn.ADCY)) {
            capex = d3decay(&ix->Cn.ADCY,
               ix->cnwk.go.paeff, pafp, capex);
            ix->cnwk.go.paeff += ix->Cn.ADCY.ndal;
            pafp += ix->Cn.ADCY.ndal;
            }
         pafp[0] = capex;
         pexSums[0] = jasl(pexSums[0], capex);
         pnsum = jasl(pnsum, capex);
         }
      if (gotin) {                   /* Inhibition */
         si32 capin;
         swlodm(capin, raxn[0]);
         if (qdecay(&ix->Cn.ADCY)) {
            capin = d3decay(&ix->Cn.ADCY,
               ix->cnwk.go.paeff, pafn, capin);
            ix->cnwk.go.paeff += ix->Cn.ADCY.ndal;
            pafn += ix->Cn.ADCY.ndal;
            }
         pafn[0] = capin;
         pinSums[0] = jasl(pinSums[0], capin);
         pnsum = jasl(pnsum, capin);
         }
      CDAT.pfaffs[zbict] = pnsum;
      } /* End decay & save */

   else if (ix->phopt & (PHASJ|PHASR|PHASCNV|PHASVDU)) {
      /* Just add and store all 32 sums */
      si64 pnsum = jesl(0);
      if (gotex) {                   /* Excitation */
         register int p;
         for (p=0; p<PHASE_RES; p++) {
            swlodm(pafp[p], raxp[p+OPhA]);
            pexSums[p] = jasw(pexSums[p], raxp[p+OPhA]);
            pnsum = jasw(pnsum, raxp[p+OPhA]);
            }
         }
      if (gotin) {                   /* Inhibition */
         register int p;
         for (p=0; p<PHASE_RES; p++) {
            swlodm(pafn[p], raxn[p+OPhA]);
            pinSums[p] = jasw(pinSums[p], raxn[p+OPhA]);
            pnsum = jasw(pnsum, raxn[p+OPhA]);
            }
         }
      CDAT.pfaffs[zbict] = pnsum >> PHASE_EXP;
      } /* End summing inputs with phase */

   else if (ix->phopt & PHASU) {
      /* PHASU, no volt-dep or convolution,
      *  all phase bins are the same, do not save in paf[pn]  */
      si64 pnsum = jesl(0);
      if (gotex) {                   /* Excitation */
         register int p;
         swlodm(pafp[0], raxp[0]);
         for (p=0; p<PHASE_RES; p++)
            pexSums[p] = jasw(pexSums[p], raxp[0]);
         pnsum = jasw(pnsum, raxp[0]);
         }
      if (gotin) {                   /* Inhibition */
         register int p;
         swlodm(pafn[0], raxn[0]);
         for (p=0; p<PHASE_RES; p++)
            pinSums[p] = jasw(pinSums[p], raxn[0]);
         pnsum = jasw(pnsum, raxn[0]);
         }
      CDAT.pfaffs[zbict] = pnsum;
      } /* End case: all phase bins the same */

   else if (ix->phopt & PHASC) {
      /* PHASC, result just goes into one bin */
      si64 pnsum = jesl(0);
      register int p = (int)ix->Cn.phi;
      if (gotex) {                   /* Excitation */
         swlodm(pafp[0], raxp[p+OPhA]);
         pexSums[p] = jasw(pexSums[p], raxp[p+OPhA]);
         pnsum = jasw(pnsum, raxp[p+OPhA]);
         }
      if (gotin) {                   /* Inhibition */
         swlodm(pafn[0], raxn[p+OPhA]);
         pinSums[p] = jasw(pinSums[p], raxn[p+OPhA]);
         pnsum = jasw(pnsum, raxn[p+OPhA]);
         }
      CDAT.pfaffs[zbict] = pnsum;
      } /* End case: all phase bins the same */

SKIP_CONNTYPE_CONTRIBUTION: ;

/* If they exist, save AK (net ax) and RAW (afference).  This may be
*  for SIMDATA, bar plots, or future uses.  Intermediate storage in
*  prd data is necessary to avoid serializing d3go in order to send
*  data to host cell-by-cell for output.
*  Note:  Originally, si64 data were reduced here to floats to avoid
*  losing high order in SIMDATA.  However, this made d3lplt bar plot
*  code very clumsy, so now we just replace overflow values with
*  largest si32--should be rare.
*  N.B. d3allo() guarantees that AK,RAW are stored on appropriate
*  4-byte storage boundaries.  */

   if (xnexists(ix, AK)) {
      si32 *pak = (si32 *)(Lija.pxnd + ix->xnoff[AK]);
      si64 ak64 = CDAT.pfaffs[zbict];
      swlodm(*pak, ak64);
      }
   if (xnexists(ix, RAW)) {
      si32 *praw = (si32 *)(Lija.pxnd + ix->xnoff[RAW]);
      si64 raw64 = CDAT.praffs[zbict];
      swlodm(*praw, raw64);
      }

   }  /* End of loop over connection types */

/***********************************************************************
*                                                                      *
*                 Section 4.  Complete cell processing                 *
*                                                                      *
*  Beginning of what was cellfin routine in original IBM version--     *
*  Now that all inputs have been processed, finish up calculation      *
*     of cell response, leaving net afferent voltage in CDAT.new_affs. *
*     Evaluate si and phi if there is phasing.                         *
*  Terms in new_affs:                                                  *
*     Input from normal and voltage-dependent connection types.        *
*     Squared and shunting inhibition.                                 *
*     Probes.                                                          *
*  Additional transformations to get cell response:                    *
*     Apply pt, nt thresholds.                                         *
*     Refractory period.                                               *
*     Response function (Izhikevich, Brette-Gerstner, spike, tanh,     *
*        step vs knee function, others to be added).                   *
*     Depression (considered as presynaptic, so applied here to s(i)   *
*        as it would affect synapses postsynaptic to this cell).       *
*  Update PSI if refractory amplification is in use                    *
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

      e64dec(ject+CTO_AFF);
      sumaffs(il, CDAT.pTotSums);
      sumaffs(il, CDAT.pCellResp);

/* Handle squared inhibitory types.  The scaling of the squared sums
*  is: S20 + S20 - 20 = S20, but need to divide by an additional S7
*  in COMPAT=C mode to make 128mV = 1.0 multiplier.  Because we do
*  not currently have a routine to multiply, round, shift two 64-bit
*  numbers, we prescale by 9 or 12 to 32-bits, multiply back to 64,
*  then finish scaling and rounding.  Another option would be to go
*  to double-precision floating point here.  */

      if (il->kaff[SGM_Total] & AFF_Square) {
         si32 cresp;
         register int p;
         if (RP->compat & OLDRANGE) for (p=0; p<laffs; p++) {
            swlodm(cresp, jsrsw(CDAT.pSqSums[p], 12));
            CDAT.pCellResp[p] = jrsw(CDAT.pCellResp[p],
               jsrsw(jasl(jmsw(cresp, cresp), 4), 3));
            }
         else for (p=0; p<laffs; p++) {
            swlodm(cresp, jsrsw(CDAT.pSqSums[p], 9));
            CDAT.pCellResp[p] = jrsw(CDAT.pCellResp[p],
               jsrsw(jasl(jmsw(cresp, cresp), 2), 2));
            }
         } /* End squared inhibition */

/* Handle shunting inhibition.  d3oafs requires PSP_NEG to get us here,
*  so subtract rather than add the sums to 1.0 in the denominator
*  (Numerator and denom are S20/27, left shift to keep same scale).
*  R72, 02/03/17, GNR - Per Piech paper, divide by (1 + ShSums)
*  R74, 08/28/17, GNR - Add siha (half afference) parameter
*/

      if (il->kaff[SGM_Total] & AFF_Shunt) {
         register int p;
         for (p=0; p<laffs; p++) {
            si64 den = jrsw(jesl(il->Ct.siha), CDAT.pShSums[p]);
            CDAT.pCellResp[p] = dmrswwqd(CDAT.pCellResp[p],
               den, il->Ct.siha);
            }
         } /* End shunting inhibition */

/* If cells have phase, call d3phase to calculate final activity and
*  phase according to phimeth and simeth controls.  Then save final
*  phase distribution for SIMDATA if requested (this is necessary to
*  avoid serializing d3go to send pCellResp to host for output). Note
*  that phase distribution is stored in host order, not memacc order.
*  If no phase, s(i) is just the result of the above calculations.  */

      if (il->phshft) {
         vmterms = d3phase(il);
         if (il->ctf & CTFPD) {
            si32 tphd[PHASE_RES];
            register int p;
            for (p=0; p<PHASE_RES; p++) {
               swlodm(tphd[p], CDAT.pCellResp[p]);
               }
            memcpy((char *)plsd+il->ctoff[PHD],
               (char *)tphd, sizeof(tphd));
            }
         }
      else
         vmterms = *CDAT.pCellResp;

/* Process probe option - add input to selected cells.
*  N.B.  Currently, probes are added into final cell_affs with no
*     particular phase.  It may be desirable to change this.
*     (Then the probe would be added to the 64-bit pCellResp values
*     and there would be no need for an overflow check here.)  */

      CDAT.probeval = 0;
      for (pprb=il->pctprb; pprb; pprb=pprb->pnctp) {
         if (pprb->lpptr > 0 && ilstnow(&pprb->prbit) == (long)icell) {
            si32 ttpv, tpval;
            /* If there is a user function, call it now */
            if (pprb->pupsprb) tpval =
               (long)((float)S20*pprb->pupsprb((long)il->seqvmo,
               (long)icell, (long)RP->CP.ser, (long)RP->CP.trial,
               (long)CDAT.curr_cyc1, (float)pprb->sprb/(float)S20));
            else
               tpval = pprb->sprbcyc;
            ttpv = CDAT.probeval;
            jasjdm(CDAT.probeval, ttpv, tpval);
            CDAT.events |= PROBED;
            /* Advance to next cell and save cell number for restarting
            *  iterator on next trial.  (This is now done redundantly
            *  in each cycle in case cycle loop terminates early.)  */
            ilstiter(&pprb->prbit);
            --pprb->lpptr;
            pprb->probnxt = ilstnow(&pprb->prbit);
            } /* End if probe active */
         } /* End loop over probes */
      /* Add probe total into vmterms */
      vmterms = jasl(vmterms, CDAT.probeval);

/*---------------------------------------------------------------------*
*  That completes evaluation of cell afferents, which are up to here   *
*  same as vmterms.  Now apply response function to get cell response. *
*  R78, 07/06/18, GNR - Compute newvm with noise+pers for integration  *
*---------------------------------------------------------------------*/

      /*** THIS WOULD BE THE PLACE TO STORE cell_affs IN prd DATA
      **** IF SAVING FOR BAR PLOTS, COMPAT=V, SIMDATA, etc.  ****/
      swlodm(CDAT.new_affs, vmterms);  /* Save for possible dprnt */

/* Perform KNEE or STEP function evaluation of net afference.
*  Check for hits (positive and negative) and set triggers to
*     count them later if statistics being done.
*  (In Darwin II, positive hits were counted before inhib.)
*  Really should check pt,nt against excit,inhib sums separately.
*/

      e64dec(ject+CTO_RFN);
      if (il->ctf & CTFDR) {
         /* STEP response */
         if (CDAT.new_affs >= il->Ct.pt)        hitpt = 1;
         else if (CDAT.new_affs <= il->Ct.nt)   hitnt = 1;
         else vmterms = jesl(0);
         }
      else {
         /* KNEE response */
         if (CDAT.new_affs >= il->Ct.pt)
            vmterms = jrsl(vmterms, il->Ct.pt), hitpt = 1;
         else if (CDAT.new_affs <= il->Ct.nt)
            vmterms = jrsl(vmterms, il->Ct.nt), hitnt = 1;
         else vmterms = jesl(0);
         }

/* V8A, 04/15/96, GNR - This code was heavily revised to give each
*  cell an individual dynamically variable 'st' firing threshold
*  and to couple refractory periods to 'st' levels rather than to
*  depression.  Redundant code was introduced to improve clarity.
*  Depression is now applied based on D(t) rather than on D(t-1).
*  Former "LTP" calculations and "hit" statistics were removed.
*  Old-style ('rt') refractory period was already removed in V7B.  */

/* If in an absolute refractory period, step timer and either set
*  newvm to ahp or let it decay.  It might be possible to save some
*  time by omitting above calculation of cell afferents during abso-
*  lute refractory periods, however, great care would be needed to
*  assure deterministic updating of all random number seeds and
*  pointers in the connection loops.  */

      if (wkdst < DST_CLK0S16) { /* Guaranteed 0 if prf is NULL */
         /* Set event bit and count refractory events */
         CDAT.events |= REFRAC;
         /* Advance noise seed deterministically */
         if (il->No.noikadd & NFL_GENN) udevskip(&il->No.nsd, 2);
         /* Tick clock.  At end of refractory interval,
         *  set to resume normal newvm processing. */
         if ((wkdst += S16) >= DST_CLK0S16) wkdst = prf->psdst;
         /* If refractory period is not abrupt, decay down
         *  from previous activity towards baseline + AHP.
         *  This is not handled by d3decay(), as rate is not
         *  omega2 and limiting/saturating does not apply. */
         response = newvm = prf->upsahp ? (oldvmsi_dcy -
            mrsrsld(prf->upsahp, oldvmsi_dcy-prf->ahp, 28)) : prf->ahp;
         } /* End handling absolute refractory period */

/* Not in an absolute refractory period.  Calculate noise and add
*     to vmterms if NFL_ADDV flag is set.
*  Note:  NMOD can force nmodval < 0, which is OK, gives 0 noise.
*  Note:  Before V8A, noise could not initiate a spike, then until
*    R78 it was added into term that was checked for a spike.  Since
*    R78, it is only added if NFL_ADDV flag is set.  */

      else {
         if (il->No.noikadd & NFL_GENN) {
            CDAT.noise = d3noise(
               il->No.nmn, il->No.nsg, CDAT.nmodval, il->No.nsd);
            if (il->No.noikadd & NFL_ADDV)
               vmterms = jasl(vmterms, CDAT.noise);
            }

/* Switch according to response function */

         switch ((enum RespFunc)il->rspmethu) {

/* IZHIKEVICH function--evaluate response as described by MATLAB
*  code in "Simple Model of Spiking Neurons" by E.M. Izhikevich,
*  IEEE Trans. Neural Networks, 14:1569-1572 (2003) (RF_IZHI3)
*  and in his book "Dynamical Systems in Neuroscience, MIT Press
*  (2007) (RF_IZHI7), p. 274.  The methods of dealing with numerical
*  instability in "Hybrid Spiking Models" by E.M.I., Phil Trans. Roy.
*  Soc. A368:5061-5070 (2010) have not been implemented, but should
*  be looked at again.
*
*  I assume all afferences and noise are voltages, not currents,
*  and so do not divide by Cm.
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
*  enough precision to replicate E.I.'s examples.  As of R76, s(i) is
*  now always stored in 32-bit VMP variable.  For RF_BREG and RF_IZHx
*  response functions, dynamic variable IZU is allocated, where u(i)
*  or w(i) can be stored as si32 values.  This is forced on a word
*  boundary by d3allo.  The value of s(i) is of course still trun-
*  cated to 16 bits for transmission to other cells.
*
*  As of R78, calculation of (v - vr)^3 was rewritten to improve
*  precision and this made the divergence in the MV and FS tests
*  occur a little later.  The I tried adding more rounding to the
*  calculations of a(bv-u) but this gave same curves or worse
*  (VS cell) so this code was removed because a little slower.
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
            d3gtjl4a(CDAT.izhu, pizu);    /* S22/29 */
            e64dec(pi7->Z.iovec);
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
                        tt = mrssl(tt, abs32(tt), FBrf);
                        tt = mrssl(tt, (si32)pi7->Z.izrc, FBrf);
                        /* S31 + 7/14 - 31 + (20-7) ==> S20/27 */
                        c += tt << (FBwk-FBsi); }
                     if (pi7->Z.izrd) {   /* Use individual cell d */
                        tt = udev(&lzseed) << 1;
                        tt = mrssl(tt, abs32(tt), FBrf);
                        tt = mrssl(tt, (si32)pi7->Z.izrd, FBrf);
                        /* S31 + 7/14 - 31 + (22-7) ==> S22/29 */
                        d += tt << (FBIu-FBsi); }
                     }
                  else {                  /* Variable c,d in mem */
                     rd_type *prr = plsd + il->ctoff[IZRA] +
                        il->nizvab * HSIZE;
                     if (pi7->Z.izrc) {   /* Use individual cell c */
                        d3gtjh2a(tt, prr); prr += HSIZE;
                        /* S7/14 + 16 (d3gtjh2a) - 3 ==> S20/27 */
                        c += SRA(tt,FBrs-FBwk); }
                     if (pi7->Z.izrd) {   /* Use individual cell d */
                        d3gtjh2a(tt, prr);
                        /* S7/14 + 16 (d3gtjh2a) - 1 ==> S22/29 */
                        d += SRA(tt, 1); }
                     }
                  }
               if (pi7->umc) c +=
                  mrsrsld(pi7->uumctCm, CDAT.izhu, FBIu+24-FBwk);
               response = CDAT.izhc = c;
               CDAT.izhu += CDAT.izhd = d;
               if (CDAT.izhu > pi7->uumaxoCm)
                  CDAT.izhu = pi7->uumaxoCm;
               CDAT.izhu &= ~IZ_SPIKED;
               } /* End spike termination action */
            else {                     /* Not a spike last time */
               si64 t64 = jmsw(oldvm, oldvm - pi7->izvt);
               si32 a,b,tt,wuv3,simvur = oldvm - pi7->Z.vur;
               /* If calc Vm*(Vm-vt) as only 32 bits, must shift away
               *  an extra 5 to avoid overflow on max pos si, then lose
               *  most precision if si is very small, so calc 64.
               *  Here S20/27 + 20/27 + 30/23 - 50 ==> S20/27 */
               t64 = mrsrswd(t64, pi7->ukoCm, 50);
               t64 = jasw(t64, vmterms);
               t64 = jrsl(t64, SRA(CDAT.izhu, FBIu-FBwk));
               /* Integrate */
               if (RP->CP.runflags & RP_DOINT)
                  t64 = mrsrswdm(t64, RP->eudt, FBod);
               t64 = jasl(t64, oldvm);
               swlodm(response, t64);
               a = pi7->Z.iza;
               /* Note:  Even if using bvlo, still must update
               *  lzseed if b is variable and in OPTMEM mode,
               *  so do not jump around update code below  */
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
                        tt = mrssl(tt, (si32)pi7->Z.izra, FBrf);
                        /* S31 + 14 - 31 + 14 ==> S28 */
                        a += tt << 14; }
                     if (pi7->Z.izrb) {   /* Use individual cell b */
                        tt = udev(&lzseed) << 1;
                        if (simvur >= pi7->usmvtest) {
                           tt = mrssl(tt, (si32)pi7->Z.izrb, FBrf);
                           /* S31 + 14 - 31 + 14 ==> S28 */
                           b += tt << 14; }
                        }
                     }
                  else {
                     rd_type *prr = plsd + il->ctoff[IZRA];
                     if (pi7->Z.izra) {   /* Use individual cell a */
                        d3gtjh2a(tt, prr); prr += HSIZE;
                        /* S14 + 16 (d3gtjh2a) - 2 ==> S28 */
                        a += SRA(tt,2); }
                     if (simvur >= pi7->usmvtest) {
                        /* Use individual cell b */
                        d3gtjh2a(tt, prr);
                        /* S14 + 16 (d3gtjh2a) - 2 ==> S28 */
                        b += SRA(tt,2); }
                     }
                  }
               if (il->ctf & CTFRANOS)
                  udevskip(&lzseed, (si32)il->nizvcd);
               CDAT.izha = a;
               CDAT.izhb = b;
               /* E.Izhi. uses oldvm here, not new response */
               tt = mrssld(b, simvur, FBIab+FBwk-FBIu) - CDAT.izhu;
               /* R78, 07/17/18, GNR - simvur is S20/27 so square is
               *  S40/54 in 64 bits with max 22/2 int bits so fits.
               *  Cube is S60/81 in 94 bits w/max 33/6 int bits so
               *  fits.  Right shift by tsqis = 36/50 gives S24/31 so
               *  33/6 int bits fits in t64.  Then mult by wuv3 (S36)
               *  (has to fit 94 bits) followed by right shift 20+36+4
               *  (for tsquis extras)-22 = 38 gives S22/29 to match
               *  normal izhu.  Also added final rounding.  */
               if (wuv3) {             /* Need v-cubed term */
                  si64 t64 = jmsw(simvur, simvur);
                  t64 = mrsswd(t64, simvur, tsqis);
                  t64 = mrsrswd(t64, wuv3, FBwk+40-FBIu);
                  t64 = jasl(t64, tt);
                  t64 = mrsrswdm(t64, a, FBIab);
                  swloem(tt, t64, pi7->Z.iovec);
                  CDAT.izhu += tt;
                  }
               else
                  CDAT.izhu += mrsrsld(tt, a, FBIab);
               CDAT.izhu &= ~IZ_SPIKED;
               /* Now cap the response as commented above */
               tt = pi7->Z.vpeak;
               if (pi7->uumvptCm) tt +=
                  mrsrsld(pi7->uumvptCm, CDAT.izhu, FBIu+24-FBwk);
               if (response > tt) {
                  CDAT.izhu |= IZ_SPIKED;
                  response = tt;
                  }
               }
            newvm = response;
            d3ptjl4a(CDAT.izhu, pizu);
            break;   /* Skip old-style decay */
            } /* End RF_IZHI7X local scope */

case RF_IZH3X:  {          /* 2003 version with variable a,b,c,d */
            struct IZ03DEF *pi3 = (struct IZ03DEF *)il->prfi;
            si32 aseed,bseed,b,tt,wkvm;
            d3gtjl4a(CDAT.izhu, pizu);    /* S22/29 */
            e64dec(pi3->Z.iovec);

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
                  tt = mrssl(bseed<<1, (si32)pi3->Z.izrb, FBrf);
                  /* S31 + 14 - 31 + 14 ==> S28 */
                  b += tt << 14; }
               else {                        /* Indiv b in memory */
                  int qzra = pi3->Z.izra != 0;
                  rd_type *prb = plsd + il->ctoff[IZRA] + HSIZE*qzra;
                  d3gtjh2a(tt, prb);
                  /* S14 + 16 (d3gtjh2a) - 2 = S28 */
                  b += SRA(tt,2); }
               } /* End getting individual b */

            /* Adjust CNS Vm(i)(t-1) for 2003 model Vrest scale */
            if (il->ctf & CTFI3IVC)
               CDAT.i3vr = d3i3vr(pi3, b);
            else if (il->ctf & CTFI3IVR) {
               rd_type *pivr = plsd + il->ctoff[IZVr];
               d3gtjl4a(CDAT.i3vr, pivr); }
            else
               CDAT.i3vr = pi3->i3vr;
            wkvm = oldvm + CDAT.i3vr;

            /* Perform reset if a spike was stored last time */
            if (wkvm >= pi3->Z.vpeak) {
               si32 c = pi3->Z.izc, d = pi3->Z.izd;
               CDAT.events |= SPIKED;
               if (il->nizvcd) {
                  if (il->Ct.ctopt & OPTMEM) {
                     /* N.B.  Must discard bits to bring precision to
                     *  same number of bits saved when not OPTMEM.  */
                     if (pi3->Z.izrc) {   /* Use individual cell c */
                        tt = udev(&lzseed) << 1;
                        tt = mrssl(tt, abs32(tt), FBrf);
                        tt = mrssl(tt, (si32)pi3->Z.izrc, FBrf);
                        /* S31 + 7/14 - 31 + (20-7) ==> S20/27 */
                        c += tt << (FBwk-FBsi); }
                     if (pi3->Z.izrd) {   /* Use individual cell d */
                        tt = udev(&lzseed) << 1;
                        tt = mrssl(tt, abs32(tt), FBrf);
                        tt = mrssl(tt, (si32)pi3->Z.izrd, FBrf);
                        /* S31 + 7/14 - 31 + (22-7) ==> S22/29 */
                        d += tt << (FBIu-FBsi); }
                     }
                  else {                  /* Variable c,d in mem */
                     rd_type *prr = plsd + il->ctoff[IZRA] +
                        il->nizvab * HSIZE;
                     if (pi3->Z.izrc) {   /* Use individual cell c */
                        d3gtjh2a(tt, prr); prr += HSIZE;
                        /* S7/14 + 16 (d3gtjh2a) - 3 ==> S20/27 */
                        c += SRA(tt,FBrs-FBwk); }
                     if (pi3->Z.izrd) {   /* Use individual cell d */
                        d3gtjh2a(tt, prr);
                        /* S7/14 + 16 (d3gtjh2a) - 1 ==> S22/29 */
                        d += SRA(tt, 1); }
                     }
                  }
               response = CDAT.izhc = c;
               CDAT.izhu += CDAT.izhd = d;
               } /* End spike termination action */
            else {                     /* Not a spike last time */
               /* R78, 07/20/18 - Recast to minimize overflow cks */
               si64 t64;
               si32 a = pi3->Z.iza;
               /* S20/27+S30/23-26 = S24 */
               t64 = jsrsw(jmsw(wkvm, pi3->izcv2), 30+FBwk-FBsc);
               t64 = jasl(t64, pi3->izcv1);
               t64 = mrsrswd(t64, wkvm, FBsc);
               t64 = jasl(t64, pi3->izcv0);
               t64 = jasw(t64, vmterms);
               t64 = jrsl(t64, SRA(CDAT.izhu, FBIu-FBwk));
               /* Integrate */
               if (RP->CP.runflags & RP_DOINT)
                  t64 = mrsrswdm(t64, RP->eudt, FBod);
               t64 = jasl(t64, wkvm);
               swlodm(response, t64);
               if (pi3->Z.izra) {
                  if (il->Ct.ctopt & OPTMEM) {
                     /* N.B.  Must discard bits to bring precision to
                     *  same number of bits saved when not OPTMEM.  */
                     tt = mrssl(aseed<<1, (si32)pi3->Z.izra, FBrf);
                     /* S31 + 14 - 31 + 14 ==> S28 */
                     a += tt << 14; }
                  else {
                     rd_type *prr = plsd + il->ctoff[IZRA];
                     d3gtjh2a(tt, prr); prr += HSIZE;
                     /* S14 + 16 (d3gtjh2a) - 2 ==> S28 */
                     a += SRA(tt,2); }
                     }
               if (il->ctf & CTFRANOS)
                  udevskip(&lzseed, (si32)il->nizvcd);
               CDAT.izha = a;             /* For detail print */
               CDAT.izhb = b;
               /* E.Izhi. uses newsi here in 2003, oldvm in 2007 */
               tt = mrssld(b, response-pi3->Z.vur, FBIab+FBwk-FBIu) -
                  CDAT.izhu;
               CDAT.izhu += mrsrsld(tt, a, FBIab);
               /* Now cap the response as commented above */
               if (response > pi3->Z.vpeak) response = pi3->Z.vpeak;
               }
            newvm = response -= CDAT.i3vr;   /* Back to CNS scale */
            d3ptjl4a(CDAT.izhu, pizu);
            break;   /* Skip old-style decay */
            } /* End RF_IZH3X local scope */

case RF_IZH7:  {           /* Faster version, no extras (except vur) */
            struct IZ07DEF *pi7 = (struct IZ07DEF *)il->prfi;
            d3gtjl4a(CDAT.izhu, pizu);    /* S22/29 */
            e64dec(pi7->Z.iovec);
            /* Perform reset if a spike was stored last time.
            *  R78, 07/09/18, GNR - Use IZ_SPIKED flag in case
            *     response was decreased on wasy to being oldvm.  */
            if (CDAT.izhu & IZ_SPIKED) {
               CDAT.events |= SPIKED;
               response = pi7->Z.izc;
               CDAT.izhu = (CDAT.izhu + pi7->Z.izd) & ~IZ_SPIKED;
               } /* End spike termination action */
            else {                        /* Not a spike last time */
               si64 t64 = jmsw(oldvm, oldvm - pi7->izvt);
               si32 tt,simvur = oldvm - pi7->Z.vur;
               /* S20/27 + 20/27 + 30/23 - 50 ==> S20/27 */
               t64 = mrsrswd(t64, pi7->ukoCm, 50);
               t64 = jasw(t64, vmterms);
               t64 = jrsl(t64, SRA(CDAT.izhu, FBIu-FBwk));
               /* Integrate */
               if (RP->CP.runflags & RP_DOINT)
                  t64 = mrsrswdm(t64, RP->eudt, FBod);
               t64 = jasl(t64, oldvm);
               swlodm(response, t64);
               /* E.Izhi. uses oldvm here, not new response */
               tt = mrssld(pi7->Z.izb, simvur, FBIab+FBwk-FBIu) -
                  CDAT.izhu;
               CDAT.izhu = (CDAT.izhu +
                  mrsrsld(tt, pi7->Z.iza, FBIab)) & ~IZ_SPIKED;
               /* Now cap the response as commented above */
               if (response > pi7->Z.vpeak) {
                  CDAT.izhu |= IZ_SPIKED;
                  response = pi7->Z.vpeak;
                  }
               }
            newvm = response;
            d3ptjl4a(CDAT.izhu, pizu);
            break;   /* Skip old-style decay */
            } /* End RF_IZH7 local scope */

case RF_IZH3:  {           /* 2003 version of the model */
            struct IZ03DEF *pi3 = (struct IZ03DEF *)il->prfi;
            si32 tt,wkvm;
            d3gtjl4a(CDAT.izhu, pizu);    /* S22/29 */
            e64dec(pi3->Z.iovec);
            /* Adjust CNS Vm(i) for 2003 model Vrest scale, which,
            *  in RF_IZH3 case, is constant in pi3->i3vr.  */
            wkvm = oldvm + pi3->i3vr;
            /* Perform reset if a spike was stored last time */
            if (wkvm >= pi3->Z.vpeak) {
               CDAT.events |= SPIKED;
               response = pi3->Z.izc;
               CDAT.izhu += pi3->Z.izd;
               } /* End spike termination action */
            else {
               /* S20/27+S30/23-26 = S24 */
               si64 t64 = jsrsw(jmsw(wkvm, pi3->izcv2), 30+FBwk-FBsc);
               t64 = jasl(t64, pi3->izcv1);
               t64 = mrsrswd(t64, wkvm, FBsc);
               t64 = jasl(t64, pi3->izcv0);
               t64 = jasw(t64, vmterms);
               t64 = jrsl(t64, SRA(CDAT.izhu, FBIu-FBwk));
               /* Integrate */
               if (RP->CP.runflags & RP_DOINT)
                  t64 = mrsrswdm(t64, RP->eudt, FBod);
               t64 = jasl(t64, wkvm);
               swlodm(response, t64);
               /* E.I. uses newsi here in 2003, oldvm in 2007 */
               tt = mrssld(pi3->Z.izb, response - pi3->Z.vur,
                  FBIab+FBwk-FBIu) - CDAT.izhu;
               CDAT.izhu += mrsrsld(tt, pi3->Z.iza, FBIab);
               /* Now cap the response as commented above */
               if (response > pi3->Z.vpeak) response = pi3->Z.vpeak;
               }
            newvm = response -= pi3->i3vr;   /* Back to CNS scale */
            d3ptjl4a(CDAT.izhu, pizu);
            break;   /* Skip old-style decay */
            } /* End RF_IZH3 local scope */

/* Brette-Gerstner (aEIF) function -- taken from R. Brette and W.
*  Gerstner, J. Neurophysiol. 94:3637-3642 (2005).  The Izhikevich
*  IZU variable stores 'w' for this function.  The exponential is
*  approximated by exp(y) ~ 2^k * (1 + f - c) where k = int(y/ln2)
*  and f = frac(y/ln2) as suggested by N. N. Schraudolph */

case RF_BREG:  {
            struct BREGDEF *pbg = (struct BREGDEF *)il->prfi;
            d3gtjl4a(CDAT.izhu, pizu);
            e64dec(pbg->iovec);
            /* Determine whether a spike was stored last time */
            if (oldvm >= pbg->vPeak) {
               CDAT.events |= SPIKED;
               response = pbg->vReset;
               CDAT.izhu += pbg->ubgb;
               } /* End spike termination action */
            else {
               /* Calculate new response = s(i) and cap it.  If the
               *  exponential term gets big enough to overflow,
               *  declare a spike.  */
               si64 t64;
               si32 tt = oldvm - pbg->vT;
               if (tt > pbg->uspthr) response = pbg->vPeak;
               else {
#if 0    /* Test code--use library exponential */
                  float x = expf((float)tt/(float)pbg->delT);
                  x *= (float)pbg->ugLdToCm;
                  tt = (si32)x;
#else
                  si32 y,k;
                  /* S20 + 28 - 24 = S24 */
                  y = mrsrsld(tt, pbg->u1oln2dT, 24);
                  k = SRA(y,24);    /* Exponent of 2 */
                  /* Add 1 (S24) - (c = 45799 (S20)) to the fraction,
                  *  then multiply by gL*delT/Cm before scaling.  The
                  *  scaling shift and k shift can then be combined,
                  *  assuring that the shift is always to the right
                  *  (if uspthr test passed).  mrsrsle does not detect
                  *  shift > 64, so test for it here.  */
                  if (k <= -40) tt = 0;
                  else {
                     y = (y & (S24-1)) + 16044432;
                     tt = mrsrsld(y, pbg->ugLdToCm, (24-k));
                     }
#endif
                  t64 = vmterms;
                  t64 = jasl(t64, tt);
                  t64 = jrsl(t64, CDAT.izhu);
                  t64 = jrsw(t64, mrsrswj(oldvm, pbg->ugLoCm, FBIab));
                  /* Integrate */
                  if (RP->CP.runflags & RP_DOINT)
                     t64 = mrsrswdm(t64, RP->eudt, FBod);
                  t64 = jasl(t64, oldvm);
                  swlodm(response, t64);
                  if (response > pbg->vPeak) response = pbg->vPeak;
                  }
               /* Calculate new w */
               tt = mrssld(oldvm, pbg->ubga, FBIab) - CDAT.izhu;
               CDAT.izhu += mrsrsld(tt, pbg->u1otW, FBIab);
               }
            newvm = response;
            d3ptjl4a(CDAT.izhu, pizu);
            break;   /* Skip old-style decay */
            } /* End RF_BREG local scope */

/* SPIKE function--fire a spike if afference plus noise is above
*  firing threshold.
*  Rev, 10/06/13, GNR - Perform decay even if there is no spike.
*/
case RF_SPIKE: {
            if (RP->CP.runflags & RP_DOINT)
               vmterms = mrsrswdm(vmterms, RP->eudt, FBod);
            vmterms = jasl(vmterms, CDAT.pers);
            swlodm(newvm, vmterms);
            if (newvm >= wkst) {
               response = il->Ct.sspike;
               CDAT.events |= SPIKED;  /* Not currently used */
               } /* End spike threshold exceeded */
            /* Input does not exceed firing threshold */
            else
               response = newvm;
            } /* End RF_SPIKE local scope */
            break;

/* Hyperbolic tangent function.  Note that separate scales for map-
*  ping +/- afferences to tanh args are not provided--user can use
*  connection-type and noise scales for this purpose. Also Note that
*  because a transcendental function is involved, results are not
*  guaranteed to be identical on all machine architectures.
*  R78, 07/10/18, GNR - Bug fix:  Include persistence in tanh arg.  */

case RF_TANH:
            if (RP->CP.runflags & RP_DOINT)
               vmterms = mrsrswdm(vmterms, RP->eudt, FBod);
            vmterms = jasl(vmterms, CDAT.pers);
            swlodm(newvm, vmterms);
            response = (si32)(il->ctwk.go.fstanh *
               tanhf((float)newvm*il->ctwk.go.fvmscl));
            break;

/* Traditional piecewise-linear response */

case RF_STEP:
case RF_KNEE:
            if (RP->CP.runflags & RP_DOINT)
               vmterms = mrsrswdm(vmterms, RP->eudt, FBod);
            vmterms = jasl(vmterms, CDAT.pers);
            swlodm(newvm, vmterms);
            response = newvm;
            if (il->Ct.sslope != S24)
               response = mrsrsld(response, il->Ct.sslope, FBsc);
            break;
            } /* End rspmeth switch */
         } /* End not refractory */

      CDAT.new_vm = newvm;
      d3ptjl4(newvm, pvmp);   /* Save as oldvm for next time */

/* Add noise to response if it was not already in vmterms */

      if (il->No.noikadd & NFL_ADDR) {
         si32 tt;
         jasjdm(tt, response, CDAT.noise);
         response = tt;
         }

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

      e64dec(ject+CTO_RFN);   /* May have been changed by IZHI/BREG */
      if (il->ctf & CTFDN) {
         rd_type *pdpr = plsd + il->ctoff[DEPR];
         si32 depression,tsimrt;
         /* Fetch depression from data structure */
         d3gtjl1(depression, pdpr);
         /* Update depression with random rounding and store
         *  in CDAT for stats before capping.
         *  Note:  Old NSF proposal used (oldsi-rt) in place
         *  of oldsi > 0 here.  This formula was removed:
         *  it was incorrect in that it failed to test for
         *  negative s(i)-rt or use SRA.  */
         tsimrt = (oldsi > 0) ? oldsi : 0;
         CDAT.depr = depression = (
            mrssl(depression, il->Dp.omegad, FBod-Ss2hi) +
            mrssld(tsimrt, il->Dp.upsd, 23) + (rnd3>>15)) >> Ss2hi;
         /* Don't let stored value exceed 255--but leave it in
         *  CDAT.depr for printing so user can see the problem */
         if (depression > 255) depression = 255;
         /* Depress cell response, s(depr) = s(calc)*phi(D).
         *  S20+S8=S28 and scale back to S20. */
         if (depression) response = mrsrsl(response,
            (si32)(ui32)ctab[depression], 8);
         /* Store new value back into data structure */
         d3ptjl1(depression, pdpr);
         }  /* End of depression */

/* Code for updating refractory period threshold.  Test for refrac-
*  tory period is now based on newvm, which allows Izhi/aEIF neurons
*  to have a refractory period.
*  If dst exists and newvm > wkst, update, round, scale (first to S20
*  so large values can be capped, then to S23), treating 128 mV as 1.0
*  old-style, then store hi result, giving effective S7 in memory.  if
*  an absolute refractory period exists, set to enter one on next cycle,
*  otherwise replace dst with psdst ("post-spike dst") on next cycle.
*  The previous wkst, which is the one used to evaluate new_si, is
*  left in CDAT.dst for dprnt.  rnd2 is shifted to amount to (1/2)
*  when wkdst is shifted and high order stored.
*
*  Rev, 05/05/08, GNR - Restore checking for refractory period for
*     all response functions, not just RF_SPIKE.  For a time in V8D,
*     this was not done, but the doc specifies it, and it is easily
*     turned off if not wanted.
*  Rev, 12/31/08, GNR - Values large enough to set the sign bit are
*     now trapped as overflows--old code would have incorrectly stored
*     them as timers.  */

      if (prf) {
         if (newvm >= wkst)
            wkdst = prf->refrac ? refrcs16 : prf->psdst;
         else {
            wkdst = mrssld(wkdst, prf->omegadst, FBod) +
               ((response > 0) ? mrssld(response, prf->upsdst,
                  Ss2hi) : 0) + (rnd2>>18);
            if (wkdst >= S28) wkdst = (S28-1);
            }
         if (wkdst > DST_CLK0S16) wkdst <<= 3;  /* Not a timer */
         d3ptjh2a(wkdst, plsd+il->ctoff[DST]);
         } /* End dst update */

/* If doing either or both kinds of autoscaling, record the total
*  afference in a list that will be used later to find the inputs
*  leading to responses in the specified astf1-astfl range.
*  Work space is allocated for this at CDAT.passl0 by d3nset.  */

      if (il->kautu & (KAUT_HTN|KAUT_HPN)) d3assv(il, CDAT.new_affs);

/* Round, scale to S7, and store final s(i) value.  If necessary,
*  replace response by max response that will fit in two bytes or
*  by 1.0 (S14) if COMPAT=C and not KAUT_APP as set in rspmax/min.
*  If using old s(i) range, negative s(i) is not allowed, but
*  we now allow s(i) == 1 (S14), previously not representable.
*  Note:  With rspmax/min mechanism, underflows on mV scale are
*  no longer counted, consistent with COMPAT=C behavior.  */

      if (response > CDAT.rspmax) {
         response = CDAT.rspmax, ntrunchi = 1;
         }
      else if (response < CDAT.rspmin) {
         response = CDAT.rspmin, ntrunclo = 1;
         }

      CDAT.new_si = response;    /* Save S20 response */

      {  si32 respS7 =
            (response + (1 << (FBwk-FBsi-1))) >> (FBwk-FBsi);
         d3ptjl2(respS7, psnew);

/* Calculate and store updated mean response (qbar and sbar).  In V7B,
*  this code was changed to perform random-rounding of the new sbar
*  with il->rsd.  This avoids stationary sbar values that arise with
*  fixed rounding, e.g.  with no input, integer sbar can never decay
*  below 1/(2*sdamp).  Changed again at V8D to use simpler signed
*  rounding algorithm.  There is no check for overflow because sdamp
*  is checked at input time and respS7 is limited to two bytes in code
*  just above.  In V8G, these updates are deferred in KAUT_APP mode
*  to the rescaling in d3autsc().  */

         if (!(il->kautu & KAUT_APP)) {
            if (ctexists(il,SBAR)) {
               si32 sbar1;
               if (il->ctf & CTFDOFS)
                  sbar1 = respS7;
               else {
                  sbar1 = (si32)il->Dc.sdamp*(respS7 - CDAT.sbar) +
                     (rnd1 >> Ss2hi);
                  sbar1 = SRA(sbar1, FBdf) + CDAT.sbar;
                  }
               d3ptjl2(sbar1, plsd+il->ctoff[SBAR]);
               }
            if (ctexists(il,QBAR)) {
               si32 qbar1 = respS7;
               if (il->orkam & KAMBCM)
                  qbar1 *= qbar1, qbar1 = SRA(qbar1,(FBsi+FBsi));
               if (!(il->ctf & CTFDOFS)) {
                  qbar1 = (si32)il->Dc.qdamp*(qbar1 - CDAT.qbar) +
                     (rnd1 & (S15-1));
                  qbar1 = SRA(qbar1, FBdf) + CDAT.qbar;
                  }
               d3ptjl2(qbar1, plsd+il->ctoff[QBAR]);
               }
            } /* End if KSQUP_GO */

/* Update and store amplification refractoriness ("psi").
*  PSI (S15) = (psiacc (S30) * Si (S27) * upspsi (S22) >> 64) -
*     (oldPSI * zetapsi (S15) >> 15) with 0 <= PSI <= 1S15.
*  N.B.  If KAMLP is set, PSI variable is guaranteed to exist,
*  but KAMLP may be shut off in late series, so existence of PSI
*  does not mean it needs to be computed.
*  N.B.  If we move amplification after response calc, this calc
*  should then use new response rather than old_si.  */

         if (il->orkam & KAMLP) {
            /* First compute decay of psi(t-1) */
            si32 tups, tpsi = S15 - CDAT.ompsi;
            tpsi -= (tpsi * (si32)il->Dc.zetapsi) >> FBdf;
            /* Add the growth term */
            e64dec(ject+CTO_PSI);
            psiacc = mrsswd(psiacc, oldvmsi_amp, 42);
            swlodm(tups, psiacc);
            tpsi += mrssld(tups, il->Dc.upspsi, 22);
            if (tpsi < 0) tpsi = 0;
            else if (tpsi > S15) tpsi = S15;
            d3ptjl2(tpsi, plsd+il->ctoff[PSI]);
            } /* End PSI update */

         } /* End scope of respS7 */

      if (il->phshft) psnew[LSmem] = CDAT.new_phi;
      hitht = response >= il->Ct.ht;

/*---------------------------------------------------------------------*
*    Accumulate response statistics if this is a statistical cycle     *
*     (These are statistics that need to be done for every cell.)      *
*            (statenbc takes into account ALLCYC switch.)              *
*---------------------------------------------------------------------*/

      if (statenbc) {
         int  irb;                  /* Response bin for statistics */
         ddist *pdd = il->pctddst;  /* Ptr to double stats */

         /* Accumulate hit and truncation sums.  (This method
         *  eliminates multiple 'if (statenbc)'s in code above.) */
         nhitspt += hitpt, nhitsht += hitht;
         il->CTST->hitneg += hitnt;
         il->CTST->sover  += ntrunchi;
         il->CTST->sunder += ntrunclo;
         if (CDAT.events & REFRAC) il->CTST->nrfrc++;
         if (CDAT.events & SPIKED) il->CTST->nspike++;

         if (CDAT.new_affs >= 0) {
            /* Positive afference--accumulate sum and count */
            il->CTST->sumpos = jasl(il->CTST->sumpos, CDAT.new_affs);
            il->CTST->numpos++;
            /* Record highest score */
            if (CDAT.new_affs > il->CTST->hiscor)
               il->CTST->hiscor = CDAT.new_affs;
            }
         else {
            /* Negative - accumulate sum and count */
            il->CTST->sumneg = jasl(il->CTST->sumneg, CDAT.new_affs);
            il->CTST->numneg++;
            /* Record lowest score */
            if (CDAT.new_affs < il->CTST->loscor)
               il->CTST->loscor = CDAT.new_affs;
            }

         /* Accumulate average response and hit data for
         *  sharpness and cross-response statistics.  */
         il->CTST->sumrsp = jasl(il->CTST->sumrsp, response);
         if (hitht) {
            il->CTST->sharp = jasl(il->CTST->sharp, response);
            il->CTST->nsharp++;
            if (il->ctwk.go.marking) bytior(plsd + il->ctoff[XRM],
               (long)il->lmm, pmark);
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
            irb = (LDSTAT/2) - bitsz((abs32(response)>>(FBwk-1))+1);
            if (irb < 0) irb = 0;
            }

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
         if (il->Ct.kstat & KSTCL) {
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

/* The hdist, ldist, KSTHR, KSTMG, KSTGP stats are only collected after
*  the last cycle of a trial.  */

         if (!cyclestogo) {

            /* Record response history data by modality */
            if (il->Ct.kstat & KSTHR) {
               hist *phist = il->phrs;
               int imnum,himnum = 1 << il->nmdlts;
               int khist = (1 - hitht) +
                  ((oldsi < il->Ct.ht) << 1);
               for (imnum=1; imnum<himnum; imnum<<=1,++phist)
                  ++phist->h[khist +
                     (((il->ctwk.go.delmk & imnum) != 0) << 2)];
               }

            /* Accumulate high and low responses */
            if (response > lhisi) {
               lhisi = response;
#ifdef DBGHLSD
   if (il->seqvmo == 1) {  char wresp[10],hiresp[10];
      if (RP->compat & OLDRANGE) {
         wbcdwtPn(&response, wresp, (27<<RK_BS)+(5<<RK_DS)+RK_IORF+RK_NI32+7);
         wbcdwtPn(&lhisi, hiresp, (27<<RK_BS)+(5<<RK_DS)+RK_IORF+RK_NI32+7); }
      else {
         wbcdwtPn(&response, wresp, (20<<RK_BS)+(4<<RK_DS)+RK_IORF+RK_NI32+7);
         wbcdwtPn(&lhisi, hiresp, (20<<RK_BS)+(4<<RK_DS)+RK_IORF+RK_NI32+7); }
      dbgprt(ssprintf(NULL,"Tr %4d, CT %4s, cell %5jed, si %8s, newhisi %8s",
         RP->CP.trial, il->lname, CDAT.cdcell, wresp, hiresp));
      }
#endif
               }
            if (response < llosi) {
               llosi = response;
#ifdef DBGHLSD
   if (il->seqvmo == 1) {  char wresp[10],loresp[10];
      if (RP->compat & OLDRANGE) {
         wbcdwtPn(&response, wresp, (27<<RK_BS)+(5<<RK_DS)+RK_IORF+RK_NI32+7);
         wbcdwtPn(&llosi, loresp, (27<<RK_BS)+(5<<RK_DS)+RK_IORF+RK_NI32+7); }
      else {
         wbcdwtPn(&response, wresp, (20<<RK_BS)+(4<<RK_DS)+RK_IORF+RK_NI32+7);
         wbcdwtPn(&llosi, loresp, (20<<RK_BS)+(4<<RK_DS)+RK_IORF+RK_NI32+7); }
      dbgprt(ssprintf(NULL,"Tr %4d, CT %4s, cell %5jed, si %8s, newlosi %8s",
         RP->CP.trial, il->lname, CDAT.cdcell, wresp, loresp));
      }
#endif
               }
#ifndef PAR
            /* In the serial case, develop bin number for hdist,ldist.
            *  (Done in d3gsta if PAR after hi,lo over nodes is known.)  */
            if (irb > ihisi) {
               ihisi = irb;
#ifdef DBGHLSD
   if (il->seqvmo == 1)
      dbgprt(ssprintf(NULL,"Tr %4d, CT %4s, cell %5jed, newhisibin %4d",
         RP->CP.trial, il->lname, CDAT.cdcell, irb));
#endif
               }
            if (irb < ilosi) {
               ilosi = irb;
#ifdef DBGHLSD
   if (il->seqvmo == 1)
      dbgprt(ssprintf(NULL,"Tr %4d, CT %4s, cell %5jed, newlosibin %4d",
         RP->CP.trial, il->lname, CDAT.cdcell, irb));
#endif
               }
#endif   /* !PAR */

            }  /* End !cyclestogo */

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

         /* Accumulate distribution of Breg w = Izhi u variable if
         *  IZHI or BREG respmeths, otherwise dist of Vm.  */
         {  si32 tvar = (il->Ct.rspmeth >= RF_BREG) ?
               SRA(CDAT.izhu, FBIu-FBwk) : CDAT.new_vm;
            pdd->d[irb] = jasl(pdd->d[irb], tvar);
            ++pdd;
            }

         /* Accumulate distribution of persistence term.
         *  Rev, 08/25/14, GNR - Test on new decay_bit so zero pers
         *  is included in the histogram after reset cycle.  */
         if (il->Dc.omegau) {
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
         *  Use psdst when dst < DST_CLK0S16 (acting as a clock).
         *  N.B.  Prior to V8C, this statistic was based on the
         *  newly calculated value of DST (wkdst). It is better
         *  to use the old dst (CDAT.dst), because that is the
         *  value actually used to calculate cell response.  */
         if (prf) {
            si32 tdst = CDAT.dst;
            if (tdst < DST_CLK0S16) tdst = prf->psdst;
            pdd->d[irb] = jasl(pdd->d[irb], tdst);
            /* Increment pdd here if more stats added later... */
            }

         /* One last loop over connection types to accumulate distri-
         *  butions of final afferent voltages, raw affs, average Sj,
         *  connection decay, and s(i) vs FDM category.  */
         for (ix=il->pct1; ix; ix=ix->pct) {
            int zbict = (int)ix->ict - 1;
            pdd = il->pctddst + ix->ocnddst + OaVd;
            if (ix->qaffmods) {
               pdd->d[irb] =
                  jasw(pdd->d[irb], CDAT.praffs[zbict]); ++pdd;
               }
            pdd->d[irb] = jasw(pdd->d[irb], CDAT.pfaffs[zbict]); ++pdd;
            if (qdecay(&ix->Cn.ADCY)) {
               si32 *ppaf = CDAT.pAffD32 + ix->odpafs + ix->npafraw;
               si32 tax = 0;
               if (ix->sssck & PSP_POS) {
                  tax += *ppaf;
                  ppaf += ix->npaftot; }
               if (ix->sssck & PSP_NEG)
                  tax += *ppaf;
               pdd->d[irb] = jasl(pdd->d[irb], tax); ++pdd;
               }
            if (cnexists(ix, RBAR)) {
               rd_type *pnuk = ix->psyn0 + Lija.lcoff + ix->xnoff[NUK];
               long nuk;
               d3gtln(nuk, pnuk, ix->nuklen);
               if (nuk > 0) {
                  pdd->d[irb] = jasl(pdd->d[irb],
                     jdswq(prbar[zbict], nuk));
                  }
               }
            if (ix->cnflgs & DOFDIS) {
               int iFd = ix->ucij.m.oFDMdist + icell%ix->ucij.m.nkt;
               il->pctdist[iFd].d[irb] += 1;
               }
            } /* End ix loop */

         /* Loop over GCONN blocks--Accumulate
         *  distributions of bandsums and persistence */
         {  struct INHIBBLK *ib;       /* Ptr to inhibition block */
         int gn = CDAT.groupn - il->logrp;
         for (ib=il->pib1; ib; ib=ib->pib) {
            struct INHIBBAND *bnde, *bnd = ib->inhbnd;
            pdd = il->pctddst + ib->oidist;
            for (bnde=bnd+ib->nib; bnd<bnde; bnd++,pdd++)
               pdd->d[irb] = jasw(pdd->d[irb], bnd->bandsum[gn]);
            /* If there is decay, accumulate persistence */
            if (qdecay(&ib->GDCY)) {
               si32 *pgx = CDAT.pAffD32 + ib->ogafs;
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
            si32 *pmx = CDAT.pAffD32 + imb->omafs;
            pdd = il->pctddst + imb->omdist;
            pdd[OmVd].d[irb] = jasl(pdd[OmVd].d[irb], pmx[0]);
            if (qdecay(&imb->MDCY)) pdd[OmWd].d[irb] =
               jasl(pdd[OmWd].d[irb], pmx[1]);
            }

         } /* End statistical section */

/* Record firing of cell for regeneration whether or not there
*  are any statistics.  */

      if ((il->Ct.ctopt & OPTRG) && hitht)
         *(plsd + il->ctoff[RGH]) |= 1;

/* If current cell has been listed for detailed print,
*  call d3dprt or d3pdpn to print the selected data.  */

      if (do_dprint && ilsttest(il->dpclb->pclil, (ilstitem)icell)) {
#ifdef PAR
         d3pdpn();
#else
         d3dprt();
#endif
         }  /* End detail print */

/* If there are any PHASR connections, update phase seeds now.
*  Note:  This extra loop over conntypes allows phseed to be
*  reused multiple times during processing of one cell (as
*  wpseed), e.g. for input + dprt + neuroanat plot, with just
*  this one update that corrects for any skipped connections.  */

      if (il->orphopt & PHASR) {
         for (ix=il->pct1; ix; ix=ix->pct) {
            if (ix->phopt & PHASR)
               udevskip(&ix->phseed, ix->nc);
            }
         }

/* R70, 01/19/17, GNR - d3exit call here when RP->ovflow != 0 was
*  removed -- now done in darwin3 main after all trial functions
*  have completed.  */

      }  /* End loop over cells */

/*---------------------------------------------------------------------*
*  Accumulate statistics that only need to be done after cell loop.    *
*         (numbers of hits and distribution of high/low s(i))          *
*---------------------------------------------------------------------*/

   if (statenbc) {
      il->CTST->nstcap += 1;
      il->CTST->hitpos += nhitspt;
#ifdef PARn
      /* Accumulate info for [hl]dist in qhlsi array */
      if (lhisi > qhlsi->hiresp) qhlsi->hiresp = lhisi;
      if (llosi < qhlsi->loresp) qhlsi->loresp = llosi;

      /* Accumulate info for KSTMG|KSTGP in qhlsi array
      *  (adding one to qhlsi index to skip generic HILOSI above) */
      if (il->Ct.kstat & (KSTGP|KSTMG) && !cyclestogo) {
         int ibyt,ibit,ibit2;
         for (ibyt=0,ibit=1; ibyt<(int)il->lmm; ibyt++) {
            ibit2 = ibit + 8;
            if (pmark[ibyt]) for ( ; ibit<ibit2; ibit++) {
               if (bittst(pmark, ibit)) {
                  struct HILOSI *qhl = qhlsi + ibit;
                  qhl->nstim += 1;
                  qhl->nhits += nhitsht;
                  if (lhisi > qhl->hiresp) qhl->hiresp = lhisi;
                  if (llosi < qhl->loresp) qhl->loresp = llosi;
                  }
               } /* End bit loop */
            else ibit = ibit2;
            } /* End byte loop */
         }  /* End recording KSTMG|KSTGP info */
#endif

#ifndef PAR    /*** SERIAL computer ***/
      il->CTST->hdist[ihisi] += 1;
      il->CTST->ldist[ilosi] += 1;
#ifdef DBGHLS
   if (il->seqvmo == 1)
      dbgprt(ssprintf(NULL, "Tr %4d, CT %4s, hdist[%4d] = %8d, "
         "ldist[%4d] = %8d", RP->CP.trial, il->lname, ihisi,
         il->CTST->hdist[ihisi], ilosi, il->CTST->ldist[ilosi]));
#endif
      if (il->Ct.kstat & (KSTGP|KSTMG) && !cyclestogo) {
         struct GPSTAT *pgp;
         int ibyt,ibit,ibit2;
         for (ibyt=ibit=0; ibyt<(int)il->lmm; ibyt++) {
            ibit2 = ibit + 8;
            if (pmark[ibyt]) for ( ; ibit<ibit2; ibit++) {
               if (bittst(pmark, ibit+1)) {
                  pgp = pgp0 + ibit;
                  pgp->nstim += 1;
                  pgp->nhits += nhitsht;
                  if (lhisi > pgp->hiresp)   pgp->hiresp = lhisi;
                  if (lhisi < pgp->lohiresp) pgp->lohiresp = lhisi;
                  if (llosi > pgp->hiloresp) pgp->hiloresp = llosi;
                  if (llosi < pgp->loresp)   pgp->loresp = llosi;
                  }
               } /* End bit loop */
            else ibit = ibit2;
            } /* End byte loop */
         } /* End recording GPSTAT stats */
#endif   /*** END SERIAL ***/

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

#ifndef PAR0
         /* Add contribution of this celltype to superposition plot.
         *  R77, 02/22/18, GNR - This code moved up from after the
         *     seed updating so noise seed at start of cycle is
         *     available for use in bar graph plots.  */
         if ((RP->kpl1 & (KPLSUP|KPLMFSP)) && (plot_cycle >= 0) &&
               !(il->Ct.kctp & CTPHP))
            d3lplt(il, il->ps2, plot_cycle, plvx, plvy);
#endif

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
         *  updating on each trial instead of once per CYCLE card.
         *  R67, 11/25/16, GNR - phseed update now in cycle init.  */

         {  si64 nskip = jesl(il->nelt);
#ifndef PAR0
            nskip = jrsl(nskip, il->hicell);
#endif
            if (il->No.noikadd & NFL_GENN)
               udevwskp(&il->No.nsd, jslsw(nskip,1));
            udevwskp(&il->rsd, jslsw(nskip,2));

            for (ix=il->pct1; ix; ix=ix->pct) {
               if (ix->kgfl & KGNBP) continue;
               if (ix->cnflgs & UPRSD) udevwskp(&ix->rseed,
                  jmswj(nskip,(si32)ix->upcij));
               } /* End updating conntype seeds */

#ifdef DBG_PHSEED
            if (il->ctf & CTFNC) {
dbgprt(ssprintf(NULL, "lyrfin Ser %d, Trial %d, Celltype %4s phiseed"
   " %jd", RP->CP.ser, RP->CP.trial, il->lname, il->pctpd->phiseed));
               udevwskp(&il->pctpd->phiseed, nskip);
dbgprt(ssprintf(NULL, "  Adv by %d, resulting in %jd",
   nskip, il->pctpd->phiseed));
               }
#else
            if (il->ctf & CTFNC)
               udevwskp(&il->pctpd->phiseed, nskip);
#endif

            } /* End scope of nskip */
#endif

         /* Determine and/or apply autoscale for next trial while
         *  response data are still available in CDAT.prgtht */
         if (il->kautu & (KAUT_HTN|KAUT_HPN|KAUT_APP)) d3autsc(il);

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
            /* Bug fix, 01/01/17: Remove test on RP->pgdcl,
            *  d3gfsvct does a full test on il->suitms.  */
            d3gfsvct(NULL, il, ~(GFLIJ|GFDIJ|GFABCD));
#endif
            stattmr(OP_POP_TMR, 0);
            }

         /* If this layer is updated by another one,
         *  wait until it comes up.  */
         if (!(il->ctf & CTFDU)) {
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

/* Collect high/low response statistics */

#ifdef PAR
   if (statenbg & STEG_THIS) {
      if (++CDAT.ihlsi >= RP->nhlssv) {
#ifdef DBGHLS
         dbgprt("d3go calling d3ghls/d3zhls");
#endif
         d3ghls();
         d3zhls();
         }
      }
#endif

/* Collect categorization statistics */

#ifndef PARn
   if (RP0->n0flags & N0F_ANYKCG)
         for (il=RP->pfct; il; il=il->pnct) {
      if (il->ctf & CTFSTENB && il->orkam & KAMCG) {
         s_type *psi = il->pps[0];
         int icell,hii = 0;
         si32 wksi,hisi = -SI32_MAX;
         int  llsp = (int)il->lsp;
         for (icell=0; icell<il->nelt; psi+=llsp,++icell) {
            d3gtjs2(wksi, psi);
            if (wksi > hisi) hisi = wksi, hii = icell;
            }
         hii /= il->nel;   /* Get group number */
         for (ix=il->pct1; ix; ix=ix->pct) if (ix->Cn.kam & KAMCG) {
            struct MODALITY *pm = ix->psmdlt;
            if (bittst(CDAT.pmbits + pm->mdltgoff,
                  (long)(hii % pm->ncs) + 1))
               ix->nccat += 1;
            }
         }
      } /* End categorization correct counting */
#endif

/* This completes one cycle--
*     Compute adaptive value of response.
*     Allow decay and amplification to begin in 2nd cycle.  */

   if (RP->pvblkr) d3valu(RP->pvblkr);
   CDAT.flags = 0;

   if (--cyclestogo >= 0) goto NEWCYCLE;

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
*  Location pND->narb contains a count of the number of equivalent     *
*  Cij being affected by this cleanup call.  This value is used to     *
*  increment relevant statistics.                                      *
*  V8D, 07/25/07, GNR - Remove nmod to pre-avg code in preparation     *
*    for complete rewrite of averaging.                                *
*---------------------------------------------------------------------*/

#ifndef PAR0
static void cijclean(struct CONNTYPE *ix, struct CONNDATA *pND) {

   register si32 work;           /* Various temporary values */
   si32 curr_cij = pND->curr_cij;/* Value of Cij being adjusted */
   si32 trun_new_cij;            /* Truncated new Cij */
   si32 worksign;                /* Holds sign of curr_cij */
   int  nbcshft = ix->Cn.nbc+14; /* Shift for new Cij */

/* Random-round or truncate accord to whether new Cij is in bounds */

   if ((work=absj(curr_cij)) < ix->bigc16) {
      /* The new Cij is within truncation limits.
      *  Apply random rounding because Cij is highly quantized.
      *  A separate rounding seed is used to avoid changing any
      *  other random quantities.  Note that the rounding term
      *  is always positive because truncation always goes toward
      *  algebraically lower values.  A positive value just below
      *  bigc16 can be incremented to equal, but not exceed, it.  */
      curr_cij += udev(&ix->rseed) >> nbcshft;
      }
   else {
      if (work > ix->bigc16)
         /* New Cij is absolutely out-of-bounds, count overflows */
         ix->CNST.cover += pND->narb;
      curr_cij = (curr_cij < 0) ? -ix->bigc16 : ix->bigc16;
      }

/* Handle sign changes--
*  Count all attempts to change sign of Cij
*  If sign change disallowed, set result to zero--
*     will get sign when stored below.  */

   worksign = curr_cij;    /* Hold sign in case set to zero */
   if ((pND->old_cij ^ curr_cij) < 0) {
      ix->CNST.cschg += pND->narb;
      if (ix->Cn.kam & KAMNC)
         { curr_cij = 0; worksign = pND->old_cij; }
      }

/* Convert new Cij back to S31 and return in pND structure.
*     If magnitude is zero, give it sign of new or old Cij
*        according to whether sign changes are allowed.
*     Code assumes overflowing Cij have already been reset.  */

   work = (curr_cij << 15) & ix->cmsk;
   trun_new_cij = SRA(work,15);
   if (work == 0) work = SI32_SGN & worksign;
   pND->new_cij = work;

/* If baseline Cij is being maintained, update it now (S16).
*  (Setup code sets vrho to 0 if Cij0 not allocated.)  */

   if (ix->cnwk.go.vrho) {
      /* To avoid bias, use the rounded and truncated new Cij */
      si32 curr_cij0 = pND->curr_cij0;
      curr_cij0 += mrsrsle(trun_new_cij - curr_cij0,
         ix->cnwk.go.vrho, FBsc, ix->iovec+CNO_AMP);
      /* Make sure |base_cij| < 1.0 (this is necessary
      *  because rho can in principle be > 1.0).  */
      if ((work=absj(curr_cij0)) < ix->bigc16)
         /* Now have the new base_cij (S16) and it did not
         *  overflow.  Apply random rounding as for curr_cij. */
         curr_cij0 += udev(&ix->rseed) >> nbcshft;
      else
         /* base_cij did overflow.  Set to limiting value. */
         curr_cij0 = (curr_cij0 < 0) ? -ix->bigc16 : ix->bigc16;

      /* Enforce same sign change restriction as for Cij,
      *  except we don't have to worry about minus 0. */
      if (((pND->old_cij ^ curr_cij0) < 0) &&
         (ix->Cn.kam & KAMNC)) pND->new_cij0 = 0;
      else
         pND->new_cij0 = (curr_cij0 << 15) & ix->cmsk;
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
*  Rev, 04/07/13, Sums are now si64, overflow checking not needed.     *
*---------------------------------------------------------------------*/

static void sumaffs(struct CELLTYPE *il, si64 *totals) {

   if (il->phshft) {          /* Cells have phase */
      for ( ; *pcontbl > EndAllConsTbl; pcontbl++) {
         if (*pcontbl == EndConsTbl)
            totals += PHASE_RES;
         else {
            si64 *fa = CDAT.pAffD64 + *pcontbl;
            register int p;      /* Phase index */
            for (p=0; p<PHASE_RES; p++)
               totals[p] = jasw(totals[p], fa[p]);
            }
         } /* End loop over consolidation tables */
      } /* End sums for cells with phases */
   else {                     /* Cells do not have phase */
      for ( ; *pcontbl > EndAllConsTbl; pcontbl++) {
         if (*pcontbl == EndConsTbl)
            totals += 1;
         else {
            si64 *fa = CDAT.pAffD64 + *pcontbl;
            *totals = jasw(*totals, *fa);
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

   for (imv=il->pmrv1; imv; imv=imv->pmdv) {
      if (curr_cyc1 < imv->Mdc.mdefer)
         imv->umds.mdsum = jesl(0);
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
            d3gtjs2(wksi, psi);
            wksi -= lmjrv;
            if      (wksi > lmthi) wksi -= lsmthi;
            else if (wksi < lmtlo) wksi -= lsmtlo;
            else continue;
            if (imv->Mdc.mopt & IBOPMX) {
               if (qsw(jrsl(wsum, wksi)) < 0) wsum = jesl(wksi);
               }
            else
               wsum = jasl(wsum, wksi);
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

static void layerupd(struct CELLTYPE *il, si16 cyclestogo) {

#ifndef PARn
   struct EFFARB  *pea;
#endif
   int    mxsd;               /* Max s delay */

/* If this cell type feeds any connections with delay, push
*  all s(i) values in pipeline back one time step, making room
*  for newest s(i) in current slot.  This is done by rotating
*  the pointer array at *pps.  This allows the pointer for delay
*  jd to be accessed directly at il->pps[jd] w/o modulo arith.
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
   for (pea=il->pfea; pea; pea=pea->pnetct) {

      long (*iterator)(iter *);  /* Ptr to iterator function */
      float *pewd;               /* Ptr to output data storage */
      s_type *psi;               /* Ptr to Si data */
      iter wkitc;                /* Working iteration control */
      float wescl;               /* Effector output scale */
      si32 weft;                 /* Effector action threshold */
      si32 wksi;                 /* Unpacked value of s(i) */
      int  ias,iase;             /* Arbor separation */
      int  icn;                  /* Current cell number */
      int  iei;                  /* EXCIT or INHIB */
      int  ojnwd;                /* Offset into pjnwd */

      /* Omit if frozen (output is zeroed above) */
      if (pea->keop & KEOP_FRZ) continue;
      /* Omit if cycle is too early for allocated space */
      ojnwd = (pea->arbw[EXCIT] + pea->arbw[INHIB]) *
         (pea->entc - (cyclestogo + 1));
      if (ojnwd < 0) continue;

      /* Loop over excitation, inhibition lists */
      for (iei=EXCIT; iei<NEXIN; ++iei) {
         /* Omit if not coded for this celltype (or at all) */
         if (pea->pect[iei] != il) continue;

         /* Working constants for cell loop */
         pewd = RP0->pjnwd + pea->jjnwd[iei] + ojnwd;
         weft = (si32)pea->eft[iei];
         wescl = pea->esclu[iei];
         if (!(pea->keop & KEOP_CELLS)) wescl /= (float)pea->arbw[iei];

         /* Prepare loop over possible multiple chgs outputs.
         *  d3schk promises no pea entry here if nchgs == 0.  */
         iase = pea->nchgs * pea->arbsep[iei];
         for (ias=0; ias<iase; ias+=pea->arbsep[iei]) {

            /* Prepare loop over cells.  Entire loop will be
            *  repeated for each iteration of ias loop, with
            *  ias added to cell number.  d3schk and d3news
            *  should check that nelt will not be exceeded.  */
            if (pea->pefil[iei]) {     /* User cell list */
               iterator = ilstiter;
               ilstset(&wkitc, pea->pefil[iei], 0);
               }
            else {                     /* Synthetic iterator */
               iterator = allciter;
               wkitc.inow = 0;
               wkitc.iend = (ilstitem)pea->arbw[iei];
               }

            if (pea->keop & KEOP_CELLS) {
               /* Send data for individual cells to device.
               *  Loop over cells contributing to this effector */
               while ((icn=(int)iterator(&wkitc)) >= 0) {
                  icn += ias;
                  psi = il->pps[0] + spsize(icn,il->phshft);
                  d3gtjs2(wksi, psi);
                  if (wksi < weft) wksi = 0;
                  *pewd++ = wescl*(float)wksi;
                  } /* End of loop over cells */
               } /* End KEOP_CELLS method */
            else {
               /* Sending sums, not all cells */
               si64 lsum = jesl(0);    /* Layer sum */
               /* Loop over cells contributing to this effector */
               while ((icn=(int)iterator(&wkitc)) >= 0) {
                  icn += ias;
                  psi = il->pps[0] + spsize(icn,il->phshft);
                  d3gtjs2(wksi, psi);
                  if (wksi < weft) continue;
                  lsum = jasl(lsum, wksi);
                  } /* End of loop over cells */
               /* Store the result */
               *pewd = wescl*swflt(lsum);
               } /* End !KEOP_CELLS */

            } /* End changes loop */
         } /* End of loop over excit and inhib outputs */
      } /* End of loop over EFFECTOR blocks */

#endif   /* not PARn */
   } /* End layerupd() */

