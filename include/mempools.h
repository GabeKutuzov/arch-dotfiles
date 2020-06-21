/* (c) Copyright 1998-2016, The Rockefeller University *11115* */
/* $Id: mempools.h 59 2017-01-13 20:34:16Z  $ */
/***********************************************************************
*                             mempools.h                               *
* Definitions for managing shared memory pools in a parallel computer. *
*                                                                      *
* Information in this header should be visible only to the memory pool *
* management routines.  See memshare.h for user-visible definitions.   *
* All names have been changed to allow coexistence of new, old libs.   *
************************************************************************
*  V1A, 06/13/98, GNR - New header file                                *
*  V1C, 03/23/00, GNR - No pmblk on comp nodes, add mbdptr, datptr,    *
*                       mbsize, negative mbtype for structured data    *
*  V1F, 05/27/01, GNR - Add chngpool() routine                         *
*  ==>, 03/22/08, GNR - Last date before committing to svn repository  *
*  V1G, 09/21/08, GNR - Changes for 64-bit compilation                 *
*  V2A, 04/27/16, GNR - Changes for MPI environment                    *
*  ==>, 04/27/16, GNR - Last mod before committing to svn mpi repo     *
***********************************************************************/

#ifndef __MEMPOOLS__
#define __MEMPOOLS__

#include "sysdef.h"
#include "memshare.h"
#include "swap.h"
#ifdef PARn
#include "rkhash.h"
#endif

/* Definitions of abexit codes used by memory management */
#define MP_FREEP_FAIL   31 /* Unable to free unallocated region */
#define MP_ALLOC_FAIL   32 /* Unable to allocate requested block */
#define MP_REF_UNALLO   33 /* Bad pointer to realloc or whatpool */
#define MP_COMP_ALLOC   34 /* Attempt to alloc shared on comp node */
#define MP_UNREC_POOL   35 /* Pool number not a memtype */
#define MP_CHG_STATIC   36 /* Changed StaticTree after broadcast */
#define MP_RESEND_BAD   37 /* Unable to condense node memory */
#define MP_BAD_POOLDR   38 /* Bad pool control data encountered */
#define MP_UNXLAT_PTR   39 /* Unable to translate host pointer */

#define MEMSHARE_MSG 0x3120 /* Message type for memory broadcast */
#define MPMAGIC 0x47523000 /* Number stored in host pnblk */
#define MPMMask 0xfffff000 /* Mask for isolating MPMAGIC */
#define MPM_ADD       0x10 /* Code for adding new pool entry */
#define MPM_DEL       0x20 /* Code for deleting pool entry */
#define MPM_CHG       0x30 /* Code for changing pool entry--must
                           ** contain MPM_ADD bits as a subset.  */
#define MPM_LOC       0x40 /* Code for locating realloc'd block */
#define MPM_CHP       0x80 /* Code for changing pool of a block */
#define CodeMask      0x70 /* Mask to extract non-CHP type code */
#define PoolMask      0x0f /* Mask to extract pool from uhn.magic */
#define OldPShift        8 /* Shift to move old pool to pool */
#define MXMEMMSG        48 /* Max allowed size for emsg arg */

/* Pointers passed between nodes with nncom are expanded to this
*  length, which should be the largest size of a pointer on any node
*  in the network.  intptr should be an unsigned integer type large
*  enough to hold such a pointer.  */

#if FMPSIZE == 8
typedef ui64 intptr;
#define putiptr nnputu8
#define getiptr nngetu8
#else
typedef ui32 intptr;
#define putiptr nnputi4
#define getiptr nngeti4
#endif

/* Structure used to generate bidirectional linked lists
*  used to allow easy insertion/deletion of mblkdefs.  */
struct mbbdlink {
   struct mblkdef *pnmbd;  /* Ptr to next mblkdef in list */
   struct mblkdef *ppmbd;  /* Ptr to previous mblkdef (for freeing) */
   };

/* Memory information needed on host or comp node.  Note that pmblk
*  is not strictly required on the host--refs to it can be replaced
*  with datptr macros--but we need same length as on PARn nodes, so
*  might as well keep pmblk and make magic an intptr.  */
struct mblkinfo {
#ifdef PARn
   void *hlink;            /* Link field for hash table collisions */
   intptr phblk;           /* Integerized host ptr for hashing */
#else
   void *pmblk;            /* Ptr to memory data block */
   intptr magic;           /* Magic number, op, pool */
#endif
   long mbtype;            /* Mem blk size if > 0, type code if <= 0 */
   long nmb;               /* Number of blocks in this allocation */
   };
/* Length of items from mblkinfo sent between nodes (sent as
*  separate items, so not same as sizeof(struct mblkinfo))  */
#define LMBIMSG (FMPSIZE+3*FMLSIZE)

/* Memory allocation information stored at front of each block.
*  (Size will be rounded up to next multiple of BYTE_ALIGN.)  */
struct mblkdef {
   struct mbbdlink link;   /* Linkers must be first in struct! */
   struct mblkinfo info;   /* Info describing the data block */
   };
#define LMBDUP ALIGN_UP(sizeof(struct mblkdef))

/* Header for an mblkdef linked list */
struct mbdlhdr {
   struct mbbdlink link;   /* Linkers to the bidirectional list */
   unsigned long nlblks;   /* Number of blocks in the list */
   };

/* Combined headers for base and mods lists, with total length */
struct mbdlist {
   struct mbdlhdr base;    /* Header for base list */
#ifdef PAR0
   struct mbdlhdr mods;    /* Header for mods list */
#endif
#ifdef USE_MPI
   size_t lallmblks;       /* Length of all blocks of this type */
#else
   long lallmblks;         /* Length of all blocks of this type */
#endif
   };

/* Global data structure, maintained on all nodes.  The single hash
*  table is used to locate information about memory allocations based
*  on their locations.  All entries in MI must be initialized to 0 by
*  the loader per C spec for static structures.  */
#ifndef MAIN
extern
#endif
struct meminfo {
#ifdef PARn
   struct mbdlist mchain[Nshpools]; /* Info on mblkdef chains by type */
   struct htbl *pmemhash;           /* Ptr to hash table header */
#else
   struct mbdlist mchain[Npools];   /* Info on mblkdef chains by type */
#endif
   si32 bcsttype;             /* Type of broadcast: */
#define BCST_First   0           /* First--must be initially 0 */
#define BCST_Normal  1           /* Normal broadcast */
#define BCST_Resend  2           /* Resending all from start */
#define BCST_Fail    3           /* Ran out on StaticTree or Resend */
   } MI;

/* Internal function prototypes and macros */
extern void allolink(struct mblkdef *pa, struct mbdlhdr *pl);
#ifdef PARn
#ifdef USE_MPI
extern struct mbdlist *allounlk(struct mblkdef *pa, intptr magic);
#else
extern struct mbdlist *allounlk(struct mblkdef *pa, long magic);
#endif
#else
extern struct mbdlist *allounlk(struct mblkdef *pa);
#endif
extern struct mblkdef *allogetp(struct mblkdef *pa);
extern void allodel0(struct mblkdef *pa);
extern void memdump(mempoolspec pools);
#define mbdptr(data) ((struct mblkdef *)((char *)data - LMBDUP))
#define datptr(pmdb) ((void *)((char *)pmdb + LMBDUP))
#define mbsize(pmdb) (pmdb->info.nmb * (pmdb->info.mbtype > 0 ? \
   pmdb->info.mbtype : \
   NC.pnxtt[NC.jnxtt][pmdb->info.mbtype & MBTMask]))

#endif  /* __MEMPOOLS__ */

