/* (c) Copyright 1992-2015, The Rockefeller University *11115* */
/* $Id: setmf.c 5 2008-03-13 20:32:23Z  $ */
/***********************************************************************
*                     ROCKS metafile plot library                      *
*                               setmf()                                *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void setmf(const char *fname, const char *station,               *
*        const char *title, const char *icon, long buflen,             *
*        ui32 dbgmask, int enc, int lci, int lcf)                      *
*                                                                      *
*  DESCRIPTION:                                                        *
*     Initializes the metafile/X window plot package.                  *
*                                                                      *
*  ARGUMENTS:                                                          *
*     fname   Name of the metafile to be written.  If 'fname' is a     *
*             NULL pointer, no file is written,  If 'fname' is a       *
*             pointer to a null string, an error is generated--this    *
*             was an option with the ASCII metafile to write to        *
*             stdout, but this cannot be done with the binary metafile.*
*     station Name of a graphics station where X window output should  *
*             be displayed, e.g. "acteon:0.0".  A null pointer indi-   *
*             cates that no X output should be generated.  A pointer   *
*             to a null string indicates that the login host should    *
*             be used.                                                 *
*     title   Text string to be displayed in the window title bar for  *
*             the graphics output.  It should generally be the name of *
*             the application program.  A null pointer or a pointer to *
*             a null string causes the title to be left blank.         *
*     icon    Path name of a file containing an icon image to be used  *
*             when the X graphics window is minimized.  A null pointer *
*             or a pointer to a null string indicates that a generic   *
*             icon should be used.                                     *
*     buflen  Length of the internal buffer to be allocated for meta-  *
*             file data.  This should normally be at least equal to    *
*             the hardware blocksize on the device where the metafile  *
*             is written.  If 'buflen' is less than the default value  *
*             (currently 4096 bytes), the default is used.             *
*     dbgmask Undefined data which may be used during debugging.  It   *
*             is OR'd bitwise with any debugging mask passed from the  *
*             driver program in a parallel computer and transmitted to *
*             mfdraw.                                                  *
*     enc     Indicates the metafile encoding to be used.              *
*             'B' indicates the binary encoding used with the V2       *
*             library.  Other values are obsolete or reserved for      *
*             future use and generate an error with the current call.  *                                              *
*     lci     Length of the integer part of coordinate values in bits. *
*             '0' indicates the default should be used, currently 6.   *
*             Values less than 6 will be set to 6.                     *
*     lcf     Length of coordinate fractions in bits.  'lcf' may range *
*             from 1 to 24.  If a value outside this range is given,   *
*             coordinates will have 10 fraction bits.  The total of    *
*             'lci' + 'lcf' may not exceed 28.                         *
*                                                                      *
*  RETURN VALUES:                                                      *
*     None                                                             *
*                                                                      *
*  PARALLEL COMPUTATION:                                               *
*     On parallel computers, setmf() only needs to be called from      *
*     node 0.  If called on other nodes, no error occurs, but the      *
*     call is ignored.                                                 *
*                                                                      *
*  NOTES:                                                              *
*     The former routines setmeta() and setmfd() have been combined    *
*     in order to eliminate confusion due to having two different      *
*     buflen arguments.  setmf() may be called multiple times before   *
*     newplt(), and it cleans up after itself, but it is an error to   *
*     call it once newplt() has been called.                           *
*                                                                      *
************************************************************************
*  V2A, 01/18/99, GNR - Made from combination of setmeta and setmfd    *
*  Rev, 08/08/08, GNR - Reorder args, make dbgmask ui32, kill lci      *
*  Rev, 02/07/15, GNR - Minor changes for V2A library                  *
***********************************************************************/

#define I_AM_SETMF      /* Instantiate _RKG */

#include <ctype.h>
#include "mfint.h"
#include "mfio.h"
#include "rksubs.h"
#include "plots.h"

void setmf(const char *fname, const char *station, const char *title,
   const char *icon, long buflen, ui32 dbgmask, int enc, int lci,
   int lcf) {

#ifndef PARn
   /* Error if already started plotting */
   if (_RKG.MFFlags & MFF_DidIfin) abexit(96);

   /* Initialize common area if not already done */
   if (!(_RKG.MFFlags & MFF_DidSetup)) rkginit();

   /* Kill any previous setmf() setup */
   _RKG.s.MFMode = 0;

   /* Handle 'fname' argument */
   if (_RKG.MFfn) {
      freev(_RKG.MFfn, "Metafile name");
      _RKG.MFfn = NULL; }
   if (fname) {
      _RKG.s.MFMode |= METF;
      if (fname[0] == '\0')
         abexitm(234, "Empty file name to setmf");
      else {
         _RKG.MFfn = (char *)mallocv(strlen(fname)+1,
            "Metafile name");
         strcpy(_RKG.MFfn, fname);
         }
      }

   /* Handle 'station' argument */
   if (_RKG.MFDdisplay) {
      freev(_RKG.MFDdisplay, "X Display");
      _RKG.MFDdisplay = NULL; }
   if (station) {
      _RKG.s.MFMode |= SGX;
      if (station[0]) {
         _RKG.MFDdisplay = (char *)mallocv(strlen(station)+1,
            "X Display");
         strcpy(_RKG.MFDdisplay, station);
         }
      }

   /* Handle 'title' argument */
   if (_RKG.MFDwtitl) {
      freev(_RKG.MFDwtitl, "Window title");
      _RKG.MFDwtitl = NULL; }
   if (title && title[0]) {
      _RKG.MFDwtitl = (char *)mallocv(strlen(title)+1,
         "Window title");
      strcpy(_RKG.MFDwtitl, title);
      }

   /* Handle 'icon' argument */
   if (_RKG.MFDwicon) {
      freev(_RKG.MFDwicon, "Icon name");
      _RKG.MFDwicon = NULL; }
   if (icon && icon[0]) {
      _RKG.MFDwicon = (char *)mallocv(strlen(icon)+1,
         "Icon name");
      strcpy(_RKG.MFDwicon, icon);
      }

   /* Handle 'buflen' argument.  Note:  MFBuffLen should be
   *  reasonably long to minimize I/O, so if 'buflen' is less
   *  than the default, the default is used.  This could be
   *  temporarily set smaller if needed during testing.  */
   _RKG.s.MFBuffLen = (buflen > DFLTBUFLEN) ? buflen : DFLTBUFLEN;

   /* Handle 'dbgmask' argument */
   _RKG.s.dbgmask |= dbgmask;

   /* Handle 'enc' argument */
   if (enc != 'B') mfckerr(ERR_MFMT);

   /* Handle 'lci' argument */
   if (lci < DFLTLCI) lci = DFLTLCI;
   else if (lci > MAXCBITS - DFLTLCF) lci = MAXCBITS - DFLTLCF;
   _RKG.s.lci = (short)lci;

   /* Handle 'lcf' argument */
   if (lcf <= 0) lcf = DFLTLCF;
   else if (lcf > MAXCBITS - lci) lcf = MAXCBITS - lci;
   _RKG.s.lcf = (short)lcf;

#endif /* !PARn */

   } /* End setmf() */

