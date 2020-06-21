/* (c) Copyright 1998-2016, The Rockefeller University *11115* */
/* $Id: membcst.c 3 2017-01-16 19:29:37Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                              membcst()                               *
*                                                                      *
*  Routine to broadcast shared memory information to comp nodes in a   *
*  parallel computer.  This routine, along with the host shared memory *
*  allocation routines allocpm(), allocpc(), allocpr(), freep(), and   *
*  their verbose (error-message-printing) equivalents, forms a package *
*  supporting five memory pools.  These pools are named by elements of *
*  the enum mempool.   Calls to the host allocation routines result in *
*  allocation of corresponding memory blocks on comp nodes and sharing *
*  of their contents when membcst() is called. The sizes of the blocks *
*  are adjusted according to the sizes of the constituent types on the *
*  comp nodes, the contents of the blocks are copied, the byte order   *
*  of the elements is reversed if necessary, and pointers are adjusted *
*  to point to the corresponding objects in comp node address space.   *
*  The five pools have special properties, as follows:                 *
*     Static      Handling is optimized on the assumption that these   *
*                 blocks will never be reallocated or freed during     *
*                 execution and no new blocks will be added after the  *
*                 first call to membcst().  Contents may change and    *
*                 new contents are sent to comp nodes by membcst().    *
*     Dynamic     Normal case.  Can be reallocated or freed ad lib.    *
*     Shared      Same as Dynamic with expectation that broadcast      *
*                 frequency will be much higher.  Keeping Dynamic      *
*                 a separate pool saves time updating control info.    *
*     Private     Space is allocated on comp nodes, but contents are   *
*                 not copied or processed in any way.                  *
*     Host        Not allocated on or copied to comp nodes.  Allows    *
*                 the allocation routines and memsize() to be used     *
*                 for data that exist only on the host.                *
*                                                                      *
*  Pointers in Static, Dynamic, and Shared areas may point to the      *
*  heads of blocks in any shared pool. All such pointers are adjusted  *
*  by membcst() to point to the corresponding blocks in the address    *
*  space of each recipient comp node.  Pointers to addresses inside    *
*  shared memory blocks cannot be adjusted.  They must either be       *
*  computed on each comp node or should be stored as offsets into      *
*  structured areas that may be relocated by a broadcast.              *
*                                                                      *
*  Synopsis:                                                           *
*     void *membcst(mempoolspec pools)                                 *
*                                                                      *
*  Argument:                                                           *
*     pools    Sum of one or more values indicating the memory pools   *
*              that are to be broadcast.  Use the constants beginning  *
*              with "MPS_" defined in memshare.h for this purpose.     *
*              (The special value (1<<Npools) may be used to trigger   *
*              a full BCST_Resend mode cycle for test purposes.        *
*              (3<<Npools) does the same and forces resend to fail.)   *
*                                                                      *
*  Returns:    Pointer to the start of the first memory block that     *
*              was processed in the lowest numbered pool included      *
*              in the 'pools' argument.  This should probably be a     *
*              root application data area containing pointers to all   *
*              other areas that comp nodes need access to.  Returns    *
*              NULL if no data were broadcast by this call.            *
*                                                                      *
*  Note:  Whenever the structure of any memory pool changes, membcst() *
*  automatically rebroadcasts the contents of all pools that can have  *
*  pointers (Static, Dynamic, and Shared) at the next call, allowing   *
*  pointers on comp nodes to be updated.  To minimize this overhead,   *
*  avoid changing memory allocations in loops that do shared memory    *
*  broadcasts.  (If necessary, code could be added to broadcast a list *
*  of pointers that need to be updated--it is not possible for comp    *
*  nodes to determine this without additional host information.)       *
*                                                                      *
*  As a special case, if a region is marked by MBT_Uninit when it is   *
*  allocated, any pointers within that group that are found during a   *
*  broadcast not to point to the start of a controlled allocation      *
*  will simply be left untouched, on the assumption that they are      *
*  managed by unique code on each node.                                *
*                                                                      *
*  This package provides memory management for parallel applications   *
*  in a manner that places minimal requirements on the application,    *
*  namely: (1) it must use the versions of malloc, calloc, realloc,    *
*  and free provided in the package, (2) it must call membcst() with   *
*  argument indicating which memory pools have changed each time data  *
*  need to be made consistent on all computational nodes, and (3) it   *
*  must provide the NXDRtypes table defined as extern in mempools.h.   *
*  This is normally done by running the nxdr2 utility at make time.    *
*  The same allocation routines, without membcst(), can be used on     *
*  serial computers--nxdr2 must still be run to make the tables.       *
*                                                                      *
*  N.B.  There is no special start-up call for these routines.  The    *
*  global memory information struct MI must be zeroed at load time.    *
************************************************************************
*  V1A, 06/13/98, GNR - Newly written package                          *
*  V1B, 03/04/00, GNR - Add HostString, use same calls for serial and  *
*                       parallel systems, add bit to force BCST_Resend *
*  V1C, 03/23/00, GNR - No pmblk on comp nodes, add mbdptr, datptr,    *
*                       use negative mbtype for structured data        *
*  V1D, 05/29/00, GNR - MBT_Unint bit in mtype argument                *
*  V1F, 06/30/01, GNR - Add handling for MPM_CHP (change pool) blocks  *
*  ==>, 12/29/09, GNR - Last mod before committing to svn repository   *
*  Rev, 04/25/16, GNR - Eliminate most longs for 64-bit mpi version    *
*  ==>, 10/18/16, GNR - Last mod before committing to svn mpi repo     *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include "sysdef.h"
#include "rksubs.h"
#include "rkarith.h"
#include "swap.h"
#include "mpitools.h"
#include "memshare.h"
#include "mempools.h"

/* Define MBDBG as sum of desired vales for debug output */
#define MBD_PROG  0x01     /* Provide progress reports */
#define MBD_XLAT  0x02     /* Report calls to xlatptr[x] */
#define MBD_CHGP  0x04     /* Report ChangedPools */
#define MBD_HDRS  0x08     /* Report broadcast of header info */
#define MBD_ALLO  0x10     /* Report allocations in PARn nodes */
#define MBDBG 0

