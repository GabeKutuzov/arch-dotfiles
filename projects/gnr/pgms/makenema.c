/***********************************************************************
*                          makeNEMA program                            *
*                                                                      *
*                      Write acrNEMA file header                       *
*                                                                      *
*  N.B.  This routine writes only those fields which have been found   *
*  to be written by the UPenn 3Dviewnix package.  All fields marked    *
*  with an asterisk in the standards document are written even if      *
*  only NULL values are available, again following UPenn practice.     *
*  Values of all fields that can be specified by the user are taken    *
*  from calling arguments, so this routine is application-independent. *
*  Two-byte image densities are always assumed, one value per cell.    *
*  Orientation of x along rows, y along columns, and z perpendicular   *
*  to sections is assumed.  This routine could be made fully general   *
*  with a little more work.                                            *
*                                                                      *
*  Because each header group requires a length record at the start,    *
*  this program caches all header records in RAM before writing them.  *
*  Routine is not concerned with bute swapping--will write standard    *
*  NEMA data if run on a big-endian machine (e.g. SPARC), otherwise    *
*  will require modification.                                          *
*                                                                      *
*  Per specification, all new records are aligned on two-byte          *
*  boundaries.  This means the HeaderItem.length field has to be       *
*  moved in with memcpy, because it may not be on a word boundary.     *
*                                                                      *
*  Prototype in:  NEMA.h                                               *
*  Arguments:                                                          *
*     vers        0008,0010   Version of calling program               *
*     inst        0008,0080   Institution                              *
*     dept        0008,1040   Department                               *
*     note        0009,8030   General comment                          *
*     study       0020,0010   Study number                             *
*     series      0020,0011   Series number                            *
*     idim        0029,8000   Scene dimensions                         *
*     loden       0029,8040   Lowest density in image                  *
*     hiden       0029,8050   Highest density in image                 *
*     ncol        0029,8095   Number of columns in image               *
*     nrow        0029,8095   Number of rows in image                  *
*     nsec        0029,80a0   Number of sections in image              *
*     xpix        0029,80a5   Size of pixels along x (mm)              *
*     ypix        0029,80a5   Size of pixels along y (mm)              *
*     zpix        0018,0050   Slice thickness                          *
*                                                                      *
*  Returns:       0 if successful, otherwise nonzero error signal      *
*                                                                      *
*  V1A, 01/25/95, GNR - Initial version                                *
***********************************************************************/

#include <sys/stdtypes.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "NEMA.h"
#include "NEMA.HDR"

#define NORMAL            0      /* Value returned if all is well */
#define ERROR            -1      /* Value returned on error */

/* The following parameters are "built-in" to this version
*  and cannot be changed by the user.  If converting any of
*  these into calling arguments, be sure to consider the strange
*  interactions of some of the various format-definition fields.  */
#define DfltValsPerCell    1     /* Values per cell */
#define DfltIntsPerVect    1     /* Integers per vector */
#define DfltBitsPerDens   16     /* Bits per density */
#define DfltLowSlice     0.0     /* Z coordinate of low slice */

/*=====================================================================*
*   Auxiliary routines used to write header fields of various types    *
*=====================================================================*/

/* Global data for controlling data packing */
#define InitialCacheSize 1024    /* Size of first cache allocation--
                                 *  must be at least 4 or big trouble */
#define ANrecl             12    /* Length of an AN record */
/* Code depends on fact that CacheAllo is initialized to zero
*  by rules of the C language standard.  */
static struct cdata {
   size_t CacheAllo;             /* Cache storage allocated */
   char *pCache;                 /* Pointer to cache */
   char *pNextCache;             /* Pointer to next cache location */
   HeaderItem *pRecHead;         /* Ptr to head of current record */
   int CurrentGrp;               /* Current record group */
   int Gfd;                      /* Global file definition */
   } CD;

/*---------------------------------------------------------------------*
*                            BeginGroup()                              *
*                                                                      *
*  This routine begins buffering commands for the NEMA group specified *
*  in the argument.  Space is reserved for the GenericGroupLength      *
*  record that will be inserted at the start of group by EndGroup.     *
*---------------------------------------------------------------------*/

void BeginGroup(int GrpNum) {
   
   if (!CD.CacheAllo) {
      CD.CacheAllo = InitialCacheSize;
      CD.pCache = mallocv((size_t)InitialCacheSize, "Command cache");
      }
   CD.pNextCache = CD.pCache + sizeof(HeaderItem) + sizeof(long);
   CD.CurrentGrp = GrpNum;

   } /* End BeginGroup */


/*---------------------------------------------------------------------*
*                              EndGroup                                *
*                                                                      *
*  This routine ends the group initiated by the most recent call to    *
*  BeginGroup.  The total length is calculated and inserted into the   *
*  generic group length record and the buffered commands are written.  *
*---------------------------------------------------------------------*/

