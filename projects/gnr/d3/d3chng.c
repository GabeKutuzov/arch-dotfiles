/* (c) Copyright 1990-2018, The Rockefeller University *21116* */
/* $Id: d3chng.c 77 2018-03-15 21:08:14Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                              d3chng()                                *
*                         Process CHANGE card                          *
*                                                                      *
*  A philosophical note:                                               *
*     In this program, we want to match input keys with smatch() so    *
*  matching semantics will be identical throughout CNS.  However,      *
*  smatch() requires an array of pointers to the valid keys that is    *
*  not directly available from the chop_tbl array used here to define  *
*  the manifold change options in an easily maintained way.  After     *
*  some work with a modified local version of smatch(), the solution   *
*  adopted was to construct a pointer array during the first call and  *
*  keep its address in static memory for subsequent calls.             *
*                                                                      *
************************************************************************
*  V3L, 05/08/90, GNR - Latest CMS version                             *
*  V4A, 05/25/90, JMS - Translated to C                                *
*  V5E, 07/25/92, GNR - Move mtj to LCnD struct, #if-out old rescale   *
*                       Add rho and dvi, reformat, merge beta & mxib   *
*  V6A, 03/26/93, GNR - Add KAMDQ, KAMDR bits                          *
*  V6D, 02/22/94, GNR - Add sspike, ctscll                             *
*  V7A, 05/07/94, GNR - Correct coding of DELAY option                 *
*  V7B, 07/04/94, GNR - Add amplif scales, repertoire plot parms,      *
*                       amplif rule L, ptscll, stscll, remove ctscll,  *
*                       eliminate byte swapping caused by using d3ptln *
*  Rev, 08/24/94, GNR - Restructure mod and GCONN params, add decay    *
*  V7C, 01/28/95, GNR - Add vdt and vdha parameters                    *
*  V8A, 05/06/95, GNR - H,X,Y,Z,C,F,U stats, CDCY to CELLTYPE, falloff *
*                       D metric, remove "LTP", add REFRAC, mxmod, pst,*
*                       use d3inhs() to scale betas, add CH_SS bit and *
*                       keylist array, use smatch() to match keys      *
*  Rev, 03/01/97, GNR - Remove change restriction on KRPRR, add KRPEP  *
*  Rev, 09/28/97, GNR - Independent dynamic allocations per conntype   *
*  Rev, 12/23/97, GNR - Add ADCY, PPF, remove some stats restrictions  *
*  Rev, 02/18/98, GNR - Add resting potential, afterhyperpolarization  *
*  Rev, 10/28/00, GNR - Remove XRH, tabulate KRPHR over all cycles     *
*  V8B, 03/10/01, GNR - Add SVITMS                                     *
*  V8C, 03/01/03, GNR - Variable input via eqscan(), restructure chop  *
*                       table for metric units, compatibility scales   *
*  V8D, 11/11/06, GNR - Make D,P,T,Q,K options same in rep,cell blocks *
*  Rev, 04/07/07, GNR - Remove NMT, NSCL, provide as MT, MSCL options  *
*  Rev, 09/28/07, GNR - Add RDAMP, move KAMFZ to OPTFZ                 *
*  Rev, 10/21/07, GNR - Add RF (Response Function) option              *
*  Rev, 12/27/07, GNR - Separate KP and KCTP options                   *
*  ==>, 01/09/08, GNR - Last mod before committing to svn repository   *
*  Rev, 03/26/08, GNR - Add SHAPE, 05/21/08 add ChThr codes            *
*  V8E, 01/19/09, GNR - Add Izhikevich cell response function, COLOR,  *
*                       regularize (rep,*) usage, better type switch   *
*  Rev, 05/05/09, GNR - Revised getthr and all input & amp test vars.  *
*  Rev, 09/06/09, GNR - Add variables for Brette-Gerstner (aEIF) cells,*
*                       expand key, esfc to hold longer strings        *
*  V8G, 08/13/10, GNR - Add AUTOSCALE parameters                       *
*  V8H, 02/24/11, GNR - Allow to kill, not add, KAMCG                  *
*  Rev, 05/25/12, GNR - Rename ASxxx, add GBCM, remove d3inhs() call   *
*  Rev, 06/07/12, GNR - Add ADEFER, CNOPT, QPULL                       *
*  Rev, 04/23/13, GNR - Add MXSI, long->si32 changes                   *
*  Rev, 05/11/13, GNR - Remove gconn falloff                           *
*  V8I, 05/14/13, GNR - Add AUTOSCALE KAUT=H parameters                *
*  Rev, 07/25/13, GNR - Add [A,S,G,M,C]USAGE parameters                *
*  Rev, 09/18/13, GNR - Correct handling of DECAY parameter updates    *
*  Rev, 08/22/14, GNR - Si only EXP decay, remove DECAYDEF from DCYDFLT*
*  R63, 11/03/15, GNR - Add vdopt codes                                *
*  R66, 02/17/16, GNR - Change long random seeds to typedef si32 orns  *
*  R72, 03/09/17, GNR - Add REGION XGAP,YGAP                           *
*  R74, 08/05/17, GNR - Split kstat off from kctp, add siha            *
*  R77, 01/30/18, GNR - Add vdmm, emxsj, 05/09/18 sslope               *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <ctype.h>
#include <math.h>
#include "sysdef.h"
#include "d3global.h"
#include "d3opkeys.h"
#include "rocks.h"
#include "rkxtra.h"
#include "bapkg.h"

#define LN2S16 45426.093625176   /* Ln(2) * S16 */

/* Define methods of handling various CHANGE options */
enum handling {
   rep,
   layer,
   lsvitms,
   autscl,
   bregrf,
   iz03rf,
   iz07rf,
   phsdata,
   rfrcdata,
   sishape,
   sicolor,
   sicolsrc,
   conntype,
   ppfdata,
   krpcode,
   kctpcode,
   kstacode,
   kamcode,
   kamway,
   cnopcode,
   vdopcode,
   modtype,
   gcontype,
   getbeta,
   getmxib
   };

/* Define change-operation-table-entry (chopte) */
struct chopte {
   char     key[10];          /* User-entered value name */
   char     esfc[16];         /* eqscan format code */
   short    flags;            /* Flags for value handling */
/* Values for flags (low 5 bits are allocation code for variable) */
#define ChALLO  0x1f          /* Check that variable is allocated */
#define ChSS    0x20          /* Variable allows subscript */
#define ChDCV   0x40          /* Value is a decay parameter */
#define ChRPL   0x80          /* Has repertoire name only, no layer */
#define ChMT  0x0100          /* Variable is mt,mtlo,mjrev */
#define ChThr 0x0400          /* Variable is a B7/B14 threshold */
#define ChScl 0x0800          /* Variable is conntype scale */
#define ChKEY 0x1000          /* Variable is mcodes type */
#define ChALR 0x2000          /* Item can affect all layers in rep */
   enum handling type;        /* Value type */
   int      offs;             /* Offset into structure */
   int      len;              /* Length of item */
   };

/* Macro to get the offset and length of a field in a structure--
*  doesn't work for array fields */
#define ofl(styp, fld) \
   ((char *)&(((struct styp *)0)->fld) - (char *)0), \
   sizeof(((struct styp *)0)->fld)

/* Two different flavors of exit that the main function can use */
#define fld_err(n) chng_exit(enomsg, n)
#define errmsg(m)  chng_exit(m, 0)

