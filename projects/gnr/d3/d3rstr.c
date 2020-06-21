/* (c) Copyright 1990-2016, The Rockefeller University *21115* */
/* $Id: d3rstr.c 72 2017-04-14 19:11:21Z  $ */
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
*  R67, 04/27/16, GNR - Remove Darwin 1 support, mpitools call args    *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "celldata.h"
#include "d3fdef.h"
#include "savblk.h"
#include "rocks.h"

/* Pound-define DBGRSTR for debug output */
/*** #define DBGRSTR ***/

extern struct CELLDATA CDAT;

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

#ifndef PAR0
   size_t coff;               /* Cell data offset  */
   size_t llel;               /* Local copy of lel */
   size_t lllt;               /* Local copy of llt */
#endif

   size_t lbuf;               /* Length of pd data buffer */
   size_t lls;                /* Local copy of ls */
   int klsd,krrd;             /* Flags for reading "ls", prd data */
#ifdef PAR0
   int node;                  /* Node communications */
#endif
#if defined(PAR0) || defined(DBGRSTR)
   int ncr;                   /* Cells remaining on current node */
#endif

/* All restore files are left open by d3fchk().  Code here to open
*  files after NCUBE overlay was removed, 11/28/96.  */

/* Allocate a buffer that may be used for unequal si-phi data restore
*  sizes or for transferring data segments from files to messages.
*  The global workspace CDAT.psws is used for this purpose.  */
   pd = (s_type *)CDAT.psws;

/*---------------------------------------------------------------------*
*                      Read the normal CNS data                        *
*---------------------------------------------------------------------*/

