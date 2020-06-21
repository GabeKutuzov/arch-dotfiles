/* Driver for routine jacobi */
/* Modified by GNR to handle my version of the jacobi routine */

#include <stdio.h>

#define NP 10
#define NMAT 3

int jacobi(double *a, double *val, double *vec, double *dd,
      int N, int kvec, int ksort);

int main(void)
{
	int i,j,k,l,nrot,kl,jj;
	static double a[3][3]=
		{1.0,2.0,3.0,
		2.0,2.0,3.0,
		3.0,3.0,3.0};
	static double b[5][5]=
		{-2.0,-1.0,0.0,1.0,2.0,
		-1.0,-1.0,0.0,1.0,2.0,
		0.0,0.0,0.0,1.0,2.0,
		1.0,1.0,1.0,1.0,2.0,
		2.0,2.0,2.0,2.0,2.0};
	static double c[NP][NP]=
		{5.0,4.3,3.0,2.0,1.0,0.0,-1.0,-2.0,-3.0,-4.0,
		4.3,5.1,4.0,3.0,2.0,1.0,0.0,-1.0,-2.0,-3.0,
		3.0,4.0,5.0,4.0,3.0,2.0,1.0,0.0,-1.0,-2.0,
		2.0,3.0,4.0,5.0,4.0,3.0,2.0,1.0,0.0,-1.0,
		1.0,2.0,3.0,4.0,5.0,4.0,3.0,2.0,1.0,0.0,
		0.0,1.0,2.0,3.0,4.0,5.0,4.0,3.0,2.0,1.0,
		-1.0,0.0,1.0,2.0,3.0,4.0,5.0,4.0,3.0,2.0,
		-2.0,-1.0,0.0,1.0,2.0,3.0,4.0,5.0,4.0,3.0,
		-3.0,-2.0,-1.0,0.0,1.0,2.0,3.0,4.0,5.0,4.0,
		-4.0,-3.0,-2.0,-1.0,0.0,1.0,2.0,3.0,4.0,5.0};
	double r[NP];
	static int num[4]={0,3,5,10};
   double gnra[NP*(NP+1)/2], gnrv[NP*NP], gnrd0[NP], gnrd1[NP];

	for (i=1;i<=NMAT;i++) {
      kl = 0;
		if (i == 1) {
         for (k=0; k<num[i]; k++) {
            for (l=0; l<=k; l++) gnra[kl++] = a[k][l];
            }
         }
		else if (i == 2) {
         for (k=0; k<num[i]; k++) {
            for (l=0; l<=k; l++) gnra[kl++] = b[k][l];
            }
         }
		else if (i == 3) {
         for (k=0; k<num[i]; k++) {
            for (l=0; l<=k; l++) gnra[kl++] = c[k][l];
            }
         }
		nrot = jacobi(gnra, gnrd0, gnrv, gnrd1, num[i], 1, i%2);
		printf("matrix number %2d\n",i);
		printf("number of JACOBI rotations: %3d\n",nrot);
		printf("eigenvalues: \n");
		for (j=1;j<=num[i];j++) {
			printf("%12.6f",gnrd0[j-1]);
			if ((j % 5) == 0) printf("\n");
		}
		printf("\neigenvectors:\n");
      kl = 0;
		for (j=1;j<=num[i];j++) {
			printf("%9s %3d \n","number",j);
			for (k=1;k<=num[i];k++) {
				printf("%12.6f",gnrv[kl++]);
				if ((k % 5) == 0) printf("\n");
			}
			printf("\n");
		}
		/* eigenvector test */
		printf("eigenvector test\n");
      jj = 0;
		for (j=1;j<=num[i];j++,jj+=num[i]) {
			for (l=0;l<num[i];l++) {
				r[l]=0.0;
            kl = jj;
		      if (i == 1) {
               for (k=0; k<num[i]; k++) {
                  r[l] += a[k][l] * gnrv[kl++];
                  }
               }
		      else if (i == 2) {
               for (k=0; k<num[i]; k++) {
                  r[l] += b[k][l] * gnrv[kl++];
                  }
               }
		      else if (i == 3) {
               for (k=0; k<num[i]; k++) {
                  r[l] += c[k][l] * gnrv[kl++];
                  }
               }
				}
			printf("vector number %3d\n",j);
			printf("%11s %14s %10s\n",
				"vector","mtrx*vec.","ratio");
         kl = jj;
			for (l=0;l<num[i];l++,kl++)
				printf("%12.6f %12.6f %12.6f\n",
					gnrv[kl],r[l],r[l]/gnrv[kl]);
		}
	}
	return 0;
}
