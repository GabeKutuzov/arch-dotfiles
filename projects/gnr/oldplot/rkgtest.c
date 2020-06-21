/* (c) Copyright 2002-2017, The Rockefeller University *21114* */
/* $Id: rkgtest.c 34 2017-09-15 18:13:10Z  $ */
/***********************************************************************
*                              rkgtest.c                               *
*                                                                      *
*                 Test routine for ROCKS plot library                  *
*                                                                      *
*  N.B.  Calls that are commented out are not implemented in smarx     *
*     mfdraw.  Uncomment when available for test.  color() is accepted *
*     but has no effect.                                               *
************************************************************************
*  Version 1, 04/03/92, ROZ                                            *
*  Rev, 08/11/97, GNR - Remove refs to ROZ files, add ifdef PAR        *
*  V2A, 08/18/02, GNR - Test calls in new V2A library                  *
*  ==>, 02/21/03, GNR - Last mod before committing to svn repository   *
*  Rev, 10/10/11, GNR - Change ngraph calls --> newplot                *
*  Rev, 07/21/16, GNR - Revised for MPI environment                    *
*  Rev, 04/26/17, GNR - Rename oldtest to rkgtest, merge test/rkgtest.c*
*  Rev, 05/22/17, GNR - Add bitmap mode tests using readpng images,    *
*                       Merge in PAR changes from mpitools/test        *
*  Rev, 07/31/17, GNR - Add a geometry-controlled colored bitmap       *
*  Rev, 09/19/18, GMK - Added an eigth test graph,                     *
*                       Added dbgprt() to some existing plots          *
***********************************************************************/

#define MAIN            /* Required by main progs using rocks routines */
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sysdef.h"
#include "rkarith.h"
#ifdef PARn
#include "rksubs.h"
#else
#include "rocks.h"
#include "rkxtra.h"
#endif
#include "plots.h"
#include "images.h"
#include "mpitools.h"
#include "memshare.h"

/* Define size of plot (square for simple row/col assignment */
#define PLSIZE  8.0

/* Define message types */
#define RKGTEST_MSG  0x3218
#define RKGTMAP_MSG  0x3219

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

   imgdef *pimd;
   byte *pim5;
   float boxxy,myx0,myy0;
#ifndef PAR0
   float y,vv[6];
   float xpoly[5] = { .15, .25, .25, .35, .35 };
   float ypoly[5] = { .15, .15, .25, .25, .35 };
   int i;
   byte  bmpat[8] = { 0xA5, 0x5A, 0xA5, 0x5A,
                      0xcc, 0x33, 0xcc, 0x33 };
#endif
   int node /* ,hgreen */ ;
   int mcol;
   int myrow,mycol,relnode;
   int numrow,numcol;

#ifdef PAR
   aninit(ANI_DBMSG|ANI_MFSR, 0, 0);
   dbgprt(ssprintf(NULL,
      "rkgtest is alive node=%d proc=%d host=%d dim=%d \n",
       NC.node, NC.procid, NC.hostid, NC.dim));
   /* Determine how many little boxes to make for nodes */
   numcol = (int)ceil(sqrt((double)NC.cnodes));
   numrow = (int)ceil((double)NC.cnodes/(double)numcol);
#else
   NC.node = NC.dim = 0;
   NC.total = 1;
   numrow = numcol = 1;
#endif
   node = NC.node;

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
   cryout(RK_P1, "    ", RK_LN1, NULL);
   /* Use short buffer to exercise mfsr more (but bitmap() will fail! */
   /* Test all combos of file only, graph only, or both */
#if 1
   setmf("rkgtest.mf", "localhost:0", "RKG PLOT LIBRARY TEST", NULL,
      8000, 0, 'B', 0, 0);
#else  /** Other examples, pick one only **/
   setmf("rkgtest.mf", "localhost:0", "RKG PLOT LIBRARY TEST", NULL,
      8000, 0, 'B', 0, 0);
   setmf("rkgtest.mf", NULL, "RKG PLOT LIBRARY TEST", NULL,
      8000, 0, 'A', 0, 0);
   setmf(NULL, "localhost:0", "RKG PLOT LIBRARY TEST", NULL,
      8000, 0, 'A', 0, 0);
