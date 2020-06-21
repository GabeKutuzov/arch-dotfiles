/* (c) Copyright 1991-2011, Neurosciences Research Foundation, Inc. */
/* $Id: lijarg.h 46 2011-05-20 20:14:09Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                         LIJARG Header File                           *
*                                                                      *
*  Define structures used to pass arguments to Lij,Cij,Sj generation   *
*  routines and to read CONNLIST (external Lij) files.  Some values    *
*  are passed from first connection generating routine to subsequent   *
*  connection generating routines.  No LIJARG variables are exchanged  *
*  between host or processing nodes.                                   *
*                                                                      *
*  V5E, 07/22/92, GNR - Add cellnum, jct, ECLREC struct for CONNLIST   *
*  Rev, 03/02/93, GNR - Add support for subarbors                      *
*  V8A, 04/27/96, GNR - Add group[nxy]                                 *
*  V8D, 08/06/06, GNR - Extensive rewrite                              *
*  ==>, 01/25/07, GNR - Last mod before committing to svn repository   *
*  Rev, 06/24/08, GNR - Add jsyn, remove nskip, mskip, lsas            *
*  V8F, 06/04/10, GNR - KGNE2 with Cij, KGNEX without Cij              *
*  V8H, 10/30/10, GNR - Add xn (once per conntype) variable allocation *
*  Rev, 05/25/11, GNR - Add globals for initsi                         *
***********************************************************************/

