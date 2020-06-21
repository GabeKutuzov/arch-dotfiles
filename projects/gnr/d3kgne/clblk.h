/* (c) Copyright 1991-2003, Neurosciences Research Foundation, Inc. */
/* $Id: clblk.h 3 2008-03-11 19:50:00Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               clblk.h                                *
*                              04/08/90                                *
*                                                                      *
*   Definition of DARWIN III Cell List and Cell List Header Blocks     *
*                                                                      *
*  Notes:  The CLBLK clnm and clseq fields replicate info in the       *
*  CELLTYPE block, but clnm may be needed to identify a CLBLK before   *
*  it has been matched to its CELLTYPE, and clseq is needed if a sort  *
*  is done.  Use of tricks to save the space for clnm and clseq in     *
*  node memory would no doubt cost more in code than would be saved.   *
*  CLHDRs are stored only in host memory.  CLBLKs may be in Host or    *
*  Dynamic memory, depending on use.                                   *
*                                                                      *
*  V8B, 02/18/01, GNR - Complete rewrite, use ROCKS iterators          *
*  Rev, 07/14/01, GNR - Add CLHDR def, findcl() & d3clrm() prototypes  *
*  V8C, 08/31/03, GNR - Use ptr to ilst, not ilst itself, in CLBLK to  *
*                       to avoid bug if ilstread deletes then creates  *
*  ==>, 08/03/07, GNR - Last mod before committing to svn repository   *
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
struct CLHDR *findcl(int clid, int ierr);
int getclid(short *pclid, short *pclndx);
void d3clck(struct CLHDR *pclh, void *pdblk, int kclck);
/* Values for kclck: */
#define CLCK_SORT    1           /* Sort the cell list by seqvmo */
#define CLCK_DPRINT  2           /* Merge in dprint info */
#define CLCK_PROBE   4           /* Merge in probe info */
#define CLCK_IJPL    8           /* Checking for IJ plot */
void d3clrm(int clid);           /* Remove a cell list */
void d3clrx(int kctcl);          /* Remove all refs to a clst class */
struct CLHDR *d3clst(int clid, int kmult);
/* Values for kmult: */
#define CLKM_MULTI   0           /* Allow multiple lists */
#define CLKM_SINGLE  1           /* Restrict to single list */
#define CLKM_INDIV   2           /* Process individually */
#endif   /* Not PARn */

struct CLBLK {                /* Cell list for one celltype */
   struct CLBLK    *pnclb;       /* Ptr to next CLBLK.  Must be
                                 *  first item in struct for sort() */
   struct CELLTYPE *pcll;        /* Pointer to cell layer */
   ilst            *pclil;       /* Pointer to iteration info */
   ui16            clseq;        /* Sorting key = il->seqvmo */
   rlnm            clnm;         /* Cell list rep-layer name */
   };