#endif
   setmovie(MM_STILL, 0, 0);
#endif

/*---------------------------------------------------------------------*
*                          First test graph                            *
*---------------------------------------------------------------------*/

   newplt(PLSIZE,PLSIZE,0,0,"SOLID","BLACK","DEFAULT",0);
   dbgprt("(rkgtest): First newplt call returned\n");   
/*    hgreen = ctsyn("GREEN");  */
#ifndef PARn
   symbol(2.0,0.15,0.21,"Frame # 1",0.0,9);
/*   ctscol(hgreen);  */
/*   symbc(0, " Text added by symbcxxx", 0, 20);  */
   DrawGrid(boxxy, numrow, numcol);
#endif
#ifndef PAR0
   pencol("RED");
   number(myx0+0.25, myy0+0.25, 0.33, (double)node+0.123, 30.0, 3);
/*   numbc(0, (double)(node+2), 60.0, 2);  */
   for (i=0;i<4;++i) {
      float y = myy0 + boxxy*(0.05 + 0.25*(float)i);
      arrow(myx0+boxxy*0.05,y,myx0+boxxy*0.15,y,boxxy*0.08,45.0);
      line(myx0+boxxy*0.2,y,myx0+boxxy*0.3,y);
      line(myx0+boxxy*0.2,y+boxxy*0.1,myx0+boxxy*0.3,y+boxxy*0.1);
/*      gmode(GM_XOR);  */
      ellips(myx0+boxxy*0.7,y,boxxy*0.2,boxxy*0.15,0,0);
      }
#endif

/*---------------------------------------------------------------------*
*                          Second test graph                           *
*---------------------------------------------------------------------*/

   newplt(PLSIZE,PLSIZE,0,0,"SOLID","BLACK","DEFAULT",0);
   dbgprt("(rkgtest): Second newplt call returned\n");
#ifndef PARn
   symbol(2.0,0.15,0.21,"Frame # 2",0.0,9);
   DrawGrid(boxxy, numrow, numcol);
#endif
#ifndef PAR0
   color(20+node*10);
   plot(myx0+boxxy*0.1,myy0+boxxy*0.1,PENUP);
   plot(myx0+boxxy*0.2,myy0+boxxy*0.2,PENDOWN);
   color(50+node*10);
   number(myx0+boxxy*0.1,myy0+boxxy*0.3,boxxy*0.08,
      (double)NC.node,0,-1);
   color(150+node*10);
/*   arc(myx0+boxxy*0.25,myy0+boxxy*0.3,
      myx0+boxxy*0.35,myy0+boxxy*0.35,45);  */
   color(150+node*10);
   rect(myx0+boxxy*0.4,myy0+boxxy*0.1,0.5*boxxy,0.4*boxxy,-1);
   color(130+node*10);
   circle(myx0+boxxy*0.6,myy0+boxxy*0.5,boxxy*0.35,0);
   color(10+node*10);
   circle(myx0+boxxy*0.8,myy0+boxxy*0.7,boxxy*0.25,-1);
#endif

/*---------------------------------------------------------------------*
*                          Third test graph                            *
*---------------------------------------------------------------------*/

   newplt(PLSIZE,PLSIZE,0.0,0.0,"SOLID","RED","DEFAULT",0);
   dbgprt("(rkgtest): Third newplt call returned\n");
#ifndef PARn
   symbol(2.0,0.15,0.21,"Frame # 3",0.0,9);
   DrawGrid(boxxy, numrow, numcol);
#endif
#ifndef PAR0
   pencol("YELLOW");
   axlin(myx0+boxxy*0.1, myy0+boxxy*0.4, "Axis tilted 20\x9c",
      boxxy*0.75, 20.0, 0.0, boxxy*0.1, 0.25, 0.5, boxxy*0.02, 0, 1, 2);
   axlog(myx0+boxxy*0.8, myy0+boxxy*0.1, "Log axis test", boxxy*0.5,
      110.0, 0.0, boxxy*0.1, 1.0, 10.0, 0.20, AX_TKCCW+AX_TKEXP, 1, 9);
