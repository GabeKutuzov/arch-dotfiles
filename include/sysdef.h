/* (c) Copyright 1989-2017, The Rockefeller University *11115* */
/* $Id: sysdef.h 61 2017-04-13 20:31:56Z  $ */
/***********************************************************************
*                                                                      *
*                           S Y S D E F . H                            *
*                                                                      *
***********************************************************************/

/* This file contains definitions of machine- and operating system-
*  specific parameters AND THEN SOME... The particular system to be
*  defined should be specified by a compile-time -D option from the
*  first column of the following list:
*
*  SYSTEM NAME    ARCHITECTURE    OP SYS              COMPILER
*  ------ ----    ------------    -- ---              --------
*    IBMMVS          IBM370        MVS                 IBMCC
*    IBMVM           IBM370        VM                  IBMCC
*    MACOS64         INTEL64,BIT64 OSX,UNIX            GCC,XCODE
*    PCLINUX         INTEL32       LINUX,UNIX,BSD      GCC
*    PCLUX64         INTEL64,BIT64 LINUX,UNIX,BSD      GCC
*    SUN4            SPARC         SUNOS,UNIX,BSD      ACC
*    SUN5            SPARC         SOLARIS,UNIX,SVR4   SWSCC
*
*  Note that at this time, PCLUX64 and MACOS64 are defined as BIT64,
*  meaning the memory model is that pointers are 64 bit.  Most likely,
*  longs are also 64-bit, but we define types sbig,ubig that are
*  guaranteed to be 64 bits when BIT64 is defined, otherwise 32 bits
*  (probably equivalent to size_t).
*/

/* V1A, 01/27/89, G.N.Reeke
*  V2A, 04/19/89, JWT and GNR - Implement above naming scheme
*  V2D, 08/10/89, GNR - Add BYTE_ALIGN
*  V2F, 05/24/91, GNR - Add OSK (Oettle+Reichler) defs
*  V2G, 02/28/92, GNR - Add ACC (SUN4 ANSI C compiler) defs
*  V2H, 08/08/92, GNR - Add SRA macro
*  V2I, 11/11/92, GNR - Add ALIGN_TYPE
*  V2J, 02/05/93, ABP - Remove Sun386, add transputer(s)
*  V2K, 08/09/93, ABP - Add STRUCT_ALIGN, topology defs
*  Rev, 04/19/94, GNR - #undef INT_MAX etc. to eliminate conflicts,
*                       remove 'L' suffix from all INT_MAX defs
*  Rev, 11/30/96, GNR - Add 'UNIX', delete T9000
*  Rev, 05/07/97, GNR - Add O2000, R10K32, IRIX, SGICC
*  Rev, 10/08/97, GNR - Add BSIZE, SHSIZE, LSIZE, RSIZE, DSIZE
*  Rev, 02/02/98, GNR - Add SUN5, BSD, SOLARIS, SVR4, SWSCC
*  Rev, 10/18/98, GNR - Add ISIZE, PSIZE, make all xSIZE compile-time
*  Rev, 01/01/99, GNR - Add BITSPERBYTE, BITSPERLONG (09/20/03)
*  Rev, 04/18/99, GNR - Add PCLINUX, BYTE_ORDER-->BYTE_ORDRE for Linux,
*                       add HAS_I64, INTEL32, labs macro, remove CLCC
*  Rev, 09/04/99, GNR - Remove support for NCUBE, add WSIZE
*  Rev, 12/12/02, GNR - Add OSXGCC and OSXMWCC
*  Rev, 02/22/03, GNR - Add si16,ui16,si32,ui32
*  Rev, 02/07/06, GNR - Add PCLUX64
*  Rev, 03/01/07, GNR - Add MAX_HOST_NAME_LEN and MAX_FQDN_LENGTH
*  Rev, 08/18/07, GNR - Remove PC, DOS, TURBOC, XP8T, TCX
*  ==>, 03/22/08, GNR - Last date before committing to svn repository
*  Rev, 08/23/08, GNR - Add abs32 macro, BITSPERSHORT, BITSPERUI32
*  Rev, 12/27/08, GNR - PCLINUX HAS_I64, si32,ui32 depend on LSIZE
*  Rev, 01/01/09, GNR - Add conditional on NO_I64 for testing
*  Rev, 04/20/09, GNR - Remove O2000, R10K32, IRIX, SGICC
*  Rev, 07/07/09, GNR - Add abexit_error class for C++ users
*  Rev, 11/30/12, GNR - Add defs for SI64_MAX and UI64_MAX
*  Rev, 03/04/13, GNR - Better logic for abs32() definition
*  Rev, 06/09/13, GNR - Add abexitq(), abexitmq()
*  Rev, 08/28/13, GNR - Add sbig,ubig types, abs64, remove OSK
*  Rev, 10/25/13, GNR - Add LONG_SGN, SI64_SGN, SI32_SGN, UI32_SGN
*  Rev, 05/26/14, GNR - typedef si32,ui32 before use (NO_I64 jcsw)
*  Rev, 05/29/14, GNR - Use jnsw for NO_I64 abs64
*  Rev, 10/09/14, GNR - Add SI64_01, UI64_01, qvar(x)
*  Rev, 12/04/15, GNR - Remove OSXGCC, OSXMWCC, PPCG4, add MACOS64
*  Rev, 12/11/15, GNR - Add ulng.  02/04/16, add qqvar(), ZSIZE==STSIZE
*  R46, 02/04/17, GNR - Delete ICC, INMOS8, XP8I support
*/

