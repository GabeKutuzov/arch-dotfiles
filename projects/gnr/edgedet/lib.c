/*----------------------------------------------------------------------

	Grey level include file

					J. R. Parker
					Laboratory for Computer Vision
					University of Calgary
					Calgary, Alberta, Canada

  ---------------------------------------------------------------------- */

#include <stdio.h>
#include <math.h>
#include <malloc.h>
#include <fcntl.h>
#include <io.h>
#include <graphics.h>
#include <stdlib.h>
#include <dos.h>
#include <bios.h>


/* The image header data structure      */
struct header {
	int nr, nc;             /* Rows and columns in the image */
	int oi, oj;             /* Origin */
};

/*      The IMAGE data structure        */
struct image {
		struct header *info;            /* Pointer to header */
		unsigned char **data;           /* Pixel values */
};

#define SQRT2 1.414213562
#define BLACK 0
#define WHITE 1

long seed = 132531;
typedef struct image * IMAGE;

#if defined (MAX)
int    PBM_SE_ORIGIN_COL=0, PBM_SE_ORIGIN_ROW=0;
char **arg;
int maxargs;
#else
extern int PBM_SE_ORIGIN_COL, PBM_SE_ORIGIN_ROW;
#endif

int range (IMAGE im, int i, int j);
void print_se (IMAGE p);
IMAGE Input_PBM (char *fn);
IMAGE Output_PBM (IMAGE image, char *filename);
void get_num_pbm (FILE *f, char *b, int *bi, int *res);
void pbm_getln (FILE *f, char *b);
void pbm_param (char *s);
struct image  *newimage (int nr, int nc);
void freeimage (struct image  *z);
void sys_abort (int val, char *mess);
void CopyVarImage (IMAGE *a, IMAGE *b);
void Display (IMAGE x);
float ** f2d (int nr, int nc);
void srand32 (long k);
double drand32 ();
void disp_lo_grey (struct image *x);
void disp_hi_grey (struct image *x);


/*      Check that a pixel index is in range. Return TRUE(1) if so.     */

int range (IMAGE im, int i, int j)
{
	if ((i<0) || (i>=im->info->nr)) return 0;
	if ((j<0) || (j>=im->info->nc)) return 0;
	return 1;
}

/*      PRINT_SE - Print a structuring element to the screen    */

void print_se (IMAGE p)
{
	int i,j;

	printf ("\n=====================================================\n");
	if (p == NULL)
	  printf (" Structuring element is NULL.\n");
	else 
	{
	  printf ("Structuring element: %dx%d origin at (%d,%d)\n",
		p->info->nr, p->info->nc, p->info->oi, p->info->oj);
	  for (i=0; i<p->info->nr; i++)
	  {
	    printf ("	");
	    for (j=0; j<p->info->nc; j++)
	      printf ("%4d ", p->data[i][j]);
	    printf ("\n");
	  }
	}
	printf ("\n=====================================================\n");
}

IMAGE Input_PBM (char *fn)
{
	int i,j,k,n,m,bi, b;
	unsigned char ucval;
	int val;
	long here;
	char buf1[256];
	FILE *f;
	IMAGE im;

	strcpy (buf1, fn);
	f = fopen (buf1, "r");
	if (f==NULL)
	{
	  printf ("Can't open the PBM file named '%s'\n", buf1);
	  return 0;
	}

	pbm_getln (f, buf1);
	if (buf1[0] == 'P')
	{
	  switch (buf1[1])
	  {
case '1':       k=1; break;
case '2':       k=2; break;
case '3':       k=3; break;
case '4':       k=4; break;
case '5':       k=5; break;
case '6':       k=6; break;
default:        printf ("Not a PBM/PGM/PPM file.\n");
		return 0;
	  }
	}
	bi = 2;

	get_num_pbm (f, buf1, &bi, &m);         /* Number of columns */
	get_num_pbm (f, buf1, &bi, &n);         /* Number of rows */
	if (k!=1 && k!=4) get_num_pbm (f, buf1, &bi, &b);      /* Max value */
	else b = 1;

	fprintf (stderr,"\nPBM file class %d size %d columns X %d rows Max=%d\n",
		k, m, n, b);
	
/* Binary file? Re-open as 'rb' */        
	if (k>3)
	{
	  here = ftell (f);
	  fclose (f);
	  f = fopen (fn, "rb");       
	  here++;
	  if (fseek(f, here, 0) != 0) 
	  {
	    printf ("Input_PBM: Sync error, file '%s'. Use ASCII PGM.\n",fn);
	    exit (3);
	  }
	}

/* Allocate the image */
	if (k==3 || k==6)       /* Colour */
	  sys_abort (0, "Colour image.");
	else 
	{
	  im = (IMAGE)newimage (n, m);
	  im->info->oi = PBM_SE_ORIGIN_ROW;
	  im->info->oj = PBM_SE_ORIGIN_COL;
	  PBM_SE_ORIGIN_ROW = 0;
	  PBM_SE_ORIGIN_COL = 0;
	  for (i=0; i<n; i++)
	    for (j=0; j<m; j++)
	      if (k<3)
	      {
		fscanf (f, "%d", &val);
		im->data[i][j] = (unsigned char)val;
	      } else {
		fscanf (f, "%c", &ucval);
		im->data[i][j] = ucval;
	      }
	}
	return im;
}