/*   vv[VXX] = 1.0, vv[VXY] = 0.0, vv[VXC] = 2.0;
   vv[VYX] = 0.0, vv[VYY] = 1.0, vv[VYC] = 2.0;
   framec(1, vv);  */
   axpol(myx0+boxxy*0.4, myy0+boxxy*0.4, "Polar axis test",
      boxxy*0.375, boxxy*0.0625, boxxy*0.125, 1.0, 1.2, 45.0,
      boxxy*0.033, boxxy*0.025, 0.16, 8, 4, AX_LBLTOP, 1, 5);
/*   gobjid(1009);  */
/*   polygn(2.0, 1.0, 2.5, 12, 15.0, 0.8, 1.1, 0);  */
/*   frame(0);  */
   for (i=0; i<5; ++i) {
      xpoly[i] = myx0 + boxxy*xpoly[i];
      ypoly[i] = myy0 + boxxy*ypoly[i];
      }
   retrace(2);
   polyln(CLOSED_THICK, 5, xpoly, ypoly);
   retrace(0);
#endif

/*---------------------------------------------------------------------*
*                          Fourth test graph                           *
*---------------------------------------------------------------------*/

   newplt(PLSIZE,PLSIZE,0,0,"SOLID","BLUE","DEFAULT",0);
   dbgprt("(rkgtest): Fourth newplt call returned");
#ifndef PARn
   symbol(2.0,0.15,0.21,"Frame # 4",0.0,9);
   DrawGrid(boxxy, numrow, numcol);
#endif
#ifndef PAR0
   pencol("RED");
   bitmaps(bmpat, 8*numcol, 8*numrow, 0.2*PLSIZE, 0.2*PLSIZE,
      0.6*PLSIZE, 0.6*PLSIZE, 8*mycol, 8*myrow,
      8, 8, BM_BW|BM_NSME, GM_SET);
#endif

/*---------------------------------------------------------------------*
*              Fifth test graph -- colored image (8 bit)               *
*---------------------------------------------------------------------*/