#ifndef __SYSDEF__
#define __SYSDEF__

/***********************************************************************
*                                                                      *
*           Architecture, Op-Sys, and Compiler Definitions             *
*                                                                      *
***********************************************************************/

#ifdef SYSTEM_RECOGNIZED
#undef SYSTEM_RECOGNIZED
#endif

#ifdef IBMMVS                 /* IBM 370 running MVS */
#define SYSTEM_RECOGNIZED
#define IBM370
#define MVS
#define IBMCC
#define RFM_FOPEN
#endif

#ifdef IBMVM                  /* IBM 370 running VM */
#define SYSTEM_RECOGNIZED
#define IBM370
#define VM
#define IBMCC
#define RFM_FOPEN
#endif

#ifdef MACOS64                /* Mac Intel with OSX and XCODE */
#define SYSTEM_RECOGNIZED
#define INTEL64
#define BIT64
#define UNIX
#define OSX
#define XCODE
#define GCC
#define RFM_OPEN              /* Not tested */
#endif

#ifdef PCLINUX                /* LINUX running on INTEL PC */
#define SYSTEM_RECOGNIZED
#define INTEL32
#define LINUX
#define UNIX
#define BSD
#define GCC
#define RFM_OPEN
#define h_addr h_addr_list[0] /* for backward compatibility */
#endif

#ifdef PCLUX64                /* 64-bit LINUX running on INTEL PC */
#define SYSTEM_RECOGNIZED
#define BIT64
#define INTEL64
#define LINUX
#define UNIX
#define BSD
#define GCC
#define RFM_OPEN
#define h_addr h_addr_list[0] /* for backward compatibility */
#endif

#ifdef SUN4                   /* SUN 4 */
#define SYSTEM_RECOGNIZED
#define SPARC
#define SUNOS
#define UNIX
#define BSD
#define ACC
#define RFM_FDOPEN
#endif

#ifdef SUN5                   /* SUN 5 (i.e. Solaris 2) */
#define SYSTEM_RECOGNIZED
#define SPARC
#define SOLARIS
#define UNIX
#define SVR4
#define SWSCC
#define RFM_FDOPEN
#endif

/* KLUGE ALERT: Due to indefinite support of the "#error" construct on
*               certain machines the following (hopefully) non-existent
*               file is conditionally included to flag the lack of a
*               defined system at pre-processor time...    */
#ifndef SYSTEM_RECOGNIZED
#include <NO_SYSTEM_DEFINED.OOPS>
#endif

/* Remove definitions that may conflict with limits.h
*  (The #ifdefs eliminate warnings with some compilers) */
#ifdef BYTE_MAX
#undef BYTE_MAX
#endif
#ifdef SHRT_MAX
#undef SHRT_MAX
#endif
#ifdef INT_MAX
#undef INT_MAX
#endif
#ifdef LONG_MAX
#undef LONG_MAX
#endif

/***********************************************************************
*                                                                      *
*                     Common Constants and Macros                      *
*                                                                      *
***********************************************************************/

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE  1
#endif

