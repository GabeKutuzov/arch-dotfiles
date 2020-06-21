/* (c) Copyright 1998-2016, The Rockefeller University *11115* */
/* $Id: nncom.c 3 2017-01-16 19:29:37Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                               nncom()                                *
*                                                                      *
*  A function to send and receive data in nxdr protocol over buffered  *
*  streams.  This is a simplified successor to hhcom().  It uses the   *
*  new swapping scheme to correctly handle cases where the local size  *
*  of a data item is not the same as its size in messages.  The code   *
*  is 64-bit pointer-ready.  It uses the function xlatptr() exported   *
*  from membcst.c to translate pointers.  For proper translation,      *
*  pointers must point to shared memory blocks managed via allocs()    *
*  and membcst() family of routines.  Pointers received on the host    *
*  are always ignored (not changed in memory).  Non-IEEE-floating      *
*  point can be handled without further changes here by writing the    *
*  appropriate lem{to|fm}r{4|8} routines.  Handling of byte swapping   *
*  has been changed to improve efficiency.                             *
*                                                                      *
*  Synopsis:                                                           *
*     void nncom(struct NNSTR *pnn, void **ppobj, long *pt, int flags) *
*                                                                      *
*  Arguments:                                                          *
*     pnn   - A pointer to an open NNSTR hybrid stream that is to      *
*             be used for sending or receiving the data.  May be a     *
*             broadcast stream.  (It might be a good idea some day     *
*             to provide a version of this routine that could take     *
*             an RFdef here, for making machine-independent binary     *
*             data files--after changing byte order to be same in      *
*             internode broadcasts and binary data files.              *
*     ppobj - A pointer to a pointer to an object (simple or           *
*             structured data type), that is to be sent or received.   *
*             Upon return, the pointer at *ppobj will have been        *
*             updated to point to the next following object.           *
*     pt    - A pointer to the nxdr conversion table that is to be     *
*             used to translate the object between processor types.    *
*             (A pointer rather than an offset into NXDRTT is required *
*             here to allow hand-coded translation tables to be used.) *
*     flags - Must take on one of the following values:                *
*             NNC_Send (0):  Send data to the partner node.  Pointers  *
*                 are always sent as if they were unsigned integers of *
*                 length FMPSIZE.                                      *
*             NNC_Rcv  (1):  Receive data from the partner node.       *
*                 Pointers are translated to the local address space   *
*                 using the xlatptr() function provided by membcst.c.  *
*                 Pointers received on the host are always discarded.  *
*             NNC_SendPtrs (2):  Send only pointers to the partner     *
*                 node (this mode is for internal use by membcst).     *
*             NNC_RcvPtrs  (3):  Receive pointers only from partner    *
*                 node.  Pointers are translated to the local address  *
*                 space using xlatptr(). Do not use on host, pointers  *
*                 received there are always discarded.                 *
*             NNC_RcvUnint (5):  Same as NNC_Rcv except pointers are   *
*                 translated to local address space using xlatptrx().  *
*                 If a pointer cannot be translated, it is left        *
*                 unchanged and no error occurs.  Do not use on host,  *
*                 pointers received are always discarded.              *
*                                                                      *
*  Return Value:  A value is returned indirectly by changing *ppobj.   *
*                                                                      *
*  Errors:    Terminates execution with abexit on any kind of error.   *
*                                                                      *
*  Notes:     When an 'X' code is encountered, indicating structured   *
*             data, nncom() calls itself recursively to process each   *
*             base type in the structure.  When a 'J' code is found,   *
*             indicating a union, nncom() uses the alignment info      *
*             and offset following the 'J' code to locate a pointer    *
*             in the NXDRUT table to a manually written routine which  *
*             it then calls (this indirection allows pointers to have  *
*             a length different from longs). This routine determines  *
*             which instance of the union is currently active and in   *
*             turn calls nncom() recursively to translate the selected *
*             instance.  With hand-coded conversion tables, code 'K'   *
*             may be used to indicate that the following two words are *
*             respectively the alignment info and a direct pointer to  *
*             the conversion routine for the union.  The interface to  *
*             these union conversion routines is as follows:           *
*                                                                      *
*  void funame(struct NNSTR *pnn, void **ppobj, void *parent,          *
*             int flags)                                               *
*                                                                      *
*  funame --  The name of the function is generated by nxdr. See nxdr  *
*             documentation in /home/nsi/docs.                         *
*  pnn, ppobj, flags -- These arguments have the same meaning as in    *
*             nncom() and they are passed by funame() to the copy of   *
*             nncom() which it invokes to do the actual conversion.    *
*  parent --  Pointer to the parent block that contains the union. The *
*             functions will have to cast this to the correct type and *
*             use fields in this block to decide on the union instance.*
*                                                                      *
*  This format is defined as 'unicvtf' (union conv. function) in the   *
*  header file generated by nxdr. The meaning of this type has changed *
*  from that used by the old hhcom() routine.                          *
*                                                                      *
************************************************************************
*  V1A, 10/17/98, GNR - New routine, based on hhcom()                  *
*  ==>, 12/29/09, GNR - Last mod before committing to svn repository   *
*  Rev, 04/27/16, GNR - Add new 'Z' type, overflow checking for 'L'    *
*  Rev, 09/23/16, GNR - Change NNC_RcvUnint action--leave ptr unchanged*
*  Rev, 09/25/16, GNR - Align union conversions using new 'J' coding   *
*  Rev, 10/18/16, GNR - Bug Fix:  Align again at end of struct         *
*  ==>, 10/19/16, GNR - Last mod before committing to svn mpi repo     *
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include "sysdef.h"
#include "mpitools.h"
#include "memshare.h"
#include "mempools.h"
#include "swap.h"
#include "rkarith.h"

/* Define NNDBG as sum of desired values for debug output */
#define NND_TYPE  0x01     /* Report type of each object */
#define NND_JUMP  0x02     /* Report details of union jumps */
#define NND_DATA  0x04     /* Print actual data */
#define NNDBG 0

