/* (c) Copyright 1988-2016, The Rockefeller University *11115* */
/* $Id: jntdef.h 59 2017-01-13 20:34:16Z  $ */
/***********************************************************************
*                    JNTDEF.H Header File For CNS                      *
*                             11/28/88                                 *
*                                                                      *
*  Declaration of JNTDEF joint data block                              *
*     (JNTDEF blocks are allocated in an array of pa->njnts structures *
*     (+1 if univ joint, +1 if grip) rooted at pa->pjnt).              *
*                                                                      *
*  N.B.  With the goal of minimizing the incestuous knowledge of CNS   *
*  internals in the env package and vice-versa, NEWARB def was added   *
*  at V8H.  When defined, it eliminates changes.h--now there is only   *
*  an offset (ojnwd) that env can use to find the motor neuron output  *
*  that drives joint movements.  (This is an offset rather than a ptr  *
*  in case pjnwd becomes movable or we want to work through a socket.  *
*  It is set to LONG_MAX to indicate no changes in this joint.)        *
*  When NEWARB is undefined, the old mechanism is in place in case an  *
*  old version needs to be compiled.                                   *
*                                                                      *
*  Rev, 05/02/91, GNR - Insert CHANGES structure                       *
*  ==>, 07/01/02, GNR - Last date before committing to svn repository  *
*  V8H, 01/02/11, GNR - Add JNTDEF_INCLUDED and NEWARB mechanism       *
*  R66, 03/12/16, GNR - Add nta,ntl, make tla,tll fixpt                *                                   *
***********************************************************************/

#ifndef JNTDEF_INCLUDED
#define JNTDEF_INCLUDED

#include "env.h"
#ifndef NEWARB
#include "changes.h"
#endif

   struct JNTDEF {
#ifdef NEWARB
      long ojnwd;             /* Offset in pjnwd to env user changes */
#else
      struct CHANGES jc;      /* Joint changes structure */
#endif
/* The following union allows for alternative definitions of certain
*  variables for universal joints and gripper joints.
*  N.B.  If any of tla,tll,nta,ntl is zero, tla is set to zero by
*  g1jnt() as a quick-test flag to indicate touch pad not in use.  */
      union {
         struct {
            float ja;         /* Joint angle */
            float jx;         /* X coord of distal end of segment */
            float jy;         /* Y coord of distal end of segment */
            float jmn;        /* Minimum joint angle */
            float jmx;        /* Maximum joint angle */
            float ja0;        /* Reset value for joint angle */
            ui32 tnvc;        /* VT_SRC: Sum nvcells for lower jnts */
            si32 tla;         /* Touch length axially (Sxy = S15) */
            si32 tll;         /* Touch length laterally (Sxy) */
            si32 tzi;         /* Z coordinate of joint */
            short ntl,nta;    /* Touch pad lateral,axial divisions */
            short olvty;      /* VT_SRC: Sum of nta for lower joints */
            } jnt;
         struct {
            float ujsclx;     /* Universal joint distance scale */
            float deltax;     /* Universal joint delta X */
            float deltay;     /* Universal joint delta Y */
            float ujdxn;      /* Delta X of last joint */
            float ujdyn;      /* Delta Y of last joint */
            float ujtau;      /* Tau angle of current uj motion */
            float ujdtau;     /* Difference tau angle */
            long ctime;       /* Num of trials to stay in canon mode */
            long ujct;        /* Trials remaining in curr canon mode */
            si32 ujvt;        /* Value threshold (S8) */
            si32 ujvi;        /* Value index */
            } ujnt;
         struct {
            float gwidth;     /* Gripper width */
            float gspeed;     /* Gripper speed */
            long gtime;       /* Time in grip mode */
            long grct;        /* Trials remaining in curr grip mode */
            si32 grvt;        /* Grip value threshold (S8) */
            si32 grvi;        /* Grip value index  */
            } grip;
         } u;
      float jl;               /* Joint length (bigdif for univ. jt.) */
      float scla;             /* Scale for angles in degrees/s unit */
      };

enum kjnt { Normal, Universal, Gripper };

#endif /* JNTDEF_INCLUDED */
