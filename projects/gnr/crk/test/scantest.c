/*--------------------------------------------------------------------*/
/*                                                                    */
/*    CRYIN and SCAN test                                             */
/*                                                                    */
/* Rev, 10/25/01, GNR - Interpret RK.scancode for the user            */
/*--------------------------------------------------------------------*/

/*
   This test is designed to run interactively.  The program reads
   control cards from the keyboard, prints them, then calls cdscan    
   and scan to parse them.  The resulting fields and scancodes are
   printed.  Odd scan flags must be tested by recompilation.    
*/

#define MAIN
#include <stdio.h>
#include <stdlib.h>
#ifdef PC
#include <conio.h>
#define PUTCHAR(c) putch(c)
#else
#define PUTCHAR(c) (putchar(c), fflush(stdout))
#endif
#include <ctype.h>
#include <string.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"

void main() {

   char lea[33];
   char bcdic[13];
   char *card;
   int ic,ict;
   int len;

   char *scodetxt[] = { "PMI", "COM", "EQL", "AST",
      "SLA", "LPN", "INP", "RTP", "END" };

/* Do until terminated */
   for (;;) {                 /* Read loop */
      card = cryin();
      if (!card || (ssmatch(card,"QUIT",4) > 0)) exit(0);
      cdprnt(card);
      printf("\nCalling cdscan...\n");
      cdscan(card,0,32,0);
      for (;;) {              /* Scan loop */
         /* Try various codes here */
#if 0
         ic = scan(lea, RK_ASTDELIM|RK_SLDELIM|RK_PMDELIM);
#else
         ic = scan(lea, RK_SCANFNM);
#endif
         if (RK.scancode & RK_ENDCARD) break;
         len = RK.length;
         ibcdwt(3587L,&bcdic[0],(long)ic);
         ibcdwt(3587L,&bcdic[4],(long)RK.scancode);
         ibcdwt(3587L,&bcdic[8],(long)len);
         cryout(RK_P2," Scan returned ic, scancode, length =",RK_LN1,
            bcdic,RK_CCL+12,NULL);
         cryout(RK_P2,"    with field = ",RK_LN1,lea,RK_CCL,NULL);
         if (ic) {
            cryout(RK_P2,"    Scancode bits:",RK_LN1,NULL);
            for (ict=0; ic; ic>>=1,++ict) if (ic & 1) {
               cryout(RK_P2, " ", 1, scodetxt[ict], 3, NULL);
               if (ic & ~1) cryout(RK_P2, ",", 1, NULL);
               }
            }
         cryout(RK_P2," ",RK_CCL|RK_FLUSH, NULL);
         } /* End of scan loop */
      } /* End of forever loop */
   } /* End of program */

