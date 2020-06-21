/* (c) Copyright 1988-2017, The Rockefeller University *11116* */
/* $Id: d3memacc.h 76 2017-12-12 21:17:50Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                        D3MEMACC Header File                          *
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
*     changed.                                                         *
*                                                                      *
*  Therefore, D3MEMACC_BYTE_ORDER is not necessarily equal to          *
*     BYTE_ORDRE as defined in sysdef.h for a particular cpu.          *
*                                                                      *
*  The system for naming the macros has been expanded, 03/02/13, in    *
*     anticipation of the changeover of longs to [su]i32s for 64-bit   *
*     compilations.  The new macros that assume 32-bit arguments are   *
*     of the form "d3xxj[hls][1234n][a] where 'xx' indicates the       *
*     direction of transfer ['pt' for put to memory, 'gt' for get      *
*     from memory], 'j' indicates 32-bit variable, 'h' indicates       *
*     transfer high-order bits (signed/unsigned does not matter),      *
*     'l' indicates transfer low-order bits without sign extension,    *
*     's' indicates transfer low-order bits with sign extension (only  *
*     needed with 'gt'), [1234n] indicates number of bytes to transfer *
*     (where 'n' indicates variable length is an argument), and 'a'    *
*     indicates that memory alignment to type length is guaranteed.    *
*                                                                      *
*  R67, 12/08/16, also added a macros d3ptms2 to save and d3gtms2 to   *
*     restore 16-bit values directly to/from 16-bits in storage.       *
*                                                                      *
*  The old macros that assume long arguments are retained unchanged.   *
*     The names of these macros are the same as above without the      *
*     'j' and guaranteed alignment is indicated with type letter 'i'   *
*     instead of the 'a' at the end of the macro name.                 *
*     Old macros will be removed when no longer needed.                *
*                                                                      *
*  d3pt[j]l1(wkvar,location)                                           *
*     Store the low order byte of 'wkvar' in 'location'.               *
*                                                                      *
*  d3gt[j]l1(wkvar,location)                                           *
*     Assign the value of the byte pointed to by 'location' to the     *
*     low order byte of 'wkvar'.  Zero the remaining bytes of 'wkvar'. *
*                                                                      *
*  d3gtjs1(wkvar,location)                                             *
*     Assign the signed value of the char pointed to by 'location' to  *
*     the signed long word at 'wkvar'.                                 *
*                                                                      *
*  d3pt[j]l2[a](wkvar,location)                                        *
*     Load the 2-byte unit pointed to by 'location' with the value of  *
*     the low order two bytes of 'wkvar'.                              *
*                                                                      *
*  d3gt[j]l2(wkvar,location)                                           *
*     Assign the value of the 2-byte unit pointed to by 'location'     *
*     to the low order two bytes of 'wkvar'.  Zero the other bytes     *
*     in 'wkvar'.                                                      *
*                                                                      *
*  d3gtjs2[a](wkvar,location)                                          *
*     Assign the signed value of the 2-byte unit pointed to by         *
*     'location' to the signed long word at 'wkvar'.                   *
*                                                                      *
*  d3pt[j]l3(wkvar,location)                                           *
*     Load the 3-byte unit pointed to by 'location' with the value of  *
*     the low order 3 bytes of 'wkvar'.                                *
*                                                                      *
*  d3gt[j]l3(wkvar,location)                                           *
*     Assign the value of the 3-byte unit pointed to by 'location' to  *
*     the low order 3 bytes of 'wkvar'. Zero the other byte of 'wkvar'.*
*                                                                      *
*  d3pt[j]l4(wkvar,location)                                           *
*     Load the 4-byte unit pointed to by 'location' with the value of  *
*     'wkvar'.                                                         *
*                                                                      *
*  d3gt[j]l4[a](wkvar,location)                                        *
*     Assign the value of the 4-byte unit pointed to by 'location'     *
*     to 'wkvar'.                                                      *
*                                                                      *
*  d3pt[j]ln(wkvar,location,nmbyts)                                    *
*     Load the the nmbyts bytes pointed to by 'location' with the low  *
*     order nmbyts bytes of 'wkvar'.                                   *
*                                                                      *
*  d3gt[j]ln(wkvar,location,nmbyts)                                    *
*     Assign the low order nmbyts bytes of 'wkvar' the value of the    *
*     nmbyts bytes pointed to by 'location'.  Zero the remaining       *
*     bytes of 'wkvar'.                                                *
*                                                                      *
*  d3pt[j]h1(wkvar,location)                                           *
*     Load the byte pointed to by 'location' with the value of the     *
*     high order byte of 'wkvar'.                                      *
*                                                                      *
*  d3gt[j]h1(wkvar,location)                                           *
*     Assign the value of the byte pointed to by 'location' to the     *
*     high order byte of 'wkvar'.  Zero the other bytes in 'wkvar'.    *
*                                                                      *
*  d3pt[j]h2[a](wkvar,location)                                        *
*     Load the 2-byte unit pointed to by 'location' with the value of  *
*     the high order two bytes of 'wkvar'.                             *
*                                                                      *
*  d3gt[j]h2[a](wkvar,location)                                        *
*     Assign the value of the 2-byte unit pointed to by 'location'     *
*     to the high order two bytes of 'wkvar'.  Zero the other bytes    *
*     in 'wkvar'.                                                      *
*                                                                      *
*  d3pt[j]h3(wkvar,location)                                           *
*     Load the 3-byte unit pointed to by 'location' with the value of  *
*     high order 3 bytes of 'wkvar'.                                   *
*                                                                      *
*  d3gt[j]h3(wkvar,location)                                           *
*     Assign the value of the 3-byte unit pointed to by 'location' to  *
*     the high order 3 bytes of 'wkvar'.  Zero the other byte of       *
*     'wkvar'.                                                         *
*                                                                      *
*  d3pt[j]h4(wkvar,location)                                           *
*     Load the 4-byte unit pointed to by 'location' with the value of  *
*     'wkvar' [same as d3pt[j]l4].                                     *
*                                                                      *
*  d3gt[j]h4(wkvar,location)                                           *
*     Assign the value of the 4-byte unit pointed to by 'location'     *
*     to 'wkvar' [same as d3gt[j]l4].                                  *
*                                                                      *
*  d3pt[j]hn(wkvar,location,nmbyts)                                    *
*     Load the the nmbyts bytes pointed to by 'location' with the high *
*     order nmbyts bytes of 'wkvar'.                                   *
*                                                                      *
*  d3gt[j]hn(wkvar,location,nmbyts)                                    *
*     Assign the high order nmbyts bytes of 'wkvar' the value of the   *
*     nmbyts bytes pointed to by 'location'.  Zero the remaining       *
*     bytes of 'wkvar'.                                                *
*                                                                      *
*  d3ptil2(wkvar,location)                                             *
*     Load the 2-byte unit pointed to by 'location' with the value of  *
*     the low order two bytes of 'wkvar'.                              *
*                                                                      *
*  d3gtis2(wkvar,location)                                             *
*     Assign the signed value of the 2-byte unit pointed to by         *
*     'location' to the signed si32 word at 'wkvar'.                   *
*                                                                      *
*       'wkvar'    -- A long or si32 type variable name. The address   *
*                       operator is applied to 'wkvar' in some of the  *
*                       macros, so it must be an 'lvalue'.             *
*       'location' -- An expression whose value is a byte pointer      *
*                       that points to the first location in memory    *
*                       to be accessed.                                *
*       'nmbyts'   -- An integer type expression indicating the        *
*                       number of bytes to transfer.  Must be in the   *
*                       range 1-4.                                     *
*                                                                      *
*   Restrictions:                                                      *
*                                                                      *
*     i) 'wkvar' must be the name of a long variable with the old      *
*           macros or an si32 or ui32 variable with the new macros.    *
*           In general, illegal code will result from failure to       *
*           comply with these restrictions.                            *
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
************************************************************************
*  V4A, 12/24/88, Initial version                                      *
*  Rev, 09/28/92, GNR - Store all data in little-endian order          *
*  V8C, 03/19/03, GNR - Add d3gts2                                     *
*  ==>, 09/25/06, GNR - Last mod before committing to svn repository   *
*  V8E, 08/01/09, GNR - Add d3pti4 and d3gti4                          *
*  V8I, 03/02/13, GNR - Begin implementing d3xxj macros                *
*  R67, 12/08/16, GNR - Add d3gtm2, d3ptm2                             *
*  R76, 12/11/17, GNR - Put a few more locs in parens where needed     *
***********************************************************************/

