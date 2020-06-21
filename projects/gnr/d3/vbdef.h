/* (c) Copyright 1988-2017, The Rockefeller University *11116* */
/* $Id: vbdef.h 76 2017-12-12 21:17:50Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                         VBDEF.H Header File                          *
*    Define value block (VBDEF) and value data (VDTDEF) structures     *
*                                                                      *
************************************************************************
*  V4A, 06/10/88, Initial version                                      *
*  Rev, 08/03/91, GNR - Add SNOUT value, isolate dependencies on       *
*                       arithmetic value of kval by adding VBKID,VBNID *
*  V5C, 12/07/91, GNR - Add vbase, u1                                  *
*  Rev, 02/14/92, GNR - Add D1ENV and D1CAT value schemes              *
*  Rev, 03/26/92, GNR - Add AVGSI value scheme                         *
*  V8A, 04/08/95, GNR - ENVVAL pval points to MODALITY, remove D1ENV,  *
*                       recode vbkid, remove vbnid                     *
*  Rev, 08/15/96, GNR - Add VBVOPAV and VBVOPFS bits                   *
*  V8B, 12/28/00, GNR - Move to new memory management routines, add    *
*                       separate threadings for rep vs environ types   *
*  V8D, 02/13/07, GNR - Add BBD type                                   *
*  ==>, 12/24/07, GNR - Last mod before committing to svn repository   *
*  V8F, 02/20/10, GNR - Remove PLATFORM support                        *
*  R67, 04/27/16, GNR - Remove Darwin 1 support                        *
*  R76, 10/20/17, GNR - Add CONSTANT value, longs to si32s             *
***********************************************************************/


/* Value schemes (values taken on by kval):
*  CAUTION:  When adding new values, be sure to update VBKID,
*  vbtnames in d3valu, value spec printing in d3echo, etc.  */
#define VB_TENT   0  /* TENT   */
#define VB_FIRE   1  /* NFIRE  */
#define VB_AVGSI  2  /* AVGSI  */
#define VB_DIST   3  /* HHDIST */
#define VB_RING   4  /* HHRING */
#define VB_DLIN   5  /* HHDLIN */
#define VB_RLIN   6  /* HHRLIN */
#define VB_UJMD   7  /* UJMODE */
#define VB_GRIP   8  /* GRIP   */
#define VB_ENVL   9  /* ENVVAL */
#define VB_CONS  10  /* CONSTANT */
                     /* Code 11 currently not used */
#ifdef BBDS
#define VB_BBD   12  /* BBD    */
#endif
#define VBNUM    13  /* Number of value scheme types */

/* Bits indicating usage of vbase, vbest, vhalf vs VB code */
#define EVBBASE 0x05e3
#define EVBBEST 0x0003
#define EVBDAMP 0x13ff
#define EVBHALF 0x0003

/* Kind of value scheme as a function of kval */
#define VBK_REP   0  /* Repertoire  */
#define VBK_DIST  1  /* Distance    */
#define VBK_UJG   2  /* UJM or GRIP */
#define VBK_ENV   3  /* Environment */
#define VBK_CONS  4  /* Constant    */
                     /* Code 5 currently not used */
#define VBK_BBD   6  /* BBD socket  */
/* Note:  Leave codes in for schemes not compiled */
#define VBKID {0,0,0,1,1,1,1,2,2,3,4,5,6}

#define DFLT_VALUE   SFIXP(0.5,8)   /* Default value of the value */

/* Structure to define a value scheme */
struct VBDEF {
   struct VBDEF *pnxvb;       /* Ptr to next VB record */
   struct VBDEF *pnxvt;       /* Ptr to next VB in rep or env thread */
   short  ivdt;               /* Index of this VDTDEF entry (0 base) */
   rlnm valnm;                /* Value target rep-layer name */
   byte kval;                 /* Value scheme (see above flags) */
   byte vbkid;                /* Generic value scheme type */
   byte vbopt;                /* Option bits -- from user input */
#define VBVOPNP   0x01           /* N No plot             (VBVOP0N) */
#define VBVOPBR   0x02           /* B Bypass reset        (VBVOP0B) */
#define VBVOPAV   0x04           /* A Absolute value                */
#define VBVOPFS   0x08           /* F Fast startup                  */
#define VBVOP2C   0x10           /* 2 Plot in second column         */
   byte vbflg;                /* Flag bits -- internal */
#define VBFLGC1   0x01           /* First cycle in run              */
#define VBFLGCR   0x02           /* First cycle since reset         */
#define VBFLCON   0x04           /* Value is constant               */
   si32 vbase;                /* Base value (FBvl=8) */
   si32 vmin;                 /* Min value to avoid penalty (FBvl=8) */
   si32 vscl;                 /* Scale factor for value (FBsc=24)   */
   int  vbest;                /* Number firing for optimal value */
   int  vhalf;                /* Half-width of value curve */
   union VBu1 {               /* Parameters for specific value types */
      /* N.B.  Only vrp is transmitted to comp nodes */
      struct {                /* REP type: */
         struct CELLTYPE *pvsrc; /* Ptr to source CELLTYPE */
         si32 omvbd2;            /* = (1-vbase)/2 (FBvl=8) */
         } vrp;
      struct {                /* DIST or UJG type: */
         ARM *mvarm;             /* Ptr to relevant arm block */
         WINDOW *mvwdw;          /* Ptr to relevant window block */
         float vradm2;           /* = -1/rad**2 */
         } vhh;
      struct {                /* ENV type: */
         struct MODALITY *pvmdl; /* Ptr to source MODALITY */
         } vev;
#ifdef BBDS
      struct {                /* BBD type: */
         BBDTYPE *pbvdev;        /* Ptr to BBD info */
         float fbbdval;          /* The actual (float) value */
         } vbbd;
#endif
      } u1;
   ui16 iovec;                /* Overflow event code */
   ui16 vdamp;                /* Damping factor for value (FBdf=15) */
   ui16 vdelay;               /* Delay after reset */

   };

/* Structure to contain results of value calculations.
*  (Padded so size is a multiple of long alignment).  */

struct VDTDEF {
   si32 fullvalue;            /* Full value (FBvl=8) for this cycle */
   si32 cnstrval;             /* Value coerced to 0<=V<1.0 (FBvl=8) */
   si32 vdampprv;             /* vdamp(t-1) (FBvl=8) */
   si32 vcalcnow;             /* vcalc(t) before scaling (FBvl=8) */
   };

