/* (c) Copyright 1989-2009, The Rockefeller University *11115* */
/* $Id: bapkg.h 59 2017-01-13 20:34:16Z  $ */
/***********************************************************************
*                         BAPKG.H Header File                          *
*                     BAPKG function declarations                      *
*                                                                      *
*  V1A, 04/19/89, GNR - Initial version                                *
*  Rev, 12/26/91, GNR - Add bitcmp.  Rev, 02/26/92, GNR - Add bytnxr   *
*  Rev, 07/04/92, GNR - Change bytxxx ptr args to void *               *
*  Rev, 08/27/05, GNR - Add BITPKDEF, bitpack, bitunpk, setbpack, etc. *
*  ==>, 03/22/08, GNR - Last date before committing to svn repository  *
*  Rev, 01/07/09, GNR - Change bitor to bitior -- it's a C++ keyword   *
***********************************************************************/

typedef struct BITPKDEF {
   byte  *pb0;                   /* Ptr to start of data array */
   byte  *pbe;                   /* Ptr to end+1 of data array */
   byte  *pb;                    /* Ptr to current byte in pb0 */
   int   ibr;                    /* Bits remaining in current byte */
   } BITPKDEF;

void bitclr(unsigned char *array, long bit);
void bitcmp(unsigned char *array, long bit);
long bitcnt(void *array, long bytlen);
void bitior(unsigned char *t1, int jt,
   unsigned char *s1, int js, long len);
int  bitpack(struct BITPKDEF *pbpd, long item, int nbits);
long bitunpk(struct BITPKDEF *pbpd, int nbits);
void bitset(unsigned char *array, long bit);
int  bittst(unsigned char *array, long bit);
void bytand(void *array1, long bytlen, void *array2);
void bytior(void *array1, long bytlen, void *array2);
void bytmov(void *array1, long bytlen, void *array2);
void bytxor(void *array1, long bytlen, void *array2);
void bytnxr(void *array1, long bytlen, void *array2);
void setbpack(struct BITPKDEF *pbpd,
   void *pbits, size_t npbpd, long io1);
void setbunpk(struct BITPKDEF *pbpd,
   void *pbits, size_t npbpd, long io1);
