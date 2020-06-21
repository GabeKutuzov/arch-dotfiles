/* (c) Copyright 1988-2011, The Rockefeller University *11115* */
/* $Id: envtest.c 23 2017-01-16 19:28:30Z  $ */
/***********************************************************************
*              Darwin III Environment Simulation Package               *
*                               ENVTEST                                *
*                                                                      *
*  Driver for DarwinIII environment setup program... allows            *
*  environment to run as stand-alone module                            *
************************************************************************
*  V1A, 12/01/88, GNR - Initial version                                *
*  Rev, 03/04/92, GNR - Use rf io routines                             *
*  Rev, 04/23/97, GNR - Add library file name as command-line option   *
*  Rev, 07/22/97, GNR - Add dbgmask argument to setmfd() call          *
*  Rev, 02/22/03, GNR - Change s_type to stim_type                     *
*  ==>, 02/22/03, GNR - Last mod before committing to svn repository   *
*  Rev, 10/10/11, GNR - Change ngraph call to newplt                   *
*  Rev, 10/12/11, GNR - Remove setmfd(), use setmf()                   *
***********************************************************************/

#define MAIN

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"

#include "env.h"
#include "rocks.h"
#include "rkxtra.h"
#include "plots.h"

#define EPIX_INITX   6           /* Value of kx for test */
#define EPIX_INITY   6
#define ETEST_MXOBJ 32
#define ETEST_MXREF 32

int main (int argc, char *argv[]) {

   ENVIRONMENT *Eptr;            /* Ptr to current environment */
   struct RFdef *shapelib;       /* Ptr to shape lib file record */
   char *card;
   char *display;                /* Ptr to name of display to use */
   stim_type *inpary;
   char stitle[44];
   long ncyc;                    /* Number of cycles env runs */
   long series = 0, trial = 0;
   long llx = 1<<EPIX_INITX;
   long lly = 1<<EPIX_INITY;
   int ret;

   if (argc != 2) {
      cryout(RK_P1, "0***Synopsis: envtest <library-file>",
         RK_LN2, NULL);
      abexit(1);
      }
   if (!(display = getenv("DISPLAY"))) {
      cryout(RK_P1, "0***DISPLAY not found in environment.",
         RK_LN2, NULL);
      abexit(2);
      }
   sprmpt("env>");
   shapelib = rfallo(argv[1], READ, BINARY, DIRECT,
      TOP, NO_LOOKAHEAD, REWIND, RELEASE_BUFF, IGNORE, IGNORE,
      IGNORE, ABORT);
   Eptr = envset(shapelib, NULL, NULL, NULL, NULL, EPIX_INITX,
      EPIX_INITY, ETEST_MXOBJ, ETEST_MXREF, ENV_COLOR, ENV_SAVOBJ);
   if (!Eptr) {
      cryout(RK_P1, "0***Unable to initialize environemnt.",
         RK_LN2, NULL);
      abexit(3);
      }
   if (envopt(Eptr, 0.625, -1.0, 16, 0, -1, 12, 10, 4, NULL, "RGB",
         ENV_SQUARE, TRUE, FALSE, FALSE, TRUE) < 0) {
      cryout(RK_P1, "0***Error setting environment options.",
         RK_LN2, NULL);
      abexit(4);
      }

   setmf(NULL, display, "envtest", NULL, 2048, 0, 'B', 0, 0);

   inpary = (stim_type *) callocv(1, 2*llx*lly, "ENV: inpary");

/* Process environment cards until END card */

   while ((card = cryin())) {    /* Assignment intended */
      if (!ssmatch(card,"CYCLE",4)) {
         cdprnt(card);
         envctl(Eptr,card); }
      else {
         inform("(sw1il)",&ncyc,NULL);
         envser(Eptr);
         ++series;
         for (trial=1; trial<=ncyc; trial++) {
            if ((ret=envcyc(Eptr, inpary, NULL)) < 0) {
               endplt(); return ret; }
            if (newplt(11.0, 11.0, 0, 0, "SOLID", "RED", "ALLWHITE", 0))
               goto BreakOut;
            sconvrt(stitle, "(8HSeries J0IL8,8H  Trial J0IL8)",
               &series, &trial, NULL);
            if ((ret=envplot(Eptr, inpary, 1.0, 1.0, 8.0, 8.0, stitle))
               != 0) { endplt(); return ret; }
            } /* End trial for */
         }  /* End else */
      } /* End while cryin */
BreakOut:
   endplt();
   return ENV_NORMAL;

   } /* End main (envtest) */
