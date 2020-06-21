/* (c) Copyright 2007-2017, The Rockefeller University *11116* */
/* $Id: d3master.c 78 2018-08-02 18:36:58Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                              g2master                                *
*                      Read group II MASTER card                       *
*                                                                      *
*  This card modifies global system defaults for various network       *
*  parameters.  Defaults read from the MASTER card remain in effect    *
*  until the next MASTER card.  This card now has subdivisions cor-    *
*  responding to the regular Group II card where the parameters of     *
*  that subdivision may be found.  The subdivision is indicated by     *
*  the first positional keyword.                                       *
*                                                                      *
*  The only argument is a pointer to the master random number seed.    *
*                                                                      *
************************************************************************
*  V8D, 12/29/07, GNR - New routine, based on code from d3grp2         *
*  ==>, 01/09/08, GNR - Last mod before committing to svn repository   *
*  Rev, 03/22/08, GNR - Add conntype-level MASTER cards, CELLTYPE CPR, *
*                       CPT, and SHAPE                                 *
*  V8E, 01/14/09, GNR - Add MASTER IZHIKEVICH, PHASE cards             *
*  Rev, 03/11/09, GNR - Add MASTER IZHI2003                            *
*  Rev, 09/02/09, GNR - Add MASTER AEIF                                *
*  Rev, 08/14/10, GNR - Add MASTER AUTOSCALE                           *
*  Rev, 06/25/13, GNR - Add V~ test on stanh                           *
*  R66, 02/17/16, GNR - Change long random seeds to typedef si32 orns  *
*  R72, 03/09/17, GNR - Add MASTER REGION XGAP, YGAP                   *
*  R74, 09/13/17, GNR - Add siha (earlier) and cpcap                   *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "clblk.h"
#include "d3opkeys.h"
#include "rocks.h"
#include "rkxtra.h"

void g2master(orns *inseed) {

   ui32  ic;                  /* kwscan read flags */
   int   imat;                /* Subdivision index */
   int   kwret;               /* kwscan return code */
   short tgrid;
   enum  ksubenum { BAD_MATCH=0, MS_SEED, MS_REGION, MS_REP, MS_CT,
      MS_BREG, MS_AUTSCL, MS_DECAY, MS_DEPR, MS_IZHI2003, MS_IZHI2007,
      MS_IZHI, MS_IZHIKEVICH, MS_NOISE, MS_PHASE, MS_AMPLIF, MS_DELAY,
      MS_PPF, MS_PPF2, MS_PARAMS, MS_PARAMS2, MS_THRESH };

   static char *msubs[] = { "SEED", "REGION", "REPERTOIRE",
      "CELLTYPE", "AEIF", "AUTOSCALE", "DECAY", "DEPRESSION",
      "IZHI2003", "IZHI2007", "IZHI", "IZHIKEVICH", "NOISE",
      "PHASE", "AMPLIFICATION", "DELAY", "PPF", "PAIRED",
      "PARAMS", "PARAMETERS", "THRESHOLDS" };
