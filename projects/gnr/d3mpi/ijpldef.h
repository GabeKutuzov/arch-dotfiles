/* (c) Copyright 1992-2017, The Rockefeller University *11115* */
/* $Id: ijpldef.h 74 2017-09-18 22:18:33Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                         IJPLDEF Header File                          *
*                                                                      *
************************************************************************
*  Rev, 03/22/92, GNR - cmtl->pcmtl, cmtarg, kcml to shorts, ARBOR     *
*  Rev, 09/28/97, GNR - Remove cml1, implement use of psyn0            *
*  V8B, 08/01/01, GNR - CIJ0PLOT, standard cell lists, labels          *
*  V8C, 07/06/03, GNR - Cell responses in millivolts, add conductances *
*  V8D, 09/24/07, GNR - Add MABCIJ, etc. options, SJPLOT, RJPLOT       *
*  ==>, 12/28/07, GNR - Last mod before committing to svn repository   *
*  Rev, 06/20/08, GNR - Add smshrink                                   *
*  Rev, 06/28/10, GNR - Move xlo,yhi to this block                     *
*  R67, 12/08/16, GNR - Improved variable scaling, smshrink->smscale   *
*  R72, 03/30/17, GNR - Add Tloc, Sloc, kcmsmap, pfnsc, cmclloc, bgrvs *
***********************************************************************/

#include "plots.h"

#define CM_END_DATA LONG_MAX       /* End signal for node-node stream */
#define CM_Cij2S14        17       /* Shift to bring Cij to S14 */
#define CM_V2CSeqS        26       /* Scale of var*v2cseq */
#define CM_Cij2rgb         6       /* S14 Cij to S8 rgb color */
#define CM_V2MaxCM        23       /* Scale of var/vard for scaling */

#ifndef PARn
   struct CMVSUMS {           /* Sums for averaging plot variables */
      si64  sumvar;           /* Sum of the variable */
      si32  numvar;           /* Number of entries in the sum */
      };
#endif

   struct IJPLDEF {
      struct IJPLDEF  *pnijpl;/* Linked list chain pointer */
      struct CELLTYPE *pcmtl; /* Ptr to target layer block */
      struct CONNTYPE *pcmtx; /* Ptr to target connection block */
      struct CLBLK    *pcmcl; /* Ptr to cell list or NULL */
      float           *pfnsc; /* Sin,cos tables for needles */
      struct CPLOC     Tloc;  /* Target cell locator info */
      struct CPLOC     Sloc;  /* Source cell locator info */
      float cmpltx;           /* Plot X coordinate */
      float cmplty;           /* Plot Y coordinate */
      float cmpltw;           /* Plot width  */
      float cmplth;           /* Plot height */
      float cmtopy;           /* Starting y allowing for interleave */
      float smscale;          /* Scale factor for source map */
      ui32  nc1or3;           /* Number connections * 3 if colored */
      ui32  srcncx;           /* Number of source cells/gp along X */
      ui32  tgtncx;           /* Number of target cells/gp along X */
      clloc cmclloc;          /* Cell list identifier */
/* Next 3 items are stored on scale of variable being tested except
*  (S14) for Cij/Cij0 (so can hold +/-1 in a short).  Default marker
*  (-1) would be very hard to input on these scales by accident or
*  on purpose.  */
#define CMLIMDFLT -1          /* Marker for setting default limits */
      short cmmin;            /* Min Cij (S14) or Mij (S7/14), etc. */
      short cmmax;            /* Max Cij (S14) or Mij (S7/14), etc. */
      short cmamn;            /* Min abs(Cij) or abs(Mij), etc. */
      short cmltln;           /* Label text length */
      txtid cmltxt;           /* Label text locator--0 to use lname */
      ui16  cmopt;            /* Plot option flags: */
#define CMSNOM   0x0002          /* S w/o M for forcing S      */
#define CMSMAP   0x0003          /* M|S Linear Source Map      */
#define CMNTTL   0x0004          /* O Omit titles     (CMO3O)  */
#define CMVARY   0x0008          /* V Vary size       (CMO3V)  */
#define CMSIGN   0x0010          /* P Plot +/-        (CMO3P)  */
#define CMCOL    0x0020          /* K Colored         (CMO3K)  */
#define CMRTR    0x0040          /* 2 Retrace                  */
#define CMNOX    0x0080          /* X Omit out-of-range        */
#define CMTMAP   0x0100          /* T Target linear map        */
#define CMTAVG   0x0200          /* A Average over targets     */
#define CMISIL   0x0400          /* I Ignore source interleave */
#define CMITIL   0x0800          /* J Ignore target interleave */
#define CMSGRD   0x1000          /* G Add source grid          */
#define CMONDL   0x2000          /* N Plot orientation needles */
#define CMBOX    0x4000          /* B Map src into nrx,nry box */
#define CMCIRC   0x8000          /* C Use old circle colors    */
#define CMFOPT 0x010000          /* F use fixed colors--stored */
                                 /*   in doseq DCS_xFIX bits   */
      ui16  cmtarg;           /* Number of target connection type */
      ui16  doseq;            /* Type of color selection  Has  */
                              /* ix->cnxc.tvcsel for KCIJSEL plots */
#define DCS_SCALE    0           /* Scale excit or inhib color */
#define DCS_CSEQ     1           /* Use traditional color sequence */
#define DCS_EFIX     2           /* Use fixed excit color */
#define DCS_IFIX     4           /* Use fixed inhib color */
      bgrv  exbgr;            /* Excitatory high color components */
      bgrv  inbgr;            /* Inhibitory high color components */
      rlnm  cmtnm;            /* Target rep-layer name */
      byte  kcmplt;           /* Kind of plot--one of KIJPlot
                              *  enum in d3global.h  */
      byte  kcmsmap;          /* Kind of source mapping */
#define KCM_GCFLIJ 0x01          /* Group and cell from Lij */
#define KCM_COFLIJ 0x02          /* Cell only from Lij */
#define KCM_IAFLIJ 0x04          /* Input array from Lij */
#define KCM_CFJSYN 0x08          /* Cell from jsyn */
#define KCM_FSTXYA 0x10          /* Must adjust Lij from Lij0 */
#define KCM_DISABL 0x80          /* Plot is disabled */
      char  cmeico[NEXIN][COLMAXCH];   /* Excit,inhib colors */
      char  cmltc[COLMAXCH];           /* Label color */
      };                      /* End IJPLDEF */

/* In a parallel computer, only a few of the IJPLDEF variables are
*  needed on comp nodes.  To save overhead, an array of IJPLNODE
*  structs is built with these data and passed at membcst() time.
*  But do not use #ifdef here--def needed at serial nxdr2 time.  */

   struct IJPLNODE {
      struct CELLTYPE *pcmtl; /* Ptr to target layer block */
      struct CONNTYPE *pcmtx; /* Ptr to target connection block */
      struct CLBLK    *pcmcl; /* Ptr to cell list or NULL */
      ui16  cmopt;            /* Plot option flags--defs above */
      ui16  doseq;            /* Type of color selection */
      bgrv  exbgr;            /* Excitatory high color components */
      bgrv  inbgr;            /* Inhibitory high color components */
      byte  kcmplt;           /* Kind of plot--one of KCIJx defs
                              *  in d3global.h */
      byte  kcmsmap;          /* Kind of source mapping */
      };

/* Prototype for IJPLDEF block initialization function */
void cmdflt(struct IJPLDEF *);
