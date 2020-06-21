/* (c) Copyright 1992-2017, The Rockefeller University *21114* */
/* $Id: mfhead.c 36 2018-05-07 22:18:50Z  $ */
/***********************************************************************
*                              mfhead()                                *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void mfhead(void)                                                *
*                                                                      *
*  DESCRIPTION:                                                        *
*     This function writes the header of the metafile.  The header     *
*     is made of three lines.  The first line identifies the file as   *
*     an NSI graphic metafile and gives the version.  The second line  *
*     contains the title and the third line contains the date.  This   *
*     function is called only once in a program's lifetime.  Note that *
*     flushing the buffer is not needed, because mfst is called next.  *
*                                                                      *
*  RETURN VALUES:                                                      *
*     None                                                             *
************************************************************************
*  Version 1, 05/12/92, ROZ                                            *
*  Rev, 07/15/93, GNR - Remove args, itta(), output of state,          *
*                       use MFCmdBuff, correct record 1 format         *
*  Rev, 07/30/97, GNR - Add MFB_INIT bit to MFCountWord                *
*  ==>, 01/25/07, GNR - Last mod before committing to svn repository   *
*  Rev, 10/11/11, GNR - _NCG to _RKG, gmf.h now in glu.h, etc.         *
*                       Use title from setmf(), not gettit(), to       *
*                       remove dependency on cryout machinery.         *
*  R35, 09/29/17, GNR - Use L_TITL from rocks.h, not local length 61   *
***********************************************************************/

#include <ctype.h>
#include "glu.h"
#include "rocks.h"
#include "rkxtra.h"

#ifndef PARn

void mfhead(void) {

   /* Signal that this record contains headers */
   _RKG.MFCountWord |= MFB_INIT;

   /* Version record */
#define LEN_VERS_REC  21
   mfhchk(LEN_VERS_REC);
   memcpy((char *)_RKG.MFCurrPos,
      "PLOTDATA V1C D  8  3\n",LEN_VERS_REC);
   _RKG.MFCurrPos += LEN_VERS_REC;

   /* Title record */
   mfhchk(L_TITL+1);
   if (_RKG.MFDwtitl) {
      int lts = strnlen(_RKG.MFDwtitl, L_TITL);
      if (lts > 0) {
         memcpy((char *)_RKG.MFCurrPos, _RKG.MFDwtitl, lts);
         _RKG.MFCurrPos += lts;
         }
      }
   *_RKG.MFCurrPos++ = '\n';

   /* Time stamp record */
#define LEN_TIME_REC  13
   mfhchk(LEN_TIME_REC);
   tstamp((char *)_RKG.MFCurrPos);
   *(_RKG.MFCurrPos+(LEN_TIME_REC-1)) = '\n';
   _RKG.MFCurrPos += LEN_TIME_REC;

   } /* End mfhead() */
#endif
