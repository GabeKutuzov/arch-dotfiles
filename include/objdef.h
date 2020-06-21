/* (c) Copyright 1988-2016, The Rockefeller University *11115* */
/* $Id: objdef.h 59 2017-01-13 20:34:16Z  $ */
/***********************************************************************
*                        OBJECT Header File                            *
*                  Declaration for OBJECT data block                   *
*                                                                      *
*  V1A, 11/28/88, GNR - Initial version                                *
*  Rev, 10/20/92, GNR - Add provision for PROB and CORR cards          *
*  Rev, 01/19/93, GNR - Add isn,ign fields, jdelay,jecvt to shorts     *
*  Rev, 07/22/95, GNR - Add libbase, reformulate BC flags              *
*  Rev, 04/10/97, GNR - Add provision for bursting and changing ISI    *
*  ==>, 07/01/02, GNR - Last date before committing to svn repository  *
*  Rev, 02/23/16, GNR - Seeds to si32 for 64-bit compilation           *
***********************************************************************/

#ifndef OBJECT_HDR_INCLUDED
#define OBJECT_HDR_INCLUDED

#include "env.h"
#include "dslb.h"

/* Following structure contains info needed for SELECT objects only   */
   struct SELOBJDEF {
      long *psprob;           /* Ptr to selection probability         */
      long librec;            /* Current library item number          */
      long libstart;          /* Starting library item number         */
      long libend;            /* Ending library item number           */
      long libbase;           /* Base library item number             */
      long selend;            /* Cycle when select object is deleted  */
      long selint0;           /* Initial selection interval           */
      long selint;            /* Working selection trial interval     */
      long stay;              /* Stay time for select object          */
      long staycnt;           /* Number of cycles since object was
                                 placed on IA, negative during gdelay */
      si32 orgseed;           /* Original select seed                 */
      si32 curseed;           /* Current select seed                  */
      short burst;            /* Number of presentations in burst     */
      short burstcnt;         /* Counts up until burst completed      */
      short dselint;          /* Increment in selint after burst      */
      short gdelay;           /* Gription delay until new selection   */
      short *pscorr;          /* Reference numbers of correlated stim */
      short *psperm;          /* Ptr to selection permutation         */
      short ux;               /* Used x from select card              */
      short uy;               /* Used y from select card              */
      } ;

