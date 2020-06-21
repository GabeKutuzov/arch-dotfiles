/* (c) Copyright 1989-2018, The Rockefeller University *21116* */
/* $Id: envshp.c 25 2018-05-07 22:13:48Z  $ */
/***********************************************************************
*              Darwin III Environment Simulation Package               *
*                              ENVSHP                                  *
*                      Process a 'SHAPE' card                          *
*                                                                      *
************************************************************************
*                                                                      *
*  int envshp(struct DSLBDEF ***ppshp, short tux, short tuy,           *
*     long alloc, stim_type hipix, stim_type lopix)                    *
*                                                                      *
*     ppshp:  Ptr to ptr to where next shape goes                      *
*     tux:    Maximum x-size to be loaded (see below).                 *
*     tuy:    Maximum y-size to be loaded (see below).                 *
*     alloc:  Allocation switch.  If > 0, the record already           *
*             exists, its length is ALLOC, and **ppshp points          *
*             to it.  If 0, a new record is to be created and          *
*             its location returned in **ppshp.                        *
*     hipix:  High pixel value                                         *
*     lopix:  Low pixel value                                          *
*                                                                      *
*  Note: If ALLOC = 0, a new record is to be allocated.  It will be    *
*     large enough hold the entire object, as we are being called      *
*     from D3LIBPRP or ENVCTL reading a SHAPE card.  In this case,     *
*     TUX,TUY serve only to pass the EUX,EUY default sizes for         *
*     old-style SHAPE cards that don't have their own size info.       *
*     Old and new hexadecimal formats will be stored as bit arrays.    *
*                                                                      *
*  If ALLOC > 0, a record already exists.  A check is made to be       *
*     sure the new record will fit in the existing space of size       *
*     ALLOC words.  If not, an error occurs (the shape is not cut).    *
*     Hexadecimal objects are now translated to densities EILO,EIHI.   *
*     As above, TUX,TUY are used to define TSX,TSY only for old-       *
*     format shapes.  Call is from ENVCYC doing a SELECT and TUX,      *
*     TUY are being used to pass the SELECT card values, which act     *
*     to define the MALLOC size but not the shape size.  This          *
*     decision was made to avoid complex skip code for the 3 new       *
*     formats, and to allow a file of mixed shapes to be read with     *
*     a user-specified preallocation of storage.                       *
*                                                                      *
*  The maximum size of a Darwin-II shape can never exceed 12x12.       *
*  Control card assumed to be already read via CRYIN and printed.      *
*                                                                      *
*  The format of the constructed record is as follows:                 *
*     1) Ptr to next record (initialized to NULL)                      *
*     2) Name of shape (4 characters)                                  *
*     3) Group identifier (short)                                      *
*     4) Format code (short): 0=old,1=A,2=B,3=C,4=X                    *
*        (256 is added if object has any transparent pixels)           *
*     5) x-size (short)                                                *
*     6) y-size (short)                                                *
*     7) pixel data, 1-byte per pixel, in COLUMN order                 *
************************************************************************
*  V4A, 01/17/89, JWT - Translated into C                              *
*  Rev, 09/11/90,  MC - Fixed length bytes in calloc, malloc           *
*  Rev, 01/28/97, GNR - Trivial changes for ROCKS conversion changes   *
*  ==>, 02/22/03, GNR - Last mod before committing to svn repository   *
*  V8H, 12/01/10, GNR - Separate env.h,envglob.h w/ECOLMAXCH change    *
*  R25, 04/01/18, GNR - Convert ibcdin calls to wbcdin, ipix to int    *
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "sysdef.h"
#include "envglob.h"
#include "rocks.h"
#include "rkxtra.h"
#include "bapkg.h"

#define D2     0           /* Darwin II format        */
#define D3ORIG 4           /* Early Darwin III format */
#define OLD    0           /* Old (bit array) format  */
#define ADJ    1           /* Adjacent byte format    */
#define BW     2           /* Black and white format  */
#define COLOR  3           /* Color format            */
#define HEX    4           /* Hexadecimal format      */
#define TRANSPARENT 256    /* Transparent pixel flag (added to FMT) */
#define byte_format(F) (F&3) /* Test for byte (vs. bit) pixel format */

