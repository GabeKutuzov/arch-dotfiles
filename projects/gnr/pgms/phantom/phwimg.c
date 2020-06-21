/***********************************************************************
*                          'phantom' program                           *
*                               phwimg                                 *
*                                                                      *
*                 Generate and write the actual image                  *
*                                                                      *
*  N.B.  The algorithm used in this program allocates space for the    *
*  entire 3D image in one block, rather than, for example, doing one   *
*  section at a time.  This was deemed to be reasonable because it     *
*  will minimize development time, and there is probably enough real   *
*  memory on current workstations to accommodate typical applications  *
*  (e.g. 100x100x100 two-byte pixels = "only" 2MB).                    *
*                                                                      *
*  Arguments:  nfd, file descriptor for the open output file           *
*  Returns:    0 if successful, otherwise nonzero error signal         *
*                                                                      *
*  V1A, 02/03/95, GNR - Initial version                                *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sysdef.h"
#include "phantom.h"
#include "mat33.h"
#include "NEMA.HDR"
#include "rocks.h"
#include "rkarith.h"

float poidev(float xm, long *idum);

extern struct phglob *PH;

int phwimg(int nfd) {

   struct phglob *pH = PH;       /* Local ptr to globals */
   HeaderItem GrpHd;             /* Group header */
   Lattice *pl;                  /* Ptr to current lattic data */
   Object *po;                   /* Ptr to current object struct */
   Brick  *pb;                   /* Ptr to current brick struct */
   Cylinder *pc;                 /* Ptr to current cylinder struct */
   Sphere *ps;                   /* Ptr to current sphere struct */
   short *pi;                    /* Ptr to image */
   short *pix,*piy,*piz;         /* Pointer to image col,row,sec */
   float afmn[3],afmx[3];        /* Augmented image box faces */
   float bmx1,bmx2,bmx3;         /* Max image box extent */
   float bmn1,bmn2,bmn3;         /* Min image box extent */
   float crrhh;                  /* Cylinder radius*radius + ht*ht */
   float obj[3];                 /* Handle coords of current object */
   float oqmn[3],oqmx[3];        /* Min,max obj extent along x,y,z
                                 *     relative to image box origin */
   float ow3mn,ow3mx;            /* Min,max abs obj extent along w3 */
   float qmn[3],qmx[3];          /* Min,max obj extent along x,y,z
                                 *     relative to object origin */
   float u1,u2,u3;               /* Coords of lattice point in fracs */
   float vim[8][3];              /* Coords of image vertices */
   float w2mn,w2mx;              /* Min,max rel obj extent along w2 */
   float y,z;                    /* Coords of image point in mm */
   float yr,zr;                  /* y,z relative to object origin */
   long imgsize = pH->cols * pH->rows * pH->secs;  /* Constants */
   long secsize = pH->cols * pH->rows;          /* for indexing */
   long colsize = pH->cols;                /* into image points */
   long imgbytesize = imgsize*sizeof(short);
   long fakelength = 0;          /* Incorrect length field required
                                 *  in GRP_SceneData header record */
   long ix,iy,iz;                /* Coords of image point in grids */
   long ixmn,iymn,izmn;          /* Min values of ix,iy,iz */
   long ixmx,iymx,izmx;          /* Max values of ix,iy,iz */
   long l1,l2,l3;                /* Lattice index along Lv1,2,3 */
   long lmn1,lmn2,lmn3;          /* Min index along Lv1,2,3 */
   long lmx1,lmx2,lmx3;          /* Max index along Lv1,2,3 */
   int ia,i2,i3;                 /* Axis counter and permutations */
   int irho;                     /* Working density value */
   int iv;                       /* Vertex, edge, or face counter */

   static int ip2[3] = { 1, 2, 0 }; /* Permutation vectors */
   static int ip3[3] = { 2, 0, 1 };

/* Allocate and clear memory for the complete 3D image */

   pi = (short *)callocv(imgsize, sizeof(short), "Image");