#define NMSUBS (sizeof(msubs)/sizeof(char *))

   cdscan(RK.last, 1, SCANLEN, RK_WDSKIP);
   imat = match(0, RK_MREQF, ~(RK_COMMA|RK_EQUALS), 0, msubs, NMSUBS);
   if (imat <= BAD_MATCH) {
      skip2end();
      return;
      }
   if (imat > 1 && RK.scancode & RK_EQUALS) ermark(RK_PUNCERR);
   /* Force any thresholds or scales to go to dfthr storage */
   RP0->ksrctyp = BADSRC;
   switch ((enum ksubenum)imat) {

/*---------------------------------------------------------------------*
*                           SEED parameters                            *
*---------------------------------------------------------------------*/

   case MS_SEED:
      eqscan(inseed, "VIJ", RK_BEQCK);
      break;

/*---------------------------------------------------------------------*
*                    REGION/REPERTOIRE parameters                      *
*---------------------------------------------------------------------*/

   case MS_REGION:
   case MS_REP:
      tgrid = -1;
      kwscan(&ic,
         "KRP%KH" okrpkrp,       &RP0->GRpD.krp,
         "GRIDS%V>IH",           &tgrid,
         "XGRID%V>IH",           &RP0->GRpD.ngridx,
         "YGRID%V>IH",           &RP0->GRpD.ngridy,
         "VXO%F",                &RP0->GRpD.vxo,
         "VYO%F",                &RP0->GRpD.vyo,
         "XGAP%VF",              &RP0->GRpD.xgap,
         "YGAP%VF",              &RP0->GRpD.ygap,
         "BPTIMER%N" KWS_TIMNW,  &RP0->GRpD.jbptim,
         NULL);

      if (tgrid > 0) RP0->GRpD.ngridx = RP0->GRpD.ngridy = tgrid;
      break;

/*---------------------------------------------------------------------*
*                         CELLTYPE parameters                          *
*---------------------------------------------------------------------*/

   case MS_CT:
      while (kwret = kwscan(&ic, /* Assignment intended */
         "RF%X",
         "SHAPE%X",
         "COLOR%X",
         "COLSRC%X",
         "VREST%X",
         "REST%X",
         "SIHA%B20/27IJ$mV",     &RP0->GCtD.siha,
         "SLOPE%V>B" qqv(FBsc) "IJ", &RP0->GCtD.sslope,
         "SPIKE%VB20/27IJ$mV",   &RP0->GCtD.sspike,
         "THAMP%V~B20/27IJ$mV",  &RP0->GCtD.stanh,
         "PT%B20/27IJ$mV",       &RP0->GCtD.pt,
         "ST%B20/27IJ$mV",       &RP0->GCtD.st,
         "NT%B20/27IJ$mV",       &RP0->GCtD.nt,
         "HT%B20/27IJ$mV",       &RP0->GCtD.ht,
         "CM%V>8B23UJ$pF",       &RP0->GCtD.Cm,
         "CPCAP%B7/14IH$mV",     &RP0->GCtD.cpcap,
         "CPR%VF",               &RP0->GCtD.cpr,
         "CPT%B7/14IH$mV",       &RP0->GCtD.cpt,
         "CTOPT%KH" okctopt,     &RP0->GCtD.ctopt,
         "KCTP%KH" okctkctp,     &RP0->GCtD.kctp,
         "STATS%KH" okctsta,     &RP0->GCtD.kstat,
         "KETIMER%N" KWS_TIMNW,  &RP0->GCtD.jketim,
         "REST%B20/27IJ$mV",     &RP0->GCtD.vrest,
         "VREST%B20/27IJ$mV",    &RP0->GCtD.vrest,
         "TAUM%V>B20UJ$ms",      &RP0->GCtD.taum,
         NULL)) switch (kwret) {
      case 1:  /* RF (Response Function) */
         RP0->GCtD.rspmeth = getrspfn();
         break;
      case 2:  /* SHAPE */
         getshape(&RP0->GCtD);
         break;
      case 3:  /* COLOR */
         getsicol(&RP0->GCtD);
         break;
      case 4:  /* COLSRC */
         getscsrc(&RP0->GCtD);
         break;
      case 5:  /* VREST option */
      case 6:  /* REST (compat) option */
         getvrest(&RP0->GCtD, RK_EQCHK);
         break;
         } /* End kwswitch and while */
      break;

/*---------------------------------------------------------------------*
*                  BRETTE-GERSTNER (aEIF) parameters                   *
*---------------------------------------------------------------------*/

   case MS_BREG:
      g2breg(&RP0->GBGD);
      break;

/*---------------------------------------------------------------------*
*                        AUTOSCALE parameters                          *
*---------------------------------------------------------------------*/

   case MS_AUTSCL:
      g2autsc(&RP0->GASD);
      break;

/*---------------------------------------------------------------------*
*                          DECAY parameters                            *
*---------------------------------------------------------------------*/

   case MS_DECAY:
      g2decay(NULL, &RP0->GDcD, &RP0->GCnD);
      break;

/*---------------------------------------------------------------------*
*                        DEPRESSION parameters                         *
*---------------------------------------------------------------------*/

   case MS_DEPR:
      g2dprsn(&RP0->GDpD);
      break;

/*---------------------------------------------------------------------*
*                        IZHIKEVICH parameters                         *
*---------------------------------------------------------------------*/

   case MS_IZHI2003:
      g2iz03(&RP0->GIz3D);
      break;

   case MS_IZHI2007:
   case MS_IZHI:
   case MS_IZHIKEVICH:
      g2iz07(&RP0->GIz7D);
      break;


/*---------------------------------------------------------------------*
*                          NOISE parameters                            *
*---------------------------------------------------------------------*/

   case MS_NOISE:
      g2noise(&RP0->GNoD);
      break;

/*---------------------------------------------------------------------*
*                          PHASE parameters                            *
*---------------------------------------------------------------------*/

   case MS_PHASE:
      g2phase(&RP0->GPdD, &RP0->GDcD);
      break;

/*---------------------------------------------------------------------*
*                      AMPLIFICATION parameters                        *
*---------------------------------------------------------------------*/

   case MS_AMPLIF:
      g2amplif(&RP0->GCnD);
      break;

/*---------------------------------------------------------------------*
*                          DELAY parameters                            *
*---------------------------------------------------------------------*/

   case MS_DELAY:
      g2delay(&RP0->GCnD);
      break;

/*---------------------------------------------------------------------*
*             PPF (PAIRED PULSE FACILITATION) parameters               *
*                                                                      *
*  Note:  The global defaults for PPF in d3dflt.c do not activate PPF  *
*  for all connection types in the run, but this MASTER PPF card does. *
*---------------------------------------------------------------------*/

   case MS_PPF:
   case MS_PPF2:
      scanagn();  /* So g2ppf can tell which key name was used */
      g2ppf(&RP0->GCnD, &RP0->GPpfD);
      RP0->n0flags |= N0F_MPPFE; /* Force PPFDATA alloc in CONNTYPE */
      break;

/*---------------------------------------------------------------------*
*                PARAMS, PARAMETERS, THRESHOLDS cards                  *
*---------------------------------------------------------------------*/

   case MS_PARAMS:
   case MS_PARAMS2:
      g2param(NULL, &RP0->GCnD, NO);
      break;

   case MS_THRESH:
      g2param(NULL, &RP0->GCnD, YES);
      break;

      } /* End subdivision switch */

   } /* End g2master() */
