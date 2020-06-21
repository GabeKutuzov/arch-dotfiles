/* (c) Copyright 1988-2018, The Rockefeller University *21116* */
/* $Id: envctl.c 25 2018-05-07 22:13:48Z  $ */
/***********************************************************************
*              Darwin III Environment Simulation Package               *
*                               ENVCTL                                 *
*           Process one control card for environment module            *
*                                                                      *
************************************************************************
*                                                                      *
*     A word about the way object sizes are controlled:                *
*                                                                      *
*        The variables TUX,TUY read from CREATE or SELECT cards are    *
*     defaulted to the window size (PXMAX-PXMIN+1,PYMAX-PYMIN+1).      *
*     An error occurs if the values entered exceed the default         *
*     because correct operation of ENVPIA requires this restriction.   *
*     These values are used in calls to ENVLOD or ENVSHP to restrict   *
*     the portion of the shape that is loaded into the object.  The    *
*     values are stored in UXUY in the OBJECT record for the SELECT    *
*     case, where loads occur subsequent to control card processing.   *
*                                                                      *
*        The variables TSX,TSY, which are stored in the SHAPE record,  *
*     indicate the size of the current loaded shape, which may be      *
*     less than or equal to the TUX,TUY limits.  These sizes will be   *
*     filled in from EUX,EUY only in the case of old-style user (not   *
*     library) shapes that do not have their size specified on the     *
*     SHAPE card.  TSX,TSY are stored by ENVSAV in the replay file     *
*     so the stimulus will be the same size when it is reconstructed.  *
*                                                                      *
*        Finally, we have the SHPSIZ variable in the OBJECT record,    *
*     which specifies the size of the block allocated for the          *
*     SHAPE record for that object.  It is used to avoid the need      *
*     to reallocate when a new shape is assigned, and thus may be      *
*     greater than or equal to the needs of the current SHAPE.         *
************************************************************************
*  V4A, 12/17/88, JWT - Translated into C                              *
*  Rev, 03/28/90, JWT - DELETE moved to envdel(), refnum to object     *
*       lookup table implemented, selection interval introduced        *
*  Rev, 04/17/90, JWT - Add support for pixel objects, delete          *
*       permutations                                                   *
*  V5C, 12/03/91, GNR - Correct envlod bug in SELECT REPEAT            *
*  Rev, 03/04/92, GNR - Use rf io routines                             *
*  Rev, 04/14/92, GNR - Fix selend bug (added cycnum to LONG_MAX)      *
*  Rev, 08/09/92, GNR - Introduce LIBHANDLE struct for port            *
*  Rev, 01/20/93, GNR - Add isn,ign to OBJDEF structure, reformat,     *
*       fix objflag boundary condition bug                             *
*  Rev, 05/27/95, GNR - Return pointer to created OBJDEF               *
*  Rev, 07/20/95, GNR - Assign DFLT_ENVVAL to objects w/o user value,  *
*       introduce libbase parameter and default_object routine, fix    *
*       errors in setting boundary conditions                          *
*  Rev, 04/11/97, GNR - Read SELECT parameters for bursting, dselint   *
*  Rev, 08/30/01, GNR - Automatic update of mxref per input found      *
*  Rev, 01/10/04, GNR - Print SELECT seeds                             *
*  ==>, 04/19/05, GNR - Last mod before committing to svn repository   *
*  Rev, 12/31/08, GNR - Replace jm64sl with mssle                      *
*  V8H, 12/01/10, GNR - Separate env.h,envglob.h w/ECOLMAXCH change    *
*  Rev, 02/23/16, GNR - Seeds to si32 for 64-bit compilation           *
*  R25, 04/01/18, GNR - Convert ibcdin calls to wbcdin (leave longs)   *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "envglob.h"
#include "rocks.h"
#include "rkxtra.h"
#include "rkarith.h"
#include "bapkg.h"

#define CREATE 1             /* Possible */
#define CSHAPE 2
#define MOVE   3                      /* input */
#define DELETE 4
#define SELECT 5                             /* card types */
#define ENOISE 6

   static char *CARD_HEADS[] =
      { "CREATE", "SHAPE", "MOVE", "DELETE", "SELECT", "ENOISE" };

#define default_start(i) (*(i) == ' ')
#define DEFAULT -1

