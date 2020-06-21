/* Gradient edge detector - no templates */
/* G1 */

#define MAX
#include "lib.h"
#include <math.h>

void grad1 (IMAGE x);

void grad1 (IMAGE x)
{
	int i,j,k;
	double z, dx, dy;
	int pmax=0, pmin=255, thresh = 128;

	for (i=x->info->nr-1; i>= 1; i--)
	  for (j=x->info->nc-1; j>=1; j--)
	  {
	    dx = (x->data[i][j]-x->data[i-1][j]);
	    dy = (x->data[i][j]-x->data[i][j-1]);
	    z = sqrt(dx*dx + dy*dy);

	    if (z > 255.0) z = 255.0;
	     else if (z < 0.0) z = 0.0;
	    x->data[i][j] = (unsigned char)z;
	    if (pmax < (unsigned char)z) pmax = (unsigned char)z;
	    if (pmin > (unsigned char)z) pmin = (unsigned char)z;
	  }
	thresh = (pmax-pmin)/2;

	for (i=0; i<x->info->nr; i++)
	{
	  x->data[i][0] = 0;
	  x->data[i][x->info->nc-1] = 0;
	}
	for (j=0; j<x->info->nc; j++)
	{
	  x->data[0][j] = 0;
	  x->data[x->info->nr-1][j] = 0;
	}

/*	Threshold	*/
	for (i=0; i<x->info->nr; i++)
	  for (j=0; j<x->info->nc; j++)
	    if (x->data[i][j] > thresh) x->data[i][j] = 0;
	     else x->data[i][j] = 255;
}

main (int argc, char *argv[])
{
	IMAGE x=0;

	if (argc < 2)
	{
	  printf ("Usage: grad1 <image> \n");
	  exit (1);
	}

	x = Input_PBM (argv[1]);
	if (x == 0)
	{
	  printf ("No input image ('%s')\n", argv[1]);
	  exit (2);
	}

	grad1 (x);

	Output_PBM (x, "grad1.pgm");
	printf ("Output is in file 'grad1.pgm'\n");
}
