/* (c) Copyright 2017, The Rockefeller University *11115* */
/* $Id: gathtest.c 17 2016-06-30 20:47:06Z  $ */
/***********************************************************************
*                       MPI tools test program                         *
*                              GATHTEST                                *
*                                                                      *
*  This is a simple test for function mpgather.  The same executable   *
*  can function on all nodes.                                          *
*----------------------------------------------------------------------*
*  V1A, 01/28/17, GNR - New routine                                    *
***********************************************************************/

#define MAIN

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "mpitools.h"
#include "collect.h"
#include "rkarith.h"

#define ASIZE 1003         /* Size of test data */
#define NODO    10         /* Offset of node data so it is guarded
                           *  below and above with diagnostic data. */
#define GATHTEST_MSG 12569 /* Message type */

/*=====================================================================*
*                               ckgath                                 *
*=====================================================================*/

static int ckgath(char *titl, char *tbyte, int *orig, int tnum) {
   int *test = (int *)tbyte;
   int i,nerr = 0;
   int im = tnum > 8 ? 4 : 1; 
   for (i=0; i<ASIZE; ++i) nerr += test[i*im] != orig[i];
   printf("Test %d: %s, %d errors\n",tnum,titl,nerr);
   if (nerr) {
      for (i=0; i<ASIZE; ++i) {
         printf("   %6d%6d", orig[i], test[i*im]);
         if ((i+1) % 4 == 0) printf("\n");
         }
      if (i % 4) printf("\n");
      return 1;
      }
   return 0;
   } /* End ckgath() */


/*=====================================================================*
*                            gathtest main                             *
*=====================================================================*/

