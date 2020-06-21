/***********************************************************************
*                          'phantom' program                           *
*                               phrdcc                                 *
*                                                                      *
*                      Read input control cards                        *
*                                                                      *
*  Arguments:  None                                                    *
*  Returns:    0 if successful, otherwise nonzero error signal         *
*                                                                      *
*  V1A, 01/24/95, GNR - Initial version                                *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "phantom.h"

extern struct phglob *PH;

int phrdcc(void) {
   
   struct phglob *pH = PH;       /* Local ptr to globals */
   Lattice **ppl = &PH->pflat;   /* Ptr to next lattice struct */
   Lattice *pl = NULL;           /* Ptr to current lattice struct */
   Object **ppo;                 /* Ptr to next object struct */
   Object *po;                   /* Ptr to current object struct */
   Brick  *pb;                   /* Ptr to current brick struct */
   Cylinder *pc;                 /* Ptr to current cylinder struct */
   Sphere *ps;                   /* Ptr to current sphere struct */
   int ilv;                      /* Lattice vector counter */
   char lea[DFLT_MAXLEN+1];      /* Space for scanning */
   
/* Keyword strings for and number of control cards */
   static char *cckey[] = {
      "FILE",
      "STUDY",
      "SERIES",
      "NOTE",
      "IMAGE",
      "PIXEL",
      "NOISE",
      "LATTICE",
      "BRICK",
      "CYLINDER",
      "SPHERE"
      };
   static int ncc = sizeof(cckey)/sizeof(char *);

/* Control card switch cases  */
/* N.B.  The enum values must match the corresponding cckey entries! */
   enum cctype {
      FILE_C = 1,
      STUDY,
      SERIES,
      NOTE,
      IMAGE,
      PIXEL,
      NOISE_C,
      LATTICE_C,
      BRICK_C,
      CYLINDER_C,
      SPHERE_C
      } ccsw;

