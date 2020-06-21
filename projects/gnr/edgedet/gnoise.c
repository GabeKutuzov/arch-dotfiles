/* Gaussian noise, zero mean. SD is given by arg 1 */

#include <stdio.h>
#include <math.h>
#define MAX
#include "lib.h"
double drand32();

double gaussian (double mean, double sd);
double negexp (double beta);

double negexp (double beta)
{
	double x;

	x = drand32();
	return -log(x);
}

double normal (double mean, double sd)
{
	static double a = 0, w2 = 0;
	double u1, u2, b, v, w1;
	int s, t;

	if (a == 1)
	{
	  a = 0;
	  return mean + w2*sd;
	}

L2:
	a = 1;
	u1 = drand32();
	u1 = u1 + u1;
	if (u1 > 1.0)
	{
	  s = 0; u1 -= 1.0;
	} else s = 1;
	u2 = drand32();
	u2 = u2+u2;
	if (u2 > 1.0)
	{
	  t = 0; u2 -= 1.0;
	} else t = 1;

	b = u1*u1 + u2*u2;
	if (b>1) goto L2;

	v = negexp(1.0);
	w1 = u1*sqrt(2*v/b);
	w2 = u2*sqrt(2.0*v/b);
	if (s==0) w1 = -w1;
	if (t == 0) w2 = -w2;
	return mean + w1*sd;
}


double gaussian (double mean, double sd)
{
	double x, y;
	static double r2p = 0.0;

	if (r2p == 0.0) r2p = 1.0/sqrt (2.0*3.1415926535);
	return drand32()*sd;
}

int main (int argc, char *argv[])
{
	int i=0, j=0, k=0, xmin=0, xmax=0, kk=0;
	double x=0.0, sd=0.0;
	int image[512][512];
	FILE *f;
	IMAGE im=0;

	if (argc < 3)
	{
	  printf ("Usage: gnoise <image> <standard deviation>\n");
	  exit (1);
	}

	im = Input_PBM (argv[1]);
	if (im == 0)
	{
	  printf ("No input image ('%s')\n", argv[1]);
	  exit (2);
	}

	sscanf (argv[2], "%lf", &sd);
	printf ("Standard deviation is %lf\n", sd);
	xmin = 10000; xmax = -xmin;

	for (i=0; i<im->info->nr; i++)
	  for (j=0; j<im->info->nc; j++)
	  {
	    x = (int)(normal(0.0, sd) + 0.5);
	    if (x > xmax) xmax = x;
	    if (x < xmin) xmin = x;
	    image[i][j] = x;
	  }
	x = xmax - xmin;
	printf ("Max is %d min =%d\n", xmax, xmin);
	k = 0;
	f = fopen ("gnoise.pgm", "w");
	fprintf (f, "P2\n%d %d 255\n", im->info->nc, im->info->nr);
	for (i=0; i<im->info->nr; i++)
	  for (j=0; j<im->info->nc; j++)
	  {
	    kk = im->data[i][j] + image[i][j];
	    if (kk < 0) kk = 0;
	    else if (kk > 255) kk = 255;
	    im->data[i][j] = (unsigned char)kk;
	    image[i][j] = image[i][j] - xmin;
	    fprintf (f, "%3d ", image[i][j]);
	    k++;
	    if (k > 15) 
	    {
	      k = 0;
	      fprintf (f, "\n");
	    }
	  }
	fprintf (f, "\n");
	fclose (f);
	printf ("Raw noise file is 'gnoise.pgm'\n");
	Output_PBM (im, "noisy.pgm");
	printf ("Noisy image file is 'noisy.pgm'\n");
}
