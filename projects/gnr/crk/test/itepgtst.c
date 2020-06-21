/* Test routine for iterepgf */
/* Requires plot library and mfdraw */

/* Notes:  Make gives a warning that can be ignored--calling atexit
*  with endplt, which does not return void.  */

#define MAIN
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "iterepgf.h"
#include "plots.h"

#define  BIGPOLY  13    /* Number of vertices in biggest polygon */
#define  NBORDV    3    /* Number of border values to test */

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

   byte *pwim;
   xyf poly[BIGPOLY];
   IEpgfWk work[BIGPOLY];
   float border = 1.5;
   float rw,rh;
   float x0,x1,y0,y1,xi,yi;
   IterEpgf P;
   int mode,ibord;
   int ngx,ngy;
   char mlbl[20];

   /* Values of border to test */
   static float bord0[NBORDV] = { 1.5, 0.6, 0.0 };

   atexit(endplt);
   setmf(NULL, "localhost", "ITEREPGF TEST", NULL,
      1024, 0, 'B', 0, 0);
   setmovie(1,0,0);
   pwim = mallocv(256, "Work image");
   memcpy(mlbl, "Mode 0", 6);

/* Repeat 3 times with border as given in bord0 */

for (ibord=0; ibord<NBORDV; ++ibord) {
   border = bord0[ibord];

/* Repeat the whole thing eight times for each marking mode */
/*  *** ONE LEVEL OF INDENTING SUPPRESSED HERE TO END *** */

for (mode=0; mode<8; ++mode) {

   newplt(8.0, 8.0, 0, 0, "DEFAULT", "BLACK", "DEFAULT", 0);
   sconvrt(mlbl, "('Border ',F4.3,', mode ',I1)", &border,
      &mode, NULL);
   symbol(3.28, 7.7, 0.28, mlbl, 0, 19);

   /* First plot -- Quad not touching edges */
   x0 = 0.5, x1 = 3.5, y0 = 4.5, y1 = 7.5;
   ngx = 24, ngy = 24;
   xi = (x1 - x0)/(float)ngx, rw = 0.8*xi;
   yi = (y1 - y0)/(float)ngy, rh = 0.8*yi;

   poly[0].x =  4.6, poly[0].y =  3.2;
   poly[1].x =  6.2, poly[1].y = 20.6;
   poly[2].x = 15.8, poly[2].y = 15.8;
   poly[3].x = 19.6, poly[3].y =  5.8;

   makerect(x0, y0, x1, y1, ngx, ngy);
   symbol(x0, y0-0.3, 0.21, "Quad inside box", 0, 15);
   pencol("YELLOW");
   InitEpgfIndexing(&P, poly, work, pwim, border, ngx, ngy, 4, mode);
   while (GetNextPointInEpgf(&P)) {
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
   ngx = 20, ngy = 18;
   xi = (x1 - x0)/(float)ngx, rw = 0.8*xi;
   yi = (y1 - y0)/(float)ngy, rh = 0.8*yi;

   poly[0].x =  4.6, poly[0].y =  4.0;
   poly[1].x =  9.4, poly[1].y = 16.0;
   poly[2].x = 17.4, poly[2].y =  4.0;

   makerect(x0, y0, x1, y1, ngx, ngy);
   symbol(x0, y0-0.3, 0.21, "Triangle w/horiz. edge", 0, 22);
   pencol("YELLOW");
   InitEpgfIndexing(&P, poly, work, pwim, border, ngx, ngy, 3, mode);
   while (GetNextPointInEpgf(&P)) {
      float fx = x0 + xi*(0.1 + (float)P.ix);
      float fy = y1 - yi*(0.9 + (float)P.iy);
      if (P.ioff != P.iy * ngx + P.ix)
         convrt("(P1,' T2, P.ioff = ',J1IL8,'wrong at x,y = (',"
            "J0IL8,1H,J0IL8,1H))", &P.ioff, &P.ix, &P.iy, NULL);
      rect(fx, fy, rw, rh, -1);
      }

   /* Third plot -- Concave figure not simply connected */
   x0 = 0.5, x1 = 3.5, y0 = 0.5, y1 = 2.9;
   ngx = 30, ngy = 24;
   xi = (x1 - x0)/(float)ngx, rw = 0.8*xi;
   yi = (y1 - y0)/(float)ngy, rh = 0.8*yi;

   poly[0].x =  -4.2, poly[0].y =   2.0;
   poly[1].x =   6.6, poly[1].y =  24.6;
   poly[2].x =  12.0, poly[2].y =  15.8;
   poly[3].x =  19.6, poly[3].y =  15.8;
   poly[4].x =  15.2, poly[4].y =  10.0;
   poly[5].x =  20.0, poly[5].y =  EpgfPly(2.0,ngy);
   poly[6].x =   2.0, poly[6].y =   4.0;
   poly[7].x =   6.2, poly[7].y =  10.0;
   poly[8].x =   2.0, poly[8].y =  EpgfPly(13.0,ngy);
   poly[9].x =  18.0, poly[9].y =  18.0;
   poly[10].x = 32.0, poly[10].y = 18.0;
   poly[11].x = 33.0, poly[11].y = 22.0;
   poly[12].x = 18.0, poly[12].y = 22.0;

   makerect(x0, y0, x1, y1, ngx, ngy);
   symbol(x0, y0-0.3, 0.21, "Pentagon overlapping", 0, 20);
   pencol("YELLOW");
   InitEpgfIndexing(&P, poly, work, pwim, border, ngx, ngy, 13, mode);
#if 0
   convrt("(P1,'0ix=',J0IL6,', iy=',J0IL6,', bs=',J0I6,"
      "', smnx=',J0B25IJ8.4,', smxx=',J0B25IJ8.4)",
      &P.ix,&P.iy,&P.bs,&P.smnx,&P.smxx,NULL);
   {  IEpgfWk *pwe;
      for (pwe=P.wkel; pwe; pwe=pwe->pne) {
         convrt("(P1,'   Line (',J0B25IJ8.4,1H,J0B25IJ8.4,') to (',"
            "J0B25IJ8.4,1H,J0B25IJ8.4,') has rix=',J0B25IJ8.4,' and "
            " slope=',J0B25I8.5)", &pwe->slx, &pwe->sly, &pwe->shx,
            &pwe->shy, &pwe->rix, &pwe->slope, NULL);
         }
      cryout(RK_P1, "\0", RK_LN0+RK_FLUSH+1, NULL);
      }
#endif
   while (GetNextPointInEpgf(&P)) {
      float fx = x0 + xi*(0.1 + (float)P.ix);
      float fy = y1 - yi*(0.9 + (float)P.iy);
#if 0
   {  IEpgfWk *pwe;
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
   ngx = 22, ngy = 26;
   xi = (x1 - x0)/(float)ngx, rw = 0.8*xi;
   yi = (y1 - y0)/(float)ngy, rh = 0.8*yi;

   poly[0].x = -2.6, poly[0].y =  3.2;
   poly[1].x = 24.4, poly[1].y =  6.6;
   poly[2].x = 32.0, poly[2].y = 15.8;
   poly[3].x = 13.6, poly[3].y = 20.8;
   poly[4].x = -9.2, poly[4].y = 19.4;

   makerect(x0, y0, x1, y1, ngx, ngy);
   symbol(x0, y0-0.3, 0.21, "2-side overlap", 0, 14);
   pencol("YELLOW");
   InitEpgfIndexing(&P, poly, work, pwim, border, ngx, ngy, 5, mode);
   while (GetNextPointInEpgf(&P)) {
      float fx = x0 + xi*(0.1 + (float)P.ix);
      float fy = y1 - yi*(0.9 + (float)P.iy);
      if (P.ioff != P.iy * ngx + P.ix)
         convrt("(P1,' T4, P.ioff = ',J1IL8,'wrong at x,y = (',"
            "J0IL8,1H,J0IL8,1H))", &P.ioff, &P.ix, &P.iy, NULL);
      rect(fx, fy, rw, rh, -1);
      }

   }} /* End mode and border loops */

   /* Fifth test (no plot) -- Deliberate concave polygon error */
   cryout(RK_P1, "0Starting test with deliberate abexit 68.",
      RK_LN2, NULL);
   ngx = 16, ngy = 11;
   poly[0].x = 2.3, poly[0].y = 1.6;
   poly[1].x = 40.2, poly[1].y = -3.3;
   poly[2].x = 16.0, poly[2].y = 7.9;
   poly[3].x = 9.8, poly[3].y = 3.4;
   poly[4].x = 4.6, poly[4].y = 9.7;

   InitEpgfIndexing(&P, poly, work, pwim, border, ngx, ngy, 5, mode);
   while (GetNextPointInEpgf(&P)) {
      if (P.ioff != P.iy * ngx + P.ix)
         convrt("(P1,' T4, P.ioff = ',J1IL8,'wrong at x,y = (',"
            "J0IL8,1H,J0IL8,1H))", &P.ioff, &P.ix, &P.iy, NULL);
      }

   endplt();
   freev(pwim, "Work image");
   cryout(RK_P1, "0End of iterepgf test", RK_LN2+RK_FLUSH, NULL);
   return 0;
   }
