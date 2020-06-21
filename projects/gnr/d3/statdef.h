/* (c) Copyright 1990-2016, The Rockefeller University *11116* */
/* $Id: statdef.h 70 2017-01-16 19:27:55Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                         STATDEF Header File                          *
*  Definition of STAT statistics block and structs used for MXSTATS    *
*                                                                      *
************************************************************************
*  Rev, 03/23/90, Initial version                                      *
*  V7C, 11/12/94, GNR - Divide ngr into ncs+nds, add pnr               *
*  Rev, 03/29/95, GNR - Revise GSDEF struct for corrected MPP stats    *
*  V8A, 06/14/95, GNR - Move per-modality items to MODALITY blocks,    *
*                       eliminate GGSDEF blocks                        *
*  Rev, 10/28/00, GNR - Remove XRH, tabulate KRPHR over all cycles     *
*  ==>, 06/28/02, GNR - Last mod before committing to svn repository   *
*  V8E, 11/07/09, GNR - Move nntr to CLSTAT.nstcap (may differ per il) *
*  V8H, 08/18/12, GNR - Add ncas to GSDEF                              *
***********************************************************************/

/* Macro to return number of stimulus groupings in a modality.
*  There is always at least 1, even if no GRPNO cards entered.  */
#define NStimGrps(pmdlt) ((pmdlt->ngpnt > 0) ? pmdlt->ngpnt : 1)

/* Structure to define a stimulus class (aka group).  There is one
*  of these for each class in each GRPNO in each modality.  */

struct GSDEF {             /* Stimulus group statistics definition */
   byte *grpmsk;           /* Ptr to bit array of length ndsbyt with
                           *  bit set for each stim in jth stim class */
   xstt ng;                /* Total responses to jth class of stimuli
                           *  = sum over all cells of nhtj */
   xstt nhtj;              /* Number of hits to stimuli in class j
                           *  by cell currently being processed.  */
   ui32 nstj;              /* Number of stimuli in class j */
   ui32 ncas;              /* No. cells responding to any stim in j */
   } ;

/* Global structure for cross-response statistics.  It is important
*  that this structure is not contained in the general RP broadcast,
*  as each node maintains local pointers here that are not handled
*  by the hash table mechanism.  */

struct STDEF {
   ui32 *pxrmtx;              /* Ptr to cross-response matx */
   struct GSDEF *pgsblk;      /* Ptr to block of GSDEF structures */
   byte *pgrpblk;             /* Ptr to group membership info,
                              *  includes pgrpncl, pgrpmsk arrays */
   ui16 *pgrpncl;             /* Ptr to number of classes by GRPNO */
   byte *pgrpmsk;             /* Ptr to group membership masks */
   short ngpntt;              /* Total over modalities of ngpnt */
   short ngsblk;              /* Number of GSDEF blocks allocated */
   long lgrpblk;              /* Length of group membership info */
   };

#ifndef MAIN
extern
#endif
   struct STDEF STAT;

/* Format of pgrpblk group membership broadcast info:
*  (1) ui16 grpncl[ngpntt = total(ngpno)]
*        (Number of classes on GRPNO card indexed by GRPNO card)
*  (2) byte grpmsk[total(ngpno*ncs*ndsbyt)]
*        (One grpmsk array of length ndsbyt for each class occurring
*        on each GRPNO card)--these are the same arrays pointed to
*        by grpmsk in the GSDEF structures above.  */

