/* (c) Copyright 1998-2011, The Rockefeller University *11114* */
/***********************************************************************
*                              ELIBDUMP                                *
*                              07/08/98                                *
*                                                                      *
*  Dump (plot) the contents of an environment library.  Code in this   *
*  routine is completely independent of the standard env library for   *
*  debugging purposes.                                                 *
*                                                                      *
*  Synopsis:                                                           *
*     elibdump [-g<ng>] [-r<nr>] [-G] <file>                           *
*  The following options are recognized:                               *
*     -g<ng>   Group stimuli <ng> to a plot (default: 1 to a plot).    *
*     -r<nr>   Arrange the <ng> stimuli in a plot in <nr> rows         *
*              (default: fix(sqrt(ng)).                                *
*     -G       Draw grid lines on each plot.                           *
*     <file>   Name of stimulus library file to be dumped.             *
*                                                                      *
*  This program is assigned error codes in the range 380-389           *
*                                                                      *
*  V1A, 07/08/98, GNR - New program                                    *
*  Rev, 05/22/99, GNR - Use new swapping scheme                        *
*  Rev, 09/26/08, GNR - Minor type changes for 64-bit compilation      *
*  Rev, 10/12/11, GNR - Remove setmfd(), use setmf()                   *
***********************************************************************/

#define MAIN
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "rfdef.h"
#include "swap.h"
#include "bapkg.h"
#include "plots.h"
#include "dslb.h"

/* Configuration parameters */
#define PLOT_SIZE     10.0    /* Nominal plot size (inches) */
#define FRAME_MARGIN  0.06    /* Fractional size of frame margins */
#define PIXEL_SIZE    0.80    /* Pixel size as fraction of grid box */
#define GRID_COLOR   "BLUE"   /* Grid line color */
#define STIM_COLOR "YELLOW"   /* Mono stimulus color */
#define STIM_RGB       011    /* Stimulus color as octal RGB value */
#define LIBMAKER "D3LIBPRP"   /* Name of shape library make utility */
#define VERSIZE         12    /* Size of version info in header */
#define qVERSIZE       "12"   /* Quoted VERSIZE */
#define LIB_REC_SIZE    80    /* Size of one record in library */
#define FMT_MASK      0xff    /* Format mask */
#define TSP_FLAG    0x0100    /* Transparency flag */
#define EIHI          0xf0    /* Value assigned to on-bits */
#define EILO          0x08    /* Value assigned to off-bits */

/* Quantities derived from the above */
#define LIB_REC_BITS  (LIB_REC_SIZE<<3)

/* Define shape library index record */
struct LIBINDEF {
   char shape_name[SHP_NAME_SIZE];  /* Name of shape */
   char recnumb[FMJSIZE];           /* Record number */
   };

/* Macro to seek by shape record number rather than byte number */
#define SRSeek(f, r) rfseek(f, LIB_REC_SIZE*(r), SEEKABS, ABORT)

/* Macro to determine whether object is stored as bit array */
#define bit_format(fmt) (!((fmt)&3))

