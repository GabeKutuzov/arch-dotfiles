/* (c) Copyright 1995-2012, The Rockefeller University *11115* */
/* $Id: findmdlt.c 76 2017-12-12 21:17:50Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                              findmdlt                                *
*                                                                      *
*  This function is used to locate a MODALITY block given its name     *
*  and the number of a window through which it is viewed (0 if not     *
*  viewed through a window).  If the block does not exist, one is      *
*  created, unless the window number had the special code value -1.    *
*  A NULL pointer is returned in this case or if the maximum allowed   *
*  number of modalities is exceeded.                                   *
*                                                                      *
*  Note that if a modality is created by the user at Group I time and  *
*  then used only for viewing through a window, the window modality    *
*  block will inherit the ncs,nds parameters from the parent, which    *
*  may have no other use.                                              *
*                                                                      *
*  Synopsis:  struct MODALITY *findmdlt(char *mdltname, int iwdw)      *
************************************************************************
*  V8A, 04/29/95, GNR - Initial version                                *
*  V8B, 12/03/00, GNR - Move to new memory management routines         *
*  Rev, 08/11/01, GNR - Use text cache to remove mdltname length limit *
*  ==>, 05/30/07, GNR - Last mod before committing to svn repository   *
*  Rev, 08/16/12, GNR - Add mdltbid                                    *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "rocks.h"
#include "rkxtra.h"
#include "bapkg.h"

#ifdef PARn
#error This routine is not used on comp nodes
#else
struct MODALITY *findmdlt(char *mname, int itst) {

   struct MODALITY *pmdlt;       /* Scan pointer */
   struct MODALITY **ppmdlt;     /* Linked-list linker */
   int imdlt = 1;                /* Index of this modality */
   int iwdw = max(itst, 0);      /* Number of view window */
   txtid htmn = findtxt(mname);  /* Temp modality name locator */

/* Search for modality block with requested name and number */

   /* Assignment intended in for loop test clause on next line */
   for (ppmdlt=&RP->pmdlt1; pmdlt=*ppmdlt; ++imdlt) {
      if (htmn == pmdlt->hmdltname &&
         (iwdw == (int)pmdlt->mdltwdw)) break;
      ppmdlt = &pmdlt->pnmdlt;
      }

/* If requested modality not found, create it, unless window number
*  was specified as -1, which indicates that we are searching for
*  the parent modality of a window modality, in which case there is
*  no need to create it if it doesn't already exist.  Give error
*  if maximum number of modalities has already been reached.  */

   if (!pmdlt && itst >= 0) {
      if (RP->nmdlt >= MAX_MODALITIES)
         cryout(RK_E1, "0***UNABLE TO CREATE MODALITY \"", RK_LN2,
            mname, LTEXTF, "\"", RK_CCL+1,
            " ***MAX OF ", RK_LN1, qqv(MAX_MODALITIES), RK_CCL,
            " MODALITIES EXCEEDED.", RK_CCL, NULL);
      else {
         pmdlt = (struct MODALITY *)
            allocpcv(Static, 1, IXstr_MODALITY|MBT_Unint, "modality");
         mdltdflt(pmdlt);           /* Initialize MODALITY block */
         pmdlt->hmdltname = (txtid)savetxt(mname);
         if (iwdw > 0) {
            /* Store window number and pointer to actual window */
            pmdlt->mdltwdw = iwdw;
            pmdlt->pmdwdw = RP0->ppwdw[iwdw - 1];
            }
         pmdlt->mdltno = imdlt;     /* Store modality index num */
         bitset((byte *)(&pmdlt->mdltbid), (long)imdlt);
         *ppmdlt = pmdlt;           /* Link into end of list */
         }
      } /* End if block not found */

   return pmdlt;
   } /* End findmdlt */
#endif
