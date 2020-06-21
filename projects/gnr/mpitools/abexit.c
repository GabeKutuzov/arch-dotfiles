/* (c) Copyright 1998-2016, The Rockefeller University *11117* */
/* $Id: abexit.c 3 2017-01-16 19:29:37Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*    abexit - Intercept generic ROCKS errors, convert to appexit       *
*   abexitm - Intercept generic ROCKS errors with messages             *
*  abexitme - Intercept generic ROCKS errors with messages and errno   *
*                                                                      *
*  These routines replace the standard ROCKS abexit/abexitm routines   *
*  in those environments where the cryout() message mechanism is not   *
*  available, e.g. on subprocs or comp nodes of a parallel program.    *
*  The abnormal termination is intercepted and handled by mpitools     *
*  library routine appexit(), which sends a message to the parent      *
*  host process to shut down the entire process network.               *
*                                                                      *
*  These routines are included only in the mpitP0 and mpitPn libraries,*
*  so they will be automatically loaded in parallel programs when      *
*  one of these libraries is included before the standard libcrk,      *
*  which will contain the serial versions.                             *
*                                                                      *
*  Arguments to abexit() and abexitm() are documented in apndx5.crk    *
*                                                                      *
************************************************************************
*  V1A, 03/30/98, GNR - Broken out from d3exit.c                       *
*  Rev, 09/19/99, GNR - Add abexitme                                   *
*  ==>, 12/29/09, GNR - Last mod before committing to svn repository   *
*  Rev, 01/05/12, GNR - Add noreturn attribute to appexit              *
*  Rev, 04/23/16, GNR - Change abexitme to put errno in appexit ival,  *
*                       Reorder appexit args to put pointer first.     *
*  ==>, 10/03/16, GNR - Last mod before committing to svn mpi repo     *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "sysdef.h"
#include "mpitools.h"

void abexit(int code) {

   appexit(NULL, code, 0);

   } /* End abexit() */

void abexitm(int code, char *emsg) {

   appexit(emsg, code, 0);

   } /* End abexitm () */

void abexitme(int code, char *emsg) {

   appexit(emsg, code, errno);

   } /* End abexitme() */
