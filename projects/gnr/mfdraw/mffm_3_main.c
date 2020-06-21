/* (c) Copyright 2011, The Rockefeller University *11110* */
#include <stdio.h>
#include <stdlib.h>

//+++++++++++++++++++++++++++++++++++++++++++++ 6/5/2008
#include <malloc.h>

#include "mffm.h"

/* Function to call mffm_cleanup and then exit */
static void mfquit(int rc) {
   long stats[MFFM_NSTATS];
   mffm_cleanup(stats);
   printf("\nCleanup statistics:\n   Fixed overhead %d\n   Total Block memory %d\n"
      "   Frame headers %d\n   Data memory %d\n   Header waste %d\n"
      "   Data waste %d\n", stats[0], stats[1], stats[2], stats[3], stats[4], stats[5]);
   exit(rc);
   } /* End mfquit() */

//main is test driver for mffm.c - note all compiles with gcc -Wall
int main(void) {

    FrameNum frame;
    int rc = 0;
  //+++++++++++++++++++++++++++ test 1 +++++++++++++++++++++++++++++++
  {
    printf("start of test 1\n");
    //Syntax:  int mffm_init(size_t MaxMem, size_t LUHdr, long HistSize)

    rc = mffm_init(6000000,1000,100);

    printf("test 1: called mffm_init()\n");
    switch (rc) {
    case MFFM_OK :
      printf("mffm_init ok\n");
      break;
    case  (MFFM_ERR_NOMEM) :
      printf("Not enough memory for initial index, etc");
      mfquit(rc);
    case  (MFFM_ERR_SEMOP) :
      printf("A system semaphore call returned an error.\n");
      mfquit(rc);
    default :
      printf("Invalid return value from mffm_init()\n");
      mfquit(rc);
    }
  }
  //++++++++++++++++++++++++++++ test 2  +++++++++++++++++++++++++++++++
  {
  // Syntax:  int mffm_create(FrameNum frame)
    printf("start of test 2\n");
    rc = mffm_create(1);
    printf("test 2: called mffm_create()\n");
    switch (rc) {
    case MFFM_OK :
      printf("Empty frame was created and added to index.\n");
      break;
    case MFFM_EXISTS :
      printf(" This frame already exists.  No action taken.\n");
      break;
    case  (MFFM_ERR_NOMEM) :
      printf("Not enough memory for header or expanded index ");
      mfquit(rc);
    case  (MFFM_ERR_SEMOP) :
      printf("A system semaphore call returned an error.\n");
      mfquit(rc);
    case  (MFFM_ERR_BADARG) :
      printf("Attempt to create a frame with number <= 0\n");
      mfquit(rc);
    case  (MFFM_ERR_OFLOCK) :
      printf("An attempt was made to release the least-recently viewed frame to make space, but that frame was locked.\n  Probably all the memory is taken up with frame headers.\n Either allocate more memory or rewrite this package to release frame headers when needed. \n");
      mfquit(rc);
    default :
      printf("Invalid return value from mffm_create()\n");
      mfquit(rc);
    }
  }
  //++++++++++++++++++++++++++++ test 3  +++++++++++++++++++++++++++++++
  {
    //Syntax:  int mffm_store(FrameNum frame, byte *pdat, long ldat)
    printf("start of test 3\n");
    byte *pdat = "data to be stored in frame 1";
    printf("pdat points to:   %s\n", pdat);
    long ldat = 28;
    rc = mffm_store(1, pdat, ldat);
    printf("test 3: called mffm_stored()\n");
    switch (rc) {
    case MFFM_OK :
      printf("Data successfully stored in frame\n");
      break;
    case  (MFFM_ERR_NOMEM) :
      printf("Not enough memory for MaxMem data storage.");
      mfquit(rc);
    case  (MFFM_ERR_SEMOP) :
      printf("A system semaphore call returned an error.\n");
      mfquit(rc);
    case  (MFFM_ERR_BADFRAME) :
      printf("the requested frame does not exist.\n");
      mfquit(rc);
    case  (MFFM_ERR_OFLOCK) :
      printf(" An attempt was made to release the least recently viewed frame to make space, but that frame was locked.  See note above\n");
      mfquit(rc);
    case  (MFFM_ERR_BADARG) :
      printf("The data count (ldat) is 0 or negative or the data pointer (pdat) is NULL.\n");
      mfquit(rc);
    default :
      printf("Invalid return value from mffm_store()\n");
      mfquit(rc);
    }
  }

  //+++++++++++++++++++++++++++ test 4 +++++++++++++++++++++++++++++++++
  {
    //Syntax:  int mffm_put_hdr(FrameNum frame, void *puhd);
    printf("start of test 4\n");
    frame = 1;
    byte *puhd = "this is frame 1 frame data for test of mffm_put_hdr";
    printf("puhd points to:   %s\n", puhd);
    rc = mffm_put_hdr(frame, puhd);
    switch (rc) {
    case MFFM_OK :
      printf("Data successfully stored in frame %d\n", frame);
      break;
    case  (MFFM_ERR_SEMOP) :
      printf("A system semaphore call returned an error.\n");
      mfquit(rc);
    case  (MFFM_ERR_BADFRAME) :
      printf("the requested frame does not exist.\n");
      mfquit(rc);
    default :
      printf("Invalid return value from mffm_put_hdr()\n");
      mfquit(rc);
    }
  }
  //++++++++++++++++++++++++ test 5 ++++++++++++++++++++++++++++++++++++
  {
    //Syntax:  int mffm_end_put(FrameNum frame)
    printf("start of test 5\n");
    frame = 1;
    printf("write lock to frame %d to be removed\n", (int)frame);
    rc = mffm_end_put(frame);
    switch (rc) {
    case MFFM_OK :
      printf("Operation successful\n");
      break;
    case  (MFFM_ERR_SEMOP) :
      printf("A system semaphore call returned an error.\n");
      mfquit(rc);
    case  (MFFM_ERR_BADFRAME) :
      printf("the specified frame does not exist.\n");
      mfquit(rc);
    case (MFFM_ERR_NOWLOCK) :
      printf("The specified frame is not write-locked.\n");
      mfquit(rc);
    default :
      printf("Invalid return value from mffm_put_hdr()\n");
      mfquit(rc);
    }
  }

  //++++++++++++++++++++++++ test 6 ++++++++++++++++++++++++++++++++++++

    int sizeofdrawcmd = 10000;          //+++++++++++++++ 6/10/2008
    byte *pdat = malloc(sizeofdrawcmd); //+++++++++++++++ 6/10/2008
    byte *prdat;                        // Ptr to read data, GNR
    long ldat;                          // ++++++++++++ 6/10/2008

    printf("start of test 6 frame loop\n");
    for (frame=2; frame < 1000; ++frame) {

      // Syntax:  int mffm_create(FrameNum frame)
      rc = mffm_create(frame);
      if (rc) printf("Non-zero return from mffm_create(%d)\n", frame);
      switch (rc) {
      case MFFM_OK :
//      printf("Empty frame was created and added to index.\n");
        break;
      case MFFM_EXISTS :
        printf(" This frame already exists.  No action taken.\n");
        continue;
      case  (MFFM_ERR_NOMEM) :
        printf("Not enough memory for header or expanded index ");
        mfquit(rc);
      case  (MFFM_ERR_SEMOP) :
        printf("A system semaphore call returned an error.\n");
        mfquit(rc);
      case  (MFFM_ERR_BADARG) :
        printf("Attempt to create a frame with number <= 0\n");
        mfquit(rc);
      case  (MFFM_ERR_OFLOCK) :
        printf("An attempt was made to release the least-recently viewed frame to make space, but that frame was locked.\n  Probably all the memory is taken up with frame headers.\n Either allocate more memory or rewrite this package to release frame headers when needed. \n");
        mfquit(rc);
      default :
        printf("Invalid return value from mffm_create()\n");
        mfquit(rc);
      }


      //we make the number of commands a random number
      //int rannum = rand();
      //printf("the number of commands = %d\n", rannum);


      int loop = 1;
      for(; loop < 500; loop++) {  //++++++++++++++++++++++++++ (note loop < 600 will exceed maxmem) add drawing commands data to frame 2 6/4/2008;

        ldat = sizeofdrawcmd;         //+++++++++++++++ 6/10/2008

        //Syntax:  int mffm_store(frame, byte *pdat, long ldat)
        //printf("test 6 with loop = %d, frame = %d\n", loop, frame);

        sprintf(pdat, "data at beginning of stored data is %d", loop); //+++++++++++++++++++++++ 6/5/2008

        //printf("mffm_store stores drawing commands to frame %d\n", frame);
        rc = mffm_store(frame, pdat, ldat);
        if (rc) printf("Non-zero return from mffm_store(%d)\n", frame);
        switch (rc) {
        case MFFM_OK :
          //printf("mffm_store successful\n");
          break;
        case MFFM_ERR_NOMEM :
          printf("Not enough memory for MaxMem data storage\n");
          mfquit(rc);
        case  (MFFM_ERR_SEMOP) :
          printf("A system semaphore call returned an error.\n");
          mfquit(rc);
        case  (MFFM_ERR_BADFRAME) :
          printf("the requested frame does not exist.\n");
          mfquit(rc);
        case (MFFM_ERR_OFLOCK)  :
          printf("An attempt was made to release the least recently viewed frame\
 to make space, but that frame was locked\n");
          mfquit(rc);
        case (MFFM_ERR_NOWLOCK) :
          printf("The requested frame is not write-locked.\n");
          mfquit(rc);
        case (MFFM_ERR_BADARG) :
          printf("The data count (ldat) is 0 or negative or the data pointer (pdat) is NULL.\n");
          mfquit(rc);
        default :
          printf("Invalid return value from mffm_store()\n");
          mfquit(rc);
        }

      } //+++++++++++++++++++++++++++++++++++++++++++++++++ end of loop for adding drawing commands data to frame 2  6/4/2008
      if (!(frame % 100)) printf("last frame stored is frame %d\n", (int)frame); //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 6/4/2008


    // Syntax:  int mffm_end_put(FrameNum frame)
//    printf("test 6 - mffm_end_put\n");
    rc = mffm_end_put(frame);
    switch (rc) {
    case MFFM_OK :
//     printf("Operation successful.\n");
      break;
    case  (MFFM_ERR_SEMOP) :
      printf("A system semaphore call returned an error.\n");
      mfquit(rc);
    case  (MFFM_ERR_BADFRAME) :
      printf("the specified frame does not exist.\n");
      mfquit(rc);
    case (MFFM_ERR_NOWLOCK) :
      printf("The requested frame is not write-locked.\n");
      mfquit(rc);
    default :
      printf("Invalid return value from mffm_end_put()\n");
      mfquit(rc);
    }
  } /* End frame loop */

    //Syntax:  int mffm_access(FrameNum frame, FrameNum *pafn)
    printf("test 6 - start mffm_access\n");
    FrameNum pafn;
    frame = 995;

    rc = mffm_access(frame, &pafn); //++++++++++++++++++++++++++++++= replace line above 6/10/2008

    printf("Frame number requested = %d, actually accessed = %ld\n", frame, pafn);
    switch (rc) {
    case MFFM_OK :
      printf("mffm_access successful\n");
      break;
    case MFFM_NOHIST :
      printf("Either there is no viewing history (this is the first mffm_access call) or on a -1 call\
 the oldest frame in the viewing history was the frame just accessed, or on a 0 call the newest frame\
 in the viewing history was the frame just accessed.");
      break;
    case MFFM_WLOCK :
      printf("The frame is write-locked.\n");
      break;
    case MFFM_RLOCK :
      printf("The frame is already read-locked");
      break;
    case MFFM_NODATA :
      printf("The frame has no data associated with it. Either it was created but never loaded,\
 or else the data were discarded to make room for newer frames.");
      break;
    case MFFM_NOFRAME:
      printf("The requested frame does not exist\n");
      break;
    case  (MFFM_ERR_SEMOP) :
      printf("A system semaphore call returned an error.\n");
      mfquit(rc);
    default :
      printf("Invalid return value from mffm_get_data()\n");
      mfquit(rc);
    }
    //Syntax:  int mffm_get_data(FrameNum frame, byte **ppd, long *pld)
    printf("test 6 - mffm_get_data()\n");

    int xloop = 0;
    for(; xloop < 500; xloop++) { //++++++++++++++++++++++++++++++++++++ 6/5/2008

      byte **ppd;   //Pointer to a pointer in the caller's program where the address of the data will be returned.
      long *pld;    //Pointer to a long word in the caller's program where the length of the data will be returned
      //This word will be set to 0 when there are no more data for this frame to be returned.
      ppd = &prdat;
      pld = &ldat;
      rc =  mffm_get_data(frame, ppd, pld);
      //printf("length of data in frame 2 = %d\n", (int)*pld);
      //printf("*** data in frame %d = %s ***\n", frame,  ppd[0]);   // ++++++++++++++++++ added 6/5/2008

      switch (rc) {
      case MFFM_OK :
        printf("mffm_get_data successful, length = %d\n", ldat);
        printf("*** Data in frame %d = %s ***\n", frame, ppd[0]);   // ++++++++++++++++++ added 6/5/2008
        break;
      case MFFM_NODATA :
        printf("There are no (or no more) data to be returned.\n");
        break;
      case MFFM_ERR_NOMEM :
        printf("Not enough memory for data assembly area.\n");
        mfquit(rc);
      case  (MFFM_ERR_SEMOP) :
        printf("A system semaphore call returned an error.\n");
        mfquit(rc);
      case  (MFFM_ERR_BADFRAME) :
        printf("the requested frame does not exist.\n");
        mfquit(rc);
      case (MFFM_ERR_NORLOCK) :
        printf("The requested frame is not read-locked.\n");
        mfquit(rc);
      case (MFFM_ERR_BADARG) :
        printf("Either ppd or pld is a NULL pointer.\n");
        mfquit(rc);
      default :
        printf("Invalid return value from mffm_get_data()\n");
        mfquit(rc);
      }


    } //++++++++++++++++++++++++++++++++++++ 6/5/2008




    //Syntax:  int mffm_end_get(FrameNum frame)
    printf("test 6 - end_get_return\n");
    rc = mffm_end_get(frame);
    switch (rc) {
    case MFFM_OK :
      printf("mffm_end_get successful.\n");
      break;
    case  (MFFM_ERR_SEMOP) :
      printf("A system semaphore call returned an error.\n");
      mfquit(rc);
    case  (MFFM_ERR_BADFRAME) :
      printf("the requested frame does not exist.\n");
      mfquit(rc);
    case (MFFM_ERR_NORLOCK) :
      printf("The requested frame is not read-locked.\n");
      mfquit(rc);
    }
  //++++++++++++++++++++++++ end of tests ++++++++++++++++++++++++++++++
  mfquit(0);
  return 0;
}
