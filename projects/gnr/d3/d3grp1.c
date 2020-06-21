/* (c) Copyright 1990-2016, The Rockefeller University *21116* */
/* $Id: d3grp1.c 78 2018-08-02 18:36:58Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                              d3grp1.c                                *
*              Read and interpret Group I control cards                *
*                                                                      *
************************************************************************
*  Rev, 02/23/90, GNR - Accept & ignore "RESPONSE FUNCTION PARALLEL"   *
*  Rev, 03/28/90, JWT - Add max object ref number and plot letter      *
*                          height values to PLIM card                  *
*  Rev, 01/14/91,  MC - Added PHASE code                               *
*  V5A, 04/19/91, GNR - Remove ljnwd allocation for value blocks, add  *
*                          PLATFORM card, fix GRAFDATA cell list bug   *
*  V5B, 08/09/91, GNR - Add SNOUT, add Z,R codes to PLATFORM card      *
*  V5C, 10/27/91, GNR - Add code for D1ARRAY card, PLIM DIAMETER,      *
*                          remove FDMATRIX and RESTORE cards           *
*  Rev, 12/05/91, GNR - Add vbase to VALUE card                        *
*  Rev, 01/18/92, GNR - Add D1BARS to COLORS card                      *
*  Rev, 02/14/92, GNR - Add D1ENV and D1CAT value schemes              *
*  Rev, 02/18/92, GNR - Add CAMEL card                                 *
*  Rev, 03/22/92, GNR - Move CIJPLOT,MIJPLOT cards from d3grp3         *
*  Rev, 03/26/92, GNR - Add AVGSI value scheme                         *
*  Rev, 04/13/92, ABP - Add VDRD and VDRE cards                        *
*  V5E, 06/12/92, GNR - Add D1 surround and spread parameters          *
*  Rev, 08/08/92, GNR - Add GRAPH items=B (baseline Cij) to SAVE card  *
*  Rev, 08/16/92, GNR - Add COMPAT option to RESPONSE FUNCTION card    *
*  Rev, 01/19/93, GNR - Add OBJID, OBJXY, OBJALL options to SAVE card  *
*  V6A, 03/29/93, GNR - Add NOSS card, COLORS STIM=RGB                 *
*  Rev, 05/20/93, GNR - Add PERSLIM to RESP FN, move STEP to runflags  *
*  Rev, 07/07/93, GNR - Add SENSE card                                 *
*  V6D, 02/02/94, GNR - Remove ONLINE, add SAVE delays, RESP FN SPIKE  *
*  V7A, 04/29/94, GNR - Add user-defined senses to SENSE card          *
*  V7B, 08/03/94, GNR - Add REGENERATION card                          *
*  V7C, 11/14/94, GNR - Add NCS,NDS on INPUT, PLIM (w/ NGR for compat) *
*  V8A, 04/15/95, GNR - Move ARM, JOINT, GRIP, WINDOW cards to d3ajw,  *
*                       handle SENSE VJ,VT,VW, add MODALITY, OPTIMIZE  *
*                       DEFER-ALLOC, RESP FN DECAY, all PAF,PCF values *
*                       must be positive, delete D1ENV & COMPAT=H,P,N, *
*                       add COMPAT=R, VOP=A,F                          *
*  Rev, 11/27/96, GNR - Remove support for non-hybrid version          *
*  Rev, 01/31/97, GNR - Complete PAF,PCF processing in d3g1g3 file     *
*  Rev, 05/03/97,  LC - Obj and Val store as lists, not bit array      *
*  Rev, 08/25/97, GNR - Implement V3 GRAFDATA, ignore whitespace cards *
*  Rev, 12/30/97, GNR - Remove DEFER-ALLOC, add RESERVE                *
*  V8B, 12/09/00, GNR - New memory management routines, trial timers   *
*  Rev, 02/10/01, GNR - Allow SIMDATA items to change at Grp III time  *
*  Rev, 05/20/01, GNR - Remove ARBOR options on CIJPLOT/MIJPLOT        *
*  Rev, 07/14/01, GNR - Add numbered CLIST card, use text cache        *
*  Rev, 08/27/01, GNR - CIJPLOT, MIJPLOT, PLAB, PLINE, PLOC to d1g1g3  *
*  V8C, 02/22/03, GNR - Cell responses in millivolts, COMPAT=C         *
*  V8D, 01/27/04, GNR - Add conductances and ions                      *
*  Rev, 01/27/07, GNR - Look to next group if unrecognized card        *
*  Rev, 10/02/07, GNR - Add PSCALE, TANH response function             *
*  ==>, 01/06/08, GNR - Last mod before committing to svn repository   *
*  Rev, 04/01/08, GNR - Cards that don't ref s(i) OK before COMPAT=C   *
*  Rev, 06/16/08, GNR - Add CSELPLOT                                   *
*  Rev, 08/15/08, GNR - Add EFFECTOR ECELLS, ICELLS                    *
*  V8E, 01/14/09, GNR - Add RESPONSE FUNCTION IZHIKEVICH               *
*  Rev, 11/05/09, GNR - Remove NOSS card, add STATISTICS card          *
*  V8F, 02/13/10, GNR - Add UPGM cameras, remove PLATFORM, VDR support *
*  Rev, 04/13/10, GNR - Add KERNEL & PREPROC cards, CAMERA to g1cam()  *
*  V8H, 11/07/10, GNR - Implement BBD SENSE and CAMERA modalities      *
*  Rev, 12/04/10, GNR - Move EFFECTOR code to d3ajw.c                  *
*  Rev, 04/01/11, GNR - Change RP_IMMAMP to adefer                     *
*  Rev, 10/12/11, GNR - Remove setmeta(), setmfd(), use setmf()        *
*  Rev, 06/07/12, GNR - Add adefer per conntype                        *
*  V8I, 10/07/12, GNR - Remove RESPONSE ADDPERS, PERSLIMIT, DECAY      *
*  R63, 10/28/15, GNR - Make hlfeff default 128 mV for mV scale        *
*  R66, 03/11/16, GNR - Rename nvx->nsxl, nvy->nsya so != ix->nv[xy]   *
*  R67, 04/27/16, GNR - Remove Darwin 1 support                        *
*  R74, 09/03/17, GNR - Added MNBPR to PLIMITS -- see Notes below      *
*  R76, 10/20/17, GNR - Add CONSTANT value, value longs to si32        *
*  R77, 02/17/18, GNR - Remove PLIM DIAMETER                           *
*  R78, 05/12/18, GNR - Rewrite OPTIMIZE w/kwscan, add nhlssv          *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "d3fdef.h"
#include "d3opkeys.h"
#include "clblk.h"
#include "tvdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "bapkg.h"
#include "plots.h"

extern char **getcc2(int *pncc2);

/* Prototypes for a few things called just from here */
void g1arm(void);
void g1cam(void);
#ifdef BBDS
void g1eff(void);
#endif
void g1grip(void);
void g1jnt(void);
void g1kern(void);
void g1prep(void);
void g1ujnt(void);
void g1wdw(void);

