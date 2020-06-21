/* (c) Copyright 20xx, The Rockefeller University *nnnnn* */
/*********************************************************************
*                                                                    *
* SYNOPSIS:                                                          *
*     byte* readbits(struct MFsdef *s, size_t nbits)                 *
*                                                                    *
* DESCRIPTION:                                                       *
*    The purpose of this function is to pull an arbitrary amount     *
*     of bits from a byte array at a specified index, increment      *
*     the index and return the value of the bits through other       *
*     functions, such as:                                            *
*     readbitsi() - readbits and return a signed integer.            *
*     readbitsf() - readbits and return a floating-point number.     *
*     readbytes() - align to byte-boundary and read characters.      *
*                                                                    *
**********************************************************************
*  Version 1, 00/00/19, GMK                                          *
*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "sysdef.h"
#include "rfdef.h"
#include "readmf.h"

/*=====================================================================*
*                             readbits()                               *
*  Read arbitrary amount of bits from the metafile.                    *
*=====================================================================*/

byte* readbits(MFS *mfs, size_t nbits) {
      
   int ofs, sft;       /* Bit-flip offset, bit-shift offset */
   int br, br0, blft;
   int bt;
   int i;
   int rbo  = 0;
   int bl   = 0;
   int j    = 0;
   int k    = 0;

   ui16 bb = BITSPERBYTE; /* Intermediate for b/B constant */   

   int* bits = malloc(nbits*sizeof(int));
   byte *bitstrm = malloc((nbits/bb)*sizeof(byte));

   /* Determine the bit remainder */
   br = br0 = (mfs->bytesread*bb)-mfs->bitpos;
   if (br != 0) br = br0 = bb-br;
   
   bl = bb-br;
   blft = nbits;

   for (i=0;i<nbits;i++) {
      
      /* Determine byte to read  */
      bt = mfs->bitpos/bb;

      /* Handle reading next byte */
      if (bt >= mfs->bytesread) {
         if(!rfread(mfs->rfptr,&mfs->currbyte,1,ABORT)) {
            /* End of file */
            mfs->eof = END_OF_FILE;
            return;
         }
         mfs->bytesread++;
         if(i>0) rbo++;  /* Ensure 'rbo' is 0 on first pass */         
         br = 0, j = 0;
         k  = i;
         blft = nbits-i;
         bl = bb-br;
      }

      /* Current byte can hold all remaining bits */
      if (blft < bl) {
         ofs = (k-1)+blft-j;
         sft = bl-blft+j;
      }
      /* Current byte has extra bits */
      else {
         ofs = (rbo+1)*bb-(br0+j)-1;
         sft = j;
      }
            
      bits[ofs] = (mfs->currbyte >> sft) & 1;  /* place bits */
      br++;j++;
      mfs->bitpos++;

   }

   /* store bits into bitstrm pointer */
   for (i=0;i<nbits;i++) {
      bt = nbits/bb; /* index of current byte */
      bitstrm[i] = (bitstrm[i] & 1<<bb-(bb*(i/bb)));
   }

   return bitstrm;

} /* End of readbits */

/*---------------------------------------------------------------------*
*              !!!BELOW either is or to be DEPRACATED!!!               *
*---------------------------------------------------------------------*/

/*=====================================================================*
*                            readbitsi()                               *
*  Read bits and return value as an integer.                           *
*=====================================================================*/

int readbitsi(MFS *mfs, size_t nbits) {
   
   int *bits;   /* Pointer to bit array  */
   int i   = 0;
   int ret = 0;
   bits = malloc(nbits*sizeof(int));
   // define GRAB_BITS?
   readbits(mfs,nbits,bits);
   /* Get numeric value of bits[] */
   for (i=0; i<nbits; i++) {
      if (bits[i]==1)
         // NEW SHIFTING HERE //
         ret += pow(2, nbits-i-1);
   }
   free(bits);
   return ret;
} /* end of readbitsi */

/*=====================================================================*
*                            readbitsf()                               *
*  Read bits and return value as a float.                              *
*=====================================================================*/

float readbitsf(MFS *mfs, size_t nbits) {

   int *bits;   /* Pointer to bit array  */
   int i = 0;
   float ret = 0;
   bits = malloc(nbits*sizeof(int));
   readbits(mfs,nbits,bits);
   /* Get numeric value of bits[] */
   for (i=0; i<nbits; i++) {
      if (bits[i]==1)
         ret += pow(2, (nbits-i-1)*-1);
   }
   free(bits);
   return ret;
}

/*=====================================================================*
*                            readbytes()                               *
*  Algign to byte-boundary and read a character string from metafile.  *
*=====================================================================*/

char* readbytes(MFS *mfs, size_t nbytes) {

   char *ret = malloc(sizeof(byte)*nbytes);

   /* Check for alignment */
   if ((mfs->bitpos/mfs->bytesread) > 0) {
      rfseek(mfs->rfptr,1,SEEK_CUR,ABORT);
      mfs->bytesread++;
   }

   if (rfread(mfs->rfptr,ret,nbytes,ABORT)) {
      /* Alignment for next read */
      mfs->bytesread += nbytes;
      mfs->bitpos = mfs->bytesread*BITSPERBYTE;
   }

   return ret;
}
