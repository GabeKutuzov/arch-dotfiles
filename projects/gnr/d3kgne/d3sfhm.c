/* (c) Copyright 1997, Neurosciences Research Foundation, Inc. */
/* $Id: d3sfhm.c 3 2008-03-11 19:50:00Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                              d3sfhm.c                                *
*                                                                      *
*               Routines to support save file renaming                 *
*                                                                      *
*  getsfhm()      Locates proto SFHMATCH structure given a header type *
*  initsfhm()     Initializes prototype SFHMATCH structures for the    *
*                    three classes of connections from cell type data  *
*  linksfhm()     Link an SFHMATCH into a list for d3fchk processing   *
*                                                                      *
*  V8A, 10/19/97, GNR - Initial version                                *
*  ==>, 02/17/05, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "rocks.h"

#define SFHMAlloSize    64    /* Number SFHMATCHs to allocate at once */

/* Persistent data used only by routines in this file */
static struct {
   struct SFHMATCH
      sfhm0[SFHMNTYP];        /* Restore file defaults per type */
   short insfr[SFHMNTYP];     /* Indexes of ends of chains */
   short nsfrallo;            /* Number of SFHMATCHs allocated */
   short nsfrused;            /* Number of SFHMATCHs used */
   } HM;

/*=====================================================================*
*                              getsfhm()                               *
*                                                                      *
*  This routine returns the address of an SFHMATCH block into which    *
*  a control card processor can insert user specifications.  In the    *
*  case of a cell type block, the caller must initialize the block.    *
*  In the case of a connection type block, the rrennm and fno fields   *
*  will have been copied from the parent cell type block.  Fields      *
*  ppl and ict, needed just for generating error messages in d3fchk,   *
*  are maintained here.                                                *
*                                                                      *
*  Arguments:                                                          *
*     hmtype   One of the SFHM type codes defined in misc.h.           *
*     path     Ptr to owning CELLTYPE, CONNTYPE, INHIBBLK, or MODBY    *
*=====================================================================*/

struct SFHMATCH *getsfhm(int hmtype, void *path) {

   struct SFHMATCH *phm = &HM.sfhm0[hmtype];
   phm->path = path;
   if (hmtype == SFHMCELL) {
      phm->ppl = (struct CELLTYPE *)path;
      phm->rct = ~SHRT_MAX;
      phm->ict = 0;
      }
   else {
      ++phm->rct;
      ++phm->ict;
      }

   return phm;
   } /* End getsfhm() */


/*=====================================================================*
*                              initsfhm                                *
*                                                                      *
*  This routine copies the cell type SFHMATCH structure into the       *
*  prototypes for the three classes of connection types, thereby       *
*  establishing default rrennm and fno fields for them.                *
*=====================================================================*/

void initsfhm(void) {

   HM.sfhm0[SFHMCONN] = HM.sfhm0[SFHMGCON] = HM.sfhm0[SFHMMODU] =
      HM.sfhm0[SFHMCELL];

   } /* End initsfhm() */


/*=====================================================================*
*                             linksfhm()                               *
*                                                                      *
*  This routine links a completed SFHMATCH block into an array of such *
*  blocks according to the type of header to which it refers.  If the  *
*  user sets the "Override Restore" bit, this routine is never called. *
*  On the first call, memory is allocated and end-of-chain pointers    *
*  are initialized.                                                    *
*                                                                      *
*  Argument:                                                           *
*     phm      Address of SFHMATCH block to be linked                  *
*     hmtype   One of the SFHM type codes defined in misc.h.           *
*=====================================================================*/

void linksfhm(struct SFHMATCH *phm, int hmtype) {

   int i,ihm = HM.nsfrused;

   if (ihm >= HM.nsfrallo) {
      HM.nsfrallo += SFHMAlloSize;
      if (RP0->psfhm) {    /* Expand allocation */
         RP0->psfhm = (struct SFHMATCH *)reallocv(RP0->psfhm,
            HM.nsfrallo * sizeof(struct SFHMATCH),
            "Restore File Info");
         }
      else {               /* Initial allocation */
         RP0->psfhm = (struct SFHMATCH *)mallocv(
            HM.nsfrallo * sizeof(struct SFHMATCH),
            "Restore File Info");
         for (i=0; i<SFHMNTYP; i++) {
            HM.insfr[i] = -1;
            }
         }
      }

   RP0->psfhm[ihm] = *phm;    /* Copy the block */

   /* Link new block into appropriate string.  Make sure string is
   *  ended properly, as there is no good place to clean up later. */
   RP0->psfhm[ihm].nxthm = -1;
   if ((i = HM.insfr[hmtype]) < 0)  /* First entry */
      RP0->isfhm1[hmtype] = (short)ihm;
   else                       /* Chain entries */
      RP0->psfhm[i].nxthm = (short)ihm;
   HM.insfr[hmtype] = (short)ihm;

   ++HM.nsfrused;             /* Increment SFHMATCH count */

   } /* End linksfhm() */

