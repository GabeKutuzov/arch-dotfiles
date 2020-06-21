/* (c) Copyright 1993, Neurosciences Research Foundation, Inc. */
/* $Id: xpstubs.c 3 2008-03-11 19:50:00Z  $ */
/***********************************************************************
*                      NeuMachine Source File                          *
*                               xpstubs.c                              *
*                                                                      *
*  Stubs used on transputers.                                          *
*                                                                      *
*  Written by Ariel Ben-Porath                                         *
*  V1A, 05/07/93 Initial version                                       *
*  ==>, 04/20/94, GNR - Last mod before committing to svn repository   *
***********************************************************************/

typedef unsigned char byte;

void bkpt(void) {
   int xxx;
   xxx = 0;

   while(!xxx)
      xxx = xxx;
   }

void nvecrcv(void *p) {
   return;
   }
