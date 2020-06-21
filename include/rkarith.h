/* (c) Copyright 1989-2018, The Rockefeller University *11115* */
/* $Id: rkarith.h 65 2018-08-15 19:26:04Z  $ */
/***********************************************************************
*                       RKARITH.H Header File                          *
*  Functions to implement 64-bit arithmetic and/or overflow checking   *
*                                                                      *
************************************************************************
* V1A, 04/19/89, GNR - Initial version                                 *
* Rev, 06/19/99, GNR - Add typedefs and routines for si64, ui64        *
* Rev, 03/22/03, GNR - Add jasl, jasle, jaul, jaule, mssw, msuw, etc.  *
* Rev, 02/06/06, GNR - Add lmulup, umulup, nelmead, nmiter[ns]a        *
* Rev, 01/04/06, GNR - Macros for 64-bit machines, add bitsz           *
* ==>, 03/22/08, GNR - Last date before committing to svn repository   *
* Rev, 11/15/08, GNR - Add bitszul, bitszsl, bitszu32, bitszs32,       *
*                      updates and corrections for 64-bit compilation  *
* Rev, 12/28/08, GNR - Fix jabs def, add mss[lw]e, msrs[lw]e           *
* Rev, 01/15/09, GNR - Add jmuwb                                       *
* Rev, 06/13/09, GNR - Add vdivl                                       *
* Rev, 10/23/10, GNR - Add erfcf                                       *
* Rev, 05/25/12, GNR - Delete redundant dm64nq, dm64nr, jm64nh defs.   *
* Rev, 08/21/12, GNR - Add amuwwe, jmuwwe, uw2le, sw2le, ul2w, sl2w    *
* Rev, 10/18/12, GNR - Begin adding/converting to macros for some ops  *
* Rev, 11/28/12, GNR - ja[su]aem macros return max values on overflow  *
* Rev, 12/01/12, GNR - Add dsrswqe, dsruwqe, dmsjqe, dmujqe,           *
*                      jdswqe, jdswbe, jduwqe, msrsl, msrul            *
* Rev, 03/31/13, GNR - Add swflt, uwflt, swloem, uwloem                *
* Rev, 04/28/13, GNR - Add dbl2swe, dbl2uwe                            *
* Rev, 06/10/13, GNR - Add dsrsjqe, dsrswq, dsrswwje, dsrswwqe,        *
*                      dsruwq, dsruwwje, dsruwwqe, mssl                *
* Rev, 08/22/13, GNR - [su]wloem macros return max values on overflow  *
* Rev, 08/26/13, GNR - Add jmuwje,absb,absj,absm,absw                  *
* Rev, 10/19/13, GNR - Add jckss, jckus, jcksd, jckud                  *
* Rev, 10/20/13, GNR - Add e64dec, e64dac, entire family of routines   *
*                      and macros xxxxd() that use preset error codes  *
* Rev, 10/22/13, GNR - Add jck[su]lo macros                            *
* Rev, 04/06/14, GNR - Make swlo family return si32, swlou for ui32    *
* Rev, 05/25/14, GNR - For symmetry, add jrsld, jrsle, jruld, jrule    *
* Rev, 07/18/14, GNR - Add ui32powe, equate ui32pow, remove deigen     *
* Rev, 09/07/14, GNR - Add ms[su]wj, msr[su]wj, mls[su]w[de],          *
*                      mls[su]wj[de], correct mssl macro               *
* Rev, 09/27/14, GNR - Remove msrsl, msrul                             *
* Rev, 10/06/14, GNR - Add mrs[rsu]x family (assume s >= 0) and        *
*                      ms[rsu]x macros that call those with |s|.       *
* R60, 05/10/16, GNR - Add sw2z[de], uw2z[de], ja[su]lo[de], jauz[de]  *
* R61, 12/02/16, GNR - Add jmswj.  12/27/16, add drswq.                *
* R62, 08/28/17, GNR - Add dmsrwwqd, dmsrwwqe                          *
* R63, 02/21/18, GNR - Add mrsrsl (no overflow checking), mrsrsqd      *
* R69, 08/30/18, GNR - Add dsrswjq (no overflow checking)              *
***********************************************************************/

#ifndef RKARITH_HDR_INCLUDED
#define RKARITH_HDR_INCLUDED

#include <stdlib.h>
#include "sysdef.h"

