#include "RndNums.h"

static int iset = 0;
static double gset = 0.0;

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//@                                                                            @
//@     Title:  void rndseed (int seed)                                        @
//@                                                                            @
//@     Action: Sets the seed for the random number generator.                 @
//@                                                                            @
//@     Input:  seed - the random number generator seed.                       @
//@     Output: none                                                           @
//@                                                                            @
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

void rndseed (int seed) {
	srand (seed);
} /* end rndseed */

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//@                                                                            @
//@     Title:  double rnd01 ()                                                @
//@                                                                            @
//@     Action: returns a random number between 0 and 1.                       @
//@                                                                            @
//@     Input:  none                        		                       @
//@     Output: none                                                           @
//@                                                                            @
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

double rnd01 () {
	double rmax = 1.0 + (double)RAND_MAX;
	return (double)rand ()/rmax;
} /* end rnd01 */

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//@                                                                            @
//@     Title:  double rnduniform (double min, double max)                     @
//@                                                                            @
//@     Action: returns a random number uniformly distributed between min      @
//@             and max                   				                       @
//@                                                                            @
//@     Input:  min - the lower range value.  			                       @
//@     		max - the upper range value.  			                       @
//@     Output: uniform random number                                          @
//@                                                                            @
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

double rnduniform (double min, double max) {
	return min + (max - min) * rnd01 ();
} /* end rnduniform */


/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//@                                                                            @
//@     Title:  double gaussdev ()                                             @
//@                                                                            @
//@     Action: returns a random number with a gaussian distribution (0,1)     @
//@             "Algorithm from Numerical Recipes in C", Second Ed.            @
//@             Section 7.2, Page 289                                          @
//@                                                                            @
//@     Input:  none    						                               @
//@     Output: random number with the gaussian distribution (0,1).            @
//@                                                                            @
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

double gaussdev () {
	
	double fac, rsq, v1, v2;
	
	if (!iset) {
		do {
			v1 = 2 * rnd01 () - 1;		
			v2 = 2 * rnd01 () - 1;
			rsq = v1*v1 + v2*v2;
		} while (rsq >= 1.0 || rsq == 0.0);
	
		fac = (double) sqrt (-2.0*log(rsq)/rsq);
		gset = v1*fac;
		iset = 1;
		return v2*fac;
	}
	else {
		iset = 0;
		return gset;
	}
	
} /* end gaussdev */

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//@                                                                            @
//@     Title:  double rndgauss (double mean, double stdev)                    @
//@                                                                            @
//@     Action: returns a random number with a gaussian distribution.          @
//@             and max.  "Algorithm from Numerical Recipes in C", Second Ed.  @
//@             Section 7.2, Page 289                                          @
//@                                                                            @
//@     Input:  mean - the mean value of the gaussian distribution.            @
//@     		stdev - the standard deviation of the gaussian distribution.   @
//@     Output: random number with the given gaussian distribution.            @
//@                                                                            @
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

double rndgauss (double mean, double stdev) {
	
	return mean + stdev * (gaussdev ());
	
} /* end rndgauss */
