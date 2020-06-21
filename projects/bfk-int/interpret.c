/***********************************************************************
*                                                                      *
*                        Brainfuck Interpreter                         *
*                                                                      *
***********************************************************************/

#include<stdio.h>
#include<stdlib.h>

int main(int argc, char **argv) {

   int *d_ptr = malloc(8*sizeof(int));

   d_ptr[3] = 32;

   printf("%d : %d : %d\n", d_ptr[3], d_ptr[8], d_ptr[4127]);


   /* Read file shit */


   free(d_ptr);

   return 0;
}
