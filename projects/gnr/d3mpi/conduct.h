/* (c) Copyright 2000-2016, The Rockefeller University *11116* */
/* $Id: conduct.h 73 2017-04-26 20:36:40Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                        CONDUCT.H Header File                         *
*  This file contains control structures for CONDUCTANCES and IONS     *
*                                                                      *
************************************************************************
*  V8D, 01/24/04, GNR - New file                                       *
*  ==>, 12/28/07, GNR - Last mod before committing to svn repository   *
*  Rev, 10/25/13, GNR - Eliminate 'L' constants                        *
*  R67, 10/05/16, GNR - Place kcond, kactv before unions for ucvt      *
***********************************************************************/

#define Lg            3       /* Length of conductance data, full
                              *  and per cell  (CNDG) */
#define LgHNT (Lg+2*HSIZE)    /* Length of conductance with hit info */
#define ognh         Lg       /* Offset to number of hits */
#define ogth   (Lg+HSIZE)     /* Offset to time of last hit */

/*---------------------------------------------------------------------*
*                    COND (Conductance or Channel)                     *
*---------------------------------------------------------------------*/

struct CONDUCT {
   struct CONDUCT *pncd;      /* Ptr to next conductance */
   struct IONTYPE *piond;     /* Ptr to iond (dependence) info */
   struct IONTYPE *pionr;     /* Ptr to ionr (reversal) info */
   struct IONTYPE *piont;     /* Ptr to iont (thresh) info */
   struct DECAYDEF gDCY;      /* Traditional CNS decay info */
   byte  cdopts;              /* Conductance option flags */
#define CDOP_IONREV  0x01        /* Erev from pionr */
#define CDOP_IONEXT  0x02        /* Affects external ion conc */
#define CDOP_IONINT  0x04        /* Affects internal ion conc */
   byte  cdflgs;              /* Conductance control flags */
#define CDFL_GgDCY   0x01        /* Cond is gated & kdcy != 0 */
#define CDFL_GNDCY   0x02        /* Cond is gated & kdcy == 0 */
#define CDFL_ACTEXT  0x04        /* Activated by external ion */
#define CDFL_HITNT   0x08        /* Space allocated for hit
                                 *  numbers and times */
   schr  kcond;               /* Kind of conductance */
#define CDTP_PASSIVE      0
#define CDTP_LINEAR       1
#define CDTP_GATED        2      /* Above this value: gated */
#define CDTP_ALPHA        2
#define CDTP_DOUBLE       3
#define CDTP_N            4      /* Number of conductance types */
#define CDTP_NDECAY       4      /* Invalid value used in decay test */
   schr  kactv;               /* Kind of activation */
#define CDAC_CELLFIRE     0
#define CDAC_SYNAPTIC     1
#define CDAC_IONIC        2
#define CDAC_POISSON      3
#define CDAC_N            4      /* Number of activation types */
   union CDUn1 {              /* Type ifno per kcond */
      struct {
         si32  vth;              /* Volt thresh (mV S20/27) */
         si32  vmax;             /* E for imax (mV S20/27) */
         ui32  vmmth;            /* vmax - vth */
         } lin;
      struct {
         ui32  ttpk;             /* Time to peak (ms S20) */
         ui32  taum;             /* Cell time const (ms S20) */
         } alf;
      struct {
         float taur;             /* Rise time (ms) */
         float taud;             /* Decay time (ms) */
         } dbl;
      } ugt;
   union CDUn2 {              /* Activation info per cdflgs */
      struct {
         float ionth;            /* Ion thresh (uM) */
         float react;            /* Reactivation thresh (uM) */
         txtid hiont;            /* Handle to iont name */
         } ion;
      struct {
         ui32  pppc;             /* Probability per cycle (S28) */
         orns  seed;             /* Random number seed */
         } psn;
      si32  at;                  /* Volt thresh (mV S20/27) */
      } uga;
   float cdcycut;             /* Conductance decay cutoff */
   float gfac;                /* Ion multiplier f (/uM) */
   si32  Vrev;                /* Fixed value of Vrev (mV S20/27) */
   ui32  gbar;                /* Maximum conductance (nS S20) */
   ui32  cdvin;               /* Data values entered flags */
   ui32  ogcat;               /* Offset to activation table */
   ui16  ogrd;                /* Offset to this cond in rep data */
   short gexp;                /* g exponent (S8) */
   txtid hcdnm;               /* Handle to conductance name */
   txtid hiond;               /* Handle to iond name */
   txtid hionr;               /* Handle to ionr name */
   short iact;                /* Index of activation conntype */
   ui16  nc2dc;               /* Number of cycles to decay cutoff */
   ui16  ocafs;               /* Offset into conducance affs data */
   ui16  ocsei[NEXIN];        /* Offset into excit/inhib AffSums */
   ui16  refrac;              /* Refractory period for reactivation */
   byte  cpspov;              /* Conductance PSP usage overrides */
   byte  cssck,cssck0;        /* Conductance sum sign check */
   union CDUn3 {              /* Protect igi from broadcast */
      byte igi;                  /* Integer bits in CNDG data */
      byte igidum;
      } ugi;
   };

/* Position codes for CONDUCTANCE keywords.  (These must
*  agree with the convention for ic setting by kwscan and
*  with cndreqnm array in d3cchk.)  */
#ifdef NEEDCDEFS
#define CDKY_REQD1 10            /* Position of first option in CDKY
                                 *  list that is sometimes required */
