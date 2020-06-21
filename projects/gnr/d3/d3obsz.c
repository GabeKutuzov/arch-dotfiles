/* (c) Copyright 2016, The Rockefeller University *11113* */
/***********************************************************************
*         Test program -- print sizes of major Cns structures          *
*                                                                      *
************************************************************************
*  R67, 04/27/16, GNR - Remove Darwin 1 support                        *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "collect.h"
#include "d3global.h"
#include "celldata.h"
#include "ijpldef.h"
#include "clblk.h"
#include "statdef.h"
#include "tvdef.h"

int main(int argc, char *argv[]) {

   char *itnms[] = {
"armwdw_type",
"fdm_type",
"gac_type",
"ilst",
"ilstitem",
"outnow_type",
"struct AUTSCL",
"struct BREGDEF",
"struct CLSTAT",
"struct GPSTAT",
"struct CELLTYPE",
"struct CLBLK",
"struct CONDUCT",
"struct CNSTAT",
"struct CONNTYPE",
"struct CPDEF",
"struct FDHEADER",
"struct IJPLNODE",
"struct IONTYPE",
"struct INHIBBAND",
"struct INHIBBLK",
"struct IZHICOM",
"struct IZ03DEF",
"struct IZ07DEF",
"struct MODALITY",
"struct MODCOM",
"struct MODBY",
"struct MODVAL",
"struct PCF",
"struct PHASEDEF",
"struct PPFDATA",
"struct PRBDEF",
"struct PREPROC",
"struct REPBLOCK",
"struct RFRCDATA",
"struct RPDEF",
"struct UTVDEF",
"struct VBDEF",
"struct VDTDEF",
"struct TVDEF",
NULL
      };
   int itsz[] = {
      sizeof(armwdw_type),
      sizeof(fdm_type),
      sizeof(gac_type),
      sizeof(ilst),
      sizeof(ilstitem),
      sizeof(outnow_type),
      sizeof(struct AUTSCL),
      sizeof(struct BREGDEF),
      sizeof(struct CLSTAT),
      sizeof(struct GPSTAT),
      sizeof(struct CELLTYPE),
      sizeof(struct CLBLK),
      sizeof(struct CONDUCT),
      sizeof(struct CNSTAT),
      sizeof(struct CONNTYPE),
      sizeof(struct CPDEF),
      sizeof(struct FDHEADER),
      sizeof(struct IJPLNODE),
      sizeof(struct IONTYPE),
      sizeof(struct INHIBBAND),
      sizeof(struct INHIBBLK),
      sizeof(struct IZHICOM),
      sizeof(struct IZ03DEF),
      sizeof(struct IZ07DEF),
      sizeof(struct MODALITY),
      sizeof(struct MODCOM),
      sizeof(struct MODBY),
      sizeof(struct MODVAL),
      sizeof(struct PCF),
      sizeof(struct PHASEDEF),
      sizeof(struct PPFDATA),
      sizeof(struct PRBDEF),
      sizeof(struct PREPROC),
      sizeof(struct REPBLOCK),
      sizeof(struct RFRCDATA),
      sizeof(struct RPDEF),
      sizeof(struct UTVDEF),
      sizeof(struct VBDEF),
      sizeof(struct VDTDEF),
      sizeof(struct TVDEF),
      0
      };
   int i=0;

   while (itsz[i] > 0) {
      printf("Size of %s     = %d\n",itnms[i],itsz[i]);
      ++i;
      }
   return 0;
   }