/* If this definition is changed, d3save, d3rstr, d3gfhd, d3gfsv
*  must be examined--see discussion above.  */
#define D3MEMACC_BYTE_ORDER -1

/* Macro to calculate offset in a word or halfword of a byte
*  according to its magnitude (0=highest,... 3=lowest) */
#if BYTE_ORDRE > 0
#define WO(m) (m)
#define HO(m) (m)
#else
#define WO(m) (3-m)
#define HO(m) (1-m)
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

#define d3gtjl1(var,loc) (var = (ui32)*(byte *)(loc))

#define d3gtjs1(var,loc) (var = (si32)*(byte *)(loc))

#define d3ptjl1(var,loc) (*(byte *)(loc) = (byte)(var))

#define d3gtms2(var,loc) ( \
      *((byte *)&var+HO(0)) = *((byte *)(loc)+MO(0,1)), \
      *((byte *)&var+HO(1)) = *((byte *)(loc)+MO(1,1)))

#define d3ptms2(var,loc) ( \
      *((byte *)(loc)+MO(0,1)) = *((byte *)&var+HO(0)), \
      *((byte *)(loc)+MO(1,1)) = *((byte *)&var+HO(1)))

#define d3gtjl2(var,loc) (var = 0, \
      *((byte *)&var+WO(2)) = *((byte *)(loc)+MO(0,1)), \
      *((byte *)&var+WO(3)) = *((byte *)(loc)+MO(1,1)))

