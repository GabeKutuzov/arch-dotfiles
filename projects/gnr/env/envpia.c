/* (c) Copyright 1989-2010, The Rockefeller University *21116* */
/* $Id: envpia.c 23 2017-01-16 19:28:30Z  $ */
/***********************************************************************
*              Darwin III Environment Simulation Package               *
*                               ENVPIA                                 *
*     Subroutine envpia:  Put an object on the input array             *
*                                                                      *
************************************************************************
*                                                                      *
*     Call: envpia(Eptr,inpary,robj)                                   *
*     where: Eptr   = pointer to current environment block             *
*            inpary = location of input array                          *
*            robj = pointer to object record in link array             *
*                                                                      *
*     Program transfers pixels from shape record to input array,       *
*        applies any requested rotation and checks for object          *
*        precedence in Z and shine-through of transparent pixels.      *
*     If KSVOC bit is set, just "or" the pixels together--this is a    *
*        kludge to permit combination of visual and touch objects      *
*        using the color mechanism to keep them separate.  It is       *
*        currently supported for the NOROT case only.                  *
*     Program applies 'zero' or 'toroidal' boundary conditions.        *
*     Reference numbers are placed in alternate columns for grip.      *
*     Program assumes pixel boundaries are 'reasonable' as set         *
*        in envctl, namely, 0 <= pxmin < pxmax <= lx and               *
*                           0 <= pymin < pymax <= ly                   *
*     Program further assumes that the documented restriction that     *
*        tsx <= pxmax-pxmin+1, tsy <= pymax-pymin-1 is enforced by     *
*        the calling program (done in envctl).                         *
*     For optimal speed, following cases are handled separately:       *
*        1) No rotation and no transparent or or'ed pixels             *
*        2) No rotation, but must check transparent or or'ed pixels    *
*        3) Rotation or nonintegral coords, with or without or'ing     *
*              and transparency                                        *
*     Each of these is further divided according to whether boundary   *
*        condition is 'zero' or 'toroidal'.                            *
************************************************************************
*  Written by George N. Reeke                                          *
*  V4A, 01/21/89, SCD - Converted to 'C' from version V3A              *
*  Rev, 07/24/90, GNR - Change centering convention - NOROT            *
*  Rev, 09/19/90, JMS - Change sra1 macro const from -1 to 1           *
*  Rev, 08/27/92, GNR - Add KSVOC option                               *
*  Rev, 02/22/03, GNR - Change s_type to stim_type                     *
*  ==>, 02/22/03, GNR - Last mod before committing to svn repository   *
*  Rev, 12/31/08, GNR - Replace jm64nh with mssle                      *
*  V8H, 12/01/10, GNR - Separate env.h,envglob.h w/ECOLMAXCH change    *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sysdef.h"
#include "envglob.h"
#include "rkarith.h"

#define TWOPI 6.283185
#define TRANSPARENT 256    /* Transparent pixel format flag */

/* Values the variable 'manip_typ' takes on: */
#define NOROT    0
#define NOTSP    1
#define ISROT    2

/* Values the variable 'bnd_typ' takes on: */
#define ZERO     0
#define TOROIDAL 1

/* Values the variable 'bnd_chk' takes on: */
#define BND_ZERO  0
#define BND_LTB   1
#define BND_FTB   2

/* Perform a right shift that preserves sign and truncates fraction */
#define sra1(x)  (((x) >= 0) ? ((x) >> 1) : -((1 - (x)) >> 1))
#define sra16(x) (((x) >= 0) ? ((x) >> 16) : -((s16frac-(x)) >> 16))