#ifdef PARn
/*=====================================================================*
*                              mbphash()                               *
*  This is the hash routine for memory pointers. With 32-bit pointers, *
*  it just returns the pointer as an unsigned long int, which hashlkup *
*  takes modulo the table size to get an index.  With 64-bit pointers, *
*  it combines the upper and lower halves with XOR.                    *
*                                                                      *
*  Note:  rk hash package expects this function to return an unsigned  *
*  long that is 32(64) bits on a 32(64) bit machine.                   *
*                                                                      *
*  Synopsis:  unsigned long mbphash(void *p)                           *
*  Argument:  p is a pointer to the intptr to be hashed.  The value is *
*             assumed to be in a message buffer--possibly not aligned. *
*  Returns:   32-bit hash value for use by hashlkup function.          *
*=====================================================================*/

unsigned long mbphash(void *p) {
   char *pm = (char *)p;
#if PSIZE == 4          /* 32-bit machine: ulong is 32 in our model */
   return lemtoi4(pm);
#else                   /* 64-bit machine */
#if FMPSIZE == 4
#error FMPSIZE is 4 but PSIZE is 8 on this machine
#endif
   unsigned long lft = (unsigned long)lemtoi4(pm);
   unsigned long rgt = (unsigned long)lemtoi4((pm+4));
   return lft ^ rgt;
#endif
   } /* End of hash function */

/*=====================================================================*
*              Exported functions xlatptr(), xlatptrx()                *
*  These functions are used to translate a pointer from host address   *
*  space to node address space.  They simply look up the pointer in    *
*  the hash table maintained by membcst to find the relevant mblkdef   *
*  block and get the address of the node data there.  Translation of   *
*  pointers inside memory blocks is no longer supported.  NULL ptrs    *
*  are allowed and are translated to NULL pointers without wasting     *
*  time doing a lookup.                                                *
*                                                                      *
*  Synopsis:  void *xlatptr(void *p)                                   *
*  Argument:  p is a pointer to the host pointer stored as an intptr.  *
*  Returns:   Corresponding pointer in comp node address space.        *
*  Errors:    xlatptr() terminates execution if the pointer cannot be  *
*             translated--this can only result from a programming      *
*             error.  xlatptrx() returns the illegal value XLAT_NOLKUP *
*             if translation fails.                                    *
*=====================================================================*/

void *xlatptr(void *p) {

   void *rptr;
   static intptr nullptr = 0;

   if (memcmp((char *)p, (char *)&nullptr, FMPSIZE)) {
      register struct mblkdef *pa =
         (struct mblkdef *)hashlkup(MI.pmemhash, p);
      if (!pa) abexit(MP_UNXLAT_PTR);
      rptr = datptr(pa);
#if defined(MBDBG) && MBDBG & MBD_XLAT
      dbgprt(ssprintf(NULL,"xlatptr called, arg = %p, ret = %p",
         *(void **)p, rptr));
#endif
      return rptr;
      }
   else
#if defined(MBDBG) && MBDBG & MBD_XLAT
      dbgprt(ssprintf(NULL,"xlatptr called, arg = %p, ret = NULL",
         *(void **)p));
#endif
      return NULL;

   } /* End xlatptr() */

