/**
 * @file RndNums.h
 *
 * Set of routines for generating random numbers.   
 */
#ifndef RANDNUMS
#define RANDNUMS

#include <stdlib.h>
#include <math.h>

void rndseed (int seed);
double rnd01 ();
double rnduniform (double min, double max);
double gaussdev ();
double rndgauss (double mean, double stdev);

#endif
