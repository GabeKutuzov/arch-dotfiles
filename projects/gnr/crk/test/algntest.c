/* (c) Copyright 2000, George N. Reeke, Jr. */
/*---------------------------------------------------------------------*
*      algntest.c -- determine alignment enforced by C compiler        *
*                                                                      *
*  Running this program under TURBOC, LINUX gcc, SUNOS acc, and        *
*  SOLARIS cc gives same result in all cases--no alignment is en-      *
*  forced on the chars in or after the inner structure.  Presumably,   *
*  if any longer items were added in those locations, they would be    *
*  aligned individually according to the policy of each compiler.      *
*                                                                      *
*  V1A, 08/04/99, G. N. Reeke                                          *
*  V1B, 03/03/00, GNR - Test alignment following anonymous struct      *
*---------------------------------------------------------------------*/

/* Include standard library functions */

#define MAIN
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

main() {

/* Declarations: */

   struct {
      double d1;
      char   c1;
      struct {
         char c2;
         char c3;
         } inner;
      char   c4;
      } outer;

   printf("\nThe outer structure is at %x\n", &outer);
   printf("\nThe inner structure is at %x\n", &outer.inner);
   printf("\nThe char after the inner is at %x\n", &outer.c4);

   } /* End algntest */