/* Error message types--enum is used to spec desired message in calls
*  to error message function (see chng_exit() and macros below).
*
*  It would be good to keep the enum constants & the error message
*  texts in sync, otherwise you'll probably generate confusing error
*  messages.  */
enum errmsg_typ {
   enomsg = -1,
   erep = 0,
   elay,
   exsub,
   esubsc,
   egcsubs,
   esclovfl,
   eoptstor,
   exmh,
   eampopt,
   emultamp,
   eadecay,
   };

/* C longjmp buffers */
static jmp_buf err_jmp,       /* For jumps out of error function */
      eof_jmp;                /* For jumps out of scan() on eof */

/* Buffer for scan() - static because used by loop exit function.
*  Make it long enough for maximum possible keycode string */
#define MXFLDLEN ((SCANLEN > MXKEYLEN) ? SCANLEN : MXKEYLEN)
static char fldbuf[MXFLDLEN+1];

/* Array of pointers to option keys constructed for smatch() call */
static char **keylist;        /* Initialized to NULL per ANSI C std. */

/*---------------------------------------------------------------------*
*                              chng_exit                               *
*                                                                      *
*              Handle field value and card syntax errors               *
*                                                                      *
*  This is a static (local) function containing common code to recover *
*  from an error - scans the card starting at the field in error until *
*  it reaches a clean field.  Generates an error message.  Exits with  *
*  longjmp to beginning of main loop.                                  *
*---------------------------------------------------------------------*/

static void chng_exit(enum errmsg_typ emsg_ndx, int msg_code) {

static char *errtxt[] = {
      " ***REGION/REPERTOIRE AT \"$\" NOT FOUND.",
      " ***CELL TYPE AT \"$\" NOT FOUND.",
      " ***SUBSCRIPT AT \"$\" NOT ALLOWED WITH THIS VARIABLE.",
      " ***SUBSCRIPT FOR ITEM AT \"$\" EXCEEDS NUMBER OF ITEMS "
           "IN NETWORK.",
      " ***GCONN TYPE SUBSCRIPT REQUIRED.",
      " ***SCALE AT \"$\" OVERFLOWED.",
      " ***OPTION AT \"$\" REQUIRES PRIOR STORAGE ALLOCATION.",
      " ***C,G,H,X,Y,Z MUST BE REQUESTED IN GROUP II.",
      " ***OPTION AT \"$\" MUST BE REQUESTED ON \"AMPLIF\" CARD.",
      " ***MORE THAN ONE AMPLIFICATION OPTION REQUESTED."
      " ***ADECAY CANNOT BE USED WITH PHASED INPUTS."
      };

   ermark(msg_code);          /* Generate error message */

   if (emsg_ndx != enomsg)
       cryout(RK_P1, errtxt[emsg_ndx], RK_LN2, NULL);

   for (;;) {
      /* If last scan was right-paren (while in parens) or
      *  (comma or blank) (while not in parens), return */
      if (RK.scancode == (RK_INPARENS|RK_RTPAREN) ||
         (RK.scancode & ~(RK_COMMA|RK_BLANK)) == 0) break;
      /* Otherwise, scan to skip a field, exit at end-of-card */
      if (scan(fldbuf, 0) & RK_ENDCARD) longjmp(eof_jmp,0);
      }

   /* Branch to top of main loop */
   longjmp(err_jmp, 0);
   } /* End chng_exit */

/*---------------------------------------------------------------------*
*                            find_inhibblk                             *
*                                                                      *
*  This function gets a ptr to an inhibition block given its index.    *
*  Counting of INHIBBLK's starts at 1.                                 *
*---------------------------------------------------------------------*/
static struct INHIBBLK *find_inhibblk(struct CELLTYPE *il, int ndx) {

   struct INHIBBLK *ib;

   if (ndx < 1) chng_exit(esubsc, RK_MARKFLD);

   for (ib = il->pib1; ib; ib = ib->pib)
      if (! --ndx) break;

   if (!ib) chng_exit(esubsc, RK_MARKFLD);

   return(ib);
   } /* End find_inhibblk */

/*---------------------------------------------------------------------*
*                            update_decay                              *
*                                                                      *
*  This function updates a specific DECAYDEF based on parameters read  *
*  by getdecay() into newval.nd, with checking for illegal changes in  *
*  kdcy method.  Note that we cannot just call getdecay() after the    *
*  DECAYDEF is located, because there might be updates of multiple     *
*  conntype ADCYs, and we have no way to back up the card scan over    *
*  what might rarely be a continuation inside the parentheses.         *
*---------------------------------------------------------------------*/

static void update_decay(struct DECAYDEF *pdcd, struct DECAYDEF *pnew) {

   /* New kdcy spec, if entered, is always OK if same as old kdcy.
   *  If not same, it is still OK if NODECAY, if CHNGOK bit is set,
   *  or if it changes among set DCYEXP, DCYLIM, DCYSAT.  */
   if (pnew->dcyopt & DCY_NOKDE || pnew->kdcy == NODECAY) {
      offdecay(pdcd); return; }
   if (pnew->kdcy == pdcd->kdcy)
      goto Update_decay_parms;
   if (pdcd->dcyopt & DCY_CHGOK || (pdcd->kdcy > 0 &&
         pdcd->kdcy <= DCYSAT && pnew->kdcy <= DCYSAT))
      pdcd->kdcy = pnew->kdcy;
   else {
      cryout(RK_E1, "0***Invalid decay method change--"
         "Use CHNGOK.", RK_LN2, NULL);
      offdecay(pdcd);
      return;
      }
Update_decay_parms:
   pdcd->omega = pnew->omega;
   pdcd->du1 = pnew->du1;
   pdcd->ndal = pdcd->kdcy >= DCYALPH ? 2 : 1;
   } /* End update_decay */


/*---------------------------------------------------------------------*
*                               d3chng                                 *
*                                                                      *
*  Function to implement the options on the CNS Group III CHANGE card. *
*---------------------------------------------------------------------*/

