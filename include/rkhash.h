/* (c) Copyright 1998-2008, The Rockefeller University *11115* */
/* $Id: rkhash.h 59 2017-01-13 20:34:16Z  $ */
/***********************************************************************
*                           ROCKS C LIBRARY                            *
*                        rkhash.h header file                          *
*                                                                      *
************************************************************************
*  V1A, 09/26/98, GNR - New header file                                *
*  ==>, 07/01/02, GNR - Last date before committing to svn repository  *
***********************************************************************/

/* Definitions used by hash table functions */

#define HTMinSize 10                /* Minimum entries in hash table */

/* Define hash table information structure */

typedef struct htbl {
   void **phtbl;                    /* Ptr to the hash table */
   unsigned long (*hashfn)(void *); /* Ptr to user's hash function */
   unsigned long nallo;             /* No. table entries allocated */
   unsigned long nhtbl;             /* Current hash table capacity */
   unsigned long niiht;             /* Num. of items in hash table */
   int  lkey;                       /* Length of keys */
   short ohk;                       /* Offset to hash keys */
   short ohl;                       /* Offset to link field */
   } htbl;

/* Prototypes for functions dealing with hash tables */

struct htbl *hashinit(unsigned long (*hashfn)(void *),
      long nht, int lkey, int ohk, int ohl);
void hashadd(struct htbl *phtb, void *pdata);
void *hashlkup(struct htbl *phtb, void *pkey);
void hashdel(struct htbl *phtb, void *pdata);
void hashrlse(struct htbl *phtb);
