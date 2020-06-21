#include <stdio.h>
#include <string.h>
#include <math.h>
#define MAX
#include "lib.h"

#define OUTLINE 25

/* Function prototypes */
void main( int argc, char **argv);
void shen(IMAGE im, IMAGE res);
void compute_ISEF (float **x, float **y, int nrows, int ncols);
float ** f2d (int nr, int nc);
void apply_ISEF_vertical (float **x, float **y, float **A, float **B, 
			  int nrows, int ncols);
void apply_ISEF_horizontal (float **x, float **y, float **A, float **B, 
			    int nrows, int ncols);
IMAGE compute_bli (float **buff1, float **buff2, int nrows, int ncols);
void locate_zero_crossings (float **orig, float **smoothed, IMAGE bli, 
			    int nrows, int ncols);
void threshold_edges (float **in, IMAGE out, int nrows, int ncols);
int mark_connected (int i, int j,int level);
int is_candidate_edge (IMAGE buff, float **orig, int row, int col);
float compute_adaptive_gradient (IMAGE BLI_buffer, float **orig_buffer, 
				 int row, int col);
void estimate_thresh (double *low, double *hi, int nr, int nc);
void debed (IMAGE im, int width);
void embed (IMAGE im, int width);

/* globals for shen operator*/
double b = 0.9;				/* smoothing factor 0 < b < 1 */
double low_thresh=20, high_thresh=22;	/* threshold for hysteresis */
double ratio = 0.99;
int window_size = 7;
int do_hysteresis = 1;
float **lap;			/* keep track of laplacian of image */
int nr, nc;			/* nrows, ncols */
IMAGE edges;			/* keep track of edge points (thresholded) */
int thinFactor;

void main(int argc, char **argv)
{
	int i,j,n,m;
	IMAGE im, res;
	FILE *params;
   
/* Command line args - file name, maybe sigma */
	if (argc < 2)
	{
	  printf ("USAGE: shen <inmagefile>\n");
	  exit (1);
	}
	im = Input_PBM (argv[1]);
	if (im == 0)
	{
	  printf ("Can't read input image from '%s'.\n", argv[1]);
	  exit (2);
	}

/* Look for parameter file */
	params = fopen ("shen.par", "r");
	if (params)
	{
	  fscanf (params, "%lf", &ratio);
	  fscanf (params, "%lf", &b);
	  if (b<0) b = 0;
	    else if (b>1.0) b = 1.0;
	  fscanf (params, "%d", &window_size);
	  fscanf (params, "%d", &thinFactor);
	  fscanf (params, "%d", &do_hysteresis);

	  printf ("Parameters:\n");
	  printf (" %% of pixels to be above HIGH threshold: %7.3f\n", ratio);
	  printf (" Size of window for adaptive gradient  :  %3d\n", 
			window_size);
	  printf (" Thinning factor                       : %d\n", thinFactor);
	  printf ("Smoothing factor                       : %7.4f\n", b);
	  if (do_hysteresis) printf ("Hysteresis thresholding turned on.\n");
	    else printf ("Hysteresis thresholding turned off.\n");
	  fclose (params);
	}
	else printf ("Parameter file 'shen.par' does not exist.\n");

	embed (im, OUTLINE);

	res = newimage (im->info->nr, im->info->nc);
	shen (im, res);

	debed (res, OUTLINE);

	Output_PBM (res, "shen.pgm");
	printf ("Output file is 'shen.pgm'\n");
}

void shen (IMAGE im, IMAGE res)
{
	register int i,j;
	float **buffer;	
	float **smoothed_buffer;
	IMAGE bli_buffer;
   
/* Convert the input image to floating point */
	buffer = f2d (im->info->nr, im->info->nc);
	for (i=0; i<im->info->nr; i++)
	  for (j=0; j<im->info->nc; j++)
	    buffer[i][j] = (float)(im->data[i][j]);

/* Smooth input image using recursively implemented ISEF filter */
	smoothed_buffer =  f2d( im->info->nr,  im->info->nc);
	compute_ISEF (buffer, smoothed_buffer, im->info->nr, im->info->nc);
      
/* Compute bli image band-limited laplacian image from smoothed image */
	bli_buffer = compute_bli(smoothed_buffer,
			buffer,im->info->nr,im->info->nc); 
      
/* Perform edge detection using bli and gradient thresholding */
	locate_zero_crossings (buffer, smoothed_buffer, bli_buffer, 
			     im->info->nr, im->info->nc);
      
	free(smoothed_buffer[0]); free(smoothed_buffer);
	freeimage (bli_buffer);
	
	threshold_edges (buffer, res, im->info->nr, im->info->nc);
	for (i=0; i<im->info->nr; i++)
	  for (j=0; j<im->info->nc; j++)
	    if (res->data[i][j] > 0) res->data[i][j] = 0;
	     else res->data[i][j] = 255;
	
	free(buffer[0]); free(buffer);
}

