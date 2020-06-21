/* (c) Copyright 1991-2003, Neurosciences Research Foundation, Inc. */
/* $Id: d1def.h 48 2012-04-23 18:40:15Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d1def.h                                *
*                                                                      *
*  Define parameters and control blocks for embedding Darwin 1 in CNS  *
*                                                                      *
*  Note:  Unlike the case with CELLTYPE, we assume here that d1 data   *
*  will always be spread across all parallel nodes, not a subset.      *
*  This is because the number of d1 elements should generally be large *
*  enough to make any special load optimization unneccessary.  -GNR    *
*                                                                      *
*  Initial version, 09/22/91, GNR                                      *
*  V5E, 06/12/92, GNR - Add surround radius and replacement radius     *
*  Rev, 06/30/92, ABP - Move function prototypes to bottom of file     *
*  Rev, 12/11/92, ABP - Pull D1STAT out of D1BLK, for nxdr compat.     *
*  Rev, 08/17/93, GNR - Add cpn,crn,etc. for 2**n independence         *
*  Rev, 10/18/01, GNR - Separate kd1 into three components             *
*  V8C, 02/22/03, GNR - Cell responses in millivolts, add conductances *
*  ==>, 02/22/03, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include "swap.h"

/* Basic statistics (in inner structure for gather) */

struct D1STATS {
   long d1sumsco;             /* Sum of match scores (S8)             */
   long d1ncyc;               /* Number of cycles completed           */
   long d1maxsco;             /* Maximum score (S8)                   */
   long d1minsco;             /* Minimum score (S8)                   */
   long d1numgtt;             /* Number greater than amp threshold    */
   };


/* Master D1 information block */

struct D1BLK {                /* Info for an array of d1 repertoires  */
   struct D1BLK *pnd1b;       /* Ptr to next d1 block                 */
   d1s_type *pd1s;            /* Pointer to match scores              */
   byte *pd1r;                /* Ptr to repertoire data               */
   d1s_type *psr1,*psr2;      /* Repertoire bounds used in d1go only  */
   float d1x,d1y,d1w,d1h;     /* Plotting origin and box size         */
   float hspacing,vspacing;   /* Horizontal, vertical spacing         */
   float cpr;                 /* Cell plot radius                     */
   long odin;                 /* Offset in pbcst to d1 input data     */
   long lod1e;                /* Number of low cell on my node        */
   long mynd1;                /* Number of elements on my node        */
   long nepr;                 /* Number of elements per repertoire    */
#ifdef PAR
   long cpn;                  /* Cells per node                       */
   long crn;                  /* Cell remainder                       */
   long cut;                  /* First cell on node (node1+crn)       */
#endif
   long nsurr;                /* Surround radius for amp test         */
   long nsmxh;                /* Max hits allowed in surround         */
   long nsprd;                /* Spread radius for amplification (S1) */
   long ngxd1;                /* Number of groups along x (bubble)    */
   long ngyd1;                /* Number of groups along y (bubble)    */
   long d1seed;               /* Generating seed                      */
   long mutfreq;              /* Mutation frequency (S31)             */
   long wobblewt;             /* Weight of wobble matches (S16)       */
   short cpt;                 /* Circle plot threshold (S8)           */
   short jfile;               /* Input file selector                  */
   short rfile;               /* Restore file selector                */
   short li1r;                /* Length of input for 1 repertoire     */
   short nd1r;                /* Number of d1 repertoires             */
   short nbpe;                /* Number of bits per element           */
   short nbyt;                /* Number of bytes per element          */
   short ampf;                /* Amplification factor (S8)            */
   short ampm;                /* Amplification score multiplier (S0)  */
   short ampt;                /* Amplification threshold (S8)         */
   short rept;                /* Replacement threshold (S8)           */
   unsigned short kd1p;       /* Print/plot option codes:             */
#define KD1_PLOT    0x0001       /* P  Plot                           */
#define KD1_ALLS    0x0002       /* A  Plot in all series             */
#define KD1_OMIT    0x0004       /* O  Omit titles                    */
#define KD1_RETR    0x0008       /* 2  Retrace                        */
#define KD1_NOST    0x0010       /* U  Omit statistics                */
#define KD1_BUBB    0x0020       /* B  Make bubble plot               */
#define KD1_NUMB    0x0040       /* N  Put cell numbers on bar plot   */
#define KD1_SCPL    0x0080       /* S  Put scores on bar plot         */
#define KD1_GRPS    0x0100       /* C  Put group (category) on plot   */
   byte kd1o;                 /* OPT codes:                           */
#define KD1_XOR     0x01         /* X  Count matching ones and zeros  */
#define KD1_OVER    0x02         /* O  Override (or no) restore       */
#define KD1_GRAY    0x04         /* G  Use Gray code                  */
#define KD1_KEEP    0x08         /* K  Keep unaccessed scores         */
#define KD1_INDX    0x10         /* I  Indexed data                   */
#define KD1_MUTS    0x20         /* M  Mutate self                    */
#define KD1_VER1    0x40         /* 1  Expect or got V1 input file    */
   byte kd1f;                 /* Internal D1 flags:                   */
#define KD1_ALLO    0x01         /* Allocate storage--array is used   */
#define KD1_MATR    0x02         /* Matched restore file header       */


/* Basic statistics (in inner structure for gather) */

   struct D1STATS stats;

/* A few variables needed at setup time are stored in stats.
*  They are defined as macros to avoid putting stats in a union. */
#define D1offsi stats.d1sumsco
#define D1offrd stats.d1maxsco

   }; /* End d1blk */

/* DIGINPUT file header */

struct D1FHDR {
   char dhtype[4];            /* Tag "D1IN"                           */
   char dhvers[FMSSIZE];      /* Version number                       */
   char dhmode[FMSSIZE];      /* File mode                            */
#define DHM_NORM  0           /*    Normal                            */
#define DHM_INDX  1           /*    Indexed                           */
   char dhnd1b[FMLSIZE];      /* Number of blocks                     */
   char dhnt[FMLSIZE];        /* Number of trials                     */
   }; /* End D1FHDR */

/* DIGINPUT file block head */

struct D1FBHD {
   char dhnd1r[FMSSIZE];      /* Number of Darwin 1 repertoires       */
   char dhnbpe[FMSSIZE];      /* Number of bits per element           */
   }; /* End D1FBHD */

/* D1 function prototypes */

char *d1nset(char *);
void d1dflt(struct D1BLK *);
void d1exch(struct D1BLK *, int retflag);
void d1genr(void);
void d1go(byte *);
void d1rplt(void);
