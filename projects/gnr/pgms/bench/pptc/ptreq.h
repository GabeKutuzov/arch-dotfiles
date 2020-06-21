/*---------------------------------------------------------------------*
*                               ptreq.h                                *
*                                                                      *
*  This file contains structure and option bit definitions for MVP     *
*  transfer controller requests for machines running "big-endian".     *
*  Unions are used to define fields that have multiple interpretations *
*  according to the transfer mode.  Names of option bits have been     *
*  taken from the GIC header file "xfer.h" for compatibility.          *
*                                                                      *
*  N.B.  A transfer request structure called "PTREQ" is defined in     *
*  mp_ptreq.h, but it has just the next field and 15 longs defined.    *
*                                                                      *
*  V1A, 07/30/95, GNR - Initial version                                *
*---------------------------------------------------------------------*/

#ifndef _PTREQ_H_
#define _PTREQ_H_

#include "mvp.h"              /* Pick up register defs */

/* Useful memory layout constants */
#define NumPP          4      /* Number of PPs on a chip */
#define PPRAM_SIZE  2048      /* Size of one PP RAM area */
#define RAMPitch  0x1000      /* Spacing between nodes in RAM  */
#define PPRAMShift    12      /* Shift to multiply by RAMPitch */

typedef long FixedGuide;      /* Used to build a guide table */

typedef struct PTReq {

   struct PTReq *next;        /* Next entry in linked list */
   long  options;             /* Packet transfer options */
   void  *srcAddr;            /* Source start address */
   void  *dstAddr;            /* Destination start address */
   short srcBCnt;             /* Source B count (Y-size-1) */
   short srcACnt;             /* Source A count (X-size) */
   short dstBCnt;             /* Destination B count (Y-size-1) */
   short dstACnt;             /* Destination A count (X-size) */
   long  srcCCnt;             /* Source C count (num_patches-1)
                              *  or number of guided entries */
   long  dstCCnt;             /* Destination C count (num_patches-1)
                              *  or number of guided entries */
   union {
   long  srcBPitch;           /* Source B skip offset */
      long  LSFill;           /* Source least-signif fill value */
      } w32;
   long  dstBPitch;           /* Destination B skip offset */
   union {
      long  srcCPitch;        /* Source C skip offset */
      long  MSFill;           /* Source most-signif fill value */
      long  *srcGuide;        /* Source guide table location */
      } w40;
   union {
      long  dstCPitch;        /* Destination C skip offset */
      long  *dstGuide;        /* Destionation guide table location */
      } w44;
   long  srcTransp[2];        /* Source transparency value(s) */
   long  reserved[2];         /* Reserved */
   } PTReq;

#define PTREQ_SIZE (sizeof(PTReq))

/* Packet Transfer Options field definitions */

/* dst update mode */
#define PTOPT_DUM_MASK     0x00000003
#define PTOPT_DUM_NUP      0x00000000
#define PTOPT_DUM_ADDB     0x00000001
#define PTOPT_DUM_ADDC     0x00000002
#define PTOPT_DUM_ADDCT    0x00000003

/* dst transfer mode */
#define PTOPT_DTM_MASK     0x00000070
#define PTOPT_DTM_DIM      0x00000000
#define PTOPT_DTM_VARDEL   0x00000040
#define PTOPT_DTM_VAROFF   0x00000050
#define PTOPT_DTM_FIXDEL   0x00000060
#define PTOPT_DTM_FIXOFF   0x00000070

/* src update mode */
#define PTOPT_SUM_MASK     0x00000300
#define PTOPT_SUM_NUP      0x00000000
#define PTOPT_SUM_ADDB     0x00000100
#define PTOPT_SUM_ADDC     0x00000200
#define PTOPT_SUM_ADDCT    0x00000300

/* src transfer mode */
#define PTOPT_STM_MASK     0x00007000
#define PTOPT_STM_DIM      0x00000000
#define PTOPT_STM_FILL     0x00001000
#define PTOPT_STM_FIXLUT   0x00003000
#define PTOPT_STM_VARDEL   0x00004000
#define PTOPT_STM_VAROFF   0x00005000
#define PTOPT_STM_FIXDEL   0x00006000
#define PTOPT_STM_FIXOFF   0x00007000

/* PT access mode */
#define PTOPT_PAM_MASK     0x00070000
#define PTOPT_PAM_NORM     0x00000000
#define PTOPT_PAM_PERI     0x00010000
#define PTOPT_PAM_BLK      0x00020000
#define PTOPT_PAM_SER      0x00030000
#define PTOPT_PAM_8        0x00040000
#define PTOPT_PAM_16       0x00050000
#define PTOPT_PAM_32       0x00060000
#define PTOPT_PAM_64       0x00070000

/* eXchange src/dst */
#define PTOPT_X_MASK       0x00080000
#define PTOPT_X_NORM       0x00000000
#define PTOPT_X_SWAP       0x00080000

/* Reverse source B addressing */
#define PTOPT_RSB_MASK     0x02000000
#define PTOPT_RSB_NORM     0x00000000
#define PTOPT_RSB_REV      0x02000000

/* Reverse source C addressing */
#define PTOPT_RSC_MASK     0x04000000
#define PTOPT_RSC_NORM     0x00000000
#define PTOPT_RSC_REV      0x04000000

/* Reverse A addressing */
#define PTOPT_RA_MASK      0x08000000
#define PTOPT_RA_NORM      0x00000000
#define PTOPT_RA_REV       0x08000000

/* Reverse destination B addressing */
#define PTOPT_RDB_MASK     0x01000000
#define PTOPT_RDB_NORM     0x00000000
#define PTOPT_RDB_REV      0x01000000

/* Reverse destination C addressing */
#define PTOPT_RDC_MASK     0x02000000
#define PTOPT_RDC_NORM     0x00000000
#define PTOPT_RDC_REV      0x02000000

/* Interrupt when finished */
#define PTOPT_I_MASK       0x10000000
#define PTOPT_I_NOINT      0x00000000
#define PTOPT_I_INT        0x10000000

/* Packet transfer status */
#define PTOPT_PTS_MASK     0x60000000
#define PTOPT_PTS_NOSUS    0x00000000
#define PTOPT_PTS_SUS      0x20000000
#define PTOPT_PTS_SUSFSRC  0x40000000
#define PTOPT_PTS_SUSFDST  0x60000000

/* Stop -- end of the PT linked list */
#define PTOPT_S_MASK       0x80000000
#define PTOPT_S_LINKCONT   0x00000000
#define PTOPT_S_LINKEND    0x80000000

/* Function calls relating to PTReqs */

#ifdef _MVP_PP
#define DPtReqIssue(PT) { \
   char **ptcreq = (char **)(0x010000fcL + ((COMM & 7)<<12)); \
   while (COMM & (1<<29)); \
   *ptcreq = (char *)PT; COMM |= (1<<28); }
#define DPtReqWait() \
   { while (COMM & (1<<29)); }
#else    /* Just for syntax testing on other compilers */
void DPtReqIssue(PTReq *PT);
void DPtReqWait(void);
#endif

#endif /*_PTREQ_H_*/
