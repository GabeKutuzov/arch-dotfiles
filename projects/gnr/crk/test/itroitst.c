/* Test routine for iterroi */
/* Requires plot library and mfdraw */

#define MAIN
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "iterroi.h"
#include "plots.h"

#define MAIN

/* Make a rectangle and add grids */
static void makerect(float x0, float y0, float x1, float y1,
      int ngx, int ngy) {

   float xi,yi;
   int ig;     /* Grid index */

   pencol("GREEN");
   retrace(1);
   rect(x0, y0, x1-x0, y1-y0, 0);
   retrace(0);
   xi = (x1 - x0)/(float)ngx;
   yi = (y1 - y0)/(float)ngy;

   for (ig=1; ig<ngx; ++ig) {
      float xg = x0 + xi*(float)ig;
      line(xg,y0,xg,y1);
      }
   for (ig=1; ig<ngy; ++ig) {
      float yg = y0 + yi*(float)ig;
      line(x0,yg,x1,yg);
      }
   } /* End makerect() */

int main() {

   float x0,x1,y0,y1,xi,yi;
   float xp=0,yp=0;        /* Previous point */
   IterRoi R;
   int ngx,ngy;
   int kf;

   setmf(NULL, "localhost", "ITERROI TEST", NULL,
      1024, 0, '8', 0, 0);
   setmovie(1,0,0);
   newplt(8.0, 8.0, 0, 0, "DEFAULT", "BLACK", "DEFAULT", 0);

   /* First plot -- Full square, even sides */
   x0 = 0.5, x1 = 3.5, y0 = 4.5, y1 = 7.5;
   ngx = 12, ngy = 12;
   xi = (x1 - x0)/(float)ngx;
   yi = (y1 - y0)/(float)ngy;

   makerect(x0, y0, x1, y1, ngx, ngy);
   symbol(x0, y0-0.3, 0.21, "12 x 12 Full Square", 0, 19);
   pencol("YELLOW");
   InitOIRectIndexing(&R, 12, 12, 0, 0, 12, 12);
   kf = 1;
   while (GetNextPointInOIRect(&R)) {
      float fx = x0 + xi*(0.5 + (float)R.ix);
      float fy = y1 - yi*(0.5 + (float)R.iy);
      if (R.ioff != R.iy * ngx + R.ix)
         convrt("(P1,' T1, R.ioff = ',J1IL8,'wrong at x,y = (',"
            "J0IL8,1H,J0IL8,1H))", &R.ioff, &R.ix, &R.iy, NULL);
      if (!kf) line(xp,yp,fx,fy);
      xp = fx, yp = fy, kf = 0;
      }

   /* Second plot -- Rectangle with short, odd y axis */
   x0 = 4.5, x1 = 7.5, y0 = 4.5, y1 = 6.75;
   ngx = 12, ngy = 9;
   xi = (x1 - x0)/(float)ngx;
   yi = (y1 - y0)/(float)ngy;

   makerect(x0, y0, x1, y1, ngx, ngy);
   symbol(x0, y0-0.3, 0.21, "10 x 7 Rectangle", 0, 16);
   pencol("YELLOW");
   InitOIRectIndexing(&R, 12, 9, 2, 2, 10, 7);
   kf = 1;
   while (GetNextPointInOIRect(&R)) {
      float fx = x0 + xi*(0.5 + (float)R.ix);
      float fy = y1 - yi*(0.5 + (float)R.iy);
      if (R.ioff != R.iy * ngx + R.ix)
         convrt("(P1,' T2, R.ioff = ',J1IL8,'wrong at x,y = (',"
            "J0IL8,1H,J0IL8,1H))", &R.ioff, &R.ix, &R.iy, NULL);
      if (!kf) line(xp,yp,fx,fy);
      xp = fx, yp = fy, kf = 0;
      }

   /* Third plot -- Rectangle with short, odd x axis */
   x0 = 0.5, x1 = 3.5, y0 = 0.5, y1 = 2.9;
   ngx = 15, ngy = 12;
   xi = (x1 - x0)/(float)ngx;
   yi = (y1 - y0)/(float)ngy;

   makerect(x0, y0, x1, y1, ngx, ngy);
   symbol(x0, y0-0.3, 0.21, "5 x 8 Rectangle", 0, 15);
   pencol("YELLOW");
   InitOIRectIndexing(&R, 15, 12, 3, 2, 5, 8);
   kf = 1;
   while (GetNextPointInOIRect(&R)) {
      float fx = x0 + xi*(0.5 + (float)R.ix);
      float fy = y1 - yi*(0.5 + (float)R.iy);
      if (R.ioff != R.iy * ngx + R.ix)
         convrt("(P1,' T3, R.ioff = ',J1IL8,'wrong at x,y = (',"
            "J0IL8,1H,J0IL8,1H))", &R.ioff, &R.ix, &R.iy, NULL);
      if (!kf) line(xp,yp,fx,fy);
      xp = fx, yp = fy, kf = 0;
      }

   /* Fourth plot -- Just a single line */
   x0 = 4.5, x1 = 7.5, y0 = 0.5, y1 = 2.7;
   ngx = 15, ngy = 11;
   xi = (x1 - x0)/(float)ngx;
   yi = (y1 - y0)/(float)ngy;

   makerect(x0, y0, x1, y1, ngx, ngy);
   symbol(x0, y0-0.3, 0.21, "12 x 1 Rectangle", 0, 16);
   pencol("YELLOW");
   InitOIRectIndexing(&R, 15, 11, 2, 6, 12, 1);
   kf = 1;
   while (GetNextPointInOIRect(&R)) {
      float fx = x0 + xi*(0.5 + (float)R.ix);
      float fy = y1 - yi*(0.5 + (float)R.iy);
      if (R.ioff != R.iy * ngx + R.ix)
         convrt("(P1,' T4, R.ioff = ',J1IL8,'wrong at x,y = (',"
            "J0IL8,1H,J0IL8,1H))", &R.ioff, &R.ix, &R.iy, NULL);
      if (!kf) line(xp,yp,fx,fy);
      xp = fx, yp = fy, kf = 0;
      }

   cryout(RK_P1, "\0", RK_LN0+RK_FLUSH+1, NULL);
   endplt();
   cryout(RK_P1, "0End of iterroi test", RK_LN2, NULL);
   cryout(RK_P1, "\0", RK_LN0+RK_FLUSH+1, NULL);
   return 0;
   }
