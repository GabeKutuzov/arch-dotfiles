/* (c) Copyright 1991-2017, Neurosciences Research Foundation, Inc. */
/***********************************************************************
*                              rftest.c                                *
*                                                                      *
*                  Test the new ROCKS rfio routines                    *
*                                                                      *
*  Rev, 08/20/98, GNR - Add test for rfgets                            *
*  Rev, 01/12/17, GNR - Add test for MK_TEMP
***********************************************************************/

#define MAIN
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "rfdef.h"

#define RecordLength 1000
#define NumRecs      1000

/* Other things to test:
   (1) RETAIN_BUFF
*/

double second(void);

int main(void) {

   double time0,time1,tt;
   struct RFdef *rfd;         /* Ptr to RFdef */
   long irec;                 /* Record counter */
   char tfn[LFILNM];          /* Room to make temp file name */
   unsigned char td[RecordLength];     /* Test data */

   /* Start ROCKS I/O and the clock */
   settit("TITLE rf io routines test program");
   cryout(RK_P1,"    ",RK_LN1,NULL);
   /* Print startup message, allocate file, read clock */
   printf("RFTEST beginning execution, recl = %d\n",RecordLength);
   fflush(stdout);

/*---------------------------------------------------------------------*
*  Test allocate, write, rewind, read sequence                         *
*---------------------------------------------------------------------*/

   rfd = rfallo("rftest.dat",IGNORE,BINARY,SEQUENTIAL,TOP,
      IGNORE,REWIND,RETAIN_BUFF,4096,IGNORE,IGNORE,ABORT);
   memset(td,0xa5,RecordLength);
   time0 = second();

   /* Open file, start buffering */
   rfopen(rfd,NULL,WRITE,SAME,SAME,TOP,
      SAME,SAME,SAME,SAME,SAME,SAME,ABORT);
   printf("rfopen'd rftest.dat for write\n"); fflush(stdout);

   /* Write the data.  Arbitrarily flush every so often,
   *  just to see that the data end up still intact.  */
   for (irec=0; irec<NumRecs; irec++) {
      td[0] = irec & 0xff;
      rfwrite(rfd, td, RecordLength, ABORT);
      if (irec % 97 == 1) rfflush(rfd, ABORT);
      }
   printf("rfwrote %ld on rftest.dat\n",irec); fflush(stdout);

   /* Close file */
   rfclose(rfd,REWIND,RETAIN_BUFF,ABORT);
   printf("rfclosed rftest.dat\n"); fflush(stdout);

   time1 = second();
   tt = time1 - time0;
   time0 = time1;
   printf("Elapsed time for writing with rfwrite %f\n",tt);
      fflush(stdout);

   /* Open for reading back in */
   rfopen(rfd,NULL,READ,SAME,SAME,TOP,
      SAME,SAME,SAME,SAME,SAME,SAME,ABORT);
   printf("rfopened rftest.dat for read\n"); fflush(stdout);

   /* Read the data */
   for (irec=0; irec<NumRecs; irec++) {
      rfread(rfd,td,RecordLength,ABORT);
      if (td[0] != (irec & 0xff)) {
         printf("Data mismatch at record %ld\n",irec);
         fflush(stdout); }
      }
   printf("rfread %ld on rftest.dat\n",irec); fflush(stdout);

   /* Close the data file */
   rfclose(rfd,SAME,RELEASE_BUFF,ABORT);
   printf("rfclosed rftest.dat\n"); fflush(stdout);

   time1 = second();
   tt = time1 - time0;
   time0 = time1;
   printf("Elapsed time for reading with rfread %f\n",tt);
      fflush(stdout);

/*---------------------------------------------------------------------*
*  Test reading a test file (rftest.dat) via cryin                     *
*---------------------------------------------------------------------*/
#if 0
   /* Open again, this time from stdin */
   rfopen(rfd, NULL, READ, SysIn, SEQUENTIAL, TOP, LOOKAHEAD,
      REWIND, RELEASE_BUFF, IGNORE, IGNORE, IGNORE, ABORT);
   printf("rfopened stdin\n"); fflush(stdout);

   /* Read till end of file */
   while (rfgets(rfd, td, RecordLength, ABORT) > 0) {
      printf("%s\n", td); fflush(stdout);
      if (memcmp(td,"end",3) == 0) break;
      }
#else
   printf("reading from stdin via cryin\n"); fflush(stdout);

   /* Read till end of file */
   while (cryin()) {
      printf("%s\n", RK.last); fflush(stdout); }
#endif

   printf("Reached eof on stdin\n");

/*---------------------------------------------------------------------*
*  Similiar write/read sequence to the above, but using a temp file    *
*---------------------------------------------------------------------*/

   memset(td,0xa5,RecordLength);
   /* Clear the contents of the rfd to test the ACC_DEFER code */
   memset(rfd, 0, sizeof(struct RFdef));
   rfd->accmeth = ACC_DEFER;

   /* Open file, start buffering */
   strcpy(tfn, "Rftest-temp-file-XXXXXX");
   rfopen(rfd, tfn, READWRITE, TEXT, MK_TEMP, TOP, LOOKAHEAD,
      NO_REWIND, RELEASE_BUFF, IGNORE, IGNORE, NumRecs, ABORT);
   printf("rfopen'd %s for read-write\n", tfn); fflush(stdout);

   /* Write the data.  Arbitrarily flush every so often,
   *  just to see that the data end up still intact.  */
   for (irec=0; irec<NumRecs; irec++) {
      td[0] = irec & 0xff;
      rfwrite(rfd, td, RecordLength, ABORT);
      if (irec % 97 == 1) rfflush(rfd, ABORT);
      }
   printf("rfwrote %ld on temp file\n",irec); fflush(stdout);

   /* Rewind file without closing */
   rfflush(rfd, ABORT);
   rfseek(rfd, 0, SEEK_SET, ABORT); 
   printf("rewound temp file\n"); fflush(stdout);

   time1 = second();
   tt = time1 - time0;
   time0 = time1;
   printf("Elapsed time for writing with rfwrite %f\n",tt);
      fflush(stdout);

   /* No need to reopen for reading, just read the data */
   for (irec=0; irec<NumRecs; irec++) {
      rfread(rfd,td,RecordLength,ABORT);
      if (td[0] != (irec & 0xff)) {
         printf("Data mismatch at record %ld\n",irec);
         fflush(stdout); }
      }
   printf("rfread %ld on temp file\n",irec); fflush(stdout);

   /* Close the temp data file */
   rfclose(rfd,SAME,RELEASE_BUFF,ABORT);
   printf("rfclosed temp file\n"); fflush(stdout);

   time1 = second();
   tt = time1 - time0;
   time0 = time1;
   printf("Elapsed time for reading with rfread %f\n",tt);
      fflush(stdout);

   return 0;

   } /* End rftest() */
