/* (c) Copyright 1992, Neurosciences Research Foundation, Inc. */
/***********************************************************************
*                               rstdump                                *
*                                                                      *
*  Jiffy routine to dump header of a version 2 CNS SAVENET file.       *
*  The name of the file is passed as the only command-line argument.   *
*  V1A, 04/01/92, GNR                                                  *
***********************************************************************/

#define  MAIN
#define  D1
#define  LSNAME         4     /* Should match d3global.h */
#define  D3NAM_LENGTH   4     /* So works with old savblk.h */
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "sysdef.h"
#ifdef NN
#include "/home/cdr/d3/savblk.h"
#else
#include "savblk.h"
#endif
#include "rocks.h"
#include "rfdef.h"

long swap4(long i4);             /* Swapper prototype */

main(int argc, char *argv[]) {

   struct RFdef *fp;
   struct SFHDR   sfh;
   struct SFD1HDR d1h;
   struct SFCTHDR cth;
   struct SFCNHDR cnh;
   int i,j;

/* Start up ROCKS etc. */

   RK.iexit = 0;
   settit("TITLE Dump CNS Version 2 SAVENET file");
   setpid("rstdump");
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

/* Read and print header info */

   rfread(fp,&sfh,sizeof(struct SFHDR),ABORT);
   if (sfh.vers != 2) {
      cryout(RK_P1,"0***SAVENET file not version 2.",RK_LN2,NULL);
      return 98;
      }
   cryout(RK_P1,"0Overall header",RK_LN5,NULL);
   convrt("(P1,4X,'Version: ',j0ih4,', Number d1 blocks: ',j0ih4,"
      "', Number layers: ',j0ih4,', Spare field: ',j0ih4)",
      &sfh.vers,&sfh.sfhnd1b,&sfh.sfhnlay,&sfh.spare,NULL);
   cryout(RK_P1,"    Title: ",RK_LN1,sfh.title,TITLE_LEN,NULL);
   cryout(RK_P1,"    Date/time: ",RK_LN1,sfh.timestamp,DATE_LEN,NULL);

/* Loop over Darwin I blocks and print them */

   for (i=1; i<=sfh.sfhnd1b; i++) {
      rfread(fp,&d1h,sizeof(struct SFD1HDR),ABORT);
      convrt("(P1,'0Darwin I header ',j0i4)",&i,NULL);
      convrt("(P1,4X,'Elements: ',j0il8,', Seed: ',j0il12)",
         &d1h.sfd1nepr,&d1h.sfd1seed,NULL);
      convrt("(P1,4X,'Repertoires: ',j0ih6,', Bits/element: ',j0ih6)",
         &d1h.sfd1nd1r,&d1h.sfd1nbpe,NULL);
      convrt("(P1,4X,'S offset: ',j0il8,', R offset: ',j0il8)",
         &d1h.offd1s,&d1h.offd1r,NULL);
      }

/* Loop over Layer blocks and print them */

   for (i=1; i<=sfh.sfhnlay; i++) {
      long bits;
      rfread(fp,&cth,sizeof(struct SFCTHDR),ABORT);
      convrt("(P1,'0Layer header ',j0i4)",&i,NULL);
      cryout(RK_P1,"    Rep name: ",RK_LN1,cth.repnam,LSNAME,
         ", Layer name: ",RK_CCL,cth.celnam,LSNAME,NULL);
      convrt("(P1,4X,'NGX: ',j0il8,', NGY: ',j0il8,"
         "', NEL: ',j0il8,', NCE: ',j0il8)",
         &cth.sfctngx,&cth.sfctngy,&cth.sfctnel,&cth.sfctnce,NULL);
      convrt("(P1,4X,'LS: ',j0il8,', LEL: ',j0il8,', LLT: ',j0il8)",
         &cth.sfctls,&cth.sfctlel,&cth.sfctllt,NULL);
      convrt("(P1,4X,'NSEED: ',j0il12,', PHISEED: ',j0il12)",
         &cth.sfctnsd,&cth.sfctpsd,NULL);
      memcpy(&bits,cth.sfctbits,sizeof(long));
      bits = swap4(bits);
      convrt("(P1,4X,'BITS: ',Z8,', NCT: ',j0ih4,', PHSHFT: ',j0ih4)",
         &bits,&cth.sfctnct,&cth.sfctphshft,NULL);
      convrt("(P1,4X,'S offset: ',j0il8,', R offset: ',j0il8)",
         &cth.offsi,&cth.offprd,NULL);

/* Loop over Conntype blocks and print them */

      for (j=1; j<=cth.sfctnct; j++) {
         rfread(fp,&cnh,sizeof(struct SFCNHDR),ABORT);
         convrt("(P1,'0Conntype header ',j0i4)",&j,NULL);
         cryout(RK_P1,"    Src rep name: ",RK_LN1,cnh.srcsnam,
            LSNAME,", Src layer name: ",RK_CCL,cnh.srclnam,
            LSNAME,NULL);
         convrt("(P1,4X,'NC: ',j0il8,', LC: ',j0ih4,"
            "', NBC: ',j0ih4,', KGEN: ',Z8)",
            &cnh.sfcnnc,&cnh.sfcnlc,&cnh.sfcnnbc,&cnh.sfcnkgen,NULL);
         convrt("(P1,4X,'SRCNGX: ',j0il8,', SRCNGY: ',j0il8,"
            "', SRCNEL: ',j0il8,', SRCNELT: ',j0il8)",
            &cnh.sfcnsrcngx,&cnh.sfcnsrcngy,&cnh.sfcnsrcnel,
            &cnh.sfcnsrcnelt,NULL);
         convrt("(P1,4X,'CSEED: ',j0il12,', NSEED: ',j0il12,"
            "', LSEED: ',j0il12,', RSEED: ',j0il12)",
            &cnh.sfcncseed,&cnh.sfcnnseed,&cnh.sfcnlseed,
            &cnh.sfcnrseed,NULL);
         convrt("(P1,4X,'R offset: ',j0il8)",
            &cnh.offprd,NULL);
         } /* End conntype loop */
      } /* End celltype loop */

/* Finished */

   rfclose(fp,SAME,SAME,ABORT);
   cryout(RK_P1,"   ",RK_LN1+RK_FLUSH,NULL);
   return 0;
   } /* End rstdump */

