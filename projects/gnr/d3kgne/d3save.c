/* (c) Copyright 1992-2010, Neurosciences Research Foundation, Inc. */
/* $Id: d3save.c 40 2010-11-03 16:02:24Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3save                                 *
*                                                                      *
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
********************************************************************** *
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
***********************************************************************/

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
#ifndef PARn
#include "rocks.h"
#endif
#include "rkxtra.h"

/* Macros for storing items in headers, allowing for misalignment */
#define SFHPutE(itm,data) bemfmr4(h->itm.d, (float)data)
#define SFHPutS(itm,data) bemfmi2(h->itm.d, (short)data)
#define SFHPutL(itm,data) bemfmi4(h->itm.d, (long)data)

void d3save(int svstage)  {

   struct REPBLOCK  *ir;      /* Ptr to a repertoire, etc. */
   struct CELLTYPE  *il;
#ifdef D1
   struct D1BLK    *pd1;      /* Ptr to D1 data */
#endif
#ifdef PAR0
   char             *pd;      /* Ptr to data received from a node */
#endif
#ifndef PARn
   struct CONNTYPE  *ix;
   struct INHIBBLK  *ib;
   struct MODBY    *imb;
   union  SFANYHDR   *h;
   struct RFdef    *pfd;
   char           *ufnm;      /* Ptr to used file name */
#ifdef D1
   s_type           *p1;      /* D1 repertoire data pointer */
#endif
   long          offile;      /* Offset of a data class in file */
   long          offsyn;      /* Offset in file to repertoire data
                              *  for first connection of first cell */
#endif

   long size;                 /* Size of all data being written */

#ifdef PAR
   struct NNSTR svio;         /* Control structure for i/o */
   long nsize;                /* Size of node data */
#endif
#ifdef PAR0
   long msize;                /* Size of block data */
   int relnode;               /* Relative node number of source */
#ifdef D1
   int nodes = NC.total - NC.headnode;   /* Nodes used */
#endif
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
      ibcdwt(RK_IORF+RK_PAD0+D3SV_ALCFN-1, RP0->pcsnfn + RP0->lbsnfn,
         ++RP0->jsnf);
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
         d3exit(SAVEXF_ERR, NULL, 0);
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
   SFHPutS(sfhdr2.sfhnd1b, RP->nd1s);
   SFHPutS(sfhdr2.sfhnrep, RP->nrep);
   SFHPutL(sfhdr2.sfhsnseed, RP->snseed);
   rfwrite(pfd, h, sizeof(struct SFHDR2), ABORT);

/* Offset of first s(i) = total size of all headers */

   offile = sizeof(struct SFHDR) + sizeof(struct SFHDR2)
#ifdef D1
      + (int)RP->nd1s*sizeof(struct SFD1HDR)
#endif
      + RP->nrep    *sizeof(struct SFREPHDR)
      + RP0->totnlyr*sizeof(struct SFCTHDR)
      + RP0->totconn*sizeof(struct SFCNHDR)
      + RP0->totgcon*sizeof(struct SFCGHDR)
      + RP0->totmodu*sizeof(struct SFCMHDR);

#ifdef D1
/* If there are Darwin 1 blocks, write out headers for them */
   if (RP->nd1s) {
      for (pd1=RP->pd1b1; pd1; pd1=pd1->pnd1b) {
         long lend1 = pd1->nd1r*pd1->nepr;
         memcpy((char *)h, (char *)&ursfd1hdr,
            sizeof(struct SFD1HDR));
         SFHPutL(d1hdr.sfd1nepr, pd1->nepr);
         SFHPutS(d1hdr.sfd1nd1r, pd1->nd1r);
         SFHPutS(d1hdr.sfd1nbpe, pd1->nbpe);
         SFHPutL(d1hdr.sfd1seed, pd1->d1seed);
         SFHPutL(d1hdr.offd1s, offile);
            offile += lend1;
         SFHPutL(d1hdr.offd1r, offile);
            offile += lend1*pd1->nbyt;
         rfwrite(pfd, h, sizeof(struct SFD1HDR), ABORT);
         } /* End writing SFD1HDRs */
      } /* End D1 */
#endif

/* Write headers for all cell types and connection types */

   for (ir=RP->tree; ir; ir=ir->pnrp) {

      /* Set up repertoire header */
      memcpy((char *)h, (char *)&ursfrephdr,
         sizeof(struct SFREPHDR));
      strncpy(h->rephdr.repnam.d, ir->sname, LSNAME);
      SFHPutL(rephdr.sfrngx, ir->ngx);
      SFHPutL(rephdr.sfrngy, ir->ngy);
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
         SFHPutL(cthdr.sfctnel,    il->nel);
         SFHPutL(cthdr.sfctlel,    il->lel);
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
         {  si32 tas = il->kautu ? RP->psclmul[il->oautsc] : S24;
            SFHPutL(cthdr.sfctasm, tas); }
         SFHPutL(cthdr.sfctnsd, il->nsd);
         SFHPutL(cthdr.sfctrsd, il->rsd);
         {  si32 tpsd = il->pctpd ? il->pctpd->phiseed : 0;
            SFHPutL(cthdr.sfctpsd, tpsd); }
         {  si32 tzsd = (il->Ct.rspmeth >= RF_IZH3) ?
            ((struct IZHICOM *)il->prfi)->izseed : 0;
            SFHPutL(cthdr.sfctzsd, tzsd); }
         SFHPutL(cthdr.offsi, offile);
            offile += (il->mxsdelay1*il->lspt);
         SFHPutL(cthdr.offprd, offile);
         rfwrite(pfd, h, sizeof(struct SFCTHDR), ABORT);

         /* Loop over specific connection types and write headers */
         offsyn = offile + il->ls;
         for (ix=il->pct1; ix; ix=ix->pct) {
            memcpy((char *)h, (char *)&ursfcnhdr,
               sizeof(struct SFCNHDR));
            strncpy(h->cnhdr.srcsnam.d, ix->srcnm.rnm, LSNAME);
            strncpy(h->cnhdr.srclnam.d, ix->srcnm.lnm, LSNAME);
            SFHPutL(cnhdr.sfcnnc,     ix->nc);
            SFHPutS(cnhdr.sfcnlc,     ix->lc);
            SFHPutS(cnhdr.sfcnlxn,    ix->lxn);
            SFHPutS(cnhdr.sfcnnbc,    ix->Cn.nbc);
            SFHPutL(cnhdr.sfcnkgen,   ix->kgen);
            {  register int i;
               for (i=0; i<CNNDV; i++)
                  bemfmi2(h->cnhdr.sfcnoff.d[i], ix->cnoff[i]);
               for (i=0; i<XNNDV; i++)
                  bemfmi2(h->cnhdr.sfxnoff.d[i], ix->xnoff[i]);
               }
            SFHPutL(cnhdr.sfcnsrcngx, ix->srcngx);
            SFHPutL(cnhdr.sfcnsrcngy, ix->srcngy);
            SFHPutL(cnhdr.sfcnsrcnel, ix->srcnel);
            SFHPutL(cnhdr.sfcnsrcnelt,ix->srcnelt);
            SFHPutL(cnhdr.sfcnloff,   ix->loff);
            SFHPutL(cnhdr.sfcnlsg,    ix->lsg);
            SFHPutL(cnhdr.sfcnnrx,    ix->nrx);
            SFHPutL(cnhdr.sfcnnry,    ix->nry);
            SFHPutS(cnhdr.sfcnnhx,    ix->nhx);
            SFHPutS(cnhdr.sfcnnhy,    ix->nhy);
            SFHPutS(cnhdr.sfcnnvx,    ix->nvx);
            SFHPutS(cnhdr.sfcnnvy,    ix->nvy);
            SFHPutL(cnhdr.sfcnnsa,    ix->nsa);
            SFHPutL(cnhdr.sfcnnsax,   ix->nsax);
            SFHPutL(cnhdr.sfcnnux,    ix->nux);
            SFHPutL(cnhdr.sfcnnuy,    ix->nuy);
            SFHPutL(cnhdr.sfcnradius, ix->radius);
            SFHPutL(cnhdr.sfcnstride, ix->stride);
            SFHPutL(cnhdr.sfcnxoff,   ix->xoff);
            SFHPutL(cnhdr.sfcnyoff,   ix->yoff);
            SFHPutL(cnhdr.sfcncseed,  ix->cseed);
            SFHPutL(cnhdr.sfcnlseed,  ix->lseed);
            SFHPutL(cnhdr.sfcnpseed,  ix->phseed);
            SFHPutL(cnhdr.sfcnrseed,  ix->rseed);
            SFHPutL(cnhdr.offprd, offsyn);
               offsyn += ix->lct;
            SFHPutS(cnhdr.sfcncbc,    ix->cbc);
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

#ifdef D1
/*---------------------------------------------------------------------*
*                       Write out the D1 data.                         *
*---------------------------------------------------------------------*/

   /* Loop over all D1 blocks */
   for (pd1=RP->pd1b1; pd1; pd1=pd1->pnd1b) {
      if ((size = pd1->nd1r*pd1->nepr) <= 0) continue;

/* Get a copy of the s(i) data and write it.
*    Note:  Inasmuch as all nodes except host have a copy of all
*  s(i) data in the current implementation, it would be easy and a
*  little quicker just to have node 1 send us a copy.  Instead, the
*  following code does an exchange.  It will therefore still work
*  in a future version wherein the data are distributed.  */

#ifdef PAR
      d1exch(pd1, TRUE);      /* If parallel, must exchange data */
#endif

#ifndef PARn                  /* Node 0 must locate and write data */
#ifdef PAR
      p1 = RP0->ps0;          /* Parallel: use gen'l exchange area */
#else
      p1 = pd1->pd1s;         /* Serial: data directly addressible */
#endif
      rfwrite(pfd, p1, size, ABORT);
#endif

/* Write the pd1r repertoire data */

#ifdef PARn
      /* Parallel comp node:  Send data to host */
      if (pd1->mynd1) {       /* (A node can have no data) */
         nnput(&svio, pd1->pd1r, pd1->mynd1*pd1->nbyt);
         nnpfl(&svio);
         } /* End sending data */
#endif

#ifdef PAR0
      /* Parallel host:  Get the data seriatim and write */
      for (relnode=0; relnode<nodes; relnode++) {
         /* Calculate nsize = size of data on current source node.
         *  Terminate loop when nsize becomes zero. */
         if (!(nsize = (pd1->cpn + (relnode < pd1->crn))*pd1->nbyt))
            break;
         nngcr(&svio, relnode+NC.headnode, SAVENET_MSG);
         while (nsize) {
            msize = nngetp(&svio, (void **)&pd);
            msize = min(nsize, msize);
            rfwrite(pfd, pd, msize, ABORT);
            nngsk(&svio, msize);
            nsize -= msize;
            } /* End transferring comp node data to file */
         nngcl(&svio);
         } /* End loop over comp nodes */
#endif /* PAR0 */

#ifndef PAR
      /* Serial:  Write repertoire data directly */
      size *= pd1->nbyt;      /* Total data length */
      rfwrite(pfd, pd1->pd1r, size, ABORT);
#endif

      } /* End loop over D1 blocks */
#endif /* D1 */

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
      if (nsize = il->mcells*il->lel) {   /* Assignment intended */
         nnput(&svio, il->prd, nsize);
         nnpfl(&svio);
         } /* End sending data */
#endif

#ifdef PAR0
      /* Parallel host:  Get the data and write */
      if (il->lel > 0) {
         for (relnode=0; relnode<il->nodes; relnode++) {
            /* Calculate nsize = size of data on current src node.
            *  Terminate loop when nsize becomes zero. */
            if (!(nsize = (il->cpn + (relnode < il->crn))*il->lel))
               break;
            nngcr(&svio, relnode+il->node1, SAVENET_MSG);
            while (nsize) {
               msize = nngetp(&svio, (void **)&pd);
               msize = min(nsize, msize);
               rfwrite(pfd, pd, msize, ABORT);
               nngsk(&svio, msize);
               nsize -= msize;
               } /* End transferring comp node data to file */
            nngcl(&svio);
            } /* End node loop */
         } /* End if lel > 0 */
#endif

#ifndef PAR
      /* Serial:  Write repertoire data directly */
      if (size = il->llt) {   /* Assignment intended */
         rfwrite(pfd, il->prd, size, ABORT);
         }
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
