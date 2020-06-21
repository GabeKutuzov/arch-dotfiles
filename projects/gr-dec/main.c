/*=====================================================================*
*                         The Children's Place                         *
*                         Gift Receipt Decoder                         *
*=====================================================================*/

#include <stdio.h>
#include <ctype.h>

int main() {

   char code[6] = {1,1,1,1,1,1};

   printf("\n-- The Children's Place --\n- Gift Receipt Decoder -\n");
   printf("\nBreakdown\nNumber\t\tCode\n"
      "0\t\tZ\n"
      "1\t\tX\n"
      "2\t\tC\n"
      "3\t\tV\n"
      "4\t\tB\n"
      "5\t\tN\n"
      "6\t\tM\n"
      "7\t\tL\n"
      "8\t\tK\n"
      "9\t\tJ\n");

   printf("\nEnter 6-digit code: ");
   scanf("%6s", code);

   printf("\n Result: $");

   int i;
   int leadzero = 0;
   for(i = 0; i < 6; i++) {

      char upc = toupper(code[i]);
      code[i] = upc;

      if(i == 4) printf(".");

#define cond code[i] ==
      if(cond'Z' && leadzero > 0) printf("0");

      else {
         if(code[i] != 'Z') leadzero++;
         if(cond'X') printf("1");
         if(cond'C') printf("2");
         if(cond'V') printf("3");
         if(cond'B') printf("4");
         if(cond'N') printf("5");
         if(cond'M') printf("6");
         if(cond'L') printf("7");
         if(cond'K') printf("8");
         if(cond'J') printf("9");
      }
   } 

   printf("\n\n");
   return 0;
}
