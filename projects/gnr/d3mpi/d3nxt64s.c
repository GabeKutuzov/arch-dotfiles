/* This file is generated automatically by nxdr2.  It
*  contains data conversion tables for interprocessor
*  messaging.  Do not edit by hand!  */

/* When compiled with -DPAR, the application must
*  provide a definition of unicvtf, the interface
*  to union conversion routines, in some header
*  file that is included in the -f input file. */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

/* (c) Copyright 1993-2016, The Rockefeller University *11114* */
/* $Id: d3hdrs.c 70 2017-01-16 19:27:55Z  $ */
/***********************************************************************
*                              d3hdrs.c                                *
*                                                                      *
*  This file contains a list of all the header files that must         *
*  be read by nxdr2.  (Does not need to include files #included        *
*  from other files, e.g. d3global.h)  They must be ordered such       *
*  that symbols are defined before use.                                *
************************************************************************
*  ==>, 12/30/07, GNR - Last mod before committing to svn repository   *
*  V8F, 02/16/10, GNR - Merge in old d3hdrs.h, add utvdef.h            *
*  R67, 04/27/16, GNR - Remove Darwin 1 support                        *
***********************************************************************/

#include <stddef.h>
#include "sysdef.h"
#include "collect.h"
#include "d3global.h"
#include "celldata.h"
#include "ijpldef.h"
#include "clblk.h"
#include "statdef.h"
#include "tvdef.h"