/* Component extractor and magnitude test macros for 64-bit types */
#ifdef HAS_I64
#define swhi(x) ((si32)SRA((x),BITSPERUI32)) /* Extract high */
#define swlo(x) ((si32)(x))                  /* Extract low  */
#define swlou(x) ((ui32)(x))                 /* Cannot overflow */
#define swdbl(x) ((double)(x))               /* Convert to double */
#define swflt(x) ((float)(x))                /* Convert to float */
#define qsw(x) (x)                           /* Query magnitude */
#define uwhi(x) ((x)>>BITSPERUI32)
#define uwlo(x) ((ui32)(x))
#define uwdbl(x) ((double)(x))
#define uwflt(x) ((float)(x))
#define qcuw(a,b) ((b) > (a))
#define quw(x) (x)
#else
#define swhi(x) x.hi
#define swlo(x) (si32)x.lo
#define swlou(x) x.lo
#define swdbl(x) ((double)x.hi * 4294967296.0 + (double)x.lo)
#define swflt(x) ((float)x.hi * 4294967296.0 + (float)x.lo)
#define qsw(x) (x.hi ? x.hi : (x.lo != 0))
#define uwhi(x) x.hi
#define uwlo(x) x.lo
#define uwdbl(x) ((double)x.hi * 4294967296.0 + (double)x.lo)
#define uwflt(x) ((float)x.hi * 4294967296.0 + (float)x.lo)
#define qcuw(a,b) (b.hi > a.hi || (b.hi == a.hi && b.lo > a.lo))
#define quw(x) (x.hi | x.lo)
#endif

/* Function to preset error action for 64-bit arithmetic */
#ifdef __cplusplus
extern "C" {
#endif
void e64set(unsigned int act, void *p);
#define E64_ABEXIT   0  /* Codes for error actions */
#define E64_COUNT    1
#define E64_FLAGS    2
#define E64_CALL     3
void e64push(unsigned int act, void *p);
void e64pop(void);
void e64dec(int ec);
#define E64_STKDPTH  5  /* Depth of push-down RKX stack */
/* Function to execute error action */
void e64act(void *fnm, unsigned int ec);
void e64dac(void *fnm);
#define EAshft      24  /* Shift to encode action type */
#define EAabx(ec) (((E64_ABEXIT+1)<<EAshft) | (ec))
#define EAct(ec)  (((E64_COUNT+1)<<EAshft) | (ec))
#define EAfl(ec)  (((E64_FLAGS+1)<<EAshft) | (ec))
#define EAcb(ec)  (((E64_CALL+1)<<EAshft) | (ec))
void e64stest(int i_am_test);
int  e64qtest(void);
#ifdef __cplusplus
}
#endif

/* Base 64-bit arithmetic functions */
#ifdef HAS_I64
/* Multiply, shift, add */
#define amlssw(sum,x,y,s) (sum + (((si64)(x)*(si64)(y)) << (s)))
#define amrssw(sum,x,y,s) (sum + (((si64)(x)*(si64)(y)) >> (s)))
#define amlsuw(sum,x,y,s) (sum + (((ui64)(x)*(ui64)(y)) << (s)))
#define amrsuw(sum,x,y,s) (sum + (((ui64)(x)*(ui64)(y)) >> (s)))
/* Concatenate */
#define jcsw(hi,lo) (((si64)(hi)<<BITSPERUI32) | (si64)(ui32)(lo))
#define jcuw(hi,lo) (((ui64)(hi)<<BITSPERUI32) | (ui64)(ui32)(lo))
/* To and from longs */
#define sl2w(x) ((si64)(x))
#define ul2w(x) ((ui64)(x))
#if LSIZE == 8
#define sw2ld(x)    ((long)(x))
#define sw2le(x,ec) ((long)(x))
#define uw2ld(x)    ((unsigned long)(x))
#define uw2le(x,ec) ((unsigned long)(x))
#else
long sw2ld(si64 x);
long sw2le(si64 x, int ec);
unsigned long uw2ld(ui64 x);
unsigned long uw2le(ui64 x, int ec);
#endif
/* Others */
#define dm64nq(m1,m2,d) ((si32)((si64)(m1)*(si64)(m2)/(si64)(d)))
#define dm64nr(m1,m2,d) ((si32)((si64)(m1)*(si64)(m2)%(si64)(d)))
#define jm64nh(m1,m2) ((si32)SRA((si64)(m1)*(si64)(m2),BITSPERUI32))
#define jesl(lo) ((si64)(lo))                   /* Extend */
#define jeul(lo) ((ui64)(lo))
#define jasw(x,y) ((x) + (y))                   /* Add */
#define jauw(x,y) ((x) + (y))
#define jasl(x,y) ((x) + (si64)(y))
#define jaslo(x,y) ((x) + (si64)(y))
#define jaul(x,y) ((x) + (ui64)(y))
#define jaulo(x,y) ((x) + (ui64)(y))
#define jnsw(x)   (-(x))                        /* Negate */
#define jmsw(x,y) ((si64)(x) * (si64)(y))       /* Multiply */
#define jmswj(x,y) ((x) * (si64)(y))
#define jmuw(x,y) ((ui64)(x) * (ui64)(y))
#define jmuwj(x,y) ((x) * (ui64)(y))
#define jdswq(x,y) ((si32)((x) / (si64)(y)))    /* Divide */
#define jduwjq(x,y) ((x) / (si64)(y))
#define jduwq(x,y) ((ui32)((x) / (ui64)(y)))
#define jrsw(x,y) ((x) - (y))                   /* Reduce (subtract) */
#define jruw(x,y) ((x) - (y))
#define jrsl(x,y) ((x) - (si64)(y))
#define jrul(x,y) ((x) - (ui64)(y))
#define jsrsw(x,s) SRA((x), (s))                /* Shift right */
#define jsruw(x,s) ((x) >> (s))
/* Use the following only when there is no possibility of overflow */
#define jslsw(x,s) ((x) << (s))                 /* Shift left */
#define jsluw(x,s) ((x) << (s))
#define mrssw(x,y,s) SRA(((x)*(si64)(y)),abs(s)) /* Multiply & scale */
#define mrsuw(x,y,s) (((x)*(ui64)(y)) >> abs(s))
#define mrssl(x,y,s) ((si32)SRA(((si64)(x)*(si64)(y)),abs(s)))
#define mrsul(x,y,s) ((ui32)(((ui64)(x)*(ui64)(y)) >> abs(s)))
/* These cannot overflow because right shift |s| is applied */
#define mrsswj(x,y,s) SRA(((si64)(x)*(si64)(y)),abs(s))
#define mrsuwj(x,y,s) (((ui64)(x)*(ui64)(y)) >> abs(s))

