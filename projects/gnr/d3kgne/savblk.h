/* (c) Copyright 1991-2010, The Neurosciences Research Foundation */
/* $Id: savblk.h 48 2012-04-23 18:40:15Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                              savblk.h                                *
*              Header data definitions for SAVENET file                *
*                                                                      *
*  Unfortunately, the C language does not provide compile-time access  *
*  to types and scales of data items, therefore, the SFHKey info in    *
*  this file must be maintained by hand.  Note that each header is     *
*  defined in the file-message format described in swap.h.  Header     *
*  data items and their descriptions are stored as unaligned character *
*  strings to give a common medium for exchange between unlike CPUs.   *
*                                                                      *
*  Note that when it comes to writing the actual network data, each    *
*  supported CPU will write the data as a direct binary image of the   *
*  data as they exist in memory.  Data are kept in a standard endian-  *
*  order (D3MEMACC_BYTE_ORDER) on all CPUs so swapping is not needed,  *
*  but paddding may have to be added or removed when the data are read *
*  back in.  This may be changed in future versions so data are native *
*  in memory and big-endian in SAVENET files.                          *
*                                                                      *
*  Previous save file versions were as follows:                        *
*  Vers. 1 -- Translation of IBM definition to C                       *
*  Vers. 2 -- Moved headers to front, added info for conntype skipping *
*  Vers. 3 -- All data to be little-endian, 3-byte Lij order fixed     *
*  Vers. 4 -- Adds lspdt to header, s(i) array stored with all delays  *
*  Vers. 8 -- Self-descriptive header items, back to big-endian        *
*                                                                      *
*  Written, 12/14/90, MC                                               *
*  V5C, 09/24/91,  MC - New header formats for combining savenet files *
*  Rev, 02/04/92, GNR - Add header format for Darwin 1 blocks          *
*  Rev, 09/29/92, GNR - Advance to version 3                           *
*  V6D, 02/05/94, GNR - Version 4, add axonal delays, snseed           *
*  V7B, 07/20/94, GNR - Add rsd to celltype header                     *
*  V8A, 09/28/97, GNR - Version 8 SAVENET file with self-descriptive   *
*                       header data, independent allocation for each   *
*                       conntype, add SFREPHDR, GC & MOD phase seeds.  *
*  Rev, 11/24/00, GNR - Separate RADIUS parameter, exp() hand vision   *
*  V8C, 02/22/03, GNR - Cell responses in millivolts, add conductances *
*  V8D, 09/30/07, GNR - Add Rij = <Sj>                                 *
*  ==>, 11/26/07, GNR - Last mod before committing to svn repository   *
*  V8G, 09/04/10, GNR - Add mechanism to save autoscales               *
*  V8H, 10/20/10, GNR - Add xnoff,lxn, allow earlier w/nuklen == lxn   *
***********************************************************************/

#include "swap.h"

/* Codes for expressing time-mode of save file in calls to d3save(): */
#define D3SV_ENDXEQ  0           /* Save at end of execution */
#define D3SV_FMGRP3  1           /* Save once--Group III card */
#define D3SV_NEWSER  2           /* Save at start of trial series */
#define D3SV_ALCFN   4           /* Added length of created filename */

#define SF_VERS      8           /* Save file version */
#define SF_IDENT "SAVENET V8H "  /* Save file identification */
#define SFVERS_LEN  12           /* Length of SF_IDENT field */

#define SFTITLE_LEN 60
#define SFDATE_LEN  12
#define SFKEY_LEN    4

#define SFTPM   0x0f             /* Mask for type bits */
#define SFGOT   0x20             /* Set when field got from input */
#define SFREQ   0x40             /* This field is always required */
#define SFMCK   0xc0             /* Set to request match checking */

/* Typedefs for shorts, longs, floats, doubles defined as chars */
typedef char cshort[FMSSIZE];
typedef char clong[FMLSIZE];
typedef char cfloat[FMESIZE];
typedef char cdble[FMDSIZE];

typedef struct {              /* Header block descriptor */
   char  blkname[SFKEY_LEN];     /* Code name for the block */
   clong blklen;                 /* Length of the block */
   } SFHBlk;

