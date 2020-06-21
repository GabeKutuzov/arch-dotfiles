#include <stdio.h>

int main() {
   long long_max  = 0x7FFFFFFFFFFFFFFFL;
/*   enum { Mon, Tues, Wed } day;   */
   printf("sizeof(int) = %d\n", sizeof(int));
   printf("sizeof(long) = %d\n", sizeof(long));
   printf("sizeof(long long) = %d\n", sizeof (long long));
   printf("the biggest long is %ld\n", long_max);
/* Following line gives compile error--
*     "invalid application of 'sizeof' to incomplete type 'enum day'"
*     This tells us we cannot include enums in structs analyzed by
*     nxdr2
*  printf("sizeof(enum) = %d\n", sizeof(enum day));   */
   return 0;
   }
