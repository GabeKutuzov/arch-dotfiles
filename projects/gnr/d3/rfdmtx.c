/* (c) Copyright 1989-2016, The Rockefeller University *21115* */
/* $Id: rfdmtx.c 70 2017-01-16 19:27:55Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               rfdmtx                                 *
*                                                                      *
*       Read feature detection matrices                                *
*             Call: rfdmtx()                                           *
*             All variables are referenced via RP block                *
*                                                                      *
*       Functions performed by this subroutine:                        *
*       1) Allocates space for nfdh matrix headers at RP->pfdh         *
*       2) Reads nfdh sets of matrices from successive files           *
*             on unit FDMATRIX, storing header blocks and matrices     *
*                                                                      *
*       Matrices are stored with (BITSPERUI32/NBPMCW) = 8 coeffs       *
*           packed into one word                                       *
*       Each coeff has a sign bit and 3 fraction bits, with negs       *
*           stored as two's complements, as in Darwin III Cij's.       *
*       Partial words are packed right with zeros to fill word         *
*       Each matrix begins a new word in storage                       *
*                                                                      *
************************************************************************
*  Version 1A, 11/13/85, G.N. Reeke                                    *
*  V4A, 03/08/89, Translated to 'C' from version V3A                   *
*  Rev, 11/27/89, JMS - Fixed bug in repositioning 'rc' ptr when       *
*                       done processing one word's worth of numbers.   *
*  Rev, 05/23/90, JMS - ifdef'd out final check for eof at bottom      *
*                       since eof isn't recognized by fgets in some    *
*                       cases - assembly level trace showed problem is *
*                       below fgets.  Should use feof instead of fgets *
*                       anyway. Code now depends on correct number of  *
*                       matrices and matrix dimensions in header.      *
*  Rev, 10/09/91, GNR - Use standard d3file layout for files.          *
*  Rev, 03/06/92, GNR - Make compatible with RFdefs.                   *
*  Rev, 09/21/98, GNR - Use rfgets instead of fgets.                   *
*  V8B, 12/09/00, GNR - Move to new memory management routines         *
*  V8D, 09/04/06, GNR - Parametrize bits per matrix element as NBPMCE  *
*  ==>, 09/04/06, GNR - Last mod before committing to svn repository   *
*  R66, 02/24/16, GNR - longs to ui32 or int for 64-bit compilation.   *
*                       Modified packing routine to remove requirement *
*                       that no matrix can extend over a card boundary.*
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "d3fdef.h"
#include "rocks.h"

#ifdef PARn
#error This routine is not used on comp nodes
#else

#define NMCEPW (BITSPERUI32/NBPMCE) /* Matrix elements per fdm_type */
#define FIRST_DATA_COLUMN 8
#define LAST_DATA_COLUMN 72
#define CROFE  (RK_P1 + (FILE_ERR << 12))