/* N.B.  This structure must contain no items that require alignment */
typedef struct {              /* Header-item descriptors */
   char  key[SFKEY_LEN];         /* Code name for the item */
   byte  type;                   /* Type code--see defs above */
   byte  fsclidim;               /* In file: binary scale if [F|U]TYPE,
                                 *  On input: dimension of data read */
   byte  len;                    /* Length of one element */
   byte  dim;                    /* Dimension */
   } SFHKey;

/* Various kinds of variables with their descriptors */
typedef struct { SFHKey k; cshort d; } kdshort;
typedef struct { SFHKey k; clong  d; } kdlong;
typedef struct { SFHKey k; cfloat d; } kdfloat;
typedef struct { SFHKey k; cdble  d; } kddble;
typedef struct { SFHKey k; char d[LSNAME]; } kdname;

/*=====================================================================*
*  For each of the following header declarations, a single static      *
*  prototype is defined with everything but the variable data values.  *
*  This is copied into dynamic storage and filled in when a SAVENET    *
*  file is written or restored.                                        *
*----------------------------------------------------------------------*
*  The idea of the SFMCK bit is that it marks fields that must match   *
*  after a header block has been chosen for use based on name matches. *
*  (These fields are of course required and so also have SFREQ set.)   *
*  There could be a routine for each type of header that would gene-   *
*  rate that header from current data.  This routine would be called   *
*  in place of the in-line code in d3save and again in d3fchk to make  *
*  a current prototype.  All items with the SFMCK bit set could then   *
*  be tested in a single routine common to all header types.  This     *
*  scheme has not currently been implemented.                          *
*=====================================================================*/

/* File identification generic to all CNS files.
*  This info does not use SFHBlk or SFHKey descriptors.  */
struct SFHDR {
   char filetype[SFVERS_LEN];    /* "SAVENET <Vers>" goes here */
   char title[SFTITLE_LEN];      /* Title from creating run */
   char timestamp[SFDATE_LEN];   /* Time date stamp (yymmddhhmmss) */
   char stitle[LSTITL];          /* Series and trial when written */
   };

/* Overall Header */
struct SFHDR2 {
   SFHBlk sfhdblk;
   kdshort sfhnd1b;              /* Number of Darwin 1 blocks */
   kdshort sfhnrep;              /* Number of repertoires in file */
   kdlong  sfhsnseed;            /* Stimulus noise seed */
   };

#ifdef MAIN
struct SFHDR2 ursfhdr2 = {
   {  "HDR2"  },
   {{ "ND1B", UTYPE|SFREQ, 0, FMSSIZE, 1 }},
   {{ "NREP", UTYPE|SFREQ, 0, FMSSIZE, 1 }},
   {{ "SNSD", UTYPE,       0, FMLSIZE, 1 }},
   };
#else
extern struct SFHDR2 ursfhdr2;
#endif

#ifdef D1
/* Darwin 1 header */
struct SFD1HDR {
   SFHBlk sfd1blk;
   kdlong  sfd1nepr;             /*    pd1->nepr   */
   kdshort sfd1nd1r;             /*    pd1->nd1r   */
   kdshort sfd1nbpe;             /*    pd1->nbpe   */
   kdlong  sfd1seed;             /*    pd1->d1seed */
   kdlong  offd1s;               /*    offset of pd1s data in file */
   kdlong  offd1r;               /*    offset of pd1r data in file */
   };

#ifdef MAIN
struct SFD1HDR ursfd1hdr = {
   {  "D1HD"  },
   {{ "NEPR", UTYPE|SFREQ, 0, FMLSIZE, 1 }},
   {{ "ND1R", UTYPE|SFREQ, 0, FMSSIZE, 1 }},
   {{ "NBPE", UTYPE|SFREQ, 0, FMSSIZE, 1 }},
   {{ "D1SD", UTYPE,       0, FMLSIZE, 1 }},
   {{ "OD1S", FTYPE|SFREQ, 0, FMLSIZE, 1 }},
   {{ "OD1R", FTYPE|SFREQ, 0, FMLSIZE, 1 }},
   };
#else
extern struct SFD1HDR ursfd1hdr;
#endif
#endif

