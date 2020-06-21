/* (c) Copyright 1991-2012 Neurosciences Research Foundation, Inc. */
/* $Id: d3inhb.c 51 2012-05-24 20:53:36Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3inhb                                 *
*                                                                      *
*     CNS geometric connection evaluation routine (serial version)     *
*                                                                      *
*     Start-of-layer call: d3inhb()                                    *
*        Output:  The scaled, uncapped individual band contributions   *
*           in nib blocks of ngx*ngy words beginning at bandsum.       *
*        Calls d3bxvp if requested to plot inhibitory surround boxes.  *
*           Returns nonzero if interrupt button pressed in d3bxvp      *
*           (signal to terminate execution), otherwise zero.           *
*                                                                      *
*     Start-of-group call: d3inhg()                                    *
*        Output:                                                       *
*           This routine looks up scaled, uncapped band sums for the   *
*           current group and stores the inner ring (r0sum) totals     *
*           (if IBOPT=V) and the capped outer ring excitatory and      *
*           inhibitory (rnsums) totals for later use by d3inhc.        *
*                                                                      *
*     Start-of-cell call: void d3inhc()                                *
*        Output:                                                       *
*           (1) The undecayed, uncapped bandsums are returned in the   *
*           dynamic INHIBBAND structures.  All but the innermost band  *
*           (IBOPT=V) will have been calculated earlier, by d3inhg().  *
*           (2) The capped but otherwise "raw" total values applicable *
*           to the current cell are saved in CDAT.pAffData.  (3)       *
*           Values undergoing decay are passed to the next time step   *
*           in iepers,iipers.  (4) Effective values following optional *
*           decay and phasing are in CDAT.pAffData.  (5) Excitatory    *
*           and H,Q,S inhibitory totals are in CDAT.pGeomSums.         *
*                                                                      *
*     See d3gcon.c for the parallel versions of these routines.  The   *
*        parallel versions provide the same interface via CDAT, but    *
*        internally, the sums are done by brute-force methods each     *
*        time d3inhg() is called.  Setup in d3tchk is different.       *
*                                                                      *
************************************************************************
*  Written by George N. Reeke                                          *
*  V4A, 01/27/89, SCD - Translated to 'C' from version V3F             *
*  Rev, 11/01/90, GNR - Pass d3bxvp return code through                *
*  V5E, 07/05/92, GNR - Add d3inhg to handle phase                     *
*  Rev, 08/08/92, GNR - Handle compilers that do logical right shifts  *
*  V6D, 01/10/94, GNR - Add self-avoidance (OPT=V), d3inhc routine,    *
*                       update calc of falloff to agree with par vers. *
*  V7A, 05/01/94, GNR - Add delay, correct bandsum bug with OPT=X,     *
*                       correct BC=NORM bug, add COMPAT=N for old way  *
*  V8A, 09/03/95, GNR - Move falloff calcs to d3foff.c, phase calcs    *
*                       to gmphaff, control mode of inhibition per     *
*                       INHIBBLK, add decay, remove COMPAT=N           *
*  Rev, 09/27/96, GNR - Variable scaling for greater falloff accuracy, *
*                       consolidate two loops over INHIBBLKs in d3inhb *
*  V8C, 03/01/03, GNR - Cell responses in millivolts, add conductances *
*  ==>, 09/29/06, GNR - Last mod before committing to svn repository   *
*  Rev, 04/23/08, GNR - Add ssck (sum sign check) mechanism & IBOPKR   *
*  Rev, 12/31/08, GNR - Replace jm64sl,jm64sh with mssle or msrsle     *
*  V8E, 05/05/09, GNR - Add gjrev, modify thresh tests to elim 0's     *
*  V8G, 08/17/10, GNR - Add autoscale adjustment to betas              *
*  V8H, 04/01/11, GNR - Add gdefer                                     *
*  Rev, 04/09/12, GNR - Add IBOPT=F, calc decay even during defer      *
*  Rev, 05/16/12, GNR - Count scaling overflows, but do not kill run   *
***********************************************************************/

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rkarith.h"
#include "d3global.h"
#include "celldata.h"
#include "rocks.h"

extern struct CELLDATA CDAT;

/***********************************************************************
*                                                                      *
*                               d3inhb                                 *
*                                                                      *
***********************************************************************/

