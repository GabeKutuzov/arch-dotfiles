/* (c) Copyright 1995, The Rockefeller University *11113* */
/***********************************************************************
*                               dumpNEMA                               *
*                                                                      *
*     Program to dump contents of a 3dviewnix ACR-NEMA image file to   *
*  stdout.  Not all of the header item recognized by dumpNEMA are      *
*  assigned reasonable values by the current implementation of         *
*  3dviewnix.                                                          *
*                                                                      *
*  Synopsis:   dumpNEMA("filespec")                                    *
*  Argument: 'filespec' is the path name of the 3dviewnix file to      *
*              be dumped.                                              *
*                                                                      *
*  Restriction:  This routine does not worry about byte order swapping.*
************************************************************************
*  Initial version, 01/13/95, G.N. Reeke                               *
***********************************************************************/

#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "NEMA.HDR"

/*---------------------------------------------------------------------*
*                           lengck function                            *
*                                                                      *
*  This function checks that the length of a header item is what is    *
*  expected and generates a fatal error if not.                        *
*---------------------------------------------------------------------*/

void lengck(int lfnd, int lexp, char *msgtxt) {

   if (lfnd != lexp) {
      char emsg[120];
      sprintf(emsg,
         "dumpNEMA: Header item %s has length %d, expected %d.",
         msgtxt,lfnd,lexp);
      fatal(emsg);
      }

   } /* End lengck() */


/*---------------------------------------------------------------------*
*                           readck function                            *
*                                                                      *
*  This function performs a normal read() and checks that the number   *
*  of bytes returned is the number requested, returning a fatal error  *
*  if not.                                                             *
*---------------------------------------------------------------------*/

void readck(int fd, char *buf, int n) {

   if (read(fd, buf, n) != n)
      fatal("dumpNEMA: Error reading input file.");

   } /* End readck() */


/*---------------------------------------------------------------------*
*                           skipck function                            *
*                                                                      *
*  This function skips over n bytes of data in a file and returns a    *
*  fatal error if the lseek is not successful.                         *
*---------------------------------------------------------------------*/

void skipck(int fd, int n) {

   if (lseek(fd, n, SEEK_CUR) < 0)
      fatal("dumpNEMA: Error skipping data in input file.");

   } /* End skipck() */


/*---------------------------------------------------------------------*
*                        getANstring function                          *
*                                                                      *
*  This function reads an alphanumeric string (NEMA data type AN)      *
*  into a buffer and checks for read errors and excessive length.      *
*---------------------------------------------------------------------*/

void getANstring(int fd, char *string, int len) {

   if ((len + 1) > MAX_ANLen)
      fatal("dumpNEMA: Header data string too long.");
   readck(fd, string, len);
   string[len] = '\0';

   } /* End getANstring() */


/*---------------------------------------------------------------------*
*                       dumpNEMAstring function                        *
*                                                                      *
*  This function allocates space for a string of length given in a     *
*  NEMA file header item, then reads the string into the space and     *
*  adds a standard C terminator.                                       *
*---------------------------------------------------------------------*/

void dumpNEMAstring(int fd, char **string, int len) {

   char *p = *string = malloc(len + 1);
   readck(fd, p, len);
   p[len] = '\0';

   } /* End dumpNEMAstring() */


/*---------------------------------------------------------------------*
*                          dumpNEMA function                           *
*---------------------------------------------------------------------*/