#define IM5WID   300
#define IM5HGT   400
#define STRIPE    15
#define BRIGHT   255
#define oRed       0
#define oGrn       1
#define oBlu       2

   newplt(7.0,6.0,0.0,0.0,"SOLID","RED","DEFAULT",0);
   dbgprt("(rkgtest): Fifth newplt call returned\n");
   /* Allocate enough for tests 5 and 6 */
   pim5 = callocv(3*IM5WID*IM5HGT, HSIZE, "Images 5&6");
   {
#ifdef PAR
      div_t ihpn;       /* Image height per node */
      int yoff, iht;
#endif
      int rgap = IM5WID*(IM5HGT-STRIPE);
      int cgap = IM5WID-STRIPE;
      int ir,ic;
      memset(pim5, 0, 3*HSIZE*IM5WID*IM5HGT);
      /* Not the fastest code here... */
      /* Make a red stripe around the outside */
      for (ir=0; ir<STRIPE; ++ir) {
         int trow = ir*IM5WID;
         int brow = trow + rgap;
         for (ic=0; ic<IM5WID; ++ic) {
            pim5[3*(trow+ic)+oRed] =  BRIGHT;
            pim5[3*(brow+ic)+oRed] =  BRIGHT;
            }
         }
      for (ir=STRIPE; ir<IM5HGT-STRIPE; ++ir) {
         int lrow = ir*IM5WID;
         int rrow = lrow + cgap; 
         for (ic=0; ic<STRIPE; ++ic) {
            pim5[3*(lrow+ic)+oRed] =  BRIGHT;
            pim5[3*(rrow+ic)+oRed] =  BRIGHT;
            }
         }
      /* Make a green stripe inside the red stripe */
      rgap -= 4*IM5WID*STRIPE;
      cgap -= 4*STRIPE;
      for (ir=2*STRIPE; ir<3*STRIPE; ++ir) {
         int trow = ir*IM5WID;
         int brow = trow + rgap;
         for (ic=2*STRIPE; ic<IM5WID-3*STRIPE; ++ic) {
            pim5[3*(trow+ic)+oGrn] =  BRIGHT;
            pim5[3*(brow+ic)+oGrn] =  BRIGHT;
            }
         }
      for (ir=2*STRIPE; ir<IM5HGT-2*STRIPE; ++ir) {
         int lrow = ir*IM5WID;
         int rrow = lrow + cgap; 
         for (ic=2*STRIPE; ic<3*STRIPE; ++ic) {
            pim5[3*(lrow+ic)+oGrn] =  BRIGHT;
            pim5[3*(rrow+ic)+oGrn] =  BRIGHT;
            }
         }
      /* Make a blue stripe inside the green stripe */
      rgap -= 4*IM5WID*STRIPE;
      cgap -= 4*STRIPE;
      for (ir=4*STRIPE; ir<5*STRIPE; ++ir) {
         int trow = ir*IM5WID;
         int brow = trow + rgap;
         for (ic=4*STRIPE; ic<IM5WID-5*STRIPE; ++ic) {
            pim5[3*(trow+ic)+oBlu] =  BRIGHT;
            pim5[3*(brow+ic)+oBlu] =  BRIGHT;
            }
         }
      for (ir=4*STRIPE; ir<IM5HGT-4*STRIPE; ++ir) {
         int lrow = ir*IM5WID;
         int rrow = lrow + cgap; 
         for (ic=4*STRIPE; ic<5*STRIPE; ++ic) {
            pim5[3*(lrow+ic)+oBlu] =  BRIGHT;
            pim5[3*(rrow+ic)+oBlu] =  BRIGHT;
            }
         }
#ifdef PAR
      /* At this point, all nodes should have a copy of the image */
      ihpn = div(IM5HGT, NC.total);
      iht = ihpn.quot + (node < ihpn.rem);
      yoff = node*ihpn.quot + min(node, ihpn.rem);
      bitmap(pim5, IM5WID, IM5HGT, 1.0, 1.0,
         0, yoff, IM5WID, iht, BM_C24, GM_SET);
#ifndef PARn
      symbol(0.5,0.5,0.25,"Frame # 5",0.0,9);
#endif
#else
      /* Serial plotting for Image 5 */
      bitmap(pim5, 2*IM5WID, IM5HGT, 1.0, 1.0,
         0, 0, IM5WID, IM5HGT/2, BM_C24|BM_NSME, GM_SET);
      bitmap(pim5+3*IM5WID*(IM5HGT/2), 2*IM5WID, IM5HGT, 1.0, 1.0,
         IM5WID/2, IM5HGT/2, IM5WID, IM5HGT/2, BM_C24|BM_NSME, GM_SET);
      symbol(0.5,0.5,0.25,"Frame # 5",0.0,9);
#endif
      } /* End graph 5 local scope */

/*---------------------------------------------------------------------*
*             Sixth test graph -- colored image (16 bit)               *
*---------------------------------------------------------------------*/

