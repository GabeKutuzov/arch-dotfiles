/* Test routine for iterpgfg */
/* Requires plot library and mfdraw */

/* Notes:  Make gives a warning that can be ignored--calling atexit
*  with endplt, which does not return void.  */

#define MAIN
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "iterpgfg.h"
#include "plots.h"

#define  BIGPOLY  13

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
   IPgfgWk work[BIGPOLY];
   float rw,rh;
   float x0,x1,y0,y1,xi,yi;
   IterPgfg P;
   int ngx,ngy;
   int ns;

   atexit(endplt);
   setmf(NULL, "localhost", "ITERPGFG TEST", NULL,
      1024, 0, 'B', 0, 0);
   setmovie(1,0,0);

/* Repeat the whole thing twice with internal,external marking */

for (ns=1; ns>-3; ns-=2) {

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
   InitPgfgIndexing(&P, poly, work, ngx, ngy, 4*ns);
   while (GetNextPointInPgfg(&P)) {
      float fx = x0 + xi*(0.1 + (float)P.ix);
      float fy = y1 - yi*(0.9 + (float)P.iy);
#if 0
      convrt("(P1,' Point returned is (',J0IL8,1H,J0IL8,1H))",
         &P.ix,&P.iy,NULL);
#endif
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
   InitPgfgIndexing(&P, poly, work, ngx, ngy, 3*ns);
   while (GetNextPointInPgfg(&P)) {
      float fx = x0 + xi*(0.1 + (float)P.ix);
      float fy = y1 - yi*(0.9 + (float)P.iy);
      if (P.ioff != P.iy * ngx + P.ix)
         convrt("(P1,' T2, P.ioff = ',J1IL8,'wrong at x,y = (',"
            "J0IL8,1H,J0IL8,1H))", &P.ioff, &P.ix, &P.iy, NULL);
      rect(fx, fy, rw, rh, -1);
      }

   /* Third plot -- Concave figure not simply connected */
   x0 = 0.5, x1 = 3.5, y0 = 0.5, y1 = 2.9;
   ngx = 15, ngy = 12;
   xi = (x1 - x0)/(float)ngx, rw = 0.8*xi;
   yi = (y1 - y0)/(float)ngy, rh = 0.8*yi;

   poly[0].x = -2.1, poly[0].y = 1.0;
   poly[1].x =  3.3, poly[1].y = 12.3;
   poly[2].x =  6.0, poly[2].y = 7.9;
   poly[3].x =  9.8, poly[3].y = 7.9;
   poly[4].x =  7.6, poly[4].y = 5.0;
   poly[5].x = 10.0, poly[5].y = PgfgPly(1.0,ngy);
   poly[6].x =  1.0, poly[6].y = 2.0;
   poly[7].x =  3.1, poly[7].y = 5.0;
   poly[8].x =  1.0, poly[8].y = PgfgPly(6.5,ngy);
   poly[9].x =  9.0, poly[9].y = 9.0;
   poly[10].x = 16.0,poly[10].y = 9.0;
   poly[11].x = 16.5,poly[11].y = 11.0;
   poly[12].x = 9.0, poly[12].y = 11.0;

   makerect(x0, y0, x1, y1, ngx, ngy);
   symbol(x0, y0-0.3, 0.21, "Pentagon overlapping", 0, 20);
   pencol("YELLOW");
   InitPgfgIndexing(&P, poly, work, ngx, ngy, 13*ns);
#if 0
   convrt("(P1,'0ix=',J0IL6,', iy=',J0IL6,', bs=',J0I6,"
      "', smnx=',J0B25IJ8.4,', smxx=',J0B25IJ8.4)",
      &P.ix,&P.iy,&P.bs,&P.smnx,&P.smxx,NULL);
   {  IPgfgWk *pwe;
      for (pwe=P.wkel; pwe; pwe=pwe->pne) {
         convrt("(P1,'   Line (',J0B25IJ8.4,1H,J0B25IJ8.4,') to (',"
            "J0B25IJ8.4,1H,J0B25IJ8.4,') has rix=',J0B25IJ8.4,' and "
            " slope=',J0B25I8.5)", &pwe->slx, &pwe->sly, &pwe->shx,
            &pwe->shy, &pwe->rix, &pwe->slope, NULL);
         }
      cryout(RK_P1, "\0", RK_LN0+RK_FLUSH+1, NULL);
      }
#endif
   while (GetNextPointInPgfg(&P)) {
      float fx = x0 + xi*(0.1 + (float)P.ix);
      float fy = y1 - yi*(0.9 + (float)P.iy);
#if 0
   {  IPgfgWk *pwe;
      convrt("(P1,' Point returned is (',J0IL8,1H,J0IL8,1H))",
         &P.ix,&P.iy,NULL);
      for (pwe=P.wkal; pwe; pwe=pwe->pne) {
         convrt("(P1,'   Line (',J0B25IJ8.4,1H,J0B25IJ8.4,') to (',"
            "J0B25IJ8.4,1H,J0B25IJ8.4,') has rix=',J0B25IJ8.4,' and "
            " slope=',J0B25I8.5)", &pwe->slx, &pwe->sly, &pwe->shx,
            &pwe->shy, &pwe->rix, &pwe->slope, NULL);
         }
      }
#endif
      if (P.ioff != P.iy * ngx + P.ix)
         convrt("(P1,' T3, P.ioff = ',J1IL8,'wrong at x,y = (',"
            "J0IL8,1H,J0IL8,1H))", &P.ioff, &P.ix, &P.iy, NULL);
      rect(fx, fy, rw, rh, -1);
      }
#if 0
      cryout(RK_P1, "\0", RK_LN0+RK_FLUSH+1, NULL);
#endif

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
   InitPgfgIndexing(&P, poly, work, ngx, ngy, 5*ns);
   while (GetNextPointInPgfg(&P)) {
      float fx = x0 + xi*(0.1 + (float)P.ix);
      float fy = y1 - yi*(0.9 + (float)P.iy);
      if (P.ioff != P.iy * ngx + P.ix)
         convrt("(P1,' T4, P.ioff = ',J1IL8,'wrong at x,y = (',"
            "J0IL8,1H,J0IL8,1H))", &P.ioff, &P.ix, &P.iy, NULL);
      rect(fx, fy, rw, rh, -1);
      }

   } /* End ns loop */

   /* Fifth test (no plot) -- Deliberate concave polygon error */
   cryout(RK_P1, "0Starting test with deliberate abexit 68.",
      RK_LN2, NULL);
   ngx = 16, ngy = 11;
   poly[0].x = 2.3, poly[0].y = 1.6;
   poly[1].x = 40.2, poly[1].y = -3.3;
   poly[2].x = 16.0, poly[2].y = 7.9;
   poly[3].x = 9.8, poly[3].y = 3.4;
   poly[4].x = 4.6, poly[4].y = 9.7;

   InitPgfgIndexing(&P, poly, work, ngx, ngy, 5);
   while (GetNextPointInPgfg(&P)) {
      if (P.ioff != P.iy * ngx + P.ix)
         convrt("(P1,' T4, P.ioff = ',J1IL8,'wrong at x,y = (',"
            "J0IL8,1H,J0IL8,1H))", &P.ioff, &P.ix, &P.iy, NULL);
      }

   endplt();
   cryout(RK_P1, "0End of iterpgfg test", RK_LN2+RK_FLUSH, NULL);
   return 0;
   }
