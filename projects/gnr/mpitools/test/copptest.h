/* (c) Copyright 2016, The Rockefeller University */
/* $Id: copptest.h 17 2016-06-30 20:47:06Z  $ */
/***********************************************************************
*                       MPI tools test program                         *
*                             COPPTEST.H                               *
*                                                                      *
*  This is the structure definition for the coppack() test program.    *
*  It declares some structures that can be processed with nxdr2 and    *
*  allocated with allocpmv, then filled with data for the collecting   *
*  tests.  Actual data structures are meaningless.                     *
*                                                                      *
*----------------------------------------------------------------------*
*  V1A, 08/25/16, GNR - New routine                                    *
***********************************************************************/

#define szddat 1000
#define szudat 1200
#define szfdat  801
#define szwdat 1200
#define szldat 1200

#define CPPACK_MSG 0x3112     /* Message type for coppack */

struct ctst1 {
   double ddat1[szddat];
   ui32   udat1[szudat];
   };

struct ctst2 {
   float fdat2[szfdat];       /* Odd number to check auto alignment */
   si64  wdat2[szwdat];
   long  ldat2[szldat];
   };

struct bcstptrs {
   struct ctst1 *bctst1;
   struct ctst2 *bctst2;
   };