int main (int argc, char *argv[]) {

   static char *fmts[] = {       /* Format names, all 20 chars */
      "Old (Darwin II)     ",
      "A (1 hex char/pixel)",
      "B (black and white) ",
      "C (color)           ",
      "X (hex, 1 bit/pixel)" };
   static char *tnsp[] = { "No", "Yes" };

   struct DSLBDEF SInfo;         /* Shape info for one shape */
   struct RFdef *shapelib;       /* Ptr to shape lib file record */
   struct LIBINDEF *pli;         /* Ptr to one index entry */
   struct LIBINDEF *plibindex;   /* Ptr to library index */
   struct LIBINDEF *pindexend;   /* Ptr to end of library index */
   char *display;                /* Ptr to name of display to use */
   char *fname = NULL;           /* Name of file to be dumped */
   char *pobj,*po;               /* Ptr to object array */
   float x,y;                    /* Location of current drawing */
   float xg,yg;                  /* Size of one grid square */
   float xl,yl;                  /* Limiting x,y for drawing loop */
   float xm,ym;                  /* Size of frame margin along x,y */
   float xp,yp,xp2,yp2;          /* Size of half,full object pixel */
   float xo,yo,xw,yh;            /* Origin, width, height of frame */
   float x1,y1,x2,y2;            /* Coords of stimulus box in frame */
   float ylo,ylh;                /* Label origin and height */
   long lindex;                  /* Length of index */
   long numshapes;               /* Number of shapes in library */
   long recnum;                  /* Record number for current shape */
   size_t llo,lloa;              /* Size of largest object, alloc */
   int drawgrid = NO;            /* YES to draw grid lines */
   int objfmt;                   /* Object format w/o transparent bit */
   int iarg,ic,ig;               /* Loop counters */
   int ng = 1;                   /* Number of stimuli in a group */
   int nr = 0, nc;               /* Number of rows,cols in a gallery */
   char libvers[VERSIZE];        /* Name & version of make utility */
   byte bbuff[LIB_REC_SIZE];     /* Buffer for unpacking bit arrays */

/* Analyze command line parameters */

   settit("TITLE Environment library file dump");
   nopage(RK_PGNONE);

   if (argc < 2 || argc > 5) abexitm(380,
      "Synopsis: elibdump [-g<ng>] [-r<nr>] [-G] <library-file>");

   for (iarg=1; iarg<argc; iarg++) {
      int la = strlen(argv[iarg]);

      if (strncmp(argv[iarg], "-g", 2) == 0 && la > 2) {
         ng = (int)ibcdin(RK_IORF+RK_CTST+RK_QPOS+RK_ZTST+la-3,
            argv[iarg]+2);
         }

      else if (strncmp(argv[iarg], "-r", 2) == 0 && la > 2) {
         nr = (int)ibcdin(RK_IORF+RK_CTST+RK_QPOS+RK_ZTST+la-3,
            argv[iarg]+2);
         }

      else if (strcmp(argv[iarg], "-G") == 0)
         drawgrid = YES;

      else if (argv[iarg][0] == '-')
         abexitm(380, "Unrecognized command-line switch.");

      else if (fname)
         abexitm(380, "Apparently more than one file name entered.");

      else {
         fname = mallocv(la+1, "File name");
         strcpy(fname, argv[iarg]);
         }

      } /* End loop over arguments */

/* Check that filename and DISPLAY have been specified */

   if (!fname) abexitm(380,
      "You must enter the name of the file to be dumped.");

   if (!(display = getenv("DISPLAY")))
      abexitm(381, "DISPLAY not found in environment.");
   /* Display defaults to still mode--no need to call setmovie() */
   setmf(NULL, display, "Env library dump", NULL, 0, 0, 'B', 0, 0);

   if (RK.iexit)
      abexitm(380, "Error interpreting command-line parameters.");

/* Open library file, check and list header */

   shapelib = rfallo(fname, READ, BINARY, DIRECT,
      TOP, NO_LOOKAHEAD, REWIND, RELEASE_BUFF, IGNORE, IGNORE,
      IGNORE, ABORT);
   rfopen(shapelib, NULL, READ, BINARY, DIRECT,
      TOP, NO_LOOKAHEAD, REWIND, RELEASE_BUFF, IGNORE, IGNORE,
      IGNORE, ABORT);
   rfread(shapelib, libvers, VERSIZE, ABORT);
   if (strncmp(libvers, LIBMAKER, strlen(LIBMAKER))) {
      cryout(RK_P1, "0***Input file apparently is not a shape library.",
         RK_LN2, NULL);
      abexit(382);
      }
   numshapes = (long)rfri4(shapelib);
   cryout(RK_P2, "0Contents of environment stimulus library file",
      RK_LN3, "    ", RK_LN0, fname, RK_CCL, NULL);
   convrt("(P2,'0File was generated by ',A" qVERSIZE
      ",' and contains ',J0IL5,' objects.')",
      libvers, &numshapes, NULL);

/* Determine plotting gallery parameters */

   if (ng <= 0 || ng > numshapes) ng = numshapes;
   if (nr <= 0 || nr > ng) nr = (int)sqrt((double)ng);
   nc = (ng + nr - 1)/nr;
   xw = PLOT_SIZE/(float)nc;
   yh = PLOT_SIZE/(float)nr;
   xm = FRAME_MARGIN * xw;
   ym = FRAME_MARGIN * yh, ylh= 0.7 * ym;

/* Allocate space for library index and read it in */

   lindex = numshapes * sizeof(struct LIBINDEF);
   plibindex = (struct LIBINDEF *)mallocv(lindex, "Library index");
   pindexend = plibindex + numshapes;
   SRSeek(shapelib, 1);
   rfread(shapelib, (char *)plibindex, lindex, ABORT);

/* Loop over objects in library.  Read them in.
*  Print characteristics and plot shape of each object. */

   cryout(RK_P2, "0Object  Group         Format         "
      "X-size  Y-size  Transparent", RK_NFSUBTTL+RK_LN2, NULL);
   llo = lloa = 0;
   ig = ng;
   for (pli=plibindex; pli<pindexend; pli++) {

      /* Read header for current object */
      recnum = bemtoi4(pli->recnumb);
      SRSeek(shapelib, recnum);
      rfread(shapelib, SInfo.nshp, SHP_NAME_SIZE, ABORT);
      SInfo.tgp = rfri2(shapelib);
      SInfo.tfm = rfri2(shapelib);
      SInfo.tsx = rfri2(shapelib);
      SInfo.tsy = rfri2(shapelib);
      objfmt = (int)SInfo.tfm & FMT_MASK;

      /* Just a couple of little sanity checks.  These checks exit
      *  rather than skip object, because if check fails, entire
      *  library file is probably damaged beyond all recognition.  */
      if (strncmp(SInfo.nshp, pli->shape_name, SHP_NAME_SIZE)) {
         cryout(RK_P1, "0***Name of library object (", RK_LN2,
            SInfo.nshp, SHP_NAME_SIZE,
            ") does not match name in index (", RK_CCL,
            pli->shape_name, SHP_NAME_SIZE, ").", RK_CCL, NULL);
         abexit(383);
         }
      if (objfmt > (sizeof(fmts)/sizeof(char *))) {
         cryout(RK_P1, "0***Format code for library object ", RK_LN2,
            SInfo.nshp, SHP_NAME_SIZE, " is invalid.", RK_CCL, NULL);
         abexit(384);
         }
      if (SInfo.tsx <= 0 || SInfo.tsy <= 0) {
         cryout(RK_P1, "0***Library object ", RK_LN2,
            SInfo.nshp, SHP_NAME_SIZE,
            " has nonpositive x or y dimension.", RK_CCL, NULL);
         abexit(385);
         }

      /* List info for this object */
      convrt("(P2,2XA4UH7,3XA20IH7IH8,3XA3)", SInfo.nshp,
         &SInfo.tgp, fmts[objfmt], &SInfo.tsx,
         &SInfo.tsy, tnsp[(SInfo.tfm&TSP_FLAG)!=0], NULL);

      /* High-water allocation for largest shape */
      llo = SInfo.tsx * SInfo.tsy;
      if (llo > lloa) {
         if (!lloa) pobj = mallocv(llo, "Object pixels");
         else pobj = reallocv(pobj, llo, "Object pixels");
         lloa = llo;
         }

      /* Read in the actual stimulus pixels */
      if (bit_format(objfmt)) {        /* CASE I: BIT ARRAY */
         long recl = (llo+7)>>3;
         long ib = LIB_REC_BITS;       /* Triggers first read */
         int ix,iy;
         po = pobj;
         for (ix=0; ix<SInfo.tsx; ix++) {
            for (iy=0; iy<SInfo.tsy; iy++) {
               /* Read another buffer when contents all unpacked */
               if (++ib > LIB_REC_BITS) {
                  long lread = min(LIB_REC_SIZE, recl);
                  recl -= lread;
                  rfread(shapelib, bbuff, lread, ABORT);
                  ib = 1;   }
               *po++ = bittst(bbuff, ib) ? EIHI : EILO;
               } /* End loop over Y */
            } /* End loop over X */
         } /* End bit format (case 1) */

      else {                           /* CASE 2: FORMATS A,B,C */
         rfread(shapelib, pobj, llo, ABORT);
         }

      /* Locate frame for this object */
      if (++ig > ng) {              /* Step through gallery */
         /* Start a new gallery */
         newplt(PLOT_SIZE, PLOT_SIZE, 0.0, 0.0,
            "DEFAULT", "CYAN", "DEFAULT", 0);
         ig = ic = 1;
         xo = 0.0;
         yo = PLOT_SIZE - yh, ylo = yo + 0.1 * ym;
         }
      else if (++ic > nc) {         /* Start a new row */
         ic = 1;
         xo = 0.0;
         yo -= yh, ylo = yo + 0.1 * ym;
         }
      else {                        /* Start a new column */
         xo += xw;
         }
      x1 = xo + xm;
      x2 = xo + xw - xm;
      y1 = yo + ym;
      y2 = yo + yh - ym;
      xg = (x2 - x1)/(float)SInfo.tsx;
      yg = (y2 - y1)/(float)SInfo.tsy;
      xp2 = PIXEL_SIZE * xg, xp = 0.5*xp2;
      yp2 = PIXEL_SIZE * yg, yp = 0.5*yp2;
      /* Half box with roundoff allowance */
      xl = x2 - 0.49*xg;
      yl = y1 + 0.49*yg;

      /* Draw a box around the object */
      pencol(GRID_COLOR);
      rect(x1, y1, x2-x1, y2-y1, THIN);

      /* Draw grid lines if requested */
      if (drawgrid) {
         for (y=y2-yg; y>yl; y-=yg)
            line(x1,y,x2,y);
         for (x=x1+xg; x<xl; x+=xg)
            line(x,y1,x,y2);
         }

      /* Label the object */
      symbol(x1, ylo, ylh, SInfo.nshp, 0.0, strlen(SInfo.nshp));

      /* Draw the object */
      po = pobj;
      if (bit_format(objfmt)) pencol(STIM_COLOR);
      for (x=x1+0.5*xg; x<xl; x+=xg) {
         for (y=y2-0.5*yg; y>yl; y-=yg) {
            byte pixel = *po++;

            /* Determine color to draw this pixel */
            switch (objfmt) {
            case 0:        /* Old-style, black & white object */
            case 4:        /* New style, black & white object */
               if (pixel <= EILO) continue;
               break;
            case 1:        /* Hexadecimal gray-scale object */
            case 2:        /* Decimal gray-scale object */
               /* Derive 8 levels of STIM_RGB from gray level */
               if (pixel < 32) continue;
               color(STIM_RGB * (pixel >> 5));
               break;
            case 3:        /* Full-color object */
               if (pixel == 0) continue;
               color(pixel);
               break;
               } /* End color switch */

            rect(x-xp, y-yp, xp2, yp2, FILLED);
            } /* End y loop */
         } /* End x loop */

      } /* End loop over all library objects */

   endplt();
   cryout(RK_P1, "\0", RK_FLUSH+RK_LN0+1, NULL);
   return 0;

   } /* End main (elibdump) */
