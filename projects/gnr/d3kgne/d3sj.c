/* (c) Copyright 2006-2010, Neurosciences Research Foundation, Inc. */
/* $Id: d3sj.c 42 2011-01-03 21:37:10Z  $ */
/***********************************************************************
*                             CNS Program                              *
*          d3sj - Connection Afferent Input Lookup Routines            *
*                                                                      *
*  Note:  These routines are designed to make Sj and phase values      *
*  accessible at any point in CNS without regard to the input source   *
*  and whether Lij values are stored or computed.  This makes CNS      *
*  easier to understand and maintain, at a cost of some inner loop     *
*  calls and redundant bits of arithmetic.                             *
*                                                                      *
*  None of these routines should be called on a PAR0 node--this file   *
*  should not even be compiled on a PAR0 node, hence no #ifdefs.       *
*                                                                      *
*----------------------------------------------------------------------*
*                                                                      *
*  int ix->sjbr(struct CONNTYPE *ix)                                   *
*                                                                      *
*  This routine may be called once for each connection in succession   *
*     of the cell and connection type for which it has been set.  It   *
*     returns TRUE if Sj is found, FALSE if Sj is not found (because   *
*     Lij was out-of-bounds on the input array after window shifting). *
*     The value of Sj on the new (mV S7) scale is returned as sjval    *
*     in the Lija structure.  External inputs are mapped such that a   *
*     value of 1 is returned as 128 mV.  If the input is colored, the  *
*     raw C8, C16, or C24 value is returned in Lija.sjraw.             *
*  All external inputs are subject to color selection and, as of V8C,  *
*     are left-shifted by Sv2mV so that a value of 1.0 (S8) is equi-   *
*     valent to 128 mV (S7).                                           *
*  Prerequisites are that d3kij() and d3kiji() must have been called   *
*     to set up for the particular cell type and cell number to be     *
*     accessed, and d3kijx() and d3sjx() must have been called to      *
*     set up for the particular connection type to be accessed, in     *
*     that order.  The caller must maintain the synapse index (isyn)   *
*     and increment the data pointer (psyn) in the Lija structure,     *
*     as these are shared among all d3lij, d3cij, etc. calls.          *
*                                                                      *
*  int ix->pjbr(struct CONNTYPE *ix)                                   *
*                                                                      *
*  This routine returns the value of the input phase, if any, and      *
*     stores a copy in Lija.pijval.                                    *
*  Prerequisites:  Setup described above for sjbr routine also sets    *
*     up for this routine.  In addition, sjbr() must be called         *
*     before pjbr().                                                   *
*----------------------------------------------------------------------*
*                                                                      *
*  void d3sjx(struct CONNTYPE *ix)                                     *
*                                                                      *
*  This routine must be called to initialize the LIJARG array for a    *
*     particular connection type after the celltype and cell have      *
*     been established by d3kiji().                                    *
*                                                                      *
*----------------------------------------------------------------------*
*  V8D, 09/28/06, GNR - New routines, combining code from d3go() and   *
*                       other scattered places in CNS.                 *
*  ==>, 11/03/07, GNR - Last mod before committing to svn repository   *
*  Rev, 05/11/08, GNR - Make separate gsjval routine for value input   *
*  Rev, 06/03/08, GNR - Add color opponency                            *
*  Rev, 06/23/08, GNR - Windows bounds checks are no longer necessary  *
*                       inasmuch as lijbr routines now check windows   *
*  V8F, 02/15/10, GNR - Remove conditionals on TV inputs               *
*  V8H, 11/15/10, GNR - gsjvgr4 returns FALSE if sensor out-of-range   *
***********************************************************************/

#define LIJTYPE struct LIJARG

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "nccolors.h"
#include "d3global.h"
#include "rkarith.h"
#include "swap.h"

extern struct LIJARG Lija;

/*---------------------------------------------------------------------*
*                  Prototypes for Sj lookup routines                   *
*---------------------------------------------------------------------*/

