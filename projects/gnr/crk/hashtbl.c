/* (c) Copyright 1998-2008, The Rockefeller University *11115* */
/* $Id: hashtbl.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                              hashtbl.c                               *
*                                                                      *
*  This file contains routines that can be used to create and maintain *
*  generic hash tables.  The caller provides a hash function suitable  *
*  to the type of data at hand.  Duplicate hash values are handled by  *
*  building linked lists of items off each table entry.  It is assumed *
*  that the items being accessed are data structures of some sort, and *
*  that each contains a field where the hash key is stored and also a  *
*  reserved field of type (void *) that the hash routines can use as a *
*  place to store links when identical hash values occur.  Prototypes  *
*  and definitions for this module are in rkhash.h.                    *
*                                                                      *
*  There is provision for the routines to enlarge a hash table when it *
*  becomes too full.  When this happens, each data item is rehashed    *
*  using the new table length.  The user-supplied hash function should *
*  return a long value (32-bit or 64-bit as the case may be).  The     *
*  remainder modulo the current table size is used to construct the    *
*  index into the hash table.                                          *
*                                                                      *
*  1) Exported function hashinit()                                     *
*     Purpose:    To initialize a hash table.  Must be called before   *
*                    any of the other functions in this module.        *
*     Synopsis:   struct htbl *hashinit(unsigned long (*hashfn)        *
*                    (void *), long nht, int lkey, int ohk, int ohl)   *
*     Arguments:  'hashfn' is a pointer to a user-supplied function    *
*                    that takes as argument a pointer to whatever it   *
*                    is that the user is hashing and returns an        *
*                    unsigned long (32- or 64-bit) hash value.         *
*                 'nht' is the number of entries expected to be put    *
*                    in the table.  The actual length allocated for    *
*                    the table will be the lowest prime at least as    *
*                    large as (2*nht).  Each time the table becomes    *
*                    more than 3/4 full, it will be expanded by 50%.   *
*                    This action can be disabled by passing a nega-    *
*                    tive value for nht.                               *
*                 'lkey' is the length of the keys, in bytes. If       *
*                    lkey == 0, hash keys will be assumed to be        *
*                    NULL-terminated strings.  If lkey < 0, the item   *
*                    at offset 'ohk' is a pointer to the string data.  *
*                 'ohk' is the offset in the user data structures      *
*                    where hash keys are stored.  This argument will   *
*                    be used to construct arguments to hashfn().       *
*                 'ohl' is the offset in the user data structures      *
*                    where hash links can be stored.  It should be     *
*                    a multiple of the alignment size for the CPU.     *
*     Returns:    A pointer to an htbl structure that contains all the *
*                    information needed to work with this hash table.  *
*                    The value is used as an argument to the remain-   *
*                    ing routines in this package, allowing the user   *
*                    application to maintain multiple hashes at one    *
*                    time.                                             *
*     Errors:     Terminates execution if unable to allocate memory    *
*                    for the hash table.                               *
*                                                                      *
*  2) Exported function hashadd()                                      *
*     Purpose:    Add an entry to a hash table.                        *
*     Synopsis:   void hashadd(struct htbl *phtb, void *pdata)         *
*     Arguments:  'phtb' is a pointer to a hash table struct           *
*                    created by an earlier call to hashinit().         *
*                 'pdata' is a pointer to the user data struct         *
*                    to be added to the hash table.                    *
*     Returns:    Nothing.                                             *
*     Errors:     Terminates execution if unable to expand hash        *
*                    table when necessary.                             *
*                                                                      *
*  3) Exported function hashlkup()                                     *
*     Purpose:    Look up a data item stored in a hash table.          *
*     Synopsis:   void *hashlkup(struct htbl *phtb, void *pkey)        *
*     Arguments:  'phtb' is a pointer to a hash table struct           *
*                    created by an earlier call to hashinit().         *
*                 'pkey' is a pointer to the key identifying           *
*                    the data item to be looked up.                    *
*     Returns:    Pointer to the data item.  In case of duplicate      *
*                    data items, the link field in the user data       *
*                    structure can be followed to find additional      *
*                    entries with the same key.                        *
*     Errors:     Returns NULL pointer if item is not in table.        *
*                                                                      *
*  4) Exported function hashdel()                                      *
*     Purpose:    Remove an item from a hash table.                    *
*     Synopsis:   void hashdel(struct htbl *phtb, void *pdata)         *
*     Arguments:  'phtb' is a pointer to a hash table struct           *
*                    created by an earlier call to hashinit().         *
*                 'pdata' is a pointer to the user data item           *
*                    to be deleted from the hash table.                *
*     Returns:    Nothing.  The reference to the data item is          *
*                    removed from the hash table.  The data item       *
*                    itself is unaffected.                             *
*     Errors:     Terminates execution if the requested data           *
*                    item is not stored in the hash table.             *
*                                                                      *
*  5) Exported function hashrlse()                                     *
*     Purpose:    Release storage allocated to a hash table.           *
*     Synopsis:   void hashrlse(struct htbl *phtb)                     *
*     Argument:   'phtb' is a pointer to the hash table struct         *
*                    that is to be released.  The user data are not    *
*                    affected--the link fields in the user data can    *
*                    be reclaimed for other purposes if desired.       *
*     Returns:    Nothing.                                             *
*     Errors:     Execution terminated by freev() if phtb is an        *
*                    invalid pointer.                                  *
*                                                                      *
************************************************************************
*  V1A, 09/25/98, GNR - New module                                     *
*  Rev, 11/16/08, GNR - Modify as required for 64-bit systems          *
*  ==>, 11/17/08, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sysdef.h"
#include "rksubs.h"
#include "rkarith.h"
#include "rkhash.h"

/*=====================================================================*
*                              hashinit                                *
*=====================================================================*/

