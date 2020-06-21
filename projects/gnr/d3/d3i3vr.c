/* (c) Copyright 2017, The Rockefeller University *11115* */
/* $Id: d3i3vr.c 78 2018-08-02 18:36:58Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                           d3i3vr, d3i3avr                            *
*                                                                      *
*  "Izhikevich 2003 Vrest": In the RF_IZH3 model, Vm is absolute, but  *
*  the CNS convention is to store Vm relative to Vrest.  And Vrest is  *
*  a function of cv2,cv1,cv0, and b.  (This can be derived by setting  *
*  du/dt = 0, whence u = b*Vrest, then setting dv/dt = 0, plugging in  *
*  this value for u, and solving for Vrest.)  If b is constant, Vrest  *
*  is computed by d3i3vr() in a call from d3bgis() (node 0) and saved  *
*  in IZ03DEF.i3vr. But if b is variable, and OPTIMIZE STORAGE is not  *
*  in effect, Vrest is computed for all cells on each node by a call   *
*  to d3i3avr() from d3nset() and stored at IZVr in the prd data.      *
*                                                                      *
*  Note added, 07/13/18:  Vr is a solution of a quadratic.  The sqrt   *
*  term can be added or subtracted and both solutions are valid equi-  *
*  librium potentials.  For now, program just chooses the positive     *
*  solution--may need option to favor one or the other.                *
*                                                                      *
*  To compute a single value of Vrest, call:                           *
*     si32 d3i3vr(struct IZ03DEF *pi3, si32 b)                         *
*  Arguments:                                                          *
*     pi3         Ptr to relevant IZ03DEF block                        *
*     b           b for current cell (S28)                             *
*  Returns:                                                            *
*     i3vr        Vrest for this cell (S20)                            *
*                                                                      *
*  To compute Vrest for all cells of type il on current node, call:    *
*     void d3i3avr(struct CELLTYPE *il)                                *
************************************************************************
*  V8E, 10/25/09, GNR - New routine                                    *
*  ==>, 10/31/09, GNR - Last mod before committing to svn repository   *
*  R76, 12/05/17, GNR - Add IZVr variable instead of IZVU[2]           *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "sysdef.h"
#include "d3global.h"
#include "rocks.h"
#include "rkarith.h"

/*=====================================================================*
*                               d3i3vr                                 *
*=====================================================================*/

si32 d3i3vr(struct IZ03DEF *pi3, si32 b) {

   double fqb = 2.0*(double)pi3->izcv1 - 0.125*(double)b;   /* (S25) */
   double fqa = (double)pi3->izcv2;    /* (S30) */
   double fqc = (double)pi3->izcv0;    /* (S20) */
   double fvr = fqb + sqrt(fqb*fqb - 4.0*fqa*fqc);
   /* S24 + 25 - (30-1) = 20 */
   return (si32)(-dS24*fvr/fqa);

   } /* End d3i3vr */

#ifndef PAR0
/*=====================================================================*
*                               d3i3avr                                *
*  (This routine assumes the caller has verified that the il->ctf      *
*  CTFI3IVR bit is set, i.e. rspmeth is RF_IZH3, OPTIMIZE is SPEED,    *
*  and Izhikevich b parameter is variable per cell.  Individual b      *
*  parameters are calculated here (and not lifted from IZRA data)      *
*  because d3i3avr() is called before d3genr() stores them at start    *
*  of run.)                                                            *
*=====================================================================*/

void d3i3avr(struct CELLTYPE *il) {

   struct IZHICOM *piz = (struct IZHICOM *)il->prfi;
   rd_type *pivr = il->prd + il->ctoff[IZVr];
   size_t llel;
   si32 lzseed = piz->izseed;
   si32 lzrb = (si32)piz->izrb;
   si32 b,ivr;
   int  icell;

#ifdef PAR
   udevwskp(&lzseed, jmsw(il->nizvar, il->locell));
#endif
   llel  = uw2zd(il->lel);

   for (icell=il->locell; icell<il->hicell; ++icell, pivr+=llel) {
      if (piz->izra) udev(&lzseed);
      ivr = udev(&lzseed) << 1;
      /*  N.B.  Must discard bits to bring precision to
      *  same number of bits as saved in IZRA data.  */
      ivr = mrssl(ivr, lzrb, FBrf);
      /* S31 + 14 - 31 + 14 ==> S28 */
      b = piz->izb + (ivr << 14);
      ivr = d3i3vr((struct IZ03DEF *)piz, b);
      d3ptjl4(ivr, pivr);
      udevskip(&lzseed, il->nizvcd);
      } /* End icell loop */

   } /* End d3i3avr() */
#endif
