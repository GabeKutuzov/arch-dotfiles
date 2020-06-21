/* (c) Copyright 1988-2016, The Rockefeller University *11115* */
/* $Id: modby.h 74 2017-09-18 22:18:33Z  $ */
/***********************************************************************
*                             CNS Program                              *
*    MODBY Header File - Defines MODCOM, MODBY, and MODVAL blocks      *
*                                                                      *
************************************************************************
*  V4A, 06/10/88, Initial version                                      *
*  V5C, 12/07/91, GNR - Remove pgsrc to modval block, reorder          *
*  V6A, 03/29/93, GNR - Add mdist statistic                            *
*  V7A, 05/01/94, GNR - Add modulation delay, rename opt->mopt, pha    *
*  V7C, 08/28/94, GNR - Add modulation decay                           *
*  V8A, 09/23/95, GNR - Add mvaleff, mxmod, oas, pdist, phasing block, *
*                       rename all vars to start with 'm'              *
*  Rev, 10/04/96, GNR - Put mvaleff in union for broadcast protection  *
*  Rev, 12/29/97, GNR - Add omdist, mpers                              *
*  Rev, 10/04/98, GNR - Put mdsum in a union for broadcast protection  *
*  V8C, 05/04/03, GNR - Scale of mxmod,mvalcal,etc. to S20, mt to S7   *
*  V8D, 06/03/06, GNR - Remove phasing options--always treat as PHASU  *
*  ==>, 12/28/07, GNR - Last mod before committing to svn repository   *
*  Rev, 04/21/08, GNR - Add ssck (sum sign check) mechanism            *
*  Rev, 05/03/08, GNR - MOPT=K and COMPAT=K for knee response          *
*  Rev, 05/06/09, GNR - Add mjrev                                      *
*  Rev, 05/13/09, GNR - Add MODCOM, MOPUNQ bit, incorporate MODVAL     *
*  V8H, 03/31/11, GNR - Add mdefer                                     *
*  Rev, 04/09/12, GNR - Add MOPT=F, mdefer in MODVAL is minimum        *
*  Rev, 12/24/12, GNR - New overflow checking                          *
*  Rev, 02/16/13, GNR - mvaleff is now MDCY.peipv                      *
*  Rev, 04/03/13, GNR - Add IBOPMX                                     *
*  R66, 02/22/16, GNR - mxmod to si32                                  *
***********************************************************************/

struct MODCOM {            /* Params ref'd by MODVAL from MODBY */
   struct EXTRCOLR mvxc;   /* Color extraction parameters */
   short mt,mtlo;          /* Modulation thresholds (mV S7/14) */
   short mjrev;            /* Mod reversal potential (mV S7/14) */
   ui16 mdefer;            /* Cycles to defer modulation */
   byte mdelay;            /* Modulation delay */
   byte mopt;              /* Options--IBOP must match inhibblk: */
#define MOPUNQ       0x01     /* U Uniquify MODVAL block */
#define MOPNEG       0x02     /* N Apply only neg modulation */
#define IBOPMX       0x04     /* M Use max(input), not sum(input) */
#define IBOPKR       0x08     /* K Knee response (mt subtracted) */
#define IBOPTQ       0x10     /* Q Squared modulation */
#define IBOPTS       0x20     /* S Shunting modulation */
#define IBOPFS       0x40     /* F Reset & fast start on new trial */
#define IBOPOR       0x80     /* O Override restore */
   byte mpspov;            /* Modulatory PSP usage overrides */
   byte jself;             /* 1 if self input */
   };

struct MODBY {
   struct MODBY *pmby;     /* Ptr to next modby block */
   struct MODVAL *pmvb;    /* Pointer to modulation value block */
   long mscl;              /* Scale factor for modulation (S20) */
   struct DECAYDEF MDCY;   /* Modulation decay parameter block */
   struct MODCOM Mc;       /* Summation parameters */
   si32  mxmod;            /* Maximum modulation (mV S20) */
   ui16  mfscyc;           /* Cycle to perform IBOPFS reset */
   short mct;              /* Mod conn number (0 for NMOD) */
   short mto,mtolo;        /* Mod thresh (old style) (mV S7/14) */
   ui16  omafs;            /* Offset into modulatory affs data */
   ui16  omsei[NEXIN];     /* Offset into excit/inhib AffSums */
   ui16  omdist;           /* Offset to mdist in pctddst array */
#define OmVd 0                /* Offset to vdist from omdist */
#define OmWd 1                /* Offset to wdist from omdist */
   rlnm msrcnm;            /* Modulation source rep-layer name */
   byte msrctyp;           /* Modulation source type */
   byte mssck,mssck0;      /* Modulation sum sign check */
   };

/* N.B.  A copy of one parent Mc is placed in the MODVAL block,
*  rather than a pointer to it, because our broadcast system has
*  no way to update a pointer into a block on a PAR system. This
*  block is quite small, so there is no great storage penalty. */
struct MODVAL {
   struct MODVAL *pmdv;    /* Ptr to next MODVAL block */
   gvin_fn mdgetfn;        /* Ptr to function to get data */
   void    *amsrc;         /* Ptr to actual source cells */
   union MVu1 {            /* Allows mdsum to be protected from
                           *  change during a broadcast  */
      void *pgsrc;            /* Ptr to source block (tchk->snsa) */
      si64 mdsum;             /* Actual mod sum (S14 = mV S7) */
      } umds;
   size_t omsrc;           /* Offset in pbcst to virtual src */
   struct MODCOM Mdc;      /* Summation params */
   int  iovec;             /* Offset to overflow count */
   si32 mdnel;             /* Number of terms in sum */
   byte mdincr;            /* Increment in source array */
   byte mdsrcndx;          /* Modulation source index */
   byte mdsrctyp;          /* Type of connection source as
                           *  defined in d3global.h (0 = rep) */
   };
