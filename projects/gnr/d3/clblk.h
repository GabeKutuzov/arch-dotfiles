/* (c) Copyright 1990-2018, The Rockefeller University *11116* */
/* $Id: clblk.h 77 2018-03-15 21:08:14Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               clblk.h                                *
*   Definition of DARWIN III Cell List and Cell List Header Blocks     *
*                                                                      *
*  Notes:  The CLBLK clnm and clseq fields replicate info in the       *
*  CELLTYPE block, but clnm may be needed to identify a CLBLK before   *
*  it has been matched to its CELLTYPE.  clseq was used for sorting,   *
*  (currently not needed).  (clnm.rnm,clnm.lnm) are coded as 0 for     *
*  anonymous cell lists (input as '*') and after d3clck, clseq=0xffff. *
*  Use of tricks to save the space for clnm and clseq in node memory   *
*  would no doubt cost more in code than would be saved.               *
*                                                                      *
*  CLHDRs are stored only in host memory.  CLBLKs may be in Host or    *
*  Dynamic memory, depending on use.                                   *
************************************************************************
*  V1A, 04/08/90, GNR - Initial version                                *
*  V8B, 02/18/01, GNR - Complete rewrite, use ROCKS iterators          *
*  Rev, 07/14/01, GNR - Add CLHDR def, findcl() & d3clrm() prototypes  *
*  V8C, 08/31/03, GNR - Use ptr to ilst, not ilst itself, in CLBLK to  *
*                       to avoid bug if ilstread deletes then creates  *
*  ==>, 08/03/07, GNR - Last mod before committing to svn repository   *
*  R67, 11/11/16, GNR - Update comments to reflect anonymous lists     *
*  R72, 02/23/17, GNR - Add CLCK_SIMDAT code, d3clck error return      *
*  R77, 02/17/18, GNR - Add CLCK_LPLT code                             *
***********************************************************************/

#ifndef PARn
struct CLHDR {                /* Header for multi-celltype cell list */
   struct CLHDR    *pnclh;       /* Ptr to next cell list header */
   struct CLBLK    *pfclb;       /* Ptr to first CLBLK in this list */
   short           clnum;        /* Number of this cell list */
   short           clflgs;       /* Flag bits as follows: */
#define CLFL_USED    1           /* This list is in use */
#define CLFL_SORTED  2           /* This list has been sorted */
#define CLFL_SHARED  4           /* Share list in Dynamic pool */
   short           nclb;         /* Number of CLBLKs in list */
   };

/* Prototype routines that use CLHDR or CLBLK structs */
struct CLHDR *findcl(clloc *pclloc, int ierr);
int getclid(clloc *pclloc);
int d3clck(struct CLHDR *pclh, void *pdblk, int kclck);
/* Values for kclck: */
#define CLCK_DPRINT  1           /* Merge in dprint info */
#define CLCK_PROBE   2           /* Merge in probe info */
#define CLCK_IJPL    4           /* Checking for IJ plot */
#define CLCK_SIMDAT  8           /* Checking for SIMDATA */
#define CLCK_NANAT  16           /* Checking for neuroanatomy */
#define CLCK_LPLT   32           /* Checking for layer plot */
/* Errors returned by d3clck: */
#define CLCK_BADNDX  1           /* User index exceeds list size */
#define CLCK_BADLYR  2           /* Layer name does not match */
#define CLCK_BADPRB  3           /* Wrong cell list for probe */
void d3clrm(int clnum);          /* Remove a cell list */
void d3clrx(int kctcl);          /* Remove all refs to a clst class */
struct CLHDR *d3clst(int clnum, int kmult);
/* Values for kmult: */
#define CLKM_MULTI   0           /* Allow multiple lists */
                                 /* (Used with CLIST cards) */
#define CLKM_SINGLE  1           /* Restrict to single list */
                                 /* (Used for CELLS=(list)) */
#define CLKM_INDIV   2           /* Process individually */
                                 /* (Used with SAVE, DETAIL cards) */
#endif   /* Not PARn */

struct CLBLK {                /* Cell list for one celltype */
   struct CLBLK    *pnclb;       /* Ptr to next CLBLK.  Must be
                                 *  first item in struct for sort() */
   struct CELLTYPE *pcll;        /* Pointer to cell layer */
   ilst            *pclil;       /* Pointer to iteration info */
   ui16            clseq;        /* Sorting key = il->seqvmo */
#define CLBLK_ANON UI16_MAX         /* Anonymous list flag */
   rlnm            clnm;         /* Cell list rep-layer name */
   };
