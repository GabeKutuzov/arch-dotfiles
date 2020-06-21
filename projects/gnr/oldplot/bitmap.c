/* (c) Copyright 1992-2017, The Rockefeller University *21114* */
/* $Id: bitmap.c 34 2017-09-15 18:13:10Z  $ */
/***********************************************************************
*                              bitmap()                                *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void bitmap(const byte *array, int rowlen, int colht,            *
*        float xc, float yc, int xoffset, int yoffset, int iwd,        *
*        int iht, int type, int mode)                                  *
*                                                                      *
*  DESCRIPTION:                                                        *
*     Plots an image on the screen in the specified location and size. *
*                                                                      *
*  ARGUMENTS:                                                          *
*     'array' is a pointer to a byte array containing the data to be   *
*     plotted.  Data for each horizontal row must be packed in left-   *
*     to-right order, with one bit per pixel if the 'type' is BM_BW,   *
*     one byte per pixel if the 'type' is BM_GS or BM_C8, two bytes    *
*     per pixel if the 'type' is BM_GS16 or BM_C16, three bytes per    *
*     pixel if the 'type' is BM_C24, 6 bytes per pixel if the 'type'   *
*     is BM_C48, or 'lci' bits per pixel if the 'type' is BM_COLOR     *
*     (i.e. each value is a color index).  Rows are arranged in memory *
*     from top to bottom if 'iht' > 0, and from bottom to top if       *
*     'iht' < 0.                                                       *
*                                                                      *
*     'rowlen' and 'colht' give the size of the complete bitmap        *
*     (not just the portion being plotted by this call) in pixels.     *
*     The length of each row in bytes implied by rowlen may be         *
*     greater than or equal to the row length implied by the           *
*     'type', 'xoffset', 'iwd', and 'lci' parameters.                  *
*                                                                      *
*     'xc' and 'yc' are the coordinates in inches of the lower left-   *
*     hand corner of the complete bitmap relative to the current       *
*     plot origin. These coordinates are adjusted according to the     *
*     values of 'xoffset' and 'yoffset' for this bitmap fragment.      *
*                                                                      *
*     'xoffset' and 'yoffset' are positive x,y offsets (in pixels)     *
*     of this bitmap fragment relative to the full bitmap specified    *
*     by 'rowlen' and 'colht'.  These parameters allow bitmaps to be   *
*     plotted in subtiles from different computational nodes in a      *
*     parallel computer.  If the BM_NSME ("bitmap-no submap extrac-    *
*     tion") bit is specified in the 'type' parameter, then the        *
*     data array ('array' argument) contains only the information      *
*     to be plotted by this call.  In the default case (BM_NSME not    *
*     specified), the argument array contains the entire bitmap and    *
*     the data to be plotted are extracted from this full bitmap       *
*     according to the 'xoffset', 'yoffset', 'iwd', and 'iht'          *
*     parameters.  ('yoffset' counts from the top of the bitmap to     *
*     the top of the fragment if iht > 0 and from the bottom of the    *
*     bitmap to the bottom of the fragment if iht < 0.)                *
*                                                                      *
*     'iwd' and 'iht' are the width and height in pixels of the        *
*     bitmap or portion of a bitmap to be plotted by this call.        *
*     If 'iht' > 0, the data in 'array' are arranged in top-to-        *
*     bottom order;  if 'iht' < 0, the data are arranged bottom-       *
*     to-top.  (Data are always stored in the metafile in top-to-      *
*     bottom order.)                                                   *
*                                                                      *
*     type is BM_BW (0) if the bitmap contains black-and-white data,   *
*     BM_GS (1) if the bitmap contains 8-bit grayscale data, BM_GS16   *
*     (2) if the bitmap contains 16-bit grayscale data, BM_C48 (3)     *
*     if the bitmap contains 48-bit (16 bits per color) color data,    *
*     BM_COLOR (4) if the bitmap contains indexed color data, BM_C8    *
*     (5) if the bitmap contains 8-bit (2-3-3 BGR) color data,         *
*     BM_C16 (6) is the bitmap contains 16 bit (5 bits per color,      *
*     leftmost bit ignored) color data, or BM_C24 (7) if the bitmap    *
*     contains 24 bit (8 bits per color) color data.  Other values     *
*     may be defined in future versions to indicate bitmap             *
*     compression.  In a black-and-white bitmap, each pixel value is   *
*     0 to indicate background color or 1 to indicate foreground       *
*     color.                                                           *
*                                                                      *
*     'mode' is GM_SET (0) to set the color index at each point of the *
*     bitmap unconditionally from the array data, GM_XOR (1) to "xor"  *
*     the array data with any existing data at each point, GM_AND (2)  *
*     to "and" the array data with any existing data at each point,    *
*     and GM_CLR (3) to clear any existing data at points of the       *
*     bitmap at which the data array is nonzero.  Type BM_BW should    *
*     generally be specified with mode GM_CLR, because all nonzero     *
*     array values are equivalent in this case.                        *
*                                                                      *
*  NOTES:                                                              *
*     At the moment, 'lci' and BM_COLOR modes are not supported.       *
*     Color data can be provided in BM_C8, BM_C16, BM_C24, or BM_C48   *
*     modes.                                                           *
*                                                                      *
*     mfdraw is not aware of the distinction iht > 0 vs iht < 0.  All  *
*     data it receives are treated as iht > 0, i.e. data arranged top  *
*     to bottom but xoffs,yoffs refer to the lower left corner of the  *
*     bitmap.  It is the responsibility of bitmap() and bitmaps() to   *
*     rearrange the parameters and data array accordingly.             *
*                                                                      *
*     In a parallel computer, the map is divided into segments that    *
*     will fit in a single buffer, each with its own header.  In a     *
*     serial computer, this is not necessary.                          *
*                                                                      *
*     For performance reasons, arguments checking is minimal.          *
*                                                                      *
*  RETURN VALUES:                                                      *
*     None.                                                            *
************************************************************************
*  Version 1, 06/23/92, ROZ                                            *
*  Rev, 07/15/93, GNR - Revise buffering--must be reexamined.          *
*  Rev, 04/22/93,  LC - Revised the buffering, cleaned up routine.     *
*  Rev, 11/25/96, GNR - Delete native NCUBE plotting support           *
*  Rev, 03/15/06, GNR - Use Byte* macros, correct type,mode coding     *
*  Rev, 10/23/06, GNR - Add grayscale type, correct order of args in   *
*                       writeup to agree with that in the code.        *
*  Rev, 06/02/07, GNR - Add BM_C8, BM_C16, BM_C24 modes.               *
*  Rev, 09/25/07, GNR - Force next pencol to emit code (bug w/openGL)  *
*  ==>, 02/27/08, GNR - Last mod before committing to svn repository   *
*  Rev, 10/11/11, GNR - _NCG to _RKG, gmf.h now in glu.h, etc.         *
*  Rev, 03/16/13, GNR - Fix bug in way color code was forced           *
*  Rev, 08/29/14, GNR - Add 'const' to pointer args                    *
*  Rev, 08/24/16, GNR - Correct height loop exit, yoffset description  *
*  R34, 05/17/17, GNR - Add BM_GS16 and BM_C48 modes                   *
***********************************************************************/

