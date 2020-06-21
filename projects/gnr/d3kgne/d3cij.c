/* (c) Copyright 2010, Neurosciences Research Foundation, Inc. */
/* $Id: d3cij.c 30 2010-07-09 21:26:56Z  $ */
/***********************************************************************
*                             CNS Program                              *
*           d3cij - Connection Strength Generation Routines            *
*                                                                      *
*  Note:  These routines are designed to make Cij values accessible    *
*  at any point in CNS without regard to whether or not KAMMF is in    *
*  effect.  This makes CNS easier to understand and maintain, at a     *
*  cost of some inner loop calls and redundant bits of arithmetic.     *
*                                                                      *
*  None of these routines should be called on a PAR0 node--this file   *
*  should not even be compiled on a PAR0 node, hence no #ifdefs.       *
*                                                                      *
*----------------------------------------------------------------------*
*                                                                      *
*  int ix->cijbr(struct CONNTYPE *ix)                                  *
*                                                                      *
*  This routine may be called once for each connection in succession   *
*     of the cell and connection type for which it has been set.  It   *
*     returns YES if a Cij value exists for this connection, else NO.  *
*  The actual value of Cij, scaled to S31, is returned in the LIJARG   *
*     structure.  cijsgn has the original sign (-0 intact), cijval     *
*     is the same except -0 set to +0 for use in computation.          *
*  Prerequisites are that d3kij() and d3kiji() must have been called   *
*     to set up for the particular cell type and cell number to be     *
*     accessed, and d3kijx() and d3cijx() must have been called to     *
*     set up for the particular connection type to be accessed, in     *
*     that order.  The caller must maintain the synapse index (isyn)   *
*     and increment the data pointer (psyn) in the Lija structure,     *
*     as these are shared among all d3lij, d3cij, etc. calls.          *
*  The d3lij() family of routines may or may not be called interleaved *
*     with the d3cij() family calls, but do not generate or look up    *
*     Cij for one cell while generating or looking up Lij for a        *
*     different cell (on the same node).                               *
*                                                                      *
*----------------------------------------------------------------------*
*                                                                      *
*  void d3cijx(struct CONNTYPE *ix)                                    *
*                                                                      *
*  This routine must be called to initialize the Lija array for a      *
*     particular connection type after the celltype and cell have      *
*     been established by d3kij(), d3kiji(), and d3kijx().             *
*                                                                      *
*----------------------------------------------------------------------*
*  V8D, 08/01/06, GNR - New routines, combining code from d3go() and   *
*                       other scattered places in CNS.                 *
*  ==>, 10/31/07, GNR - Last mod before committing to svn repository   *
*  Rev, 06/06/08, GNR - Add gcijml() for KGNMC matrices with NOPSAL    *
*  V8F, 06/05/10, GNR - Add gcije()                                    *
***********************************************************************/

#define LIJTYPE struct LIJARG

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "rkarith.h"

extern struct LIJARG Lija;

/*---------------------------------------------------------------------*
*               Prototypes for Cij generating routines                 *
*---------------------------------------------------------------------*/

int gcije(struct CONNTYPE *ix);
int gcijg(struct CONNTYPE *ix);
int gcijl(struct CONNTYPE *ix);
int gcijm(struct CONNTYPE *ix);
int gcijml(struct CONNTYPE *ix);
int gcijr(struct CONNTYPE *ix);

/*=====================================================================*
*                               d3cijx                                 *
*                                                                      *
*  Initialize for Cij generation for a particular connection type.     *
*=====================================================================*/

void d3cijx(struct CONNTYPE *ix) {

   /* If retrieving and not computing Cij, nothing is needed.  */
   if (Lija.kcall == KMEM_GO && cnexists(ix, CIJ)) return;

   /* Variables required with all options (or simpler to compute
   *  than the test to skip them, except bigc31 differs for KGNMC) */
   Lija.bigc24 = SRA(ix->cmsk,7);
   Lija.rndcon = (1L)<<(24-ix->Cn.nbc);
   Lija.nbcm2  = ix->Cn.nbc - 2;
   Lija.csas   = 0;
   Lija.msyn   = 0;
   if (ix->kgen & KGNMC) {       /* Setups for matrix Cij only */
      Lija.curmptr = ix->ucij.m.fstmptr;
      Lija.bigc31  = (ui32)ix->cmsk & (~((ui32)(~0L) >> NBPMCE)); }
   else
      Lija.bigc31  = (ui32)(ix->cmsk & LONG_MAX);

   /* This value prevents subarbor detection when not cloning Cij */
   Lija.csai   = (ix->Cn.saopt & SAOPTC) ? ix->nsa : 0;

   /* If Lij or jsyn info required, set up for generating Lij */
   if (ix->cnflgs & NEEDLIJ) d3lijx(ix);

   } /* End d3cijx() */


