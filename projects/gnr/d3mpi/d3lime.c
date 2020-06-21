/* (c) Copyright 2013, The Rockefeller University *11115* */
/* $Id: d3lime.c 74 2017-09-18 22:18:33Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                              d3lime.c                                *
*                                                                      *
*             Print error messages relating to variables               *
*                 that exceed size limitations in CNS                  *
*                                                                      *
*  This routine is designed to be called from e64act when an overflow  *
*  is detected in calculations relating to memory allocation, so the   *
*  arguments are those specified for an E64_CALL-type 'ec' argument    *
*  to e64act or a math routine.                                        *
*                                                                      *
*  Synopsis:                                                           *
*     d3lime(char *fnnm, int ec)                                       *
*                                                                      *
*  Arguments:                                                          *
*     fnnm     Name of the calling routine.                            *
*     ec       Error code containing 3 components as follows:          *
*                 Left byte:  Ignored, may be 4 indicating an E64_CALL *
*                    error type.                                       *
*                 Middle two bytes:  A unique code to identify the     *
*                    particular error being reported.                  *
*                 Right byte:  Number of the particular connection     *
*                    type, camera, effector, or value, etc. where      *
*                    the error occurred.                               *
*                                                                      *
*  Globals:                                                            *
*     Program assumes that CDAT.cdlayr contains a pointer to the       *
*     celltype where the error occurred, if relevant (because we       *
*     can't put it in the call args and meet the E64_CALL spec).       *
************************************************************************
*  V8I, 05/18/13, GNR - Initial version                                *
*  ==>, 05/18/13, GNR - Last mod before committing to svn repository   *
*  R72, 02/17/17, GNR - Add PLC_KERN error                             *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "sysdef.h"
#include "d3global.h"
#include "celldata.h"
#include "rocks.h"
#include "rkxtra.h"

extern struct CELLDATA CDAT;

/***********************************************************************
*                              d3lime()                                *
***********************************************************************/

void d3lime(char *fnnm, int ec) {

   struct CELLTYPE *il = CDAT.cdlayr;
   int ierr = (ec >> BITSPERBYTE) & UI16_MAX;
   int item = ec & BYTE_MAX;
   int use64 = NO;
   char ait[6];

   sconvrt(ait, "(J1U4,X)", item);

   switch (ierr) {
   case PLC_GCWS: {
      char sigt[6];
      if (item > 0) wbcdwt(&item, sigt, RK_IORF|RK_LFTJ|RK_NINT);
      else strcpy(sigt, "ALL");
      cryout(RK_E1, "0***MEM FOR ", RK_LN3, fmturlnm(il), RK_CCL,
         sigt, RK_CCL, " GCONNS EXCEEDS 32 BITS.", RK_CCL, NULL);
#ifdef INTEL32
      use64 = YES;
#endif
      break;
      }
   case PLC_GCXS:
      cryout(RK_E1, "0***EXTENDED GCONN EXCEEDS LAYER SIZE FOR GCONN "
         "TYPE", RK_LN2, ait, 5, "TO ", 3, fmturlnm(il), LXRLNM, NULL);
      break;
   case PLC_RNQY:
      cryout(RK_E1, "0***PLOT ROWS EXCEED 65536 FOR REGION ",
         RK_LN2, il->pback->sname, LSNAME, NULL);
      break;
   case PLC_KERN:
      cryout(RK_E1, "0***CONNECTION KERNEL SIZE > 2^28 FOR CONNTYPE ",
         RK_LN2, ait, 5, "TO ", 3, fmturlnm(il), LXRLNM, NULL);
      break;
   case PLC_TVSZ:
      cryout(RK_E1, "0***IMAGE EXCEEDS 32 BITS FOR CAMERA ", RK_LN2,
         ait, 5, NULL);
      break;
   case PLC_PPSZ:
      cryout(RK_E1, "0***PREPROCESSOR INPUT EXCEEDS 16 BITS FOR "
         "CAMERA ", RK_LN2, ait, 5, NULL);
      break;
   case PLC_ALLO:
      cryout(RK_E1, "0***MEMORY FOR ", RK_LN3, fmturlnm(il), RK_CCL,
         " CELL DATA EXCEEDS 32 BITS.", RK_CCL, NULL);
#ifdef INTEL32
      use64 = YES;
#endif
      break;
      } /* End ierr switch */

   if (use64)
      cryout(RK_E1, " ***USE 64-BIT VERSION OF CNS.", RK_LN0, NULL);

   } /* End d3lime() */