/* These defs have been introduced when changing ibcdin calls to wbcdin.
*  Still to do is change many of these longs to ints */
#define LEA2INT  (RK_IORF|RK_CTST|RK_NINT|DFLT_MAXLEN-1)
#define LEA2LONG (RK_IORF|RK_CTST|RK_NLONG|DFLT_MAXLEN-1)
#define SID2LONG (RK_IORF|RK_CTST|RK_NLONG|SHP_NAME_SIZE-1)

/* Definitions for (S)MATCH (string matching) calls
*  CAUTION: ordering of strings in these arrays MUST correspond to
*           that of defined object bit flags (see file "objdef.h")   */
#define MVBCNUM 4
   static char *mvbc[]   = { "MIRROR","NONE","TOROIDAL","EDGE" };
#define SCBCNUM 2
   static char *scbc[]   = { "ZERO","TOROIDAL" };

#define SOURCENUM 3
   static char *source[] = { "PIXEL","SEQUENTIAL","LIBRARY" };
#define PIX 1                 /* Stimulus objects in pixel format  */
#define SEQ 2                 /* Stimulus objects from shape cards */
#define LIB 3                 /* Stimulus objects from library     */

#define OTYPENUM 4
   static char *otype[]  = { "RANDOM","CYCLE","REPEAT","PERMUTED" };
#define KORAND 1              /* Choose at random                  */
#define KOCYC  2              /* Cycle through stimulus objects    */
#define KOREP  3              /* Repeat same stimulus object       */
#define KOPERM 4              /* Permute stimulus objects          */

#define SCYCOPNUM 2
   static char *scopt[]  = { "EVERY", "UNTIL" };

#define REFNUM   1   /* Flags reading of object reference number */
#define INTERVAL 2   /* Flags reading of object interval size */
#define UNTILCYC 4   /* Flags reading of object deletion cycle */

static char outbuff[16];  /* Output buffer */
static void default_object(struct ENVDEF *Eptr, struct OBJDEF *Obj);
static int process_object(struct ENVDEF *Eptr, struct OBJDEF *Obj,
   short tux, short tuy);

struct OBJDEF *envctl(struct ENVDEF *Eptr, char *card) {

   LIBHANDLE handle;          /* envloc/envlod object handle */
   OBJECT *Optr;              /* Object pointer */
   float orien1;              /* Initial object rotation angle */
   float dir1;                /* Initial object direction    (degrees) */
   float angle1;              /* Object linear increment     (degrees) */
   float angvl1;              /* Object rotational increment (degrees) */
   float ejdst1;              /* Cycles between random object jumps    */
   long iend;                 /* Ending cycle for moving object */
   ui32 ic;                   /* kwscan field recognition flags */
   int  iref;                 /* Object reference number        */
   int  tint;                 /* Selection interval */
   int  card_type;            /* Switch value for card type */
   int  kwret;                /* kwscan return value        */
   short tux,tuy;             /* X,Y shape size limits */
   short kode,mbc;            /* Generic bit bunch (for flag setting)  */
   short ksel,kord;           /* Select and order options */
   char lea[DFLT_MAXLEN+1];   /* Input buffer */
   char fid[SHP_NAME_SIZE];   /* First shape name */
   char bid[SHP_NAME_SIZE];   /* Base shape name  */

/* Determine card type and switch accordingly */