int envpia(struct ENVDEF *Eptr,stim_type *inpary,struct OBJDEF *robj) {

   stim_type *inp_ptr;        /* Pointer into input array          */
   byte *pxl_ptr;             /* Pointer to byte to be moved into
                              *  input array                       */
   byte *id_ptr;              /* Pointer to ID bytes of input array*/
   byte *rot_pxl;             /* Pointer to destination of rotated
                              *   pixel                            */
   stim_type *xfence;         /* Pointer used to cue wrapping      */
   stim_type *yfence;         /* Pointer used to cue wrapping      */

   long xobjsiz;              /* X size of object                  */
   long yobjsiz;              /* Y size of object                  */
   long xwinsiz;              /* X size of window                  */
   long ywinsiz;              /* Y size of window                  */
   long xoffset;              /* X offset from center to upper left
                              *  corner                            */
   long yoffset;              /* Y offset from center to upper left
                              *  corner                            */
   long sourceskip;           /* Used for advancing pointer to
                              *  object pixels                     */
   long firstmove;
   long destskip;             /* Used to advance pointer to input
                              *  array                             */
   long xwrap;                /* Thing added to dest pointer when
                              *  wrap occurs in X                  */
   long lcollen;              /* Number of bytes within a column to
                              *  load                              */
   long right_edge;           /* (RXL when not xobjsiz)            */
   long bottom_edge;          /* (RYL when not yobjsiz)            */
   long xstart;               /* X start (R0)                      */
   long ystart;               /* Y start (R0)                      */
   long ncols;                /* Column counter                    */
   long xctr;                 /* Column counter                    */
   long lpxfix;               /* 4096*xwinsiz - lpxmin             */
   long lpyfix;               /* 4096*ywinsiz - lpymin             */
   long sinrot;               /* (S16) Sine of rotation            */
   long cosrot;               /* (S16) Cosine of rotation          */
   long xcoords;              /* Starting X coord (used when
                              *  there is rotation)                */
   long ycoords;              /* Starting Y coord (used when
                              *  there is rotation)                */

/* ---------------  Local copies of various items ---------------- */
   long lpxmin;               /* Local copy of pxmin               */
   long lpxmax;               /* Local copy of pxmax               */
   long lpymin;               /* Local copy of pymin               */
   long lpymax;               /* Local copy of pymax               */
   long lly;                  /* Local copy of ly                  */
   long llymul2;              /* Local copy of lymul2              */
   long llxsub1;              /* Local copy of lxsub1              */
   long llysub1;              /* Local copy of lysub1              */
   byte lref;                 /* Local copy of irf (reference #)   */
/*------------------------- Constants ---------------------------- */
   long s16frac = 0xffff;     /* Used for extracting fraction of
                              *  (S16) variables                   */
   short kx1;                 /* Copy of kx+1 for X coord shifting */
   char manip_typ;            /* Selects code to handle transparent
                              *  pixels, or rotation, or neither, or
                              *  both as needed                    */
   char bnd_typ;              /* Selects code to handle requested
                              *  boundary type                     */
   char bnd_chk;              /* Selects code to check boundaries
                              *  when rotation is being done       */
   char or_type;              /* TRUE if or'ing pixels             */

/* Error exit if no shape found--no msg, is coding error */

   if (!robj || !robj->pshape) abexit(160);

/* Set up initial X,Y loop limits */

   xobjsiz = robj->pshape->tsx;
   yobjsiz = robj->pshape->tsy;

   kx1 = Eptr->eky + 1;    /* For X coord shifting */

/* Make local copies of a tasteful selection of variables */

   lly = Eptr->ly;
   llymul2 = Eptr->lymul2;
   lref = (byte)robj->irf;
   lpxmin = robj->pxmin;
   lpxmax = robj->pxmax;
   lpymin = robj->pymin;
   lpymax = robj->pymax;

/* If toroidal, prepare the window size parameters.
*  Prepare boundary control switch.  */

   or_type = (Eptr->kcolor == ENV_ORCOL);
   /* The following six variables are only used with TOROIDAL,
   *  but are set here to eliminate uninitialized warnings.  */
   llxsub1 = Eptr->lxsub1;
   llysub1 = Eptr->lysub1;
   xwinsiz = lpxmax - lpxmin + 1;
   ywinsiz = lpymax - lpymin + 1;
   lpxfix = (xwinsiz<<12) - lpxmin;
   lpyfix = (ywinsiz<<12) - lpymin;
   bnd_typ = (robj->placebounds & OPLTB) ? TOROIDAL : ZERO;

/* Assign 'manip_typ' to select code to rotate or handle
*  transparent pixels or handle toroidal boundaries as needed. */

   if (robj->objflag & OBJNI) manip_typ = ISROT;
   else {
      float fwork;           /* orient/TWOPI                        */

/* Test for whether mod(rot,2pi) ~ 0
*
*  It is worth doing a sophisticated test here.  Idea:
*  If the angular difference from a multiple of 2pi exceeds
*  1/max(tsx,tsy)/256, then edge may move enough to generate
*  a difference of 1 density unit in some pixel, and rotation
*  is necessary.  */

      fwork = robj->orient >= 0.0 ?
         robj->orient/TWOPI : robj->orient/-TWOPI;
      /* If difference is great enough (see comment above) go rotate */
      if (((fwork - (long)fwork)*max(xobjsiz,yobjsiz)) >= (1.0/256.0))
         manip_typ = ISROT;
      else {

/* Rotation not needed.
*  Make offsets from center to upper left corner pixel and
*  complete calculation of 'manip_typ'.  */

         xoffset = xobjsiz>>1;
         yoffset = yobjsiz>>1;
         if (or_type) manip_typ = NOROT;
         else if (!(robj->pshape->tfm & TRANSPARENT))
            manip_typ = NOTSP;
         else if ((robj->pshape->tfm & 3) ||
            (Eptr->eilo == 0)) manip_typ = NOROT;
         else manip_typ = NOTSP; /* End of 'manip_typ' calculation */
         }
      }

   switch (manip_typ) {

/*---------------------------------------------------------------------*
*                                                                      *
*            No rotation, no or'ing or transparent pixels              *
*                                                                      *
*---------------------------------------------------------------------*/

case NOTSP:
      inp_ptr = inpary;
      pxl_ptr = (byte *)dslpix(robj->pshape);

      switch (bnd_typ) {
   case ZERO:         /* Zero boundaries */
         ncols  = xobjsiz;
         xstart  = sra16(robj->icurx) - xoffset; /* Use integer part
                                                 *     of icurx */
         right_edge = xobjsiz + xstart - 1;
         if (xstart < lpxmin) {
            long xskip;
            /* Start is left of border */
            xskip = lpxmin - xstart;
            xstart = lpxmin;
            /* Reduce column count by X skip.
            *  Get out if there is nothing left. */
            if ((ncols -= xskip) <= 0) return ENV_NORMAL;
            pxl_ptr += xskip*yobjsiz;
            }
         inp_ptr += xstart<<kx1;    /* Offset destination by
                                    *  2*nsy*xstart */

         /* Now check right boundary */
         if ((right_edge -= lpxmax) > 0)
            if ((ncols -= right_edge) <= 0) return ENV_NORMAL;

         lcollen = yobjsiz;
         ystart = sra16(robj->icury) - yoffset; /* Use integer part
                                                *     of icury */
         bottom_edge = yobjsiz + ystart - 1;
         if (ystart < lpymin) {
            long yskip;
            /* Start is above border */
            yskip = lpymin - ystart;
            ystart = lpymin;
            /* Reduce bytes-per-column count by Y skip.
            *  Get out if there is nothing left.  */
            if ((lcollen -= yskip) <= 0) return ENV_NORMAL;
            pxl_ptr += yskip;
            }
         inp_ptr += ystart;

         /* Now check bottom boundary */
         if ((bottom_edge -= lpymax) > 0)
            if ((lcollen -= bottom_edge) <= 0) return ENV_NORMAL;

         /* Move the object to the input array */
         for (; ncols>0; ncols--) {
             /* Copy in a column of the object (or a portion
             * thereof) to the input array. */
             memcpy((void *)inp_ptr, (void *)pxl_ptr, lcollen);
             pxl_ptr += yobjsiz; /* Advance to next column of
                                 *  object. */
             inp_ptr += lly;     /* Position destination pointer
                                 *  on flag byte column. */
             /* Load in reference numbers. */
             memset((void *)inp_ptr, robj->irf, lcollen);
             inp_ptr += lly;     /* Advance to next image column */
             }  /* End of loop over columns */
      /*------- End of ZERO boundary case -------*/

      return ENV_NORMAL;    /* All done */

   case TOROIDAL:          /* Toroidal boundaries */

/* Determine starting X and offset in input array */

   /* Right shift on icur[xy] instead of divide by (1 << 16) because
   *  IBM used halfword op to get high 16 bits - don't care about
   *  fraction */
      xstart = ((xwinsiz<<12) /* Big pos mult so rmndr can't be < 0 */
              + (robj->icurx >> 16)    /* Integer part of X coord */
              - xoffset                /* Less half-width of object */
              - lpxmin) % xwinsiz;     /* Now 'xstart' is X coord
                                       *  modulo window width. */
      inp_ptr += (xstart + lpxmin)<<kx1;     /* Advance inp_ptr
                                             *  to left X */

/* Determine starting Y and offset in input array */

      ystart = ((ywinsiz<<12) /* Big pos mult so rmndr can't be < 0 */
              + (robj->icury >> 16)    /* Integer part of Y coord */
              - yoffset                /* Less half-hght of object */
              - lpymin) % ywinsiz
              + lpymin;                /* Bring back to input array
                                       *  origin. */
      inp_ptr += ystart;      /* Offset into input array */

      if ((firstmove = lpymax - ystart + 1) >= yobjsiz) {
         destskip = llymul2 - yobjsiz;
         firstmove = yobjsiz;
         }
      else destskip = llymul2 - yobjsiz + ywinsiz;

      xfence = inpary + ((lpxmax + 1)<<kx1);
      xwrap = xwinsiz<<kx1;       /* X wrap (RX) */
      id_ptr = inp_ptr + lly;

/* Move object to input array in strips */

      for (ncols=xobjsiz; ncols>0; ncols--) {  /* Loop over columns */
          long colseg;        /* Size in bytes of the chunks in
                              *  columns are copied in to input array
                              * (RDL) */
          long bytes_to_go;   /* Number of bytes of present column
                              *  that remain to be copied. (RSL) */

          /* Prepare for loop over column segments */
          colseg = firstmove;    /* Begin with length of first
                                 *     move. */
          bytes_to_go = yobjsiz; /* Move a whole column of the
                                 *     object. */
MOV_COL_SEG:     /* Top of non-standard loop over column segments */
         /* Load in reference numbers */
         memset((void *)id_ptr,robj->irf,colseg);
         id_ptr += colseg;
         memcpy((void *)inp_ptr, (void *)pxl_ptr, colseg);
         inp_ptr += colseg;
         pxl_ptr += colseg;
         if ((bytes_to_go-=colseg) > 0) {
            /* There is some of this column left that must be
            *  wrapped vertically. Back destination pointers
            *  to upper strip and set to complete column. */
            inp_ptr -= ywinsiz;
            id_ptr -= ywinsiz;
            colseg = bytes_to_go;
            goto MOV_COL_SEG;
            }  /* End of loop over column segments */

         inp_ptr += destskip;   /* Add unused length of 2 columns */
         id_ptr += destskip;    /* Ditto */

         if (inp_ptr >= xfence) {  /* See if we need to wrap */
            /* We have gone past right window fence.
            *  Wrap back to left edge. */
            inp_ptr -= xwrap;
            id_ptr -= xwrap;
            }
         } /* End loop over columns */
      /*------- End of TOROIDAL boundary case -------*/
      return ENV_NORMAL;        /* All done */
      } /* End of boundary-type switch */

/*---------------------------------------------------------------------*
*                                                                      *
*           No rotation, some or'ing or transparent pixels             *
*                                                                      *
*---------------------------------------------------------------------*/

case NOROT:
      inp_ptr = inpary;
      pxl_ptr = (byte *)dslpix(robj->pshape);

      switch (bnd_typ) {

/* No rotation, transparent, zero boundaries */

   case ZERO:
         destskip = llymul2;
         sourceskip = yobjsiz;
         ncols = xobjsiz;

         xstart = sra16(robj->icurx) - xoffset;
         right_edge = xobjsiz + xstart - 1;

         if (xstart < lpxmin) {  /* See if we're left of border*/
            long xskip;
            /* Start is left of the border. */
            xskip = lpxmin - xstart;
            xstart = lpxmin;
            /* Reduce column count by X skip.
            *  Get out if there is nothing left. */
            if ((ncols -= xskip) <= 0) return ENV_NORMAL;
            pxl_ptr += xskip * yobjsiz;
            }
         inp_ptr += xstart<<kx1;    /* Offset destination by
                                    *  2*nsy*xstart */

         /* Now check right boundary */
         if ((right_edge -= lpxmax) > 0)
            if ((ncols -= right_edge) <= 0) return ENV_NORMAL;

         lcollen = yobjsiz;
         ystart = sra16(robj->icury) - yoffset; /* Use integer part
                                                *     of icury */
         bottom_edge = yobjsiz + ystart - 1;

         if (ystart < lpymin) {  /* See if we're above border */
            long yskip;
            /* Start is above border */
            yskip = lpymin - ystart;
            ystart = lpymin;
            /* Reduce bytes-per-column count by Y skip.
            *  Get out if there is nothing left. */
            if ((lcollen -= yskip) <= 0) return ENV_NORMAL;
            pxl_ptr += yskip;
            }
         inp_ptr += ystart;

         /* Now check bottom boundary */
         if ((bottom_edge -= lpymax) > 0)
            if ((lcollen -= bottom_edge) <= 0) return ENV_NORMAL;

         destskip -= lcollen;
         sourceskip -= lcollen;
         for (; ncols>0; ncols--) {
            byte *collim;     /* Limit of loop over pixels */
            for (collim=pxl_ptr+lcollen; pxl_ptr<collim; pxl_ptr++) {
               *(inp_ptr + lly) = lref;      /* Load ref num */
               /* Handle "or" pixels option */
               if (or_type) {
                  *inp_ptr |= *pxl_ptr;
                  }
               /* Otherwise, only move in non-transparent pixels. */
               else if (*pxl_ptr) {
                  *inp_ptr = *pxl_ptr;       /* Load pixel */
                  }
               inp_ptr++;
               }  /* End of loop over pixels */
            pxl_ptr += sourceskip;
            inp_ptr += destskip;
            }  /* End of loop over columns */
         /*------- End of ZERO boundary-type -------*/
         return ENV_NORMAL;

/* No rotation, transparent, toroidal boundaries */

   case TOROIDAL:
      xstart = ((xwinsiz<<12) /* Big pos mult so rmndr can't be < 0 */
              + sra16(robj->icurx)     /* Integer part of X coord */
              - xoffset                /* Less half-width of object */
              - lpxmin) % xwinsiz;     /* Now 'xstart' is X coord
                                       *  modulo window width. */
      inp_ptr += (xstart + lpxmin)<<kx1;  /* Advance inp_ptr
                                          *  to left X */
      yfence = inp_ptr + lpymax;
      destskip = llymul2 - yobjsiz;

/* Determine starting Y and offset in input array */

      ystart = ((ywinsiz<<12) /* Big pos mult so rmndr can't be < 0 */
              + sra16(robj->icury)     /* Integer part of X coord */
              - yoffset                /* Less half-hght of of object*/
              - lpymin) % ywinsiz
              + lpymin;                /* Bring back to input array
                                       *  origin. */
      inp_ptr += ystart;      /* Offset into input array */

      if ((inp_ptr + yobjsiz) > yfence) destskip += ywinsiz;
      xfence = inpary + ((lpxmax + 1)<<kx1);
      xwrap = xwinsiz<<kx1;

      for (ncols=xobjsiz; ncols>0; ncols--) {
         byte *collim;   /* Limit for loop over pixels */
         for (collim=pxl_ptr+yobjsiz; pxl_ptr<collim; pxl_ptr++) {
            /* Handle "or" pixels option */
            *(inp_ptr + lly) = lref;      /* Load ref num */
            if (or_type) {
               *inp_ptr |= *pxl_ptr;
               }
            /* Only move in non-transparent pixels. */
            else if (*pxl_ptr) {
               *inp_ptr = *pxl_ptr;         /* Load pixel */
               }
            /* Advance input array pointer and see if
            *  we're past the Y fence */
            if (++inp_ptr > yfence) inp_ptr -= ywinsiz;
            }  /* End of loop over pixels */
         yfence += llymul2;
         if ((inp_ptr += destskip) >= xfence) {
            inp_ptr -= xwrap;
            yfence -= xwrap;
            }
         } /* End of loop over columns */
      /*------- End of TOROIDAL-type boundary -------*/
      return ENV_NORMAL;
      } /* End of boundary-type switch */

/*---------------------------------------------------------------------*
*                                                                      *
*        Here if there is rotation or nonintegral translation.         *
*                                                                      *
*---------------------------------------------------------------------*/

case ISROT:

/*
*           The program loops over all the pixels in the source
*     object.  Transparent pixels are skipped.  For other pixels,
*     the four integral locations surrounding the rotated coords
*     of the pixel are examined.  If a location already belongs
*     to this object, a contribution is added in for the current
*     pixel according to its distance from the point (binormal
*     interpolation).  Otherwise, the old value is replaced with
*     the first contribution from the current object and the
*     referenece number is set to identify the location as being
*     part of this new object.
*
*           There are three versions of the storage routine
*     according to the boundary conditions: (1) zero boundaries,
*     (2) toroidal on full input array, (3) toroidal on subset.
*     Because the rotation angle can reverse the direction of
*     looping in destination x, y, or both, the simple expedient
*     of checking boundary conditions individually for each point
*     is used.  Some time could be saved by figuring out, for the
*     zero case, when a line leaves the window and terminating the
*     loop there.  In the toroidal case, it doesn't make much
*     difference, because all points eventually get stored.
*/

/* Determine which of the three boundary conditions is in use.
*  Set 'bnd_chk' to execute appropriate check code.
*  Calculate particular constants needed for each method.  */

      if (bnd_typ == TOROIDAL) {
         bnd_chk = (xwinsiz==Eptr->lx && ywinsiz==lly) ?
            BND_FTB : BND_LTB;
         }
      else bnd_chk = BND_ZERO;
      /* Done calculating 'bnd_chk' */

/* Prepare sin and cos of rotation angle.
*    The rotated coords are maintained by 'strength reduction',
*    i.e. coords are incremented for each pixel visited, rather
*    than multiplied when a hit is found.  (Note: the wisdom of
*    this move depends on how many black pixels are in a typical
*    shape.  Anyway, we're in fixed point, so no accuracy is lost.)
*/
      cosrot = (long)(cos((double)(robj->orient)) * (double)(1L<<16));
      sinrot = (long)(sin((double)(robj->orient)) * (double)(1L<<16));

/* Get rotated coords of upper left hand corner of object (S16) */

      xcoords = sra1(-cosrot*xobjsiz - sinrot*yobjsiz) + robj->icurx;
      ycoords = sra1(-cosrot*yobjsiz + sinrot*xobjsiz) + robj->icury;

/* Main pixel rotation loop */

      pxl_ptr = (byte *)dslpix(robj->pshape);
      /*Initialize pointer to object*/
      for (xctr=xobjsiz; xctr>0; xctr--) {
         long xcoord;      /* (S16) Rotated X coordinate   */
         long ycoord;      /* (S16) Rotated Y coordinate   */
         long ixcoord;     /* Integer part of xcoord       */
         long iycoord;     /* Integer part of ycoord       */
         long yctr;        /* Counter for loop over pixels */

         /* Prepare for Y loop over pixels */
         xcoord = xcoords;
         ycoord = ycoords;
         for (yctr=yobjsiz; yctr>0; yctr--) {
            long wkpix;             /* Work space        */
            long yfrc;              /* (S16) Y fraction  */
            long pixfrcx;           /* Frac ( X )*PIXEL  */
            long pixfrc1mx;         /* Frac (1-X)*PIXEL  */
            long pixel;             /* Pixel value.
                                    *  Used by envpia.h  */

            /* Skip transparent pixels */
            if (!(*pxl_ptr)) goto SKIP_PIXEL;

            ixcoord = sra16(xcoord);   /* Integer x coord */
            iycoord = sra16(ycoord);   /* Integer y coord */
            pixfrcx = xcoord & s16frac;
            /* yfrc = (1.0-yfrac) (S16) */
            yfrc = 65536 - (ycoord & s16frac);
            wkpix = *pxl_ptr;
            pixfrc1mx = ((long)wkpix)<<16;
            pixfrcx *= wkpix;
            pixfrc1mx -= pixfrcx;
            /* S16+S16-32 = S0 */
            pixel = mssle(yfrc, pixfrc1mx, -BITSPERUI32, 0);
            /* Preset value for second pixel here */
            pixfrc1mx = (pixfrc1mx>>16) - pixel;

/* First point is at (X,Y) with value (1-XFRAC)*(1-YFRAC)*PIXEL */
#define BND_LABEL1 OUT_OF_WINDOW1
#include "envpia.h"

               iycoord++;
               pixel = pixfrc1mx;

/* Second point is at (X,Y+1) with value (1-XFRAC)*YFRAC*PIXL */
#undef BND_LABEL1
#define BND_LABEL1 OUT_OF_WINDOW2
#include "envpia.h"

               ixcoord++;
               iycoord--;
               /* S16+S16-32 = S0 */
               pixel = mssle(yfrc, pixfrcx, -BITSPERUI32, 0);
               pixfrcx = (pixfrcx>>16) - pixel;


/* Third point is at (X+1,Y) with value XFRAC*(1-YFRAC)*PIXEL */
#undef BND_LABEL1
#define BND_LABEL1 OUT_OF_WINDOW3
#include "envpia.h"

               iycoord++;
               pixel = pixfrcx;   /* Use precalculated pixel value */


/* Fourth point is at (X+1,Y+1) with value XFRAC*YFRAC*PIXEL */
#undef BND_LABEL1
#define BND_LABEL1 OUT_OF_WINDOW4
#include "envpia.h"

SKIP_PIXEL:    xcoord += sinrot;  /* Increment rotated coords */
               ycoord += cosrot;
               pxl_ptr++;
               } /* End of loop over pixels */
            xcoords += cosrot;
            ycoords -= sinrot;
            } /* End of loop over columns */
         } /* End of 'manip_typ' switch */
      return ENV_NORMAL;
   } /* End of envpia() */