int EndGroup(void) {
   
   long recl = CD.pNextCache - CD.pCache;
   long datl = recl - sizeof(HeaderItem) - sizeof(long);
   HeaderItem *pHead = (HeaderItem *)CD.pCache;
   char *pLength = CD.pCache + sizeof(HeaderItem);

   pHead->group = CD.CurrentGrp;
   pHead->item = GenericGroupLength;
   pHead->length = sizeof(long);
   memcpy(pLength, (char *)&datl, sizeof(long));

   if (write(CD.Gfd, CD.pCache, recl) < 0) return ERROR;
   return NORMAL;

   } /* End EndGroup */


/*---------------------------------------------------------------------*
*                              TestSpace                               *
*                                                                      *
*  This routine assures that the current command cache will hold the   *
*  number of additional bytes given by the argement.                   *
*---------------------------------------------------------------------*/

void TestSpace(size_t size) {
   
   size_t used = CD.pNextCache - CD.pCache;
   size_t rlen = CD.CacheAllo - used;

   if (size > rlen) {
      size_t allo = CD.CacheAllo << 1;
      CD.pCache = realloc(CD.pCache, allo);
      if (!CD.pCache) fatal("Error allocating header group.");
      CD.CacheAllo = allo;
      CD.pNextCache = CD.pCache + used;
      }

   } /* End TestSpace */


/*---------------------------------------------------------------------*
*                              ANRecord                                *
*                                                                      *
*  Buffer a double-precision numeric data record.  The first argument  *
*  gives the record type, the second gives the data item.  A little    *
*  pointer to the location of the header is saved for Append call.     *
*---------------------------------------------------------------------*/

void ANRecord(int RecType, double RecData) {
   
   HeaderItem *pHead;
   long datl = ANrecl;

   TestSpace(sizeof(HeaderItem) + ANrecl);
   pHead = CD.pRecHead = (HeaderItem *)CD.pNextCache;
   pHead->group = CD.CurrentGrp;
   pHead->item = RecType;
   memcpy((char *)&pHead->length, (char *)&datl, sizeof(long));
   CD.pNextCache += sizeof(HeaderItem);
   /* Use sprintf instead of bcdout to get 'e' in exponentials */
   sprintf(CD.pNextCache, "%*.6e", ANrecl, RecData);
   CD.pNextCache += ANrecl;

   } /* End ANRecord */


/*---------------------------------------------------------------------*
*                           AppendANRecord                             *
*                                                                      *
*  Append the double-precision numeric data item given by the argument *
*  to the current alphanumeric data record.  3Dviewnix seems to allow  *
*  a padding blank only at the end of the entire multiple data value.  *
*---------------------------------------------------------------------*/

void AppendANRecord(double RecData) {

   long llen;                    /* Length in label record */
   size_t apsize = ANrecl + 1;   /* One for the '\' divider */
   int kpad;                     /* TRUE if must pad data string */

   memcpy((char *)&llen, (char *)&CD.pRecHead->length, sizeof(long));
   /* If last character in buffer is a blank, it was a padding
   *  character left there by previous ANRecord or AppendANRecord.
   *  We back up over it so it will be overwritten with new data.
   */
   if (*(CD.pNextCache-1) == ' ') {
       --CD.pNextCache; --llen; }
   kpad = (llen + apsize) & 1;
   TestSpace(apsize+kpad);
   llen += (apsize+kpad);
   memcpy((char *)&CD.pRecHead->length, (char *)&llen, sizeof(long));
   sprintf(CD.pNextCache, "\\%*.6e", ANrecl, (double)RecData);
   CD.pNextCache += apsize;
   if (kpad) *CD.pNextCache++ = ' ';

   } /* End AppendANRecord */


/*---------------------------------------------------------------------*
*                              ATRecord                                *
*                                                                      *
*  Buffer an ASCII text record.  The first argument gives the record   *
*  type, the second is a pointer to the data string.  A NULL pointer   *
*  is allowed and indicates that the data string is empty.             *
*---------------------------------------------------------------------*/

void ATRecord(int RecType, char *RecText) {

   HeaderItem *pHead;
   long datl,itml;
   int kodd;                     /* TRUE if length was odd */

   /* Get data length.  Item length is one more if datl is odd */
   datl = (RecText ? strlen(RecText) : 0);
   itml = datl + (kodd = datl & 1);
   TestSpace(sizeof(HeaderItem) + (size_t)itml);
   pHead = CD.pRecHead = (HeaderItem *)CD.pNextCache;
   pHead->group = CD.CurrentGrp;
   pHead->item = RecType;
   memcpy((char *)&pHead->length, (char *)&itml, sizeof(long));
   CD.pNextCache += sizeof(HeaderItem);
   if (RecText) {
      memcpy(CD.pNextCache, RecText, datl);
      if (kodd) CD.pNextCache[datl] = ' ';
      CD.pNextCache += (size_t)itml;
      }

   } /* End ATRecord */