#include "sysdef.h"
#include "glu.h"
#include "rksubs.h"
#define MAX_BM_HEADER 38         /* Max BitMap header size plus
                                 *  one for LF at end of pane */

void bitmap(const byte *array, int rowlen, int colht,
            float xc, float yc, int xoffset, int yoffset,
            int iwd, int iht, int type, int mode) {

   const byte  *pcol;            /* Ptr to current column in map */
   const byte  *prow;            /* Ptr to current row in map */
   int   height;                 /* Row loop size */
   int   i,j;                    /* Row, column loop controls */
   int   jtest;                  /* rlen if lbs > 0, else 0 */
   int   lbs,rbs;                /* Left, right byte shifts */
   int   MapType;                /* Type arg less BM_NSME bit */
   int   NoExtract;              /* No subimage extraction */
   int   rlen;                   /* Len. of panel row in bytes */
   int   rlen2;                  /* Twice rlen */
   int   rowbytes;               /* Bytes in rowlen pixels */
   int   stride;                 /* rowbytes with sign of iht */
   int   xoffs;                  /* X offset into bitmap, in bytes */
   int   yoffs;                  /* Y offset stored in header */
   byte  ls,ms;                  /* Least,most signif nibble */
   byte  n2;                     /* Two nibbles of data */

/* If graphics turned off, skip the whole routine */

   if (!_RKG.s.MFActive) return;

/* Set up image extraction according to map type */

   NoExtract = type & BM_NSME;
   MapType   = type & ~BM_NSME;
   switch (type) {
   case BM_BW:                /* Black and White map */
      lbs = BitRemainder(xoffset);
      rbs = 8 - lbs;
      rlen = BytesInXBits(iwd+lbs);
      rowbytes = BytesInXBits(rowlen);
      xoffs = ByteOffset(xoffset);
      jtest = lbs > 0 ? rlen : 0;
      break;
   case BM_GS:                /* Gray scale map */
   case BM_C8:                /* Old SunOS/NCUBE 8-bit color map */
      rlen = iwd;
      rowbytes = rowlen;
      xoffs = xoffset;
      break;
   case BM_GS16:              /* 16-bit gray scale or color */
   case BM_C16:
      rlen = iwd + iwd;
      rowbytes = rowlen + rowlen;
      xoffs = xoffset + xoffset;
      break;
   case BM_C48:               /* 48-bit color */
      rlen = 6*iwd;
      rowbytes = 6*rowlen;
      xoffs = 6*xoffset;
      break;
   case BM_COLOR:             /* Color map */
      abexit(289);
#if 0
      /* Here is what the code might look like for 'lci' = 8 bits */
      rlen = iwd;
      rowbytes = rowlen;
      xoffs = xoffset;
#endif
      break;
   case BM_C24:               /* 24-bit color */
      rlen = iwd + iwd + iwd;
      rowbytes = rowlen + rowlen + rowlen;
      xoffs = xoffset + xoffset + xoffset;
      break;
   case BM_BW|BM_NSME:        /* Black and White, no extract */
      xoffs = lbs = jtest = 0;
      rowbytes = rlen = BytesInXBits(iwd);
      rbs = 8;
      break;
   case BM_GS|BM_NSME:        /* Gray scale, no extract */
   case BM_C8|BM_NSME:        /* Old SunOS/NCUBE 8-bit color map */
      rowbytes = rlen = iwd;
      xoffs = 0;
      break;
   case BM_GS16|BM_NSME:      /* 16-bit gray scale, no extract */
   case BM_C16|BM_NSME:       /* 16-bit color, no extract */
      rowbytes = rlen = iwd + iwd;
      xoffs = 0;
      break;
   case BM_C48|BM_NSME:       /* 48-bit color, no extract */
      rowbytes = rlen = 6*iwd;
      xoffs = 0;
      break;
   case BM_COLOR|BM_NSME:     /* Color map, no extract */
      abexit(289);
#if 0
      rowbytes = rlen = iwd;
      xoffs = 0;
#endif
      break;
   case BM_C24|BM_NSME:       /* 24-bit color, no extract */
      rowbytes = rlen = iwd + iwd + iwd;
      xoffs = 0;
      break;
      }  /* End maptype switch */
   rlen2 = rlen + rlen;

   if (iht > 0) {
      stride = rowbytes;
      yoffs = colht - yoffset;
      prow = array + (NoExtract ? 0 : rowbytes*yoffset);
      }
   else {
      iht = abs(iht);
      stride = -rowbytes;
      yoffs = yoffset;
      prow = array +
         rowbytes*((NoExtract ? iht : yoffset + iht) - 1);
      }

/* If parallel, loop over buffers, making a new header for each
*     in case another node interrupts us.
*  If serial, omit buffer length test, write panel continuously.  */

   while (iht > 0) {

      /* Check whether at least one row will fit in current or
      *  a new buffer.  If not, terminate.  The row length is
      *  effectively (rlen*2) because the data are expanded to
      *  two hex chars per byte.  If map type is BM_BW and the
      *  xoffset doesn't fall on a byte boundary, (rlen*2) can
      *  be up to 3 bytes greater than the actual number of
      *  bytes used by a row.  This should not cause a problem--
      *  it makes the height variable more conservative.  */

      mfbchk(MAX_BM_HEADER + rlen2);

#ifdef PAR
      /* Now that we have a buffer that will hold at least one
      *  row, determine how many rows will actually fit in order
      *  to write an appropriate header.  */
      height =
         (_RKG.MFTopPos - _RKG.MFCurrPos - MAX_BM_HEADER)/rlen2;
      if (height > iht) height = iht;
#else
      height = iht;
#endif

      if (stride > 0) yoffs -= height;

/* Generate segment header */

      *_RKG.MFCurrPos++ = 'B';
      *_RKG.MFCurrPos++ = type + '0';
      *_RKG.MFCurrPos++ = mode + '0';
      i2a(_RKG.MFCurrPos,(int)(xc*1000.0),5,DECENCD);
      i2a(_RKG.MFCurrPos,(int)(yc*1000.0),5,DECENCD);
      i2a(_RKG.MFCurrPos,rowlen,4,DECENCD);     /* pixels */
      i2a(_RKG.MFCurrPos,colht,4,DECENCD);
      i2a(_RKG.MFCurrPos,xoffset,4,DECENCD);    /* pixels */
      i2a(_RKG.MFCurrPos,yoffs,4,DECENCD);
      i2a(_RKG.MFCurrPos,iwd,4,DECENCD);
      i2a(_RKG.MFCurrPos,height,4,DECENCD);

/* Loop over rows of map, sending data to file.  If parallel,
*  above calculations assure that all will fit in same buffer.  */

      for (i=0; i<height; i++,prow+=stride) {
         mfbchk(rlen2);
         pcol = prow + xoffs;

/* Convert map data according to type.
*  (Put switch outside column loop for better performance.)  */

         if (MapType == BM_BW) {       /* Black and White */
            for (j=1; j<=rlen; j++,pcol++) {
               n2 = pcol[0] << lbs;
               if (j < jtest) n2 |= pcol[1] >> rbs;
               ls = n2 & 0x0f;
               ms = (n2 >> 4) & 0x0f;
               *_RKG.MFCurrPos++ =
                  (unsigned char)(ms > 9 ? ms + ('a'-10): ms + '0');
               *_RKG.MFCurrPos++ =
                  (unsigned char)(ls > 9 ? ls + ('a'-10): ls + '0');
               }
            } /* End Black and white */

         else {                        /* Gray or color */
            for (j=0; j<rlen; j++,pcol++) {
               ls = pcol[0] & 0x0f;
               ms = pcol[0] >> 4;
               *_RKG.MFCurrPos++ =
                  (unsigned char)(ms > 9 ? ms + ('a'-10): ms + '0');
               *_RKG.MFCurrPos++ =
                  (unsigned char)(ls > 9 ? ls + ('a'-10): ls + '0');
               }
            } /* End gray or color */

         } /* End row loop */

      if (stride < 0) yoffs += height;

      *_RKG.MFCurrPos++ = '\n';
      iht -= height;
      }  /* End iht loop */

   /* Set code to force next pencol() call to send color --
   *  (this may be unnecessary, derived from an old bug fix) */
   if (_RKG.currcolType == 'K') _RKG.currcolType = 'J';

   }  /* End bitmap() */