/* Values used to define stage of CNS processing.
*  Note:  KMEM_GO is defined as 0 to optimize some tests */
#define KMEM_GO      0  /* Running, pick up or generate per OPTMEM */
#define KMEM_GENR    1  /* First call, generate all Lij */
#define KMEM_REGEN   2  /* Regenerating, compute unless external */

   struct LIJARG {

/*---------------------------------------------------------------------*
*                      Items generic to all uses                       *
*                                                                      *
*  N.B.  Code assumes Lij, Cij, etc. generation will always occur for  *
*  same celltype and cell at any one time.  psyn is set by d3kijx()    *
*  but is to be incremented by the caller once per synapse loop        *
*  regardless of which variables are used.                             *
*---------------------------------------------------------------------*/

      struct CELLTYPE *kl;    /* Ptr to governing cell layer */
      rd_type *pxnd;          /* Ptr to per-conntype data */
      rd_type *psyn;          /* Ptr to synapse data */
      s_type  *psj;           /* Ptr to sj and phase */
      long isyn;              /* Synapse index (counts up from 0) */
      long jsyn;              /* True synapse number (after skips) */
      long kcell;             /* Current cell number */
      long lcoff;             /* Offset to prd data for this cell */
      long nuk;               /* Number of connections actually used */
      int  kcall;             /* One of the modes defined above */
      int  lijrc;             /* Return code from lijbr() routine */
      int  nLij;              /* Counts calls to lijbr() routine
                              *  that successfully generated an Lij */
      int  nDij;              /* Counts calls to dijbr() routine */

/*---------------------------------------------------------------------*
*                    Items used for Lij generation                     *
*---------------------------------------------------------------------*/

      /* The following items relate to a particular celltype, cell,
      *  group, and connection type.  They are set by d3kiji() or
      *  d3lijx() and are not changed by other routines.  */
      long groupn;            /* Number of current group */
      long groupx;            /* X coord of current group */
      long groupy;            /* Y coord of current group */
      long grouptop;          /* Number of first cell in next group */
      long nskipr;            /* Number skipped conns remaining */
      long ngx;               /* Groups per row in region */
      long satest;            /* Self-avoidance cell test */
      int  jctype;            /* Connection type (external) */
      int  new_group;         /* TRUE if entering a new cell group */

      /* The following items relate to a particular connection.
      *  They may be set and changed by d3lijx() or by particular
      *  generating routines.  They are not expected to be
      *  preserved when going to a new connection type.  */
      int (*lijbr)(struct CONNTYPE *ix);
                              /* Ptr to current generating routine */
      rd_type *pskpl;         /* Ptr to skipped synapse list */
      long fstx;              /* X coord of first target */
      long fsty;              /* Y coord of first target */
      long fstr;              /* Cell number of first source */
      long isas,nsas;         /* Current,next subarbor start */
      long jsa;               /* Subarbor row count */
      long lb2s;              /* lb2 subarbor reset point */
      long lijval;            /* Lij value returned */
      long rowct;             /* Working row count (KGNDG) */
      long rowx;              /* X coord at start of row (KGNDG) */
      long rowy;              /* Y coord at start of row (KGNDG) */
      long skipndx;           /* Index of next cell skipped */
      long svlseed;           /* Saved Lij seed for subarb repeat */
      long wlseed;            /* Working Lij seed */
      long xcur,ycur;         /* Current x,y coords (KGNDG,KGNBL) */
      long rowe,boxe;         /* Row, horiz. box ends (KGNBL,KGNPT) */

/*---------------------------------------------------------------------*
*                    Items used for Cij generation                     *
*                                                                      *
*  Some of these variables are duplicates of variables in the Lij      *
*  section above, so that either or both sets of generating calls      *
*  can be interleaved without double counting.                         *
*---------------------------------------------------------------------*/

      /* The following items relate to a particular celltype, cell,
      *  and connection type.  They are set by d3cijx() and not
      *  changed by other routines.  */
      long *curmptr;          /* Current matrix pointer */
      long bigc24;            /* Max coeff (S24) */
      ui32 bigc31;            /* Max |coeff| (S31) */
      long rndcon;            /* Rounding constant */
      int  nbcm2;             /* nbc - 2 */

      /* The following items relate to a particular connection */
      rd_type *pskpc;         /* Ptr to skipped synapse list */
      long csas,csai;         /* Cij subarbor reset point */
      long curcmat;           /* Current coefficient matrix entry */
      long lwcseed;           /* Local copy of CONNTYPE var wcseed */
      long msyn;              /* Synapse number (matrix method) */
      si32 cijsgn;            /* Cij value with original sign */
      si32 cijval;            /* Cij value with -0 set to +0 */
      si16 cijext;            /* External Cij left by d3elij */
      si16 matcnt;            /* Cij matrix count */

/*---------------------------------------------------------------------*
*                    Items used for Dij generation                     *
*---------------------------------------------------------------------*/

      /* The following items relate to a particular celltype, cell,
      *  and connection type.  They are set by d3dijx() and not
      *  changed by other routines.  */
      long wdmn;              /* Mean delay plus roundoff */

      /* The following items relate to a particular connection */
      long wdseed;            /* Working delay seed */
      ui32 dijval;            /* Dij value */

/*---------------------------------------------------------------------*
*                      Items used for Sj lookup                        *
*---------------------------------------------------------------------*/

      long wpseed;            /* Local copy of working phseed */
      long sjraw;             /* Raw Sj (before color remapping)--
                              *  (not stored if repertoire input) */
      long sjval;             /* Sj value (long for d3gts2 macro) */
      int  ics3;              /* Color selection for EXTR3C option */

/*---------------------------------------------------------------------*
*          Globals for initsi calls (constant across cells)            *
*---------------------------------------------------------------------*/

      void (*resetx)(struct CELLTYPE *il, ui32 icell);
      int  irow1;             /* First row for ni1 */
      int  irow2;             /* First row for ni2 */
      int  Noisy;             /* TRUE to generate noise */
      int  Phsupd;            /* Method of phase generation */
      };

/*---------------------------------------------------------------------*
*               External connection list record format                 *
*  Note:  Because connection type numbers and Cij are both restricted  *
*  to not more than 16 bits, they are packed into one word of ECLREC.  *
*  When external input is KGNE2, Cij is read from this record.  If     *
*  external input is KGNEX, Cij is generated, ecij field is ignored.   *
*  By definition, all fields here are big-endian.  Hence the addition  *
*  of ecij in the high-order of jct does not affect jct in old files.  *
*---------------------------------------------------------------------*/

   struct ECLREC {
      si32 xjcell;            /* Source cell number */
      si32 xicell;            /* Target cell number */
      si16 ecij;              /* External Cij value */
      ui16 jct;               /* Relative connection type number */
      };

