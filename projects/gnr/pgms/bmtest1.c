/* (c) Copyright 2006-2011, The Rockefeller University *11113* */
/***********************************************************************
*                        bitmap() test program                         *
*                                                                      *
*  V1A, 03/15/06, GNR - Just draw a black-and-white map in pieces      *
*  V2A, 10/28/06, GNR - Add a grayscale bitmap                         *
*  Rev, 10/12/11, GNR - Remove setmeta(), setmfd(), use setmf()        *
***********************************************************************/

#define MAIN

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include "sysdef.h"
#include "rocks.h"
#include "itersph.h"
#include "/home/cdr/src/oldplot/plots.h"

#define PLTXSZ   6.4       /* Max x plot size (inches) */
#define PLTYSZ   3.6       /* Max y plot size (inches) */
#define Xorg     0.25      /* Section plot X origin offset */
#define Yorg     0.25      /* Section plot Y origin offset */
#define GSMXSZ   400       /* Grayscale map x size (pixels) */
#define GSMYSZ   300       /* Grayscale map y size (pixels) */
#define GSCRAD    50.0     /* Radius of grayscale cone */

void main(void) {

   IterSph ISph;           /* Sphere indexing data */
   double cx0,cy0;         /* Center coord of cone */
   double rad;             /* Radius of a point */
   double row,col;         /* Location of a point */
   byte *pgray,*pg;        /* Ptr to gray map */
   int  irow,jcol;         /* Row, column coords in graymap */
   byte gmv;               /* Graymap value */

/* Here is the black-white map */
   static ui32 bm[30][2] = {
      { 0x00000000, 0x00000000 },   /* Row  0 */
      { 0x00000000, 0x00000000 },   /* Row  1 */
      { 0x00000000, 0x00000000 },   /* Row  2 */
      { 0x000000ff, 0xf0000000 },   /* Row  3 */
      { 0x00000000, 0x00000000 },   /* Row  4 */
      { 0x00fe0080, 0x101ff800 },   /* Row  5 */
      { 0x010180c0, 0x10100600 },   /* Row  6 */
      { 0x020080a0, 0x10100300 },   /* Row  7 */
      { 0x040000b0, 0x10100100 },   /* Row  8 */
      { 0x08000090, 0x10100100 },   /* Row  9 */
      { 0x08000098, 0x10100200 },   /* Row 10 */
      { 0x08000088, 0x10100600 },   /* Row 11 */
      { 0x0800008c, 0x10100c00 },   /* Row 12 */
      { 0x08000084, 0x10101800 },   /* Row 13 */
      { 0x08000086, 0x10106000 },   /* Row 14 */
      { 0x08000082, 0x101fe000 },   /* Row 15 */
      { 0x083fe083, 0x10103000 },   /* Row 16 */
      { 0x08010081, 0x10101800 },   /* Row 17 */
      { 0x08010081, 0x90100c00 },   /* Row 18 */
      { 0x0c010080, 0x90100400 },   /* Row 19 */
      { 0x04030080, 0xd0100600 },   /* Row 20 */
      { 0x06020080, 0x50100200 },   /* Row 21 */
      { 0x02060080, 0x30100300 },   /* Row 22 */
      { 0x038c0080, 0x30100100 },   /* Row 23 */
      { 0x00f80080, 0x10100100 },   /* Row 24 */
      { 0x00000000, 0x00000000 },   /* Row 25 */
      { 0x00000000, 0x00000000 },   /* Row 26 */
      { 0x0f3cc03c, 0xc00cf300 },   /* Row 27 */
      { 0x00000000, 0x00000000 },   /* Row 28 */
      { 0x00000000, 0x00000000 },   /* Row 29 */
      };

   settit("Bitmap test program");
   setmf("bmtest.mf", "", "bmtest", NULL, 0, 0, 'B', 0, 0);

/* First the black-white map -- in three pieces */

   newplt(PLTXSZ, PLTYSZ, 0.0, 0.0,
      "DEFAULT", "BLACK", "DEFAULT", 0);
   pencol("RED");
   rect(Xorg, Yorg, PLTXSZ-2.0*Xorg, PLTYSZ-2.0*Yorg, 0);
   pencol("MAGENTA");

   bitmap((byte *)bm, 60, 30, 2.0*Xorg, 2.0*Yorg,
      0, 0, 60, 12, BM_BW, GM_SET);
   bitmap((byte *)bm, 60, 30, 2.0*Xorg, 2.0*Yorg,
      0, 12, 60, 18, BM_BW, GM_SET);
   bitmap((byte *)bm, 60, 30, 2.0*Xorg, 2.0*Yorg,
      24, 3, 12, 1, BM_BW, GM_XOR);

/* Now create and write a grayscale map */

   pgray = (byte *)mallocv(GSMXSZ*GSMYSZ, "Grayscale map");
   /* First fill map with a top-bottom gradient with
   *  a superimposed sine wave */
   for (irow=0,pg=pgray; irow<GSMYSZ; irow++) {
      gmv = 53 + irow/2 + (byte)(10.0*sin(0.06283*(double)irow));
      for (jcol=0; jcol<GSMXSZ; jcol++,pg++)
         *pg = gmv;
      } /* End irow loop */
   /* Now stick a little cone in the middle of the map
   *  that covers the entire 0-255 density range.  */
   cx0 = (double)(GSMXSZ/2) - 0.5;
   cy0 = (double)(GSMYSZ/2) - 0.5;
   InitSphereIndexing(&ISph, 1.0, 1.0, 1.0,
      cx0, cy0, 0.0, GSCRAD, GSMXSZ, GSMYSZ, 1);
   while (GetNextPointInSphere(&ISph)) {
      if (ISph.iz != 0) continue;   /* Just in case */
      row = (double)ISph.iy - cy0;
      col = (double)ISph.ix - cx0;
      rad = sqrt(row*row + col*col)/GSCRAD;
      if (rad > 1.0) rad = 1.0;
      gmv = (byte)(255.0*(1.0 - rad));
      pgray[ISph.ioff] = gmv;
      }
   /* Now write the bitmap, first with no scaling,
   *  then with scaling to make it half as big */
   newplt(PLTXSZ, PLTYSZ, 0.0, 0.0,
      "DEFAULT", "BLACK", "DEFAULT", 0);
   bitmap(pgray, GSMXSZ, GSMYSZ, 2.0*Xorg, 2.0*Yorg,
      0, 0, GSMXSZ, GSMYSZ/2, BM_GS, GM_SET);
   bitmap(pgray, GSMXSZ, GSMYSZ, 2.0*Xorg, 2.0*Yorg,
      0, GSMYSZ/2, GSMXSZ, GSMYSZ/2, BM_GS, GM_SET);
   pencol("RED");
   rect(Xorg, Yorg, PLTXSZ-2.0*Xorg, PLTYSZ-2.0*Yorg, 0);

   newplt(PLTXSZ, PLTYSZ, 0.0, 0.0,
      "DEFAULT", "BLACK", "DEFAULT", 0);
   bitmaps(pgray, GSMXSZ, GSMYSZ, 2.0*Xorg, 2.0*Yorg,
      2.8, 2.1, 0, 0, GSMXSZ/2, GSMYSZ, BM_GS, GM_SET);
   bitmaps(pgray, GSMXSZ, GSMYSZ, 2.0*Xorg, 2.0*Yorg,
      2.8, 2.1, GSMXSZ/2, 0, GSMXSZ/2, GSMYSZ, BM_GS, GM_SET);
   retrace(2);
   pencol("GREEN");
   rect(2.0*Xorg, 2.0*Yorg, 2.8, 2.1, 0);
   pencol("YELLOW");
   circle(2.0*Xorg+1.4, 2.0*Yorg+1.05, 0.4, 0);

   endplt();

   } /* End bmtest() */

