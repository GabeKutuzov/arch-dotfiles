/* A little test of access speeds */
#include <time.h>
#include <stdlib.h>
#include "sysdef.h"
#include "rkarith.h"

#define NTEST 100000
#define NRPT     100

int main(void) {
   clock_t t0,t;
   long sums[2];
   long data[NTEST];
   long seed = 1007;
   int ipm[2] = { 0, 1 };
   int i,j;

   for (i=0; i<NTEST; ++i) {
      data[i] = udev(&seed) % 10000;
      if (seed & 1048576) data[i] = -data[i];
      }

   t0 = clock();
   for (j=0; j<NRPT; ++j) {
      sums[0] = sums[1] = j;
      for (i=0; i<NTEST; ++i) {
         if (data[i] >= 0) sums[ipm[0]] += data[i];
         else sums[ipm[1]] += data[i];
         }
      }
   t = clock();
   printf("Method 1: if on data, time = %d\n", (int)(t - t0));
   printf("   Sums are %d, %d\n", sums[0], sums[1]);
   
   t0 = clock();
   for (j=0; j<NRPT; ++j) {
      sums[0] = sums[1] = j;
      for (i=0; i<NTEST; ++i) {
         sums[ipm[data[i] < 0]] += data[i];
         }
      }
   t = clock();
   printf("Method 2: logical subscript, time = %d\n", (int)(t - t0));
   printf("   Sums are %d, %d\n", sums[0], sums[1]);

   t0 = clock();
   for (j=0; j<NRPT; ++j) {
      sums[0] = sums[1] = j;
      for (i=0; i<NTEST; ++i) {
         sums[ipm[(ui32)data[i] >> 31]] += data[i];
         }
      }
   t = clock();
   printf("Method 3: shift sign to subscript, time = %d\n", (int)(t - t0));
   printf("   Sums are %d, %d\n", sums[0], sums[1]);

   return 0;
   }