/* Persistent data shared with d3ajw routines */
struct AJWsetup {
   struct ARMDEF **pla;       /* Ptr to end of ARM linked list */
   struct WDWDEF **plw;       /* Ptr to end of WDW linked list */
   struct EFFARB **pplea;     /* Ptr to end of EFFARB linked list */
   struct ARMDEF *pca;        /* Ptr to current ARM block */
   struct WDWDEF *pdw;        /* Ptr to most recent driven window */
   struct EFFARB *pcea;       /* Ptr to current arm's first EFFARB */
   int ijnt;                  /* Current joint number */
   } AJW;

#ifdef BBDS
/*---------------------------------------------------------------------*
*                               g1bbdn                                 *
*  This little routine can be called to read a bbd device name (sense, *
*  value, camera, or effector) and separate it into potentially a host *
*  name (possibly with a port) and a device name.                      *
*---------------------------------------------------------------------*/

void g1bbdn(txtid *phost, txtid *pname) {

   int oldlen = scanlen(MAX_FQDN_LENGTH+7);
   char lea[MAX_FQDN_LENGTH+8];     /* Scan field */

   scanck(lea, (RK_NEWKEY|RK_REQFLD|RK_SLDELIM),
      ~(RK_BLANK|RK_COMMA|RK_SLASH|RK_LFTPAREN));
   if (RK.scancode & RK_SLASH) {
      /* Got a slash, field is host */
      *phost = (txtid)savetxt(lea);
      /* Advance to the device name */
      scanck(lea, RK_REQFLD, ~(RK_BLANK|RK_COMMA));
      }
   else
      *phost = 0;                /* Signify default host */
   if (RK.length >= MaxDevNameLen)
      ermark(RK_LENGERR);
   *pname = (txtid)savetxt(lea);
   scanlen(oldlen);
   } /* End g1bbdn() */
#endif

/*---------------------------------------------------------------------*
*                               getcc1                                 *
*  This little routine can be called to get a pointer to the Group 1   *
*  control card keys and the number of keys for use in d3grp0 look-    *
*  ahead.  It seems that just making cc1 global doesn't do the trick.  *
*---------------------------------------------------------------------*/

char **getcc1(int *pncc1) {

/* Keyword strings for and number of Group I control cards */

   static char *cc1[] = {
      "ARM",
      "CIJPLOT",
      "CIJ0PLOT",
      "CLIST",
      "COLORS",
      "CONDUCTANCE",
      "CSELPLOT",
      "CSET",
#ifdef BBDS
      "EFFECTOR",
#endif
      "ESJPLOT",
      "FACTOR",
      "GRIP",
      "INPUT",
      "ION",
      "JOINT",
      "KERNEL",
      "MIJPLOT",
      "MODALITY",
      "OPTIMIZE",
      "PAF",
      "PCF",
      "PLABEL",
      "PLIMITS",
      "PLINE",
      "PLOC",
      "PPFPLOT",
      "PREPROC",
      "PSCALE",
      "REGENERATION",
      "REPLAY",
      "RESPONSE FUNCTION",
      "RJPLOT",
      "SAVE",
      "SENSE",
      "SJPLOT",
      "STATISTICS",
      "STATS",
      "TRACE",
      "CAMERA",
      "TV",          /* Old synonym for "CAMERA" */
      "IMGTOUCH",
      "UNIVERSAL JOINT",
      "VALUE",
      "VERIFY",
      "WINDOW"
      } ;

   *pncc1 = sizeof(cc1)/sizeof(char *);
   return cc1;
   } /* End getcc1() */

/*---------------------------------------------------------------------*
*                               d3grp1                                 *
*---------------------------------------------------------------------*/

