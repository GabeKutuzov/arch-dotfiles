/***********************************************************************
*                             rfrwtest.c                               *
*                                                                      *
*        Test the new ROCKS rf{r|w}{i2|i4|i8|r4|r8} I/O routines       *
*                                                                      *
*  V1A, 06/04/99, GNR - New test                                       *
*  Rev, 10/24/99, GNR - Add rf[rw][iu]8                                *
*  Rev, 09/26/08, GNR - Minor type changes for 64-bit compilation      *
***********************************************************************/

#define MAIN
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkarith.h"
#include "rkxtra.h"
#include "rfdef.h"

#define RecordLength 1000
#define NumRecs      1000

double second(void);

void main(void) {

   /* One of each kind of data */
   double td;
   float  tf;
   long   tl;
   si64   tw;
   ui64   ty;
   short  ts;
   char   tc[RecordLength];
   double time0,time1,tt;
   struct RFdef *rfd;         /* Ptr to RFdef */
   long irec;                 /* Record counter */

   /* Start ROCKS I/O and the clock */
   settit("TITLE rfr|rfw io routines test program");
   cryout(RK_P1,"    ",RK_LN1,NULL);
   /* Print startup message, allocate file, read clock */
   printf("RFRWTEST beginning execution, recl = %d\n",RecordLength);
   fflush(stdout);
   rfd = rfallo("rfrwtest.dat",IGNORE,BINARY,SEQUENTIAL,TOP,
      IGNORE,REWIND,RETAIN_BUFF,RecordLength,IGNORE,IGNORE,ABORT);
   time0 = second();

   /* Open file, start buffering */
   rfopen(rfd,NULL,WRITE,SAME,SAME,TOP,
      SAME,SAME,SAME,SAME,SAME,SAME,ABORT);
   printf("rfopen'd rfrwtest.dat for write\n"); fflush(stdout);

   /* Write the data.  Arbitrarily flush every so often, just to see
   *  that the data end up still intact.  Write in short->long order
   *  to test that misalignment works OK.  */
   for (irec=0; irec<NumRecs; irec++) {
      rfwi2(rfd,(short)irec);
      rfwi4(rfd,(si32)(3*irec));
      rfwi8(rfd,jcsw(irec,irec+irec));
      rfwu8(rfd,jcuw(irec+irec,irec));
      rfwr4(rfd,2.5*(float)irec);
      rfwr8(rfd,3.75*(double)irec);
      if (irec % 97 == 1) rfflush(rfd, ABORT);
      }
   printf("rfwrote %ld on rfrwtest.dat\n",irec); fflush(stdout);

   /* Close file */
   rfclose(rfd,REWIND,RETAIN_BUFF,ABORT);
   printf("rfclosed rfrwtest.dat\n"); fflush(stdout);

   time1 = second();
   tt = time1 - time0;
   time0 = time1;
   printf("Elapsed time for writing with rfwrite %f\n",tt);
      fflush(stdout);

   /* Open for reading back in */
   rfopen(rfd,NULL,READ,SAME,SAME,TOP,
      SAME,SAME,SAME,SAME,SAME,SAME,ABORT);
   printf("rfopened rfrwtest.dat for read\n"); fflush(stdout);

   /* Read the data */
   for (irec=0; irec<NumRecs; irec++) {
      ts = rfri2(rfd);
      if (ts != (short)irec) {
         printf("Short data mismatch at record %ld, read %ld\n",
            irec,(long)ts);
         fflush(stdout); }
      tl = (long)rfri4(rfd);
      if (tl != 3*(long)irec) {
         printf("Long data mismatch at record %ld, read %ld,"
            " should be %ld\n",irec,tl,3*(long)irec);
         fflush(stdout); }
      tw = rfri8(rfd);
      if (swhi(tw) != irec || swlo(tw) != irec+irec) {
         printf("Wide data mismatch at record %ld, read (%ld,%ld),"
            " should be (%ld,%ld)\n",irec,swhi(tw),swlo(tw),
            irec, irec+irec);
         fflush(stdout); }
      ty = rfru8(rfd);
      if (uwhi(ty) != irec+irec || uwlo(ty) != irec) {
         printf("Unsigned wide data mismatch at record %ld, read "
            "(%ld,%ld), should be (%ld,%ld)\n", irec, uwhi(ty),
            uwlo(ty), irec+irec, irec);
         fflush(stdout); }
      tf = rfrr4(rfd);
      if (tf != 2.5*(float)irec) {
         printf("Float data mismatch at record %ld, read %g,"
            " should be %g\n",irec,tf,2.5*(float)irec);
         fflush(stdout); }
      td = rfrr8(rfd);
      if (td != 3.75*(double)irec) {
         printf("Double data mismatch at record %ld, read %g,"
            " should be %g\n",irec,td,3.75*(double)irec);
         fflush(stdout); }
      }
   printf("rfread %ld on rfrwtest.dat\n",irec); fflush(stdout);

   /* Close the data file */
   rfclose(rfd,SAME,RELEASE_BUFF,ABORT);
   printf("rfclosed rfrwtest.dat\n"); fflush(stdout);

   time1 = second();
   tt = time1 - time0;
   time0 = time1;
   printf("Elapsed time for reading with rfread %f\n",tt);
      fflush(stdout);

#if 1
   /* Open again, this time from stdin */
   rfopen(rfd, NULL, READ, SysIn, SEQUENTIAL, TOP, LOOKAHEAD,
      REWIND, RELEASE_BUFF, IGNORE, IGNORE, IGNORE, ABORT);
   printf("rfopened stdin\n"); fflush(stdout);

   /* Read till end of file */
   while (rfgets(rfd, tc, RecordLength, ABORT) > 0) {
      printf("%s\n", tc); fflush(stdout);
      if (memcmp(tc,"end",3) == 0) break;
      }
#else
   printf("reading from stdin via cryin\n"); fflush(stdout);

   /* Read till end of file */
   while (cryin()) {
      printf("%s\n", RK.last); fflush(stdout); }
#endif

   printf("Reached eof on stdin\n");

   } /* End rfrwtest() */
