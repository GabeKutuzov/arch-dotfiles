/* (c) Copyright 2006, The Rockefeller University *11115* */
/* $Id: d3dij.c 70 2017-01-16 19:27:55Z  $ */
/***********************************************************************
*                             CNS Program                              *
*            d3dij - Connection Delay Generation Routines              *
*                                                                      *
*  Note:  These routines are designed to make Dij values accessible at *
*  any point in CNS without regard to whether or not OPTIMIZE STORAGE  *
*  is in effect.  This makes CNS easier to understand and maintain, at *
*  a cost of some inner loop calls and redundant bits of arithmetic.   *
*                                                                      *
*  None of these routines should be called on a PAR0 node--this file   *
*  should not even be compiled on a PAR0 node, hence no #ifdefs.       *
*                                                                      *
*----------------------------------------------------------------------*
*                                                                      *
*  ui32 ix->dijbr(struct CONNTYPE *ix)                                 *
*                                                                      *
*  This routine may be called once for each connection in succession   *
*     of the cell and connection type for which it has been set.  It   *
*     returns the value of the axonal delay, Dij, an integer.  This    *
*     value is also returned in the LIJARG structure.                  *
*  Prerequisites are that d3kij() and d3kiji() must have been called   *
*     to set up for the particular cell type and cell number to be     *
*     accessed, and d3kijx() and d3dijx() must have been called to     *
*     set up for the particular connection type to be accessed, in     *
*     that order.  The caller must maintain the synapse index (isyn)   *
*     and increment the data pointer (psyn) in the Lija structure,     *
*     as these are shared among all d3lij, d3cij, etc. calls.          *
*  Rev, 09/29/06, GNR - Add nDij counter so user code and sjbr() code  *
*     can both call dijbr() and get just one execution.                *
*                                                                      *
*----------------------------------------------------------------------*
*                                                                      *
*  void d3dijx(struct CONNTYPE *ix)                                    *
*                                                                      *
*  This routine must be called to initialize the LIJARG array for a    *
*     particular connection type after the celltype and cell have      *
*     been established by d3kiji().                                    *
*                                                                      *
************************************************************************
*  V8D, 09/28/06, GNR - New routines, combining code from d3go() and   *
*                       other scattered places in CNS.                 *
*  ==>, 08/29/07, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "lijarg.h"
#include "rkarith.h"

extern struct LIJARG Lija;

/*---------------------------------------------------------------------*
*               Prototypes for Dij generating routines                 *
*---------------------------------------------------------------------*/

ui32 gdijg(struct CONNTYPE *ix);
ui32 gdijc(struct CONNTYPE *ix);
ui32 gdijr(struct CONNTYPE *ix);
ui32 gdijn(struct CONNTYPE *ix);
ui32 gdiju(struct CONNTYPE *ix);

/*=====================================================================*
*                               d3dijx                                 *
*                                                                      *
*  Initialize Lija struct for a particular connection type.            *
*=====================================================================*/

void d3dijx(struct CONNTYPE *ix) {

   /* DLY_NORM: Make working dmn with round-off added.
   *  (Quicker to do this every time than test for KMEM_GO
   *  nDij < 0 and Dij not stored and DLY_NORM) */
   Lija.wdmn = ix->Cn.dmn + (S20>>1);
   Lija.nDij = 0;

   } /* End d3dijx() */


/*---------------------------------------------------------------------*
*                                gdijg                                 *
*                                                                      *
*  Get Dij from connection data in memory.                             *
*  (This routine does not need to bother with nDij because it          *
*  doesn't change any seeds or other values in memory.)                *
*---------------------------------------------------------------------*/

ui32 gdijg(struct CONNTYPE *ix) {

   return Lija.dijval = (ui32)Lija.psyn[ix->cnoff[DIJ]];

   } /* End gdijg() */


/*---------------------------------------------------------------------*
*                                gdijc                                 *
*                                                                      *
*  Return constant Dij (this routine also used if Dij do not exist).   *
*  (This routine does not need to bother with nDij because it          *
*  doesn't change any seeds or other values in memory.)                *
*---------------------------------------------------------------------*/

ui32 gdijc(struct CONNTYPE *ix) {

   return Lija.dijval = (ui32)ix->Cn.mxcdelay;

   } /* End gdijc() */


/*---------------------------------------------------------------------*
*                                gdijr                                 *
*                                                                      *
*  Generate Dij from a uniform random distribution (DLY_UNIF method).  *
*  Note:  Seed is not advanced for skipped conns, but result is still  *
*  deterministic.                                                      *
*---------------------------------------------------------------------*/

ui32 gdijr(struct CONNTYPE *ix) {

   if (Lija.nDij < 0) d3dijx(ix);
   if (Lija.nDij <= Lija.isyn) {
      ++Lija.nDij;
      Lija.dijval = udev(&ix->wdseed) % ((si32)ix->Cn.mxcdelay+1);
      }
   return Lija.dijval;

   } /* End gdijr() */


/*---------------------------------------------------------------------*
*                                gdijn                                 *
*                                                                      *
*  Generate Dij from a normal distribution (DLY_NORM method).          *
*  Note:  Seed is not advanced for skipped conns, but result is still  *
*  deterministic.                                                      *
*---------------------------------------------------------------------*/

ui32 gdijn(struct CONNTYPE *ix) {

   if (Lija.nDij < 0) d3dijx(ix);
   if (Lija.nDij <= Lija.isyn) {
      ui32 Dij;
      si32 sDij = ndev(&ix->wdseed, Lija.wdmn, ix->Cn.dsg);
      ++Lija.nDij;
      if (sDij < 0) Dij = 0;
      else {
         ui32 mxDij = (ui32)ix->Cn.mxcdelay;
         Dij = (ui32)sDij >> 20; /* Scale to integer */
         /* Safety in case ndev changed to permit
         *  values more than mean + 3*sigma  */
         if (Dij > mxDij) Dij = mxDij;
         }
      Lija.dijval = Dij;
      }

   return Lija.dijval;

   } /* End gdijn() */


/*---------------------------------------------------------------------*
*                                gdiju                                 *
*                                                                      *
*  Call a user routine to generate Dij -- not yet implemented          *
*---------------------------------------------------------------------*/

ui32 gdiju(struct CONNTYPE *ix) {

   if (Lija.nDij <= Lija.isyn) {
      ++Lija.nDij;
      Lija.dijval = (ui32)ix->Cn.mxcdelay;
      }

   return Lija.dijval;

   } /* End gdiju() */

