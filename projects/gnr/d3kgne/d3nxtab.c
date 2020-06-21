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

/* (c) Copyright 1993-2008, Neurosciences Research Foundation, Inc. */
/* $Id: d3hdrs.c 28 2010-04-12 20:18:17Z  $ */
/***********************************************************************
*                              d3hdrs.c                                *
*  This file contains a list of all the header files that must         *
*  be read by nxdr2.  (Does not need to include files #included        *
*  from other files, e.g. d3global.h)  They must be ordered such       *
*  that symbols are defined before use.                                *
************************************************************************
*  ==>, 12/30/07, GNR - Last mod before committing to svn repository   *
*  V8F, 02/16/10, GNR - Merge in old d3hdrs.h, add utvdef.h            *
***********************************************************************/

#include <stddef.h>
#include "sysdef.h"
#include "collect.h"
#include "d3global.h"
#include "celldata.h"
#include "ijpldef.h"
#include "clblk.h"
#include "d1def.h"
#include "statdef.h"
#include "tvdef.h"

long NXDRTT[] = {
   sizeof(armwdw_type),		/* armwdw_type */
   ( 1 << 8 ) | 'F',
   sizeof(fdm_type),		/* fdm_type */
   ( 1 << 8 ) | 'L',
   sizeof(gac_type),		/* gac_type */
   ( 1 << 8 ) | 'L',
   sizeof(ilst),		/* ilst */
   ( sizeof(void *)<<8 | 's'),
   ( 1 << 8 ) | 'p',
   ( 1 << 8 ) | 'u',
   ( 6 << 8 ) | 'L',
   sizeof(ilstitem),		/* ilstitem */
   ( 1 << 8 ) | 'N',
   sizeof(outnow_type),		/* outnow_type */
   ( 1 << 8 ) | 'H',
   sizeof(si32),		/* si32 */
   ( 1 << 8 ) | 'L',
   sizeof(struct AUTSCL),		/* str_AUTSCL */
   ( sizeof(unsigned long)<<8 | 's'),
   ( 1 << 8 ) | 'l',
   ( 2 << 8 ) | 'n',
   ( 1 << 8 ) | 'l',
   ( 2 << 8 ) | 'M',
   sizeof(struct BREGDEF),		/* str_BREGDEF */
   ( sizeof(unsigned long)<<8 | 's'),
   ( 1 << 8 ) | 'n',
   ( 4 << 8 ) | 'l',
   ( 1 << 8 ) | 'n',
   ( 2 << 8 ) | 'l',
   ( 1 << 8 ) | 'n',
   ( 2 << 8 ) | 'l',
   ( 1 << 8 ) | 'n',
   ( 3 << 8 ) | 'L',
   sizeof(struct CLSTAT),		/* str_CLSTAT */
   ( sizeof(si64)<<8 | 's'),
   ( 4 << 8 ) | 'w',
   ( 44 << 8 ) | 'L',
   sizeof(struct GPSTAT),		/* str_GPSTAT */
   ( 5 << 8 ) | 'L',
   sizeof(struct CTDFLT),		/* str_CTDFLT */
   ( sizeof(long)<<8 | 's'),
   ( 1 << 8 ) | 'f',
   ( 4 << 8 ) | 'l',
   ( 1 << 8 ) | 'n',
   ( 2 << 8 ) | 'l',
   ( 1 << 8 ) | 'n',
   ( 1 << 8 ) | 'l',
   ( 1 << 8 ) | 'n',
   ( 1 << 8 ) | 'm',
   ( 1 << 8 ) | 'h',
   ( 4 << 8 ) | 'b',
   ( 12 << 8 ) | 'C',
   sizeof(struct DECAYDEF),		/* str_DECAYDEF */
   ( sizeof(long)<<8 | 's'),
   ( 1 << 8 ) | 'f',
   ( 1 << 8 ) | 'L',
   sizeof(struct DCYDFLT),		/* str_DCYDFLT */
   ( sizeof(long)<<8 | 's'),
   ( 1 << 8 ) | 'x',
   52, /* str_DECAYDEF */
   ( 2 << 8 ) | 'l',
   ( 1 << 8 ) | 'n',
   ( 2 << 8 ) | 'M',
   sizeof(struct rlnm_str),		/* str_rlnm_str */
   ( sizeof(unsigned short)<<8 | 's'),
   ( 1 << 8 ) | 'h',
   ( 8 << 8 ) | 'C',
   sizeof(union CTctwk),		/* uni_CTctwk_1 */
   ( sizeof(void *)<<8 | 's'),
   ( 1 << 8 ) | 'p',
   ( 1 << 8 ) | 'l',
   ( 1 << 8 ) | 'x',
   63, /* str_rlnm_str */
   ( 2 << 8 ) | 'B',
   sizeof(union CTctwk),		/* uni_CTctwk_2 */
   ( sizeof(void *)<<8 | 's'),
   ( 1 << 8 ) | 'p',
   ( 3 << 8 ) | 'l',
   ( 2 << 8 ) | 'h',
   ( 4 << 8 ) | 'C',
   sizeof(union CTctwk),		/* uni_CTctwk_3 */
   ( sizeof(long)<<8 | 's'),
   ( 1 << 8 ) | 'f',
   ( 1 << 8 ) | 'l',
   ( 3 << 8 ) | 'm',
   ( 1 << 8 ) | 'C',
   sizeof(union CTctwk),		/* uni_CTctwk_4 */
   ( 1 << 8 ) | 'P',
   sizeof(union CTctwk),		/* uni_CTctwk */
   ( 1 << 8 ) | 'J',
   ( 0 << 8 ) | sizeof(void *),
   sizeof(struct CELLTYPE),		/* str_CELLTYPE */
   ( sizeof(void *)<<8 | 's'),
   ( 25 << 8 ) | 'p',
   ( 1 << 8 ) | 'x',
   39, /* str_CTDFLT */
   ( 1 << 8 ) | 'x',
   56, /* str_DCYDFLT */
   ( 6 << 8 ) | 'l',
   ( 2 << 8 ) | 'n',
   ( 1 << 8 ) | 'l',
   ( 3 << 8 ) | 'n',
   ( 3 << 8 ) | 'h',
   ( 1 << 8 ) | 'm',
   ( 4 << 8 ) | 'c',
   ( 1 << 8 ) | 'j',
   (0<<8) | sizeof(void *), /* uni_CTctwk */
   ( 2 << 8 ) | 'n',
   ( 9 << 8 ) | 'l',
   ( 1 << 8 ) | 'n',
   ( 1 << 8 ) | 'l',
   ( 2 << 8 ) | 'm',
   ( 14 << 8 ) | 'h',
   ( 26 << 8 ) | 'm',
   ( 10 << 8 ) | 'b',
   ( 2 << 8 ) | 'C',
   sizeof(struct CLBLK),		/* str_CLBLK */
   ( sizeof(void *)<<8 | 's'),
   ( 3 << 8 ) | 'p',
   ( 1 << 8 ) | 'm',
   ( 1 << 8 ) | 'X',
   63, /* str_rlnm_str */
   sizeof(union CDUn1),		/* uni_CDUn1_1 */
   ( sizeof(unsigned long)<<8 | 's'),
   ( 2 << 8 ) | 'l',
   ( 1 << 8 ) | 'N',
   sizeof(union CDUn1),		/* uni_CDUn1_2 */
   ( 2 << 8 ) | 'N',
   sizeof(union CDUn1),		/* uni_CDUn1_3 */
   ( 2 << 8 ) | 'F',
   sizeof(union CDUn1),		/* uni_CDUn1 */
   ( 1 << 8 ) | 'J',
   ( 1 << 8 ) | sizeof(long),
   sizeof(union CDUn2),		/* uni_CDUn2_1 */
   ( sizeof(long)<<8 | 's'),
   ( 2 << 8 ) | 'f',
   ( 1 << 8 ) | 'M',
   sizeof(union CDUn2),		/* uni_CDUn2_2 */
   ( 2 << 8 ) | 'N',
   sizeof(union CDUn2),		/* uni_CDUn2_3 */
   ( 1 << 8 ) | 'L',
   sizeof(union CDUn2),		/* uni_CDUn2 */
   ( 1 << 8 ) | 'J',
   ( 2 << 8 ) | sizeof(long),
   sizeof(union CDUn3),		/* uni_CDUn3_1 */
   ( 1 << 8 ) | 'B',
   sizeof(union CDUn3),		/* uni_CDUn3_2 */
   ( 1 << 8 ) | 'B',
   sizeof(union CDUn3),		/* uni_CDUn3 */
   ( 1 << 8 ) | 'J',
   ( 3 << 8 ) | sizeof(unsigned char),
   sizeof(struct CONDUCT),		/* str_CONDUCT */
   ( sizeof(void *)<<8 | 's'),
   ( 4 << 8 ) | 'p',
   ( 1 << 8 ) | 'x',
   52, /* str_DECAYDEF */
   ( 1 << 8 ) | 'j',
   (1<<8) | sizeof(long), /* uni_CDUn1 */
   ( 1 << 8 ) | 'j',
   (2<<8) | sizeof(long), /* uni_CDUn2 */
   ( 1 << 8 ) | 'l',
   ( 2 << 8 ) | 'f',
   ( 1 << 8 ) | 'l',
   ( 2 << 8 ) | 'n',
   ( 1 << 8 ) | 'm',
   ( 1 << 8 ) | 'h',
   ( 3 << 8 ) | 'm',
   ( 1 << 8 ) | 'h',
   ( 5 << 8 ) | 'm',
   ( 2 << 8 ) | 'b',
   ( 2 << 8 ) | 'c',
   ( 1 << 8 ) | 'J',
   (3<<8) | sizeof(unsigned char), /* uni_CDUn3 */
   sizeof(struct CNSTAT),		/* str_CNSTAT */
   ( sizeof(si64)<<8 | 's'),
   ( 2 << 8 ) | 'w',
   ( 8 << 8 ) | 'L',
   sizeof(union CNucij),		/* uni_CNucij_1 */
   ( sizeof(unsigned long)<<8 | 's'),
   ( 5 << 8 ) | 'l',
   ( 1 << 8 ) | 'M',
   sizeof(union CNucij),		/* uni_CNucij_2 */
   ( sizeof(long)<<8 | 's'),
   ( 4 << 8 ) | 'f',
   ( 3 << 8 ) | 'l',
   ( 1 << 8 ) | 'm',
   ( 2 << 8 ) | 'B',
   sizeof(union CNucij),		/* uni_CNucij_3 */
   ( sizeof(void *)<<8 | 's'),
   ( 3 << 8 ) | 'p',
   ( 4 << 8 ) | 'l',
   ( 1 << 8 ) | 'M',
   sizeof(union CNucij),		/* uni_CNucij_4 */
   ( 3 << 8 ) | 'L',
   sizeof(union CNucij),		/* uni_CNucij */
   ( 1 << 8 ) | 'J',
   ( 4 << 8 ) | sizeof(void *),
   sizeof(struct CNDFLT),		/* str_CNDFLT */
   ( sizeof(void *)<<8 | 's'),
   ( 2 << 8 ) | 'p',
   ( 1 << 8 ) | 'x',
   52, /* str_DECAYDEF */
   ( 5 << 8 ) | 'l',
   ( 1 << 8 ) | 'n',
   ( 10 << 8 ) | 'l',
   ( 1 << 8 ) | 'm',
   ( 11 << 8 ) | 'h',
   ( 1 << 8 ) | 'm',
   ( 10 << 8 ) | 'h',
   ( 1 << 8 ) | 'c',
   ( 9 << 8 ) | 'B',
   sizeof(union CNcnwk),		/* uni_CNcnwk_1 */
   ( sizeof(void *)<<8 | 's'),
   ( 2 << 8 ) | 'p',
   ( 4 << 8 ) | 'h',
   ( 1 << 8 ) | 'x',
   63, /* str_rlnm_str */
   ( 1 << 8 ) | 'L',
   sizeof(union CNcnwk),		/* uni_CNcnwk_2 */
   ( sizeof(void *)<<8 | 's'),
   ( 1 << 8 ) | 'p',
   ( 2 << 8 ) | 'l',
   ( 1 << 8 ) | 'H',
   sizeof(union CNcnwk),		/* uni_CNcnwk_3 */
   ( sizeof(long)<<8 | 's'),
   ( 6 << 8 ) | 'f',
   ( 1 << 8 ) | 'n',
   ( 2 << 8 ) | 'C',
   sizeof(union CNcnwk),		/* uni_CNcnwk_4 */
   ( sizeof(void *)<<8 | 's'),
   ( 1 << 8 ) | 'p',
   ( 3 << 8 ) | 'l',
   ( 1 << 8 ) | 'm',
   ( 1 << 8 ) | 'h',
   ( 2 << 8 ) | 'B',
   sizeof(union CNcnwk),		/* uni_CNcnwk_5 */
   ( sizeof(void *)<<8 | 's'),
   ( 1 << 8 ) | 'p',
   ( 1 << 8 ) | 'I',
   sizeof(union CNcnwk),		/* uni_CNcnwk_6 */
   ( 2 << 8 ) | 'F',
   sizeof(union CNcnwk),		/* uni_CNcnwk */
   ( 1 << 8 ) | 'J',
   ( 5 << 8 ) | sizeof(void *),
   sizeof(union CNul1),		/* uni_CNul1_1 */
   ( 6 << 8 ) | 'L',
   sizeof(union CNul1),		/* uni_CNul1_2 */
   ( sizeof(unsigned long)<<8 | 's'),
   ( 6 << 8 ) | 'l',
   ( 4 << 8 ) | 'C',
   sizeof(union CNul1),		/* uni_CNul1_3 */
   ( sizeof(long)<<8 | 's'),
   ( 7 << 8 ) | 'f',
   ( 1 << 8 ) | 'L',
   sizeof(union CNul1),		/* uni_CNul1_4 */
   ( 5 << 8 ) | 'L',
   sizeof(union CNul1),		/* uni_CNul1 */
   ( 1 << 8 ) | 'J',
   ( 6 << 8 ) | sizeof(long),
   sizeof(union CNul2),		/* uni_CNul2_1 */
   ( sizeof(long)<<8 | 's'),
   ( 3 << 8 ) | 'f',
   ( 2 << 8 ) | 'L',
   sizeof(union CNul2),		/* uni_CNul2_2 */
   ( 3 << 8 ) | 'L',
   sizeof(union CNul2),		/* uni_CNul2_3 */
   ( 3 << 8 ) | 'L',
   sizeof(union CNul2),		/* uni_CNul2_4 */
   ( 2 << 8 ) | 'L',
   sizeof(union CNul2),		/* uni_CNul2_5 */
   ( 6 << 8 ) | 'L',
   sizeof(union CNul2),		/* uni_CNul2 */
   ( 1 << 8 ) | 'J',
   ( 7 << 8 ) | sizeof(long),
   sizeof(struct CONNTYPE),		/* str_CONNTYPE */
   ( sizeof(si64)<<8 | 's'),
   ( 16 << 8 ) | 'p',
   ( 1 << 8 ) | 'w',
   ( 1 << 8 ) | 'p',
   ( 1 << 8 ) | 'x',
   173, /* str_CNSTAT */
   ( 1 << 8 ) | 'n',
   ( 2 << 8 ) | 'l',
   ( 1 << 8 ) | 'n',
   ( 1 << 8 ) | 'j',
   (4<<8) | sizeof(void *), /* uni_CNucij */
   ( 19 << 8 ) | 'l',
   ( 1 << 8 ) | 'x',
   197, /* str_CNDFLT */
   ( 3 << 8 ) | 'n',
   ( 4 << 8 ) | 'h',
   ( 1 << 8 ) | 'c',
   ( 1 << 8 ) | 'b',
   ( 2 << 8 ) | 'l',
   ( 2 << 8 ) | 'n',
   ( 1 << 8 ) | 'l',
   ( 1 << 8 ) | 'n',
   ( 42 << 8 ) | 'l',
   ( 1 << 8 ) | 'j',
   (5<<8) | sizeof(void *), /* uni_CNcnwk */
   ( 1 << 8 ) | 'j',
   (6<<8) | sizeof(long), /* uni_CNul1 */
   ( 1 << 8 ) | 'j',
   (7<<8) | sizeof(long), /* uni_CNul2 */
   ( 1 << 8 ) | 'l',
   ( 12 << 8 ) | 'h',
   ( 8 << 8 ) | 'b',
   ( 1 << 8 ) | 'm',
   ( 4 << 8 ) | 'h',
   ( 10 << 8 ) | 'm',
   ( 1 << 8 ) | 'x',
   63, /* str_rlnm_str */
   ( 2 << 8 ) | 'c',
   ( 3 << 8 ) | 'b',
   ( 3 << 8 ) | 'c',
   ( 4 << 8 ) | 'b',
   ( 2 << 8 ) | 'c',
   ( 6 << 8 ) | 'B',
   sizeof(struct CPDEF),		/* str_CPDEF */
   ( sizeof(ui64)<<8 | 's'),
   ( 1 << 8 ) | 'y',
   ( 1 << 8 ) | 'n',
   ( 5 << 8 ) | 'l',
   ( 1 << 8 ) | 'i',
   ( 1 << 8 ) | 'n',
   ( 1 << 8 ) | 'h',
   ( 4 << 8 ) | 'B',
   sizeof(struct FDHEADER),		/* str_FDHEADER */
   ( sizeof(void *)<<8 | 's'),
   ( 2 << 8 ) | 'h',
   ( 1 << 8 ) | 'l',
   ( 1 << 8 ) | 'P',
   sizeof(struct IJPLNODE),		/* str_IJPLNODE */
   ( sizeof(void *)<<8 | 's'),
   ( 3 << 8 ) | 'p',
   ( 1 << 8 ) | 'm',
   ( 2 << 8 ) | 'B',
   sizeof(struct IONTYPE),		/* str_IONTYPE */
   ( sizeof(void *)<<8 | 's'),
   ( 2 << 8 ) | 'p',
   ( 11 << 8 ) | 'f',
   ( 1 << 8 ) | 'h',
   ( 1 << 8 ) | 'm',
   ( 1 << 8 ) | 'h',
   ( 2 << 8 ) | 'B',
   sizeof(struct INHIBBAND),		/* str_INHIBBAND */
   ( sizeof(void *)<<8 | 's'),
   ( 3 << 8 ) | 'l',
   ( 1 << 8 ) | 'P',
   sizeof(struct INHIBBLK),		/* str_INHIBBLK */
   ( sizeof(void *)<<8 | 's'),
   ( 6 << 8 ) | 'p',
   ( 1 << 8 ) | 'f',
   ( 1 << 8 ) | 'x',
   63, /* str_rlnm_str */
   ( 8 << 8 ) | 'h',
   ( 6 << 8 ) | 'm',
   ( 4 << 8 ) | 'b',
   ( 1 << 8 ) | 'x',
   52, /* str_DECAYDEF */
   ( 1 << 8 ) | 'l',
   ( 2 << 8 ) | 'h',
   ( 6 << 8 ) | 'l',
   ( 6 << 8 ) | 'p',
   ( 8 << 8 ) | 'L',
   sizeof(struct IZHICOM),		/* str_IZHICOM */
   ( sizeof(unsigned long)<<8 | 's'),
   ( 5 << 8 ) | 'l',
   ( 4 << 8 ) | 'm',
   ( 2 << 8 ) | 'L',
   sizeof(struct IZ03DEF),		/* str_IZ03DEF */
   ( sizeof(unsigned long)<<8 | 's'),
   ( 1 << 8 ) | 'x',
   365, /* str_IZHICOM */
   ( 5 << 8 ) | 'L',
   sizeof(struct IZ07DEF),		/* str_IZ07DEF */
   ( sizeof(unsigned long)<<8 | 's'),
   ( 1 << 8 ) | 'x',
   365, /* str_IZHICOM */
   ( 2 << 8 ) | 'l',
   ( 2 << 8 ) | 'm',
   ( 16 << 8 ) | 'L',
   sizeof(union MOu1),		/* uni_MOu1_1 */
   ( 1 << 8 ) | 'P',
   sizeof(union MOu1),		/* uni_MOu1_2 */
   ( 2 << 8 ) | 'L',
   sizeof(union MOu1),		/* uni_MOu1 */
   ( 1 << 8 ) | 'J',
   ( 8 << 8 ) | sizeof(void *),
   sizeof(struct MODALITY),		/* str_MODALITY */
   ( sizeof(void *)<<8 | 's'),
   ( 9 << 8 ) | 'p',
   ( 1 << 8 ) | 'j',
   (8<<8) | sizeof(void *), /* uni_MOu1 */
   ( 4 << 8 ) | 'm',
   ( 2 << 8 ) | 'h',
   ( 3 << 8 ) | 'm',
   ( 1 << 8 ) | 'h',
   ( 2 << 8 ) | 'm',
   ( 2 << 8 ) | 'B',
   sizeof(struct MODCOM),		/* str_MODCOM */
   ( sizeof(unsigned short)<<8 | 's'),
   ( 8 << 8 ) | 'b',
   ( 3 << 8 ) | 'h',
   ( 1 << 8 ) | 'm',
   ( 2 << 8 ) | 'B',
   sizeof(union MBu1),		/* uni_MBu1_1 */
   ( 1 << 8 ) | 'P',
   sizeof(union MBu1),		/* uni_MBu1_2 */
   ( 1 << 8 ) | 'L',
   sizeof(union MBu1),		/* uni_MBu1 */
   ( 1 << 8 ) | 'J',
   ( 9 << 8 ) | sizeof(void *),
   sizeof(struct MODBY),		/* str_MODBY */
   ( sizeof(void *)<<8 | 's'),
   ( 2 << 8 ) | 'p',
   ( 1 << 8 ) | 'j',
   (9<<8) | sizeof(void *), /* uni_MBu1 */
   ( 2 << 8 ) | 'l',
   ( 1 << 8 ) | 'x',
   52, /* str_DECAYDEF */
   ( 1 << 8 ) | 'x',
   400, /* str_MODCOM */
   ( 3 << 8 ) | 'h',
   ( 4 << 8 ) | 'm',
   ( 1 << 8 ) | 'x',
   63, /* str_rlnm_str */
   ( 2 << 8 ) | 'B',
   sizeof(union MVu1),		/* uni_MVu1_1 */
   ( 1 << 8 ) | 'P',
   sizeof(union MVu1),		/* uni_MVu1_2 */
   ( 1 << 8 ) | 'W',
   sizeof(union MVu1),		/* uni_MVu1 */
   ( 1 << 8 ) | 'J',
   ( 10 << 8 ) | sizeof(si64),
   sizeof(struct MODVAL),		/* str_MODVAL */
   ( sizeof(si64)<<8 | 's'),
   ( 3 << 8 ) | 'p',
   ( 1 << 8 ) | 'j',
   (10<<8) | sizeof(si64), /* uni_MVu1 */
   ( 2 << 8 ) | 'l',
   ( 1 << 8 ) | 'x',
   400, /* str_MODCOM */
   ( 3 << 8 ) | 'B',
   sizeof(struct PCF),		/* str_PCF */
   ( sizeof(void *)<<8 | 's'),
   ( 1 << 8 ) | 'p',
   ( 1 << 8 ) | 'm',
   ( 32 << 8 ) | 'L',
   sizeof(struct PHASEDEF),		/* str_PHASEDEF */
   ( sizeof(void *)<<8 | 's'),
   ( 1 << 8 ) | 'p',
   ( 3 << 8 ) | 'l',
   ( 4 << 8 ) | 'B',
   sizeof(struct PPFDATA),		/* str_PPFDATA */
   ( sizeof(unsigned long)<<8 | 's'),
   ( 2 << 8 ) | 'l',
   ( 2 << 8 ) | 'h',
   ( 2 << 8 ) | 'm',
   ( 1 << 8 ) | 'B',
   sizeof(struct PRBDEF),		/* str_PRBDEF */
   ( sizeof(void *)<<8 | 's'),
   ( 4 << 8 ) | 'p',
   ( 3 << 8 ) | 'l',
   ( 2 << 8 ) | 'h',
   ( 2 << 8 ) | 'm',
   ( 4 << 8 ) | 'b',
   ( 6 << 8 ) | 'l',
   ( 1 << 8 ) | 'M',
   sizeof(struct PREPROC),		/* str_PREPROC */
   ( sizeof(void *)<<8 | 's'),
   ( 4 << 8 ) | 'p',
   ( 2 << 8 ) | 'p',
   ( 4 << 8 ) | 'n',
   ( 5 << 8 ) | 'm',
   ( 2 << 8 ) | 'b',
   ( 1 << 8 ) | 'l',
   ( 5 << 8 ) | 'f',
   ( 3 << 8 ) | 'n',
   ( 6 << 8 ) | 'm',
   ( 5 << 8 ) | 'B',
   sizeof(struct RPDFLT),		/* str_RPDFLT */
   ( sizeof(unsigned short)<<8 | 's'),
   ( 1 << 8 ) | 'm',
   ( 2 << 8 ) | 'h',
   ( 2 << 8 ) | 'B',
   sizeof(struct REPBLOCK),		/* str_REPBLOCK */
   ( sizeof(void *)<<8 | 's'),
   ( 4 << 8 ) | 'p',
   ( 1 << 8 ) | 'x',
   481, /* str_RPDFLT */
   ( 4 << 8 ) | 'c',
   ( 8 << 8 ) | 'l',
   ( 6 << 8 ) | 'f',
   ( 1 << 8 ) | 'm',
   ( 6 << 8 ) | 'h',
   ( 1 << 8 ) | 'm',
   ( 2 << 8 ) | 'B',
   sizeof(struct RFRCDATA),		/* str_RFRCDATA */
   ( sizeof(unsigned long)<<8 | 's'),
   ( 5 << 8 ) | 'l',
   ( 1 << 8 ) | 'M',
   sizeof(struct RPDEF),		/* str_RPDEF */
   ( sizeof(ui64)<<8 | 's'),
   ( 19 << 8 ) | 'p',
   ( 1 << 8 ) | 'x',
   318, /* str_CPDEF */
   ( 9 << 8 ) | 'f',
   ( 12 << 8 ) | 'l',
   ( 6 << 8 ) | 'n',
   ( 4 << 8 ) | 'l',
   ( 1 << 8 ) | 'n',
   ( 4 << 8 ) | 'l',
   ( 4 << 8 ) | 'i',
   ( 1 << 8 ) | 'm',
   ( 1 << 8 ) | 'h',
   ( 1 << 8 ) | 'm',
   ( 2 << 8 ) | 'h',
   ( 8 << 8 ) | 'm',
   ( 1 << 8 ) | 'h',
   ( 1 << 8 ) | 'm',
   ( 3 << 8 ) | 'h',
   ( 3 << 8 ) | 'm',
   ( 1 << 8 ) | 'h',
   ( 1 << 8 ) | 'm',
   ( 360 << 8 ) | 'c',
   ( 8 << 8 ) | 'b',
   ( 1 << 8 ) | 'c',
   ( 5 << 8 ) | 'b',
   ( 1 << 8 ) | 'c',
   ( 7 << 8 ) | 'B',
   sizeof(struct UTVDEF),		/* str_UTVDEF */
   ( sizeof(void *)<<8 | 's'),
   ( 1 << 8 ) | 'p',
   ( 9 << 8 ) | 'm',
   ( 1 << 8 ) | 'h',
   ( 11 << 8 ) | 'B',
   sizeof(union VBu1),		/* uni_VBu1_1 */
   ( sizeof(void *)<<8 | 's'),
   ( 1 << 8 ) | 'p',
   ( 1 << 8 ) | 'L',
   sizeof(union VBu1),		/* uni_VBu1_2 */
   ( sizeof(void *)<<8 | 's'),
   ( 2 << 8 ) | 'p',
   ( 1 << 8 ) | 'F',
   sizeof(union VBu1),		/* uni_VBu1_3 */
   ( 1 << 8 ) | 'P',
   sizeof(union VBu1),		/* uni_VBu1 */
   ( 1 << 8 ) | 'J',
   ( 11 << 8 ) | sizeof(void *),
   sizeof(struct VBDEF),		/* str_VBDEF */
   ( sizeof(void *)<<8 | 's'),
   ( 2 << 8 ) | 'p',
   ( 1 << 8 ) | 'h',
   ( 1 << 8 ) | 'x',
   63, /* str_rlnm_str */
   ( 4 << 8 ) | 'b',
   ( 5 << 8 ) | 'l',
   ( 1 << 8 ) | 'j',
   (11<<8) | sizeof(void *), /* uni_VBu1 */
   ( 2 << 8 ) | 'M',
   sizeof(struct VDTDEF),		/* str_VDTDEF */
   ( 4 << 8 ) | 'L',
   sizeof(struct TVDEF),		/* str_TVDEF */
   ( sizeof(void *)<<8 | 's'),
   ( 6 << 8 ) | 'p',
   ( 1 << 8 ) | 'x',
   531, /* str_UTVDEF */
   ( 1 << 8 ) | 'l',
   ( 4 << 8 ) | 'f',
   ( 2 << 8 ) | 'h',
   ( 2 << 8 ) | 'M',
   };

#ifdef PAR
   extern unicvtf NXFuni_CTctwk_u;
   extern unicvtf NXFuni_CDUn1_u;
   extern unicvtf NXFuni_CDUn2_u;
   extern unicvtf NXFuni_CDUn3_u;
   extern unicvtf NXFuni_CNucij_u;
   extern unicvtf NXFuni_CNcnwk_u;
   extern unicvtf NXFuni_CNul1_u;
   extern unicvtf NXFuni_CNul2_u;
   extern unicvtf NXFuni_MOu1_u;
   extern unicvtf NXFuni_MBu1_u;
   extern unicvtf NXFuni_MVu1_u;
   extern unicvtf NXFuni_VBu1_u;

unicvtf *NXDRUT[] = {
   NXFuni_CTctwk_u,
   NXFuni_CDUn1_u,
   NXFuni_CDUn2_u,
   NXFuni_CDUn3_u,
   NXFuni_CNucij_u,
   NXFuni_CNcnwk_u,
   NXFuni_CNul1_u,
   NXFuni_CNul2_u,
   NXFuni_MOu1_u,
   NXFuni_MBu1_u,
   NXFuni_MVu1_u,
   NXFuni_VBu1_u,
   };
#endif