/* Calculate and store coordinates of image vertices */

   for (iv=0; iv<8; iv++) {
      vim[iv][X] = (iv&4) ? pH->Face[X] : 0.0;
      vim[iv][Y] = (iv&2) ? pH->Face[Y] : 0.0;
      vim[iv][Z] = (iv&1) ? pH->Face[Z] : 0.0;
      }

/* Loop over all replication lattices */

   for (pl=pH->pflat; pl; pl=pl->pnlat) {

/* Loop over all objects on current replication lattice */

      for (po=pl->pfobj; po; po=po->pnobj) {

/*---------------------------------------------------------------------*
*  Determine maximal extent of object along axes x,y,z and Lw2,Lw3.    *
*  Projections on Lw2 are relative to the object origin, projections   *
*  on Lw3 are relative to the global (image box) origin.  Projections  *
*  on x,y,z are stored in both forms for later use.  The max and min   *
*  values are redundant for symmetrical objects, but are carried along *
*  for use with future asymmetrical shapes.                            *
*---------------------------------------------------------------------*/

         w2mn = ow3mn = BIG;
         w2mx = ow3mx = -BIG;
         for (ia=0; ia<3; ia++) {
            qmn[ia] = oqmn[ia] = BIG;
            qmx[ia] = oqmx[ia] = -BIG; }

         switch (po->kobj) {

/*---------------------------------------------------------------------*
*  (Three levels of indenting suppressed)                              *
*---------------------------------------------------------------------*/

case BRICK:                         /*** BRICK ***/

   pb = &po->o.b;                /* Ptr to brick data */

/* Determine maximal extent of brick (fractional) along q and w
*  directions by calculating projections of each of 8 brick vertices
*  on each of these directions in turn, saving max and min of each.
*/

   for (iv=0; iv<8; iv++) {
      float s1,s2,s3,t,v[3],w[3];
      s1 = (iv&4) ? -0.5 : 0.5;
      s2 = (iv&2) ? -0.5 : 0.5;
      s3 = (iv&1) ? -0.5 : 0.5;
      for (ia=0; ia<3; ia++) {
         w[ia] = s1*pb->Be[0][ia] + s2*pb->Be[1][ia] +
            s3*pb->Be[2][ia];
         if (w[ia] < qmn[ia]) qmn[ia] = w[ia];
         if (w[ia] > qmx[ia]) qmx[ia] = w[ia];
         v[ia] = po->O[ia] + w[ia];
         if (v[ia] < oqmn[ia]) oqmn[ia] = v[ia];
         if (v[ia] > oqmx[ia]) oqmx[ia] = v[ia];
         }
      if (pl->nLv > 1) {
         t = dot33(w, pl->Lw2);
         w2mn = min(t, w2mn); w2mx = max(t, w2mx);
      if (pl->nLv > 2) {
         t = dot33(v, pl->Lw3);
         ow3mn = min(t, ow3mn); ow3mx = max(t, ow3mx);
         }}
      } /* End vertex loop */

   break;

/*--------------------------------------------------------------------*/

case CYLINDER:                         /*** CYLINDER ***/

   {  float ca;                  /* Cosine of axis,Lw angle */
      float dc,da;               /* Proj of ctr,axis on Lw */
      float sa2;                 /* Sin squared of some angle */

      pc = &po->o.c;             /* Ptr to cylinder data */
      crrhh = pc->Cr*pc->Cr + pc->haxis*pc->haxis; /* For later */

/* Determine maximal extent of cylinder (fractional) along q and w
*  directions by calculating projection of the axis on each of these
*  directions and adding in contribution the radius would make when
*  aligned in plane that contains axis and projection direction.  */

      /* Projections on coordinate axes */
      for (ia=0; ia<3; ia++) {
         da = 0.5*fabs(pc->Ca[ia]);
         ca = da/pc->haxis;
         sa2 = 1.0 - ca*ca;
         if (sa2 > 0.0) da += pc->Cr*sqrt(sa2);
         oqmn[ia] = po->O[ia] + (qmn[ia] = -da);
         oqmx[ia] = po->O[ia] + (qmx[ia] = da);
         }
      /* Projections on (Lv1 x Lv2) x Lv1 axis */
      if (pl->nLv > 1) {
         w2mx = 0.5*fabs(dot33(pc->Ca, pl->Lw2));
         ca = da/pc->haxis;
         sa2 = pl->Lw2w2 - ca*ca;
         if (sa2 > 0.0) w2mx += pc->Cr*sqrt(sa2);
         w2mn = -w2mx;
      /* Projections on normals to Lv1, Lv2 lattice planes */
      if (pl->nLv > 2) {
         dc = dot33(po->O, pl->Lw3);
         da = 0.5*fabs(dot33(pc->Ca, pl->Lw3));
         ca = da/pc->haxis;
         sa2 = pl->Lw3w3 - ca*ca;
         if (sa2 > 0.0) da += pc->Cr*sqrt(sa2);
         ow3mn = dc - da;
         ow3mx = dc + da;
         }}
      } /* End cylinder local scope */

   break;

/*--------------------------------------------------------------------*/

case SPHERE:                           /*** SPHERE ***/

   {  float dc;                  /* Proj of ctr on Lw */

      ps = &po->o.s;             /* Ptr to sphere data */

      /* Projections on coordinate axes */
      for (ia=0; ia<3; ia++) {
         oqmn[ia] = po->O[ia] + (qmn[ia] = -ps->Sr);
         oqmx[ia] = po->O[ia] + (qmx[ia] = ps->Sr);
         }
      /* Projections on (Lv1 x Lv2) x Lv1 axis */
      if (pl->nLv > 1) {
         w2mx = ps->Sr*pl->Llw2;
         w2mn = -w2mx;
      /* Projections on normals to Lv1, Lv2 lattice planes */
      if (pl->nLv > 2) {
         dc = dot33(po->O, pl->Lw3);
         ow3mn = dc - ps->Sr;
         ow3mx = dc + ps->Sr;
         }}
      } /* End sphere local scope */

   break;

/*--------------------------------------------------------------------*/

/* Handle other kinds of objects here */

/*--------------------------------------------------------------------*/

default:
   fatal("Other cases not yet implemented.",32);
   break;


      } /* End object type switch */

/* Store coordinates of image box faces augmented by object size.
*  N.B.  It is not an error that afmn is set from oqmx and vice-versa.
*  We need room for top half of an object below bottom face and for
*  bottom half of an object above the top face.  */

   for (ia=0; ia<3; ia++) {
      afmn[ia] = -oqmx[ia];
      afmx[ia] = pH->Face[ia] - oqmn[ia];
      }

/* Determine ranges for loops over lattice translations.  For the
*  third translation, this is done by calculating the intersections
*  of the eight corners of the image with the set of planes defined
*  by the first and second axes and taking the extremes, adjusted
*  for the size and position of the object on the lattice.  If the
*  third axis does not exist, this loop is trivialized.  */

   if (pl->nLv > 2) {
      bmn3 = BIG; bmx3 = -BIG;
      for (iv=0; iv<8; iv++) {
         float t = dot33(vim[iv], pl->Lw3);
         bmn3 = min(t,bmn3); bmx3 = max(t,bmx3);
         } /* End vertex loop */
      /* Adjust for object size and origin */
      lmn3 = (long)ceil(bmn3 - ow3mx);
      lmx3 = (long)floor(bmx3 - ow3mn);
      }
   else lmn3 = lmx3 = 0;

/* Loop over third lattice direction */

   for (l3=lmn3; l3<=lmx3; l3++) {

      u3 = (float)l3;

/* Determine range for second translation.  This is now a two-
*  dimensional problem in the plane defined by the current value
*  of l3*Lv3.  We must check the intersections of the current
*  lattice plane with each of the 12 edges of the image box,
*  keeping only those that are inside the box.  If the second
*  axis does not exist, this loop is trivialized.  */

      if (pl->nLv > 1) {
         bmn2 = BIG; bmx2 = -BIG;
         /* Check each of the three Cartesian axes in turn */
         for (ia=0; ia<3; ia++) {
            float d,te;          /* Denom, test edge */
            i2 = ip2[ia]; i3 = ip3[ia];
            d = pl->Lv[1][i2]*pl->Lv[0][i3] -
                pl->Lv[0][i2]*pl->Lv[1][i3];
            if (fabs(d) > 1.0E-12) for (iv=0; iv<4; iv++) {
               float e2 = ((iv&2) ? afmn[i2] : afmx[i2]) -
                  u3*pl->Lv[2][i2];
               float e3 = ((iv&1) ? afmn[i3] : afmx[i3]) -
                  u3*pl->Lv[2][i3];
               u2 = (e2*pl->Lv[0][i3] - e3*pl->Lv[0][i2])/d;
               u1 = (e3*pl->Lv[1][i2] - e2*pl->Lv[1][i3])/d;
               te = u1*pl->Lv[0][ia] + u2*pl->Lv[1][ia] +
                    u3*pl->Lv[2][ia];
               if (te < afmn[ia] || te > afmx[ia]) continue;
               bmn2 = min(u2,bmn2); bmx2 = max(u2,bmx2);
               } /* End corner loop */
            } /* End loop over three axial directions */

         /* Adjust for object size and origin */
         lmn2 = (long)ceil(bmn2 - w2mx);
         lmx2 = (long)floor(bmx2 - w2mn);
         }
      else lmn2 = lmx2 = 0;

/* Loop over second lattice direction */

      for (l2=lmn2; l2<=lmx2; l2++) {

         u2 = (float)l2;

/* Determine range for first lattice translation.  We now must
*  check all the intersections of the line defined by current
*  values of u2,u3 against the six bounding planes of the image.
*/
         if (pl->nLv > 0) {
            bmn1 = BIG; bmx1 = -BIG;
            /* Check each of the three Cartesian axes in turn */
            for (ia=0; ia<3; ia++) if (pl->Lv[0][ia]) {
               float t,t2,t3;
               i2 = ip2[ia]; i3 = ip3[ia];
               t = u2*pl->Lv[1][ia] + u3*pl->Lv[2][ia];
               for (iv=0; iv<2; iv++) {
                  u1 = ((iv ? afmn[ia] : afmx[ia]) - t)/pl->Lv[0][ia];
                  t2 = u1*pl->Lv[0][i2] +
                        u2*pl->Lv[1][i2] + u3*pl->Lv[2][i2];
                  if (t2 < afmn[i2] || t2 > afmx[i2]) continue;
                  t3 = u1*pl->Lv[0][i3] +
                        u2*pl->Lv[1][i3] + u3*pl->Lv[2][i3];
                  if (t3 < afmn[i3] || t3 > afmx[i3]) continue;
                  bmn1 = min(u1,bmn1); bmx1 = max(u1,bmx1);
                  } /* End face loop */
               } /* End loop over three axial directions */

            lmn1 = (long)ceil(bmn1);
            lmx1 = (long)floor(bmx1);
            }
         else lmn1 = lmx1 = 0;

/* Loop over first lattice direction */

         for (l1=lmn1; l1<=lmx1; l1++) {

/*---------------------------------------------------------------------*
*  Six levels of indenting suppressed                                  *
*---------------------------------------------------------------------*/

/* At this point we are sitting on a lattice point such that a copy
*  of the object placed on this point will have at least some part
*  inside the image box.  The task now is to locate all such points
*  and store the object density there.  */

   u1 = (float)l1;

   for (ia=0; ia<3; ia++)
      obj[ia] = po->O[ia] +
         u1*pl->Lv[0][ia] + u2*pl->Lv[1][ia] + u3*pl->Lv[2][ia];

#ifdef DEBUG
printf("Sitting on %8.4f %8.4f %8.4f\n",obj[0],obj[1],obj[2]);
#endif

/* Determine looping range in z for current object and loop */

   izmn = (long)ceil((obj[Z] + qmn[Z])/pH->Vox[Z]);
   if (izmn < 0) izmn = 0;
   else if (izmn >= pH->secs) continue;
   izmx = (long)floor((obj[Z] + qmx[Z])/pH->Vox[Z]);
   if (izmx < 0) continue;
   else if (izmx >= pH->secs) izmx = pH->secs - 1;

   z = pH->Vox[Z]*(float)izmn;
   piz = pi + izmn*secsize;

#ifdef DEBUG
printf("z looping range set to %d to %d\n",izmn,izmx);
#endif

   for (iz=izmn; iz<=izmx; iz++,z+=pH->Vox[Z],piz+=secsize) {

/* Determine looping range in y for current object and loop */

      zr = z - obj[Z];

#ifdef DEBUG
printf("   Sitting on z = %8.4f, zr = %8.4f\n",z,zr);
#endif

      bmn2 = BIG; bmx2 = -BIG;
      switch (po->kobj) {

      case BRICK:                   /*** BRICK ***/
         /* Check each of the three object axes in turn */
         for (ia=0; ia<3; ia++) {
            float dz,slope,s2,s3,ty,zm,z1,z2;
            if (dz = pb->Be[ia][Z]) {  /* Assignment intended */
               i2 = ip2[ia]; i3 = ip3[ia];
               slope = pb->Be[ia][Y]/dz;
               for (iv=0; iv<4; iv++) {
                  s2 = (iv&2) ? -0.5 : 0.5;
                  s3 = (iv&1) ? -0.5 : 0.5;
                  zm = s2*pb->Be[i2][Z] + s3*pb->Be[i3][Z] - zr;
                  z1 = zm - 0.5*pb->Be[ia][Z];
                  z2 = zm + 0.5*pb->Be[ia][Z];
                  if (z1*z2 >= 0) continue;
                  ty = s2*pb->Be[i2][Y] + s3*pb->Be[i3][Y] -
                      0.5*pb->Be[ia][Y] - slope*z1;
                  bmn2 = min(ty,bmn2); bmx2 = max(ty,bmx2);
                  } /* End corner loop */
               } /* End checking one edge vector */
            } /* End axis loop */
         break;

      case CYLINDER:                /*** CYLINDER ***/
         {  float dx = pc->Cn[X];
            float dy = pc->Cn[Y];
            float dz = pc->Cn[Z];
            float dxx = dx*dx;
            float tzy = zr*dy;
            float tzz = zr*dz;
            float dyz,trx;
            dyz = (dxx > 1.0) ? 0.0 : sqrt(1.0 - dxx);
            trx = pc->Cr*dyz;
            /* Check positive and negative faces of cylinder in turn.
            *  Note that if dx = 1.0 or dy = 1.0, then dz = 0.0, and
            *  we jigger txx,tyy to force end-plane intersection test.
            */
            for (iv=0; iv<2; iv++) {
               float a,b,c,d,tzh;
               float txx,ty,tyy;
               float s = (iv ? 1.0 : -1.0);
               if (fabs(dz) < 1.0E-4)
                  txx = tyy = BIG;
               else {
                  ty = (tzy + s*trx)/dz;
                  txx = dxx*((tyy = ty*dy) + tzz)/(1.0 - dxx);
                  }
               if (fabs(txx + tyy + tzz) <= pc->haxis) {
                  bmn2 = min(ty,bmn2); bmx2 = max(ty,bmx2);
                  }
               else {
               /* No intersection of plane with cylindrical surface.
               *  Test end plane instead.  */
                  a = 1.0 - dz*dz;
                  if (fabs(a) < 1.0E-4) continue;
                  tzh = tzz - s*pc->haxis;
                  b = dy*tzh;
                  c = tzh*tzh - dxx*(crrhh - zr*zr);
                  if ((d = b*b - a*c) < 0.0) continue;
                  d = sqrt(d);
                  ty = (-b + d)/a;
                  bmn2 = min(ty,bmn2); bmx2 = max(ty,bmx2);
                  ty = (-b - d)/a;
                  bmn2 = min(ty,bmn2); bmx2 = max(ty,bmx2);
                  } /* End end-plane test */
               } /* End loop over two faces */
            } /* End cylinder local scope */
         break;

      case SPHERE:                  /*** SPHERE ***/
         {  float ty = ps->Sr*ps->Sr - zr*zr;
            if (ty < 0.0) continue;
            ty = sqrt(ty);
            bmn2 = -ty; bmx2 = ty;
            } /* End sphere local scope */

         } /* End object type switch for y */

      iymn = (long)ceil((obj[Y] + bmn2)/pH->Vox[Y]);
      if (iymn < 0.0) iymn = 0;
      else if (iymn >= pH->rows) continue;
      iymx = (long)floor((obj[Y] + bmx2)/pH->Vox[Y]);
      if (iymx < 0) continue;
      else if (iymx >= pH->rows) iymx = pH->rows - 1;

      y = pH->Vox[Y]*(float)iymn;
      piy = piz + iymn*colsize;

#ifdef DEBUG
printf("   y looping range set to %d to %d\n",iymn,iymx);
#endif

      for (iy=iymn; iy<=iymx; iy++,y+=pH->Vox[Y],piy+=colsize) {

/* Determine looping range in x for current object and loop */

         yr = y - obj[Y];

#ifdef DEBUG
printf("      Sitting on y = %8.4f, yr = %8.4f\n",y,yr);
#endif

         bmn1 = BIG; bmx1 = -BIG;
         switch (po->kobj) {

         case BRICK:                /*** BRICK ***/
            /* Check each of the three object face pairs in turn */
            for (ia=0; ia<3; ia++) if (pb->Bw[ia][X]) {
               float tx,t2,t3;
               i2 = ip2[ia]; i3 = ip3[ia];
               for (iv=0; iv<2; iv++) {
                  tx = ((iv ? -0.5 : 0.5) - yr*pb->Bw[ia][Y] -
                        zr*pb->Bw[ia][Z])/pb->Bw[ia][X];
                  t2 = tx*pb->Bw[i2][X] + yr*pb->Bw[i2][Y] +
                        zr*pb->Bw[i2][Z];
                  if (t2 < -0.5 || t2 > 0.5) continue;
                  t3 = tx*pb->Bw[i3][X] + yr*pb->Bw[i3][Y] +
                        zr*pb->Bw[i3][Z];
                  if (t3 < -0.5 || t3 > 0.5) continue;
                  bmn1 = min(tx,bmn1); bmx1 = max(tx,bmx1);
                  }
               } /* End edge loop */
            break;

         case CYLINDER:             /*** CYLINDER ***/
            {  float tx;
               float dx = pc->Cn[X];
               float dy = pc->Cn[Y];
               float dz = pc->Cn[Z];
               float tyz = yr*dy + zr*dz;
               float tyyzz = yr*yr + zr*zr;
               float a = 1.0 - dx*dx;
               float b = dx*tyz;
               float d = tyz*tyz - a*(tyyzz - pc->Cr*pc->Cr);
               /* Intersect y,z line with cylindrical surface */
               if (d > 0.0) d = sqrt(d);
               for (iv=0; iv<2; iv++) {
                  float s = (iv ? 1.0 : -1.0);
                  int ksurf = 0;
                  if (fabs(a) < 1.0E-4 || d < 0) ksurf = 1;
                  else {
                     tx = (b + s*d)/a;
                     if (fabs(tx*dx + tyz) > pc->haxis) ksurf = 1;
                     }
                  if (ksurf) {
                     /* No solutions on round surface, test ends */
                     if (fabs(dx) < 1.0E-4) continue;
                     tx = (s*pc->haxis - tyz)/dx;
                     if (tx*tx + tyyzz > crrhh) continue;
                     }
                  bmn1 = min(tx,bmn1); bmx1 = max(tx,bmx1);
                  } /* End checking two ends of cylinder */
               } /* End cylinder local scope */
            break;

         case SPHERE:               /*** SPHERE ***/
            {  float tx = ps->Sr*ps->Sr - yr*yr - zr*zr;
               if (tx < 0.0) continue;
               tx = sqrt(tx);
               bmn1 = -tx; bmx1 = tx;
               } /* End sphere local scope */

            } /* End object type switch for x */

         ixmn = (long)ceil((obj[X] + bmn1)/pH->Vox[X]);
         if (ixmn < 0) ixmn = 0;
         else if (ixmn >= pH->cols) continue;
         ixmx = (long)floor((obj[X] + bmx1)/pH->Vox[X]);
         if (ixmx < 0) continue;
         else if (ixmx >= pH->cols) ixmx = pH->cols - 1;

         pix = piy + ixmn;

#ifdef DEBUG
printf("      x looping range set to %d to %d\n",ixmn,ixmx);
#endif

/* Insert object density at current lattice point.
*  N.B.  Currently defined objects have a uniform density, and
*  therefore the switch here is not strictly necessary.  It is
*  put here to allow for future implementation of objects that
*  have non-uniform density (density a function of coords).
*  To improve speed, the x loop is placed inside the switch.  */

         switch (po->kobj) {

         case BRICK:             /*** BRICK ***/
            irho = pb->rho;
            if (irho > 0) {
               for (ix=ixmn; ix<=ixmx; ix++,pix++) {
                  *pix = max(*pix,irho);
                  } /* End object x loop */
               }
            else {
               for (ix=ixmn; ix<=ixmx; ix++,pix++) {
                  int drho = *pix+irho;
                  *pix = max(drho,0);
                  } /* End object x loop */
               }
            break;

         case CYLINDER:          /*** CYLINDER ***/
            irho = pc->rho;
            if (irho > 0) {
               for (ix=ixmn; ix<=ixmx; ix++,pix++) {
                  *pix = max(*pix,irho);
                  } /* End object x loop */
               }
            else {
               for (ix=ixmn; ix<=ixmx; ix++,pix++) {
                  int drho = *pix+irho;
                  *pix = max(drho,0);
                  } /* End object x loop */
               }
            break;

         case SPHERE:            /*** SPHERE ***/
            irho = ps->rho;
            if (irho > 0) {
               for (ix=ixmn; ix<=ixmx; ix++,pix++) {
                  *pix = max(*pix,irho);
                  } /* End object x loop */
               }
            else {
               for (ix=ixmn; ix<=ixmx; ix++,pix++) {
                  int drho = *pix+irho;
                  *pix = max(drho,0);
                  } /* End object x loop */
               }
            break;

            } /* End object type switch for density */

         } /* End object y loop */

      } /* End object z loop */

/*---------------------------------------------------------------------*
*                        End Loop over objects                         *
*----------------------------------------------------------------------*
*                   Resume three indents suppressed                    *
*---------------------------------------------------------------------*/

            } /* End loop on third lattice vector */

         } /* End loop on second lattice vector */

      } /* End loop on first lattice vector */

/*---------------------------------------------------------------------*
*              End loop over three lattice translations                *
*----------------------------------------------------------------------*
*                       Resume normal indenting                        *
*---------------------------------------------------------------------*/

         } /* End object loop */

      } /* End lattice loop */

