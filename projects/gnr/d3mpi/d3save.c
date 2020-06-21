/* (c) Copyright 1985-2017, The Rockefeller University *21116* */
/* $Id: d3save.c 76 2017-12-12 21:17:50Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3save                                 *
*                 SAVE REPERTOIRES FOR LATER RESTART                   *
*                                                                      *
*  Call in parallel on all nodes.  Code in d3news() assures that       *
*  d3save will never be called during a replay.  Argument indicates    *
*  timing of this call, which affects which file name to use.  It may  *
*  be NULL on comp nodes.  The header data will be big-endian for all  *
*  systems and will always be stored using memcpy to squeeze out any   *
*  padding in the header blocks.  (This is a bit extravagant, but      *
*  allows future revisions to be made in the header block contents     *
*  without regard to item-size ordering.)  Repertoire data will be     *
*  saved as they are in memory, little-or big-endian as set by         *
*  D3MEMACC_BYTE_ORDER, independent of native hardware byte order.     *
*  Code assumes that this will always remain the same on all systems,  *
*  so no swapping will be performed.                                   *
*                                                                      *
*  The key design idea to understand is that elements such as cell and *
*  repertoire data may contain padding, and we must be able to restore *
*  a file to a machine that has a different padding arrangement than   *
*  the one where the file was written.  To this end, d3save just dumps *
*  all the data as it exists in memory, padding and all, and records   *
*  in the file headers the total lengths (ls, lel, llt) including the  *
*  padding.  During restoration, d3rstr must skip over the padding     *
*  while reading only the data actually needed.  This is accompished   *
*  with the aid of the pre- and post-read skips calculated by d3fchk.  *
*  This design makes sense because (1) it favors efficiency during     *
*  saving, which is done more frequently than restoring, and (2) the   *
*  pre- and post-read skips are necessary in any event to skip over    *
*  information that the user has chosen to override, therefore the     *
*  ability to skip over the padding comes at no additional cost.       *
*                                                                      *
*  No effort is made to store modality markings that are in various    *
*  stages of axonal delay.  These should be cleared at zsta and reset  *
*  time so statistics do not depend on events in previous trial series.*
************************************************************************
*  V1A, 12/13/85, G.N. REEKE                                           *
*  V3L, 03/26/90, GNR - Remove PRVRSP                                  *
*  V4A, 09/14/90, MC -  Translate into C                               *
*  V5A, 12/12/90, MC -  Parallelize, spin off d3savn.c                 *
*  V5C, 02/04/92, GNR - Incorporate Darwin 1, merge with d3savn        *
*  V5E, 08/08/92, GNR - Call tstamp for timestamp                      *
*  Rev, 09/28/92, GNR - Make SAVENET file little-endian on all systems *
*  Rev, 12/10/92, ABP - Use nnput/get, add HYB code                    *
*  V6C, 08/17/93, GNR - Remove 2**n dependence, prepare for load optim.*
*  V6D, 02/05/94, GNR - Store full s(i) array, including axonal delay  *
*  V8A, 11/28/96, GNR - Remove support for non-hybrid version          *
*  Rev, 03/01/97, GNR - No need to collect s(i),phase--always on host  *
*  V8A, 09/28/97, GNR - Version 8 SAVENET file with self-descriptive   *
*                       header data, independent allocation for each   *
*                       conntype, add SFREPHDR, GC & MOD phase seeds.  *
*  Rev, 11/24/00, GNR - Separate RADIUS parameter                      *
*  V8D, 11/26/07, GNR - NEWSERIES, CREATE-FILENAMES, ROTATE-FILENAMES  *
*  ==>, 11/27/07, GNR - Last mod before committing to svn repository   *
*  V8E, 02/06/09, GNR - Add il->nizvar,izseed for Izhikevich cells     *
*  V8G, 09/04/10, GNR - Add autoscale saving                           *
*  V8H, 10/20/10, GNR - Add xnoff,lxn, allow earlier w/nuklen == lxn   *
*  R65, 01/02/16, GNR - n[gq][xyg] to ui16, ngrp to ui32, ncpg to ubig *
*  R66, 02/17/16, GNR - Change long random seeds to typedef si32 orns  *
*  R67, 04/27/16, GNR - Remove Darwin 1 support, mpitools call args    *
*  R75, 09/22/17, GNR - Add tcwid                                      *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "d3fdef.h"
#include "celldata.h"
#include "savblk.h"
#ifndef PARn
#include "rocks.h"
#endif
#include "rkxtra.h"

extern struct CELLDATA CDAT;

/* Macros for storing items in headers, allowing for misalignment */
#define SFHPutE(itm,data) bemfmr4(h->itm.d, (float)data)
#define SFHPutS(itm,data) bemfmi2(h->itm.d, (short)data)
#define SFHPutJ(itm,data) bemfmi4(h->itm.d, (si32)data)
#define SFHPutiL(itm,data) bemfmil(h->itm.d, (long)data)
#define SFHPutuL(itm,data) bemfmil(h->itm.d, (ulng)data)
#define SFHPutiW(itm,data) bemfmi8(h->itm.d, (si64)data)
#define SFHPutuW(itm,data) bemfmu8(h->itm.d, (ui64)data)