/* Repertoire header */
struct SFREPHDR {
   SFHBlk sfrepblk;
   kdname  repnam;               /*    ir->sname     */
   kdlong  sfrngx;               /*    ir->ngx       */
   kdlong  sfrngy;               /*    ir->ngy       */
   struct {
      SFHKey k;
      char d[GPNDV][FMSSIZE];
      } sfgpoff;                 /*    ir->gpoff     */
   kdshort sfnlyr;               /*    ir->nlyr      */
   };

#ifdef MAIN
struct SFREPHDR ursfrephdr = {
   {  "RPHD"  },
   {{ "RNAM", ATYPE|SFREQ, 0, LSNAME,  1 }},
   {{ "NGX ", UTYPE|SFMCK, 0, FMLSIZE, 1 }},
   {{ "NGY ", UTYPE|SFMCK, 0, FMLSIZE, 1 }},
   {{ "GPOF", UTYPE|SFMCK, 0, FMSSIZE, GPNDV }},
   {{ "NLYR", UTYPE|SFREQ, 0, FMSSIZE, 1 }},
   };
#else
extern struct SFREPHDR ursfrephdr;
#endif

/* Celltype header */
struct SFCTHDR {
   SFHBlk sfctblk;
   kdname  celnam;               /*    il->lname     */
   kdlong  sfctnel;              /*    il->nel       */
   kdlong  sfctlel;              /*    il->lel       */
   kdshort sfctls;               /*    il->ls        */
   kdshort sfctphshft;           /*    il->phshft    */
   kdshort sfctsdly1;            /*    il->mxsdelay1 */
   kdshort sfctnizv;             /*    il->nizvar | I3IVRBIT */
#define I3IVRBIT 0x100
   kdshort sfctnct;              /*    il->nct       */
   kdshort sfctngct;             /*    il->ngct      */
   kdshort sfctnmct;             /*    il->nmct      */
   struct {
      SFHKey k;
      char d[CTNDV][FMSSIZE];
      } sfctoff;                 /*    il->ctoff     */
   kdlong  sfctasm;              /*    autoscale     */
   kdlong  sfctnsd;              /*    il->nsd       */
   kdlong  sfctrsd;              /*    il->rsd       */
   kdlong  sfctpsd;              /*    il->pctpd->phiseed         */
   kdlong  sfctzsd;              /*    il->prfi->izseed           */
   kdlong  offsi;                /*    offset of si data in file  */
   kdlong  offprd;               /*    offset of prd data in file */
   };

#ifdef MAIN
struct SFCTHDR ursfcthdr = {
   {  "CTHD"  },
   {{ "LNAM", ATYPE|SFREQ, 0, LSNAME,  1 }},
   {{ "NEL ", UTYPE|SFMCK, 0, FMLSIZE, 1 }},
   {{ "LEL ", UTYPE|SFREQ, 0, FMLSIZE, 1 }},
   {{ "LS  ", UTYPE|SFREQ, 0, FMSSIZE, 1 }},
   {{ "PHSH", UTYPE|SFREQ, 0, FMSSIZE, 1 }},
   {{ "MXDL", UTYPE|SFREQ, 0, FMSSIZE, 1 }},
   {{ "NIZV", UTYPE,       0, FMSSIZE, 1 }},
   {{ "NCT ", UTYPE|SFREQ, 0, FMSSIZE, 1 }},
   {{ "NGCT", UTYPE|SFREQ, 0, FMSSIZE, 1 }},
   {{ "NMCT", UTYPE|SFREQ, 0, FMSSIZE, 1 }},
   {{ "CTOF", FTYPE|SFMCK, 0, FMSSIZE, CTNDV }},
   {{ "AUTS", FTYPE,      24, FMLSIZE, 1 }},
   {{ "NSD ", UTYPE,       0, FMLSIZE, 1 }},
   {{ "RSD ", UTYPE,       0, FMLSIZE, 1 }},
   {{ "PHSD", UTYPE,       0, FMLSIZE, 1 }},
   {{ "IZSD", FTYPE,       0, FMLSIZE, 1 }},
   {{ "OSI ", FTYPE|SFREQ, 0, FMLSIZE, 1 }},
   {{ "OPRD", FTYPE|SFREQ, 0, FMLSIZE, 1 }},
   };
