/* (c) Copyright 1992, Neurosciences Research Foundation, Inc. */
/***********************************************************************
*                              rstoldmp                                *
*                                                                      *
*  Jiffy routine to dump header of a version 1 CNS SAVENET file.       *
*  The name of the file is passed as the only command-line argument.   *
*  V1A, 04/07/92, GNR - Rewrite of rstdump.c                           *
***********************************************************************/

#define  MAIN
#define  NDVARS        24     /* Number of dynamic variables */
#define  D3NAM_LENGTH  20     /* So works with old savblk.h */
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "sysdef.h"
#include "rocks.h"
#include "rfdef.h"

struct OLDSAVBLK {
   char repnam[D3NAM_LENGTH];    /*   ir->name     */
   long rgrp;                    /*   ir->ngrp     */ 
   char typenm[D3NAM_LENGTH];    /*   il->lname    */
   short xnit;                   /*   il->nit      */
   short xtype;                  /*   il->nct      */
   long xperel;                  /*   il->nce      */
   long lencel;                  /*   il->ls       */
   long lenel;                   /*   il->lel      */
   long lenlay;                  /*   il->llt      */
   long celgrp;                  /*   il->nel      */
   byte cbits[(NDVARS+7)/8];     /*   il->bits     */
   } *oldhdr;

struct SEEDBLK {
   long cseed;
   long lseed;
   long nseed;
   long rseed;
   } *seeds;

long swap4(long i4);             /* Swapper prototype */

main(int argc, char *argv[]) {

   struct RFdef *fp;
   long offhdr = 0, offdat = 0;
   long nelt;
   int nlay,nctp,rc,j;

/* Start up ROCKS etc. */

   RK.iexit = 0;
   settit("TITLE Dump CNS Version 1 SAVENET file");
   setpid("rstoldmp");
   cryout(RK_P1,"0CNS SAVENET file dump",RK_LN2+RK_SUBTTL,NULL);

/* Make sure there is a command-line argument */

   if (argc == 1) {
       cryout(RK_P1,"0***No SAVENET file specified.",RK_LN2,NULL);
       return 99; 
       }

/* Allocate rfdef block and try to open file */

   fp = rfallo(argv[1], READ, BINARY, SEQUENTIAL,
      TOP, LOOKAHEAD, REWIND, RELEASE_BUFF, 8192, 0, 0, ABORT);
   rfopen(fp, NULL, SAME, SAME, SAME, SAME, SAME, SAME, SAME,
      SAME, SAME, SAME, ABORT);
   oldhdr = (struct OLDSAVBLK *)mallocv(sizeof(struct OLDSAVBLK),
      "OLDSAVBLK");
   seeds = (struct SEEDBLK *)mallocv(sizeof(struct SEEDBLK),
      "SEEDBLK");

/* Loop over Layer blocks and print them */

   for (nlay=nctp=0; ;) {        /* Loop until eof */
      long bits;
      rc = rfread(fp,oldhdr,sizeof(struct OLDSAVBLK),ABORT);
      if (!rc) break;
      nlay++;
      convrt("(P1,'0Layer header ',j1i4,' at offset ',j0il8)",
         &nlay,&offhdr,NULL);
      cryout(RK_P1,
         "    Rep name: ",RK_LN1,oldhdr->repnam,D3NAM_LENGTH,
         ", Layer name: ",RK_CCL,oldhdr->typenm,D3NAM_LENGTH,NULL);
      convrt("(P1,4X,'NGRPS: ',j0il8,', NIT: ',j0ih8,"
         "', NCT: ',j0ih8)",
         &oldhdr->rgrp,&oldhdr->xnit,&oldhdr->xtype,NULL);
      convrt("(P1,4X,'NCE: ',j0il8,', LS: ',j0il8,', LEL: ',j0il8)",
         &oldhdr->xperel,&oldhdr->lencel,&oldhdr->lenel,NULL);
      memcpy(&bits,oldhdr->cbits,sizeof(long));
      bits = swap4(bits);
      convrt("(P1,4X,'LLT: ',j0il12,', NEL: ',j0il8,', BITS: ',Z8)",
         &oldhdr->lenlay,&oldhdr->celgrp,&bits,NULL);

/* Loop over SEED blocks and print them */

      for (j=1; j<=oldhdr->xtype; j++) {
         rc = rfread(fp,seeds,sizeof(struct SEEDBLK),ABORT);
         if (!rc) break;
         convrt("(P1,'0Conntype SEED header ',j0i4)",&j,NULL);
         convrt("(P1,4X,'CSEED: ',j0il12,', LSEED: ',j0il12,"
            "', NSEED: ',j0il12,', RSEED: ',j0il12)",
            &seeds->cseed,&seeds->lseed,&seeds->nseed,
            &seeds->rseed,NULL);
         } /* End conntype loop */

/* Update offsets, advance file to next header */

      offdat = offhdr + sizeof(struct OLDSAVBLK) +
         oldhdr->xtype*sizeof(struct SEEDBLK);
      convrt("(P1,'0Offset of data: ',j0il12)",&offdat,NULL);
      nelt = oldhdr->rgrp*oldhdr->celgrp;
      offhdr = offdat + nelt*(1 + oldhdr->lenel);
      rfseek(fp,offhdr,SEEKABS,ABORT);
      } /* End celltype loop */

/* Finished */

   rfclose(fp,SAME,SAME,ABORT);
   convrt("(P1,'0TOTAL CELL TYPES FOUND: ',j0i8,', CONNTYPES: ',j0i8)",
      &nlay,&nctp,NULL);
   cryout(RK_P1,"   ",RK_LN1+RK_FLUSH,NULL);
   return 0;
   } /* End rstdump */

