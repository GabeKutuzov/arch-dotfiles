/* (c) Copyright 1997, Neurosciences Research Foundation, Inc. */
/* $Id: d3itcl.c 3 2008-03-11 19:50:00Z  $ */
/***********************************************************************
*                              d3itcl.c                                *
*                   CNS Cortical Network Simulator                     *
*                                                                      *
*  This collection of routines allows the user to iterate through      *
*  the cells in a standard CNS cell list.  The state of the iterator   *
*  is kept in a separate data structure from the cell list so that     *
*  in principle, multiple threads could access the same cell list.     *
*                                                                      *
*  V8A, 08/26/97, GNR - Newly written                                  *
*  ==>, 08/27/97, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "d3global.h"
#include "clblk.h"

/*---------------------------------------------------------------------*
*                            CLProbeRange                              *
*                                                                      *
*  This routine is used internally to probe whether the next entry in  *
*  a cell list is a start of range, and, if so, to store the range     *
*  parameters in the CELLITER block and skip over them in the CLBLK.   *
*  If it's not a range, icend is set to 0 and incr is undefined.       *
*---------------------------------------------------------------------*/

static void CLProbeRange(struct CELLITER *pcit) {

   struct CLBLK *pclb = pcit->pcib;
   short icnext = pcit->ici + 1;
   
   /* Store first cell in list as current cell */
   pcit->icell = pclb->pcells[pcit->ici];

   /* A negative entry is a stride or end of range indicator */
   if (icnext < pclb->clused && pclb->pcells[icnext] < 0) {
      if (!(pclb->pcells[icnext] & END_CELL_RANGE)) {
         /* It's a stride.  Code assumes an end of range will
         *  always follow, no need to test against clused again.  */
         pcit->incr = pclb->pcells[icnext] & 0x3fffffff;
         ++icnext;
         }
      else                          /* Not a stride */
         pcit->incr = 1;
      /* Store end of range and skip over it on list */
      pcit->icend = 1 - pclb->pcells[icnext] - pcit->incr;
      pcit->ici = icnext;
      }
   else {                           /* Not a range */
      pcit->icend = 0;
      }

   } /* End CLProbeRange() */

/*=====================================================================*
*                             CLInitIter                               *
*                                                                      *
*  This routine initializes an iterator to point to the first cell     *
*  in a given cell list.                                               *
*                                                                      *
*  Synopsis: void CLInitIter(struct CELLITER *pcit, struct CLBLK *pclb)*
*                                                                      *
*  Arguments:                                                          *
*     pcit     Ptr to the iterator to be initialized.  Any previous    *
*              contents of this data structure will be destroyed.      *
*     pclb     Ptr to the first block in a cell list chain that is     *
*              to be scanned.                                          *
*=====================================================================*/

void CLInitIter(struct CELLITER *pcit, struct CLBLK *pclb) {

   if (!pclb->pcells)
      d3exit(CELLITER_ERR, NULL, 0);
   pcit->pcib = pclb;
   pcit->ici  = 0;
   CLProbeRange(pcit);

   } /* End CLInitIter() */

/*=====================================================================*
*                              CLIterate                               *
*                                                                      *
*  This routine advances a cell list iterator to the next cell.        *
*                                                                      *
*  Synopsis: int CLIterate(struct CELLITER *pcit)                      *
*                                                                      *
*  Argument:                                                           *
*     pcit     Ptr to the iterator that is to be stepped.              *
*                                                                      *
*  Returns:    1 if there is another cell to process, 0 if list ended. *
*                                                                      *
*=====================================================================*/

int CLIterate(struct CELLITER *pcit) {

   struct CLBLK *pclb = pcit->pcib;

   /* If inside a range, increment to next cell in range.
   *  Note that value in icend is manipulated to avoid need
   *  to test icell+incr against it.  ici does not change.  */
   if (pcit->icell < pcit->icend) {
      pcit->icell += (long)pcit->incr;
      return 1; }
   /* Advance to next list entry */
   if (++pcit->ici < (long)pclb->clused) {
      CLProbeRange(pcit);
      return 1; }
   /* Advance to next list block */
   if (pclb->pnclb) {
      CLInitIter(pcit, pclb->pnclb);
      return 1; }
   /* Reached end of all blocks on this list */
   return 0;
   } /* End CLIterate() */

/*=====================================================================*
*                              qNextType                               *
*                                                                      *
*  This routine returns a pointer to the CELLBLK for the cell type     *
*  that the next cell in the iteration belongs to.  Result is not      *
*  valid until after CLInitIter has been called for this iterator.     *
*                                                                      *
*  Argument:                                                           *
*     pcit     Ptr to the iterator that is to be tested.               *
*                                                                      *
*  Returns:    Ptr to CELLBLK of the next cell in the iteration.       *
*                                                                      *
*=====================================================================*/

struct CELLBLK *qNextType(struct CELLITER *pcit) {

   return pcit->pcib->pcll;

   } /* End qNextType() */

/*=====================================================================*
*                              qNextCell                               *
*                                                                      *
*  This routine returns the id number of the next cell in a cell list. *
*  The result is not valid until after CLInitIter has been called for  *
*  this iterator.                                                      *
*                                                                      *
*  Argument:                                                           *
*     pcit     Ptr to the iterator that is to be tested.               *
*                                                                      *
*  Returns:    Cell number of the next cell in the iteration.          *
*                                                                      *
*=====================================================================*/

long qNextCell(struct CELLITER *pcit) {

   return pcit->icell;

   } /* End qNextCell() */