int main(void) {

   int origdat[ASIZE], nodedat[ASIZE+NODO];
   char testdat[2*ASIZE*sizeof(void *)];
   int i,its,navg,nrem;
   int kstride;
   int absnode,relnode;
   int mystart,mycount;
   int nerr = 0;

   aninit(ANI_DBMSG,0,0);

#if 0
   /* Wait for debugger */
   {  volatile int cwt = 1;
      while (cwt) {
         sleep(1);
         }
      }
#endif

   if (NC.node == NC.hostid)
      printf("Running gathtest with %d nodes\n", NC.total);

   /* Make original data with consecutive numbers to make
   *  it easy to check the answers visually.  */
   for (i=0; i<ASIZE; ++i) origdat[i] = 1001+i;

   /* Perform all tests twice, first without, then with a stride
   *  on Node 0.  One level of indenting suppressed, here to end.  */

   for (kstride = 0; kstride < 16; kstride += 8) {

/*---------------------------------------------------------------------*
*                 Tests with host not providing data                   *
*---------------------------------------------------------------------*/

   relnode = NC.node - NC.headnode;
   navg = ASIZE/NC.cnodes, nrem = ASIZE%NC.cnodes;

   /* Test 1:  Divide data as evenly as possible.  */
   /* Every test will store junk where gather should not touch */
   memset(nodedat,0xb1,sizeof(nodedat));
   memset(testdat,0xc1,sizeof(testdat));
   if (NC.node == NC.hostid) {
      mystart = 2*kstride, mycount = 0;
      }
   else {
      mystart = navg*relnode + min(relnode, nrem);
      mycount = navg + (relnode < nrem);
      memcpy(nodedat+NODO, origdat+mystart, 4*mycount);
      }
   mpgather(testdat, nodedat+NODO, mystart, sizeof(int),
      ASIZE, mycount, GATHTEST_MSG);
   if (NC.node == NC.hostid) {
      nerr += ckgath("Even dist, no host data",
         testdat, origdat, kstride+1);
      }

   /* Test 2:  Omit data from Node 1 (relnode 0), an odd-numbered
   *  node, which implies no data transmission in first cycle.  */
   memset(nodedat,0xb2,sizeof(nodedat));
   memset(testdat,0xc2,sizeof(testdat));
   if (NC.node == NC.hostid) {
      mystart = 2*kstride, mycount = 0;
      }
   else if (relnode == 0) {
      mystart = mycount = 0;
      }
   else if (relnode == 1) {
      mystart = 0;
      mycount = 2*navg + (0 < nrem) + (1 < nrem);
      memcpy(nodedat+NODO, origdat, 4*mycount);
      }
   else {
      mystart = navg*relnode + min(relnode, nrem);
      mycount = navg + (relnode < nrem);
      memcpy(nodedat+NODO, origdat+mystart, 4*mycount);
      }
   mpgather((char *)testdat, nodedat+NODO, mystart, sizeof(int),
      ASIZE, mycount, GATHTEST_MSG);
   if (NC.node == NC.hostid) {
      nerr += ckgath("No headnode or host data",
         testdat, origdat, kstride+2);
      }

   /* Test 3:  Omit data from Node 2 (relnode 1), an even-numbered
   *  node, and let Node 1, below it, provide its data.  */
   memset(nodedat,0xb3,sizeof(nodedat));
   memset(testdat,0xc3,sizeof(testdat));
   if (NC.node == NC.hostid) {
      mystart = 2*kstride, mycount = 0;
      }
   else if (relnode == 0) {
      mystart = 0;
      mycount = 2*navg + (0 < nrem) + (1 < nrem);
      memcpy(nodedat+NODO, origdat, 4*mycount);
      }
   else if (relnode == 1) {
      mystart = mycount = 0;
      }
   else {
      mystart = navg*relnode + min(relnode, nrem);
      mycount = navg + (relnode < nrem);
      memcpy(nodedat+NODO, origdat+mystart, 4*mycount);
      }
   mpgather(testdat, nodedat+NODO, mystart, sizeof(int),
      ASIZE, mycount, GATHTEST_MSG);
   if (NC.node == NC.hostid) {
      nerr += ckgath("No node 2 or host data",
         testdat, origdat, kstride+3);
      }

   /* Test 4:  Again omit data from Node 2 (relnode 1), but
   *  this time let Node 3 (relnode 2) provide its data.  */
   memset(nodedat,0xb4,sizeof(nodedat));
   memset(testdat,0xc4,sizeof(testdat));
   if (NC.cnodes < 3) {
      if (NC.node == NC.hostid)
         printf("Test 4 skipped because < 3 comp nodes\n");
      goto Skip4;
      }
   if (NC.node == NC.hostid) {
      mystart = 2*kstride, mycount = 0;
      }
   else if (relnode == 1) {
      mystart = mycount = 0;
      }
   else if (relnode == 2) {
      mystart = navg + min(1, nrem);  /* Data from relnode 1 */
      mycount = 2*navg + (1 < nrem) + (2 < nrem);
      memcpy(nodedat+NODO, origdat+mystart, 4*mycount);
      }
   else {
      mystart = navg*relnode + min(relnode, nrem);
      mycount = navg + (relnode < nrem);
      memcpy(nodedat+NODO, origdat+mystart, 4*mycount);
      }
   mpgather(testdat, nodedat+NODO, mystart, sizeof(int),
      ASIZE, mycount, GATHTEST_MSG);
   if (NC.node == NC.hostid) {
      nerr += ckgath("Node 3 covers for node 2",
         testdat, origdat, kstride+4);
      }
Skip4: ;

/*---------------------------------------------------------------------*
*  Repeat same tests, now with host providing its share of the data    *
*---------------------------------------------------------------------*/

   if (kstride) break;     /* Stride tests only with no Node 0 data */
   absnode = NC.node;
   navg = ASIZE/(NC.cnodes+1), nrem = ASIZE%(NC.cnodes+1);

   /* Test 5:  Divide data as evenly as possible.  */
   /* Every test will store junk where gather should not touch */
   memset(nodedat,0xb5,sizeof(nodedat));
   memset(testdat,0xc5,sizeof(testdat));
   mycount = navg + (absnode < nrem);
   if (absnode == 0) {
      mystart = 2*kstride;
      memcpy(nodedat+NODO, origdat, 4*mycount);
      }
   else {
      mystart = navg*absnode + min(absnode, nrem);
      memcpy(nodedat+NODO, origdat+mystart, 4*mycount);
      }
   mpgather(testdat, nodedat+NODO, mystart, sizeof(int),
      ASIZE, mycount, GATHTEST_MSG);
   if (NC.node == NC.hostid) {
      nerr += ckgath("Even dist, with host data",
         testdat, origdat, kstride+5);
      }

   /* Test 6:  Omit data from Node 1 (absnode 1), an odd-numbered
   *  node, which implies no data transmission in first cycle.  */
   memset(nodedat,0xb6,sizeof(nodedat));
   memset(testdat,0xc6,sizeof(testdat));
   if (absnode == 0) {
      mystart = 2*kstride;
      mycount = navg + (absnode < nrem);
      memcpy(nodedat+NODO, origdat, 4*mycount);
      }
   else if (absnode == 1) {
      mystart = mycount = 0;
      }
   else if (absnode == 2) {
      mystart = navg + min(1, nrem);
      mycount = 2*navg + (1 < nrem) + (2 < nrem);
      memcpy(nodedat+NODO, origdat+mystart, 4*mycount);
      }
   else {
      mystart = navg*absnode + min(absnode, nrem);
      mycount = navg + (absnode < nrem);
      memcpy(nodedat+NODO, origdat+mystart, 4*mycount);
      }
   mpgather(testdat, nodedat+NODO, mystart, sizeof(int),
      ASIZE, mycount, GATHTEST_MSG);
   if (NC.node == NC.hostid) {
      nerr += ckgath("No Node 1 data",
         testdat, origdat, kstride+6);
      }

   /* Test 7:  Omit data from Node 2 (absnode 1), an even-numbered
   *  node, and let Node 1, below it, provide its data.  */
   memset(nodedat,0xb7,sizeof(nodedat));
   memset(testdat,0xc7,sizeof(testdat));
   if (absnode == 0) {
      mystart = 2*kstride;
      mycount = navg + (absnode < nrem);
      memcpy(nodedat+NODO, origdat, 4*mycount);
      }
   else if (absnode == 1) {
      mystart = navg + min(1, nrem);
      mycount = 2*navg + (1 < nrem) + (2 < nrem);
      memcpy(nodedat+NODO, origdat+mystart, 4*mycount);
      }
   else if (absnode == 2) {
      mystart = mycount = 0;
      }
   else {
      mystart = navg*absnode + min(absnode, nrem);
      mycount = navg + (absnode < nrem);
      memcpy(nodedat+NODO, origdat+mystart, 4*mycount);
      }
   mpgather(testdat, nodedat+NODO, mystart, sizeof(int),
      ASIZE, mycount, GATHTEST_MSG);
   if (NC.node == NC.hostid) {
      nerr += ckgath("No node 2 data, Node 1 covers",
         testdat, origdat, kstride+7);
      }

   /* Test 8:  Again omit data from Node 2 (absnode 1), but
   *  this time let Node 3 (absnode 2) provide its data.  */
   memset(nodedat,0xb8,sizeof(nodedat));
   memset(testdat,0xc8,sizeof(testdat));
   if (NC.cnodes < 3) {
      if (NC.node == NC.hostid)
         printf("Test 8 skipped because < 3 comp nodes\n");
      goto Skip8;
      }
   if (absnode == 0) {
      mystart = 2*kstride;
      mycount = navg + (absnode < nrem);
      memcpy(nodedat+NODO, origdat, 4*mycount);
      }
   else if (absnode == 2) {
      mystart = mycount = 0;
      }
   else if (absnode == 3) {
      mystart = 2*navg + min(2, nrem);  /* Data from absnode 2 */
      mycount = 2*navg + (2 < nrem) + (3 < nrem);
      memcpy(nodedat+NODO, origdat+mystart, 4*mycount);
      }
   else {
      mystart = navg*absnode + min(absnode, nrem);
      mycount = navg + (absnode < nrem);
      memcpy(nodedat+NODO, origdat+mystart, 4*mycount);
      }
   mpgather(testdat, nodedat+NODO, mystart, sizeof(int),
      ASIZE, mycount, GATHTEST_MSG);
   if (NC.node == NC.hostid) {
      nerr += ckgath("No node 2 data, Node 3 covers",
         testdat, origdat, kstride+8);
      }
Skip8: ;

   } /* End kstride loop; end indent suppression */

   if (NC.node == NC.hostid)
      printf("==>Test complete, %d had errors\n", nerr);
   appexit(NULL,0,0);
   return 0;
   } /* End gathtest() */
