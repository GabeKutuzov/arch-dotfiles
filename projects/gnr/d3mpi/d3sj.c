/* (c) Copyright 2006-2018, The Rockefeller University *11115* */
/* $Id: d3sj.c 77 2018-03-15 21:08:14Z  $ */
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
*  Design decree:  All images in core will be native system endian--   *
*  input routines (d3imgt) must reverse if needed as they are called   *
*  only once per image while images may be accessed multiple times.    *
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
*     value of 1 is returned as 128 mV.                                *
*  All external inputs are subject to color selection and, as of V8C,  *
*     are left-shifted by Sv2mV so that a value of 1.0 (S8) is equi-   *
*     valent to 128 mV (S7).  Preprocessor outputs are already S7/14.  *
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
************************************************************************
*  V8D, 09/28/06, GNR - New routines, combining code from d3go() and   *
*                       other scattered places in CNS.                 *
*  ==>, 11/03/07, GNR - Last mod before committing to svn repository   *
*  Rev, 05/11/08, GNR - Make separate gsjval routine for value input   *
*  Rev, 06/03/08, GNR - Add color opponency                            *
*  Rev, 06/23/08, GNR - Windows bounds checks are no longer necessary  *
*                       inasmuch as lijbr routines now check windows   *
*  V8F, 02/15/10, GNR - Remove conditionals on TV inputs               *
*  V8H, 11/15/10, GNR - gsjvgr4 returns FALSE if sensor out-of-range   *
*  V8I, 03/02/13, GNR - Begin implementing d3xxj macros                *
*  R74, 06/16/17, GNR - Add routines for 48-bit color, eliminate old   *
*                       GrayFromRGB and ColorMode, no more save sjraw  *
*  Rev, 07/11/17, GNR - Implement weighted averaging GrayFromRGB       *
*  R75, 09/24/17, GNR - Add NOPVP Sj method (pix from highest angle)   *
*  R78, 04/23/18, GNR - Image & preproc gray-from-color now in d3imgt  *
*                       remove C16 access and C8 except from IA        *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "lijarg.h"
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
int gsjsfg16(struct CONNTYPE *ix);  /* PP_STGH -- Gray from GS16 */
int gsjsfc48(struct CONNTYPE *ix);  /* PP_STGH -- Color from C48 */
int gsjsfo48(struct CONNTYPE *ix);  /* PP_STGH -- OpCol from C48 */
int gsjvpg16(struct CONNTYPE *ix);  /* PP_GETP -- NOPVP from GS16 */
int gsjppg16(struct CONNTYPE *ix);  /* PREPROC -- Gray from GS16 */
int gsjppc48(struct CONNTYPE *ix);  /* PREPROC -- Color from C48 */
int gsjppo48(struct CONNTYPE *ix);  /* PREPROC -- OpCol from C48 */
int gsjtvggs(struct CONNTYPE *ix);  /* Cam-08  -- Gray from GS */
int gsjtvc24(struct CONNTYPE *ix);  /* Cam-08  -- Color from C24 */
int gsjtvo24(struct CONNTYPE *ix);  /* Cam-08  -- OpCol from C24 */
int gsjtvgg16(struct CONNTYPE *ix); /* Cam-16  -- Gray from GS16 */
int gsjtvc48(struct CONNTYPE *ix);  /* Cam-16  -- Color from C48 */
int gsjtvo48(struct CONNTYPE *ix);  /* Cam-16  -- OpCol from C48 */
int gsjhv(struct CONNTYPE *ix);     /* Hand vision */
int gsjrep(struct CONNTYPE *ix);    /* Repertoire */
int gsjup(struct CONNTYPE *ix);     /* Call to user program */
int gsjval(struct CONNTYPE *ix);    /* Value */
int gsjvg(struct CONNTYPE *ix);     /* Virtual groups */
int gsjvgr4(struct CONNTYPE *ix);   /* Float virtual groups */

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

   Lija.wpseed = ix->phseed;        /* PHASR only -- PAR or serial */
   Lija.ecs3 = (int)ix->cnxc.tveoff;/* Init excit color rotation */
   Lija.ics3 = (int)ix->cnxc.tvioff;/* Init inhib color rotation */
   if (Lija.nLij < 0) d3lijx(ix);
   if (Lija.nDij < 0 && !(ix->kgfl & KGNXR)) d3dijx(ix);

   } /* End d3sjx() */