void d3save(int svstage)  {

   struct CELLTYPE  *il;
#ifdef PAR0
   char             *pd;      /* Ptr to data received from a node */
#endif
#ifndef PARn
   struct REPBLOCK  *ir;      /* Ptr to a repertoire, etc. */
   struct CONNTYPE  *ix;
   struct INHIBBLK  *ib;
   struct MODBY    *imb;
   union  SFANYHDR   *h;
   struct RFdef    *pfd;
   char           *ufnm;      /* Ptr to used file name */
   ui64 offile;               /* Offset of a data class in file */
   ui64 offsyn;               /* Offset in file to repertoire data
                              *  for first connection of first cell */
#endif

#ifdef PAR
   struct NNSTR svio;         /* Control structure for i/o */
#endif
#ifdef PAR0
   int msize;                 /* Size of block data */
   int nsize;                 /* Size of node data */
   int relnode;               /* Relative node number of source */
#endif

#ifndef PARn

/*---------------------------------------------------------------------*
* Determine which file to write according to user options and timing   *
*                                                                      *
*  (getsave() assures that the required number of filenames were       *
*  entered in Group 0)                                                 *
*---------------------------------------------------------------------*/

   ufnm = NULL;               /* Use old name if not creating */
   if (svstage == D3SV_ENDXEQ) {
      /* End execution -- use first file as documented.  (The original
      *  file name will have been modified by any KSVCFN calls.)  */
      pfd = d3file[SAVREP_OUT];
      if (RP0->pcsnfn) {
         RP0->pcsnfn[RP0->lbsnfn] = '\0';
         ufnm = RP0->pcsnfn;
         }
      }

   else if (RP->ksv & KSVCFN) {
      /* Not final save -- user asked to create filenames */
      if (RP0->jsnf == 0) {
         /* First time through--setup area to make filenames */
         RP0->lbsnfn = (short)strlen(d3file[SAVREP_OUT]->fname);
         RP0->pcsnfn = callocv(1, RP0->lbsnfn+D3SV_ALCFN+1,
            "SAVENET file names");
         strcpy (RP0->pcsnfn, d3file[SAVREP_OUT]->fname);
         }
      ++RP0->jsnf;
      wbcdwt(&RP0->jsnf, RP0->pcsnfn + RP0->lbsnfn,
         RK_IORF|RK_PAD0|RK_NHALF|D3SV_ALCFN-1);
      pfd = d3file[SAVREP_OUT];
      ufnm = RP0->pcsnfn;
      }

   else if (RP->ksv & KSVRFN) {
      /* Rotate existing file names */
      if (!(RP0->psnf = RP0->psnf->nf))
         RP0->psnf = d3file[SAVREP_OUT]->nf;
      pfd = RP0->psnf;
      }

   else {
      /* Traditional way -- use until all used up, then error */
      if (!(RP0->psnf = RP0->psnf->nf))
         d3exit(NULL, SAVEXF_ERR, 0);
      pfd = RP0->psnf;
      }

/*---------------------------------------------------------------------*
*                 Open output file and create headers                  *
*---------------------------------------------------------------------*/

   rfopen(pfd, ufnm, SAME, SAME, SAME, SAME, SAME,
      SAME, SAME, SAME, SAME, SAME, ABORT);

/* Create and write out the overall header.  The same dynamic header
*  space will be used for all kinds of headers.  */

   h = (union SFANYHDR *)callocv(1, sizeof(union SFANYHDR),
      "Save file header");    /* Clear junk from strings */
   memcpy(h->sfhdr.filetype, SF_IDENT, SFVERS_LEN);
   strncpy(h->sfhdr.title, gettit(), SFTITLE_LEN);
   tstamp(h->sfhdr.timestamp);
   strncpy(h->sfhdr.stitle, RP0->stitle, LSTITL);
   rfwrite(pfd, h, sizeof(struct SFHDR), ABORT);

/* Create and write out the second header */

   memcpy((char *)h, (char *)&ursfhdr2, sizeof(struct SFHDR2));
   SFHPutS(sfhdr2.sfhnd1b, 0);
   SFHPutS(sfhdr2.sfhnrep, RP->nrep);
   SFHPutJ(sfhdr2.sfhsnseed, RP->snseed);
   rfwrite(pfd, h, sizeof(struct SFHDR2), ABORT);

/* Offset of first s(i) = total size of all headers */

   {  ulng toffile = sizeof(struct SFHDR) + sizeof(struct SFHDR2)
      + RP->nrep    *sizeof(struct SFREPHDR)
      + RP0->totnlyr*sizeof(struct SFCTHDR)
      + RP0->totconn*sizeof(struct SFCNHDR)
      + RP0->totgcon*sizeof(struct SFCGHDR)
      + RP0->totmodu*sizeof(struct SFCMHDR);
      offile = jeul(toffile);
      }  /* End toffile local scope */

/* Write headers for all cell types and connection types */

   for (ir=RP->tree; ir; ir=ir->pnrp) {

      /* Set up repertoire header */
      memcpy((char *)h, (char *)&ursfrephdr,
         sizeof(struct SFREPHDR));
      strncpy(h->rephdr.repnam.d, ir->sname, LSNAME);
      SFHPutS(rephdr.sfrngx, ir->ngx);
      SFHPutS(rephdr.sfrngy, ir->ngy);
      {  register int i;
         for (i=0; i<GPNDV; i++)
            bemfmi2(h->rephdr.sfgpoff.d[i], ir->gpoff[i]);
         }
      SFHPutS(rephdr.sfnlyr, ir->nlyr);
      rfwrite(pfd, h, sizeof(struct SFREPHDR), ABORT);

      for (il = ir->play1; il; il=il->play) {
         short tnizvar = (short)il->nizvar;
         if (il->ctf & CTFI3IVR) tnizvar |= I3IVRBIT;
         /* Set up cell type header */
         memcpy((char *)h, (char *)&ursfcthdr,
            sizeof(struct SFCTHDR));
         strncpy(h->cthdr.celnam.d, il->lname, LSNAME);
         SFHPutJ(cthdr.sfctnel,    il->nel);
         SFHPutuW(cthdr.sfctlel,   il->lel);
         SFHPutS(cthdr.sfctls,     il->ls);
         SFHPutS(cthdr.sfctphshft, il->phshft);
         SFHPutS(cthdr.sfctsdly1,  il->mxsdelay1);
         SFHPutS(cthdr.sfctnizv,   tnizvar);
         SFHPutS(cthdr.sfctnct,    il->nct);
         SFHPutS(cthdr.sfctngct,   il->ngct);
         SFHPutS(cthdr.sfctnmct,   il->nmct);
         {  register int i;
            for (i=0; i<CTNDV; i++)
               bemfmi2(h->cthdr.sfctoff.d[i], il->ctoff[i]);
            }
         if (il->pauts) {
            bemfmi4(h->cthdr.sfctasmul.d[0], il->pauts->asmul[0]);
            bemfmi4(h->cthdr.sfctasmul.d[1], il->pauts->asmul[2]);
            }
         else {
            bemfmi4(h->cthdr.sfctasmul.d[0], S24);
            bemfmi4(h->cthdr.sfctasmul.d[1], S24);
            }
         SFHPutJ(cthdr.sfctnsd, il->No.nsd);
         SFHPutJ(cthdr.sfctrsd, il->rsd);
         {  si32 tpsd = il->pctpd ? il->pctpd->phiseed : 0;
            SFHPutJ(cthdr.sfctpsd, tpsd); }
         {  si32 tzsd = (il->Ct.rspmeth >= RF_IZH3) ?
            ((struct IZHICOM *)il->prfi)->izseed : 0;
            SFHPutJ(cthdr.sfctzsd, tzsd); }
         SFHPutuW(cthdr.offsi, offile);
            offile = jauw(offile, jmuwj(il->wspt,il->mxsdelay1));
         SFHPutuW(cthdr.offprd, offile);
         rfwrite(pfd, h, sizeof(struct SFCTHDR), ABORT);

         /* Loop over specific connection types and write headers */
         offsyn = jaul(offile, (ui32)il->ls);
         for (ix=il->pct1; ix; ix=ix->pct) {
            memcpy((char *)h, (char *)&ursfcnhdr,
               sizeof(struct SFCNHDR));
            strncpy(h->cnhdr.srcsnam.d, ix->srcnm.rnm, LSNAME);
            strncpy(h->cnhdr.srclnam.d, ix->srcnm.lnm, LSNAME);
            SFHPutJ(cnhdr.sfcnnc,     ix->nc);
            SFHPutS(cnhdr.sfcnlc,     ix->lc);
            SFHPutS(cnhdr.sfcnlxn,    ix->lxn);
            SFHPutS(cnhdr.sfcnnbc,    ix->Cn.nbc);
            SFHPutS(cnhdr.sfcncbc,    ix->cbc);
            SFHPutJ(cnhdr.sfcnkgen,   ix->kgen);
            {  register int i;
               for (i=0; i<CNNDV; i++)
                  bemfmi2(h->cnhdr.sfcnoff.d[i], ix->cnoff[i]);
               for (i=0; i<XNNDV; i++)
                  bemfmi2(h->cnhdr.sfxnoff.d[i], ix->xnoff[i]);
               }
            SFHPutS(cnhdr.sfcnsrcngx, ix->srcngx);
            SFHPutS(cnhdr.sfcnsrcngy, ix->srcngy);
            SFHPutJ(cnhdr.sfcnsrcnel, ix->srcnel);
            SFHPutJ(cnhdr.sfcnsrcnelt,ix->srcnelt);
            SFHPutJ(cnhdr.sfcnloff,   ix->loff);
            SFHPutJ(cnhdr.sfcnlsg,    ix->lsg);
            SFHPutS(cnhdr.sfcnnrx,    ix->nrx);
            SFHPutS(cnhdr.sfcnnry,    ix->nry);
            SFHPutS(cnhdr.sfcnnhx,    ix->nhx);
            SFHPutS(cnhdr.sfcnnhy,    ix->nhy);
            SFHPutJ(cnhdr.sfcnnsa,    ix->nsa);
            SFHPutJ(cnhdr.sfcnnsax,   ix->nsax);
            SFHPutJ(cnhdr.sfcnnux,    ix->nux);
            SFHPutJ(cnhdr.sfcnnuy,    ix->nuy);
            SFHPutJ(cnhdr.sfcnradius, ix->hradius);
            SFHPutE(cnhdr.sfcntcwid,  ix->Cn.tcwid);
            SFHPutJ(cnhdr.sfcnstride, ix->stride);
            SFHPutJ(cnhdr.sfcnxoff,   ix->xoff);
            SFHPutJ(cnhdr.sfcnyoff,   ix->yoff);
            SFHPutJ(cnhdr.sfcncseed,  ix->cseed);
            SFHPutJ(cnhdr.sfcnlseed,  ix->lseed);
            SFHPutJ(cnhdr.sfcnpseed,  ix->phseed);
            SFHPutJ(cnhdr.sfcnrseed,  ix->rseed);
            SFHPutJ(cnhdr.sfcncmin,   ix->cijmin);
            SFHPutuW(cnhdr.offprd, offsyn);
               offsyn = jaulo(offsyn, ix->lct);
            rfwrite(pfd, h, sizeof(struct SFCNHDR), ABORT);
            } /* End writing specific conntype headers */

         /* Loop over geometric connection types and write headers.
         *  (These currently exist solely as placeholders for future
         *  data, phase options having been removed in V8D.)  */
         for (ib=il->pib1; ib; ib=ib->pib) {
            memcpy((char *)h, (char *)&ursfcghdr,
               sizeof(struct SFCGHDR));
            strncpy(h->cghdr.gsrcrid.d, ib->gsrcnm.rnm, LSNAME);
            strncpy(h->cghdr.gsrclid.d, ib->gsrcnm.lnm, LSNAME);
            SFHPutS(cghdr.sfcgngb, ib->ngb);
            SFHPutS(cghdr.sfcgnib, ib->nib);
            rfwrite(pfd, h, sizeof(struct SFCGHDR), ABORT);
            } /* End writing geometric connection type headers */

         /* Loop over modulatory connection types and write headers.
         *  (These currently exist solely as placeholders for future
         *  data, phase options having been removed in V8D.)  */
         for (imb=il->pmby1; imb; imb=imb->pmby) {
            memcpy((char *)h, (char *)&ursfcmhdr,
               sizeof(struct SFCMHDR));
            strncpy(h->cmhdr.msrcrid.d, imb->msrcnm.rnm, LSNAME);
            strncpy(h->cmhdr.msrclid.d, imb->msrcnm.lnm, LSNAME);
            rfwrite(pfd, h, sizeof(struct SFCMHDR), ABORT);
            } /* End writing modulatory connection type headers */

         offile += il->llt;   /* Now update the offset */
         } /* End loop over layers */
      } /* End loop over repertoires */
   free(h);                 /* Free header buffer */

#endif                        /* End comp node exclusion */

#ifdef PARn
   nnpcr(&svio, NC.hostid, SAVENET_MSG);
#endif

/*---------------------------------------------------------------------*
*                    Write out the normal CNS data                     *
*---------------------------------------------------------------------*/

   for (il=RP->pfct; il; il=il->pnct) {

#ifndef PARn
/* Write s(i),phase data.  As of V8A, always available on all nodes.  */

      if (il->lspt) {
         int idly;
         for (idly=0; idly<(int)il->mxsdelay1; idly++)
            rfwrite(pfd, il->pps[idly], il->lspt, ABORT);
         } /* End writing s(i),phase(i) data */
#endif

/* Write out the cell and connection data */

#ifdef PARn
      /* Parallel comp node:  Send data to host (if I have any) */
      if (quw(il->llt)) {
         rd_type *tprd = il->prd;
         ui64 nsize = il->llt;
         int  msize;
         while (quw(nsize)) {
            msize = qcuw(jesl(INT_MAX), nsize) ? INT_MAX : swlo(nsize);
            nnput(&svio, tprd, msize);
            tprd += msize;
            nsize = jrsl(nsize, msize);
            }
         nnpfl(&svio);
         } /* End sending data */
#endif

#ifdef PAR0
      /* Parallel host:  Get the data and write */
      if (qsw(il->lel) > 0) {
         for (relnode=0; relnode<il->nodes; relnode++) {
            /* Calculate nsize = size of data on current src node.
            *  Terminate loop when nsize becomes zero. */
            nsize = uwlod(jmuwj(il->lel, il->cpn + (relnode<il->crn)));
            if (!nsize) break;
            nngcr(&svio, relnode+il->node1, SAVENET_MSG);
            while (nsize) {
               msize = nngetp(&svio, (void **)&pd);
               msize = min(nsize, msize);
               rfwrite(pfd, pd, (size_t)msize, ABORT);
               nngsk(&svio, msize);
               nsize -= msize;
               } /* End transferring comp node data to file */
            nngcl(&svio);
            } /* End node loop */
         } /* End if lel > 0 */
#endif

#ifndef PAR
      /* Serial:  Write repertoire data directly */
      rfwrite(pfd, il->prd, uw2zd(il->llt), ABORT);
#endif

      } /* End loop over layers */

/* Finish up.  Close file and write message to user */

#ifdef PARn
   nnpcl(&svio);
#else
   rfclose(pfd, SAME, SAME, ABORT);
   cryout(RK_P3, "0Networks saved to ", RK_LN2,
      pfd->fname, RK_CCL, NULL);
#endif

   } /* End d3save() */
