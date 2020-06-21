/* (c) Copyright 1991-2006, Neurosciences Research Foundation, Inc. */
/* $Id: rfdmtx.c 11 2008-12-27 15:51:19Z  $ */
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
*       Partial words are packed left with zeros to fill word          *
*       Each matrix begins a new word in storage                       *
*                                                                      *
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
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "d3global.h"
#include "d3fdef.h"
#include "rocks.h"

#ifdef PARn
#error This routine is not used on comp nodes
#else

#define NMCEPL (BITSPERUI32/NBPMCE) /* Matrix elements per long */
#define WORDS_PER_CARD 4
#define CROFE  (RK_P1 + (FILE_ERR << 12))

void rfdmtx(void) {

   struct FDHEADER *rh;      /* Pointer to current header block */
   struct RFdef *fdfile,     /* Pointer to file list */
      *nfdfile;              /* Pointer to next file while freeing */
   char card[CDSIZE+2];      /* Location into which cards are read */
   char *pcd;                /* Pointer to card data */
   long *pm;                 /* Pointer to current matrix word */
   long patcnt;              /* Pattern counter */
   long badct;               /* Number of bad characters */
   long work;                /* Scratch space */
   long pack;                /* Location intp which coeffs are packed */
   int setno;                /* Current set number */
   int wrdctr;               /* Word counter */
   int wdpm;                 /* Number of words per matrix */
   int nrem;                 /* Number of elements in last word */
   int cfcnt;                /* Element counter */
   int ncoeffs;              /* Element counter limit */

/* Allocate space for the header blocks */

   RP->pfdh = rh = (struct FDHEADER *)allocpcv(Static,
      RP->nfdh, IXstr_FDHEADER, "FDHEADER structures.");

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

      do {
         if (!rfgets(fdfile, card, CDSIZE+1, ABORT)) {
            cryout(CROFE, "0***UNEXPECTED EOF ON RFDMTX FILE ",
               RK_P2, fdfile->fname, RK_CCL, NULL);
            goto NEXT_SET;
            }

         /* If it's a comment print it out and keep looking for the
         *  header.  Otherwise break out and read header. */
         if (card[0] == '*')
            cryout(RK_P4, card, RK_LN1, NULL);
         else break;
         } while (1);

      sinform(card, "(S0,IH12,IH12,IL12)", &rh->mrx, &rh->mry,
         &rh->npat, NULL);

/* Calculate space needed to store this set and allocate it */

      work = rh->mrx*rh->mry;          /* Elements per matrix */
      wdpm = (work+NMCEPL-1)/NMCEPL;   /* Longs per matrix */
      nrem = work - (wdpm-1)*NMCEPL;   /* Elements in last long */
      rh->pfdm = (long *)allocpcv(
         Static, rh->npat*wdpm, IXfdm_type, "FD matrices");
      badct = 0;           /* Clear the error counter */

/* Begin processing one matrix--
*  Convert data to binary and store in matrix.  */

      for (patcnt=1,pm=rh->pfdm; patcnt<=rh->npat; patcnt++) {
         for (wrdctr=1; wrdctr<=wdpm; wrdctr++) {
            /* Every WORDS_PER_CARD words a new card must be read. */
            if (!((wrdctr-1)%WORDS_PER_CARD)) {
               do {
                  if (!rfgets(fdfile, card, CDSIZE+1, ABORT)) {
                     cryout(CROFE,
                        "0***UNEXPECTED EOF ON RFDMTX FILE ", RK_P2,
                        fdfile->fname, RK_CCL, NULL);
                     goto NEXT_SET;
                     }
                  } while (card[0] == '*');
               /* Point pcd to first element */
               pcd = card + 8;
               }

/* Begin processing one word--
*     Read 2-character values and pack into 'pack'
*     Count NMCEPL elements per word except on last word
*        where nrem elements are used.  */

            ncoeffs = (wrdctr == wdpm) ? nrem : NMCEPL ;
            pack = 0;
            pcd += (ncoeffs-1)<<1;  /* Point to last coeff in word */
            for (cfcnt=0; cfcnt<ncoeffs; cfcnt++) {
               int digit;
               byte element;
               /* Caution: The assumption is made that the characters
               *           in pcd are in the same code (ANSI or EBCDIC)
               *           as a character constant in C. */
               digit = (int)((*(pcd + 1)) - '0');
               if (digit<0 || digit>7) badct++;  /* Count errors */
               /* Handle sign */
               switch (*pcd) {
               case '0':
               case ' ':
               case '+':
                     element = digit;
                     break;
               case '1':
               case '-':
                     /* Test for special -0 case */
                     element = digit ? -digit : 8 ;
                     break;
               default: badct++;
                  }
               pack = (((unsigned long)pack)>>NBPMCE) +
                      (((unsigned long)element)<<(BITSPERUI32-NBPMCE));
               pcd -= 2;
               } /* End of loop over elements */
            *pm++ = pack;
            pcd += (ncoeffs+1)<<1;  /* Point to next cluster on card */
            } /* End of loop over words */
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