/*=====================================================================*
*                         Sj lookup routines                           *
*  Note:  We may want a cycle-6 version of this for opponent colors    *
*=====================================================================*/

static byte cselsucc[4] = { 0, 1, 2, 0 };    /* Cyclic permute of BGR */

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
   Lija.sjval = (si32)ix->srcorg[Lij] << Sv2mV; /* To S14 = mV (S7) */

   return TRUE;

   } /* End gsjiaggs() */


/*---------------------------------------------------------------------*
*                              gsjiag08                                *
*                                                                      *
*  Get gray Sj from 8-bit color input array.                           *
*     (See notes under gsjiaggs re Lij checking).                      *
*  This uses a more accurate averaging algorithm than was possible     *
*  with the old S8 output because the result is now S14.               *
*---------------------------------------------------------------------*/

int gsjiag08(struct CONNTYPE *ix) {

   long Lij;
   si32 v;

   if (!Lija.lijbr(ix)) return FALSE;
   Lij = Lija.lijval + ix->osrc;
   if (Lij & RP->xymaskc) return FALSE;

   v = (si32)ix->srcorg[Lij];
   Lija.sjval = ((v << 7 & 0x6000) + (v << 9 & 0x7000) +
                (v << 12 & 0x7000) + ix->cnxc.tvrndc)/(2*NColorDims);

   return TRUE;

   } /* End gsjiag08() */


/*---------------------------------------------------------------------*
*                              gsjiac08                                *
*                                                                      *
*  Get colored Sj from 8-bit color input array.                        *
*     (See notes under gsjiaggs re Lij checking).                      *
*---------------------------------------------------------------------*/

int gsjiac08(struct CONNTYPE *ix) {

   struct EXTRCOLR *px = &ix->cnxc;
   long Lij;
   int  ec;

   if (!Lija.lijbr(ix)) return FALSE;
   Lij = Lija.lijval + ix->osrc;
   if (Lij & RP->xymaskc) return FALSE;

   ec = Lija.ecs3;
   /* Rotate color if EXTR3C, else this code does nothing */
   Lija.ecs3 = cselsucc[Lija.ecs3 + px->tv3cinc];

   Lija.sjval = ((si32)ix->srcorg[Lij] & px->tvmask[ec]) <<
      px->tvshft[ec];

   return TRUE;

   } /* End gsjiac08() */


/*---------------------------------------------------------------------*
*                              gsjiao08                                *
*                                                                      *
*  Get color opponent Sj from 8-bit color input array.                 *
*     (See notes under gsjiaggs re Lij checking).                      *
*---------------------------------------------------------------------*/

int gsjiao08(struct CONNTYPE *ix) {

   struct EXTRCOLR *px = &ix->cnxc;
   long Lij;
   si32 v;
   int  ec,ic;

   if (!Lija.lijbr(ix)) return FALSE;
   Lij = Lija.lijval + ix->osrc;
   if (Lij & RP->xymaskc) return FALSE;

   ec = Lija.ecs3, ic = Lija.ics3;
   /* Rotate colors if EXTR3C, else this code does nothing */
   Lija.ecs3 = cselsucc[Lija.ecs3 + px->tv3cinc];
   Lija.ics3 = cselsucc[Lija.ics3 + px->tv3cinc];

   v = (si32)ix->srcorg[Lij];
   Lija.sjval = ((v & px->tvmask[ec]) << px->tvshft[ec]) -
                ((v & px->tvmask[ic]) << px->tvshft[ic]);

   return TRUE;

   } /* End gsjiao08() */


/*---------------------------------------------------------------------*
*                              gsjsfg16                                *
*                                                                      *
*  Get Sj from steerable filter off 16-bit grayscale preproc output    *
*  Scaling:  {trig(S16)*pix(S14)=>S30}^2(=>S60)>>46(=>S14).            *
*---------------------------------------------------------------------*/

