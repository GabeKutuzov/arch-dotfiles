/* (c) Copyright 1993, Neurosciences Research Foundation, Inc. */
/* $Id: d3memacc.h 26 2009-12-31 21:09:33Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                        D3MEMACC Header File                          *
*                                                                      *
*                    Darwin3 memory access macros                      *
*                                                                      *
*  This file contains definitions of macros used for accessing         *
*     unstructured data (e.g. Darwin3 repertoire data).  In order      *
*     to remain reasonably portable and still permit optimization      *
*     for better performance on particular machines, the scheme has    *
*     been adapted that all accesses to unstructured memory should     *
*     be made via the macros in this file.  These can be optimized     *
*     (or converted to calls to Assembler functions) on particular     *
*     machines if desired.                                             *
*                                                                      *
*  The expansions of these macros are statements, not expressions.     *
*                                                                      *
*  Note that since all access to unstructured memory is supposed to    *
*     be done with these macros, we can arbitrarily decide the byte    *
*     order independently of the native order on a given machine.      *
*     In particular, we choose to make all data little-endian so that  *
*     SAVENET files can exchanged freely between machines without any  *
*     need to swap individual Lij,Cij values.  There is no cost to     *
*     this on the big-endian SPARC because we must access the data     *
*     one byte at a time anyway due to alignment restrictions.  On a   *
*     machine like an IBM mainframe, we probably want to store the     *
*     data in native order so ICM/STCM instructions can be used for    *
*     data access, and do the swapping in d3save/d3rstr.  If this      *
*     optimization is implemented, the macros given here need to be    *
*     changed, along with the code discussed in the next paragraph.    *
*                                                                      *
*  Note that D3MEMACC_BYTE_ORDER is not necessarily equal to           *
*     BYTE_ORDRE as defined in sysdef.h for a particular cpu.          *
*                                                                      *
*  The names of the macros are of the form "d3xxon" to indicate the    *
*     direction of transfer [xx=pt for put to memory, xx=gt for get    *
*     from memory], the bytes of interest in the working variable      *
*     [o=h for high order, o=l for low order, o=s for low order with   *
*     sign extension, o=i for 32-bit data that is guaranteed by d3allo *
*     to be aligned on a word boundary], and the number of bytes to be *
*     transferred [n].                                                 *
*                                                                      *
*  d3ptl1(wkvar,location)                                              *
*     Store the low order byte of 'wkvar' in 'location'.               *
*                                                                      *
*  d3gtl1(wkvar,location)                                              *
*     Assign the value of the byte pointed to by 'location' to the     *
*     low order byte of 'wkvar'.  Zero the remaining bytes of 'wkvar'. *
*                                                                      *
*  d3gts1(wkvar,location)                                              *
*     Assign the signed value of the char pointed to by 'location' to  *
*     the signed long word at 'wkvar'.                                 *
*                                                                      *
*  d3ptl2(wkvar,location)                                              *
*     Load the 2-byte unit pointed to by 'location' with the value of  *
*     the low order two bytes of 'wkvar'.                              *
*                                                                      *
*  d3gtl2(wkvar,location)                                              *
*     Assign the value of the 2-byte unit pointed to by 'location'     *
*     to the low order two bytes of 'wkvar'.  Zero the other bytes     *
*     in 'wkvar'.                                                      *
*                                                                      *
*  d3gts2(wkvar,location)                                              *
*     Assign the signed value of the 2-byte unit pointed to by         *
*     'location' to the signed long word at 'wkvar'.                   *
*                                                                      *
*  d3ptl3(wkvar,location)                                              *
*     Load the 3-byte unit pointed to by 'location' with the value of  *
*     the low order 3 bytes of 'wkvar'.                                *
*                                                                      *
*  d3gtl3(wkvar,location)                                              *
*     Assign the value of the 3-byte unit pointed to by 'location' to  *
*     the low order 3 bytes of 'wkvar'. Zero the other byte of 'wkvar'.*
*                                                                      *
*  d3ptl4(wkvar,location)                                              *
*     Load the 4-byte unit pointed to by 'location' with the value of  *
*     'wkvar'.                                                         *
*                                                                      *
*  d3gtl4(wkvar,location)                                              *
*     Assign the value of the 4-byte unit pointed to by 'location'     *
*     to 'wkvar'.                                                      *
*                                                                      *
*  d3ptln(wkvar,location,nmbyts)                                       *
*     Load the the nmbyts bytes pointed to by 'location' with the low  *
*     order nmbyts bytes of 'wkvar'.                                   *
*                                                                      *
*  d3gtln(wkvar,location,nmbyts)                                       *
*     Assign the low order nmbyts bytes of 'wkvar' the value of the    *
*     nmbyts bytes pointed to by 'location'.  Zero the remaining       *
*     bytes of 'wkvar'.                                                *
*                                                                      *
*  d3pth1(wkvar,location)                                              *
*     Load the byte pointed to by 'location' with the value of the     *
*     high order byte of 'wkvar'.                                      *
*                                                                      *
*  d3gth1(wkvar,location)                                              *
*     Assign the value of the byte pointed to by 'location' to the     *
*     high order byte of 'wkvar'.  Zero the other bytes in 'wkvar'.    *
*                                                                      *
*  d3pth2(wkvar,location)                                              *
*     Load the 2-byte unit pointed to by 'location' with the value of  *
*     the high order two bytes of 'wkvar'.                             *
*                                                                      *
*  d3gth2(wkvar,location)                                              *
*     Assign the value of the 2-byte unit pointed to by 'location'     *
*     to the high order two bytes of 'wkvar'.  Zero the other bytes    *
*     in 'wkvar'.                                                      *
*                                                                      *
*  d3pth3(wkvar,location)                                              *
*     Load the 3-byte unit pointed to by 'location' with the value of  *
*     high order 3 bytes of 'wkvar'.                                   *
*                                                                      *
*  d3gth3(wkvar,location)                                              *
*     Assign the value of the 3-byte unit pointed to by 'location' to  *
*     the high order 3 bytes of 'wkvar'.  Zero the other byte of       *
*     'wkvar'.                                                         *
*                                                                      *
*  d3pth4(wkvar,location)                                              *
*     Load the 4-byte unit pointed to by 'location' with the value of  *
*     'wkvar' [same as d3ptl4].                                        *
*                                                                      *
*  d3gth4(wkvar,location)                                              *
*     Assign the value of the 4-byte unit pointed to by 'location'     *
*     to 'wkvar' [same as d3gtl4].                                     *
*                                                                      *
*  d3pthn(wkvar,location,nmbyts)                                       *
*     Load the the nmbyts bytes pointed to by 'location' with the high *
*     order nmbyts bytes of 'wkvar'.                                   *
*                                                                      *
*  d3gthn(wkvar,location,nmbyts)                                       *
*     Assign the high order nmbyts bytes of 'wkvar' the value of the   *
*     nmbyts bytes pointed to by 'location'.  Zero the remaining       *
*     bytes of 'wkvar'.                                                *
*                                                                      *
*  d3pti4(wkvar,location)                                              *
*     Load the word-aligned 4-byte unit pointed to by 'location' with  *
*     the value of 'wkvar'.                                            *
*                                                                      *
*  d3gti4(wkvar,location)                                              *
*     Assign the value of the word-aligned 4-byte unit pointed to by   *
*     'location' to 'wkvar'                                            *
*                                                                      *
*       'wkvar'    -- A long type variable name. The address operator  *
*                       is applied to 'wkvar' in some of the macros,   *
*                       so it must be an 'lvalue'.                     *
*       'location' -- An expression whose value is a byte pointer      *
*                       that points to the first location in memory    *
*                       to be accessed.                                *
*       'nmbyts'   -- An integer type expression indicating the        *
*                       number of bytes to transfer.  Must be in the   *
*                       range 1-4.                                     *
*                                                                      *
*   Restrictions:                                                      *
*                                                                      *
*     i) 'wkvar' must be the name of a long variable.  In general,     *
*           illegal code will result from failure to comply with       *
*           this restriction.                                          *
*                                                                      *
*    ii) In the case of the d3xxxn macros one must be careful that     *
*          the length of the transfer is in the range 1-4.  There is   *
*          no provision for flagging illegal values of the transfer    *
*          length (these limitations are partly the result of an       *
*          effort to maximize the efficiency of the macros while       *
*          minimizing the amount of code involved).                    *
*                                                                      *
*   iii) 'location' must be a byte pointer.  Since there is no         *
*          prototype checking mechanism for macros one must be         *
*          particularly careful about this since the macros will       *
*          will quietly access the wrong data if given a pointer of    *
*          the wrong type.                                             *
*                                                                      *
*  Performance note:  For a long time, we had separate macros for the  *
*     NCUBE that used unaligned loads/stores of shorts or longs for    *
*     possible better speed.  However, tests showed that there was no  *
*     advantage to this, and so the present macros were adopted.       *
*     They are clearer and will work for all byte order combinations.  *
*                                                                      *
*  V4A, 12/24/88, Initial version                                      *
*  Rev, 09/28/92, GNR - Store all data in little-endian order          *
*  V8C, 03/19/03, GNR - Add d3gts2                                     *
*  ==>, 09/25/06, GNR - Last mod before committing to svn repository   *
*  V8E, 08/01/09, GNR - Add d3pti4 and d3gti4                          *
***********************************************************************/

