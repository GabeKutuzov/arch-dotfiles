/* (c) Copyright 1993, Neurosciences Research Foundation, Inc. */
/* $Id: sunstubs.c 3 2008-03-11 19:50:00Z  $ */
/***********************************************************************
*                            CNS Source File                           *
*                              sunstubs.c                              *
*                                                                      *
*  Stubs used with serial version                                      *
*                                                                      *
*  Written by Ariel Ben-Porath                                         *
*  V1A, 05/07/93 Initial version                                       *
*  V7A, 04/26/94, GNR - Remove video stubs, make NOMAD stubs condtl    *
*  ==>, 08/08/07, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stdio.h>
#include "sysdef.h"

void bkpt(void) {
   int ii = 0;
   
   while (ii==0) sleep(1);
   }

void flush(void) {
   }
