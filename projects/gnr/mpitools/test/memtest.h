/***********************************************************************
*                              memtest.h                               *
*                                                                      *
*  This is the header file for memtest.c.  It contains definitions of  *
*  several structure types that are used in the tests.  This file must *
*  be processed by the nxdr2 utility to prepare conversion tables for  *
*  the parallel tests.  Base types of various lengths are deliberately *
*  mixed to make trouble.  These structures have no actual purpose.    *
*                                                                      *
*  V1A, 09/12/99, GNR - New program                                    *
***********************************************************************/

/* Message types */
#define MEMTEST_ACK  0x7748
#define MEMTEST_PTRS 0x7749

struct mts1 {
   int s1i1;
   double s1d1;
   struct mts1 *pfs1;
   struct mts2 *pns2;
   double s1d2;
   char s1c1;
   };

struct mts2 {
   char s2c1;
   long s2l1;
   struct mts1 *pns1;
   char s2c2;
   };

union mtu3 {
   struct mts1 us1;
   struct mts2 us2;
   long us3[5];
   };

typedef struct mtsu {
   int tu;
   union mtu3 suu1;
   } mttsu;
