/* (c) Copyright 1992-2015, The Rockefeller University *21115* */
/* $Id: mfhead.c 20 2011-12-24 02:59:03Z  $ */
/***********************************************************************
*                     ROCKS metafile plot library                      *
*                              mfhead()                                *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void mfhead(void)                                                *
*                                                                      *
*  DESCRIPTION:                                                        *
*     This function writes the header of the metafile.  The header     *
*     is made of three lines.  The first line identifies the file as   *
*     an RKG graphic metafile and gives the version.  The second line  *
*     contains the title and the third line contains the date.  This   *
*     function is called only once in a program's lifetime.  Note that *
*     flushing the buffer is not needed, because a start record is     *
*     written next.                                                    *
*                                                                      *
*  RETURN VALUES:                                                      *
*     None                                                             *
************************************************************************
*  Version 1, 05/12/92, ROZ                                            *
*  Rev, 07/15/93, GNR - Remove args, itta(), output of state,          *
*                       use MFCmdBuff, correct record 1 format         *
*  Rev, 07/30/97, GNR - Add MFB_INIT bit to MFCountWord                *
*  Rev, 10/11/11, GNR - _NCG to _RKG, gmf.h now in glu.h, etc.         *
*                       Use title from setmf(), not gettit(), to       *
*                       remove dependency on cryout machinery.         *
*  Rev, 10/04/15, GNR - Update for current library spec                *
*  ==>, 10/07/15, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <ctype.h>
#include <string.h>
#include "sysdef.h"
#include "mfint.h"
#include "mfio.h"
#include "rksubs.h"

#ifdef PARn
#error File mfhead.c should not be compiled for PARn nodes
#else

void mfhead(void) {

   Win *pwin = _RKG.pcw;
   int lrec,ltit;             /* Length of record,title */

   /* Signal that this record contains headers */
   pwin->MFCountWord |= MFB_INIT;

   /* Version record */
#define LEN_VERS_REC  21
   mfhchk(lrec = Bytes2Bits(LEN_VERS_REC));
   ssprintf(pwin->MFCP, "PLOTDATA V2A B%3ed%3ed\n",
      (int)_RKG.s.lci, (int)_RKG.s.lcf);
   pwin->MFCP += LEN_VERS_REC;
   pwin->MFBrem -= lrec;

   /* Title record */
#define LEN_TITL_REC  61
   mfhchk(lrec = Bytes2Bits(LEN_TITL_REC));
   ltit = LEN_TITL_REC - 1;
   if (_RKG.MFDwtitl) {
      int lts = strnlen(_RKG.MFDwtitl, ltit);
      if (lts > 0) {
         memcpy((char *)pwin->MFCP, _RKG.MFDwtitl, lts);
         pwin->MFCP += lts;
         ltit -= lts;
         }
      }
   if (ltit > 0) {
      memset((char *)pwin->MFCP, ' ', ltit);
      pwin->MFCP += ltit;
      }
   *pwin->MFCP++ = '\n';
   pwin->MFBrem -= lrec;


   /* Time stamp record */
#define LEN_TIME_REC  13
   mfhchk(lrec = Bytes2Bits(LEN_TIME_REC));
   tstamp((char *)pwin->MFCP);
   pwin->MFCP[LEN_TIME_REC-1] = '\n';
   pwin->MFCP += LEN_TIME_REC;
   pwin->MFBrem -= lrec;

   } /* End mfhead() */
#endif /* Not PARn */