/* Following structure contains info needed for all kinds of objects  */
   struct OBJDEF {
      struct OBJDEF *pnxobj;  /* Pointer to next object               */
      struct DSLBDEF *pshape; /* Pointer to shape (dslb) array        */
      float ejdst;            /* Event jump distance squared          */
      float angle;            /* Rate of change in dir (radians)      */
      float angvel;           /* Rate of change in orient (radians)   */
      float orient;           /* Current orientation angle (radians)  */
      float dir;              /* Current direc. of linear mvt (rads)  */
      float vel;              /* Velocity (pix/cycle) of linear mvt   */
      float esignx;           /* Sign of current x motion; hit dir    */
      float esigny;           /* Sign of current y motion; hit vel    */
      float amp;              /* Amplitude of osc (pixels)            */
      float freq;             /* Frequency of osc (trials/osc)        */
      long irf;               /* Reference number                     */
      long movsta;            /* Cycle at which movement starts       */
      long movend;            /* Cycle at which movement ends         */
      long shpsiz;            /* Words allocated to shape array       */
      long jump;              /* Jump cycle interval                  */
      long jmpcnt;            /* Counts up until next jump            */
      long userdata;          /* Caller can store stuff here          */
      si32 elox;              /* Relative x of leftmost pixel  (S16)  */
      si32 ehix;              /* Relative x of rightmost pixel (S16)  */
      si32 eloy;              /* Relative y of highest pixel   (S16)  */
      si32 ehiy;              /* Relative y of lowest pixel    (S16)  */
      si32 iorgx;             /* Starting X coord (S16)               */
      si32 iorgy;             /* Starting Y coord (S16)               */
      si32 icurx;             /* Current X coord (S16)                */
      si32 icury;             /* Current Y coord (S16)                */
      si32 jxmin;             /* Lower X bound of motion window (S16) */
      si32 jxmax;             /* Upper X bound of motion window (S16) */
      si32 jymin;             /* Lower Y bound of motion window (S16) */
      si32 jymax;             /* Upper Y bound of motion window (S16) */
      si32 frac;              /* Freq(sign chng) in random walk (S16) */
      si32 jsizx;             /* = jxmax - jxmin (S16)                */
      si32 jsizy;             /* = jymax - jymin (S16)                */
      si32 jseed;             /* Seed for generating jump coords      */
      si32 mseed;             /* Seed for random walk coords          */
      short iz;               /* Permanent z coord                    */
      ui16  isn;              /* Stimulus number                      */
      ui16  ign;              /* Group number                         */
      short iadapt;           /* Adaptiveness value (S8)              */
      short jdelay;           /* Event jump delay parameter           */
      short jevct;            /* Counts consecutive events to jump    */
      short pxmin;            /* Lower X boundary of placement window */
      short pxmax;            /* Upper X boundary of placement window */
      short pymin;            /* Lower y boundary of placement window */
      short pymax;            /* Upper y boundary of placement window */
      short objflag;          /* Other object flags                   */
/* Values objflag takes on: */
#define  OBJBR    0x01        /* Bounds reset--call envlim    (OBF3B) */
#define  OBJRE    0x02        /* Select 'reuse' option flag   (OBF3R) */
#define  OBJUO    0x04        /* User object, not library     (OBF3U) */
#define  OBJNI    0x08        /* Nonintegral coords allowed   (OBF3N) */
#define  OBJRW    0x10        /* Random walk                  (OBF3W) */
#define  OBJHT    0x20        /* Object has been hit          (OBF3H) */
#define  OBJSQ    0x40        /* Square wave oscillation      (OBF3Q) */
#define  OBJSW    0x80        /* Sawtooth oscillation         (OBF3S) */
#define  OBJBO   0x100        /* Boustrophedon                (OBF2B) */
#define  OBJLT   0x200        /* Load trigger for LIB_REPT    ( NEW ) */
#define  OBJRB   0x400        /* Reset model after burst      ( NEW ) */

      char placebounds;       /* Placement boundary conditions        */
/* Values placebounds takes on: */
#define  OPLZB    0x01        /* Zero boundary conditions     (OBP2Z) */
#define  OPLTB    0x02        /* Toroidal boundary conditions (OBP2T) */

      char movebounds;        /* Movement boundary conditions         */
/* Values movebounds takes on: */
#define  BNDMR    0x01        /* Mirror boundary conditions   (OBM2M) */
#define  BNDNO    0x02        /* No boundary conditions       (OBM2N) */
#define  BNDTB    0x03        /* Toroidal boundary conditions (OBM2T) */
#define  BNDEG    0x04        /* Edge boundary conditions     (OBM2E) */

      char bouster;           /* Boustrophedonal movement regulator   */
/* Values bouster takes on (envcyc() makes use of arithmetic values): */
#define BOUSTEP      0        /* Just ended cycle, step next time  */
#define BOUOSC       1        /* Normal oscillation in progress    */
#define BOUINIT      2        /* First time thru (omit skip)       */

      char seltyp;            /* Select type code                     */
/* Types of objects.  N.B.  If you plan on changing these defs, note
*  that values are sometimes results of computations in envctl().     */
#define LIB_CREATE   0        /* Created object                       */
#define SEQUENT      2        /* Select obj from seq stim file        */
#define LIB_RAND     3        /* Select obj from lib randomly         */
#define LIB_CYCL     4        /* Select obj from lib cyclicly         */
#define LIB_REPT     5        /* Select obj from lib repeatedly       */
#define LIB_PERM     6        /* Select obj from lib random permut'n  */
#define PIX_TYPE     8        /* Amount added to LIB code
                                 to get PIX code                      */
#define PIX_RAND    11        /* Select pixel file obj randomly       */
#define PIX_CYCL    12        /* Select pixel file obj cyclicly       */
#define PIX_REPT    13        /* Select pixel file obj repeatedly     */
#define PIX_PERM    14        /* Select pixel file obj random permutn */

      struct SELOBJDEF sel;   /* Select object info (if needed)       */
      } ;

/* Following define facilitates pseudo-separate SEL/OBJECT defs */
#define OBJECT_SIZE   (sizeof(struct OBJDEF) - sizeof(struct SELOBJDEF))

#endif   /* OBJECT_HDR_INCLUDED */