#ifndef OFF
#define OFF   0
#endif
#ifndef ON
#define ON    1
#endif

#ifndef NO
#define NO    0
#endif
#ifndef YES
#define YES   1
#endif

typedef signed char schr;
typedef unsigned char byte;
typedef unsigned long ulng;

/* Sizes of various data types--defined as constants, not as
*  sizeof(), so as to be available at preprocessor time.
*  May be redefined below for specific systems.
*  (Sizes used in messages are defined in swap.h) */

#define PSIZE          4      /* Size of a pointer */
#define ZSIZE          4      /* Size of a size_t */
#define STSIZE         4      /* Size of a size_t (compatibility) */
#define DSIZE          8      /* Size of a double */
#define ESIZE          4      /* Size of a float */
#define WSIZE          8      /* Size of a long long, si64, ui64 */
#define LSIZE          4      /* Size of a long */
#define JSIZE          4      /* Size of an si32 or ui32 */
#define I32SIZE        4      /* Size of an si32 or ui32 */
#define ISIZE          4      /* Size of an integer */
#define HSIZE          2      /* Size of a short */
#define SHSIZE         2      /* (Compatibility) */
#define BSIZE          1      /* Size of a byte */
#define BITSPERBYTE    8      /* Bits in a byte */
#define BITSPERSHORT  16      /* Bits in a short */
#define BITSPERUI32   32      /* Bits in a ui32 */
#define BITSPERLONG   32      /* Bits in a long */
#define BITSPERUI64   64      /* Bits in a ui64 */

#define CDSIZE       160      /* Longest input card record */
#define LNSIZE       133      /* Length of longest line that can be
                                 printed (including carriage cntrl) */
#define qCDSIZE     "160"     /* In quotes for use in strings */
#define qLNSIZE     "133"
#define MAX_HOST_NAME_LEN  63 /* Maximum length of one host name */
#define MAX_FQDN_LENGTH   255 /* Maximum length of full domain name */

/* Definitions relating to representation of fixed-point numbers.
*  These can be redefined below if different on specific systems.  */

#define SCHR_MAX  0x7F        /* Largest signed byte */
#define BYTE_MAX  0xFF        /* Largest unsigned byte */
#define SHRT_MAX  0x7FFF      /* Largest short integer */
#define UI16_MAX  0xFFFF      /* Largest unsigned 16-bit integer */
#define INT_MAX   0x7FFFFFFF  /* Largest integer */
#define LONG_MAX  0x7FFFFFFFL /* Largest long integer */
#define SI32_MAX  0x7FFFFFFF  /* Largest signed 32-bit integer */
#define UI32_MAX  0xFFFFFFFFU /* Largest unsigned 32-bit integer */
#define UI32_SIZE     10      /* Number of decimal digits needed to
                                 represent a 32-bit integer fully */
#define LONG_SIZE     10      /* Number of decimal digits needed to
                                 represent a long integer fully */
#define WIDE_SIZE     20      /* Number of decimal digits needed to
                                 represent a 64-bit integer fully */

/* Definitions relating to representation of floating-point numbers.
*  These values are for IEEE standard--redefine below if different.  */

#define EXP_SIZE       4      /* Number of decimal digits, including
                                 a sign, needed to represent exponent
                                 of largest double float number */
#define FLT_EXP_MAX   38      /* Largest decimal exponent (float) */
#define DBL_EXP_MAX  308      /* Largest decimal exponent (double) */
#define OUT_SIZE      16      /* Number of decimal digits needed to
                                 represent a double float number to
                                 full accuracy */

/***********************************************************************
*                                                                      *
*                   Architecture-Dependent Symbols                     *
*                                                                      *
***********************************************************************/

/*---------------------------------------------------------------------*
*                               IBM370                                 *
*---------------------------------------------------------------------*/
#ifdef IBM370
#define BYTE_ORDRE     1      /* BYTE_ORDRE = +1 if variables stored
                                 high order first, -1 if stored low
                                 order first */
#define ALIGN_TYPE     1      /* TRUE if address alignment on multiple
                                 of item size is optimal or required */
#define BYTE_ALIGN     8      /* If ALIGN_TYPE=0, address alignment to
                                 avoid performance penalties.  If
                                 ALIGN_TYPE=1, worst case alignment */
