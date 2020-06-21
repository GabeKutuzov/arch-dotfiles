/* (c) Copyright 2002-2016, Neurosciences Research Foundation, Inc. */
/* $Id: rkgtest.c 20 2011-12-24 02:59:03Z  $ */
/***********************************************************************
*                              rkgtest.c                               *

$$$ THIS VERSION IS OBSOLETE -- USE VERSION in ~/src/oldplot 

*                                                                      *
*                 Test routine for ROCKS plot library                  *
*                                                                      *
*  N.B.  There are some plot commands that I wanted to test here but   *
*  they are not properly implemented in smarx mfdraw.  These are       *
*  enclosed in pound-if 0 so can be tested when new plot lib and       *
*  new mfdraw are available.                                           *
*                                                                      *
*  Version 1, 04/03/92, ROZ                                            *
*  Rev, 08/11/97, GNR - Remove refs to ROZ files, add ifdef PAR        *
*  V2A, 08/18/02, GNR - Test calls in new V2A library                  *
*  ==>, 02/21/03, GNR - Last mod before committing to svn repository   *
*  Rev, 10/10/11, GNR - Change ngraph calls --> newplot                *
*  Rev, 07/21/16, GNR - Revised for MPI environment                    *
***********************************************************************/

#define MAIN            /* Required by main progs using rocks routines */
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkarith.h"
#include "bapkg.h"
#include "plots.h"
#include "mpitools.h"
/* Need mempools for blkbcst->nncom->xlatptr->MI to load MI struct */
#include "mempools.h"

/* Define local debug options here */
#define MGD_START    1  /* Startup messages */
#define MGDBG MGD_START

/* Define size of plot (square for simple row/col assignment */
#define PLSIZE  8.0

#ifndef PARn
/*---------------------------------------------------------------------*
*                              DrawGrid                                *
*---------------------------------------------------------------------*/

static void DrawGrid(float boxxy, int numrow, int numcol) {

   int i;

   pencol("GREEN");
   for (i=0; i<=numcol; ++i) {   /* Draw verticals */
      float x = 0.5 + boxxy*(float)i;
      line(x,0.5,x,PLSIZE-0.5);
      }
   for (i=0; i<=numrow; ++i) {   /* Draw horizontals */
      float y = (PLSIZE-0.5) - boxxy*(float)i;
      line(0.5,y,PLSIZE-0.5,y);
      }

   } /* End DrawGrid() */
#endif


/*---------------------------------------------------------------------*
*                            Main program                              *
*---------------------------------------------------------------------*/

