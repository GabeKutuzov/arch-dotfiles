/* (c) Copyright 1988-2016, The Rockefeller University *11116* */
/* $Id: wdwdef.h 59 2017-01-13 20:34:16Z  $ */
/***********************************************************************
*                    WDWDEF.H Header File For CNS                      *
*  Declaration of WDWDEF data block used to define a CNS view window   *
*  (linked list of WDWDEFs is rooted at RP0->pwdw1, linked via pwdw).  *
*                                                                      *
*  N.B.  With the goal of minimizing the incestuous knowledge of CNS   *
*  internals in the env package and vice-versa, NEWARB def was added   *
*  at V8H.  When defined, it eliminates changes.h--now there is only   *
*  an offset (ojnwd) that env can use to find the motor neuron output  *
*  that drives window movements.  (This is an offset rather than a ptr *
*  in case pjnwd becomes movable or we want to work through a socket.) *
*  When NEWARB is undefined, the old mechanism is in place in case an  *
*  old version needs to be compiled.                                   *
*----------------------------------------------------------------------*
*  V1A, 11/28/88, GNR - Initial version                                *
*  Rev, 05/02/91, GNR - Insert CHANGES structure                       *
*  V7D, 04/08/95, GNR - Kinesthesia info to VCELL, merge CHWDWDEF blk  *
*  V8A, 04/17/97, GNR - Include changes.h, define WINDOW "type"        *
*  ==>, 07/01/02, GNR - Last date before committing to svn repository  *
*  V8H, 11/28/10, GNR - Change COLMAXCH to ECOLMAXCH                   *
*  Rev, 01/02/11, GNR - Add WDWDEF_INCLUDED and NEWARB mechanism       *
*  R66, 03/12/16, GNR - Add prototype for wdwdflt                      *
***********************************************************************/

#ifndef WDWDEF_INCLUDED
#define WDWDEF_INCLUDED

#include "env.h"
#ifndef NEWARB
#include "changes.h"
#endif

struct WDWDEF {
   struct WDWDEF *pnxwdw;     /* Ptr to next WDWDEF block */
   struct WDWDEF *pdrive;     /* Ptr to window that drives this one */
#ifdef NEWARB
   long ojnwd;                /* Offset in pjnwd to env user changes */
#else
   struct CHANGES wc;         /* Window changes structure */
#endif
/**** NOTE: D3 broadcast assumes WX,WY,WWD,WHT fields contiguous ****/
   float wx;                  /* Window X coordinate */
   float wy;                  /* Window Y coordinate */
   float wwd;                 /* Window width */
   float wht;                 /* Window height */
/*     ***                    *  (window rotation will go here */
   float wx0;                 /* Reset window X coordinate */
   float wy0;                 /* Reset window Y coordinate */
   float wwd0;                /* Reset window width */
   float wht0;                /* Reset window height */
/*     ***                    *  (reset window rotation will go here) */
   float sclx;                /* Coord change scale in pixels/s unit */
   float sclr;                /* Rotation scale in pixels/s unit */
   short kchng;               /* Kind of window changes */
/* Possible values of kchng (-n links to stimulus n)
*  N.B.  Some code (e.g. KGNPB setting) assumes kchng is fixed
*  throughout a run.  */
#define WDWZIP   0               /* No change */
#define WDWT     1               /* Translate */
#define WDWTR    2               /* Translate and rotate */
#define WDWTRD   3               /* Translate, rotate, and dilate */
#define WDWLCK   4               /* Lock to previous window */
#define WDWPIG   5               /* Piggyback on previous window */
#define WDWPTR   6               /* Piggyback, translate, rotate */
#define WDWPTRD  7               /* Piggy, trans, rotate, dilate */
   byte wdwno;                /* Window number (counting from 0) */
   byte flags;                /* Control flags */
#define  WBWOPBR   0x01          /* Bypass reset          (WBWOP0B) */
#define  WBWOPOR   0x02          /* Override bypass reset (WBWOP0O) */
   char wcolr[ECOLMAXCH];     /* Window color                       */
   };

/* Get number of arbors, kinesthetic rows for a window */
int wdw_narb(struct WDWDEF *pw);
int wdw_nvwr(struct WDWDEF *pw);
#endif  /* WDWDEF_INCLUDED */
