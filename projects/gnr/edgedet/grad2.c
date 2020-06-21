/* Gradient edge detector - no templates */
/* G2 */

#define MAX
#include "lib.h"
#include <math.h>

void grad2 (IMAGE x);

void grad2 (IMAGE x)
{
	int i,j,k;
	double z, dx, dy;
	IMAGE y;
	int pmax=0, pmin=255, thresh = 128;

	y = newimage (x->info->nr, x->info->nc);
	for (i=1; i<x->info->nr-1; i++)
	  for (j=1; j<x->info->nc-1; j++)
	  {
	    dx = (double)(x->data[i][j+1] - x->data[i][j-1]);
	    dy = (double)(x->data[i+1][j]-x->data[i-1][j]);

	    z = sqrt(dx*dx + dy*dy);
	    if (z > 255.0) z = 255.0;
	     else if (z < 0.0) z = 0.0;
	    y->data[i][j] = (unsigned char)z;
            if (pmax < (unsigned char)z) pmax = (unsigned char)z;
            if (pmin > (unsigned char)z) pmin = (unsigned char)z;
	  }
	  thresh = (pmax-pmin)/2;

	for (i=0; i<x->info->nr; i++)
	{
	  y->data[i][0] = 0;
	  y->data[i][x->info->nc-1] = 0;
	}
	for (j=0; j<x->info->nc; j++)
	{
	  y->data[0][j] = 0;
	  y->data[x->info->nr-1][j] = 0;
	}
	for (i=0; i<x->info->nr; i++)
	  for (j=0; j<x->info->nc; j++)
	    x->data[i][j] = y->data[i][j];

/*      Threshold       */
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
	  printf ("Usage: grad2 <image> \n");
	  exit (1);
	}

	x = Input_PBM (argv[1]);
	if (x == 0)
	{
	  printf ("No input image ('%s')\n", argv[1]);
	  exit (2);
	}

	grad2 (x);

	Output_PBM (x, "grad2.pgm");
	printf ("Output is in file '%s'\n", "grad2.pgm");
}
