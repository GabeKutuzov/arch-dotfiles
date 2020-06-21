/* (c) Copyright 1991-2011 Neurosciences Research Foundation, Inc. */
/* $Id: simdata.h 48 2012-04-23 18:40:15Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                         SIMDATA Header File                          *
*                                                                      *
*        Define items to be saved in SIMDATA and CLIENT files          *
*                                                                      *
*  Rev, 05/10/91, Initial version                                      *
*  Rev, 03/07/92, GNR - Remove obsolete A,D,H,P and averaging options  *
*  V6D, 02/09/94, GNR - Add D option (store delays)                    *
*  V8A, 05/23/97, GNR - Add A option (store afference, A(k))           *
*  Rev, 08/25/97, GNR - Define GDSELECT block, add I option (save IA), *
*                       N (internal cycles), M (Mij), S (Sbar), U      *
*                       (connections used)                             *
*  Rev, 08/17/00, GNR - Add P option (store paired-pulse facilitation) *
*  V8B, 02/10/01, GNR - Rename GRAFDATA->SIMDATA, GDSELECT->SDSELECT   *
*  Rev, 12/27/07, GNR - Move T,J,W to high order, add Q                *
*  V8D, 01/31/08, GNR - Add SAVE SIMDATA SENSE                         *
*  ==>, 02/01/08, GNR - Last mod before committing to svn repository   *
*  V8E, 01/31/09, GNR - Add codes V and Z for Izhikevich neurons       *
*  V8G, 08/28/10, GNR - Add code X for autoscale multipliers           *
*  V8H, 11/08/10, GNR - Add SAVE SNSID list                            *
*  Rev, 05/06/11, GNR - Use 'R' for arm, 'J' for Sj, split d3gfsv()    *
***********************************************************************/

#ifndef CLTYPE
#define CLTYPE void
#endif
#ifndef TVTYPE
#define TVTYPE void
#endif

#define EndMark     (-999999)    /* Official SIMDATA end-of-file */

/* Struct for coding sense data to be saved */
struct GDSNS {
   struct GDSNS *pngsns;      /* Ptr to next GDSNS block */
   struct VCELL *pvsns;       /* Ptr to sense to be saved */
   char   vcname[LSNAME];     /* Built-in or protocol name */
   txtid  hgsnm;              /* Name locator or built-in index */
   };

/* Controls for SIMDATA/CLIENT data selection */
struct SDSELECT {
   CLTYPE *pgdcl;             /* Ptr to cell list */
   struct GDSNS *pgdsns[2];   /* Ptrs to sense, sense-id lists */
#define PGD_SENSE 0              /* Index of sense list ptr */
#define PGD_SNSID 1              /* Index of sense-id list ptr */
   TVTYPE *pgdcam;            /* Ptr to upgm camera */
   ilst   *pilobj;            /* Ptr to iter list for objects */
   ilst   *pilval;            /* Ptr to iter list for values */
   ui32   svitms;             /* Items requested to be saved */
   ui32   suitms;             /* Items actually to be saved */
   ui32   gdsoff;             /* Offset to previous segment hdr */
   short  clid;               /* Cell list id--negative if local */
   short  gdfile;             /* Index to file type in d3fdef.h */
   ui16   gdseg;              /* Segment number being written */
   txtid  hcamnm;             /* Camera name handle (0 if none) */
   short  ngdo;               /* Number of objects in pilobj */
   short  ngdv;               /* Number of values  in pilval */
   };

/* Values that svitms can take on.  N.B.  Items relating to cell
*  and connection types are kept in the low-order bits.  These
*  are copied from the global svitms to il->svitms and ix->svitms
*  when those blocks are created, so control cards can replace or
*  just modify them.  Items without letter codes are set internally,
*  not from control cards.  */
#define GFABCD      0x0001       /* V Izhikevich a,b,c,d (seg 0) */
#define GFSBAR      0x0002       /* S Time-averaged s(i) */
#define GFQBAR      0x0004       /* Q Slower time-averaged s(i) */
#define GFPHI       0x0008       /* F Phase angles */
#define GFDIS       0x0010       /* O Phase distributions */
#define GFLIJ       0x0020       /* L Lij's (seg 0) */
#define GFCIJ       0x0040       /* C Cij's */
#define GFCIJ0      0x0080       /* B Cij0's */
#define GFDIJ       0x0100       /* D Dij's */
#define GFSJ        0x0200       /* J Sj's */
#define GFMIJ       0x0400       /* M Mij's */
#define GFPPF       0x0800       /* P PPFij's */
#define GFAFF       0x1000       /* A Afferences */
#define GFNUK       0x2000       /* U Number of connections used */
#define GFIFUW      0x4000       /* Z Izhikevich 'u' variable */
#define GFASM       0x8000       /* X Autoscale multiplier */
#define GFSI    0x00010000       /* Cell response, s(i):
                                 *     In svitms, means has a list.
                                 *     In suitms, list or GFGLBL */
#define GFGLBL  0x00020000       /* Global--Output all cells */
#define GFD1T   0x00080000       /* T Tips = last joint on each arm */
#define GFD1R   0x00100000       /* R Arm joint positions */
#define GFD1W   0x00200000       /* W Windows, those that have moved */
#define GFEFF   0x00400000       /* E Efference (changes array) */
#define GFIA    0x00800000       /* I Input array */
#define GFNTC   0x01000000       /* N INternal cycles */
#define GFVAL   0x04000000       /* Value responses */
#define GFOXY   0x08000000       /* Object coordinates */
#define GFOID   0x10000000       /* Object identifiers */
#define GFSNS   0x20000000       /* Sense data */
#define GFSID   0x40000000       /* External sense id data */
#define GFUTV   0x80000000       /* User camera stimulus info */