int gsjsfg16(struct CONNTYPE *ix) {

   si16 *psrc;
   si64 gh2sum;
   si32 ocos,osin;         /* Orientation cos,sin */
   si32 vg,vh;
   int  toff;

   if (!Lija.lijbr(ix)) return FALSE;

   psrc = (si16 *)ix->srcorg + Lija.lijval;
   toff = Lija.gcell + Lija.gcell;  /* Double for cos/sin */
   ocos = Lija.psfcst[ix->oPPsct + toff];
   osin = Lija.psfcst[ix->oPPsct + toff + 1];
   vg = ocos*(si32)psrc[0] +
        osin*(si32)psrc[ix->srcnimg];
   vh = ocos*(si32)psrc[2*ix->srcnimg] +
        osin*(si32)psrc[3*ix->srcnimg];
   gh2sum = jasw(jmsw(vg,vg),jmsw(vh,vh));
   Lija.sjval = swlo(jsrrsw(gh2sum, 46));

   return TRUE;

   } /* End gsjsfg16() */


/*---------------------------------------------------------------------*
*                              gsjsfc48                                *
*                                                                      *
*  Selected color Sj from steerable filter off 3x16-bit preproc output *
*  Scaling:  {trig(S16)*pix(S14)=>S30}^2(=>S60)>>46(=>S14).            *
*---------------------------------------------------------------------*/

int gsjsfc48(struct CONNTYPE *ix) {

   struct EXTRCOLR *px;
   si16 *psrc;
   si64 gh2sum;
   long Lij3;
   si32 ocos,osin;         /* Orientation cos,sin */
   si32 vg,vh;
   int  toff;

   if (!Lija.lijbr(ix)) return FALSE;

   px = &ix->cnxc;
   Lij3 = Lija.lijval + Lija.lijval + Lija.lijval + Lija.ecs3;
   /* Rotate color if EXTR3C, else this code does nothing */
   Lija.ecs3 = cselsucc[Lija.ecs3 + px->tv3cinc];

   psrc = (si16 *)ix->srcorg + Lij3;
   toff = Lija.gcell + Lija.gcell;  /* Double for cos/sin */
   ocos = Lija.psfcst[ix->oPPsct + toff];
   osin = Lija.psfcst[ix->oPPsct + toff + 1];
   vg = ocos*(si32)psrc[0] +
        osin*(si32)psrc[ix->srcnimg];
   vh = ocos*(si32)psrc[2*ix->srcnimg] +
        osin*(si32)psrc[3*ix->srcnimg];
   gh2sum = jasw(jmsw(vg,vg),jmsw(vh,vh));
   Lija.sjval = swlo(jsrrsw(gh2sum, 46));

   return TRUE;

   } /* End gsjsfc48() */


/*---------------------------------------------------------------------*
*                              gsjsfo48                                *
*                                                                      *
*  Opponent color Sj from steerable filter off 3x16-bit preproc output *
*  Scaling:  {trig(S16)*pix(S14)=>S30}^2(=>S60)>>46(=>S14).            *
*---------------------------------------------------------------------*/

int gsjsfo48(struct CONNTYPE *ix) {

   struct EXTRCOLR *px;
   si16 *psrc;
   si64 gh2sum;
   long Lij3;
   si32 ocos,osin;         /* Orientation cos,sin */
   si32 vg,vh;
   int  toff;

   if (!Lija.lijbr(ix)) return FALSE;

   px = &ix->cnxc;
   Lij3 = Lija.lijval + Lija.lijval + Lija.lijval;

   toff = Lija.gcell + Lija.gcell;  /* Double for cos/sin */
   ocos = Lija.psfcst[ix->oPPsct + toff];
   osin = Lija.psfcst[ix->oPPsct + toff + 1];
   /* Get the excitatory color */
   psrc = (si16 *)ix->srcorg + Lij3 + Lija.ecs3;
   vg = ocos*(si32)psrc[0] +
        osin*(si32)psrc[ix->srcnimg];
   vh = ocos*(si32)psrc[2*ix->srcnimg] +
        osin*(si32)psrc[3*ix->srcnimg];
   gh2sum = jasw(jmsw(vg,vg),jmsw(vh,vh));
   /* Get the inhibitory color */
   psrc = (si16 *)ix->srcorg + Lij3 + Lija.ics3;
   vg = ocos*(si32)psrc[0] +
        osin*(si32)psrc[ix->srcnimg];
   vh = ocos*(si32)psrc[2*ix->srcnimg] +
        osin*(si32)psrc[3*ix->srcnimg];
   gh2sum = jrsw(gh2sum, jasw(jmsw(vg,vg),jmsw(vh,vh)));

   /* Rotate colors if EXTR3C, else this code does nothing */
   Lija.ecs3 = cselsucc[Lija.ecs3 + px->tv3cinc];
   Lija.ics3 = cselsucc[Lija.ics3 + px->tv3cinc];

   Lija.sjval = swlo(jsrrsw(gh2sum, 46));

   return TRUE;

   } /* End gsjsfo48() */


