/* Rosenfeld edge evaluation */
#define MAX
#include "lib.h"

IMAGE x;
int neighborhood [3][3];
float table[257];
int dtable[257];

double alpha (double a, double b);
void pixdir (int k, int *a, int *b);
double L(int k, int i, int j);
double R(int k, int i, int j);
double C (int i, int j);
double T (int i, int j);
double max3 (double a, double b, double c);
void grad_init ();
int grad_table_index (IMAGE x, int row, int col);
int gdir (IMAGE x, int row, int col);
float gangle (IMAGE x, int row, int col);
void set_neighborhood (int d);

double max3 (double a, double b, double c)
{
	if (a >= b && a >= c) return a;
	if (b>= a && b >= c) return b;
	if (c>=a && c>=b) return c;
}

double alpha (double a, double b)
{
	return fabs ((180.0-fabs(a-b))/180.0);
}

void pixdir (int k, int *a, int *b)
{
	switch (k)
	{
case 0:	*a = 0;   *b =  1; break;
case 1:	*a = -1;  *b =  1; break;
case 2:	*a = -1;  *b =  0; break;
case 3:	*a = -1;  *b = -1; break;
case 4:	*a =  0;  *b = -1; break;
case 5:	*a =  1;  *b = -1; break;
case 6:	*a =  1;  *b =  0; break;
case 7:	*a =  1;  *b =  1; break;
default:	printf ("Bad direction %d. Must be 0-7.\n", k);
		exit(1);
	}
}

double L(int k, int i, int j)
{
	double d, dk, z, y;
	int ii, jj;
	
/* An edge pixel? */
	if (x->data[i][j] == 0) return 0.0;

/* Get directions */
	d = gangle(x, i, j);
	pixdir (k, &ii, &jj);
	ii += i; jj += j;
	if (x->data[ii][jj] == 0) return 0.0;
	dk = gangle (x, ii, jj);
	y = alpha (d, dk);
	z = alpha (k*45.0, d+90.0);	
	return z*y;
}

double R(int k, int i, int j)
{
	double d, dk, z, y;
	int ii, jj;
	
/* An edge pixel? */
	if (x->data[i][j] == 0) return 0.0;

/* Get directions */
	d = gangle(x, i, j);
	pixdir (k, &ii, &jj);
	ii += i; jj += j;
	if (x->data[ii][jj] == 0) return 0.0;
	dk = gangle (x, ii, jj);
	y = alpha (d, dk);
	z = alpha (k*45.0, d-90.0);     
	return z*y;
}

double C (int i, int j)
{
	int ii,jj,k, dd;
	double l1,l2,l3,r1,r2,r3, left, right;

	if (x->data[i][j] == 0) return 0.0;

	for (ii=0; ii<3; ii++)
	  for (jj=0; jj<3; jj++)
	    neighborhood[ii][jj] = 0;
	neighborhood[1][1] = 1;

	dd = gdir(x, i, j);
/* Look right */
	r1 = R((dd+6)%8, i, j);
	r2 = R((dd+5)%8, i, j);
	r3 = R((dd+7)%8, i, j);
	right = max3 (r1, r2, r3);
	if (right == r1) set_neighborhood ((dd+6)%8);
	else if (right == r2) set_neighborhood((dd+5)%8);
	else set_neighborhood ((dd+7)%8);

/* Look left */
	l1 = L((dd+2)%8, i, j);
	l2 = L((dd+3)%8, i, j);
	l3 = L((dd+1)%8, i, j);
	left = max3 (l1, l2, l3);
	if (left == l1) set_neighborhood ((dd+2)%8 );
	else if (left == l2) set_neighborhood ((dd+3)%8 );
	else set_neighborhood ((dd+1)%8 );

	return (right+left)/2.0;
}

void set_neighborhood (int d)
{
	int i, j;

	pixdir (d, &i, &j);
	neighborhood[i+1][j+1] = 1;
}