int gsjiaggs(struct CONNTYPE *ix);  /* Input array -- Gray from GS */
int gsjiag08(struct CONNTYPE *ix);  /* Input array -- Gray from C8 */
int gsjiac08(struct CONNTYPE *ix);  /* Input array -- Color from C8 */
int gsjiao08(struct CONNTYPE *ix);  /* Input array -- Color opponent */
int gsjvg(struct CONNTYPE *ix);     /* Virtual groups */
int gsjvgr4(struct CONNTYPE *ix);   /* Float virtual groups */
int gsjtvggs(struct CONNTYPE *ix);  /* TV camera -- Gray from GS */
int gsjtvg08(struct CONNTYPE *ix);  /* TV camera -- Gray from C8 */
int gsjtvg16(struct CONNTYPE *ix);  /* TV camera -- Gray from C16 */
int gsjtvg24(struct CONNTYPE *ix);  /* TV camera -- Gray from C24 */
int gsjtvc08(struct CONNTYPE *ix);  /* TV camera -- Color from C8 */
int gsjtvo08(struct CONNTYPE *ix);  /* TV camera -- OpCol from C8 */
int gsjtvc16(struct CONNTYPE *ix);  /* TV camera -- Color from C16 */
int gsjtvo16(struct CONNTYPE *ix);  /* TV camera -- OpCol from C16 */
int gsjtvc24(struct CONNTYPE *ix);  /* TV camera -- Color from C24 */
int gsjtvo24(struct CONNTYPE *ix);  /* TV camera -- OpCol from C24 */
int gsjhv(struct CONNTYPE *ix);     /* Hand vision */
int gsjrep(struct CONNTYPE *ix);    /* Repertoire */
int gsjup(struct CONNTYPE *ix);     /* Call to user program */
int gsjval(struct CONNTYPE *ix);    /* Value */

int gpjc(struct CONNTYPE *ix);      /* Constant phase */
int gpjj(struct CONNTYPE *ix);      /* Input phase */
int gpjr(struct CONNTYPE *ix);      /* Random phase */
int gpju(struct CONNTYPE *ix);      /* Uniform phase */


/*=====================================================================*
*                                d3sjx                                 *
*                                                                      *
*  Initialize for Sj lookup for a particular connection type.  All     *
*  that is needed is to be sure Lij and Dij generation are available.  *
*=====================================================================*/

void d3sjx(struct CONNTYPE *ix) {

   Lija.wpseed = ix->phseed;     /* PHASR only */
   Lija.ics3 = 0;                /* Initialize color rotation */
   if (Lija.nLij < 0) d3lijx(ix);
   if (Lija.nDij < 0 && !(ix->kgfl & KGNXR)) d3dijx(ix);

   } /* End d3sjx() */


/*=====================================================================*
*                         Sj lookup routines                           *
*=====================================================================*/

/*---------------------------------------------------------------------*
*                              gsjiaggs                                *
*                                                                      *
*  Get gray Sj from gray input array.                                  *
*     Lij generation routines check bounds against iawxsz,iawysz.      *
*     Nonetheless, when nux,nuy do not match wwd,wht for KGNTP, a      *
*     further bounds check must be executed here when the stored       *
*     window offset is added.  There is no test for KGNTP, because     *
*     osrc is zero if not KGNTP and the test always passes.            *
*---------------------------------------------------------------------*/

int gsjiaggs(struct CONNTYPE *ix) {

   long Lij;

   if (!Lija.lijbr(ix)) return FALSE;
   Lij = Lija.lijval + ix->osrc;
   if (Lij & RP->xymaskc) return FALSE;

   /* Pick up Sj (S14) (no need to save psj for phase) */
   Lija.sjval = (long)ix->srcorg[Lij] << Sv2mV; /* To S14 = mV (S7) */

   return TRUE;

   } /* End gsjiaggs() */


/*---------------------------------------------------------------------*
*                              gsjiag08                                *
*                                                                      *
*  Get gray Sj from 8-bit color input array.                           *
*     (See notes under gsjiaggs re Lij checking).                      *
*---------------------------------------------------------------------*/

int gsjiag08(struct CONNTYPE *ix) {

   long Lij,Sj;

   if (!Lija.lijbr(ix)) return FALSE;
   Lij = Lija.lijval + ix->osrc;
   if (Lij & RP->xymaskc) return FALSE;

   Lija.sjraw = Sj = (long)ix->srcorg[Lij];
   Sj = GrayFromRGB(Sj);
   Lija.sjval = Sj << Sv2mV;        /* To mV (S7) */

   return TRUE;

   } /* End gsjiag08() */


