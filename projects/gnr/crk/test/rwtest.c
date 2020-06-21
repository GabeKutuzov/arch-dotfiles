/***********************************************************************
*                               rwtest                                 *
*                                                                      *
*  This program writes a file of 10 Mbytes of random data using either *
*  10000 writes of 1000 bytes each or else 1000000 writes of 10 bytes  *
*  each and compares the times.  This version uses open/write/close.   *
*  When compiled for the PC, reuses a 1K buffer-cannot malloc 10000000.*
***********************************************************************/

#define MAIN
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "sysdef.h"
#ifdef DOS
#include <io.h>
#include <sys\stat.h>
#endif
#ifdef UNIX
#include <sys/types.h>
#include <sys/stat.h>
#endif
#include "rocks.h"
#include "rfdef.h"

#define DATA_SIZE 10000000
#define BLOCK_SIZE 4096
#ifdef DOS
#define ALLO_SIZE 1000
#else
#define ALLO_SIZE 10000000
#endif

extern double second(void);

void main (void) {

   double t0,t1,t2,dt1,dt2;
   long *data;
   int fid;
   long i,ii;

   data = (long *)malloc(ALLO_SIZE);
   if (!data) {
      printf("\nFailed to allocate data buffer\n");
      exit(1); }
   for (i=0; i<ALLO_SIZE/sizeof(long); i++) data[i] = rand();

   t0 = second();

/* First test:  Write file in big chunks */

   ii = 1000;
#ifdef DOS
   fid = open("test2", O_WRONLY|O_CREAT|O_TRUNC|O_BINARY,
      S_IWRITE|S_IREAD);
#else
   fid = open("test2", O_WRONLY|O_CREAT|O_TRUNC, S_IWRITE|S_IREAD);
#endif
   if (fid < 0) {
      printf("\nUnable to open test2 file\n");
      exit(2);
      }

   for (i=0; i<DATA_SIZE; i+=ii)
#ifdef DOS
      write(fid, data, ii);
#else
      write(fid, (char *)data+i, ii);
#endif

   close(fid);
   t1 = second();
   dt1 = t1 - t0;
   /* For reasons unknown, printf here on PC prints doubles wrong */
   convrt("(/'Time for writing in chunks of ',J1IL10,'is ',J1Q12.5/)",
      &ii, &dt1, NULL);

/* Second test:  Write file in little chunks */

   ii = 10;
#ifdef DOS
   fid = open("test2", O_WRONLY|O_CREAT|O_TRUNC|O_BINARY,
      S_IWRITE|S_IREAD);
#else
   fid = open("test2", O_WRONLY|O_CREAT|O_TRUNC, S_IWRITE|S_IREAD);
#endif
   if (fid < 0) {
      printf("\nUnable to open test2 file\n");
      exit(3);
      }

   for (i=0; i<DATA_SIZE; i+=ii)
#ifdef DOS
      write(fid, data, ii);
#else
      write(fid, (char *)data+i, ii);
#endif

   close(fid);
   t2 = second();
   dt2 = t2 - t1;
   /* For reasons unknown, printf here on PC prints doubles wrong */
   convrt("(/'Time for writing in chunks of ',J1IL10,'is ',J1Q12.5/)",
      &ii, &dt2, NULL);
   cryout(RK_P1,"\0",RK_LN0+RK_FLUSH+1,NULL);

   } /* End rwtest */