#else    /* System does not have I64 */
#ifdef __cplusplus
extern "C" {
#endif
/* Multiply, shift, add */
#define amlssw(sum,x,y,s) amssw((sum), (x), (y), (s))
#define amrssw(sum,x,y,s) amssw((sum), (x), (y), -(s))
#define amlsuw(sum,x,y,s) amsuw((sum), (x), (y), (s))
#define amrsuw(sum,x,y,s) amsuw((sum), (x), (y), -(s))
/* Concatenate */
si64 jcsw(si32 hi, ui32 lo);
ui64 jcuw(ui32 hi, ui32 lo);
/* To and from longs */
ui64 ul2w(ulng x);
si64 sl2w(long x);
long sw2ld(si64 x);
long sw2le(si64 x, int ec);
unsigned long uw2ld(ui64 x);
unsigned long uw2le(ui64 x, int ec);
/* Others */
#define dm64nq(m1,m2,d) jdswq(jmsw((m1),(m2)),(d))
si32 dm64nr(si32 mul1, si32 mul2, si32 div);
#define jm64nh(m1,m2) swhi(jmsw((m1),(m2)))
si64 jesl(si32 lo);                             /* Extend */
ui64 jeul(ui32 lo);
si64 jasw(si64 x, si64 y);                      /* Add */
ui64 jauw(ui64 x, ui64 y);
si64 jasl(si64 x, si32 y);
si64 jaslo(si64 x, long y);
ui64 jaul(ui64 x, ui32 y);
ui64 jaulo(ui64 x, ulng y);
si64 jnsw(si64 x);                              /* Negate */
si64 jmsw(si32 x, si32 y);                      /* Multiply */
si64 jmswj(si64 x, si32 y);
ui64 jmuw(ui32 x, ui32 y);
ui64 jmuwj(ui64 x, ui32 y);
si32 jdswq(si64 x, si32 y);
si64 jduwjq(ui64 x, ui32 y);
ui32 jduwq(ui64 x, ui32 y);
si64 jrsw(si64 x, si64 y);                      /* Reduce (subtract) */
ui64 jruw(ui64 x, ui64 y);
si64 jrsl(si64 x, si32 y);
ui64 jrul(ui64 x, ui32 y);
si64 jsrsw(si64 x, int s);                      /* Shift right */
ui64 jsruw(ui64 x, int s);
/* Use the following only when there is no possibility of overflow */
#define jslsw(x,s) jssw((x), (s))               /* Shift left */
#define jsluw(x,s) jsuw((x), (s))
si64 mrssw(si64 x, si32 y, int s);              /* Multiply & scale */
ui64 mrsuw(ui64 x, ui32 y, int s);
si32 mrssl(si32 x, si32 y, int s);
ui32 mrsul(ui32 x, ui32 y, int s);
/* These cannot overflow because right shift |s| is applied */
si64 mrsswj(si32 x, si32 y, int s);
ui64 mrsuwj(ui32 x, ui32 y, int s);
#ifdef __cplusplus
}
#endif
#endif

