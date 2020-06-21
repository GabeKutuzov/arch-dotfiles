/* (c) Copyright 2002, Neurosciences Research Foundation, Inc. */
/***********************************************************************
*                              rkgtest.c                               *
*                                                                      *
*                 Test routine for ROCKS plot library                  *
*                                                                      *
*  Version 1, 04/03/92, ROZ                                            *
*  Rev, 08/11/97, GNR - Remove refs to ROZ files, add ifdef PAR        *
*  V2A, 08/18/02, GNR - Test calls in new V2A library                  *
***********************************************************************/

#define MAIN            /* Required by main progs using rocks routines */
#include <stdio.h>
#include <stddef.h>
#include "sysdef.h"
#include "rocks.h"
#include "plots.h"
#include "nsitools.h"

int main(void) {

   float y,vv[6];
#ifndef PARn
   float xpoly[5] = { 1.5, 2.5, 2.5, 3.5, 3.5 };
   float ypoly[5] = { 1.5, 1.5, 2.5, 2.5, 3.5 };
#endif
   int i,node,hgreen;

#ifdef PAR
   int proc,host,dim;

   whoami(&node,&proc,&host,&dim);
   dbgprt(ssprintf(NULL,
      "rkgtest is alive node=%d proc=%d host=%d dim=%d \n",
       node,proc,host,dim));
#else
   node = 0;
#endif

#ifndef PARn
   settit("TITLE ROCKS plot library test");
   setmf("rkgtest.mf", "", "RKG PLOT LIBRARY TEST", NULL,
      1020, 0, 'B', 0, 0);
   setmovie(MM_STILL, 0, 0);
#endif

   /* First test graph */
   newplt(8.0,6.0,1.0,0.5,"SOLID","BLACK","DEFAULT",0);
   dbgprt("(rkgtest): First newplt call returned\n");
   hgreen = ctsyn("GREEN");
   color(20+node*10);
   symbol(2.0,(float)node,0.5,"Frame # 1",0.0,9);
   for (i=0;i<4;++i) {
      y = 0.1 + 0.1*(float)i + 0.5*(float)node;
      color(50+node*10);
      arrow(1.0,y,2.0,y,0.25,45.0);
      color(40+node*10);
      line(2.5,y,3.5,y);
      color(30+node*10);
      gmode(GM_XOR);
      line(2.5,y,3.5,y);
      color(20+node*10);
      ellips(5.0,y,0.5,0.2,0,0);
      }
   ctscol(hgreen);
   symbc(0, " Text added by symbcxxx", 0, 20);
   number(2.0, 0.25+(float)node, 0.33, (double)node, 30.0, 0);
   numbc(0, (double)(node+2), 60.0, 2);
   /* Second test graph */
   newplt(14.0,14.0,0,0,"SOLID","BLACK","DEFAULT",0);
   dbgprt("(rkgtest): Second newplt call returned\n");
   y = 0.25 + (float)node;
   color(20+node*10);
   plot(1.0,y,PENUP);
   plot(2.0,y,PENDOWN);
   color(50+node*10);
   symbol(9.0,y,0.5,"Frame # 2",0.0,9);
   color(150+node*10);
   arc(2.5,y,3.5,y,45);
   color(150+node*10);
   rect(4.0,y,1.0,1.0,-1);
   color(130+node*10);
   circle(6.5,y,0.5,0);
   color(10+node*10);
   circle(9.0,y,0.5,-1);

   /* Third test graph */
   newplt(8.0,8.0,0.0,0.0,"SOLID","RED","DEFAULT",0);
#ifndef PARn
   axlin(1.0, 1.0, "Axis tilted 20\x9c", 6.0, 20.0,
      0.0, 1.0, 0.25, 0.5, 0.20, 0, 1, 5);
   axlog(1.0, 1.0, "Log axis test", 5.0, 110.0,
      0.0, 1.0, 1.0, 10.0, 0.20, AX_TKCCW+AX_TKEXP, 1, 9);
   vv[VXX] = 1.0, vv[VXY] = 0.0, vv[VXC] = 2.0;
   vv[VYX] = 0.0, vv[VYY] = 1.0, vv[VYC] = 2.0;
   framec(1, vv);
   axpol(2.0, 2.0, "Polar axis test", 2.0, 0.5, 0.5, 1.0, 1.2,
      45.0, 0.33, 0.25, 0.16, 8, 4, AX_LBLTOP, 1, 5);
   /* bitmap(); */
   gobjid(1009);
   polygn(2.0, 1.0, 2.5, 12, 15.0, 0.8, 1.1, 0);
   frame(0);
   retrace(2);
   polyln(CLOSED_THICK, 5, xpoly, ypoly);
#endif
   dbgprt("(rkgtest): Third newplt call returned\n");
   endplt();
   dbgprt("(rkgtest): Node finished.");
   return 0;
   } /* End main */

