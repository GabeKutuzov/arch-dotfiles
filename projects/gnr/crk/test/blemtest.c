/*--------------------------------------------------------------------*/
/*                                                                    */
/*                            blemtest.c                              */
/*                                                                    */
/* Determine whether the bem/lem method of swapping takes any longer  */
/* than the old way with assembler swap routines.  Only i2 and i4     */
/* swapping are tested, because these are used by far the most often. */
/* Each test is performed twice to eliminate buffering advantage.     */
/*                                                                    */
/* N.B.  All test values should include bytes with sign bit set       */
/* in order to test that these do not propagate improperly!           */
/*                                                                    */
/* V1A, 07/04/99, GNR - New routine                                   */
/*--------------------------------------------------------------------*/

#define  MAIN
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "rfdef.h"
#include "swap.h"

#define RecordLength 1000
#define NumRecs      1000
#define NumRepeats      2

double second(void);
long swap4(long);

void main(void) {

   double time0,time1,tt;
   union {           /* Test data */
      unsigned char td[RecordLength];
      long tl[RecordLength/4];
      } ud;
   struct RFdef *rfd;         /* Ptr to RFdef */
   long i,irec;               /* Record counter */
   int irpt;

   /* Start ROCKS I/O and the clock */
   settit("TITLE ROCKS I/O swapping speed test program");
   cryout(RK_P1,"    ",RK_LN1,NULL);
   /* Print startup message, allocate file, read clock */
   printf("BLEMTEST beginning execution, recl = %d\n",RecordLength);
   fflush(stdout);
   rfd = rfallo("rftest.dat",IGNORE,BINARY,SEQUENTIAL,TOP,
      IGNORE,REWIND,RETAIN_BUFF,4096,IGNORE,IGNORE,ABORT);
   memset(ud.td,0xa5,RecordLength);
   time0 = second();

   for (irpt=0; irpt<NumRepeats; irpt++) {

/*---------------------------------------------------------------------*
*                   Test I.  Use old-style swapping                    *
*---------------------------------------------------------------------*/

      /* Open file, start buffering */
      rfopen(rfd,NULL,WRITE,SAME,SAME,TOP,
         SAME,SAME,SAME,SAME,SAME,SAME,ABORT);
      printf("rfopen'd rftest.dat for swap-write\n"); fflush(stdout);

      /* Write the data */
      for (irec=0; irec<NumRecs; irec++) {
         for (i=0; i<(RecordLength/4); i++) {
            long *pud = ud.tl + i;
            *pud = swap4(irec^i);
            }
         rfwrite(rfd, ud.td, RecordLength, ABORT);
         if (irec % 97 == 1) rfflush(rfd, ABORT);
         }
      printf("rfwrote %ld on rftest.dat\n",irec); fflush(stdout);

      /* Close file */
      rfclose(rfd,REWIND,RETAIN_BUFF,ABORT);

      time1 = second();
      tt = time1 - time0;
      time0 = time1;
      printf("Elapsed time for writing with swap-rfwrite %f\n",tt);
         fflush(stdout);

      /* Open for reading back in */
      rfopen(rfd,NULL,READ,SAME,SAME,TOP,
         SAME,SAME,SAME,SAME,SAME,SAME,ABORT);
      printf("rfopened rftest.dat for read\n"); fflush(stdout);

      /* Read the data */
      for (irec=0; irec<NumRecs; irec++) {
         rfread(rfd, ud.td, RecordLength, ABORT);
         for (i=0; i<(RecordLength/4); i++) {
            long *pud = ud.tl + i;
            if (swap4(*pud) != (irec ^ i)) {
               printf("Data mismatch at record %ld\n",irec);
               fflush(stdout); }
            }
         }
      printf("rfread %ld on rftest.dat\n",irec); fflush(stdout);

      /* Close the data file */
      rfclose(rfd,SAME,RELEASE_BUFF,ABORT);

      time1 = second();
      tt = time1 - time0;
      time0 = time1;
      printf("Elapsed time for reading with swap-rfread %f\n",tt);
         fflush(stdout);

/*---------------------------------------------------------------------*
*                Test II.  Use lem-bem-style swapping                  *
*  Note that we use lem on big-endian machines, bem on little-endian,  *
*  to be sure that the routine actually has some work to do.           *
*---------------------------------------------------------------------*/

      /* Open file, start buffering */
      rfopen(rfd,NULL,WRITE,SAME,SAME,TOP,
         SAME,SAME,SAME,SAME,SAME,SAME,ABORT);
      printf("rfopen'd rftest.dat for bem-lem-write\n"); fflush(stdout);

      /* Write the data */
      for (irec=0; irec<NumRecs; irec++) {
         for (i=0; i<RecordLength; i+=4) {
            char *pud = (char *)(ud.td + i);
#if BYTE_ORDRE < 0
            bemfmi4(pud,(~irec^i));
#else
            lemfmi4(pud,(~irec^i));
#endif
            }
         rfwrite(rfd, ud.td, RecordLength, ABORT);
         if (irec % 97 == 1) rfflush(rfd, ABORT);
         }
      printf("rfwrote %ld on rftest.dat\n",irec); fflush(stdout);

      /* Close file */
      rfclose(rfd,REWIND,RETAIN_BUFF,ABORT);

      time1 = second();
      tt = time1 - time0;
      time0 = time1;
      printf("Elapsed time for writing with bem-lem-rfwrite %f\n",tt);
         fflush(stdout);

      /* Open for reading back in */
      rfopen(rfd,NULL,READ,SAME,SAME,TOP,
         SAME,SAME,SAME,SAME,SAME,SAME,ABORT);
      printf("rfopened rftest.dat for read\n"); fflush(stdout);

      /* Read the data */
      for (irec=0; irec<NumRecs; irec++) {
         rfread(rfd,ud.td,RecordLength,ABORT);
         for (i=0; i<RecordLength; i+=4) {
            char *pud = (char *)(ud.td+i);
            if (
#if BYTE_ORDRE < 0
               bemtoi4(pud)
#else
               lemtoi4(pud)
#endif
                  != (~irec^i)) {
               printf("Data mismatch at record %ld\n",irec);
               fflush(stdout); }
            }
         }
      printf("rfread %ld on rftest.dat\n",irec); fflush(stdout);

      /* Close the data file */
      rfclose(rfd,SAME,RELEASE_BUFF,ABORT);

      time1 = second();
      tt = time1 - time0;
      time0 = time1;
      printf("Elapsed time for reading with bem-lem-rfread %f\n",tt);
         fflush(stdout);

      } /* End repeat loop */

   } /* End of program */