/* If noise has been called for, now add noise to the image.  This
*  is done after all geometrical components have been processed, so
*  that noise will not affect addition and subtraction of objects.  */

   if (pH->sigma) {
      long *pseed = &pH->nseed;
      short *pie = pi + imgsize;
#if 0  /* Computation of true Poisson noise */
      float xm = pH->sigma;
      for (pix=pi; pix<pie; pix++)
         *pix += (short)poidev(xm, pseed);
#else /* Approximate Poisson noise with normal for speed */
      long cmn = 65536.0*pH->sigma;
      long csg = 1048576.0*sqrt(pH->sigma);
      for (pix=pi; pix<pie; pix++) {
         register long npix = *pix + (ndev(pseed, cmn, csg)>>16);
         if (npix < 0) npix = 0;
         *pix = npix; }
#endif
      } /* End noise */

/* Create and write header records for the image.  Due to some
*  unexplained anomaly in 3dviewnix, the length fields here are
*  set to zero rather than the size of the actual data.  */

   GrpHd.group = GRP_SceneData;
   GrpHd.item = GSCDGroupLength;
   GrpHd.length = sizeof(long);
   if (write(nfd, &GrpHd, sizeof(HeaderItem)) < 0) return ERROR;
   if (write(nfd, &fakelength, sizeof(long)) < 0) return ERROR;

   GrpHd.item = GSCDImage;
   GrpHd.length = imgbytesize;
   GrpHd.length = 0;
   if (write(nfd, &GrpHd, sizeof(HeaderItem)) < 0) return ERROR;

/* Write out the full image */

   if (write(nfd, pi, imgbytesize) < 0) return ERROR;

   } /* End phwimg */

