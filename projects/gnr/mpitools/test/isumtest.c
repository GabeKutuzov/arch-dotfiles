/* (c) Copyright 2016, George N. Reeke, Jr. */
/* $Id: isumtest.c 17 2016-06-30 20:47:06Z  $ */
/***********************************************************************
*                       MPI tools test program                         *
*                              ISUMTEST                                *
*                                                                      *
*  This is just a very simple test to see whether isum and the         *
*  functions it calls (anread, anwrite) work in the MPI environment.   *
*----------------------------------------------------------------------*
*  V1A, 06/30/16, GNR - New routine                                    *
***********************************************************************/

#define MAIN

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "mpitools.h"

int main(void) {

   int tsum;
   aninit(0,0,0);
   if (NC.node == NC.hostid)
      printf("Running isumtest with %d nodes\n", NC.total);
   tsum = isum(NC.node);
   if (NC.node == NC.hostid)
      printf("The sum of the node numbers returned is %d\n", tsum);
   MPI_Finalize();
   return 0;
   } /* End isumtest() */
