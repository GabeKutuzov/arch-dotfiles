/* (c) Copyright 2018, The Rockefeller University *11113* */
/***********************************************************************
*                               tiffcam                                *
************************************************************************
*  This is a CNS user program to read tiff images as UPGM-type input   *
*  to a neural system model.  This first version reads just one image  *
*  and so should be used with FREQUENCY = SERIES.  Ability to read     *
*  a collection of images to be added later.                           *
*                                                                      *
*  The calling arguments are as specified in the CNS documention for   *
*  a UPGM-type image source (CAMERA control card).                     *
*----------------------------------------------------------------------*
*  Initialization call:                                                *
*     int tiffcamset(struct UTVDEF *putv, char *camnm,                 *
*        char *ufile, char *udata, enum tvinit_kcall kcall)            *
*  where:                                                              *
*     putv     is a pointer to a CNS-suppled UTVDEF structure with     *
*              additional parameters (e.g. the size of the desired     *
*              subimages), and where this program can store a pointer  *
*              to its persistent internal data structure. The UTVDEF   *
*              struct is defined in utvdef.h in the CNS source code    *
*              directory.                                              *
*     camnm    Name of this camera (not used by tiffcamset).           *
*     ufile    Ptr to string entered by user which is path name to     *
*              a .tiff image file.                                     *
*     udata    Not used in this first version.                         *
*     kcall    TV_INIT for the initialization call.  The tvinit_kcall  *
*              enum is defined in utvdef.h.                            *
*                                                                      *
*  Return value:                                                       *
*     0        Success.                                                *
*     1        Error TBD                                               *
*                                                                      *
*  Notes:  Max length of ufile and udata strings is 250 chars each,    *
*     enforced by code in d3grp1, not this program.                    *
*----------------------------------------------------------------------*
*  Image acquisition call (made only after tiffcamset call succeeds):  *
*     int tiffcamget(struct UTVDEF *putv, PIXEL *image)                *
*  where:                                                              *
*     putv     is a pointer to the same UTVDEF structure used by       *
*              the setup call.                                         *
*     image    Pointer to array of size tvrx x tvry where image should *
*              be stored.                                              *
*  Return value:                                                       *
*     0        Success.                                                *
*     1        Error reading data file (caller should give abexit      *
*              code 743).                                              *
*----------------------------------------------------------------------*
*  Reset call:                                                         *
*     int tiffcamset(struct UTVDEF *putv, char *camnm,                 *
*        char *ufile, char *udata, enum tvinit_kcall kcall)            *
*  where:                                                              *
*     All values are as described above for the initial tiffcamset()   *
*     call, except kcall is TV_RESET.  (This call will not be made     *
*     from CNS if putv->tvuflgs & TV_BYPR is set).  This call just     *
*     "rewinds" the image file to the beginning so the image can be    *
*     read again.                                                      *
*----------------------------------------------------------------------*
*  Restart call:                                                       *
*     int tiffcamset(struct UTVDEF *putv, char *camnm,                 *
*        char *ufile, char *udata, enum tvinit_kcall kcall)            *
*  where:                                                              *
*     All values are as described above for the initial tiffcamset()   *
*     call, except kcall is TV_RESTART.  This is basically the same    *
*     as the original tiffcamset() call--all settings and storage are  *
*     released and the program reads new ufile and udata information.  *
*     The possible return codes are also as listed above for the       *
*     initial tiffcamset() call.                                       *
*----------------------------------------------------------------------*
*  Closeout call:                                                      *
*     int tiffcamset(struct UTVDEF *putv, char *cam,                   *
*        char *ufile, char *udata, enum tvinit_kcall kcall)            *
*  where:                                                              *
*     putv, cam, and return values are same as above.  ufile and       *
*     udata are NULL.  kcall is TV_CLOSE.                              *
*     All files are closed and all memory is released.                 *
*----------------------------------------------------------------------*
*  Note:  This program will be loaded dynamically into CNS at run      *
*  time.  There will be no CRK environment--Errors are just reported   *
*  via nonzero return codes with messages in errtab file.              *
************************************************************************
*  V1A, 07/06/17, GNR - New program                                    *
*  Rev, 09/12/17, GNR - Return ign = isn = 1.  Later fix for > 1 image *
*  V1B, 04/10/18, GNR - Add code to center image with TV_RCTR flag     *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <tiffio.h>
#include "sysdef.h"
#include "rkarith.h"
#include "utvdef.h"


/*=====================================================================*
*                             tiffcamcls                               *
*                                                                      *
*  Close any open tiff file and clean up memory.                       *
*=====================================================================*/