/* If this definition is changed, d3save, d3rstr, d3gfhd, d3gfsv
*  must be examined--see discussion above.  */
#define D3MEMACC_BYTE_ORDER -1

/* Macro to calculate offset in a word of a byte according to its
*  magnitude (0=highest,... 3=lowest) */
#if BYTE_ORDRE > 0
#define WO(m) (m)
#else
#define WO(m) (3-m)
#endif

/* Macro to calculate offset in a memory block of length (n+1) of a
*  byte according to its magnitude (0=highest, ... n=lowest) */
#if D3MEMACC_BYTE_ORDER > 0
#define MO(m,n) (m)
#else
#define MO(m,n) (n-m)
#endif

/*---------------------------------------------------------------------*
*          Macros for dealing with low-order bytes of a word           *
*---------------------------------------------------------------------*/

#define d3gtl1(wkvar,location) (wkvar = (ui32)*(byte *)(location))

#define d3gts1(wkvar,location) (wkvar = (si32)*(schr *)(location))

#define d3ptl1(wkvar,location) (*(byte *)(location) = (byte)(wkvar))

#define d3gtl2(wkvar,location) (wkvar = 0, \
      *((byte *)&wkvar+WO(2)) = *((byte *)(location)+MO(0,1)), \
      *((byte *)&wkvar+WO(3)) = *((byte *)(location)+MO(1,1)))