long NXDRTT[] = {
   sizeof(armwdw_type),              /* armwdw_type */
   ( 1 << 8 ) | 'F',
   sizeof(asmul_type),               /* asmul_type */
   ( 2 << 8 ) | 'I',
   sizeof(clloc),                    /* clloc */
   ( 2 << 8 ) | 'H',
   sizeof(fdm_type),                 /* fdm_type */
   ( 1 << 8 ) | 'U',
   sizeof(gac_type),                 /* gac_type */
   ( 1 << 8 ) | 'L',
   sizeof(jdat_type),                /* jdat_type */
   ( 1 << 8 ) | 'I',
   sizeof(ilst),                     /* ilst */
   ( 8 << 8 ) | 's',
   ( 1 << 8 ) | 'p',
   ( 1 << 8 ) | 'z',
   ( 5 << 8 ) | 'l',
   ( 1 << 8 ) | 'I',
   sizeof(ilstitem),                 /* ilstitem */
   ( 1 << 8 ) | 'N',
   sizeof(orns),                     /* orns */
   ( 1 << 8 ) | 'I',
   sizeof(outnow_type),              /* outnow_type */
   ( 1 << 8 ) | 'H',
   sizeof(struct AUTSCL),            /* str_AUTSCL */
   ( 4 << 8 ) | 's',
   ( 7 << 8 ) | 'i',
   ( 1 << 8 ) | 'h',
   ( 5 << 8 ) | 'm',
   ( 1 << 8 ) | 'H',
   sizeof(struct BREGDEF),           /* str_BREGDEF */
   ( 4 << 8 ) | 's',
   ( 1 << 8 ) | 'u',
   ( 4 << 8 ) | 'i',
   ( 1 << 8 ) | 'u',
   ( 5 << 8 ) | 'i',
   ( 1 << 8 ) | 'u',
   ( 3 << 8 ) | 'i',
   ( 1 << 8 ) | 'M',
   sizeof(struct CPLOC),             /* str_CPLOC */
   ( 6 << 8 ) | 'F',
   sizeof(struct CLSTAT),            /* str_CLSTAT */
   ( 8 << 8 ) | 's',
   ( 4 << 8 ) | 'w',
   ( 61 << 8 ) | 'L',
   sizeof(struct GPSTAT),            /* str_GPSTAT */
   ( 4 << 8 ) | 's',
   ( 2 << 8 ) | 'u',
   ( 4 << 8 ) | 'I',
   92,                               /* str_CTDFLT */
   ( 4 << 8 ) | 's',
   ( 1 << 8 ) | 'f',
   ( 4 << 8 ) | 'i',
   ( 1 << 8 ) | 'u',
   ( 4 << 8 ) | 'i',
   ( 1 << 8 ) | 'u',
   ( 1 << 8 ) | 'i',
   ( 3 << 8 ) | 'm',
   ( 2 << 8 ) | 'h',
   ( 10 << 8 ) | 'b',
   ( 24 << 8 ) | 'C',
   32,                               /* str_DCYDFLT */
   ( 4 << 8 ) | 's',
   ( 6 << 8 ) | 'i',
   ( 4 << 8 ) | 'M',
   20,                               /* str_NDFLT */
   ( 4 << 8 ) | 's',
   ( 4 << 8 ) | 'i',
   ( 4 << 8 ) | 'B',
   12,                               /* str_rlnm_str */
   ( 2 << 8 ) | 's',
   ( 2 << 8 ) | 'h',
   ( 8 << 8 ) | 'C',
   26,                               /* uni_CTctwk_1_tree */
   ( 8 << 8 ) | 's',
   ( 1 << 8 ) | 'p',
   ( 1 << 8 ) | 'i',
   ( 1 << 8 ) | 'x',
   ( 69 ),
   ( 2 << 8 ) | 'B',
   26,                               /* uni_CTctwk_1 */
   ( 1 << 8 ) | 'X',
   ( 73 ),
   40,                               /* uni_CTctwk_2_phs1 */
   ( 8 << 8 ) | 's',
   ( 1 << 8 ) | 'p',
   ( 1 << 8 ) | 'z',
   ( 2 << 8 ) | 'y',
   ( 2 << 8 ) | 'h',
   ( 4 << 8 ) | 'C',
   40,                               /* uni_CTctwk_2 */
   ( 1 << 8 ) | 'X',
   ( 83 ),
   16,                               /* uni_CTctwk_3_go */
   ( 4 << 8 ) | 's',
   ( 2 << 8 ) | 'f',
   ( 1 << 8 ) | 'i',
   ( 1 << 8 ) | 'm',
   ( 2 << 8 ) | 'C',
   16,                               /* uni_CTctwk_3 */
   ( 1 << 8 ) | 'X',
   ( 93 ),
   40,                               /* uni_CTctwk */
   ( 1 << 8 ) | 'J',
   ( 0 << 8) | 8,
   sizeof(struct CELLTYPE),          /* str_CELLTYPE */
   ( 8 << 8 ) | 's',
   ( 37 << 8 ) | 'p',
   ( 1 << 8 ) | 'x',
   ( 49 ),
   ( 1 << 8 ) | 'x',
   ( 61 ),
   ( 2 << 8 ) | 'l',
   ( 1 << 8 ) | 'x',
   ( 65 ),
   ( 2 << 8 ) | 'i',
   ( 3 << 8 ) | 'u',
   ( 1 << 8 ) | 'h',
   ( 3 << 8 ) | 'h',
   ( 1 << 8 ) | 'm',
   ( 2 << 8 ) | 'b',
   ( 4 << 8 ) | 'c',
   ( 1 << 8 ) | 'j',
   ( 0 << 8) | 8,
   ( 4 << 8 ) | 'y',
   ( 2 << 8 ) | 'z',
   ( 8 << 8 ) | 'f',
   ( 14 << 8 ) | 'u',
   ( 1 << 8 ) | 'i',
   ( 18 << 8 ) | 'i',
   ( 2 << 8 ) | 'm',
   ( 22 << 8 ) | 'h',
   ( 1 << 8 ) | 'm',
   ( 6 << 8 ) | 'h',
   ( 26 << 8 ) | 'm',
   ( 19 << 8 ) | 'b',
   ( 2 << 8 ) | 'C',
   sizeof(struct CLBLK),             /* str_CLBLK */
   ( 8 << 8 ) | 's',
   ( 3 << 8 ) | 'p',
   ( 1 << 8 ) | 'm',
   ( 1 << 8 ) | 'X',
   ( 69 ),
   8,                                /* uni_DCDu1_1 */
   ( 2 << 8 ) | 'I',
   8,                                /* uni_DCDu1_2 */
   ( 2 << 8 ) | 'F',
   4,                                /* uni_DCDu1_3 */
   ( 1 << 8 ) | 'I',
   8,                                /* uni_DCDu1 */
   ( 1 << 8 ) | 'J',
   ( 1 << 8) | 4,
   28,                               /* str_DECAYDEF */
   ( 8 << 8 ) | 's',
   ( 1 << 8 ) | 'p',
   ( 1 << 8 ) | 'i',
   ( 1 << 8 ) | 'm',
   ( 2 << 8 ) | 'b',
   ( 1 << 8 ) | 'j',
   ( 1 << 8) | 4,
   ( 2 << 8 ) | 'B',
   12,                               /* uni_CDUn1_1_lin */
   ( 4 << 8 ) | 's',
   ( 2 << 8 ) | 'i',
   ( 1 << 8 ) | 'U',
   12,                               /* uni_CDUn1_1 */
   ( 1 << 8 ) | 'X',
   ( 161 ),
   8,                                /* uni_CDUn1_2 */
   ( 2 << 8 ) | 'U',
   8,                                /* uni_CDUn1_3 */
   ( 2 << 8 ) | 'F',
   12,                               /* uni_CDUn1 */
   ( 1 << 8 ) | 'J',
   ( 2 << 8) | 4,
   10,                               /* uni_CDUn2_1_ion */
   ( 4 << 8 ) | 's',
   ( 2 << 8 ) | 'f',
   ( 1 << 8 ) | 'M',
   10,                               /* uni_CDUn2_1 */
   ( 1 << 8 ) | 'X',
   ( 175 ),
   8,                                /* uni_CDUn2_2_psn */
   ( 4 << 8 ) | 's',
   ( 1 << 8 ) | 'u',
   ( 1 << 8 ) | 'I',
   8,                                /* uni_CDUn2_2 */
   ( 1 << 8 ) | 'X',
   ( 182 ),
   4,                                /* uni_CDUn2_3 */
   ( 1 << 8 ) | 'I',
   10,                               /* uni_CDUn2 */
   ( 1 << 8 ) | 'J',
   ( 3 << 8) | 4,
   1,                                /* uni_CDUn3_1 */
   ( 1 << 8 ) | 'B',
   1,                                /* uni_CDUn3_2 */
   ( 1 << 8 ) | 'B',
   1,                                /* uni_CDUn3 */
   ( 1 << 8 ) | 'J',
   ( 4 << 8) | 1,
   sizeof(struct CONDUCT),           /* str_CONDUCT */
   ( 8 << 8 ) | 's',
   ( 4 << 8 ) | 'p',
   ( 1 << 8 ) | 'x',
   ( 152 ),
   ( 2 << 8 ) | 'b',
   ( 2 << 8 ) | 'c',
   ( 1 << 8 ) | 'j',
   ( 2 << 8) | 4,
   ( 1 << 8 ) | 'j',
   ( 3 << 8) | 4,
   ( 2 << 8 ) | 'f',
   ( 1 << 8 ) | 'i',
   ( 3 << 8 ) | 'u',
   ( 1 << 8 ) | 'm',
   ( 1 << 8 ) | 'h',
   ( 3 << 8 ) | 'm',
   ( 1 << 8 ) | 'h',
   ( 5 << 8 ) | 'm',
   ( 3 << 8 ) | 'b',
   ( 1 << 8 ) | 'J',
   ( 4 << 8) | 1,
   sizeof(struct CNSTAT),            /* str_CNSTAT */
   ( 8 << 8 ) | 's',
   ( 2 << 8 ) | 'w',
   ( 9 << 8 ) | 'L',
   22,                               /* str_LMINPUT */
   ( 4 << 8 ) | 's',
   ( 5 << 8 ) | 'i',
   ( 1 << 8 ) | 'M',
   22,                               /* uni_CNucij_1 */
   ( 1 << 8 ) | 'X',
   ( 227 ),
   32,                               /* uni_CNucij_2_l2 */
   ( 4 << 8 ) | 's',
   ( 4 << 8 ) | 'f',
   ( 3 << 8 ) | 'i',
   ( 1 << 8 ) | 'm',
   ( 2 << 8 ) | 'B',
   32,                               /* uni_CNucij_2 */
   ( 1 << 8 ) | 'X',
   ( 234 ),
   12,                               /* uni_CNucij_3 */
   ( 3 << 8 ) | 'F',
   44,                               /* uni_CNucij_4_m */
   ( 8 << 8 ) | 's',
   ( 3 << 8 ) | 'p',
   ( 5 << 8 ) | 'I',
   44,                               /* uni_CNucij_4 */
   ( 1 << 8 ) | 'X',
   ( 245 ),
   12,                               /* uni_CNucij_5 */
   ( 3 << 8 ) | 'I',
   44,                               /* uni_CNucij */
   ( 1 << 8 ) | 'J',
   ( 5 << 8) | 8,
   196,                              /* str_CNDFLT */
   ( 8 << 8 ) | 's',
   ( 2 << 8 ) | 'p',
   ( 1 << 8 ) | 'x',
   ( 152 ),
   ( 5 << 8 ) | 'i',
   ( 1 << 8 ) | 'u',
   ( 9 << 8 ) | 'i',
   ( 1 << 8 ) | 'f',
   ( 5 << 8 ) | 'i',
   ( 2 << 8 ) | 'm',
   ( 13 << 8 ) | 'h',
   ( 2 << 8 ) | 'm',
   ( 11 << 8 ) | 'h',
   ( 10 << 8 ) | 'B',
   16,                               /* uni_CNcnwk_1_tree */
   ( 8 << 8 ) | 's',
   ( 2 << 8 ) | 'p',
   ( 4 << 8 ) | 'h',
   ( 1 << 8 ) | 'x',
   ( 69 ),
   ( 1 << 8 ) | 'I',
   16,                               /* uni_CNcnwk_1 */
   ( 1 << 8 ) | 'X',
   ( 272 ),
   26,                               /* uni_CNcnwk_2_phs1 */
   ( 8 << 8 ) | 's',
   ( 1 << 8 ) | 'p',
   ( 2 << 8 ) | 'y',
   ( 1 << 8 ) | 'H',
   26,                               /* uni_CNcnwk_2 */
   ( 1 << 8 ) | 'X',
   ( 282 ),
   23,                               /* uni_CNcnwk_3_go */
   ( 8 << 8 ) | 's',
   ( 1 << 8 ) | 'p',
   ( 3 << 8 ) | 'i',
   ( 1 << 8 ) | 'h',
   ( 1 << 8 ) | 'B',
   23,                               /* uni_CNcnwk_3 */
   ( 1 << 8 ) | 'X',
   ( 290 ),
   12,                               /* uni_CNcnwk_4_reset */
   ( 8 << 8 ) | 's',
   ( 1 << 8 ) | 'p',
   ( 1 << 8 ) | 'I',
   12,                               /* uni_CNcnwk_4 */
   ( 1 << 8 ) | 'X',
   ( 299 ),
   8,                                /* uni_CNcnwk_5 */
   ( 1 << 8 ) | 'L',
   26,                               /* uni_CNcnwk */
   ( 1 << 8 ) | 'J',
   ( 6 << 8) | 8,
   48,                               /* uni_CNul1_1 */
   ( 6 << 8 ) | 'L',
   32,                               /* uni_CNul1_2_s */
   ( 8 << 8 ) | 's',
   ( 2 << 8 ) | 'l',
   ( 2 << 8 ) | 'i',
   ( 2 << 8 ) | 'm',
   ( 4 << 8 ) | 'C',
   32,                               /* uni_CNul1_2 */
   ( 1 << 8 ) | 'X',
   ( 313 ),
   40,                               /* uni_CNul1_3_w */
   ( 8 << 8 ) | 's',
   ( 7 << 8 ) | 'f',
   ( 1 << 8 ) | 'L',
   40,                               /* uni_CNul1_3 */
   ( 1 << 8 ) | 'X',
   ( 322 ),
   64,                               /* uni_CNul1_4 */
   ( 8 << 8 ) | 'L',
   64,                               /* uni_CNul1 */
   ( 1 << 8 ) | 'J',
   ( 7 << 8) | 8,
   32,                               /* uni_CNul2_1_a */
   ( 8 << 8 ) | 's',
   ( 3 << 8 ) | 'f',
   ( 2 << 8 ) | 'L',
   32,                               /* uni_CNul2_1 */
   ( 1 << 8 ) | 'X',
   ( 334 ),
   24,                               /* uni_CNul2_2 */
   ( 3 << 8 ) | 'L',
   28,                               /* uni_CNul2_3_k */
   ( 8 << 8 ) | 's',
   ( 3 << 8 ) | 'p',
   ( 1 << 8 ) | 'U',
   28,                               /* uni_CNul2_3 */
   ( 1 << 8 ) | 'X',
   ( 343 ),
   16,                               /* uni_CNul2_4 */
   ( 2 << 8 ) | 'L',
   12,                               /* uni_CNul2_5_q */
   ( 8 << 8 ) | 's',
   ( 1 << 8 ) | 'p',
   ( 1 << 8 ) | 'I',
   12,                               /* uni_CNul2_5 */
   ( 1 << 8 ) | 'X',
   ( 352 ),
   32,                               /* uni_CNul2 */
   ( 1 << 8 ) | 'J',
   ( 8 << 8) | 8,
   16,                               /* str_EXTRCOLR */
   ( 2 << 8 ) | 's',
   ( 3 << 8 ) | 'm',
   ( 10 << 8 ) | 'B',
   sizeof(struct CONNTYPE),          /* str_CONNTYPE */
   ( 8 << 8 ) | 's',
   ( 19 << 8 ) | 'p',
   ( 1 << 8 ) | 'x',
   ( 223 ),
   ( 1 << 8 ) | 'l',
   ( 3 << 8 ) | 'u',
   ( 1 << 8 ) | 'j',
   ( 5 << 8) | 8,
   ( 1 << 8 ) | 'x',
   ( 257 ),
   ( 8 << 8 ) | 'i',
   ( 2 << 8 ) | 'u',
   ( 2 << 8 ) | 'i',
   ( 1 << 8 ) | 'u',
   ( 1 << 8 ) | 'i',
   ( 2 << 8 ) | 'u',
   ( 3 << 8 ) | 'i',
   ( 2 << 8 ) | 'h',
   ( 2 << 8 ) | 'm',
   ( 1 << 8 ) | 'c',
   ( 3 << 8 ) | 'b',
   ( 2 << 8 ) | 'z',
   ( 1 << 8 ) | 'l',
   ( 2 << 8 ) | 'n',
   ( 1 << 8 ) | 'l',
   ( 1 << 8 ) | 'j',
   ( 6 << 8) | 8,
   ( 1 << 8 ) | 'j',
   ( 7 << 8) | 8,
   ( 1 << 8 ) | 'j',
   ( 8 << 8) | 8,
   ( 6 << 8 ) | 'f',
   ( 2 << 8 ) | 'i',
   ( 6 << 8 ) | 'u',
   ( 32 << 8 ) | 'i',
   ( 1 << 8 ) | 'u',
   ( 2 << 8 ) | 'i',
   ( 5 << 8 ) | 'u',
   ( 4 << 8 ) | 'i',
   ( 12 << 8 ) | 'h',
   ( 1 << 8 ) | 'x',
   ( 362 ),
   ( 4 << 8 ) | 'h',
   ( 13 << 8 ) | 'm',
   ( 1 << 8 ) | 'h',
   ( 1 << 8 ) | 'x',
   ( 69 ),
   ( 1 << 8 ) | 'c',
   ( 2 << 8 ) | 'b',
   ( 3 << 8 ) | 'c',
   ( 6 << 8 ) | 'b',
   ( 2 << 8 ) | 'c',
   ( 17 << 8 ) | 'B',
   sizeof(struct CONNDATA),          /* str_CONNDATA */
   ( 15 << 8 ) | 'I',
   sizeof(struct CPDEF),             /* str_CPDEF */
   ( 8 << 8 ) | 's',
   ( 1 << 8 ) | 'y',
   ( 4 << 8 ) | 'u',
   ( 1 << 8 ) | 'n',
   ( 1 << 8 ) | 'l',
   ( 4 << 8 ) | 'u',
   ( 1 << 8 ) | 'i',
   ( 1 << 8 ) | 'h',
   ( 4 << 8 ) | 'B',
   sizeof(struct FDHEADER),          /* str_FDHEADER */
   ( 8 << 8 ) | 's',
   ( 2 << 8 ) | 'h',
   ( 1 << 8 ) | 'i',
   ( 1 << 8 ) | 'P',
   sizeof(struct IJPLNODE),          /* str_IJPLNODE */
   ( 8 << 8 ) | 's',
   ( 3 << 8 ) | 'p',
   ( 2 << 8 ) | 'm',
   ( 6 << 8 ) | 'h',
   ( 2 << 8 ) | 'B',
   sizeof(struct IONTYPE),           /* str_IONTYPE */
   ( 8 << 8 ) | 's',
   ( 2 << 8 ) | 'p',
   ( 11 << 8 ) | 'f',
   ( 1 << 8 ) | 'h',
   ( 1 << 8 ) | 'm',
   ( 1 << 8 ) | 'h',
   ( 2 << 8 ) | 'B',
   sizeof(struct INHIBBAND),         /* str_INHIBBAND */
   ( 8 << 8 ) | 's',
   ( 1 << 8 ) | 'p',
   ( 4 << 8 ) | 'I',
   sizeof(struct INHIBBLK),          /* str_INHIBBLK */
   ( 8 << 8 ) | 's',
   ( 5 << 8 ) | 'p',
   ( 1 << 8 ) | 'x',
   ( 152 ),
   ( 4 << 8 ) | 'w',
   ( 1 << 8 ) | 'z',
   ( 2 << 8 ) | 'i',
   ( 8 << 8 ) | 'u',
   ( 1 << 8 ) | 'i',
   ( 1 << 8 ) | 'x',
   ( 69 ),
   ( 3 << 8 ) | 'm',
   ( 1 << 8 ) | 'h',
   ( 1 << 8 ) | 'm',
   ( 1 << 8 ) | 'h',
   ( 2 << 8 ) | 'm',
   ( 6 << 8 ) | 'h',
   ( 8 << 8 ) | 'm',
   ( 4 << 8 ) | 'B',
   sizeof(struct IZHICOM),           /* str_IZHICOM */
   ( 4 << 8 ) | 's',
   ( 5 << 8 ) | 'i',
   ( 4 << 8 ) | 'm',
   ( 2 << 8 ) | 'i',
   ( 1 << 8 ) | 'M',
   sizeof(struct IZ03DEF),           /* str_IZ03DEF */
   ( 4 << 8 ) | 's',
   ( 1 << 8 ) | 'x',
   ( 475 ),
   ( 4 << 8 ) | 'I',
   sizeof(struct IZ07DEF),           /* str_IZ07DEF */
   ( 4 << 8 ) | 's',
   ( 1 << 8 ) | 'x',
   ( 475 ),
   ( 2 << 8 ) | 'i',
   ( 2 << 8 ) | 'm',
   ( 16 << 8 ) | 'I',
   8,                                /* uni_MOu1_1 */
   ( 1 << 8 ) | 'P',
   16,                               /* uni_MOu1_2 */
   ( 2 << 8 ) | 'L',
   16,                               /* uni_MOu1 */
   ( 1 << 8 ) | 'J',
   ( 9 << 8) | 8,
   sizeof(struct MODALITY),          /* str_MODALITY */
   ( 8 << 8 ) | 's',
   ( 9 << 8 ) | 'p',
   ( 1 << 8 ) | 'j',
   ( 9 << 8) | 8,
   ( 2 << 8 ) | 'm',
   ( 3 << 8 ) | 'm',
   ( 2 << 8 ) | 'h',
   ( 4 << 8 ) | 'm',
   ( 1 << 8 ) | 'h',
   ( 2 << 8 ) | 'm',
   ( 2 << 8 ) | 'B',
   sizeof(struct MODCOM),            /* str_MODCOM */
   ( 2 << 8 ) | 's',
   ( 1 << 8 ) | 'x',
   ( 362 ),
   ( 3 << 8 ) | 'h',
   ( 1 << 8 ) | 'm',
   ( 4 << 8 ) | 'B',
   sizeof(struct MODBY),             /* str_MODBY */
   ( 8 << 8 ) | 's',
   ( 2 << 8 ) | 'p',
   ( 1 << 8 ) | 'l',
   ( 1 << 8 ) | 'x',
   ( 152 ),
   ( 1 << 8 ) | 'x',
   ( 512 ),
   ( 1 << 8 ) | 'i',
   ( 1 << 8 ) | 'm',
   ( 3 << 8 ) | 'h',
   ( 4 << 8 ) | 'm',
   ( 1 << 8 ) | 'x',
   ( 69 ),
   ( 3 << 8 ) | 'B',
   8,                                /* uni_MVu1_1 */
   ( 1 << 8 ) | 'P',
   8,                                /* uni_MVu1_2 */
   ( 1 << 8 ) | 'W',
   8,                                /* uni_MVu1 */
   ( 1 << 8 ) | 'J',
   ( 10 << 8) | 8,
   sizeof(struct MODVAL),            /* str_MODVAL */
   ( 8 << 8 ) | 's',
   ( 3 << 8 ) | 'p',
   ( 1 << 8 ) | 'j',
   ( 10 << 8) | 8,
   ( 1 << 8 ) | 'z',
   ( 1 << 8 ) | 'x',
   ( 512 ),
   ( 1 << 8 ) | 'i',
   ( 1 << 8 ) | 'i',
   ( 3 << 8 ) | 'B',
   sizeof(struct PCF),               /* str_PCF */
   ( 8 << 8 ) | 's',
   ( 1 << 8 ) | 'p',
   ( 1 << 8 ) | 'm',
   ( 32 << 8 ) | 'I',
   sizeof(struct PHASEDEF),          /* str_PHASEDEF */
   ( 8 << 8 ) | 's',
   ( 1 << 8 ) | 'p',
   ( 3 << 8 ) | 'i',
   ( 1 << 8 ) | 'm',
   ( 4 << 8 ) | 'B',
   sizeof(struct PPFDATA),           /* str_PPFDATA */
   ( 4 << 8 ) | 's',
   ( 2 << 8 ) | 'i',
   ( 2 << 8 ) | 'h',
   ( 2 << 8 ) | 'm',
   ( 1 << 8 ) | 'B',
   48,                               /* str_ITER */
   ( 8 << 8 ) | 's',
   ( 1 << 8 ) | 'p',
   ( 1 << 8 ) | 'l',
   ( 4 << 8 ) | 'N',
   sizeof(struct PRBDEF),            /* str_PRBDEF */
   ( 8 << 8 ) | 's',
   ( 5 << 8 ) | 'p',
   ( 3 << 8 ) | 'i',
   ( 2 << 8 ) | 'h',
   ( 2 << 8 ) | 'm',
   ( 3 << 8 ) | 'b',
   ( 1 << 8 ) | 'x',
   ( 569 ),
   ( 7 << 8 ) | 'i',
   ( 2 << 8 ) | 'M',
   42,                               /* str_UPREPR */
   ( 8 << 8 ) | 's',
   ( 2 << 8 ) | 'p',
   ( 8 << 8 ) | 'm',
   ( 1 << 8 ) | 'u',
   ( 1 << 8 ) | 'm',
   ( 4 << 8 ) | 'B',
   sizeof(struct PREPROC),           /* str_PREPROC */
   ( 8 << 8 ) | 's',
   ( 4 << 8 ) | 'p',
   ( 1 << 8 ) | 'x',
   ( 585 ),
   ( 3 << 8 ) | 'z',
   ( 4 << 8 ) | 'f',
   ( 2 << 8 ) | 'u',
   ( 3 << 8 ) | 'm',
   ( 2 << 8 ) | 'm',
   ( 4 << 8 ) | 'B',
   23,                               /* str_RPDFLT */
   ( 4 << 8 ) | 's',
   ( 4 << 8 ) | 'f',
   ( 1 << 8 ) | 'm',
   ( 2 << 8 ) | 'h',
   ( 1 << 8 ) | 'B',
   sizeof(struct REPBLOCK),          /* str_REPBLOCK */
   ( 8 << 8 ) | 's',
   ( 4 << 8 ) | 'p',
   ( 1 << 8 ) | 'x',
   ( 603 ),
   ( 1 << 8 ) | 'y',
   ( 2 << 8 ) | 'n',
   ( 4 << 8 ) | 'c',
   ( 4 << 8 ) | 'f',
   ( 1 << 8 ) | 'u',
   ( 2 << 8 ) | 'i',
   ( 2 << 8 ) | 'm',
   ( 4 << 8 ) | 'h',
   ( 4 << 8 ) | 'm',
   ( 1 << 8 ) | 'm',
   ( 1 << 8 ) | 'B',
   sizeof(struct RFRCDATA),          /* str_RFRCDATA */
   ( 4 << 8 ) | 's',
   ( 5 << 8 ) | 'i',
   ( 1 << 8 ) | 'M',
   sizeof(struct RPDEF),             /* str_RPDEF */
   ( 8 << 8 ) | 's',
   ( 19 << 8 ) | 'p',
   ( 1 << 8 ) | 'x',
   ( 422 ),
   ( 9 << 8 ) | 'f',
   ( 1 << 8 ) | 'y',
   ( 3 << 8 ) | 'l',
   ( 1 << 8 ) | 'z',
   ( 1 << 8 ) | 'n',
   ( 5 << 8 ) | 'l',
   ( 3 << 8 ) | 'i',
   ( 8 << 8 ) | 'u',
   ( 2 << 8 ) | 'm',
   ( 6 << 8 ) | 'u',
   ( 4 << 8 ) | 'i',
   ( 1 << 8 ) | 'u',
   ( 5 << 8 ) | 'i',
   ( 1 << 8 ) | 'h',
   ( 1 << 8 ) | 'm',
   ( 2 << 8 ) | 'h',
   ( 11 << 8 ) | 'm',
   ( 2 << 8 ) | 'h',
   ( 7 << 8 ) | 'm',
   ( 1 << 8 ) | 'h',
   ( 1 << 8 ) | 'm',
   ( 360 << 8 ) | 'c',
   ( 8 << 8 ) | 'b',
   ( 1 << 8 ) | 'c',
   ( 7 << 8 ) | 'b',
   ( 1 << 8 ) | 'c',
   ( 6 << 8 ) | 'b',
   ( 1 << 8 ) | 'C',
   sizeof(struct TERNARY),           /* str_TERNARY */
   ( 8 << 8 ) | 's',
   ( 1 << 8 ) | 'p',
   ( 6 << 8 ) | 'i',
   ( 1 << 8 ) | 'U',
   sizeof(struct UTVDEF),            /* str_UTVDEF */
   ( 8 << 8 ) | 's',
   ( 2 << 8 ) | 'p',
   ( 4 << 8 ) | 'z',
   ( 2 << 8 ) | 'u',
   ( 15 << 8 ) | 'm',
   ( 1 << 8 ) | 'h',
   ( 14 << 8 ) | 'B',
   12,                               /* uni_VBu1_1_vrp */
   ( 8 << 8 ) | 's',
   ( 1 << 8 ) | 'p',
   ( 1 << 8 ) | 'I',
   12,                               /* uni_VBu1_1 */
   ( 1 << 8 ) | 'X',
   ( 675 ),
   20,                               /* uni_VBu1_2_vhh */
   ( 8 << 8 ) | 's',
   ( 2 << 8 ) | 'p',
   ( 1 << 8 ) | 'F',
   20,                               /* uni_VBu1_2 */
   ( 1 << 8 ) | 'X',
   ( 682 ),
   8,                                /* uni_VBu1_3 */
   ( 1 << 8 ) | 'P',
   12,                               /* uni_VBu1_4_vbbd */
   ( 8 << 8 ) | 's',
   ( 1 << 8 ) | 'p',
   ( 1 << 8 ) | 'F',
   12,                               /* uni_VBu1_4 */
   ( 1 << 8 ) | 'X',
   ( 691 ),
   20,                               /* uni_VBu1 */
   ( 1 << 8 ) | 'J',
   ( 11 << 8) | 8,
   sizeof(struct VBDEF),             /* str_VBDEF */
   ( 8 << 8 ) | 's',
   ( 2 << 8 ) | 'p',
   ( 1 << 8 ) | 'h',
   ( 1 << 8 ) | 'x',
   ( 69 ),
   ( 4 << 8 ) | 'b',
   ( 3 << 8 ) | 'i',
   ( 2 << 8 ) | 'i',
   ( 1 << 8 ) | 'j',
   ( 11 << 8) | 8,
   ( 3 << 8 ) | 'M',
   sizeof(struct VDTDEF),            /* str_VDTDEF */
   ( 4 << 8 ) | 'I',
   sizeof(struct TVDEF),             /* str_TVDEF */
   ( 8 << 8 ) | 's',
   ( 6 << 8 ) | 'p',
   ( 1 << 8 ) | 'x',
   ( 667 ),
   ( 4 << 8 ) | 'z',
   ( 4 << 8 ) | 'f',
   ( 1 << 8 ) | 'i',
   ( 1 << 8 ) | 'h',
   ( 3 << 8 ) | 'm',
   ( 2 << 8 ) | 'm',
   ( 1 << 8 ) | 'M',
   };