#define BRT16 32000

   newplt(7.0,6.0,0.0,0.0,"SOLID","RED","DEFAULT",0);
   dbgprt("(rkgtest): Sixth newplt call returned\n");
   {  ui16 *pim6 = (ui16 *)pim5;
#ifdef PAR
      div_t ihpn;       /* Image height per node */
      int yoff, iht;
#endif
      int rgap = IM5WID*(IM5HGT-STRIPE);
      int cgap = IM5WID-STRIPE;
      int ir,ic;
      memset(pim6, 0, 3*HSIZE*IM5WID*IM5HGT);
      /* Not the fastest code here... */
      /* Make a red stripe around the outside */
      for (ir=0; ir<STRIPE; ++ir) {
         int trow = ir*IM5WID;
         int brow = trow + rgap;
         for (ic=0; ic<IM5WID; ++ic) {
            pim6[3*(trow+ic)+oRed] =  BRT16;
            pim6[3*(brow+ic)+oRed] =  BRT16;
            }
         }
      for (ir=STRIPE; ir<IM5HGT-STRIPE; ++ir) {
         int lrow = ir*IM5WID;
         int rrow = lrow + cgap; 
         for (ic=0; ic<STRIPE; ++ic) {
            pim6[3*(lrow+ic)+oRed] =  BRT16;
            pim6[3*(rrow+ic)+oRed] =  BRT16;
            }
         }
      /* Make a green stripe inside the red stripe */
      rgap -= 4*IM5WID*STRIPE;
      cgap -= 4*STRIPE;
      for (ir=2*STRIPE; ir<3*STRIPE; ++ir) {
         int trow = ir*IM5WID;
         int brow = trow + rgap;
         for (ic=2*STRIPE; ic<IM5WID-3*STRIPE; ++ic) {
            pim6[3*(trow+ic)+oGrn] =  BRT16;
            pim6[3*(brow+ic)+oGrn] =  BRT16;
            }
         }
      for (ir=2*STRIPE; ir<IM5HGT-2*STRIPE; ++ir) {
         int lrow = ir*IM5WID;
         int rrow = lrow + cgap; 
         for (ic=2*STRIPE; ic<3*STRIPE; ++ic) {
            pim6[3*(lrow+ic)+oGrn] =  BRT16;
            pim6[3*(rrow+ic)+oGrn] =  BRT16;
            }
         }
      /* Make a blue stripe inside the green stripe */
      rgap -= 4*IM5WID*STRIPE;
      cgap -= 4*STRIPE;
      for (ir=4*STRIPE; ir<5*STRIPE; ++ir) {
         int trow = ir*IM5WID;
         int brow = trow + rgap;
         for (ic=4*STRIPE; ic<IM5WID-5*STRIPE; ++ic) {
            pim6[3*(trow+ic)+oBlu] =  BRT16;
            pim6[3*(brow+ic)+oBlu] =  BRT16;
            }
         }
      for (ir=4*STRIPE; ir<IM5HGT-4*STRIPE; ++ir) {
         int lrow = ir*IM5WID;
         int rrow = lrow + cgap; 
         for (ic=4*STRIPE; ic<5*STRIPE; ++ic) {
            pim6[3*(lrow+ic)+oBlu] =  BRT16;
            pim6[3*(rrow+ic)+oBlu] =  BRT16;
            }
         }
#ifdef PAR
      /* At this point, all nodes should have a copy of the image */
      ihpn = div(IM5HGT, NC.total);
      iht = ihpn.quot + (node < ihpn.rem);
      yoff = node*ihpn.quot + min(node, ihpn.rem);
      bitmap(pim6, IM5WID, IM5HGT, 1.0, 1.0,
         0, yoff, IM5WID, iht, BM_C48, GM_SET);
#ifndef PARn
      symbol(0.5,0.5,0.25,"Frame # 5",0.0,9);
#endif
#else
      /* Serial plotting for Image 5 */
      bitmap(pim6, 2*IM5WID, IM5HGT, 1.0, 1.0,
         0, 0, IM5WID, IM5HGT/2, BM_C48|BM_NSME, GM_SET);
      bitmap(pim6+3*IM5WID*(IM5HGT/2), 2*IM5WID, IM5HGT, 1.0, 1.0,
         IM5WID/2, IM5HGT/2, IM5WID, IM5HGT/2, BM_C48|BM_NSME, GM_SET);
      symbol(0.5,0.5,0.25,"Frame # 6",0.0,9);
#endif
      } /* End graph 6 local scope */
   freev(pim5, "Images 5&6");

/*---------------------------------------------------------------------*
*                         Seventh test graph                           *
*---------------------------------------------------------------------*/

   newplt(11.0,8.0,0.0,0.0,"SOLID","RED","DEFAULT",0);
   dbgprt("(rkgtest): Seventh newplt call returned\n");
#ifndef PARn
   pimd = readpng("/usr/share/doc/qt4/html/images/"
      "mandelbrot-example.png", IMG_8BIT);
   if (pimd->errex == IER_OPEN)
      pimd = readpng("/opt/MatlabR2016b/toolbox/images/imdata/"
         "coins.png", IMG_8BIT);   
   if (pimd->errex == IER_OPEN)
      pimd = readpng("1280x1024_dawn.png",
         IMG_8BIT);
   if (pimd->errex == IER_OPEN)
      pimd = readpng("earth_800x800.png",
         IMG_ERRX|IMG_8BIT);
