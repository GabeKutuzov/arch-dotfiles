/* (c) Copyright 1995-2003, Neurosciences Research Foundation, Inc. */
/* $Id: d3rprt.c 3 2008-03-11 19:50:00Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                              d3rprt.c                                *
*                                                                      *
*                 Region/Repertoire printing routine                   *
*                                                                      *
*   ---This function should be called in parallel from all nodes---    *
*         (Although, currently comp nodes have nothing to do)          *
*                                                                      *
*  Synopsis:   void d3rprt(struct REPBLOCK *ir)                        *
*  Argument:   'ir' is a ptr to the REPBLOCK of the repertoire         *
*              whose data are to be printed                            *
*                                                                      *
*  V7C, 01/18/95, GNR - Convert from V1K Fortran, parallelize, allow   *
*                       for phase, omit line heads if only one layer   *
*  V8A, 03/01/97, GNR - Eliminate d3exch() call, all s(i) now on host  *
*  V8C, 03/29/03, GNR - Cell responses in millivolts--revise output    *
*                       scaling for 9 hyperpol. + 9 depol. log ranges  *
*  ==>, 12/28/07, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#ifndef PARn
#include "rocks.h"
#include "rkxtra.h"
#include "rkarith.h"
#endif

#define LHLFMAP 9       /* Length of pos or neg character half-map */

/*=====================================================================*
*         Comp node code for d3rprt--currently nothing to do           *
*=====================================================================*/

#ifdef PARn
void d3rprt(struct REPBLOCK *ir) {

   } /* End d3rprt */
#endif

/*=====================================================================*
*       Serial and host code for d3rprt--print repertoire data         *
*=====================================================================*/

