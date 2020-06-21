/* Sobel edge detector - no templates */

#define MAX
#include "lib.h"
#include <math.h>

void thresh (IMAGE z)
{
	int histo[256];
	int i,j,t;
	
/* Compute a grey level histogram */
	for (i=0; i<256; i++) histo[i] = 0;
	for (i=1; i<z->info->nr-1; i++)
	  for (j=1; j<z->info->nc-1; j++)
	  {
	    histo[z->data[i][j]]++;
	  }
	
/* Threshold at the middle of the occupied levels */
	i = 255; 
	while (histo[i] == 0) i--;
	j = 0;
	while (histo[j] == 0) j++;
	t = (i+j)/2;

/* Apply the threshold */
	for (i=1; i<z->info->nr-1; i++)
	  for (j=1; j<z->info->nc-1; j++)
	    if (z->data[i][j] >= t) z->data[i][j] = 0;
	    else z->data[i][j] = 255;
}

/*	Apply a Sobel edge mask to the image X	*/

void sobel (struct image *x)
{
	int i,j,n,m,k;
	IMAGE z;

	z = 0;
	copy (&z, x);

	for (i=0; i<x->info->nr; i++)
	  for (j=0; j<x->info->nc; j++) 
	    z->data[i][j] = 255;

/* Now compute the convolution, scaling */
	for (i=1; i<x->info->nr-1; i++)
	  for (j=1; j<x->info->nc-1; j++) 
	  {
	    n = (x->data[i-1][j+1]+2*x->data[i][j+1]+x->data[i+1][j+1]) -
	        (x->data[i-1][j-1]+2*x->data[i][j-1]+x->data[i+1][j-1]);
	    m = (x->data[i+1][j-1]+2*x->data[i+1][j]+x->data[i+1][j+1])-
	        (x->data[i-1][j-1]+2*x->data[i-1][j]+x->data[i-1][j+1]);
	    k = (int)( sqrt( (double)(n*n + m*m) )/4.0 );
	    z->data[i][j] = k;
	  }

	thresh (z);

	for (i=0; i<x->info->nr; i++)
	  for (j=0; j<x->info->nc; j++) 
	    x->data[i][j] = z->data[i][j];
}

main (int argc, char *argv[])
{
	IMAGE x=0;

	if (argc < 2)
	{
	  printf ("Usage: %s <image> \n", argv[0]);
	  exit (1);
	}

	x = Input_PBM (argv[1]);
	if (x == 0)
	{
	  printf ("No input image ('%s')\n", argv[1]);
	  exit (2);
	}

	sobel (x);

	Output_PBM (x, "sobel.pgm");
	printf ("Output is in file 'sobel.pgm'\n");
}
