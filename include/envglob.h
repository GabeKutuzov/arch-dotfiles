/* (c) Copyright 1988-2016, The Rockefeller University *21116* */
/* $Id: envglob.h 64 2018-05-07 21:58:15Z  $ */
/***********************************************************************
*                         Environment Program                          *
*                         ENVGLOB Header File                          *
*  Global definitions for CNS environment.  (These are used by the     *
*  environment routines themselves--should not be needed by callers.)  *
*                                                                      *
*  V4A, 12/01/88, JWT - It is ALIVE                                    *
*  Rev, 03/28/90, GNR - Added reference number to object lookup table  *
*  Rev, 04/04/90, GNR - Removed hex stimulus files, added pixel files  *
*  Rev, 01/15/92, GNR - Added support for colored environment          *
*  Rev, 03/03/92, GNR - Change filedefs to RFdefs                      *
*  Rev, 05/09/92, GNR - D3NAM_LENGTH-->LSNAME, KSVSH out, add KSVRT    *
*  Rev, 06/06/92, GNR - Delete ESV's, use KSV's defined in env.h       *
*  Rev, 08/09/92, GNR - Define LIBHANDLE structure for envlod port     *
*  Rev, 08/27/92, GNR - Added KSVOC bit to permit "or"-ing objects     *
*  Rev, 05/29/95, GNR - Removed private CNS flag defs to CNS headers   *
*  Rev, 04/10/97, GNR - Merge with envdef.h, eliminate envnoph.h and   *
*                       envph.h & need for caller to access internals  *
*  ==>, 08/18/07, GNR - Last date before committing to svn repository  *
*  Rev, 11/28/10. GNR - Eliminate COLMAXCH collision with CNS          *
*  Rev, 01/06/11, GNR - Eliminate #defined names for internal structs  *
*  Rev, 02/23/16, GNR - Seeds to si32 for 64-bit compilation           *
***********************************************************************/

#ifndef ENVGLOB_HDR_INCLUDED
#define ENVGLOB_HDR_INCLUDED

#include "env.h"
#include "dslb.h"
#include "objdef.h"

/* Configuration parameters */
#define MAXENV         15     /* Max env dimension (log 2) */
#define LIB_REC_SIZE   80     /* Size of shape library record */
#define MAX_OBJ_SIZE   12     /* Largest allowed Darwin II obj size */
#define NARMPOS        13     /* Number of possible arm positions */
#define POBJRF_INCR    32     /* Incremental size of pobjrf table--
                              *  must be a power of two--see envctl */
#define END_OF_RECS    -1     /* End of save rec marker */

#define NORM            1     /* Regular object movement */
#define JUMP            2     /* Object jump */

#define SEARCH_LIB 0    /* envloc() search for lib shape only */
#define SEARCH_ALL 1    /* envloc() search for lib or user shape */
#define NOT_FOUND -1    /* envloc() return when shape not found */

#define PI2      6.2831853       /* 2*pi */
#define TORADS   1.745329e-2     /* Degrees to radians */
#define ZEROISH  1.0e-4          /* Zero, give or take... */
#define SM16     1.525879e-5     /* 2**-16 */
#define S16  65536               /* 2**16 */
#define FS16 65536.0             /* (float)S16 */

#define sign(x,y)  ((y)<0.0 ? -fabs(x) : fabs(x)) /* FORTRAN sign()   */

/* Round a double or float in cast */
#define RTOJRND(x) ((si32)((x) + 0.5))

struct ESVDEF {               /* Object information in replay file    */
   char  shpname[SHP_NAME_SIZE];
   float orien;
   long  xcurr,ycurr;
   short xmin,xmax;
   short ymin,ymax;
   short xsize,ysize;
   short z;
   short flag;
   } ;

/* The following definition must agree with that in vbdef.h */
#define DFLT_ENVVAL  0x8000   /* ENVVAL code to use default (0.5) */

/* Define shape library index record */
struct LIBINDEF {
   char shape_name[4];        /* Name of shape */
   si32 recnum;               /* Record number of shape */
   };

/* Define shape library handle */
#define LIBHANDLE struct LIBHNDDEF
struct LIBHNDDEF {            /* Handle for library or loaded object  */
   struct DSLBDEF *pmemsh;    /* Ptr to memory shape or NULL if lib   */
   si32 libsh;                /* Number of lib shape                  */
   } ;