void *xlatptrx(void *p) {

   void *rptr;
   static intptr nullptr = 0;

   if (memcmp((char *)p, (char *)&nullptr, FMPSIZE)) {
      register struct mblkdef *pa =
         (struct mblkdef *)hashlkup(MI.pmemhash, p);
      rptr = pa ? datptr(pa) : (void *)XLAT_NOLKUP;
#if defined(MBDBG) && MBDBG & MBD_XLAT
      dbgprt(ssprintf(NULL,"xlatptr called, arg = %p, ret = %p",
         *(void **)p, rptr));
#endif
      return rptr;
      }
   else
#if defined(MBDBG) && MBDBG & MBD_XLAT
      dbgprt(ssprintf(NULL,"xlatptr called, arg = %p, ret = NULL",
         *(void **)p));
#endif
      return NULL;

   } /* End xlatptrx() */
#endif

/*=====================================================================*
*                              membcst()                               *
*  This routine is used to broadcast shared memory data from the host  *
*  to the comp nodes. A broadcast stream is created, mblkdefs for any  *
*  data blocks that have been changed in any pools are transmitted,    *
*  the recipient nodes use them to create a hash table for converting  *
*  pointers, then all the actual memory blocks in the selected pools   *
*  are converted to nxdr format and broadcast.  The receivers convert  *
*  the data to the local representation and look up all pointers in    *
*  the hash table. If allocation on a node fails, tries to defragment  *
*  by deallocating and resending all data.  Routine membcst() acts as  *
*  a synchronization point even if no data are sent.                   *
*=====================================================================*/