void rfdmtx(void) {

   struct FDHEADER *rh;    /* Pointer to current header block */
   struct RFdef *fdfile,   /* Pointer to file list */
      *nfdfile;            /* Pointer to next file while freeing */
   char card[CDSIZE+2];    /* Location into which cards are read */
   char *pcd,*pcde;        /* Pointer to card data */
   fdm_type *pm;           /* Pointer to current matrix word */
   long badct;             /* Number of bad characters */
   size_t nfdws;           /* Words for npat patterns */
   int cfct;               /* Element counter */
   int nmels;              /* Number of elements in a matrix */
   int nwpm;               /* Number of fdm_type words per matrix */
   int patcnt;             /* Pattern counter */
   int pshft;              /* Packing shift */
   int setno;              /* Current set number */

/* Allocate space for the header blocks */

   RP->pfdh = rh = (struct FDHEADER *)allocpcv(Static,
      RP->nfdh, IXstr_FDHEADER, "FDHEADER structures.");
   pcde = card + LAST_DATA_COLUMN;

/* Loop over FDMATRIX files:
*     Read a card from FDMATRIX unit.  Print it if it's a comment.
*     Read mrx, mry, npat from the card using inform.
*     (Note: nfdh is guaranteed to equal length of file list
*        because counted in d3grp0.  Therefore, no checks made.)
*/

   for (setno=1,fdfile=d3file[FDMATRIX]; setno<=RP->nfdh;
         setno++,rh++) {

      /* Open a file.  Give error if unable to do so. */
      if (!rfopen(fdfile, SAME, READ, TEXT, SEQUENTIAL, TOP,
            LOOKAHEAD, NO_REWIND, RELEASE_BUFF, IGNORE, CDSIZE,
            IGNORE, NO_ABORT)) {
         cryout(CROFE, "0***UNABLE TO OPEN RFDMTX FILE ", RK_P2,
            fdfile->fname, RK_CCL, NULL);
         goto BAD_OPEN_NEXT_SET;
         }

      /* Read a line.  If it's a comment print it out and keep looking
      *  for the header.  Otherwise break out and read header.  */
      while (1) {
         if (!rfgets(fdfile, card, CDSIZE+1, ABORT)) {
            cryout(CROFE, "0***UNEXPECTED EOF ON RFDMTX FILE ",
               RK_P2, fdfile->fname, RK_CCL, NULL);
            goto NEXT_SET;
            }
         if (card[0] != '*') break;
         cryout(RK_P4, card, RK_LN1, NULL);
         } /* End header/comment search loop */
      sinform(card, "(S0,IH12,IH12,I12)", &rh->mrx, &rh->mry,
         &rh->npat, NULL);

/* Calculate space needed to store this set and allocate it */

      nmels = rh->mrx*rh->mry;         /* Elements per matrix */
      nwpm = (nmels+NMCEPW-1)/NMCEPW;  /* fdm_type per matrix */
      nfdws = (size_t)rh->npat * (size_t)nwpm;
      rh->pfdm = pm = (fdm_type *)allocpcv(
         Static, nfdws, IXfdm_type, "FD matrices");
      /* Zero it all at once so can OR elements in singly */
      memset(pm, 0, nfdws*sizeof(fdm_type));
      badct = 0;           /* Clear the error counter */

/* Begin processing one matrix--
*  Convert data to binary and store in matrix.  */

      for (patcnt=1; patcnt<=rh->npat; patcnt++) {
         pcd = pcde;
         pshft = BITSPERUI32;
         for (cfct=0; cfct<nmels; pcd+=2,++cfct) {
            int digit;           /* One matrix element */
            fdm_type element;
            /* Finished current (or first) card, read another */
            if (pcd >= pcde) {
               do {
                  if (!rfgets(fdfile, card, CDSIZE+1, ABORT)) {
                     cryout(CROFE,
                        "0***UNEXPECTED EOF ON RFDMTX FILE ", RK_P2,
                        fdfile->fname, RK_CCL, NULL);
                     goto NEXT_SET;
                     }
                  } while (card[0] == '*');
               /* Point pcd to first element */
               pcd = card + FIRST_DATA_COLUMN;
               }

            /* Read 2-column matrix element, convert to binary,
            *     and pack into matrix storage, starting a new
            *     fdm_type word when current one is full.
            *  Caution: The assumption is made that the characters
            *     in pcd are in the same code (ANSI or EBCDIC) as
            *     a character constant in C. */
            digit = (int)((*(pcd + 1)) - '0');
            if (digit < 0 || digit > 7) badct++;   /* Count errors */
            /* Handle sign */
            switch (*pcd) {
            case '0':
            case ' ':
            case '+':
                  element = (fdm_type)digit;
                  break;
            case '1':
            case '-':
                  /* Test for special -0 case */
                  element = (digit == 0) ? ((fdm_type)1 << (NBPMCE-1)) :
                     (((fdm_type)1 << NBPMCE) - (fdm_type)digit);
                  break;
            default: badct++;
               } /* End sign switch */
            pshft -= NBPMCE;
            *pm |= element << pshft;
            if (pshft == 0) pshft = BITSPERUI32, ++pm;
            } /* End of loop over elements */
         /* Start on a new word for next pattern */
         if (pshft < BITSPERUI32) ++pm;
         } /* End of loop over patterns */

/* End of set */

      /* Print error count if nonzero */
      if (badct) {
         convrt("(P1,'0***FDMATRIX FILE ',J1I3,'HAD ',J1IL12,"
            "'BAD CHARACTERS.')", &setno, &badct, NULL);
         RK.iexit |= FILE_ERR; }

      /* Be sure file has been read to end */
      if (rfgets(fdfile, card, CDSIZE+1, ABORT))
         cryout(CROFE, "0***NO EOF AT END OF RFDMTX FILE ",
            RK_P2, fdfile->fname, RK_CCL, NULL);

NEXT_SET:
      rfclose(fdfile, SAME, SAME, ABORT);
BAD_OPEN_NEXT_SET:
      nfdfile = fdfile->nf;      /* Point to next file */
      free(fdfile->fname);
      free(fdfile);
      fdfile = nfdfile;
      } /* End of loop over sets */
   d3file[FDMATRIX] = NULL;
   } /* End of rfdmtx() */
#endif /* ndef PARn */
