/* (c) Copyright 2007, The Rockefeller University *11115* */
/* $Id: vbbdtest.c 28 2017-01-13 20:35:15Z  $ */
/***********************************************************************
*                              VBBDTEST                                *
*                              02/05/07                                *
*                                                                      *
*  A little test program for the BBDc client library                   *
*                                                                      *
*  V1A, 02/05/07, GNR - New program                                    *
*  ==>, 02/07/07, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "bbd.h"

int main (int argc, char *argv[]) {
   
   static char logfile[] = { "vBBD_test_log" };

   if (argc != 2) {
      fputs("***Synopsis: vbbdtest <CNS control file>\n\n", stdout);
      abexit(1);
      }

   bbdcinit(NULL, 0, argv[1], logfile);

   bbdcwait();

   return 0; 
   } /* End main (vbbdtest) */
