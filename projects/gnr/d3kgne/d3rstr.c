/* (c) Copyright 1991-2011, Neurosciences Research Foundation */
/* $Id: d3rstr.c 7 2008-05-02 22:16:44Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3rstr                                 *
*                                                                      *
*                RESTORE A NETWORK FROM A SAVENET FILE                 *
*                                                                      *
*  This routine to be called in parallel on all nodes.  There are no   *
*  arguments--routine now handles all repertoires before returning.    *
*  Routine works with Vers. 8 SAVENET file format.  It assumes that    *
*  d3fchk has been called to set up needed values in repertoire tree.  *
*  Routine must be called before d3genr for two reasons: (1) values    *
*  left in ctwk,cnwk by d3fchk must be preserved, (2) space for phases *
*  added in d3genr to s(i) array will be destroyed in d3rstr.  See     *
*  d3save.c for a discussion of little- vs. big-endian issues.         *
*                                                                      *
*  The key design idea to understand is that elements such as cell and *
*  repertoire data may contain padding, and we must be able to restore *
*  a file to a machine that has a different padding arrangement than   *
*  the one where the file was written.  To this end, d3save just dumps *
*  all the data as it exists in memory, padding and all, and records   *
*  in the file headers the total lengths (ls, lel, llt) including the  *
*  padding.  During restoration, d3rstr must skip over the padding     *
*  while reading only the data actually needed.  This is accomplished  *
*  by seeking to the appropriate location before each read using an    *
*  initial offset and data length per cell left behind by d3fchk.      *
*  This design makes sense because (1) it favors efficiency during     *
*  saving, which is done more frequently than restoring, (2) the seeks *
*  are necessary in any event to skip over information that the user   *
*  has chosen to override, therefore the ability to skip over the pad- *
*  ding comes at no additional cost, (3) it is now possible to restore *
*  in a different order than the data were saved and even to restore   *
*  data to more than one target cell or connection type.               *
*                                                                      *
*  In the present design, it is not possible to restore partial data   *
*  for individual cells or connections and generate the rest, with the *
*  one exception of phase information.  At the cell level, the code    *
*  can read partial data (up to lsrstr bytes) and skip the rest.  The  *
*  items not restored are currently dynamic and not generated, but     *
*  this could be changed.  A similar mechanism could be introduced     *
*  at the connection level if necessary, allowing generation of items  *
*  beyond some single cutoff point.  This would slow down the restore. *
*  It would be even worse to try to restore individual noncontiguous   *
*  data items.  This is left to the future if needed.                  *
*                                                                      *
*  Given that the data must be read serially from the restore files on *
*  the host, there doesn't seem to be any faster alternative to the    *
*  linear message-passing algorithm used here to distribute the data   *
*  to the comp nodes.                                                  *
************************************************************************
*  V1A, 12/13/90, M.COOK                                               *
*  Rev, 09/26/91,  MC - Partial adaptation for Version 2 SAVENET files *
*  V5C, 02/05/92, GNR - Complete rewrite, add D1, do all reps at once  *
*  Rev, 12/09/92, ABP - Add HYB code                                   *
*  V6C. 08/16/93, GNR - Use findcell() to prepare for load optimiz.    *
*  V6D, 02/07/94, GNR - Add axonal delays                              *
*  V7B, 07/07/94, GNR - Eliminate klcd to avoid swapping cnwk.phs1,    *
*                       allow for machine-dependent padding in ls data *
*  V8A, 11/28/96, GNR - Remove support for non-hybrid version          *
*  Rev, 03/01/97, GNR - Distribute s(i) by broadcast stream            *
*  V8A, 09/28/97, GNR - Version 8 SAVENET file with self-descriptive   *
*                       header data, independent allocation for each   *
*                       conntype, add SFREPHDR, GC & MOD phase seeds.  *
*  V8B, 12/16/00, GNR - Move to new memory management routines         *
*  V8C, 03/27/03, GNR - Cell responses in millivolts, add conductances *
*  ==>, 12/28/07, GNR - Last mod before committing to svn repository   *
*  V8H, 06/03/11, GNR - Use generalized setinsi,initsi routines        *
***********************************************************************/