#ifndef PARn
void d3rprt(struct REPBLOCK *ir) {

   struct CELLTYPE *il;    /* Ptr to a cell type control block */
   s_type *ps1;            /* Ptr to s(i) data in page loop */
   s_type *ps3;            /* Ptr to s(i) data in vert loop */
   s_type *ps4,*ps4e;      /* Ptr to, end of s(i) data in cell loop */
   s_type *ps5,*ps5e;      /* Ptr to, end of s(i) data in box loop */
   s_type *ps6,*ps6e;      /* Ptr to, end of s(i) data in row loop */

   long iv;                /* Cell activity bin */
   long lgpsp;             /* Length of s,phi data for one group */
   long lngx,lngy;         /* Number of groups along x,y */
   long lnqg;              /* Lines needed to print one group */
   long lnqxsp;            /* Length of s,phi data for rnqx cells */
   long ncr;               /* Number of s,phi data remaining */
   long nhc;               /* Number of s,phi data in block */
   long rnqx,rnqy;         /* Effective size of print block along x,y */
   long rows;              /* Rows in all blocks (excluding gutters) */

   int bpp;                /* Blocks per page (horiz) */
   int bxr;                /* Blocks remaining = Blocks -
                           *     (pages done)*bpp */
   int gpb;                /* Groups per box (vertical) = nqy/nqg */
   int gpp;                /* Groups per page (horizontal) (Case II) */
   int gppsp;              /* Length of s,phi data for gpp groups */
   int ic;                 /* Character selector */
   int ip1;                /* Vertical strip (page) counter */
   int ip2;                /* Vertical group counter */
   int kml;                /* True if printing > 1 layer */
   int kms;                /* True if printing > 1 strip */
   int lc0,lc1;            /* Starting, working column in box */
   int llsp;               /* Local copy of il->lsp */
   int lphs;               /* Local copy of il->phshft */
   int mxlrn;              /* Maximum printable rep name length */
   int nhb;                /* Horizontal boxes on this page =
                           *     min(bxr,bpp) */
   int npx;                /* Number of columns printed along x */
   int nqx1,nqy1;          /* One plus nqx,nqy (size of print block)
                           *     (nqy1 must be int for convrt call) */
   int nspout;             /* Number of lines to spout */

   /* Characters to be printed for each of 18 ranges of s(i) */
   static char CHARS[2*LHLFMAP] = "{[(<=~-: .01234568";
   char line[LNSIZE+1];    /* Output text assembly */

/* Determine numbers of rows and columns in blocks and full output */

   lnqg = ir->nqg;
   lngx = ir->ngx, lngy = ir->ngy;
   rnqx = ir->nqx, rnqy = ir->nqy;
   rows = lnqg*lngy;       /* Total rows in all blocks */
   mxlrn  = min(ir->lrn, 78);
   nspout = (ir->Rp.krp & KRPSP) ? (rows + (rows-1)/rnqy + 2) : 0;
   rnqy = min(rnqy,rows);
   nqx1 = rnqx + 1, nqy1 = rnqy + 1;

/*---------------------------------------------------------------------*
*                Case I:  More than one cell per group                 *
*---------------------------------------------------------------------*/

   if (ir->ncpg > 1) {

      gpb = rnqy/lnqg;
      kml = ir->nlyr > 1;
      bpp = (LNSIZE - (kml ? (LSNAME+1) : 0))/nqx1;
      bxr = lngx;
      kms = (bxr > bpp);

/* Loop over pages (vertical strips needed if repertoire
*  is wider than will fit on one page) */

      for (ip1=0; ip1<lngx; ip1+=bpp, bxr-=bpp) {

         /* Print header for current strip.  (Extra blank in cryout
         *  string is so convrt string can start with a character that
         *  is not an ANSI carriage control, thus continuing same line.)
         */ 
         if (nspout) spout(nspout);
         tlines(rnqy+3);
         cryout(RK_P4,"0",RK_LN2+1,getrktxt(ir->hrname),mxlrn,
            " Region/Repertoire ",RK_CCL,NULL);
         if (kms)             /* Append cell number if multi-strip */
            convrt("('(Strip beginning at group ',J0I8H))",&ip1,NULL);

         /* Establish pointer to first cell on page for each layer */
         for (il=ir->play1; il; il=il->play) {
            il->ctwk.rprt.icp = il->pps[0] +
               spsize(ip1,il->phshft)*il->nel; }

         /* Establish number of boxes in a row on this page */
         nhb = min(bxr,bpp);
         npx = nhb*nqx1;

/* Loop over groups in vertical direction */

         for (ip2=0; ip2<lngy; ip2++) {

            /* Insert a blank line every gpb groups */
            if (ip2 % gpb == 0) convrt("(P4LX4)",&nqy1,NULL);

/* Loop over layers of cells, printing one or more lines for each */

            for (il=ir->play1; il; il=il->play) {

               llsp = il->lsp;
               lphs = il->phshft;
               lnqxsp = spsize(rnqx,lphs);
               ncr = lgpsp = spsize(il->nel,lphs);
               ps4e = (ps4 = il->ctwk.rprt.icp) + lgpsp;

/* Loop over lines devoted to current layer in current box */

               for ( ; ps4<ps4e; ps4+=lnqxsp, ncr-=lnqxsp) {

                  memset(line,' ',npx);
                  lc0 = 1;
                  nhc = min(ncr,lnqxsp);

/* Loop over horizontal boxes, leaving blank columns between them */

                  ps5e = ps4 + lgpsp*nhb;
                  for (ps5=ps4; ps5<ps5e; ps5+=lgpsp, lc0+=nqx1) {

                     lc1 = lc0;

/* Loop over cells within a line within a box.
*  Map s(i) to a character symbol.  Due to the larger possible range
*  of s(i) starting with V8C, this mapping is now based on separate
*  log scales for hyperpolarizing and depolarizing responses.  */

                     ps6e = ps5 + nhc;
                     for (ps6=ps5; ps6<ps6e; ps6+=llsp, lc1++) {
                        d3gts2(iv,ps6);
                        if (iv >= 0) {
                           ic = LHLFMAP + bitsz((iv>>(FBsi-1)) + 1);
                           if (ic >= 2*LHLFMAP) ic = 2*LHLFMAP - 1;
                           }
                        else {
                           ic = LHLFMAP - bitsz((labs(iv)>>(FBsi-1))+1);
                           if (ic < 0) ic = 0;
                           }
                           line[lc1] = CHARS[ic];
                        } /* End ps6 loop */
                     } /* End ps5 loop */

                  /* Print current line */
                  if (kml) cryout(RK_P4," ",RK_LN1+1,il->lname,LSNAME,
                     line,npx,NULL);
                  else
                     cryout(RK_P4,line,RK_LN1+npx,NULL);
                  } /* End ps4 loop */

               /* Row is complete for this layer,
               *  advance starting pointer to next row */
               il->ctwk.rprt.icp += lgpsp*lngx;
               } /* End cell layer loop */

            } /* End vertical group loop */
         } /* End page (horizontal strip) loop */
      } /* End Case I */

/*---------------------------------------------------------------------*
*                Case II:  Exactly one cell per group                  *
*---------------------------------------------------------------------*/

   else {

/* This special case is provided to eliminate some of the white space
*  between groups and the redundant layer names when NCPG=1.  (In this
*  case there can be only one celltype in the repertoire.)  The groups
*  are formatted as a straight two-dimensional array of (NGX,NGY), 
*  with white space determined by (NQX,NQY).  It is not required that
*  NQX,NQY be factors of NGX,NGY.  */ 

      il = ir->play1;
      llsp = il->lsp;
      lphs = il->phshft;
      lnqxsp = spsize(rnqx,lphs);
      ncr = lgpsp = spsize(lngx,lphs);
      ps1 = il->pps[0];
      bpp = LNSIZE/nqx1;
      gpp = bpp*rnqx;
      gppsp = spsize(gpp,lphs);
      bxr = (lngx-1)/rnqx + 1;
      kms = (bxr > bpp);

/* Loop over pages (vertical strips needed if repertoire
*  is wider than will fit on one page) */

      for (ip1=0; ip1<lngx; ip1+=gpp,ps1+=gppsp,bxr-=bpp,ncr-=gppsp) {

         /* Print header for current strip */
         if (nspout) spout(nspout);
         tlines(rnqy+3);
         cryout(RK_P4,"0",RK_LN2+1,getrktxt(ir->hrname),mxlrn,
            " Region/Repertoire ",RK_CCL,NULL);
         if (kms)             /* Append cell number if multi-strip */
            convrt("('(Strip beginning at group ',J0I8H))",&ip1,NULL);

         /* Establish number of boxes in a row on this page */
         nhb = min(bxr,bpp);
         npx = nhb*nqx1;

/* Loop over groups in vertical direction */

         ps3 = ps1;
         for (ip2=0; ip2<lngy; ip2++, ps3+=lgpsp) {

            /* Insert a blank line every gpb groups */
            if (ip2 % rnqy == 0) convrt("(P4LX4)",&nqy1,NULL);

            memset(line,' ',npx);
            lc0 = 1;
            nhc = min(ncr,lnqxsp);

/* Loop over horizontal boxes, leaving blank columns between them */

            ps5e = ps3 + lnqxsp*nhb;
            for (ps5=ps3; ps5<ps5e; ps5+=lnqxsp, lc0+=nqx1) {

               lc1 = lc0;

/* Loop over cells within a line within a box.
*  Map s(i) to character symbol */

               ps6e = ps5 + nhc;
               for (ps6=ps5; ps6<ps6e; ps6+=llsp, lc1++) {
                  d3gts2(iv,ps6);
                  if (iv >= 0) {
                     ic = LHLFMAP + bitsz((iv>>(FBsi-1)) + 1);
                     if (ic >= 2*LHLFMAP) ic = 2*LHLFMAP - 1;
                     }
                  else {
                     ic = LHLFMAP - bitsz((labs(iv)>>(FBsi-1))+1);
                     if (ic < 0) ic = 0;
                     }
                  line[lc1] = CHARS[ic];
                  } /* End ps6 loop */
               } /* End ps5 loop */

            /* Print current line */
            cryout(RK_P4,line,RK_LN1+npx,NULL);

            } /* End ps3 loop */
         } /* End ps1 loop */
      } /* End Case II */

   } /* End d3rprt */
#endif

