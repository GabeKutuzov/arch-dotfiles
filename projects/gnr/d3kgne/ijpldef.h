/* (c) Copyright 1992-2010, Neurosciences Research Foundation, Inc. */
/* $Id: ijpldef.h 30 2010-07-09 21:26:56Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                         IJPLDEF Header File                          *
*  Rev, 03/22/92, GNR - cmtl->pcmtl, cmtarg, kcml to shorts, ARBOR     *
*  Rev, 09/28/97, GNR - Remove cml1, implement use of psyn0            *
*  V8B, 08/01/01, GNR - CIJ0PLOT, standard cell lists, labels          *
*  V8C, 07/06/03, GNR - Cell responses in millivolts, add conductances *
*  V8D, 09/24/07, GNR - Add MABCIJ, etc. options, SJPLOT, RJPLOT       *
*  ==>, 12/28/07, GNR - Last mod before committing to svn repository   *
*  Rev, 06/20/08, GNR - Add smshrink                                   *
*  Rev, 06/28/10, GNR - Move xlo,yhi to this block                     *                     *
***********************************************************************/

#define CM_END_DATA LONG_MAX       /* End signal for node-node stream */
#define CM_Cij2S8         23       /* Shift to bring Cij to S8 */
#define CM_Mij2S8          6       /* Shift to bring Mij to S8 */
#define CM_Sj2S8           6       /* Shift to bring Sj  to S8 */

#define SHRNKDFLT        2.0       /* Impossible shrink signals dflt */

#ifndef PARn
   struct CMVSUMS {           /* Sums for averaging plot variables */
      long  sumvar;           /* Sum of the variable */
      long  numvar;           /* Number of entries in the sum */
      };
#endif

   struct IJPLDEF {
      struct IJPLDEF  *pnijpl;/* Linked list chain pointer */
      struct CELLTYPE *pcmtl; /* Ptr to target layer block */
      struct CONNTYPE *pcmtx; /* Ptr to target connection block */
      struct CLBLK    *pcmcl; /* Ptr to cell list or NULL */
      float cmpltx;           /* Plot X coordinate */
      float cmplty;           /* Plot Y coordinate */
      float cmpltw;           /* Plot width  */
      float cmplth;           /* Plot height */
      float cmtopy;           /* Starting y allowing for interleave */
      float scdx,scdy;        /* Source cell delta x, y */
      float sgdx,sgdy;        /* Source group delta x, y */
      float tcdx,tcdy;        /* Target cell delta x, y */
      float tgdx,tgdy;        /* Target group delta x, y */
      float smshrink;         /* Shring factor for source map */
      float xlo,yhi;          /* Bounds adjusted for shrink */
      long  srcncx;           /* Number of source cells along X */
      long  tgtncx;           /* Number of target cells along X */
/* Next 3 items are stored on scale of variable being tested except
*  (S14) for Cij/Cij0 (so can hold +/-1 in a short).  Default marker
*  (-1) would be very hard to input on these scales by accident or
*  on purpose.  */
#define CMLIMDFLT -1          /* Marker for setting default limits */
      short cmmin;            /* Min Cij (S14) or Mij (S7/14), etc. */
      short cmmax;            /* Max Cij (S14) or Mij (S7/14), etc. */
      short cmamn;            /* Min abs(Cij) or abs(Mij), etc. */
      short cmclid;           /* Cell list identifier */
      short cmclndx;          /* Cell list index */
      short cmltln;           /* Label text length */
      txtid cmltxt;           /* Label text locator--0 to use lname */
      ui16  cmopt;            /* Plot option flags: */
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
#define CMNOSB   0x2000          /* N Omit sbar (OBSOLETE)     */
      ui16  cmtarg;           /* Number of target connection type */
      rlnm  cmtnm;            /* Target rep-layer name */
      byte  kcmplt;           /* Kind of plot--one of KCIJPlot
                              *  enum in d3global.h  */
      byte  kcmpldis;         /* TRUE if plot is disabled */
      char  cmltc[COLMAXCH];  /* Label color */
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
      byte  kcmplt;           /* Kind of plot--one of KCIJx defs
                              *  in d3global.h */
      byte  kcmpldis;         /* TRUE if plot is disabled */
      };

/* Prototype for IJPLDEF block initialization function */
void cmdflt(struct IJPLDEF *);