#define STRUCT_ALIGN   0      /* If 1, align struct on multiple of its
                                 largest element, if > 1, align on that
                                 size regardless of member sizes */
#undef  EXP_SIZE
#define EXP_SIZE       3      /* Number of decimal digits, including
                                 a sign, needed to represent exponent
                                 of largest double float number */
#undef  FLT_EXP_MAX
#define FLT_EXP_MAX   75      /* Largest decimal exponent (float) */
#undef  DBL_EXP_MAX
#define DBL_EXP_MAX   75      /* Largest decimal exponent (double) */
#endif /* End IBM370 */

/*---------------------------------------------------------------------*
*                    INTEL 80x86 (in 32-bit mode)                      *
*---------------------------------------------------------------------*/
#ifdef INTEL32
#define BYTE_ORDRE    -1      /* Variables stored low-order first */
#define ALIGN_TYPE     0      /* TRUE if address alignment on multiple
                                 of item size is optimal or required */
#define BYTE_ALIGN     4      /* If ALIGN_TYPE=0, address alignment to
                                 avoid performance penalties.  If
                                 ALIGN_TYPE=1, worst case alignment */
#define STRUCT_ALIGN   1      /* If 1, align struct on multiple of its
                                 largest element, if > 1, align on that
                                 size regardless of member sizes */
#ifndef NO_I64                /* For building test codes */
#define HAS_I64        1      /* Defined if has 64-bit arithmetic */
#endif
#endif /* End INTEL32 */

/*---------------------------------------------------------------------*
*                    INTEL 80x86 (in 64-bit mode)                      *
*---------------------------------------------------------------------*/
#ifdef INTEL64
#undef  PSIZE
#define PSIZE          8      /* Size of a pointer */
#undef  STSIZE
#define STSIZE         8      /* Size of a size_t variable */
#undef  ZSIZE
#define ZSIZE          8      /* Size of a size_t variable */
#undef  LSIZE
#define LSIZE          8
#undef  BITSPERLONG
#define BITSPERLONG   64      /* Bits in a long */
#undef  LONG_MAX
#define LONG_MAX 0x7FFFFFFFFFFFFFFFL
#undef  LONG_SIZE
#define LONG_SIZE     20      /* Chars to represent a long */
#define BYTE_ORDRE    -1      /* Variables stored low-order first */
#define ALIGN_TYPE     0      /* TRUE if address alignment on multiple
                                 of item size is optimal or required */
#define BYTE_ALIGN     8      /* If ALIGN_TYPE=0, address alignment to
                                 avoid performance penalties.  If
                                 ALIGN_TYPE=1, worst case alignment */
#define STRUCT_ALIGN   1      /* If 1, align struct on multiple of its
                                 largest element, if > 1, align on that
                                 size regardless of member sizes */
#define HAS_I64        1      /* Defined if has 64-bit arithmetic */
#endif /* End INTEL64 */

/*---------------------------------------------------------------------*
*                                SPARC                                 *
*---------------------------------------------------------------------*/
#ifdef SPARC
#define BYTE_ORDRE     1      /* Variables stored high-order first */
#define ALIGN_TYPE     1      /* TRUE if address alignment on multiple
                                 of item size is optimal or required */
#define BYTE_ALIGN     8      /* If ALIGN_TYPE=0, address alignment to
                                 avoid performance penalties.  If
                                 ALIGN_TYPE=1, worst case alignment */
#define STRUCT_ALIGN   0      /* If 1, align struct on multiple of its
                                 largest element, if > 1, align on that
                                 size regardless of member sizes */
#endif /* End SPARC */


/***********************************************************************
*                                                                      *
*                 Operating-System-Dependent Symbols                   *
*                                                                      *
***********************************************************************/

/* N.B.  LNSIZE, the maximum length of a printed line, really depends
*  on the printer, not the operating system.  The user can manipulate
*  RK.pgcls and RK.ttcls to modify the behavior of the ROCKS package
*  according to the devices in use.  LNSIZE as defined here should be
*  considered as the maximum line length for operator messages.  */

/*---------------------------------------------------------------------*
*                                 MVS                                  *
*---------------------------------------------------------------------*/
#ifdef MVS
#define LFILNM        45      /* One plus length of longest legal
                                 file path name */
