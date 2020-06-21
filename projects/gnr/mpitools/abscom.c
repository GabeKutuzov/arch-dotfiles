/* (c) Copyright 1993-2016, The Rockefeller University *21115* */
/* $Id: abscom.c 4 2017-02-03 18:57:19Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                               abscom.c                               *
*                                                                      *
*  This file contains source code for the NCUBE-like communications    *
*  routines, which provide that service on non-NCUBE architectures.    *
*  This is the version for MPI systems.  The following NCUBE functions *
*  are supported: nread, nwrite, nreadp, nwritep, ngetp, nrelp, ntest. *
*  Each of these is implemented by a routine whose name is derived by  *
*  adding the prefix 'a' to the NCUBE name. (e.g. anwrite).            *
*  In addition, verbose versions of angetp and anrelp are supplied.    *
*                                                                      *
*  aninit() should be called before using any of these routines.       *
*  appexit(NULL, 0, 0) should be called after normal execution, as it  *
*     ensure that MPI_Finalize is performed.                           *
*                                                                      *
*  N.B. These routines have to be provided even in serial libraries,   *
*  to support communication with a robotic device via anwrite(), etc.  *
*                                                                      *
*  Error handling:  The traditionally unused nread/nwrite argument,    *
*  cflg, has now been preempted for error handling control.  If cflg   *
*  is a NULL pointer, errors are returned to the caller.  Otherwise,   *
*  errors result in abexit().  This allows callers to write code that  *
*  is not complicated with endless error tests.  Of course, the abexit *
*  may fail due to communication errors, but it was felt that the most *
*  common errors would be caused by bad calling parameters, and the    *
*  abexit would still work in these cases.  Depending on the imple-    *
*  mentation, some errors will always be trapped at lower levels.      *
*                                                                      *
************************************************************************
*  V1A, 04/19/93, Ariel Ben-Porath, initial version                    *
*  V2A, 11/17/96, GNR - Remove broadcst to its own file for NCUBE use, *
*                       add interprocess dispatching on host w/compnn, *
*                       add COMPNN to eliminate NCUBE calls if no lib, *
*                       add support for antest on hybrid hosts.        *
*  V2B, 07/25/98, GNR - Use cflg arg to control error processing.      *
*  V8B, 09/05/99, GNR - Eliminate COMPNN support, add angetpv, anrelpv *
*  Rev, 09/19/99, GNR - Use abexitme() where appropriate.              *
*  ==>, 12/29/09, GNR - Last mod before committing to svn repository   *
*  Rev, 03/09/16, GNR - Remove set and test write sizes                *
*  New, 09/08/16, GNR - Rewritten to use MPI infrastructure            *
*  Rev, 01/24/17, GNR - Use MPI_get_count to get count for antest      *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "sysdef.h"
#include "mpitools.h"
#include "swap.h"
#include "rksubs.h"

/*---------------------------------------------------------------------*
*                              anread()                                *
*                                                                      *
*  Synopsis and description identical to the matching Vertex function  *
*  except nonzero cflg is ptr to error message string and signal to    *
*  terminate execution when an error occurs.                           *
*---------------------------------------------------------------------*/

int anread(void *buf, int nbytes, int *src, int *type, char *cflg) {

   MPI_Status rstat;
   int rc;
   if (*src == BCSTADDR) {
      rc = MPI_Barrier(NC.commc);
      if (rc && cflg) abexitm(168, ssprintf(NULL, "anread: "
         "MPI_Barrier ON NODE %d GAVE ERROR %d.", NC.node, rc));
      rc = MPI_Bcast(buf, nbytes, MPI_UNSIGNED_CHAR,
         NC.hostid, NC.commc);
      if (rc && cflg) abexitm(168, ssprintf(NULL, "anread: "
         "MPI_Bcast TYPE %d ON NODE %d GAVE ERROR %d.",
         *type, NC.node, rc));
      }
   else {
      rc = MPI_Recv(buf, nbytes, MPI_UNSIGNED_CHAR, *src, *type,
         NC.commc, &rstat);
      if (rc && cflg) abexitm(48, ssprintf(NULL, "anread: "
         "MPI_Recv %48s, TYPE %d MSG TO NODE %d GAVE ERROR %d.",
         cflg, *type, NC.node, rc));
      *src = rstat.MPI_SOURCE;
      *type = rstat.MPI_TAG;
      }
   return rc;

   } /* End anread() */

/*---------------------------------------------------------------------*
*                              anwrite()                               *
*                                                                      *
*  Synopsis and description identical to the matching Vertex function  *
*  except nonzero cflg is ptr to error message string and signal to    *
*  terminate execution when an error occurs.                           *
*---------------------------------------------------------------------*/