/*---------------------------------------------------------------------*
*                              gsjvpg16                                *
*                                                                      *
*  Get Sj from steerable filter for which highest pixel over nel       *
*  orientations has been prestored by code at d3imgt.  Sj is scaled    *
*  according to the difference in orientation between the input and    *
*  the current connection.                                             *
*---------------------------------------------------------------------*/

int gsjvpg16(struct CONNTYPE *ix) {

   si32 vpdat, vpsj;
   int  dang;

   if (!Lija.lijbr(ix)) return FALSE;

   vpdat = RP->pjdat[ix->cnosfmx + Lija.lijval];
   vpsj  = vpdat & ((1 << SjVPB) - 1);
   dang = Lija.gcell - (si32)((ui32)vpdat >> SjVPB);
   /* Get dang to proper range for scale multiplier lookup */
   dang = abs32(dang);
   if ((dang+dang) > Lija.kl->nel) dang = Lija.kl->nel - dang;

   Lija.sjval = mrssl(vpsj, ix->cijfrac[dang], SjVPB);

   return TRUE;

   } /* End gsjvpg16() */


/*---------------------------------------------------------------------*
*                              gsjppg16                                *
*                                                                      *
*  Get Sj from 16-bit grayscale preproc output (already S7/14)         *
*---------------------------------------------------------------------*/

int gsjppg16(struct CONNTYPE *ix) {

   if (!Lija.lijbr(ix)) return FALSE;

   Lija.sjval = (si32)((si16 *)ix->srcorg)[Lija.lijval];

   return TRUE;

   } /* End gsjppg16() */



/*---------------------------------------------------------------------*
*                              gsjppc48                                *
*                                                                      *
*  Selected color Sj from 16-bit preproc output (already S7/14)        *
*---------------------------------------------------------------------*/

int gsjppc48(struct CONNTYPE *ix) {

   long Lij3;

   if (!Lija.lijbr(ix)) return FALSE;

   Lij3 = Lija.lijval + Lija.lijval + Lija.lijval + Lija.ecs3;

   /* Rotate color if EXTR3C, else this code does nothing */
   Lija.ecs3 = cselsucc[Lija.ecs3 + ix->cnxc.tv3cinc];

   Lija.sjval = (si32)((si16 *)ix->srcorg)[Lij3];

   return TRUE;

   } /* End gsjppc48() */


/*---------------------------------------------------------------------*
*                              gsjppo48                                *
*                                                                      *
*  Opponent color Sj from 16-bit preproc output (already S7/14)        *
*---------------------------------------------------------------------*/

int gsjppo48(struct CONNTYPE *ix) {

   struct EXTRCOLR *px;
   si16 *psrc;
   long Lij3;

   if (!Lija.lijbr(ix)) return FALSE;

   Lij3 = Lija.lijval + Lija.lijval + Lija.lijval;
   psrc = (si16 *)ix->srcorg + Lij3;
   px = &ix->cnxc;

   Lija.sjval = (si32)psrc[Lija.ecs3] - (si32)psrc[Lija.ics3];

   /* Rotate colors if EXTR3C, else this code does nothing */
   Lija.ecs3 = cselsucc[Lija.ecs3 + px->tv3cinc];
   Lija.ics3 = cselsucc[Lija.ics3 + px->tv3cinc];

   return TRUE;

   } /* End gsjppo48() */


/*---------------------------------------------------------------------*
*                              gsjtvggs                                *
*                                                                      *
*  Get gray-scale Sj from a 8-bit gray-scale TV camera                 *
*---------------------------------------------------------------------*/

int gsjtvggs(struct CONNTYPE *ix) {

   if (!Lija.lijbr(ix)) return FALSE;

   /* Pick up Sj (S8) (no need to save psj for phase) */
   Lija.sjval = ((si32)ix->srcorg[Lija.lijval]) << Sv2mV;

   return TRUE;

   } /* End gsjtvgs() */



