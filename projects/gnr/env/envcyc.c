/* (c) Copyright 1988-2016, The Rockefeller University *21116* */
/* $Id: envcyc.c 25 2018-05-07 22:13:48Z  $ */
/***********************************************************************
*              Darwin III Environment Simulation Package               *
*                               ENVCYC                                 *
*                       Update the environment                         *
*                                                                      *
*  Synopsis:  int envcyc(struct ENVDEF *Eptr, stim_type *inpary,       *
*        float *change)                                                *
*  Arguments:                                                          *
*     Eptr:   Ptr to current environment                               *
*     inpary: The object array to be updated                           *
*     change: The change array for network output                      *
*  Value returned:                                                     *
*     ENV_NORMAL  (0)      Successful completion                       *
*     ENV_RESET   (1)      Caller should reset networks                *
*     ENV_FATAL  (-1)      Fatal error                                 *
************************************************************************
*  V4A, 12/18/88, JWT - Translated into C                              *
*  Rev, 11/21/89, JMS - Changed loop counters to int to handle         *
*                       boundary conditions                            *
*  Rev, 03/28/90, JWT - Object deletion through routine (envdel)       *
*  Rev, 03/29/90, JWT - Object selection interval (EVERY),             *
*                       removed hex stimulus file pointer,             *
*  Rev, 03/04/92, GNR - Use rf io routines                             *
*  Rev, 08/09/92, GNR - Introduce LIBHANDLE for port                   *
*  Rev, 09/01/92, GNR - Add arm edge confinement                       *
*  Rev, 01/20/93, GNR - Add isn,ign to OBJDEF structure                *
*  Rev, 04/05/93, GNR - Correct permutation bug                        *
*  V7D, 04/15/95, GNR - Merge kchng,flags into WDWDEF, add qobjvis(),  *
*                       qobjtor(), libbase                             *
*  V8A, 04/11/97, GNR - Add select object bursting and dselint         *
*  ==>, 12/16/07, GNR - Last mod before committing to svn repository   *
*  V8H, 12/01/10, GNR - Separate env.h,envglob.h w/ECOLMAXCH change    *
*  Rev, 01/02/11, GNR - Add temp test on change == NULL                *
*  Rev, 01/30/11, GNR - Use ojnwd to find changes                      *
*  Rev, 02/23/16, GNR - Seeds to si32 for 64-bit compilation           *
***********************************************************************/

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "envglob.h"
#include "jntdef.h"
#include "armdef.h"
#include "wdwdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "bapkg.h"
#include "rkarith.h"

#define changed_bounds(O)     ((O)->objflag & OBJBR)
#define is_hit(O)             ((O)->objflag & OBJHT)
#define random_walk(O)        ((O)->objflag & OBJRW)
#define boustrophedon(O)      ((O)->objflag & OBJBO)
#define square(O)             ((O)->objflag & OBJSQ)
#define sawtooth(O)           ((O)->objflag & OBJSW)
#define is_select(O)          ((O)->seltyp > LIB_CREATE)
#define is_pixel_type(O)      ((O)->seltyp >= PIX_TYPE)
#define is_move_time(O)       ((O)->movsta<=cycnum1 && \
                                 (O)->movend>=cycnum1)
#define is_rotating(O)        ((O)->angvel)
#define linear_movement(O)    ((O)->vel)
#define oscillar_movement(O)  ((O)->amp)
#define gription_check(A)     (cycnum1!=1 && (A)->jatt&ARMGP)
#define is_universal(A)       ((A)->jatt&ARMAU)
#ifdef NEWARB
#define jnt_is_moving(J)      ((J)->ojnwd != LONG_MAX)
#define wdw_is_moving(W)      ((W)->ojnwd != LONG_MAX)
#define get_jptr1(JW)         (jptr = (JW)->ojnwd)
#else
#define jnt_is_moving(J)      ((J)->jc.arbw > 0)
#define wdw_is_moving(W)      ((W)->wc.arbw > 0)
#define get_jptr1(JW)         (++jptr)
#endif

static float refang[NARMPOS] = { -90.0,   0.0,   0.0,  90.0,   90.0,
   -180.0, -180.0, -90.0, -180.0,  90.0, -90.0,   0.0, -180.0 };

/*=====================================================================*
*                               qobjtor                                *
*                                                                      *
*  This little function may be called to determine whether an object   *
*  has toroidal placement boundary conditions.  This avoids the need   *
*  for the calling application to look into the contents of an OBJDEF. *
*                                                                      *
*  The argument is a pointer to the OBJDEF to be tested.               *
*  The value returned is TRUE if the object is toroidal, else FALSE.   *
*=====================================================================*/

int qobjtor(OBJECT *pobj) {

   return ((pobj->placebounds & OPLTB) ? TRUE : FALSE);
   } /* End qobjtor */


/*=====================================================================*
*                               qobjvis                                *
*                                                                      *
*  This little function may be called to determine whether an object   *
*  is currently visible on the input array.  This avoids the need for  *
*  the calling application to look into the contents of an OBJDEF.     *
*  Currently, an object is invisible only if it is of SELECT type,     *
*  its 'stay' time has been exceeded or it has been hit off the IA,    *
*  and it has not yet been replaced with a new object.                 *
*                                                                      *
*  The argument is a pointer to the OBJDEF to be tested.               *
*  The value returned is TRUE if the object is visible, else FALSE.    *
*=====================================================================*/

int qobjvis(OBJECT *pobj) {

   return (pobj->seltyp == LIB_CREATE) || ((pobj->objflag & OBJHT) ?
      (pobj->sel.staycnt >= 0) : (pobj->sel.staycnt < pobj->sel.stay)
      );
   } /* End envvis */


/*=====================================================================*
*                               envcyc                                 *
*=====================================================================*/

