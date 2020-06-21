/* (c) Copyright 1992-2011, Neurosciences Research Foundation, Inc. */
/* $Id: d3gfsv.c 46 2011-05-20 20:14:09Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3gfsv                                 *
*                         WRITE SIMDATA FILE                           *
*                                                                      *
*  As of V8H, this routine was divided into d3gfsv(), which writes all *
*  the items that are not related to the cell network, and d3gfsvct(), *
*  which handles cell and connection variables for a single celltype.  *
*  Both are called from the main pgm to write segment 0.  d3gfsv() is  *
*  called from d3go at the start of each cycle (GFNTC) or each trial,  *
*  and d3gfsvct() is called when celltype processing is complete. This *
*  allows variables like Sj that may change between the end of layer   *
*  processing and the end of a cycle (time of old d3gfsv() call) to be *
*  stored with their current values.  The cost of this is that new Si  *
*  and phase have to be acquired from il->ps2 and, if parallel, passed *
*  from comp nods to node 0.
*                                                                      *
*  Uses the graph data file format designed by Pearson & Shu (V.3).    *
*                                                                      *
*  Synopses:                                                           *
*     void d3gfsv(struct SDSELECT *pgd, ui32 gdm)                      *
*     void d3gfsvct(struct SDSELECT *pgd, struct CELLTYPE *il,         *
*        ui32 gdm)                                                     *
*  Arguments:                                                          *
*     pgd      Ptr to SDSELECT block defining items to be saved.       *
*     il       Ptr to CELLTYPE for cell type to be recorded.           *
*     gdm      Mask specifying items to be saved in this cycle.        *
*                                                                      *
*  Notes:  Call d3gfsv() on serial or host nodes only, d3gfsvct() on   *
*     all nodes.  Argument 'pgd' is not used on comp nodes, so it is   *
*     safe to pass a NULL value.  Both routines exit directly to       *
*     d3exit() in case of I/O errors.                                  *
*                                                                      *
*     All data are now written in big-endian order for portability.    *
*  Buffering in rfwrite is about as efficient as we can do.            *
*     Input array coords are stored as arrays of two FL2S3 variables.  *
*  Lij, Dij, and Izh[a-d] are written to a separate segment 0 by       *
*  separate calls to these routines with an appropriate gdm argument.  *
*  Lij is always written as a 4-byte quantity, basically because       *
*  values for all conntypes have to be the same length--there is no    *
*  provision in the header for different instances of the same item    *
*  to have different lengths, and the size must be big enough to code  *
*  out-of-bounds connections uniquely (as -1 values).                  *
*     Cij and Cij0 are now written as 2-byte quantities to save space. *
*  Like Lij, they all have to be the same length.  It will no longer   *
*  be possible to detect out-of-bounds by looking at Cij.  Instead,    *
*  the user can request the NUK (number used) variable at connection-  *
*  type level.  This approach will make SIMDATA files much shorter.    *
*     All out-of-bounds values must be written to the output file      *
*  because a constant record length is required by the file spec.      *
*     In the parallel case, the Cij and Lij are expanded on comp       *
*  nodes before sending to host, so the node-to-node I/O is all        *
*  uniform and because only the comp node can tell when an out-of-     *
*  bounds value is encountered.  This makes the msgs a little longer.  *
************************************************************************
*  Revision history:                                                   *
*  V2C, 03/07/87, GNR - Newly written                                  *
*  Rev, 05/  /90,  MC - Translate to C                                 *
*  Rev, 05/01/91, GNR - Make pgdcl a direct ptr to cell list           *
*  Rev, 03/02/92, KJF - Remove obsolete options, add Cij/Lij           *
*  Rev, 03/09/92, GNR - Use rf io routines, recode Cij/Lij,            *
*                       remove averaging                               *
*  V5E, 08/08/92, GNR - Add Cij0                                       *
*  Rev, 09/30/92, GNR - Make GRAFDATA little-endian on all systems     *
*  Rev, 01/19/93, GNR - Add support for OBJID                          *
*  Rev, 08/15/93, ABP - Use findcell                                   *
*  V6D, 02/09/94, GNR - Add Dij, check existence of Lij,Cij, etc.      *
*  V8A, 04/15/95, GNR - Merge kchng,flags into WDWDEF                  *
*  Rev, 11/27/96, GNR - Remove support for non-hybrid version          *
*  Rev, 08/26/97, GNR - Add IA, changes, PHIDIST, AK, NUK, MIJ, SBAR,  *
*                       cell list stride, internal cycles, gdm mask    *
*  Rev, 05/30/99, GNR - Use new swapping scheme                        *
*  Rev, 08/19/00, GNR - Add PPFIJ                                      *
*  V8B, 03/18/01, GNR - Use ROCKS iteration lists                      *
*  V8C, 03/22/03, GNR - Cell responses in millivolts, add conductances *
*  V8D, 07/29/06, GNR - Use lijbr() to fetch or generate Lij           *
*  Rev, 09/24/06, GNR - Use cijbr() to fetch or generate Cij           *
*  Rev, 12/27/07, GNR - Add QBAR                                       *
*  Rev, 02/01/08, GNR - Add SAVE SIMDATA SENSE                         *
*  ==>, 02/01/08, GNR - Last mod before committing to svn repository   *
*  Rev, 09/26/08, GNR - Minor type changes for 64-bit compilation      *
*  V8E, 02/05/09, GNR - Add Izhikevich u variable and a,b,c,d params   *
*  V8F, 06/05/10, GNR - Add CAMID, CAMINFO variables                   *
*  V8G, 08/28/10, GNR - Add ASM variable                               *
*  V8H, 11/08/10, GNR - Add SNSID variables                            *
*  Rev, 05/06/11, GNR - Use 'R' for arm, 'J' for Sj, split d3gfsv()    *
***********************************************************************/