#define RKFILE struct RFdef

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "d3fdef.h"
#ifdef D1
#include "d1def.h"
#endif
#include "savblk.h"
#include "rocks.h"

extern struct LIJARG Lija;

/*---------------------------------------------------------------------*
*                               d3rstr                                 *
*                                                                      *
*  Read data from savenet files and transmit to nodes for storage      *
*---------------------------------------------------------------------*/

void d3rstr(void) {

#ifdef PAR
   struct NNSTR   nnio;       /* Control block for node-node i/o */
#endif
#ifndef PARn
   struct RFdef   *pdf;       /* Si, phase file descriptor */
   struct RFdef   *xfd;       /* Layer,conntype file descriptor */
#endif
   struct REPBLOCK *ir;
   struct CELLTYPE *il;
   struct CONNTYPE *ix;
   s_type          *pd;       /* Buffer for reading data */
   s_type          *p1;       /* Data ptr */

   long lbuf;                 /* Length of pd data buffer */
   long lls;                  /* Local copy of ls */
#ifdef PAR0
   long ncr;                  /* Cells remaining on current node */
#else
   long coff;                 /* Cell data offset  */
   long llel;                 /* Local copy of lel */
   long lllt;                 /* Local copy of llt */
#endif

   int klsd,krrd;             /* Flags for reading "ls", prd data */
#ifdef PAR0
   int node;                  /* Node communications */
#endif

/* All restore files are left open by d3fchk().  Code here to open
*  files after NCUBE overlay was removed, 11/28/96.  */

#ifdef D1
/*---------------------------------------------------------------------*
*                          Read the D1 data                            *
*---------------------------------------------------------------------*/

   struct D1BLK *pd1;
   long d1size;                           /* Length of all data */
   long nodes = NC.total - NC.headnode;   /* Nodes used */

/* Loop over all D1 blocks */

   for (pd1=RP->pd1b1; pd1; pd1=pd1->pnd1b) {
      /* Omit block if override-restore bit is set */
      if (pd1->kd1p & KD1_OVER) continue;
      /* Omit block if no cells--this is just a sanity check */
      if ((d1size = pd1->nd1r*pd1->nepr) <= 0) continue;

/* Find the file descriptor that applies to this block.
*  Seek to the required s(i) data.  */

#ifndef PARn
      {  int ii;
         for (pdf=d3file[SAVREP_IN],ii=1;
            ii<pd1->rfile; pdf=pdf->nf,++ii) ;
         } /* End ii scope */
      rfseek(pdf, pd1->D1offsi, SEEKABS, ABORT);
#endif

/* Read in the s(i) data and broadcast to all nodes */

#ifdef PAR0
      p1 = RP0->ps0;
#else
      p1 = pd1->pd1s;
#endif
#ifndef PARn
      rfread(pdf, p1, d1size, ABORT);
#endif
#ifdef PAR0
      nnpcr(&nnio, BCSTADDR, RESTORE_MSG);
      nnput(&nnio, p1, d1size);
      nnpcl(&nnio);
#endif
#ifdef PARn
      nngcr(&nnio, BCSTADDR, RESTORE_MSG);
      nnget(&nnio, p1, d1size);
      nngcl(&nnio);
#endif

/* Now seek to the repertoire data.
*  (With present file arrangement, we are in fact already there,
*  but this will cover future changes in accord with the spec.)  */

#ifndef PARn
      rfseek(pdf, pd1->D1offrd, SEEKABS, ABORT);
#endif

/* Read the pd1r repertoire data */

#ifdef PARn
      /* Parallel comp node:  Receive data from host in 8K chunks.
      *  (No prompt is necessary because host serializes
      *  sending to all the other nodes.)  */
      if (pd1->mynd1) {       /* (A node can have no data) */
         nngcr(&nnio, NC.hostid, RESTORE_MSG);
         nnget(&nnio, pd1->pd1r, pd1->mynd1*pd1->nbyt);
         nngcl(&nnio);        /* Close data stream */
         } /* End receiving data */
#endif

#ifdef PAR0
      /* Parallel host:  Read the data and send to other nodes */
      pd = angetpv(MAX_MSG_LENGTH, "D1 restore buffer");
      for (node=0; node<nodes; node++) {
         long msize,nsize;    /* Data length */
         /* Calculate nsize = size of data on current target node.
         *  Terminate loop when nsize becomes zero. */
         if (!(nsize = (pd1->cpn + (node < pd1->crn))*pd1->nbyt))
            break;
         /* Open a stream to the current target node */
         nnpcr(&nnio, node+NC.headnode, RESTORE_MSG);
         /* Read and forward the data in 8K chunks */
         while (msize = min(nsize, MAX_MSG_LENGTH)) {
            rfread(pdf, pd, msize, ABORT);
            nnput(&nnio, pd, msize);
            nsize -= msize;
            } /* End copying file data for this node */
         nnpcl(&nnio);  /* Close data stream */
         } /* End loop over comp nodes */
      anrelp(pd);       /* Release read buffer */
#endif /* PAR0 */

      /* Serial:  Read repertoire data directly */
#ifndef PAR
      rfread(pdf, pd1->pd1r, d1size*pd1->nbyt, ABORT);
#endif

      } /* End loop over D1 blocks */
#endif /* D1 */

/*---------------------------------------------------------------------*
*                      Read the normal CNS data                        *
*---------------------------------------------------------------------*/

/* Loop over all repertoires and celltypes */

   for (ir=RP->tree; ir; ir=ir->pnrp) {

      /* (If there were any repertoire-level data, this would
      *  be the place to read them in--but there aren't any.) */

      for (il=ir->play1; il; il=il->play) {

#ifndef PARn
         /* On host, pick up the file descriptor for cell data */
         pdf  = il->ctwk.phs1.fdes;
#endif

/*---------------------------------------------------------------------*
*                       Process s(i), phi data                         *
*---------------------------------------------------------------------*/

/* Omit si,phi data if user requested "Override Restore" or "Noise
*  Restore" option.  N.B.  After d3fchk has run, the il->Ct.ctopt
*  OPTOR bit refers only to cell-level data.  The cnopt NOPOR bit
*  controls restoration of connection data.
*
*  The basic idea of this code is as follows:  In the serial case,
*  when the input data match the memory data, just read into the
*  data areas.  Otherwise, read into a large buffer in blocks and
*  perform the necessary memory to memory data transformations. In
*  the parallel case, the host reads in the data, saves as for the
*  serial host, and also broadcasts to all nodes, which perform the
*  necessary transformations in duplicate.  However, to minimize
*  the length of the broadcasts, unused phase info is removed at
*  the host before broadcasting.
*
*  Rev, 03/01/97, GNR - Use a broadcast stream, allowing multiple
*                       delay times to be buffered together.
*  Rev, 03/27/03, GNR - Ability to convert old one-byte s(i) to new
*                       two-byte, S7 s(i).
*/

         if (!(il->Ct.ctopt & (OPTOR|OPTNR))) {

            int ExpandSi = (il->ctf & RSTOLDSI) != 0;
            int CopyPhi  = il->ctwk.phs1.rfphshft & (int)il->phshft;
            /* Length of old and new s(i) data */
            int ls1 = (1 << il->ctwk.phs1.rfphshft) + (1 - ExpandSi);
            int ls2 = (int)il->lsp;
            /* Delay counter, number of delay rows to be restored */
            int idly,ndrr = min((int)il->mxsdelay1,
               (int)il->ctwk.phs1.rfdelay);
            long siz1 = il->nelt*ls1;

#ifndef PARn   /* Serial and parallel host */
            /* Seek to the s(i) data */
            rfseek(pdf, il->ctwk.phs1.offsi, SEEKABS, ABORT);
            /* Allocate a buffer if cannot restore in place. It
            *  must be a multiple of the incoming record length
            *  to avoid having any data split between two buffer
            *  reads.  */
            if (ls1 != ls2) {
               lbuf = MAX_MSG_LENGTH - MAX_MSG_LENGTH%ls1;
               pd   = (s_type *)mallocv(lbuf, "Restore buffer");
               }
#ifdef PAR0
            /* Open broadcast stream */
            nnpcr(&nnio, BCSTADDR, RESTORE_MSG);
#endif

#else /* is PARn */
            /* Allocate a buffer if cannot restore in place. It
            *  must be a multiple of the incoming record length
            *  to avoid having any data split between two buffer
            *  reads.  */
            long siz2 = il->nelt*ls2;
            if (ls1 < ls2) {  /* Different from host test! */
               lbuf = MAX_MSG_LENGTH - MAX_MSG_LENGTH%ls1;
               pd   = (s_type *)mallocv(lbuf, "Restore buffer");
               }
            /* Open broadcast stream */
            nngcr(&nnio, BCSTADDR, RESTORE_MSG);
#endif

            /* Process delays in order */
            for (idly=0; idly<ndrr; idly++) {
               /* Get location of data on current node */
               p1 = il->pps[idly];

#ifndef PARn   /* Serial and parallel host */

               if (ls1 == ls2) {    /* Data fit in pps space */
                  rfread(pdf, p1, siz1, ABORT);
                  if (ExpandSi) {   /* (Throwing away phase) */
                     s_type *pr,*pr2 = p1 + siz1;
                     for (pr=p1; pr<pr2; pr+=LSmem) {
                        long tsi = (long)(*pr) << FBsi;
                        d3ptl2(tsi, pr);
                        }
                     }
#ifdef PAR0
                  nnput(&nnio, p1, siz1);
#endif
                  }
               else {               /* Process data in blocks */
#ifdef PAR0
                  s_type *ps1;            /* Ptr to broadcast data */
#endif
                  s_type *ps=p1;          /* Ptr  for storing data */
                  s_type *pr,*pr2;        /* Ptrs for reading data */
                  long msize,nsize=siz1;  /* Length of data */
                  /* Assignment intended */
                  while (msize = min(nsize,lbuf)) {
                     rfread(pdf, pd, msize, ABORT);
                     pr2 = pd + msize;
                     if (ls1 > ls2) {
                        /* If contracting data (throwing away phase),
                        *  do it now, then broadcast shorter record.
                        *  (This case can only occur with 2-byte si.)
                        */
#ifdef PAR0
                        ps1 = ps;
#endif
                        for (pr=pd; pr<pr2; pr+=ls1,ps+=ls2)
                           memcpy((char *)ps, (char *)pr, ls1);
#ifdef PAR0
                        nnput(&nnio, ps1, ps-ps1);
#endif
                        } /* End if (ls1 > ls2) */
                     else {
                        /* If expanding data (converting old si to
                        *  new si and/or adding space for new phase),
                        *  broadcast first, then do the expansion.
                        */
#ifdef PAR0
                        nnput(&nnio, pd, msize);
#endif
                        if (ExpandSi)
                           for (pr=pd; pr<pr2; pr+=ls1,ps+=ls2) {
                              long tsi = (long)(*pr) << FBsi;
                              d3ptl2(tsi, ps);
                              if (CopyPhi) ps[ls2-1] = pr[ls1-1];
                              }
                        else
                           for (pr=pd; pr<pr2; pr+=ls1,ps+=ls2)
                              memcpy((char *)ps, (char *)pr, ls1);
                        } /* End else (ls1 < ls2) */
                     nsize -= msize;
                     } /* End loop over buffer loads */
                  } /* End expanding/contracting data */

#else /* is PARn */

               if (ls1 >= ls2) {    /* Data fit in pps space */
                  nnget(&nnio, p1, siz2);
                  }
               else {               /* Process data in records */
                  s_type *ps=p1;          /* Ptr  for storing data */
                  s_type *pr,*pr2;        /* Ptrs for reading data */
                  long msize,nsize=siz1;  /* Length of data */
                  /* Assignment intended */
                  while (msize = min(nsize,lbuf)) {
                     nnget(&nnio, pd, msize);
                     pr2 = pd + msize;
                     /* Expand si to S7L2 if reading old save file */
                     if (ExpandSi)
                        for (pr=pd; pr<pr2; pr+=ls1,ps+=ls2) {
                           long tsi = (long)(*pr) << FBsi;
                           d3ptl2(tsi, ps);
                           if (CopyPhi) ps[ls2-1] = pr[ls1-1];
                           }
                     else
                        for (pr=pd; pr<pr2; pr+=ls1,ps+=ls2)
                           memcpy((char *)ps, (char *)pr, ls1);
                     nsize -= msize;
                     } /* End loop over buffer loads */
                  } /* End expanding data */

#endif
               } /* End delay row loop */

            /* Close broadcast stream and free buffer */
#ifdef PAR0
            nnpcl(&nnio);
#endif
#ifdef PARn
            nngcl(&nnio);
#endif
            if (ls1 != ls2) freev(pd, "Restore buffer");

            /* Add phase to s(i) if (ls1 < ls2) and fill in any
            *  delay rows that were not found in input file.  This
            *  is done even on Node 0 to advance noise and phase
            *  seeds and to provide data in case d3rpsi called.
            *  Rev, 07/24/10, GNR - New initsi call, do on Node 0 */
            {  int ni1 = (ls1 < ls2) ? ndrr : 0;
               int ni2 = (int)il->mxsdelay1 - ndrr;
               if (ni1 + ni2) {
                  setinsi(il, ni1, ni2);
                  Lija.resetx = NULL;  /* Kill u,w setting */
                  initsi(il, -1);
                  }
               }
#endif
            } /* End restoring s(i) */

/*---------------------------------------------------------------------*
*                       Process repertoire data                        *
*---------------------------------------------------------------------*/

/* In preparation for loading repertoire data, determine length of
*  buffer needed to transfer data to comp nodes.  Set lls to the
*  length of the cell data ("ls" data) to be restored (omitting
*  dynamic and statistical variables and any padding).  This value is
*  preset by d3allo().  Set llel to the length of data for one cell in
*  memory, rflen to the length of data for one cell in the input file.
*/
         lls = il->lsrstr;

         krrd = klsd = !(il->Ct.ctopt & OPTOR) && (lls > 0);
#ifdef PAR0
         lbuf = lls;
#endif
         for (ix=il->pct1; ix; ix=ix->pct) {
            if (ix->cnwk.phs1.klcd =   /* Assignment intended */
                  !(ix->Cn.cnopt & NOPOR) && (ix->lct > 0)) {
#ifdef PAR0
               lbuf = max(lbuf, ix->lct);
#endif
               krrd |= TRUE;
               } /* End not overriding restore */
            } /* End conntype setup loop */

/* Restore repertoire data */

         if (krrd) {

#ifdef PARn
            /* Parallel comp node:  Use nnget routine to receive
            *  data from host in chunks for "ls" data and each
            *  conntype for which restoration is not overridden.
            *  Host will send to each node just its own cells. */
            if (il->mcells) { /* Omit if node has no cells */
               llel = il->lel;      /* Cell increment */
               lllt = il->llt;      /* Coff limit     */
               nngcr(&nnio, NC.hostid, RESTORE_MSG);
               for (coff=0; coff<lllt; coff+=llel) {
                  if (klsd) nnget(&nnio, il->prd+coff, lls);
                  for (ix=il->pct1; ix; ix=ix->pct) {
                     if (ix->cnwk.phs1.klcd)
                        nnget(&nnio, ix->psyn0+coff, ix->lct);
                     } /* End conntype loop */
                  } /* End cell loop */
               nngcl(&nnio);        /* Close stream */
               }
#endif

#ifdef PAR0
            /* Parallel host node:  Read the data and send to dest.
            *  Allocate read buffer  */
            lbuf = min(lbuf, MAX_MSG_LENGTH); /* Just in case */
            pd = angetpv(lbuf, "Restore buffer");
            /* Loop over nodes */
            for (node=0; node<il->nodes; node++) {
               long msize,nsize;    /* Data length */
               /* Calculate ncr = number cells on this node.
               *  Break out of node loop when no cells left. */
               if ((ncr = il->cpn + (node<il->crn)) == 0) break;
               /* Open a stream to the current target node */
               nnpcr(&nnio, node+il->node1, RESTORE_MSG);
               /* Loop over cells */
               while (ncr--) {
                  if (klsd) {
                     rfseek(pdf, il->ctwk.phs1.offprd, SEEKABS, ABORT);
                     rfread(pdf, pd, lls, ABORT);
                     nnput(&nnio, pd, lls);
                     /* Position for next cell */
                     il->ctwk.phs1.offprd += il->ctwk.phs1.rflel;
                     } /* End "ls" data */
                  /* Loop over connection types */
                  for (ix=il->pct1; ix; ix=ix->pct) {
                     if (ix->cnwk.phs1.klcd) {
                        xfd = ix->cnwk.phs1.fdes;
                        rfseek(xfd, ix->cnwk.phs1.offprd,
                           SEEKABS, ABORT);
                        /* Read and forward the data in 8K chunks */
                        nsize = ix->lct;
                        while (msize = min(nsize,lbuf)) {
                           rfread(xfd, pd, msize, ABORT);
                           nnput(&nnio, pd, msize);
                           nsize -= msize;
                           } /* End lct data */
                        /* Position for next cell */
                        ix->cnwk.phs1.offprd += ix->cnwk.phs1.rflel;
                        } /* End "lc" data */
                     } /* End conntype loop */
                  } /* End cell loop */
               nnpcl(&nnio);     /* Close stream */
               } /* End loop over comp nodes */
            anrelp(pd);           /* Release read buffer */
#endif /* PAR0 */

#ifndef PAR
            /* Serial:  Read repertoire data directly.
            *  Loop over cells */
            llel = il->lel;
            lllt = il->llt;
            for (coff=0; coff<lllt; coff+=llel) {
               if (klsd) {
                  rfseek(pdf, il->ctwk.phs1.offprd, SEEKABS, ABORT);
                  rfread(pdf, il->prd+coff, lls, ABORT);
                  /* Position for next cell */
                  il->ctwk.phs1.offprd += il->ctwk.phs1.rflel;
                  } /* End "ls" data */
               /* Loop over connection types */
               for (ix=il->pct1; ix; ix=ix->pct) {
                  if (ix->cnwk.phs1.klcd) {
                     xfd = ix->cnwk.phs1.fdes;
                     rfseek(xfd, ix->cnwk.phs1.offprd,
                        SEEKABS, ABORT);
                     /* Read the data directly into prd storage */
                     rfread(xfd, ix->psyn0+coff, ix->lct, ABORT);
                     /* Position for next cell */
                     ix->cnwk.phs1.offprd += ix->cnwk.phs1.rflel;
                     } /* End "lc" data */
                  } /* End conntype loop */
               } /* End cell loop */
#endif

            } /* End restore repertoire data */
         } /* End celltype loop */
      } /* End repertoire loop */

/* Release space for restore files */

#ifndef PARn
   {  struct RFdef *psvf,*pnsvf;  /* Ptrs to save file */
      /* Close files, free space */
      for (psvf=d3file[SAVREP_IN]; psvf; ) {
         pnsvf = psvf->nf;    /* Get ptr to next before freeing! */
         rfclose(psvf, REWIND, RELEASE_BUFF, ABORT);
         free(psvf->fname);
         free(psvf);
         psvf = pnsvf;
         }
      d3file[SAVREP_IN] = NULL;
      }
#endif

   } /* End d3rstr */

