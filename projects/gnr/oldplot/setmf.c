/* (c) Copyright 1999-2014, The Rockefeller University *11114* */
/* $Id: setmf.c 30 2017-01-16 19:30:14Z  $ */
/***********************************************************************
*                               setmf()                                *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void setmf(const char *fname, const char *station,               *
*        const char *title, const char *icon, long buflen,             *
*     ui32 dbgmask, int enc, int lci, int lcf)                         *
*                                                                      *
*  DESCRIPTION:                                                        *
*     Initializes the metafile/X window plot package.                  *
*                                                                      *
*  ARGUMENTS:                                                          *
*     fname   Name of the metafile to be written.  If 'fname' is a     *
*             NULL pointer, no file is written,  If 'fname' is a       *
*             pointer to a null string, output is written to stdout.   *
*     station Name of a graphics station where X window output should  *
*             be displayed, e.g. "acteon:0.0".  A null pointer indi-   *
*             cates that no X output should be generated.  A pointer   *
*             to a null string indicates that the login host should    *
*             be used.                                                 *
*     title   Text string to be displayed in the window title bar for  *
*             the graphics output.  It should generally be the name of *
*             the application program.  A null pointer or a pointer to *
*             a null string causes the title to be left blank.         *
*     icon    Name of a file containing an icon image to be used when  *
*             the X graphics window is minimized.  A null pointer or a *
*             pointer to a null string indicates that a generic icon   *
*             should be used.  (Ignored if 'enc' = '8'.)               *
*     buflen  Length of the internal buffer to be allocated for        *
*             metafile data.  This should normally be at least equal   *
*             to the hardware blocksize on the device where the meta-  *
*             file is written.  If 'buflen' is <= 0, a default value   *
*             (currently 4096 bytes) is used.                          *
*     dbgmask Undefined data which may be used during debugging.  It   *
*             is OR'd bitwise with any debugging mask passed from the  *
*             driver program in a parallel computer and transmitted to *
*             mfdraw.                                                  *
*     enc     Indicates the metafile encoding to be used.              *
*                 'A'   Use ASCII encoding.                            *
*                 'B'   Use Binary encoding.                           *
*                 '8'   Make output compatible with mfdraw version 8   *
*                       (R. Zarom SunOS and S. Marx Linux versions).   *
*                       Encoding is ASCII, lcf is 10, startup differs. *
*             Any other value gives an error.                          *
*     lci     A currently unused parameter, retained for compatibility *
*             with earlier versions of the specification.  It should   *
*             be set to 0 to avoid conflicts with any future uses.     *
*     lcf     Length of coordinate fractions in bits.  With the        *
*             decimal encoding, enough fraction digits will be         *
*             provided externally to encode this number of bits.       *
*             'lcf' may range from 1 to 24.  If a value outside        *
*             this range is given, coordinates will have 10 bits.      *
*                                                                      *
*  RETURN VALUES:                                                      *
*     None                                                             *
*                                                                      *
*  NOTES:                                                              *
*     The former routines setmeta() and setmfd() have been combined    *
*     in order to eliminate confusion due to having two different      *
*     buflen arguments.  setmf() may be called multiple times before   *
*     newplt(), and it cleans up after itself, but it is an error to   *
*     call it once newplt() has been called.                           *
************************************************************************
*  V2A, 01/18/99, GNR - Made from combination of setmeta and setmfd    *
*  Rev, 08/08/08, GNR - Reorder args, make dbgmask ui32, kill lci      *
*  ==>, 10/13/11, GNR - Last mod before committing to svn repository   *
*  Rev, 08/29/14, GNR - Add 'const' to pointer args                    *
***********************************************************************/

#define I_AM_SETMF      /* Instantiate _RKG */

#include <ctype.h>
#include "glu.h"
#include "mfio.h"
#include "rksubs.h"
#include "plots.h"

void setmf(const char *fname, const char *station, const char *title,
   const char *icon, long buflen, ui32 dbgmask, int enc, int lci,
   int lcf) {

#ifndef PARn
   /* Error if already started plotting */
   if (_RKG.MFFlags & MFF_ModeSet) abexit(96);

   /* Kill any previous setup */
   _RKG.s.MFMode = 0;

   /* Handle 'fname' argument */
   if (_RKG.MFfn) {
      freev(_RKG.MFfn, "Metafile name");
      _RKG.MFfn = NULL; }
   if (fname) {
      _RKG.s.MFMode |= METF;
      if (fname[0] == '\0')
         _RKG.s.MFMode |= MFSO;
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

   /* Handle 'buflen' argument */
   _RKG.s.MFBuffLen = (buflen > 0) ? buflen : DFLTBUFLEN;

   /* Handle 'dbgmask' argument */
   _RKG.s.dbgmask |= dbgmask;

   /* Handle 'lci' argument */
   _RKG.s.lci = (short)lci;

   /* Handle 'lcf' argument */
   if (lcf <= 0 || lcf > 24) lcf = 10;
   _RKG.s.lcf = (short)lcf;

   /* Handle 'enc' argument */
   if      (enc == '8') _RKG.s.MFMode |= (ASCMF|MFD8C);
   else if (enc == 'A') _RKG.s.MFMode |= ASCMF;
   else if (enc != 'B') mfckerr(ERR_MFMT);

   /* Tell newplt() this call has been made */
   _RKG.MFFlags |= MFF_SetMFRan;

#endif /* !PARn */

   } /* End setmf() */