int d3inhb(void) {

   struct INHIBBLK *ib;          /* Ptr to current INHIBBLK */
   struct CELLTYPE *il,*jl;      /* Ptr to source CELLTYPE */
   struct XYNORMDEF *xy;         /* Ptr to normalization table */
   long *bptr;                   /* Ptr into boxsum array */
   long *beptr;                  /* Ptr into end of boxsum array */
   long *bndsumptr;              /* Pointer into bandsum array */
   long *povct;                  /* Ptr to overflow count */
   long *xptr;                   /* Ptr for boxsum x loop */
   long *xeptr,*yeptr;           /* Limits for x,y loops */
   s_type *pcell, *plimit;       /* Pointer and limit for cell loop */
   long gpsnorm;                 /* Groups in full-sized rings */
   long nbytes;                  /* Number of bytes */
   long nrows;                   /* Number of rows */
   long ovct;                    /* Overflow count if no stats */
   long ring_cnt;                /* Ring counter */
   long rrbs;                    /* Boxsum offset */
   long rrhs;                    /* Horizontal strip offset */
   long xtotal;                  /* Total of boxsums for OPT=X */

/* Loop over all INHIBBLKs associated with this cell type */

   il = CDAT.cdlayr;
   povct = (il->Ct.kctp & CTPNS) ? &ovct : &il->CTST->gmover;
   ovct = 0;
   for (ib=il->pib1; ib; ib=ib->pib) {

      /* If deferring (and no decay or decay-fast-start) this GCONN,
      *  zero bandsums and AffData for d3dprt and omit calculation. */
      if (CDAT.curr_cyc1 <= ib->gcdefer) {
         struct INHIBBAND *pb = ib->inhbnd;
         int iband;
         for (iband=0; iband<(int)ib->nib; iband++,pb++)
            memset((char *)pb->bandsum, 0, ib->l1xy*sizeof(long));
         if (ib->lafd > 0) memset(
            (char *)(CDAT.pAffData+ib->ogafs), 0, (size_t)ib->lafd);
         }
      else {   /* Retain single indenting of following code ... */

      /* Make local copies of selected variables */
      long ligrp;                      /* Len s(j) data for 1 grp  */
      long lit = (long)ib->itt;        /* Local inhibition threshs */
      long lnt = (long)ib->ittlo;
      long lsit = (long)ib->subitt;    /* Local subtracted threshs */
      long lsnt = (long)ib->subnitt;
      long litr = (long)ib->gjrev;     /* Local rev potl adjuster  */
      long nibm1      = ib->nib - 1;   /* Number of bands minus 1  */
      long nrm1       = ib->l1n1;      /* Number of rings minus 1  */
      long l_l2n1     = nrm1+nrm1;     /* 2*(nr-1)                 */
      long l_l1x      = ib->l1x;       /* ngx                      */
      long l_l1y      = ib->l1y;       /* ngy                      */
      long l_l1x1     = ib->l1x - 1;   /* ngx-1                    */
      long l_l1n1x    = ib->l1n1x;     /* (nr-1)*ngx               */
      long l_l1xn1    = nrm1+ib->l1x;  /* ngx+nr-1                 */
      long l_l1xn2    = ib->l1xn2;     /* ngx+2*(nr-1)             */
      long l_l1n1xn2  = ib->l1n1xn2;   /* (nr-1)*(ngx+2*(nr-1))    */
      long l_l1xy     = ib->l1xy;      /* ngx*ngy                  */
      long l_l1yxn2   = ib->l1yxn2;    /* ngy*(ngx+2*(nr-1))       */
      long l_l1xn2yn2 = ib->l1xn2yn2;  /* (ngx+l2n1)*(ngy+l2n1)    */

      int do_norm;                     /* TRUE if BC=NORM          */
      int iband;                       /* Band index               */
      int iring;                       /* Ring index               */
      int pbsf;                        /* Post-beta shift factor   */
      int sjincr;                      /* Stride for Sj            */
      int sfs;                         /* Adjusted xhift factor    */

      int opt_eq_X = (ib->ibopt & IBOPTX != 0); /* TRUE if OPT=X */

/*---------------------------------------------------------------------*
*                                                                      *
*     Stage I--Form the box sums (sums over cells in each group)       *
*                                                                      *
*---------------------------------------------------------------------*/

/* Clear the boxsum array.
*     (Assume a new INHIBBLK will always draw from a new
*     source layer, otherwise why would it be there?--
*     therefore, cannot reuse boxsums from previous block.) */

      memset((char *)(ib->boxsum), 0, l_l1xn2yn2*sizeof(long));

/* Carry out box sums.  V8C: shift Sj by ihsf so have room to add
*  over all the cells in the widest band.  In earlier versions,
*  this was done only when there was multiplication by falloff.  */

      jl = ib->pisrc;
      ligrp = spsize(jl->nel,jl->phshft);
      if (lit) lit -= 1;
      if (lnt) lnt += 1;
      sjincr = jl->lsp;
      pcell = jl->pps[ib->ihdelay];
      sfs = Ss2hi - (int)ib->ihsf;
      for (bptr=ib->boxsum00; bptr<ib->boxsumbb; bptr+=l_l2n1) {
         beptr = bptr + l_l1x1;
         while (bptr <= beptr) {  /* Loop over x */
            long work, wksum = 0;
            plimit = pcell + ligrp;
            for ( ; pcell < plimit; pcell+=sjincr) {
               d3gts2(work, pcell);
               work -= litr;
               if      (work > lit) wksum -= (work - lsit);
               else if (work < lnt) wksum -= (work - lsnt);
               }  /* End of loop over cells */
            /* If requested, square the boxsums.  Rescale by dS14 and
            *  group size to keep in range, NEL*(SUM/NEL)^2 = SUM^2/
            *  NEL.  With mV scales, sum can be +/-, so restore sign
            *  after squaring.  */
            if (ib->ibopt & IBOPGS) {
               double dwks = (double)wksum, dnel = dS14*(double)jl->nel;
               wksum = (long)(dwks * dwks / dnel);
               if (dwks < 0.0) wksum = -wksum;
               }
            /* Store the sum at boxsum(X,Y).  Sum needs to be
            *  on scale S23 - ib->ihsf, but wksum is S7  */
            *bptr++ = (sfs >= 0) ? wksum << sfs : SRA(wksum, -sfs);
            }  /* End of X loop */
         }  /* End of Y loop */

/*---------------------------------------------------------------------*
*                                                                      *
*     Stage II--Fill in borders of boxsum array                        *
*        according to selected boundary conditions                     *
*                                                                      *
*---------------------------------------------------------------------*/

      switch ((enum BoundaryCdx)ib->kbc) {

         long *swatchend;        /* Pointer used by bc=noise          */
         long *topsrc;           /* Top source                        */
         long *botsrc;           /* Bottom source                     */
         long *topdest;          /* Top destination                   */
         long *botdest;          /* Bottom destination                */
         long *leftdest;         /* Left destination                  */
         long *rightsrc;         /* Right source                      */
         long *lsrl;             /* Left source row limit             */
         long l_border;          /* Local copy of border for bc=noise */

case BC_ZERO:     /* Zero boundary */
         break;   /* End of case 0 */

case BC_NORM:     /* Norm */
         break;   /* End of case 1 */

case BC_NOISE:    /* Noise boundary */

/* In first loop, noise value is stored in all boxes from
*     start of boxsum array up to boxsum(0,0) and in the
*     symmetrically located points at the bottom.
*  If ngy > 1, a second loop fills in the sides in swatches
*     of 2*(nr-1) boxes that cover one row at the right and
*     the next row at the left in one contiguous piece.  */

         l_border = ib->border;
         bptr = ib->boxsum;
         beptr = ib->boxsum + l_l1xn2yn2; /* End of boxsum array */
         swatchend = ib->boxsum00 - 1;    /* End of first swatch
                                              at boxsum(-1,0) */
         while (bptr <= swatchend) {
            *(--beptr) = *bptr++ = l_border;
            }

         while ((bptr+=l_l1x) < beptr) {
            swatchend += l_l1xn2;      /* Limit for double swatch */
            while (bptr <= swatchend) *bptr++ = l_border;
            }
         break;  /* End of case 2 */

case BC_EDGE:   /* Nearest edge boundary */

/* First the left and right edges are filled in with
*     duplicates of the adjacent box inside the border.
*  Then the top and bottom are filled in by copying the
*     nearest adjacent full row (including left/right edges).  */

         beptr = ib->boxsumbb;
         for (bptr=ib->boxsum00; bptr<=beptr; bptr+=l_l1xn2) {
            long *leftlim = bptr;
            long left_edge_value;
            long right_edge_value;

            left_edge_value  = *bptr;
            right_edge_value = *(bptr + l_l1x1);
            bptr -= nrm1;  /* Back up to first storage location */

            /* Complete one pair of side swatches */
            while (bptr < leftlim) {
               *(bptr + l_l1xn1) = right_edge_value;
               *bptr++ = left_edge_value;
               }
            }

/* Top/bottom moves--move whole rows with memcpy */

         topdest = ib->boxsum;
         botdest = ib->boxsumbb;
         topsrc  = topdest + l_l1n1xn2;
         botsrc  = botdest - l_l1xn2;
         nrows   = nrm1;                  /* Set row counter */
         nbytes = l_l1xn2 * sizeof(long); /* Bytes per row to move */
         while (nrows--) {
            memcpy((char *)topdest,(char *)topsrc,nbytes);
            topdest += l_l1xn2;
            memcpy((char *)botdest,(char *)botsrc,nbytes);
            botdest += l_l1xn2;
            }
         break;   /* End of case 3 */

case BC_MIRROR:   /* Mirrors at boundaries */

/* First the left and right edges are filled in with
*     mirror points from inside the border.
*  Then the top and bottom are filled in by copying full
*     rows throughs the mirror (including left/right edges).
*  Setup tests assure that nrings <= ngx,ngy so no overlap. */

         lsrl = ib->boxsum00 + nrm1;      /* Left source row limit */
         for (bptr=ib->boxsum00; bptr<ib->boxsumbb; bptr+=l_l1xn1) {
            long *blptr = bptr - 1;
            while (bptr < lsrl) {
               *(bptr + l_l1x) = *(blptr + l_l1x);
               *blptr-- = *bptr++;
               }
            lsrl += l_l1xn2;   /* Move pickup limit to next row */
            }

/* Top/bottom moves--move whole rows with memcpy */

         topsrc  = ib->boxsum + l_l1n1xn2;
         topdest = topsrc - l_l1xn2;
         botdest = ib->boxsumbb;
         botsrc  = botdest - l_l1xn2;
         nbytes = l_l1xn2 * sizeof(long); /* Bytes per move */
         nrows = nrm1;                    /* Number of rows to copy */
         while (nrows--) {
            memcpy((char *)topdest,(char *)topsrc,nbytes);
            topdest -= l_l1xn2;
            topsrc  += l_l1xn2;
            memcpy((char *)botdest,(char *)botsrc,nbytes);
            botdest += l_l1xn2;
            botsrc  -= l_l1xn2;
            }
         break;   /* End of case 4 */

case BC_TOROIDAL:   /* Toroidal boundary conditions */

/* First, duplicate the values at the left and right edges.
*  Then, duplicate the top and bottom.  */

         leftdest = ib->boxsum00 - nrm1;
         rightsrc = leftdest + l_l1x;
         beptr = ib->boxsumbb;
         nbytes = nrm1 * sizeof(long);
         while (rightsrc <= beptr) {   /* Loop to bottom of center
                                       *  region. */
            /* Move right inside to left outside */
            memcpy((char *)leftdest,(char *)rightsrc,nbytes);
            leftdest += nrm1;
            rightsrc += nrm1;

            /* Move left inside to right outside */
            memcpy((char *)rightsrc,(char *)leftdest,nbytes);
            leftdest += nrm1 + l_l1x;
            rightsrc += nrm1 + l_l1x;
            }

/* Top/bottom moves--move entire border area with memcpy */

         nbytes = l_l1n1xn2 * sizeof(long);  /* Length of entire
                                             *  top/bottom borders */
         topdest = ib->boxsum;
         botsrc  = leftdest - l_l1n1xn2;
         /* Move bottom piece to top */
         memcpy((char *)topdest,(char *)botsrc,nbytes);
         topdest += l_l1n1xn2;
         botsrc  += l_l1n1xn2;
         /* Move top piece to bottom */
         memcpy((char *)botsrc,(char *)topdest,nbytes);
         break;    /* End of case 5 */

         }  /* End of boundary-condition switch */

/*---------------------------------------------------------------------*
*                                                                      *
*     Stage III--Apply positional modulation (falloff)                 *
*        Apply to each boxsum now, before any strip sums done.         *
*           Border areas are modulated according to their proper       *
*           distances, thus breaking mirror or toroid symmetries.      *
*        The calculation of the falloff factor is now in the d3foff    *
*           module, which may implement a table lookup or a direct     *
*           calculation.  The user may choose any of three metrics     *
*           and any of four falloff curves.                            *
*        Each calculated modulation factor is applied to four          *
*           boxsums, symmetrically located at the corners of a         *
*           rectangle centered at the center of the boxsum array.      *
*                                                                      *
*---------------------------------------------------------------------*/

/* Skip if there's no positional modulation */

      if (ib->ibopt & IBOPFO) {
         FoffFn d3foff;       /* Pointer to falloff calc func  */
         long *ulcorner;      /* Pointer to upper left corner  */
         long *llcorner;      /* Pointer to lower left corner  */
         long bckskpincr;     /* Back skip increment           */
         long curx,cury;      /* x,y coords of current box     */
         long deltax;         /* x dist betw. symm-equiv boxes */
         long hrpu;           /* Half row portion used         */
         long rowlength;      /* Length of used portion of row */
         long skipincr;       /* Skip increment                */
         long startx;         /* Starting x coord of each row  */
         long totrowlength;   /* Length of all rows used       */

/* Set loop controls for scanning boxsum array */

         if (ib->kbc >= BC_NOISE) {
            /* Boundaries are non-zero.
            *  Set loop control for entire boxsum array.  */
            rowlength = l_l1xn2;
            startx = cury = -nrm1;
            totrowlength = l_l1xn2yn2;
            ulcorner = ib->boxsum;
            }
         else {
            /* Zero (or NORM) boundaries, set loop control for
            *  inner boxsum array only--no borders.  */
            rowlength = l_l1x;
            startx = cury = 0;
            totrowlength = l_l1yxn2;
            ulcorner = ib->boxsum00;
            }

/* Common loop setup for both sizes */

         llcorner = ulcorner + totrowlength - l_l1xn2;
         /* Length of half row portion used = rowlength/2 rounded up */
         hrpu = (rowlength + 1)>>1;
         skipincr = l_l1xn2 - hrpu;
         bckskpincr = l_l1xn2 + hrpu;

/* Loop over boxsum array, applying falloff values
*  to sets of four symmetry-related groups.  */

         d3foff = qfofffnc(ib);
         for (; ulcorner<=llcorner; ulcorner+=skipincr) {
            curx = startx;
            for (deltax=rowlength-1; deltax>=0; deltax-=2) {

               long falloff_val = d3foff(ib, curx, cury);

               /* Upper left */
               *ulcorner = msrsle(*ulcorner, falloff_val,
                  -FBfo, OVF_GEOM);

               /* Upper right.  Omit if same as upper left.  */
               if (deltax > 0) {
                  long *urcorner = ulcorner + deltax;
                  *urcorner = msrsle(*urcorner, falloff_val,
                     -FBfo, OVF_GEOM);
                  }

               /* Lower half.  Omit if on center line. */
               if (llcorner > ulcorner) {
                  /* Lower left */
                  *llcorner = msrsle(*llcorner, falloff_val,
                     -FBfo, OVF_GEOM);

                  /* Lower right.  Omit if same as lower left.  */
                  if (deltax > 0) {
                     long *lrcorner = llcorner + deltax;
                     *lrcorner = msrsle(*lrcorner, falloff_val,
                        -FBfo, OVF_GEOM);
                     }
                  } /* End lower half */

               curx++;     /* Increment x coordinate of current box */
               ulcorner++; /* Move upper pointer to right to center */
               llcorner++; /* Move lower pointer to right */
               }           /* End x loop */

            cury++;        /* Increment y coordinate of current box */
            llcorner -= bckskpincr; /* Move vertical limit up one row */
            } /* End y loop */
         } /* End positional modulation */

/*---------------------------------------------------------------------*
*                                                                      *
*     Stage IV.  Perform band sums                                     *
*        Treat the central group as a band all by itself.              *
*        Then loop over the remaining bands (separate betas).          *
*           Within each band, loop over ngb rings.                     *
*           Ring processing proceeds outward, with first a             *
*           horizontal strip update, then a ring summation,            *
*           and finally a vertical strip update.  If OPT=X,            *
*           then branch to stage V to process the last ring.           *
*        At this stage, the unscaled band sums are stored for          *
*           later use with OPT=X.  Sums may be skipped when            *
*           beta=0 only when OPT=X is not requested for the            *
*           same INHIBBLK (see Stage V).                               *
*                                                                      *
*---------------------------------------------------------------------*/

/* Inner ring processing:
*  Copy boxsums into first bandsum segment and accumulate xtotal =
*  sum of all boxsums.  (xtotal is only needed for OPT=X, but it
*  probably takes longer to test this than just always to add.)  */

      xtotal = 0;
      if (opt_eq_X || ib->inhbnd[0].beta) {
         yeptr = ib->boxsumbb;
         bndsumptr = ib->inhbnd[0].bandsum;
         for (xptr=ib->boxsum00; xptr<yeptr; xptr+=l_l2n1) {
            xeptr = xptr + l_l1x1;
            while (xptr <= xeptr) {
               xtotal += *xptr;
               *bndsumptr++ = *xptr++;
               }
            }
         }

/* Done inner (self) group.  See if there are any outer bands */

      if (!nibm1) goto BANDSFIN;    /* Skip to end of loop if
                                    *  done with all betas */

/* Initialize hstrip array (for horiz strip sums) */

      {  long *source, *dest = ib->hstrip;
         nbytes = l_l1x * sizeof(long);
         for (source = ib->boxsum+nrm1;
               source < ib->boxsum+l_l1xn2yn2;
               source+=l_l1xn2, dest+=l_l1x) {
            memcpy((char *)dest, (char *)source, nbytes);
            }
         } /* End local scope */

/* Initialize vstrip array (for vert strip sums) */

      memcpy((char *)ib->vstrip, (char *)(ib->boxsum+l_l1n1xn2),
         l_l1yxn2*sizeof(long));

/* Start ring number at zero */

      iring = 0;        /* Number of current ring */
      rrhs = rrbs = 0;  /* Zero the vertical ring offsets */
      iband = 0;        /* Number of current band. Note that iband
                        *  is incremented right away in loop below. */

      for (;;) {

         /* Note non-standard loop control */
         if (++iband > nibm1) goto BANDSFIN;    /* Done all bands */

         /* If on last band and OPT=X, skip to stage V */
         if (opt_eq_X && (iband == nibm1)) break;

/* Zero the bandsum array for this band */

         memset((char *)(ib->inhbnd[iband].bandsum), 0,
            l_l1xy*sizeof(long));

         for (ring_cnt=ib->ngb; ring_cnt>0; ring_cnt--) {
            long *strptr;
            iring++;
            rrbs += l_l1xn2;     /* boxsum offset */
            rrhs += l_l1x;       /* hstrip offset */

/* Horizontal strip update.
*  Loop over hstrip array, adding two cells to each
*  horizontal strip, from (x+ring,y) and (x-ring,y).  */

            strptr = ib->hstrip;
            bptr = ib->boxsum + nrm1;
            beptr = ib->boxsum + l_l1xn2yn2;
            for ( ; bptr<=beptr; bptr+=l_l2n1) {
               long *horiz_limit;

               horiz_limit = bptr + l_l1x1;
               for ( ; bptr<=horiz_limit; bptr++) {
                  /* hstrip(X,Y) = hstrip(X,Y) +
                  *  boxsum(X+ring,Y) + boxsum(X-ring,Y) */
                  *strptr++ += *(bptr+iring) + *(bptr-iring);
                  }  /* End loop over columns */
               }  /* End of loop over rows */

/* Ring summation--add four tiles to make a full ring
*  (skip loop if beta = 0).  */

            if (ib->inhbnd[iband].beta || opt_eq_X) {
               long *vlim;
               long *hstrptr;
               long *vstrptr;

               bndsumptr = ib->inhbnd[iband].bandsum;
               hstrptr = ib->hstrip + l_l1n1x;
               vstrptr = ib->vstrip + nrm1;
               vlim = vstrptr + l_l1yxn2;

/* Loop over Y and X, summing four tiles to make a ring */

               for ( ; vstrptr<vlim; vstrptr+=l_l2n1) {
                  long *xlim;
                  for (xlim=vstrptr+l_l1x1; vstrptr<=xlim;
                        hstrptr++, vstrptr++) {
                     *bndsumptr++ += *(hstrptr+rrhs) +   /* Top    */
                                    *(hstrptr-rrhs)  +   /* Bottom */
                                    *(vstrptr-iring) +   /* Left   */
                                    *(vstrptr+iring);    /* Right  */
                     }  /* End loop over X */
                  }  /* End loop over Y */
               }  /* End of ring summation */

/* Vertical strip update.
*  Loop over vstrip array, adding two cells to each
*  vertical strip, from (X,Y+ring) and (X,Y-ring).  */

            strptr = ib->vstrip;
            bptr = ib->boxsum + l_l1n1xn2;
            for (beptr=ib->boxsumbb; bptr<beptr; bptr++) {
               *strptr++ += *(bptr + rrbs) + *(bptr - rrbs);
               }
            }  /* End of loop over rings */
         }  /* End of loop over bandsums */

/*---------------------------------------------------------------------*
*                                                                      *
*     Stage V.  Special last-band processing for OPT=X                 *
*        Instead of a normal last-band sum, here take the              *
*           recorded xtotal and subtract from it the inhibition        *
*           for the inner bands which has already been stored in       *
*           the bandsum array.  It is more efficient to do this        *
*           in a separate pass, as here, whenever ngb > 1, as          *
*           the main summation loop is executed once per ring          *
*           but this is only done once per band.                       *
*        To minimize any memory paging, groups are processed           *
*           sequentially with an outer loop over bands rather          *
*           than adding all bands for one group at a time, even        *
*           though that would sum in a register and save stores.       *
*        For consistency with handling of other bands, scaling         *
*           is postponed to stage VI.                                  *
*                                                                      *
*---------------------------------------------------------------------*/

      if (ib->inhbnd[nibm1].beta) {
         long *bndlim;
         long *ibndsumptr;
         long *obndsumptr;
         long bndcnt;

/* Store xtotal in all groups of outer segment */

         obndsumptr = ib->inhbnd[nibm1].bandsum;
         bndsumptr = obndsumptr;
         bndlim = bndsumptr + l_l1xy;
         while (bndsumptr < bndlim) *bndsumptr++ = xtotal;

/* Subtract all inner bands from outer band for all groups */

         ibndsumptr = ib->inhbnd[0].bandsum;  /* Innermost band */
         bndcnt = nibm1;
         while (bndcnt--) {
            bndsumptr = obndsumptr;           /* Outermost band */
            while (bndsumptr < bndlim) {
               *bndsumptr++ -= *ibndsumptr++; /* Subtract hole sum */
               }
            }
         }  /* End of last bandsum */

BANDSFIN:
      /* Perform (border) verification plot if requested */
      if (RP->kpl & KPLEDG) {
         int rc = d3bxvp(ib);
         if (rc > 0) return rc;
         }

/*---------------------------------------------------------------------*
*                                                                      *
*     Stage VI.  Normalize boundaries, scale, and store band sums.     *
*           Capping is now done in d3inhg or d3inhc.                   *
*        Stage VI must be done after the extended field sums, which    *
*           use unscaled values from the inner bands.  Stage VI only   *
*           needs to loop over bands, not rings.                       *
*        For OPT=X, capping and scaling is performed without           *
*           normalization as the center hole should not be             *
*           expanded outside the bounds included in xtotal.            *
*        This algorithm is incorrect if the boundary condition is      *
*           other than zero, norm, or toroidal with no falloff.        *
*           In this version, these cases are rejected by input         *
*           checking in g2gconn.  It is not exactly clear how the      *
*           other cases would be defined, but probably one would       *
*           set the border to zero, then go back and repeat all        *
*           the ring sums, subtracting from last band sum.             *
*        Another decision: to loop over all bands, one cell            *
*           (thus doing fewer evaluations of normalization)            *
*           or do all cells, one band (thus avoiding cache loads       *
*           and paging due to large stride).  This may be wrong,       *
*           but I pick the former as being less bad in a big run.      *
*                                                                      *
***********************************************************************/

/* Prepare for normalization */

      do_norm = (ib->kbc == BC_NORM);     /* TRUE if BC=NORM */
      xy = ib->xynorm;
      gpsnorm = ib->l1area;

/* Loop over bands, then y, then x.
*  If beta=0, clear band sums and proceed directly to next band. */

      pbsf = (int)ib->ihsf - 23;
      e64push(E64_COUNT, povct);
      for (iband=0; iband<=nibm1; iband++) {
         long xcoord, ycoord;
         si32 l_beta, wkbeta;
         bndsumptr = ib->inhbnd[iband].bandsum;

         if ((l_beta = ib->inhbnd[iband].asbeta) == 0) {
            memset((char *)bndsumptr, 0, l_l1xy*sizeof(long));
            continue;
            }

         /* If about to do last band and OPT=X, kill norm */
         if (opt_eq_X && (iband == nibm1)) do_norm = FALSE;

         for (ycoord=0; ycoord<l_l1y; ycoord++) {
            for (xcoord=0; xcoord<l_l1x; xcoord++,bndsumptr++) {
               /* Normalized beta = beta*(total area)/(used area) */
               wkbeta = (do_norm) ? dm64nq(l_beta, gpsnorm,
                  ((si32)xy[xcoord].x)*((si32)xy[ycoord].y)) : l_beta;

/* Scale total bandsum for current group.  Dynamic scaling is used
*  to improve accuracy.  Given ib->ihsf = 1+log2(max terms in sum-1),
*  sum is (S23/30-ihsf), times beta (S20) yields S(43/50-ihsf),
*  rescaled to (S20/27) for storage.  */

               *bndsumptr = msrsle(*bndsumptr, wkbeta, pbsf, OVF_GEOM);
               }  /* End of loop over x */
            }  /* End of loop over y */
         }  /* End of loop over bands */
      e64pop();
      }}  /* End else and grand loop over INHIBBLKs */

   return D3_NORMAL;
   }  /* End d3inhb() */


