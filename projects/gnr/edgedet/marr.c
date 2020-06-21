/* Marr/Hildreth edge detection */

#include <math.h>
#include <stdio.h>
#define MAX
#include "lib.h"

float ** f2d (int nr, int nc);
void convolution (IMAGE im, float **mask, int nr, int nc, float **res,
	int NR, int NC);
float gauss(float x, float sigma);
float LoG (float x, float sigma);
float meanGauss (float x, float sigma);
void marr (float s, IMAGE im);
void dolap (float **x, int nr, int nc, float **y);
void zero_cross (float **lapim, IMAGE im);
float norm (float x, float y);
float distance (float a, float b, float c, float d);

void main (int argc, char *argv[])
{
	int i,j,n;
	float s=1.0;
	FILE *params;
	IMAGE im1, im2;


/* Read parameters from the file marr.par */
	if (argc > 2)
	  sscanf (argv[2], "%f", &s);
	else
	{
	  params = fopen ("marr.par", "r");
	  if (params)
	  {
	    fscanf (params, "%f", &s);	/* Gaussian standard deviation */
	    fclose (params);
	  }
	}
	printf ("Standard deviation= %lf\n", s);

/* Command line: input file name */
	if (argc < 2)
	{
	  printf ("USAGE: marr <filename> <standard deviation>\n");
	  printf ("Marr edge detector - reads a PGM format file and\n");
	  printf (" detects edges, creating 'marr.pgm'.\n");
	  exit (1);
	}

	im1 = Input_PBM (argv[1]);
	if (im1 == 0)
	{
	  printf ("No input image ('%s')\n", argv[1]);
	  exit (2);
	}

	im2 = newimage (im1->info->nr, im1->info->nc);
	for (i=0; i<im1->info->nr; i++)
	  for (j=0; j<im1->info->nc; j++)
	    im2->data[i][j] = im1->data[i][j];

/* Apply the filter */
	marr (s-0.8, im1);
	marr (s+0.8, im2);
    
	for (i=0; i<im1->info->nr; i++)
	  for (j=0; j<im1->info->nc; j++)
	    if (im1->data[i][j] > 0 && im2->data[i][j] > 0)
		im1->data[i][j] = 0;
	    else im1->data[i][j] = 255;

	Output_PBM (im1, "marr.pgm");
	printf ("Done. File is 'marr.pgm'.\n");
}

float norm (float x, float y)
{
	return (float) sqrt ( (double)(x*x + y*y) );
}

float distance (float a, float b, float c, float d)
{
	return norm ( (a-c), (b-d) );
}

void marr (float s, IMAGE im)
{
	int width;
	float **smx;
	int i,j,k,n;
	float **lgau, z;

/* Create a Gaussian and a derivative of Gaussian filter mask  */
	width = 3.35*s + 0.33;
	n = width+width + 1;
	printf ("Smoothing with a Gaussian of size %dx%d\n", n, n);
	lgau = f2d (n, n);
	for (i=0; i<n; i++)
	  for (j=0; j<n; j++)
	    lgau[i][j] = LoG (distance ((float)i, (float)j,
			 (float)width, (float)width), s);

/* Convolution of source image with a Gaussian in X and Y directions  */
	smx = f2d (im->info->nr, im->info->nc);
	printf ("Convolution with LoG:\n");
	convolution (im, lgau, n, n, smx, im->info->nr, im->info->nc);

/* Locate the zero crossings */
	printf ("Zero crossings:\n");
	zero_cross (smx, im);

/* Clear the boundary */
	for (i=0; i<im->info->nr; i++)
	{
	  for (j=0; j<=width; j++) im->data[i][j] = 0;
	  for (j=im->info->nc-width-1; j<im->info->nc; j++)
		im->data[i][j] = 0;
	}
	for (j=0; j<im->info->nc; j++)
	{
	  for (i=0; i<= width; i++) im->data[i][j] = 0;
	  for (i=im->info->nr-width-1; i<im->info->nr; i++)
		im->data[i][j] = 0;
	}

	free(smx[0]); free(smx);
	free(lgau[0]); free(lgau);
}

/*	Gaussian	*/
float gauss(float x, float sigma)
{
    return (float)exp((double) ((-x*x)/(2*sigma*sigma)));
}

float meanGauss (float x, float sigma)
{
	float z;

	z = (gauss(x,sigma)+gauss(x+0.5,sigma)+gauss(x-0.5,sigma))/3.0;
	z = z/(PI*2.0*sigma*sigma);
	return z;
}

float LoG (float x, float sigma)
{
	float x1;

	x1 = gauss (x, sigma);
	return (x*x-2*sigma*sigma)/(sigma*sigma*sigma*sigma) * x1;
}

/*
float ** f2d (int nr, int nc)
{
	float **x, *y;
	int i;

	x = (float **)calloc ( nr, sizeof (float *) );
	y = (float *) calloc ( nr*nc, sizeof (float)  );
	if ( (x==0) || (y==0) )
	{
	  fprintf (stderr, "Out of storage: F2D.\n");
	  exit (1);
	}
	for (i=0; i<nr; i++)
	  x[i] = y+i*nc;
	return x;
}
*/

void convolution (IMAGE im, float **mask, int nr, int nc, float **res,
	int NR, int NC)
{
	int i,j,ii,jj, n, m, k, kk;
	float x, y;

	k = nr/2; kk = nc/2;
	for (i=0; i<NR; i++)
	  for (j=0; j<NC; j++)
	  {
	    x = 0.0;
	    for (ii=0; ii<nr; ii++)
	    {
	      n = i - k + ii;
	      if (n<0 || n>=NR) continue;
	      for (jj=0; jj<nc; jj++)
	      {
		m = j - kk + jj;
		if (m<0 || m>=NC) continue;
		x += mask[ii][jj] * (float)(im->data[n][m]);
	      }
	    }
	    res[i][j] = x;
	  }
}

void zero_cross (float **lapim, IMAGE im)
{
	int i,j,k,n,m, dx, dy;
	float x, y, z;
	int xi,xj,yi,yj, count = 0;
	IMAGE deriv;

	for (i=1; i<im->info->nr-1; i++)
	  for (j=1; j<im->info->nc-1; j++)
	  {
	    im->data[i][j] = 0;
	    if(lapim[i-1][j]*lapim[i+1][j]<0) {im->data[i][j]=255; continue;}
	    if(lapim[i][j-1]*lapim[i][j+1]<0) {im->data[i][j]=255; continue;}
	    if(lapim[i+1][j-1]*lapim[i-1][j+1]<0) {im->data[i][j]=255; continue;}
	    if(lapim[i-1][j-1]*lapim[i+1][j+1]<0) {im->data[i][j]=255; continue;}
	  }
}

/*	An alternative way to compute a Laplacian	*/
void dolap (float **x, int nr, int nc, float **y)
{
	int i,j,k,n,m;
	float u,v;

	for (i=1; i<nr-1; i++)
	  for (j=1; j<nc-1; j++)
	  {
	    y[i][j] = (x[i][j+1]+x[i][j-1]+x[i-1][j]+x[i+1][j]) - 4*x[i][j];
	    if (u>y[i][j]) u = y[i][j];
	    if (v<y[i][j]) v = y[i][j];
	  }
}