int main(void) {

   float boxxy,myx0,myy0;
#ifndef PARn
   char station[] = "";
#endif
#ifndef PAR0
   float xpoly[5] = { .15, .25, .25, .35, .35 };
   float ypoly[5] = { .15, .15, .25, .25, .35 };
   int   i;
   byte  bmpat[8] = { 0xA5, 0x5A, 0xA5, 0x5A,
                      0xcc, 0x33, 0xcc, 0x33 };
#endif
   int mcol;
   int myrow,mycol,relnode;
   int numrow,numcol;

#ifdef PAR
   aninit(ANI_MFSR|ANI_DBMSG, 0, 0);
#if MGDBG & MGD_START
   dbgprt(ssprintf(NULL,
      "rkgtest is alive node=%d proc=%d host=%d total=%d",
       NC.node, NC.procid, NC.hostid, NC.total));
#endif
   /* Determine how many little boxes to make for nodes */
   numcol = (int)ceil(sqrt((double)NC.cnodes));
   numrow = (int)ceil((double)NC.cnodes/(double)numcol);
#else
   NC.dim = -1;
   NC.node = 0;
   NC.total = 1;
   numrow = numcol = 1;
#endif

#ifdef PARn
   boxxy = (PLSIZE-1.0)/(float)(max(numrow,numcol));
   relnode = NC.node - NC.headnode;
   myrow = relnode/numcol;
   mycol = relnode - myrow*numcol;
   myx0 = 0.5 + boxxy*(float)mycol;
   myy0 = (PLSIZE-0.5) - boxxy*(float)(myrow+1);
   mcol = 100*NC.node/NC.total;
#else
   boxxy = (PLSIZE-1.0);
   relnode = 0;
   mycol = myrow = 0;
   myx0 = myy0 = 0.5;
   mcol = 50;
   settit("TITLE ROCKS plot library test");
   /* Use short buffer to exercise mfsr more */
   /* Test all combos of file only, graph only, or both */
#if 0
   setmf("rkgtest.mf", NULL, "RKG PLOT LIBRARY TEST", NULL,
      508, 0, 'A', 0, 0);
#else
   setmf(NULL, station, "RKG PLOT LIBRARY TEST", NULL,
      508, 0, 'A', 0, 0);
#endif
   setmovie(MM_STILL, 0, 0);
#endif

   /* First test graph */
   newplt(PLSIZE,PLSIZE,0,0,"SOLID","BLACK","DEFAULT",0);
#if defined(PAR) && MGDBG & MGD_START
   dbgprt("(rkgtest): First newplt call returned");
#endif
#ifndef PARn
   symbol(2.0,0.15,0.21,"Frame # 1",0.0,9);
   DrawGrid(boxxy, numrow, numcol);
#endif
#ifndef PAR0
   pencol("RED");
   number(myx0+0.25, myy0+0.25, 0.33, (double)NC.node, 30.0, -1);
   color(20+mcol);
   for (i=0;i<4;++i) {
      float y = myy0 + boxxy*(0.05 + 0.3*(float)i);
      arrow(myx0+boxxy*0.05,y,myx0+boxxy*0.15,y,boxxy*0.08,45.0);
      line(myx0+boxxy*0.2,y,myx0+boxxy*0.3,y);
      line(myx0+boxxy*0.2,y+boxxy*0.1,myx0+boxxy*0.3,y+boxxy*0.1);
      ellips(myx0+boxxy*0.7,y,boxxy*0.25,boxxy*0.2,0,0);
      }
#endif

   /* Second test graph */
   newplt(PLSIZE,PLSIZE,0,0,"SOLID","BLACK","DEFAULT",0);
#if defined(PAR) && MGDBG & MGD_START
   dbgprt("(rkgtest): Second newplt call returned");
#endif
#ifndef PARn
   symbol(2.0,0.15,0.21,"Frame # 2",0.0,9);
   DrawGrid(boxxy, numrow, numcol);
#endif
#ifndef PAR0
   color(20+mcol);
   /* plot() command not supported in smarx mfdraw */
#if 1
   line(myx0+boxxy*0.1,myy0+boxxy*0.1,myx0+boxxy*0.2,myy0+boxxy*0.2);
#else
   plot(myx0+boxxy*0.1,myy0+boxxy*0.1,PENUP);
   plot(myx0+boxxy*0.2,myy0+boxxy*0.2,PENDOWN);
#endif
   color(50+mcol);
   number(myx0+boxxy*0.1,myy0+boxxy*0.3,boxxy*0.08,
      (double)NC.node,0,-1);
   color(150+mcol);
#if 0
   /* plot() command not supported in smarx mfdraw */
   arc(myx0+boxxy*0.25,myy0+boxxy*0.3,
      myx0+boxxy*0.35,myy0+boxxy*0.35,45);
#endif
   color(150+mcol);
   rect(myx0+boxxy*0.4,myy0+boxxy*0.1,0.5*boxxy,0.4*boxxy,-1);
   color(130+mcol);
   circle(myx0+boxxy*0.6,myy0+boxxy*0.5,boxxy*0.35,0);
   color(10+mcol);
   circle(myx0+boxxy*0.9,myy0+boxxy*0.7,boxxy*0.25,-1);
#endif

   /* Third test graph */
   newplt(PLSIZE,PLSIZE,0,0,"SOLID","RED","DEFAULT",0);
#if defined(PAR) && MGDBG & MGD_START
   dbgprt("(rkgtest): Third newplt call returned");
#endif
#ifndef PARn
   symbol(2.0,0.15,0.21,"Frame # 3",0.0,9);
   DrawGrid(boxxy, numrow, numcol);
#endif
#ifndef PAR0
   pencol("YELLOW");
   axlin(myx0+boxxy*0.1, myy0+boxxy*0.4, "Axis tilted 20\x9c",
      boxxy*0.75, 20.0, 0.0, boxxy*0.1, 0.25, 0.5, boxxy*0.02, 0, 1, 2);
   retrace(2);
   for (i=0; i<5; ++i) {
      xpoly[i] = myx0 + boxxy*xpoly[i];
      ypoly[i] = myy0 + boxxy*ypoly[i];
      }
   /* smarx mfdraw apparently does not close the polyln
   *  and does not draw it in double thickness  */
   polyln(CLOSED_THICK, 5, xpoly, ypoly);
   retrace(1);
#endif

   /* Fourth test graph */
   newplt(PLSIZE,PLSIZE,0,0,"SOLID","RED","DEFAULT",0);
#if defined(PAR) && MGDBG & MGD_START
   dbgprt("(rkgtest): Fourth newplt call returned");
#endif
#ifndef PARn
   symbol(2.0,0.15,0.21,"Frame # 4",0.0,9);
   DrawGrid(boxxy, numrow, numcol);
#endif
#ifndef PAR0
   axlog(myx0+boxxy*0.6, myy0+boxxy*0.1, "Log axis test",
      boxxy*0.5, 110.0, 0.0, boxxy*0.1, 1.0, 10.0, 0.20,
      AX_TKCCW+AX_TKEXP, 1, 9);
#endif

   /* Fifth test graph */
   newplt(PLSIZE,PLSIZE,0,0,"SOLID","RED","DEFAULT",0);
#if defined(PAR) && MGDBG & MGD_START
   dbgprt("(rkgtest): Fifth newplt call returned");
#endif
#ifndef PARn
   symbol(2.0,0.15,0.21,"Frame # 5",0.0,9);
   DrawGrid(boxxy, numrow, numcol);
#endif
#ifndef PAR0
   axpol(myx0+boxxy*0.4, myy0+boxxy*0.4, "Polar axis test",
      boxxy*0.375, boxxy*0.0625, boxxy*0.125, 1.0, 2.0,
      45.0, boxxy*0.033, boxxy*0.025, boxxy*0.02, 8, 4,
      AX_LBLTOP, 1, 5);
#endif

   /* Sixth test graph */
   newplt(PLSIZE,PLSIZE,0,0,"SOLID","BLUE","DEFAULT",0);
#if defined(PAR) && MGDBG & MGD_START
   dbgprt("(rkgtest): Sixth newplt call returned");
#endif
#ifndef PARn
   symbol(2.0,0.15,0.21,"Frame # 6",0.0,9);
   DrawGrid(boxxy, numrow, numcol);
#endif
#ifndef PAR0
   pencol("RED");
   /* Here again we have an inadequate implementation of
   *  the spec in smarx mfdraw.  Apparently, even bitmaps()
   *  draws a 1:1 bitmap.  So here I expand each bit to a
   *  byte just to make something one can see.  */
#if 0
   {  byte xmpat[512];
      int jr,jc,jo,kk=1;
      for (jr=0; jr<512; jr+=64) {
         for (jc=0; jc<8; ++jc,++kk)
            xmpat[jr+jc] = bittst(bmpat, kk) ? 0xff : 0;
         for (jo=8; jo<64; jo+=8)
            memcpy(xmpat+jr+jo, xmpat+jr, 8);
         }
      bitmap(xmpat, 64*numcol, 64*numrow, 0.2*PLSIZE, 0.2*PLSIZE,
         64*mycol, 64*myrow, 64, 64, BM_BW|BM_NSME, GM_SET);
      }
#else
   bitmaps(bmpat, 8*numcol, 8*numrow, 0.2*PLSIZE, 0.2*PLSIZE,
      0.6*PLSIZE, 0.6*PLSIZE, 8*mycol, 8*myrow,
      8, 8, BM_BW|BM_NSME, GM_SET);
#endif
#endif

#if defined(PAR) && MGDBG & MGD_START
   dbgprt("(rkgtest): Sixth plot finished.");
#endif
   endplt();
#if defined(PAR) && MGDBG & MGD_START
   dbgprt("(rkgtest): endplt() returned, node finished.");
#endif

#ifdef PAR
   appexit(NULL,0,0);
#else
   return (0);
#endif
   } /* End main */