/*---------------------------------------------------------------------*
*                              BDRecord                                *
*                                                                      *
*  Buffer a 32-bit binary data record.  The first argument gives the   *
*  record type, the second gives the actual data.                      *
*---------------------------------------------------------------------*/

void BDRecord(int RecType, int RecData) {

   HeaderItem *pHead;
   long datl = sizeof(long);
   long item = (long)RecData;

   TestSpace(sizeof(HeaderItem) + sizeof(long));
   pHead = CD.pRecHead = (HeaderItem *)CD.pNextCache;
   pHead->group = CD.CurrentGrp;
   pHead->item = RecType;
   memcpy((char *)&pHead->length, (char *)&datl, sizeof(long));
   CD.pNextCache += sizeof(HeaderItem);
   memcpy(CD.pNextCache, (char *)&item, sizeof(long));
   CD.pNextCache += sizeof(long);

   } /* End BDRecord */


/*---------------------------------------------------------------------*
*                              BIRecord                                *
*                                                                      *
*  Buffer a 16-bit binary data record.  The first argument gives the   *
*  record type, the second gives the actual data.                      *
*---------------------------------------------------------------------*/

void BIRecord(int RecType, int RecData) {

   HeaderItem *pHead;
   long datl = sizeof(short);
   short item = (short)RecData;

   TestSpace(sizeof(HeaderItem) + sizeof(short));
   pHead = CD.pRecHead = (HeaderItem *)CD.pNextCache;
   pHead->group = CD.CurrentGrp;
   pHead->item = RecType;
   memcpy((char *)&pHead->length, (char *)&datl, sizeof(long));
   CD.pNextCache += sizeof(HeaderItem);
   memcpy(CD.pNextCache, (char *)&item, sizeof(short));
   CD.pNextCache += sizeof(short);

   } /* End BIRecord */


/*---------------------------------------------------------------------*
*                              MBIRecord                               *
*                                                                      *
*  Buffer a multiple-value 16-bit binary integer data record.  The     *
*  first argument gives the record type, the second is the number of   *
*  data items, and the subsequent arguments are the actual data.       *
*---------------------------------------------------------------------*/

void MBIRecord(int RecType, int N, ...) {

   va_list pitem;
   HeaderItem *pHead;
   long datl = N*sizeof(short);
   int i;
   short item;

   va_start(pitem, N);
   TestSpace(sizeof(HeaderItem) + N*sizeof(short));
   pHead = CD.pRecHead = (HeaderItem *)CD.pNextCache;
   pHead->group = CD.CurrentGrp;
   pHead->item = RecType;
   memcpy((char *)&pHead->length, (char *)&datl, sizeof(long));
   CD.pNextCache += sizeof(HeaderItem);
   for (i=0; i<N; i++) {
      item = va_arg(pitem, short);
      memcpy(CD.pNextCache, (char *)&item, sizeof(short));
      CD.pNextCache += sizeof(short);
      }
   va_end(pitem);

   } /* End MBIRecord */


/*=====================================================================*
*                  makeNEMA -- write acrNEMA header                    *
*                                                                      *
*  N.B.  When writing NULL values to AN numeric type fields, routine   *
*  ATRecord is used.  The correct conversion type routine should be    *
*  substituted if numeric values are later placed in these fields.     *
*=====================================================================*/

