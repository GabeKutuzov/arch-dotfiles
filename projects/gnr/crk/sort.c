/* (c) Copyright 1989-2008, The Rockefeller University *11116* */
/* $Id: sort.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                SORT                                  *
*                 Perform radix sort on a linked list                  *
*                                                                      *
*  This routine performs radix sorting of data structures organized    *
*  in a linked list.  Keys may consist of any multiple of 4 bits.      *
*                                                                      *
*  Usage:  void *sort(void *index, int keyoff, int n, int ktype)       *
*                                                                      *
*  Arguments:                                                          *
*     'index' points to a linked list of structures containing         *
*  the data to be sorted.  It is assumed that the first element of     *
*  each structure is a pointer to the next structure, and that the     *
*  list is terminated by a NULL pointer.  The pointers, and only       *
*  the pointers, are modified by sort.                                 *
*                                                                      *
*     'keyoff' is the offset in bytes from the beginning of each       *
*  structure to the beginning of the key.  The remainder of each       *
*  structure may contain data of any length.  Normally, the keys       *
*  are placed immediately after the linked-list pointers and           *
*  keyoff = sizeof(void *).  All sorts are in logical ascending        *
*  sequence (i.e. keys are treated as unsigned integers).  To sort     *
*  in descending sequence, the keys should be negated before calling   *
*  sort.  (This argument was not present in the FORTRAN version.)      *
*                                                                      *
*     'n' is the number of hexadecimal digits in the keys.             *
*                                                                      *
*     'ktype' indicates the type of the keys, and is 0 for numeric     *
*  keys and !=0 for character keys.  The order of significant bytes    *
*  is always left-to-right for character keys, but depends on the      *
*  hardware for numeric keys.                                          *
*                                                                      *
*  Value returned:  sort returns a pointer to the first structure      *
*     in the sorted list.  This pointer typically must be cast to      *
*     the appropriate type.                                            *
*                                                                      *
************************************************************************
*  V1A, 02/08/89, GNR - Converted from my  Assembler version           *
*  Rev, 04/03/89, GNR - Modify for NCUBE compilation                   *
*  Rev, 04/19/89, GNR - Prototype to rkxtra.h                          *
*  ==>, 07/01/02, GNR - Last date before committing to svn repository  *
***********************************************************************/

/*--------------------------------------------------------------------*
*     Global declarations                                             *
*--------------------------------------------------------------------*/

#define NBINS  16             /* Number of bins--DO NOT CHANGE */
#include <stddef.h>
#include <stdio.h>
#include "sysdef.h"
#include "rkxtra.h"

/*--------------------------------------------------------------------*
*     SORT                                                            *
*--------------------------------------------------------------------*/

void *sort(void *index, int keyoff, int n, int ktype) {

   void *bin[NBINS];          /* Bin headers */
   void *next[NBINS];         /* Ptr to next entry in each bin */
   void *ip;                  /* Master data pointer */
   void *iq;                  /* Working data pointer */
   int idigit;                /* Key hex digit counter */
   int keyshift;              /* Key shift */
   int keystep;               /* Key step */
   register int jb;           /* Bin counter */
   register unsigned int key; /* Particular instance of a key */

/* Set up for left-to-right or right-to-left operation */

   if (((ip = index) != NULL) && (n > 0)) {

#if BYTE_ORDRE < 0
      if (ktype) {            /* Character ==> right-to-left */
         keyoff += (n-1)>>1;
         keyshift = 4*(n&1);
         keystep = -1; }
      else {                  /* Number ==> left-to-right */
         keyshift = 0;
         keystep = 1; }
#else
      keyoff += (n-1)>>1;     /* Always right-to-left */
      keyshift = 4*(n&1);
      keystep = -1;
#endif

/* Loop over all the hex digits in the keys */

      for (idigit=0; idigit<n; idigit++) {

/* Initialize each bin so first pointer will go in base cell */

         for (jb=0; jb<NBINS; jb++) {
            bin[jb] = NULL;
            next[jb] = bin + jb; }

/* Traverse linked list, extracting current key digit and
   assigning each record to top of appropriate bin */

         for (iq=ip; iq; iq=*(void **)iq) {
            key = (unsigned int)*((unsigned char *)iq+keyoff);
            jb = (key>>keyshift) & (NBINS-1);
            *(void **)next[jb] = iq; /* Extend list */
            next[jb]  = iq;   /* Point to next list end */
            } /* End loop over data records */

/* End of pass, stack bottom of each bin on top of previous */

         for (iq=&ip,jb=0; jb<NBINS; jb++) {
            if (bin[jb]) {
               *(void **)iq = bin[jb];
               iq  = next[jb]; }
            }
         *(void **)iq = NULL; /* Terminate linked list */

/* Set shift and offset for next pass */

         if (keyshift) keyshift = 0, keyoff += keystep;
         else          keyshift = 4;
         } /* End idigit loop */
      } /* End if (index) ... */

   return ip;
   } /* End sort function */

