/* Pratt edge evaluation */
#define MAX
#include "lib.h"

int Nedge = 0;

float distance (float i, float j, float ii, float jj);

float distance (float i, float j, float ii, float jj)
{
	double x, y;

	if (ii > 1000 || jj > 1000) return 1000.0;
	x = (double)( (i-ii)*(i-ii) + (j-jj)*(j-jj) );
	y = sqrt (x);
	return (float)y;
}

void main (int argc, char *argv[])
{
	int   Ia = 0,		 /* Measured number of edge pixels */
	      Ii = 0,		/* Actual number of edge pixels   */
	      maxi = 0,		/* Max of Ia and Ii */
	      ii = 0,		/* Index of min distance pixel */
	      i,j, k;
	float alpha = 0.0,	/* Constant for scaling, = 1/9    */
	      sum = 0.0,	/* Sum of pixel measures          */
	      *actuali,		/* Actual i indices of edge pixels */
	      *actualj,		/* Actual j indices of edge pixels */
	      d = 0.0, 		/* Minimum distance */
	      z = 0.0;
	IMAGE x;
	char  filename[128], *p;
	char efile[128];
	FILE  *f;

/* Open the image file */
	printf ("Eval1 - Pratt edge detector evaluation.\n");

	if (argc < 3)
	{
	  printf ("USAGE: eval1 <image file> <pixel data file>\n");
	  exit (2);
	}

	x = Input_PBM (argv[1]);
	if (x == 0)
	{
	  printf ("No input image ('%s')\n", argv[1]);
	  exit (2);
	}


/* Read actual edge data from name.edg */
	strcpy (filename, argv[1]);
	strcpy (efile, argv[2]);
	f = fopen (efile, "r");
	if (f == NULL)
	{
	  printf ("Can't open the edge data file %s.\n", efile);
	  exit (1);
	}

	fscanf (f, "%d", &Ii);
	actuali = (float *)malloc (sizeof(float)*Ii);
	actualj = (float *)malloc (sizeof(float)*Ii);
	for (i=0; i<Ii; i++)
	{
	  fscanf (f, "%f", &(actuali[i]));
	  fscanf (f, "%f", &(actualj[i]));
	}
	fclose (f);

/* For Marr, Shen, and Canny - Ignore padded boundary */
	if (argc > 3)
	{
	  sscanf (argv[3], "%d", &Nedge);
	}

/* Count the actual number of pixels */
	for (i=0; i<x->info->nr; i++)
	  for (j=0; j<x->info->nc; j++)
	    if (x->data[i][j] == 0)
	      Ia++;

	alpha = 1.0/9.0;

/* Compute the expression for each edge pixel */
	for (i=0; i<x->info->nr; i++)
	  for (j=0; j<x->info->nc; j++)
	  {
	    if (x->data[i][j] == 0)
	    {
	      d = 1000;
	      for (k=0; k<Ii; k++)
	      {
		z = distance ((float)i, (float)j, actuali[k], actualj[k]);
		if (z < d) 
		{
		  d = z;
		  ii = k;
		}
	      }
	      sum += 1.0/(1.0+alpha*d*d);
	    }
	  }
	if (Nedge == 0)
	{
	  if (Ia > Ii) maxi = Ia;
	   else maxi = Ii;
	} else 
	{
	  if (Ia > Nedge) maxi = Ia;
	   else maxi = Nedge;
	}

	printf ("PRATT (E1) Edge detector evaluation: E1 = %f\n", sum/maxi);
	printf ("Image file: %s %dx%d has %d edge pixels, saw %d.\n",
		argv[1], x->info->nr, x->info->nc, Ii, Ia);
}
