/* (c) Copyright 1993-1998, The Rockefeller University *21113* */
/***********************************************************************
*                              icon2stim                               *
*                                                                      *
*  Program to convert the output of the openwin icon editor to the     *
*  stimulus format of a CNS SHAPE card (before mklib is run).          *
*  Allows graphical input of stimuli.                                  *
*                                                                      *
*  Usage:                                                              *
*     icon2stim <icon-file> <out-file> <shape-name> x y c|b            *
*                                                                      *
*     icon-file: input file in format as from icon editor              *
*        (ColorXPixmap for color, MonoXPixmap for black & white).      *
*     out-file: output file in format of stimulus files.               *
*     shape-name: name assigned to stimulus on SHAPE card.             *
*     x,y: dimensions of shape.  Needed because icon editor only has   *
*        a limited number of sizes (16x16,32x32,48x48,64x64,128x128).  *
*        icon2stim cuts down icon editor output to size x by y.        *
*     c|b: c produces colour SHAPE (x,y,C) and b produces black and    *
*         white SHAPE (x,y,A).  In case C, black and white are inter-  *
*         changed.  In case B, white becomes black and anything else   *
*         becomes white.  This because icon editor uses white backgnd. *
*                                                                      *
************************************************************************
*  V1A, 08/12/93, JW  - Initial version.  Handles colored icons only.  *
*  V1B, 08/13/93, JW  - Modified to do black and white.                *
*  V1C, 07/03/98, GNR - Generate backslash continuation for long       *
*                       lines, optimize a little and reformat.         *
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define MAXCOLORS  16   /* Maximum colors that can be handled */
#define MAXROWCOL 256   /* Maximum rows or columns in icon */

