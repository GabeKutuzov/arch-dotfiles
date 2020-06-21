
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
void copy (IMAGE *a, IMAGE b);
void CopyVarImage (IMAGE *a, IMAGE *b);
void Display (IMAGE x);
float ** f2d (int nr, int nc);