int envcyc(struct ENVDEF *Eptr, stim_type *inpary, float *change) {

   LIBHANDLE handle;    /* Library object handle        */
   struct OBJDEF *prev; /* Ptr to last inspected object */
   struct OBJDEF *Obj;  /* Ptr to current object        */
   struct ARMDEF *pa;   /* Ptr to current arm           */
   struct JNTDEF *pj;   /* Ptr to current arm joint     */
   struct WDWDEF *pw;   /* Ptr to current window        */
   struct WDWDEF *pwo;  /* Ptr to previous window       */
   char outbuff[16];    /* Output buffer                */
   float adir;          /* Direction of linear movement (radians) */
   float ampl;          /* Movement amplification           */
   float ampx,ampy;     /* Oscillatory movement along X,Y   */
   float aprv[4];       /* Copy of window dimensions        */
   float angl;          /* Movement angle                   */
   float xb[5];         /* X values for arm gription check  */
   float yb[5];         /* Y values for arm gription check  */
   float radjt;         /* Arm movement (radians)           */
   float cosax,sinax;   /* Trig values for calculating joint angle  */
   float slope[4];      /* Slope of joint angle   */
   float darm;          /* Distance arm tip moved in current cycle  */
   float xd,yd;         /* X,Y components of darm */
   float asign;         /* Joint angle multiplier (toggles +/-1.0)  */
   float gtll;          /* Gription touch length laterally */
   long itmpx;          /* X coord relative to left window edge */
   long itmpy;          /* Y coord relative to top window edge */
   long xlit,xbig;      /* Smallest,largest X coordinate for joint  */
   long ylit,ybig;      /* Smallest,largest Y coordinate for joint  */
   long lowx,topx;      /* Lower,upper bound of X range for joint   */
   long lowy,topy;      /* Lower,upper bound of Y range for joint   */
   long yint;           /* Y coord for joint (temp) */
   long jptr = -1;      /* Index into change array */
   si32 ii;             /* Random number used to select an object   */
   si32 jxy[2];         /* Random numbers used for jump coords      */
   si32 irdlxy[4];      /* Random numbers for random walk coords    */
   int i,xe,ixe,        /* For loop counters */
      ixp,ipx,iyp,ipy;
   int irf1;            /* Reference number of object to search for */
   int newser;          /* Flag for new series */
   int rc = ENV_NORMAL; /* Return code */
   char njnts1;         /* Number of joints in current arm */
   char reuse;          /* Flag for re-use of current object */
   char first;          /* Flag first lib record (select) */
   char move_type = 0;  /* Flag for type of move (normal or jump) */

/* Following are local copies of heavily referenced environment values */
   float rrlx =  Eptr->rlx;
   float rrly =  Eptr->rly;
   float rrlxm1 = rrlx - 1.0;
   float rrlym1 = rrly - 1.0;
   long llymul2 = Eptr->lymul2;
   long lltotal = Eptr->ltotal;
   si32 llxs16 =  Eptr->lxs16;
   si32 llys16 =  Eptr->lys16;
   si32 llxs16m1 = Eptr->lxs16m1;
   si32 llys16m1 = Eptr->lys16m1;
   short llx = Eptr->lx;
   short lly = Eptr->ly;

/* Increment cumulative cycle number, check for new series */
   long cycnum1 = ++Eptr->cycnum;
   newser = Eptr->newser;
   Eptr->newser = FALSE;

/* Set input array to background density */
   memset((char *)inpary, Eptr->eilo, lltotal);
/* Zero auxiliary columns */
   for (i=lly; i<lltotal; i+=llymul2)
      memset((char *)inpary+i, 0, lly);

/*---------------------------------------------------------------------*
*                    PROCESS ALL OBJECTS IN MODEL                      *
*---------------------------------------------------------------------*/

   for (prev = NULL, Obj = Eptr->pobj1;
        Obj; Obj = (prev = Obj)->pnxobj) {

      /* Determine whether object is SELECT */
      if (is_select(Obj)) {

/*---------------------------------------------------------------------*
*                        PROCESS SELECT OBJECT                         *
*---------------------------------------------------------------------*/

/* If object has been hit, count down to replacement time */

         if (is_hit(Obj)) {
            /* Is object still visible (tumbling away from hit)?
            *  (Omit normal checks of selend, selint, & stay times) */
            if (Obj->sel.staycnt >= 0) goto EXECUTE_MOVE;
            /* Count to GDELAY and let obj reappear after delay */
            if (++Obj->sel.staycnt == 0) {
               Obj->objflag &= ~OBJHT;
               Obj->sel.staycnt = Obj->sel.selint;
               Obj->sel.burstcnt = Obj->sel.burst;
               Obj->esignx = Obj->esigny = 1.0;
               }
            continue;
            } /* End hit */

/* Delete object if its time is up (default time is LONG_MAX) */

         if (Obj->sel.selend < cycnum1) {
            envdel(Eptr, Obj, prev);
            Obj = prev; /* Back up Obj for next iteration */
            continue;
            }

/* Object is still alive... check if active this cycle */

         if (++Obj->sel.staycnt < Obj->sel.selint) {
            if (Obj->sel.staycnt >= Obj->sel.stay)
               continue;      /* Not active */
            }

/* If a burst is in progress, select the same object again */

         else if (++Obj->sel.burstcnt < Obj->sel.burst) {
            Obj->sel.staycnt = 0;
            Obj->jmpcnt = Obj->jump;
            }

/* Otherwise, it's time to pick a new object.  Increment selection
*  interval and transmit reset signal to caller if requested.  */

         else {
            Obj->sel.burstcnt = Obj->sel.staycnt = 0;
            Obj->jmpcnt = Obj->jump;
            reuse = first = Obj->sel.librec < 0;
            if (newser) {
               Obj->sel.selint = Obj->sel.selint0;
               if (Obj->objflag & OBJRE) reuse = TRUE; }
            else {
               Obj->sel.selint += (long)Obj->sel.dselint;
               if (Obj->objflag & OBJRB) rc = ENV_RESET; }

            /* Select object according to requested method */

            switch (Obj->seltyp) {  /* Cases defined in envglob.h */

            case PIX_RAND:
            case LIB_RAND:
               /* Select from library or pixel file randomly */
               if (reuse) Obj->sel.curseed = Obj->sel.orgseed;
               rannum(&ii, 1, &Obj->sel.curseed, 0);
               Obj->sel.librec = Obj->sel.libstart + ii %
                     (Obj->sel.libend - Obj->sel.libstart + 1);
               break;

            case PIX_CYCL:
            case LIB_CYCL:
               /* Select from library or pixel file cyclicly */
               if ((Obj->sel.librec >= Obj->sel.libend) || reuse)
                  Obj->sel.librec = Obj->sel.libstart;
               else Obj->sel.librec++;
               break;

            case PIX_PERM:
            case LIB_PERM: {
               /* Select from lib or pixel file w/ random permutation.
               * On first use or non-reuse new series, generate new
               * permutation.  Note that permutation index in CURSEED
               * goes from 0 to NPERM-1, but library item number in
               * PSPERM[index] counts from 1.  */
               si32 nperm = Obj->sel.libend - Obj->sel.libstart + 1;
               if (first || (newser && !reuse)) {
                  /* Init permutation with seqential integers */
                  short *ps, *psi, tmp;
                  long il = Obj->sel.libstart, endl = Obj->sel.libend;
                  ps = psi = Obj->sel.psperm;
                  while (il <= endl) *psi++ = il++;

                  /* Generate the random permutation */
                  while (--nperm) /* Knuth shuffle */ {
                     si32 tos = udev(&Obj->sel.orgseed);
                     psi = ps + mssle(tos, nperm, -31, 0);
                     tmp = *psi;
                     *psi = ps[nperm];
                     ps[nperm] = tmp;
                     } /* End nperm while */

                  /* Init CURSEED to serve as permutation list index */
                  Obj->sel.curseed = 0;
                  } /* End new permutation generation */

               else if ((Obj->sel.curseed >= nperm-1) || reuse)
                  /* re-use existing permutation */
                  Obj->sel.curseed = 0;

               else /* Normal case: advance to next permutation */
                  Obj->sel.curseed++;

               /* Pick up permutation from stored table */
               Obj->sel.librec = Obj->sel.psperm[Obj->sel.curseed];
               }
               break;

            case PIX_REPT:
            case LIB_REPT:
               /* Clear load trigger first time, otherwise set it */
               Obj->objflag ^= OBJLT;
               break;

            default:      /* Unknown select source */
               goto SELECT_ERR;
               } /* End switch */

            /* Set bounds change flag (objects newly selected) */
            Obj->objflag |= OBJBR;
            } /* End pick... */

         if (is_pixel_type(Obj)) {  /* Read a pixel object */
            struct RFdef *infd = Eptr->pixfile;
            byte ref = (byte)Obj->irf;
            size_t offset;
            if (!(infd->iamopen & IAM_ANYOPEN)) rfopen(infd, NULL,
               READ, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE,
               IGNORE, IGNORE, IGNORE, IGNORE, ABORT);

            /* Fill auxiliary columns with reference number
            * (used by D3VTCH to determine z coords) */
            memset((char *)inpary, ref, lltotal);

            /* Position to desired record */
            offset = lly*(llx*Obj->sel.librec + 1);  /* 1 for header */
            rfseek(infd, offset, SEEKABS, ABORT);

            /* Read pixel data */
            for (i=0; i<lltotal; i+=llymul2)
               rfread(infd, inpary+i, lly, ABORT);
            /* Set stimulus and group number from file sequence */
            Obj->isn = Obj->ign =
               Obj->sel.librec - Obj->sel.libbase + 1;
            /* Omit motion processing */
            continue;
            } /* End loading pixel object */

         else if (!(Obj->objflag & OBJLT)) {
            /* Load library object */
            envloc(Eptr, NULL, Obj->sel.librec, &handle);
            envlod(Eptr, Obj, &handle, Obj->sel.ux, Obj->sel.uy);
            /* Set stimulus number from library sequence, group
            *  number from shape record */
            Obj->isn = Obj->sel.librec - Obj->sel.libbase + 1;
            Obj->ign = Obj->pshape->tgp;
            } /* End loading library object */

         /* Clear trigger ==> load again next time (unless REPT) */
         Obj->objflag &= ~OBJLT;
         } /* End SELECT processing */

      if (is_move_time(Obj)) {

/*---------------------------------------------------------------------*
*                        PROCESS OBJECT MOTION                         *
*---------------------------------------------------------------------*/

EXECUTE_MOVE:
         /* If object is rotating, update rotation and
         *  set bounds change flag */
         if (is_rotating(Obj)) {
            Obj->orient = fmod((double)(Obj->orient+Obj->angvel),PI2);
            Obj->objflag |= OBJBR;
            }

         /* If object has been hit, have it tumble away.
         *  (esignx is used for direction, esigny for velocity) */
         if (is_hit(Obj)) {
            adir = Obj->esignx;
            ampl = Obj->esigny*FS16;
            Obj->icurx += RTOJRND(ampl*cos((double)adir));
            Obj->icury -= RTOJRND(ampl*sin((double)adir));

            /* Determine when object should disappear */
            if (changed_bounds(Obj)) envlim(Eptr, Obj);
            if ((Obj->icurx+Obj->ehix < 0) ||
                  (Obj->icurx+Obj->elox > llxs16m1) ||
                  (Obj->icury+Obj->ehiy < 0) ||
                  (Obj->icury+Obj->eloy > llys16m1)) {
               /* Time to disappear... */
               if (is_select(Obj)) {
                  /* Begin countdown to new object */
                  Obj->sel.staycnt = -Obj->sel.gdelay;
                  continue;
                  }
               else {
                  /* Remove created object from linked list */
                  envdel(Eptr, Obj, prev);
                  if (Obj=prev) continue;    /* Assignment intended */
                  break;
                  }
               } /* End out of bounds */
            } /* End hit object */

         else {               /* Object not hit */

            /* Check for uniformly timed jump */
            if (Obj->jump>0 && ++Obj->jmpcnt>Obj->jump)
               move_type = JUMP;
            else {
               /* Check for event-driven jump (just arm(s) for now) */
               if (Obj->jdelay >= 0) {
                  float x,y;
                  for (pa=Eptr->parm1; pa && !move_type; pa=pa->pnxarm)
                     if ((x = pa->armx - (float)Obj->icurx*SM16,x*x) +
                        (y = pa->army - (float)Obj->icurx*SM16,y*y) <=
                        Obj->ejdst) move_type = (++Obj->jevct -
                        Obj->jdelay < 0) ? NORM : JUMP;
                  if (!pa) Obj->jevct = 0;
                  } /* End if jdelay */
               }  /* End jump check else */

/* Execute move (either jump or normal) */

            switch (move_type) {

            case JUMP:
               Obj->jmpcnt = 1;
               Obj->jevct = 0;
               rannum(jxy, 2, &Obj->jseed, 0);

               /* Assign original coords to current
               *  (no jump oscillation) */
               Obj->icurx = Obj->iorgx =
                  Obj->jxmin + mssle(jxy[0], Obj->jsizx, -31, 0);
               Obj->icury = Obj->iorgy =
                  Obj->jymin + mssle(jxy[1], Obj->jsizy, -31, 0);
               Obj->movsta = cycnum1;
               break;   /* End case JUMP */

            case NORM:
            case 0:  /* Default to NORMAL move */
               /* Update motion direction
               *  (used for linear and oscillation) */
               Obj->dir = fmod((double)(Obj->dir+Obj->angle),PI2);
               if (linear_movement(Obj)) {
                  /* Note: ESIGNX and ESIGNY are used to reflect motion
                  *  when mirror boundary is encountered (also used in
                  *  random walk to reverse motion from time to time */
                  if (random_walk(Obj)) {
                     /* Note on random walk scaling: RNDLX and Y are in
                     *  range (0...65535) for correct s16 coord scale */
                     rannum(irdlxy, 4, &Obj->mseed, 15);
                     if (irdlxy[2]<=Obj->frac)
                        Obj->esignx = -Obj->esignx;
                     Obj->iorgx +=
                        RTOJRND(Obj->vel*irdlxy[0]*Obj->esignx);
                     if (irdlxy[3]<=Obj->frac)
                        Obj->esigny = -Obj->esigny;
                     Obj->iorgy +=
                        RTOJRND(Obj->vel*irdlxy[1]*Obj->esigny);
                     } /* End random walk */

                  else {         /* Not random walk */
                     if (boustrophedon(Obj)) {
                        /* Check for the 1/2 and 0 cycle points.
                        *  At appropriate times, perform linear motion
                        *  and retard oscillation.  */
                        if (Obj->bouster == BOUSTEP) {   /* Step time */
                           Obj->bouster = BOUOSC;     /* Turn on osc. */
                           Obj->movsta++;    /* Freeze osc next trial */
                           }
                        else {               /* Oscillation/init time */
                           /* Trigger step in next trial if end of
                           *  half-cycle */
                           if (fabs(fmod((double)(cycnum1-Obj->movsta),
                              (double)(0.5*Obj->freq))) <= ZEROISH)
                              Obj->bouster--;
                           goto OSCILLATE;
                           }
                        }  /* End boustrophedon */

                     /* Do systematic linear motion */
                     adir = Obj->dir;
                     ampl = FS16*Obj->vel;
                     Obj->iorgx +=
                        RTOJRND(ampl*cos((double)adir)*Obj->esignx);
                     Obj->iorgy -=
                        RTOJRND(ampl*sin((double)adir)*Obj->esigny);
                     } /* End not random walk */
                  } /* End linear movement */

OSCILLATE:
               /* Execute oscillatory movement whenever AMP is nonzero
               * (oscillation is along line perpendicular to DIR, per-
               * mitting interesting combinations with linear motion) */
               if (oscillar_movement(Obj)) {
                  ampl = FS16*Obj->amp;
                  ampx = ampl*sin((double)Obj->dir);
                  ampy = ampl*cos((double)Obj->dir);
                  angl = (cycnum1 - Obj->movsta)/Obj->freq;
                  /* Motion determined by oscillation type */
                  if (square(Obj) || sawtooth(Obj)) {
                     /* Make a "straight-line" version of cos(ANGLE),
                     *  where ANGLE is known to be non-negative */
                     if ((angl=4.0*fmod((double)angl,1.0)) > 2.0)
                        angl -= 3.0;
                     else
                        angl = 1.0 - angl;
                     if (sawtooth(Obj)) {
                        /* Sawtooth overides square */
                        Obj->icurx = Obj->iorgx - RTOJRND(ampx*angl);
                        Obj->icury = Obj->iorgy - RTOJRND(ampy*angl);
                        }
                     else {
                        /* Sqare wave */
                        Obj->icurx =
                           Obj->iorgx - RTOJRND(sign(ampx,angl));
                        Obj->icury =
                           Obj->iorgy - RTOJRND(sign(ampy,angl));
                        }
                     } /* End SQUARE or SAWTOOTH */
                  else {
                     /* Cosine oscillation */
                     angl *= PI2;
                     Obj->icurx = Obj->iorgx - RTOJRND(ampx*cos(angl));
                     Obj->icury = Obj->iorgy - RTOJRND(ampy*cos(angl));
                     }
                  } /* End oscillatory motion */
               else {
                  /* no oscillation, assign original coords to current */
                  Obj->icurx = Obj->iorgx;
                  Obj->icury = Obj->iorgy;
                  }
               break;   /* End case NORMAL */

               } /* End switch */

/* Apply specified boundary conditions for all motion.
*  Note:  New coordinate calculation is easier (AND vs. MOD) if
*     boundaries are at edges of entire input array (vs. at edges
*     of inner window, so separate code is used. */

            switch(Obj->movebounds) {

               long wsizx,wsizx2;   /* Window size in X, 2 times */
               long wsizy,wsizy2;   /* Window size in Y, 2 times */

            case BNDMR:    /* MIRROR */
               /* Check if object is too big for defined window
               *  (cannot be done in ENVCTL as selection and/or
               *  rotation may introduce bounds error).
               *  In case of large translation, MOD (or AND when
               *  jsizx,y = lx,y) facilitates double bounds */
               if (changed_bounds(Obj)) envlim(Eptr, Obj);
               wsizx = (long)(Obj->jsizx - Obj->ehix + Obj->elox);
               wsizx2 = wsizx << 1;
               wsizy = (long)(Obj->jsizy - Obj->ehiy + Obj->eloy);
               wsizy2 = wsizy << 1;
               if (wsizx <= 0 || wsizy <= 0)
                  convrt("('0--> ENVCYC: Object ',J1IL6,"
                     "'exceeds mirror bounds')", &Obj->irf, NULL);

               /* Calculate new X coordinate */
               itmpx = (long)(Obj->icurx + Obj->elox - Obj->jxmin) %
                  wsizx2;
               if (itmpx < 0) itmpx += wsizx2;
               /* If odd-numbered window, move to left */
               if (itmpx >= wsizx) {
                  itmpx = wsizx2 - 1 - itmpx;
                  Obj->esignx = -Obj->esignx;
                  }
               itmpx += (long)(Obj->jxmin - Obj->elox);
               swloem(Obj->icurx, itmpx, EAabx(162))

               /* Calculate new Y coordinate */
               itmpy = (long)(Obj->icury + Obj->eloy - Obj->jymin) %
                  wsizy2;
               if (itmpy < 0) itmpy += wsizy2;
               /* If odd-numbered window, move up */
               if (itmpy >= wsizy) {
                  itmpy = wsizy2 - 1 - itmpy;
                  Obj->esigny = -Obj->esigny;
                  }
               itmpy += (long)(Obj->jymin - Obj->eloy);
               swloem(Obj->icury, itmpy, EAabx(162))
               break; /* End case BNDMR */

            case BNDTB:    /* TOROIDAL */
               /* Calculate new X coordinate */
               if (Obj->jsizx == llxs16) Obj->icurx &= llxs16m1;
               else {
                  if ((itmpx = (Obj->icurx - Obj->jxmin)%Obj->jsizx)
                     < 0) itmpx += Obj->jsizx;
                  itmpx += (long)Obj->jxmin;
                  swloem(Obj->icurx, itmpx, EAabx(162))
                  }

               /* Calculate new Y coordinate */
               if (Obj->jsizy == llys16) Obj->icury &= llys16m1;
               else {
                  if ((itmpy = (Obj->icury - Obj->jymin)%Obj->jsizy)
                     < 0) itmpy += Obj->jsizy;
                  itmpy += (long)Obj->jymin;
                  swloem(Obj->icury, itmpy, EAabx(162))
                  }
               break; /* End case BNDTO */

            case BNDEG:    /* EDGE */
               /* Edge boundary (place object at nearest edge) */
               if (changed_bounds(Obj)) envlim(Eptr, Obj);
               /* Calculate new X coordinate */
               Obj->icurx = (Obj->icurx > -Obj->elox) ?
                  min(Obj->icurx, llxs16m1-Obj->ehix) :
                  min(-Obj->elox, llxs16m1-Obj->ehix);

               /* Calculate new Y coordinate */
               Obj->icury = (Obj->icury > -Obj->eloy) ?
                  min(Obj->icury, llys16m1-Obj->ehiy) :
                  min(-Obj->eloy, llys16m1-Obj->ehiy);
               break; /* End case BNDEG */

            case BNDNO:    /* NONE */
               break; /* End case NONE */

            default:       /* Error trap for bad BC option */
               goto BOUNDS_ERR;
               } /* End switch */

            } /* End object not hit */
         } /* End object motion */
      /* Place object on input array... pixels checked in envpia() */
      envpia(Eptr, inpary, Obj);

      }  /* End Obj loop */

/*---------------------------------------------------------------------*
*                         ADD RECEPTOR NOISE                           *
*                                                                      *
*   Mean fraction of receptors with noise = UNOISE                     *
*   Noise amplitude added to each such receptor is Gaussian with       *
*     mean = ANOISE and standard deviation = SNOISE                    *
*---------------------------------------------------------------------*/

   if (Eptr->unoise) {
      long  icnv1 = 0;
      long  lp1,lp2;
      long  index2;
      si32  *pr1;
      float *pr2;
      for (lp1=1,lp2=lly; lp1<=lltotal; lp1+=llymul2,lp2+=llymul2) {
         pr1 = Eptr->prnd1;
         pr2 = Eptr->prnd2;
         rannum(pr1, lly, &Eptr->iseed1, 15);
         rannor(pr2, lly, &Eptr->iseed2, Eptr->anoise, Eptr->snoise);
         for (index2=lp1; index2<=lp2; pr1++,pr2++,index2++) {
            if (Eptr->unoise >= *pr1) {
               icnv1 = (long)inpary[index2] + (long)(256.0 * *pr2);
               if (icnv1>255) icnv1 = 255;
               else if (icnv1<0) icnv1 = 0;
               inpary[index2] = (stim_type)icnv1;
               }
            } /* End inner for */
         } /* End outer for */
      } /* End if enoise */

/*---------------------------------------------------------------------*
*                        CALCULATE ARM POSITIONS                       *
*                                                                      *
*  Determine bend angle variables and coords in ARM and JOINT blocks.  *
*  Note: coordinates are relative to input array, not display.         *
*---------------------------------------------------------------------*/

   if (Eptr->parm1) {

      float x,y,x0,y0;     /* X,Y values for arm movement (temp) */

      /* Initialize AATTX and AATTY to starting arm positions */
      envarm(Eptr);

      /* Process all arms */
      for (pa = Eptr->parm1; pa; pa = pa->pnxarm) {
         x = pa->aattx;
         y = pa->aatty;
         angl = refang[pa->jpos];
         asign = 1.0;

         /* Process all joints */
         pj = pa->pjnt;
         njnts1 = pa->njnts;
         for (i=0; i<njnts1; pj++,i++) {
            if (change && jnt_is_moving(pj)) {
               /* Update joint angle */
               float ja1 = pj->u.jnt.ja +
                  change[get_jptr1(pj)]*pj->scla;
               /* Apply the angular limits tests */
               ja1 = min(ja1, pj->u.jnt.jmx);
               ja1 = max(ja1, pj->u.jnt.jmn);
               pj->u.jnt.ja = ja1;
               }
            /* Update position using new angle */
            angl += 180.0 + asign*pj->u.jnt.ja;
            radjt = TORADS*angl;
            pj->u.jnt.jx = x += pj->jl*cos((double)radjt);
            pj->u.jnt.jy = y -= pj->jl*sin((double)radjt);
            asign = -asign;
            } /* End loop over joints */

/* Implement universal joint if present:
*     1) Apply change to the integrated x,y shifts,
*     2) Confine this change to the input array area if requested,
*     3) Reflect confined changes back to delta[xy] and [xy]dd,
*     3) Calculate the affine tranformation matrix elements,
*     4) Apply the tranformation to all joints.
*  Note:  Reflection of confined changes back to delta[xy] is a mess,
*     but is necessary to prevent accumulating out-of-bounds changes
*     that must be undone before arm can move again.  Reflection is
*     also necessary for correct kinesthesia.
*  Note:  In this block, x,y are the coords of the arm tip and pj
*     points to the extra joint block at the end of the joint list
*     that contaeins UJ info.  */

         if (is_universal(pa)) {
            float sclx1  = pj->u.ujnt.ujsclx;   /* xy scale    */
            float rn2;        /* Final joint radius (squared)  */
            float sdn,cdn;    /* Unnorm. sin,cos affine trans. */
            float xdd,ydd;    /* Change in delta[xy] in this cycle */
            float xp,yp;      /* Moved joint coords (x',y')    */
            /* Calculate the angular changes from the scaled shifts */
            if (!change || sclx1 == 0.0 || pj->scla == 0.0 ||
                  !jnt_is_moving(pj))
               xdd = ydd = 0.0;
            else {
               xdd = change[get_jptr1(pj)]*pj->scla;
               ydd = change[++jptr]*pj->scla;
               xp = x + sclx1*sin(TORADS*(pj->u.ujnt.deltax+xdd));
               yp = y + sclx1*sin(TORADS*(pj->u.ujnt.deltay+ydd));
               /* If edge check requested, confine tip of arm to area
               *  of input array.  atan2 is used to get asin.  */
               if (pa->jatt & ARMEC) {
                  if (xp < 0.0 || xp > rrlx) {
                     if (xp < 0.0) xp = 0.0;
                     else          xp = rrlx;
                     sdn = (xp - x)/sclx1;
                     cdn = (sdn >= 1.0) ? 0.0 : sqrt(1.0 - sdn*sdn);
                     xdd = atan2(sdn,cdn)/TORADS - pj->u.ujnt.deltax;
                     }
                  if (yp < 0.0 || yp > rrly) {
                     if (yp < 0.0) yp = 0.0;
                     else          yp = rrly;
                     sdn = (yp - y)/sclx1;
                     cdn = (sdn >= 1.0) ? 0.0 : sqrt(1.0 - sdn*sdn);
                     ydd = atan2(sdn,cdn)/TORADS - pj->u.ujnt.deltay;
                     }
                  } /* End edge confinement tests */
               pj->u.ujnt.deltax += xdd;
               pj->u.ujnt.deltay += ydd;
               /* Place the origin at the attachment point */
               x -= x0 = pa->aattx; xp -= x0;
               y -= y0 = pa->aatty; yp -= y0;
               /* Calculate the affine transformation matrix elements */
               if ((rn2 = x*x + y*y) > 1e-12)
                  cdn = (xp*x + yp*y)/rn2, sdn = (yp*x - xp*y)/rn2;
               else
                  cdn = sdn = 0.0;
               /* Apply the transformation to the joints */
               for (pj=pa->pjnt,i=0; i<njnts1; pj++,i++) {
                  xp = pj->u.jnt.jx - x0;
                  yp = pj->u.jnt.jy - y0;
                  pj->u.jnt.jx = x = xp*cdn - yp*sdn + x0;
                  pj->u.jnt.jy = y = yp*cdn + xp*sdn + y0;
                  } /* End joint shifting loop */
               } /* End changes not zero */

            /* Calculate absolute and relative motions for kinesthesia.
            *  Here SDN,CDN = sin,cos of delta tau, with sign of sdn
            *    reversed to allow for left-handed pixel coords.
            *  EJDTAU=0 when motion is vertical along -Y.  */
            if (pa->dovjtk > 0) {
               sdn = ydd*pj->u.ujnt.ujdxn - xdd*pj->u.ujnt.ujdyn;
               cdn = xdd*pj->u.ujnt.ujdxn + ydd*pj->u.ujnt.ujdyn;
               pj->u.ujnt.ujtau  = (fabs(xdd) + fabs(ydd) > 1e-12) ?
                  atan2(xdd,-ydd) : 0.0;
               pj->u.ujnt.ujdtau = (fabs(sdn) + fabs(cdn) > 1e-12) ?
                  atan2(sdn,cdn)  : 0.0;
               pj->u.ujnt.ujdxn = xdd;
               pj->u.ujnt.ujdyn = ydd;
               }
            pj++;
            } /* End universal joint */

         /* Common joint code */
         pa->armx0 = pa->armx;
         pa->armx = x;
         pa->army0 = pa->army;
         pa->army = y;

/* Deal with gription */

         if (gription_check(pa)) {
            gtll = pj->u.grip.gwidth;
            xd = pa->armx0 - x;
            yd = pa->army0 - y;

            /* Check for arm movement of zero */
            if ((darm = sqrt((double)(xd*xd + yd*yd))) == 0.0)
               continue;
            cosax = xd/darm;
            sinax = yd/darm;
            if (cosax) slope[3] = slope[1] =  sinax/cosax;
            if (sinax) slope[2] = slope[0] = -cosax/sinax;
            xb[0] = x - 0.5*gtll*sinax;
            yb[0] = y + 0.5*gtll*cosax;
            xb[1] = xb[0] + gtll*sinax;
            yb[1] = yb[0] - gtll*cosax;
            xb[2] = xb[1] + darm*cosax;
            yb[2] = yb[1] + darm*sinax;
            xb[3] = xb[0] + darm*cosax;
            yb[3] = yb[0] + darm*sinax;
            xb[4] = xb[0];

            /* Find smallest and largest x(IA) coordinate */
            xlit = rrlxm1;
            xbig = 0.0;
            for (i=0; i<4; i++) {
               xlit = min(xlit, xb[i]);
               xbig = max(xbig, xb[i]);
               }
            lowx = max(xlit, 0.0);
            topx = min(xbig-1e-12, rrlxm1);

            /* Loop over columns of pixels in the X range and determine
            *  Y range in each column by checking all vertices and in-
            *  tersections of lines against left and right box edges. */
            ipx = llymul2*lowx + lly; /* (lly for extra code cols) */
            for (ixp=lowx; ixp<=topx; ipx+=llymul2,ixp++) {
               ylit = rrlym1;
               ybig = 0.0;
               for (ixe=ixp; ixe<=ixp+1; ixe++) {
                  char j;
                  for (xe=ixe,j=0; j<4; j++) {
                     if ((xe - xb[j+1])*(xe - xb[j]) < 0.0) {
                        yint = yb[j] + (xe - xb[j])*slope[j];
                        ylit = min(ylit, yint);
                        ybig = max(ybig, yint);
                        }
                     if ((long)xb[j] == ixe) {
                        yint = yb[j];
                        ylit = min(ylit, yint);
                        ybig = max(ybig, yint);
                        }
                     } /* End j loop */
                  } /* End ixe loop */
               lowy = max(ylit, 0.0);
               topy = min(ybig-1e-12, rrlym1);

               /* Loop over all pixels in this column that touch object
               *  (inpary[ipy] = 0 for not hit, otherwise ref num of
               *  hit object).  */
               ipy = ipx + lowy;
               for (iyp=lowy; iyp<=topy; ipy++,iyp++) {
                  if ((irf1 = (int)inpary[ipy]) == 0) continue;

                  /* Find pointer to object hit */
                  if (!(Obj = Eptr->pobjrf[irf1])) goto UFO_ERR;

                  /* If object already hit, end of iteration */
                  if (is_hit(Obj)) continue;

                  /* Not already hit... mark as hit now and store
                  *  desired move direction and velocity in ESIGNX
                  *  and ESIGNY.  Origin coordinates and other motion
                  *  parameters must be saved for use with new object
                  *  selected later.  staycnt is left alone, but will
                  *  not be checked again until object moves off IA. */
                  Obj->objflag |= OBJHT;
                  Obj->movsta = cycnum1;
                  Obj->movend = LONG_MAX;
                  Obj->esignx = atan2(yd,-xd);
                  Obj->esigny = pj->u.grip.gspeed*darm;

                  /* Following line of code is not used due to resulting
                  *  ugly rotations... designed to introduce more spin
                  *  if side-swiped (yd*sinax + xd*cosax = distance from
                  *  hand to object times sin(angle between hand-object
                  *  and hand-motion vectors))... if this code executes,
                  *  existing rotation must be explicity saved. */
#if 0
                     Obj->angvel = -0.13 * (yd*sinax + xd*cosax);
#endif
                  } /* End iyp loop */

               } /* End ixp loop */
            } /* End gription */
         } /* End arm loop */
      } /* End if arms */

/*---------------------------------------------------------------------*
*                           PROCESS WINDOWS                            *
*---------------------------------------------------------------------*/

   /* Note: pwo is set just to get rid of 'use before def' warning */
   for (pw=pwo=Eptr->pwdw1; pw; pwo=pw,pw=pw->pnxwdw) {

      float x,y;                    /* Working coords */
      short lkchng = pw->kchng;     /* Local copy of kchng */

      if (lkchng == WDWLCK) {
         /* Window locked */
         pw->wx  += aprv[0];
         pw->wy  += aprv[1];
         pw->wwd += aprv[2];
         pw->wht += aprv[3];
         }

      else if (lkchng < WDWZIP) {
         /* Window linked to an object... must search for object each
         * cycle, as objects can be freely created and destroyed */
         memset((char *)aprv, 0, 4*sizeof(float));
         irf1 = -lkchng;   /* Assignment intended--next line */
         if (!(Obj = Eptr->pobjrf[irf1])) goto UFO_ERR;

         x = (float)(Obj->icurx)*SM16 - 0.5*pw->wwd;
         y = (float)(Obj->icury)*SM16 + 0.5*pw->wht;
         aprv[0] = x - pw->wx;
         aprv[1] = y - pw->wy;
         pw->wx = x;
         pw->wy = y;
         }

      else if (!change || !wdw_is_moving(pw))
         memset((char *)aprv, 0, 4*sizeof(float));
      else if (lkchng >= WDWPIG) {
         /* Piggyback window--moves relative to previous */
         float sclx1 = pw->sclx;

         /* Apply changes, test for edge of previous window.  Need
         *  two changes: current and previous.  Previous change is
         *  stored in aprv[0], current change in changes array.  */
         if ((pw->wx += (aprv[0] + change[get_jptr1(pw)]*sclx1)) <
            pwo->wx) pw->wx = pwo->wx;
         if ((pw->wy -= (aprv[1] + change[++jptr]*sclx1)) >
            pwo->wy) pw->wy = pwo->wy;
#if 0
         /* Code for dilating linked window--not yet supported */
         if (lkchng > WDWPTRD) {
            pw->wwd = (((pw->wwd += (aprv[2] + change[++jptr]*sclx1))
               < 1.0) ? 1.0 : min(pw->wwd, pwo->wwd));
            pw->wht = (((pw->wht += (aprv[3] + change[++jptr]*sclx1))
               < 1.0) ? 1.0 : min(pw->wht, pwo->wht));
            }
#endif

         /* Width of previous window minus current width */
         pw->wx = min(pw->wx, pwo->wx + pwo->wwd - pw->wwd);
         pw->wy = max(pw->wy, pwo->wy - pwo->wht + pw->wht);
         aprv[0] = pw->wx - aprv[0];
         aprv[1] = pw->wy - aprv[1];
         aprv[2] = pw->wwd - aprv[2];
         aprv[3] = pw->wht - aprv[3];
         } /* End piggyback */

      else {
         /* Normal window--moves independently */
         float sclx1 = pw->sclx;
         /* Save our coordinates as previous values in case
         *  locked/piggied window follows */
         aprv[0] = pw->wx;
         aprv[1] = pw->wy;
         aprv[2] = pw->wwd;
         aprv[3] = pw->wht;
         if ((pw->wx += change[get_jptr1(pw)]*sclx1) < 0.0)
            pw->wx = 0.0;
         if ((pw->wy -= change[++jptr]*sclx1) > rrly)
            pw->wy = rrly;
         if (lkchng == WDWTRD) {
            pw->wwd = (((pw->wwd += change[++jptr]*sclx1) < 1.0) ?
                1.0 : min(pw->wwd, rrlx));
            pw->wht = (((pw->wht += change[++jptr]*sclx1) < 1.0) ?
                1.0 : min(pw->wht, rrly));
            }
         pw->wx = min(pw->wx, rrlx - pw->wwd);
         pw->wy = max(pw->wy, pw->wht);
         aprv[0] = pw->wx - aprv[0];
         aprv[1] = pw->wy - aprv[1];
         aprv[2] = pw->wwd - aprv[2];
         aprv[3] = pw->wht - aprv[3];
         } /* End else */
      } /* End pw loop */

/* Print the object list when requested */

   if (Eptr->kobjlist && Eptr->pobj1) {
      char valprt[8];
      cryout(RK_P4,"0 REF  NAME  ISN  IGN  ORGX  ORGY  ORGZ  CURX  "
         "CURY  VALUE", RK_LN2, NULL);
      for (Obj=Eptr->pobj1; Obj; Obj=Obj->pnxobj) {
         if ((unsigned short)Obj->iadapt == DFLT_ENVVAL)
            strcpy(valprt, "   N/A ");
         else
         wbcdwt(&Obj->iadapt, valprt,
            RK_BSCL*8|RK_D*4|RK_IORF|RK_NHALF|6);
         if (Eptr->kobjlist > 1) spout(1);
         convrt("(P4,IL5,2XA4,2*UH5,2B16IJ6.2,IH6,2B16IJ6.2,A7)",
            &Obj->irf, Obj->pshape->nshp, &Obj->isn, &Obj->ign,
            &Obj->iorgx, &Obj->iz, &Obj->icurx, valprt, NULL);
         }
      } /* End object print */

   return rc;

SELECT_ERR:
   cryout(RK_P1, "0--> ENVCYC: Unknown source for select object",
      RK_LN2, NULL);
   RK.iexit |= ENV_ERR;
   return ENV_FATAL;

BOUNDS_ERR:
   cryout(RK_P1, "0--> ENVCYC: Unknown movement boundary condition",
      RK_LN2, NULL);
   RK.iexit |= ENV_ERR;
   return ENV_FATAL;

UFO_ERR:
   sconvrt(outbuff, "('0',J1I6)", &irf1, NULL);
   cryout(RK_P1, "0--> ENVCYC: Object ", RK_LN2, outbuff, RK_CCL,
      "is a UFO", RK_CCL, NULL);
   RK.iexit |= ENV_ERR;
   return ENV_FATAL;

   } /* End envcyc */