/*---------------------------------------------------------------------*
*                                gcije                                 *
*                                                                      *
*  Get Cij (S31) left in Lija.cijext by d3elij().                      *
*---------------------------------------------------------------------*/

int gcije(struct CONNTYPE *ix) {

   /* Isolate nbc bits and check for special -0 case */
   Lija.cijsgn = (Lija.cijext << BITSPERSHORT) & ix->cmsk;
   Lija.cijval = (Lija.cijsgn != MINUS0) ? Lija.cijsgn : 0;

   return YES;

   } /* End gcije() */


/*---------------------------------------------------------------------*
*                                gcijg                                 *
*                                                                      *
*  Get Cij (S31) from connection data in memory.                       *
*---------------------------------------------------------------------*/

int gcijg(struct CONNTYPE *ix) {

   d3gthn(Lija.cijsgn, Lija.psyn+ix->cnoff[CIJ], ix->cijlen);

   /* Isolate nbc bits and check for special -0 case */
   Lija.cijsgn &= ix->cmsk;
   Lija.cijval = (Lija.cijsgn != MINUS0) ? Lija.cijsgn : 0;

   return YES;

   } /* End gcijg() */


/*---------------------------------------------------------------------*
*                                gcijl                                 *
*                                                                      *
*  Generate Cij by motor map ("lambda", KGNLM) method.                 *
*---------------------------------------------------------------------*/

int gcijl(struct CONNTYPE *ix) {

   float fx0,fx1,fy0,fy1;           /* Corners */
   ldiv_t qrm;
   long cc;                         /* Current map value */
   long Cij;                        /* Unrounded Cij value */
   long lmix,lmiy;                  /* Integer map coordinates */
   si32 lldx,lldy;                  /* Local copies of cldx,cldy */
   int  lr = ix->ucij.l2.curang;    /* Angle */

   if (!Lija.lijbr(ix)) return NO;

   if (Lija.jsyn == Lija.csas) {    /* Time to clone? */
      Lija.lwcseed = ix->wcseed;    /* Working seed = starting seed */
      Lija.csas += Lija.csai;
      }

   if (ix->ucij.l2.flags & LOPTC) {
      /* Use target cell coordinates */
      lmix = Lija.groupx;
      lmiy = Lija.groupy;
      }
   else if (ix->kgfl & KGNIA) {
      /* Use input array coordinates */
      lmix = Lija.lijval >> RP->ky1;
      lmiy = Lija.lijval & RP->ymask;
      }
   else {
      /* Use source cell coordinates */
      qrm = ldiv(Lija.lijval/ix->srcnel,ix->srcngx);
      lmix = qrm.rem;
      lmiy = qrm.quot;
      }

   lldx = ix->ucij.l2.cldx, lldy = ix->ucij.l2.cldy;

   /* Apply X mirror if requested */
   if (ix->ucij.l2.flags & LOPXM && (lmix += lmix) > lldx)
      lmix = lldx + lldx - lmix;

   /* Apply Y mirror if requested */
   if (ix->ucij.l2.flags & LOPYM && (lmiy += lmiy) > lldy)
      lmiy = lldy + lldy - lmiy;

   /* Rotate coords if requested while performing the bilinear inter-
   *  polation.  Formerly, the map coeffs were literally swapped, but
   *  now we do it by modulo indexing, thus avoiding the need for
   *  storage for rotated coeffs.  This code redundantly generates
   *  the old strictly vertical or horizontal gradients.  */
   fx0 = (float)lmix; fx1 = (float)(lldx - lmix);
   fy0 = (float)lmiy; fy1 = (float)(lldy - lmiy);
   cc  = fx1*fy1*ix->ucij.l2.wc[( lr )&3] +
         fx1*fy0*ix->ucij.l2.wc[(lr+1)&3] +
         fx0*fy0*ix->ucij.l2.wc[(lr+2)&3] +
         fx0*fy1*ix->ucij.l2.wc[(lr+3)&3];
   Cij = ndev(&Lija.lwcseed, cc, ix->ucij.l2.csg);

   /* Round Cij to required accuracy
   *  Make sure |Cij| < 1 and store -0 if -2**(-nbc) <= Cij < 0
   *  Cases:  initial Cij       rounded Cij    result stored
   *  (nbc=5)  0 1111 1x         1 0000 0x         0 1111
   *           0 0000 0x         0 0000 1x         0 0000
   *           1 1111 1x         0 0000 0x         1 0000
   *           1 0000 0x         1 0000 1x         1 0001
   *  BUG FIX, 07/23/92, 08/26/92: If |Cij| > 1.0 before rounding
   *  (due to choice of mean and sigma), overflow was not detected. */
   Lija.cijval = (Cij + Lija.rndcon) & Lija.bigc24;
   if (labs(Lija.cijval) >= S24)    /* Overflow */
      /* Generate +/-bigc31 without an "if" test */
      Lija.cijsgn = Lija.cijval = (long)(
         Lija.bigc31 + (((ui32)(Cij & MINUS0)) >> Lija.nbcm2));
   else if (Lija.cijval == 0)       /* Zero, but keep sign */
      Lija.cijsgn = Cij & MINUS0, Lija.cijval = 0;
   else
      /* Normal case--shift to S31 */
      Lija.cijsgn = Lija.cijval <<= 7;

   return YES;

   } /* End gcijl() */