/*---------------------------------------------------------------------*
*                              gsjtvc24                                *
*                                                                      *
*  Get Sj of selected color from a 24-bit color TV camera              *
*  This code assumes the red, then green, then blue data are in        *
*  memory in left->right order regardless of the machine endian mode.  *
*---------------------------------------------------------------------*/

int gsjtvc24(struct CONNTYPE *ix) {

   struct EXTRCOLR *px = &ix->cnxc;
   long Lij3;
   int  ec;

   if (!Lija.lijbr(ix)) return FALSE;
   Lij3 = Lija.lijval + Lija.lijval + Lija.lijval;

   ec = Lija.ecs3;
   /* Rotate color if EXTR3C, else this code does nothing */
   Lija.ecs3 = cselsucc[Lija.ecs3 + px->tv3cinc];

   Lija.sjval = (si32)ix->srcorg[Lij3+ec] << Sv2mV;

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

   struct EXTRCOLR *px = &ix->cnxc;
   long Lij3;
   int  ec,ic;

   if (!Lija.lijbr(ix)) return FALSE;
   Lij3 = Lija.lijval + Lija.lijval + Lija.lijval;

   ec = Lija.ecs3, ic = Lija.ics3;
   /* Rotate color if EXTR3C, else this code does nothing */
   Lija.ecs3 = cselsucc[Lija.ecs3 + px->tv3cinc];
   Lija.ics3 = cselsucc[Lija.ics3 + px->tv3cinc];

   Lija.sjval = ((si32)ix->srcorg[Lij3+ec] -
                 (si32)ix->srcorg[Lij3+ic]) << Sv2mV;

   return TRUE;

   } /* End gsjtvo24() */


/*---------------------------------------------------------------------*
*                              gsjtvgg16                               *
*                                                                      *
*  Get gray-scale Sj from a 16-bit gray-scale TV camera, reduce to S14 *
*---------------------------------------------------------------------*/

int gsjtvgg16(struct CONNTYPE *ix) {

   if (!Lija.lijbr(ix)) return FALSE;

   /* Pick up Sj (S16) (no need to save psj for phase) */
   Lija.sjval = ((si32)((ui16 *)ix->srcorg)[Lija.lijval] + 2) >> 2;

   return TRUE;

   } /* End gsjtvgg16() */



/*---------------------------------------------------------------------*
*                              gsjtvc48                                *
*                                                                      *
*  Get Sj of selected color from a 48-bit color TV camera              *
*  This code assumes the red, then green, then blue data are in        *
*  memory in left->right order regardless of the machine endian mode.  *
*---------------------------------------------------------------------*/

int gsjtvc48(struct CONNTYPE *ix) {

   ui16 *psrc;
   long Lij3;

   if (!Lija.lijbr(ix)) return FALSE;

   Lij3 = Lija.lijval + Lija.lijval + Lija.lijval + Lija.ecs3;
   psrc = (ui16 *)ix->srcorg;

   /* Rotate color if EXTR3C, else this code does nothing */
   Lija.ecs3 = cselsucc[Lija.ecs3 + ix->cnxc.tv3cinc];

   Lija.sjval = ((si32)psrc[Lij3] + 2) >> 2;

   return TRUE;

   } /* End gsjtvc48() */


/*---------------------------------------------------------------------*
*                              gsjtvo48                                *
*                                                                      *
*  Get color opponency color from a 48-bit color TV camera             *
*  This code assumes the red, then green, then blue data are in        *
*  memory in left->right order regardless of the machine endian mode.  *
*---------------------------------------------------------------------*/

int gsjtvo48(struct CONNTYPE *ix) {

   struct EXTRCOLR *px;
   ui16 *psrc;
   long Lij3;

   if (!Lija.lijbr(ix)) return FALSE;

   Lij3 = Lija.lijval + Lija.lijval + Lija.lijval;
   psrc = (ui16 *)ix->srcorg + Lij3;
   px = &ix->cnxc;

   Lija.sjval = ((si32)psrc[Lija.ecs3] -
                 (si32)psrc[Lija.ics3] + 2) >> 2;

   /* Rotate colors if EXTR3C, else this code does nothing */
   Lija.ecs3 = cselsucc[Lija.ecs3 + px->tv3cinc];
   Lija.ics3 = cselsucc[Lija.ics3 + px->tv3cinc];

   return TRUE;

   } /* End gsjtvo48() */


