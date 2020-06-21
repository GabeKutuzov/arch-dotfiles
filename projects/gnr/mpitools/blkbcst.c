/* (c) Copyright 2000-2016, The Rockefeller University *11115* */
/* $Id: blkbcst.c 3 2017-01-16 19:29:37Z  $ */
/*---------------------------------------------------------------------*
*                          MPI Tools Library                           *
*                              blkbcst()                               *
*                                                                      *
*  Broadcast an array of blocks of a single kind in a hybrid system,   *
*     optionally including nxdr2 conversion and buffering.  Replaces   *
*     broadcst(), which used old-style nxdr tables.                    *
*                                                                      *
*  Synopsis:                                                           *
*     void blkbcst(void *msg, long *nxtbl, size_t n, int msgtype)      *
*                                                                      *
*  Argument(s):                                                        *
*     msg      Pointer to data.                                        *
*     nxtbl    Pointer to nxdr2-style conversion table or NULL.  A     *
*              pointer is used rather than an NXDRTT index so that     *
*              this routine can be used with hand-coded conversion     *
*              tables.  If 'nxtbl' is NULL, there is no conversion--   *
*              data of length n bytes are broadcast unchanged.         *
*     n        If 'nxtbl' is not NULL, 'n' is the number of blocks     *
*              of data to broadcast, i.e. the number of times to       *
*              apply 'nxtbl'.  Otherwise, 'n' is the length of the     *
*              data to broadcast.                                      *
*     msgtype  Message type code to use for the broadcast.             *
*                                                                      *
*  Wait conditions:                                                    *
*     This routine sends all the data on the sending node before       *
*     returning and returns on receiving nodes when the data are       *
*     complete.  For streaming with other messages, use nncom()        *
*     (for data conversion) or nnput()/nnget() (no conversion).        *
*                                                                      *
*  Error conditions:                                                   *
*     When something goes wrong, the application is terminated.        *
*                                                                      *
*  Underlying code in MPI_Send/MPI_Recv is used actually to perform    *
*  the broadcast.  This function adds streaming and format conversion  *
*  via nncom and relatives to those basic capabilities.                *
************************************************************************
*  V1A, 12/03/00, GNR - New routine, based on ABP's broadcst().        *
*  ==>, 12/29/09, GNR - Last mod before committing to svn repository   *
*  Rev, 04/24/16, GNR - Change type of 'n' to size_t                   *
*  ==>, 07/01/16, GNR - Last mod before committing to svn mpi repo     *
*---------------------------------------------------------------------*/

#include <stddef.h>
#include <stdio.h>
#include "sysdef.h"
#include "mpitools.h"
#include "memshare.h"

void blkbcst(void *msg, long *nxtbl, size_t n, int msgtype) {

   if (n > 0) {
      struct NNSTR stream;
      char *pdat = msg;
      size_t i;
      int sendflag;

/* Create a stream */

#ifdef PAR0
      nnpcr(&stream, BCSTADDR, msgtype);
      sendflag = NNC_Send;
#else
      nngcr(&stream, BCSTADDR, msgtype);
      sendflag = NNC_Rcv;
#endif


/* Perform the broadcast, with or without data conversion */

      if (nxtbl)
         for (i=0; i<n; i++)
            nncom(&stream, (void **)&pdat, nxtbl, sendflag);
      else {
#ifdef PAR0
         nnput(&stream, pdat, ckul2i(n,"blkbcst"));
#else
         nnget(&stream, pdat, ckul2i(n,"blkbcst"));
#endif
         }

/* Close out the data stream */

#ifdef PAR0
      nnpcl(&stream);
#else
      nngcl(&stream);
#endif

      } /* End if n>0 */

   return;
   } /* End blkbcst() */

