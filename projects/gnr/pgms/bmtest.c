/* (c) Copyright 2006-2011, The Rockefeller University *11113* */
/***********************************************************************
*                        bitmap() test program                         *
*                                                                      *
*  V1A, 03/15/06, GNR - Just draw a black-and-white map in pieces      *
*  V2A, 10/28/06, GNR - Add a grayscale bitmap                         *
*  V3A, 06/03/07, GNR - Add a 24-bit color bitmap                      *
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
#include "/home/cdr/src/oldplot/plots.h"
#include "itersph.h"

#define PLTXSZ   6.4          /* Max x plot size (inches) */
#define PLTYSZ   3.6          /* Max y plot size (inches) */
#define Xorg     0.25         /* Section plot X origin offset */
#define Yorg     0.25         /* Section plot Y origin offset */
#define GSMXSZ 400            /* Grayscale map x size (pixels) */
#define HLFWID (GSMXSZ/2)     /* Half width of color map */
#define GSMYSZ 300            /* Grayscale map y size (pixels) */
#define GSCRAD  50.0          /* Radius of grayscale cone */
#define COS30    0.866025404
#define SIN30    0.500
#define TAN30    0.577350269

int main(void) {

   IterSph ISph;           /* Sphere indexing data */
   double cx0,cy0;         /* Center coord of cone */
   double rad;             /* Radius of a point */
   double row,col;         /* Location of a point */
   byte *pmap,*pg;         /* Ptr to gray or color map */
   byte *pmid;             /* Ptr to middle of row */
   int  irow,jcol;         /* Row, column coords in graymap */
   byte gmv;               /* Graymap value */

/* Here is the black-white map */
   static byte bm[30][8] = {
      { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  /* Row  0 */
      { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  /* Row  1 */
      { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  /* Row  2 */
      { 0x00, 0x00, 0x00, 0xff, 0xf0, 0x00, 0x00, 0x00 },  /* Row  3 */
      { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  /* Row  4 */
      { 0x00, 0xfe, 0x00, 0x80, 0x10, 0x1f, 0xf8, 0x00 },  /* Row  5 */
      { 0x01, 0x01, 0x80, 0xc0, 0x10, 0x10, 0x06, 0x00 },  /* Row  6 */
      { 0x02, 0x00, 0x80, 0xa0, 0x10, 0x10, 0x03, 0x00 },  /* Row  7 */
      { 0x04, 0x00, 0x00, 0xb0, 0x10, 0x10, 0x01, 0x00 },  /* Row  8 */
      { 0x08, 0x00, 0x00, 0x90, 0x10, 0x10, 0x01, 0x00 },  /* Row  9 */
      { 0x08, 0x00, 0x00, 0x98, 0x10, 0x10, 0x02, 0x00 },  /* Row 10 */
      { 0x08, 0x00, 0x00, 0x88, 0x10, 0x10, 0x06, 0x00 },  /* Row 11 */
      { 0x08, 0x00, 0x00, 0x8c, 0x10, 0x10, 0x0c, 0x00 },  /* Row 12 */
      { 0x08, 0x00, 0x00, 0x84, 0x10, 0x10, 0x18, 0x00 },  /* Row 13 */
      { 0x08, 0x00, 0x00, 0x86, 0x10, 0x10, 0x60, 0x00 },  /* Row 14 */
      { 0x08, 0x00, 0x00, 0x82, 0x10, 0x1f, 0xe0, 0x00 },  /* Row 15 */
      { 0x08, 0x3f, 0xe0, 0x83, 0x10, 0x10, 0x30, 0x00 },  /* Row 16 */
      { 0x08, 0x01, 0x00, 0x81, 0x10, 0x10, 0x18, 0x00 },  /* Row 17 */
      { 0x08, 0x01, 0x00, 0x81, 0x90, 0x10, 0x0c, 0x00 },  /* Row 18 */
      { 0x0c, 0x01, 0x00, 0x80, 0x90, 0x10, 0x04, 0x00 },  /* Row 19 */
      { 0x04, 0x03, 0x00, 0x80, 0xd0, 0x10, 0x06, 0x00 },  /* Row 20 */
      { 0x06, 0x02, 0x00, 0x80, 0x50, 0x10, 0x02, 0x00 },  /* Row 21 */
      { 0x02, 0x06, 0x00, 0x80, 0x30, 0x10, 0x03, 0x00 },  /* Row 22 */
      { 0x03, 0x8c, 0x00, 0x80, 0x30, 0x10, 0x01, 0x00 },  /* Row 23 */
      { 0x00, 0xf8, 0x00, 0x80, 0x10, 0x10, 0x01, 0x00 },  /* Row 24 */
      { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  /* Row 25 */
      { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  /* Row 26 */
      { 0x0f, 0x3c, 0xc0, 0x3c, 0xc0, 0x0c, 0xf3, 0x00 },  /* Row 27 */
      { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  /* Row 28 */
      { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  /* Row 29 */
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

   pmap = (byte *)mallocv(3*GSMXSZ*GSMYSZ, "Test map");
   /* First fill map with a top-bottom gradient with
   *  a superimposed sine wave */
   for (irow=0,pg=pmap; irow<GSMYSZ; irow++) {
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
      pmap[ISph.ioff] = gmv;
      }
   /* Now write the bitmap, first with no scaling,
   *  then with scaling to make it half as big */
   newplt(PLTXSZ, PLTYSZ, 0.0, 0.0,
      "DEFAULT", "BLACK", "DEFAULT", 0);
   bitmap(pmap, GSMXSZ, GSMYSZ, 2.0*Xorg, 2.0*Yorg,
      0, 0, GSMXSZ, GSMYSZ/2, BM_GS, GM_SET);
   bitmap(pmap, GSMXSZ, GSMYSZ, 2.0*Xorg, 2.0*Yorg,
      0, GSMYSZ/2, GSMXSZ, GSMYSZ/2, BM_GS, GM_SET);
   pencol("RED");
   rect(Xorg, Yorg, PLTXSZ-2.0*Xorg, PLTYSZ-2.0*Yorg, 0);

   newplt(PLTXSZ, PLTYSZ, 0.0, 0.0,
      "DEFAULT", "BLACK", "DEFAULT", 0);
   bitmaps(pmap, GSMXSZ, GSMYSZ, 2.0*Xorg, 2.0*Yorg,
      2.8, 2.1, 0, 0, GSMXSZ/2, GSMYSZ, BM_GS, GM_SET);
   bitmaps(pmap, GSMXSZ, GSMYSZ, 2.0*Xorg, 2.0*Yorg,
      2.8, 2.1, GSMXSZ/2, 0, GSMXSZ/2, GSMYSZ, BM_GS, GM_SET);
   retrace(2);
   pencol("GREEN");
   rect(2.0*Xorg, 2.0*Yorg, 2.8, 2.1, 0);
   pencol("YELLOW");
   circle(2.0*Xorg+1.4, 2.0*Yorg+1.05, 0.4, 0);

/* Now create and write a color triangle */

#if 1
   /* First set everything to middle gray */
   memset((char *)pmap, 128, 3*GSMXSZ*GSMYSZ);
   /* Make a red gradient top to bottom,
   *  a green gradient lower left to upper right,
   *  and a blue gradient lower right to upper left,
   *  but coloring only the interior of the color triangle.  */
   pmid = pmap + 3*(24*GSMXSZ + HLFWID);
   for (irow=25; irow<=275; ++irow,pmid+=3*GSMXSZ) {
      double ys30,cval;
      int w = (int)(TAN30*(double)(irow-25));
      int y = 275 - irow;
      ys30 = (double)y * SIN30;
      pg = pmid - 3*w;
      for (jcol=HLFWID-w; jcol<=HLFWID+w; ++jcol,pg+=3) {
         /* Red value prop to perp distance from red vertex */
         pg[0] = (byte)(255*y/250);
         /* Green value prop to perp distance from green vertex */
         col = (double)(jcol - 56);
         rad = COS30 * col + ys30;
         cval = 255*(1.0-rad/250.0);
         if (cval < 0.0) cval = 0.0;
         else if (cval > 255.0) cval = 255.0;
         pg[1] = (byte)cval;
         /* Blue value prop to perp distance from blue vertex */
         col = (double)(jcol - 344);
         rad = ys30 - COS30 * col;
         cval = 255*(1.0-rad/250.0);
         if (cval < 0.0) cval = 0.0;
         else if (cval > 255.0) cval = 255.0;
         pg[2] = (byte)cval;
         } /* End column loop */
      } /* End row loop */

   /* Now write the bitmap, first with no scaling,
   *  then with scaling to make it half as big */
   newplt(PLTXSZ, PLTYSZ, 0.0, 0.0,
      "DEFAULT", "BLACK", "DEFAULT", 0);
   bitmap(pmap, GSMXSZ, GSMYSZ, Xorg, Yorg,
      0, 0, GSMXSZ, GSMYSZ, BM_C24, GM_SET);

   newplt(PLTXSZ, PLTYSZ, 0.0, 0.0,
      "DEFAULT", "BLACK", "DEFAULT", 0);
   bitmaps(pmap, GSMXSZ, GSMYSZ, Xorg, Yorg,
      2.8, 2.1, 0, 0, GSMXSZ, GSMYSZ, BM_C24, GM_SET);
#endif

   endplt();
   return 0;

   } /* End bmtest() */

