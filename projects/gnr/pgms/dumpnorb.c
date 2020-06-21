/* (c) Copyright 2010, The Rockefeller University *11113* */
/***********************************************************************
*                              dumpnorb                                *
*                                                                      *
************************************************************************
*  This is a CNS utility program to dump the metadata from one of      *
*  the files provided in the "small NORB dataset" by Fu Jie Huang      *
*  and Yann LeCun, Courant Institute, New York University.  See:       *
*  Y. LeCun, F.J. Huang, and L. Bottou, Learning methods for generic   *
*  object recognition with invariance to pose and lighting, IEEE       *
*  Computer Society Conference on Computer Vision and Pattern          *
*  Recognition (CVPR), 2004.                                           *
*                                                                      *
*  Synopsis:                                                           *
*     dumpnorb ufile                                                   *
*  Where:                                                              *
*     ufile is a partial file path name, the path to the NORB files,   *
*     up to and including the string "training" or "testing".  The     *
*     rest ("-cat.mat", and "-info.mat") will be supplied internally.  *
*     These two files must exist in the same directory.  The actual    *
*     image file is not read.                                          *
*  Errors:                                                             *
*     Abend codes 540-549 are reserved for the use of this program.    *
*     540      Wrong number of arguments.                              *                                                        *
*     541      Unable to open category file.                           *
*     542      Unable to read category file.                           *
*     543      Unable to close category file.                          *
*     544      Unable to open information file.                        *
*     545      Unable to read information file.                        *
*     546      Unable to close information file.                       *
*     547      Did not reach end-of-file simultaneously on info        *
*              and category files.                                     *
*     548      Unable to allocate memory.                              *
*     549      Information out of documented range in a header file.   *
************************************************************************
*  V1A, 06/02/10, GNR - New program, based on norbrand.c               *
***********************************************************************/

#define MAIN

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "sysdef.h"
#include "rocks.h"

/* Define maximum values for the various selector types */
#define MX_CAT     4
#define MX_LGT     5
#define MX_AZI    17
#define MX_ELV     8
#define MX_INST    9
#define MX_IMGS 24300

/* The NORBHDR structure can contain a NORB .mat file header */

struct NORBHDR {
   ui32  magic;
#define CAT_MAGIC 0x1E3D4C54  /* Magic number for integer matrix */
#define DAT_MAGIC 0x1E3D4C55  /* Magic number for byte matrix */
   ui32  ndim;
   ui32  dim[4];
   } ;

/*---------------------------------------------------------------------*
*                              norbswp4                                *
*                                                                      *
*  Do big-endian vs. little-endian byte swap on a 4-byte value.        *
*---------------------------------------------------------------------*/

#if BYTE_ORDRE > 0
static void norbswp4(void *pi4) {
   byte *pb = (byte *)pi4;
   pb[3] ^= pb[0]; pb[0] ^= pb[3]; pb[3] ^= pb[0];
   pb[2] ^= pb[1]; pb[1] ^= pb[2]; pb[2] ^= pb[1];
   } /* End norbswp4() */
#endif

/*---------------------------------------------------------------------*
*                        dumpnorb main program                         *
*---------------------------------------------------------------------*/