/*	Recursive filter realization of the ISEF 
	(Shen and Castan CVIGP March 1992)	 */
void compute_ISEF (float **x, float **y, int nrows, int ncols)
{
	float **A, **B;
   
	A = f2d(nrows, ncols); /* store causal component */
	B = f2d(nrows, ncols); /* store anti-causal component */
   
/* first apply the filter in the vertical direcion (to the rows) */
	apply_ISEF_vertical (x, y, A, B, nrows, ncols);
   
/* now apply the filter in the horizontal direction (to the columns) and */
/* apply this filter to the results of the previous one */
	apply_ISEF_horizontal (y, y, A, B, nrows, ncols);
   
   /* free up the memory */
	free (B[0]); free(B);
	free (A[0]); free(A);
}

void apply_ISEF_vertical (float **x, float **y, float **A, float **B, 
			  int nrows, int ncols)
{
	register int row, col;
	float b1, b2;
   
	b1 = (1.0 - b)/(1.0 + b);
	b2 = b*b1;
   
/* compute boundary conditions */
	for (col=0; col<ncols; col++)
	{

/* boundary exists for 1st and last column */
	   A[0][col] = b1 * x[0][col];	
	   B[nrows-1][col] = b2 * x[nrows-1][col];
	}
   
/* compute causal component */
	for (row=1; row<nrows; row++)
	  for (col=0; col<ncols; col++)
	    A[row][col] = b1 * x[row][col] + b * A[row-1][col];

/* compute anti-causal component */
	for (row=nrows-2; row>=0; row--)
	  for (col=0; col<ncols; col++)
	    B[row][col] = b2 * x[row][col] + b * B[row+1][col];

/* boundary case for computing output of first filter */
	for (col=0; col<ncols-1; col++)
	  y[nrows-1][col] = A[nrows-1][col]; 

/* now compute the output of the first filter and store in y */
/* this is the sum of the causal and anti-causal components */
	for (row=0; row<nrows-2; row++)
	  for (col=0; col<ncols-1; col++)
	    y[row][col] = A[row][col] + B[row+1][col];
}  


void apply_ISEF_horizontal (float **x, float **y, float **A, float **B, 
			    int nrows, int ncols)
{
	register int row, col;
	float b1, b2;
   
	b1 = (1.0 - b)/(1.0 + b);
	b2 = b*b1;
   
/* compute boundary conditions */
	for (row=0; row<nrows; row++)
	{
	   A[row][0] = b1 * x[row][0];
	   B[row][ncols-1] = b2 * x[row][ncols-1];
	}

/* compute causal component */
	for (col=1; col<ncols; col++)
	  for (row=0; row<nrows; row++)
	    A[row][col] = b1 * x[row][col] + b * A[row][col-1];

/* compute anti-causal component */
	for (col=ncols-2; col>=0; col--)
	  for (row=0; row<nrows;row++)
	    B[row][col] = b2 * x[row][col] + b * B[row][col+1];

/* boundary case for computing output of first filter */     
	for (row=0; row<nrows; row++)
	  y[row][ncols-1] = A[row][ncols-1];

/* now compute the output of the second filter and store in y */
/* this is the sum of the causal and anti-causal components */
	for (row=0; row<nrows; row++)
	  for (col=0; col<ncols-1; col++)
	    y[row][col] = A[row][col] + B[row][col+1];
}  

/* compute the band-limited laplacian of the input image */
IMAGE compute_bli (float **buff1, float **buff2, int nrows, int ncols)
{
	register int row, col;
	IMAGE bli_buffer;
   
	bli_buffer = newimage(nrows, ncols);
	for (row=0; row<nrows; row++)
	  for (col=0; col<ncols; col++)
	    bli_buffer->data[row][col] = 0;
   
/* The bli is computed by taking the difference between the smoothed image */
/* and the original image.  In Shen and Castan's paper this is shown to */
/* approximate the band-limited laplacian of the image.  The bli is then */
/* made by setting all values in the bli to 1 where the laplacian is */
/* positive and 0 otherwise. 						*/
	for (row=0; row<nrows; row++)
	  for (col=0; col<ncols; col++)
	  {
            if (row<OUTLINE || row >= nrows-OUTLINE ||
                  col<OUTLINE || col >= ncols-OUTLINE) continue;
	    bli_buffer->data[row][col] = 
		((buff1[row][col] - buff2[row][col]) > 0.0);
	  }
	return bli_buffer;
}