#else
extern struct SFCTHDR ursfcthdr;
#endif

/* Specific connection type header */
struct SFCNHDR {
   SFHBlk sfcnblk;
   kdname  srcsnam;              /*    ix->srcnm.rnm */
   kdname  srclnam;              /*    ix->srcnm.lnm */
   kdlong  sfcnnc;               /*    ix->nc        */
   kdshort sfcnlc;               /*    ix->lc        */
   kdshort sfcnlxn;              /*    ix->lxn       */
   kdshort sfcnnbc;              /*    ix->nbc       */
   kdlong  sfcnkgen;             /*    ix->kgen      */
   struct {
      SFHKey k;
      char d[CNNDV][FMSSIZE];
      } sfcnoff;                 /*    ix->cnoff     */
   struct {
      SFHKey k;
      char d[XNNDV][FMSSIZE];
      } sfxnoff;                 /*    ix->xnoff     */
   kdlong  sfcnsrcngx;           /*    ix->srcngx    */
   kdlong  sfcnsrcngy;           /*    ix->srcngy    */
   kdlong  sfcnsrcnel;           /*    ix->srcnel    */
   kdlong  sfcnsrcnelt;          /*    ix->srcnelt   */
   kdlong  sfcnloff;             /*    ix->loff      */
   kdlong  sfcnlsg;              /*    ix->lsg       */
   kdlong  sfcnnrx;              /*    ix->nrx       */
   kdlong  sfcnnry;              /*    ix->nry       */
   kdshort sfcnnhx;              /*    ix->nhx       */
   kdshort sfcnnhy;              /*    ix->nhy       */
   kdshort sfcnnvx;              /*    ix->nvx       */
   kdshort sfcnnvy;              /*    ix->nvy       */
   kdlong  sfcnnsa;              /*    ix->nsa       */
   kdlong  sfcnnsax;             /*    ix->nsax      */
   kdlong  sfcnnux;              /*    ix->nux       */
   kdlong  sfcnnuy;              /*    ix->nuy       */
   kdlong  sfcnradius;           /*    ix->radius    */
   kdlong  sfcnstride;           /*    ix->stride    */
   kdlong  sfcnxoff;             /*    ix->xoff      */
   kdlong  sfcnyoff;             /*    ix->yoff      */
   kdlong  sfcncseed;            /*    ix->cseed     */
   kdlong  sfcnlseed;            /*    ix->lseed     */
   kdlong  sfcnpseed;            /*    ix->phseed    */
   kdlong  sfcnrseed;            /*    ix->rseed     */
   kdlong  offprd;               /*    offset to rep data in file */
   kdshort sfcncbc;              /*    ix->cbc       */
   };