void *membcst(mempoolspec pools) {

   void *pfobj;               /* Ptr to first object transmitted */
   struct mbdlist *pl;        /* Ptr to info for a given pool */

#ifdef PAR

/*---------------------------------------------------------------------*
*                   membcst() for parallel computer                    *
*---------------------------------------------------------------------*/

   struct NNSTR hhmem;        /* Buffered data stream */
   struct mblkdef *pa,*pb;    /* Ptr to mblkdef being processed */
   struct mbbdlink *pq;       /* Ptr to mods or base queue */
   size_t ldata;              /* Length of a data area */
   /* N.B.  making nbr and tnbr size_t fails, prob is unsigned */
   long   tnbr;               /* Total or temp num blks received */
#ifdef PARn
   size_t ltot;               /* Total length of mblkdef + data */
   long   nbr = -1;           /* Number of blocks to be received */
   intptr tip;                /* Temp for exchanging pointers */
   intptr tmagic;             /* Temp for receiving info.magic */
   long tmbtype;              /* Temp for receiving info.mbtype */
   long tnmb;                 /* Temp for receiving info.nmb */
   int DoTestResend;          /* Flag for special mode */
#else
   mempoolspec ips;           /* Pool selector */
#endif
   si32 MblkdefAck;           /* Acknowledgement of mblkdef chain */
   mempoolspec ChangedPools;  /* Bit flags for pools that changed */
   mempoolspec tpools;        /* Temp for working with pool lists */

/*---------------------------------------------------------------------*
*       All nodes initiate communication via a broadcast stream        *
*---------------------------------------------------------------------*/

#ifdef PAR0
   nnpcr(&hhmem, BCSTADDR, MEMSHARE_MSG);
#else
   nngcr(&hhmem, BCSTADDR, MEMSHARE_MSG);
#endif

HereToResend:
   pfobj = NULL;              /* Clear return value */
   MblkdefAck = 0;            /* Clear ack signal */

/*---------------------------------------------------------------------*
*     Host determines which pools have changed data block layout since *
*  last broadcast and sends this information to comp nodes.  On first  *
*  broadcast, host sends total number of blocks of all kinds to comp   *
*  nodes. They use this information to set initial size of hash table. *
*---------------------------------------------------------------------*/

#ifdef PAR0
   ChangedPools = 0;
   switch (MI.bcsttype) {
   case BCST_First:           /* First time, check all pools */
      tnbr = 0;
      for (pl=&MI.mchain[(int)Static],ips=MPS_Static;
            pl<&MI.mchain[Nshpools]; ++pl,ips<<=1) {
         tnbr += pl->mods.nlblks + pl->base.nlblks;
         if (pl->mods.nlblks) ChangedPools |= ips;
         }
#if defined(MBDBG) && MBDBG & MBD_PROG
      dbgprt(ssprintf(NULL,"MBDBG Loc 1, tnbr = %ld", tnbr));
#endif
#if defined(MBDBG) && MBDBG & MBD_CHGP
      dbgprt(ssprintf(NULL,"MBDBG First, ChangedPools = %d",
         ChangedPools));
#endif
      nnputil(&hhmem, tnbr);
      break;
   case BCST_Normal:          /* Check all but Static pool */
      for (pl=&MI.mchain[(int)Dynamic],ips=MPS_Dynamic;
            pl<&MI.mchain[Nshpools]; ++pl,ips<<=1)
         if (pl->mods.nlblks) ChangedPools |= ips;
#if defined(MBDBG) && MBDBG & MBD_CHGP
      dbgprt(ssprintf(NULL,"MBDBG Normal, ChangedPools = %d",
         ChangedPools));
#endif
      break;
   case BCST_Resend:          /* Check base blocks--all but Static */
      for (pl=&MI.mchain[(int)Dynamic],ips=MPS_Dynamic;
            pl<&MI.mchain[Nshpools]; ++pl,ips<<=1)
         if (pl->base.nlblks) ChangedPools |= ips;
#if defined(MBDBG) && MBDBG & MBD_CHGP
      dbgprt(ssprintf(NULL,"MBDBG Resend, ChangedPools = %d",
         ChangedPools));
#endif
      break;
      } /* End bcsttype switch */
   nnputi4(&hhmem, ChangedPools);
#else
   if (MI.bcsttype == BCST_First) {
      tnbr = nngetil(&hhmem);
#if defined(MBDBG) && MBDBG & MBD_PROG
      dbgprt(ssprintf(NULL,"MBDBG Loc 1A, got tnbr = %ld", tnbr));
#endif
      MI.pmemhash = hashinit(mbphash, tnbr, sizeof(intptr),
         (char *)&((struct mblkdef *)0)->info.phblk - (char *)0,
         (char *)&((struct mblkdef *)0)->info.hlink - (char *)0);
      } /* End distributing total number of blocks */
   ChangedPools = nngeti4(&hhmem);
#if defined(MBDBG) && MBDBG & MBD_CHGP
      dbgprt(ssprintf(NULL,"MBDBG Received ChangedPools = %d",
         ChangedPools));
#endif
#endif

/*---------------------------------------------------------------------*
*  On first broadcast, host sends size information for all Static pool *
*  blocks (which may vary in size between nodes).  Comp nodes use this *
*  information to allocate the entire static pool in one large block.  *
*  At this point, there can be no LOC or DEL blocks to worry about.    *
*---------------------------------------------------------------------*/

   pl = &MI.mchain[(int)Static];
   pa = NULL;              /* Safety and warning eliminator */
   if (MI.bcsttype == BCST_First) {
#ifdef PAR0
      pq = &pl->mods.link;
#if defined(MBDBG) && MBDBG & MBD_PROG
      dbgprt(ssprintf(NULL,"MBDBG Loc 2.1, nblks = %ld",
         pl->mods.nlblks));
#endif
      nnputil(&hhmem, pl->mods.nlblks);
      if (pa = pq->pnmbd) for ( ;      /* Assignment intended */
            pa!=(struct mblkdef *)pq; pa=pa->link.pnmbd) {
         nnputil(&hhmem, pa->info.nmb);
         nnputil(&hhmem, pa->info.mbtype);
         }
#if defined(MBDBG) && MBDBG & MBD_PROG
      dbgprt("MBDBG Loc 2.2 (Sent static blkinfo)");
#endif
#else /* PARn */
      tnbr = nngetil(&hhmem);
#if defined(MBDBG) && MBDBG & MBD_PROG
      dbgprt(ssprintf(NULL,"MBDBG Loc 2.1A, got tnbr = %ld "
         "static blocks", tnbr));
#endif
      ltot = tnbr * LMBDUP;
      while (tnbr--) {
         tnmb = nngetil(&hhmem);
         tmbtype = nngetil(&hhmem);
         ldata = tnmb * (tmbtype > 0 ? tmbtype :
            NC.pnxtt[NC.jnxtt][tmbtype & MBTMask]);
         ltot += ALIGN_UP(ldata);
         }
#if defined(MBDBG) && MBDBG & (MBD_PROG|MBD_ALLO)
      dbgprt(ssprintf(NULL,"MBDBG Loc 2.2A About to alloc %zd static)",
         ltot));
#endif
      pa = (struct mblkdef *)malloc(pl->lallmblks = ltot);
      if (!pa) goto CheckResend;
#endif
      } /* End distributing static pool information */

#ifdef PAR0
/*---------------------------------------------------------------------*
*  Host broadcasts all mblkdefs in mods queue for each modified pool   *
*  (all base blocks in all pools if resending to clear fragmentation). *
*  Each queue is preceded by a count so receiver knows when to stop.   *
*  MPM_ADD and MPM_CHG blocks are merged into the base queue after     *
*  being sent, whereas MPM_LOC and MPM_DEL blocks are deleted (the     *
*  memory they point to has already been deleted).  For best speed,    *
*  an acknowledgement is not exchanged until all chains are finished.  *
*---------------------------------------------------------------------*/

#if defined(MBDBG) && MBDBG & (MBD_CHGP|MBD_HDRS)
      dbgprt(ssprintf(NULL,"MBDBG Looping on ChangedPools = %d",
         ChangedPools));
#endif
   for (tpools=ChangedPools; tpools; ++pl,tpools>>=1) if (tpools & 1) {
      if (MI.bcsttype == BCST_Resend) {
         pq = &pl->base.link;
#if defined(MBDBG) && MBDBG & MBD_HDRS
      dbgprt(ssprintf(NULL,"MBDBG Loc 3A, Resend pool = %d, "
         "blocks = %ld", pl - MI.mchain, pl->base.nlblks));
#endif
         nnputil(&hhmem, pl->base.nlblks); }
      else {
         pq = &pl->mods.link;
#if defined(MBDBG) && MBDBG & MBD_HDRS
      dbgprt(ssprintf(NULL,"MBDBG Loc 3B, pool = %d, mods "
         "blocks = %ld", pl - MI.mchain, pl->mods.nlblks));
#endif
         nnputil(&hhmem, pl->mods.nlblks); }
      pa = pq->pnmbd;
      do {
         /* Expand pointers to the largest size found on any
         *  node.  Swapping is not necessary, because the only
         *  use that can be made of a pointer on another node
         *  is to look it up in a hash table, and each time we
         *  do this (here and in nncom), we omit swapping.  */
         putiptr(&hhmem, (intptr)pa->info.pmblk);
         putiptr(&hhmem, pa->info.magic);
         nnputil(&hhmem, pa->info.nmb);
         nnputil(&hhmem, pa->info.mbtype);
#if defined(MBDBG) && MBDBG & MBD_HDRS
      dbgprt(ssprintf(NULL,"   Sent pmblk = %p, magic = %ld, "
         "pfx = %ld, type = %ld, nmb = %ld", pa->info.pmblk,
         pa->info.magic, pa->info.mbtype >> (BITSPERLONG-2),
         pa->info.mbtype & MBTMask, pa->info.nmb));
#endif
         /* Save ptr to next blk because pa area is
         *  either modified or freed... */
         pb = pa->link.pnmbd;
         /* Free location or change-pool-location blocks.  (It's OK
         *  to use free() here--address checked when block made.) */
         if (pa->info.magic & MPM_LOC)
            free(pa);
         /* Merge new and revised blocks into base list */
         else if (pa->info.magic & (MPM_CHP | MPM_ADD)) {
            allolink(pa, &pl->base);
            pa->info.magic &= ~(CodeMask | MPM_CHP);
            }
         /* If it's a deletion, just free it */
         else if (pa->info.magic & MPM_DEL)
            free(pa);
         /* If already in base list, MPM bits are clear and
         *  falls through to here--no other action occurs.  */
         pa = pb;
         } while (pa != (struct mblkdef *)pq);
      /* Indicate mods list is now empty and clean up ptrs */
      pl->mods.nlblks = 0;
      pl->mods.link.pnmbd = pl->mods.link.ppmbd = NULL;
      } /* End loop over changed memory pools */
   nnpfl(&hhmem);       /* Flush data stream */

#else
/*---------------------------------------------------------------------*
*  Comp nodes receive mods blocks from host and update allocations     *
*  accordingly.  Static pool is handled separately, allocating space   *
*  for mblkdefs and data in one large block.  If unable to allocate    *
*  required storage, set to enter resending mode, but remain in synch  *
*  by skipping rest of broadcast data until count is satisfied.        *
*---------------------------------------------------------------------*/

/* Deal with initial broadcast of Static pool.
*  N.B.  pa is set to bottom of static pool by code above.  */

   tpools = ChangedPools;
   if (tpools & MPS_Static) {
      /* Safety check */
      if (MI.bcsttype != BCST_First) abexit(MP_CHG_STATIC);
      /* Get number of mblkdefs to expect */
      nbr = nngetil(&hhmem);
#if defined(MBDBG) && MBDBG & MBD_PROG
      dbgprt(ssprintf(NULL,"MBDBG Loc 4, nbr = %ld", nbr));
#endif
      /* Bring over the mblkdefs */
      while (nbr--) {
         pa->info.hlink = NULL;
         pa->info.phblk = getiptr(&hhmem);
         tmagic = getiptr(&hhmem);
         if ((tmagic & CodeMask) != MPM_ADD) /* JIC */
            abexit(MP_BAD_POOLDR);
         pa->info.nmb    = nngetil(&hhmem);
         pa->info.mbtype = nngetil(&hhmem);
         allolink(pa, &pl->base);
#if defined(MBDBG) && MBDBG & MBD_HDRS
      dbgprt(ssprintf(NULL,"   Static Rcvd phblk = %p, magic = %ld, "
         "pfx = %ld, type = %ld, nmb = %ld", pa->info.phblk, tmagic,
         pa->info.mbtype >> (BITSPERLONG-2),
         pa->info.mbtype & MBTMask, pa->info.nmb));
#endif
         hashadd(MI.pmemhash, pa);
         ldata = mbsize(pa);
         pa = (struct mblkdef *)((char *)pa+LMBDUP+ALIGN_UP(ldata));
         } /* End loop over mblkdefs */
      } /* End first (and only) broadcast of Static pool defs */

/* Deal with pools other than Static */

   DoTestResend = (NC.node == NC.tailnode) && (pools & (3<<Npools));
   pl = &MI.mchain[(int)Dynamic];
#if defined(MBDBG) && MBDBG & MBD_CHGP
      dbgprt(ssprintf(NULL,"MBDBG Looping over tpools = %d",
         tpools & ~MPS_Static));
#endif
   for (tpools>>=1; tpools; ++pl,tpools>>=1) if (tpools & 1) {
      /* Get number of mblkdefs to expect */
      nbr = nngetil(&hhmem);
#if defined(MBDBG) && MBDBG & MBD_PROG
      dbgprt(ssprintf(NULL,"MBDBG Loc 5, pool = %d, nbr = %ld",
         pl - MI.mchain, nbr));
#endif
      while (nbr--) {
         tip     = getiptr(&hhmem);
         tmagic  = getiptr(&hhmem);
         tnmb    = nngetil(&hhmem);
         tmbtype = nngetil(&hhmem);
#if defined(MBDBG) && MBDBG & MBD_HDRS
      dbgprt(ssprintf(NULL,"   Rcvd phblk = %p, magic = %ld, "
         "pfx = %ld, type = %ld, nmb = %ld", tip, tmagic,
         tmbtype >> (BITSPERLONG-2), tmbtype & MBTMask, tnmb));
#endif
         if (DoTestResend) goto CheckResend;
         /* Get size according to whether structured or not */
         ldata = tnmb * (tmbtype > 0 ?
            tmbtype : NC.pnxtt[NC.jnxtt][tmbtype & MBTMask]);
         ltot = LMBDUP + ALIGN_UP(ldata);
#if defined(MBDBG) && MBDBG & MBD_HDRS
      dbgprt(ssprintf(NULL,"   Computed ltot = %zd", ltot));
#endif
         switch (tmagic & (CodeMask|MPM_CHP)) {
         case 0:           /* This is a resent base block */
         case MPM_ADD:     /* Add this block to the parent pool */
            /* Allocate for mblkdef and data */
            pa = (struct mblkdef *)malloc(ltot);
#if defined(MBDBG) && MBDBG & MBD_ALLO
      dbgprt(ssprintf(NULL,"   Allocd ltot at %p", pa));
#endif
            if (!pa) goto CheckResend;
            goto FinishAdd;
         case MPM_DEL:     /* Delete this block */
         case MPM_LOC:     /* Locate a block for reallocation */
         case MPM_CHP:     /* Move a block to a different pool */
         case (MPM_CHP|MPM_LOC):    /* Relocate and reallocate */
            /* Locate the block based on host location */
            pa = (struct mblkdef *)hashlkup(MI.pmemhash, &tip);
            if (!pa) abexit(MP_REF_UNALLO);  /* JIC */
            /* Unlink the block and decrement pool size */
            {  struct mbdlist *pol = allounlk(pa, tmagic);
               pol->lallmblks -= ltot;
               } /* End pol local scope */
            /* Remove the hash table entry */
            hashdel(MI.pmemhash, pa);
            if (tmagic & MPM_LOC) {
               /* After LOC, should get an MPM_CHG block next.
               *  Leave location of old block in pb.  */
               pb = pa;
               }
            else if (tmagic & MPM_CHP) {
               /* This is a straight pool change--go add to new pool */
               goto FinishAdd;
               }
            else {
               /* This is a straight deletion */
               free(pa);
               /* Help trap errors in reallocs */
               pb = NULL;
               }
            break;
         case MPM_CHG:     /* Reallocate a block */
            /* Allocate for mblkdef and data */
            if (!pb) abexit(MP_BAD_POOLDR);
            pa = (struct mblkdef *)realloc(pb, ltot);
            if (!pa) goto CheckResend;
FinishAdd:
            /* Link the mblkdef into the pool */
            allolink(pa, &pl->base);
            /* Increment global pool size */
            pl->lallmblks += ltot;
            /* Copy mblkdef info to permanent location */
            pa->info.phblk  = tip;
            pa->info.nmb    = tnmb;
            pa->info.mbtype = tmbtype;
            /* Add the new block into the hash table */
            hashadd(MI.pmemhash, pa);
            /* Help trap errors in reallocs */
            pb = NULL;
            break;
         default:
            abexit(MP_BAD_POOLDR);
            } /* End block type switch */
         } /* End loop over incoming blocks */
      } /* End loop over changed memory pools */

/* Check whether all mblkdefs were received and stored and the
*  corresponding memory successfully allocated.  If not, skip over
*  any remaining mblkdefs.  If this is first broadcast (no memory
*  fragmentation can have happened yet) or already a resend, then
*  terminate with failure.  Otherwise, initiate a resend at the host.
*/

CheckResend:
   if (nbr >= 0) {
#if defined(MBDBG) && MBDBG & MBD_PROG
      dbgprt(ssprintf(NULL,"MBDBG Loc 6, nbr = %ld", nbr));
#endif
      MblkdefAck =
         (MI.bcsttype == BCST_Normal) ? BCST_Resend : BCST_Fail;
      nngsk(&hhmem, nbr * LMBIMSG);
      /* Skip data for any remaining unprocessed pools */
#if defined(MBDBG) && MBDBG & MBD_CHGP
      dbgprt(ssprintf(NULL,"MBDBG CheckResend loop over tpools = %d",
         tpools));
#endif
      for (tpools>>=1; tpools; tpools>>=1) if (tpools & 1) {
         nbr = nngetil(&hhmem);
#if defined(MBDBG) && MBDBG & MBD_PROG
         dbgprt(ssprintf(NULL,"MBDBG Loc 7, nbr = %ld", nbr));
#endif
         nngsk(&hhmem, nbr * LMBIMSG);
         }
      }

#endif

/*---------------------------------------------------------------------*
*  Control of BCST_Resend mode, which attempts to defeat memory        *
*  fragmentation by reallocating all node memory after a failure.      *
*  Host collects largest of MblkdefAck flags from all nodes, then      *
*  broadcasts that value back to them (no need to flush if going       *
*  right on to data broadcast).  All nodes perform indicated action.   *
*  Comp nodes delete all but StaticTree before accepting new data.     *
*---------------------------------------------------------------------*/

   MblkdefAck = jmax(MblkdefAck);

#ifdef PAR0
   nnputi4(&hhmem, MblkdefAck);
   switch (MblkdefAck) {
   case BCST_Fail:
      nnpfl(&hhmem);
      abexit(MP_RESEND_BAD);
   case BCST_Resend:
      nnpfl(&hhmem);
      MI.bcsttype = BCST_Resend;
      pools |= MPS_Dynamic | MPS_Shared;
      goto HereToResend;
      } /* End MblkdefAck switch */
#else
   MblkdefAck = nngeti4(&hhmem);
   switch (MblkdefAck) {
   case BCST_Fail:
      /* If, after some time, we are still alive, something is wrong
      *  with the signal handling, and we shall commit suicide.  */
      sleep(5);
      abexit(MP_ALLOC_FAIL);
   case BCST_Resend:
      /* Remove data other than Static */
      for (pl=&MI.mchain[(int)Dynamic];
            pl<&MI.mchain[Nshpools]; ++pl) {
         pq = &pl->base.link;
         if ((pa = pq->pnmbd) != NULL) {
            for ( ; pa!=(struct mblkdef *)pq; pa=pb) {
               pb = pa->link.pnmbd;
               hashdel(MI.pmemhash, pa);
               free(pa);
               }
            pq->pnmbd = pq->ppmbd = NULL;
            }
         pl->base.nlblks = pl->lallmblks = 0;
         } /* End removing memory pools */
      MI.bcsttype = BCST_Resend;
      pools |= MPS_Dynamic | MPS_Shared;
      pools &= ~(1<<Npools);
      goto HereToResend;
      } /* End MblkdefAck switch */
#endif

/*---------------------------------------------------------------------*
*    Broadcast actual data.  Use nncom type conversion mechanism.      *
*---------------------------------------------------------------------*/

/* Make final list of data pools to be transmitted, namely, those
*  requested in the call plus any that might have changed pointers,
*  but never the Private or Host pools.  Bad pools arg is not an
*  error.  */

   tpools = MPS_Static | MPS_Dynamic | MPS_Shared;
   if (!ChangedPools) tpools &= pools;
#if defined(MBDBG) && MBDBG & MBD_CHGP
   dbgprt(ssprintf(NULL,"MBDBG Data bcst, tpools = %d", tpools));
#endif

/* Loop over modified and requested data pools */

   pl = &MI.mchain[0];
   for ( ; tpools; ++pl,tpools>>=1,pools>>=1) if (tpools & 1) {
      long *pnxdrt;
      size_t nmbs, nmbd = pl->base.nlblks;
#ifdef PAR0
      int sendflag = (pools & 1) ? NNC_Send : NNC_SendPtrs;
#if defined(MBDBG) && MBDBG & MBD_PROG
      dbgprt(ssprintf(NULL,"MBDBG Loc 10, sendflag = %d", sendflag));
#endif
#else
      int sendflag, tsflag = (pools & 1) ? NNC_Rcv : NNC_RcvPtrs;
#if defined(MBDBG) && MBDBG & MBD_PROG
      dbgprt(ssprintf(NULL,"MBDBG Loc 10A, tsflag = %d", tsflag));
#endif
#endif
      for (pa=pl->base.link.pnmbd; nmbd; pa=pa->link.pnmbd,--nmbd) {
         char *pobj = datptr(pa);
         /* Keep track of first block sent for return to user */
         if (!pfobj && pa->info.nmb > 0) pfobj = pobj;
         if (pa->info.mbtype > 0) {
            /* This is an unstructured data type--not necessary
            *  to transfer it unless in pool requested by user.
            *  Can move all array elements in one go */
            if (pools & 1) {
               ldata = pa->info.nmb * pa->info.mbtype;
#ifdef PAR0
               nnput(&hhmem, pobj, ckul2i(ldata, "membcst"));
#else
               nnget(&hhmem, pobj, ckul2i(ldata, "membcst"));
#endif
#if defined(MBDBG) && MBDBG & MBD_PROG
      dbgprt(ssprintf(NULL,"MBDBG Loc 11, ldata = %zd", ldata));
#endif
               }
            }
         else {
            pnxdrt = NC.pnxtt[NC.jnxtt] + (pa->info.mbtype & MBTMask);
#ifndef PAR0
            sendflag = pa->info.mbtype & MBT_Unint ?
               tsflag | NNC_RcvUnint : tsflag;
#endif
#if defined(MBDBG) && MBDBG & MBD_PROG
      dbgprt(ssprintf(NULL, "MBDBG Loc 12, pfx = %ld, mbtype = %ld, "
         "sendflag = %d", pa->info.mbtype >> (BITSPERLONG-2),
         pa->info.mbtype & MBTMask, sendflag));
#endif
            for (nmbs=pa->info.nmb; nmbs; --nmbs)
               /* nncom updates pobj to point to next object */
               nncom(&hhmem, (void **)&pobj, pnxdrt, sendflag);
            }
         } /* End sending data for one pool */
      } /* End loop over broadcast memory pools */

   /* Flush and close data stream */
#ifdef PAR0
   nnpcl(&hhmem);
#else
   nngcl(&hhmem);
#endif

#else    /* Serial computer */

/*---------------------------------------------------------------------*
*                    membcst() for serial computer                     *
*---------------------------------------------------------------------*/

/* Determine address of first allocated block in pool list */

   pfobj = NULL;
   pools &= MPS_AllPools;
   for (pl=&MI.mchain[(int)Static]; pools; ++pl,pools>>=1) {
      if ((pools & 1) && (pl->base.nlblks > 0)) {
         pfobj = pl->base.link.pnmbd->info.pmblk;
         break; }
      } /* End loop over requested memory pools */

#endif

/*---------------------------------------------------------------------*
*                         Common return code                           *
*---------------------------------------------------------------------*/

   /* Next broadcast will be normal type */
   MI.bcsttype = BCST_Normal;

   return pfobj;
   } /* End membcst() */