void locate_zero_crossings (float **orig, float **smoothed, IMAGE bli, 
			    int nrows, int ncols)
{
	register int row, col;
   
	for (row=0; row<nrows; row++)
	{
	   for (col=0; col<ncols; col++)
	   {

/* ignore pixels around the boundary of the image */
	   if (row<OUTLINE || row >= nrows-OUTLINE ||
		 col<OUTLINE || col >= ncols-OUTLINE)
	   {
	     orig[row][col] = 0.0;
	   }

 /* next check if pixel is a zero-crossing of the laplacian  */
	   else if (is_candidate_edge (bli, smoothed, row, col))
	   {

/* now do gradient thresholding  */
	      float grad = compute_adaptive_gradient (bli, smoothed, row, col);
	      orig[row][col] = grad;
	   }
	   else  orig[row][col] = 0.0;		    
	  }
	}
}

void threshold_edges (float **in, IMAGE out, int nrows, int ncols)
{
	register int i, j;
   
	lap = in;
	edges = out;
	nr = nrows;
	nc = ncols;
   
	estimate_thresh (&low_thresh, &high_thresh, nr, nc);
	if (!do_hysteresis)
	  low_thresh = high_thresh;

	for (i=0; i<nrows; i++)
	  for (j=0; j<ncols; j++)
	    edges->data[i][j] = 0;
   
	for (i=0; i<nrows; i++)
	  for (j=0; j<ncols; j++)
	  {
            if (i<OUTLINE || i >= nrows-OUTLINE ||
                  j<OUTLINE || j >= ncols-OUTLINE) continue;

/* only check a contour if it is above high_thresh */
	    if ((lap[i][j]) > high_thresh) 

/* mark all connected points above low thresh */
	      mark_connected (i,j,0);	
	  }
   
	for (i=0; i<nrows; i++)	/* erase all points which were 255 */
	  for (j=0; j<ncols; j++)
	    if (edges->data[i][j] == 255) edges->data[i][j] = 0;
}

/*	return true if it marked something */ 
int mark_connected (int i, int j, int level)
{
	 int notChainEnd;
    
   /* stop if you go off the edge of the image */
	if (i >= nr || i < 0 || j >= nc || j < 0) return 0;
   
   /* stop if the point has already been visited */
	if (edges->data[i][j] != 0) return 0;	
   
   /* stop when you hit an image boundary */
	if (lap[i][j] == 0.0) return 0;
   
	if ((lap[i][j]) > low_thresh)
	{
	   edges->data[i][j] = 1;
	}
	else
	{
	   edges->data[i][j] = 255;
	}
   
	notChainEnd =0;
    
	notChainEnd |= mark_connected(i  ,j+1, level+1);
	notChainEnd |= mark_connected(i  ,j-1, level+1);
	notChainEnd |= mark_connected(i+1,j+1, level+1);
	notChainEnd |= mark_connected(i+1,j  , level+1);
	notChainEnd |= mark_connected(i+1,j-1, level+1);
	notChainEnd |= mark_connected(i-1,j-1, level+1);
	notChainEnd |= mark_connected(i-1,j  , level+1);
	notChainEnd |= mark_connected(i-1,j+1, level+1);

	if (notChainEnd && ( level > 0 ) )
	{
	/* do some contour thinning */
	  if ( thinFactor > 0 )
	  if ( (level%thinFactor) != 0  )
	  {
	    /* delete this point */  
	    edges->data[i][j] = 255;
	  }
	}
    
	return 1;
}