IMAGE Output_PBM (IMAGE image, char *filename)
{
	FILE *f;
	int i,j,k, perline;
	char buf1[64];

	strcpy (buf1, filename);
	if (image->info->nc > 20) perline = 20;
	 else perline = image->info->nc-1;
	f = fopen (buf1, "w");
	if (f == 0) sys_abort (0, "Can't open output file.");

	fprintf (f,"P2\n#origin %d %d\n",image->info->oj,image->info->oi);
	fprintf (f, "%d %d %d\n", image->info->nc, image->info->nr, 255);
	k = 0;
	for (i=0; i<image->info->nr; i++)
	  for (j=0; j<image->info->nc; j++)
	  {
		fprintf (f, "%d ", image->data[i][j]);
		k++;
		if (k > perline)
		{
		  fprintf (f, "\n");
		  k = 0;
		}
	  }
	fprintf (f, "\n");
	fclose (f);
	return image;
}

void get_num_pbm (FILE *f, char *b, int *bi, int *res)
{
	int i;
	char str[80];

	while (b[*bi]==' ' || b[*bi]=='\t' || b[*bi]=='\n')
	{
	  if (b[*bi] == '\n') 
	  {
	    pbm_getln (f, b);
	    *bi = 0;
	  } else
	  *bi += 1;
	}

	i = 0;
	while (b[*bi]>='0' && b[*bi]<='9')
	  str[i++] = b[(*bi)++];
	str[i] = '\0';
	sscanf (str, "%d", res);
}

/* Get the next non-comment line from the PBM file f into the
   buffer b. Look for 'pragmas' - commands hidden in the comments */

void pbm_getln (FILE *f, char *b)
{
	int i;
	char c;

/* Read the next significant line (non-comment) from f into buffer b */
	do
	{

/* Read the next line */
	  i = 0;
	  do
	  {
	    fscanf (f, "%c", &c);
	    b[i++] = c;
	    if (c == '\n') b[i] = '\0';
	  } while (c != '\n');

/* If a comment, look for a special parameter */
	  if (b[0] == '#') pbm_param (b);

	} while (b[0]=='\n' || b[0] == '#');
}

/*      Look for a parameter hidden in a comment        */
void pbm_param (char *s)
{
	int i,j;
	char key[24];

/* Extract the key word */
	for (i=0; i<23; i++)
	{
	  j = i;
	  if (s[i+1] == ' ' || s[i+1] == '\n') break;
	  key[i] = s[i+1];
	}
	key[j] = '\0';

/* Convert to lower case */
	for (i=0; i<j; i++)
	  if ( (key[i]>='A') && (key[i]<='Z') )
		key[i] = (char) ( (int)key[i] - (int)'A' + (int)'a' );

/* Which key word is it? */
	if (strcmp(key, "origin") == 0)         /* ORIGIN key word */
	{
	  sscanf (&(s[j+1]), "%d %d", 
	    &PBM_SE_ORIGIN_COL, &PBM_SE_ORIGIN_ROW);
	  return;
	}
}

struct image  *newimage (int nr, int nc)
{
	struct image  *x;                /* New image */
	unsigned char *ptr;             /* new pixel array */
	int i;

	if (nr < 0 || nc < 0) {
		printf ("Error: Bad image size (%d,%d)\n", nr, nc);
		return 0;
	}

/*      Allocate the image structure    */
	x = (struct image  *) malloc( sizeof (struct image) );
	if (!x) {
		printf ("Out of storage in NEWIMAGE.\n");
		return 0;
	}

/*      Allocate and initialize the header      */

	x->info = (struct header *)malloc( sizeof(struct header) );
	if (!(x->info)) {
		printf ("Out of storage in NEWIMAGE.\n");
		return 0;
	}
	x->info->nr = nr;       x->info->nc = nc;
	x->info->oi = x->info->oj = 0;

/*      Allocate the pixel array        */

	x->data = (unsigned char **)malloc(sizeof(unsigned char *)*nr); 

/* Pointers to rows */
	if (!(x->data)) {
		printf ("Out of storage in NEWIMAGE.\n");
		return 0;
	}
	
	for (i=0; i<nr; i++) 
	{
	  x->data[i] = (unsigned char *)malloc(nc*sizeof(unsigned char));
	  if (x->data[i]==0)
	  {
		printf ("Out of storage. Newimage - row %d\n", i);
		exit(1);
	  }
	}
	return x;
}

