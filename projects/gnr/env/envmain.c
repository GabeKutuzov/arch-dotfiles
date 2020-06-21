/* (c) Copyright 1988-2011, The Rockefeller University *11115* */
/* $Id: envmain.c 23 2017-01-16 19:28:30Z  $ */
/***********************************************************************
*              Darwin III Environment Simulation Package               *
*                               ENVMAIN                                *
*                                                                      *
*  Driver for DarwinIII environment setup program... allows            *
*  environment to run as stand-alone module                            *
************************************************************************
*  V1A, 12/01/88, GNR - Initial version                                *
*  Rev, 03/04/92, GNR - Use rf io routines                             *
*  ==>, 04/16/97, GNR - Last mod before committing to svn repository   *
*  Rev, 10/10/11, GNR - Change ngraph call to newplt                   *
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
#define ETEST_NGR   32
#define ETEST_MXOBJ 32
#define ETEST_MXREF 32

int env_drive() {

   ENVIRONMENT *Eptr;            /* Ptr to current environment */
   struct RFdef *shapelib;       /* Ptr to shape lib file record */
   char *card;
   byte *inpary;
   long ncyc;                    /* Number of cycles env runs */
   long series = 1, trial = 1;
   long llx = 1<<EPIX_INITX;
   long lly = 1<<EPIX_INITY;
   int ret;
   int ic;                       /* Loop counter */

      shapelib = rfallo("/tmp/shapes.in", READ, BINARY, DIRECT,
         TOP, NO_LOOKAHEAD, REWIND, RELEASE_BUFF, IGNORE, IGNORE,
         IGNORE, ABORT);
      Eptr = envset(shapelib, NULL, NULL, NULL, NULL, EPIX_INITX,
         EPIX_INITY, ETEST_NGR, ETEST_MXOBJ, ETEST_MXREF, ENV_COLOR,
         ENV_SAVOBJ);
      envopt(Eptr, -1.0, -1.0, 16, -1, -1, 12, 10, 4, NULL, NULL,
         NULL, NULL, ENV_SQUARE, FALSE, FALSE, FALSE, TRUE);

      inpary = (byte *) callocv(1, llx*lly, "ENV: inpary");

/* Process environment cards */

   while ((card = cryin())) {    /* Assignment intended */
      if (!ssmatch(card,"CYCLE",4)) {
         cdprnt(card);
         envctl(Eptr,card); }
      else {
         inform("(sw1il)",&ncyc,NULL);
         for (ic=0; ic<ncyc; ic++) {
            if ((ret=envcyc(Eptr, inpary, NULL)) != 0) return ret;
            newplt(11.0, 11.0, 0, 0, "SOLID", "RED", "ALLWHITE", 0);
            if ((ret=envplot(Eptr, inpary, 0.0, 0.0, 8.0, 8.0,
               series, trial++)) != 0) return ret;
            } /* End for ic */
         }  /* End else */
      } /* End while cryin */
   endplt();
   return ENV_NORMAL;

   } /* End envmain */