   card_type = smatch(0,card,CARD_HEADS,6);
   switch (card_type) {

/*---------------------------------------------------------------------*
*                         Process CREATE card                          *
*---------------------------------------------------------------------*/

case CREATE:
      /* Allocate new object */
      Optr = (struct OBJDEF *)
         callocv(1, OBJECT_SIZE, "ENVCTL: create");
      /* Assign default values (same as SELECT block) */
      default_object(Eptr, Optr);
      orien1 = 0.0;
      /* (tux,tuy) = (0,0) indicates that default is to be used */
      tux = tuy = 0;
      /* Read object name and number (if specified) */
      inform("(SW1A4=V>0<65536IL)", fid, &Optr->irf, NULL);
      /* Read keyword parameters */
      while ((kwret = kwscan(&ic,
         "BC%X",
         "NONINTEGRAL%X",
         "X%VB16IJ",&Optr->iorgx,
         "Y%VB16IJ",&Optr->iorgy,
         "Z%IH",&Optr->iz,
         "ORIENTATION%F",&orien1,
         "V%B8IH",&Optr->iadapt,
         "KUX%VIH",&tux,
         "KUY%VIH",&tuy,
         "XMIN%VIH",&Optr->pxmin,
         "XMAX%VIH",&Optr->pxmax,
         "YMIN%VIH",&Optr->pymin,
         "YMAX%VIH",&Optr->pymax,  NULL)) != 0)  {
         switch(kwret) {
         case 1:   /* BC */
            Optr->placebounds = match(RK_EQCHK, RK_MREQF,
               ~RK_COMMA, 0, scbc, SCBCNUM);
            break;
         case 2:   /* NONINTEGRAL */
            Optr->objflag |= OBJNI;
            break;
            } /* End kwret switch */
         }

      /* Set derived values */
      Optr->orient = TORADS*orien1;

      /* Get object matrix from library */
      if (envloc(Eptr, fid, SEARCH_ALL, &handle)) goto SHPID_ERR;
      envlod(Eptr, Optr, &handle, tux, tuy);
      /* Complete initialization, link, and update IPRERF */
      Optr->isn = Optr->irf;
      Optr->ign = Optr->pshape->tgp;
      process_object(Eptr, Optr, tux, tuy);
      return Optr;   /* End case CREATE */

/*---------------------------------------------------------------------*
*                         Process SHAPE card                           *
*---------------------------------------------------------------------*/

case CSHAPE:
      /* Note: ENVSHP exit error ignored here...
      *  picked up by IEXIT */
      envshp(&Eptr->pshpfill, Eptr->eux, Eptr->euy, 0,
         Eptr->eihi, Eptr->eilo);
      break;         /* End case CSHAPE */

/*---------------------------------------------------------------------*
*                          Process MOVE card                           *
*---------------------------------------------------------------------*/

case MOVE:
      /* Read reference number of object to be moved */
      inform("(SW1I)", &iref, NULL);
      if (iref<=0 || iref>(int)Eptr->mxref) goto OBJREF_ERR;
      if (jfind(card,"UNT",5)) inform("(SXIL)", &iend, NULL);
      else iend = LONG_MAX;

      /* Locate object to be moved */
      if (!(Optr = Eptr->pobjrf[iref]))   /* Assignment intended */
         goto OBJREF_ERR;

      /* Set default values for motion */
      kode = Optr->objflag;
      mbc = BNDMR;  /* Default (or reset) to mirror bounds */
      Optr->movsta = Eptr->cycnum+1;
      Optr->movend = iend;
      /* ("frac" is (S16): default 0.25 = 65536/4 = 16384) */
      Optr->frac = 16384;
      Optr->freq = 10.0;
      Optr->jseed = 34652 + (si32)iref;
      Optr->mseed =  1008 + (si32)iref;
      Optr->esignx = Optr->esigny = 1.0;
      Optr->jdelay = -1;
      dir1 = angle1 = angvl1 = 0.0;
      ejdst1 = -1.0;

      /* Read keyword parameter values */
      ic = 0;
      while ((kwret = kwscan(&ic,
         "BC%X",                          /* Must be key 1 */
         "NONINTEGRAL%X",                 /* Must be key 2 */
         "SQUAREWAVE%X",                  /* Must be key 3 */
         "SAWTOOTH%X",                    /* Must be key 4 */
         "BOUSTROPHEDON%X",               /* Must be key 5 */
         "RANDOM%X",                      /* Must be key 6 */
         "JSEED%VIJ",&Optr->jseed,        /* Must be key 7 */
         "MSEED%VIJ",&Optr->mseed,        /* Must be key 8 */
         "DIRECTION%f",&dir1,
         "VELOCITY%vf",&Optr->vel,
         "ANGLE%f",&angle1,
         "FRACTION%vb16IJ",&Optr->frac,
         "ROTATION%f",&angvl1,
         "AMPLITUDE%vf",&Optr->amp,
         "FREQUENCY%vf",&Optr->freq,
         "JUMP%il",&Optr->jump,
         "JDELAY%ih",&Optr->jdelay,
         "EJDIST%f",&ejdst1,
         "XMIN%vb16IJ",&Optr->jxmin,
         "XMAX%vb16IJ",&Optr->jxmax,
         "YMIN%vb16IJ",&Optr->jymin,
         "YMAX%vb16IJ",&Optr->jymax,  NULL)) != 0)
         switch (kwret) {
         case 0:   /* End of card */
            break;
         case 1:   /* BC */
            mbc = match(RK_EQCHK, RK_MREQF,
               ~RK_COMMA, 0, mvbc, MVBCNUM);
            break;
         case 2:   /* NONINTEGRAL */
            kode |= OBJNI;
            break;
         case 3:   /* SQUAREWAVE */
            kode |= OBJSQ;
            break;
         case 4:   /* SAWTOOTH */
            kode |= OBJSW;
            break;
         case 5:   /* BOUSTROPHEDON */
            kode |= OBJBO;
            break;
         case 6:   /* RANDOM */
            kode |= OBJRW;
            break;
            } /* End kwret switch and kwscan while */

      /* Combine motion flags with other flags */
      Optr->objflag   = kode;
      Optr->movebounds = mbc;
      Optr->jxmin = max(Optr->jxmin, 0);
      Optr->jxmax = min(Optr->jxmax, Eptr->lxs16m1);
      if ((Optr->jsizx = Optr->jxmax-Optr->jxmin+1) <= 0)
         goto BOUNDS_ERR;
      Optr->jymin = max(Optr->jymin, 0);
      Optr->jymax = min(Optr->jymax, Eptr->lys16m1);
      if ((Optr->jsizy = Optr->jymax-Optr->jymin+1) <= 0)
         goto BOUNDS_ERR;
      if (ejdst1>0.0) Optr->ejdst = ejdst1*ejdst1;

      /* Convert all angular values to radians */
      Optr->dir    = TORADS*dir1;
      Optr->angle  = TORADS*angle1;
      Optr->angvel = TORADS*angvl1;

      /* Display default seeds */
      if (!bittst((byte *)&ic, 7L)) convrt("(P3,'    (Jump seed "
         "default is ',J0IJ12,H))", &Optr->jseed, NULL);
      if (!bittst((byte *)&ic, 8L)) convrt("(P3,'    (Walk seed "
         "default is ',J0IJ12,H))", &Optr->mseed, NULL);
      break;         /* End case MOVE */

/*---------------------------------------------------------------------*
*                         Process DELETE card                          *
*---------------------------------------------------------------------*/

case DELETE:
      cdscan(card, 1, DFLT_MAXLEN, RK_WDSKIP);
      /* Delete all objects specified by input list of ref #s */
      while (!(scan(lea,0) & RK_ENDCARD)) {
         struct OBJDEF *prev;    /* Ptr to last inspected object */
         wbcdin(lea, &iref, LEA2INT|RK_QPOS);
         /* Find target object */
         for (prev = NULL, Optr = Eptr->pobj1;
               Optr && Optr->irf != (long)iref;
               prev = Optr, Optr = Optr->pnxobj) ;
            /* Delete object if found */
            if (Optr) envdel(Eptr, Optr, prev);
            else goto OBJREF_ERR;
         } /* End while */
      break;         /* End case DELETE */

/*---------------------------------------------------------------------*
*                         Process SELECT card                          *
*---------------------------------------------------------------------*/

case SELECT: {
      int read_limits = TRUE;
      int dupchk = 0;      /* Set bits to check for dup options */
      cdscan(card, 1, DFLT_MAXLEN, RK_WDSKIP);

      /* Allocate and initialize select object */
      Optr = (struct OBJDEF *)
         callocv(1, sizeof(struct OBJDEF), "ENVCTL: select");
      /* Assign default values (same as CREATE block) */
      default_object(Eptr, Optr);
      /* Initialize SELECT-specific object fields */
      Optr->sel.orgseed = 19842653;
      Optr->sel.librec = Optr->sel.libend = DEFAULT;
      Optr->sel.stay = 1;
      Optr->sel.burst = 1;
      Optr->sel.gdelay = 4;

      /* Read positional parameters */
      iend = LONG_MAX;
      tint = -1;
      while (read_limits)
         switch (scan(lea,0)) {

      case RK_EQUALS: /* Keyword argument */
         read_limits = FALSE;
         scanagn();
         break;

      case RK_BLANK: /* Positional arg (ref #, EVERY, or UNTIL) */
      case RK_COMMA:
         if (!cntrl(lea)) /* Must be reference number */ {
            if (dupchk & REFNUM) /* deja vu? */ goto INPUT_ERR;
            wbcdin(lea, &Optr->irf, LEA2LONG);
            dupchk |= REFNUM;
            } /* End ref num */
         else /* Non-numeric value... EVERY or UNTIL */ {
            if (RK.scancode & RK_COMMA) ermark(RK_PUNCERR);
            switch (smatch(0,lea,scopt,SCYCOPNUM)) {

            case 1: /* EVERY */
               if (dupchk & INTERVAL) goto INPUT_ERR;
               scanck(lea,0,~RK_COMMA);
               wbcdin(lea, &tint, LEA2INT);
               dupchk |= INTERVAL;
               break;

            case 2: /* UNTIL */
               if (dupchk & UNTILCYC) goto INPUT_ERR;
               scanck(lea,0,~RK_COMMA);
               wbcdin(lea, &iend, LEA2LONG);
               iend += Eptr->cycnum;
               dupchk |= UNTILCYC;
               read_limits = FALSE;
               }  /* End smatch switch */
            } /* End else non-numeric */
         break; /* End positional arg case */

      default: goto INPUT_ERR;
         } /* End switch */

      /* Read keyword parameters */
      fid[0] = bid[0] = ' ';
      ksel = LIB;
      kord = KOCYC;
      tux = tuy = 0;
      ic = 0;
      while ((kwret = kwscan(&ic,
         "REUSE%X",                       /* Must be key 1 */
         "BC%X",                          /* Must be key 2 */
         "NONINTEGRAL%X",                 /* Must be key 3 */
         "KIND%X",                        /* Must be key 4 */
         "ORDER%X",                       /* Must be key 5 */
         "RESET%X",                       /* Must be key 6 */
         "SEED%VIJ",&Optr->sel.orgseed,   /* Must be key 7 */
         "FIRST%A4",fid,
         "BASE%A4",bid,
         "BURST%V>IH",&Optr->sel.burst,
         "DSELINT%IH",&Optr->sel.dselint,
         "STAY%VIL",&Optr->sel.stay,
         "X%VB16IJ",&Optr->iorgx,
         "Y%VB16IJ",&Optr->iorgy,
         "Z%IH",&Optr->iz,
         "GDELAY%VIH",&Optr->sel.gdelay,
         "KUX%V>IH",&tux,
         "KUY%V>IH",&tuy,
         "XMIN%VIH",&Optr->pxmin,
         "XMAX%VIH",&Optr->pxmax,
         "YMIN%VIH",&Optr->pymin,
         "YMAX%VIH",&Optr->pymax,
         "RANGE%V>IL",&Optr->sel.libend, NULL)) != 0)
         switch (kwret) {
         case 1:           /* REUSE */
            Optr->objflag |= OBJRE;
            break;
         case 2:           /* BC */
            Optr->placebounds = match(RK_EQCHK, RK_MREQF,
               ~RK_COMMA, 0, scbc, SCBCNUM);
            break;
         case 3:           /* NONINTEGRAL */
            Optr->objflag |= OBJNI;
            break;
         case 4:           /* KIND */
            ksel = match(RK_EQCHK, RK_MREQF, ~RK_COMMA, 0,
               source, SOURCENUM);
            break;
         case 5:           /* ORDER */
            kord = match(RK_EQCHK, RK_MREQF, ~RK_COMMA, 0,
               otype, OTYPENUM);
            break;
         case 6:           /* RESET */
            Optr->objflag |= OBJRB;
            break;
            } /* End kwret switch */

      /* Make current seed equal original seed */
      Optr->sel.curseed = Optr->sel.orgseed;
      /* Display default seed */
      if (!bittst((byte *)&ic, 7L)) convrt("(P3,'    (Selection "
         "seed default is ',J0IJ12,H))", &Optr->sel.orgseed, NULL);

      /* Compute interval-related quantities */
      Optr->sel.selend = iend;
      if (tint < 0) {
         tint = (int)Optr->sel.stay;
         if (tint < 0) tint = 1;
         }
      if (Optr->sel.stay < 0) Optr->sel.stay = tint;
      Optr->sel.staycnt = Optr->sel.selint = Optr->sel.selint0 = tint;
      Optr->sel.burstcnt = Optr->sel.burst;

      /* Make default (tux,tuy) be the (pxmin,pymin...) window
      *  (generous default size is ok since actual storage alloc
      *  is deferred to envlod())  */
      if (tux <= 0) tux = Optr->pxmax - Optr->pxmin + 1;
      if (tuy <= 0) tuy = Optr->pymax - Optr->pymin + 1;

      /*---------------------------------------------------------*
      * Put together the final SELTYP code and start things up:  *
      *                                                          *
      * SELTYP indicates object select type:                     *
      *   0: CREATE                                              *
      *   1: not used                                            *
      *   2: *SEQUENTIAL (defunct)                               *
      *   3: LIBRARY RANDOM          11: PIXEL RANDOM            *
      *   4: LIBRARY CYCLE           12: PIXEL CYCLE             *
      *   5: LIBRARY REPEAT          13: PIXEL REPEAT            *
      *   6: LIBRARY PERMUTED        14: PIXEL PERMUTED          *
      *---------------------------------------------------------*/

      switch (ksel) {
      case LIB: /* Library input */
         /* Make SELTYP code reflect ORDER option */
         Optr->seltyp = SEQUENT + kord;

         /* Determine starting item number */
         if (default_start(fid)) Optr->sel.libstart = 0;
         else if (envloc(Eptr, fid, SEARCH_LIB, &handle))
            goto SHPID_ERR;
         else Optr->sel.libstart = handle.libsh;

         /* Determine base item number (for GRPNO, ENVVAL) */
         if (default_start(bid)) Optr->sel.libbase = Optr->sel.libstart;
         else if (envloc(Eptr, bid, SEARCH_LIB, &handle))
            goto SHPID_ERR;
         else Optr->sel.libbase = handle.libsh;

         /* Determine ending item number */
         if (Optr->sel.libend == DEFAULT)
            Optr->sel.libend = Eptr->libnum - 1;
         else {
            Optr->sel.libend += Optr->sel.libstart - 1;
            if (Optr->sel.libend >= Eptr->libnum) {
               cryout(RK_P1,"0-->Warning: Range reduced to size of "
                  "library file.", RK_LN2, NULL);
               Optr->sel.libend = Eptr->libnum - 1;
               }
            }

         /* If order is REPEAT, set trigger to load once only */
         if (kord == KOREP) {
            Optr->sel.librec = Optr->sel.libstart;
            Optr->objflag |= OBJLT;
            }
         break;

      case PIX: /* Pixel (Optronics scanner) input */
         /* Make SELTYP code reflect ORDER option */
         Optr->seltyp = (SEQUENT + PIX_TYPE) + kord;

         /* Determine starting, base, and ending item numbers */
         if (default_start(fid))
            Optr->sel.libstart = 0;
         else
            wbcdin(fid, &Optr->sel.libstart, SID2LONG);
         if (default_start(fid))
            Optr->sel.libbase = Optr->sel.libstart;
         else
            wbcdin(bid, &Optr->sel.libbase, SID2LONG);
         if (Optr->sel.libend == DEFAULT)
            Optr->sel.libend = Eptr->pixnum - 1;
         else {
            Optr->sel.libend += Optr->sel.libstart - 1;
            if (Optr->sel.libend >= Eptr->pixnum) {
               cryout(RK_P1,"0-->Warning: Range reduced to size of "
                  "pixel file.", RK_LN2, NULL);
               Optr->sel.libend = Eptr->pixnum - 1;
               }
            }

         break;
         }  /* End ksel switch */

      /* Error if library start is before library base */
      if (Optr->sel.libbase > Optr->sel.libstart) goto BASE_ERR;

      /* If order is PERMUTED, alloc space for permutation */
      if (kord == KOPERM) {
         short nperm = Optr->sel.libend - Optr->sel.libstart + 1;
         Optr->sel.psperm = (short *)
            mallocv(nperm*sizeof(short),"ENVCTL: permuted");
         }

      /* Save (tux,tuy) for regulation of later loads */
      Optr->sel.ux = tux;
      Optr->sel.uy = tuy;
      /* Complete initialization, link, and update IPRERF */
      process_object(Eptr, Optr, tux, tuy);
      return Optr;
      } /* End SELECT local scope */

/*---------------------------------------------------------------------*
*                         Process ENOISE card                          *
*  N.B.  Placement of the 'KPIXEL' parameter on this card is an old    *
*  documented anomaly designed to allow control of pixel shape when    *
*  running outside CNS.                                                *
*---------------------------------------------------------------------*/

case ENOISE:
      cdscan(card, 1, DFLT_MAXLEN, RK_WDSKIP);
      /* Read keyword parameters */
      kwscan(&ic,
         "UNOISE%VB16IJ",&Eptr->unoise,
         "ANOISE%VF",&Eptr->anoise,
         "SNOISE%VF",&Eptr->snoise,
         "KPIXEL%VIC",&Eptr->kpixel,
         "USEED%VIJ",&Eptr->iseed1,
         "ASEED%VIJ",&Eptr->iseed2, NULL);
      /* Allocate noise arrays */
      if (!Eptr->prnd1) Eptr->prnd1 =
         (si32 *)mallocv(Eptr->ly*sizeof(si32), "ENVCTL: prnd1");
      if (!Eptr->prnd2) Eptr->prnd2 =
         (float *)mallocv(Eptr->ly*sizeof(float), "ENVCTL: prnd2");
      break;         /* End case ENOISE */

/*---------------------------------------------------------------------*
*                 Process unrecognized control cards                   *
*---------------------------------------------------------------------*/

default:
      goto INPUT_ERR;

         }  /* End card identifier switch */
      return NULL;

/*---------------------------------------------------------------------*
*                    Error exit labels for envctl                      *
*---------------------------------------------------------------------*/

INPUT_ERR:  cryout(RK_P1, "0***Unrecognized or duplicated input.",
               RK_LN2, NULL);
            RK.iexit |= ENV_ERR;
            return NULL;

SHPID_ERR:  cryout(RK_P1, "0***Shape name ", RK_LN2,
               fid, RK_CCL+4, " not found in library.", RK_CCL, NULL);
            RK.iexit |= ENV_ERR;
            return NULL;

OBJREF_ERR: sconvrt(outbuff, "('0',J1I6)", &iref, NULL);
            cryout(RK_P1, "0***Obj # ", RK_LN2,
               outbuff, RK_CCL, " not found", RK_CCL, NULL);
            RK.iexit |= ENV_ERR;
            skip2end();
            return NULL;

BASE_ERR:   cryout(RK_P1, "0***First item is before base item"
               " in shape library.", RK_LN2, NULL);
            RK.iexit |= ENV_ERR;
            return NULL;

BOUNDS_ERR: cryout(RK_P1, "0***Invalid motion bounds", RK_LN2,
               NULL);
            RK.iexit |= ENV_ERR;
            return NULL;

   } /* End envctl() */