/***********************************************************************
*                                                                      *
*                               d3inhg                                 *
*                                                                      *
***********************************************************************/

void d3inhg(void) {

   FoffFn d3foff;             /* Pointer to falloff calc fn */
   struct CELLTYPE *il;       /* Pointer to current CELLTYPE */
   struct INHIBBLK *ib;       /* Pointer to current INHIBBLK */
   struct INHIBBAND *pb;      /* Pointer to current INHIBBAND */
   long *pgx;                 /* Pointer to afference data */
   long gn,gx,gy;             /* Current group coordinates */
   int  iband;                /* Number of current band */
   int  laffs,p;
   ui16 ccyc0;                /* Current cycle (0-based) */

/* Setups before INHIBBLK loop */

   il = CDAT.cdlayr;             /* Locate CELLTYPE */
   if (!(il->ctf & CTFGV))       /* Clear afference accumulators */
      memset((char *)CDAT.pGeomSums, 0, CDAT.lGeomSums);
   laffs = 1 << il->pdshft;
   ccyc0 = CDAT.curr_cyc1 - 1;

/* Loop over all INHIBBLKs for this cell type */

   gn = CDAT.groupn, gx = CDAT.groupx, gy = CDAT.groupy;
   for (ib=il->pib1; ib; ib=ib->pib)
         if (CDAT.curr_cyc1 > ib->gcdefer) {

/* Locate proper falloff calculation function */

      d3foff = qfofffnc(ib);

/* Loop over bands */

      ib->rnsums[EXCIT] = ib->rnsums[INHIB] = 0;
      pb = ib->inhbnd;           /* Locate band data */
      for (iband=0; iband<ib->nib; iband++, pb++) {

/* For all bands (all but the innermost band if IBOEFV), cap
*  the bandsums and accumulate the totals into rnsums.  For
*  the innermost band if IBOEFV, instead store the scaled
*  total in r0sum and the regenerated scale factor in r0scale.
*  An awkwardness here is that the scale factor for each group
*  has already been calculated in d3inhb on the fly, but there
*  is no room to store all these scales, so the one we want now
*  must be reconstructed as wkbeta = beta * norm * falloff.
*  V8C, 10/12/03, GNR - Corrected norm to use l1area, not l1xy. */

         if (pb->beta) {
            long wksum = pb->bandsum[gn];
            si32 wkbeta;
            if (iband > 0 || !(ib->ibopt & IBOEFV)) {
               /* Outer band or any band when not IBOEFV */
               if (wksum >= 0)
                  ib->rnsums[EXCIT] += min(wksum, pb->mxib);
               else
                  ib->rnsums[INHIB] += max(wksum, -(pb->mxib));
               }
            else {
               /* Scaled inner band */
               ib->r0sum = wksum;
               /* Calculate overall inner band scale */
               wkbeta = pb->asbeta;
               /* Look up falloff factor and correct wkbeta */
               if (ib->ibopt & IBOPFO) {
                  long falloff = d3foff(ib, gx, gy);
                  wkbeta = mssle(wkbeta, falloff, -FBfo, OVF_GEOM);
                  } /* End looking up falloff factor */
               /* Look up and apply normalization factor */
               if (ib->kbc == BC_NORM) {
                  struct XYNORMDEF *xy = ib->xynorm;
                  wkbeta = dm64nq(wkbeta, ib->l1area,
                     ((si32)xy[gx].x) * ((si32)xy[gy].y));
                  }
               ib->r0scale = wkbeta;
               } /* End ring 0 */
            } /* End if beta */

         } /* End loop over bands */

/* All bands for this INHIBBLK are now done.  If CTFGV, items needed
*  for individual cell calculations have been stored, there is no more
*  to do here.  Otherwise, finish up for current group, handling excit
*  and inhib contributions separately, and using d3decay to apply
*  decay.  If celltype has phase, distribute gcon uniformly, otherwise
*  there is just one term.  */

      if (il->ctf & CTFGV) continue;

      pgx = CDAT.pAffData + ib->ogafs;

      if (ib->gssck & PSP_POS) {                   /* EXCIT terms */

         long *pSums = CDAT.pAffData + ib->ogsei[EXCIT];
         long gaxp = *pgx++ = ib->rnsums[EXCIT];
         /* N.B. 04/19/12, GNR - Here w/OPT=F we store current value
         *  as persistence for next time but do not add omega times
         *  it into itself--to give a more gentle startup.  */
         if (ib->GDCY.omega) {                     /* Apply decay */
            if (CDAT.curr_cyc1 == ib->gfscyc)
               *pgx++ = ib->ieveff[gn] = gaxp;
            else
               gaxp = d3decay(&ib->GDCY, ib->ieveff+gn, gaxp, pgx++);
            }
         *pgx++ = gaxp;
         if (ccyc0 >= ib->gdefer) {
            for (p=0; p<laffs; p++) pSums[p] += gaxp;
            }

         } /* End excitatory terms */

      if (ib->gssck & PSP_NEG) {                   /* INHIB terms */

         long *pSums = CDAT.pAffData + ib->ogsei[INHIB];
         long gaxn = *pgx++ = ib->rnsums[INHIB];
         if (ib->GDCY.omega) {                     /* Apply decay */
            if (CDAT.curr_cyc1 == ib->gfscyc)
               *pgx++ = ib->iiveff[gn] = gaxn;
            else
               gaxn = d3decay(&ib->GDCY, ib->iiveff+gn, gaxn, pgx++);
            }
         *pgx++ = gaxn;
         if (ccyc0 >= ib->gdefer) {
            for (p=0; p<laffs; p++) pSums[p] += gaxn;
            }

         } /* End inhibitory terms */

      } /* End loop over INHIBBLKs */

   } /* End d3inhg() */


