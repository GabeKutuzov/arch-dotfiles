#include <stdlib.h>
#include <stdio.h>

int *a;

int main()
{
   a = malloc(sizeof(int));
   *a = 32; 
   printf("%d", *a);
   free(a);
   return 0;
}