/***********************************************************************
*                                                                      *
*  default_object: sets parameter defaults that are common to OBJECTs  *
*                  SELOBJECTs prior to scanning the control card       *
*                                                                      *
*  N.B.  Program assumes entire block has been set to zero on entry    *
*                                                                      *
*  input:  Eptr = pointer to active environment block                  *
*          Obj  = pointer to object being defined                      *
***********************************************************************/

static void default_object(struct ENVDEF *Eptr, struct OBJDEF *Obj) {

   Obj->irf = Eptr->iprerf + 1;
   Obj->iorgx = (si32)(Obj->pxmax = Eptr->lxsub1)<<15;
   Obj->iorgy = (si32)(Obj->pymax = Eptr->lysub1)<<15;
   Obj->iadapt = (short)DFLT_ENVVAL;
   Obj->objflag = OBJBR;
   Obj->placebounds = OPLZB;
   Obj->movebounds  = BNDMR;
   Obj->bouster = BOUINIT;
   } /* End default_object() */

/***********************************************************************
*                                                                      *
*  process_object: performs common initialization and linked list      *
*                  insertion for OBJECTs and SELOBJECTs...             *
*                  also updates IPRERF value                           *
*                                                                      *
*  input:  Eptr = pointer to active environment block                  *
*          Obj  = pointer to object being defined                      *
*          tux  = X shape size                                         *
*          tuy  = Y shape size                                         *
*  output: non-zero on error                                           *
***********************************************************************/

