/* (c) Copyright 1993-2017, The Rockefeller University *21115* */
/* $Id: collect.h 60 2017-02-03 18:50:28Z  $ */
/***********************************************************************
*                              COLLECT.H                               *
*  Definitions and prototypes for the statistics gathering mechanism.  *
*                                                                      *
*  V1A, 01/15/93, ABP - Initial Version                                *
*  Rev, 06/07/03, GNR - Add LLONG (=long long) type code               *
*  ==>, 09/30/04, GNR - Last date before committing to svn repository  *
*  Rev, 07/07/12, GNR - Modify for use with coppack routines           *
*  Rev, 05/24/13, GNR - Revise COPDEF to allow 2-byte repeat counts    *
*  Rev, 06/17/16, GNR - Update for current definition of coppack       *
*  Rev, 01/28/17, GNR - Add mpgather                                   *
***********************************************************************/

/* Following section defines manner in which vectors may be combined */

/* Defined vector operations
*  N.B.  These are octal constants -- if more types are defined,
*  these definitions must be changed so as not to conflict.  */
#define REPEAT 000   /* Repeat a sequence of operations defined below */
#define ADD    010   /* Addition */
#define MAX    020   /* Maximum  */
#define MIN    030   /* Minimum  */
#define NOOP   040   /* Null operator- data in low node returned */
#define LOR    050   /* Logical OR operator */

/* Defined vector element types.  N.B.  If these codes are ever
*  changed, must change typlen[] in coppack.c to agree.  */
#define LONG   0     /* Long */
#define INT    1     /* Int  */
#define SHORT  2     /* Short */
#define FLOAT  3     /* Float */
#define DOUBLE 4     /* Double */
#define LLONG  5     /* Long Long */
#define SI32V  6     /* Signed 32-bit integer */
#define UI32V  7     /* Unsigned 32-bit integer */
#define OPMSK 077    /* Mask to extract op code from optype */
#define TPMSK  7     /* Mask to extract type code from optype */

/*---------------------------------------------------------------------*
*         COPDEF: Structure which directs vector combination           *
*                                                                      *
* Rev, 05/24/13, GNR:  This structure was revised to allow the 2-byte  *
* count field to be used as the repeat count for all operations.  For  *
* compatibility with old COPDEFs (except REPEAT operations) the field  *
* previously called 'rep', the number of COPDEFS in a repeating COPDEF *
* group (RCG), was removed and the number of COPDEFs in the repeat is  *
* now encoded in the left 10 bits of the 'optype' field for REPEATs.   *
*---------------------------------------------------------------------*/

#define RepShft  6      /* Number of bits in optype */
#define RepCop(rep) ((rep) << RepShft) /* Generate a repeat operator */
#define NxtAddr 0100    /* coppack cnt flag to advance block address */

struct COPDEF {
   ui16 optype;         /* Right 6 bits:  Operation and operand type.
                        *  Left 10 bits:  Number of following COPDEFs
                        *  included in a REPEAT.  */
   ui16 count;          /* Number of times operation is to be applied */
   };
typedef struct COPDEF COPD;

/*---------------------------------------------------------------------*
*      CPKDEF: Structure which holds packing status for coppack        *
*---------------------------------------------------------------------*/

#define NCPKALL     10     /* Initial number of addresses in a CPKD */

#define CPK_NOPREV 200     /* Error -- No previous block to join to */
#define CPK_RCGLNG 201     /* Error -- RCG exceeds opcnt */
#define CPK_NSTREP 202     /* Error -- Nested repeats */
#define CPK_TMBLKS 203     /* Error -- Too many data blocks */
#define CPK_BLKCMP 204     /* Error -- Blocks had different COPDEFs */
#define CPK_EFREED 205     /* Error -- Tried to reuse freed storage */
#define CPK_SMBSIZ 206     /* Error -- bsize parameter way too small */
#define CPK_MPIRDE 207     /* Error -- code returned by MPI_Recv */
#define CPK_MPIWRE 208     /* Error -- code returned by MPI_Send */

#define MPG_RDOMME 587     /* Error -- Rcvd data offset mismatch */
#define MPG_RBCTME 588     /* Error -- Num blks rcvd != header nblk */

struct CPKDEF {
   byte *buffw;         /* Ptr to start of write message buffer */
   byte *buffr;         /* Ptr to start of read message buffer */
   byte *pcd;           /* Ptr to current data = end of previous */
   byte **pal;          /* Ptr to address list */
   int alallo;          /* Number of pal addresses allocated */
   int alused;          /* Number of pal addresses used */
   int bsize;           /* Buffer size (bytes) */
   int odat;            /* Offset of data in buffw or buffr */
   int type;            /* Message type for this stream */
   int flags;           /* Flags */
#define CPK_INC0  0x01  /* Include node 0 data in collection */
#define CPK_FREED 0x10  /* This memory was freed--do not reuse */
   };
typedef struct CPKDEF CPKD;

/*---------------------------------------------------------------------*
*                        Function declarations                         *
*---------------------------------------------------------------------*/

#ifdef __cpluplus
extern "C" {
#endif
#ifdef USE_MPI
CPKD *coppini(int type, int bsize, int flags);
void coppack(CPKD *pcpk, void *pdat, COPD *ops, int opcnt);
void copcomb(CPKD *pcpk);
void copfree(CPKD *pcpk);
void mpgather(char *pbuf, void *pdat, int myoff, int ldat,
   int tblks, int nblks, int type);
#endif

void vecombine(void *dest, void *src, struct COPDEF *ops, int opcnt);
void vecollect(char *mypart, struct COPDEF *ops, int opcnt,
   int type, int len);
#ifdef __cplusplus
}
#endif