static void tiffcamcls(struct UTVDEF *putv) {

   if (putv && putv->ptvsrc) {
      TIFFClose((TIFF *)putv->ptvsrc);
      putv->ptvsrc = NULL;
      }
   return;
   }  /* End tiffcamcls() */


/*=====================================================================*
*                             tiffcamget                               *
*  Note:  This code is written assuming A channel, if present, comes   *
*  before color channels and color channels are in order RGB.  This    *
*  will have to be verified by testing, not clear in documentation     *
*  (and may be variable and explicated in additional tags).            *
*=====================================================================*/

int tiffcamget(struct UTVDEF *putv, PIXEL *image) {

   byte *piminu;              /* Ptr to used part of input row */
   byte *pimine;              /* Ptr to end of used part of input row */
   size_t ipxsz;              /* Size of input pixels */
   size_t alen;               /* Length of A channel data */
   size_t xlen;               /* Length of used part of one pixel */
   size_t rlen;               /* Length of used part of input row */
   size_t slen;               /* Length of skipped part of input row */
   size_t olen;               /* Length of one output row */
   ui32 irow  = (ui32)putv->tvyo;
   ui32 irowe = irow + (ui32)putv->tvry;
   alen = (putv->tvkicol & TVC_ACHAN) ?
      (size_t)1 << (putv->tvkicol & TVC_B8) : 0;
   xlen = (size_t)putv->tvipxsz;
   ipxsz = xlen + alen;
   olen = xlen * (size_t)putv->tvrx;
   rlen = ipxsz * (size_t)putv->tvrx;
   slen = ipxsz * (size_t)putv->tvxo;
   piminu = putv->p1row + slen;
   pimine = piminu + rlen;
   for ( ; irow<irowe; ++irow) {
      if (TIFFReadScanline((TIFF *)putv->ptvsrc, putv->p1row,
         irow, 0) < 0) { tiffcamcls(putv); return 1; }
      if (putv->tvkicol & TVC_ACHAN) {
         /* Eliminate A channel information */
         byte *pim = piminu;
         while (pim < pimine) {
            pim += alen;
            memcpy(image, pim, xlen);
            pim += xlen, image += xlen;
            }
         }
      else {
         /* Just copy the info--using shorter of actual image row
         *  length or tvrx requested by user in d3g1imin.  */
         memcpy(image, piminu, olen);
         image += olen; 
         }
      }  /* End row loop */

   putv->ign = putv->isn = 1;
   return 0;
   }  /* End tiffcamget() */


/*=====================================================================*
*                             tiffcamset                               *
*  Note:  Behavior of tiffcamset/tiffcamget pair was modified in V1B.  *
*  Option TV_RCTR was added, which resets tvr[xy]o to return a piece   *
*  of size tvr[xy]e out of the center of the image if the image is     *
*  larger than the request, otherwise returns the whole image in that  *
*  direction (assuming code in caller will add a blank border).  If    *
*  TV_RCTR is not coded, old behavior was to honor the requested off-  *
*  set and make the image smaller if necessary.  New behavior is to    *
*  honor tvr[xy] if possible, using smaller offset if necessary and,   *
*  as with TV_RCTR, return all the image there is if the image is      *
*  smaller than the request.                                           *
*=====================================================================*/

