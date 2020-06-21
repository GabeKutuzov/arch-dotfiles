/* Test routine for itercirc */
/* Requires plot library and mfdraw */

#define MAIN
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "itercirc.h"
#include "plots.h"

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

   float rw,rh;
   float x0,x1,y0,y1,xi,yi;
   IterCirc C;
   int ngx,ngy;

   setmf(NULL, "localhost", "ITERCIRC TEST", NULL,
      1024, 0, 'B', 0, 0);
   setmovie(1,0,0);
   newplt(8.0, 8.0, 0, 0, "DEFAULT", "BLACK", "DEFAULT", 0);

   /* First plot -- Full square, circle not touching edges */
   x0 = 0.5, x1 = 3.5, y0 = 4.5, y1 = 7.5;
   ngx = 12, ngy = 12;
   xi = (x1 - x0)/(float)ngx, rw = 0.8*xi;
   yi = (y1 - y0)/(float)ngy, rh = 0.8*yi;

   makerect(x0, y0, x1, y1, ngx, ngy);
   symbol(x0, y0-0.3, 0.21, "3.6 unit circle", 0, 15);
   pencol("YELLOW");
   InitCircleIndexing(&C, 5, 7, 3.6, ngx, ngy);
   while (GetNextPointInCircle(&C)) {
      float fx = x0 + xi*(0.1 + (float)C.ix);
      float fy = y1 - yi*(0.9 + (float)C.iy);
      if (C.ioff != C.iy * ngx + C.ix)
         convrt("(P1,' T1, C.ioff = ',J1IL8,'wrong at x,y = (',"
            "J0IL8,1H,J0IL8,1H))", &C.ioff, &C.ix, &C.iy, NULL);
      rect(fx, fy, rw, rh, -1);
      }

   /* Second plot -- Circle overlapping rectangle */
   x0 = 4.5, x1 = 7.5, y0 = 4.5, y1 = 6.75;
   ngx = 10, ngy = 9;
   xi = (x1 - x0)/(float)ngx, rw = 0.8*xi;
   yi = (y1 - y0)/(float)ngy, rh = 0.8*yi;

   makerect(x0, y0, x1, y1, ngx, ngy);
   symbol(x0, y0-0.3, 0.21, "5.1 unit circle", 0, 15);
   pencol("YELLOW");
   InitCircleIndexing(&C, 5, 5, 5.1, ngx, ngy);
   while (GetNextPointInCircle(&C)) {
      float fx = x0 + xi*(0.1 + (float)C.ix);
      float fy = y1 - yi*(0.9 + (float)C.iy);
      if (C.ioff != C.iy * ngx + C.ix)
         convrt("(P1,' T2, C.ioff = ',J1IL8,'wrong at x,y = (',"
            "J0IL8,1H,J0IL8,1H))", &C.ioff, &C.ix, &C.iy, NULL);
      rect(fx, fy, rw, rh, -1);
      }

   /* Third plot -- Circle with overlapping at URHC */
   x0 = 0.5, x1 = 3.5, y0 = 0.5, y1 = 2.9;
   ngx = 15, ngy = 12;
   xi = (x1 - x0)/(float)ngx, rw = 0.8*xi;
   yi = (y1 - y0)/(float)ngy, rh = 0.8*yi;

   makerect(x0, y0, x1, y1, ngx, ngy);
   symbol(x0, y0-0.3, 0.21, "6.0 unit circle", 0, 15);
   pencol("YELLOW");
   InitCircleIndexing(&C, 11, 3, 6.0, ngx, ngy);
   while (GetNextPointInCircle(&C)) {
      float fx = x0 + xi*(0.1 + (float)C.ix);
      float fy = y1 - yi*(0.9 + (float)C.iy);
      if (C.ioff != C.iy * ngx + C.ix)
         convrt("(P1,' T3, C.ioff = ',J1IL8,'wrong at x,y = (',"
            "J0IL8,1H,J0IL8,1H))", &C.ioff, &C.ix, &C.iy, NULL);
      rect(fx, fy, rw, rh, -1);
      }

   /* Fourth plot -- Circle overlapping at LLHC */
   x0 = 4.5, x1 = 7.5, y0 = 0.5, y1 = 2.7;
   ngx = 16, ngy = 11;
   xi = (x1 - x0)/(float)ngx, rw = 0.8*xi;
   yi = (y1 - y0)/(float)ngy, rh = 0.8*yi;

   makerect(x0, y0, x1, y1, ngx, ngy);
   symbol(x0, y0-0.3, 0.21, "5.5 unit circle", 0, 15);
   pencol("YELLOW");
   InitCircleIndexing(&C, 3.2, 8.7, 5.5, ngx, ngy);
   while (GetNextPointInCircle(&C)) {
      float fx = x0 + xi*(0.1 + (float)C.ix);
      float fy = y1 - yi*(0.9 + (float)C.iy);
      if (C.ioff != C.iy * ngx + C.ix)
         convrt("(P1,' T4, C.ioff = ',J1IL8,'wrong at x,y = (',"
            "J0IL8,1H,J0IL8,1H))", &C.ioff, &C.ix, &C.iy, NULL);
      rect(fx, fy, rw, rh, -1);
      }

   cryout(RK_P1, "\0", RK_LN0+RK_FLUSH+1, NULL);
   endplt();
   cryout(RK_P1, "0End of itercirc test", RK_LN2+RK_FLUSH, NULL);
   return 0;
   }
