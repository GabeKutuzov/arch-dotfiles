/* (c) Copyright 1991-2003, Neurosciences Research Foundation, Inc. */
/* $Id: d1nset.c 3 2008-03-11 19:50:00Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d1nset                                 *
*                                                                      *
*     This routine sets all variables in D1BLK structures that differ  *
*  among nodes and that get written over when the tree structure is    *
*  broadcast from the root node at the start of every trial series.    *
*     The argument and also the value returned is a pointer to the     *
*  storage space allocated for D1 data on this node.  This routine     *
*  should be called with a NULL argument the first time, causing the   *
*  space to be malloc'd and allocated to the various repertoires.      *
*     This routine is similar to d3nset, which performs the same       *
*  function for D3 repertoires.  Note that #ifdef D1 is not needed--   *
*  d1nset is only linked in if CNS is compiled for D1 operation.       *
*  However, #ifdef PAR needed because d1nset does serial allocations.  *
*     Each pd1s and pd1r array is aligned on a natural boundary.       *
*  Note that unused D1BLKs are deleted by d3tchk, so no check needed.  *
*                                                                      *
*  V5C, 11/21/91, GNR - Newly written                                  *
*  V6C, 08/17/93, GNR - Add cpn,crn,cut to remove 2**n dependency      *
*  V7A, 04/24/94, GNR - Hormonize par/ser calc of ld1rall,ld1sall      *
*  V8C, 02/22/03, GNR - Cell responses in millivolts, add conductances *
*  ==>, 02/17/05, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#define D1TYPE  struct D1BLK

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "d3global.h"
#include "d1def.h"
#include "rocks.h"

char *d1nset(char *mypd1) {

   D1TYPE *pd1;            /* Ptr to current D1BLK */
#ifndef PAR0
   char *pnxtr;            /* Ptr to next repertoire */
   char *pnxts;            /* Ptr to next score table */
   long ld1rall = 0;       /* Length of all d1 data on this node */
   long ld1sall = 0;       /* Length of d1 s(i) lookup data */
#endif
#ifdef PARn
   long relnode;           /* Relative node number */
#endif

/* Scan through all D1BLK's to determine space needed on my node.
*  This differs from the global calculation done in d3allo.
*  N.B.  This code assumes each array of D1 repertoires will be
*  exchanged as a whole through host, because nd1r is analogous
*  to ngy for real repertoires.  */

#ifdef PARn
   /* Get relative number of current node */
   relnode = max((NC.node-NC.headnode),0);
#endif

   for (pd1=RP->pd1b1; pd1; pd1=pd1->pnd1b) {
#ifdef PAR0
      /* Parallel host--handles no cells */
      pd1->lod1e = 0;
      pd1->mynd1 = 0;
#endif
#ifdef PARn
      /* Parallel node n--allocate my cells */
      pd1->mynd1 = pd1->cpn;
      if (relnode < pd1->crn) /* Node in low group that divides rem */
         pd1->lod1e = relnode*(++pd1->mynd1);
      else                 /* Node in high group */
         pd1->lod1e = relnode*pd1->mynd1 + pd1->crn;
#endif
#ifndef PAR
      /* Serial--all cells on host */
      pd1->lod1e = 0;
      pd1->mynd1 = pd1->nd1r*pd1->nepr;
#endif
#ifndef PAR0
      /* Accumulate total space, allowing for alignment padding */
      ld1rall += ALIGN_UP(pd1->mynd1*pd1->nbyt);
      ld1sall += ALIGN_UP(pd1->nd1r*pd1->nepr);
#endif
      } /* End counting space */

#ifndef PAR0   /* Exclude allocation of rep space on host--
               *  exchange will use RP0->ps0.  */

/* If the space was not already allocated, allocate it now */

   if (!mypd1)
      mypd1 = (char *)mallocv(ld1rall+ld1sall,"D1 rep data");
   pnxtr = (pnxts = mypd1) + ld1sall;

/* Loop over all D1BLK's again to fill in the data pointers.
*  Note that every node needs room for all cells, but only
*  its own repertoire data.  */

   for (pd1=RP->pd1b1; pd1; pd1=pd1->pnd1b) {
      pd1->pd1s = (d1s_type *)pnxts;   /* Allocate cells */
      pd1->pd1r = (byte *)pnxtr;       /* Allocate reps */
      pnxts += ALIGN_UP(pd1->nd1r*pd1->nepr);
      pnxtr += ALIGN_UP(pd1->mynd1*pd1->nbyt);
      } /* End allocating cells */
#endif

   return mypd1;
   } /* End d1nset */