/*---------------------------------------------------------------------*
*                              gsjiac08                                *
*                                                                      *
*  Get colored Sj from 8-bit color input array.                        *
*     (See notes under gsjiaggs re Lij checking).                      *
*---------------------------------------------------------------------*/

int gsjiac08(struct CONNTYPE *ix) {

   long Lij,Sj;
   int  ic;

   if (!Lija.lijbr(ix)) return FALSE;
   Lij = Lija.lijval + ix->osrc;
   if (Lij & RP->xymaskc) return FALSE;

   ic = Lija.ics3;
   Lija.ics3 = (Lija.ics3 + 1) % NColorDims;

   Lija.sjraw = Sj = (long)ix->srcorg[Lij];
   Lija.sjval = (Sj & ix->cnxc.tvmask[ic]) << ix->cnxc.tvshft[ic];

   return TRUE;

   } /* End gsjiac08() */


/*---------------------------------------------------------------------*
*                              gsjiao08                                *
*                                                                      *
*  Get color opponent Sj from 8-bit color input array.                 *
*     (See notes under gsjiaggs re Lij checking).                      *
*---------------------------------------------------------------------*/

int gsjiao08(struct CONNTYPE *ix) {

   long Lij,v,rgb[NColorDims];
   int  ic;

   if (!Lija.lijbr(ix)) return FALSE;
   Lij = Lija.lijval + ix->osrc;
   if (Lij & RP->xymaskc) return FALSE;

   ic = Lija.ics3;
   Lija.ics3 = (Lija.ics3 + 1) % NColorDims;

   Lija.sjraw = v = (long)ix->srcorg[Lij];
   rgb[0] = (v & 0x07) << 11;    /* Red */
   rgb[1] = (v & 0x38) <<  8;    /* Green */
   rgb[2] = (v & 0xc0) <<  6;    /* Blue */

   Lija.sjval = rgb[ix->cnxc.tvshft[ic]] - rgb[ix->cnxc.tvmask[ic]];

   return TRUE;

   } /* End gsjiao08() */


/*---------------------------------------------------------------------*
*                                gsjvg                                 *
*                                                                      *
*  Get Sj from virtual groups (one byte) or value                      *
*  Note:  The reason tvmode is checked here is that VT (touch) input   *
*     is allowed to be "colored" in order to pack more than one        *
*     object on a touch array.  This code is now obsolete and will     *
*     be removed when color selection is moved down to d3vtch.         *
*  Note:  Darwin II checked bounds also for VG, but this was for       *
*     VF,VR and is no longer needed.                                   *
*---------------------------------------------------------------------*/

int gsjvg(struct CONNTYPE *ix) {

   long Sj;

   if (!Lija.lijbr(ix)) return FALSE;

   /* Pick up Sj (S8) (no need to save psj for phase) */
   Lija.sjraw = Sj = (long)ix->srcorg[Lija.lijval<<ix->lijshft];
   if (ix->cnxc.tvmode == Col_GS)   /* Already gray */
      Lija.sjval = Sj << Sv2mV;
   else if (ix->cnxc.tvmask[0] == 0)/* Extract gray from 8-bit color */
      Lija.sjval = GrayFromRGB(Sj) << Sv2mV;
   else                             /* Extract color */
      Lija.sjval = (Sj & ix->cnxc.tvmask[0]) << ix->cnxc.tvshft[0];

   return TRUE;

   } /* End gsjvg() */


/*---------------------------------------------------------------------*
*                               gsjvgr4                                *
*                                                                      *
*  Get Sj from float virtual groups (four byte)                        *
*---------------------------------------------------------------------*/

int gsjvgr4(struct CONNTYPE *ix) {

   float *psj;

   if (!Lija.lijbr(ix)) return FALSE;

   /* Pick up Sj and scale to S14 (S7 mv) */
   psj = (float *)ix->srcorg;
   Lija.sjval = Lija.sjraw = (long)(RP->fvcscl * psj[Lija.lijval]);

   return (Lija.sjraw <= SHRT_MAX && Lija.sjraw >= -SHRT_MAX);

   } /* End gsjvgi4() */


