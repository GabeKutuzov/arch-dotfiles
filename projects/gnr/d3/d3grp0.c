/* (c) Copyright 1990-2007, The Rockefeller University *21116* */
/* $Id: d3grp0.c 70 2017-01-16 19:27:55Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3grp0                                 *
*              Read and interpret Group Zero control cards             *
*                                                                      *
*  To add a new file type to CNS, do the following:                    *
*     (1) Edit d3fdef.h to add a defined name for the new file type    *
*  (e.g. MYFILETYPE).  This name will be used as a subscript in the    *
*  d3file array to the linked list that will be created for the new    *
*  file type. Increment the total number of file types, FILETYPE_NUM.  *
*  Note how conditional compilation is handled for these definitions.  *
*     (2) Add the name for the control card for the new file type to   *
*  the array cc0 in d3grp0.c at cc0[MYFILETYPE].                       *
*     (3) Add new entries in the faa, fablksi, and farecl tables below *
*  to give the rfallo arguments (the second through eight in order),   *
*  the rfallo blksize, and the rfallo record length arguments,         *
*  respectively, that are to be used for the new file type.            *
*     (4) Add a new case to the switch in the code below if needed to  *
*  handle any special actions needed when the new control card is      *
*  processed, for example, setting bits to turn on some program        *
*  option, saving the file count, etc.                                 *
*     (5) To access the files, use d3file[MYFILETYPE] to get at the    *
*  linked list created here.                                           *
************************************************************************
*  Rev, 08/07/90, JMS - Code added for overlays                        *
*  Rev, 10/10/90, GNR - Replace MAX_FILNM with LFILNM from sysdef.h    *
*  Rev, 01/12/90, GNR - Add SAVE card to force exit to group I.        *
*  Rev, 09/24/91, GNR - Add DIGINPUT card, handle all files same way.  *
*  Rev, 03/05/92, GNR - Use rf io routines, add block sizes            *
*  Rev, 06/10/92, GNR - Add METAFILE card and BUFL parameter.          *
*  V8A, 12/01/96, GNR - Remove support for overlays                    *
*  Rev, 09/04/97, GNR - Ignore whitespace cards                        *
*  Rev, 10/31/97, GNR - Increase default METAFILE blksize (512-->2048) *
*  Rev, 04/14/01, GNR - GRAFDATA becomes just a synonym for SIMDATA    *
*  V8B, 10/07/01, GNR - Add UPROGLIB, increase default blocksizes      *
*  V8D, 01/27/07, GNR - Look to next group if unrecognized card        *
*  ==>, 11/25/07, GNR - Last mod before committing to svn repository   *
*  R67, 04/27/16, GNR - Remove Darwin 1 support                        *
***********************************************************************/

/* Note: The current version of this code makes linked lists of all
   filetypes, even those which can only have one instance.  Since
   each control card is most likely to occur only once, we simply
   scan for the end of an existing linked list rather than maintain
   end-of-list pointers.  Normally, the scan will find the list empty.
   If more than one file is entered but only one can be used, the
   program will silently ignore the supernumeraries. */

/* Note: When a file contains variable-length records, LRECL is set
   arbitrarily to equal the blocksize.  The IBM implementation will
   have to be reviewed for compatibility with BDAM if needed. */

/* Note: When a group I control card name is an initial substring
   of a group 0 control card name, that card should be entered here
   and should produce a normal exit when recognized.  This prevents
   the group I card from being accepted as an abbreviation of the
   group 0 card.  Thus, SAVE is otherwise confused with SAVENET. */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "rocks.h"
#include "rkxtra.h"
#include "d3fdef.h"

extern char **getcc1(int *pncc1);