int main(int argc, char *argv[]) {

   char *fname;               /* File name */
   char *prdd;                /* Ptr to data being read */
   struct NORBHDR nhdr;       /* Headers */
   struct {                   /* Current image info */
      int   iinst;               /* Instance */
      int   ielev;               /* Elevation */
      int   iazim;               /* Azimuth */
      int   light;               /* Light */
      } iinf;
   int  fdcat;                /* File descriptor for category file */
   int  fdinf;                /* File descriptor for information file */
   int  imgno;                /* Image number (for fdimg lseek) */
   int  icat;                 /* Current image category */
   int  ldat;                 /* Length of data being read */
   int  lfnm;                 /* Length of file name */

/* Check that there is just one argument -- the partial file name */

   if (argc != 2)
      abexitm(540, "Usage: dumpnorb <NORB file up to '-dat'>");

/* Make space to construct the two file names */

   lfnm = strlen(argv[1]) + 10;
   fname = mallocv(lfnm,"File name");
   if (!fname) abexit(548);

/* Open category file and check header */

   strcpy(fname, argv[1]);
   strcat(fname, "-cat.mat");
   fdcat = open(fname, O_RDONLY);
   if (fdcat < 0)
      abexitme(541, "Unable to open category file");
   ldat = sizeof(struct NORBHDR) - sizeof(ui32);
   prdd = (char *)&nhdr;
   while (ldat > 0) {
      int lr = (int)read(fdcat, prdd, ldat);
      if (lr <= 0)
         abexitme(542, "Unable to read category file");
      prdd += lr, ldat -= lr;
      }
#if BYTE_ORDRE > 0
   norbswp4(&nhdr.magic);
   norbswp4(&nhdr.ndim);
   norbswp4(&nhdr.dim[0]);
   norbswp4(&nhdr.dim[1]);
   norbswp4(&nhdr.dim[2]);
#endif
   if (nhdr.magic != CAT_MAGIC || nhdr.ndim  != 1 ||
         nhdr.dim[0] != MX_IMGS || nhdr.dim[1] != 1 ||
         nhdr.dim[2] != 1)
      abexitm(541, "Bad header in category file");

/* Open info file and check header */

   strcpy(fname, argv[1]);
   strcat(fname, "-info.mat");
   fdinf = open(fname, O_RDONLY);
   if (fdinf < 0)
      abexitme(544, "Unable to open information file");
   ldat = sizeof(struct NORBHDR) - sizeof(ui32);
   prdd = (char *)&nhdr;
   while (ldat > 0) {
      int lr = (int)read(fdinf, prdd, ldat);
      if (lr <= 0)
         abexitme(545, "Unable to read information file");
      prdd += lr, ldat -= lr;
      }
#if BYTE_ORDRE > 0
   norbswp4(&nhdr.magic);
   norbswp4(&nhdr.ndim);
   norbswp4(&nhdr.dim[0]);
   norbswp4(&nhdr.dim[1]);
   norbswp4(&nhdr.dim[2]);
#endif
   if (nhdr.magic != CAT_MAGIC || nhdr.ndim  != 2 ||
         nhdr.dim[0] != MX_IMGS || nhdr.dim[1] != 4 ||
         nhdr.dim[2] != 1)
      abexitm(544, "Bad header in information file");

/* Now read the cat and info files and print the information */

   cryout(RK_P2, "0Image No.  Category  Instance  Azimuth  "
      "Elevation  Lighting", RK_LN2|RK_SUBTTL, NULL);

   for (imgno = 1; imgno<=MX_IMGS; ++imgno) {
      ldat = sizeof(int);     /* Read category from cat file */
      prdd = (char *)&icat;
      while (ldat > 0) {
         int lr = (int)read(fdcat, prdd, ldat);
         if (lr == 0)
            abexitm(542, "Premature eof on category file");
         if (lr < 0)
            abexitme(542, "Unable to read category file");
         prdd += lr, ldat -= lr;
         }
#if BYTE_ORDRE > 0
      norbswp4(&icat);
#endif

      ldat = sizeof(iinf);    /* Read other info from info file */
      prdd = (char *)&iinf;
      while (ldat > 0) {
         int lr = (int)read(fdinf, prdd, ldat);
         if (lr == 0)
            abexitm(545, "Premature eof on information file");
         if (lr < 0)
            abexitme(545, "Unable to read information file");
         prdd += lr, ldat -= lr;
         }
#if BYTE_ORDRE > 0
      norbswp4(&iinf.iinst);
      norbswp4(&iinf.ielev);
      norbswp4(&iinf.iazim);
      norbswp4(&iinf.light);
#endif

/* Sanity checks */

      if (iinf.iazim < 0 || iinf.iazim & 1 || iinf.iazim > 2*MX_AZI)
         abexitm(549, "Invalid azimuth");
      if (iinf.ielev < 0 || iinf.ielev > MX_ELV)
         abexitm(549, "Invalid elevation");
      if (iinf.light < 0 || iinf.light > MX_LGT)
         abexitm(549, "Invalid lighting");
      if (icat < 0 || icat > MX_CAT)
         abexitm(549, "Invalid category");
      if (iinf.iinst < 0 || iinf.iinst > MX_INST)
         abexitm(549, "Invalid instance");

      convrt("(P2,I9,I8,I10,I9,I10,I11)", &imgno, &icat,
         &iinf.iinst, &iinf.iazim, &iinf.ielev, &iinf.light,
         NULL);
      } /* End image loop */

/* Now just check that the two files reached end-of-file */

   if ((int)read(fdcat, &icat, 1) != 0)
      abexitm(542, "Did not reach eof on category file");
   if ((int)read(fdinf, &icat, 1) != 0)
      abexitm(545, "Did not reach eof on information file");

/* Close files */

   cryout(RK_P1, "\0", RK_LN0+RK_FLUSH+1, NULL);
   if (close(fdcat) < 0)
      abexitme(543, "Unable to close category file");
   if (close(fdinf) < 0)
      abexitme(546, "Unable to close information file");

   return 0;

   } /* End dumpnorb */