static int process_object(struct ENVDEF *Eptr, struct OBJDEF *Obj,
   short tux, short tuy) {

   struct OBJDEF *Ocurr, **Oprev; /* Obj list insertion ptrs */
   long irf1 = Obj->irf; /* Object reference number */
   short iz1;            /* Z coordinate value (grows TOWARDS you) */

   /* Initialize common CREATE and SELECT object fields */
   if (irf1<=0 || irf1>UI16_MAX) goto REFNUM_ERR;
   Obj->icurx = Obj->iorgx;
   Obj->icury = Obj->iorgy;
   Obj->movsta = LONG_MAX;
   Obj->jdelay = -1;
   Obj->ejdst = 4.0;
   if ((Obj->pxmin = max(Obj->pxmin,0)) >
       (Obj->pxmax = min(Obj->pxmax,Eptr->lxsub1))) goto BOUNDS_ERR;
   if ((Obj->pymin = max(Obj->pymin,0)) >
       (Obj->pymax = min(Obj->pymax,Eptr->lysub1))) goto BOUNDS_ERR;
   if (qobjtor(Obj) && (tux>(Obj->pxmax - Obj->pxmin + 1) ||
      tuy>(Obj->pymax - Obj->pymin + 1))) goto TWRAP_ERR;
   Obj->jxmin = (si32)Obj->pxmin<<16;
   Obj->jxmax = ((si32)Obj->pxmax<<16) + 65535L;
   Obj->jymin = (si32)Obj->pymin<<16;
   Obj->jymax = ((si32)Obj->pymax<<16) + 65535L;
   Obj->jsizx = Obj->jxmax - Obj->jxmin + 1;
   Obj->jsizy = Obj->jymax - Obj->jymin + 1;

/* Insert new object into object list (based on z coordinate) */

   iz1 = Obj->iz;
   for (Ocurr = Eptr->pobj1, Oprev = &Eptr->pobj1;
        Ocurr; Ocurr = Ocurr->pnxobj)  {
        if (Ocurr->iz <= iz1) Oprev = &Ocurr->pnxobj;
        if (Ocurr->irf == irf1) goto DUPREF_ERR;
        }

/* Insert object immed before object with next highest z (if found) */

   Obj->pnxobj = *Oprev;
   *Oprev = Obj;

/* Maintain max ref # and object lookup table.  If table
*  overflows, set size to next larger multiple of 32, but
*  record mxref as size-1 to allow for unused zero entry.
*  Set unused entries to zero.
*  We have already checked above that irf1 <= SHRT_MAX.  */

   Eptr->iprerf = max(Eptr->iprerf,irf1);
   if (irf1 > Eptr->mxref) {
      struct OBJDEF **po, **poe;
      size_t newsize = (irf1 + (POBJRF_INCR-1)) & ~(POBJRF_INCR-1);
      Eptr->pobjrf = (struct OBJDEF **)reallocv(Eptr->pobjrf,
         newsize*sizeof(struct OBJDEF *), "Object ref table");
      poe = Eptr->pobjrf + newsize;
      for (po=Eptr->pobjrf+Eptr->mxref+1; po<poe; ++po) *po = NULL;
      Eptr->mxref = (short)(newsize - 1);
      }
   Eptr->pobjrf[irf1] = Obj;
   return ENV_NORMAL;

/**** ERROR LABELS ****/
BOUNDS_ERR: cryout(RK_P1, "0***Invalid motion boundaries.",
               RK_LN2, NULL);
            RK.iexit |= ENV_ERR;
            return ENV_FATAL;

REFNUM_ERR: sconvrt(outbuff, "('0',J1IL6)", &irf1, NULL);
            cryout(RK_P1, "0***Obj # ", RK_LN2,
               outbuff, RK_CCL, " invalid.", RK_CCL, NULL);
            RK.iexit |= ENV_ERR;
            return ENV_FATAL;

DUPREF_ERR: sconvrt(outbuff, "('0',J1IL6)", &irf1, NULL);
            cryout(RK_P1, "0***Reference number ", RK_LN2,
               outbuff, RK_CCL, "is repeated.", RK_CCL, NULL);
            RK.iexit |= ENV_ERR;
            return ENV_FATAL;

TWRAP_ERR:  cryout(RK_P1, "0***Object is too large "
               "for toroidal window.", RK_LN2, NULL);
            RK.iexit |= ENV_ERR;
            return ENV_FATAL;

   }  /* End process_object() */

