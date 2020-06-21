/* (c) Copyright 2017, The Rockefeller University *11114* */
/* $Id: readpng.c 33 2017-05-03 16:14:57Z  $ */
/***********************************************************************
*                              readpng.c                               *
*                                                                      *
*     Routines to read a .png image and return in original format      *
* (Use getpng to read a .png image and force desired size and format)  *
*----------------------------------------------------------------------*
*                               readpng                                *
*                                                                      *
*  Read a png image file and return format info and image data.        *
*                                                                      *
*  Prototyped in:  $(HOME)/include/images.h                            *
*                                                                      *
*  Synopsis:  imgdef *readpng(char *fname, int kops)                   *
*                                                                      *
*  Arguments:                                                          *
*     fname    Path to .png image file to be read                      *
*     kops     OR of selected operation control bits (see images.h)    *
*                                                                      *
*  Returns:                                                            *
*     Pointer to an imgdef structure (defined in images.h) that        *
*     contains information about the format (colored or grayscale,     *
*     image depth), a pointer to the image data, and an error code     *
*     if an error was detected (this is set to zero if result is OK).  *
*     The imgdef should be released when no longer needed to avoid     *
*     memory leakage.  Use getpng() instead to force a fixed image     *
*     format.  With that routine, the same imgdef can be reused for    *
*     multiple images.                                                 *
*                                                                      *
*  Notes:                                                              *
*     Routines assume they are running in a crk-type environment, i.e. *
*     print with cryout etc.  They can run in a serial program or Node *
*     0 of a parallel program.                                         *
*----------------------------------------------------------------------*
*                               freepng                                *
*                                                                      *
*  Routine to free memory allocated for an image read by readpng.      *
*                                                                      *
*  Prototyped in:  $(HOME)/include/images.h                            *
*                                                                      *
*  Synopsis:   void freepng(*pimd)                                     *
*                                                                      *
*  Arguments:                                                          *
*     pimd     Ptr to an imgdef returned by a previous call to         *
*              readpng().                                              *
*                                                                      *
*----------------------------------------------------------------------*
*  Acknowledgements included with the example.c file used here:        *
*  This file has been placed in the public domain by the authors.      *
*  Maintained 1998-2010 Glenn Randers-Pehrson                          *
*  Maintained 1996, 1997 Andreas Dilger                                *
*  Written 1995, 1996 Guy Eric Schalnat, Group 42, Inc.                *
************************************************************************
*  V1A, 05/18/17, GNR - Based on example.c supplied with libpng        *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <png.h>
#include "sysdef.h"
#include "rocks.h"
#include "rockv.h"
#include "plots.h"
#include "images.h"

/*=====================================================================*
*           Error and warning functions for libpng routines            *
*=====================================================================*/


#ifdef GCC
static void png_error_fn(png_structp png_ptr, png_const_charp errtxt)
   __attribute__ ((noreturn));
#endif

static void png_error_fn(png_structp png_ptr, png_const_charp errtxt) {
   imgdef *pimd = (imgdef *)png_ptr;
   pimd->errex = IER_PNGX;
   abexitm(IER_PNGX, (const char *)errtxt);
   } /* End pgn_error_fn() */

static void png_warn_fn(png_structp png_ptr, png_const_charp wrntxt) {

   cryout(RK_P2, "0-->WARNING:  A libpng routine returned the followin"
      "g warning", RK_LN3, "    ", (const char *)wrntxt, RK_LN0, NULL);
   } /* End png_warn_fn() */


/*=====================================================================*
*                               readpng                                *
*=====================================================================*/