#endif /* End MVS */

/*---------------------------------------------------------------------*
*                                 VM                                   *
*---------------------------------------------------------------------*/
#ifdef VM
#define LFILNM        45      /* NOTE: Deliberately defined same as
                                 MVS so object code can be exchanged
                                 between MVS and VM--actual LFILNM
                                 under CMS is 20 */
#endif /* End VM */

/*---------------------------------------------------------------------*
*                                UNIX                                  *
*---------------------------------------------------------------------*/
#ifdef LINUX
#define LFILNM       257
#elif defined(UNIX)
#define LFILNM        67      /* One plus length of longest legal
                                 file path name under this OS */
#endif /* End UNIX */

/*---------------------------------------------------------------------*
*                                 OSX                                  *
*---------------------------------------------------------------------*/
#ifdef OSX
#define LFILNM        67      /* One plus length of longest legal
                                 file path name under this OS */
#endif /* End UNIX */

/***********************************************************************
*                                                                      *
*                   Compiler-Dependent Definitions                     *
*                                                                      *
***********************************************************************/

#if defined ACC || defined IBMCC || defined GCC || defined XCODE || \
    defined SWSCC
#define ANSI_C
#endif

/* Define 16- and 32-bit arithmetic types */
typedef short si16;
typedef unsigned short ui16;
#if LSIZE == 8
typedef int si32;
typedef unsigned int ui32;
#define LONG_SGN 0x8000000000000000L
#define SI32_SGN 0x80000000
#define UI32_SGN 0x80000000U
#else
typedef long si32;
typedef unsigned long ui32;
#define LONG_SGN 0x80000000L
#define SI32_SGN 0x80000000L
#define UI32_SGN 0x80000000UL
#endif

/* Define arithmetic types that vary with 32- vs 64-bit compilations */
#ifdef BIT64
#if LSIZE == 8
typedef long sbig;
typedef unsigned long ubig;
#define SBIG_MAX 0x7FFFFFFFFFFFFFFFL
#define UBIG_MAX 0xFFFFFFFFFFFFFFFFUL
#else
typedef long long sbig;
typedef unsigned long long ubig;
#define SBIG_MAX 0x7FFFFFFFFFFFFFFFLL
#define UBIG_MAX 0xFFFFFFFFFFFFFFFFULL
#endif
#else /* !BIT64 */
typedef long sbig;
typedef unsigned long ubig;
#define SBIG_MAX 0x7FFFFFFFL
#define UBIG_MAX 0xFFFFFFFFUL
#endif /* BIT64 */

/* Define 64-bit arithmetic types (following nxdr rule) */
#ifdef HAS_I64
#if LSIZE == 8
typedef long si64;
typedef unsigned long ui64;
#define SI64_MAX 0x7FFFFFFFFFFFFFFFL
#define SI64_SGN 0x8000000000000000L
#define SI64_01  0x0000000000000001L
#define UI64_MAX 0xFFFFFFFFFFFFFFFFUL
#define UI64_01  0x0000000000000001UL
#else
typedef long long si64;
typedef unsigned long long ui64;
#define SI64_MAX 0x7FFFFFFFFFFFFFFFLL
#define SI64_SGN 0x8000000000000000LL
#define SI64_01  0x0000000000000001LL
#define UI64_MAX 0xFFFFFFFFFFFFFFFFULL
#define UI64_01  0x0000000000000001ULL
#endif
#else /* !HAS_I64 */
#if BYTE_ORDRE < 0
struct tsi64 { unsigned long lo; long hi; };
struct tui64 { unsigned long lo; unsigned long hi; };
#else
struct tsi64 { long hi; unsigned long lo; };
struct tui64 { unsigned long hi; unsigned long lo; };
#endif
typedef struct tsi64 si64;
typedef struct tui64 ui64;
si64 jcsw(si32 hi, ui32 lo);
ui64 jcuw(ui32 hi, ui32 lo);
si64 jnsw(si64 x);
#define SI64_MAX jcsw(SI32_MAX, UI32_MAX)
#define SI64_01  jcsw(0,1)
#define UI64_MAX jcuw(UI32_MAX, UI32_MAX)
#define UI64_01  jcuw(0,1)
#endif /* HAS_I64 */

