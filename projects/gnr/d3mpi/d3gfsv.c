/* (c) Copyright 1987-2018, The Rockefeller University *21116* */
/* $Id: d3gfsv.c 77 2018-03-15 21:08:14Z  $ */
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
*  from comp nodes to node 0.                                          *
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
*  All fields in out-of-bounds records are recorded as all binary 1's. *
*  However, if the connection exists, but Sj is not provided, Sj is    *
*  coded as '-0' (0x8000).                                             *
*     In the parallel computer, the Cij and Lij are expanded (or con-  *
*  tracted) on comp nodes before sending to host, so the node-to-node  *
*  I/O is all uniform.                                                 *
************************************************************************
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
*  V8I, 03/02/13, GNR - Begin implementing 64-bit, nnputlong->nnputi32 *
*  R67, 05/03/16, GNR - mpitools call args regularized                 *
*  R72, 02/26/17, GNR - Use mpgather, eliminate serial gathering       *
*  R76, 12/03/17, GNR - Add VMP variable, IZU for Izhi u, not IZVU[1]  *
*  R77, 02/08/18, GNR - Add RAW variable, put end test in allciter     *                               *
*  R77, 02/27/18, GNR - Change AK,RAW from ETYPE to FTYPE, S(20/27)    *
***********************************************************************/

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rkarith.h"
#include "collect.h"
#include "d3global.h"
#include "lijarg.h"
#include "jntdef.h"
#include "armdef.h"
#include "wdwdef.h"
#include "objdef.h"
#include "celldata.h"
#include "d3fdef.h"
#include "tvdef.h"
#include "clblk.h"
#include "simdata.h"
#include "bapkg.h"
#include "swap.h"

extern struct LIJARG Lija;
extern struct CELLDATA CDAT;

/* Define DBGGFSV for full debug output of PARn data */
/*** #define DBGGFSV  ***/
/*** #define DBGRFFL ***/

/***********************************************************************
*                   sdputi32, sdputshort, sdputreal                    *
*  Macros to send a single long, i32, short, or float from a comp node *
*  to the host in big-endian format (NONSTANDARD so further swapping   *
*  is not needed at host when writing directly to output file).        *
*  First argument:  Current address in the mpgather pdat area (using   *
*                   shared data area CDAT.psws).                       *
*  Second argument: Pointer to the data item to be moved.              *
*  N.B.  These particular data types have same message length as item  *
*     length.  If adding long types, must take difference into account.*
*----------------------------------------------------------------------*
*  These macros avoid disturbing the value of the second arg in situ.  *
***********************************************************************/

#if D3MEMACC_BYTE_ORDER < 0
#define sdputi32(pd,pi32) \
      { pd[0] = ((char *)(pi32))[3]; \
        pd[1] = ((char *)(pi32))[2]; \
        pd[2] = ((char *)(pi32))[1]; \
        pd[3] = ((char *)(pi32))[0]; pd += FMJSIZE; }
#define sdputshort(pd,pi16) \
      { pd[0] = ((char *)(pi16))[1]; \
        pd[1] = ((char *)(pi16))[0]; pd += FMSSIZE; }
#define sdputreal(pd,pr4) \
      { bemfmr4(pd,*(float *)(pr4)); pd += FMESIZE; }
#else
#define sdputi32(pd,pi32) \
      { memcpy(pd,(char *)(pi32),FMJSIZE); pd += FMJSIZE; }
#define sdputshort(pd,pi16) \
      { memcpy(pd,(char *)(pi16),FMSSIZE); pd += FMSSIZE; }
#define sdputreal(pd,pr4) \
      { memcpy(pd,(char *)(pr4),FMESIZE); pd += FMESIZE; }
#endif

/***********************************************************************
*                              allciter                                *
*  This routine is used instead of ilstiter when we want to send data  *
*  for all cells in a layer and do not have an official ilst.  May be  *
*  accessed from other CNS routines.                                   *
*  R77, 02/17/18, GNR - Put end of list test here instead of in caller *
***********************************************************************/

   long allciter(iter *pit) {

      if (pit->inow > pit->iend) return -1;
      else return (long)pit->inow++;

      } /* End allciter() */

/***********************************************************************
*                               d3gfsv                                 *
***********************************************************************/