/*---------------------------------------------------------------------*
*                              gsjtvggs                                *
*                                                                      *
*  Get gray-scale Sj from a gray-scale TV camera                       *
*---------------------------------------------------------------------*/

int gsjtvggs(struct CONNTYPE *ix) {

   if (!Lija.lijbr(ix)) return FALSE;

   /* Pick up Sj (S8) (no need to save psj for phase) */
   Lija.sjval = ((long)ix->srcorg[Lija.lijval]) << Sv2mV;

   return TRUE;

   } /* End gsjtvgs() */


/*---------------------------------------------------------------------*
*                              gsjtvg08                                *
*                                                                      *
*  Get gray-scale Sj from an 8-bit color TV camera                     *
*  Note:  Here the GrayFromRGB conversion is written out explicitly    *
*  rather than using the macro in nccolors.h with the idea that this   *
*  mode may ultimately be defined differently than the 8-bit IA color. *
*  Also, the roundoff was modified because the result is shifted left. *
*---------------------------------------------------------------------*/

int gsjtvg08(struct CONNTYPE *ix) {

   long Sj;

   if (!Lija.lijbr(ix)) return FALSE;

   Lija.sjraw = Sj = (long)ix->srcorg[Lija.lijval];
   Lija.sjval = (((Sj << 7 & 0x6000) +
      (Sj << 9 & 0x7000) + (Sj << 12 & 07000))/3 + 1) >> 1;

   return TRUE;

   } /* End gsjtvg08() */


/*---------------------------------------------------------------------*
*                              gsjtvc08                                *
*                                                                      *
*  Get Sj of selected color from an 8-bit color TV camera              *
*  (The stored tvshft brings the color value to S14 = mv S7.)          *
*---------------------------------------------------------------------*/

int gsjtvc08(struct CONNTYPE *ix) {

   long Sj;
   int  ic;

   if (!Lija.lijbr(ix)) return FALSE;

   ic = Lija.ics3;
   Lija.ics3 = (Lija.ics3 + 1) % NColorDims;

   Lija.sjraw = Sj = (long)ix->srcorg[Lija.lijval];
   Lija.sjval = (Sj & ix->cnxc.tvmask[ic]) << ix->cnxc.tvshft[ic];

   return TRUE;

   } /* End gsjtvc08() */


/*---------------------------------------------------------------------*
*                              gsjtvo08                                *
*                                                                      *
*  Get color opponency color from an 8-bit color TV camera             *
*---------------------------------------------------------------------*/

int gsjtvo08(struct CONNTYPE *ix) {

   long v,rgb[NColorDims];
   int  ic;

   if (!Lija.lijbr(ix)) return FALSE;

   ic = Lija.ics3;
   Lija.ics3 = (Lija.ics3 + 1) % NColorDims;

   Lija.sjraw = v = (long)ix->srcorg[Lija.lijval];
   rgb[0] = (v & 0x07) << 11;    /* Red */
   rgb[1] = (v & 0x38) <<  8;    /* Green */
   rgb[2] = (v & 0xc0) <<  6;    /* Blue */

   Lija.sjval = rgb[ix->cnxc.tvshft[ic]] - rgb[ix->cnxc.tvmask[ic]];

   return TRUE;

   } /* End gsjtvo08() */


/*---------------------------------------------------------------------*
*                              gsjtvg16                                *
*                                                                      *
*  Get gray-scale Sj from a 16-bit color TV camera                     *
*  This code assumes the red, then green, then blue data are in        *
*  memory in left->right order regardless of the machine endian mode.  *
*---------------------------------------------------------------------*/

int gsjtvg16(struct CONNTYPE *ix) {

   byte *pSj;
   long Sj;
   short sSj;

   if (!Lija.lijbr(ix)) return FALSE;

   pSj = ix->srcorg + Lija.lijval + Lija.lijval;
   sSj = bemtoi2(pSj);
   Lija.sjraw = Sj = (long)Sj;
   Sj = ((Sj & 0x7c00) + (Sj << 5 & 0x7c00) + (Sj << 10 & 0x7c00))/3;
   Lija.sjval = (Sj + 1) >> 1;   /* Round, shift to S14 = mV (S7) */

   return TRUE;

   } /* End gsjtvg16() */


