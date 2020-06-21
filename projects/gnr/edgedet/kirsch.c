/* Kirsch edge detector - no templates */

#define MAX
#include "lib.h"
#include <math.h>

int di[8] = {0, -1, -1, -1, 0, 1, 1, 1};
int dj[8] = {1, 1, 0, -1, -1, -1, 0, 1};

void kirsch (IMAGE x);
int apply_mask (IMAGE im, int K, int row, int col);
int all_masks (IMAGE im, int row, int col);
void thresh (IMAGE z);

/*	Apply a Sobel edge mask to the image X	*/

void kirsch (IMAGE x)
{
	int i,j,n,m,k,rmax,rmin;
	IMAGE z;
	float rng, zmax, zmin;

	z = newimage (x->info->nr, x->info->nc);
	for (i=0; i<x->info->nr; i++)
	  for (j=0; j<x->info->nc; j++) 
	    z->data[i][j] = 255;

	zmax = 0; zmin = 1000;
	for (i=1; i<x->info->nr-1; i++)
	  for (j=1; j<x->info->nc-1; j++) 
	  {
	    if (i>60 && j >= 126)
	    {
		rng = 0.0;
	    }
	    rng = (float)(all_masks(x, i, j)/15.0);
	    if (zmax < rng) zmax = rng;
	    if (zmin > rng) zmin = rng;
	    z->data[i][j] = (unsigned char)rng;
	  }
	printf ("Max value seen was %f min was %f\n", zmax, zmin);

	thresh (z);

	for (i=0; i<x->info->nr; i++)
	   for (j=0; j<x->info->nc; j++) 
		x->data[i][j] = z->data[i][j];
}

/* Apply Kirsch mask K to pixel (row, col) */

int apply_mask (IMAGE im, int K, int row, int col)
{
	int i,j,k,n,m;

	n = (int)(im->data[di[K]+row][dj[K]+col])
		+ (int)(im->data[di[(K+1)%8]+row][dj[(K+1)%8]+col])
		+ (int)(im->data[di[(K+7)%8]+row][dj[(K+7)%8]+col]);
	m = 0;
	for (i=2; i<7; i++)
	  m += (int)(im->data[di[(K+i)%8]+row][dj[(K+i)%8]+col]);
	k = abs(n*5 - m*3);
	return k;
}

/* Apply ALL masks to pixel (row, col) and return the largest */

int all_masks (IMAGE im, int row, int col)
{
	int resp[8], k, kb;

	kb = 0;
	for (k=0; k<8; k++)
	{
	  resp[k] = apply_mask (im, k, row, col);
	  if (resp[k] > resp[kb]) kb = k;
	}
	return resp[kb];
}

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
	if (i-j <= 1) t = i;
	else t = (i+j)/2;
 
 /* Apply the threshold */
	for (i=1; i<z->info->nr-1; i++)
	  for (j=1; j<z->info->nc-1; j++)
	    if (z->data[i][j] >= t) z->data[i][j] = 0;
	    else z->data[i][j] = 255;
 }
 
main (int argc, char *argv[])
{
	IMAGE x=0;

	if (argc < 2)
	{
	  printf ("Usage: kirsch <image> \n");
	  exit (1);
	}

	x = Input_PBM (argv[1]);
	if (x == 0)
	{
	  printf ("No input image ('%s')\n", argv[1]);
	  exit (2);
	}

	kirsch (x);

	Output_PBM (x, "kirsch.pgm");
	printf ("Output is in file 'kirsch.pgm'\n");
}