#ifdef PAR
   extern unicvtf NXFuni_CTctwk_u;
   extern unicvtf NXFuni_DCDu1_u;
   extern unicvtf NXFuni_CDUn1_u;
   extern unicvtf NXFuni_CDUn2_u;
   extern unicvtf NXFuni_CDUn3_u;
   extern unicvtf NXFuni_CNucij_u;
   extern unicvtf NXFuni_CNcnwk_u;
   extern unicvtf NXFuni_CNul1_u;
   extern unicvtf NXFuni_CNul2_u;
   extern unicvtf NXFuni_MOu1_u;
   extern unicvtf NXFuni_MVu1_u;
   extern unicvtf NXFuni_VBu1_u;

unicvtf *NXDRUT[] = {
   NXFuni_CTctwk_u,
   NXFuni_DCDu1_u,
   NXFuni_CDUn1_u,
   NXFuni_CDUn2_u,
   NXFuni_CDUn3_u,
   NXFuni_CNucij_u,
   NXFuni_CNcnwk_u,
   NXFuni_CNul1_u,
   NXFuni_CNul2_u,
   NXFuni_MOu1_u,
   NXFuni_MVu1_u,
   NXFuni_VBu1_u,
   };
#else
unicvtf *NXDRUT[] = { NULL };
#endif

