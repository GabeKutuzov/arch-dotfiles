/***********************************************************************
*                              phantom.c                               *
*                                                                      *
*     This program reads a control file containing a description of    *
*  an image lattice and a set of objects to be placed in that lattice. *
*  It creates the phantom image and writes it out as an acrNEMA image. *
*  The control file is read from stdin using ROCKS library routines.   *
*                                                                      *
*     The commands accepted in the control file are documented in the  *
*  file 'phantom.doc'                                                  *
*                                                                      *
*  Command line arguments:                                             *
*     (Currently none)                                                 *
*                                                                      *
*  V1A, 01/20/95, GNR - Initial version                                *
***********************************************************************/

#define VERSION   "PHANTOM V1A"  /* Current version of this pgm */
#define IDIM      3              /* Dimensionality of image */

#define MAIN
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "phantom.h"
#include "NEMA.h"
#include "NEMA.HDR"

struct phglob *PH;               /* Global data structure */

/*=====================================================================*
*               Functions used by phantom main program                 *
*=====================================================================*/

/*---------------------------------------------------------------------*
*                                fatal                                 *
*                                                                      *
*  Generate fatal error message and terminate                          *
*  Argument:                                                           *
*     txt      text string to be printed                               *
*     num      error code to be returned                               *
*---------------------------------------------------------------------*/

void fatal(char *txt, int num) {

   cryout(RK_P1,"0***",RK_LN2+4,txt,RK_CCL,NULL);
   exit(num);
   } /* End fatal */


/*=====================================================================*
*                        phantom main program                          *
*=====================================================================*/

int main(int argc, char *argv[]) {

   struct phglob *pH;            /* Local copy of PH */
   int nfd;                      /* Output file descriptor */

/* Generate error if unrecognized command-line arguments */

   if (argc > 1) {
      fatal("Unrecognized command-line arguments",1);
      }

/* Allocate and initialize PH global parameter block */

   pH = PH = (struct phglob *)callocv(1,sizeof(struct phglob),
      "Global parameter struct");
   pH->inst = "The Rockefeller University";
   pH->dept = "Laboratory of Biological Modelling";

/*---------------------------------------------------------------------*
*                    Read and check control cards                      *
*---------------------------------------------------------------------*/

/* Read control cards */

   if (phrdcc())
      fatal("Error in control file.",2);

/* Check for required control file input */

   if (phckin())
      fatal("Error in input data.",3);

/*---------------------------------------------------------------------*
*                         Prepare image file                           *
*---------------------------------------------------------------------*/

/* Write acrNEMA header */

   if ((nfd = open(pH->fnm, O_WRONLY+O_CREAT+O_TRUNC,
         S_IRUSR+S_IWUSR+S_IRGRP)) < 0)
      fatal("Unable to open output file.",4);

   if (makeNEMA(nfd,VERSION,pH->inst,pH->dept,pH->note,pH->study,
         pH->series,IDIM,0,pH->hiden,pH->cols,pH->rows,
         pH->secs,pH->Vox[X],pH->Vox[Y],pH->Vox[Z]))
      fatal("Error writing acrNEMA header.",5);

/* Generate image */

   if (phwimg(nfd))
      fatal("Error generating phantom image.",6);

   close(nfd);
   return 0;
   } /* End phantom main */
