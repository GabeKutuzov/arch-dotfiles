/***********************************************************************
*                      Test program for mfbitpk                        *
*                                                                      *
*  V1A, 03/14/15, G.N. Reeke - New program                             *
***********************************************************************/

#define MAIN

#include "../mfint.h"

struct RKGdef _RKG;

/*---------------------------------------------------------------------*
*                                ptbr                                  *
*                Function to print binary result lines                 *
*---------------------------------------------------------------------*/

static void ptbr(byte *res, int nb) {

#define MXLRES 44
   char *pb;
   char pbuf[MXLRES+(MXLRES/8)+3];
   int ib;
   if (nb > MXLRES) exit(99);
   for (pb=pbuf,ib=0; ib<nb; ++ib) {
      int mo = ib >> 3;
      int mm = 1 << (7 - (ib & 7));
      if (mm == 0x80) *pb++ = ' ';
      *pb++ = (res[mo] & mm) ? '1' : '0';
      }
   while (ib++ < MXLRES) *pb++ = ' ';
   cryout(RK_P1, pbuf, RK_LN1 + (pb - pbuf), NULL);
   }

/*---------------------------------------------------------------------*
*                               mfflush                                *
*                Fake output call - just print message                 *
*---------------------------------------------------------------------*/

void mfflush(void) {
   cryout(RK_P1," mfflush called",RK_LN1,NULL);
   }

/*---------------------------------------------------------------------*
*                              mfpktest                                *
*---------------------------------------------------------------------*/

int main(void) {

   int io,il;
   ui32 data=1;
   byte bz[5],bo[5];

   settit("      mfbitpk test");
   rkginit();

   for (il=1; il<=BITSPERUI32; ++data,++il) {
      for (io=0; io<BITSPERBYTE; ++io) {
         memset(bz, 0, 5);
         memset(bo, 0xff, 5);
         if (io == 0)
            convrt("(P1,'0-->Length',I4)",&il,NULL);
         else {
            cryout(RK_P1," ",RK_LN1+1,NULL);
            }
         _RKG.MFCP = bz;
         _RKG.MFbrem = 39-io;
         mfbitpk(data, il);
         ptbr(bz, 40);

         _RKG.MFCP = bo;
         _RKG.MFbrem = 39-io;
         mfbitpk(data, il);
         ptbr(bo, 40);
         }
      }

   cryout(RK_P1,"\0",RK_LN0+RK_FLUSH+1,NULL);
   return 0;
   }
