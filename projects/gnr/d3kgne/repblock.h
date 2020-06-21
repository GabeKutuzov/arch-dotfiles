/* (c) Copyright 1991-2010, Neurosciences Research Foundation, Inc. */
/* $Id: repblock.h 29 2010-06-15 22:02:00Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                        REPBLOCK Header File                          *
*               (Note: references D3GLOBAL Header File)                *
*                                                                      *
*  V6D, 02/03/94, GNR - Delete KRPCL,DRPPF, add vxo,vyo                *
*  V7C, 11/12/94, GNR - Add sdist by FDM (KRPFD) and by IGN (KRPSC)    *
*  V8A, 05/03/95, GNR - Control stats at CELLTYPE level, add 'Y','Z'   *
*  Rev, 03/10/97, GNR - Add KRPEP                                      *
*  V8B, 01/27/01, GNR - Add jrptim                                     *
*  V8D, 11/11/06, GNR - Make D,P,T,Q,K options same in rep,cell blocks *
*  Rev, 12/27/07, GNR - Separate KP and KCTP on MASTER, REGION         *
*  ==>, 12/28/07, GNR - Last mod before committing to svn repository   *
*  V8F, 04/15/10, GNR - Move RPDFLT here from misc.h                   *
***********************************************************************/

/*---------------------------------------------------------------------*
*                               RPDFLT                                 *
*  Repertoire/Region defaults that may be entered on MASTER cards.     *
*---------------------------------------------------------------------*/

struct RPDFLT {
   ui16  krp;                 /* Repertoire-level plot options */
#define KRPRR     0x0001         /* 'R' Repertoire print      (KRP3R) */
#define KRPPL     0x0002         /* 'B' Repertoire plot       (KRP3B) */
#define KRPSP     0x0004         /* 'S' Spout output          (KRP2S) */
#define KRPDL     0x0008         /* 'L' Layer division lines  (KRP3L) */
#define KRPAP     0x0010         /* 'A' All series print      (KRP3A) */
#define KRPWA     0x0020         /* 'W' Wallace               (KRP3W) */
#define KRPIL     0x0040         /* 'I' Intermingled layers   (KRP1I) */
#define KRPVT     0x0100         /* 'V' View time-series data (KRP1V) */
#define KRPPG     0x0200         /* 'G' Plot grids            (KRP2G) */
#define KRPOM     0x0400         /* 'O' Omit titles           (KRP1O) */
#define KRPPR     0x0800         /* '2' Plot retrace          (KRP22) */
/* Following bits are not options, but are set in d3news()          */
#define KRPNS     0x2000         /* All celltypes have no stats       */
#define KRPNZ     0x4000         /* My node has nonzero cells         */
#define KRPBP     0x8000         /* Ct.kctp requires bubble plot      */
   short ngridx;              /* Grid interval along x */
   short ngridy;              /* Grid interval along y */
   byte  jrptim;              /* Number of rep print timer */
   byte  jbptim;              /* Number of bubble plot timer */
   };

/*---------------------------------------------------------------------*
*                              REPBLOCK                                *
*---------------------------------------------------------------------*/

struct REPBLOCK {
   struct REPBLOCK *pnrp;     /* Ptr to next REPBLOCK block */
   struct CELLTYPE *play1;    /* Ptr to first CELLTYPE block */
   struct IONTYPE  *pionr;    /* Ptr to first related ion type */
   rd_type         *pgpd;     /* Ptr to group data (IONG) */
   struct RPDFLT   Rp;        /* Parameters inherited from MASTER */
   char sname[LSNAME];        /* Short name of repertoire */
   long lgt;                  /* Len of group data for all groups */
   long lrt;                  /* Len of repertoire - total bytes  */
   long ngrp;                 /* No. of groups in repertoire */
   long ncpg;                 /* No. of cells (all types) per gp */
   long ngx;                  /* No. of groups along X */
   long ngy;                  /* No. of groups along Y */
   long nqy;                  /* Block print Y */
   long nqg;                  /* Lines needed to print 1 group */

   float aplx;                /* Plot X direction */
   float aply;                /* Plot Y direction */
   float aplw;                /* Plot width */
   float aplh;                /* Plot height */
   float vxo;                 /* KP=V plot x offset */
   float vyo;                 /* KP=V plot y offset */
   ui16  andctopt;            /* AND of all ctopt bits (after d3news) */
   short gpoff[GPNDV];        /* Offsets to dynamic group variables */
   short lg;                  /* Len of group lvl data of 1 group */
   short lrn;                 /* Length of long repertoire name */
   short mxnib;               /* Max(nib) over child gconns */
   short nqx;                 /* Block print X */
   txtid hrname;              /* Text locator for long rep name */
   byte  niong;               /* Number of ions at group level */
   byte  nlyr;                /* Number of layers (cell types) */
   };
