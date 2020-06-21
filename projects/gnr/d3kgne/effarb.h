/* (c) Copyright 2010, Neurosciences Research Foundation, Inc. */
/* $Id: effarb.h 44 2011-03-04 22:36:14Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                         EFFARB Header File                           *
*                                                                      *
*     This block stores information defining motor neuron arbors       *
*                     for all types of effectors                       *
*                                                                      *
*  Notes relating to V8H revisions:                                    *
*  --Old arb1/arbw/cumarb mechanism has been removed.  A cell list     *
*    is the only way to specify an arbor.                              *
*  --To freeze an effector but keep its ljnwd entry, user can omit     *
*    celltype names and cell lists.  If just the cell list is omitted, *
*    an artificial arbor comprising all cells of the given type is     *
*    constructed.  OPT=F freezes and also omits ljnwd entries.         *
*  --Only BBD devices can use KEOP_CELLS option.                       *
*  --Group III EFFECTOR card can modify scales, thresholds, and arbs   *
*    for any effector type, also cell list if KEOP_CELLS is off.       *
*    Therefore d3news() must recheck for nelt overflow.                *
*  --The signal to envcyc() to omit moving something is UI32_MAX in    *
*    the pjnwd offset in the JOINT or WINDOW structure.                *
*                                                                      *
*  V8H, 11/07/10, GNR - New file, based on old effector.h              *
***********************************************************************/

#ifndef PARn
   struct EFFARB {
      struct EFFARB *pneb;    /* Ptr to next EFFARB block */
      struct EFFARB *pnetct;  /* Ptr to next EFFARB for a CELLTYPE */
      struct CELLTYPE *pect[NEXIN];
                              /* Ptrs to excit,inhib cell types */
      void  *peff;            /* Ptr to the actual effector */
      ilst  *pefil[NEXIN];    /* Ptr to effector cell lists */
      long  arbs;             /* Arbor separation */
      long  arbw;             /* Cells in 1 arbor (derived) */
      long  jjnwd[NEXIN];     /* Offsets of changes array entries */
      long  ljnwd;            /* Number of entries in pjnwd */
      float escli[NEXIN];     /* Effector scales (input) */
      float esclu[NEXIN];     /* Effector scales (used) */
      ui32  nchgs;            /* Number of device inputs = arbors */
      short eft[NEXIN];       /* Effector thresholds (mV S7/14) */
      rlnm  exinnm[NEXIN];    /* Source rep-layer names */
      txtid heffnm;           /* Handle to device name or number of
                              *  an unnamed device (joint,window) */
      byte  keop;             /* Option bits */
#define KEOP_FRZ     0x01        /* 'F' Freeze this effector */
#define KEOP_CELLS   0x02        /* 'C' Return all cells, not sum */
      byte keff;              /* Kind of effector */
#define KEF_ARM         0     /* Arm joint */
#define KEF_UJ          1     /* Universal joint */
#define KEF_WDW         2     /* Window */
#ifdef BBDS
#define KEF_BBD         3     /* BBD device */
#endif
      };

/* Function to format an effector name for an error message */
char *fmteffnm(struct EFFARB *pea);
#endif /* !defined(PARn) */
