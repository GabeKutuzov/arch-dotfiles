/***********************************************************************
*                          'phantom' program                           *
*                               phckin                                 *
*                                                                      *
*                  Check input data for consistency                    *
*                                                                      *
*  Arguments:  None                                                    *
*  Returns:    0 if successful, otherwise nonzero error signal         *
*                                                                      *
*  V1A, 01/25/95, GNR - Initial version                                *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "phantom.h"
#include "mat33.h"

extern struct phglob *PH;

/*---------------------------------------------------------------------*
*                               sortLv                                 *
*                                                                      *
*  This routine sorts a set of lattice vectors by increasing length    *
*---------------------------------------------------------------------*/

void sortLv(Lattice *pl) {

   float shortest;               /* Shortest vector remaining */
   float t[3];                   /* Temporary vector */
   int ishort;                   /* Selector of shortest vector */
   int i,j;                      /* Vector counters */

   for (i=0; i<pl->nLv-1; i++) {
      shortest = pl->Llv[ishort = i];
      for (j=i+1; j<pl->nLv; j++)
         if (pl->Llv[j] < shortest) shortest = pl->Llv[ishort = j];
      if (ishort > i) {
         copy3(pl->Lv[ishort], t);
         copy3(pl->Lv[i], pl->Lv[ishort]);
         copy3(t,  pl->Lv[i]);
         pl->Llv[ishort] = pl->Llv[i];
         pl->Llv[i] = shortest;
         }
      }

   } /* End sortLv */


/*---------------------------------------------------------------------*
*                               phckin                                 *
*---------------------------------------------------------------------*/

