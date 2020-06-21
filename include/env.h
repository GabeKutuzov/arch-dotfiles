/* (c) Copyright 1992-2016, The Rockefeller University *11116* */
/* $Id: env.h 59 2017-01-13 20:34:16Z  $ */
/***********************************************************************
*                         Environment Program                          *
*                           ENV Header File                            *
*  This file contains prototypes for user-visible environment routines *
*                                                                      *
*  V1A, 04/21/89, GNR - Initial version                                *
*  Rev, 03/03/92, GNR - Change filedefs to RFdefs                      *
*  Rev, 08/09/92, GNR - Introduce LIBHANDLE to envloc,envlod           *
*  Rev, 05/27/95, GNR - Make envctl return ptr to objdef, add qobjvis, *
*                       qobjtor                                        *
*  Rev, 04/11/97, GNR - Add defs for return values, new envopt(),      *
*                       envgob1(), envgob(), envser() functions        *
*  Rev, 08/29/01, GNR - Add envgmxr()                                  *
*  Rev, 02/22/03, GNR - Replace s_type with stim_type                  *
*  Rev, 11/01/03, GNR - Add envprt()                                   *
*  Rev, 06/26/08, GNR - Add ENV_PRNOI                                  *
*  ==>, 06/26/08, GNR - Last date before committing to svn repository  *
*  Rev, 11/28/10, GNR - Add ECOLMAXCH to eliminate COLMAXCH dependence *
*  Rev, 01/06/11, GNR - Eliminate #defined names for internal structs  *
*  Rev, 03/09/16, GNR - Add envvers(), envlim()                        *
***********************************************************************/

#ifndef ENV_HDR_INCLUDED
#define ENV_HDR_INCLUDED

#include "rfdef.h"

#define DFLT_ENVVAL  0x8000   /* ENVVAL code to use default (0.5) */
#define SHP_NAME_SIZE     4   /* Length of a SHAPE name */
#define ECOLMAXCH        12   /* Max chars allowed in color name */
#define qECMC           "12"  /* Quoted ECOLMAXCH */

/* Return codes used by env routines: */
#define ENV_NORMAL      0     /* Successful completion */
#define ENV_RESET       1     /* Caller should reset networks */
#define ENV_FATAL      -1     /* Fatal error */
#define ENV_ERR         2     /* Environment error for RK.iexit */

/* Incomplete structure types defined elsewhere where needed */
struct ENVDEF;
typedef struct ENVDEF ENVIRONMENT;
struct OBJDEF;
typedef struct OBJDEF OBJECT;
struct ARMDEF;
typedef struct ARMDEF ARM;
struct JNTDEF;
typedef struct JNTDEF JOINT;
struct WDWDEF;
typedef struct WDWDEF WINDOW;
struct DSLBDEF;
typedef struct DSLBDEF SHAPE;

/* Type definition for stimulus data (input array) */
typedef unsigned char stim_type;

/* Prototypes of env routines */
int envarm(ENVIRONMENT *Eptr);
OBJECT *envctl(ENVIRONMENT *Eptr, char *card);
int envcyc(ENVIRONMENT *Eptr, stim_type *ia, float *change);
void envdel(ENVIRONMENT *Eptr, OBJECT *obj, OBJECT *prev);
int envget(ENVIRONMENT *Eptr, stim_type *ia);
int envgmxr(ENVIRONMENT *Eptr);
OBJECT *envgob(ENVIRONMENT *Eptr, int index);
OBJECT *envgob1(ENVIRONMENT *Eptr);
int envlim(ENVIRONMENT *Eptr, OBJECT *robj);
int envopt(ENVIRONMENT *Env, double epr, double eslht, int ept,
   int eilo, int eihi, int kux, int kuy, int egrid, char *ecolgrd,
   char *ecolstm, int kpixel, int kfill, int kretrace, int komit,
   int kobjlist);
/* Values kpixel argument can take on: */
#define ENV_CIRCLE 0    /* Draw pixels as circles */
#define ENV_SQUARE 1    /* Draw pixels as squares */
int envpia(ENVIRONMENT *Eptr, stim_type *ia, OBJECT *robj);
int envplot(ENVIRONMENT *Eptr, stim_type *ia, float xstart,
   float ystart, float xlong, float ylong, char *stitle);
void envprt(ENVIRONMENT *Eptr, stim_type *ia, int ksprt);
/* Values ksprt argument can take on: */
#define ENV_PRSPT  4    /* SPOUT the environment print */
#define ENV_PRNOI  8    /* Print noise-contaminated IA */
int envsav(ENVIRONMENT *Eptr, stim_type *ia);
void envser(ENVIRONMENT *Eptr);
ENVIRONMENT *envset(struct RFdef *libfile, struct RFdef *repfile,
   struct RFdef *pixfile, ARM *parm1, WINDOW *pwdw1,
   int kx, int ky, int mxobj, int mxref, int kcolor, int kesave);
/* Values kcolor argument can take on:
*  N.B. Some CNS code assumes (kcolor!=0) ==> colored env.  */
#define ENV_MONO   0    /* Environment is monochrome */
#define ENV_COLOR  1    /* Environment is colored */
#define ENV_ORCOL  2    /* Environment objects OR colors together */
/* Values kesave argument can take on: */
#define ENV_SAVOBJ 0    /* Save only objects in replay file */
#define ENV_SAVALL 1    /* Save full input array in replay file */
int envshp(SHAPE ***ppshp, short tux, short tuy,
   long alloc, stim_type hipix, stim_type lopix);
char *envvers(void);
int qobjtor(OBJECT *Obj);
int qobjvis(OBJECT *Obj);

#endif  /* ENV_HDR_INCLUDED */
