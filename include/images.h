/* (c) Copyright 2017, The Rockefeller University *11116* */
/* $Id: iamges.h 59 2017-01-13 20:34:16Z  $ */
/***********************************************************************
*                              images.h                                *
*     This header file contains declarations and definitions needed    *
*  for including image input and processing routines in applications.  *
*                                                                      *
*  These routines provide an interface beteen libpng routines and      *
*  applications (and when available, other types, e.g. jpeg, tiff).    *
*                                                                      *
*  V1A, 05/20/17, GNR - Newly written                                  *
*  ==>, 05/22/17, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include "sysdef.h"
#include "plotdefs.h"

/* Configuration defs */
#define PNG_HDR_BYTES_TO_CHECK   4

struct imgdef_t {
   void *pimg;             /* Ptr to actual image data */
   byte **pprow;           /* Ptr to list of row ptrs (getpng only) */
   int  height;            /* Image height (rows) */
   int  width;             /* Image width (pixels, not bytes) */
/* If an image is read by a library routine (e.g. readpng), imgfmt
*  will contain BM_xxx codes from plotdefs.h constructed from more
*  detailed information returned in imgdepth, imgmode */
   ui16 imgfmt;            
   ui16 imgops;            /* Optional operation codes:  */
/* Operations for readpng (kops arg) or getpng */
#define IMG_ERRX  0x0001      /* Abexit on errors */
#define IMG_8BIT  0x0002      /* Force pixel depth to 8 bits */
/* Operations for getpng only */
#define IMG_CNTR  0x0010      /* Center image if small */
#define IMG_BAVG  0x0020      /* Set border to average density */
#define IMG_FPXD  0x0040      /* Force pixel depth to imgfmt value */
#define IMG_FRGB  0x0080      /* Force gray/color to imgfmt value */
#define IMG_ERLG  0x0100      /* Force error if image too large */
   ui16 imgdepth;          /* Image depth from retrieval library */
   ui16 imgmode;           /* Image mode from retrieval library */
   int  errex;             /* Nonzero code returned on error */
   };
typedef struct imgdef_t imgdef;

/* Error codes */
#define IER_OPEN     730   /* Unable to open input file */
#define IER_RHDR     731   /* File header not readable */
#define IER_HDRC     732   /* Header does not match png file */
#define IER_PNGX     733   /* Error text returned by libpng fn */
#define IER_PRDS     734   /* png_create_read_struct returned NULL */
#define IER_PINS     735   /* png_create_info_struct returned NULL */
#define IER_BFMT     736   /* Nonstandard input image format */

/* Routines to deal with .png images */
imgdef *readpng(char *fname, int kops);
void freepng(imgdef *pimd);
int  getpng(char *fname, imgdef *pimd);