#define NEWARB          /* Use new EFFARB mechanism */
#define SDSTYPE struct SDSELECT
#define CLTYPE  struct CLBLK
#define LIJTYPE struct LIJARG
#define TVTYPE  struct TVDEF

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "d3global.h"
#include "jntdef.h"
#include "armdef.h"
#include "wdwdef.h"
#include "objdef.h"
#include "d3fdef.h"
#include "tvdef.h"
#include "clblk.h"
#include "simdata.h"
#include "bapkg.h"
#include "swap.h"

extern LIJTYPE Lija;

/***********************************************************************
*                  nnputlong, nnputshort, nnputreal                    *
*  Macros to send a single long, short, or float from a comp node to   *
*  the host in big-endian format (NONSTANDARD so further swapping is   *
*  not needed at host when writing directly to output file).  First    *
*  arg is the address of the NNSTR stream data structure.  Second      *
*  arg is the data item to be moved.                                   *
*----------------------------------------------------------------------*
*  These macros avoid disturbing the value of the second arg in situ.  *
***********************************************************************/

#define nnputlong(a,b)  \
      { bemfmi4(cv,(b)); nnput(a,cv,FMLSIZE); }
#define nnputshort(a,b) \
      { bemfmi2(cv,(b)); nnput(a,cv,FMSSIZE); }
#define nnputreal(a,b) \
      { bemfmr4(cv,(b)); nnput(a,cv,FMESIZE); }

/***********************************************************************
*                              allciter                                *
*  This routine is used instead of ilstiter when we want to send data  *
*  for all cells in a layer and do not have an official ilst.  The     *
*  test for the end of the range is done by the caller.                *
*  (May be accessed from other PAR0 routines.)                         *
***********************************************************************/

   long allciter(iter *pit) {

      return (long)pit->inow++;

      } /* End allciter() */

/***********************************************************************
*                               d3gfsv                                 *
***********************************************************************/