/* Wides (used to calc memory allocs) to size_t */
#if ZSIZE == WSIZE
#define sw2zd(x)    ((size_t)(x))
#define sw2ze(x,ec) ((size_t)(x))
/* We are not going to worry about x <= 2^63 */
#define uw2zd(x)    ((size_t)(x))
#define uw2ze(x)    ((size_t)(x))
#else
size_t sw2zd(si64 x);
size_t sw2ze(si64 x, int ec);
size_t uw2zd(ui64 x);
size_t uw2ze(ui64 x, int ec);
#endif

/* Functions implemented as macros.  This set just implements
*  combinations of lower-level routines.  They expect arguments
*  and return results of the types given in the comments.  */

/* Absolute values of smed, si32, sbig, si64 values
*  Note:  abs32(x) and abs64(x) are defined in sysdef.h  */
#define absm(x) abs(x)
#define absj(x) abs32(x)
#ifdef BIT64
#define absb(x) abs64(x)
#else
#define absb(x) abs32(x)
#endif
#define absw(x) abs64(x)
/* jabs(x) is obsolete, now equivalent to absw(x), kept for compat */
#define jabs(x) abs64(x)

/* ui64 amuwwd(ui64 sum, ui64 x, ui64 y) */
#define amuwwd(sum,x,y) jauwd((sum), jmuwwd((x),(y)))
/* ui64 amuwwe(ui64 sum, ui64 x, ui64 y, int ec) */
#define amuwwe(sum,x,y,ec) jauwe((sum), jmuwwe((x),(y),(ec)),(ec))

/* Use these when the sign of the shift is not known at compile time */
/* si64 amssw(si64 sum, si32 x, si32 y, int s) */
#define amssw(sum,x,y,s) jasw((sum), jssw(jmsw((x),(y)),(s)))
/* si64 amsswd(si64 sum, si32 x, si32 y, int s) */
#define amsswd(sum,x,y,s) jaswd((sum),jsswd(jmsw((x),(y)),(s)))
/* si64 amsswe(si64 sum, si32 x, si32 y, int s, int ec) */
#define amsswe(sum,x,y,s,ec) \
   jaswe((sum),jsswe(jmsw((x),(y)),(s),(ec)),(ec))
/* ui64 amsuw(ui64 sum, ui32 x, ui32 y, int s) */
#define amsuw(sum,x,y,s) jauw((sum), jsuw(jmuw((x),(y)),(s)))
/* ui64 amsuwd(ui64 sum, ui32 x, ui32 y, int s) */
#define amsuwd(sum,x,y,s) jauwd((sum),jsuwd(jmuw((x),(y)),(s)))
/* ui64 amsuwe(ui64 sum, ui32 x, ui32 y, int s, int ec) */
#define amsuwe(sum,x,y,s,ec) \
   jauwe((sum),jsuwe(jmuw((x),(y)),(s),(ec)),(ec))

/* Traditional arithmetic with 64-bit intermediate results */
/* si32 dm64nb(si32 mul1, si32 mul2, si32 div, si32 *rem) */
#define dm64nb(m1,m2,d,r) jdswb(jmsw((m1),(m2)),(d),(r))
/* si32 dmsjqd(si32 mul1, si32 mul2, si32 div) */
#define dmsjqd(m1,m2,d) jdswqd(jmsw((m1),(m2)),(d))
/* si32 dmsjqe(si32 mul1, si32 mul2, si32 div, int ec) */
#define dmsjqe(m1,m2,d,ec) jdswqe(jmsw((m1),(m2)),(d),(ec))
/* ui32 dmujqd(ui32 mul1, ui32 mul2, ui32 div) */
#define dmujqd(m1,m2,d) jduwqd(jmuw((m1),(m2)),(d))
/* ui32 dmujqe(ui32 mul1, ui32 mul2, ui32 div, int ec) */
#define dmujqe(m1,m2,d,ec) jduwqe(jmuw((m1),(m2)),(d),(ec))
/* si32 ds64nq(si32 hi32, ui32 lo32, int ishft, si32 div) */
#define ds64nq(hi,lo,s,d) jdswq(jssw(jcsw((hi),(lo)),(s)),(d))

