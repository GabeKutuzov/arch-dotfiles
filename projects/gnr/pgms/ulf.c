/* (c) Copyright 1992, The Rockefeller University *11112* */
/*---------------------------------------------------------------------*
*                                ulf.c                                 *
*                                                                      *
*  Underline filter.  This program is designed to filter the output of *
*  CNS to remove the underlines so it will be more readable on a CRT.  *
*  It has one command line argument, which is the name of the file to  *
*  be filtered.  The output goes to stdout.                            *
*  V1A, 02/25/92, GNR                                                  *
*---------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#define MaxLine 399           /* Max line length = 3 printer lines */

int main(int argc, char *argv[]) {

   FILE *fd;                  /* Input file descriptor */
   char line[MaxLine+1];      /* Plenty of room for 3 lines */
   char *pl;                  /* Line pointer */
   int icr;                   /* Location of carriage return */

   if (argc>2) {
      printf("Give input file as only command-line argument\n");
      printf("Or no command-line arguments to process stdin\n");
      exit(1);
      }

   if (argc==1)
      fd = (stdin);
   else {
      if (!(fd = fopen(argv[1],"r"))) {
         printf("File open error\n");
         exit(2);
         }
      }

   while (fgets(line,MaxLine,fd)) {
      /* Find a bare carriage return */
      for (icr=0,pl=line; *pl; ++pl)
         if (*pl == '\r') {
            icr = pl - line + 1;
            break;
            }
      /* Omit the newline, which puts adds */
      for (pl=line; *pl; ++pl) if (*pl == '\n') *pl = '\0';
      /* Output the truncated string */
      puts(line+icr);
      } /* End while */

   return 0;
   } /* End ulf */