#if BYTE_ORDRE > 0               /* BIG-ENDIAN MACHINE */
#define CDKY_TYPE   0x80000000   /* Not required: default PASSIVE */
#define CDKY_IOND   0x40000000   /* Not required: default NO IONDEP */
#define CDKY_VREV   0x20000000   /* Not required: default 0 */
#define CDKY_GEXP   0x10000000   /* Not required: default 1 */
#define CDKY_OPT    0x08000000   /* Not required: default NONE */
#define CDKY_REFR   0x04000000   /* Not required: default 0 */
#define CDKY_IONC   0x02000000   /* Not required unless OPT entered */
#define CDKY_DCUT   0x01000000   /* Not required: default 0.01 */
#define CDKY_DCY    0x00800000   /* Not required: CNS-style decay */
#define CDKY_ACTV   0x00400000   /* Required if ALPHA or DOUBLE */
#define CDKY_GBAR   0x00200000
#define CDKY_VTH    0x00100000
#define CDKY_VMAX   0x00080000
#define CDKY_TTPK   0x00040000
#define CDKY_TAUM   0x00020000
#define CDKY_TAUR   0x00010000
#define CDKY_TAUD   0x00008000
#define CDKY_IONA   0x00004000
#define CDKY_PPPC   0x00002000
#define CDKY_SEED   0x00001000
#define CDKY_AT     0x00000800
#else                           /* LITTLE-ENDIAN MACHINE */
#define CDKY_TYPE   0x00000080   /* Not required: default PASSIVE */
#define CDKY_IOND   0x00000040   /* Not required: default NO IONDEP */
#define CDKY_VREV   0x00000020   /* Not required: default 0 */
#define CDKY_GEXP   0x00000010   /* Not required: default 1 */
#define CDKY_OPT    0x00000008   /* Not required: default NONE */
#define CDKY_REFR   0x00000004   /* Not required: default 0 */
#define CDKY_IONC   0x00000002   /* Not required unless OPT entered */
#define CDKY_DCUT   0x00000001   /* Not required: default 0.01 */
#define CDKY_DCY    0x00008000   /* Not required: CNS-style decay */
#define CDKY_ACTV   0x00004000   /* Required if ALPHA or DOUBLE */
#define CDKY_GBAR   0x00002000
#define CDKY_VTH    0x00001000
#define CDKY_VMAX   0x00000800
#define CDKY_TTPK   0x00000400
#define CDKY_TAUM   0x00000200
#define CDKY_TAUR   0x00000100
#define CDKY_TAUD   0x00800000
#define CDKY_IONA   0x00400000
#define CDKY_PPPC   0x00200000
#define CDKY_SEED   0x00100000
#define CDKY_AT     0x00080000
#endif

#endif   /* NEEDCDEFS */

/*---------------------------------------------------------------------*
*                      CONDSET (Conductance Set)                       *
*---------------------------------------------------------------------*/

struct CONDSET {
   struct CONDSET *pncs;      /* Ptr to next CONDSET */
   txtid *phcsnms;            /* Ptr to list of included hcdnms */
   txtid  hcsnm;              /* Handle to name of this set */
   ui16   nhallo;             /* Number of handles allocated */
   ui16   nhused;             /* Number of handles used */
   };
#define CSETINCR  10          /* Incremental list size */

/*---------------------------------------------------------------------*
*                    IONTYPE (Ionic Concentration)                     *
*                                                                      *
*  Note:  The range of possible concentrations and volumes precludes   *
*  use of scaled fixed-point here.  Results may differ slightly on     *
*  machines with non-IEEE floating point.  To correct this would       *
*  require use of computed binary scales and also writing a fixed-     *
*  point log routine for the Nernst equation.                          *
*---------------------------------------------------------------------*/

struct IONTYPE {
   struct IONTYPE *pnion;     /* Ptr to next ion */
#ifdef PAR
   struct IONTYPE *pnionexch; /* Ptr to next ion to be exchanged */
#endif
   float  *pCext;             /* Ptr to external conc */
   float  Cext;               /* Current external conc  (mM) */
   float  DCext;              /* Change in Cext (mM) */
   /* N.B. In next 4 var pairs, code assumes 'int' precedes 'ext' */
   float  C0int;              /* Initial/reset internal conc (mM) */
   float  C0ext;              /* Initial/reset external conc (mM) */
   float  Cminint;            /* Minimum internal conc  (mM) */
   float  Cminext;            /* Minimum external conc  (mM) */
   float  Vint;               /* Effective internal volume (um3) */
   float  Vext;               /* Effective external volume (um3) */
   float  tauint;             /* Internal decay time (msec) */
   float  tauext;             /* External decay time (msec) */
   float  zF;                 /* Z x Faraday const (scaled) */
   short  oCi;                /* Offset to internal ion conc in prd
                              *  data.  N.B.: oCe (offset to Cext) is
                              *  (oCi+4) if Cext is per cell.  oCi is
                              *  same for all CELLTYPEs in the region
                              *  if this is a region-level ion */
   txtid  hionnm;             /* Handle to name */
   short  z;                  /* Ion charge (integer) */
   byte   ionopts;            /* User options */
#define ION_REGION   1           /* External conc is regional */
#define ION_GROUP    2           /* External conc is per group */
#define ION_CELL     4           /* External conc is per cell */
   byte   ionflgs;            /* Control flags */
                                 /* N.B.  Four low-order bits are
                                 *  reserved to carry CDOP flags.  */
#define ION_USED   0x10          /* This IONTYPE is in use */
   };

