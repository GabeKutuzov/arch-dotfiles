/* (c) Copyright 1994-2015, The Rockefeller University *211115* */
/* $Id: mfckerr.c 5 2008-03-13 20:32:23Z  $ */
/***********************************************************************
*                     ROCKS metafile plot library                      *
*                              mfckerr()                               *
*                                                                      *
* DESCRIPTION:                                                         *
*  This function analyzes an error code returned by mfsr or a plot     *
*  library routine and exits with the appropriate abexit error number. *
*  It is intended for use on a parallel computer, where it may need    *
*  to shut down a comp node network.  On a serial computer, it should  *
*  just perform the equivalent abexit() termination (where it is not   *
*  expected to provide the error message text that an abexitm() call   *
*  would provide, so this call should be avoided in serial-only code.) *
*                                                                      *
* SYNOPSIS:                                                            *
*  void mfckerr(ui32 error)                                            *
*                                                                      *
* N.B.                                                                 *
*  On parallel computers, the error code may arrive with multiple      *
*  bits set.  This routine should accordingly produce multiple error   *
*  messages.  This needs to be fixed next time we have a parallel      *
*  system available for testing.                                       *
************************************************************************
*  V1A, 03/05/94,  LC, GNR - Newly written                             *
*  Rev, 02/24/08, GNR - Serial versions exit with abexitm messages     *
*  Rev, 10/11/11, GNR - _NCG to _RKG, gmf.h now in glu.h, etc.         *
*  ==>, 04/12/15, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stddef.h>
#include "sysdef.h"
#include "rocks.h"
#include "mfio.h"

void mfckerr(ui32 error) {

   ui32 errbit = 1;
   int i;

   if (error == 0) return;

   for (i=0; i<ERRTOT; i++) {
      if (error & errbit)
         abexit(ERRBEG+i);
      else
         errbit<<=1;
      }
   } /* End mfckerr */