int phckin(void) {
   
   struct phglob *pH = PH;       /* Local ptr to globals */
   Lattice *pl;                  /* Ptr to current lattice struct */
   Object  *po;                  /* Ptr to current object struct */
   Brick   *pb;                  /* Ptr to current brick struct */
   Cylinder *pc;                 /* Ptr to current cylinder struct */
   Sphere *ps;                   /* Ptr to current sphere struct */
   double V;                     /* Volume of object being tested */
   int ilat,iobj;                /* Lattice and object counters */
   int i,j;                      /* General counters */

/* Check for required global data */

   if (pH->Vox[X] <= 0.0 || pH->Vox[Y] <= 0.0 || pH->Vox[Z] <= 0.0) {
      cryout(RK_P1,"0***Pixel dimensions negative or missing.",
         RK_LN2,NULL);
      RK.iexit |= 1;
      }

   if (pH->cols <= 0 || pH->rows <= 0 || pH->secs <= 0) {
      cryout(RK_P1,"0***Image dimensions negative or missing.",
         RK_LN2,NULL);
      RK.iexit |= 1;
      }

   if (!pH->fnm) {
      cryout(RK_P1,"0***File name is missing.",
         RK_LN2,NULL);
      RK.iexit |= 1;
      }

   if (pH->study <= 0) {
      cryout(RK_P1,"0-->Missing study number set to 1.",
         RK_LN2,NULL);
      pH->study = 1;
      }

   if (pH->series <= 0) {
      cryout(RK_P1,"0-->Missing series number set to 1.",
         RK_LN2,NULL);
      pH->series = 1;
      }

/* Calculate vectors that define image bounds (mm) */

   pH->Face[X] = (double)(pH->cols-1)*pH->Vox[X];
   pH->Face[Y] = (double)(pH->rows-1)*pH->Vox[Y];
   pH->Face[Z] = (double)(pH->secs-1)*pH->Vox[Z];

/* Check lattice information.  Length checking is done before
*  sorting so vector identification numbers are meaningful.  */

   ilat = 0;
   if (!(pl = pH->pflat)) {
      cryout(RK_P1,"0***At least one lattice is required.",
         RK_LN2,NULL);
      RK.iexit |= 1;
      }
   else do {
      float t,tv[3];

      ++ilat;

      for (i=0; i<pl->nLv; i++) {
         int ii = i + 1;
         if ((pl->Llv[i] = len3(pl->Lv[i])) < 1.0E-4) {
            convrt("(P1,'0***Lattice ',J1I6,'vector ',J1I6,"
               "'is absurdly short.')",&ilat,&ii,NULL);
            RK.iexit |= 1;
            }
         for (j=ii; j<pl->nLv; j++) {
            cross33(pl->Lv[i], pl->Lv[j], tv);
            if (dot33(tv, tv) < 1.0E-16) {
               int jj = j + 1;
               convrt("(P1,'0***Lattice ',J1I6,'vector ',J1I6,"
                  " is [nearly] parallel to vector ',J1I6,H.)",
                  ilat,&ii,&jj,NULL);
               RK.iexit |= 1;
               }
            }
         }

/* Initializations required only if more than 1 lattice vector */

      if (pl->nLv > 1) {

         /* Sort the lattice vectors on increasing length.  This is
         *  done to minimize iterations of outer loops in phwimg.  */
         sortLv(pl);

         /* Store 2nd and 3rd dimension reciprocal lattice vectors
         *  and their squared lengths */
         cross33(pl->Lv[0], pl->Lv[1], pl->Lw3);
         cross33(pl->Lw3, pl->Lv[0], pl->Lw2);
         t = dot33(pl->Lv[1], pl->Lw2);
         if (fabs(t) > 1.0E-16)
            for (i=0; i<3; i++) pl->Lw2[i] /= t;
         pl->Lw2w2 = dot33(pl->Lw2, pl->Lw2);
         pl->Llw2 = sqrt(pl->Lw2w2);

      if (pl->nLv > 2) {

         t = dot33(pl->Lv[2], pl->Lw3);
         if (fabs(t) > 1.0E-12)
            for (i=0; i<3; i++) pl->Lw3[i] /= t;
         pl->Lw3w3 = dot33(pl->Lw3, pl->Lw3);

         }}

/* Check objects associated with current lattice */

      iobj = 0;
      if (!(po = pl->pfobj)) {
         convrt("(P1,'0***Lattice ',J1I6,"
            "'requires at least one object.')",&ilat,NULL);
         RK.iexit |= 1;
         }
      else do {
         ++iobj;

         switch (po->kobj) {
         
         case BRICK:          /* Error checking for bricks */
            
            pb = &po->o.b;
            /* Calculate volume enclosed by brick.  A small value
            *  indicates coplanar vectors.  */
            V = triple(pb->Be[0], pb->Be[1], pb->Be[2]);
            if (fabs(V) < 1.0E-12) {
               convrt("(P1,'0***Brick object ',J1I6,'on lattice '"
                  ",J1I6,'has degenerate volume.')",&iobj,&ilat,NULL);
               RK.iexit |= 1;
               }
            /* Compute and store reciprocal face vectors */
            cross33(pb->Be[1], pb->Be[2], pb->Bw[0]);
            cross33(pb->Be[2], pb->Be[0], pb->Bw[1]);
            cross33(pb->Be[0], pb->Be[1], pb->Bw[2]);
            for (i=0; i<3; i++) {
               for (j=0; j<3; j++)
                  pb->Bw[i][j] /= V;
               }

            break;

         case CYLINDER:       /* Error checking for cylinders */
            
            pc = &po->o.c;
            if (pc->Cr < 1.0E-4) {
               convrt("(P1,'0***Cylinder object ',J1I6,'on lattice '"
                  ",J1I6,'has degenerate radius.')",&iobj,&ilat,NULL);
               RK.iexit |= 1;
               }
            /* Compute length of hemiaxis of cylinder and check */
            pc->haxis = 0.5*len3(pc->Ca);
            if (pc->haxis < 0.5E-4) {
               convrt("(P1,'0***Cylinder object ',J1I6,'on lattice '"
                  ",J1I6,'has degenerate axis.')",&iobj,&ilat,NULL);
               RK.iexit |= 1;
               }
            else norm3(pc->Ca,pc->Cn);

            break;

         case SPHERE:         /* Error checking for sphere */
            
            ps = &po->o.s;
            if (ps->Sd < 1.0E-4) {
               convrt("(P1,'0***Sphere object ',J1I6,'on lattice '"
                  ",J1I6,'has degenerate diameter.')",&iobj,&ilat,NULL);
               RK.iexit |= 1;
               }
            ps->Sr = 0.5*ps->Sd;

            break;

            } /* End object type switch */

         po = po->pnobj;
         } while (po);        /* End object checking */

      pl = pl->pnlat;
      } while (pl);           /* End lattice checking */

   return RK.iexit;
   } /* End phckin */

