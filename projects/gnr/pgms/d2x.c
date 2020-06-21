/* dhex.c */
/* quick decimal to hex converter */
/* command line: dhex dd      dd is decimal number */
/* w.e.g. 5/90 */
/* Rev, 03/20/12, GNR - Correct to handle numbers >= 2**31 */
#include <stdio.h>

main(int argc, char *argv[]) {
   unsigned long num;
   while (--argc > 0) {
      sscanf(*++argv, "%ld", &num);
      printf("%12ld  0x%lx\n", num, num);
      }
   }