/* This macro can replace mrsrswd when it is possible that the first
*  argument fits in 32 bits, so the 96-bit product is not needed.
*  si64 mrsrswdm(si64 x, si32 y, int s)  */
#ifdef HAS_I64
#define mrsrswdm(x,y,s) (jckslo(x) ? mrsrswd(x,y,s) : \
   SRA(SRA(x * (si64)y, (s-1)) + 1, 1))
#else /* No 64-bit arithmetic */
#define mrsrswdm(x,y,s) mrsrswd(x,y,s)
#endif

/* These are obsolete, use m[lr]ssle,m[lr]srsle for error checking */
/* si32 jm64sh(si32 mul1, si32 mul2, int ishft) */
#define jm64sh(m1,m2,s) swhi(jssw(jmsw((m1),(m2)),(s)))
/* ui32 jm64sl(si32 mul1, si32 mul2, int ishft) */
#define jm64sl(m1,m2,s) swlo(jssw(jmsw((m1),(m2)),(s)))

/* Just 32-bit check signed/unsigned sum/difference for overflow */
#define jckss(sum,a,b) (((sum ^ (a)) & ~((a) ^ (b))) < 0)
#define jckus(a,b)     (~(b) < (a))
#define jcksd(dif,a,b) (((dif ^ (a)) & ((a) ^ (b))) < 0)
#define jckud(a,b)     ((b) > (a))

/* Just check whether a 64-bit number fits in a 32-bit number--
*  argument must be a single variable for x.hi/x.lo extraction.  */
#ifdef HAS_I64
#if LSIZE == 8
#define jckslo(x) (labs(x) > 0x000000007fffffffL)
#define jckulo(x) ((x) > 0x00000000ffffffffUL)
#else
#define jckslo(x) (llabs(x) > 0x000000007fffffffLL)
#define jckulo(x) ((x) > 0x00000000ffffffffULL)
#endif
#else /* No 64-bit arith */
#define jckslo(x) (x.hi ^ SRA(x.lo,BITSPERUI32-1))
#define jckulo(x) (x.hi)
#endif

/* 32-bit sums and differences implemented as macros.  These macros
*  generate inline code for faster execution.  Those with names ending
*  in 'm' have result as an argument--these cannot be called as func-
*  tions.  Macros may be used for signed or unsigned long, [su]i32, or
*  int variables--the function calls are needed for [su]i64 types.
*  WARNING:  The sum or dif cannot be the same as a or b.  */
#define jasidm(sum,a,b)             /* Just add signed integers */ \
   sum = (a) + (b); if (jckss(sum,(a),(b))) { \
      e64dac("jasidm"); sum = (a) >= 0 ? INT_MAX : -INT_MAX; }
#define jasiem(sum,a,b,ec) \
   sum = (a) + (b); if (jckss(sum,(a),(b))) { \
      e64act("jasiem", ec); sum = (a) >= 0 ? INT_MAX : -INT_MAX; }
#define jasjdm(sum,a,b)             /* Just add SI32s */ \
   sum = (a) + (b); if (jckss(sum,(a),(b))) { \
      e64dac("jasjdm"); sum = (a) >= 0 ? SI32_MAX : -SI32_MAX; }
#define jasjem(sum,a,b,ec) \
   sum = (a) + (b); if (jckss(sum,(a),(b))) { \
      e64act("jasjem", ec); sum = (a) >= 0 ? SI32_MAX : -SI32_MAX; }
#define jasldm(sum,a,b)             /* Just add signed longs */ \
   sum = (a) + (b); if (jckss(sum,(a),(b))) { \
      e64dac("jasldm"); sum = (a) >= 0 ? LONG_MAX : -LONG_MAX; }
#define jaslem(sum,a,b,ec) \
   sum = (a) + (b); if (jckss(sum,(a),(b))) { \
      e64act("jaslem", ec); sum = (a) >= 0 ? LONG_MAX : -LONG_MAX; }

#define jrsidm(dif,a,b)             /* Subtract signed integers */ \
   dif = (a) - (b); if (jcksd(dif,(a),(b))) { \
      e64dac("jrsidm"); dif = (a) >= 0 ? INT_MAX : -INT_MAX; }
#define jrsiem(dif,a,b,ec) \
   dif = (a) - (b); if (jcksd(dif,(a),(b))) { \
      e64act("jrsiem", ec); dif = (a) >= 0 ? INT_MAX : -INT_MAX; }
#define jrsjdm(dif,a,b)             /* Subtract signed SI32s */ \
   dif = (a) - (b); if (jcksd(dif,(a),(b))) { \
      e64dac("jrsjdm"); dif = (a) >= 0 ? SI32_MAX : -SI32_MAX; }