imgdef *readpng(char *fname, int kops) {

   imgdef *pimd;                 /* Ptr to result image info struct */

   FILE   *fp = NULL;
   png_structp png_ptr = NULL;   /* (Not my naming convention!) */
   png_infop   info_ptr = NULL;
   int    rc = 0;                /* Return code */
   int    xforms;                /* Image transforms */
   char   hdrbuf[PNG_HDR_BYTES_TO_CHECK];

   /* If this alloc fails, always abexit--there is no space
   *  for an imgdef with an errex for caller to check */
   pimd = callocv(1, sizeof(imgdef), "readpng imgdef");
   pimd->imgops = (ui16)kops;

   /* Open the file and check some signature bytes */
   fp = fopen(fname, "rb");
   if (!fp) { rc = IER_OPEN; goto ErrRet; }
   if (fread(hdrbuf, 1, PNG_HDR_BYTES_TO_CHECK, fp) !=
      PNG_HDR_BYTES_TO_CHECK) { rc = IER_RHDR; goto ErrRet; }
   if (png_sig_cmp(hdrbuf, (png_size_t)0, PNG_HDR_BYTES_TO_CHECK))
      { rc = IER_HDRC; goto ErrRet; }

   /* Create and initialize the png_struct and png_info structs */
   png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
      (png_voidp *)pimd, png_error_fn, png_warn_fn);
   if (!png_ptr) { rc = IER_PRDS; goto ErrRet; }
   info_ptr = png_create_info_struct(png_ptr);
   if (!info_ptr) { rc = IER_PINS; goto ErrRet; }

   /* Initialize I/O */
   png_init_io(png_ptr, fp);
   png_set_sig_bytes(png_ptr, PNG_HDR_BYTES_TO_CHECK);
   png_set_user_limits(png_ptr, UI16_MAX, UI16_MAX);

   /* Set up transforms and read the image (high-level interface) */
   xforms = PNG_TRANSFORM_STRIP_ALPHA;
   if (pimd->imgops & IMG_8BIT) xforms |= PNG_TRANSFORM_STRIP_16;
   png_read_png(png_ptr, info_ptr, xforms, NULL);

   /* Pick up image info */
   pimd->height   = (int)png_get_image_height(png_ptr, info_ptr);
   pimd->width    = (int)png_get_image_width (png_ptr, info_ptr);
   pimd->imgdepth = (int)png_get_bit_depth   (png_ptr, info_ptr);
   pimd->imgmode  = (int)png_get_color_type  (png_ptr, info_ptr);
   /* Construct imgfmt from imgdepth + imgmode */
   if (pimd->imgdepth < 8) pimd->imgfmt = BM_NONSTD;
   else switch (pimd->imgmode) {
   case PNG_COLOR_TYPE_GRAY:
      pimd->imgfmt = pimd->imgdepth == 16 ? BM_GS16 : BM_GS;
      break;
   case PNG_COLOR_TYPE_PALETTE:
#if 0
      pimd->imgfmt = BM_COLOR;
#else  /*** DEBUG ***/
      pimd->imgfmt = BM_GS;
#endif
      break;
   case PNG_COLOR_TYPE_RGB:
      pimd->imgfmt = pimd->imgdepth == 16 ? BM_C48 : BM_C24;
      break;
   default:
      pimd->imgfmt = BM_NONSTD;
      } /* End imgmode switch */

   /* Get list of row pointers and use to reconstruct image */
   pimd->pprow = (byte **)png_get_rows(png_ptr, info_ptr);
   {  size_t isz, rsz;
      int    irow;
      static byte rowmul[] = { 0, 1, 2, 6, 4, 0, 2, 3 };
      rsz = (size_t)rowmul[pimd->imgfmt & 7];
      if (rsz == 0) { rc = IER_BFMT; goto ErrRet; }
      rsz *= (size_t)pimd->width;
      isz = (size_t)pimd->height * rsz;
      byte *pim = mallocv(isz, "readpng image");
      pimd->pimg = pim;
      for (irow=0; irow<pimd->height; ++irow) {
         memcpy(pim, pimd->pprow[irow], rsz); pim += rsz; }
      } /* End isz,rsz local scope */

   /* Return:  Clean up library allocs and close file.  Errors
   *  do same except abexitm if IMG_ERRX and nonzero return code */
ErrRet:
   if (fp) fclose(fp);
   png_destroy_read_struct(&png_ptr, info_ptr ? &info_ptr : NULL, NULL);
   if (rc != 0 && pimd->imgops & IMG_ERRX)
      abexitm(rc, ssprintf(NULL, "readpng error %d", rc));
   pimd->errex = rc;
   return pimd;

   } /* End readpng() */

/*=====================================================================*
*                               freepng                                *
*                                                                      *
*  (Assumes readpng on exit frees everything allocated outside imgdef  *
*=====================================================================*/

void freepng(imgdef *pimd) {

   if (pimd->pimg) free(pimd->pimg);
   free(pimd);
   } /* End of freepng() */