/* Note field length not included in these macros */
#define STR2UI16 (RK_IORF|RK_CTST|RK_NUNS|RK_NHALF)
#define HEX2INT  (RK_HEXF|RK_CTST|RK_NINT)
#define STR2INT  (RK_IORF|RK_CTST|RK_NINT)

#define FCNUM 4              /* Format codes */
static char *fc[] = { "A","B","C","X" };
static char hxmt[4] = { 8,12,14,15 };  /* Masks for hex input bytes */

/*---------------------------------------------------------------------*
*                            new_format()                              *
*                                                                      *
*  Determine format type                                               *
*---------------------------------------------------------------------*/

static char new_format(char *chptr) {
   char ch;
   while ((ch = *chptr++) != '\n') if (ch == '(') return YES;
   return NO;
   }

/*---------------------------------------------------------------------*
*                              envshp()                                *
*---------------------------------------------------------------------*/

int envshp(struct DSLBDEF ***ppshp, short tux, short tuy, long alloc,
      stim_type hipix, stim_type lopix) {

   struct DSLBDEF *Dptr;   /* (**ppshp) */
   byte *tpx;              /* Ptr to dynamic input buffer */
   stim_type *pixarray;    /* Ptr to pixel "array" for current shape */
   long ldsb;              /* Length of shape memory allocation */
   int  ipix;              /* Decimal input */
   int  pword;             /* Hex input */
   short tsx1, tsy1;       /* X size, Y size (temp) */
   short tfm1;             /* Pixel format (temp) */
   ui16  tgp1;             /* Object group (temp) */
   short ix,iy,ir,iq,jx;   /* Loop counters */
   char lea[DFLT_MAXLEN+1]; /* Input buffer */
   char tnshp[4];          /* Shape name (temp) */
   char fmtlett[2];        /* Format letter */
   char nbit;              /* Bit-wise index into hex number */

/* Check for leading "SHAPE" (optional) */

   cdscan(RK.last, strncmp(RK.last, "SHAPE", 5) ? 0 : 1,
      DFLT_MAXLEN, RK_WDSKIP);

/* Check if card is in new format (marked by a starting '('):
*  Requires a kludge to differentiate between otherwise equivalent
*  '(' and ',' delimiters.  Note: jfind() is not used here due to
*  potentially ambiguous return value of 0.
*
*  Process shape card based on format type (new or old) */

   if (new_format(RK.last)) {
      /* Read name and group number.
      *  Look for new-style format and size params. */
      strcpy(fmtlett, "A");
      inform("(sa4vuhv(v>ih,v>ih,na1v))",
         tnshp, &tgp1, &tsx1, &tsy1, fmtlett, NULL);
      tfm1 = smatch(0, fmtlett, fc, FCNUM);
      }
   else /* No parenthesized parameters (old format) */ {
      /* Read name and group number.
      *  Use default dimensions.  */
      inform("(s0a4)", tnshp, NULL);
      switch (scan(lea, RK_REQFLD)) {
         case D2:     /* Already came to comma ==> Darwin II format
                         (no field name) */
                      wbcdin(tnshp, &tgp1, STR2UI16|sizeof(tnshp)-1);
                      strncpy(tnshp, "DAR2", 4);
                      break;

         case D3ORIG: /* Blank ==> early Darwin III format */
                      /* Read group number and synchronize scan */
                      wbcdin(lea, &tgp1, STR2UI16|DFLT_MAXLEN);
                      if (scan(lea, RK_REQFLD) != 0) goto INPUT_ERR;
                      break;

         default:     /* Format error */ goto INPUT_ERR;
         }
      tsx1 = min(MAX_OBJ_SIZE, tux);
      tsy1 = min(MAX_OBJ_SIZE, tuy);
      tfm1 = OLD;
      }

/* Generate an error if X or Y size zero or less */

   if (tsx1<=0 || tsy1<=0) goto SIZE_ERR;

/* Calculate space needed for shape record and check allocation.
*  If format is old or hex (0 or 4) allocate bits for bytes */

   ldsb = sizeof(struct DSLBDEF) +
      (byte_format(tfm1) ? tsx1*tsy1 : BytesInXBits(tsx1*tsy1));
   if (alloc > 0) { if (ldsb > alloc) goto PREALLOC_ERR; }
   else /* New allocation */
      **ppshp = (struct DSLBDEF *)
         callocv(1, ldsb, "SHAPE card info");

/* Assign shape field values */

   Dptr = **ppshp;
   strncpy(Dptr->nshp, tnshp, SHP_NAME_SIZE);
   Dptr->pnxshp = NULL;
   Dptr->tgp = tgp1;
   Dptr->tfm = tfm1;
   Dptr->tsx = tsx1;
   Dptr->tsy = tsy1;

/* Record address where next shape in list goes */

   *ppshp = &Dptr->pnxshp;

/* Read pixel information according to given format (byte and bit
*  values differentiated inside inner (X) loop) */

   pixarray = (stim_type *)dslpix(Dptr);
   switch (tfm1) {
            int   lhex;
            short lstx,fstx;
case OLD:   /* Old format: one hexadecimal word per row */
            lstx = 20 + max(10, tsx1);
            fstx = lstx - tsx1 + 1;

            /* Skip over unused records at top */
            for (iy=1; iy<=12-tsy1; iy++)
               if (scan(lea, RK_REQFLD)) goto INPUT_ERR;
            /* Read in pixel data */
            scanagn();  /* First item already scanned */
            for (iy=0; iy<tsy1; iy++) {
               if (scan(lea, RK_REQFLD) & ~RK_COMMA) goto INPUT_ERR;
               wbcdin(lea, &pword, HEX2INT|3);
               /* Inner loop: alloc'ed and non-alloc'ed versions */
               if (alloc) {
                  /* Existing memory block (SELECT from shape file):
                  *  convert to pixel densities using hipix and lopix
                  *  argument values.  */
                  for (iq=iy,ix=fstx; ix<=lstx; iq+=tsy1,ix++) {
                     if ((ipix=bittst((byte *)&pword, ix)) == 0)
                        Dptr->tfm |= TRANSPARENT;
                     pixarray[iq] = ipix ? hipix : lopix;
                     }
                  } /* End storing into old shape block */
               else {
                  /* New block: transfer bits to contiguous array
                  *  with transposition.  */
                  for (iq=iy+1,ix=fstx; ix<=lstx; iq+=tsy1,ix++)
                     if (!bittst((byte *)&pword, ix))
                        Dptr->tfm |= TRANSPARENT;
                     else bitset(pixarray, iq);
                  } /* End else */
               } /* End outer (Y) loop */
            break;

case ADJ:   /* Adjacent byte (monochrome) format */
            tpx = (byte *)mallocv(tsx1+1, "SHAPE: A format");
            scanlen(tsx1);
            /* Read pixel data, one scan per pixel row */
            for (iy=0; iy<tsy1; iy++) {
               if (scan((char *)tpx, RK_REQFLD) & ~RK_COMMA)
                  goto INPUT_ERR;
               for (iq=iy,ix=0; ix<tsx1; iq+=tsy1,ix++) {
                  /* Note format spec, reading one hex char per call */
                  wbcdin((char *)(tpx+ix), &ipix, HEX2INT);
                  if (ipix == 0) Dptr->tfm |= TRANSPARENT;
                  pixarray[iq] = (stim_type)ipix;
                  }
               }
            free(tpx);
            break;

case BW:    /* Black and white format */
            /* Read data, one scan per pixel */
            for (iy=0; iy<tsy1; iy++) {
               for (iq=iy,ix=0; ix<tsx1; iq+=tsy1,ix++) {
                  if (scan(lea, RK_REQFLD) & ~RK_COMMA) goto INPUT_ERR;
                  wbcdin(lea, &ipix, STR2INT|DFLT_MAXLEN-1);
                  if (ipix == 0) Dptr->tfm |= TRANSPARENT;
                  pixarray[iq] = (stim_type)ipix;
                  }
               }
            break;

case COLOR: /* Color format */
            /* Read data, one scan per pixel */
            for (iy=0; iy<tsy1; iy++) {
               for (iq=iy,ix=0; ix<tsx1; iq+=tsy1,ix++) {
                  if (scan(lea, RK_REQFLD) & ~RK_COMMA) goto INPUT_ERR;
                  /* Assign color code (IPIX) = "233" packing of
                  *  blue, green, red values, where each value is
                  *  taken from 2 or 3 low-order bits of respective
                  *  input character.  Note implicit dependence here
                  *  on conversion from ASCII codes to octal digits. */
                  if ((ipix = ((*lea&3)<<6) + ((*(lea+1)&7)<<3) +
                     (*(lea+2)&7)) == 0) Dptr->tfm |= TRANSPARENT;
                  pixarray[iq] = (stim_type)ipix;
                  }
               }
            break;

case HEX:   /* Variable-size hexadecimal format */
            lhex = ((tsx1+3)>>2) + 1;  /* Add 1 for leading 'X' */
            tpx = (byte *)callocv(1, lhex+1, "SHAPE: hex format");
            scanlen(lhex);
            /* Read pixel data, one scan per pixel row */
            for (iy=0; iy<tsy1; iy++) {
               if (scan((char *)tpx, RK_REQFLD) & ~RK_COMMA)
                  goto INPUT_ERR;
               /* Choose starting byte w/in string by check for 'X' */
               jx = (*tpx=='X') ? 1 : 0;
               /* Inner loop: alloc'ed and non-alloc'ed versions */
               if (alloc) {
                  /* Existing memory block (SELECT from shape file):
                  *  convert to pixel densities using hipix and lopix
                  *  argument values.  */
                  iq = iy;
                  for (ix=1; ix<=tsx1; jx++,ix+=4) {
                     nbit = min(3, tsx1-ix);
                     wbcdin((char *)(tpx+jx), &ipix, HEX2INT);
                     if ((ipix & hxmt[nbit]) != hxmt[nbit])
                        Dptr->tfm |= TRANSPARENT;
                     for (ir=0; ir<=nbit; ipix<<=1,iq+=tsy1,ir++)
                        pixarray[iq] = ipix & 8 ? hipix : lopix;
                     }
                  } /* End then */
               else {
                  /* New block: transfer bits to contiguous array
                  *  with transposition.  */
                  iq = iy+1;
                  for (ix=1; ix<=tsx1; jx++,ix+=4) {
                     nbit = min(3, tsx1-ix);
                     wbcdin((char *)(tpx+jx), &ipix, HEX2INT);
                     if ((ipix & hxmt[nbit]) != hxmt[nbit])
                        Dptr->tfm |= TRANSPARENT;
                     for (ir=0; ir<=nbit; ipix<<=1,iq+=tsy1,ir++)
                        if (ipix & 8) bitset(pixarray, iq);
                     }
                  } /* End else */
               } /* End outer (iy) loop */
            free(tpx);
            break;

default:    /* Unknown format */ goto INPUT_ERR;

      } /* End switch */

   thatsall();
   return ENV_NORMAL;

/**** ERROR LABELS ****/

PREALLOC_ERR: cryout(RK_P1, "0*** ENVSHP: Shape size exceeds "
                     "pre-allocated storage", RK_LN2, NULL);
              RK.iexit |= ENV_ERR;
              skip2end();
              return ENV_FATAL;

SIZE_ERR:     cryout(RK_P1, "0*** ENVSHP: Zero X or Y shape size",
                     RK_LN2, NULL);
              RK.iexit |= ENV_ERR;
              skip2end();
              return ENV_FATAL;

INPUT_ERR:    cryout(RK_P1, "0*** ENVSHP: Unrecognized input",
                     RK_LN2, NULL);
              RK.iexit |= ENV_ERR;
              skip2end();
              return ENV_FATAL;

   } /* End envshp() */