/***********************************************************************
*                                                                      *
*                               d3inhc                                 *
*                                                                      *
***********************************************************************/

void d3inhc(void) {

   struct CELLTYPE *il;       /* Pointer to current CELLTYPE */
   struct INHIBBLK *ib;       /* Pointer to current INHIBBLK */
   struct INHIBBAND *pb;      /* Pointer to current INHIBBAND */
   long *pgx;                 /* Pointer to afference data */
   long gaxp,gaxn;            /* Positive,negative sums */
   long gn;                   /* Current group number */
   int  laffs,p;
   ui16 ccyc0;                /* Current cycle (0-based) */

/* If no INHIBBLK has effective self-avoidance (IBOEFV), return at
*  once, all work has been done in d3inhg().  (This is just a safety,
*  we should not be called from d3go() in this case.)  */

   il = CDAT.cdlayr;
   if (!(il->ctf & CTFGV)) return;

/* Otherwise, it is necessary to process all INHIBBLKs, not just
*  the ones with IBOEFV set, because we need to clear the GeomSums
*  and add all terms into them--there is no place to store partial
*  sums from the INHIBBLKs that do not have IBOEFV set, alas.  */

   memset((char *)CDAT.pGeomSums, 0, CDAT.lGeomSums);
   laffs = 1 << il->pdshft;
   gn = CDAT.groupn;
   ccyc0 = CDAT.curr_cyc1 - 1;
   for (ib=il->pib1; ib; ib=ib->pib)
         if (CDAT.curr_cyc1 > ib->gcdefer) {

/* Initialize totals from d3inhg() data for outer bands */

      pb = ib->inhbnd;
      gaxp = ib->rnsums[EXCIT];
      gaxn = ib->rnsums[INHIB];

/* If this is an INHIBBLK with effective self-avoidance, and if
*  activity of the current cell at (t-1) exceeds 'it' threshold,
*  scale it and subtract from saved central ring sum (algebraic
*  add--neg sums stored).  Save this as the adjusted first band
*  sum.  */

      if (ib->ibopt & IBOEFV) {

         long litt, work, wksum = ib->r0sum;
         long effsi = CDAT.old_siS7 - (long)ib->gjrev;
         litt = (long)ib->itt; if (litt) litt -= 1;
         if (effsi > litt) {
            work = effsi - (long)ib->subitt;
            goto UseSiVal;
            }
         litt = (long)ib->ittlo; if (litt) litt += 1;
         if (effsi < litt) {
            work = effsi - (long)ib->subnitt;
            goto UseSiVal;
            }
         goto OmitSiVal;
UseSiVal:
         wksum += msrsle(work, ib->r0scale, -FBsi, OVF_GEOM);
         pb->bandsum[gn] = wksum;

         /* Apply cap to inner band sum and add into outer band sums */
         if (wksum >= 0)
            gaxp += min(wksum, pb->mxib);
         else
            gaxn += max(wksum, -(pb->mxib));
         } /* End adjusting for current cell */
OmitSiVal: ;

/* Apply decay, store detail info and accumulate sums according
*  to mode of excitation or inhibition, cf. d3inhg().  */

      pgx = CDAT.pAffData + ib->ogafs;

      if (ib->gssck & PSP_POS) {                   /* EXCIT terms */

         long *pSums = CDAT.pAffData + ib->ogsei[EXCIT];
         *pgx++ = gaxp;
         if (ib->GDCY.omega) {                     /* Apply decay */
            if (CDAT.curr_cyc1 == ib->gfscyc)
               *pgx++ = ib->ieveff[CDAT.cdcell] = gaxp;
            else gaxp = d3decay(&ib->GDCY, ib->ieveff+CDAT.cdcell,
               gaxp, pgx++);
            }
         *pgx++ = gaxp;
         if (ccyc0 >= ib->gdefer) {
            for (p=0; p<laffs; p++) pSums[p] += gaxp;
            }

         } /* End excitatory terms */

      if (ib->gssck & PSP_NEG) {                   /* INHIB terms */

         long *pSums = CDAT.pAffData + ib->ogsei[INHIB];
         *pgx++ = gaxn;
         if (ib->GDCY.omega) {                     /* Apply decay */
            if (CDAT.curr_cyc1 == ib->gfscyc)
               *pgx++ = ib->iiveff[CDAT.cdcell] = gaxn;
            else gaxn = d3decay(&ib->GDCY, ib->iiveff+CDAT.cdcell,
               gaxn, pgx++);
            }
         *pgx++ = gaxn;
         if (ccyc0 >= ib->gdefer) {
            for (p=0; p<laffs; p++) pSums[p] += gaxn;
            }

         } /* End inhibitory terms */

      } /* End INHIBBLK loop */

   } /* End d3inhc() */