void d3gfsv(struct SDSELECT *pgd, ui32 gdm) {

#ifndef PARn
   iter wkitc;                   /* Working iteration control */
   struct RFdef *pgdf = d3file[pgd->gdfile];
   ui32         lsui = pgd->suitms & gdm;

/*---------------------------------------------------------------------*
*                            TRIAL NUMBER                              *
*---------------------------------------------------------------------*/

   rfwi4(pgdf, RP0->trials0);
#ifdef DBGRFFL
rfflush(pgdf, ABORT);
#endif

/*---------------------------------------------------------------------*
*                             INPUT ARRAY                              *
*---------------------------------------------------------------------*/

   if (lsui & GFIA) {
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
   if (lsui & GFD1W) {
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

   if (lsui & (GFOXY|GFOID)) {
      OBJECT *pobj;
      int iref;
      ilstset(&wkitc, pgd->pilobj, 0);
      while ((iref = (int)ilstiter(&wkitc)) >= 0) {
         pobj = envgob(RP0->Eptr, iref);
         if (pobj) {          /* Object exists */
            if (lsui & GFOXY) {  /* Coords to S(3) */
               rfwi2(pgdf, (short)(pobj->icurx >> 13));
               rfwi2(pgdf, (short)(pobj->icury >> 13));
               }
            if (lsui & GFOID) {
               rfwi2(pgdf, pobj->isn);
               rfwi2(pgdf, pobj->ign);
               }
            } /* End case: Object exists */
         else {               /* Object does not exist */
            if (lsui & GFOXY) {  /* Store coords as zero */
               rfwi4(pgdf, 0);
               }
            if (lsui & GFOID) {
               rfwi2(pgdf, -1);
               rfwi2(pgdf, -1);
               }
            } /* End case: Object does not exist */
         } /* End object loop */
      } /* End object output */

/*---------------------------------------------------------------------*
*                                VALUE                                 *
*---------------------------------------------------------------------*/

   if (lsui & GFVAL) {
      int ival;
      ilstset(&wkitc, pgd->pilval, 0);
      while ((ival = (int)ilstiter(&wkitc)) > 0)
         rfwi2(pgdf, (short)RP->pvdat[ival-1].fullvalue);
      } /* End value output */

/*---------------------------------------------------------------------*
*                      EFFERENCE (CHANGES ARRAY)                       *
*  This code now handles non-IEEE floats if such ever come along.      *
*---------------------------------------------------------------------*/

   if (lsui & GFEFF) {
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

   if (lsui & GFSID) {
      struct GDSNS *pgsns;
      struct VCELL *pvc;
      for (pgsns=pgd->pgdsns[PGD_SNSID]; pgsns; pgsns=pgsns->pngsns) {
         pvc = pgsns->pvsns;
         rfwi2(pgdf, pvc->ign);
         rfwi2(pgdf, pvc->isn);
         }
      } /* End sensor id data */

/*---------------------------------------------------------------------*
*                 JOINT COORDINATES (T option--long ago was J)         *
*---------------------------------------------------------------------*/

   if (lsui & GFD1J) {
      struct ARMDEF *pa;      /* Ptr to current arm */
      long i;                 /* Joint counter */
      for (pa=RP0->parm1; pa; pa=pa->pnxarm) { /* Loop over arms */
         for (i=0; i<pa->njnts; i++) {
            rfwi2(pgdf, (short)(8.0*(pa->pjnt+i)->u.jnt.jx));
            rfwi2(pgdf, (short)(8.0*(pa->pjnt+i)->u.jnt.jy));
            }
         }
      } /* End joint data */

/*---------------------------------------------------------------------*
*                             SENSOR DATA                              *
*---------------------------------------------------------------------*/

   if (lsui & GFSNS) {
      struct GDSNS *pgsns;
      struct VCELL *pvc;
      for (pgsns=pgd->pgdsns[PGD_SENSE]; pgsns; pgsns=pgsns->pngsns) {
         pvc = pgsns->pvsns;
         if (pvc->vcflags & VKR4) {
            /* Floating-point data */
            float *psf = RP->paw + pvc->ovc;
            float *psfe = psf + pvc->nvcells;
            while (psf<psfe)
               rfwr4(pgdf, *psf++);
            }
         else {
            /* Fixed point data--convert to float for
            *  consistent view of data by getgd3 reader */
            byte *psi = RP->pbcst + pvc->ovc;
            byte *pse = psi + pvc->nvcells;
            while (psi<pse)
               rfwr4(pgdf, (float)((ui32)*psi++ << Sv2mV));
            }
         }
      } /* End sensor data */

/*---------------------------------------------------------------------*
*                  USER CAMERA STIMULUS OBJECT INFO                    *
*---------------------------------------------------------------------*/

   if (lsui & GFUTV) {
      struct TVDEF *cam = pgd->pgdcam;
      rfwi2(pgdf, cam->utv.ign);
      rfwi2(pgdf, cam->utv.isn);
      rfwrite(pgdf, cam->utv.utvinfo, NUTVINFO, ABORT);
      } /* End camera stimulus object info */

#ifdef DBGRFFL
rfflush(pgdf, ABORT);
#endif
#endif            /* End non-network section */
   } /* End d3gfsv() */


/***********************************************************************
*                              d3gfsvct                                *
*                      CELL AND CONNECTION DATA                        *
***********************************************************************/

void d3gfsvct(struct SDSELECT *pgd, struct CELLTYPE *il, ui32 gdm) {

#ifdef PARn
   char            *pd;          /* Ptr to data buffer */
#else
   struct RFdef    *pgdf = d3file[pgd->gdfile];
#endif
   struct CLBLK    *pclb;        /* Ptr to current cell list block */
#ifndef PAR0
   long (*iterator)(iter *);     /* Ptr to iterator function */
   iter            wkitc;        /* Working iteration control */
   struct IZHICOM  *piz;         /* Ptr to Izhikevich params */
   s_type          *psi;         /* Ptr to s(i),phi(i) data */
   struct CONNTYPE *ix;
   rd_type         *ig0;         /* Ptr to cell data */
   size_t          celloff;      /* Offset to cell data */
   int             cellno;       /* Number of current cell */
   int  kopt = (il->Ct.ctopt & OPTMEM) == 0;
   int  izhushft = (il->Ct.rspmeth == RF_BREG) ? FBIu-FBwk : 0;
   int  offIFUW,offABCD;         /* Offsets to Izhikevich data */
   int  offSBAR,offQBAR,offPHD;  /* Offsets to cell items */
   int  offVm;                   /* Offset to Vm(t) */
#endif
   ui32            ilsu = il->suitms & gdm;
   int             lblk;         /* Length of data for one cell */
   int             mncell;       /* Low cell accessed on node */
   int             mxcell;       /* High cell accessed on node */

#ifdef PARn
   if (!(ilsu & (GFVMP|GFSBAR|GFQBAR|GFPHI|GFDIS|GFLIJ|GFCIJ|GFCIJ0|
      GFDIJ|GFSJ|GFMIJ|GFPPF|GFRAW|GFAFF|GFNUK|GFIFUW|GFSI|GFABCD)))
      return;
#else
   if (!ilsu) return;
#endif

   /* Prepare loop over cells */
   pclb = il->psdclb;
   lblk = gdm & (GFLIJ|GFDIJ|GFABCD) ? il->lsd1c0 : il->lsd1c;
   mncell = il->locell;
   mxcell = il->hicell - 1;
#ifdef PARn
   pd = (char *)CDAT.psws + lblk*il->nsdcan;
#endif

#ifndef PAR0
   if (il->suitms & GFGLBL) {
      iterator = allciter;
      wkitc.inow = (ilstitem)mncell;
      wkitc.iend = (ilstitem)mxcell;
      }
   else {
      iterator = ilstiter;
      ilstset(&wkitc, pclb->pclil, (ilstitem)mncell);
      }
   piz = (struct IZHICOM *)il->prfi;
   /* Now using individual offsets for each item with one ig0
   *  pointer, rather than separate pointers for each item.
   *  May generate slightly better code, since pointers would
   *  need to be stored back in memory after incrementing.  */
   offVm   = (int)il->ctoff[VMP];
   offSBAR = (int)il->ctoff[SBAR];
   offQBAR = (int)il->ctoff[QBAR];
   offPHD  = (int)il->ctoff[PHD];
   offIFUW = (int)il->ctoff[IZU];
   offABCD = (int)il->ctoff[IZRA];
#endif

#ifdef PARn

/*---------------------------------------------------------------------*
*  Comp nodes gather and send cell and connection data to node zero    *
* (It is unnecessary to send asmul, which is always on Node 0, but as  *
* of the d3gfsvct() split, it is necessary to send new Si and phi from *
* il->ps2.  Node-node message routines terminate on error--checks not  *
* needed here. This code violates our general rule that interprocessor *
* messages are little-endian in order that data received on host can   *
* be written to the big-endian file all of a piece.)                   *
*---------------------------------------------------------------------*/

   while ((cellno=(int)iterator(&wkitc)) >= 0) {
      int relcell = cellno - mncell;

#ifdef DBGGFSV
dbgprt(ssprintf(NULL,"~Trial %d, Lname %2s, Cell %d",
   RP->CP.trial, il->lname, cellno));
#endif
      /* Find cell data for current cell */
      psi = il->ps2 + spsize(relcell, il->phshft);
      celloff = uw2zd(jmuwj(il->lel, relcell));
      ig0 = il->prd + celloff;

      /* Write cell-level data (STATE, PHASE, IFUW, Vm, SBAR, QBAR,
      *  PhiDist, ABCD).  (Note:  d3allo guarantees IZU, PHD,
      *  and Ak are aligned properly if required by the CPU.  */

      if (ilsu & GFSI) {               /* s(i) */
#if LSmem != FMSSIZE
#error LSmem is not same size as a short--recode d3gfsv
#endif
         sdputshort(pd, psi);
#ifdef DBGGFSV
{  si32 tsi; d3gtjs2(tsi, psi);
   dbgprt(ssprintf(NULL,"--Si = %jd", tsi));
   }
#endif
         }
      if (ilsu & GFPHI) {              /* phi */
         *pd++ = psi[LSmem];
#ifdef DBGGFSV
dbgprt(ssprintf(NULL,"--Phi = %d", (int)psi[LSmem]));
#endif
         }
      if (ilsu & GFIFUW) {             /* Izhikevich u */
         si32 izhu;
         d3gtjl4a(izhu, (ig0+offIFUW));
         izhu <<= izhushft;
         sdputi32(pd, &izhu);
         }
      if (ilsu & GFVMP) {              /* Vm */
         sdputi32(pd, ig0+offVm);
         }
      if (ilsu & GFSBAR) {             /* sbar */
         sdputshort(pd, ig0+offSBAR);
         }
      if (ilsu & GFQBAR) {             /* Slow sbar */
         sdputshort(pd, ig0+offQBAR);
         }
      if (ilsu & GFDIS) {              /* Phase distribution */
      /* N.B.  Phase dists are stored in host byte order, not
      *  D3MEMACC_BYTE_ORDER.  */
         char *pfd = (char *)(ig0 + offPHD);
#if BYTE_ORDRE < 0
         char *pfde = pfd + PHASE_RES*FMJSIZE;
         for ( ; pfd<pfde; pfd+=FMJSIZE) {
            *pd++ = pfd[3]; *pd++ = pfd[2];
            *pd++ = pfd[1]; *pd++ = pfd[0];
            }
#else
         memcpy(pd, pfd, PHASE_RES*FMJSIZE);
         pd += PHASE_RES*FMJSIZE;
#endif
#ifdef DBGGFSV
{  si32 tfd0; d3gtjl4(tfd0, ig0+offPHD);
   si32 tfdn; d3gtjl4(tfdn, ig0+offPHD+31*JSIZE);
   dbgprt(ssprintf(NULL,"--PhiDist[0,31] = %jd,%jd", tfd0,tfdn));
   }
#endif
         }
      /* GFABCD will only be set if this is segment 0 and
      *  Izhikevich variables actually exist */
      if (ilsu & GFABCD) {
         si32 abcd,rr;
         if (kopt) {             /* Values are in memory */
            rd_type *prr = ig0 + offABCD;
            if (piz->izra) {
               d3gtjh2a(rr, prr); prr += HSIZE;
               abcd = piz->iza + SRA(rr,2);
               sdputi32(pd, &abcd); }
            if (piz->izrb) {
               d3gtjh2a(rr, prr); prr += HSIZE;
               abcd = piz->izb + SRA(rr,2);
               sdputi32(pd, &abcd); }
            if (piz->izrc) {
               d3gtjh2a(rr, prr); prr += HSIZE;
               abcd = piz->izc + SRA(rr,FBrs-FBwk);
               sdputi32(pd, &abcd); }
            if (piz->izrd) {
               d3gtjh2a(rr, prr); prr += HSIZE;
               abcd = piz->izd + SRA(rr,FBrs-FBwk);
               sdputi32(pd, &abcd); }
            }
         else {                  /* Values must be computed */
            si32 lzseed = piz->izseed;
            udevwskp(&lzseed, jmsw(il->nizvar,cellno));
            /* N.B. Must discard bits to bring precision to
            *  same number of bits saved when not OPTMEM.  */
            if (piz->izra) {
               rr = udev(&lzseed) << 1;
               rr = mrssl(rr, (si32)piz->izra, FBrf);
               abcd = piz->iza + (rr << 14);
               sdputi32(pd, &abcd); }
            if (piz->izrb) {
               rr = udev(&lzseed) << 1;
               rr = mrssl(rr, (si32)piz->izrb, FBrf);
               abcd = piz->izb + (rr << 14);
               sdputi32(pd, &abcd); }
            if (piz->izrc) {
               rr = udev(&lzseed) << 1;
               rr = mrssl(rr, abs32(rr), FBrf);
               rr = mrssl(rr, (si32)piz->izrc, FBrf);
               abcd = piz->izc + (rr << (FBwk-FBsi));
               sdputi32(pd, &abcd); }
            if (piz->izrd) {
               rr = udev(&lzseed) << 1;
               rr = mrssl(rr, abs32(rr), FBrf);
               rr = mrssl(rr, (si32)piz->izrd, FBrf);
               abcd = piz->izd + (rr << (FBwk-FBsi));
               sdputi32(pd, &abcd); }
            }
         }

      /* If no connection stuff being stored, continue with next cell */
      if (!(ilsu & (GFRAW|GFAFF|GFNUK|GFLIJ|GFCIJ|GFCIJ0|GFSJ|
            GFDIJ|GFMIJ|GFPPF))) continue;

      /* Find conntype data for current cell */
      d3kij(il, KMEM_GO);
      d3kiji(il, cellno);

      /* Loop over connection types */
      for (ix=il->pct1; ix; ix=ix->pct) {
         long lnc = (long)ix->nc;
         long msyn = -1;      /* Local jsyn tracker */
         int  offCij0 = (int)ix->cnoff[CIJ0];
         int  offMij  = (int)ix->cnoff[MIJ];
         int  offPPFij= (int)ix->cnoff[PPF];
         int  llc     = (int)ix->lc;
         int  l1conn  = (int)(gdm & (GFLIJ|GFDIJ) ?
                           ix->lsd1n0 : ix->lsd1n);
         ui32 ixsu = ix->suitms & gdm;

         /* Always need Lij, even if just to maintain Lija.jsyn */
         d3kijx(ix);
         d3lijx(ix);
         if (ixsu & GFCIJ) d3cijx(ix);
         if (ixsu & GFDIJ) d3dijx(ix);
         if (ixsu & GFSJ)  d3sjx(ix);

         /* Send connection-type level data (AK, RAW, and NUK) */
         if (ixsu & GFAFF) {
            si32 ak = *(si32 *)(Lija.pxnd + ix->xnoff[AK]);
            sdputi32(pd, &ak); }
         if (ixsu & GFRAW) {
            si32 raw = *(si32 *)(Lija.pxnd + ix->xnoff[RAW]);
            sdputi32(pd, &raw); }
         if (ixsu & GFNUK) {
            si32 nuk32 = (si32)Lija.nuk;
            sdputi32(pd, &nuk32);
#ifdef DBGGFSV
   dbgprt(ssprintf(NULL,"---ict = %d, nuk = %jd",
      (int)ix->ict, nuk32));
#endif
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
               memset(pd, 0xff, l1conn);
               pd += l1conn;
               } /* Next synapse */
            if (msyn >= lnc) break;       /* Skipped last conn */

            if (ixsu & GFLIJ) {           /* Lij's needed */
               si32 L32 = (si32)Lija.lijval;
               sdputi32(pd, &L32);
               }
            /* Note:  Don't need SRA to shift Cij or Cij0--
            *  throwing away high-order bits anyway.  */
            if (ixsu & GFCIJ) {           /* Cij's are wanted */
               short svCij = ix->cijbr(ix) ?
                  (short)(Lija.cijval >> Ss2hi) : -1;
               sdputshort(pd, &svCij);
               }
            if (ixsu & GFCIJ0) {          /* Cij0's are wanted */
               si32 Cij0; short sCij0;
               d3gthn(Cij0, Lija.psyn+offCij0, ix->cijlen);
               Cij0 &= ix->cmsk;          /* Isolate nbc bits */
               sCij0 = (Cij0 == SI32_SGN) ? 0 : Cij0 >> Ss2hi;
               sdputshort(pd, &sCij0);
               }
            if (ixsu & GFSJ) {            /* Sj's are wanted */
               short svSj = ix->sjbr(ix) ?
                  (short)Lija.sjval : MISSING_SJ_16;
               sdputshort(pd, &svSj);
               }
            if (ixsu & GFMIJ) {           /* Mij's are wanted */
               sdputshort(pd, Lija.psyn+offMij);
               }
            if (ixsu & GFDIJ) {           /* Dij's are wanted */
               *pd++ = (char)ix->dijbr(ix);
               }
            if (ixsu & GFPPF) {           /* PPF's are wanted */
               sdputshort(pd, Lija.psyn+offPPFij);
               }
            Lija.psyn += llc;
            }        /* Next synapse */
         }           /* Next CONNTYPE */
      }              /* Next cell */
   /* And move all this stuff to Node 0 */
   mpgather((char *)CDAT.psws, (char *)CDAT.psws + lblk*il->nsdcan,
      il->nsdcpn, lblk, il->nsdcan, il->nsdctn, SIMDATA_MSG);

#else    /* PARn not defined */

/*---------------------------------------------------------------------*
*  Node zero and serial nodes write cell and connection data to file   *
*---------------------------------------------------------------------*/

   /* Write autoscale multiplier if requested (GFASM is forced
   *  off if autoscale does not exist)  */
   if (ilsu & GFASM) rfwi4(pgdf, il->pauts->asmul[0]);

#ifdef PAR

/*---------------------------------------------------------------------*
*     Parallel computer host node--mpgather data from comp nodes       *
*---------------------------------------------------------------------*/

   /* No data coming from comp nodes */
   if (!(ilsu & (GFVMP|GFSBAR|GFQBAR|GFPHI|GFDIS|GFLIJ|GFCIJ|GFCIJ0|
      GFDIJ|GFSJ|GFMIJ|GFPPF|GFRAW|GFAFF|GFNUK|GFIFUW|GFSI|GFABCD)))
      return;

   /* Gather all data for this celltype from all nodes and write */
   mpgather((char *)CDAT.psws, NULL, 0, lblk, il->nsdcan, 0,
      SIMDATA_MSG);
   rfwrite(pgdf, CDAT.psws, lblk*il->nsdcan, ABORT);

#else

/*---------------------------------------------------------------------*
*  Serial computer--write cell and connection data from local memory   *
*---------------------------------------------------------------------*/

   /* Loop over cells */
   while ((cellno=(int)iterator(&wkitc)) >= 0) {

      /* Find cell data for current cell */
      psi = il->ps2 + spsize(cellno, il->phshft);
      celloff = uw2zd(jmuwj(il->lel, cellno));
      ig0 = il->prd + celloff;

      /* Write cell-level data (STATE, PHASE, IFUW, SBAR, QBAR,
      *  PhiDist, ABCD).  (Note:  d3allo guarantees IZU, PHD, and Ak
      *  are aligned properly if required by the CPU.  Phase dists are
      *  stored in host byte order, not D3MEMACC_BYTE_ORDER.)  */

      if (ilsu & GFSI) {               /* s(i) */
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
#ifdef DBGRFFL
rfflush(pgdf, ABORT);
#endif
         }
      if (ilsu & GFPHI) {              /* phi */
         rfwrite(pgdf, psi+LSmem, 1, ABORT);
#ifdef DBGRFFL
rfflush(pgdf, ABORT);
#endif
         }
      if (ilsu & GFIFUW) {             /* Izhikevich u */
         si32 izhu;
         d3gtjl4a(izhu, (ig0+offIFUW));
         izhu <<= izhushft;
         rfwi4(pgdf, izhu);
         }
      if (ilsu & GFVMP) {              /* Vm */
         rfwi4(pgdf, *(si32 *)(ig0+offVm));
         }
      if (ilsu & GFSBAR) {             /* sbar */
#if D3MEMACC_BYTE_ORDER < 0
         char bemsbar[2];
         bemsbar[0] = ig0[offSBAR+1];
         bemsbar[1] = ig0[ offSBAR ];
         rfwrite(pgdf, bemsbar, FMSSIZE, ABORT);
#else
         rfwrite(pgdf, ig0+offSBAR, FMSSIZE, ABORT);
#endif
#ifdef DBGRFFL
rfflush(pgdf, ABORT);
#endif
         }
      if (ilsu & GFQBAR) {             /* Slow sbar */
#if D3MEMACC_BYTE_ORDER < 0
         char bemqbar[2];
         bemqbar[0] = ig0[offQBAR+1];
         bemqbar[1] = ig0[ offQBAR ];
         rfwrite(pgdf, bemqbar, FMSSIZE, ABORT);
#else
         rfwrite(pgdf, ig0+offQBAR, FMSSIZE, ABORT);
#endif
#ifdef DBGRFFL
rfflush(pgdf, ABORT);
#endif
         }
      if (ilsu & GFDIS) {              /* Phase distribution */
#if BYTE_ORDRE < 0
         si32 *pfdis = (si32 *)(ig0 + offPHD);
         register int j;

         for (j=0; j<PHASE_RES; j++)
            rfwi4(pgdf, pfdis[j]);
#else
         rfwrite(pgdf, (char *)(ig0 + offPHD),
            PHASE_RES*FMJSIZE, ABORT);
#endif
#ifdef DBGRFFL
rfflush(pgdf, ABORT);
#endif
         }
      /* GFABCD will only be set if this is segment 0 and
      *  Izhikevich variables actually exist */
      if (ilsu & GFABCD) {
         si32 abcd,rr;
         if (kopt) {             /* Values are in memory */
            rd_type *prr = ig0 + offABCD;
            if (piz->izra) {
               d3gtjh2a(rr, prr); prr += HSIZE;
               abcd = piz->iza + SRA(rr,2);
               rfwi4(pgdf, abcd); }
            if (piz->izrb) {
               d3gtjh2a(rr, prr); prr += HSIZE;
               abcd = piz->izb + SRA(rr,2);
               rfwi4(pgdf, abcd); }
            if (piz->izrc) {
               d3gtjh2a(rr, prr); prr += HSIZE;
               abcd = piz->izc + SRA(rr,FBrs-FBwk);
               rfwi4(pgdf, abcd); }
            if (piz->izrd) {
               d3gtjh2a(rr, prr); prr += HSIZE;
               abcd = piz->izd + SRA(rr,FBrs-FBwk);
               rfwi4(pgdf, abcd); }
            }
         else {                  /* Values must be computed */
            si32 lzseed = piz->izseed;
            udevwskp(&lzseed, jmsw(il->nizvar,cellno));
            if (piz->izra) {
               rr = udev(&lzseed) << 1;
               rr = mrssl(rr, (si32)piz->izra, FBrf);
               abcd = piz->iza + (rr << 14);
               rfwi4(pgdf, abcd); }
            if (piz->izrb) {
               rr = udev(&lzseed) << 1;
               rr = mrssl(rr, (si32)piz->izrb, FBrf);
               abcd = piz->izb + (rr << 14);
               rfwi4(pgdf, abcd); }
            if (piz->izrc) {
               rr = udev(&lzseed) << 1;
               rr = mrssl(rr, abs32(rr), FBrf);
               rr = mrssl(rr, (si32)piz->izrc, FBrf);
               abcd = piz->izc + (rr << (FBwk-FBsi));
               rfwi4(pgdf, abcd); }
            if (piz->izrd) {
               rr = udev(&lzseed) << 1;
               rr = mrssl(rr, abs32(rr), FBrf);
               rr = mrssl(rr, (si32)piz->izrd, FBrf);
               abcd = piz->izd + (rr << (FBwk-FBsi));
               rfwi4(pgdf, abcd); }
            }
         }
      /* If no connection stuff being stored, continue with next cell */
      if (!(ilsu & (GFRAW|GFAFF|GFNUK|GFLIJ|GFCIJ|GFCIJ0|GFSJ|
                  GFDIJ|GFMIJ|GFPPF))) continue;

      /* Find conntype data for current cell */
      d3kij(il, KMEM_GO);
      d3kiji(il, cellno);

      /* Loop over connection types */
      for (ix=il->pct1; ix; ix=ix->pct) {
         long lnc = (long)ix->nc;
         long msyn = -1;      /* Local jsyn tracker */
         int  offCij0 = (int)ix->cnoff[CIJ0];
         int  offMij  = (int)ix->cnoff[MIJ];
         int  offPPFij= (int)ix->cnoff[PPF];
         int  llc     = (int)ix->lc;
         int  l1conn  = (int)(gdm & (GFLIJ|GFDIJ) ?
                           ix->lsd1n0 : ix->lsd1n);
         ui32 ixsu = ix->suitms & gdm;

         /* Always need Lij, even if just to maintain Lija.jsyn */
         d3kijx(ix);
         d3lijx(ix);
         if (ixsu & GFCIJ) d3cijx(ix);
         if (ixsu & GFDIJ) d3dijx(ix);
         if (ixsu & GFSJ)  d3sjx(ix);

         /* Write connection-type level data (Ak and NUk) */
         if (ixsu & GFAFF) {
            si32 ak = *(si32 *)(Lija.pxnd + ix->xnoff[AK]);
            rfwi4(pgdf, ak); }
         if (ixsu & GFRAW) {
            si32 raw = *(si32 *)(Lija.pxnd + ix->xnoff[RAW]);
            rfwi4(pgdf, raw); }
         if (ixsu & GFNUK) {
            rfwi4(pgdf, (si32)Lija.nuk);
            }

/* Loop over synapses */

         /* Have a record of all -1 ready for encoding
         *     skipped connections */
         memset((char *)CDAT.psws, 0xff, l1conn);

         for (Lija.isyn=0; Lija.isyn<lnc; ++Lija.isyn) {
            Lija.lijbr(ix);               /* Get Lij */
            /* Fill in -1 data for any skipped connections */
            while (++msyn < Lija.jsyn)
               rfwrite(pgdf, CDAT.psws, l1conn, ABORT);
            if (msyn >= lnc) break;       /* Skipped last conn */

            if (ixsu & GFLIJ) {           /* Lij's needed */
               rfwi4(pgdf, (si32)Lija.lijval);
               }
            /* Note:  Don't need SRA to shift Cij or Cij0--
            *  throwing away high-order bits anyway.  */
            if (ixsu & GFCIJ) {           /* Cij's are wanted */
               short svCij = ix->cijbr(ix) ?
                  (short)(Lija.cijval >> Ss2hi) : -1;
               rfwi2(pgdf, svCij);
               }
            if (ixsu & GFCIJ0) {          /* Cij0's are wanted */
               si32 Cij0; short sCij0;
               d3gthn(Cij0, Lija.psyn+offCij0, ix->cijlen);
               Cij0 &= ix->cmsk;          /* Isolate nbc bits */
               sCij0 = (Cij0 == SI32_SGN) ? 0 : Cij0 >> Ss2hi;
               rfwi2(pgdf, sCij0);
               }
            if (ixsu & GFSJ) {            /* Sj's are wanted */
               short svSj = ix->sjbr(ix) ?
                  (short)Lija.sjval : MISSING_SJ_16;
               rfwi2(pgdf, svSj);
               }
            if (ixsu & GFMIJ) {           /* Mij's are wanted */
#if D3MEMACC_BYTE_ORDER < 0
               char bemmij[2];
               bemmij[0] = Lija.psyn[offMij+1];
               bemmij[1] = Lija.psyn[ offMij ];
               rfwrite(pgdf, bemmij, FMSSIZE, ABORT);
#else
               rfwrite(pgdf, Lija.psyn+offMij, FMSSIZE, ABORT);
#endif
               }
            if (ixsu & GFDIJ) {           /* Dij's are wanted */
               byte svDij = (byte)ix->dijbr(ix);
               rfwrite(pgdf, &svDij, 1, ABORT);
               }
            if (ixsu & GFPPF) {        /* PPF's are wanted */
#if D3MEMACC_BYTE_ORDER < 0
               char bemppf[2];
               bemppf[0] = Lija.psyn[offPPFij+1];
               bemppf[1] = Lija.psyn[ offPPFij ];
               rfwrite(pgdf, bemppf, FMSSIZE, ABORT);
#else
               rfwrite(pgdf, Lija.psyn+offPPFij, FMSSIZE, ABORT);
#endif
               }
#ifdef DBGRFFL
rfflush(pgdf, ABORT);
#endif
            Lija.psyn += llc;
            }        /* Next synapse */
         }           /* Next CONNTYPE */
      }              /* Next cell */
#endif   /* Serial */

#endif   /* End combined host node and serial code */

   }  /* End d3gfsvct() */