struct htbl *hashinit(unsigned long (*hashfn)(void *),
      long nht, int lkey, int ohk, int ohl) {

   struct htbl *phtb = (struct htbl *)mallocv(sizeof (struct htbl),
      "Hash Table Info");
   unsigned long nn = labs(nht);
   if (nn < HTMinSize) nn = HTMinSize;    /* Just in Case */
   phtb->hashfn = hashfn;
   phtb->nallo = nn = getprime(nn+nn);
   phtb->nhtbl = (nht < 0) ? ~0 : (nn+nn+nn)>>2;
   phtb->niiht = 0;
   phtb->lkey = lkey;
   if ((ohk | ohl) & ~SHRT_MAX) abexit(95);
   phtb->ohk = (short)ohk;
   phtb->ohl = (short)ohl;
   phtb->phtbl = (void **)callocv((size_t)nn, sizeof (void *),
      "Hash Table");
   return phtb;

   } /* End hashinit() */

/*=====================================================================*
*                               hashadd                                *
*=====================================================================*/

void hashadd(struct htbl *phtb, void *pdata) {

   void *pkey,**plink,**pmyh;

   /* Count items stored in table, expand if getting full */
   if (++phtb->niiht > phtb->nhtbl) {
      /* (This code is expected to execute rarely, if ever.) */
      unsigned long nn = getprime(phtb->nhtbl << 1);
      void *phold = NULL;  /* Chain to hold data during realloc */
      void *pnxti,**poldt,**poldte = phtb->phtbl + phtb->nallo;
      /* Link all the existing entries into one hold chain
      *  prior to reallocating a bigger table.  */
      for (poldt=phtb->phtbl; poldt<poldte; ++poldt) {
         while (*poldt) {
            plink = (void **)((char *)*poldt + phtb->ohl);
            pnxti = *plink;
            *plink = phold;
            phold = *poldt;
            *poldt = pnxti;
            } /* End following link chain */
         } /* End loop through old table */
      /* Free old, then alloc new--maybe in same page--rather
      *  than call realloc, which does an unnecessary copy.  */
      freev(phtb->phtbl, "Hash Table");
      phtb->phtbl = (void **)callocv((size_t)nn, sizeof(void *),
         "Hash Table Expansion");
      /* Move old items to new table */
      while (phold) {
         plink = (void **)((char *)phold + phtb->ohl);
         pnxti = *plink;
         pkey = (void *)((char *)phold + phtb->ohk);
         if (phtb->lkey < 0) pkey = *(void **)pkey;
         pmyh = phtb->phtbl + phtb->hashfn(pkey) % nn;
         *plink = *pmyh;
         *pmyh = phold;
         phold = pnxti;
         } /* End following link chain */
      /* Store info about new table in info block */
      phtb->nallo = nn;
      phtb->nhtbl = (nn+nn+nn)>>2;
      } /* End expanding table */

   /* Determine hash bin where new item should go */
   pkey = (void *)((char *)pdata + phtb->ohk);
   if (phtb->lkey < 0) pkey = *(void **)pkey;
   pmyh = phtb->phtbl + phtb->hashfn(pkey) % phtb->nallo;
   plink = (void **)((char *)pdata + phtb->ohl);

   /* Insert new item in chain at selected location */
   *plink = *pmyh;
   *pmyh = pdata;

   } /* End hashadd() */

/*=====================================================================*
*                              hashlkup                                *
*=====================================================================*/

void *hashlkup(struct htbl *phtb, void *pkey) {

   void **pmyh = phtb->phtbl + phtb->hashfn(pkey) % phtb->nallo;
   /* Not only the hash function, but also the key must match */
   if (phtb->lkey > 0) {
      while (*pmyh &&
         memcmp((char *)*pmyh+phtb->ohk, (char *)pkey, phtb->lkey))
            pmyh = (void **)((char *)*pmyh + phtb->ohl);
      }
   else if (phtb->lkey < 0) {
      while (*pmyh &&
         strcmp(*(char **)((char *)*pmyh+phtb->ohk), (char *)pkey))
            pmyh = (void **)((char *)*pmyh + phtb->ohl);
      }
   else {
      while (*pmyh &&
         strcmp((char *)*pmyh+phtb->ohk, (char *)pkey))
            pmyh = (void **)((char *)*pmyh + phtb->ohl);
      }
   return *pmyh;

   } /* End hashlkup() */

/*=====================================================================*
*                               hashdel                                *
*=====================================================================*/

void hashdel(struct htbl *phtb, void *pdata) {

   /* Instead of calling hashlkup, we do the equivalent search
   *  inline, thus giving us a pointer to the predecessor node.  */
   void **plink,*pkey,**pmyh;
   pkey = (void *)((char *)pdata + phtb->ohk);
   if (phtb->lkey < 0) pkey = *(void **)pkey;
   pmyh = phtb->phtbl + phtb->hashfn(pkey) % phtb->nallo;
   while (*pmyh) {
      plink = (void **)((char *)*pmyh + phtb->ohl);
      if (*pmyh == pdata) {
         /* Found the target data block, unlink it and return */
         *pmyh = *plink; return; }
      else {
         /* Step through to end of this chain */
         pmyh = plink; }
      } /* End searching for matching data structure */
   abexit(94);       /* Didn't find it--this is bad */

   } /* End hashdel() */

/*=====================================================================*
*                              hashrlse                                *
*=====================================================================*/

void hashrlse(struct htbl *phtb) {

   freev(phtb->phtbl, "Hash Table");
   freev(phtb, "Hash Table Info");

   } /* End hashrlse() */