int tiffcamset(struct UTVDEF *putv, char *camnm, const char *ufile,
   const char *udata, enum tvinit_kcall kcall) {

   TIFF *tiffid;              /* TIFF file handle */
   ui32 ttag;                 /* Temp for tags */
   ui16 htag;                 /* Tag reduced to ui16 */
   int  rc = 0;               /* Return code */

/* Switch according to type of call */

   switch (kcall) {

/* Close-out call */

   case TV_CLOSE:
      tiffcamcls(putv);
      break;

/* Reset call.  We want the library to look like the file was just
*  opened so we can reread the same image.  It looks like the
*  function TIFFdOpen should do this, all we can do is try it.
*  Tried it--it returned 0, i.e. did not make a new TIFF*.  So now
*  I am going to try the brute-force method, close, the open.  */

   case TV_RESET:
      if (!putv->ptvsrc) {
         rc = 744; goto SetupErrorExit; }
      if (!ufile) {
         rc = 742; goto SetupErrorExit; }
      TIFFClose((TIFF *)putv->ptvsrc);
      putv->ptvsrc = NULL;       /* JIC */
      putv->ptvsrc = TIFFOpen(ufile, "r");
      if (!putv->ptvsrc) {
         rc = 740; goto SetupErrorExit; }
      break;

/* Restart call.  Per documentation above, should clean up everything
*  and then do a TV_INIT.  */

   case TV_RESTART:
      tiffcamcls(putv);
      /* Drop into case TV_INIT, no break ... */

/* Initialization call */

   case TV_INIT:
      /* Sanity check */
      if (putv->ptvsrc) {
         rc = 741; goto SetupErrorExit; }
      if (!ufile) {
         rc = 742; goto SetupErrorExit; }
      putv->ptvsrc = tiffid = TIFFOpen(ufile, "r");
      if (!tiffid) {
         rc = 740; goto SetupErrorExit; }
      /* File "tag" values that we are allowed to pass back to CNS */
      TIFFGetField(tiffid, TIFFTAG_IMAGEWIDTH, &ttag);
      if (ttag > UI16_MAX) {
         rc = 745; goto SetupErrorExit; };
      htag = (ui16)ttag;
      if (htag <= putv->tvrxe) {
         /* Image smaller than rqst, return all of it */
         putv->tvrx = htag, putv->tvrxo = 0; }
      else {
         putv->tvrx = putv->tvrxe;     /* No, bigger */
         if (putv->tvuflgs & TV_RCTR)  /* Take piece from center */ 
            putv->tvrxo = (htag - putv->tvrxe) >> 1;
         }
      TIFFGetField(tiffid, TIFFTAG_IMAGELENGTH, &ttag);
      if (ttag > UI16_MAX) {
         rc = 745; goto SetupErrorExit; };
      htag = (ui16)ttag;
      if (htag <= putv->tvrye) {
         /* Image smaller than rqst, return all of it */
         putv->tvry = htag, putv->tvryo = 0; }
      else {
         putv->tvry = putv->tvrye;     /* No, bigger */
         if (putv->tvuflgs & TV_RCTR)  /* Take piece from center */ 
            putv->tvryo = (htag - putv->tvrye) >> 1;
         }
      TIFFGetField(tiffid, TIFFTAG_BITSPERSAMPLE, &ttag);
      if (ttag == 8)       putv->tvkicol = TVC_B1;
      else if (ttag == 16) putv->tvkicol = TVC_B2;
      else if (ttag == 32) putv->tvkicol = TVC_B4;
      else {
         rc = 746; goto SetupErrorExit; }
      TIFFGetField(tiffid, TIFFTAG_SAMPLESPERPIXEL, &ttag);
      if (ttag > 3)        putv->tvkicol |= (TVC_Col|TVC_ACHAN);
      else if (ttag == 3)  putv->tvkicol |= TVC_Col;
      else if (ttag != 1) {
         rc = 747; goto SetupErrorExit; }
      /* Now allocate a one-row buffer.  It would be nice to read
      *  the image directly into the image arg to tiffcamget, but
      *  this way allows us to remove A channel data and crop per
      *  tvxo,tvyo.  */
      putv->tvrlen = TIFFScanlineSize(tiffid);
      break;

      } /* End kcall switch */

/* Everything seems to be OK, return 0 */
   return 0;

/* Here on all errors during setup */

SetupErrorExit:
   tiffcamcls(putv);
   return rc;

   } /* End tiffcamset() */
