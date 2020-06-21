/* (c) Copyright 2013-2014, George N. Reeke, Jr. */
/* $Id: $ */

/***********************************************************************
*           Multi-Dimensional Kolmogorov Smirnov Statistic             *
*                              mdksopt.c                               *
*                                                                      *
*  Routine to run brick-method multidimensional Kolmogorov-Smirnov     *
*  test with multiple edge divisions for optimization.  Each test is   *
*  run NREPS (default 25) times and the mean and sigma of the MDKS and *
*  mean times are printed.  Random numbers are read from binary files  *
*  made with matlab.  I/O uses ROCKS library routines.                 *
*                                                                      *
*  INPUT on stdin:  One or more ROCKS-style control cards with these   *
*     options:                                                         *
*                                                                      *
*  MDKSOPT {T1|T2|T}={UNIF|NORM|GAM}, K=k, {N1|N2|N}=n, R=r,           *
*     DOED={0|1|2}, {D1|D2|DIVS}=(d1,d2,...dm), DOBF={0|1}             *
*                                                                      *
*  'T1', 'T2', or 'T' gives distribution type for first data set,      *
*     second data set, or both data sets, bzw.                         *
*  'K' gives dimension.                                                *
*  'N1', 'N2', or 'N'  gives number of data points in first data set,  *
*     second data set, or both data sets, bzw.                         *
*  'R' gives number of repetitions.                                    *
*  'DOED' determines how the 'DIVS' options will be interpreted:       *
*     DOED=0 (the default) ==> DIVS is a list of number of divisions   *
*        to be tested in all dimensions for both data sets.            *
*     DOED=1 ==> DIVS is a list of divisions for exactly 'k'           *
*        dimensions.  The list is applied once to both data sets.      *
*     DOED=2 ==> An actual optimization is performed.  Starting        *
*        with the default number of divisions - 2, this number is      *
*        increased by one and a new calculation is performed until     *
*        the time increases by two ticks.  DIVS entries are ignored.   *
*  'D1', 'D2', or 'DIVS' gives a list of numbers of divisions          *
*     along each data axis to be tested (0 for defaults).              *
*  'DOBF' if not zero activates printing of brute-force results        *
*     for comparison.                                                  *
*                                                                      *
*  In addition, these compile-time options are available:              *
*  SIDED = 1 or 2 (any value not 1) compiles code to perform 1- or 2-  *
*     sided MDKS test.                                                 *
*  XTYPE = 0 to test float data, 1 to test double precision data.      *
*  DEBUG & 1  when SIDED = 2 prints the one-sided test results for     *
*     both orderings of the two data sets.                             *
*  DEBUG & 2  activates a detailed list of the number of points in     *
*     each brick and the max and min coordinates along each axis for   *
*     the points in each brick on the first of 'r' repeats.            *
*  DEBUG & 6  also prints the list of data points associated with      *
*     each br
*                                                                      *
*  Note:  At this writing, there are 1,250,000 random points stored in *
*  each data file.  When the product n*k*r exceeds this number, some   *
*  data points are reused to complete the full number of test repeats. *
************************************************************************
*  V1A, 12/20/13, GNR - New program                                    *
*  V1B, 02/01/14, GNR - Allow > 10 repeats, rewind input if necessary  *
*  V1C, 02/21/14, GNR - Print mean time and brute force answer         *
*  V1E, 04/09/14, GNR - Include calls for up to NRIF points in each    *
*                       timed block to improve time interval accuracy. *
*  V1F, 04/15/14, GNR - Add SIDED and DEBUG & 1 controls               *
*  V1G, 04/16/14, GNR - Add N1 and N2                                  *
*  ==>, 04/16/14, GNR - Last mod before committing to svn repository   *
*  Rev, 04/22/14, GNR - Increase MXK to 10, doed mode, DIVS[12], T[12] *
*  Rev, 04/28/14, GNR - Call new mdksdivs() to get divisions           *
*  V2A, 05/02/14, GNR - Add programmed optimization option             *
***********************************************************************/

#define SIDED 1
#define DEBUG 0
#define XTYPE 1

