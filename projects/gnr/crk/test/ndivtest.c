/***********************************************************************
*                              ndivtest                                *
*                                                                      *
*  This program is designed to (1) test whether the speed of the       *
*  rather complex long division algorithm in vdivl() is indeed         *
*  faster than the classical binary algorithm in jduwb() by            *
*  performing 1,000,000 divisions both ways with random arguments,     *
*  and (2) check that the results for the two algorithms are the       *
*  same.                                                               *
*                                                                      *
*  Source code of jduwb is built in here so this test will survive     *
*  any reconfiguration of old jduwb to use vdivl method.               *
*----------------------------------------------------------------------*
*  V1A, 05/31/09, GNR - New program                                    *
*  V1B, 06/13/09, GNR - Call vdivl externally                          *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sysdef.h"
#include "rksubs.h"
#include "rkarith.h"

#define NTESTS 1000000

/*=====================================================================*
*                  ROCKS Library - C Implementation                    *
*                                jduwb                                 *
*                                                                      *
*  Current working version with binary long division                   *
*=====================================================================*/

ui64 jduwb(ui64 x, ui32 y, ui32 *pr) {

#ifdef HAS_I64
   ui64 d;

   if (y == 0) {              /* Return 0 if divisor == 0 */
      *pr = 0; return 0; }

   d = (ui64)y;               /* Extend divisor to 64 bits */
   *pr = (ui32)(x % d);
   return (x / d);

#else   /* NO_I64 */
   ui64 rv;                   /* Will hold result */

   if (y == 0) {              /* Return 0 if divisor == 0 */
      *pr = rv.hi = rv.lo = 0; return rv; }

/* If divisor y is <= 2^16, we can use machine division
*  to get the answer much more quickly.  */

   if (y <= (UI16_MAX+1)) {
#define BPS BITSPERSHORT
      ui32 d1 = x.hi % y << BPS | x.lo >> BPS;
      ui32 d2 = d1 % y << BPS | x.lo & UI16_MAX;
      rv.hi = x.hi/y;
      rv.lo = d1/y << BPS | d2/y;
      *pr = d2 % y;
      return rv;
      }

/* If divisor y is larger than 2^16, it seems just as well to perform
*  classical long division.  Added test to skip half the divide cycles
*  in the perhaps common case that the high-order dividend is 0.  */

   else {
#define S2U (BITSPERUI32-1)   /* Shift high bit to units bit */
      ui32 dc, d0 = 0;
      int  ct;

      /* Build a 96 bit dividend register */
      if (x.hi == 0) rv.hi = x.lo, rv.lo =    0, ct = BITSPERUI32/2;
      else           rv.hi = x.hi, rv.lo = x.lo, ct = BITSPERUI32;

      /* Perform long division.  Two divide steps are done in each
      *  loop iteration in order to reduce loop overhead.  Each time
      *  the dividend is shifted left by addition, another quotient
      *  bit is brought in at the right.  */
      while (ct--) {
         dc = d0 >> S2U;               /* One division cycle */
         d0 += d0 + (rv.hi >> S2U);
         rv.hi += rv.hi + (rv.lo >> S2U);
         rv.lo += rv.lo;
         if (dc | d0 >= y) d0 -= y, rv.lo += 1;
         dc = d0 >> S2U;               /* Second division cycle */
         d0 += d0 + (rv.hi >> S2U);
         rv.hi += rv.hi + (rv.lo >> S2U);
         rv.lo += rv.lo;
         if (dc | d0 >= y) d0 -= y, rv.lo += 1;
         }

      *pr = d0;
      return rv;
      } /* End long division method */
#endif

   } /* End jduwb() */


/*=====================================================================*
*                          Main test program                           *
*=====================================================================*/

int main(int argc, char *argv[]) {

   struct divdat {
      ui64 dend;
      ui64 oldq;
      ui64 newq;
      ui32 dsor;
      ui32 oldr;
      ui32 newr;
      } *pd;
   double v0,v1;
   ui32 work[12];
   si32 seed = 1009;
   int i,ne;

   pd = calloc(NTESTS, sizeof(struct divdat));

   /* Populate the data */
   for (i=0; i<NTESTS; ++i) {
      pd[i].dend = jcuw((ui32)udev(&seed),(ui32)udev(&seed));
      pd[i].dsor = udev(&seed);
      }

   /* Perform division with old algorithm */
   v0 = second();
   for (i=0; i<NTESTS; ++i)
      pd[i].oldq = jduwb(pd[i].dend,pd[i].dsor,&pd[i].oldr);
   v1 = second();

   printf("\n1,000,000 base 2 divisions completed in %.5f sec\n",
      v1 - v0);

   /* Perform division with new algorithm */
   v0 = second();
   for (i=0; i<NTESTS; ++i) {
      struct divdat *pi = pd + i;
      vdivl((ui32 *)&pi->dend, &pi->dsor, (ui32 *)&pi->newq,
         &pi->newr, work, 2, 1, 0);
      }
   v1 = second();

#ifdef HAS_I64
   i = BITSPERUI64/2;
#else
   i = BITSPERUI32/2;
#endif
   printf("\n1,000,000 base 2^%d divisions completed in %.5f sec\n",
      i, v1 - v0);

   /* Check that the two methods agree -- print first 10 errors */
   ne = 0;
   for (i=0; i<NTESTS; ++i) {
      ui64 diff = jruw(pd[i].oldq,pd[i].newq);
      if (quw(diff) || pd[i].oldr != pd[i].newr) {
         if (++ne <= 10) printf("Difference at i = %d:\n"
            "Dividend = %8x,%8x, divisor = %8x\n"
            "New quotient = %8x,%8x, old quotient = %8x,%8x\n"
            "New remainder = %8x,    old remainder = %8x\n",
            i, uwhi(pd[i].dend), uwlo(pd[i].dend), pd[i].dsor,
            uwhi(pd[i].newq), uwlo(pd[i].newq),
            uwhi(pd[i].oldq), uwlo(pd[i].oldq),
            pd[i].newr, pd[i].oldr);
         }
      }

   printf("Total errors %d\n", ne);
   return 0;
   } /* End ndivtest */
