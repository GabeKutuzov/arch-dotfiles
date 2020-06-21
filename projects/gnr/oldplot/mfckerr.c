/* (c) Copyright 1994-2017, The Rockefeller University *21114* */
/* $Id: mfckerr.c 34 2017-09-15 18:13:10Z  $ */
/***********************************************************************
*                              mfckerr()                               *
*                                                                      *
* SYNOPSIS:                                                            *
*  void mfckerr(ui32 error)                                            *
*                                                                      *
* DESCRIPTION:                                                         *
*  This function analyzes an error code returned by mfsr and exits     *
*  with the appropriate abexit error number.                           *
*                                                                      *
* N.B.:                                                                *
*  On parallel computers, the error code may arrive with multiple      *
*  bits set.  We have no standard method to produce multiple separate  *
*  error messages, so this code now prints the hexagonal value of the  *
*  error code.  Not great, but better than nothing for rare events.    *
************************************************************************
*  V1A, 03/05/94,  LC, GNR - Newly written                             *
*  Rev, 02/24/08, GNR - Serial versions exit with abexitm messages     *
*  ==>, 03/01/08, GNR - Last mod before committing to svn repository   *
*  Rev, 10/11/11, GNR - _NCG to _RKG, gmf.h now in glu.h, etc.         *
*  Rev, 04/08/17, GNR - Print hex value of error code.                 *
*  R34, 08/01/17, GNR - Bug fix: endless wait mfckerr->abexit->endplt  *
***********************************************************************/

#include <stddef.h>
#include "sysdef.h"
#include "rocks.h"
#include "bapkg.h"
#include "glu.h"

void mfckerr(ui32 error) {

   ui32 errbit;
   int i;

   if (error == 0) return;

   _RKG.MFFlags |= MFF_MEnded;
   if (bitcnt(&error, JSIZE) > 1) {
      char hexerr[48];
      ssprintf(hexerr, "Multiple graph pkg errors, hex code = 0x%8jex",
         &error);
      abexitm(263, hexerr);
      }

   else for (i=0,errbit=1; i<ERRTOT; i++) {
      if (error & errbit)
         abexit(ERRBEG+i);
      else
         errbit<<=1;
      }
   } /* End mfckerr */

