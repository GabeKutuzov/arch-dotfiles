/* (c) Copyright 1992-2014, The Rockefeller University *21114* */
/* $Id: bitmap.c 29 2017-01-03 22:07:40Z  $ */
/***********************************************************************
*                              bitmap()                                *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void bitmap(const unsigned char *array, int rowlen, int colht,   *
*     float xc, float yc, int xoff, int yoff, int iwd, int iht,        *
*     int type, int mode)                                              *
*                                                                      *
*  DESCRIPTION:                                                        *
*     Plots an image on the screen in the specified location and size. *
*                                                                      *
*  ARGUMENTS:                                                          *
*                                                                      *
*  RETURN VALUES:                                                      *
*     None.                                                            *
************************************************************************
*  Version 1, 06/23/92, ROZ                                            *
***********************************************************************/

#include "mfint.h"
#include "plots.h"

#define MAX_BM_HEADER 38         /* Max BitMap header size plus
                                 *  one for LF at end of pane */

void bitmap(const unsigned char *array, int rowlen, int colht,
      float xc, float yc, int xoff, int yoff, int iwd, int iht,
      int type, int mode) {

   Win *pw = _RKG.pcw;           /* Locate info for current window */
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

   if (!(*pw.MFActive)) return;

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
   case BM_COLOR:             /* Color map */
      abexit(289);
#if 0
      /* Here is what the code might look like for 'lci' = 8 bits */
      rlen = iwd;
      rowbytes = rowlen;
      xoffs = xoffset;
#endif
      break;
   case BM_C16:               /* 16-bit color */
      rlen = iwd + iwd;
      rowbytes = rowlen + rowlen;
      xoffs = xoffset + xoffset;
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
   case BM_COLOR|BM_NSME:     /* Color map, no extract */
      abexit(289);
#if 0
      rowbytes = rlen = iwd;
      xoffs = 0;
#endif
      break;
   case BM_C16|BM_NSME:       /* 16-bit color */
      rowbytes = rlen = iwd + iwd;
      xoffs = 0;
      break;
   case BM_C24|BM_NSME:       /* 24-bit color */
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