int d3grp0() {

   struct RFdef **pfd;     /* Ptr to ptr to new RFdef struct */
   char **cc1;             /* Ptr to Group 1 control card keys */
   int fcnt;               /* Count of files of a particular type */
   int jft;                /* File type index */
   int ncc1;               /* Number of Group 1 control card keys */
   int smret;              /* Match value of control card */
   char fname[LFILNM+1];   /* Holding area for file names */

/*--------------------------------------------------------------------*/
/*               Definitions of group 0 control cards                 */
/*====================================================================*/
/*  N.B.! The following card names must be in the same order as the   */
/*  filetype names defined in d3fdef.h, followed by any synonyms,     */
/*  then any dummies added to avoid confusion with group I per the    */
/*  above comment.  (The match value is used to index into the        */
/*  d3file, faa, and farecl arrays.)                                  */
/*====================================================================*/

   static char *cc0[] = { "RESTORE",  "SAVENET",  "FDMATRIX",
                          "CONNLIST", "SIMDATA",  "METAFILE",
                          "UPROGLIB", "REPLAY",   "PIXEL",
                          "LIBRARY",
                        /* Following cards are synonyms for above */
                          "GRAFDATA",
                        /* Following cards are NOT file types */
                          "SAVE" };

/* The following tables define the rfallo arguments and record
*  lengths for each type of file.  Again, the order must be kept
*  in step with the filetype names in d3fdef.h .  */

   static schr faa[][7] = {
      /* RESTORE file options */
      {READ,BINARY,DIRECT,TOP,LOOKAHEAD,REWIND,RELEASE_BUFF },
      /* SAVENET file options */
      {WRITE,BINARY,SEQUENTIAL,TOP,LOOKAHEAD,NO_REWIND,RELEASE_BUFF },
      /* FDMATRIX file options */
      {READ,TEXT,SEQUENTIAL,TOP,LOOKAHEAD,NO_REWIND,RELEASE_BUFF },
      /* CONNLIST file options */
      {READ,BINARY,SEQUENTIAL,TOP,LOOKAHEAD,NO_REWIND,RELEASE_BUFF },
      /* SIMDATA file options */
      {WRITE,BINARY,SEQUENTIAL,TOP,LOOKAHEAD,REWIND,RELEASE_BUFF },
      /* METAFILE file options */
      {WRITE,TEXT,SEQUENTIAL,TOP,LOOKAHEAD,REWIND,RELEASE_BUFF },
      /* UPROGLIB file options (rfopen not used for these) */
      {READ,BINARY,DIRECT,TOP,NO_LOOKAHEAD,NO_REWIND,RELEASE_BUFF },
      /* REPLAY file options */
      {READ,BINARY,SEQUENTIAL,TOP,LOOKAHEAD,NO_REWIND,RELEASE_BUFF },
      /* PIXSTIM file options */
      {READ,BINARY,DIRECT,TOP,LOOKAHEAD,REWIND,RELEASE_BUFF },
      /* SHAPELIB file options */
      {READ,BINARY,DIRECT,TOP,NO_LOOKAHEAD,REWIND,RELEASE_BUFF },
      };

   static unsigned short fablksi[] = {
      4096,4096,1024,4096,4096,4096,32768,4096,1024,1024,
      };

   static unsigned short farecl[] = {
      4096,4096,80,8,4096,4096,32768,4096,1024,1024,
      };

/* Read and interpret Group Zero control cards */

   while (cryin()) {
      smret = smatch(RK_NMERR, RK.last, cc0,
         sizeof(cc0)/sizeof(char *));
      if (smret == FILETYPE_NUM+1) smret = SIMDATA + 1;
      if (smret <= 0 || smret > FILETYPE_NUM) {
         /* If not in Group 0 or Group 1, give error and
         *  read next one here, otherwise go to Group 1.  */
         cc1 = getcc1(&ncc1);
         if (smatch(0, RK.last, cc1, ncc1)) break;
         else continue;
         }
      jft = smret - 1;        /* Make file type index */
      cdprt1(RK.last);        /* Print the card (if oprty <= 3) */

      /* Skip out to the end of any existing file list */
      fcnt = 0;
      for (pfd = &d3file[jft]; *pfd; pfd = &(*pfd)->nf) fcnt++;

/* Scan file names and create a RFdef block for each using file
*  parameters from the above tables.  No default case is needed
*  because the 'while' stmt above filters out unrecognized cards.
*  Note that rfallo does two plain malloc's--one for the RFdef
*  block and one for the name. */

      cdscan(RK.last, 1, LFILNM, RK_WDSKIP);
      while (!(scan(fname, RK_SCANFNM) & RK_ENDCARD)) {
         if (RK.scancode == RK_EQUALS) {
            /* Equals sign found--better be BUFL */
            if (!ssmatch(fname,"BUFL",1)) ermark(RK_IDERR);
            else eqscan(fablksi+jft,"UH",0);
            }
         else {
            *pfd = rfallo(fname,faa[jft][0],faa[jft][1],faa[jft][2],
               faa[jft][3],faa[jft][4],faa[jft][5],faa[jft][6],
               (long)fablksi[jft],(long)farecl[jft],IGNORE,ABORT);
            fcnt++;              /* Count files of current type */
            pfd = &(*pfd)->nf;   /* Step through linked list */
            } /* End reading file name */
         } /* End scanning current group 0 card */

/* Error if not at least one file name found */

      if (!fcnt)
         cryout(RK_E1,"0***AT LEAST ONE FILE NAME IS REQUIRED.",
            RK_LN2,NULL);

/* Here perform any clean up or flag setting that is once-only,
*  but specific to the particular file type being processed.  */

      switch (jft) {

case SAVREP_IN:               /* RESTORE card */
         RP->ksv |= KSVRR;
         break;

case FDMATRIX:                /* FDMATRIX card */
         RP->nfdh = fcnt;
         break;

default:                      /* All others */
         ;
         } /* End postprocessing file type switch */

      } /* End while CRYIN */

   rdagn();
   return RK.iexit;
   } /* End d3grp0 */

