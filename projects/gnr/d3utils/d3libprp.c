/* (c) Copyright 1989-2008, The Rockefeller University *21113* */
/***********************************************************************
*                              D3LIBPRP                                *
*                                                                      *
*                   Create a shape library for CNS                     *
*                                                                      *
*  D3libprp is invoked as follows:                                     *
*                                                                      *
*        mklib infile outfile [libcards]                               *
*                                                                      *
*  where infile = name of input file containing shape definitions      *
*       outfile = name of output binary shape library                  *
*      libcards = name of [optional] d3libprp input card file          *
*                                                                      *
*  optional new PIXEL input card has following format:                 *
*            HI = high pixel value,  (dflt = 255)                      *
*            LO = low pixel value    (dflt = 8)                        *
*                                                                      *
*  All binary data in the output shape library are BIG-endian.         *
*  The format of the output shape library is:                          *
*     Record 1:  library header (16 bytes)                             *
*       char[12]: name and version of making program                   *
*           long: number of shapes in library                          *
*     Records 2-ff: index records (8-bytes per record)                 *
*        char[4]: shape name                                           *
*           long: starting record number for that shape                *
*                                                                      *
*     Remaining records: DSLB shape information, envshp format         *
*                         (each padded to 80 bytes)                    *
*                                                                      *
*  The format of the constructed records is as follows:                *
*     1) Ptr to next record (initialized to NULL)                      *
*     2) Name of shape (4 characters)                                  *
*     3) Group identifier (short)                                      *
*     4) Format code (short): 0=old,1=A,2=B,3=C,4=X                    *
*        (256 is added if object has any transparent pixels)           *
*     5) x-size (short)                                                *
*     6) y-size (short)                                                *
*     7) pixel data, 1-byte per pixel (formats 0 & 4) or               *
*                    1-bit  per pixel (formats 1,2,3)                  *
*                    in COLUMN order                                   *
*                                                                      *
*  V4A, 02/06/89, JWT - Converted to C                                 *
*  V4B, 05/16/89, JWT - Direct environment dependecies removed         *
*  V4C, 06/02/89, JWT - Made stand-alone routine                       *
*  Rev, 04/02/90, GNR - Remove built-in directories                    *
*  Rev, 03/04/92, GNR - Use rf io routines                             *
*  Rev, 01/29/97, GNR - Use new kwscan codes for byte input, remove    *
*                       refs to obsolete shptr->u1 union, make output  *
*                       always big-endian regardless of machine type.  *
*  Rev, 11/14/97, GNR - Remove NCUBE support                           *
*  Rev, 05/22/99, GNR - Use new swapping scheme                        *
*  Rev, 09/26/08, GNR - Minor type changes for 64-bit compilation      *
***********************************************************************/

#define MAIN
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "swap.h"
#include "envglob.h"
#include "rocks.h"
#include "rkxtra.h"

#define LIB_REC_SIZE 80        /* Size of library record in bytes   */
#define SHP_NAME_SIZE 4        /* strlen(shape name)     */
#define MAX_OBJ_SIZE 12        /* Largest allowable obj size (D II) */
#define ALPHABETIC 1           /* Alphabetic sort keys   */
#define INDEX_SIZE 8           /* Size of library index record */
#define LIBHEAD "D3LIBPRP V8A" /* Library header         */
#define LIBOK 0                /* Normal return value    */
#define LIBERR 1               /* Error return value     */
#define DLOPIX   8             /* Default lo pixel value */
#define DHIPIX 255             /* Default hi pixel value */
#define byte_format(f) (f&3)   /* Check byte (vs. bit) pixel format  */

static char *TTL = "TITLE " LIBHEAD "- DARWIN III SHAPE LIBRARY "
                   "PREPARATION UTILITY";