#ifndef SIDED              /* Set default 1-SIDED */
#define SIDED 1
#endif
#ifndef XTYPE              /* Set default double-precision */
#define XTYPE 1
#endif
#ifndef DEBUG              /* Set default no DEBUG */
#define DEBUG 0
#endif

/* Configuration parameters */
#define NRIF 1250000       /* Number of randoms in files */
#define NTYPS   3          /* Number of distribution types */
#define MXGD   20          /* Max number of gdivs to test */
#define MXK    10          /* Max value of k */
#if MXGD < MXK
#error MXGD < MXK
#endif
#define MDBGBR  8          /* Print contents of every MDBGBR bricks */

/* defs relating to doed control */
#define NEDTIKS 2          /* Default number of doed=2 upticks */
#define EDGLBL  0          /* DIVS is a global list */
#define EDDIMS  1          /* DIVS is a list by dimension */
#define EDOPTD  2          /* DIVS ignored, optimize divs */

/* Defaults for input parameters */
#define NREPS  25          /* Default number of repetitions */
#define NDIM    2          /* Default number of dimensions */
#define NDAT 1000          /* Default number of points */
#define DDIVS   0          /* Use default divisions */
#define DOBF    1          /* Default: do brute force */
#define DOED EDGLBL        /* Default: no explicit divisions */

#define MAIN

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "rkarith.h"
#include "mdks.h"
#include "../mdksint.h"
#ifdef LINUX
#include <time.h>
#endif

#if XTYPE == 0
#define Xtype float
#else
#define Xtype double
#endif