void freeimage (struct image  *z)
{
/*      Free the storage associated with the image Z    */
	int i;

	if (z != 0) 
	{
	  for (i=0; i<z->info->nr; i++)
	      free (z->data[i]);
	   free (z->info);
	   free (z->data);
	   free (z);
	}
}

void sys_abort (int val, char *mess)
{
	fprintf (stderr, "**** System library ABORT %d: %s ****\n", 
			val, mess);
	exit (2);
}

void copy (IMAGE *a, IMAGE b)
{
	CopyVarImage (a, &b);
}

void CopyVarImage (IMAGE *a, IMAGE *b)
{
	int i,j;

	if (a == b) return;
	if (*a) freeimage (*a);
	*a = newimage ((*b)->info->nr, (*b)->info->nc);
	if (*a == 0) sys_abort (0, "No more storage.\n");

	for (i=0; i<(*b)->info->nr; i++)
	  for (j=0; j< (*b)->info->nc; j++)
	    (*a)->data[i][j] = (*b)->data[i][j];
	(*a)->info->oi = (*b)->info->oi;
	(*a)->info->oj = (*b)->info->oj;
}

void Display (IMAGE x)
{
	Output_PBM (x, "x.tmp");
	system ("disp  x.tmp");
	system ("del x.tmp"); 

/*        if (x->info->nr <= 200 && x->info->nc <= 320)
	  disp_lo_grey (x);
	else disp_hi_grey (x);   */
}

float ** f2d (int nr, int nc)
{
	float **x, *y;
	int i;

	x = (float **)calloc ( nr, sizeof (float *) );
	if (x == 0)
	{
	  fprintf (stderr, "Out of storage: F2D.\n");
	  exit (1);
	}

	for (i=0; i<nr; i++)
	{  
	  x[i] = (float *) calloc ( nc, sizeof (float)  );
	  if (x[i] == 0)
	  {
	    fprintf (stderr, "Out of storage: F2D %d.\n", i);
	    exit (1);
	  }
	}
	return x;
}

/* Small system random number generator */


double drand32 ()
{
	static long a=16807L, m=2147483647L,
		    q=127773L, r = 2836L;
	long lo, hi, test;

	hi = seed / q;
	lo = seed % q;
	test = a*lo -r*hi;
	if (test>0) seed = test;
	else seed = test + m;

	return (double)seed/(double)m;
}

void srand32 (long k)
{
	seed = k;
}

/*      Display the image X using grey color map in 320x200 resolution  */
/* void disp_lo_grey (IMAGE x)                                          */
/* {                                                                    */
/*        int i, j, map[256], r, g, b;                                  */
/*        FILE *f;                                                      */
/*                                                                      */
/*        f = fopen ("debug", "w");                                     */
/*                                                                      */
/* Initialize the graphics system */                                    
/*        GrSetMode (GR_default_graphics);                              */
/*                                                                      */
/* Set all colors in the palette to greys  */                           
/*        for (i=0; i<256; i++)                                         */
/*        {                                                             */
/*          map[i] = GrAllocColor (i, 0, 55);                           */
/*          GrQueryColor (i, &r, &g, &b);                               */
/*          fprintf (f, "Colour %d is [%d, %d, %d]\n", i, r, g, b);     */
/*        }                                                             */
 
/* Copy pixels to the VGA */
/*          for(i=0; i<x->info->nr; i++)                                 */
/*          {                                                            */
/*            for (j=0; j<x->info->nc; j++)                              */
/*            {                                                          */
/*              GrPlot3d (j, i, map[(int)(x->data[i][j])] );             */
/*              fprintf (f, "%3d ", GrPixel (j, i));                     */
/*              if (j%14==0) fprintf (f, "\n");                          */
/*            }                                                          */
/*            fprintf (f, "\n");                                         */
/*          }                                                            */
/*          fclose (f);                                                  */
/*                                                                       */
/* Stop displaying when the user pushes the 'enter' key                */
/*        bioskey (0);                                                   */
/*        GrSetMode (GR_default_text);                                   */
/*}                                                                      */

/*      Display the image X as grey levels, 640x480 pixels */

/*void disp_hi_grey (struct image *x)                      */
/*{                                                        */
/*        int i,j, nr, nc;                                 */
/*                                                         */
/*  Initialize the graphics system */                      
/*        GrSetMode (GR_default_graphics);                 */
/*        for (i=0; i<256; i++)                            */
/*                GrSetColor (i, i, i, i);                 */
/*                                                         */
/*        nr = x->info->nr;       nc = x->info->nc;        */
/*        if (nr > 480) nr = 480;                          */
/*        if (nc > 640) nc = 640;                          */
/*                                                         */
/* Send the pixels to the VGA */
/*        for(i=0; i<nr; i++)                              */
/*          for (j=0; j<nc; j++)                           */
/*            GrPlot (j, i, (int)x->data[i][j]);           */
/*                                                         */
/*        bioskey(0);                                      */
/*        GrSetMode(GR_default_text);                      */
/*}                                                        */


