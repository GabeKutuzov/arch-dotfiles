/* (c) Copyright 1991-2010, The Rockefeller University *21116* */
/* $Id: changes.h 59 2017-01-13 20:34:16Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                         CHANGES Header File                          *
*    Define structure for general effector motor arbor information     *
*                                                                      *
*  Information defined here is used to fill in the changes array.      *
*----------------------------------------------------------------------*
*  Written (undated), JMS                                              *
*  Rev, 04/20/91, GNR - Add offset and kefch                           *
*  Rev, 08/29/91, GNR - Add mode                                       *
*  Rev, 04/13/97, GNR - Allow this file to be included more than once  *
*  ==>, 02/16/07, GNR - Last date before committing to svn repository  *
*  Rev, 02/21/10, GNR - Move KEF_ defs here from pfglobal.h            *
*  V8H, 11/28/10, GNR - Elimate COLMAXCH conflict with CNS             *
***********************************************************************/

/* N.B.  At present, each CHANGES structure results in nchgs entries
   being maintainted in the changes (pjnwd) array.  Once all effectors
   have been recoded to use CHANGES structures, it is planned to store
   the excitatory and inhibitory motor nerve activities separately so
   that motor force or velocity can be controlled accordingly. -GNR */
/* N.B.  nchgs is set in d3grp1 according to type of effector, but may
   be reset to zero in d3tchk if arbw is zero.  Do not do this too
   soon, as may be used in d3tchk to count kinesthetic cells. */

#ifndef CHANGES_INCLUDED
#define CHANGES_INCLUDED

/* The following conditionals provide a mechanism whereby library
*  functions (specifically, env routines) can access a few key CNS
*  configuration parameters without accessing d3global.h in all its
*  glory.  These are then picked up in jntdef.h and wdwdef.h, which
*  #include this file.  A cleaner way to do this would be nice.  */
#ifndef LSNAME
#define LSNAME        4       /* Max chars allowed in layer name */
struct  rlnm_str { ui16 hbcnm; char rnm[LSNAME]; char lnm[LSNAME]; };
typedef struct rlnm_str rlnm; /* Repertoire-layer name */
#endif
#ifndef XCELLBLOCK            /* Avoid undefined type warnings */
#define XCELLBLOCK void
#endif

struct CHANGES {
   struct CHANGES *pnch;      /* Pointer to next changes structure */
   rlnm exnm;                 /* Excitatory rep-layer name */
   rlnm innm;                 /* Inhibitory rep-layer name */
   XCELLBLOCK *ec;            /* Pointer to excitatory cell layer */
   XCELLBLOCK *ic;            /* Pointer to inhibitory cell layer */
   long arbw;                 /* Arbor width (number of cells) */
   long arb1;                 /* First cell in arbor */
   long arbs;                 /* Arbor stride */
   ui16 nchgs;                /* Number of entries in pjnwd array */
   short jjnwd;               /* Offset of changes array entry */
   short kefch;               /* Kind of effector, see defs below */
#define KEF_ARM         0     /* Arm */
#define KEF_UJ          1     /* Universal joint */
#define KEF_GRP         2     /* Gripping joint */
#define KEF_WDW         3     /* Window */
#define KEFNUM          4     /* Number of codes now defined */
   short mode;                /* Kind-dependent mode switches */
   };

#endif