void d3gfsv(struct SDSELECT *pgd, ui32 gdm) {

#ifndef PARn
   iter wkitc;                   /* Working iteration control */
   struct RFdef   *pgdf = d3file[pgd->gdfile];
   ui32           lsvi = pgd->suitms & gdm;

/*---------------------------------------------------------------------*
*                            TRIAL NUMBER                              *
*---------------------------------------------------------------------*/

   rfwi4(pgdf, RP0->trials0);

/*---------------------------------------------------------------------*
*                             INPUT ARRAY                              *
*---------------------------------------------------------------------*/

   if (lsvi & GFIA) {
      char *ppix = (char *)RP->pstim;
      char *ppixe = ppix + RP->nst + RP->nst;
      long lcol = RP->nsy, l2col = lcol + lcol;
      for ( ; ppix<ppixe; ppix+=l2col)
         rfwrite(pgdf, ppix, lcol, ABORT);
      } /* End input array output */

/*---------------------------------------------------------------------*
*                         WINDOW COORDINATES                           *
*---------------------------------------------------------------------*/

   /* As in d3gfsh(), pass windows with kchng < 0 or = 1,2,3,5,6,7 */
   if (lsvi & GFD1W) {
      struct WDWDEF *pw;      /* Ptr to current window */
      for (pw=RP0->pwdw1; pw; pw=pw->pnxwdw) {
         if (pw->kchng & 0x8003) {
            rfwi2(pgdf, (short)(8.0*(pw->wx + 0.5*pw->wwd)));
            rfwi2(pgdf, (short)(8.0*(pw->wy - 0.5*pw->wht)));
            }
         }
      } /* End window output */

/*---------------------------------------------------------------------*
*                 OBJECT COORDINATES AND IDENTIFIERS                   *
*  N.B.  For compatibility with old hc model files, the order of isn,  *
*  ign here is the reverse of that used with SNSID and CAMID output.   *
*  The cases can be distinguished in getgd3 by the variable names.     *
*---------------------------------------------------------------------*/

   if (lsvi & (GFOXY|GFOID)) {
      OBJECT *pobj;
      int iref;
      ilstset(&wkitc, pgd->pilobj, 0);
      while ((iref = (int)ilstiter(&wkitc)) >= 0) {
         pobj = envgob(RP0->Eptr, iref);
         if (pobj) {          /* Object exists */
            if (lsvi & GFOXY) {  /* Coords to S(3) */
               rfwi2(pgdf, (short)(pobj->icurx >> 13));
               rfwi2(pgdf, (short)(pobj->icury >> 13));
               }
            if (lsvi & GFOID) {
               rfwi2(pgdf, pobj->isn);
               rfwi2(pgdf, pobj->ign);
               }
            } /* End case: Object exists */
         else {               /* Object does not exist */
            if (lsvi & GFOXY) {  /* Store coords as zero */
               rfwi4(pgdf, 0);
               }
            if (lsvi & GFOID) {
               rfwi2(pgdf, -1);
               rfwi2(pgdf, -1);
               }
            } /* End case: Object does not exist */
         } /* End object loop */
      } /* End object output */

/*---------------------------------------------------------------------*
*                                VALUE                                 *
*---------------------------------------------------------------------*/

   if (lsvi & GFVAL) {
      int ival;
      ilstset(&wkitc, pgd->pilval, 0);
      while ((ival = (int)ilstiter(&wkitc)) > 0)
         rfwi2(pgdf, (short)RP->pvdat[ival-1].fullvalue);
      } /* End value output */

/*---------------------------------------------------------------------*
*                      EFFERENCE (CHANGES ARRAY)                       *
*  This code now handles non-IEEE floats if such ever come along.      *
*---------------------------------------------------------------------*/

   if (lsvi & GFEFF) {
      float *pjn,*pjne;
      pjne = RP0->pjnwd + RP0->ljnwd;
      for (pjn=RP0->pjnwd; pjn<pjne; pjn++)
         rfwr4(pgdf, *pjn);
      } /* End efference array output */

/*---------------------------------------------------------------------*
*                             SNSID DATA                               *
*  Note:  To match order used with existing GFUTV data, ign is placed  *
*  before isn, opposite to order in headers and bbd transmissions.     *
*---------------------------------------------------------------------*/

   if (lsvi & GFSID) {
      struct GDSNS *pgsns;
      struct VCELL *pvc;
      for (pgsns=pgd->pgdsns[PGD_SNSID]; pgsns; pgsns=pgsns->pngsns) {
         pvc = pgsns->pvsns;
         rfwi2(pgdf, pvc->ign);
         rfwi2(pgdf, pvc->isn);
         }
      } /* End sensor id data */

/*---------------------------------------------------------------------*
*                 JOINT COORDINATES (T and J options)                  *
*---------------------------------------------------------------------*/

   if (lsvi & (GFD1R|GFD1T)) {
      struct ARMDEF *pa;      /* Ptr to current arm */
      long i,ifirst;          /* Joint counter, first joint */
      for (pa=RP0->parm1; pa; pa=pa->pnxarm) { /* Loop over arms */
         ifirst = (lsvi & GFD1R) ? 0 : (pa->njnts-1);
         for (i=ifirst; i<pa->njnts; i++) {
            rfwi2(pgdf, (short)(8.0*(pa->pjnt+i)->u.jnt.jx));
            rfwi2(pgdf, (short)(8.0*(pa->pjnt+i)->u.jnt.jy));
            }
         }
      } /* End joint data */

/*---------------------------------------------------------------------*
*                             SENSOR DATA                              *
*---------------------------------------------------------------------*/

   if (lsvi & GFSNS) {
      struct GDSNS *pgsns;
      struct VCELL *pvc;
      for (pgsns=pgd->pgdsns[PGD_SENSE]; pgsns; pgsns=pgsns->pngsns) {
         pvc = pgsns->pvsns;
         if (pvc->vcflags & VKR4) {
            /* Floating-point data */
            float *psf = RP->paw + pvc->ovc;
            float *psfe = psf + pvc->ncells;
            while (psf<psfe)
               rfwr4(pgdf, *psf++);
            }
         else {
            /* Fixed point data--convert to float for
            *  consistent view of data by getgd3 reader */
            byte *psi = RP->pbcst + pvc->ovc;
            byte *pse = psi + pvc->ncells;
            while (psi<pse)
               rfwr4(pgdf, (float)((ui32)*psi++ << Sv2mV));
            }
         }
      } /* End sensor data */

/*---------------------------------------------------------------------*
*                  USER CAMERA STIMULUS OBJECT INFO                    *
*---------------------------------------------------------------------*/

   if (lsvi & GFUTV) {
      struct TVDEF *cam = pgd->pgdcam;
      rfwi2(pgdf, cam->utv.ign);
      rfwi2(pgdf, cam->utv.isn);
      rfwrite(pgdf, cam->utv.utvinfo, NUTVINFO, ABORT);
      } /* End camera stimulus object info */

#endif            /* End non-network section */
   } /* End d3gfsv() */


/***********************************************************************
*                              d3gfsvct                                *
*                      CELL AND CONNECTION DATA                        *
***********************************************************************/

