/* (c) Copyright 2011, The Rockefeller University *11113* */
/***********************************************************************
*                             ShowMovie.c                              *
*                                                                      *
*  To workaround the fact that movie timing does not currently work    *
*  in mfdraw, this program reads a movie metafile and sends the        *
*  frames at timed intervals to mfdraw via a socket interface.         *
************************************************************************
*  V1A, 11/08/11, GNR                                                  *
***********************************************************************/


#define MAIN

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "plots.h"
#include "/home/cdr/src/oldplot/glu.h"
#include "/home/cdr/src/oldplot/mfio.h"

/* (Ideally, MFLSIZE is near an integer fraction of the default
*  metafile buffer size (4092) but long enough to hold the
*  longest possible newplt record.)  */
#define MFLSIZE   510         /* Max size of one read */
#define TTLSIZE    60         /* Size of a standard title */
#define TSTSIZE    12         /* Size of the time stamp */
#define DFLTSCL     3         /* Default floating-point scale */

/* Globals for number parsing */
float tab[] = {1.0,10.0,100.0,1000.0,10000.0,100000.0};
int ba;

/*=====================================================================*
*                                 a2f                                  *
*         Metafile-coded ASCII floating-point number to float.         *
*         Taken from mfxpar.c in /src/mfdraw by Marx,                  *
*=====================================================================*/

static float a2fx(int scale, char *buf) {
  int cc;
  float sign =  1.0F;
  float value = 0.0F;
  ba = 0;
  if (*buf == '-') {
    buf++;
    ba++;
    sign = -1.0F;
  }
  for (;;) {
    cc = tolower(*buf++);
    ba++;
    if (cc >= 'a') {
      value = sign * (value*10.0F + (float)(cc - 'a'))/tab[scale];
      break;
    }
    value = value*10.0F + (float)(cc - '0');
  }
  return value;
} /* End a2fx() */

/*=====================================================================*
*                                 a2i                                  *
*                 Metafile-coded ASCII integer to int                  *
*                 Taken from mfxpar.c in /src/mfdraw by Marx,          *
*=====================================================================*/

static int a2ix(char *buf) {
  int cc;
  int sign =  1;
  int value = 0;
  ba = 0;
  if (*buf == '-') {
    buf++;
    ba++;
    sign = -1;
  }
  for (;;) {
    cc = tolower(*buf++);
    ba++;
    if (cc >= 'a') {
      value = sign * (value*10 + (cc - 'a'));
      break;
    }
    value = value*10 + (cc - '0');
  }
  return value;
} /* End a2ix() */

/*=====================================================================*
*                                MAIN                                  *
*=====================================================================*/

int main(int argc, char *argv[]) {

   FILE *fd;                  /* Input file descriptor */
   char line[MFLSIZE+2];      /* Room for one metafile line */
   char titl[TTLSIZE+2];      /* Copy of the title */
   int  delay;                /* Time delay (msec) */
   int  qcont;                /* True if in a continuation */
   int  sdel,udel;            /* Second, microseconds delay */

   if (argc != 3) {
      printf("Synopsis:  ShowMovie <metafile-name> "
         "<frame time (msec)>\n");
      exit(1);
      }

   if (!(fd = fopen(argv[1],"r")))
      abexitm(590, "File open error");

   delay = atoi(argv[2]);
   qcont = FALSE;
   sdel  = delay / 1000;
   udel  = 1000 * (delay % 1000);

/* Read the header to get the title, etc. */

   if (!fgets(line, MFLSIZE, fd))
      abexitm(591, "Unable to read header 1");
   if (memcmp(line, "PLOTDATA", 8))
      abexitm(592, "The specified file does not have "
         "a proper metafile header");

   if (!fgets(line, TTLSIZE+2, fd))
      abexitm(591, "Unable to read title header");
   cryout(RK_P2,"0Metafile title: ", RK_LN2, line, TTLSIZE, NULL);
   strncpy(titl, line, TTLSIZE);
   titl[TTLSIZE] = '\0';

   if (!fgets(line, TSTSIZE+2, fd))
      abexitm(591, "Unable to read timestamp");
   cryout(RK_P2," Timestamp ", RK_LN1, line, TSTSIZE, NULL);

/* Start up the mfdraw machinery */

   settit("Program to display an metafile as a movie");
   setmf(NULL, "", titl, NULL, 0, 0, 'A', 0, 10);
   setmovie(1, 0, 0);

/* Read and send */

   while (fgets(line, MFLSIZE+1, fd)) {
      char *pl = line;
      int  ll;                /* Length of command */

      /* Check for a new frame */
      if (!qcont && line[0] == '[') {
         float xsiz,ysiz,xorg,yorg;
         int   index,nc,rc;

         /* Allow the specified delay -- this is a quick-and-dirty
         *  program, so there is no attempt to subtrace the time
         *  already used parsing and drawing the last plot.  */
         rksleep(sdel, udel);

         /* We gotta parse this in order to call newplt() */
         pl += 1;
         index = a2ix(pl); pl += ba;
         xsiz = a2fx(DFLTSCL, pl); pl += ba;
         ysiz = a2fx(DFLTSCL, pl); pl += ba;
         xorg = a2fx(DFLTSCL, pl); pl += ba;
         yorg = a2fx(DFLTSCL, pl); pl += ba;
         nc = a2ix(pl); pl += ba;
         pl[nc] = '\0';
         rc = newplt(xsiz, ysiz, xorg, yorg, "DEFAULT", "GREEN",
            pl, SKP_META);
         if (rc) abexit(101);
         }

      else {
         /* Regular plot command --
         *  Check whether it is complete in this read (if not,
         *  do not look for a newplt record in the next read).
         *  Either way, can just copy to output */
         qcont = strchr(line, '\n') == NULL;
         ll = strnlen(line, MFLSIZE);
         mfbchk(ll);
         strcpy(_RKG.MFCurrPos, line);
         _RKG.MFCurrPos += ll;
         }

      } /* End while */

   cryout(RK_P2, " End ShowMovie", RK_LN1+RK_FLUSH, NULL);

   return 0;
   } /* End ShowMovie */
