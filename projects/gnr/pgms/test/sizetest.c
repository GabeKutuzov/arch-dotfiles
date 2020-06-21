/* Check sizes of int, long, long long, and size_t
*  on 32- vs 64-bit systems */
/* Test whether an explicit typecast to long in a
*  call to a function with ... prototype yields
*  long or int.  */
/* Test whether a right shift of a signed int propagates
*  the sign.  Test existence of long long, sqrtf, fabs.  */

/* Undefine these to get rid of error messages */
#define TESTLL
#define TESTFF

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>

void testfn(char *emsg, ...) {
   va_list args;
   long a1 = 0, a2 = 0;

   va_start(args, emsg);
   a1 = va_arg(args, long);
   a2 = (long)va_arg(args, int);
   printf(emsg);
   printf("Value for a1 is %ld, a2 is %ld\n", a1, a2);
   va_end(args);
   return;
   }

int main(void) {
#ifdef TESTLL
   long long lli = 1;
#endif
   long b1 = 123456789012345, b2 = 543210987654321;
   int lint = sizeof(int);
   int llon = sizeof(long);
#ifdef TESTLL
   int lllo = sizeof(long long);
#endif
   int lptr = sizeof(void *);
   int lszt = sizeof(size_t);
   int m1 = -2,rsm1;
   printf("Size of int = %d\n", lint);
   printf("Size of long = %d\n", llon);
#ifdef TESTLL
   printf("Size of long long = %d\n", lllo);
#else
   printf("Long long test was turned off\n");
#endif
   printf("Size of size_t = %d\n", lszt);
   printf("Size of a pointer = %d\n", lptr);
   testfn("Arg passing test:\n", b1, b2);
   rsm1 = m1 >> 1;
   if (rsm1 == -1)
      printf("Right shift of -2 yields -1\n");
   else if (rsm1 > 0)
      printf("Right shift of -2 yields positive value\n");
   else
      printf("Right shift of -2 yields %zd\n", rsm1);
#ifdef TESTFF
   {  float ft1 = 4.0, ft2 = -1;
      float rtft1 = sqrtf(ft1);
      float aft2 = fabsf(ft2);
      printf("sqrtf(4) = %f, fabsf(-1) = %f\n", rtft1, aft2);
      }
#else
   printf("Tests for existence of sqrtf, fabsf skipped\n")
#endif
   return 0;
   }