void  d3chng(void) {

/* Change-operation table--one entry for each possible value that can
*     be changed.  Each entry has a type value from the enumeration
*     'handling'.  The type indicates which data structure contains
*     the value, or that the particular value is to receive special
*     handling.  Values with types 'rep', 'layer', 'bregrf', 'izhirf',
*     'rfrcdata', 'conntype', 'gcontype', 'modtype', and 'ppfdata'
*     are fields in the pertinent structures, and have the offset of
*     the field from the beginning of the structure and the length of
*     the field stored in their table entries.  The instance of the
*     particular structure specified on the CHANGE card is located,
*     and the value specified on the card is stored at the correct
*     offset in the structure, possibly with special cases indicated
*     by flags.  All other values are processed by special code
*     selected by a switch on the value's type.
*  The ChThr flag labels B7/14 threshold values whose scale depends
*     on whether the specific or modulatory connection type receives
*     input from a repertoire or a virtual cells (IA, TV, etc.) and
*     whether OLDRANGE compatibility is in effect.  These conditions
*     are handled by getthr().  They may vary in a loop over connec-
*     tion types.  This is handled in an ugly fashion by pushing the
*     field back and calling getthr() for each connection type.
*  Method of identifying keys and extracting subscripts was corrected
*     in V8A.  Earlier versions did not detect ambiguous abbreviations
*     or keys with extraneous characters appended.
*  The word 'value' used anywhere in this file has nothing to do with
*     CNS value schemes.
*/

static struct chopte chop_tbl[] = {
/*Keyword  esfc           flags      type      ofl(StrucTyp,     Item)*/
/*-------  -------        -------    --------  -----------------------*/
{"ADAMP",  "V>0<=1B15UH", 0,         autscl,   ofl(AUTSCL,      adamp)},
{"ADECAY", "-",     ChDCV+ChSS,      conntype, ofl(CONNTYPE,  Cn.ADCY)},
{"ADEFER", "UH",          ChSS,      conntype, ofl(CONNTYPE,Cn.adefer)},
{"AHP",    "B20/27IJ$mV", DST,       rfrcdata, ofl(RFRCDATA,      ahp)},
{"ASTT",   "B7/14IH$mV",  0,         autscl,   ofl(AUTSCL,       astt)},
{"ASTF1",  "B15UH",       0,         autscl,   ofl(AUTSCL,      astf1)},
{"ASTFL",  "B15UH",       0,         autscl,   ofl(AUTSCL,      astfl)},
{"ASMNIMM","V>0<1B24IJ",  0,         autscl,   ofl(AUTSCL,    asmnimm)},
{"ASMXD",  "V>0<1B24IJ",  0,         autscl,   ofl(AUTSCL,      asmxd)},
{"ASMXIMM","V>0B24IJ",    0,         autscl,   ofl(AUTSCL,    asmximm)},
{"AUSAGE", "N" KWS_PSPOV, ChKEY,     layer,    ofl(CELLTYPE,   apspov)},
{"BETA",   "-",           ChSS,      getbeta                          },
{"BGA",    "B24IJ$nS",    IZU,       bregrf,   ofl(BREGDEF,       bga)},
{"BGB",    "B20/27IJ$nA", IZU,       bregrf,   ofl(BREGDEF,       bgb)},
{"BGDELT","V>B20/27IJ$mV",IZU,       bregrf,   ofl(BREGDEF,      delT)},
{"BGGL",   "B20UJ$nS",    IZU,       bregrf,   ofl(BREGDEF,        gL)},
{"BGTAUW", "V>B20UJ$ms",  IZU,       bregrf,   ofl(BREGDEF,      tauW)},
{"BGVPEAK","B20/27IJ$mV", IZU,       bregrf,   ofl(BREGDEF,     vPeak)},
{"BGVRESET","B20/27IJ$mV",IZU,       bregrf,   ofl(BREGDEF,    vReset)},
{"BGVT",   "B20/27IJ$mV", IZU,       bregrf,   ofl(BREGDEF,        vT)},
{"CM",     "V>8B23UJ$pF", 0,         layer,    ofl(CELLTYPE,    Ct.Cm)},
{"CNOPT",  "-",           ChSS+ChKEY,cnopcode                         },
{"COLOR",  "-",           ChALR,     sicolor                          },
{"COLSRC", "-",           ChALR,     sicolsrc                         },
{"CPCAP",  "B7/14IH$mV",  0,         layer,    ofl(CELLTYPE, Ct.cpcap)},
{"CPT",    "B7/14IH$mV",  0,         layer,    ofl(CELLTYPE,   Ct.cpt)},
{"DAMPFACT","V<=1B15UH",  SBAR,      layer,    ofl(CELLTYPE, Dc.sdamp)},
{"DECAY",  "V<=1B30UJ",   0,         layer,    ofl(CELLTYPE, Dc.omega)},
{"DELAY",  "VIC",         ChSS,      conntype, ofl(CONNTYPE,Cn.retard)},
{"DELTA",  "VB28IJ",      ChSS,      conntype, ofl(CONNTYPE,Cn.mdelta)},
{"DVI",    "VIH",         ChSS+CIJ0, conntype, ofl(CONNTYPE,   Cn.dvi)},
{"EPSILON","V<=1>=0B30IJ",0,         layer,   ofl(CELLTYPE,Dc.epsilon)},
{"EQUIL",  "B16IJ",       ChSS,      conntype, ofl(CONNTYPE,Cn.target)},
{"ET",     "X",     ChThr+ChSS,      conntype, ofl(CONNTYPE,    Cn.et)},
{"ETHI",   "X",     ChThr+ChSS,      conntype, ofl(CONNTYPE,    Cn.et)},
{"ETLO",   "X",     ChThr+ChSS,      conntype, ofl(CONNTYPE,  Cn.etlo)},
{"FRAC",   "V>=0<=1B24IJ",0,         layer,    ofl(CELLTYPE,   No.nfr)},
{"GAMMA",  "V<=1B24UJ",   ChSS,      conntype, ofl(CONNTYPE, Cn.gamma)},
{"GBCM",   "VB24IJ",      ChSS,      gcontype, ofl(INHIBBLK,     gbcm)},
{"GDECAY", "-",     ChDCV+ChSS,      gcontype, ofl(INHIBBLK,     GDCY)},
{"GJREV",  "CR",    ChThr+ChSS,      gcontype, ofl(INHIBBLK,    gjrev)},
{"GUSAGE", "N" KWS_PSPOV, ChSS+ChKEY,gcontype, ofl(INHIBBLK,   gpspov)},
{"H",      "V>F",         ChRPL,     rep,      ofl(REPBLOCK,     aplh)},
{"HT",     "B20/27IJ$mV", 0,         layer,    ofl(CELLTYPE,    Ct.ht)},
{"HTDN",   "B1UH",        ChSS+PPF,  ppfdata,  ofl(PPFDATA,      htdn)},
{"HTUP",   "B1UH",        ChSS+PPF,  ppfdata,  ofl(PPFDATA,      htup)},
{"IT",     "CR",    ChThr+ChSS,      gcontype, ofl(INHIBBLK,      itt)},
{"ITHI",   "CR",    ChThr+ChSS,      gcontype, ofl(INHIBBLK,      itt)},
{"ITLO",   "CR",    ChThr+ChSS,      gcontype, ofl(INHIBBLK,    ittlo)},
{"I3A",    "B28IJ",       IZU,       iz03rf,   ofl(IZ03DEF,     Z.iza)},
{"I3B",    "B28IJ",       IZU,       iz03rf,   ofl(IZ03DEF,     Z.izb)},
{"I3C",    "B20/27IJ$mV", IZU,       iz03rf,   ofl(IZ03DEF,     Z.izc)},
{"I3CV0",  "B20/27IJ$mV", IZU,       iz03rf,   ofl(IZ03DEF,     izcv0)},
{"I3CV1",  "B24IJ",       IZU,       iz03rf,   ofl(IZ03DEF,     izcv1)},
{"I3CV2","V~B30/23IJ$-/mV",IZU,      iz03rf,   ofl(IZ03DEF,     izcv2)},
{"I3D",    "B22/29IJ$mV", IZU,       iz03rf,   ofl(IZ03DEF,     Z.izd)},
{"I3VPEAK","B20/27IJ$mV", IZU,       iz03rf,   ofl(IZ03DEF,   Z.vpeak)},
{"I3VUR",  "B20/27IJ$mV", IZU,       iz03rf,   ofl(IZ03DEF,     Z.vur)},
{"I7A",    "B28IJ",       IZU,       iz07rf,   ofl(IZ07DEF,     Z.iza)},
{"I7B",    "B23IJ",       IZU,       iz07rf,   ofl(IZ07DEF,      iizb)},
{"I7BVLO", "B23IJ",       IZU,       iz07rf,   ofl(IZ07DEF,      bvlo)},
{"I7C",    "B20/27IJ$mV", IZU,       iz07rf,   ofl(IZ07DEF,     Z.izc)},
{"I7D",    "B19/26IJ$mV", IZU,       iz07rf,   ofl(IZ07DEF,      iizd)},
{"I7K",    "VB24/17IJ",   IZU,       iz07rf,   ofl(IZ07DEF,       izk)},
{"I7UMAX", "B19/26IJ$mV", IZU,       iz07rf,   ofl(IZ07DEF,      umax)},
{"I7UMC",  "B28IJ",       IZU,       iz07rf,   ofl(IZ07DEF,       umc)},
{"I7UMVP", "B28IJ",       IZU,       iz07rf,   ofl(IZ07DEF,      umvp)},
{"I7UV3",  "B30IJ",       IZU,       iz07rf,   ofl(IZ07DEF,       uv3)},
{"I7UV3LO","B30IJ",       IZU,       iz07rf,   ofl(IZ07DEF,     uv3lo)},
{"I7VPEAK","B20/27IJ$mV", IZU,       iz07rf,   ofl(IZ07DEF,   Z.vpeak)},
{"I7VT",   "B20/27IJ$mV", IZU,       iz07rf,   ofl(IZ07DEF,      izvt)},
{"I7VUR",  "B20/27IJ$mV", IZU,       iz07rf,   ofl(IZ07DEF,     Z.vur)},
{"KCTP",   "-",           ChALR+ChKEY,kctpcode                        },
{"KRP",    "-",           ChRPL+ChKEY,krpcode                         },
{"MDECAY", "-",     ChDCV+ChSS,      modtype,  ofl(MODBY,        MDCY)},
{"MEAN",   "B20/27IJ$mV", 0,         layer,    ofl(CELLTYPE,   No.nmn)},
{"MJREV",  "X",     ChThr+ChSS+ChMT, modtype,  ofl(MODBY,    Mc.mjrev)},
{"MNAX",   "B20/27IJ$mV", ChSS,      conntype, ofl(CONNTYPE,  Cn.mnax)},
{"MNSI",   "CR",    ChThr+ChSS,      conntype, ofl(CONNTYPE,  Cn.mnsi)},
{"MNSJ",   "X",     ChThr+ChSS,      conntype, ofl(CONNTYPE,  Cn.mnsj)},
{"MSCL",   "B20IL",       ChSS,      modtype,  ofl(MODBY,        mscl)},
{"MT",     "X",     ChThr+ChSS+ChMT, modtype,  ofl(MODBY,       Mc.mt)},
{"MTHI",   "X",     ChThr+ChSS+ChMT, modtype,  ofl(MODBY,       Mc.mt)},
{"MTI",    "R",     ChThr+ChSS,      conntype, ofl(CONNTYPE,   Cn.mti)},
{"MTICKS", "UH",          ChSS+MIJ,  conntype, ofl(CONNTYPE,Cn.mticks)},
{"MTJ",    "X",     ChThr+ChSS,      conntype, ofl(CONNTYPE,   Cn.mtj)},
{"MTLO",   "X",     ChThr+ChSS+ChMT, modtype,  ofl(MODBY,     Mc.mtlo)},
{"MTO",    "X",     ChThr+ChSS,      modtype,  ofl(MODBY,         mto)},
{"MTOHI",  "X",     ChThr+ChSS,      modtype,  ofl(MODBY,         mto)},
{"MTOLO",  "X",     ChThr+ChSS,      modtype,  ofl(MODBY,       mtolo)},
{"MTV","B" qqv(FBvl) "IH",ChSS,      conntype, ofl(CONNTYPE,   Cn.mtv)},
{"MUSAGE", "N" KWS_PSPOV, ChSS+ChKEY,modtype,  ofl(MODBY,   Mc.mpspov)},
{"MXAX",   "B20/27IJ$mV", ChSS,      conntype, ofl(CONNTYPE,  Cn.mxax)},
{"EMXSJ",  "X",     ChThr+ChSS,      conntype, ofl(CONNTYPE, Cn.emxsj)},
{"MXIB",   "-",           ChSS,      getmxib                          },
{"MXIMG",  "VB20/27IJ",   ChSS,      conntype, ofl(CONNTYPE, Cn.mximg)},
{"MXMIJ",  "VB14IH$7mV",  ChSS+MIJ,  conntype, ofl(CONNTYPE, Cn.mxmij)},
{"MXMOD",  "B20/27IJ$mV", ChSS,      modtype,  ofl(MODBY,       mxmod)},
{"MXMP",   "VB20IJ$7mV",  ChSS+MIJ,  conntype, ofl(CONNTYPE,  Cn.mxmp)},
{"MXSI",   "CR",    ChThr+ChSS,      conntype, ofl(CONNTYPE,  Cn.mxsi)},
{"NAPC",   "N" KWS_NAPC,  ChSS,      conntype, ofl(CONNTYPE,  Cn.napc)},
{"NAVG",   "IH",          0,         autscl,   ofl(AUTSCL,       navg)},
{"NT",     "B20/27IJ$mV", 0,         layer,    ofl(CELLTYPE,    Ct.nt)},
{"OMEGA1", "V<=1B15UH",   0,         layer,    ofl(CELLTYPE,Dc.omega1)},
{"OMEGA2", "V<=1B30UJ",   0,         layer,    ofl(CELLTYPE, Dc.omega)},
{"OMEGAD", "V>0<=1B30IL", DEPR,      layer,    ofl(CELLTYPE,Dp.omegad)},
{"OMEGADST","V>=0<1B30IJ",DST,       rfrcdata, ofl(RFRCDATA, omegadst)},
{"PHT",    "B20/27IL$mV", 0,         phsdata,  ofl(PHASEDEF,      pht)},
{"PPFLIM", "V>=-1B12IH",  ChSS+PPF,  ppfdata,  ofl(PPFDATA,    ppflim)},
{"PPFT",   "X",     ChThr+ChSS+PPF,  ppfdata,  ofl(PPFDATA,      ppft)},
{"PSDST",  "B20/27IJ$mV", DST,       rfrcdata, ofl(RFRCDATA,    psdst)},
{"PT",     "B20/27IJ$mV", 0,         layer,    ofl(CELLTYPE,    Ct.pt)},
{"QDAMP",  "V<=1B15UH",   QBAR,      layer,    ofl(CELLTYPE, Dc.qdamp)},
{"QPULL",  "V<=1B15UH",   ChSS,      conntype, ofl(CONNTYPE, Cn.qpull)},
{"RDAMP",  "V<=1B15UH",   ChSS+RBAR, conntype, ofl(CONNTYPE, Cn.rdamp)},
{"REFRAC", "UH",          DST,       rfrcdata, ofl(RFRCDATA,   refrac)},
{"RETARD", "VIC",         ChSS,      conntype, ofl(CONNTYPE,Cn.retard)},
{"RHO",    "VB24IL",      ChSS+CIJ0, conntype, ofl(CONNTYPE,   Cn.rho)},
{"RULE",   "-",           ChSS+ChKEY,kamcode                          },
{"SCALE",  "S",           ChSS+ChScl,conntype, ofl(CONNTYPE,   Cn.scl)},
{"SDAMP",  "V<=1B15UH",   SBAR,      layer,    ofl(CELLTYPE, Dc.sdamp)},
{"SHAPE",  "-",           ChALR,     sishape                          },
{"SIET",   "B20/27IJ$mV", 0,         layer,    ofl(CELLTYPE,  Dc.siet)},
{"SIGMA",  "VB24/31IJ$mV",0,         layer,    ofl(CELLTYPE,   No.nsg)},
{"SIHA",   "B20/27IJ$mV", 0,         layer,    ofl(CELLTYPE,  Ct.siha)},
{"SISCALE","B24IJ",       0,         layer,    ofl(CELLTYPE, Dc.siscl)},
{"SJMIN",  "X",     ChThr+ChSS,      conntype, ofl(CONNTYPE,    Cn.et)},
{"SJREV",  "X",     ChThr+ChSS,      conntype, ofl(CONNTYPE, Cn.sjrev)},
{"SLOPE",  "VB>" qqv(FBsc) "IJ",0,   layer,    ofl(CELLTYPE,Ct.sslope)},
{"SPIKE",  "VB20/27IJ$mV",0,         layer,    ofl(CELLTYPE,Ct.sspike)},
{"ST",     "B20/27IJ$mV", 0,         layer,    ofl(CELLTYPE,    Ct.st)},
{"STATS",   "-",          ChKEY,     kstacode,                        },
{"SUSAGE", "N" KWS_PSPOV, ChSS+ChKEY,conntype, ofl(CONNTYPE,   spspov)},
{"SVITMS", "-",           ChSS+ChKEY,lsvitms                          },
{"THAMP",  "VB20/27IJ$mV",0,         layer,    ofl(CELLTYPE, Ct.stanh)},
{"UPSD",   "VB27/20IL",   DEPR,      layer,    ofl(CELLTYPE,  Dp.upsd)},
{"UPSDST", "VB16IJ",      DST,       rfrcdata, ofl(RFRCDATA,   upsdst)},
{"UPSM",   "VB16IJ",      ChSS+MIJ,  conntype, ofl(CONNTYPE,  Cn.upsm)},
{"UPSPSI", "B22IJ",       0,         layer,    ofl(CELLTYPE,Dc.upspsi)},
{"VDHA",  "V~B20/27IJ$mV",ChSS,      conntype, ofl(CONNTYPE,  Cn.vdha)},
{"VDMM","B" qqv(FBsc) "IJ",ChSS,     conntype, ofl(CONNTYPE,  Cn.vdmm)},
{"VDOPT",  "-",           ChSS+ChKEY,vdopcode                         },
{"VDT",    "B20/27IJ$mV", ChSS,      conntype, ofl(CONNTYPE,   Cn.vdt)},
{"VI",     "UH",          ChSS,      conntype, ofl(CONNTYPE,    Cn.vi)},
{"VXO",    "F",           ChRPL,     rep,      ofl(REPBLOCK,   Rp.vxo)},
{"VYO",    "F",           ChRPL,     rep,      ofl(REPBLOCK,   Rp.vyo)},
{"W",      "V>F",         ChRPL,     rep,      ofl(REPBLOCK,     aplw)},
{"WAY",    "-",           ChSS,      kamway                           },
{"X",      "F",           ChRPL,     rep,      ofl(REPBLOCK,     aplx)},
{"XGAP",   "VF",          ChRPL,     rep,      ofl(REPBLOCK,  Rp.xgap)},
{"XGRID",  "VIH",         ChRPL,     rep,      ofl(REPBLOCK,Rp.ngridx)},
{"Y",      "F",           ChRPL,     rep,      ofl(REPBLOCK,     aply)},
{"YGAP",   "VF",          ChRPL,     rep,      ofl(REPBLOCK,  Rp.ygap)},
{"YGRID",  "VIH",         ChRPL,     rep,      ofl(REPBLOCK,Rp.ngridy)},
{"ZETAM",  "VB16IJ",      ChSS+MIJ,  conntype, ofl(CONNTYPE, Cn.zetam)},
{"ZETAPSI","V<1B15UH",    0,         layer,   ofl(CELLTYPE,Dc.zetapsi)},
};

#define  CHOP_TBLSIZ (sizeof(chop_tbl) / sizeof(struct chopte))

   struct REPBLOCK   *ir;
   struct CELLTYPE   *il;
   struct CONNTYPE   *ix;
   struct INHIBBLK   *ib;
   struct MODBY      *im;
   struct chopte  *pchop;     /* Ptr to table entry for input item */
   union {
      float nf;
      long nl;
      short ns;
      char nc;
      struct DECAYDEF nd;
      } newval;               /* New parameter value */
   int allo_flag;             /* Index of repertoire data item
                              *  that must be allocated */
   int i,ndx;
   rlnm rlname;               /* Temps for rep and layer names */

/* First time through, construct keylist array for smatch() calls */

   if (!keylist) {
      keylist = (char **)mallocv(CHOP_TBLSIZ*sizeof(char *),
         "CHANGE key list");
      for (i=0; i<CHOP_TBLSIZ; i++)
         keylist[i] = chop_tbl[i].key;
      }

/* Scan and interpret current CHANGE card */

   cdscan(RK.last, 1, SCANLEN, RK_WDSKIP);
   if (setjmp(eof_jmp)) return;

   for (;;) {

      /* Set up the jump buffer for the error exit routine.  setjmp()
      *  returns 0 on initial call, 1 when branched to by longjmp(,0)
      *  (Hence the loop--after a longjmp, reinit the buffer for the
      *  next time.) */
      while (setjmp(err_jmp)) ;

      /* Read the name of the value to change--this must come after
      *  the error setjmp(), which must be inside the loop--scanck()
      *  is not called because we want to use our own error call.  */
      if (scan(fldbuf, 0) & RK_ENDCARD) return;
      if (RK.scancode != RK_LFTPAREN) fld_err(RK_PUNCERR);

      /* If the name has an index number appended, call wbcdin to get
      *  that index and store it in ndx, otherwise set ndx to -1 to
      *  indicate that all instances of the conntype, inhib block, etc.
      *  are to be affected.  Truncate the name to remove the index. */
      i = RK.length;
      while (isdigit(fldbuf[i])) --i;
      if (i < RK.length) {
         ++i;
         wbcdin(fldbuf+i, &ndx, RK_IORF|RK_NINT|(RK.length-i));
         if (ndx < 0) fld_err(RK_NUMBERR);
         fldbuf[i] = '\0';
         }
      else
         ndx = -1;

      /* Search for table entry corresponding to this change request */
      i = smatch(RK_NMERR, fldbuf, keylist, CHOP_TBLSIZ);

      /* If no match found in table, generate error */
      if (!i) fld_err(RK_IDERR);

      /* Got a match.  Make pointer to table entry.  If this kind
      *  of entry does not permit indexing, and an index was found,
      *  generate an error.  An index of 0 is allowed only for
      *  modtype, where it indicates noise modulation. */
      pchop = chop_tbl + i - 1;
      if (pchop->flags & ChSS) {
         if (pchop->type != modtype && ndx == 0)
            chng_exit(exsub, RK_MARKFLD);
         }
      else if (ndx >= 0)
         chng_exit(exsub, RK_MARKFLD);

      /* Read repertoire name, and layer name if appropriate */
      setrlnm(&rlname, NULL, NULL);
      if (pchop->flags & ChRPL) {
         if (scan(fldbuf, RK_REQFLD) & RK_ENDCARD) return;
         if (RK.length >= LSNAME) fld_err(RK_LENGERR);
         strncpy(rlname.rnm, fldbuf, LSNAME);
         }
      else {
         d3rlnm(&rlname);
         }

      /* If punctuation isn't ')=', error */
      if (RK.scancode != (RK_INPARENS | RK_RTPAREN | RK_EQUALS))
         fld_err(RK_PUNCERR);

      /* Find the appropriate repertoire and layer */
      for (ir = RP->tree; ir; ir = ir->pnrp)
         if (!strncmp(rlname.rnm, ir->sname, LSNAME)) break;
      if (!ir) chng_exit(erep, RK_MARKFLD);

      if (!(pchop->flags & ChRPL)) {
         for (il = ir->play1; il; il = il->play)
            if (!strncmp(rlname.lnm, il->lname, LSNAME)) break;
         if (!(il || pchop->flags & ChALR))
            chng_exit(elay, RK_MARKFLD);
         }

      /* Extract allocation test flag from chop table */
      allo_flag = pchop->flags & ChALLO;

      /* Get new value for variable if it is a decay variable, a key
      *  code value, or any other variable with a nonzero length in
      *  the table.  Store in newval union for possible storage in
      *  multiple tree variables below.  Revised V8C to use eqscan
      *  for units, compat scales.  If variable is ChThr type, scan
      *  so number of scanagn()s below is correct, but do not convert.
      */
      if (pchop->flags & ChDCV) {      /* Get decay parms */
         memset((char *)&newval.nd, 0, sizeof(struct DECAYDEF));
         newval.nd.dcyopt = DCY_IOVAS; /* Do not add an iovec */
         getdecay(&newval.nd);
         RK.scancode &= ~(RK_INPARENS|RK_RTPAREN);
         }
      else if (pchop->flags & ChKEY) { /* Get new option codes */
         int oldlen = scanlen(MXKEYLEN);
         scan(fldbuf, RK_REQFLD|RK_PMEQPM);
         scanlen(oldlen);
         }
      else if (pchop->flags & (ChThr|ChScl)) /* Waste thresholds */
         scan(fldbuf, RK_REQFLD|RK_PMEQPM);
      else if (pchop->len > 0)         /* All others */
         eqscan(&newval, pchop->esfc, 0);
      if (RK.scancode & RK_ENDCARD) return;
      if (RK.scancode & ~RK_COMMA) fld_err(RK_PUNCERR);

/*---------------------------------------------------------------------*
*          Switch on the change request type & do the change           *
*---------------------------------------------------------------------*/

      switch (pchop->type) {

case rep:                  /* Store repertoire variable */
         memcpy((char *)ir + pchop->offs,(char *)&newval, pchop->len);
         break;

case layer:                /* Store layer variable */
         /* Generate error if variable requires dynamically allocated
         *  data that have not actually been allocated */
         if (allo_flag && il->ctoff[allo_flag] < DVREQ)
            chng_exit(eoptstor, RK_MARKFLD);
         memcpy((char *)il+pchop->offs, (char *)&newval, pchop->len);
         break;

case lsvitms:              /* Layer SIMDATA save items */
         /* N.B.  Per longstanding custom, this code allows  svitms
         *  requests to be made freely.  If a request is made for
         *  a variable that does not exist, code in d3gfsh() will
         *  simply turn off the offending bit.  */
         mcodes(fldbuf, oksimct, NULL);
         if (ndx < 0) {
            /* No conntype index entered--operate on celltype.
            *  d3gfsh() will propagate bits to all conntypes.  */
            mcodopl(&il->svitms);
            }
         else {
            for (ix=il->pct1; ix; ix=ix->pct) {
               /* Store conntype svitms.  When ndx has decremented
               *  to 0, we've reached the desired conntype. */
               if (--ndx == 0) {
                  mcodopl(&ix->svitms);
                  break;
                  }
               } /* End conntype loop */
            /* If stepped through conntype list w/o dropping
            *  index to 0, error */
            if (ndx > 0 && !ix) chng_exit(esubsc, RK_MARKFLD);
            } /* End storing conntype-level svitms */
         break;

case autscl:               /* AUTOSCALE parameters */
         if (!il->pauts)
            chng_exit(eoptstor, RK_MARKFLD);
         memcpy((char *)il->pauts + pchop->offs,
            (char *)&newval, pchop->len);
         break;

case bregrf:               /* Brette-Gerstner neuron parameters */
         if (il->Ct.rspmeth != RF_BREG)
            chng_exit(eoptstor, RK_MARKFLD);
         memcpy((char *)il->prfi + pchop->offs,
            (char *)&newval, pchop->len);
         break;

case iz03rf:               /* Izhikevich (2003) neuron parameters */
         if (il->Ct.rspmeth != RF_IZH3)
            chng_exit(eoptstor, RK_MARKFLD);
         memcpy((char *)il->prfi + pchop->offs,
            (char *)&newval, pchop->len);
         break;

case iz07rf:               /* Izhikevich (2007) neuron parameters */
         if (il->Ct.rspmeth != RF_IZH7)
            chng_exit(eoptstor, RK_MARKFLD);
         memcpy((char *)il->prfi + pchop->offs,
            (char *)&newval, pchop->len);
         break;

case phsdata:              /* Phase parameters */
         {  struct PHASEDEF *ppd = il->pctpd;
            /* Error if PHASEDEF structure is not there */
            if (!ppd) chng_exit(eoptstor, RK_MARKFLD);
            memcpy((char *)ppd + pchop->offs,
               (char *)&newval, pchop->len);
            } /* End phsdata local scope */
         break;

case rfrcdata:             /* Refractory period parameters */
         {  struct RFRCDATA *prf = il->prfrc;
            /* Error if RFRCDATA structure is not there */
            if (!prf) chng_exit(eoptstor, RK_MARKFLD);
            memcpy((char *)prf + pchop->offs,
               (char *)&newval, pchop->len);
            } /* End rfrcdata local scope */
         break;

case sishape:              /* Celltype plotting shape */
         {  struct CELLTYPE *jl;    /* Celltype loop ptr */
            getshape(&RP0->GCtD);   /* Use GCtD area as a temp */
            for (jl = ir->play1; jl; jl = jl->play) {
               if (il == NULL || jl == il) {
                  jl->Ct.ksiple = RP0->GCtD.ksiple;
                  jl->Ct.ksbarn = RP0->GCtD.ksbarn;
                  memcpy(jl->Ct.kksbar, RP0->GCtD.kksbar, MxKSBARS);
                  }
               } /* End celltype loop */
            } /* End sishape local scope */
         break;

case sicolor:              /* Celltype color selection */
         {  struct CELLTYPE *jl;    /* Celltype loop ptr */
            getsicol(&RP0->GCtD);   /* Use GCtD area as a temp */
            for (jl = ir->play1; jl; jl = jl->play) {
               if (il == NULL || jl == il) {
                  memcpy(jl->Ct.sicolor, RP0->GCtD.sicolor,
                     NEXIN*COLMAXCH);
                  }
               } /* End celltype loop */
            } /* End sicolsrc local scope */
         break;

case sicolsrc:             /* Celltype plot color source */
         {  struct CELLTYPE *jl;    /* Celltype loop ptr */
            RP0->GCtD.ksipce = 0;   /* Flag nothing entered */
            getscsrc(&RP0->GCtD);   /* Use GCtD area as a temp */
            for (jl = ir->play1; jl; jl = jl->play) {
               if (il == NULL || jl == il) {
                  if (RP0->GCtD.ksipce != 0)
                     jl->Ct.ksipce = RP0->GCtD.ksipce;
                  }
               } /* End celltype loop */
            } /* End sicolsrc local scope */
         break;

case conntype:             /* Connection type parameter */
case ppfdata:              /* Paired-pulse facilitation parm */
         /* Store specific conntype variable according to index.
         *  N.B.  If value requires dynamically allocated data,
         *  and no CONNTYPE index is entered, the following code
         *  could be changed to store the value just for those
         *  CONNTYPEs that have the item, but this would probably
         *  just be an invitation to error.  */
         for (ix=il->pct1; ix; ix=ix->pct) {
            /* If variable relates to PPF, store in PPFDATA */
            char *pvar = (pchop->type == ppfdata) ?
               (char *)(ix->PP) : (char *)ix;
            /* Store new value if conntype index == -1 (store
            *  in all conntypes) or == 0 (ndx has decremented
            *  to 0--we've reached desired conntype) */
            if (--ndx > 0) continue;
            /* Generate error if value requires dynamically allo-
            *  cated data that have not actually been allocated */
            if (allo_flag && ix->cnoff[allo_flag] < DVREQ)
               chng_exit(eoptstor, RK_MARKFLD);
            /* Handle DECAY parameter update.  Generate an error
            *  if EPSP decay requested and input has phase.  */
            if (pchop->flags & ChDCV) {
               if (ix->phopt & (PHASJ|PHASR|PHASC|PHASU))
                  errmsg(eadecay);
               update_decay(&ix->Cn.ADCY, &newval.nd);
               }
            /* Threshold or scale type */
            else if (pchop->flags & (ChThr|ChScl)) {
               RP0->ksrctyp = ix->cnsrctyp;
               scanagn();
               getthr(pchop->esfc, pvar + pchop->offs);
               /* Handle old-style scale normalizing */
               if (pchop->flags & ChScl && RP->compat & OLDSCALE) {
                  double newscale = (double)newval.nl *
                     sqrt(32.0/(double)ix->nc);
                  if (newscale >= 2147483648.0)
                     chng_exit(esclovfl, RK_MARKFLD);
                  ix->Cn.scl = (long)newscale;
                  } /* End handling old scale normalizing */
               }
            else
               memcpy(pvar + pchop->offs, (char *)&newval, pchop->len);

            /* If we're not storing in all conntypes, check
            *  whether we've decremented index to 0 (done) */
            if (!ndx) break;
            } /* End conntype loop */
         /* If stepped through conntype list w/o dropping
         *  index to 0, error */
         if (ndx > 0 && !ix) chng_exit(esubsc, RK_MARKFLD);
         break;

case krpcode:              /* Repertoire krp flags */
         /* Modify codes and store back in repblock */
         mcodes(fldbuf, okrpkrp, NULL);
         mcodoph(&ir->Rp.krp);
         /* Code that was formerly located here to modify celltype
         *  kctp flags from krp flags deleted, 12/27/07.  There are
         *  no longer any krp codes duplicated in kctp.  */
         break;

case kctpcode:             /* Celltype kctp flags */
         /* All the options relating to plotting can now be freely
         *  changed at Group III time.  The 'X' and 'U' options
         *  allowed at Group II time for compatibility will now be
         *  treated as errors here--user should CHANGE STATS.  */
         {  struct CELLTYPE *jl;    /* Celltype loop ptr */
            char chkeys[BITSPERSHORT];
            strncpy(chkeys, okctkctp, BITSPERSHORT);
            chkeys[0] = chkeys[1] = ' ';  /* Kill 'U', 'X' keys */
            mcodes(fldbuf, chkeys, NULL);
            for (jl = ir->play1; jl; jl = jl->play) {
               if (il == NULL || jl == il) {
                  mcodoph(&jl->Ct.kctp);
                  } /* End if jl compatible with il */
               } /* End celltype loop */
            } /* End kctpcode local scope */
         break;

case kstacode:             /* Celltype ksta flags */
         /* Modify kstat flags for named celltype, or for all cell
         *  types of this repertoire if none or '*' entered, in the
         *  manner specified by the user as recorded in RK.mckpm.
         *  Keep all statistics options disabled if RP_NOSTAT set.
         *  Make sure that if any of C,G,H,M,X,Y,Z flags is set,
         *  suitable bits defining a superset of the needed storage
         *  were set when the cell type was originally defined.
         *  (Checked by grep on kxr and kctp (now kstat), 8/31/10.)
         *  Space relating to C,G,H,M stats is allocated on each d3nset
         *  call as if it could change, but at present some key counts
         *  are taken as unchanging from d3mchk,d3allo:  lmm, CTFXD.
         *  If kstat change is allowed here, these must all be
         *  recomputed in d3nset.  */
         {  struct CELLTYPE *jl;    /* Celltype loop ptr */
            /* Store new option codes */
            mcodes(fldbuf, okctsta, NULL);
            for (jl = ir->play1; jl; jl = jl->play) {
               if (il == NULL || jl == il) {
                  mcodoph(&jl->Ct.kstat);
                  if (RP->CP.runflags & RP_NOSTAT)
                     jl->Ct.kstat = KSTNS;
                  else {
#define NKCTPT 4     /* Number of tests */
                     static const ui32 kstatb[NKCTPT] = {
                        KSTXRM,                    /* pxxxx,pnr */
                        KSTXR|KSTYM|KSTZC,         /* XRM alloc */
                        KSTXR|KSTGP|KSTMG|KSTCL,   /* GRPNOS */
                        KSTYM|KSTZC };             /* xresp mtx */
                     int itest;
                     for (itest=0; itest<NKCTPT; ++itest) {
                        if (jl->Ct.kstat & kstatb[itest] &&
                           !(jl->urkstat & kstatb[itest]))
                           errmsg(exmh);
                        }
                     } /* End else not RP_NOSTAT */
                  } /* End if jl compatible with il */
               } /* End celltype loop */
            } /* End kstacode local scope */
         break;

case kamcode:              /* Amplification options */

/* Mask of amp rule bits that are never allowed to change */
#define AMPNOCHG  (KAMMF)

         /* Consistency checks are enforced as follows:
         *  1) Bits in AMPNOCHG set are not allowed to change.
         *  2) More than one amplification code is not allowed.
         *  3) 'L,P,Q,R,S,T,U,Z' bits permitted only if PSI,SBAR,RBAR,
         *     QBAR,MIJ already present as needed.  */
         {  ui32 newkam,tkam;   /* Codes */
            mcodes(fldbuf, okcnkam, NULL);
            /* Load conntypes with kam values */
            for (ix=il->pct1; ix; ix=ix->pct) {
               /* Store new value if conntype index == -1 (store
               *  in all conntypes) or == 0 (ndx has decremented
               *  to 0 and we've reached desired conntype) */
               if (--ndx > 0) continue;
               /* Change only those options which may be changed */
               tkam = ix->Cn.kam;
               mcodopl(&tkam);
               newkam = (ix->Cn.kam & AMPNOCHG) | (tkam & ~AMPNOCHG);
               /* Check for conflicting (supernumerary) options */
               ckkam(newkam);
               /* Make sure that if L was selected, PSI exists */
               if (newkam & KAMLP && !ctexists(il, PSI))
                  chng_exit(eampopt, RK_MARKFLD);
               /* Make sure that if B|P|Q was selected, QBAR exists */
               if (newkam & (KAMBCM|KAMDQ|KAMPA) && !ctexists(il, QBAR))
                  chng_exit(eampopt, RK_MARKFLD);
               /* Make sure that if R was selected, RBAR exists */
               if ((newkam & KAMDR) && !cnexists(ix, RBAR))
                  chng_exit(eampopt, RK_MARKFLD);
               /* Make sure that if S, Z, or T-not-U, SBAR exists */
               if ((newkam & (KAMSA|KAMZS|KAMTNU)) &&
                  !ctexists(il, SBAR)) chng_exit(eampopt, RK_MARKFLD);
               /* Make sure that if S,T, or U selected, Mij exists */
               if ((newkam & (KAMSA|KAMTS|KAMUE)) &&
                  !cnexists(ix, MIJ)) chng_exit(eampopt, RK_MARKFLD);
               /* Make sure that if C selected, it was already there */
               if (newkam & ~ix->Cn.kam & KAMCG)
                  chng_exit(eampopt, RK_MARKFLD);
               /* Store back the new options */
               ix->Cn.kam = newkam;
               /* If we're not storing in all conntypes, check
               *  if we've decremented index to 0 (done) */
               if (!ndx) break;
               }
            /* If stepped through conntype list w/o dropping
            *  index to 0, generate error */
            if (ndx > 0 && !ix) chng_exit(esubsc, RK_MARKFLD);

            } /* End kamcode local scope */
         break;

case kamway:               /* Read amplification scales */
            /* Way scales are read into RP0->LCnD as a temp so they
            *  can be copied to one or more conntypes.  */
         {  getway(&RP0->LCnD);     /* Read in the scales */
            for (ix=il->pct1; ix; ix=ix->pct) {
               /* Store new value if conntype index == -1 (store
               *  in all conntypes) or == 0 (ndx has decremented
               *  to 0 and we've reached desired conntype) */
               if (--ndx > 0) continue;
               memcpy((char *)ix->Cn.wayset,(char *)RP0->LCnD.wayset,
                  sizeof(RP0->LCnD.wayset));

               /* If we're not storing in all conntypes, check
               *  if we've decremented index to 0 (done) */
               if (!ndx) break;
               }
            /* If stepped through conntype list w/o dropping
            *  index to 0, generate error */
            if (ndx > 0 && !ix) chng_exit(esubsc, RK_MARKFLD);
            } /* End kamway local scope */
         break;

case cnopcode:             /* Conntype options */

/* Mask of cnopt bits that are never allowed to change */
#define CNONOCHG  (NOPPHBITS|NOPOR|NOPVD|NOPNI|NOPVP)

         /* Consistency checks are enforced as follows:
         *  1) Bits in CNONOCHG set are not allowed to change.
         *  2) 'I' bit is permitted only if RBAR is allocated.  */
         {  ui16 newcno,tcno;    /* Codes */
            mcodes(fldbuf, okcnopt, NULL);
            /* Load conntypes with new cnopt values */
            for (ix=il->pct1; ix; ix=ix->pct) {
               /* Store new value if conntype index == -1 (store
               *  in all conntypes) or == 0 (ndx has decremented
               *  to 0 and we've reached desired conntype) */
               if (--ndx > 0) continue;
               /* Change only those options which may be changed */
               tcno = ix->Cn.cnopt;
               mcodoph(&tcno);
               newcno = (ix->Cn.cnopt & CNONOCHG) | (tcno & ~CNONOCHG);
               /* Make sure that if I was selected, RBAR exists */
               if ((newcno & NOPSJR) && !cnexists(ix, RBAR))
                  chng_exit(eoptstor, RK_MARKFLD);
               /* Store back the new options */
               ix->Cn.cnopt = newcno;
               /* If we're not storing in all conntypes, check
               *  if we've decremented index to 0 (done) */
               if (!ndx) break;
               }
            /* If stepped through conntype list w/o dropping
            *  index to 0, generate error */
            if (ndx > 0 && !ix) chng_exit(esubsc, RK_MARKFLD);

            } /* End cnopcode local scope */
         break;

case vdopcode:             /* Voltage-dependent connection options */
         mcodes(fldbuf, okvdopt, NULL);
         for (ix=il->pct1; ix; ix=ix->pct) {
            if (--ndx > 0) continue;
            mcodopc(&ix->Cn.vdopt);
            if (!ndx) break;
            }
         if (ndx > 0 && !ix) chng_exit(esubsc, RK_MARKFLD);
         break;

case modtype:              /* Modulatory connection parameter */
         /* If ndx is initially 0, the NMOD value (first MODBY block
         *  if it exists) should be changed, otherwise ordinary
         *  modulation.  Store new value if ndx < 0 (either initially
         *  -1 to store in all modtypes or has decremented to -1 and
         *  we've reached desired modtype) */
         if (il->nmct <= 0) chng_exit(esubsc, RK_MARKFLD);
         if (il->pmby1->mct == 0) ndx += 1;
         else if (ndx == 0) chng_exit(esubsc, RK_MARKFLD);
         for (im=il->pmby1; im; im=im->pmby) {
            short *pmvar;
            if (--ndx > 0) continue;
            pmvar = (short *)((char *)im + pchop->offs);
            if (pchop->flags & ChDCV)
               update_decay(&im->MDCY, &newval.nd);
            else if (pchop->flags & ChThr) {
               RP0->ksrctyp = im->msrctyp;
               scanagn();
               getthr(pchop->esfc, pmvar);
               }
            else memcpy((char *)pmvar, (char *)&newval, pchop->len);
            /* The following code is technically wrong--the user
            *  may have wanted to change mt on only one MODULATE
            *  card, but this will change all coupled MODVALs by
            *  copying Mdc from the MODCOM block just changed.
            *  The user can avoid this problem by coding MOPT=U,
            *  which will couple only one MODVAL to this MODBY.  */
            if (pchop->flags & ChMT) im->pmvb->Mdc = im->Mc;
            if (!ndx) break;
            }
         /* If stepped through modby list w/o dropping
         *  index to 0, error */
         if (ndx > 0 && !im) chng_exit(esubsc, RK_MARKFLD);
         break;

case gcontype:             /* Geometric connection parameter */
            /* Store new value if conntype index == -1 (store in all
            *  conntypes) or == 0 (ndx has decremented to 0--we've
            *  reached desired conntype) */
            for (ib=il->pib1; ib; ib=ib->pib) {
               short *pgvar;
               if (--ndx > 0) continue;
               pgvar = (short *)((char *)ib + pchop->offs);
               if (pchop->flags & ChDCV)
                  update_decay(&ib->GDCY, &newval.nd);
               else if (pchop->flags & ChThr) {
                  RP0->ksrctyp = REPSRC;
                  scanagn();
                  getthr(pchop->esfc, pgvar);
                  }
               else
                  memcpy((char *)pgvar, (char *)&newval, pchop->len);
               if (!ndx) break;
               }
            /* Error if stepped through inhib block list
            *  without dropping index to 0 */
            if (ndx > 0 && !ib) chng_exit(esubsc, RK_MARKFLD);
         break;

case getbeta:              /* Read gconn beta coefficients */
case getmxib:              /* Read gconn maximum contributions */

         {  struct INHIBBLK  *ib = find_inhibblk(il, ndx);
            struct INHIBBAND *pbnd = ib->inhbnd;
            int inib  = (int)ib->nib;
            int structsize = sizeof(struct INHIBBAND);
            if (pchop->type == getbeta)
               inform("(SV(^RB20IJV))", &structsize, &inib,
                  &pbnd->beta, NULL);
            else
               inform("(SV(^RB20/27IJ$mV,V))", &structsize, &inib,
                  &pbnd->mxib, NULL);
            } /* End getbeta/getmxib local scope */
         break;

         } /* End switch on value type */

      } /* End option read loop */

   } /* End d3chng */