/* Define ENVDEF environment control block.  (Now supports multiple
*  environments in linked list format and incorporates the GRAPHBLK
*  block of previous implementations.)  */
struct ENVDEF {
   struct ENVDEF *pnxenv;     /* Ptr to next environment              */
   struct ARMDEF *parm1;      /* Ptr to dynamic arm info              */
   struct WDWDEF *pwdw1;      /* Ptr to dynamic window info           */
   struct OBJDEF *pobj1;      /* Ptr to object list                   */
   struct LIBINDEF *plibid;   /* Ptr to shape library index array     */
   struct DSLBDEF *pexshp;    /* Ptr to external shape list           */
   struct DSLBDEF **pshpfill; /* Ptr to link field of last shape,     */
                              /* i.e. ptr to where next one built     */
   struct OBJDEF **pobjrf;    /* Ref number to object lookup table    */
   struct RFdef *libfile;     /* Ptr to shape library file record     */
   struct RFdef *repfile;     /* Ptr to replay activity records       */
   struct RFdef *pixfile;     /* Ptr to pixel format stim records     */
   si32  *prnd1;              /* Ptr to dynamic unoise array          */
   float *prnd2;              /* Ptr to dynamic anoise array          */
   float anoise;              /* Mean stimulus noise amplitude        */
   float snoise;              /* Standard dev of stimulus noise       */
   float epr;                 /* Environment plot radius              */
   float eslht;               /* Env standard letter height           */
   float rlx;                 /* Float(lx)                            */
   float rly;                 /* Float(ly)                            */
   long callnum;              /* Count of calls to envget()           */
   long cycnum;               /* Cumulative trial number              */
   long ept;                  /* Environment plot threshold (S8)      */
   long iprerf;               /* Largest ref number yet assigned      */
   long libnum;               /* Number of objects in library         */
   long pixnum;               /* Number of images in pixel file       */
   long lxt2m1;               /* 2*lx-1 (S16)                         */
   long lyt2m1;               /* 2*ly-1 (S16)                         */
   long lymul2;               /* 2*ly                                 */
   long ltotal;               /* 2*lx*ly                              */
   si32 iseed1;               /* Seed for unoise                      */
   si32 iseed2;               /* Seed for anoise                      */
   si32 lxs16;                /* lx (S16)                             */
   si32 lys16;                /* ly (S16)                             */
   si32 lxs16m1;              /* lx (S16) - 1                         */
   si32 lys16m1;              /* ly (S16) - 1                         */
   si32 unoise;               /* Frac of pixels to get noise (S16)    */
   short lx;                  /* X-size of input array                */
   short ly;                  /* Y-size of input array                */
   short lxsub1;              /* lx-1                                 */
   short lysub1;              /* ly-1                                 */
   short ekx;                 /* Log(2) input array x dim = kx        */
   short eky;                 /* Log(2) input array y dim = ky        */
   short eux;                 /* Used x-size of stimulus objects      */
   short euy;                 /* Used y-size of stimulus objects      */
   short egrid;               /* Plot grid intervals; =0 no grids     */
   short mxobj;               /* Max number of simult objects         */
   short mxref;               /* Size of pobjrf table                 */
   char ecolgrd[ECOLMAXCH];   /* Grid color                           */
   char ecolstm[ECOLMAXCH];   /* Stimulus color                       */
   stim_type eilo;            /* Value for background pixels          */
   stim_type eihi;            /* Value for stimulus pixels            */
   char kcolor;               /* Color mode: see env.h                */
   char kesave;               /* IA save mode: see env.h              */
   char kfill;                /* TRUE to fill pixels with solid color */
   char kobjlist;             /* TRUE to print object list each cycle */
   char komit;                /* TRUE to omit plot labels             */
   char kpixel;               /* Pixel drawing shape: see env.h       */
   char kretrace;             /* TRUE to retrace all drawing          */
   char newser;               /* TRUE if this is a new series         */
   } ;

/* Define routines used only internally within environment package */
int envlim(struct ENVDEF *Eptr, struct OBJDEF *robj);
int envloc(struct ENVDEF *Eptr, char *id, int itype,
   LIBHANDLE *handle);
int envlod(struct ENVDEF *Eptr, struct OBJDEF *Obj,
   LIBHANDLE *handle, short tux, short tuy);

#endif   /* ENVGLOB_HDR_INCLUDED */
