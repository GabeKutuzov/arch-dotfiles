/* Measure the Normally distributed noise in a small region.
	Assume that the mean is zero.                           */

#include <stdio.h>
#include <math.h>
#define MAX
#include "lib.h"

main(int argc, char *argv[])
{
	IMAGE im;
	long i,j,k;
	float x, y, z;
	double mean, sd;

	if (argc < 2)
	{
	  printf ("Usage: %s <image> \n", argv[0]);
	  exit (1);
	}

	im = Input_PBM (argv[1]);
	if (im == 0)
	{
	  printf ("No input image ('%s')\n", argv[1]);
	  exit (2);
	}

/* Measure */
	k = 0;
	x = y = 0.0;
	for (i=0; i<im->info->nr; i++)
	  for (j=0; j<im->info->nc; j++)
	  {
	    x += (float)(im->data[i][j]);
	    y += (float)(im->data[i][j]) * (float)(im->data[i][j]);
	    k += 1;
	  }

/* Compute estimate - mean noise is 0 */
	if ((float)(k-1) > 0.0)
	 sd = (double)(y - x*x/(float)k)/(float)(k-1);
	else sd = 1.0;
	if (k != 0)
	 mean = (double)(x/(float)k);
	else mean = -1;
	sd = sqrt(sd);
	printf ("Image mean is %10.5f Standard deviation is %10.5f\n",
			mean, sd);

/* Now assume that the uniform level is the mean, and compute the
	mean and SD of the differences from that!                       */
	x = y = z = 0.0;
	for (i=0; i<im->info->nr; i++)
	  for (j=0; j<im->info->nc; j++)
	  {
	    z = (float)(im->data[i][j] - mean);
	    x += z;
	    y += z*z;
	  }
	if ((float)(k-1) > 0.0)
	 sd = (double)(y - x*x/(float)k)/(float)(k-1);
	else sd = 1.0;
	
	if (k != 0)
	 mean = (double)(x/(float)k);
	else mean = -1;
	
	sd = sqrt(sd);
	printf ("Noise mean is %10.5f Standard deviation is %10.5f\n",
		mean, sd);
}