int main(int argc, char *argv[]) {

   HeaderItem H;              /* Header item header */
   int fd;                    /* File descriptor for input file */
   int icol,irow,isec;        /* Index values for reading image */
   int n;                     /* Sizes of various strings */
   int ncol,nrow,nsec;        /* Image dimensions */
   short bisecs;              /* Number of sections (short) */
   short bitspden;            /* Bits per density value */
   short colsrows[2];         /* Columns, rows in section */
   short imagedim;            /* Image dimensionality */
   short intpvec;             /* Integers per vector */
   short lowhigh[2];          /* Low and high density bits */
   short vals;                /* Values per cell */
   char anbuf[MAX_ANLen];     /* Buffer for alphanumeric fields */
   short *rowbuf;             /* Buffer for one row of image data */
   char *study;               /* Study id */
   char *series;              /* Series id */
   char *sdate;               /* Date of study */
   char *stime;               /* Time of study */

/* Check for the correct numbers of arguments */

   if (argc != 2)
      fatal("dumpNEMA: One input arg--the file path--"
         "is required.");

/* Open the file */

   if ((fd = open(argv[1], O_RDONLY)) < 0)
      fatal("dumpNEMA: Unable to open input file.");

/* Read file header, switch according to item type found */

   for (;;) {

      readck(fd, (char *)&H, sizeof(HeaderItem));
      switch (H.group) {

/* Handle Identification Information Group */

case GRP_IdentInfo:

         switch (H.item) {

         case GII_StudyDate:
            dumpNEMAstring(fd, &sdate, H.length);
            HdrFlags |= GOT_StudyDate;
            break;

         case GII_StudyTime:
            dumpNEMAstring(fd, &stime, H.length);
            HdrFlags |= GOT_StudyTime;
            break;

         default:
            skipck(fd, H.length);
            break;

            } /* End GII switch */

         break;

/* General Information Group is currently ignored */

/* Patient Information Group is currently ignored */

/* Acquisition Information Group is currently ignored */

/* Handle Relationship Information Group */

case GRP_RelInfo:

         switch (H.item) {

         case GRI_StudyID:
            dumpNEMAstring(fd, &study, H.length);
            HdrFlags |= GOT_StudyID;
            break;

         case GRI_Series:
            dumpNEMAstring(fd, &series, H.length);
            HdrFlags |= GOT_Series;
            break;

         default:
            skipck(fd, H.length);
            break;

            } /* End GRI switch */

         break;

/* Image Presentation Information Group is currently ignored */

/* Handle Scene Information Group */

case GRP_SceneInfo:

         switch (H.item) {

         case GSC_Dimension:
            lengck(H.length, sizeof(short), "ImageDimension");
            readck(fd, (char *)&imagedim, sizeof(imagedim));
            HdrFlags |= GOT_Dimension;
            break;

         case GSC_ValsPerCell:
            lengck(H.length, sizeof(short), "ValsPerCell");
            readck(fd, (char *)&vals, sizeof(vals));
            HdrFlags |= GOT_ValsPerCell;
            break;

         case GSC_MinDens:
            getANstring(fd, anbuf, H.length);
            sscanf(anbuf, "%le", &denmin);
            HdrFlags |= GOT_MinDens;
            break;

         case GSC_MaxDens:
            getANstring(fd, anbuf, H.length);
            sscanf(anbuf, "%le", &denmax);
            HdrFlags |= GOT_MaxDens;
            break;

         case GSC_IntegersPerVector:
            lengck(H.length, sizeof(short), "IntegersPerVector");
            readck(fd, (char *)&intpvec, sizeof(intpvec));
            HdrFlags |= GOT_IntegersPerVector;
            break;

         case GSC_BitsPerDensity:
            lengck(H.length, sizeof(short), "BitsPerDensity");
            readck(fd, (char *)&bitspden, sizeof(bitspden));
            HdrFlags |= GOT_BitsPerDensity;
            break;

         case GSC_LowHighBits:
            lengck(H.length, sizeof(lowhigh), "Low&HighBits");
            readck(fd, (char *)lowhigh, sizeof(lowhigh));
            HdrFlags |= GOT_LowHighBits;
            break;

         case GSC_ColsRows:
            lengck(H.length, sizeof(colsrows), "Cols&Rows");
            readck(fd, (char *)colsrows, sizeof(colsrows));
            ncol = (int)colsrows[0];
            nrow = (int)colsrows[1];
            HdrFlags |= GOT_ColsRows;
            break;

         /* N.B.  This code must be modified if it becomes
         *  necessary to handle images of > 3 dimensions.  */
         case GSC_Sections:
            lengck(H.length, sizeof(short), "NumberOfSections");
            readck(fd, (char *)&bisecs, sizeof(bisecs));
            nsec = (int)bisecs;
            HdrFlags |= GOT_Sections;
            break;

         case GSC_ColRowIncrs:
            getANstring(fd, anbuf, H.length);
            sscanf(anbuf, "%le\\%le", &dxcol, &dyrow);
            HdrFlags |= GOT_ColRowIncrs;
            break;

         /* N.B. The GSC_SliceLocations item is currently
         *  ignored.  This program assumes all slices are
         *  spaced evenly.  */
         default:
            skipck(fd, H.length);
            break;

            } /* End GSC switch */

         break;

/* Structure Information Group is currently ignored */

/* Display Information Group is currently ignored */

/* Handle Scene Data Group */

case GRP_SceneData:

         if (H.item == GSCDImage)
            goto ReadData;       /* Two-level break-out */
         else if (H.item == GSCDGroupLength)
            skipck(fd, H.length);
         else
            fatal("dumpNEMA: Unknown image data item found.");

         break;

/* Handle Structure Data Group */

case GRP_StructureData:

         fatal("dumpNEMA: Encountered structure data, "
            "this case not yet implemented.");

         break;

/* Handle Display Data Group */

case GRP_DisplayData:

         fatal("dumpNEMA: Encountered display data type, "
            "this case not yet implemented.");

         break;

/* Group is unknown or not significant to us--ignore it.
*  (This code will skip the entire group if the item is
*  the group length, otherwise it will skip just the item.
*  Therefore, a case must be inserted above to capture the
*  GroupLength item for any group that is not to be skipped.
*  Default case takes advantage of fact that GroupLength item
*  code is always zero--see note in NEMA.HDR)  */
default:
         if (H.item == GenericGroupLength) {
            readck(fd, (char *)&H.length, H.length);
            }
         skipck(fd, H.length);
         break;

         } /* End group switch */

      } /* End infinite read loop */


/*---------------------------------------------------------------------*
*                      Reached actual image data                       *
*---------------------------------------------------------------------*/

ReadData:

/* Check that conditions for reading data have been met:
*     (1) Dimension of image [29 8000] must be 3.
*     (2) Values per cell [29 8030] must be 1.
*     (3) Integers per vector [29 8060] must be 1.
*     (4) Bits per density [29 8080] must be 16.
*           (This restriction is trivial to remove if necessary.)
*     (5) Density storage scheme [29 8090] must be 0\f.
*     (6) ncol, nrow, nsec must have been specified.
*     (7) Column and row increments must been specified.
*/

   if (!(HdrFlags & GOT_Dimension))
      fatal("dumpNEMA: Image dimensionality not found.");
   else if (imagedim != REQ_Dimension)
      fatal("dumpNEMA: Image must be three-dimensional.");

   if (!(HdrFlags & GOT_ValsPerCell))
      fatal("dumpNEMA: Values per cell not specified.");
   else if (vals != REQ_ValsPerCell)
      fatal("dumpNEMA: Only one value per cell allowed.");

   if (!(HdrFlags & GOT_IntegersPerVector))
      fatal("dumpNEMA: Integers per vector not specified.");
   else if (intpvec != REQ_IntegersPerVector)
      fatal("dumpNEMA: Only one integer per cell allowed.");

   if (!(HdrFlags & GOT_BitsPerDensity))
      fatal("dumpNEMA: Bits per density not specified.");
   else if (bitspden != REQ_BitsPerDensity)
      fatal("dumpNEMA: Densities are not 16-bit values.");

   if (!(HdrFlags & GOT_LowHighBits))
      fatal("dumpNEMA: Density bit layout not specified.");
   else if (lowhigh[0] != REQ_LowBit || lowhigh[1] != REQ_HighBit)
      fatal("dumpNEMA: Density bit layout cannot be handled.");

   if (!(HdrFlags & GOT_ColsRows))
      fatal("dumpNEMA: Map section dimensions not found.");
   if (!(HdrFlags & GOT_Sections))
      fatal("dumpNEMA: Number of sections in image not found.");
   if (!(HdrFlags & GOT_ColRowIncrs))
      fatal("dumpNEMA: Column and row increments not found.");

   if (!(HdrFlags & GOT_SliceThickness))
      dzsec = DFLT_SliceThickness;

/* Return optional left-hand side matrices */

   if (nlhs > 1) {

      /* Create and return 'dim' matrix */
      plhs[IOMdim] = mxCreateFull(1, 3, REAL);
      pmd = mxGetPr(plhs[IOMdim]);
      pmd[0] = (double)ncol;
      pmd[1] = (double)nrow;
      pmd[2] = (double)nsec;

   if (nlhs > 2) {

      /* Create and return 'incr' matrix */
      plhs[IOMincr] = mxCreateFull(1, 3, REAL);
      pmd = mxGetPr(plhs[IOMincr]);
      pmd[0] = dxcol;
      pmd[1] = dyrow;
      pmd[2] = dzsec;

   if (nlhs > 3) {

      /* Create and return 'lims' matrix */
      plhs[IOMlims] = mxCreateFull(1, 2, REAL);
      pmd = mxGetPr(plhs[IOMlims]);
      pmd[0] = (HdrFlags & GOT_MinDens) ? denmin : 0.0;
      pmd[1] = (HdrFlags & GOT_MaxDens) ? denmax : mexGetNaN();

      }}} /* End optional matrix returns */

/* Return optional global matrices */

   if (HdrFlags & GOT_StudyDate) {
      mexEvalString("global NEMAdate");
      if (!(pm = mxCreateString(sdate)))
         fatal("dumpNEMA: Unable to create global NEMAdate.");
      mxSetName(pm, "NEMAdate");
      if (mexPutMatrix(pm))
         fatal("dumpNEMA: Unable to move NEMAdate to caller.");
      mxFreeMatrix(pm);
      mxFree(sdate);
      }

   if (HdrFlags & GOT_StudyTime) {
      mexEvalString("global NEMAtime");
      if (!(pm = mxCreateString(stime)))
         fatal("dumpNEMA: Unable to create global NEMAtime.");
      mxSetName(pm, "NEMAtime");
      if (mexPutMatrix(pm))
         fatal("dumpNEMA: Unable to move NEMAtime to caller.");
      mxFreeMatrix(pm);
      mxFree(stime);
      }

   if (HdrFlags & GOT_StudyID) {
      mexEvalString("global NEMAstudy");
      if (!(pm = mxCreateString(study)))
         fatal("dumpNEMA: Unable to create global NEMAstudy.");
      mxSetName(pm, "NEMAstudy");
      if (mexPutMatrix(pm))
         fatal("dumpNEMA: Unable to move NEMAstudy to caller.");
      mxFreeMatrix(pm);
      mxFree(study);
      }

   if (HdrFlags & GOT_Series) {
      mexEvalString("global NEMAseries");
      if (!(pm = mxCreateString(series)))
         fatal("dumpNEMA: Unable to create global NEMAseries.");
      mxSetName(pm, "NEMAseries");
      if (mexPutMatrix(pm))
         fatal("dumpNEMA: Unable to move NEMAseries to caller.");
      mxFreeMatrix(pm);
      mxFree(series);
      }

/* Read actual data, convert to MATLAB matrix elements, and store.
*  Apparently, the length field of the actual data record is set
*  to zero by 3dviewnix, so the length check is omitted here.
*  Therefore, we check that the next read gives an end-of-file.  */

   n = ncol * nrow * nsec;
   rowbuf = mxCalloc(ncol, sizeof(short));
   plhs[0] = mxCreateFull(nrow*ncol, nsec, REAL);
   pmd = mxGetPr(plhs[0]);

   for (isec=0; isec<nsec; isec++) {
      for (irow=0; irow<nrow; irow++) {

         /* Process one row of data */
         readck(fd, (char *)rowbuf, ncol*sizeof(short));
         for (icol=0; icol<ncol; icol++) {
            *pmd++ = (double)rowbuf[icol];
            } /* End column loop */

         } /* End row loop */
      } /* End section loop */

   /* Check that we read all the data in the file */
   if (read(fd, rowbuf, 1))
      fatal("dumpNEMA: Data ended before end-of-file.");
   if (close(fd) < 0)
      fatal("dumpNEMA: Unable to close input file.");
   mxFree(rowbuf);

   } /* End dumpNEMA() */