/*---------------------------------------------------------------------*
*                              gsjtvc16                                *
*                                                                      *
*  Get Sj of selected color from a 16-bit color TV camera              *
*  This code assumes the red, then green, then blue data are in        *
*  memory in left->right order regardless of the machine endian mode.  *
*  Selected color is shifted first to S15, so red does not require a   *
*     right shift, then masked and shifted back to S14 = mV S7.        *
*---------------------------------------------------------------------*/

int gsjtvc16(struct CONNTYPE *ix) {

   byte *pSj;
   long Sj;
   int  ic;
   short sSj;

   if (!Lija.lijbr(ix)) return FALSE;

   ic = Lija.ics3;
   Lija.ics3 = (Lija.ics3 + 1) % NColorDims;

   pSj = ix->srcorg + Lija.lijval + Lija.lijval;
   sSj = bemtoi2(pSj);
   Lija.sjraw = Sj = (long)Sj;
   Lija.sjval = (Sj << ix->cnxc.tvshft[ic] & 0x7c00) >> 1;

   return TRUE;

   } /* End gsjtvc16() */


/*---------------------------------------------------------------------*
*                              gsjtvo16                                *
*                                                                      *
*  Get color opponency color from a 16-bit color TV camera             *
*  This code assumes the red, then green, then blue data are in        *
*  memory in left->right order regardless of the machine endian mode.  *
*---------------------------------------------------------------------*/

int gsjtvo16(struct CONNTYPE *ix) {

   byte *pSj;
   long v,rgb[NColorDims];
   int  ic;

   if (!Lija.lijbr(ix)) return FALSE;

   ic = Lija.ics3;
   Lija.ics3 = (Lija.ics3 + 1) % NColorDims;

   pSj = ix->srcorg + Lija.lijval + Lija.lijval;
   Lija.sjraw = v = (long)bemtoi2(pSj);
   rgb[0] = v >> 1 & 0x3e00;     /* Red */
   rgb[1] = v << 4 & 0x3e00;     /* Green */
   rgb[2] = v << 9 & 0x3e00;     /* Blue */
   Lija.sjval = rgb[ix->cnxc.tvshft[ic]] - rgb[ix->cnxc.tvmask[ic]];

   return TRUE;

   } /* End gsjtvo16() */


/*---------------------------------------------------------------------*
*                              gsjtvg24                                *
*                                                                      *
*  Get gray-scale Sj from a 24-bit color TV camera                     *
*  This code assumes the red, then green, then blue data are in        *
*  memory in left->right order regardless of the machine endian mode.  *
*---------------------------------------------------------------------*/

int gsjtvg24(struct CONNTYPE *ix) {

   byte *pSj;

   if (!Lija.lijbr(ix)) return FALSE;

   pSj = ix->srcorg + Lija.lijval + Lija.lijval + Lija.lijval;
   Lija.sjraw = (long)pSj[0] << 16 | (long)pSj[1] << 8 | (long)pSj[2];
   Lija.sjval = ((((long)pSj[0] + (long)pSj[1] + (long)pSj[2]) <<
      (Sv2mV+1))/3 + 1) >> 1;

   return TRUE;

   } /* End gsjtvg24() */


/*---------------------------------------------------------------------*
*                              gsjtvc24                                *
*                                                                      *
*  Get Sj of selected color from a 24-bit color TV camera              *
*  This code assumes the red, then green, then blue data are in        *
*  memory in left->right order regardless of the machine endian mode.  *
*---------------------------------------------------------------------*/

int gsjtvc24(struct CONNTYPE *ix) {

   byte *pSj;
   int  ic;

   if (!Lija.lijbr(ix)) return FALSE;

   ic = Lija.ics3;
   Lija.ics3 = (Lija.ics3 + 1) % NColorDims;

   pSj = ix->srcorg + Lija.lijval + Lija.lijval + Lija.lijval;
   Lija.sjraw = (long)pSj[0] << 16 | (long)pSj[1] << 8 | (long)pSj[2];
   Lija.sjval = (long)pSj[ix->cnxc.tvshft[ic]] << Sv2mV;

   return TRUE;

   } /* End gsjtvc24() */