#define d3gtjs2(var,loc) \
      { union { si16 ts; byte tb[2]; } tu; \
      tu.tb[HO(0)] = ((byte *)(loc))[MO(0,1)]; \
      tu.tb[HO(1)] = ((byte *)(loc))[MO(1,1)]; \
      var = (si32)tu.ts; }

#if D3MEMACC_BYTE_ORDER == BYTE_ORDRE
#define d3gtjs2a(var,loc) (var = (si32)(*(si16 *)(loc)))
#else
#define d3gtjs2a(var,loc) d3gtjs2(var,loc)
#endif

#define d3ptjl2(var,loc) ( \
      *((byte *)(loc)+MO(0,1)) = *((byte *)&var+WO(2)), \
      *((byte *)(loc)+MO(1,1)) = *((byte *)&var+WO(3)))

#if D3MEMACC_BYTE_ORDER == BYTE_ORDRE
#define d3ptjl2a(var,loc) (*(short *)(loc) = (short)var)
#else
#define d3ptjl2a(var,loc) d3ptjl2(var,loc)
#endif

#define d3gtjl3(var,loc) (var = 0, \
      *((byte *)&var+WO(1)) = *((byte *)(loc)+MO(0,2)), \
      *((byte *)&var+WO(2)) = *((byte *)(loc)+MO(1,2)), \
      *((byte *)&var+WO(3)) = *((byte *)(loc)+MO(2,2)))

#define d3ptjl3(var,loc) ( \
      *((byte *)(loc)+MO(0,2)) = *((byte *)&var+WO(1)), \
      *((byte *)(loc)+MO(1,2)) = *((byte *)&var+WO(2)), \
      *((byte *)(loc)+MO(2,2)) = *((byte *)&var+WO(3)))

#define d3gtjl4(var,loc) ( \
      *((byte *)&var+WO(0)) = *((byte *)(loc)+MO(0,3)), \
      *((byte *)&var+WO(1)) = *((byte *)(loc)+MO(1,3)), \
      *((byte *)&var+WO(2)) = *((byte *)(loc)+MO(2,3)), \
      *((byte *)&var+WO(3)) = *((byte *)(loc)+MO(3,3)))

#if D3MEMACC_BYTE_ORDER == BYTE_ORDRE
#define d3gtjl4a(var,loc) (var = (*(si32 *)(loc)))
#else
#define d3gtjl4a(var,loc) d3gtjl4(var,loc)
#endif