void d3gfsvct(struct SDSELECT *pgd, struct CELLTYPE *il, ui32 gdm) {

   struct CLBLK    *pclb;        /* Ptr to current cell list block */
   s_type          *psi;         /* Ptr to s(i),phi(i) data */
   long (*iterator)(iter *);     /* Ptr to iterator function */
#ifdef PAR
   struct NNSTR    nnio;         /* Ctrl blk for node-node I/O */
   int             nnopened;     /* TRUE if stream opened */
#endif
   iter            wkitc;        /* Working iteration control */
#ifdef PARn
   char            cv[FMLSIZE];  /* Temp for swapped message data */
#else
   struct RFdef    *pgdf = d3file[pgd->gdfile];
#endif
#ifdef PAR0
   char            *pnd;         /* Ptr to node data */
   long            ind,jnd;      /* Length of node data written */
   long lnd = il->lsd1c;         /* Length of node data */
   int  node = -1,nnode;         /* Node,next node receiving from */
#else
   struct IZHICOM  *piz;         /* Ptr to Izhikevich params */
   struct CONNTYPE *ix;
   rd_type         *ig0;         /* Ptr to cell data */
   long            celloff;      /* Offset to cell data */
   static long     mone = -1;
   int  kopt = (il->Ct.ctopt & OPTMEM) == 0;
   int  izhushft = (il->Ct.rspmeth == RF_BREG) ? FBIu-FBwk : 0;
   int  offIFUW,offABCD;         /* Offsets to Izhikevich data */
   int  offSBAR,offQBAR,offPHD;  /* Offsets to cell items */
#endif
   long            cellno;       /* Number of current cell */
   long            mncell,mxcell;/* Lo,hi cell on my node */
   ui32            ilsv = il->suitms & gdm;

#ifdef PARn
   if (!(ilsv & (GFABCD|GFSBAR|GFQBAR|GFPHI|GFDIS|GFLIJ|GFCIJ|GFCIJ0|
      GFDIJ|GFSJ|GFMIJ|GFPPF|GFAFF|GFNUK|GFIFUW|GFSI))) return;
#else
   if (!ilsv) return;
#endif

   /* Prepare loop over cells */
   pclb = pgd->pgdcl;            /* Locate first real cell list */
   mxcell = (mncell = il->locell) + il->mcells - 1;
   if (il->suitms & GFGLBL) {
      iterator = allciter;
      wkitc.inow = mncell;
      }
   else {
      iterator = ilstiter;
      while (pclb && pclb->pcll != il) pclb = pclb->pnclb;
      ilstset(&wkitc, pclb->pclil, mncell);
      }

#ifdef PAR
   nnopened = FALSE;             /* Channel to node not open */
#endif

#ifndef PAR0
   piz = (struct IZHICOM *)il->prfi;
   /* Now using individual offsets for each item with one ig0
   *  pointer, rather than separate pointers for each item.
   *  May generate slightly better code, since pointers would
   *  need to be stored back in memory after incrementing.  */
   offSBAR = (int)il->ctoff[SBAR];
   offQBAR = (int)il->ctoff[QBAR];
   offPHD  = (int)il->ctoff[PHD];
   offIFUW = (int)il->ctoff[IZVU] + LSIZE;   /* (U stored after V) */
   offABCD = (int)il->ctoff[IZRA];
#endif

#ifdef PARn

/*---------------------------------------------------------------------*
*        Comp nodes send cell and connection data to node zero         *
* (It is unnecessary to send sclmul, which is always on Node 0, but as *
* of the d3gfsvct() split, it is necessary to send new Si and phi from *
* il->ps2.  Node-node message routines terminate on error--checks not  *
* needed here. This code violates our general rule that interprocessor *
* messages are little-endian in order that data received on host can   *
* be written to the big-endian file all of a piece.)                   *
*---------------------------------------------------------------------*/

   while ((cellno=iterator(&wkitc)) >= 0 && cellno <= mxcell) {
      long relcell = cellno - mncell;

      /* Got one, open stream to host */
      if (!nnopened) {
         nnpcr(&nnio, NC.hostid, SIMDATA_MSG);
         nnopened = TRUE; }

      /* Find cell data for current cell */
      psi = il->ps2 + spsize(relcell, il->phshft);
      celloff = relcell*il->lel;
      ig0 = il->prd + celloff;

      /* Write cell-level data (STATE, PHASE, IFUW, SBAR, QBAR,
      *  PhiDist, ABCD).  (Note:  d3allo guarantees IZVU, PHD, and Ak
      *  are aligned properly if required by the CPU.  Phase dists are
      *  stored in host byte order, not D3MEMACC_BYTE_ORDER.)  */

      if (ilsv & GFSI) {
#if D3MEMACC_BYTE_ORDER < 0
#if LSmem != FMSSIZE
#error LSmem is not same size as a short--recode d3gfsv
#endif
         char bemsi[2];
         bemsi[0] = psi[1];
         bemsi[1] = psi[0];
         nnput(&nnio, bemsi, FMSSIZE);
#else
         nnput(&nnio, psi, FMSSIZE);
#endif
         }
      if (ilsv & GFPHI)
         nnput(&nnio, psi+LSmem, 1);
         }
      if (ilsv & GFIFUW) {
         long izhu;
         d3gti4(izhu, (ig0+offIFUW));
         izhu <<= izhushft;
#if D3MEMACC_BYTE_ORDER < 0
         nnputlong(&nnio, izhu);
#else
         nnput(&nnio, &izhu, FMLSIZE);
#endif
         }
      if (ilsv & GFSBAR) {
#if D3MEMACC_BYTE_ORDER < 0
         char bemsbar[2];
         bemsbar[0] = ig0[offSBAR+1];
         bemsbar[1] = ig0[ offSBAR ];
         nnput(&nnio, bemsbar, FMSSIZE);
#else
         nnput(&nnio, ig0+offSBAR, FMSSIZE);
#endif
         }
      if (ilsv & GFQBAR) {
#if D3MEMACC_BYTE_ORDER < 0
         char bemqbar[2];
         bemqbar[0] = ig0[offQBAR+1];
         bemqbar[1] = ig0[ offQBAR ];
         nnput(&nnio, bemqbar, FMSSIZE);
#else
         nnput(&nnio, ig0+offQBAR, FMSSIZE);
#endif
         }
      if (ilsv & GFDIS) {
#if BYTE_ORDRE < 0
         long *pfdis = (long *)(ig0 + offPHD);
         register int j;
         char tfdis[PHASE_RES][FMLSIZE];

         for (j=0; j<PHASE_RES; j++)
            bemfmi4(tfdis[j],pfdis[j]);
         nnput(&nnio, tfdis, PHASE_RES*FMLSIZE);
#else
         nnput(&nnio, (char *)(ig0+offPHD), PHASE_RES*FMLSIZE);
#endif
         }
      /* GFABCD will only be set if this is segment 0 and
      *  Izhikevich variables actually exist */
      if (ilsv & GFABCD) {
         si32 abcd,rr;
         if (kopt) {             /* Values are in memory */
            rd_type *prr = ig0 + offABCD;
            if (piz->izra) {
               d3gth2(rr, prr); prr += SHSIZE;
               abcd = piz->iza + SRA(rr,2);
               nnputlong(&nnio, abcd); }
            if (piz->izrb) {
               d3gth2(rr, prr); prr += SHSIZE;
               abcd = piz->izb + SRA(rr,2);
               nnputlong(&nnio, abcd); }
            if (piz->izrc) {
               d3gth2(rr, prr); prr += SHSIZE;
               abcd = piz->izc + SRA(rr,FBrs-FBwk);
               nnputlong(&nnio, abcd); }
            if (piz->izrd) {
               d3gth2(rr, prr); prr += SHSIZE;
               abcd = piz->izd + SRA(rr,FBrs-FBwk);
               nnputlong(&nnio, abcd); }
            }
         else {                  /* Values must be computed */
            si32 lzseed = piz->izseed;
            udevskip(&lzseed, il->nizvar*cellno);
            /* N.B. Must discard bits to bring precision to
            *  same number of bits saved when not OPTMEM.  */
            if (piz->izra) {
               rr = udev(&lzseed) << 1;
               rr = mssle(rr, (si32)piz->izra, -31, OVF_IZHI);
               abcd = piz->iza + (rr << 14);
               nnputlong(&nnio, abcd); }
            if (piz->izrb) {
               rr = udev(&lzseed) << 1;
               rr = mssle(rr, (si32)piz->izrb, -31, OVF_IZHI);
               abcd = piz->izb + (rr << 14);
               nnputlong(&nnio, abcd); }
            if (piz->izrc) {
               rr = udev(&lzseed) << 1;
               rr = mssle(rr, abs32(rr), -31, OVF_IZHI);
               rr = mssle(rr, (si32)piz->izrc, -31, OVF_IZHI);
               abcd = piz->izc + (rr << (FBwk-FBsi));
               nnputlong(&nnio, abcd); }
            if (piz->izrd) {
               rr = udev(&lzseed) << 1;
               rr = mssle(rr, abs32(rr), -31, OVF_IZHI);
               rr = mssle(rr, (si32)piz->izrd, -31, OVF_IZHI);
               abcd = piz->izd + (rr << (FBwk-FBsi));
               nnputlong(&nnio, abcd); }
            }
         }

      /* If no connection stuff being stored, continue with next cell */
      if (!(ilsv & (GFAFF|GFNUK|GFLIJ|GFCIJ|GFCIJ0|GFSJ|
            GFDIJ|GFMIJ|GFPPF))) continue;

      /* Find conntype data for current cell */
      if (gdm & GFLIJ) d3kij(il, KMEM_GO);
      d3kiji(il, cellno);

      /* Loop over connection types */
      for (ix=il->pct1; ix; ix=ix->pct) {
         long gdval;          /* Unpacked data items  */
         long lnc = ix->nc;
         long msyn = -1;      /* Local jsyn tracker */
         int  offCij0 = (int)ix->cnoff[CIJ0];
         int  offMij  = (int)ix->cnoff[MIJ];
         int  offPPFij= (int)ix->cnoff[PPF];
         int  llc     = (int)ix->lc;
         int  qLij;           /* TRUE if storing Lij */
         int  qCij;           /* TRUE if calling cijbr() */
         int  qDij;           /* TRUE if calling dijbr() */
         int  qSj;            /* TRUE if calling sjbr() */
         ui32 ixsv = ix->suitms & gdm;
         short svCij;         /* Short value of Cij, Cij0, Sj */

         /* Always need Lij, even if just to maintain Lija.jsyn */
         d3kijx(ix);
         d3lijx(ix);

         /* Set flags for storage of Lij, Cij, Sj, Dij */
         qLij = (ixsv & GFLIJ) != 0;
         qCij = (ixsv & GFCIJ) != 0;
         qDij = (ixsv & GFDIJ) != 0;
         qSj  = (ixsv & GFSJ)  != 0;
         if (qCij) d3cijx(ix);
         if (qDij) d3dijx(ix);
         if (qSj)  d3sjx(ix);

         /* Send connection-type level data (AK and NUK) */
         if (ixsv & GFAFF) {
            float ak = *(float *)(Lija.pxnd + ix->xnoff[AK];
            nnputreal(&nnio, ak); }
         if (ixsv & GFNUK) {
            nnputlong(&nnio, Lija.nuk);
            }

/* Loop over synapses.
*  See notes at top of this file justifying expansion of Lij to 4
*  bytes, Cij to 2.  lijbr() is always called in order to maintain
*  Lija.jsyn so skipped connections can be inserted in their proper
*  places in sequence.  */

         for (Lija.isyn=0; Lija.isyn<lnc; ++Lija.isyn) {
            Lija.lijbr(ix);               /* Get Lij */
            while (++msyn < Lija.jsyn) {
               /* Fill in -1 data for any skipped connections */
               if (qLij)                  /* Lij's needed */
                  nnputlong(&nnio, UI32_MAX);
               if (qCij)                  /* Cij's are wanted */
                  nnputshort(&nnio, UI16_MAX);
               if (ixsv & GFCIJ0)         /* Cij0's are wanted */
                  nnputshort(&nnio, UI16_MAX);
               if (qSj)                   /* Sj's are wanted */
                  nnputshort(&nnio, UI16_MAX);
               if (ixsv & GFMIJ)          /* Mij's are wanted */
                  nnputshort(&nnio, UI16_MAX);
               if (qDij)                  /* Dij's are wanted */
                  nnput(&nnio, &mone, 1);
               if (ixsv & GFPPF)          /* PPF's are wanted */
                  nnputshort(&nnio, UI16_MAX);
               } /* Next synapse */
            if (msyn >= lnc) break;    /* Skipped last conn */

            if (qLij) {                /* Lij's needed */
               nnputlong(&nnio, Lija.lijval);
               }
            if (qCij) {                /* Cij's are wanted */
               svCij = ix->cijbr(ix) ? /* Don't need SRA here */
                  (short)(Lija.cijval >> Ss2hi) : -1;
               nnputshort(&nnio, svCij);
               }
            if (ixsv & GFCIJ0) {       /* Cij0's are wanted */
               d3gthn(gdval, Lija.psyn+offCij0, ix->cijlen);
               gdval &= ix->cmsk;      /* Isolate nbc bits */
               if (gdval == MINUS0) svCij = 0;
               else svCij = gdval>>Ss2hi;/* Don't need SRA */
               nnputshort(&nnio, svCij);
               }
            if (qSj) {                 /* Sj's are wanted */
               svCij = ix->sjbr(ix) ? (short)Lija.sjval : 0;
               nnputshort(&nnio, svCij);
               }
            if (ixsv & GFMIJ) {        /* Mij's are wanted */
#if D3MEMACC_BYTE_ORDER < 0
               char bemmij[2];
               bemmij[0] = Lija.psyn[offMij+1];
               bemmij[1] = Lija.psyn[ offMij ];
               nnput(&nnio, &bemmij, FMSSIZE);
#else
               nnput(&nnio, Lija.psyn+offMij, FMSSIZE);
#endif
               }
            if (qDij) {                /* Dij's are wanted */
               byte svDij = (byte)ix->dijbr(ix);
               nnput(&nnio, &svDij, 1);
               }
            if (ixsv & GFPPF) {        /* PPF's are wanted */
#if D3MEMACC_BYTE_ORDER < 0
               char bemppf[2];
               bemppf[0] = Lija.psyn[offPPFij+1];
               bemppf[1] = Lija.psyn[ offPPFij ];
               nnput(&nnio, bemppf, FMSSIZE);
#else
               nnput(&nnio, Lija.psyn+offPPFij, FMSSIZE);
#endif
               }
            Lija.psyn += llc;
            }        /* Next synapse */
         }           /* Next CONNTYPE */
      }              /* Next cell */
   if (nnopened) nnpcl(&nnio);   /* Close stream */

#else    /* PARn not defined */

/*---------------------------------------------------------------------*
*  Node zero and serial nodes write cell and connection data to file   *
*---------------------------------------------------------------------*/

   /* Write autoscale multiplier if requested */
   if (ilsv & GFASM) rfwi4(pgdf, RP->psclmul[il->oautsc]);

   /* Loop over cells */
   while ((cellno=iterator(&wkitc)) >= 0 && cellno <= mxcell) {

#ifdef PAR

/*---------------------------------------------------------------------*
*       Parallel computer host node--nnget data from comp nodes        *
*---------------------------------------------------------------------*/

      /* If no data coming from comp nodes, continue with next cell */
      if (!(ilsv & (GFABCD|GFSBAR|GFQBAR|GFPHI|GFDIS|GFLIJ|GFCIJ|GFCIJ0|
         GFDIJ|GFSJ|GFMIJ|GFPPF|GFAFF|GFNUK|GFIFUW|GFSI))) continue;

      /* See what node this cell is on and if new,
      *  open a stream to receive from that node.  */
      nnode = findcell(il, cellno);
      if (nnode != node) {
         if (nnopened) nngcl(&nnio); /* Close previous */
         nngcr(&nnio, node = nnode, SIMDATA_MSG);
         nnopened = TRUE; }

      /* Transfer data for one cell to output */
      for (ind=lnd; ind; ind-=jnd) {
         jnd = nngetp(&nnio, (void **)&pnd);
         jnd = min(jnd,ind);
         rfwrite(pgdf, pnd, jnd, ABORT);
         nngsk(&nnio, jnd);
         } /* End data transfer */

#else

/*---------------------------------------------------------------------*
*  Serial computer--write cell and connection data from local memory   *
*---------------------------------------------------------------------*/

      /* Find cell data for current cell */
      psi = il->ps2 + spsize(cellno, il->phshft);
      celloff = cellno*il->lel;
      ig0 = il->prd + celloff;

      /* Write cell-level data (STATE, PHASE, IFUW, SBAR, QBAR,
      *  PhiDist, ABCD).  (Note:  d3allo guarantees IZVU, PHD, and Ak
      *  are aligned properly if required by the CPU.  Phase dists are
      *  stored in host byte order, not D3MEMACC_BYTE_ORDER.)  */

      if (ilsv & GFSI) {
#if D3MEMACC_BYTE_ORDER < 0
#if LSmem != FMSSIZE
#error LSmem is not same size as a short--recode d3gfsv
#endif
         char bemsi[2];
         bemsi[0] = psi[1];
         bemsi[1] = psi[0];
         rfwrite(pgdf, bemsi, LSmem, ABORT);
#else
         rfwrite(pgdf, psi, LSmem, ABORT);
#endif
         }
      if (ilsv & GFPHI)
         rfwrite(pgdf, psi+LSmem, 1, ABORT);
      if (ilsv & GFIFUW) {
         long izhu;
         d3gti4(izhu, (ig0+offIFUW));
         izhu <<= izhushft;
         rfwi4(pgdf, izhu);
         }
      if (ilsv & GFSBAR) {
#if D3MEMACC_BYTE_ORDER < 0
         char bemsbar[2];
         bemsbar[0] = ig0[offSBAR+1];
         bemsbar[1] = ig0[ offSBAR ];
         rfwrite(pgdf, bemsbar, FMSSIZE, ABORT);
#else
         rfwrite(pgdf, ig0+offSBAR, FMSSIZE, ABORT);
#endif
         }
      if (ilsv & GFQBAR) {
#if D3MEMACC_BYTE_ORDER < 0
         char bemqbar[2];
         bemqbar[0] = ig0[offQBAR+1];
         bemqbar[1] = ig0[ offQBAR ];
         rfwrite(pgdf, bemqbar, FMSSIZE, ABORT);
#else
         rfwrite(pgdf, ig0+offQBAR, FMSSIZE, ABORT);
#endif
         }
      if (ilsv & GFDIS) {
#if BYTE_ORDRE < 0
         long *pfdis = (long *)(ig0 + offPHD);
         register int j;

         for (j=0; j<PHASE_RES; j++)
            rfwi4(pgdf, (si32)pfdis[j]);
#else
         rfwrite(pgdf, (char *)(ig0 + offPHD),
            PHASE_RES*FMLSIZE, ABORT);
#endif
         }
      /* GFABCD will only be set if this is segment 0 and
      *  Izhikevich variables actually exist */
      if (ilsv & GFABCD) {
         si32 abcd,rr;
         if (kopt) {             /* Values are in memory */
            rd_type *prr = ig0 + offABCD;
            if (piz->izra) {
               d3gth2(rr, prr); prr += SHSIZE;
               abcd = piz->iza + SRA(rr,2);
               rfwi4(pgdf, abcd); }
            if (piz->izrb) {
               d3gth2(rr, prr); prr += SHSIZE;
               abcd = piz->izb + SRA(rr,2);
               rfwi4(pgdf, abcd); }
            if (piz->izrc) {
               d3gth2(rr, prr); prr += SHSIZE;
               abcd = piz->izc + SRA(rr,FBrs-FBwk);
               rfwi4(pgdf, abcd); }
            if (piz->izrd) {
               d3gth2(rr, prr); prr += SHSIZE;
               abcd = piz->izd + SRA(rr,FBrs-FBwk);
               rfwi4(pgdf, abcd); }
            }
         else {                  /* Values must be computed */
            si32 lzseed = piz->izseed;
            udevskip(&lzseed, il->nizvar*cellno);
            if (piz->izra) {
               rr = udev(&lzseed) << 1;
               rr = mssle(rr, (si32)piz->izra, -31, OVF_IZHI);
               abcd = piz->iza + (rr << 14);
               rfwi4(pgdf, abcd); }
            if (piz->izrb) {
               rr = udev(&lzseed) << 1;
               rr = mssle(rr, (si32)piz->izrb, -31, OVF_IZHI);
               abcd = piz->izb + (rr << 14);
               rfwi4(pgdf, abcd); }
            if (piz->izrc) {
               rr = udev(&lzseed) << 1;
               rr = mssle(rr, abs32(rr), -31, OVF_IZHI);
               rr = mssle(rr, (si32)piz->izrc, -31, OVF_IZHI);
               abcd = piz->izc + (rr << (FBwk-FBsi));
               rfwi4(pgdf, abcd); }
            if (piz->izrd) {
               rr = udev(&lzseed) << 1;
               rr = mssle(rr, abs32(rr), -31, OVF_IZHI);
               rr = mssle(rr, (si32)piz->izrd, -31, OVF_IZHI);
               abcd = piz->izd + (rr << (FBwk-FBsi));
               rfwi4(pgdf, abcd); }
            }
         }
      /* If no connection stuff being stored, continue with next cell */
      if (!(ilsv & (GFAFF|GFNUK|GFLIJ|GFCIJ|GFCIJ0|GFSJ|
                  GFDIJ|GFMIJ|GFPPF))) continue;

      /* Find conntype data for current cell */
      if (gdm & GFLIJ) d3kij(il, KMEM_GO);
      d3kiji(il, cellno);

      /* Loop over connection types */
      for (ix=il->pct1; ix; ix=ix->pct) {
         long gdval;          /* Unpacked data items  */
         long lnc = ix->nc;
         long msyn = -1;      /* Local jsyn tracker */
         int  offCij0 = (int)ix->cnoff[CIJ0];
         int  offMij  = (int)ix->cnoff[MIJ];
         int  offPPFij= (int)ix->cnoff[PPF];
         int  llc     = (int)ix->lc;
         int  qLij;           /* TRUE if calling lijbr() */
         int  qCij;           /* TRUE if calling cijbr() */
         int  qDij;           /* TRUE if calling dijbr() */
         int  qSj;            /* TRUE if calling sjbr() */
         ui32 ixsv = ix->suitms & gdm;
         short svCij;         /* Short value of Cij, Cij0, Sj */

         /* Always need Lij, even if just to maintain Lija.jsyn */
         d3kijx(ix);
         d3lijx(ix);

         /* Set flags for storage of Lij, Cij, Sj, Dij */
         qLij = (ixsv & GFLIJ) != 0;
         qCij = (ixsv & GFCIJ) != 0;
         qDij = (ixsv & GFDIJ) != 0;
         qSj  = (ixsv & GFSJ)  != 0;
         if (qCij) d3cijx(ix);
         if (qDij) d3dijx(ix);
         if (qSj)  d3sjx(ix);

         /* Write connection-type level data (Ak and NUk) */
         if (ixsv & GFAFF) {
            float ak = *(float *)(Lija.pxnd + ix->xnoff[AK]);
            rfwr4(pgdf, ak); }
         if (ixsv & GFNUK) {
            rfwi4(pgdf, (si32)Lija.nuk);
            }

/* Loop over synapses */

         for (Lija.isyn=0; Lija.isyn<lnc; ++Lija.isyn) {
            Lija.lijbr(ix);            /* Get Lij */
            while (++msyn < Lija.jsyn) {
            /* Fill in -1 data for any skipped connections */
               if (qLij)               /* Lij's needed */
                  rfwi4(pgdf, -1);
               if (qCij)               /* Cij's are wanted */
                  rfwi2(pgdf, -1);
               if (ixsv & GFCIJ0)      /* Cij0's are wanted */
                  rfwi2(pgdf, -1);
               if (qSj)                /* Sj's are wanted */
                  rfwi2(pgdf, -1);
               if (ixsv & GFMIJ)       /* Mij's are wanted */
                  rfwi2(pgdf, -1);
               if (qDij)               /* Dij's are wanted */
                  rfwrite(pgdf, &mone, 1, ABORT);
               if (ixsv & GFPPF)       /* PPF's are wanted */
                  rfwi2(pgdf, -1);
               } /* Next synapse */
            if (msyn >= lnc) break;    /* Skipped last conn */

            if (qLij) {                /* Lij's needed */
               rfwi4(pgdf, (si32)Lija.lijval);
               }
            if (qCij) {                /* Cij's are wanted */
               svCij = ix->cijbr(ix) ? /* Don't need SRA here */
                  (short)(Lija.cijval >> Ss2hi) : -1;
               rfwi2(pgdf, svCij);
               }
            if (ixsv & GFCIJ0) {       /* Cij0's are wanted */
               d3gthn(gdval, Lija.psyn+offCij0, ix->cijlen);
               gdval &= ix->cmsk;      /* Isolate nbc bits */
               if (gdval == MINUS0) svCij = 0;
               else svCij = gdval>>Ss2hi;/* Don't need SRA */
               rfwi2(pgdf, svCij);
               }
            if (qSj) {                 /* Sj's are wanted */
               svCij = ix->sjbr(ix) ? (short)Lija.sjval : 0;
               rfwi2(pgdf, svCij);
               }
            if (ixsv & GFMIJ) {        /* Mij's are wanted */
#if D3MEMACC_BYTE_ORDER < 0
               char bemmij[2];
               bemmij[0] = Lija.psyn[offMij+1];
               bemmij[1] = Lija.psyn[ offMij ];
               rfwrite(pgdf, bemmij, FMSSIZE, ABORT);
#else
               rfwrite(pgdf, Lija.psyn+offMij, FMSSIZE, ABORT);
#endif
               }
            if (qDij) {                /* Dij's are wanted */
               byte svDij = (byte)ix->dijbr(ix);
               rfwrite(pgdf, &svDij, 1, ABORT);
               }
            if (ixsv & GFPPF) {        /* PPF's are wanted */
#if D3MEMACC_BYTE_ORDER < 0
               char bemppf[2];
               bemppf[0] = Lija.psyn[offPPFij+1];
               bemppf[1] = Lija.psyn[ offPPFij ];
               rfwrite(pgdf, bemppf, FMSSIZE, ABORT);
#else
               rfwrite(pgdf, Lija.psyn+offPPFij, FMSSIZE, ABORT);
#endif
               }
            Lija.psyn += llc;
            }        /* Next synapse */
         }           /* Next CONNTYPE */
#endif   /* Serial */
      }              /* Next cell */
#ifdef PAR
   if (nnopened) nngcl(&nnio);   /* Close stream */
#endif

#endif   /* End combined host node and serial code */

   }  /* End d3gfsvct() */