int main(int argc, char *argv[]) {

   FILE *fp;
   char header[12];
   int x,y,no,dum;
   char car[MAXCOLORS],dumc[MAXCOLORS],colour[MAXCOLORS][14];
   char pattern[MAXROWCOL][MAXROWCOL],lookup[256];
   int i,j,k,cc;
   unsigned int red[MAXCOLORS],green[MAXCOLORS],blue[MAXCOLORS];
   char reds[16],greens[16],blues[16];
   char rchar[MAXCOLORS],gchar[MAXCOLORS],bchar[MAXCOLORS];
   int ux,uy,uxm1,uym1;
   char outc[2];

/* Check arguments */

   if (argc != 7) {
      printf("Error: Wrong number of command line args\n");
      printf("Synopsis:  icon2stim <icon-file> <out-file> "
         "<shape-name> x y c|b\n");
      exit(1);
      }
   for (i=0; i<(int)strlen(argv[4]); i++) {
      if (!(isdigit(argv[4][i]))) {
         printf("Error: Fourth argument must be a number (x size)\n");
         exit(1);
         }
      }
   ux = atoi(argv[4]);
   uxm1 = ux - 1;
   for (i=0; i<(int)strlen(argv[5]); i++) {
      if (!(isdigit(argv[5][i]))) {
         printf("Error: Fifth argument must be a number (y size)\n");
         exit(1);
         }
      }
   uy = atoi(argv[5]);
   uym1 = uy - 1;
   if ((toupper(argv[6][0]) != 'C') && (toupper(argv[6][0]) != 'B')) {
      printf("Error: wrong code for colour/monochrome switch\n");
      exit(1);
      }

/* Read icon file */

   if ((fp = fopen(argv[1], "r")) == NULL) {
      printf("Error: Input file can't be opened\n");
      exit(1);
      }
   fgets(header, 10, fp);
   if ((strncmp(header, "! XPM2", 6)) != 0) {
      printf("Error: Input file in wrong format\n");
      exit(1);
      }
   fscanf(fp, "%d %d %d %d", &x, &y, &no, &dum);
   fgetc(fp);
   if (no > MAXCOLORS) {
      printf("Error: Too many colors, recompile icon2stim for more\n");
      exit(1);
      }
   if (x > MAXROWCOL || y > MAXROWCOL) {
      printf("Error: Too many rows or columns in input icon\n");
      exit(1);
      }
   /* Read color definitions and make little lookup table */
   memset(lookup, 0, 256);
   for (i=0; i<no; i++) {
      fscanf(fp, "%c %c %s", &car[i], &dumc[i], &colour[i]);
      fgetc(fp);
      lookup[(unsigned int)car[i]] = i;
      }
   /* Read actual icon pattern */
   for (i=0; i<y; i++) {
      for(j=0; j<x; j++) {
         pattern[i][j] = fgetc(fp);
         }
      fgetc(fp);
      }
   fclose(fp);

/* Process color information */

   if ((toupper(argv[6][0]) == 'B') && (no != 2)) {
      printf("Error: Wrong input file format for monochrome\n");
      exit(1);
      }
   for (i=0; i<no; i++) {
      strcpy(reds, "0x");
      strcpy(greens, "0x");
      strcpy(blues, "0x");
      for (j=0; j<4; j++) {
         reds[2+j] = colour[i][1+j];
         greens[2+j] = colour[i][5+j];
         blues[2+j] = colour[i][9+j];
         }
      reds[2+j] = '\0';
      greens[2+j] = '\0';
      blues[2+j] = '\0';
      sscanf(reds, "%i", &red[i]);
      sscanf(greens, "%i", &green[i]);
      sscanf(blues, "%i", &blue[i]);
      /* Dealing with peculiaritis of sscanf */
      red[i] &= 0xffff;
      green[i] &= 0xffff;
      blue[i] &= 0xffff;
      /* Interchange black and white */
      if ((red[i] | green[i] | blue[i]) == 0)
         red[i] = green[i] = blue[i] = 0xffff;
      else if ((red[i] & green[i] & blue[i]) == 0xffff)
         red[i] = green[i] = blue[i] = 0;
      /* Scale colors to number of bits used by CNS */
      red[i] = red[i] >> 13;
      green[i] = green[i] >> 13;
      blue[i] = blue[i] >> 14;
      rchar[i] = '0' + red[i];
      gchar[i] = '0' + green[i];
      bchar[i] = '0' + blue[i];
      }

/* Write output file */

   if ((fp = fopen(argv[2], "w")) == NULL) {
      printf("Error: Can't open output file\n");
      exit(1);
      }
   if (ux > x || uy > y) {
      printf("Error: User defined box bigger than in input file\n");
      exit(1);
      }

/* Write colored SHAPE */

   if (toupper(argv[6][0]) == 'C') {

      fprintf(fp, "SHAPE %s 1 (%d,%d,C),\n", argv[3], ux, uy);
      fprintf(fp, "    ");
      cc = 4;

      for (i=0; i<uy; i++) {
         for (j=0; j<ux; j++) {
            k = lookup[(unsigned int)pattern[i][j]];
            fputc(bchar[k], fp);
            fputc(gchar[k], fp);
            fputc(rchar[k], fp);
            if (j == uxm1) {
               if (i == uym1)
                  fputc('\n', fp);
               else {
                  fprintf(fp, ",\n    ");
                  cc = 4; }
               }
            else if (cc >= 64) {
               fprintf(fp, ",\n    ");
               cc = 4; }
            else {
               fputc(',', fp);
               cc += 4; }
            } /* End x loop */
         } /* End y loop */
      } /* End colored output */

/* Write black and white SHAPE */

   else {

      /*  Checking of colors has been liberalized such that black plus
      *  anything else can be used to generate a monochrome shape.  */
      if ((red[0] | green[0] | blue[0]) == 0)
         outc[0] = '0', outc[1] = 'F';
      else if ((red[1] | green[1] | blue[1]) == 0)
         outc[1] = '0', outc[0] = 'F';
      else {
         printf("Error:  Input colors not monochrome pair.\n");
         exit(1);
         }
      fprintf(fp, "SHAPE %s 1 (%d,%d,A),\n", argv[3], ux, uy);
      fprintf(fp, "    ");
      cc = 4;

      for (i=0; i<uy; i++) {
         for (j=0; j<ux; j++) {
            k = lookup[(unsigned int)pattern[i][j]];
            fputc(outc[k], fp);
            if (j == uxm1) {
               if (i == uym1)
                  fputc('\n', fp);
               else {
                  fprintf(fp, ",\n    ");
                  cc = 4; }
               }
            else if (cc >= 67) {
               fprintf(fp, "\\\n    ");
               cc = 4; }
            else
               cc += 1;
            } /* End x loop */
         } /* End y loop */
      } /* End monochrome output */
   } /* End icon2stim main program */