/*---------------------------------------------------------------------*
*                              gsjtvo24                                *
*                                                                      *
*  Get color opponency color from a 24-bit color TV camera             *
*  This code assumes the red, then green, then blue data are in        *
*  memory in left->right order regardless of the machine endian mode.  *
*---------------------------------------------------------------------*/

int gsjtvo24(struct CONNTYPE *ix) {

   byte *pSj;
   int  ic;

   if (!Lija.lijbr(ix)) return FALSE;

   ic = Lija.ics3;
   Lija.ics3 = (Lija.ics3 + 1) % NColorDims;

   pSj = ix->srcorg + Lija.lijval + Lija.lijval + Lija.lijval;
   Lija.sjraw = (long)pSj[0] << 16 | (long)pSj[1] << 8 | (long)pSj[2];
   Lija.sjval = ((long)pSj[ix->cnxc.tvshft[ic]] -
      (long)pSj[ix->cnxc.tvmask[ic]]) << Sv2mV;

   return TRUE;

   } /* End gsjtvo24() */


/*---------------------------------------------------------------------*
*                                gsjhv                                 *
*                                                                      *
*  Get Sj from hand vision                                             *
*---------------------------------------------------------------------*/

int gsjhv(struct CONNTYPE *ix) {

   Lija.sjraw = Lija.sjval = ix->ul1.w.sj;

   return TRUE;

   } /* End gsjhv() */


/*---------------------------------------------------------------------*
*                               gsjrep                                 *
*                                                                      *
*  Get Sj from a repertoire                                            *
*  Must use delay as index into ptr array addressed by srcorg.         *
*  No need to perform color masking, but must shift if input is        *
*     phased.  Phase fetching should keep in mind that source may      *
*     or may not have phases that may or may not actually get used.    *
*  Save psj in Lija for possible use by type J phase pickup routine,   *
*     but do not save sjraw, spend the extra time only if used in dprt.*
*---------------------------------------------------------------------*/

int gsjrep(struct CONNTYPE *ix) {

   s_type **ppsj = (s_type **)ix->srcorg;

   if (!Lija.lijbr(ix)) return FALSE;
   if (ix->Cn.kdelay) ppsj += ix->dijbr(ix);
   Lija.psj = *ppsj + spsize(Lija.lijval, ix->lijshft);
   d3gts2(Lija.sjval, Lija.psj);

   return TRUE;

   } /* End gsjrep() */


/*---------------------------------------------------------------------*
*                                gsjup                                 *
*                                                                      *
*  Get Sj by calling a user-written program (version without RBAR)     *
*---------------------------------------------------------------------*/

int gsjup(struct CONNTYPE *ix) {

   long (*uprg)(byte *, long, int) =
      (long (*)(byte *, long, int))ix->sjup;

   if (!Lija.lijbr(ix)) return FALSE;

   Lija.sjval = uprg((byte *)ix->srcorg, Lija.lijval,
      (int)(ix->cnxc.tvcsel & ~EXTR3C));
   return TRUE;

   } /* End gsjup() */


/*---------------------------------------------------------------------*
*                               gsjval                                 *
*                                                                      *
*  Get Sj from a value scheme                                          *
*---------------------------------------------------------------------*/

int gsjval(struct CONNTYPE *ix) {

   if (!Lija.lijbr(ix)) return FALSE;

   Lija.sjraw = Lija.sjval =
      RP->pvdat[Lija.lijval].cnstrval << Sv2mV;

   return TRUE;

   } /* End gsjval() */


/*=====================================================================*
*                        Phase lookup routines                         *
*=====================================================================*/

int gpjc(struct CONNTYPE *ix) {        /* Constant phase */

   return (int)ix->Cn.phi;

   } /* End gpjc() */


int gpjj(struct CONNTYPE *ix) {        /* Input phase */

   return (int)(Lija.psj[LSmem]+ix->Cn.retard) & PHASE_MASK;

   } /* End gpjj() */


int gpjr(struct CONNTYPE *ix) {        /* Random phase */

   return (int)udev(&Lija.wpseed) & PHASE_MASK;

   } /* End gpjr() */


int gpju(struct CONNTYPE *ix) {        /* Uniform phase */

   return 0;

   } /* End gpju() */
