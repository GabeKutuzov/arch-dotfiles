/* Test routine for itertape */
/* Requires plot library and mfdraw */

/* Notes:  Make gives a warning that can be ignored--calling atexit
*  with endplt, which does not return void.  This test gave correct
*  plots on 8/31/11--did not save metafile, but did save hand-plotted
*  results in ROCKS file in bottom desk drawer.  */

#define MAIN
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "itertape.h"
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

   xyf poly[NLENDS];
   float rw,rh;
   float x0,x1,y0,y1,xi,yi;
   IterTape T;
   int ngx,ngy;

   atexit(endplt);
   setmf(NULL, "localhost:0.1", "ITERTAPE TEST", NULL,
      1024, 0, 'B', 0, 0);
   setmovie(1,0,0);
   newplt(8.0, 8.0, 0, 0, "DEFAULT", "BLACK", "DEFAULT", 0);

   /* First plot -- Vertical tape not touching edges or grids */
   x0 = 0.5, x1 = 2.6, y0 = 4.0, y1 = 7.5;
   ngx = 12, ngy = 20;
   xi = (x1 - x0)/(float)ngx, rw = 0.8*xi;
   yi = (y1 - y0)/(float)ngy, rh = 0.8*yi;

   poly[0].x = 2.3, poly[0].y = 1.5;
   poly[1].x = 7.7, poly[1].y = 1.5;

   makerect(x0, y0, x1, y1, ngx, ngy);
   symbol(x0, y0-0.3, 0.21, "Tape inside box", 0, 15);
   pencol("YELLOW");
   InitTapeIndexing(&T, poly, poly+1, 4.0, ngx, ngy);
   while (GetNextPointOnTape(&T)) {
      float fx = x0 + xi*(0.1 + (float)T.ix);
      float fy = y1 - yi*(0.9 + (float)T.iy);
      if (T.ioff != T.iy * ngx + T.ix)
         convrt("(T1,' T1, T.ioff = ',J1IL8,'wrong at x,y = (',"
            "J0IL8,1H,J0IL8,1H))", &T.ioff, &T.ix, &T.iy, NULL);
      rect(fx, fy, rw, rh, -1);
      }
   pencol("ORANGE");
   ExtendTapeIndexing(&T, 6.4);
   while (GetNextPointOnTape(&T)) {
      float fx = x0 + xi*(0.1 + (float)T.ix);
      float fy = y1 - yi*(0.9 + (float)T.iy);
      if (T.ioff != T.iy * ngx + T.ix)
         convrt("(T1,' T1, T.ioff = ',J1IL8,'wrong at x,y = (',"
            "J0IL8,1H,J0IL8,1H))", &T.ioff, &T.ix, &T.iy, NULL);
      rect(fx, fy, rw, rh, -1);
      }
   pencol("RED");
   ExtendTapeIndexing(&T, 3.0);
   while (GetNextPointOnTape(&T)) {
      float fx = x0 + xi*(0.1 + (float)T.ix);
      float fy = y1 - yi*(0.9 + (float)T.iy);
      if (T.ioff != T.iy * ngx + T.ix)
         convrt("(T1,' T1, T.ioff = ',J1IL8,'wrong at x,y = (',"
            "J0IL8,1H,J0IL8,1H))", &T.ioff, &T.ix, &T.iy, NULL);
      rect(fx, fy, rw, rh, -1);
      }

   /* Second plot -- Horizontal tape with integer edges */
   x0 = 3.0, x1 = 7.5, y0 = 4.8, y1 = 7.5;
   ngx = 20, ngy = 12;
   xi = (x1 - x0)/(float)ngx, rw = 0.8*xi;
   yi = (y1 - y0)/(float)ngy, rh = 0.8*yi;

   poly[0].x = -1.5, poly[0].y = 10.0;
   poly[1].x = -1.5, poly[1].y = 3.0;

   makerect(x0, y0, x1, y1, ngx, ngy);
   symbol(x0, y0-0.3, 0.21, "Horizontal tape", 0, 15);
   pencol("YELLOW");
   InitTapeIndexing(&T, poly, poly+1, 4.0, ngx, ngy);
   while (GetNextPointOnTape(&T)) {
      float fx = x0 + xi*(0.1 + (float)T.ix);
      float fy = y1 - yi*(0.9 + (float)T.iy);
      if (T.ioff != T.iy * ngx + T.ix)
         convrt("(T1,' T1, T.ioff = ',J1IL8,'wrong at x,y = (',"
            "J0IL8,1H,J0IL8,1H))", &T.ioff, &T.ix, &T.iy, NULL);
      rect(fx, fy, rw, rh, -1);
      }
   pencol("ORANGE");
   ExtendTapeIndexing(&T, 5.5);
   while (GetNextPointOnTape(&T)) {
      float fx = x0 + xi*(0.1 + (float)T.ix);
      float fy = y1 - yi*(0.9 + (float)T.iy);
      if (T.ioff != T.iy * ngx + T.ix)
         convrt("(T1,' T1, T.ioff = ',J1IL8,'wrong at x,y = (',"
            "J0IL8,1H,J0IL8,1H))", &T.ioff, &T.ix, &T.iy, NULL);
      rect(fx, fy, rw, rh, -1);
      }
   pencol("RED");
   ExtendTapeIndexing(&T, 13.0);
   while (GetNextPointOnTape(&T)) {
      float fx = x0 + xi*(0.1 + (float)T.ix);
      float fy = y1 - yi*(0.9 + (float)T.iy);
      if (T.ioff != T.iy * ngx + T.ix)
         convrt("(T1,' T1, T.ioff = ',J1IL8,'wrong at x,y = (',"
            "J0IL8,1H,J0IL8,1H))", &T.ioff, &T.ix, &T.iy, NULL);
      rect(fx, fy, rw, rh, -1);
      }

   /* Third plot -- 45 degree slanted tape  */
   x0 = 0.5, x1 = 5.0, y0 = 0.5, y1 = 3.5;
   ngx = 18, ngy = 12;
   xi = (x1 - x0)/(float)ngx, rw = 0.8*xi;
   yi = (y1 - y0)/(float)ngy, rh = 0.8*yi;

   poly[0].x = -2.5, poly[0].y =  2.5;
   poly[1].x =  2.5, poly[1].y = -2.5;

   makerect(x0, y0, x1, y1, ngx, ngy);
   symbol(x0, y0-0.3, 0.21, "45 degree tape", 0, 14);
   pencol("YELLOW");
   InitTapeIndexing(&T, poly, poly+1, 4.0, ngx, ngy);
   while (GetNextPointOnTape(&T)) {
      float fx = x0 + xi*(0.1 + (float)T.ix);
      float fy = y1 - yi*(0.9 + (float)T.iy);
      if (T.ioff != T.iy * ngx + T.ix)
         convrt("(T1,' T1, T.ioff = ',J1IL8,'wrong at x,y = (',"
            "J0IL8,1H,J0IL8,1H))", &T.ioff, &T.ix, &T.iy, NULL);
      rect(fx, fy, rw, rh, -1);
      }
   pencol("ORANGE");
   ExtendTapeIndexing(&T, 6.4);
   while (GetNextPointOnTape(&T)) {
      float fx = x0 + xi*(0.1 + (float)T.ix);
      float fy = y1 - yi*(0.9 + (float)T.iy);
      if (T.ioff != T.iy * ngx + T.ix)
         convrt("(T1,' T1, T.ioff = ',J1IL8,'wrong at x,y = (',"
            "J0IL8,1H,J0IL8,1H))", &T.ioff, &T.ix, &T.iy, NULL);
      rect(fx, fy, rw, rh, -1);
      }
   pencol("RED");
   ExtendTapeIndexing(&T, 3.0);
   while (GetNextPointOnTape(&T)) {
      float fx = x0 + xi*(0.1 + (float)T.ix);
      float fy = y1 - yi*(0.9 + (float)T.iy);
      if (T.ioff != T.iy * ngx + T.ix)
         convrt("(T1,' T1, T.ioff = ',J1IL8,'wrong at x,y = (',"
            "J0IL8,1H,J0IL8,1H))", &T.ioff, &T.ix, &T.iy, NULL);
      rect(fx, fy, rw, rh, -1);
      }

   /* Fourth plot -- 30 degree slanted tape */
   x0 = 5.5, x1 = 7.5, y0 = 0.5, y1 = 4.5;
   ngx = 10, ngy = 20;
   xi = (x1 - x0)/(float)ngx, rw = 0.8*xi;
   yi = (y1 - y0)/(float)ngy, rh = 0.8*yi;

   poly[0].x = 6.5,       poly[0].y = 0.0;
   poly[1].x = 8.2320508, poly[1].y = 1.0;

   makerect(x0, y0, x1, y1, ngx, ngy);
   symbol(x0, y0-0.3, 0.21, "30 degree tape", 0, 14);
   pencol("YELLOW");
   InitTapeIndexing(&T, poly, poly+1, 5.3, ngx, ngy);
   while (GetNextPointOnTape(&T)) {
      float fx = x0 + xi*(0.1 + (float)T.ix);
      float fy = y1 - yi*(0.9 + (float)T.iy);
      if (T.ioff != T.iy * ngx + T.ix)
         convrt("(T1,' T1, T.ioff = ',J1IL8,'wrong at x,y = (',"
            "J0IL8,1H,J0IL8,1H))", &T.ioff, &T.ix, &T.iy, NULL);
      rect(fx, fy, rw, rh, -1);
      }
   pencol("ORANGE");
   ExtendTapeIndexing(&T, 6.4);
   while (GetNextPointOnTape(&T)) {
      float fx = x0 + xi*(0.1 + (float)T.ix);
      float fy = y1 - yi*(0.9 + (float)T.iy);
      if (T.ioff != T.iy * ngx + T.ix)
         convrt("(T1,' T1, T.ioff = ',J1IL8,'wrong at x,y = (',"
            "J0IL8,1H,J0IL8,1H))", &T.ioff, &T.ix, &T.iy, NULL);
      rect(fx, fy, rw, rh, -1);
      }
   pencol("RED");
   ExtendTapeIndexing(&T, 3.0);
   while (GetNextPointOnTape(&T)) {
      float fx = x0 + xi*(0.1 + (float)T.ix);
      float fy = y1 - yi*(0.9 + (float)T.iy);
      if (T.ioff != T.iy * ngx + T.ix)
         convrt("(T1,' T1, T.ioff = ',J1IL8,'wrong at x,y = (',"
            "J0IL8,1H,J0IL8,1H))", &T.ioff, &T.ix, &T.iy, NULL);
      rect(fx, fy, rw, rh, -1);
      }

   cryout(RK_P1, "\0", RK_LN0+RK_FLUSH+1, NULL);
   endplt();
   cryout(RK_P1, "0End of itertape test", RK_LN2+RK_FLUSH, NULL);
   return 0;
   }