double T (int i, int j)
{
	int ii, jj, n;
	double v;

	if (x->data[i][j] == 0) return 0.0;
	n = 0;
	for (ii= 0; ii<=2; ii++)
	{
	  for (jj= 0; jj<=2; jj++)
	  {
	    if (neighborhood[ii][jj]) continue;
	    if (x->data[i+ii-1][j+jj-1] > 0) n++;
	  }
	}
	v = (double)(6-n)/6.0;
	return v;
}

void main (int argc, char *argv[])
{
	int   i,j, k;
	float gamma = 0.0,	/* Constant for scaling, = 1/9    */
	      E2 = 0.0,		/* Sum of pixel measures          */
	      d = 0.0, 		/* Minimum distance */
	      z = 0.0;
	char  filename[128], *p;
	FILE  *f;

/* Open the image file */
	printf (" Eval2 edge detector evaluation.\n");
	if (argc < 2)
	{
		printf ("Not enough args - requires a file name, PGM.\n");
		exit (2);
	}

	x = Input_PBM (argv[1]);
	if (x == 0)
	{
	  printf ("No input image ('%s')\n", argv[1]);
	  exit (2);
	}

	for (i=0; i<x->info->nr; i++)
	  for (j=0; j<x->info->nc; j++)
	    if (x->data[i][j] > 0) x->data[i][j] = 0;
	    else x->data[i][j] = 1;

/* Compute gradient direction from the edge image */
	grad_init ();

	gamma = 0.8;

	E2 = 0;  k = 0;
	for (i=2; i<x->info->nr-2; i++)
	  for (j=2; j<x->info->nc-2; j++)
	    if (x->data[i][j] == 1)
	    {
	      z = C(i,j)*gamma + T(i,j)*(1.0-gamma);
	      E2 += z;
	      k++;
	    }

	E2 = E2/(double)k;
	printf ("Eval2 (E2) Edge detector evaluation: E2 = %f\n", E2);
	printf ("Image file: %s %dx%d \n",
		argv[1], x->info->nr, x->info->nc);
}

float gangle (IMAGE x, int row, int col)
{
	int i;
	float y;

	if (x->data[row][col] == 0) return -1.0;
	i = grad_table_index (x, row, col);
	y =  table[i]-90.0;
	if (y < 0) y += 360.0;
	return y;
}

int gdir (IMAGE x, int row, int col)
{
	int i, j;

	if (x->data[row][col] == 0) return 8;
	i = grad_table_index (x, row, col);
	j = dtable[i] - 2;
	if (j < 0) j += 8;
	return j%8;
}

int grad_table_index (IMAGE x, int row, int col)
{
	int i,j,k,n,m;

/*	Get index to table.					*/

	k = 0; m= 0;
	for (i=row-1; i<=row+1; i++) {
	   for (j=col-1; j<=col+1; j++) {
		if ( (i==row) && (j==col) ) continue; 
		 else if (x->data[i][j] > 0) k += 1<<(m++);
		 else m++;
	   }
	}
	return k;
}