/* finds zero-crossings in laplacian (buff)  orig is the smoothed image */
int is_candidate_edge (IMAGE buff, float **orig, int row, int col)
{
/* test for zero-crossings of laplacian then make sure that zero-crossing */
/* sign correspondence principle is satisfied.  i.e. a positive z-c must */
/* have a positive 1st derivative where positive z-c means the 2nd deriv */
/* goes from positive to negative as we pass through the step edge */
   
	if (buff->data[row][col] == 1 && buff->data[row+1][col] == 0) /* positive z-c */
	{ 
	   if (orig[row+1][col] - orig[row-1][col] > 0) return 1;
	   else return 0;
	}
	else if (buff->data[row][col] == 1 && buff->data[row][col+1] == 0 ) /* positive z-c */
	{
	   if (orig[row][col+1] - orig[row][col-1] > 0) return 1;
	   else return 0;
	}
	else if ( buff->data[row][col] == 1 && buff->data[row-1][col] == 0) /* negative z-c */
	{
	   if (orig[row+1][col] - orig[row-1][col] < 0) return 1;
	   else return 0;
	}
	else if (buff->data[row][col] == 1 && buff->data[row][col-1] == 0 ) /* negative z-c */
	{
	   if (orig[row][col+1] - orig[row][col-1] < 0) return 1;
	   else return 0;
	}
	else			/* not a z-c */
	  return 0;
}

float compute_adaptive_gradient (IMAGE BLI_buffer, float **orig_buffer, 
				 int row, int col)
{
	register int i, j;
	float sum_on, sum_off;
	float avg_on, avg_off;
	int num_on, num_off;
   
	sum_on = sum_off = 0.0;
	num_on = num_off = 0;
   
	for (i= (-window_size/2); i<=(window_size/2); i++)
	{
	   for (j=(-window_size/2); j<=(window_size/2); j++)
	   {
	     if (BLI_buffer->data[row+i][col+j])
	     {
	        sum_on += orig_buffer[row+i][col+j];
	        num_on++;
	     }
	     else
	     {
	        sum_off += orig_buffer[row+i][col+j];
	        num_off++;
	     }
	   }
	}
   
	if (sum_off) avg_off = sum_off / num_off;
	else avg_off = 0;
   
	if (sum_on) avg_on = sum_on / num_on;
	else avg_on = 0;
   
	return (avg_off - avg_on);
}

void estimate_thresh (double *low, double *hi, int nr, int nc)
{
	float vmax, vmin, scale, x;
	int i,j,k, hist[256], count;

/* Build a histogram of the Laplacian image. */
	vmin = vmax = fabs((float)(lap[20][20]));
	for (i=0; i<nr; i++)
	  for (j=0; j<nc; j++)
	  {
            if (i<OUTLINE || i >= nr-OUTLINE ||
                  j<OUTLINE || j >= nc-OUTLINE) continue;
	    x = lap[i][j];
	    if (vmin > x) vmin = x;
	    if (vmax < x) vmax = x;
	  }
	for (k=0; k<256; k++) hist[k] = 0;

	scale = 256.0/(vmax-vmin + 1);

	for (i=0; i<nr; i++)
	  for (j=0; j<nc; j++)
	  {
            if (i<OUTLINE || i >= nr-OUTLINE ||
                  j<OUTLINE || j >= nc-OUTLINE) continue;
	    x = lap[i][j];
	    k = (int)((x - vmin)*scale);
	    hist[k] += 1;
	  }

/* The high threshold should be > 80 or 90% of the pixels */
	k = 255;
	j = (int)(ratio*nr*nc);
	count = hist[255];
	while (count < j)
	{
	  k--;
	  if (k<0) break;
	  count += hist[k];
	}
	*hi = (double)k/scale   + vmin ;
	*low = (*hi)/2;
}

void embed (IMAGE im, int width)
{
	int i,j,I,J;
	IMAGE new;

	width += 2;
	new = newimage (im->info->nr+width+width, im->info->nc+width+width);
	for (i=0; i<new->info->nr; i++)
	  for (j=0; j<new->info->nc; j++)
	  {
	    I = (i-width+im->info->nr)%im->info->nr;
	    J = (j-width+im->info->nc)%im->info->nc;
	    new->data[i][j] = im->data[I][J];
	  }

	free (im->info);
	free(im->data[0]); free(im->data);
	im->info = new->info;
	im->data = new->data;
}

void debed (IMAGE im, int width)
{
	int i,j;
	IMAGE old;

	width +=2;
	old = newimage (im->info->nr-width-width, im->info->nc-width-width);
	for (i=0; i<old->info->nr-1; i++)
	{
	  for (j=1; j<old->info->nc; j++)
	  {
	    old->data[i][j] = im->data[i+width][j+width];
	    old->data[old->info->nr-1][j] = 255;
	  }
	  old->data[i][0] = 255;
	}

	free (im->info);
	free(im->data[0]); free(im->data);
	im->info = old->info;
	im->data = old->data;
}