#if (BYTE_ORDRE >= 0) ^ (D3MEMACC_BYTE_ORDER >= 0) == 0
#define d3gts2(wkvar,location) \
      { union { si16 ts; byte tb[2]; } tu; \
      tu.tb[0] = ((byte *)location)[0]; \
      tu.tb[1] = ((byte *)location)[1]; \
      wkvar = (long)tu.ts; }
#else
#define d3gts2(wkvar,location) \
      { union { si16 ts; byte tb[2]; } tu; \
      tu.tb[0] = ((byte *)location)[1]; \
      tu.tb[1] = ((byte *)location)[0]; \
      wkvar = (long)tu.ts; }
#endif

#define d3ptl2(wkvar,location) ( \
      *((byte *)(location)+MO(0,1)) = *((byte *)&wkvar+WO(2)), \
      *((byte *)(location)+MO(1,1)) = *((byte *)&wkvar+WO(3)))

#define d3gtl3(wkvar,location) (wkvar = 0, \
      *((byte *)&wkvar+WO(1)) = *((byte *)(location)+MO(0,2)), \
      *((byte *)&wkvar+WO(2)) = *((byte *)(location)+MO(1,2)), \
      *((byte *)&wkvar+WO(3)) = *((byte *)(location)+MO(2,2)))

#define d3ptl3(wkvar,location) ( \
      *((byte *)(location)+MO(0,2)) = *((byte *)&wkvar+WO(1)), \
      *((byte *)(location)+MO(1,2)) = *((byte *)&wkvar+WO(2)), \
      *((byte *)(location)+MO(2,2)) = *((byte *)&wkvar+WO(3)))

#define d3gtl4(wkvar,location) ( \
      *((byte *)&wkvar+WO(0)) = *((byte *)(location)+MO(0,3)), \
      *((byte *)&wkvar+WO(1)) = *((byte *)(location)+MO(1,3)), \
      *((byte *)&wkvar+WO(2)) = *((byte *)(location)+MO(2,3)), \
      *((byte *)&wkvar+WO(3)) = *((byte *)(location)+MO(3,3)))

#define d3ptl4(wkvar,location) ( \
      *((byte *)(location)+MO(0,3)) = *((byte *)&wkvar+WO(0)), \
      *((byte *)(location)+MO(1,3)) = *((byte *)&wkvar+WO(1)), \
      *((byte *)(location)+MO(2,3)) = *((byte *)&wkvar+WO(2)), \
      *((byte *)(location)+MO(3,3)) = *((byte *)&wkvar+WO(3)))

#define d3gtln(wkvar,location,nmbyts) switch(nmbyts) { \
      case 1: d3gtl1(wkvar,location); break; \
      case 2: d3gtl2(wkvar,location); break; \
      case 3: d3gtl3(wkvar,location); break; \
      case 4: d3gtl4(wkvar,location); break; }