void grad_init ()
{
	int i,j,k;
	double ang, conv, pi;
	double dxtab[256], dytab[256];

	pi = 3.1415926535;
	conv = pi/180.0;
	i=0;
	table[i++] =  0.0;		/* 0 */
	table[i++] =  135.0;
	table[i++] =  90.0;
	table[i++] =  45.0;
	table[i++] =  45.0;
	table[i++] =  0.0;
	table[i++] =  315.0;
	table[i++] =  0.0;
	table[i++] =  0.0;
	table[i++] =  45.0;
	table[i++] =  45.0;		/* 10 */
	table[i++] =  45.0;
	table[i++] =  45.0;
	table[i++] =  45.0;
	table[i++] =  0.0;
	table[i++] =  45.0;
	table[i++] =  0.0;
	table[i++] =  315.0;
	table[i++] =  315.0;
	table[i++] =  315.0;
	table[i++] =  315.0;		/* 20 */
	table[i++] =  315.0;
	table[i++] =  315.0;
	table[i++] =  315.0;
	table[i++] =  0.0;
	table[i++] =  0.0;	/* WAS 45 */
	table[i++] =  0.0;
	table[i++] =  45.0;
	table[i++] =  0.0;	/* WAS 315 */
	table[i++] =  0.0;
	table[i++] =  315.0;		/* 30 */
	table[i++] =  0.0;
	table[i++] =  45.0;
	table[i++] =  90.0;
	table[i++] =  45.0;
	table[i++] =  45.0;
	table[i++] =  45.0;
	table[i++] =  45.0;
	table[i++] =  135.0;
	table[i++] =  45.0;
	table[i++] =  135.0;		/* 40 */
	table[i++] = 135.0;	/* WAS 90 */
	table[i++] =  135.0;
	table[i++] = 45.0;
	table[i++] =  45.0;
	table[i++] =  45.0;
	table[i++] =  45.0;
	table[i++] =  45.0;
	table[i++] =  45.0;
	table[i++] = 270.0;
	table[i++] =  335.0;		/* 50 */
	table[i++] =  45.0;
	table[i++] =  225.0;
	table[i++] = 0.0;
	table[i++] =  0.0;
	table[i++] =  0.0;
	table[i++] =  0.0;	/* WAS 135 */
	table[i++] =  0.0;
	table[i++] =  45.0;
	table[i++] =  45.0;
	table[i++] =  0.0;		/* 60 */
	table[i++] =  45.0;
	table[i++] =  0.0;
	table[i++] =  45.0;
	table[i++] =  90.0;
	table[i++] =  135.0;
	table[i++] =  90.0;		/* 66 */
	table[i++] =  135.0;
	table[i++] =  225.0;
	table[i++] =  180.0;
	table[i++] =  315.0;		/* 70 */
	table[i++] =  90.0;
	table[i++] =  135.0;
	table[i++] =  135.0;
	table[i++] =  90.0;
	table[i++] =  45.0;
	table[i++] =  180.0;
	table[i++] =  45.0;
	table[i++] =  45.0;
	table[i++] =  45.0;
	table[i++] =  225.0;		/* 80*/
	table[i++] =  45.0;
	table[i++] =  270.0;
	table[i++] =  315.0;
	table[i++] =  225.0;
	table[i++] =  315.0;
	table[i++] =  270.0;
	table[i++] =  315.0;
	table[i++] =  180.0;
	table[i++] =  135.0;
	table[i++] =  0.0;		/* 90 */
	table[i++] =  45.0;
	table[i++] =  225.0;
	table[i++] =  180.0;
	table[i++] =  315.0;
	table[i++] = 0.0;
	table[i++] =  135.0;
	table[i++] = 135.0;
	table[i++] =  135.0;
	table[i++] =  90.0;
	table[i++] =  225.0;		/* 100 */
	table[i++] =  45;
	table[i++] =  45;
	table[i++] =  45;
	table[i++] =  135;
	table[i++] =  135;
	table[i++] = 90;
	table[i++] =  90;
	table[i++] =  90;
	table[i++] =  90;
	table[i++] =  90;		/* 110 */
	table[i++] =  45;
	table[i++] =  225;
	table[i++] =  135;
	table[i++] =  225;
	table[i++] = 270.0;
	table[i++] =  225;
	table[i++] =  45;
	table[i++] =  225;
	table[i++] =  270;
	table[i++] = 180;		/* 120 */
	table[i++] =  135;
	table[i++] =  135;
	table[i++] =  90;
	table[i++] =  180;
	table[i++] =  180;
	table[i++] =  225;
	table[i++] =  45;
	table[i++] =  135;
	table[i++] =  135;
	table[i++] =  315;	/* 130 */
	table[i++] =  315;
	table[i++] =  270;
	table[i++] =  315;
	table[i++] =  315;
	table[i++] =  315;
	table[i++] =  135;
	table[i++] =  135;
	table[i++] =  225.0;
	table[i++] =  90;
	table[i++] =  0;	/* 140 */
	table[i++] =  315;
	table[i++] =  315;
	table[i++] =  0;
	table[i++] =  225;
	table[i++] =  315;
	table[i++] =  315;
	table[i++] =  315;
	table[i++] =  270;
	table[i++] =  315;
	table[i++] =  315;	/* 150 */
	table[i++] =  315;
	table[i++] =  0.0;	/* WAS 225 */
	table[i++] =  180;
	table[i++] =  315;
	table[i++] =  0;
	table[i++] =  0;
	table[i++] =  315;
	table[i++] =  315;
	table[i++] =  315;
	table[i++] =  180;	/* 160 */
	table[i++] =  135;
	table[i++] =  0;
	table[i++] =  135;
	table[i++] =  225;
	table[i++] =  0;
	table[i++] =  225;
	table[i++] =  0;
	table[i++] =  135;
	table[i++] =  135;
	table[i++] =  135.0;	/* 170 */
	table[i++] =  90;
	table[i++] =  225;
	table[i++] =  90;
	table[i++] = 225;
	table[i++] =  90;
	table[i++] =  225;
	table[i++] =  135;
	table[i++] = 225;
	table[i++] = 135;
	table[i++] =  225;	/* 180 */
	table[i++] =  270;
	table[i++] =  225;
	table[i++] =  0;
	table[i++] =  180;
	table[i++] =  135;
	table[i++] =  0;
	table[i++] =  0;
	table[i++] =  225;
	table[i++] =  0;
	table[i++] =  0.0;	/* 190 */
	table[i++] =  0;
	table[i++] =  225;
	table[i++] =  135;
	table[i++] =  270;
	table[i++] =  90;
	table[i++] =  225;
	table[i++] =  315;
	table[i++] = 270;
	table[i++] =  90.0;
	table[i++] =  135;	/* 200 */
	table[i++] =  135.0;
	table[i++] =  135;
	table[i++] =  135.0;
	table[i++] =  225;
	table[i++] =  315;
	table[i++] =  90;
	table[i++] =  315;
	table[i++] =  225;
	table[i++] = 180;
	table[i++] =  270;	/* 210 */
	table[i++] =  315;
	table[i++] =  225;
	table[i++] =  315;
	table[i++] =  270;
	table[i++] =  315;
	table[i++] =  180;
	table[i++] =  315;
	table[i++] =  225;
	table[i++] =  315;
	table[i++] =  225;	/* 220 */
	table[i++] =  315;
	table[i++] =  270;
	table[i++] =  315;
	table[i++] =  180.0;
	table[i++] =  135;
	table[i++] =  90;
	table[i++] = 135;
	table[i++] =  225;
	table[i++] =  180;
	table[i++] =  225;	/* 230 */
	table[i++] =  90;
	table[i++] =  135;
	table[i++] =  135;
	table[i++] =  135;
	table[i++] =  135;
	table[i++] =  225;
	table[i++] =  180;
	table[i++] =  225;
	table[i++] =  90;
	table[i++] =  225;	/* 240 */
	table[i++] =  180;
	table[i++] = 225;
	table[i++] =  135;
	table[i++] =  225;
	table[i++] =  270.0;
	table[i++] =  225.0;	/* WAS 270 */
	table[i++] =  270.0;
	table[i++] =  180.0;
	table[i++] =  135.0;
	table[i++] =  180.0;	/* 250 */
	table[i++] =  135.0;
	table[i++] =  225.0;
	table[i++] =  180.0;
	table[i++] =  225.0;
	table[i++] =  0.0;

	for (i=0; i<256; i++) {
	  ang = table[i];

	  if ( (ang<=22.5) || (ang>=337.5) ) dtable[i] = 0; 	  /* 0 sector */
	  else if ( (ang>=22.5) && (ang<=67.5) ) dtable[i] = 1;	  /* 1 sector */
	  else if ( (ang>=67.5) && (ang<=112.5) ) dtable[i] = 2;  /* 2 sector */
	  else if ( (ang>=112.5) && (ang<=157.5) ) dtable[i] = 3; /* 3 sector */
	  else if ( (ang>=157.5) && (ang<=202.5) ) dtable[i] = 4; /* 4 sector */
	  else if ( (ang>=202.5) && (ang<=247.5) ) dtable[i] = 5; /* 5 sector */
	  else if ( (ang>=247.5) && (ang<=292.5) ) dtable[i] = 6; /* 6 sector */
	  else dtable[i] = 7;					  /* 7 sector */
	}
}

