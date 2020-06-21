/* (c) Copyright 2016, The Rockefeller University *11115* */
/* $Id: nxtupush.c 3 2017-01-16 19:29:37Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                              nxtupush                                *
*                                                                      *
*  These routines are part of a shared memory management package for   *
*  parallel computers.  They are used to push or pop the addresses of  *
*  a structure conversion table and a union conversion table onto      *
*  a stack maintained in the global NC structure.  This allows sub-    *
*  routine libraries (such as plot) to have their own conversion       *
*  tables for memory broadcasts which they can use without reference   *
*  to whatever tables the client application is using.                 *
*                                                                      *
*  Synopsis:                                                           *
*     void nxtupush(long *qnxtt, unicvtf **qnxut)                      *
*     void nxtupop(void)                                               *
*                                                                      *
*  Arguments:                                                          *
*     qnxtt    Pointer to an NXDRTT table produced by nxdr2.           *
*     qnxut    Pointer to an NXDRUT table produced by nxdr2.           *
*                                                                      *
************************************************************************
*  V1A, 08/13/16, GNR - New routines                                   *
*  ==>, 08/17/16, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include "sysdef.h"
#include "mpitools.h"

void nxtupush(long *qnxtt, unicvtf **qnxut) {

   if (++NC.jnxtt > NTTPD) abexit(262);
   NC.pnxtt[NC.jnxtt] = qnxtt;
   NC.pnxut[NC.jnxtt] = qnxut;
   } /* End nxtupush() */

void nxtupop(void) {
   if (--NC.jnxtt < 0) abexit(262);
   } /* End nxtupop() */

