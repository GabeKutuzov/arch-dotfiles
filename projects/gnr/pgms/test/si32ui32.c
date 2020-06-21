/***********************************************************************
*   Test whether (si32) applied to a big (ui16) produces a             *
*   positive or a negative result.                                     *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "sysdef.h"

int main(void) {
   si32 result1,result2,result3;
   ui16 big = 0xe000U;
   result1 = (si32)big;
   result2 = (si32)(ui32)big;
   result3 = -(si32)big;
   printf("Doing (si32)bigui16 gives %d\n", result1);
   printf("   (si32)(ui32)bigui16 gives %d\n", result2);
   printf("   -(si32)bigui16 gives %d\n", result3);
   return 0;
   }