#define d3ptjl4(var,loc) ( \
      *((byte *)(loc)+MO(0,3)) = *((byte *)&var+WO(0)), \
      *((byte *)(loc)+MO(1,3)) = *((byte *)&var+WO(1)), \
      *((byte *)(loc)+MO(2,3)) = *((byte *)&var+WO(2)), \
      *((byte *)(loc)+MO(3,3)) = *((byte *)&var+WO(3)))

#if D3MEMACC_BYTE_ORDER == BYTE_ORDRE
#define d3ptjl4a(var,loc) (*(si32 *)(loc) = var)
#else
#define d3ptjl4a(wkvar,loc) ( \
      *((byte *)(loc)+MO(0,3)) = *((byte *)&var+WO(0)), \
      *((byte *)(loc)+MO(1,3)) = *((byte *)&var+WO(1)), \
      *((byte *)(loc)+MO(2,3)) = *((byte *)&var+WO(2)), \
      *((byte *)(loc)+MO(3,3)) = *((byte *)&var+WO(3)))
#endif

/*---------------------------------------------------------------------*
*         Macros for dealing with high-order bytes of a word           *
*---------------------------------------------------------------------*/

#define d3gtjh2(var,loc) (var = 0, \
      *((byte *)&var+WO(0)) = *((byte *)(loc)+MO(0,1)), \
      *((byte *)&var+WO(1)) = *((byte *)(loc)+MO(1,1)))

#if D3MEMACC_BYTE_ORDER == BYTE_ORDRE
#define d3gtjh2a(var,loc) (var = ((si32)(*(si16 *)(loc))) << 16)
#else
#define d3gtjh2a(var,loc) d3gtjh2(var,loc)
#endif

#define d3ptjh2(var,loc) ( \
      *((byte *)(loc)+MO(0,1)) = *((byte *)&var+WO(0)), \
      *((byte *)(loc)+MO(1,1)) = *((byte *)&var+WO(1)))

/* Maybe we'll find a better d3ptjh2a later */
#define d3ptjh2a(var,loc) d3ptjh2(var,loc)


/*=====================================================================*
*   Below this point are the 'traditional' macros with long arguments  *
*=====================================================================*/

/*---------------------------------------------------------------------*
*          Macros for dealing with low-order bytes of a word           *
*---------------------------------------------------------------------*/

#define d3gtl1(wkvar,loc) (wkvar = (ui32)*(byte *)(loc))

#define d3ptl1(wkvar,loc) (*(byte *)(loc) = (byte)(wkvar))

#define d3gtl2(wkvar,loc) (wkvar = 0, \
      *((byte *)&wkvar+WO(2)) = *((byte *)(loc)+MO(0,1)), \
      *((byte *)&wkvar+WO(3)) = *((byte *)(loc)+MO(1,1)))

#define d3ptl2(wkvar,loc) ( \
      *((byte *)(loc)+MO(0,1)) = *((byte *)&wkvar+WO(2)), \
      *((byte *)(loc)+MO(1,1)) = *((byte *)&wkvar+WO(3)))

#define d3gtl3(wkvar,loc) (wkvar = 0, \
      *((byte *)&wkvar+WO(1)) = *((byte *)(loc)+MO(0,2)), \
      *((byte *)&wkvar+WO(2)) = *((byte *)(loc)+MO(1,2)), \
      *((byte *)&wkvar+WO(3)) = *((byte *)(loc)+MO(2,2)))

#define d3ptl3(wkvar,loc) ( \
      *((byte *)(loc)+MO(0,2)) = *((byte *)&wkvar+WO(1)), \
      *((byte *)(loc)+MO(1,2)) = *((byte *)&wkvar+WO(2)), \
      *((byte *)(loc)+MO(2,2)) = *((byte *)&wkvar+WO(3)))

#define d3gtl4(wkvar,loc) ( \
      *((byte *)&wkvar+WO(0)) = *((byte *)(loc)+MO(0,3)), \
      *((byte *)&wkvar+WO(1)) = *((byte *)(loc)+MO(1,3)), \
      *((byte *)&wkvar+WO(2)) = *((byte *)(loc)+MO(2,3)), \
      *((byte *)&wkvar+WO(3)) = *((byte *)(loc)+MO(3,3)))

