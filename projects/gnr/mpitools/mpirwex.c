/* (c) Copyright 2018, The Rockefeller University *11115* */
/* $Id: mpirwex.c 3 2017-01-16 19:29:37Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                          mpirex(), mpiwex                            *
*                                                                      *
*  Routines to creat a simple error message when an MPI_Send or        *
*  MPI_Recv gives an error return.  Since these events should be       *
*  very rare, it was felt reasonable to introduce the overhead of      *
*  this extra call to eliminate a lot of duplicate code elsewhere.     *
************************************************************************
*  R10, 09/01/18, GNR - New routine                                    *
*  ==>, 09/01/18, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include "sysdef.h"
#include "mpitools.h"

/* abexit for MPI_Recv errors */
void mpirex(int type, int node, int rc, char *fnm) {

   abexitm(48, ssprintf(NULL, "%s: "
      "MPI_Recv TYPE %d MSG FROM NODE %d GAVE ERROR %d.",
      fnm, type, node, rc));

   }  /* End mpirex() */

/*=====================================================================*
*                     abexit for MPI_Send errors                       *
*=====================================================================*/

void mpiwex(int type, int node, int rc, char *fnm) {

   abexitm(49, ssprintf(NULL, "%s: "
      "MPI_Send TYPE %d MSG TO NODE %d GAVE ERROR %d.",
      fnm, type, node, rc));

   }  /* End mpiwex() */