/* Define 2-part, 64-bit seed with separate struct for nxdr2.
*  Note:  These are si32, not ui32, for calcs in wdevcom.h.  */
struct wseed_type {
   si32 seed27;               /* (Neg for compat mode) */
   si32 seed31;
   };
typedef struct wseed_type wseed;

/* Define a structure that can be used to store x,y coordinates */
struct xyf_t {
   float x,y;
   };
typedef struct xyf_t xyf;

/* Define a macro which is guaranteed to perform an arithmetic
*  right shift on a signed quantity.  (The ANSI C standard permits
*  compilers to generate either an arithmetic or a logical shift.)
*  To be conservative, use the long form for unknown compilers.
*  N.B. Both arguments should be simple variables or expressions
*  with no side effects, as they may be evaluated twice.  */
#if defined ACC || defined GCC || /* defined XCODE || */ defined SWSCC
#define SRA(x,s) ((x)>>(s))
#else
#define SRA(x,s) ( ((x) >= 0) ? ((x)>>(s)) : (~(~(x)>>(s))) )
#endif

/* Define abs32(),abs64() as functions that take the absolute value
*  of a 32-bit, bzw. 64-bit integer.  On compilers that do not have
*  labs(),llabs() (or status not determined) implement as macros */
#if !defined GCC && !defined XCODE && !defined ACC
#define labs(l)  (((l) < 0) ? -l : l)
#define llabs(l) (((l) < 0) ? -l : l)
#endif

#ifdef HAS_I64
#if LSIZE == 8
#define abs32(x) abs(x)
#define abs64(x) labs(x)
#else
#define abs32(x) labs(x)
#define abs64(x) llabs(x)
#endif
#else
#define abs32(x) abs(x)
#define abs64(x) (x.hi >= 0 ? x : jnsw(x))
#endif

/* On compilers that do not have fabsf or sqrtf,
*  equivalence to fabs, sqrt, bzw.  */
#if !defined GCC /* && !defined XCODE */
#define fabsf(x) fabs(x)
#define sqrtf(x) sqrt(x)
#endif

/***********************************************************************
*                                                                      *
*                 Commonly Used Prototypes And Macros                  *
*                                                                      *
***********************************************************************/

#if BITSPERBYTE == 8          /* Always TRUE? */
#define ByteOffset(n) ((n) >> 3)
#define Bytes2Bits(n) ((n) << 3)
#define BitRemainder(n) ((n) & 7)
#define BytesInXBits(x) (((x)+7)>>3)
#else
#define ByteOffset(n) ((n) / BITSPERBYTE)
#define Bytes2Bits(n) ((n) * BITSPERBYTE)
#define BitRemainder(n) (n) % BITSPERBYTE)
#define BytesInXBits(x) (((x) + BITSPERBYTE - 1)/BITSPERBYTE)
#endif

#ifndef qqvar
#define qqvar(x) qvar(x)
#endif
#ifndef qvar
#define qvar(x) #x
#endif

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

/* Align data size to next higher multiple of BYTE_ALIGN.
*  N.B. ALIGN_UP cannot legally be applied to pointers and
*  must not be used for 64-bit arguments in 32-bit systems.  */

#define ALIGN_UP(s) (((s)+(BYTE_ALIGN-1))&~(BYTE_ALIGN-1))

/* Prototypes of essential ROCKS routines often used alone */

#ifdef __cplusplus
extern "C" {
#endif
void abexitq(int code);
void abexitmq(int code, char *emsg);
#ifdef GCC
void abexit(int code) __attribute__ ((noreturn));
void abexitm(int code, char *emsg) __attribute__ ((noreturn));
void abexitme(int code, char *emsg) __attribute__ ((noreturn));
char *ssprintf(char *string, char *format, ...);
#elif defined ANSI_C
void abexit(int code);
void abexitm(int code, char *emsg);
void abexitme(int code, char *emsg);
char *ssprintf(char *string, char *format, ...);
#else
void abexit();
void abexitm();
void abexitme();
char *ssprintf();
#endif
#ifdef __cplusplus
   }
struct abexit_error {
   int ierr;
   abexit_error(int ii) { ierr = ii; } /* Constructor */
   void abexit_catcher(abexit_error err) { abexit(err.ierr); }
   };
#endif

#endif  /* ifndef __SYSDEF__ */