#define d3ptl4(wkvar,loc) ( \
      *((byte *)(loc)+MO(0,3)) = *((byte *)&wkvar+WO(0)), \
      *((byte *)(loc)+MO(1,3)) = *((byte *)&wkvar+WO(1)), \
      *((byte *)(loc)+MO(2,3)) = *((byte *)&wkvar+WO(2)), \
      *((byte *)(loc)+MO(3,3)) = *((byte *)&wkvar+WO(3)))

#define d3gtln(wkvar,loc,nmbyts) switch(nmbyts) { \
      case 1: d3gtl1(wkvar,loc); break; \
      case 2: d3gtl2(wkvar,loc); break; \
      case 3: d3gtl3(wkvar,loc); break; \
      case 4: d3gtl4(wkvar,loc); break; }

#define d3ptln(wkvar,loc,nmbyts) switch(nmbyts) { \
      case 1: d3ptl1(wkvar,loc); break; \
      case 2: d3ptl2(wkvar,loc); break; \
      case 3: d3ptl3(wkvar,loc); break; \
      case 4: d3ptl4(wkvar,loc); break; }

/*---------------------------------------------------------------------*
*         Macros for dealing with high-order bytes of a word           *
*---------------------------------------------------------------------*/

#define d3gth1(wkvar,loc) \
      (wkvar = *(byte *)(loc) << 24)

#define d3pth1(wkvar,loc) \
      (*(byte *)(loc) = (byte)((unsigned)wkvar >> 24))

#define d3gth2(wkvar,loc) (wkvar = 0, \
      *((byte *)&wkvar+WO(0)) = *((byte *)(loc)+MO(0,1)), \
      *((byte *)&wkvar+WO(1)) = *((byte *)(loc)+MO(1,1)))

#define d3pth2(wkvar,loc) ( \
      *((byte *)(loc)+MO(0,1)) = *((byte *)&wkvar+WO(0)), \
      *((byte *)(loc)+MO(1,1)) = *((byte *)&wkvar+WO(1)))

#define d3gth3(wkvar,loc) (wkvar = 0, \
      *((byte *)&wkvar+WO(0)) = *((byte *)(loc)+MO(0,2)), \
      *((byte *)&wkvar+WO(1)) = *((byte *)(loc)+MO(1,2)), \
      *((byte *)&wkvar+WO(2)) = *((byte *)(loc)+MO(2,2)))

#define d3pth3(wkvar,loc) ( \
      *((byte *)(loc)+MO(0,2)) = *((byte *)&wkvar+WO(0)), \
      *((byte *)(loc)+MO(1,2)) = *((byte *)&wkvar+WO(1)), \
      *((byte *)(loc)+MO(2,2)) = *((byte *)&wkvar+WO(2)))

#define d3gth4(wkvar,loc) ( \
      *((byte *)&wkvar+WO(0)) = *((byte *)(loc)+MO(0,3)), \
      *((byte *)&wkvar+WO(1)) = *((byte *)(loc)+MO(1,3)), \
      *((byte *)&wkvar+WO(2)) = *((byte *)(loc)+MO(2,3)), \
      *((byte *)&wkvar+WO(3)) = *((byte *)(loc)+MO(3,3)))

#define d3pth4(wkvar,loc) ( \
      *((byte *)(loc)+MO(0,3)) = *((byte *)&wkvar+WO(0)), \
      *((byte *)(loc)+MO(1,3)) = *((byte *)&wkvar+WO(1)), \
      *((byte *)(loc)+MO(2,3)) = *((byte *)&wkvar+WO(2)), \
      *((byte *)(loc)+MO(3,3)) = *((byte *)&wkvar+WO(3)))

#define d3gthn(wkvar,loc,nmbyts) switch(nmbyts) { \
      case 1: d3gth1(wkvar,loc); break; \
      case 2: d3gth2(wkvar,loc); break; \
      case 3: d3gth3(wkvar,loc); break; \
      case 4: d3gth4(wkvar,loc); break;}

#define d3pthn(wkvar,loc,nmbyts) switch(nmbyts) { \
      case 1: d3pth1(wkvar,loc); break; \
      case 2: d3pth2(wkvar,loc); break; \
      case 3: d3pth3(wkvar,loc); break; \
      case 4: d3pth4(wkvar,loc); break; }

