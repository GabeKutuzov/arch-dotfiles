/* (c) Copyright 1988-2016, The Rockefeller University *11115* */
/* $Id: armdef.h 59 2017-01-13 20:34:16Z  $ */
/***********************************************************************
*                    ARMDEF.H Header File For CNS                      *
*                                                                      *
*  Declaration of ARMDEF data block used to define a CNS arm           *
*  (linked list of ARMDEFs is rooted at RP0->parm1, linked via parm).  *
*                                                                      *
*  V1A, 11/28/88, GNR - Initial version                                *
*  V7D, 04/08/95, GNR - Remove joint kinesthesia info to VCELL block   *
*  V8A, 04/17/97, GNR - Insert arm color field, define ARM "type"      *
*  ==>, 07/01/02, GNR - Last date before committing to svn repository  *
*  V8H, 11/28/10, GNR - Change COLMAXCH to ECOLMAXCH, armno from 1     *
*  R66, 03/12/16, GNR - Add nta,ntl,tla,tll (defaults for joints)      *
*  Rev, 03/26/16, GNR - Add ARMTP (plot touch pads)                    *
***********************************************************************/

#ifndef ARM_HDR_INCLUDED
#define ARM_HDR_INCLUDED

#include "env.h"              /* For ECOLMAXCH definition */

struct ARMDEF {
   struct ARMDEF *pnxarm;     /* Ptr to next ARMBLK                   */
   struct JNTDEF *pjnt;       /* Ptr to array of JNTBLK's             */
   float aattx;               /* Arm attachment point x               */
   float aatty;               /* Arm attachment point y               */
/****** NOTE: D3 broadcast assumes ARMX,ARMY fields contiguous  *******/
   float armx;                /* Current arm tip x coordinate         */
   float army;                /* Current arm tip y coordinate         */
   float armx0;               /* Last arm tip x coordinate            */
   float army0;               /* Last arm tip y coordinate            */
   float jmn0,jmx0;           /* Defaults for joint jmn,jmx           */
   float ja00,jl00;           /* Defaults for joint angle, length     */
   float scla0;               /* Default for joint scla               */
   si32  tll,tla;             /* Touch pad lateral,axial sz (Sxy=S15) */
   short ntl,nta;             /* Touch pad lateral,axial divisions    */
   unsigned short jatt;       /* Arm option and attachment codes      */
#define  ARMAW  0x0001        /* 'W' Attach to window                 */
#define  ARMAU  0x0002        /* 'U' Attach to universal joint        */
#define  ARMPT  0x0004        /* 'P' Pressure-sensitive touch         */
#define  ARMUJ  0x0008        /* 'C' Canonical univ. jt. rqst.        */
#define  ARMEC  0x0010        /* 'E' Edge confinement                 */
#define  ARMGE  0x0020        /* 'G' Gription enabled                 */
#define  ARMIT  0x0040        /* 'I' Touch on integral grid pts       */
#define  ARMBY  0x0080        /* 'B' Bypass reset                     */
#define  ARMTP  0x0100        /* 'T' Plot touch pads                  */
/* Following are run state flags, not setup options: */
#define  ARMOR  0x1000        /* Override bypass reset                */
#define  ARMTR  0x2000        /* Trace in progress                    */
#define  ARMGP  0x4000        /* Gription on/off (1/0)                */
   char acolr[ECOLMAXCH];     /* Arm color                            */
   byte armno;                /* Arm number (counting from 1)         */
   char jpos;                 /* Arm position code:
                              *     0-7 ccw from +x axis, 8 center    */
   char njnts;                /* Number of joints in arm              */
   byte dovjtk;               /* TRUE if doing joint kinesthesia      */
   };

#endif /* ARM_HDR_INCLUDED */
