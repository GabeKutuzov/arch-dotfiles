/* (c) Copyright 1995-2011, Neurosciences Research Foundation, Inc. */
/* $Id: modality.h 51 2012-05-24 20:53:36Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                        MODALITY Header File                          *
*                                                                      *
*  This header defines data structures and constants used to implement *
*  multiple sensory modalities, including cross-response statistcs.    *
*                                                                      *
*  NOTE:  Current logic requires nds >= largest isn, ncs >= number of  *
*  different ign, except ncs >= largest ign if any KAMCG this modality.*
*                                                                      *
*  NOTE:  Current program logic does not require a separate pnr array  *
*  for each modality (could be a global array of mxnds longs), but the *
*  separate arrays we have here may be useful for future statistics.   *
*                                                                      *
*  V8A, 04/08/95, GNR - Initial version                                *
*  Rev, 02/16/97, GNR - Add ntrs, ntcs, MDLTSTAT,                      *
*  V8B, 01/20/01, GNR - New memory management routines, trial timers   *
*  Rev, 08/11/01, GNR - Use text cache to remove mdltname length limit *
*  ==>, 11/30/03, GNR - Last mod before committing to svn repository   *
*  V8F, 05/05/10, GNR - Additions for G (response by stim group) stats *
*  V8H, 02/24/11, GNR - Add KAM=C for categorization models            *
***********************************************************************/

#ifndef GSTYPE
#define GSTYPE void
#endif

#define MAX_MODALITIES 16     /* Maximum number of modalities allowed.
                              *  Count 1 for each window through which
                              *  viewed.  N.B.  If this value is made
                              *  >16, the typedef mdlt_type in d3global
                              *  must be changed accordingly.  */
#define qMXMDLT "16"          /* MAX_MODALITIES for messages */

/* Modality statistics */
struct MDLTSTAT {
   long ntract;               /* Number of trials modality is active */
   long mxisn;                /* Highest isn encountered */
   };

struct MODALITY {
   struct MODALITY *pnmdlt;   /* Ptr to next MODALITY block */
   GSTYPE *pgsd;              /* Ptr to ncs*ngpnt GSDEF structs */
   long *pnr;                 /* Ptr to number of responses by isn */
   short *penvl;              /* Ptr to environmental value array */
   stim_mask_t *pxcyc;        /* Bit array of stim in cycle (host) */
   stim_mask_t *pxxxx;        /* Bit array of stim seen in all cycles */
   stim_mask_t *pignc;        /* Bit array for detecting ign changes */
   struct GRPDEF *pgpno;      /* Ptr to GRPNO tables (host only) */
   WINDOW *pmdwdw;            /* Ptr to linked window (host only) */
   union MOu1 {               /* Shared space for d3grp3, cycle time */
      struct GRPDEF **ppgpno;    /* For building pgpno chain */
      struct MDLTSTAT MS;        /* A few statistics */
      } um1;
   txtid hmdltname;           /* Locator for name of this modality */
   ui16  mdltboff;            /* Bit offset in sense bdcst pmbits */
   ui16  mdltgoff;            /* Bit offset of ign bits in pmbits */
   ui16  mdltnoff;            /* Bit offset of new ign  in pmbits */
   short mdltvalu;            /* Highest trial stimulus value (S8) */
   short mdltwdw;             /* Number of window through which
                              *  modality is viewed, 0 if no window */
   ui16  ncs;                 /* Number of classes of stimuli */
   ui16  nds;                 /* Number of distinct stimuli */
   ui16  ndsbyt;              /* Number of bytes to hold nds bits */
   short ngpnt;               /* Number of group number sets loaded,
                              *  = -1 if pgpno space not allocated.  */
   ui16  ntcs;                /* Number of cycles to stimulus cutoff */
   ui16  ntrs;                /* Number of trials to stimulus cutoff */
   byte mdltflgs;             /* Flags */
/* Values mdltflgs may take on: */
#define MDLTFLGS_USED   1        /* This modality is input to some cell
                                 *  type with XYZHCG stats, or ENVVAL
                                 *  scheme, and hence its stim numbers
                                 *  are included in sense broadcast. */
#define MDLTFLGS_RQST   2        /* XYZHCG stats will be done for this
                                 *  modality for any CELLTYPE connected
                                 *  thereto that requests them.  */
#define MDLTFLGS_VALU   4        /* An ENVVAL is attached */
#define MDLTFLGS_KAMC   8        /* This modality is input to some cell
                                 *  type with KAM=C, hence its ign are
                                 *  included in sense broadcast.  */
#define MDLTFLGS_CTFS  16        /* Some celltype does an rbar,qbar
                                 *  fast start when a new ign is
                                 *  presented to this modality.  */
   byte mdltno;               /* Number of this modality, from 1 */
   } ;

