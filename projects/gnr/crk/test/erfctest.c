/***********************************************************************
*                              erfctest                                *
*                                                                      *
*  Compute a few key values of erfc and compare them with "correct"    *
*  results obtained by running MathCad.                                *
*                                                                      *
*  Results obtained running this test with erfcf written by GNR based  *
*     on polynomial approx from Abramowitz and Stegun Handbook:        *
*  The mean absolute error is    5.9342373E-08                         *
*  The mean relative error is    4.4961189E-04                         *
*  The largest absolute error (   1.4947720E-07) occured at arg = 0.90 *
*  The largest relative error (   2.7915389E-03) occured at arg = 4.00 *
*                                                                      *
*  Results obtained running this test with d3erfc written by MC based  *
*     on IMSL library routine:                                         *
*  The mean absolute error is    9.8301518E-09                         *
*  The mean relative error is    2.6720457E-07                         *
*  The largest absolute error (   6.3977969E-08) occured at arg = 0.40 *
*  The largest relative error (   1.1615264E-06) occured at arg = 3.90 *
*                                                                      *
*  Results obtained running this test with rerfc written by GNR based  *
*     on Chebyshev approx by W.J. Cody, Mathematics of Computation:    *
*  The mean absolute error is    2.6084684E-09                         *
*  The mean relative error is    1.9159326E-07                         *
*  The largest absolute error (   1.8903189E-08) occured at arg = 0.60 *
*  The largest relative error (   7.6860743E-07) occured at arg = 3.90 *
*                                                                      *
************************************************************************
*  V1A, 02/15/97, GNR - Initial version                                *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "sysdef.h"
#include "rkarith.h"

struct et {
   double arg;
   double correct;
   double result;
   } ;

void main(void) {

   struct et tests[] = {
      { 0,      1.000000000000, },
      { 0.0001, 0.999887162084, },
      { 0.001,  0.998871621209, },
      { 0.01,   0.988716584444, },
      { 0.1,    0.887537083982, },
      { 0.2,    0.777297410790, },
      { 0.3,    0.671373240541, },
      { 0.4,    0.571607644953, },
      { 0.5,    0.479500122187, },
      { 0.6,    0.396143909152, },
      { 0.7,    0.322198806163, },
      { 0.8,    0.257899035292, },
      { 0.9,    0.203091787577, },
      { 1.0,    0.157299207050, },
      { 1.1,    0.119794930426, },
      { 1.2,    0.089686021770, },
      { 1.3,    0.065992055059, },
      { 1.4,    0.047714880237, },
      { 1.5,    0.033894853525, },
      { 1.6,    0.023651616655, },
      { 1.7,    0.016209541409, },
      { 1.8,    0.010909498364, },
      { 1.9,    0.007209570765, },
      { 2.0,    0.004677734981, },
      { 2.1,    0.002979466656, },
      { 2.2,    0.001862846298, },
      { 2.3,    0.001143176597, },
      { 2.4,    6.885138966450E-4, },
      { 2.5,    4.069520174450E-4, },
      { 2.6,    2.360344165293E-4, },
      { 2.7,    1.343327399406E-4, },
      { 2.8,    7.501319466541E-5, },
      { 2.9,    4.109787809947E-5, },
      { 3.0,    2.209049699864E-5, },
      { 3.1,    1.164865736725E-5, },
      { 3.2,    6.025761151718E-6, },
      { 3.3,    3.057709796472E-6, },
      { 3.4,    1.521993362896E-6, },
      { 3.5,    7.430983723911E-7, },
      { 3.6,    3.558629929623E-7, },
      { 3.7,    1.671510578838E-7, },
      { 3.8,    7.700392745669E-8, },
      { 3.9,    3.479224863767E-8, },
      { 4.0,    1.541725791476E-8, }};

   double abserr, relerr;
   double bigabs = 0.0, bigrel = 0.0;
   double bigabsarg = -1.0, bigrelarg = -1.0;
   double sumabs = 0.0, sumrel = 0.0;
   int it,nt = sizeof(tests)/sizeof(struct et);

   printf("Results of erfc test:\n"
      "        Argument           Value         Correct"
      "          Abserr          Relerr\n");

   for (it=0; it<nt; it++) {
      tests[it].result = (double)rerfc(tests[it].arg);
      abserr = fabs(tests[it].result - tests[it].correct);
      relerr = abserr/tests[it].correct;
      sumabs += abserr;
      sumrel += relerr;
      if (abserr > bigabs)
         { bigabs = abserr; bigabsarg = tests[it].arg; }
      if (relerr > bigrel)
         { bigrel = relerr; bigrelarg = tests[it].arg; }
      printf("%16.8E%16.8E%16.8E%16.8E%16.8E\n",
         tests[it].arg,tests[it].result,tests[it].correct,
         abserr,relerr);
      }

   printf("\nThe mean absolute error is %16.8E\n",sumabs/(double)nt);
   printf("The mean relative error is %16.8E\n",sumrel/(double)nt);
   printf("The largest absolute error (%16.8E) occured at arg = %4.2f\n",
      bigabs, bigabsarg);
   printf("The largest relative error (%16.8E) occured at arg = %4.2f\n",
      bigrel, bigrelarg);

   } /* End erfctest() */
