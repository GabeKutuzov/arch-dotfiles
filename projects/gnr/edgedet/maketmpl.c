/* Create template PGM files for the edge detection suite */

#include <stdio.h>

int t[256][256];		/* Image file */

void pgmout (FILE *f, int n)
{
	int i,j, k;

	k = 0;
	fprintf (f, "P2\n#Edge evaluation %d\n256 256 256\n", n);
	for (i=0; i<256; i++)
	  for (j=0; j<256; j++)
	  {
	    fprintf (f, "%d ", t[i][j]);
	    k++;
	    if (k>15) 
	    {
	      k = 0;
	      fprintf (f, "\n");
	    }
	  }
}

main()
{
	int i,j,k;
	FILE *f;

/* First edge test image */
	printf ("Creating template 1: vertical 2 regions\n");
	printf (" delta=18 at j=127 256 edge pixels.\n");
	for (i=0; i<256; i++)
	  for (j=0; j<256; j++)
	  {
	    if (j<=127) t[i][j] = 120;
	    else t[i][j] = 138;
	  }
	f = fopen ("et1.pgm", "w");
	pgmout (f, 1);
	fclose (f);

/* Second edge test image */
	printf ("Creating template 2: horizontal 2 regions\n");
	printf (" delta=18 at i=127 256 edge pixels.\n");
	for (i=0; i<256; i++)
	  for (j=0; j<256; j++)
	  {
	    if (i<=127) t[i][j] = 120;
	    else t[i][j] = 138;
	  }
	f = fopen ("et2.pgm", "w");
	pgmout (f, 2);
	fclose (f);

/* Third edge test image */
	printf ("Creating template 3:  diagonal 2 regions\n");
	printf (" delta=18 at i=j 256 edge pixels.\n");
	for (i=0; i<256; i++)
	  for (j=0; j<256; j++)
	  {
	    if (i>=j) t[i][j] = 120;
	    else t[i][j] = 138;
	  }
	f = fopen ("et3.pgm", "w");
	pgmout (f, 3);
	fclose (f);

/* Fourth edge test image */
	printf ("Creating template 4: vertical 2 steps\n");
	printf (" delta=9 at j=127 =9 at j=128 256 edge pixels.\n");
	for (i=0; i<256; i++)
	  for (j=0; j<256; j++)
	  {
	    if (j<127) t[i][j] = 120;
	    else if (j== 127) t[i][j] = 129;
	    else t[i][j] = 138;
	  }
	f = fopen ("et4.pgm", "w");
	pgmout (f, 4);
	fclose (f);

/* Fifth edge test image */
	printf ("Creating template 5: vertical 2 regions\n");
	printf (" delta=6 at j=127 =6 at j=128 =6 at j=129 256 edge pixels.\n");
	for (i=0; i<256; i++)
	  for (j=0; j<256; j++)
	  {
	    if (j<127) t[i][j] = 120;
	    else if (j==127) t[i][j] = 126;
	    else if (j == 128) t[i][j] = 132;
	    else t[i][j] = 138;
	  }
	f = fopen ("et5.pgm", "w");
	pgmout (f, 5);
	fclose (f);

}
