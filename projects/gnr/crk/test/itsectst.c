/* Test routine for itersect */
/* Requires plot library and mfdraw */
/* Correct results in itsectst1.png and itsectst2.png */

/* Note:  As usual, our plotting routines use a right-handed system
*  in which x goes from left to right, y goes from bottom to top.
*  But our iterator routines, which are designed to work with images,
*  use a left-handed system with y = 0 at the top.  Tests are run
*  twice with y coordinates reversed to see effect of clockwise vs
*  anticlockwise sector angle.  */

/* Notes:  Make gives a warning that can be ignored--calling atexit
*  with endplt, which does not return void.  */

#define MAIN
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "itersect.h"
#include "plots.h"

#define R2D 57.2957795

/* Make a rectangle -- no grids */
static void makerect(float x0, float y0, float x1, float y1) {

   pencol("GREEN");
   retrace(1);
   rect(x0, y0, x1-x0, y1-y0, 0);
   retrace(0);
   } /* End makerect() */

int main() {

   xyf p0,p1,p2;;
   float rw,rh;
   float x0,x1,y0,y1,xi,yi;
   IterSect P;
   int ngx,ngy;
   int Pass2;

   atexit(endplt);
   setmf("NULL", "localhost", "ITERSECT TEST", NULL,
      1024, 0, 'A', 0, 0);
   setmovie(MM_MOVIE,0,0);

   /* Repeat twice with y coordinates reversed second time
   *  (indenting suppressed in this loop).  */
   for (Pass2=0; Pass2<2; ++Pass2) {

   newplt(8.0, 12.0, 0, 0, "DEFAULT", "BLACK", "DEFAULT", 0);

   /* First plot -- Sector in Quads 0 and 1, origin on a grid */
   x0 = 0.5, x1 = 3.5, y0 = 8.5, y1 = 11.5;
   ngx = 30, ngy = 30;
   xi = (x1 - x0)/(float)ngx, rw = 0.8*xi;
   yi = (y1 - y0)/(float)ngy, rh = 0.8*yi;

   if (Pass2) {
      float fngy = (float)ngy;
      p0.x = 12.00, p0.y = fngy - 15.0;
      p1.x = 23.75, p1.y = fngy - 15.0;
      p2.x =  4.50, p2.y = fngy - 10.0;
      }
   else {
      p0.x = 12.00, p0.y = 15.0;
      p1.x = 23.75, p1.y = 15.0;
      p2.x =  4.50, p2.y = 10.0;
      }

   makerect(x0, y0, x1, y1);
   symbol(x0, y0-0.3, 0.21, "Sector in quads 0,1", 0, 19);
   pencol("RED");
   line(x0+xi*p0.x, y1-yi*p0.y, x0+xi*p1.x, y1-yi*p1.y);
   line(x0+xi*p0.x, y1-yi*p0.y, x0+xi*p2.x, y1-yi*p2.y);
#if 0
   arc(x0+xi*p0.x, y1-yi*p0.y, x0+xi*p1.x, y1-yi*p1.y,
      R2D*(atan2f(p2.y-p0.y, p2.x-p0.x) -
           atan2f(p1.y-p0.y, p1.x-p0.x)));
#endif
   pencol("YELLOW");
   InitSectorIndexing(&P, p0, p1, p2, ngx, ngy);
   while (GetNextPointInSector(&P)) {
      float fx = x0 + xi*(0.1 + (float)P.ix);
      float fy = y1 - yi*(0.9 + (float)P.iy);
      if (P.ioff != P.iy * ngx + P.ix)
         convrt("(P1,' T1, P.ioff = ',J1IL8,'wrong at x,y = (',"
            "J0IL8,1H,J0IL8,1H))", &P.ioff, &P.ix, &P.iy, NULL);
      rect(fx, fy, rw, rh, -1);
      }

   /* Right 1st plot -- Same with low x cutoff */
   x0 = 4.5, x1 = 7.5, y0 = 8.5, y1 = 11.5;
   ngx = 30, ngy = 30;
   xi = (x1 - x0)/(float)ngx, rw = 0.8*xi;
   yi = (y1 - y0)/(float)ngy, rh = 0.8*yi;

   if (Pass2) {
      float fngy = (float)ngy;
      p0.x =  3.50, p0.y = fngy - 15.0;
      p1.x = 15.25, p1.y = fngy - 15.0;
      p2.x = -4.00, p2.y = fngy - 10.0;
      }
   else {
      p0.x =  3.50, p0.y = 15.0;
      p1.x = 15.25, p1.y = 15.0;
      p2.x = -4.00, p2.y = 10.0;
      }

   makerect(x0, y0, x1, y1);
   symbol(x0, y0-0.3, 0.21, "Cutoff Sector in quads 0,1", 0, 26);
   pencol("RED");
   line(x0+xi*p0.x, y1-yi*p0.y, x0+xi*p1.x, y1-yi*p1.y);
   line(x0+xi*p0.x, y1-yi*p0.y, x0+xi*p2.x, y1-yi*p2.y);
#if 0
   arc(x0+xi*p0.x, y1-yi*p0.y, x0+xi*p1.x, y1-yi*p1.y,
      R2D*(atan2f(p2.y-p0.y, p2.x-p0.x) -
           atan2f(p1.y-p0.y, p1.x-p0.x)));
#endif
   pencol("YELLOW");
   InitSectorIndexing(&P, p0, p1, p2, ngx, ngy);
   while (GetNextPointInSector(&P)) {
      float fx = x0 + xi*(0.1 + (float)P.ix);
      float fy = y1 - yi*(0.9 + (float)P.iy);
      if (P.ioff != P.iy * ngx + P.ix)
         convrt("(P1,' T1, P.ioff = ',J1IL8,'wrong at x,y = (',"
            "J0IL8,1H,J0IL8,1H))", &P.ioff, &P.ix, &P.iy, NULL);
      rect(fx, fy, rw, rh, -1);
      }

   /* Second plot -- Sector in Quads 1,2,3, origin off grid */
   x0 = 0.5, x1 = 3.5, y0 = 4.5, y1 = 7.5;
   ngx = 30, ngy = 30;
   xi = (x1 - x0)/(float)ngx, rw = 0.8*xi;
   yi = (y1 - y0)/(float)ngy, rh = 0.8*yi;

   if (Pass2) {
      float fngy = (float)ngy;
      p0.x = 12.5, p0.y = fngy - 15.49;
      p1.x = 12.5, p1.y = fngy -  7.49;
      p2.x = 20.0, p2.y = fngy - 27.49;
      }
   else {
      p0.x = 12.5, p0.y = 15.49;
      p1.x = 12.5, p1.y = 7.49;
      p2.x = 20.0, p2.y = 27.49;
      }

   makerect(x0, y0, x1, y1);
   symbol(x0, y0-0.3, 0.21, "Sector in quads 1,2,3", 0, 21);
   pencol("RED");
   line(x0+xi*p0.x, y1-yi*p0.y, x0+xi*p1.x, y1-yi*p1.y);
   line(x0+xi*p0.x, y1-yi*p0.y, x0+xi*p2.x, y1-yi*p2.y);
#if 0
   arc(x0+xi*p0.x, y1-yi*p0.y, x0+xi*p1.x, y1-yi*p1.y,
      R2D*(atan2f(p2.y-p0.y, p2.x-p0.x) -
           atan2f(p1.y-p0.y, p1.x-p0.x)));
#endif
   pencol("YELLOW");
   InitSectorIndexing(&P, p0, p1, p2, ngx, ngy);
   while (GetNextPointInSector(&P)) {
      float fx = x0 + xi*(0.1 + (float)P.ix);
      float fy = y1 - yi*(0.9 + (float)P.iy);
      if (P.ioff != P.iy * ngx + P.ix)
         convrt("(P1,' T2, P.ioff = ',J1IL8,'wrong at x,y = (',"
            "J0IL8,1H,J0IL8,1H))", &P.ioff, &P.ix, &P.iy, NULL);
      rect(fx, fy, rw, rh, -1);
      }

   /* Right second plot -- Same with high y cutoff */
   x0 = 4.5, x1 = 7.5, y0 = 4.5, y1 = 6.7;
   ngx = 30, ngy = 22;
   xi = (x1 - x0)/(float)ngx, rw = 0.8*xi;
   yi = (y1 - y0)/(float)ngy, rh = 0.8*yi;

   if (Pass2) {
      float fngy = (float)ngy;
      p0.x = 12.5, p0.y = fngy - 15.49;
      p1.x = 12.5, p1.y = fngy -  6.49;
      p2.x = 20.0, p2.y = fngy - 27.49;
      }
   else {
      p0.x = 12.5, p0.y = 15.49;
      p1.x = 12.5, p1.y =  6.49;
      p2.x = 20.0, p2.y = 27.49;
      }

   makerect(x0, y0, x1, y1);
   symbol(x0, y0-0.3, 0.21, "Sector in quads 1,2,3", 0, 21);
   pencol("RED");
   line(x0+xi*p0.x, y1-yi*p0.y, x0+xi*p1.x, y1-yi*p1.y);
   line(x0+xi*p0.x, y1-yi*p0.y, x0+xi*p2.x, y1-yi*p2.y);
#if 0
   arc(x0+xi*p0.x, y1-yi*p0.y, x0+xi*p1.x, y1-yi*p1.y,
      R2D*(atan2f(p2.y-p0.y, p2.x-p0.x) -
           atan2f(p1.y-p0.y, p1.x-p0.x)));
#endif
   pencol("YELLOW");
   InitSectorIndexing(&P, p0, p1, p2, ngx, ngy);
   while (GetNextPointInSector(&P)) {
      float fx = x0 + xi*(0.1 + (float)P.ix);
      float fy = y1 - yi*(0.9 + (float)P.iy);
      if (P.ioff != P.iy * ngx + P.ix)
         convrt("(P1,' T2, P.ioff = ',J1IL8,'wrong at x,y = (',"
            "J0IL8,1H,J0IL8,1H))", &P.ioff, &P.ix, &P.iy, NULL);
      rect(fx, fy, rw, rh, -1);
      }


   /* Third plot -- Very small arc */
   x0 = 0.5, x1 = 3.5, y0 = 0.5, y1 = 3.5;
   ngx = 30, ngy = 30;
   xi = (x1 - x0)/(float)ngx, rw = 0.8*xi;
   yi = (y1 - y0)/(float)ngy, rh = 0.8*yi;

   if (Pass2) {
      float fngy = (float)ngy;
      p0.x = 15.0, p0.y = fngy - 15.0;
      p1.x =  3.5, p1.y = fngy - 13.0;
      p2.x =  3.0, p2.y = fngy - 17.0;
      }
   else {
      p0.x = 15.0, p0.y = 15.0;
      p1.x =  3.5, p1.y = 13.0;
      p2.x =  3.0, p2.y = 17.0;
      }

   makerect(x0, y0, x1, y1);
   symbol(x0, y0-0.3, 0.21, "Small sector around -x", 0, 22);
   pencol("RED");
   line(x0+xi*p0.x, y1-yi*p0.y, x0+xi*p1.x, y1-yi*p1.y);
   line(x0+xi*p0.x, y1-yi*p0.y, x0+xi*p2.x, y1-yi*p2.y);
#if 0
   arc(x0+xi*p0.x, y1-yi*p0.y, x0+xi*p1.x, y1-yi*p1.y,
      R2D*(atan2f(p2.y-p0.y, p2.x-p0.x) -
           atan2f(p1.y-p0.y, p1.x-p0.x)));
#endif
   pencol("YELLOW");
   InitSectorIndexing(&P, p0, p1, p2, ngx, ngy);
   while (GetNextPointInSector(&P)) {
      float fx = x0 + xi*(0.1 + (float)P.ix);
      float fy = y1 - yi*(0.9 + (float)P.iy);
      if (P.ioff != P.iy * ngx + P.ix)
         convrt("(P1,' T2, P.ioff = ',J1IL8,'wrong at x,y = (',"
            "J0IL8,1H,J0IL8,1H))", &P.ioff, &P.ix, &P.iy, NULL);
      rect(fx, fy, rw, rh, -1);
      }

   /* Fourth plot -- Two segments on -y */
   x0 = 4.5, x1 = 7.5, y0 = 0.5, y1 = 3.5;
   ngx = 30, ngy = 30;
   xi = (x1 - x0)/(float)ngx, rw = 0.8*xi;
   yi = (y1 - y0)/(float)ngy, rh = 0.8*yi;

   if (Pass2) {
      float fngy = (float)ngy;
      p0.x = 15.0, p0.y = fngy - 15.0;
      p1.x = 20.0, p1.y = fngy - 25.5;
      p2.x = 10.0, p2.y = fngy - 25.5;
      }
   else {
      p0.x = 15.0, p0.y = 15.0;
      p1.x = 20.0, p1.y = 25.5;
      p2.x = 10.0, p2.y = 25.5;
      }

   makerect(x0, y0, x1, y1);
   symbol(x0, y0-0.3, 0.21, "Two segments on -y", 0, 18);
   pencol("RED");
   line(x0+xi*p0.x, y1-yi*p0.y, x0+xi*p1.x, y1-yi*p1.y);
   line(x0+xi*p0.x, y1-yi*p0.y, x0+xi*p2.x, y1-yi*p2.y);
#if 0
   arc(x0+xi*p0.x, y1-yi*p0.y, x0+xi*p1.x, y1-yi*p1.y,
      R2D*(atan2f(p2.y-p0.y, p2.x-p0.x) -
           atan2f(p1.y-p0.y, p1.x-p0.x)));
#endif
   pencol("YELLOW");
   InitSectorIndexing(&P, p0, p1, p2, ngx, ngy);
   while (GetNextPointInSector(&P)) {
      float fx = x0 + xi*(0.1 + (float)P.ix);
      float fy = y1 - yi*(0.9 + (float)P.iy);
      if (P.ioff != P.iy * ngx + P.ix)
         convrt("(P1,' T2, P.ioff = ',J1IL8,'wrong at x,y = (',"
            "J0IL8,1H,J0IL8,1H))", &P.ioff, &P.ix, &P.iy, NULL);
      rect(fx, fy, rw, rh, -1);
      }

   } /* End Pass2 loop */

   cryout(RK_P1, "\0", RK_LN0+RK_FLUSH+1, NULL);
   endplt();
   cryout(RK_P1, "0End of itersect test", RK_LN2+RK_FLUSH, NULL);
   return 0;
   }
