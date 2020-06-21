/***********************************************************************
*     Test whether gcc can produce a double precision negative 0.      *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(void) {
   union {
      double dmz;
      long lmz;
      } u1;
   u1.dmz = (double)(-0);
   printf("(double)(-0) produced %lx\n", u1.lmz);
   return 0;
   }
