/***********************************************************************
*   Test what strncmp returns if one or both strings are empty.        *
*                                                                      *
*  N.B.  By test, a segfault occurs if a NULL pointer is passed to     *
*  strncmp, therefore those tests have been removed from this code.    *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sysdef.h"

int main(void) {
   int r1,r2,r3,r4,r5,r6;
   r4 = strncmp("Test string", "", 4);
   r5 = strncmp("", "Test string", 4);
   r6 = strncmp("", "", 4);
   printf("strncmp test 4: strncmp(string, \"\") yields %d\n", r4);
   printf("strncmp test 5: strncmp(\"\", string) yields %d\n", r5);
   printf("strncmp test 6: strncmp(\"\", \"\") yields %d\n", r6);
   return 0;
   }