#define jrsjem(dif,a,b,ec) \
   dif = (a) - (b); if (jcksd(dif,(a),(b))) { \
      e64act("jrsjem", ec); dif = (a) >= 0 ? SI32_MAX : -SI32_MAX; }
#define jrsldm(dif,a,b)             /* Subtract signed longs */ \
   dif = (a) - (b); if (jcksd(dif,(a),(b)))) { \
      e64dac("jrsldm"); dif = (a) >= 0 ? LONG_MAX : -LONG_MAX; }
#define jrslem(dif,a,b,ec) \
   dif = (a) - (b); if (jcksd(dif,(a),(b))) { \
      e64act("jrslem", ec); dif = (a) >= 0 ? LONG_MAX : -LONG_MAX; }

#define jauadm(sum,a,b)             /* [ijl]-type unsigned sum */ \
   sum = (a) + (b); if (jckus((a),(b))) { \
      e64dac("jauadm"); sum = ~0; }
#define jauaem(sum,a,b,ec) \
   sum = (a) + (b); if (jckus((a),(b))) { \
      e64act("jauaem", ec); sum = ~0; }
#define jruadm(dif,a,b)             /* [ijl]-type unsigned diff */ \
   dif = (a) - (b); if (jckud((a),(b))) { \
      e64dac("jruadm"); dif = 0; }
#define jruaem(dif,a,b,ec) \
   dif = (a) - (b); if (jckud((a),(b))) { \
      e64act("jruaem", ec); dif = 0; }

/* Extract signed, unsigned low 32 bits with overflow checking.
*  N.B.  x32 return values must be left-hand assignment targets.  */
#define swlodm(x32,x64) \
   if (jckslo(x64)) { e64dac("swlodm"); \
      x32 = qsw(x64) >= 0 ? SI32_MAX : -SI32_MAX; } \
   else x32 = swlo(x64);
#define swloem(x32,x64,ec) \
   if (jckslo(x64)) { e64act("swloem", ec); \
      x32 = qsw(x64) >= 0 ? SI32_MAX : -SI32_MAX; } \
   else x32 = swlo(x64);
#define uwlodm(x32,x64) \
   if (jckulo(x64)) { e64dac("uwlodm"); x32 = UI32_MAX; } \
   else x32 = uwlo(x64);
#define uwloem(x32,x64,ec) \
   if (jckulo(x64)) { e64act("uwloem", ec); x32 = UI32_MAX; } \
   else x32 = uwlo(x64);

/* Call the mrs[rsu] family of routines from old code where shift was
*  encoded as a negative value for right shift.  These macros are
*  provided only for those routines that were actually called.  */
#define msswe(x,y,s,ec)  mrsswe(x,y,abs(s),ec)
#define msuwe(x,y,s,ec)  mrsuwe(x,y,abs(s),ec)
#define mssle(x,y,s,ec)  mrssle(x,y,abs(s),ec)
#define msrsle(x,y,s,ec) mrsrsle(x,y,abs(s),ec)
#define msrswe(x,y,s,ec) mrsrswe(x,y,abs(s),ec)
#define msrule(x,y,s,ec) mrsrule(x,y,abs(s),ec)

