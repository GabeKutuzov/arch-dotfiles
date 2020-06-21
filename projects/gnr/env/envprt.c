/* (c) Copyright 2003-2018, The Rockefeller University *11116* */
/* $Id: envprt.c 25 2018-05-07 22:13:48Z  $ */
/***********************************************************************
*              Darwin III Environment Simulation Package               *
*                               ENVPRT                                 *
*                Print the contents of an input array                  *
*                                                                      *
*  Synopsis:   void envprt(struct ENVDEF *Eptr, stim_type *ia,         *
*                 int ksprt)                                           *
*  Argument:   'Eptr' is a ptr to the ENVDEF that defines the          *
*                 environment to be printed.                           *
*              'ia' is a ptr to the input array (retina) whose         *
*                 data are to be printed                               *
*              'ksprt' is a switch word contining binary options.      *
*                 Values defined at present are:                       *
*                 ENV_PRSPT (4) this output is sent to SPOUT.          *
*                 ENV_PRNOI (8) noise-contaminated IA is printed.      *
*  Notes:      This routine will mainly be used for debugging, so it   *
*              is kept simple.  No attempt is made to indicate arm     *
*              and window positions, but colored input is shown.       *
************************************************************************
*  V8C, 11/01/03, GNR - New routine, based on bgraph.ftn from 1980     *
*                       FORTRAN version of CNS.                        *
*  ==>, 11/01/03, GNR - Last mod before committing to svn repository   *
*  V8D, 06/26/08, GNR - Add ENV_PRNOI flag                             *
*  V8H, 12/01/10, GNR - Separate env.h,envglob.h w/ECOLMAXCH change    *
*  R25, 04/01/18, GNR - Convert ubcdwt call to wbcdwt                  *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "envglob.h"
#include "rocks.h"
#include "rkxtra.h"

#define NUM_RANGES   16       /* Number of value ranges */
#define RANGE_SHIFT   4       /* Shift to get NUM_RANGES from a byte */
#define OCTAL_COLS    4       /* Number of columns per octal output */
#define MAX_OPCOLS  128       /* Maximum output columns */
#if MAX_OPCOLS > LNSIZE
#error Max output columns in envprt is larger than LNSIZE
#endif

void envprt(struct ENVDEF *Eptr, stim_type *ia, int ksprt) {

   stim_type *ps,*pr,*pc;     /* Ptr to strip, row, column */
   int ivs,irow,icol;         /* Strip, row, column counter */
   int kms;                   /* TRUE if multiple strips needed */
   int ksp;                   /* SPOUT switch */
   int lcol;                  /* Length of data in an ia column */
   int lstr;                  /* Length of data in a strip */
   int ncn;                   /* Number of columns in current strip */
   int ncr;                   /* Number of columns remaining */
   int ncs;                   /* Number of columns per verical strip */
   int nrw;                   /* Number of rows in input array */

   /* Characters to be printed for each of 16 ranges of mono ia */
   static char mopc[NUM_RANGES] = "- .+0123456789HX";
   char line[LNSIZE+1];    /* Output text assembly */

   ksp = (ksprt & ENV_PRSPT) != 0;
   ncs = Eptr->kcolor ? (MAX_OPCOLS/4) : MAX_OPCOLS;
   ncr = Eptr->lx;
   nrw = Eptr->ly;
   kms = ncr > ncs;
   lcol = ncr + ncr;
   lstr = lcol*ncs;

   nopage(RK_PGLONG);         /* Omit pagination for the duration */

/* Loop over vertical strips and print identification for each */

   ps = ia + ((ksprt & ENV_PRNOI) ? nrw : 0);
   ivs = 0;
   while (ncr) {
      ncn = min(ncr,ncs);
      strcpy(line, "0Input array");
      if (kms) {
         ivs += 1;
         sconvrt(line, "(T13,8H (Strip ,J0I4,H))", &ivs, NULL); }
      tlines(nrw+3);
      if (ksp) spout(nrw+1);
      cryout(RK_P4, line, RK_LN3, " ", RK_LN0, NULL);

/* Loop over rows, then columns */

      for (irow=0,pr=ps; irow<nrw; ++irow,++pr) {

         if (Eptr->kcolor) {
            /* Colored input--print octal values */
            int ncno = OCTAL_COLS*ncn;
            for (icol=0,pc=pr; icol<ncno; icol+=OCTAL_COLS,pc+=lcol) {
               line[icol] = ' ';
               wbcdwt(pc, line+icol+1,
                  RK_OCTF|RK_NUNS|RK_NPAD0|RK_NBYTE|OCTAL_COLS-2);
               } /* End loop over columns */
            } /* End colored output */
         else {
            /* Monochrome input--print symbol values */
            for (icol=0,pc=pr; icol<ncn; ++icol,pc+=lcol)
               line[icol] = mopc[*pc>>RANGE_SHIFT];
            } /* End monochrome output */

         cryout(RK_P4, " ", RK_LN1+1, line, icol, NULL);
         } /* End loop over rows */

      ps += lstr;
      ncr = ncr - ncn;
      } /* End loop over vertical strips */

   nopage(RK_PGNORM);         /* Resume normal pagination */
   } /* End envprt() */