#define d3ptln(wkvar,location,nmbyts) switch(nmbyts) { \
      case 1: d3ptl1(wkvar,location); break; \
      case 2: d3ptl2(wkvar,location); break; \
      case 3: d3ptl3(wkvar,location); break; \
      case 4: d3ptl4(wkvar,location); break; }

/*---------------------------------------------------------------------*
*         Macros for dealing with high-order bytes of a word           *
*---------------------------------------------------------------------*/

#define d3gth1(wkvar,location) \
      (wkvar = *(byte *)(location) << 24)

#define d3pth1(wkvar,location) \
      (*(byte *)(location) = (byte)((unsigned)wkvar >> 24))

#define d3gth2(wkvar,location) (wkvar = 0, \
      *((byte *)&wkvar+WO(0)) = *((byte *)(location)+MO(0,1)), \
      *((byte *)&wkvar+WO(1)) = *((byte *)(location)+MO(1,1)))

#define d3pth2(wkvar,location) ( \
      *((byte *)(location)+MO(0,1)) = *((byte *)&wkvar+WO(0)), \
      *((byte *)(location)+MO(1,1)) = *((byte *)&wkvar+WO(1)))

#define d3gth3(wkvar,location) (wkvar = 0, \
      *((byte *)&wkvar+WO(0)) = *((byte *)(location)+MO(0,2)), \
      *((byte *)&wkvar+WO(1)) = *((byte *)(location)+MO(1,2)), \
      *((byte *)&wkvar+WO(2)) = *((byte *)(location)+MO(2,2)))

#define d3pth3(wkvar,location) ( \
      *((byte *)(location)+MO(0,2)) = *((byte *)&wkvar+WO(0)), \
      *((byte *)(location)+MO(1,2)) = *((byte *)&wkvar+WO(1)), \
      *((byte *)(location)+MO(2,2)) = *((byte *)&wkvar+WO(2)))

#define d3gth4(wkvar,location) ( \
      *((byte *)&wkvar+WO(0)) = *((byte *)(location)+MO(0,3)), \
      *((byte *)&wkvar+WO(1)) = *((byte *)(location)+MO(1,3)), \
      *((byte *)&wkvar+WO(2)) = *((byte *)(location)+MO(2,3)), \
      *((byte *)&wkvar+WO(3)) = *((byte *)(location)+MO(3,3)))

#define d3pth4(wkvar,location) ( \
      *((byte *)(location)+MO(0,3)) = *((byte *)&wkvar+WO(0)), \
      *((byte *)(location)+MO(1,3)) = *((byte *)&wkvar+WO(1)), \
      *((byte *)(location)+MO(2,3)) = *((byte *)&wkvar+WO(2)), \
      *((byte *)(location)+MO(3,3)) = *((byte *)&wkvar+WO(3)))

#define d3gthn(wkvar,location,nmbyts) switch(nmbyts) { \
      case 1: d3gth1(wkvar,location); break; \
      case 2: d3gth2(wkvar,location); break; \
      case 3: d3gth3(wkvar,location); break; \
      case 4: d3gth4(wkvar,location); break;}

#define d3pthn(wkvar,location,nmbyts) switch(nmbyts) { \
      case 1: d3pth1(wkvar,location); break; \
      case 2: d3pth2(wkvar,location); break; \
      case 3: d3pth3(wkvar,location); break; \
      case 4: d3pth4(wkvar,location); break; }

/*---------------------------------------------------------------------*
*     Macros for dealing with words that are known to be aligned       *
*---------------------------------------------------------------------*/

#if D3MEMACC_BYTE_ORDER == BYTE_ORDRE
#define d3gti4(wkvar,location) (wkvar = *(long *)location)
#define d3pti4(wkvar,location) (*(long *)location = wkvar)
#else
#define d3gti4(wkvar,location) ( \
      *((byte *)&wkvar+WO(0)) = *((byte *)(location)+MO(0,3)), \
      *((byte *)&wkvar+WO(1)) = *((byte *)(location)+MO(1,3)), \
      *((byte *)&wkvar+WO(2)) = *((byte *)(location)+MO(2,3)), \
      *((byte *)&wkvar+WO(3)) = *((byte *)(location)+MO(3,3)))
#define d3pti4(wkvar,location) ( \
      *((byte *)(location)+MO(0,3)) = *((byte *)&wkvar+WO(0)), \
      *((byte *)(location)+MO(1,3)) = *((byte *)&wkvar+WO(1)), \
      *((byte *)(location)+MO(2,3)) = *((byte *)&wkvar+WO(2)), \
      *((byte *)(location)+MO(3,3)) = *((byte *)&wkvar+WO(3)))
#endif