/* Loop over all repertoires and celltypes */

   for (ir=RP->tree; ir; ir=ir->pnrp) {

      /* (If there were any repertoire-level data, this would
      *  be the place to read them in--but there aren't any.) */

      for (il=ir->play1; il; il=il->play) {
         CDAT.cdlayr = il;       /* For d3lime */

#ifdef DBGRSTR
dbgprt(ssprintf(NULL,"d3rstr at Celltype (%4s,%4s)",
   il->pback->sname, il->lname));
#endif

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
#ifdef PARn
#if STSIZE < WSIZE
            si64 tov;            /* For overflow test calc */
#endif
            size_t siz2;
#endif
            size_t siz1;
            int ExpandSi = (il->ctf & RSTOLDSI) != 0;
            int CopyPhi  = il->ctwk.phs1.rfphshft & (int)il->phshft;
            /* Length of old and new s(i) data */
            int ls1 = (1 << il->ctwk.phs1.rfphshft) + (1 - ExpandSi);
            int ls2 = (int)il->lsp;
            /* Delay counter, number of delay rows to be restored */
            int idly,ndrr = min((int)il->mxsdelay1,
               (int)il->ctwk.phs1.rfdelay);
#if STSIZE < WSIZE
            tov = jrsl(jmsw(il->nelt, ls1), SI32_MAX);
            if (qsw(tov) < 0) d3lime("d3rstr", PLcb(PCL_RFDT));
#endif
            siz1 = jmuzjd((size_t)il->nelt, ls1);
            /* When ls1 != ls2, some code below will use part of
            *  message buffer that is largest multiple of incoming
            *  record length (ls1) to avoid having any data split
            *  between two buffer reads.  */
            lbuf = MAX_MSG_LENGTH - MAX_MSG_LENGTH%ls1;
#ifndef PARn   /* Serial and parallel host */
            /* Seek to the s(i) data */
            rfseek(pdf, uw2zd(il->ctwk.phs1.offsi), SEEKABS, ABORT);
#ifdef PAR0
            /* Open broadcast stream */
            nnpcr(&nnio, BCSTADDR, RESTORE_MSG);
#endif

#else /* is PARn */
#if STSIZE < WSIZE
            tov = jrsl(jmsw(il->nelt, ls2), SI32_MAX);
            if (qsw(tov) < 0) d3lime("d3rstr", PLcb(PCL_RFDT));
#endif
            siz2 = jmuzjd((size_t)il->nelt, ls2);
            /* Open broadcast stream */
            nngcr(&nnio, BCSTADDR, RESTORE_MSG);
#endif

#ifdef DBGRSTR
dbgprt(ssprintf(NULL,"  ls1 = %d, ls2 = %d, ndrr = %d, siz1 = %zd, "
   "ExpSi = %d, CopyPhi = %d", ls1,ls2,ndrr,siz1,ExpandSi,CopyPhi));
#endif

            /* Process delays in order */
            for (idly=0; idly<ndrr; idly++) {
#ifdef DBGRSTR
dbgprt(ssprintf(NULL," Processing idly = %d", idly));
#endif
               /* Get location of data on current node */
               p1 = il->pps[idly];

#ifndef PARn   /* Serial and parallel host */

               if (ls1 == ls2) {    /* Data fit in pps space */
                  rfread(pdf, p1, siz1, ABORT);
                  if (ExpandSi) {   /* (Throwing away phase) */
                     s_type *pr,*pr2 = p1 + siz1;
                     for (pr=p1; pr<pr2; pr+=LSmem) {
                        si16 tsi = (si16)(*pr) << FBsi;
                        d3ptms2(tsi, pr);
                        }
                     }
#ifdef DBGRSTR
dbgprt(ssprintf(NULL,"  Case ls1==ls2, bcst %zd data", siz1));
#endif

#ifdef PAR0
                  nnput(&nnio, p1, ckz2i(siz1, "d3rstr"));
#endif
                  }
               else {               /* Process data in blocks */
#ifdef PAR0
                  s_type *ps1;            /* Ptr to broadcast data */
#endif
                  s_type *ps=p1;          /* Ptr  for storing data */
                  s_type *pr,*pr2;        /* Ptrs for reading data */
                  size_t msize, nsize=siz1;  /* Length of data */
                  /* Assignment intended */
                  while (msize = min(nsize,lbuf)) {
                     rfread(pdf, pd, msize, ABORT);
#ifdef DBGRSTR
dbgprt(ssprintf(NULL,"  Case ls1!=ls2, read batch of size %zd", msize));
#endif
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
                        /* (ps-ps1 <= lbuf, so no ckz2i needed)  */
                        nnput(&nnio, ps1, ps-ps1);
#endif
                        } /* End if (ls1 > ls2) */
                     else {
                        /* If expanding data (converting old si to
                        *  new si and/or adding space for new phase),
                        *  broadcast first, then do the expansion.
                        *  (msize <= lbuf, so no ckz2i needed)
                        */
#ifdef PAR0
                        nnput(&nnio, pd, (int)msize);
#endif
                        if (ExpandSi)
                           for (pr=pd; pr<pr2; pr+=ls1,ps+=ls2) {
                              si16 tsi = (si16)(*pr) << FBsi;
                              d3ptms2(tsi, ps);
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
                  nnget(&nnio, p1, ckz2i(siz2,"d3rstr"));
#ifdef DBGRSTR
dbgprt(ssprintf(NULL,"  Case ls1 >= ls2, got bcst %zd data", siz2));
#endif
                  }
               else {               /* Process data in records */
                  s_type *ps=p1;          /* Ptr  for storing data */
                  s_type *pr,*pr2;        /* Ptrs for reading data */
                  size_t msize, nsize=siz1;  /* Length of data */
                  /* Assignment intended */
                  while (msize = min(nsize,lbuf)) {
                     nnget(&nnio, pd, (int)msize);
#ifdef DBGRSTR
dbgprt(ssprintf(NULL,"  Case ls1<ls2, read batch of size %zd", msize));
#endif
                     pr2 = pd + msize;
                     /* Expand si to S7L2 if reading old save file */
                     if (ExpandSi)
                        for (pr=pd; pr<pr2; pr+=ls1,ps+=ls2) {
                           si16 tsi = (si16)(*pr) << FBsi;
                           d3ptms2(tsi, ps);
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

            /* Close broadcast stream */
#ifdef PAR0
            nnpcl(&nnio);
#endif
#ifdef PARn
            nngcl(&nnio);
#endif

#ifndef PAR0
            /* Add phase to s(i) if (ls1 < ls2) and fill in any
            *  delay rows that were not found in input file.  */
            {  int ni1 = (ls1 < ls2) ? ndrr : 0;
               int ni2 = (int)il->mxsdelay1 - ndrr;
#ifdef DBGRSTR
dbgprt(ssprintf(NULL,"  Calling initsi with ni1 = %d, ni2 = %d",
   ni1, ni2));
#endif
               if (ni1 + ni2) initsi(il, ni1, ni2);
               }
#endif
            } /* End restoring s(i) */

/*---------------------------------------------------------------------*
*                       Process repertoire data                        *
*---------------------------------------------------------------------*/

/* In preparation for loading repertoire data, set lls to the length
*  of the cell data ("ls" data) to be restored (omitting dynamic and
*  statistical variables and any padding).  This value is preset by
*  d3allo().  Set krrd to indicate whether any connection data need
*  to be restored.  */
         lls = (size_t)il->lsrstr;
         krrd = klsd = !(il->Ct.ctopt & OPTOR) && (lls > 0);
         for (ix=il->pct1; ix; ix=ix->pct) {
            if (ix->cnwk.phs1.klcd =   /* Assignment intended */
                  !(ix->Cn.cnopt & NOPOR) && (ix->lct > 0)) {
               krrd |= TRUE;
               } /* End not overriding restore */
            } /* End conntype setup loop */
#ifdef DBGRSTR
dbgprt(ssprintf(NULL, " Setting lls = %zd, krrd = %d, klsd = %d",
   lls, krrd, klsd));
#endif

/* Restore repertoire data */

         if (krrd) {

#ifdef PARn
            /* Parallel comp node:  Use nnget routine to receive data
            *  from host in chunks for "ls" data and each conntype for
            *  which restoration is not overridden.  Must restore cells
            *  individually here as items that do not carry across
            *  trial series (statistics, padding) are not included in
            *  the saved data.  Host will send to each node just its
            *  own cells.  (Loop is empty if node has no cells.)
            *  (Data are sent in 8K chunks as a convenient buffer size
            *  for rfread->nnput sequence, but can be received in up
            *  to INT_MAX chunks--jic data exceeds INT_MAX).
            *  N.B.  Data for each cell, and first connection to each
            *  cell, are aligned on a boundary determined in d3allo().
            *  The sum of ix->lct + il->ls may be less than il->llt. */
            if (il->mcells > 0) {
               size_t icoff,jcoff;     /* Subdivisions of coff */
               size_t msize,nsize;     /* Data length */
               nngcr(&nnio, NC.hostid, RESTORE_MSG);
               llel = uw2zd(il->lel);
               lllt = uw2zd(il->llt);
#ifdef DBGRSTR
dbgprt(ssprintf(NULL, "  Starting loop over lllt = %zd data", lllt));
ncr = il->locell;
#endif
               /* Effectively loop over cells */
               for (coff=0; coff<lllt; coff+=llel) {
                  icoff = coff;
                  if (klsd) {             /* Receive ls data */
                     nsize = lls, jcoff = icoff;
#ifdef DBGRSTR
dbgprt(ssprintf(NULL, "  At cell %d, icoff = %zd, getting nsize = "
   "%zd", ncr, icoff, nsize));
++ncr;
#endif
                     while (nsize > 0) {
                        msize = min(nsize, INT_MAX);
                        nnget(&nnio, il->prd+jcoff, (int)msize);
                        jcoff += msize, nsize -= msize;
                        }
                     }
                  /* Advance to connection data (whether or not
                  *  cell data were restored)  */
                  icoff += il->ls;
                  for (ix=il->pct1; ix; ix=ix->pct) {
                     if (ix->cnwk.phs1.klcd) {
                        nsize = (size_t)ix->lct, jcoff = icoff;
#ifdef DBGRSTR
dbgprt(ssprintf(NULL, "    For ict = %d, icoff = %zd, nsize = %zd",
   ix->ict, icoff, nsize));
#endif
                        while (nsize > 0) {
                           msize = min(nsize, INT_MAX);
                           nnget(&nnio, il->prd+jcoff, (int)msize);
                           jcoff += msize, nsize -= msize;
                           }
                        } /* End if klcd */
                     /* Advance to next conntype (whether or not
                     *  this one was restored */
                     icoff += ix->lct;
                     } /* End conntype loop */
                  } /* End effective cells loop */
#ifdef DBGRSTR
dbgprt("  Closing msg stream");
#endif
               nngcl(&nnio);        /* Close stream */
               }
#endif

#ifdef PAR0
            /* Parallel host node:  Read the data and send to dest.
            *  Note that offsets here are those stored with the data
            *  file and retrieved by d3fchk()--not necessarily the
            *  same as those in the present network setup.
            *  Allocate read buffer  */
            lbuf = MAX_MSG_LENGTH;
            /* Loop over nodes */
            for (node=0; node<il->nodes; node++) {
               size_t msize,nsize;     /* Data length */
               /* Calculate ncr = number cells on this node.
               *  Break out of node loop when no cells left. */
               if ((ncr = il->cpn + (node<il->crn)) == 0) break;
               /* Open a stream to the current target node */
               nnpcr(&nnio, node+il->node1, RESTORE_MSG);
               /* Loop over cells */
#ifdef DBGRSTR
dbgprt(ssprintf(NULL, "  Start loop over %d cells", ncr));
#endif
               while (ncr--) {
                  if (klsd) {
                     rfseek(pdf, uw2zd(il->ctwk.phs1.offprd),
                        SEEKABS, ABORT);
                     /* Read and forward the data in 8K chunks */
                     nsize = lls;
#ifdef DBGRSTR
dbgprt(ssprintf(NULL, "    For ncr = %d, sending nsize = %zd",
   ncr, nsize));
#endif
                     while (nsize > 0) {
                        msize = min(nsize,lbuf);
                        rfread(pdf, pd, msize, ABORT);
                        nnput(&nnio, pd, (int)msize);
                        nsize -= msize;
                        }
                     /* Position for next cell */
                     il->ctwk.phs1.offprd = jauw(
                        il->ctwk.phs1.offprd, il->ctwk.phs1.rflel);
                     } /* End "ls" data */
                  /* Loop over connection types */
                  for (ix=il->pct1; ix; ix=ix->pct) {
                     if (ix->cnwk.phs1.klcd) {
                        xfd = ix->cnwk.phs1.fdes;
                        rfseek(xfd, uw2zd(ix->cnwk.phs1.offprd),
                           SEEKABS, ABORT);
                        /* Read and forward the data in 8K chunks */
                        nsize = ix->lct;
#ifdef DBGRSTR
dbgprt(ssprintf(NULL, "    For ict = %d, sending nsize = %zd",
   ix->ict, nsize));
#endif
                        while (nsize > 0) {
                           msize = min(nsize,lbuf);
                           rfread(xfd, pd, msize, ABORT);
                           nnput(&nnio, pd, (int)msize);
                           nsize -= msize;
                           } /* End lct data */
                        /* Position for next cell */
                        ix->cnwk.phs1.offprd = jauw(
                           ix->cnwk.phs1.offprd, ix->cnwk.phs1.rflel);
                        } /* End "lc" data */
                     } /* End conntype loop */
                  } /* End cell loop */
               nnpcl(&nnio);     /* Close stream */
               } /* End loop over comp nodes */
#endif /* PAR0 */

#ifndef PAR
            /* Serial:  Read repertoire data directly.
            *  Loop over cells */
            llel = uw2zd(il->lel);
            lllt = uw2zd(il->llt);
            for (coff=0; coff<lllt; coff+=llel) {
               if (klsd) {
                  rfseek(pdf, uw2zd(il->ctwk.phs1.offprd),
                     SEEKABS, ABORT);
                  rfread(pdf, il->prd+coff, lls, ABORT);
                  /* Position for next cell */
                  il->ctwk.phs1.offprd = jauw(
                     il->ctwk.phs1.offprd, il->ctwk.phs1.rflel);
                  } /* End "ls" data */
               /* Loop over connection types */
               for (ix=il->pct1; ix; ix=ix->pct) {
                  if (ix->cnwk.phs1.klcd) {
                     xfd = ix->cnwk.phs1.fdes;
                     rfseek(xfd, uw2zd(ix->cnwk.phs1.offprd),
                        SEEKABS, ABORT);
                     /* Read the data directly into prd storage */
                     rfread(xfd, ix->psyn0+coff, ix->lct, ABORT);
                     /* Position for next cell */
                     ix->cnwk.phs1.offprd = jauw(
                        ix->cnwk.phs1.offprd, ix->cnwk.phs1.rflel);
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