int main() {

   void *pwk = NULL, *bfwk = NULL;
   Wkhd *phd;
   int *pdivs[NDSET];
   double savg,ss2,ssig,*s=NULL;
   double tim1,ttot,tmin;
   double ttest = HUGE_X;
#if XTYPE == 0
   double *X2r,*X1r=NULL;     /* Double random data */
#endif
   Xtype *X2,*X1;             /* Allocated space */
   Xtype *X2j,*X1j;           /* Used space */
   char *fnms[NTYPS][2] = { { "unifrand", "unifran2" },
                            { "normrand", "normran2" },
                            { "gammrand", "gammran2" } };
   char *tkeys[NTYPS] = { "UNIF", "NORM", "GAM" };
   char *qkeys[] = { "QUIT", "EXIT", "END" };
   char *card;

   rkfd *rkfx1 = NULL, *rkfx2 = NULL;
   size_t nrd1,nrd2,nwk1,nwk2;
   ui32 ic;
   int g1[MXK], g2[MXK], mdivs[MXGD];
   int k = NDIM, reps = NREPS, divs = DDIVS;
   int d1 = 0, d2 = 0;        /* Divs only for doed mode */
   int t1 = 1, t2 = 1;
   int n1 = NDAT, n2 = NDAT;
   int dobf = DOBF;           /* Do brute force calc too */
   int doed = DOED;           /* DIVS input taken per dimension */
   int ie,idiv,iop,jk,jr,nk1,nk2,nrbat;
   int edtct = 0;

   /* Interpret the control card */
   RK.iexit = 0;
#if XTYPE != 0
   settit("TITLE MDKS OPTIMIZER FOR double DATA");
#else
   settit("TITLE MDKS OPTIMIZER FOR float DATA");
#endif
   nopage(RK_PGNONE);
#if SIDED == 2
   cryout(RK_P1,"0Doing 2-sided KS test",RK_LN2,NULL);
#endif
#ifdef LINUX
   tim1 = (double)CLOCKS_PER_SEC;
   convrt("(P1,'0CLOCKS_PER_SEC is ',J0Q12.2)",&tim1,NULL);
#endif
   memset(mdivs, 0, MXGD*sizeof(int));
   while ((card = cryin())) {
      cdprnt(card);
      if (smatch(RK_NMERR, card, qkeys, 3)) break;
      cdscan(card, 1, 16, RK_WDSKIP);
      while ((iop = kwscan(&ic,    /* Assignment intended */
            "N%X",
            "T1%X", "T2%X", "T%X",
            "D1%X", "D2%X", "DIVS%X",
            "K%V<=10U",    &k,
            "N1%U",        &n1,
            "N2%U",        &n2,
            "R%U",         &reps,
            "DOBF%U",      &dobf,
            "DOED%V<=2U",  &doed,
            NULL))) {
         switch (iop) {
         case 1:  /* N */
            eqscan(&n1, "U", RK_EQCHK);
            n2 = n1;
            break;
         case 2:  /* Type 1 */
            t1 = match(RK_EQCHK, RK_MREQF, ~RK_COMMA, 0,
               tkeys, NTYPS);
            break;
         case 3:  /* Type 2 */
            t2 = match(RK_EQCHK, RK_MREQF, ~RK_COMMA, 0,
               tkeys, NTYPS);
            break;
         case 4:  /* Type */
            t1 = match(RK_EQCHK, RK_MREQF, ~RK_COMMA, 0,
               tkeys, NTYPS);
            t2 = t1;
            break;
         case 5:  /* Divs for X1 */
            d1 = MXK;
            inform("(SNV(RIV))", &d1, g1, NULL);
            d1 = RK.numcnv;
            break;
         case 6:  /* Divs for X2 */
            d2 = MXK;
            inform("(SNV(RIV))", &d2, g2, NULL);
            d2 = RK.numcnv;
            break;
         case 7:  /* Divs for both sets */
            divs = MXGD;
            inform("(SNV(RIV))", &divs, mdivs, NULL);
            divs = RK.numcnv;
            if (divs <= MXK) {
               size_t nd = (d1 = d2 = divs)*sizeof(int);
               memcpy(g1, mdivs, nd);
               memcpy(g2, mdivs, nd);
               }
            break;
            }
         }

      /* Set controls for doed vs normal DIVS input */
      switch (doed) {
      case EDGLBL:
         if (divs == DDIVS)
            divs = 1, mdivs[0] = 0;
         break;
      case EDDIMS:
         if (d1 == 0 && d2 == 0) {
            memset(g1, 0, k*sizeof(int));
            memset(g2, 0, k*sizeof(int));
            } 
         else if (d1 != k || d2 != k) cryout(RK_E1, "0DOED 1 requires "
            "k axial divisions for X1 and X2.", RK_LN2, NULL);
         divs = 1;
         break;
      case EDOPTD:
         /* Find default divs */
#if XTYPE != 0
         ie = mdksallod(&pwk, NULL, NULL, n1, n2, k);
#else
         ie = mdksallof(&pwk, NULL, NULL, n1, n2, k);
#endif
         if (ie > 0) abexit(ie+680);
         ie = mdksdivs(pwk, pdivs);
         if (ie > 0) abexit(ie+680);
         for (ie=0; ie<2; ++ie) {
            mdivs[ie] = pdivs[ie][0] - 2;
            if (mdivs[ie] < 2) mdivs[ie] = 2;
            }
         divs = 100;    /* JIC loop limit */
         edtct = 0;
         ttest = HUGE_X;
         break;
      default:
         cryout(RK_E1, "0***Invalid DOED.", RK_LN2, NULL);
         } /* End doed switch */

      /* Allocate space for the data */
      nk1 = n1*k, nk2 = n2*k;
      nrbat = NRIF/max(nk1,nk2);
      /* N.B. Data in input files are always doubles */
      nrd1 = nk1*sizeof(double), nrd2 = nk2*sizeof(double);
      nwk1 = nk1*sizeof(Xtype),  nwk2 = nk2*sizeof(Xtype);
#if XTYPE == 0
      X1r = (double *)reallocv(X1r, reps*(nrd1+nrd2), "X1r,X2r");
      X2r = X1r + reps*nk1;
#endif
      s = (double *)reallocv(s, reps*(nwk1+nwk2+sizeof(double)),
         "X1,X2,s");
      X1 = (Xtype *)(s + reps);
      X2 = X1 + reps*nk1;

      if (RK.iexit) abexitm(690, "Input error");
      spout(SPTM_ALL);

      /* Open files according to specified random type */
      rkfx1 = rfallo(fnms[t1-1][0], READ, BINARY, SEQUENTIAL, TOP,
         LOOKAHEAD, REWIND, RETAIN_BUFF, IGNORE, IGNORE, IGNORE, ABORT);
      rkfx2 = rfallo(fnms[t2-1][1], READ, BINARY, SEQUENTIAL, TOP,
         LOOKAHEAD, REWIND, RETAIN_BUFF, IGNORE, IGNORE, IGNORE, ABORT);
      rfopen(rkfx1, NULL, SAME, BINARY, SEQUENTIAL, TOP,
         LOOKAHEAD, REWIND, RETAIN_BUFF, IGNORE, IGNORE, IGNORE, ABORT);
      rfopen(rkfx2, NULL, SAME, BINARY, SEQUENTIAL, TOP,
         LOOKAHEAD, REWIND, RETAIN_BUFF, IGNORE, IGNORE, IGNORE, ABORT);

      /* Loop over number of divisions to be tested */
      if (n1 == n2)
         convrt("(P1,'0For k = ',J0I4,', n = ',J0I8,"
            "', dist = (',A4,1H,A4,'), reps = ',J0I6)",
            &k, &n1, tkeys[t1-1], tkeys[t2-1], &reps, NULL);
      else
         convrt("(P1,'0For k = ',J0I4,', n1 = ',J0I8,', n2 = ',J0I8,"
            "', dist = (',A4,1H,A4,'), reps = ',J0I6)",
            &k, &n1, &n2, tkeys[t1-1], tkeys[t2-1], &reps, NULL);
      tmin = HUGE_X;
      for (idiv=0; idiv<divs; ++idiv) {
         switch (doed) {
         case EDGLBL:
            for (jk=0; jk<k; ++jk)
               g1[jk] = g2[jk] = mdivs[idiv]; 
            break;
         case EDOPTD:
            for (jk=0; jk<k; ++jk)
               g1[jk] = g2[jk] = mdivs[jk], mdivs[jk] += 1;
            break;
            } /* End doed switch */
#if XTYPE != 0
         ie = mdksallod(&pwk, g1, g2, n1, n2, k);
#else
         ie = mdksallof(&pwk, g1, g2, n1, n2, k);
#endif
         if (ie > 0) abexit(ie+680);
         phd = (Wkhd *)pwk;
         ttot = savg = ss2 = 0.0;
         for (jr=0; jr<reps; ) {
            size_t nrdb1,nrdb2;
            int jrbat,jre;
            jrbat = min(reps-jr,nrbat);
            jre = jr + jrbat;
            nrdb1 = nrd1*jrbat;
            nrdb2 = nrd2*jrbat; 
            rfseek(rkfx1, 0, SEEKABS, ABORT);
            rfseek(rkfx2, 0, SEEKABS, ABORT);
#if XTYPE != 0
            rfread(rkfx1, X1j = X1, nrdb1, ABORT);
            rfread(rkfx2, X2j = X2, nrdb2, ABORT);
#else
            rfread(rkfx1, X1r, nrdb1, ABORT);
            rfread(rkfx2, X2r, nrdb2, ABORT);
            for (ie=0; ie<nk1*jrbat; ++ie) X1[ie] = (Xtype)X1r[ie];
            for (ie=0; ie<nk2*jrbat; ++ie) X2[ie] = (Xtype)X2r[ie];
            X1j = X1, X2j = X2; 
#endif
            tim1 = second();
            for ( ; jr<jre; ++jr) {
#if XTYPE != 0
#if SIDED == 1
               s[jr] = mdks1sd(pwk, X1j, X2j);
#else
               s[jr] = mdks2sd(pwk, X1j, X2j);
#endif
#else    /* Xtype is float */
#if SIDED == 1
               s[jr] = mdks1sf(pwk, X1j, X2j);
#else
               s[jr] = mdks2sf(pwk, X1j, X2j);
#endif
#endif
               if (s[jr] < 0.0) abexit((int)(-s[jr])+690);
               savg += s[jr];
               ss2  += s[jr] * s[jr];
/*** DEBUG ***/
/* N.B.  If either of these DEBUG codes is turned on, timing
*  will incorrectly include time to execute it  */
#if SIDED != 1 && DEBUG & 1
   {  
#if XTYPE != 0
      double s1 = mdks1sd(pwk, X1j, X2j);
      double s2 = mdks1sd(pwk, X2j, X1j);
#else
      double s1 = mdks1sf(pwk, X1j, X2j);
      double s2 = mdks1sf(pwk, X2j, X1j);
#endif
      convrt("(P1,4X,'For divs[0] = ',J0I4,', repeat ',J0I8/"
         "6X,'s(X1) = ',Q10.6,', s(X2) = ',Q10.6,', mdks = ',Q10.6)",
         phd->pdiv[0], &jr, &s1, &s2, s+jr, NULL);
      }
#endif      /* DEBUG 1 */
#if DEBUG & 2
if (jr == 0) {
   char *pcbr, *pcbre;
   int ibr,jx;
   for (jx=0; jx<IX2; ++jx) {
      pcbre = phd->pBrk0[jx] + phd->lbrkj[jx];
      convrt("(P1,4X,'For repeat ',J0I8,', data set ',J0I8)",
         &jr, &jx, NULL);
      ibr = 0;
      for (pcbr=phd->pBrk0[jx]; pcbr<pcbre; pcbr+=phd->lbrick) {
         Brick *pbr = (Brick *)pcbr;
#if XTYPE != 0    /* double data */
         convrt("(P1,6X,'Brick ',J0I8,', npts = ',J0I8,', lims = ',"
            "R=2<Q12.6,Q10.6>)", &ibr, &pbr->npts, &k,
            &pbr->ub1.Blims->Xmin, &pbr->ub1.Blims->Xmax, NULL);
#if DEBUG & 4
         if (ibr % MDBGBR == 0) {
            Link *ptl;
            for (ptl=pbr->pbm1; ptl; ptl=ptl->pnxt)
               convrt("(P2,8X,RQ12.6)", &k, ptl->ul1.pdat, NULL);
            }
#endif
#else             /* float data */
         convrt("(P1,6X,'Brick ',J0I8,', npts = ',J0I8,', lims = ',"
            "R=2<F12.6,F10.6>)", &ibr, &pbr->npts, &k,
            &pbr->ub1.Blims->Xmin, &pbr->ub1.Blims->Xmax, NULL);
#if DEBUG & 4
         if (ibr % MDBGBR == 0) {
            Link *ptl;
            for (ptl=pbr->pbm1; ptl; ptl=ptl->pnxt)
               convrt("(P2,8X,RF12.6)", &k, ptl->ul1.pdat, NULL);
            }
#endif
#endif
         ibr += 1;
         } /* End brick loop */
      } /* End data set loop */
   } /* End if jr == 1 */
convrt("(P1,8X,'For repeat ',J0I8,', s = ',Q12.6)", &jr, s+jr, NULL);
#endif      /* DEBUG 2 */
/*** ENDDEBUG ***/
               X1j += nk1, X2j += nk2;
               } /* End inner jr batch loop */
            ttot += second() - tim1;
            } /* End jr reps loop */
         savg /= (double)reps;
         ssig = sqrt(ss2/(double)reps - savg*savg);
         ttot /= (double)reps;
         if (ttot < tmin) tmin = ttot;
         /* Print the result */
         ie = mdksdivs(pwk, pdivs);
         if (ie > 0) abexit(ie+680);
         if (doed == EDDIMS) 
            convrt("(P1,4X,'Divs[X1] = ',RI3,', Divs[X2] = ',RI3/"
               "6X,'mean s = ',Q8.6,', sigma s = ',Q8.6,"
               "', time/trial = ',Q12.6)",
               &k, pdivs[0], &k, pdivs[1], 
               &savg, &ssig, &ttot, NULL);
         else
            convrt("(P1,4X,'Divs = ',J0I8,', mean s = ',Q8.6,"
               "', sigma s = ',Q8.6,', time/trial = ',Q12.6)",
               pdivs[0], &savg, &ssig, &ttot, NULL);
         /* If doing optimization, exit when two upticks */
         if (doed == EDOPTD) {
            if (edtct == 0) {
               if (ttot < ttest) ttest = ttot;
               else edtct = NEDTIKS;
               }
            else {
               if (ttot > ttest) ttest = ttot, --edtct;
               if (edtct <= 0) break;
               }
            }
         } /* End loop over divisions */

      /* Do same number of repeats with brute force method */
      if (dobf) {
#if XTYPE != 0
         ie = bfksallod(&bfwk, n1, n2, k);
#else
         ie = bfksallof(&bfwk, n1, n2, k);
#endif
         if (ie > 0) abexit(ie+680);
         ttot = savg = ss2 = 0.0;
         for (jr=0; jr<reps; ) {
            size_t nrdb1,nrdb2;
            int jrbat,jre;
            jrbat = min(reps-jr,nrbat);
            jre = jr + jrbat;
            nrdb1 = nrd1*jrbat;
            nrdb2 = nrd2*jrbat; 
            rfseek(rkfx1, 0, SEEKABS, ABORT);
            rfseek(rkfx2, 0, SEEKABS, ABORT);
#if XTYPE != 0
            rfread(rkfx1, X1j = X1, nrdb1, ABORT);
            rfread(rkfx2, X2j = X2, nrdb2, ABORT);
#else
            rfread(rkfx1, X1r, nrdb1, ABORT);
            rfread(rkfx2, X2r, nrdb2, ABORT);
            for (ie=0; ie<nk1*jrbat; ++ie) X1[ie] = (Xtype)X1r[ie];
            for (ie=0; ie<nk2*jrbat; ++ie) X2[ie] = (Xtype)X2r[ie];
            X1j = X1, X2j = X2; 
#endif
            tim1 = second();
            for ( ; jr<jre; ++jr) {
#if XTYPE != 0
#if SIDED == 1
               s[jr] = bfks1sd(bfwk, X1j, X2j);
#else
               s[jr] = bfks2sd(bfwk, X1j, X2j);
#endif
#else    /* Xtype is float */
#if SIDED == 1
               s[jr] = bfks1sf(bfwk, X1j, X2j);
#else
               s[jr] = bfks2sf(bfwk, X1j, X2j);
#endif
#endif
               if (s[jr] < 0.0) abexit((int)(-s[jr])+690);
               savg += s[jr];
               ss2  += s[jr] * s[jr];
               X1j += nk1, X2j += nk2;
               } /* End inner jr batch loop */
            ttot += second() - tim1;
            } /* End jr reps loop */
         savg /= (double)reps;
         ssig = sqrt(ss2/(double)reps - savg*savg);
         ttot /= (double)reps;
         tmin = tmin > 0.0 ? ttot/tmin : 0.0;
         /* Print the result */
         convrt("(P1,'0   Brute force mean s = ',Q8.6,"
            "', sigma s = ',Q8.6,', time/trial = ',Q12.6)",
            &savg, &ssig, &ttot, NULL);
         convrt("(P1,'    Brute force time/min brick time = ',Q12.6)",
            &tmin, NULL);
         } /* End if dobf */

      } /* End while(cryin) */

   /* Cleanup */
   if (s) freev(s,"X1,X2,s");
#if XTYPE == 0
   if (X1r) freev(X1r,"X1r,X2r");
#endif
   if (pwk) {
      ie = mdksfree(pwk);
      if (ie > 0) abexit(ie+680);
      }
   if (bfwk) {
      ie = bfksfree(bfwk);
      if (ie > 0) abexit(ie+680);
      }
   if (rkfx1) rfclose(rkfx1, 0, 0, ABORT);
   if (rkfx2) rfclose(rkfx2, 0, 0, ABORT);

   cryout(RK_P1,"0End of mdksopt run",RK_LN0+RK_FLUSH+1,NULL);
   cryocls();
   return 0;
   }
