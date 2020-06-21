/* (c) Copyright 1990, The Rockefeller University *11444* */
/* $Id: vvuser.h 59 2017-01-13 20:34:16Z  $ */
/**********************************************************************/
/*                             vvuser.h                               */
/*                                                                    */
/*    This file contains function prototypes and defined constants    */
/*    for use with the "Versatile Video" routines.                    */
/*                                                                    */
/*    Written by G.N.Reeke, 11/27/90                                  */
/*                                                                    */
/* ==>, 07/01/02, GNR - Last date before committing to svn repository */
/**********************************************************************/

/* Define function prototypes: */

int vvload(int slot);
int vvinit(int slot, int xmin, int xmax, int ymin, int ymax,
   int modes, int nsavg, int kconv);
int vvgrab(int ntavg, int dosynch);
int vvgchk(int dosynch);
int vvget1(byte *image, int cam, int mode,
   int pixel, int npix);
int vvget2(byte *image, int cam, int mode,
   int x0, int y0, int dx, int dy);

/* Define maximum size of video window */

#define VV_MAXX    672        /* Max x dimension of video window */
#define VV_MAXY    486        /* Max y dimension of video window */

/* Define operational modes: */

#define VV_RED       1        /* Red image requested */
#define VV_GRN       2        /* Green image requested */
#define VV_BLU       4        /* Blue image requested */
#define VV_RGB       8        /* 24-bit color requested */
#define VV_233      16        /* 2-3-3 packed color requested */
#define VV_BW       32        /* Black and white requested */
#define VV_TV1     256        /* Data from camera 1 requested */
#define VV_TV2     512        /* Data from camera 2 requested */
#define VV_ATD    4096        /* Automatic time delay requested */

/* Define error return codes: */

#define VV_ARG_ERR   1        /* Argument error return */
#define VV_WIN_ERR   2        /* Video window is violated */
#define VV_MEM_ERR   3        /* Unable to allocate memory */
#define VV_OPN_ERR   4        /* Unable to open vvideo file */
#define VV_RDV_ERR   5        /* Unable to read vvideo file */
#define VV_LDV_ERR   6        /* Unable to load driver on video array */