/* The following require code, not just macros, in HAS_I64 or NO_I64 */
#ifdef __cplusplus
extern "C" {
#endif
/* Note jdswb returns 32-bit quotient, jduwb returns 64-bit quotient */
si32 jdswb(si64 x, si32 y, si32 *pr);  /* Divide */
ui64 jduwb(ui64 x, ui32 y, ui32 *pr);
ui64 jmuwb(ui64 x, ui32 y, ui32 *phi); /* Multiply */
void vdivl(ui32 *x, ui32 *y, ui32 *pq, ui32 *pr, ui32 *u,
   int nx, int ny, int kr);

/* Use these when the sign of the shift is not known at compile time */
si32 dsrswq(si64 x, int s, si32 y);    /* Shift, round, divide */
ui32 dsruwq(ui64 x, int s, ui32 y);
si64 jssw(si64 x, int s);              /* Shift */
ui64 jsuw(ui64 x, int s);

/* 64-bit arithmetic functions with full error checking */
/* si64 or ui64 from double */
si64 dbl2swd(double dx);
si64 dbl2swe(double dx, int ec);
ui64 dbl2uwd(double dx);
ui64 dbl2uwe(double dx, int ec);
/* Multiply, divide, round, scale */
si64 dmrswjwd(si64 x, si32 y, si32 d, int s);
si64 dmrswjwe(si64 x, si32 y, si32 d, int s, int ec);
si64 dmrswwqd(si64 x, si64 y, si32 m);
si64 dmrswwqe(si64 x, si64 y, si32 m, int ec);
/* Divide and round (for taking averages) */
si32 drswq(si64 x, si32 y);
/* Shift, divide, round */
si32 dsrsjqd(si32 x, int s, si32 y);
si32 dsrsjqe(si32 x, int s, si32 y, int ec);
si64 dsrswjqd(si64 x, int s, si32 y);
si64 dsrswjqe(si64 x, int s, si32 y, int ec);
si32 dsrswqd(si64 x, int s, si32 y);
si32 dsrswqe(si64 x, int s, si32 y, int ec);
si32 dsrswwjd(si64 x, si64 y, int s);
si32 dsrswwje(si64 x, si64 y, int s, int ec);
si64 dsrswwqd(si64 x, si64 y, int s);
si64 dsrswwqe(si64 x, si64 y, int s, int ec);
ui32 dsruwqd(ui64 x, int s, ui32 y);
ui32 dsruwqe(ui64 x, int s, ui32 y, int ec);
ui32 dsruwwjd(ui64 x, ui64 y, int s);
ui32 dsruwwje(ui64 x, ui64 y, int s, int ec);
ui64 dsruwwqd(ui64 x, ui64 y, int s);
ui64 dsruwwqe(ui64 x, ui64 y, int s, int ec);
/* Just add */
si64 jaswd(si64 x, si64 y);
si64 jaswe(si64 x, si64 y, int ec);
si64 jasld(si64 x, si32 y);
si64 jasle(si64 x, si32 y, int ec);
si64 jaslod(si64 x, long y);
si64 jasloe(si64 x, long y, int ec);
ui64 jauwd(ui64 x, ui64 y);
ui64 jauwe(ui64 x, ui64 y, int ec);
ui64 jauld(ui64 x, ui32 y);
ui64 jaule(ui64 x, ui32 y, int ec);
ui64 jaulod(ui64 x, ulng y);
ui64 jauloe(ui64 x, ulng y, int ec);
ui64 jauzd(ui64 w, size_t z);
ui64 jauze(ui64 w, size_t z, int ec);
/* Just divide */
si32 jdswbd(si64 x, si32 y, si32 *pr);
si32 jdswbe(si64 x, si32 y, int ec, si32 *pr);
si32 jdswqd(si64 x, si32 y);
si32 jdswqe(si64 x, si32 y, int ec);
ui32 jduwqd(ui64 x, ui32 y);
ui32 jduwqe(ui64 x, ui32 y, int ec);
/* Just multiply */
si32 jmsld(si32 x, si32 y);
si32 jmsle(si32 x, si32 y, int ec);
ui32 jmuld(ui32 x, ui32 y);
ui32 jmule(ui32 x, ui32 y, int ec);
ui64 jmuwjd(ui64 x, ui32 y);
ui64 jmuwje(ui64 x, ui32 y, int ec);
ui64 jmuwwd(ui64 x, ui64 y);
ui64 jmuwwe(ui64 x, ui64 y, int ec);
size_t jmuzjd(size_t z, ui32 y);
size_t jmuzje(size_t z, ui32 y, int ec);
/* Just negate */
si64 jnswd(si64 x);
si64 jnswe(si64 x, int ec);
/* Just reduce (subtract) */
si64 jrswd(si64 x, si64 y);
si64 jrswe(si64 x, si64 y, int ec);
si64 jrsld(si64 x, si32 y);
si64 jrsle(si64 x, si32 y, int ec);
ui64 jruwd(ui64 x, ui64 y);
ui64 jruwe(ui64 x, ui64 y, int ec);
ui64 jruld(ui64 x, ui32 y);
ui64 jrule(ui64 x, ui32 y, int ec);
/* Just shift left */
si64 jslswd(si64 x, int s);
si64 jslswe(si64 x, int s, int ec);
ui64 jsluwd(ui64 x, int s);
ui64 jsluwe(ui64 x, int s, int ec);
/* Shift right with rounding (cannot overflow) */
si64 jsrrsw(si64 x, int s);
ui64 jsrruw(ui64 x, int s);
si32 jsrrsj(si32 x, int s);
ui32 jsrruj(ui32 x, int s);
/* Just shift (either direction) */
si64 jsswd(si64 x, int s);
si64 jsswe(si64 x, int s, int ec);
ui64 jsuwd(ui64 x, int s);
ui64 jsuwe(ui64 x, int s, int ec);
/* Multiply, scale right, round */
si32 mrsrsl(si32 x, si32 y, int s);     /* Caller: no overflow */
si32 mrsrsld(si32 x, si32 y, int s);
si32 mrsrsle(si32 x, si32 y, int s, int ec);
si64 mrsrswd(si64 x, si32 y, int s);
si64 mrsrswe(si64 x, si32 y, int s, int ec);
si64 mrsrswj(si32 x, si32 y, int s);    /* Cannot overflow */
ui32 mrsrul(ui32 x, ui32 y, int s);     /* Caller: no overflow */
ui32 mrsruld(ui32 x, ui32 y, int s);
ui32 mrsrule(ui32 x, ui32 y, int s, int ec);
ui64 mrsruwd(ui64 x, ui32 y, int s);
ui64 mrsruwe(ui64 x, ui32 y, int s, int ec);
ui64 mrsruwj(ui32 x, ui32 y, int s);    /* Cannot overflow */
/* Multiply and scale right without rounding */
si32 mrssld(si32 x, si32 y, int s);
si32 mrssle(si32 x, si32 y, int s, int ec);
si64 mrsswd(si64 x, si32 y, int s);
si64 mrsswe(si64 x, si32 y, int s, int ec);
ui32 mrsuld(ui32 x, ui32 y, int s);
ui32 mrsule(ui32 x, ui32 y, int s, int ec);
ui64 mrsuwd(ui64 x, ui32 y, int s);
ui64 mrsuwe(ui64 x, ui32 y, int s, int ec);
/* Multiply and scale left (no rounding needed) */
si64 mlsswd(si64 x, si32 y, int s);
si64 mlsswe(si64 x, si32 y, int s, int ec);
si64 mlsswjd(si32 x, si32 y, int s);
si64 mlsswje(si32 x, si32 y, int s, int ec);
ui64 mlsuwd(ui64 x, ui32 y, int s);
ui64 mlsuwe(ui64 x, ui32 y, int s, int ec);
ui64 mlsuwjd(ui32 x, ui32 y, int s);
ui64 mlsuwje(ui32 x, ui32 y, int s, int ec);
/* Extract low-order word */
si32 swlod(si64 x);
si32 swloe(si64 x, int ec);
ui32 uwlod(ui64 x);
ui32 uwloe(ui64 x, int ec);

/* Traditional arithmetic with 64-bit intermediate results */
si32 jm64nb(si32 mul1, si32 mul2, ui32 *lo32);

/* This is obsolete, use mssle,msrsle for error checking */
si32 jm64sb(si32 mul1, si32 mul2, int ishft, ui32 *lo32);

/* Other arithmetic functions */
float bessi0(float x);
#if LSIZE == 8
#define bitszsl bitszsw
#define bitszul bitszuw
#else
#define bitszsl bitszs32
#define bitszul bitszu32
#endif
#define bitsz bitszu32  /* For compat */
int bitszs32(si32 x);
int bitszu32(ui32 x);
int bitszsw(si64 x);
int bitszuw(ui64 x);
int dmfsd(double *a, long N, float eps);
int dsinv(double *matrix, long N, float eps);
float erfcf(float x);
unsigned long getprime(unsigned long minval);
int imulup(int val, int base);
int jacobi(double *a, double *val, double *vec, double *dd,
   int N, int kvec, int ksort);
long lmulup(long val, int base);
void matm33(float *in1, float *in2, float *out);
void minv(float *matrix, int n, float *determinant,
   int *iwork1, int *iwork2);
si32 ndev(si32 *seed, si32 cmn /* S24 */, si32 csg /* S28 */);
void rannor(float *nr, int n, si32 *seed, float mean, float sigma);
void rannum(si32 *ir, int n, si32 *seed, int bits);
float rerfc(float x);
float rmbf10(float x);
void si16perm(si16 *pval, si32 *seed, si32 n);
void si32perm(si32 *pval, si32 *seed, si32 n);
si32 udev(si32 *seed);
void udevskip(si32 *seed, si32 skip);
void udevwskp(si32 *seed, si64 skip);
#define ui32pow ui32powe
ui32 ui32powd(ui32 val, int pow, int scale);
ui32 ui32powe(ui32 val, int pow, int scale, int ec);
ui32 umulup(ui32 val, int base);
ui64 ymulup(ui64 val, int base);
size_t zmulup(size_t val, int base);
#ifdef __cplusplus
}
#endif

/* Flags for matrix manipulation routines */
#define RK_POSDERR     -1  /* Matrix is not positive definite */
#define RK_EIGVV        0  /* Calculate eigenvalues and vectors */
#define RK_NOVEC        1  /* Calculate eigenvalues only */

#endif /* RKARITH_HDR_INCLUDED */