/*   if (pimd->imgfmt == BM_GS) pimd->imgfmt = BM_C8;   /*** DEBUG ***/
#endif
#ifdef PAR
   /* Use a homemade nxdr table to send the data to nodes */
   {  long IMGPLTT[] = {
      sizeof(imgdef),
      (2 << 8) | 'p',
      (2 << 8) | 'i',
      (4 << 8) | 'm',
      (1 << 8) | 'I', };
      size_t imgsz;
      div_t ihpn;          /* Image height per node */
      int yoff, iht;
#ifdef PARn
      pimd = mallocv(sizeof(imgdef), "imgdef copy");
#else
      /* On Node 0 need to copy pimd->pimg and NULL out pointers for
      *  blkbcst */
      void *pimg2 = pimd->pimg;
      pimd->pimg = NULL; pimd->pprow = NULL;
#endif
      blkbcst(pimd, IMGPLTT, 1, RKGTEST_MSG);
      imgsz = (size_t)pimd->height * (size_t)pimd->width;
#ifdef PARn
      pimd->pimg = mallocv(imgsz, "image");
#else
      pimd->pimg = pimg2;     /* Restore image ptr */
#endif
      blkbcst(pimd->pimg, NULL, imgsz, RKGTMAP_MSG);
      /* At this point, all nodes should have a copy of the image */
      ihpn = div(pimd->height, NC.total);
      iht = ihpn.quot + (node < ihpn.rem);
      yoff = node*ihpn.quot + min(node, ihpn.rem);
      bitmap(pimd->pimg, pimd->width, pimd->height, 1.0, 1.0,
         0, yoff, pimd->width, iht, pimd->imgfmt, GM_SET);
#ifdef PARn
      freev(pimd->pimg, "image");
      freev(pimd, "imgdef copy");
#else
      symbol(0.5,0.5,0.25,"Frame # 6",0.0,9);
      freepng(pimd);
#endif
      } /* End graph 5 local scope */
#else
   bitmap(pimd->pimg, pimd->width, pimd->height, 1.0, 1.0,
      0, 0, pimd->width, pimd->height, pimd->imgfmt, GM_SET);
   symbol(0.5,0.5,0.25,"Frame # 7",0.0,9);
   freepng(pimd);
#endif

/*---------------------------------------------------------------------*
*                          Eigth test graph                            *
*---------------------------------------------------------------------*/

   newplt(PLSIZE,PLSIZE,0,0,"SOLID","BLACK","DEFAULT",0);
   dbgprt("(rkgtest): Eigth newplt call returned\n");
#ifndef PARn
   symbol(2.0,0.15,0.21,"Frame # 8",0.0,9);
   DrawGrid(boxxy, numrow, numcol);
#endif
#ifndef PAR0
   pencol("BLUE");
   plot(myx0+boxxy*0,myy0+boxxy*0,PENUP);
   plot(myx0+boxxy*1,myy0+boxxy*1,PENDOWN);
   plot(myx0+boxxy*1,myy0+boxxy*0,PENUP);
   plot(myx0+boxxy*0, myy0+boxxy*1,PENDOWN);
   float xOff = 0.15; /* x offset */
   float yOff = 0.15; /* y offset */
   int xi,yi,itr;
   for(yi = 0; yi < 7; yi++){
      for(xi = 0; xi < 7; xi++){
         itr++;
         if(itr%2 == 0) pencol("RED"); /* set color to red if iteration is even */
         else pencol("BLUE");          /* set color to blue if iteration is odd */
         rect(myx0+boxxy*xOff*xi,myy0+boxxy*yOff*yi,0.1*boxxy,0.1*boxxy,1);
         pencol("WHITE");
            number(myx0+boxxy*xOff*xi+xOff/2,myy0+boxxy*yOff*yi+yOff/2,0.25,
      (double)itr,0,-1);
      }
   }
#endif

/*---------------------------------------------------------------------*
*                         Finish and clean up                          *
*---------------------------------------------------------------------*/

   endplt();
   dbgprt("(rkgtest): Node finished.");
#ifndef PARn
   cryocls();
#endif
#ifdef PAR
   appexit(NULL, 0, 0);
#else
   return 0;
#endif
   } /* End main */