int main(int argc, char *argv[] ) {

   struct RFdef   *flib;       /* Ptr to lib file descriptor */
   struct DSLBDEF *pexshp;     /* Ptr to external shape list */
   struct DSLBDEF **pshpfill;  /* Ptr to link field of last shape */
   struct DSLBDEF *shptr,      /* Ptr to current shape */
         *prev = NULL;         /* Ptr to previous shape */
   char *card;                 /* Ptr to current input card */
   long nshape;                /* Number of shapes read */
   long pixbytes;              /* Number of bytes needed to store */
                               /*    pixel info for current shape */
   long index;                 /* Current record index number */
   ui32 ic;                    /* kwscan option recorder */
   int ipr = LIB_REC_SIZE/INDEX_SIZE; /* Number of indices per record */
   int padno;                  /* Number of padding chars to output */
   short tux = MAX_OBJ_SIZE,   /* Default x,y shape size */
         tuy = MAX_OBJ_SIZE;
   byte hipix = DHIPIX;        /* High pixel value */
   byte lopix = DLOPIX;        /* Low pixel value  */
   char initing;               /* TRUE while reading non-shape
                               *  (initializing) cards */
   char outbuff[LIB_REC_SIZE]; /* Buffer for output to library */
   char infile[LFILNM];        /* Input file name buffer */
   char outfile[LFILNM];       /* Output file name buffer */

/* Make a page title and prepare to read from input file */

   settit(TTL);
   RK.iexit = 0;

/* Read shape library source and output files */

   if (argc < 3) {
      cryout(RK_P1,"0***D3LIBPRP: Input and output file names "
            "must be supplied",RK_LN2,NULL);
      exit(LIBERR);
      }
   strcpy(infile,argv[1]);
   strcpy(outfile,argv[2]);

/* Set the default values and then see if there are control cards to
*  change them...  If first card starts with a number, consider it
*  old-style library record count and ignore...  */

   tux = tuy = 12;

   initing = TRUE;
   while (initing) {
      card = cryin();
      if (card == NULL) initing = FALSE;
      else if (!strncmp(card,"INPUT",5)) {
         cdprnt(card);
         cdscan(card,1,DFLT_MAXLEN,RK_WDSKIP);
         kwscan(&ic,"KUX%v>ih",&tux,"KUY%v>ih",&tuy,NULL);
         }
      else if (!strncmp(card,"PIXEL",6)) {
         cdprnt(card);
         cdscan(card,1,DFLT_MAXLEN,RK_WDSKIP);
         kwscan(&ic, "HI%v>uc",&hipix, "LO%vuc",&lopix,  NULL);
         }
      else if (cntrl(card)) { initing = FALSE; rdagn(); }
      }

/*---------------------------------------------------------------------*
*  STAGE I.  Read shapes until EOF and build linked list...            *
*     Leading "SHAPE" is still optional (assumes non-empty input deck) *
*---------------------------------------------------------------------*/

   cdunit(infile);
   pshpfill = &pexshp;
   for (nshape=0; (card=cryin())!=NULL && strncmp(card,"END",3);
         nshape++)
      { cdprt1(card); envshp(&pshpfill,tux,tuy,0,hipix,lopix); }

/*---------------------------------------------------------------------*
*  INTERPHASE: Sort the shapes (error if none entered)...              *
*              Open output file and write header                       *
*---------------------------------------------------------------------*/

   if (!nshape) goto ZSHP_ERR;
   if (RK.iexit) goto INSHP_ERR;
   pexshp = (struct DSLBDEF *)sort(pexshp,sizeof(void *),
         SHP_NAME_SIZE<<1,ALPHABETIC);

   flib = rfallo(outfile,WRITE,BINARY,
      DIRECT,IGNORE,NO_LOOKAHEAD,REWIND,RELEASE_BUFF,IGNORE,
      LIB_REC_SIZE,nshape,ABORT);

   rfopen(flib,outfile,IGNORE,BINARY,
      IGNORE,IGNORE,IGNORE,IGNORE,IGNORE,IGNORE,
      IGNORE,nshape,ABORT);

   strcpy(outbuff,LIBHEAD);
   rfwrite(flib,outbuff,strlen(outbuff),ABORT);
   rfwi4(flib, (si32)nshape);
   padno = LIB_REC_SIZE - (strlen(outbuff) + sizeof(long));

   /* Fill outbuff with garbage for record padding */
   memset(outbuff, '@', LIB_REC_SIZE);
   rfwrite(flib,outbuff,padno,ABORT);

/*---------------------------------------------------------------------*
*  STAGE II.  Create an index at the start of the file.                *
*             One header record and at least one index record precede  *
*             first shape.                                             *
*---------------------------------------------------------------------*/

   index = (nshape-1)/ipr + 2;
   for (shptr=pexshp; shptr; shptr=(prev=shptr)->pnxshp) {
      if (prev && !strncmp(prev->nshp,shptr->nshp,SHP_NAME_SIZE))
         cryout(RK_P1,"0***NAME ",RK_LN2+9,prev->nshp,4,
            " IS DUPLICATED.",15,NULL);
      rfwrite(flib, shptr->nshp, SHP_NAME_SIZE, ABORT);
      rfwi4(flib, (si32)index);

      /* Calculate number of 80-col records required for an object of
      *  size TSX*TSY, given 1 byte/pixel (formats 1-3) or 1 byte per
      *  8 pixels (formats 0 or 4) and 12 extra header bytes always. */
      if (byte_format(shptr->tfm))
         index += 1 + (shptr->tsx*shptr->tsy + 11)/LIB_REC_SIZE;
      else /* bit format */
         index += 1 + (shptr->tsx*shptr->tsy + 95)/(LIB_REC_SIZE<<3);
      } /* End index loop */

   /* Pad last record if necessary */
   if ((padno = LIB_REC_SIZE - INDEX_SIZE*(nshape%ipr)) < LIB_REC_SIZE)
      rfwrite(flib,outbuff,padno,ABORT);

/*---------------------------------------------------------------------*
*  STAGE III.  Write out pixel data sequentially                       *
*---------------------------------------------------------------------*/

   for (shptr=pexshp; shptr; shptr=shptr->pnxshp) {
      rfwrite(flib,shptr->nshp,SHP_NAME_SIZE,ABORT);
      rfwi2(flib, shptr->tgp);
      rfwi2(flib, shptr->tfm);
      rfwi2(flib, shptr->tsx);
      rfwi2(flib, shptr->tsy);
      pixbytes = byte_format(shptr->tfm) ? shptr->tsx*shptr->tsy :
         (shptr->tsx*shptr->tsy+7)>>3;
      /* Calculate record padding (if any) */
      padno = LIB_REC_SIZE - (12+pixbytes)%LIB_REC_SIZE;
      rfwrite(flib,dslpix(shptr),pixbytes,ABORT);
      rfwrite(flib,outbuff,padno,ABORT);
      }

/* Report shape library completed */
   --index;
   convrt("(H0J1IL4,'SHAPES WRITTEN IN ',J1IL4,'RECORDS.')",
      &nshape,&index,NULL);
   rfclose(flib,REWIND,RELEASE_BUFF,ABORT);
   cdunit(NULL);
   exit(LIBOK);

/* Error returns */

   ZSHP_ERR:   cryout(RK_P1,"0***D3LIBPRP: no shapes found",RK_LN2,NULL);
               exit(LIBERR);

   INSHP_ERR:  cryout(RK_P1,"0***D3LIBPRP: terminated by input errors",
                      RK_LN2,NULL);
               exit(LIBERR);

   } /* End d3libprp */