int anwrite(void *buf, int nbytes, int dest, int type, char *cflg) {

   int rc;
   if (dest == BCSTADDR) {
      rc = MPI_Barrier(NC.commc);
      if (rc && cflg) abexitm(168, ssprintf(NULL, "anwrite: "
         "MPI_Barrier ON HOST GAVE ERROR %d.", rc));
      rc = MPI_Bcast(buf, nbytes, MPI_UNSIGNED_CHAR,
         NC.hostid, NC.commc);
      if (rc && cflg) abexitm(168, ssprintf(NULL, "anwrite: "
         "MPI_Bcast DATA TYPE %d ON HOST GAVE ERROR %d.",
         type, rc));
      }
   else {
      rc = MPI_Send(buf, nbytes, MPI_UNSIGNED_CHAR, dest, type,
         NC.commc);
      if (rc && cflg) abexitm(49, ssprintf(NULL, "anwrite: "
         "MPI_Send %48s, TYPE %d MSG TO NODE %d GAVE ERROR %d.",
         cflg, type, dest, rc));
      }
   return rc;

   } /* End anwrite() */

/*---------------------------------------------------------------------*
*                              antest()                                *
*                                                                      *
* Synopsis and description identical to the matching Vertex function.  *
*---------------------------------------------------------------------*/

int antest(int *src, int *type) {

   MPI_Status tstat;
   int flag;
   int rc = MPI_Iprobe(*src, *type, NC.commc, &flag, &tstat);
   if (rc) abexitm(169, ssprintf(NULL,
      "MPI_Iprobe FOR TYPE %d ON SOURCE %d RETURNED ERROR CODE %d.",
         *type, *src, rc));
   if (flag) {
      *src = tstat.MPI_SOURCE;
      *type = tstat.MPI_TAG;
      MPI_Get_count(&tstat, MPI_UNSIGNED_CHAR, &rc);
      }

   /* If (flag), rc returns length of available message */
   return rc;

   } /* End antest() */

/*---------------------------------------------------------------------*
*                              anreadp()                               *
*                                                                      *
*  Allocates a buffer long enough to receive the expected message,     *
*  then calls anread().                                                *
*                                                                      *
*  Synopsis and description of the anread function are identical to    *
*  the matching Vertex function except nonzero cflg is ptr to error    *
*  msg string and signal to terminate execution when an error occurs.  *
*---------------------------------------------------------------------*/

int anreadp(void **buffer, int nbytes, int *src, int *type,
      char *cflg) {

   if ((*buffer = (void *)mallocv(nbytes, cflg)) == NULL)
      return -1;  /* Only get here if cflg == NULL */
   return anread(*buffer, nbytes, src, type, cflg);

   } /* End anreadp() */

/*---------------------------------------------------------------------*
*                             anwritep()                               *
*                                                                      *
*  Assumes a buffer has previously been malloc'd and data are to be    *
*  written (by a blocking node write), then the buffer is freed.       *
*                                                                      *
*  Synopsis and description of the anwrite function are identical to   *
*  the matching Vertex function except nonzero cflg is ptr to error    *
*  msg string and signal to terminate execution when an error occurs.  *
*---------------------------------------------------------------------*/

int anwritep(void *buffer, int nbytes, int dest, int type,
      char *cflg) {

   int rc;
   rc = anwrite(buffer, nbytes, dest, type, cflg);
   free(buffer);
   return rc;

   } /* End anwritep() */

/*---------------------------------------------------------------------*
*                              angetp()                                *
*                                                                      *
*  Allocate a buffer suitable for use with an anwritep() call.         *
*  (Basically just a wrapper around malloc)                            *
*---------------------------------------------------------------------*/

void *angetp(size_t size) {

   return (void *)malloc(size);

   } /* End angetp() */

/*---------------------------------------------------------------------*
*                              angetpv()                               *
*                            Verbose angetp                            *
*---------------------------------------------------------------------*/

void *angetpv(size_t size, char *msg) {

   void *ngetpvme;

   if ((ngetpvme = (void *)malloc(size)) == NULL)
      abexitm(30, msg);
   return ngetpvme;
   } /* End ngetpv() */

/*---------------------------------------------------------------------*
*                              anrelp()                                *
*                                                                      *
*  Free a buffer obtained with angetp/angetpv/anreadp                  *
*---------------------------------------------------------------------*/

int anrelp(void *buf) {

   if (buf) free(buf);
   return 0;

   } /* End anrelp() */

/*---------------------------------------------------------------------*
*                              anrelpv()                               *
*                          Verbose anrelp()                            *
*  N.B.  Until there is a way to find out when anrelp/free has failed, *
*  this routine doesn't really do anything different than anrelp(),    *
*  but it is left here for completeness and as a skeleton for future   *
*  development.                                                        *
*---------------------------------------------------------------------*/

void anrelpv(void *freeme, char *msg) {

   if (freeme) {
      free(freeme);
      }
   } /* End anrelpv() */

