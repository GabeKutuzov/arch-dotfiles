/*--------------------------------------------------------------------*/
/*                                                                    */
/*                            CRYIN Test                              */
/*                                                                    */
/*  Rev, 08/22/98, GNR - Add test of cdunit and read from second unit */
/*  Rev, 04/13/13, GNR - Return int, add cry[io]cls calls at the end  */
/*  Rev, 04/22/16, GNR - Add test of SETENV card                      */
/*--------------------------------------------------------------------*/

#define MAIN
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rocks.h"

#define LCD 80
#define InFile  "cryitest.dat"
#define InFile2 "cryitst2.dat"

/* This program expects to read an input file named InFile
*  with the following data:
*  (1) TITLE card
*  (2) Text
*  (3) Various comments
*  (4) A DEFAULT card with values for the ENVSET card
*  (5) More text
*  (6) An ENVSET card
*  (7) Another plain text line
*  (8) EOF
*  All lines are read with gets() and printed.
*  Then they are read and printed with cryin/cdprt1
*  But after item (5), the second input file is interjected.
*
*  Define STDIN at compile time to read from the keyboard instead
*/

int main() {

   char *sgot;
   int   oeol;
   int   line;

#ifndef STDIN
   FILE *fd;
   char *seol;
   int   seof;
   char card[LCD+2];

/* First read directly with fgets and print results */

   fd = fopen(InFile,"r");
   printf("Open returned %p\n",fd);
   if (!fd) exit(1);

/* Read each line with fgets and print results */

   for (line=1; ; line++) {
      sgot = fgets(card,LCD+1,fd);
      seof = feof(fd);
      if (sgot == NULL || seof != 0) {
         printf("End-of-file reached reading line %d\n",line);
         break;
         }
      seol = (sgot ? strchr(sgot,'\n') : 0);
      oeol = (seol ? (seol-sgot) : 0);
      printf("For line %d, fgets returns %s\n"
         "   feof returns %d, newline offset is %d, data is\n"
         "==>%s\n",line,sgot,seof,oeol,card);
      } /* End reading directly */
   seof = fclose(fd);
   printf("Close returned %d\n",seof);
#endif

/* Repeat using cryin */

   printf("Now reading with cryin...\n");
   fflush(stdout);
   settit("TITLE Cryin Test Program");
   sprmpt("==>");
#ifndef STDIN
   cdunit(InFile);
#endif
   for (line=1; ; line++) {
#ifndef STDIN
      if (line == 4) cdunit(InFile2);
#endif
      sgot = cryin();
      if (sgot == NULL) {
         convrt("('0For line ',J0I6,', cryin returned end-of-file.')",
            &line, NULL);
         break; }
      oeol = strlen(sgot);
      convrt("('0For line ',J0I6,', cryin returns ',J0A80/"
         "'   line length is ',J0I6,', cdprt1 result on next line:')",
         &line, sgot, &oeol, NULL);
      cdprt1(sgot);
      } /* End reading with cryin */

   /* See whether the SETENV card did its thing */
   sgot = getenv("Cryitest");
   if (sgot)
      cryout(RK_P1, " Env variable Cryitest was set to: ", RK_LN1,
         sgot, RK_CCL, NULL);
   else
      cryout(RK_P1, " ***Env variable Cryitest was not set.",
         RK_LN1, NULL);
   sgot = getenv("Cryi1");
   if (sgot)
      cryout(RK_P1, " Env variable Cryi1 was set to: ", RK_LN1,
         sgot, RK_CCL, NULL);
   else
      cryout(RK_P1, " ***Env variable Cryi1 was not set.",
         RK_LN1, NULL);
   sgot = getenv("Cryi2");
   if (sgot)
      cryout(RK_P1, " Env variable Cryi2 was set to: ", RK_LN1,
         sgot, RK_CCL, NULL);
   else
      cryout(RK_P1, " ***Env variable Cryi2 was not set.",
         RK_LN1, NULL);

   cryout(RK_P1, " Force a page eject to see new title.",
      RK_NEWPG, NULL);
   cryout(RK_P1, "0   ",
      RK_LN2+RK_FLUSH, NULL);
   cryicls();
   cryocls();
   return 0;
   } /* End cryitest */   