/* Read a control card, print, and branch according to type */
 
   while (cryin() &&
         (ccsw = (enum cctype)smatch(RK_NMERR,RK.last,cckey,ncc))) {
      cdprt1(RK.last);        /* Print the card (if oprty <= 3) */
      switch (ccsw) {

/*---------------------------------------------------------------------*
*                              FILE card                               *
*---------------------------------------------------------------------*/

      case FILE_C:

         if (pH->fnm) {
            cryout(RK_P1," -->Discarding earlier file name.",
               RK_LN1,NULL);
            free(pH->fnm);
            }
         pH->fnm = mallocv((size_t)LFILNM+1,"File name");
         cdscan(RK.last,1,LFILNM,RK_WDSKIP);
         scanck(pH->fnm,RK_REQFLD,~RK_BLANK);

         break;

/*---------------------------------------------------------------------*
*                             STUDY card                               *
*---------------------------------------------------------------------*/

      case STUDY:
         
         if (pH->study) {
            cryout(RK_P1," -->Discarding previous study number.",
               RK_LN1,NULL);
            }
         inform("(SW1,IL)",&pH->study,NULL);

         break;

/*---------------------------------------------------------------------*
*                             SERIES card                              *
*---------------------------------------------------------------------*/

      case SERIES:
         
         if (pH->series) {
            cryout(RK_P1," -->Discarding previous series number.",
               RK_LN1,NULL);
            }
         inform("(SW1,IL)",&pH->series,NULL);

         break;

/*---------------------------------------------------------------------*
*                              NOTE card                               *
*---------------------------------------------------------------------*/

      case NOTE:

         if (pH->note) {
            cryout(RK_P1," -->Discarding earlier file note.",
               RK_LN1,NULL);
            free(pH->note);
            }
         pH->note = mallocv((size_t)MAX_NOTE_CHARS+1,"File note");
         cdscan(RK.last,1,MAX_NOTE_CHARS,RK_WDSKIP);
         scanck(pH->note,RK_REQFLD,~RK_BLANK);

         break;

/*---------------------------------------------------------------------*
*                           IMAGE SIZE card                            *
*---------------------------------------------------------------------*/

      case IMAGE:
         
         if (pH->cols || pH->rows || pH->secs) {
            cryout(RK_P1," -->Discarding previous image size.",
               RK_LN1,NULL);
            }
         inform("(SW2,3*VIL)",&pH->cols,&pH->rows,&pH->secs,NULL);

         break;

/*---------------------------------------------------------------------*
*                           PIXEL SIZE card                            *
*---------------------------------------------------------------------*/
      
      case PIXEL:
         
         if (pH->Vox[X] || pH->Vox[Y] || pH->Vox[Z]) {
            cryout(RK_P1," -->Discarding previous pixel size.",
               RK_LN1,NULL);
            }
         inform("(SW2,3VQ)",pH->Vox,NULL);

         break;

/*---------------------------------------------------------------------*
*                             NOISE card                               *
*---------------------------------------------------------------------*/

      case NOISE_C:
      
         if (pH->sigma) {
            cryout(RK_P1," -->Discarding previous sigma.",
               RK_LN1,NULL);
            }
         pH->nseed = DFLT_NSEED;
         inform("(SW1,VF,NIL)",&pH->sigma,&pH->nseed,NULL);

         break;

/*---------------------------------------------------------------------*
*                            LATTICE card                              *
*---------------------------------------------------------------------*/

      case LATTICE_C:
         
         *ppl = pl =
            (Lattice *)callocv(1,sizeof(Lattice),"Lattice data");
         ppl = &pl->pnlat;
         ppo = &pl->pfobj;

         /* Read and count 0 to 3 lattice vectors */
         cdscan(RK.last,1,DFLT_MAXLEN,RK_WDSKIP);
         for (ilv=0; ilv<3; ilv++) {
            if (scan(lea,RK_NEWKEY) & RK_ENDCARD) break;
            scanagn();
            inform("(S,3F)",pl->Lv[ilv],NULL);
            }
         pl->nLv = ilv;

         break;

/*---------------------------------------------------------------------*
*                             BRICK card                               *
*---------------------------------------------------------------------*/

      case BRICK_C:
      
         /* If no preceding lattice, create one with no vectors */
         if (!pl) {
            *ppl = pl =
               (Lattice *)callocv(1,sizeof(Lattice),"Lattice data");
            ppl = &pl->pnlat;
            ppo = &pl->pfobj;
            }
         *ppo = po =
            (Object *)callocv(1,sizeof(Object),"Object data");
         ppo = &po->pnobj;
         po->kobj = BRICK;
         pb = &po->o.b;

         inform("(SW1,I,3F,3F,3F,3F)",&pb->rho,
            po->O,pb->Be[0],pb->Be[1],pb->Be[2],NULL);
         pH->hiden = max(pH->hiden,pb->rho);

         break;

/*---------------------------------------------------------------------*
*                            CYLINDER card                             *
*---------------------------------------------------------------------*/

      case CYLINDER_C:
      
         /* If no preceding lattice, create one with no vectors */
         if (!pl) {
            *ppl = pl =
               (Lattice *)callocv(1,sizeof(Lattice),"Lattice data");
            ppl = &pl->pnlat;
            ppo = &pl->pfobj;
            }
         *ppo = po =
            (Object *)callocv(1,sizeof(Object),"Object data");
         ppo = &po->pnobj;
         po->kobj = CYLINDER;
         pc = &po->o.c;

         inform("(SW1,I,3F,3F,VF)",&pc->rho,
            po->O,pc->Ca,&pc->Cr,NULL);
         pH->hiden = max(pH->hiden,pc->rho);

         break;

/*---------------------------------------------------------------------*
*                             SPHERE card                              *
*---------------------------------------------------------------------*/

      case SPHERE_C:
         
         /* If no preceding lattice, create one with no vectors */
         if (!pl) {
            *ppl = pl =
               (Lattice *)callocv(1,sizeof(Lattice),"Lattice data");
            ppl = &pl->pnlat;
            ppo = &pl->pfobj;
            }
         *ppo = po =
            (Object *)callocv(1,sizeof(Object),"Object data");
         ppo = &po->pnobj;
         po->kobj = SPHERE;
         ps = &po->o.s;

         inform("(SW1,I,3F,VF)", &ps->rho, po->O, &ps->Sd, NULL);
         pH->hiden = max(pH->hiden,ps->rho);

         break;

         } /* End cc switch */
      } /* End while cryin */

   return RK.iexit;
   } /* End phrdcc */