/*---------------------------------------------------------------------*
*                                gsjhv                                 *
*                                                                      *
*  Get Sj from hand vision                                             *
*---------------------------------------------------------------------*/

int gsjhv(struct CONNTYPE *ix) {

   Lija.sjval = ix->ul1.w.sj;

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
*  Save psj in Lija for possible use by type J phase pickup routine.   *
*---------------------------------------------------------------------*/

int gsjrep(struct CONNTYPE *ix) {

   s_type **ppsj = (s_type **)ix->srcorg;

   if (!Lija.lijbr(ix)) return FALSE;
   if (ix->Cn.kdelay) ppsj += ix->dijbr(ix);
   Lija.psj = *ppsj + spsize(Lija.lijval, ix->lijshft);
   d3gtjs2(Lija.sjval, Lija.psj);

   return TRUE;

   } /* End gsjrep() */


/*---------------------------------------------------------------------*
*                                gsjup                                 *
*                                                                      *
*  Get Sj by calling a user-written program (version without RBAR)     *
*---------------------------------------------------------------------*/

int gsjup(struct CONNTYPE *ix) {

   si32 (*uprg)(byte *, long, int) =
      (si32 (*)(byte *, long, int))ix->sjup;

   if (!Lija.lijbr(ix)) return FALSE;

   Lija.sjval = uprg((byte *)ix->srcorg, Lija.lijval,
      (int)(ix->cnxc.tvcsel & EXMASK));
   return TRUE;

   } /* End gsjup() */


/*---------------------------------------------------------------------*
*                               gsjval                                 *
*                                                                      *
*  Get Sj from a value scheme                                          *
*---------------------------------------------------------------------*/

int gsjval(struct CONNTYPE *ix) {

   if (!Lija.lijbr(ix)) return FALSE;

   Lija.sjval = SRA(RP->pvdat[Lija.lijval].cnstrval, FBvl-FBsi);

   return TRUE;

   } /* End gsjval() */


/*---------------------------------------------------------------------*
*                                gsjvg                                 *
*                                                                      *
*  Get Sj from virtual groups (one byte) or value                      *
*     If we add 16-bit virtual groups, this routine needs a partner.   *
*  Note:  The reason tvmode is checked here is that VT (touch) input   *
*     is allowed to be "colored" in order to pack more than one        *
*     object on a touch array.  This code is now obsolete and will     *
*     be removed when color selection is moved down to d3vtch.         *
*  Note:  Darwin II checked bounds also for VG, but this was for       *
*     VF,VR and is no longer needed.                                   *
*---------------------------------------------------------------------*/

int gsjvg(struct CONNTYPE *ix) {

   struct EXTRCOLR *px = &ix->cnxc;
   si32 v;

   if (!Lija.lijbr(ix)) return FALSE;

   /* Pick up Sj (S8) (no need to save psj for phase) */
   v = (si32)ix->srcorg[Lija.lijval];
   if (px->tvmode == BM_GS)      /* Already gray */
      Lija.sjval = v << Sv2mV;
   else if ((px->tvcsel & EXMASK) == BandW)
      /* Extract gray from 8-bit color */
      Lija.sjval = ((v << 7 & 0x6000) + (v << 9 & 0x7000) +
                   (v << 12 & 0x7000) + px->tvrndc)/(2*NColorDims);
   else {                        /* Extract single color */
      int ec = px->tveoff;
      Lija.sjval = (v & px->tvmask[ec]) << px->tvshft[ec];
      }

   return TRUE;

   } /* End gsjvg() */


/*---------------------------------------------------------------------*
*                               gsjvgr4                                *
*                                                                      *
*  Get Sj from float virtual groups (four byte)                        *
*---------------------------------------------------------------------*/

int gsjvgr4(struct CONNTYPE *ix) {

   float *psj,rsj;
   int sjck;

   if (!Lija.lijbr(ix)) return FALSE;

   /* Pick up Sj and scale to S14 (S7 mv) */
   psj = (float *)ix->srcorg;
   rsj = RP->fvcscl * psj[Lija.lijval];
   sjck = rsj <= (float)SHRT_MAX && rsj >= -(float)SHRT_MAX;
   Lija.sjval = sjck ? (si32)rsj : 0;

   return sjck;

   } /* End gsjvgi4() */


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