int d3grp1(void) {

   struct MODALITY *pmdlt;    /* Pointer to modality info       */
   char **cc1,**cc2;          /* Ptrs to Group 1,2 card keys    */
   ui32 ic;                   /* kwscan field recognition codes */
   int kwret,smret;           /* kwscan,smatch return vals      */
   int ncc1,ncc2;             /* Number of Group 1,2 card keys  */
   int nsnsvj,nsnsvw,nsnsvt;  /* Number of SENSE VJ,VW,VT cards */
   int qcclate;               /* TRUE if COMPAT=C is too late   */

/* Group I control card switch cases--
*  N.B.  The enum values must match the corresponding cc1 entries!  */
   enum G1CC {
      ARMCD = 1,
      CIJPLOT,
      CIJ0PLOT,
      CLIST,
      COLORS,
      CONDUCT,
      CSELPLOT,
      CSETCD,
#ifdef BBDS
      EFFECTOR,
#endif
      ESJPLOT,
      FACTOR,
      GRIP,
      INPUT,
      IONCD,
      JOINTCD,
      KERNCD,
      MIJPLOT,
      MODALTY,
      OPTIMIZE,
      PAF,
      PCF,
      PLTLAB,
      PLIMITS,
      PLTLIN,
      PLOC,
      PPFPLOT,
      PREPROC,
      PSCALE,
      REGENR,
      RPLAY,
      RESPFN,
      RJPLOT,
      SAVE,
      SENSE,
      SJPLOT,
      STATS1,
      STATS2,
      TRACE,
      CAMERA,
      TV,
      IMGTOUCH,
      UJOINT,
      VALUE,
      VERIFY,
      WINDOWCD
      } ccindex;

   struct PCF     **plpcf= &RP->ppcf1;  /* Ptr to next PCF record ptr */
   struct VBDEF   **plvb = &RP->pvblk;       /* Ptr to next value ptr */
   struct VBDEF   **plvbe=&RP0->pvblke;     /* Ptr to next env VB ptr */
   struct VBDEF   **plvbr= &RP->pvblkr;     /* Ptr to next rep VB ptr */
#ifdef BBDS
   struct VCELL   **pplbbds = &RP0->pbbds1;  /* Ptr to next BBD sense */
#endif

   RP0->ccgrp = CCGRP1;       /* Indicate we are in group I */
   AJW.pla = &RP0->parm1;     /* Ptr to next ARM record ptr */
   AJW.plw = &RP0->pwdw1;     /* Ptr to next WDW record ptr */
   AJW.pplea = &RP0->pearb1;  /* Ptr to next EFFARB */
   AJW.pdw = NULL;            /* Ptr to most recent driven window */
   nsnsvj = nsnsvw = nsnsvt = qcclate = 0;

#ifndef PAR
   if (RP->CP.dbgflags & DBG_PPKERN) {
      setmf("KernDebug", "localhost:0", "CNSDBG", NULL,
         0, 0, '8', 0, 0);
      setmovie(1, 0, 0);
      }
#endif

/* Read and interpret a control card */

   cc1 = getcc1(&ncc1);
   cc2 = getcc2(&ncc2);
   while (cryin()) {
      ccindex = (enum G1CC)smatch(RK_NMERR, RK.last, cc1, ncc1);
      if (ccindex <= 0) {
         /* If not in Group 1 or Group 2, give an error and
         *  read next one here, otherwise go to Group 2.  */
         if (smatch(0, RK.last, cc2, ncc2)) break;
         else continue;
         }
      cdprt1(RK.last);        /* Print the card (if oprty <= 3) */
      switch (ccindex) {

/*---------------------------------------------------------------------*
*                              ARM card                                *
*---------------------------------------------------------------------*/

         case ARMCD:
            qcclate = TRUE;
            g1arm();          /* Now handled in d3ajw.c */
            break;

/*---------------------------------------------------------------------*
*                     CIJPLOT and CIJ0PLOT cards                       *
*---------------------------------------------------------------------*/

         case CIJPLOT:
            getijpl(KCIJC);
            break;

         case CIJ0PLOT:
            getijpl(KCIJC0);
            break;

/*---------------------------------------------------------------------*
*                             CLIST card                               *
*---------------------------------------------------------------------*/

         case CLIST: {
            int clnum;
            inform("(SW1V>I)", &clnum, NULL);
            d3clst(clnum, CLKM_MULTI);
            } /* End CLIST local scope */
            break;

/*---------------------------------------------------------------------*
*                             COLORS card                              *
*---------------------------------------------------------------------*/

         case COLORS:
            getcolor();
            /* As a convenience for users, if stimuli are colored,
            *  make input array colored.  This won't work on COLORS
            *  card in d3grp3, because g2conn tests kcolor sooner. */
            if (!strcmp(RP0->ecolstm, "RGB"))
               RP->kcolor = max(RP->kcolor, ENV_COLOR);
            break;

/*---------------------------------------------------------------------*
*                          CONDUCTANCE card                            *
*---------------------------------------------------------------------*/

         case CONDUCT:
            qcclate = TRUE;
            getcond(NULL, NULL);
            break;

/*---------------------------------------------------------------------*
*                            CSELPLOT card                             *
*---------------------------------------------------------------------*/

         case CSELPLOT:
            getijpl(KCIJSEL);
            break;

/*---------------------------------------------------------------------*
*                              CSET card                               *
*---------------------------------------------------------------------*/

         case CSETCD:
            getcset(NULL);
            break;

#ifdef BBDS
/*---------------------------------------------------------------------*
*                            EFFECTOR card                             *
*---------------------------------------------------------------------*/

         case EFFECTOR:
            qcclate = TRUE;
            g1eff();          /* Now handled in d3ajw.c */
            break;
#endif

/*---------------------------------------------------------------------*
*                            ESJPLOT card                              *
*---------------------------------------------------------------------*/

         case ESJPLOT:
            qcclate = TRUE;
            getijpl(KCIJES);
            break;

/*---------------------------------------------------------------------*
*                             FACTOR card                              *
*---------------------------------------------------------------------*/

         case FACTOR:
            inform("(SW1,V>F)", &RP0->pfac, NULL);
            factor(RP0->pfac);
            thatsall();
            break;

/*---------------------------------------------------------------------*
*                              GRIP card                               *
*---------------------------------------------------------------------*/

         case GRIP:
            qcclate = TRUE;
            g1grip();         /* Now handled in d3ajw.c */
            break;

/*---------------------------------------------------------------------*
*                             INPUT card                               *
*---------------------------------------------------------------------*/

/* N.B.  Best efforts to use '#' macro facility to generate %SnC codes
*  for color options fail to produce correct or even consistent results
*  on various compilers.  Accordingly, the values of ENV_MONO etc. are
*  hard coded in the kwscan() call and a preprocessor test is used to
*  flag any changes which would require updating these codes. */
/* R74, 06/25/17, GNR - Add "AR" option */
#if ENV_MONO != 0 || ENV_COLOR != 1 || ENV_ORCOL != 2
#error INPUT card processing must be revised for new ENV color codes
#endif

         case INPUT:
            inform("(SW1=K,2*VI)", &RP->kx, &RP->ky, NULL);
            kwscan(&ic,
               "KX%VI",    &RP->kx,
               "KY%VI",    &RP->ky,
               "KUX%V>UH", &RP0->kux,
               "KUY%V>UH", &RP0->kuy,
               "NCS%VUH",  &RP0->ncs,
               "NDS%VUH",  &RP0->nds,
               "LO%VIC",   &RP0->eilo,
               "HI%VIC",   &RP0->eihi,
               "BW%S0C",   &RP->kcolor,
               "RGB%S1C",  &RP->kcolor,
               "OR%S2C",   &RP->kcolor,
               NULL);
            break;

/*---------------------------------------------------------------------*
*                              ION card                                *
*---------------------------------------------------------------------*/

         case IONCD:
            qcclate = TRUE;
            getion(NULL, NULL);
            break;

/*---------------------------------------------------------------------*
*                             JOINT card                               *
*---------------------------------------------------------------------*/

         case JOINTCD:
            qcclate = TRUE;
            g1jnt();          /* Now handled in d3ajw.c */
            break;

/*---------------------------------------------------------------------*
*                             KERNEL card                              *
*---------------------------------------------------------------------*/

         case KERNCD:
            g1kern();
            break;

/*---------------------------------------------------------------------*
*                            MIJPLOT card                              *
*---------------------------------------------------------------------*/

         case MIJPLOT:
            qcclate = TRUE;
            getijpl(KCIJM);
            break;

/*---------------------------------------------------------------------*
*                            MODALITY card                             *
*                                                                      *
*  Temptation always to set RQST bit when modality is defined by this  *
*  card was resisted because modality may be used solely for ENVVAL.   *
*---------------------------------------------------------------------*/

         case MODALTY: {
            byte kxrstat = MDLTFLGS_RQST;
            char mname[LTEXTF+1];         /* Name text buffer    */
            inform("(SW1A" qLTF ")", mname, NULL);   /* Get name */
            pmdlt = findmdlt(mname, 0);   /* Find or create modality */
            if (!pmdlt) break;            /* Ignore if got too many */
            kwscan(&ic,                   /* Get keyword parameters */
               "NCS%VUH", &pmdlt->ncs,
               "NDS%VUH", &pmdlt->nds,
               "NTCS%IH", &pmdlt->ntcs,   /* Neg. to inactivate */
               "NTRS%IH", &pmdlt->ntrs,   /* Neg. to inactivate */
               "XRSTATS%OC", &pmdlt->mdltflgs, &kxrstat,
               NULL);
            } /* End MODALTY local scope */
            break;

/*---------------------------------------------------------------------*
*                            OPTIMIZE card                             *
*---------------------------------------------------------------------*/

         case OPTIMIZE: {
            static ui32 OptZZ  = RP_OPTZZ;
            static ui32 OptOTD = RP_OPTOTD;
            cdscan(RK.last, 1, SCANLEN, RK_WDSKIP);
            kwscan(&ic,
               "SPEED%OJ~",            &RP->CP.runflags, &OptZZ,
               "STORAGE%OJ",           &RP->CP.runflags, &OptZZ,
               "OMIT-TYPE-DETAIL%OJ",  &RP->CP.runflags, &OptOTD,
               "RESERVE%V>IL",         &RP->ldrsrv,
               "NHLSSV%V>UH",          &RP->nhlssv,
               NULL);
            } /* End OPTIMIZE local scope */
            break;

/*---------------------------------------------------------------------*
*                          PAF and PCF cards                           *
*---------------------------------------------------------------------*/

/* PAF tables and PCF tables are treated the same, except PCF tables
*  default to max norm.  Both kinds of tables are stored in the same
*  linked list, which means that when the user specifies a table on
*  an AMPLIF card or a PHASE card or a PARAMS card it really doesn't
*  matter if it was entered as a PCF table or a PAF table--at present
*  the only significance of this is the aforesaid normalization.  This
*  could be changed to make the two types of tables distinguishable.
*  Input routine changed, V8A, 04/03/96, to require all entries to be
*  positive.  This closes a loophole that could allow excitatory and
*  inhibitory inputs to be mixed.  -GNR */

         case PAF:
         case PCF: {
            struct PCF *pker = *plpcf = (struct PCF *)
               allocpmv(Static, IXstr_PCF, "convolution kernel");
            *(plpcf = &pker->pnxpcf) = NULL;
            inform("(SW1TH" qLTF ")", &pker->hpcfnm, NULL);
            getpcf(pker, (ccindex == PCF) ? PCFN_MAX : PCFN_NONE);
            RP->npcf++;
            }
            break;


/*---------------------------------------------------------------------*
*                             PLABEL card                              *
*---------------------------------------------------------------------*/

         case PLTLAB:
            getplab();        /* Now handled in d3g1g3.c */
            break;

/*---------------------------------------------------------------------*
*                            PLIMITS card                              *
*                                                                      *
*  Note:  Options NCS,NDS were added on INPUT card, replacing NGR.     *
*  For compatibility, the new options were also placed on the PLIMITS  *
*  card, and NGR has been implemented to set both of them.             *
*                                                                      *
*  Note re MNBPR:  Ideally, mfdraw should be able to determine display *
*  resolution and pass to the plot library for setting things like min *
*  plot object sizes, but this is complex and no use for offline plots,*
*  so the old #defined MINBPRAD has been replaced with the mnbpr param.*
*---------------------------------------------------------------------*/

         case PLIMITS: {
            static ui16 kplsqr = PLSQR, kplrtr = PLRTR;
            int   tmxr;
            qcclate = TRUE;
            cdscan(RK.last, 1, SCANLEN, RK_WDSKIP);
            while (kwret = kwscan(&ic, /* Assignment intended */
                  "SQUARE%OH",         &RP0->kplt1, &kplsqr,
                  "STDLHT%X",
                  "SPH%V>F",           &RP->sphgt,
                  "SPHEIGHT%V>F",      &RP->sphgt,
                  "SPHGT%V>F",         &RP->sphgt,
                  "SPWIDTH%V>F",       &RP->spwid,
                  "RETRACE%OH",        &RP0->kplt1, &kplrtr,
                  "MNBPR%VF",          &RP->mnbpr,
                  "MPX%VIH",           &RP0->mpx,
                  "NGR%X",
                  "NCS%VUH",           &RP0->ncs,
                  "NDS%VUH",           &RP0->nds,
                  "NQX%VIH",           &RP0->mpx,
                  "CPR%VF",            &RP0->GCtD.cpr,
                  "CPT%B7/14IH$mV",    &RP0->GCtD.cpt,
                  "VCPT%B14IH",        &RP0->vcpt,
                  "PSER%UJ",           &RP->CP.ser,
                  "EPR%VF",            &RP0->epr,
                  "EPT%VB8I",          &RP0->ept,
                  "GRIDS%VIH",         &RP0->grids,
                  "MXOBJ%VIH",         &RP0->mxobj,
                  "MXREF%VI",    &tmxr,   /* Ignore obsolete mxref */
                  NULL)) {
               switch (kwret) {

                  case 1:  /* STDLHT */
                     /* For historical reasons, this option may have
                     *  no parameter, in which case there's nothing
                     *  to do (use of stdlht is now always enabled) */
                     if (RK.scancode == RK_EQUALS)
                        eqscan(&RP->stdlht, "VF", 0);
                     break;
                  case 2:  /* NGR */
                     eqscan(&RP0->ncs, "VUH", RK_EQCHK);
                     RP0->nds = RP0->ncs;
                     break;

                  }  /* End kwret switch */
               } /* End kwret while */
            }
            break;

/*---------------------------------------------------------------------*
*                             PLINE card                               *
*---------------------------------------------------------------------*/

         case PLTLIN:
            getpline();       /* Now handled in d3g1g3.c */
            break;

/*---------------------------------------------------------------------*
*                              PLOC card                               *
*---------------------------------------------------------------------*/

         case PLOC:
            getploc();        /* Now handled in d3g1g3.c */
            break;

/*---------------------------------------------------------------------*
*                            PPFPLOT card                              *
*---------------------------------------------------------------------*/

         case PPFPLOT:
            qcclate = TRUE;
            getijpl(KCIJPPF);
            break;

/*---------------------------------------------------------------------*
*                            PREPROC card                              *
*---------------------------------------------------------------------*/

         case PREPROC:
            g1prep();
            break;

/*---------------------------------------------------------------------*
*                             PSCALE card                              *
*---------------------------------------------------------------------*/

         case PSCALE:
            gtpscale();       /* Now handled in d3g1g3.c */
            break;

/*---------------------------------------------------------------------*
*                          REGENERATION card                           *
*---------------------------------------------------------------------*/

         case REGENR:
            gtkregen();       /* Now handled in d3g1g3.c */
            break;

/*---------------------------------------------------------------------*
*                             REPLAY card                              *
*---------------------------------------------------------------------*/

         case RPLAY: {
            static const unsigned short kngn = KSVNR;
            RP->CP.runflags |= (RP_REPLAY|RP_NOSTAT);
            cdscan(RK.last, 1, SCANLEN, RK_WDSKIP);
            kwscan(&ic, "NOGENR%OH", &RP->ksv, &kngn, NULL);
            } /* End RPLAY local scope */
            break;

/*---------------------------------------------------------------------*
*                       RESPONSE FUNCTION card                         *
*                                                                      *
*  N.B.  In the IBM version, the PARALLEL keyword forced deterministic *
*  updating of random number seeds for compatibility with the parallel *
*  version.  The C version always does this, and the PARALLEL keyword  *
*  is ignored.  A little run time has been traded for a simpler code.  *
*  N.B.  The DELAY option is just for compatibility.  The timestep     *
*  must be fixed in Group I because it affects allocation of memory    *
*  for conductances and tscale for ALPHA decay.                        *
*---------------------------------------------------------------------*/

         case RESPFN: {
            static char *rf[] = { "KNEE", "STEP", "SPIKE", "TANH",
               "AEIF", "IZHI2003", "IZHI2007", "IZHIKEVICH",
               "IZHI", "ADEFER", "BITS", "DELAY", "IMMEDIATE",
               "CDCYCUT", "TIMESTEP", "SAVECIJ0", "PARALLEL",
               "COMPAT" };
            enum krf { rf_bad=0, rf_knee, rf_step, rf_spike, rf_tanh,
               rf_breg, rf_izh3, rf1_izh7, rf2_izh7, rf3_izh7,
               rf_adefer, rf_bits, rf_delay, rf_immed, rf_cdcy,
               rf_time, rf_svcij, rf_par, rf_compat };

            cdscan(RK.last, 2, SCANLEN, RK_WDSKIP);
            while ((smret = match(OFF, RK_MSCAN, ~(RK_COMMA|RK_EQUALS),
                  0, rf, sizeof rf/sizeof(char *))) != RK_MENDC)
               switch ((enum krf)smret) {
                  case rf_knee:        /* KNEE  */
                     RP0->GCtD.rspmeth = RF_KNEE;     break;
                  case rf_step:        /* STEP  */
                     RP0->GCtD.rspmeth = RF_STEP;     break;
                  case rf_spike:       /* SPIKE */
                     RP0->GCtD.rspmeth = RF_SPIKE;    break;
                  case rf_tanh:        /* TANH */
                     RP0->GCtD.rspmeth = RF_TANH;     break;
                  case rf_breg:        /* AEIF */
                     RP0->GCtD.rspmeth = RF_BREG;     break;
                  case rf_izh3:        /* IZHI2003 */
                     RP0->GCtD.rspmeth = RF_IZH3;     break;
                  case rf1_izh7:       /* IZHI2007 */
                  case rf2_izh7:
                  case rf3_izh7:
                     RP0->GCtD.rspmeth = RF_IZH7;     break;
                  case rf_adefer:      /* ADEFER */
                     eqscan(&RP0->GCnD.adefer, "UH", RK_EQCHK);
                     break;
                  case rf_bits:
                     eqscan(&RP0->GCnD.nbc, "V>1<=16IH", RK_EQCHK);
                     break;
                  case rf_delay:       /* DELAY */
                     RP0->GCnD.adefer = 1;            break;
                  case rf_immed:       /* IMMEDIATE */
                     RP0->GCnD.adefer = 0;             break;
                  case rf_cdcy:        /* CDCYCUT */
                     eqscan(&RP0->cdcycut, "V>0<1.0F", RK_EQCHK);
                     break;
                  case rf_time:        /* TIMESTEP */
                     eqscan(&RP->timestep, "UJ$us", RK_EQCHK);
                     break;
                  case rf_svcij:       /* SAVECIJ0 */
                     RP0->n0flags |= N0F_SVCIJ0;      break;
                  case rf_par:         /* PARALLEL -- ignore */
                     break;
                  case rf_compat: {    /* COMPAT */
                     ui16 compat0 = RP->compat;
                     eqscan(&RP->compat, "KH" okcompat, RK_EQCHK);
                     if (RP->compat & OLDRANGE) {
                        if (qcclate && !(compat0 & OLDRANGE))
                           cryout(RK_E1, "0***COMPAT=C MUST BE "
                              "SPECIFIED ON FIRST GROUP I CARD.",
                              RK_LN2, NULL);
                        /* Use alternative scales */
                        bscompat(RK_BSSLASH);
                        RP->bsdc = Sr2mV;
                        RP0->satdcynum /= dS7;
                        RP0->bssel = RK_BSSLASH;
                        }
                     } /* End COMPAT local scope */
                     break;
                  } /* End smret switch and while loop */
               } /* End RESPONSE FUNCTION local scope */
            break;

/*---------------------------------------------------------------------*
*                             RJPLOT card                              *
*---------------------------------------------------------------------*/

         case RJPLOT:
            qcclate = TRUE;
            getijpl(KCIJR);
            break;

/*---------------------------------------------------------------------*
*                              SAVE card                               *
*---------------------------------------------------------------------*/

         case SAVE:
            getsave();        /* Now handled in d3g1g3.c */
            break;

/*---------------------------------------------------------------------*
*                             SENSE card                               *
*                                                                      *
*  N.B. Values for nsxl,nsya input by user are checked in d3schk for   *
*  compatibility with degrees of freedom available for the requested   *
*  sense.  VCELL blocks for user-defined senses are kept in a separate *
*  linked list (really the tail end of the full VCELL list) so BBDS    *
*  code does not see the built-ins.  Detection of VJ,VW,VT is done     *
*  conservatively in case codes changed.  For these cards, must use    *
*  findvc because VCELL block may have been created already by ARM     *
*  or WINDOW card.  No 'erstring' is needed.                           *
*                                                                      *
*  Synonymous names nsxl = ntl, nsya = nta, vhw = tll are included to  *
*  allow user to pick most appropriate name according to sense type.   *
*---------------------------------------------------------------------*/

         case SENSE: {
            struct VCELL *pvc;
#ifdef BBDS
            struct VCELL *jvc;
            txtid hhname = 0;          /* Host name locator     */
#endif
            int hmname = 0;            /* Modality name locator */
            int kxrstat = FALSE;       /* XRSTATS option flag   */
            char tsname[LSNAME+1];     /* Temp for sense name   */

            qcclate = TRUE;
            inform("(SW1A" qLSN ")", tsname, NULL); /* Get name */
            tsname[LSNAME] = '\0';     /* Be sure is terminated */
            switch (getsnsid(tsname)) {
            case VJ_SRC:
               pvc = findvc(tsname, VJ_SRC, ++nsnsvj, NULL);
               break;
            case VW_SRC:
               pvc = findvc(tsname, VW_SRC, ++nsnsvw, NULL);
               break;
            case VT_SRC:
               pvc = findvc(tsname, VT_SRC, ++nsnsvt, NULL);
               break;
#ifdef BBDS
            case USRSRC:
               /* Must create VCELL block even before checking for
               *  duplicate name so g1bbdn() has someplace to put
               *  the various components of the name.  */
               pvc = (struct VCELL *)allocpcv(
                  Host, 1, sizeof(struct VCELL), "BBD sense block");
               vcdflt(pvc, tsname);
               g1bbdn(&hhname, &pvc->hvname);
               /* Check for illegal duplicate name */
               for (jvc=RP0->pbbds1; jvc; jvc=jvc->pnvc) {
                  if (pvc->hvname == jvc->hvname) {
                     dupmsg("USER-DEFINED SENSE", NULL, pvc->hvname);
                     break;
                     }
                  }
               break;
#endif
            default:
               /* We let illegal names like "TV" come into this case
               *  to give the data some place to go before errmsg.
               *  N.B.  Use allocpmv() for consistency w/findvc() */
               pvc = (struct VCELL *)allocpcv(
                  Host, 1, sizeof(struct VCELL), "sense block");
               vcdflt(pvc, tsname);    /* Initialize VCELL block */
               } /* End sense type switch */

            /* Interpret positional parameters nvc,vhw if present */
            inform("(S=V(VIHVIHV)VF)", &pvc->nsxl, &pvc->nsya,
               &pvc->vhw, NULL);

            /* Interpret keyword parameters */
            while (kwret = kwscan(&ic, /* Assignment intended */
                  "NVC%X",
                  "NVX%VIH",  &pvc->nsxl,  /* OBSOLETE */
                  "NVY%VIH",  &pvc->nsya,  /* OBSOLETE */
                  "TLA%V>B" qqv(FBxy) "IJ",  &pvc->tla,
                  "TLL%V>B" qqv(FBxy) "IJ",  &pvc->tll,
                  "VMIN%F",   &pvc->vcmin,
                  "VMAX%F",   &pvc->vcmax,
                  "X%VF",     &pvc->vcplx,
                  "Y%VF",     &pvc->vcply,
                  "W%V>F",    &pvc->vcplw,
                  "H%V>F",    &pvc->vcplh,
                  "KP%KHFNIOKQ2GB",  &pvc->kvp,
                  "CPT%B14IH",       &pvc->vcpt,
                  "HWIDTH%VF",       &pvc->vhw,
                  "MODALITY%T" qLTF, &hmname,
                  "XRSTATS%S1",      &kxrstat,
                  NULL)) {
               switch (kwret) {
               case 1:     /* NVC */
                  inform("(SV(VIHVIHV))", &pvc->nsxl, &pvc->nsya, NULL);
                  break;
                  } /* End kwret switch */
               } /* End kwret while */

            /* Copy VKTR4 bit to vcflags VKR4 to make it safe
            *  against later plot option changes.  */
            if (pvc->kvp & VKTR4) pvc->vcflags |= VKR4;

            /* Turn off plotting if height, width not given */
            if (pvc->vcplw <= 0.0 || pvc->vcplh <= 0.0)
               pvc->kvp &= ~VKPPL;

            /* Error if vcmax <= vcmin */
            if (pvc->vcmax <= pvc->vcmin)
               cryout(RK_E1, "0***vmax <= vmin NOT VALID.",
                  RK_LN2, NULL);

            /* If MODALITY or XRSTATS option given, link up to
            *  a modality block.  If mname not given here, use
            *  tsname as a default.  */
            if (kxrstat || hmname) {
               if (!hmname)
                  hmname = savetxt(tsname);
               pmdlt = findmdlt(getrktxt(hmname), 0);
               if (pmdlt) {
                  pvc->pvcmdlt = pmdlt;
                  pmdlt->mdltflgs |= MDLTFLGS_RQST; }
               } /* End processing modality */

            /* Dispose of VCELL block according to its type */
            switch (pvc->vtype) {

            case 0:
               /* An unrecognized type name is an error.  Free
               *  the block because nobody can use it.  */
               ermark(RK_MARKFLD);
               cryout(RK_P1, "0***UNRECOGNIZED SENSE NAME ", RK_LN2,
                  pvc->vcname, LSNAME, NULL);
               freep(pvc);
               break;

            case VJ_SRC:
            case VW_SRC:
               /* Kinesthesia cannot be a modality, since there is
               *  no way to assign an input stimulus number.  */
               if (kxrstat) {
                  ermark(RK_MARKFLD);
                  cryout(RK_P1, "0***KINESTHESIA CANNOT BE AN "
                     "XRSTATS MODALITY.", RK_LN2, NULL); }
               break;

            case VT_SRC:
               /* Touch already linked, nothing to do here */
               break;

            case IA_SRC:
            case VALSRC:
            case VH_SRC:
            case VS_SRC:
            case TV_SRC:
            case PP_SRC:
               /* These cases are always in error because they are
               *  handled by other control cards.  If other cases are
               *  made legal above, be sure to add their size defaults
               *  in d3schk().  */
               ermark(RK_MARKFLD);
               cryout(RK_P1, "0***INVALID SOURCE USED AS SENSE NAME: ",
                  RK_LN2, pvc->vcname, LSNAME, NULL);
               freep(pvc);
               break;

#ifdef BBDS
            case USRSRC:
               /* Register with the BBD package.  Note that bbdsregs()
               *  checks for nonnegative nsxl, nsya, so the nsxl,nsya
               *  defaults must be applied now, not in d3schk().  */
               if (pvc->nsxl < 0) pvc->nsxl = 12;
               if (pvc->nsya < 0) pvc->nsya = 1;
               pvc->lvrn = strnlen(getrktxt(pvc->hvname),
                  MaxDevNameLen);
               pvc->pvcsrc = bbdsregs(getrktxt(pvc->hvname),
                  hhname ? getrktxt(hhname) : NULL,
                  pvc->nsxl, pvc->nsya,
                  pvc->vcflags & VKR4 ? BBDSns_R4 : BBDSns_Byte);
               /* Set BBDMdlt flag if assigned to a modality */
               if (pvc->pvcmdlt && pvc->pvcsrc /*JIC*/)
                  ((BBDDev *)pvc->pvcsrc)->bdflgs |= BBDMdlt;
               /* Set a flag to simplify EVTRIAL controls */
               RP->kevtb |= EVTSNS;
               /* Set a flag to help d3gvin know what to do */
               if (pvc->vcflags & VKR4) pvc->vcxc.tvmode = BM_NONSTD;
               /* Append to user-defined sense list */
               *pplbbds = pvc;
               pplbbds = &pvc->pnvc;
               break;
#endif

            default:
               /* These will be any other built-in senses (none at
               *  present, former platform senses came here). */
               *RP0->plvc = pvc;
               RP0->plvc = &pvc->pnvc;
               } /* End type switch */
            } /* End SENSE local scope */
            break;

/*---------------------------------------------------------------------*
*                             SJPLOT card                              *
*---------------------------------------------------------------------*/

         case SJPLOT:
            qcclate = TRUE;
            getijpl(KCIJS);
            break;

/*---------------------------------------------------------------------*
*                      STATISTICS or STATS card                        *
*---------------------------------------------------------------------*/

         case STATS1:
         case STATS2:
            getstat();
            break;

/*---------------------------------------------------------------------*
*                             TRACE card                               *
*---------------------------------------------------------------------*/

         case TRACE:
            /* Let user know that Wallace is no longer supported. */
            cryout(RK_E1, "0***WALLACE PROSTHESIS HAS BEEN WITHDRAWN.",
               RK_LN2, NULL);
            continue;

/*---------------------------------------------------------------------*
*                          CAMERA or TV card                           *
*                                                                      *
*  Note:  There is a little uncertainty here exactly what options      *
*  would be allowed in the VVTV case, since that has not really been   *
*  implemented, just patterned on the VVTV NCUBE driver.  For now,     *
*  all options allowed with BBD are also allowed with VVTV and vice-   *
*  versa, but when a real implementation is available, that situation  *
*  should be checked.                                                  *
*---------------------------------------------------------------------*/

         case CAMERA:
         case TV:
            g1cam();
            break;

/*---------------------------------------------------------------------*
*                            IMGTOUCH card                             *
*---------------------------------------------------------------------*/

         case IMGTOUCH:
            qcclate = TRUE;
            getitp();
            break;

/*---------------------------------------------------------------------*
*                        UNIVERSAL JOINT card                          *
*---------------------------------------------------------------------*/

         case UJOINT:
            qcclate = TRUE;
            g1ujnt();         /* Now handled in d3ajw.c */
            break;

/*---------------------------------------------------------------------*
*                             VALUE card                               *
*---------------------------------------------------------------------*/

         case VALUE: {
            int  kv,iw;
#ifdef BBDS
            txtid hhname;           /* Host name locator */
#endif
            char mname[LTEXTF+1];
            static char vbkid0[] = VBKID; /* Kind of value params */
            extern char *vbtnames[];

            /* Link new value block to chain */
            struct VBDEF *ivb = *plvb = (struct VBDEF *)
               allocpcv(Static, 1, IXstr_VBDEF, "value");
            plvb = &ivb->pnxvb;

            /* Set type-independent defaults */
            vbdflt(ivb);
            ivb->vbflg = VBFLGC1;

            /* Count value blocks and store index for d3valu() */
            ivb->ivdt = RP->nvblk++;

            /* Generate an offset for overflow counting */
            ivb->iovec = RP->novec++;

            /* Determine value type and read positional parms. */
            cdscan(RK.last, 1, SCANLEN, RK_WDSKIP);
            kv = ivb->kval =
               match(OFF, RK_MREQF, ~RK_COMMA, 0, vbtnames, VBNUM) - 1;
            if (kv >= 0) {    /* We got a good value type */
               byte kid = vbkid0[kv];
               ivb->vbkid = kid;

               /* Thread this value block onto rep or env list */
               if (kid == VBK_REP) {
                  *plvbr = ivb, plvbr = &ivb->pnxvt; }
               else {
                  *plvbe = ivb, plvbe = &ivb->pnxvt; }

               /* The code in this switch is responsible to set
               *  any defaults that are kval dependent and then
               *  to process the positional args. as approp. */
               switch (kid) {
               case VBK_REP:     /* Repertoire type */
                  inform("(S,2A" qLSN ")",   /* 2 field req'd */
                     &ivb->valnm.rnm, NULL);
                  ivb->vhalf = 5;
                  break;
               case VBK_DIST:    /* Distance type */
                  inform("(S=2A" qLSN ")",   /* (0-2 fields OK) */
                     &ivb->valnm.rnm, NULL);
                  ivb->vhalf = min(RP->nsx, RP->nsy);
                  break;
               case VBK_UJG:     /* Universal joint or grip type */
                  inform("(S=A" qLSN ")",    /* (0-1 fields OK) */
                     ivb->valnm.rnm, NULL);
                  break;
               case VBK_ENV:     /* Environmental type */
                  /* Note that modality matching is done here, but
                  *  repertoire, etc. matching must be deferred to
                  *  d3tchk().  The modality name won't necessarily
                  *  fit in valnm.rnm, so the modality number is
                  *  stored there as a place holder.  */
                  strcpy(mname, "VISION");
                  strcpy(ivb->valnm.lnm, "0");
                  inform("(S=A" qLTF "A" qLSN ")", mname,
                     ivb->valnm.lnm, NULL);
                  wbcdin(ivb->valnm.lnm, &iw, LEA2INT);
                  pmdlt = findmdlt(mname, iw);
                  if (pmdlt) {
                     ivb->u1.vev.pvmdl = pmdlt;
                     pmdlt->mdltflgs |= MDLTFLGS_VALU;
                     sconvrt(ivb->valnm.rnm, "(J0IC" qLSN ")",
                        &pmdlt->mdltno, NULL);
                     }
                  break;
               case VBK_CONS:    /* Constant value (test code) */
                  inform("(S=B" qqv(FBvl) "IJ)", &ivb->vbase, NULL);
                  ivb->vdelay = 0;     /* Default not 1 here */
                  ivb->vbflg |= VBFLCON;
                  break;
#ifdef BBDS
               case VBK_BBD:     /* BBD type */
                  g1bbdn(&hhname, &ivb->valnm.hbcnm);
                  /* Register with the BBD package */
                  ivb->u1.vbbd.pbvdev = bbdsregv(
                     getrktxt(ivb->valnm.hbcnm),
                     hhname ? getrktxt(hhname) : NULL);
                  /* Set a flag to simplify EVTRIAL controls */
                  RP->kevtb |= EVTVAL;
                  break;
#endif
                  } /* End kid switch */
               } /* End kv >= 0 */

            /* Scan keyword items */
            /* N.B. Writeup has "DAMPFAC" but many user decks use
            *  the longer form given here ... sigh.  NOPLOT option
            *  removed, 09/23/01, because it could be confused with
            *  a (alphabetic) positional parameter above.  */
            kwscan(&ic,
               "SCALE%B" qqv(FBsc) "IJ",         &ivb->vscl,
               "DAMPFACT%V<=1B" qqv(FBdf) "UH",  &ivb->vdamp,
               "VDAMP%V<=1B" qqv(FBdf) "UH",     &ivb->vdamp,
               "VDELAY%UH",                      &ivb->vdelay,
               "BASE%B" qqv(FBvl) "IJ",          &ivb->vbase,
               "BEST%RVI", (int)(kv<=VB_FIRE),   &ivb->vbest,
               "HWIDTH%RVI",(int)(kv<=VB_UJMD),  &ivb->vhalf,
               "MIN%B" qqv(FBvl) "IJ",           &ivb->vmin,
               "VMIN%B" qqv(FBvl) "IJ",          &ivb->vmin,
               "VOP%KC" okvalue,                 &ivb->vbopt,
               NULL);

            /* Force options 'A' and 'F' if CONSTANT */
            if (kv == VB_CONS) ivb->vbopt |= VBVOPAV|VBVOPFS;
            } /* End VALUE local scope */
            break;

/*---------------------------------------------------------------------*
*                             VERIFY card                              *
*---------------------------------------------------------------------*/

         case VERIFY:
            RP0->n0flags |= N0F_TSVP;
            break;

/*---------------------------------------------------------------------*
*                             WINDOW card                              *
*---------------------------------------------------------------------*/

         case WINDOWCD:
            qcclate = TRUE;
            g1wdw();          /* Now handled in d3ajw.c */
            break;

/*---------------------------------------------------------------------*
*                    END OF GROUP I CONTROL CARDS                      *
*---------------------------------------------------------------------*/

         } /* End SMATCH switch */

     } /* End while CRYIN */

   rdagn();

#ifndef PAR
   if (RP->CP.dbgflags & DBG_PPKERN) endplt();
#endif

   /* Set scale for virtual inputs according to Si range */
   RP->fvcscl = (RP->compat & OLDRANGE) ? dS14 : dS7;

   /* Copy global grid increment to region/repertoire grids */
   RP0->GRpD.ngridx = RP0->GRpD.ngridy = RP0->grids;

   /* If no environment plot color was entered, set the default now
   *  based on whether environment objects are colored.  */
   if (!RP0->ecolstm[0]) strcpy(RP0->ecolstm,
      RP->kcolor ? "RGB" : "YELLOW");
   return RK.iexit;

   } /* End d3grp1 */
