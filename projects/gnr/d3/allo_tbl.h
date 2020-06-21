/* (c) Copyright 1990-2018, The Rockefeller University *21115* */
/* $Id: allo_tbl.h 77 2018-03-15 21:08:14Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                        allo_tbl header file                          *
*                                                                      *
*  Contains:  Tables of allocation lengths and orders for dynamically  *
*     allocated variables.  Tables are global so that d3allo, d3fchk,  *
*     d3resetn, and possibly others can access them.                   *
************************************************************************
*  Rev, 02/28/90, JMS - Broken out into separate file                  *
*  Rev, 05/15/90, JWT - Remove swall                                   *
*  Rev, 07/26/92, GNR - Add Cij0 (baseline Cij)                        *
*  V6D, 01/31/94, GNR - Add Dij (delay)                                *
*  V7B, 06/22/94, GNR - Separate lengths for Cij, Cij0 due to subarbor *
*                       skip information stored at end of Cij list,    *
*                       add SAVE_LS_COUNT code                         *
*  V8A, 06/10/95, GNR - Add RGH for regeneration instead of using XRM, *
*                       remove "LTP", add AK, NUK, DST, PHD            *
*  Rev, 09/28/97, GNR - Independent dynamic allocations per conntype,  *
*                       move order tables here from d3allo.c           *
*  Rev, 01/05/98, GNR - Add PPF and PPFT, revise stats length calc.    *
*  Rev, 10/28/00, GNR - Remove XRH, tabulate KRPHR over all cycles     *
*  V8C, 02/22/03, GNR - Cell responses in millivolts                   *
*  V8D, 03/27/05, GNR - Add conductances and ions, first group vars.   *
*  Rev, 08/20/06, GNR - Remove NUK                                     *
*  Rev, 09/27/07, GNR - Add RBAR, 12/26/07, add QBAR                   *
*  ==>, 12/26/07, GNR - Last mod before committing to svn repository   *
*  V8E, 01/12/09, GNR - Add Izhikevich cell response function          *
*  V8H, 10/20/10, GNR - Add xn (once per conntype) variable allocation *
*  Rev, 08/29/12, GNR - Add PSI                                        *
*  R76, 12/02/17, GNR - Add VMP, change IZVU to IZU + IZVr             *
*  R77, 02/08/18, GNR - Add RAW at conntype level, 02/27/18 -->S20/27  *                      *
***********************************************************************/

/* Value used to signal end of table */
#define END_OF_ALLOC_TABLE  127
/* Value used to store partial total used for ls restore */
#define SAVE_LS_COUNT       126

/* Allocation Length Tables
*
*  These tables have one halfword entry for each dynamic variable,
*  indexed by the item code.  Each entry contains the length of the
*  item in bytes.  Negative values indicate special (nonconstant)
*  lengths as defined below.  For these cases, the length is taken
*  from the cvlength array.  This array must be filled in for the
*  current cell and connection type before use.  It is indexed by
*  the absolute value of the item's length table entry.  The zero
*  element of this array is never used.
*/

#define LCIJ0  1     /* sup(nbc/8) = length of CIJ0 data */
#define LXRM   2     /* Sum(ndsbyt) = length of XRM data */
#define LLIJ   3     /* Length of Lij for this connection type */
#define LCIJ   4     /* Length of Cij data */
#define LNUK   5     /* Length of NUK data */
#define LCNDG  6     /* Bytes/cell for conductances (see OPTFC) */
#define LIONC  7     /* Bytes for nionc ions/cell = ESIZE * nionc */
#define LIONG  8     /* Bytes for niong ions/group = ESIZE * niong */
#define LIZR   9     /* Bytes for Izhikevich a,b,c,d random offsets */

#define NumSpecialLengths 9   /* Number of special length codes */

#ifdef MAIN
   short gplength[GPNDV] = {
  -LIONG,            /* code 00: IONG */
      };
   short ctlength[CTNDV] = {
   BSIZE,            /* code 00: RGH  */
   LSmem,            /* code 01: SBAR (mV S7) */
   BSIZE,            /* code 02: DEPR (S8) */
   HSIZE,            /* code 03: DST  (mV S7) or refractory timer */
   -LXRM,            /* code 04: XRM  */
   JSIZE*PHASE_RES,  /* code 05: PHD  */
   HSIZE,            /* code 06: PSI  */
  -LCNDG,            /* code 07: CNDG */
  -LIONC,            /* code 08: IONC */
   LSmem,            /* code 09: QBAR (mV S7) */
   JSIZE,            /* code 10: IZU (mV S20) */
   JSIZE,            /* code 11: IZVr (mV S20) */
   -LIZR,            /* code 12: IZRA+IZRB+IZRC+IZRD (S14) */
   JSIZE,            /* Code 13: VMP (mV S20/27) */
      };
   short xnlength[XNNDV] = {
   -LNUK,            /* code 00: NUK  */
   JSIZE,            /* code 01: AK   */
   HSIZE,            /* code 02: IFD  */
   JSIZE,            /* code 03: RAW  */
      };
   short cnlength[CNNDV] = {
   -LLIJ,            /* code 00: LIJ  */
   -LCIJ,            /* code 01: CIJ  */
  -LCIJ0,            /* code 02: CIJ0 */
   HSIZE,            /* code 03: MIJ  */
   BSIZE,            /* code 04: DIJ  */
   HSIZE,            /* code 05: PPF  */
   HSIZE,            /* code 06: PPFT */
   HSIZE,            /* code 07: RBAR */
      };
   short cvlength[NumSpecialLengths+1];
#else
   extern short gplength[];
   extern short ctlength[];
   extern short xnlength[];
   extern short cnlength[];
   extern short cvlength[];
#endif /* MAIN */

/* Allocation Order Tables
*
*  These tables determine the order in which dynamic variables are
*  allocated.  There are four tables, one each for synapse level,
*  connection type level, cell level, and group level variables.
*  Within each table, variables are ordered in decreasing size, so
*  the best chance of boundary alignment occurs.  Each entry is one
*  byte.  Each table is ended by an END_OF_ALLOC_TABLE (=0xff).
*  These tables permit item codes to be assigned sequentially as
*  new variables are added to the program without prejudice to level
*  and boundary alignment considerations.  */

#ifdef MAIN

/* (i) Group level order table */

   char ordgp[] = { IONG, END_OF_ALLOC_TABLE };

/* (ii) Cell type (layer) order table.  Any variables that will not
*  be restored from the SAVENET file, e.g. PHD, XRM, etc. must be
*  placed after the SAVE_LS_COUNT code.  As a special dispensation
*  to simplify handling of PHD, if any of these variables is
*  present, alignment is forced back on a 4-byte word boundary.  */

   char ordct[] = { VMP, IZU, IZVr, IONC, SBAR, QBAR, DST, PSI,
      DEPR, IZRA, CNDG, SAVE_LS_COUNT,
      PHD, XRM, RGH, END_OF_ALLOC_TABLE };

/* (iii) Connection type order table */

   char ordxn[] = { AK, RAW, IFD, NUK, END_OF_ALLOC_TABLE };

/* (iv) Connection data order table */

   char ordcn[] = { RBAR, PPF, PPFT, MIJ, DIJ,
         LIJ, CIJ, CIJ0, END_OF_ALLOC_TABLE };

#else
   extern char ordgp[];
   extern char ordct[];
   extern char ordxn[];
   extern char ordcn[];
#endif