/*---------------------------------------------------------------------*
*                                gcijm                                 *
*                                                                      *
*  Generate Cij by lookup from stored matrix (KGNMC) method.           *
*  Rev, 01/15/07, GNR - Avoid picking up word past end of matrix, jic  *
*---------------------------------------------------------------------*/

int gcijm(struct CONNTYPE *ix) {

   if (!Lija.lijbr(ix)) return NO;

   while (Lija.msyn <= Lija.jsyn) {
      ++Lija.msyn;
      if (Lija.jsyn == Lija.csas) { /* Time to clone? */
         Lija.curmptr = ix->ucij.m.fstmptr;
         Lija.matcnt = 0;
         Lija.csas += Lija.csai;
         }
      if (!Lija.matcnt) {
         Lija.curcmat = *Lija.curmptr++;     /* Get skeleton word */
         Lija.matcnt = (BITSPERUI32/NBPMCE); /* Restart the count */
         }
      else
         Lija.curcmat <<= NBPMCE;            /* Shift out used bits */
      Lija.matcnt -= 1;                      /* Count Cij used */
      }

   Lija.cijsgn = Lija.cijval = Lija.curcmat & Lija.bigc31;

   return YES;

   } /* End gcijm() */


/*---------------------------------------------------------------------*
*                               gcijml                                 *
*                                                                      *
*  Generate Cij by lookup from stored matrix (KGNMC) method.  Similar  *
*  to gcijm except returns the same coefficient nsa times in NOPSAL    *
*  mode (and there is no cloning).                                     *
*---------------------------------------------------------------------*/

int gcijml(struct CONNTYPE *ix) {

   if (!Lija.lijbr(ix)) return NO;

   if (Lija.jsyn != Lija.isas) return YES;

   while (Lija.msyn <= Lija.jsyn) {
      Lija.msyn += ix->nsa;
      if (!Lija.matcnt) {
         Lija.curcmat = *Lija.curmptr++;     /* Get skeleton word */
         Lija.matcnt = (BITSPERUI32/NBPMCE); /* Restart the count */
         }
      else
         Lija.curcmat <<= NBPMCE;            /* Shift out used bits */
      Lija.matcnt -= 1;                      /* Count Cij used */
      }

   Lija.cijsgn = Lija.cijval = Lija.curcmat & Lija.bigc31;

   return YES;

   } /* End gcijml() */


/*---------------------------------------------------------------------*
*                                gcijr                                 *
*                                                                      *
*  Generate Cij by random (KGNRC) method.                              *
*  V8D, 08/29/06, GNR - Seed is no longer advanced for skipped conns.  *
*                       But result is still deterministic.             *
*---------------------------------------------------------------------*/

int gcijr(struct CONNTYPE *ix) {

   long Cij;                        /* Value of Cij before rounding */

   if (!Lija.lijbr(ix)) return NO;

#if 1
   if (Lija.jsyn == Lija.csas) {    /* Time to clone? */
      Lija.lwcseed = ix->wcseed;    /* Working seed = starting seed */
      Lija.csas += Lija.csai;
      }
#else
/* The following code is needed only in test runs to advance
*  random seed when a connection is skipped, then remove it.
*  This code is not correct for the NOPSAL-type subarboring.  */
   while (++Lija.msyn <= Lija.jsyn) {
      if (Lija.jsyn == Lija.csas) { /* Time to clone? */
         Lija.lwcseed = ix->wcseed; /* Working seed = starting seed */
         Lija.csas += Lija.csai;
         }
      udev(&Lija.lwcseed);
      }
#endif

   Cij = ndev(&Lija.lwcseed, ix->ucij.r.cmn, ix->ucij.r.csg);

   /* Use low-order bits of random number to determine sign */
   if ((ui16)Lija.lwcseed > (ui16)ix->ucij.r.pp) Cij = -Cij;

   /* Round Cij to required accuracy--see explanation at gcijl() */
   Lija.cijval = (Cij + Lija.rndcon) & Lija.bigc24;
   if (labs(Lija.cijval) >= S24)    /* Overflow */
      Lija.cijsgn = Lija.cijval = (long)(
         Lija.bigc31 + (((ui32)(Cij & MINUS0)) >> Lija.nbcm2));
   else if (Lija.cijval == 0)       /* Zero, but keep sign */
      Lija.cijsgn = Cij & MINUS0, Lija.cijval = 0;
   else
      /* Normal case--shift to S31 */
      Lija.cijsgn = Lija.cijval <<= 7;

   return YES;

   } /* End gcijr() */