/*=====================================================================*
*                               nncom()                                *
*=====================================================================*/

void nncom(struct NNSTR *pnn, void **ppobj, long *pt, int flags) {

   char  *pitem;           /* Ptr to item being moved to buffer */
   char  *psblk = *ppobj;  /* Pointer to start of object block  */
#ifdef PARn
   void *(*xlatp)(void *); /* Pointer to proper translation fn  */
#endif
   long  count;            /* Field count */
   long  totsz;            /* Total local size of current entry */
#if defined(NNDBG) && NNDBG & NND_DATA
   long  idbg,ndbg;        /* Counters for debug messages */
#endif
#if STRUCT_ALIGN > 0
   int   algnstr = 0;      /* Flag and amount to align at end */
#endif
   int   sizl,sizm;        /* Object size in local,message memory */
   int   type,uptype;      /* Type of datum */

/*---------------------------------------------------------------------*
*                       nncom() executable code                        *
*---------------------------------------------------------------------*/

#if defined(NNDBG) && NNDBG != 0
   dbgprt(ssprintf(NULL, "nncom entered, table offset = %zd, "
      "flags = %d", pt - NC.pnxtt[NC.jnxtt], flags));
#endif
/* First value in the table pt is the total size of this object.
*  It is currently not used by nncom(), so skip over it. */

   pt++;

/* Determine how pointers will be translated */

#ifdef PARn
   xlatp = (flags & (NNC_RcvUnint^NNC_Rcv)) ? xlatptrx : xlatptr;
#endif

/* Loop until an upper case code letter has been processed */

   do {

      /* Extract count and type from current table entry */
      count = *pt >> 8;       /* No sign problems */
      type  = (int)(*pt & 0xFF);
      uptype = toupper(type);

      switch (uptype) {

/*---------------------------------------------------------------------*
*                        Handle derived types                          *
*---------------------------------------------------------------------*/

      case 'S':
         /* Code 'S' is used to indicate that this is a structured data
         *  type. The count field holds the size of the largest element
         *  in the structure.  (Although this value is almost surely a
         *  power of 2, the code does not assume this.)
         *  Alignment depends on sysdef.h constant STRUCT_ALIGN:
         *     If STRUCT_ALIGN > 1, align on value of STRUCT_ALIGN.
         *     If STRUCT_ALIGN == 1, align on a multiple of the size
         *        of the largest item.
         *     If STRUCT_ALIGN == 0, individual objects are aligned
         *        per ALIGN_TYPE during transfers later.   */

#if defined(NNDBG) && NNDBG & NND_TYPE
         dbgprt(ssprintf(NULL, "Code S: Align %p to %ld",
            *ppobj, count));
#endif

#if STRUCT_ALIGN > 0
         algnstr = count;
#endif
#if STRUCT_ALIGN > 1
         {  intptr rem, tpobj = (intptr)*ppobj;
            if ((rem = tpobj % STRUCT_ALIGN) > 0)
               *ppobj = (void *)(tpobj + STRUCT_ALIGN - rem);
            }
#elif STRUCT_ALIGN == 1
         {  intptr rem, tpobj = (intptr)*ppobj;
            if ((rem = tpobj % algnstr) > 0)
               *ppobj = (void *)(tpobj + algnstr - rem);
            }
#endif
         goto SkipNonTransferItem;

      case 'X':
         /* Code 'X' stands for 'eXecute another table'.  The following
         *  value is the offset of another table, and we call ourselves
         *  'count' times. */

#if defined(NNDBG) && NNDBG & NND_TYPE
         dbgprt(ssprintf(NULL, "Code X: Exec NXDRTT[%ld] %ld times",
            *(pt+1), count));
#endif

         {  long *pxtab = NC.pnxtt[NC.jnxtt] + *++pt; /* Ptr to conversion table */
            while (count--)
               nncom(pnn, ppobj, pxtab, flags);
            }
         goto SkipNonTransferItem;

      case 'J':
         /* Code 'J' stands for "Jump to custom conversion routine".
         *  These routines, for conversion of unions, must be "hand-
         *  written" and must comply with the interface specified
         *  above.  In particular, the names of the routines are
         *  constructed in a uniform way, and each routine must keep
         *  the object pointer incremented correctly--i.e. by the
         *  size of the largest member of the union, not necessarily
         *  the one selected this time.  The conversion routine, in
         *  turn, will call nncom(), after selecting the proper
         *  instance, for a classic example of co-recursion...
         *  The object pointer must be aligned on the appropriate
         *  boundary for the full union before each invocation of
         *  the unicvtf function.  */

         {  long joff = *++pt;
#if STRUCT_ALIGN > 0
            intptr rem, tpobj;
            int algnuni = joff & 0xFF;
#endif
            joff >>= 8;
            unicvtf *pucf = (unicvtf *)NC.pnxut[NC.jnxtt][joff];

#if defined(NNDBG) && NNDBG & (NND_TYPE|NND_JUMP)
            dbgprt(ssprintf(NULL, "Code J: "
               "Jump to NXDRUT[%ld] %ld times", joff, count));
#endif

            while (count--) {
#if STRUCT_ALIGN > 1
               tpobj = (intptr)*ppobj;
               if ((rem = tpobj % STRUCT_ALIGN) > 0)
                  *ppobj = (void *)(tpobj + STRUCT_ALIGN - rem);
#elif STRUCT_ALIGN == 1
               tpobj = (intptr)*ppobj;
               if ((rem = tpobj % algnuni) > 0)
                  *ppobj = (void *)(tpobj + algnuni - rem);
#endif
#if defined(NNDBG) && NNDBG & NND_JUMP
            dbgprt(ssprintf(NULL, "Calling pucf, obj = %p, "
               "parent = %p, flags = %d", *ppobj, psblk, flags));
#endif
               pucf(pnn, ppobj, psblk, flags); }
            }
         goto SkipNonTransferItem;

      case 'K':
         /* Same as 'J' except used with hand-coded conversion tables.
         *  The entry is followed by two words: the alignment size and
         *  a pointer to the desired conversion routine.  The NXDRUT
         *  table is not used.  This code can handle the case that the
         *  length of a pointer is equal to that of one or two longs.
         */

         ++pt;
         {  unicvtf *pucf;
#if STRUCT_ALIGN > 0
            intptr rem, tpobj;
            int algnuni = *pt;
#endif
            pucf = (unicvtf *)*++pt;
            pt += PSIZE/LSIZE;

#if defined(NNDBG) && NNDBG & (NND_TYPE|NND_JUMP)
            dbgprt(ssprintf(NULL, "Code K: Jump to %p %ld times",
               pucf, count));
#endif

            while (count--) {
#if STRUCT_ALIGN > 1
               tpobj = (intptr)*ppobj;
               if ((rem = tpobj % STRUCT_ALIGN) > 0)
                  *ppobj = (void *)(tpobj + STRUCT_ALIGN - rem);
#elif STRUCT_ALIGN == 1
               tpobj = (intptr)*ppobj;
               if ((rem = tpobj % algnuni) > 0)
                  *ppobj = (void *)(tpobj + algnuni - rem);
#endif
#if defined(NNDBG) && NNDBG & NND_JUMP
               dbgprt(ssprintf(NULL, "Calling pucf, obj = %p, "
                  "parent = %p, flags = %d", *ppobj, psblk, flags));
#endif
               pucf(pnn, ppobj, psblk, flags); }
            }
         continue;            /* On to next table entry */

/*---------------------------------------------------------------------*
*  Handle base types that have a finite size.  The buffered nnput/     *
*  nnget message I/O mechanism is invoked to do the actual transfer of *
*  data.  Data whose types in memory match their types in messages can *
*  simply be moved with nnput(), whereas all other data types must be  *
*  handled with individual swapping or conversion routines as needed.  *
*---------------------------------------------------------------------*/

/* Determine item size and align on start of item */

      case 'B':
      case 'C':
         sizl = sizm = BSIZE;  break;
      case 'D':
         sizl = DSIZE; sizm = FMDSIZE; break;
      case 'F':
         sizl = ESIZE; sizm = FMESIZE; break;
      case 'H':
      case 'M':
         sizl = HSIZE; sizm = FMSSIZE; break;
      case 'E':
      case 'I':
      case 'U':
         sizl = ISIZE; sizm = FMISIZE; break;
      case 'L':
      case 'N':
         sizl = LSIZE; sizm = FMLSIZE; break;
      case 'P':
      case 'V':
         sizl = PSIZE; sizm = FMPSIZE; break;
      case 'W':
      case 'Y':
         sizl = WSIZE; sizm = FMWSIZE; break;
      case 'Z':
         sizl = STSIZE; sizm = FMZSIZE; break;
      default:
#if defined(NNDBG) && NNDBG & NND_TYPE
         {  char tc[2];
            tc[0] = (char)uptype, tc[1] = '\0';
            dbgprt(ssprintf(NULL, "nncom got type %1s, "
               "about to abexit(29)", tc));
            }
#endif
         abexit(29);
         } /* End type switch */

      totsz = (long)sizl * count;
      if (sizl > 1) {
         intptr tpobj = (intptr)*ppobj;
         intptr rem,algn;
#if ALIGN_TYPE != 0
         algn = sizl;
#else
         algn = min(sizl, BYTE_ALIGN);
#endif
         if (rem = tpobj % algn)    /* Assignment intended */
            *ppobj = (void *)(tpobj + sizl - rem);
         }

      /* Handle special mode to skip non-pointer data */
      /* Not sure how to handle 'V' here -- probably never seen */
      if (flags & NNC_SendPtrs && uptype != 'P')
         goto SkipTableItem;

      pitem = (char *)*ppobj;
      if ((flags & NNC_Rcv) == 0) {

/*---------------------------------------------------------------------*
*                              Send data                               *
*---------------------------------------------------------------------*/

#if defined(NNDBG) && NNDBG & NND_TYPE
         {  unsigned char dbgtype = (unsigned char)uptype;
            dbgprt(ssprintf(NULL, "Sending %ld items of type %1s "
               "from %p", count, &dbgtype, pitem)); }
#endif

         while (count) {
            /* Create a buffer if not yet allocated */
            char *pb = nnputp(pnn, sizm);
            /* Move as many as will fit in this buffer */
            long nmv = (long)(pnn->lbsr/sizm);
            if (nmv > count) nmv = count;
            count -= nmv;

#if defined(NNDBG) && NNDBG & NND_DATA
            for (idbg=0; idbg<nmv*sizm; idbg+=32) {
               ndbg = nmv*sizm - idbg;
               if (ndbg > 32) ndbg = 32;
               dbgprt(ssprintf(NULL, "    %*m",
                  (int)ndbg, pitem+idbg));
               }
#endif

            switch (uptype) {
            case 'B':
            case 'C':
               memcpy(pb, pitem, nmv);
               pb += nmv; pitem += nmv;
               break;
            case 'D':
               while (nmv--) {
                  lemfmr8(pb, *(double *)pitem);
                  pb += sizm; pitem += sizl; }
               break;
            case 'E':
            case 'I':
            case 'U':
               while (nmv--) {
                  lemfmi4(pb, *(int *)pitem);
                  pb += sizm; pitem += sizl; }
               break;
            case 'F':
               while (nmv--) {
                  lemfmr4(pb, *(float *)pitem);
                  pb += sizm; pitem += sizl; }
               break;
            case 'H':
            case 'M':
               while (nmv--) {
                  lemfmi2(pb, *(short *)pitem);
                  pb += sizm; pitem += sizl; }
               break;
            case 'L':
            case 'N':
               while (nmv--) {
                  lemfmil(pb, *(long *)pitem);
                  pb += sizm; pitem += sizl; }
               break;
            case 'P':
            case 'V':
               /* Expand pointers to the largest size found on any
               *  node.  Swapping is not necessary, because the only
               *  use that can be made of a pointer on another node
               *  is to look it up in a hash table, and each time we
               *  do this (here and in membcst), we omit swapping.  */
               while (nmv--) {
                  intptr pv = (intptr)*(void **)pitem;
                  memcpy(pb, (char *)&pv, FMPSIZE);
                  pb += sizm; pitem += sizl; }
               break;
            case 'W':
               while (nmv--) {
                  lemfmi8(pb, *(si64 *)pitem);
                  pb += sizm; pitem += sizl; }
               break;
            case 'Y':
               while (nmv--) {
                  lemfmu8(pb, *(ui64 *)pitem);
                  pb += sizm; pitem += sizl; }
               break;
            case 'Z':
               while (nmv--) {
#if STSIZE != FMZSIZE
                  lemfmu8(pb, jeul(*(size_t *)pitem);
#else
                  lemfmu8(pb, *(size_t *)pitem);
#endif
                  pb += sizm; pitem += sizl; }
               break;
               } /* End type switch */
            nnpsk(pnn, pb - (char *)pnn->bptr);
            } /* End while count */
         }  /* End sending */

/*---------------------------------------------------------------------*
*                            Receive data                              *
*---------------------------------------------------------------------*/

      else {

#if defined(NNDBG) && NNDBG & NND_TYPE
         {  unsigned char dbgtype = (unsigned char)uptype;
            dbgprt(ssprintf(NULL, "Receiving %ld items of type %1s "
               "to %p", count, &dbgtype, pitem)); }
#endif

#ifndef PARn
         /* On host, skip all pointers */
         if (uptype == 'P')
            nngsk(pnn, sizm*count);
         else /* combines with "while" on next line ... */
#endif
         while (count) {
            /* Get a buffer and move all the data it holds */
            char *pb;
            long nmv = nngetp(pnn, (void **)&pb)/sizm;
            if (nmv <= 0) abexit(27);
            if (nmv > count) nmv = count;
            count -= nmv;

#if defined(NNDBG) && NNDBG & NND_DATA
            for (idbg=0; idbg<nmv*sizm; idbg+=32) {
               ndbg = nmv*sizm - idbg;
               if (ndbg > 32) ndbg = 32;
               dbgprt(ssprintf(NULL, "    %*m",
                  (int)ndbg, pb+idbg));
               }
#endif

            switch (uptype) {
            case 'B':
            case 'C':
               memcpy(pitem, pb, nmv);
               pb += nmv; pitem += nmv;
               break;
            case 'D':
               while (nmv--) {
                  *(double *)pitem = lemtor8(pb);
                  pb += sizm; pitem += sizl; }
               break;
            case 'E':
            case 'I':
            case 'U':
               while (nmv--) {
                  *(int *)pitem = (int)lemtoi4(pb);
                  pb += sizm; pitem += sizl; }
               break;
            case 'F':
               while (nmv--) {
                  *(float *)pitem = lemtor4(pb);
                  pb += sizm; pitem += sizl; }
               break;
            case 'H':
            case 'M':
               while (nmv--) {
                  *(short *)pitem = lemtoi2(pb);
                  pb += sizm; pitem += sizl; }
               break;
            case 'L':
               while (nmv--) {
                  *(long *)pitem = lemtoile(pb, EAabx(92));
                  pb += sizm; pitem += sizl; }
               break;
            case 'N':
               while (nmv--) {
                  *(ulng *)pitem = lemtoule(pb, EAabx(92));
                  pb += sizm; pitem += sizl; }
               break;
#ifdef PARn             /* Host handled in code above */
            case 'P':
            case 'V':
               /* Translate pointers to local address space unless
               *  receiving on host.  Failure of translation causes
               *  termination inside xlatptr(), but returns
               *  XLAT_NOLKUP if xlatptrx() is called instead.
               *  Translation of ptrs into interior of unstructured
               *  areas is no longer supported.  */
               while (nmv--) {
                  void *plocal = xlatp((void *)pb);
                  if (plocal != (void *)XLAT_NOLKUP)
                     *(void **)pitem = plocal;
                  pb += sizm; pitem += sizl; }
               break;
#endif
            case 'W':
               while (nmv--) {
                  *(si64 *)pitem = lemtoi8(pb);
                  pb += sizm; pitem += sizl; }
               break;
            case 'Y':
               while (nmv--) {
                  *(ui64 *)pitem = lemtou8(pb);
                  pb += sizm; pitem += sizl; }
               break;
            case 'Z':
               while (nmv--) {
#if STSIZE != FMZSIZE
                  *(size_t *)pitem = (size_t)lemtoule(pb, EAabx(92));
#else
                  *(size_t *)pitem = (size_t)lemtou8(pb);
#endif
                  pb += sizm; pitem += sizl; }
               break;
               }  /* End type switch */
            nngsk(pnn, pb - (char *)pnn->bptr);
            } /* End while count */
         } /* End receiving */

      /* Advance object and table pointers */
SkipTableItem:
      *ppobj = (void *)((char *)(*ppobj) + totsz);
SkipNonTransferItem:
      pt += 1;

      } while (islower(type));

#if STRUCT_ALIGN > 1
   if (algnstr) {
      intptr rem, tpobj = (intptr)*ppobj;
      if ((rem = tpobj % STRUCT_ALIGN) > 0)
         *ppobj = (void *)(tpobj + STRUCT_ALIGN - rem);
      }
#elif STRUCT_ALIGN == 1
   if (algnstr) {
      intptr rem, tpobj = (intptr)*ppobj;
      if ((rem = tpobj % algnstr) > 0)
         *ppobj = (void *)(tpobj + algnstr - rem);
      }
#endif
   } /* End nncom() */