int makeNEMA(int nfd, char *vers, char *inst, char *dept,
      char *note, long study, long series, long idim,
      long loden, long hiden, long ncol, long nrow, long nsec,
      double xpix, double ypix, double zpix) {

   double z;                     /* Slice z coordinate */         
   int i;                        /* Slice counter */
   char stamp[12];               /* Time stamp */
   char sdate[9];                /* Date string */
   char stime[9];                /* Time string */

/* Pick up current date and time and generate label strings */

   tstamp(stamp);
   sconvrt(sdate,"(#3,A2H.A2H.A2)",stamp,NULL);
   sconvrt(stime,"(#3,A2H:A2H:A2)",stamp+6,NULL);
         
/* Write command information group */

   CD.Gfd = nfd;                 /* Make file descriptor available */
   BeginGroup(GRP_CommandInfo);
   BDRecord(GCI_MessageLength, 0);
   if (EndGroup()) return ERROR;

/* Write identification information group */

   BeginGroup(GRP_IdentInfo);
   BDRecord(GII_MessageLength, 0);
#if 0 /***DEBUG***/
   ATRecord(GII_VersionNumber, vers);
#else
   ATRecord(GII_VersionNumber, "VIEWNIX1.0");
#endif
   ATRecord(GII_StudyDate, sdate);
   ATRecord(GII_StudyTime, stime);
   BIRecord(GII_DataSetType, IMAGE0);
   ATRecord(GII_Modality, NULL);
   ATRecord(GII_Institution, inst);
   ATRecord(GII_Physician, NULL);
   ATRecord(GII_Department, dept);
   ATRecord(GII_Radiologist, NULL);
   ATRecord(GII_MfgModel, NULL);
   if (EndGroup()) return ERROR;

/* Write general information group */

   BeginGroup(GRP_GeneralInfo);
   ATRecord(GGI_MessageFile, NULL);
   ATRecord(GGI_OrigSceneFile, NULL);
   ATRecord(GGI_Description, NULL);
   ATRecord(GGI_ScratchPad, note);
   if (EndGroup()) return ERROR;

/* Write patient information group */

   BeginGroup(GRP_PatientInfo);
   ATRecord(GPI_PatientName, NULL);
   ATRecord(GPI_PatientID, NULL);
   if (EndGroup()) return ERROR;

/* Write acquisition information group */

   BeginGroup(GRP_AcqInfo);
   ANRecord(GAI_SliceThickness, zpix);
   ATRecord(GAI_KVP, NULL);               /* Really AN type */
   ATRecord(GAI_RepetitionTime, NULL);    /* Really AN type */
   ATRecord(GAI_EchoTime, NULL);          /* Really AN type */
   ATRecord(GAI_ImagedNucleus, NULL);
   ATRecord(GAI_GantryTilt, NULL);        /* Really AN type */
   if (EndGroup()) return ERROR;

/* Write relationship information group */

   BeginGroup(GRP_RelInfo);
   ANRecord(GRI_StudyID, (double)study);
   ANRecord(GRI_Series, (double)series);
   if (EndGroup()) return ERROR;

/* Write image presentation information group */

   BeginGroup(GRP_ImageInfo);
   ATRecord(GPR_LUTDescriptors, NULL);    /* Really BI type */
   ATRecord(GPR_LUTDRed, NULL);           /* Really BI type */
   ATRecord(GPR_LUTDGreen, NULL);         /* Really BI type */
   ATRecord(GPR_LUTDBlue, NULL);          /* Really BI type */
   ATRecord(GPR_LUTData, NULL);           /* Really BI type */
   ATRecord(GPR_LUTDataRed, NULL);        /* Really BI type */
   ATRecord(GPR_LUTDataGreen, NULL);      /* Really BI type */
   ATRecord(GPR_LUTDataBlue, NULL);       /* Really BI type */
   if (EndGroup()) return ERROR;

/* Write scene-related information group.
*  N.B.  This initial Q&D code always assumes dimension is 3
*  when writing out sampling scheme and some other fields.  */

   BeginGroup(GRP_SceneInfo);
   BIRecord(GSC_Dimension, idim);
   ATRecord(GSC_DomainLocation, NULL);    /* Really AN type */
   ATRecord(GSC_AxisLabels, NULL);
   ATRecord(GSC_AxisUnits, NULL);         /* Really BI type */
   BIRecord(GSC_ValsPerCell, DfltValsPerCell);
   ATRecord(GSC_DensityUnits, NULL);      /* Really BI type */
   ANRecord(GSC_MinDens, (double)loden);
   ANRecord(GSC_MaxDens, (double)hiden);
   BIRecord(GSC_IntegersPerVector, DfltIntsPerVect);
   ATRecord(GSC_Signedness, NULL);        /* Really BI type */
   BIRecord(GSC_BitsPerDensity, DfltBitsPerDens);
   MBIRecord(GSC_LowHighBits, 2, 0, DfltBitsPerDens-1);
   ATRecord(GSC_AlignmentDim, NULL);      /* Really BI type */
   ATRecord(GSC_AlignmentBytes, NULL);    /* Really BI type */
   MBIRecord(GSC_ColsRows, 2, ncol, nrow);
   BIRecord(GSC_Sections, nsec);
   ANRecord(GSC_ColRowIncrs, xpix);
   AppendANRecord(ypix);
   z = DfltLowSlice;
   ANRecord(GSC_SliceLocations, z);
   for (i=1; i<nsec; i++)
      AppendANRecord(z += zpix);
   ATRecord(GSC_ProcessingOps, NULL);
   if (EndGroup()) return ERROR;

/* Write structure information group */

   BeginGroup(GRP_StructureInfo);
   if (EndGroup()) return ERROR;

/* Write display information group */

   BeginGroup(GRP_DisplayInfo);
   if (EndGroup()) return ERROR;
              
/* Return with file positioned for writing image data */

   if (CD.CacheAllo) free(CD.pCache);
   return RK.iexit;

   } /* End makeNEMA */