#ifdef MAIN
struct SFCNHDR ursfcnhdr = {
   {  "CNHD"  },
   {{ "SRID", ATYPE|SFREQ, 0, LSNAME,  1 }},
   {{ "SLID", ATYPE|SFREQ, 0, LSNAME,  1 }},
   {{ "NC  ", UTYPE|SFMCK, 0, FMLSIZE, 1 }},
   {{ "LC  ", UTYPE|SFMCK, 0, FMSSIZE, 1 }},
   {{ "LXN ", UTYPE|SFMCK, 0, FMSSIZE, 1 }},
   {{ "NBC ", UTYPE|SFMCK, 0, FMSSIZE, 1 }},
   {{ "KGEN", UTYPE|SFMCK, 0, FMLSIZE, 1 }},
   {{ "CNOF", FTYPE|SFREQ, 0, FMSSIZE, CNNDV }},
   {{ "XNOF", FTYPE,       0, FMSSIZE, XNNDV }},
   {{ "SNGX", UTYPE|SFMCK, 0, FMLSIZE, 1 }},
   {{ "SNGY", UTYPE|SFMCK, 0, FMLSIZE, 1 }},
   {{ "SNEL", UTYPE|SFMCK, 0, FMLSIZE, 1 }},
   {{ "SNLT", UTYPE|SFMCK, 0, FMLSIZE, 1 }},
   {{ "LOFF", FTYPE|SFMCK, 0, FMLSIZE, 1 }},
   {{ "LSG ", UTYPE|SFMCK, 0, FMLSIZE, 1 }},
   {{ "NRX ", UTYPE|SFMCK, 0, FMLSIZE, 1 }},
   {{ "NRY ", UTYPE|SFMCK, 0, FMLSIZE, 1 }},
   {{ "NHX ", FTYPE|SFMCK, 0, FMSSIZE, 1 }},
   {{ "NHY ", FTYPE|SFMCK, 0, FMSSIZE, 1 }},
   {{ "NVX ", FTYPE|SFMCK, 0, FMSSIZE, 1 }},
   {{ "NVY ", FTYPE|SFMCK, 0, FMSSIZE, 1 }},
   {{ "NSA ", UTYPE|SFMCK, 0, FMLSIZE, 1 }},
   {{ "NSAX", UTYPE|SFMCK, 0, FMLSIZE, 1 }},
   {{ "NUX ", UTYPE|SFMCK, 0, FMLSIZE, 1 }},
   {{ "NUY ", UTYPE|SFMCK, 0, FMLSIZE, 1 }},
   {{ "RAD ", UTYPE,       0, FMLSIZE, 1 }},
   {{ "STRI", FTYPE|SFMCK, 0, FMLSIZE, 1 }},
   {{ "XOFF", FTYPE|SFMCK, 0, FMLSIZE, 1 }},
   {{ "YOFF", FTYPE|SFMCK, 0, FMLSIZE, 1 }},
   {{ "CSD ", UTYPE,       0, FMLSIZE, 1 }},
   {{ "LSD ", UTYPE,       0, FMLSIZE, 1 }},
   {{ "SPSD", UTYPE,       0, FMLSIZE, 1 }},
   {{ "RSD ", UTYPE,       0, FMLSIZE, 1 }},
   {{ "OSYN", FTYPE|SFREQ, 0, FMLSIZE, 1 }},
   {{ "CBC ", UTYPE|SFMCK, 0, FMSSIZE, 1 }},
   };
#else
extern struct SFCNHDR ursfcnhdr;
#endif

/* Geometric connection type header.  (No actual data are
*  stored with this header at the present time.)  */
struct SFCGHDR {
   SFHBlk sfcgblk;
   kdname  gsrcrid;              /*    ib->gsrcnm.rnm */
   kdname  gsrclid;              /*    ib->gsrcnm.lnm */
   kdshort sfcgngb;              /*    ib->ngb        */
   kdshort sfcgnib;              /*    ib->nib        */
   };

#ifdef MAIN
struct SFCGHDR ursfcghdr = {
   {  "CGHD"  },
   {{ "GRID", ATYPE|SFREQ, 0, LSNAME,  1 }},
   {{ "GLID", ATYPE|SFREQ, 0, LSNAME,  1 }},
   {{ "NGB ", UTYPE|SFMCK, 0, FMSSIZE, 1 }},
   {{ "NIB ", UTYPE|SFMCK, 0, FMSSIZE, 1 }},
   };
#else
extern struct SFCGHDR ursfcghdr;
#endif

/* Modulatory connection type header.  (No actual data are
*  stored with this header at the present time.)  */
struct SFCMHDR {
   SFHBlk sfcmblk;
   kdname  msrcrid;              /*    im->msrcnm.rnm */
   kdname  msrclid;              /*    im->msrcnm.lnm */
   };

#ifdef MAIN
struct SFCMHDR ursfcmhdr = {
   {  "CMHD"  },
   {{ "MRID", ATYPE|SFREQ, 0, LSNAME,  1 }},
   {{ "MLID", ATYPE|SFREQ, 0, LSNAME,  1 }},
   };
#else
extern struct SFCMHDR ursfcmhdr;
#endif

/* Union used just to allocate space for any of the above */
union SFANYHDR {
   struct SFHDR    sfhdr;
   struct SFHDR2   sfhdr2;
#ifdef D1
   struct SFD1HDR  d1hdr;
#endif
   struct SFREPHDR rephdr;
   struct SFCTHDR  cthdr;
   struct SFCNHDR  cnhdr;
   struct SFCGHDR  cghdr;
   struct SFCMHDR  cmhdr;
   };
