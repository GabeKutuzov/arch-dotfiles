/* Test routine for iterpoly */
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
#include "iterpoly.h"
#include "plots.h"

#define  BIGPOLY  6

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

   xyf poly[BIGPOLY];
   IPolyWk work[BIGPOLY];
   float rw,rh;
   float x0,x1,y0,y1,xi,yi;
   IterPoly P;
   int ngx,ngy;

   atexit(endplt);
   setmf(NULL, "localhost", "ITERPOLY TEST", NULL,
      1024, 0, 'B', 0, 0);
   setmovie(1,0,0);
   newplt(8.0, 8.0, 0, 0, "DEFAULT", "BLACK", "DEFAULT", 0);

   /* First plot -- Quad not touching edges */
   x0 = 0.5, x1 = 3.5, y0 = 4.5, y1 = 7.5;
   ngx = 12, ngy = 12;
   xi = (x1 - x0)/(float)ngx, rw = 0.8*xi;
   yi = (y1 - y0)/(float)ngy, rh = 0.8*yi;

   poly[0].x = 2.3, poly[0].y = 1.6;
   poly[1].x = 3.1, poly[1].y = 10.3;
   poly[2].x = 8.4, poly[2].y = 7.9;
   poly[3].x = 9.8, poly[3].y = 2.9;

   makerect(x0, y0, x1, y1, ngx, ngy);
   symbol(x0, y0-0.3, 0.21, "Quad inside box", 0, 15);
   pencol("YELLOW");
   InitPolygonIndexing(&P, poly, work, ngx, ngy, 4);
   while (GetNextPointInPolygon(&P)) {
      float fx = x0 + xi*(0.1 + (float)P.ix);
      float fy = y1 - yi*(0.9 + (float)P.iy);
      if (P.ioff != P.iy * ngx + P.ix)
         convrt("(P1,' T1, P.ioff = ',J1IL8,'wrong at x,y = (',"
            "J0IL8,1H,J0IL8,1H))", &P.ioff, &P.ix, &P.iy, NULL);
      rect(fx, fy, rw, rh, -1);
      }

   /* Second plot -- Triangle with horizontal edge */
   x0 = 4.5, x1 = 7.5, y0 = 4.5, y1 = 6.75;
   ngx = 10, ngy = 9;
   xi = (x1 - x0)/(float)ngx, rw = 0.8*xi;
   yi = (y1 - y0)/(float)ngy, rh = 0.8*yi;

   poly[0].x = 2.3, poly[0].y = 2.0;
   poly[1].x = 4.7, poly[1].y = 8.0;
   poly[2].x = 8.7, poly[2].y = 2.0;

   makerect(x0, y0, x1, y1, ngx, ngy);
   symbol(x0, y0-0.3, 0.21, "Triangle w/horiz. edge", 0, 22);
   pencol("YELLOW");
   InitPolygonIndexing(&P, poly, work, ngx, ngy, 3);
   while (GetNextPointInPolygon(&P)) {
      float fx = x0 + xi*(0.1 + (float)P.ix);
      float fy = y1 - yi*(0.9 + (float)P.iy);
      if (P.ioff != P.iy * ngx + P.ix)
         convrt("(P1,' T2, P.ioff = ',J1IL8,'wrong at x,y = (',"
            "J0IL8,1H,J0IL8,1H))", &P.ioff, &P.ix, &P.iy, NULL);
      rect(fx, fy, rw, rh, -1);
      }

   /* Third plot -- Polygon with overlapping at URHC */
   x0 = 0.5, x1 = 3.5, y0 = 0.5, y1 = 2.9;
   ngx = 15, ngy = 12;
   xi = (x1 - x0)/(float)ngx, rw = 0.8*xi;
   yi = (y1 - y0)/(float)ngy, rh = 0.8*yi;

   poly[0].x = 2.3, poly[0].y = 1.6;
   poly[1].x = 12.2, poly[1].y = -3.3;
   poly[2].x = 16.0, poly[2].y = 7.9;
   poly[3].x = 9.8, poly[3].y = 10.4;
   poly[4].x = 4.6, poly[4].y = 9.7;

   makerect(x0, y0, x1, y1, ngx, ngy);
   symbol(x0, y0-0.3, 0.21, "Pentagon overlapping", 0, 20);
   pencol("YELLOW");
   InitPolygonIndexing(&P, poly, work, ngx, ngy, 5);
   while (GetNextPointInPolygon(&P)) {
      float fx = x0 + xi*(0.1 + (float)P.ix);
      float fy = y1 - yi*(0.9 + (float)P.iy);
      if (P.ioff != P.iy * ngx + P.ix)
         convrt("(P1,' T3, P.ioff = ',J1IL8,'wrong at x,y = (',"
            "J0IL8,1H,J0IL8,1H))", &P.ioff, &P.ix, &P.iy, NULL);
      rect(fx, fy, rw, rh, -1);
      }

   /* Fourth plot -- Overlap on two sides */
   x0 = 4.5, x1 = 7.5, y0 = 0.5, y1 = 4.0;
   ngx = 11, ngy = 13;
   xi = (x1 - x0)/(float)ngx, rw = 0.8*xi;
   yi = (y1 - y0)/(float)ngy, rh = 0.8*yi;

   poly[0].x = -1.3, poly[0].y = 1.6;
   poly[1].x = 12.2, poly[1].y = 3.3;
   poly[2].x = 16.0, poly[2].y = 7.9;
   poly[3].x =  6.8, poly[3].y = 10.4;
   poly[4].x = -4.6, poly[4].y = 9.7;

   makerect(x0, y0, x1, y1, ngx, ngy);
   symbol(x0, y0-0.3, 0.21, "2-side overlap", 0, 14);
   pencol("YELLOW");
   InitPolygonIndexing(&P, poly, work, ngx, ngy, 5);
   while (GetNextPointInPolygon(&P)) {
      float fx = x0 + xi*(0.1 + (float)P.ix);
      float fy = y1 - yi*(0.9 + (float)P.iy);
      if (P.ioff != P.iy * ngx + P.ix)
         convrt("(P1,' T4, P.ioff = ',J1IL8,'wrong at x,y = (',"
            "J0IL8,1H,J0IL8,1H))", &P.ioff, &P.ix, &P.iy, NULL);
      rect(fx, fy, rw, rh, -1);
      }

   /* Fifth test (no plot) -- Deliberate concave polygon error */
   cryout(RK_P1, "0Starting test with deliberate abexit 68.",
      RK_LN2, NULL);
   ngx = 16, ngy = 11;
   poly[0].x = 2.3, poly[0].y = 1.6;
   poly[1].x = 12.2, poly[1].y = -3.3;
   poly[2].x = 16.0, poly[2].y = 7.9;
   poly[3].x = 9.8, poly[3].y = 3.4;
   poly[4].x = 4.6, poly[4].y = 9.7;

   InitPolygonIndexing(&P, poly, work, ngx, ngy, 5);
   while (GetNextPointInPolygon(&P)) {
      if (P.ioff != P.iy * ngx + P.ix)
         convrt("(P1,' T4, P.ioff = ',J1IL8,'wrong at x,y = (',"
            "J0IL8,1H,J0IL8,1H))", &P.ioff, &P.ix, &P.iy, NULL);
      }

   cryout(RK_P1, "\0", RK_LN0+RK_FLUSH+1, NULL);
   endplt();
   cryout(RK_P1, "0End of iterpoly test", RK_LN2+RK_FLUSH, NULL);
   return 0;
   }
