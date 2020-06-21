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
*     DEPTH=d, DOOP={0,1}, DOBF={0|1}                                              *
*                                                                      *
*  'T1', 'T2', or 'T' gives distribution type for first data set,      *
*     second data set, or both data sets, bzw.                         *
*  'K' gives dimension.                                                *
*  'N1', 'N2', or 'N'  gives number of data points in first data set,  *
*     second data set, or both data sets, bzw.                         *
*  'R' gives number of repetitions.                                    *
*  'DEPTH' gives the hierarchical depth for the analysis (0 for        *
*     default).                                                        *
*  'DOOP' is not zero activates optimization of depth.                 *
*  'DOBF' if not zero activates printing of brute-force results        *
*     for comparison.                                                  *
*                                                                      *
*  In addition, these compile-time options are available:              *
*  SIDED = 1 or 2 (any value not 1) compiles code to perform 1- or 2-  *
*     sided MDKS test.                                                 *
*  XTYPE = 0 to test float data, 1 to test double precision data.      *
*  DEBUG & 1  when SIDED = 2 prints the one-sided test results for     *
*     both orderings of the two data sets.                             *
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
*  V6A, 05/21/14, GNR - Modified for mdks V6 -- DIVS now is DEPTH      *
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
#define HUGE_X 1.6E308     /* Really big double */
#define NRIF 1250000       /* Number of randoms in files */
#define NTYPS   3          /* Number of distribution types */
#define MNDOP   2          /* Min depths to test below default */

/* Defaults for input parameters */
#define NREPS  25          /* Default number of repetitions */
#define NDIM    2          /* Default number of dimensions */
#define NDAT 1000          /* Default number of points */
#define DDEPTH  0          /* Use default depth */
#define DOBF    1          /* Default: do brute force */
#define DOOP    0          /* Default: omit optimization */

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

/*=====================================================================*
*                               mdksopt                                *
*=====================================================================*/

int main() {

   void *pwk = NULL, *bfwk = NULL;
   Wkhd *phd;
   double savg,ss2,ssig,*s=NULL;
   double tim1,ttot,tmin;
   double dd,topt = HUGE_X;
#if XTYPE == 0
   double *X2r,*X1r=NULL;     /* Double random data */
#endif
   Xtype *X2,*X1;             /* Allocated space */
   Xtype *X2j,*X1j;           /* Used space */
   int *dret[2];
   char *fnms[NTYPS][2] = { { "unifrand", "unifran2" },
                            { "normrand", "normran2" },
                            { "gammrand", "gammran2" } };
   char *tkeys[NTYPS] = { "UNIF", "NORM", "GAM" };
   char *qkeys[] = { "QUIT", "EXIT", "END" };
   char *card;

   rkfd *rkfx1 = NULL, *rkfx2 = NULL;
   size_t nrd1,nrd2,nwk1,nwk2;
   ui32 ic;
   int k = NDIM, reps = NREPS, depth = DDEPTH;
   int t1 = 1, t2 = 1;
   int mxn, n1 = NDAT, n2 = NDAT;
   int dobf = DOBF;           /* Do brute force calc too */
   int doop = DOOP;           /* Do depth optimization */
   int ie,iop,jr,nk1,nk2,nrbat;
   int idct = MNDOP;

   /* Interpret the control card */
   RK.iexit = 0;
#if XTYPE != 0
   settit("TITLE MDKS6 OPTIMIZER FOR double DATA");
#else
   settit("TITLE MDKS6 OPTIMIZER FOR float DATA");
#endif
   nopage(RK_PGNONE);
#if SIDED == 2
   cryout(RK_P1,"0Doing 2-sided KS test",RK_LN2,NULL);
#endif
#ifdef LINUX
   tim1 = (double)CLOCKS_PER_SEC;
   convrt("(P1,'0CLOCKS_PER_SEC is ',J0Q12.2)",&tim1,NULL);
#endif
   while ((card = cryin())) {
      cdprnt(card);
      if (smatch(RK_NMERR, card, qkeys, 3)) break;
      cdscan(card, 1, 16, RK_WDSKIP);
      while ((iop = kwscan(&ic,    /* Assignment intended */
            "N%X",
            "T1%X", "T2%X", "T%X",
            "DEPTH%U",     &depth,
            "K%V<=10U",    &k,
            "N1%U",        &n1,
            "N2%U",        &n2,
            "R%U",         &reps,
            "DOBF%U",      &dobf,
            "DOOP%U",      &doop,
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
            }
         }

      mxn = n1 > n2 ? n1 : n2;
      {  double dmxn = (double)mxn;
         double dk   = (double)k;
         double dl2n = log2((double)mxn);
         double oldd = dl2n/dk;
         dd = 1.9287E-6*dmxn*oldd + 1.0305*oldd +
            0.0101*dl2n + 0.3616*dk - 2.6180;
         }

      /* Set controls for optimization vs normal run */
      if (doop) {
         depth = (int)ceil(dd);
         idct = MNDOP;
         topt = HUGE_X;
         }

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

      /* Run requested tests */
      if (n1 == n2)
         convrt("(P1,'0For k = ',J0I4,', n = ',J0I8,"
            "', dist = (',A4,1H,A4,'), reps = ',J0I6)",
            &k, &n1, tkeys[t1-1], tkeys[t2-1], &reps, NULL);
      else
         convrt("(P1,'0For k = ',J0I4,', n1 = ',J0I8,', n2 = ',J0I8,"
            "', dist = (',A4,1H,A4,'), reps = ',J0I6)",
            &k, &n1, &n2, tkeys[t1-1], tkeys[t2-1], &reps, NULL);
      if (k > 1)
         convrt("(P1,' Default depth is ',J0Q12.3)", &dd, NULL);
      tmin = HUGE_X;

NextDepth:           /* Here to try another depth */
#if XTYPE != 0
      ie = mdksallod(&pwk, &depth, &depth, n1, n2, k);
#else
      ie = mdksallof(&pwk, &depth, &depth, n1, n2, k);
#endif
      if (ie > 0) abexit(ie+680);
      ie = mdksdivs(pwk, dret);
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

/* Run the repeated tests */
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
/* N.B.  If any DEBUG codes are turned on, timing
*  will incorrectly include time to execute them */
/*---------------------------------------------------------------------*
*    DEBUG 1 -- List two one-sided tests to compare with two-sided     *
*---------------------------------------------------------------------*/
#if SIDED != 1 && DEBUG & 1
{  
#if XTYPE != 0
   double s1 = mdks1sd(pwk, X1j, X2j);
   double s2 = mdks1sd(pwk, X2j, X1j);
#else
   double s1 = mdks1sf(pwk, X1j, X2j);
   double s2 = mdks1sf(pwk, X2j, X1j);
#endif
   convrt("(P1,4X,'For depth = ',J0I4,', repeat ',J0I8/"
      "6X,'s(X1) = ',Q10.6,', s(X2) = ',Q10.6,', mdks = ',Q10.6)",
      dret[0], &jr, &s1, &s2, s+jr, NULL);
   }
#endif      /* DEBUG 1 */
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
      convrt("(P1,4X,'Depth = ',J0I8,', mean s = ',Q8.6,"
         "', sigma s = ',Q8.6,', time/trial = ',Q12.6)",
         dret[0], &savg, &ssig, &ttot, NULL);

      /* If doing optimization, decrease depth to 1 or at least
      *  MNDOP steps and until time makes an uptick.  */
      if (doop && depth > 1) {
         if (--idct > 0 || ttot < topt) {
            topt = ttot; 
            depth -= 1;
            goto NextDepth;
            }
         }

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
