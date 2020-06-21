/* (c) Copyright 2010-2011, The Rockefeller University *11113* */
/***********************************************************************
*                               mklijS1                                *
*                                                                      *
*  Program to prepare a CONNLIST file for the "Sentinel" demo.         *
*  This version prepares, for each of a given number of cells,         *
*  a set of l(ij) and c(ij) placed at random on the source             *
*  (assumed to be an image feature detector preprocessor output).      *
*  The connections for each cell are drawn at random from a set        *
*  of built-in templates, rotated to a random angle, adjusted for      *
*  the viewing elevation and possible random height of the stimu-      *
*  lus at that point, and then replicated for a given number of        *
*  subarbors with additional rotations.  C(ij) values will have        *
*  nbc == 4.                                                           *
*                                                                      *
*  This program is called from the command line as follows:            *
*                                                                      *
*  mklijS1 <output-file>                                               *
*                                                                      *
*  It expects to read the following control cards from stdin           *
*  (square brackets indicate optional items):                          *
*                                                                      *
*  TITLE (or other cryin-recognized cards)                             *
*                                                                      *
*  TARGET  ncells  nsubarb  ntmr                                       *
*     ncells   Number of cells in target celltype.                     *
*     nsubarb  Number of subarbors on each cell.  (The number          *
*              of connections in each subarbor is currently            *
*              fixed by the size of the largest precoded               *
*              connection template below.)                             *
*     ntmr     Number of template rotations.  The angle between        *
*              template rotations will be 360 deg./ntmr.  (This        *
*              number will usually be the same as nfd.)                *
*                                                                      *
*  SOURCE  nix  niy  nfd                                               *
*     nix,niy  Size of the source images.  (The center of each         *
*              generated arbor will be kept away from the edges        *
*              of the input images by half the largest template        *
*              dimension in order to avoid boundary calculations.)     *
*     nfd      Number of rotated feature detectors in the source       *
*              image set.  The angle between detectors will be         *
*              assumed to be 360 deg./nfd.                             *
*                                                                      *
*  OPTION  rfix                                                        *
*     krfix    Template rotation fixation.  Values are as follows:     *
*              0  Template rotation is selected at random and advanced *
*                 for each subarbor by same angular amount as feature  *
*                 detectors advance.  (This is the default)            *
*              1  Original operation.  Template rotation is selected   *
*                 at random and a source feature detector rotation is  *
*                 selected at random.  All subarbors are placed on     *
*                 this same source feature detector rotation.          *
*              2  Template rotation is matched to feature detector     *
*                 orientation for each subarbor (assumes 0 degree      *
*                 orientations are the same for the two sets).         *
*                                                                      *
*  ROTCTR  xrc  yrc  [sigrc]                                           *
*     xrc,yrc  Center of rotation of templates on successive sub-      *
*              arbors in LH coordinates relative to ULHC of source.    *
*              If this card is not entered, the center is set at       *
*              (nix-1)/2, (niy-1)/2.                                   *
*     sigrc    Standard deviation in center coordinates.  (Default: 0) *
*                                                                      *
*  ELEVATION  thlow  delth  nth  [zmax  zvar]                          *
*     thlow    Elevation above horizontal (theta, in deg.) of lowest   *
*              viewing elevation to be considered.  Elevation is       *
*              assumed to occur in yz plane.  (Default: 90.0 deg.)     *
*     delth    Angle (in deg) between possible viewing positions.      *
*     nth      Number of viewing angles in this run.  (Default: 1)     *
*     zmax     Highest z coordinate assumed at center of stimulus.     *
*              For purposes of calculating view angle, object is       *
*              treated as a pyramid with highest z at center, 0 at     *
*              edges, as mklij has no idea what the actual geometry    *
*              might be.  (Default: 0)                                 *
*     zvar     Fractional random variation in z calculated for the     *
*              assumed pyramidal object.  (Default: 0)                 *
*                                                                      *
*  TEMPLATE  ntx  nty                                                  *
*     ntx,nty  x,y size of template.  This card is followed by nty     *
*     cards, each with ntx 2 column fields.  Each field contains a     *
*     sign and a digit, 0 <= digit < 8.  The digit gives the value     *
*     of c(ij) in eighths.  A zero value can also be coded as a        *
*     blank.  Zero connections are not generated.  Rotations will      *
*     occur with the center of the ntx * nty rectangle as origin,      *
*     so generally the nonzero entries should touch the four edges.    *
*                                                                      *
*  SEED  iseed  [jseed]                                                *
*     iseed    Random number seed for generating connections.          *
*              (0 < iseed < 2^31).                                     *
*     jseed    Random number seed for rotation center and height ad-   *
*              justment (this exists so can compare tests with and     *
*              without these randomizations).  (Default: iseed + 1)    *
*                                                                      *
*  This program is assigned error codes in the range 550-557.          *
*                                                                      *
*  Design note:  All angles are considered as running counterclockwise *
*  from +x and the sines and cosines stored are the ones for a normal  *
*  right-handed coord system.  But the image and template coords are   *
*  in fact left-handed (y down), so signs are changed as needed to     *
*  allow for this.                                                     *
*                                                                      *
*  Design note:  I considered generating initial source locations only *
*  for pixels within a circle centered on the input image, so all      *
*  rotations would automatically also be within the image.  However,   *
*  this omits parts of the image in the corners that might contain     *
*  significant information, also with the complication that the nimux, *
*  nimuy box might not have the same center.  So I generate anywhere   *
*  in the image as before, and then if a rotation lies outside, just   *
*  move it to the nearest point inside along the line of the template  *
*  rotation, not the image rotation, which might at least catch part   *
*  of the same feature.                                                *
************************************************************************
*  V1A, 06/07/10, GNR                                                  *
*  V1B, 06/23/10, GNR - Also rotate subseq. subarbs. around img. ctr.  *
*  Rev, 06/25/10, GNR - Reverse sign of rotation of successive arbors  *
*  Rev, 07/01/10, GNR - Add option to move to rotated image segment    *
*  Rev, 07/06/10, GNR - Add ROTCTR card                                *
*  Rev, 07/15/10, GNR - Add ELEVATION card                             *
*  Rev, 07/28/10, GNR - Add debug plot. More uniform ifd,imx,imy asgn. *
*                       Remove useless COMPAT options.  Rotate both    *
*                       directions from center instead of all plus.    *
*                       Revise elevation correction.                   *
*  Rev, 07/30/10, GNR - Add OPTION krfix                               *
*  Rev, 07/31/10, GNR - Add sigrc                                      *
*  Rev, 10/12/11, GNR - Remove setmfd(), use setmf()                   *
***********************************************************************/

#define MAIN
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "rkarith.h"
#include "rfdef.h"
#include "itersph.h"
#include "swap.h"
#include "normtab.h"
#include "connlist.h"

/* Define DEBUG to turn on debug outputs:
*  1     Print template selections and locations.
*  2     Plot subarbor locations (requires 32-bit compilation)
*/
#define DEBUG      0

#if DEBUG & 2
#include "plots.h"
#define dpwd 10.0          /* Debug plot width (minus border) */
#define dpoff 0.5          /* Offset of plot rectange from edges */
#define ndpcols 8          /* Number available colors */
#endif

#define DTOR       0.017453292520   /* Degrees to radians */
#define TWOPI      6.283185308
#define PIDIV4     0.785398164
#define S8       256.0
#define S24 16777216       /* 2**24 */
#define sS4        4       /* Right shift S19 to S15 */
#define sS28      28       /* Left shift 1 to 1S28 */
#define DFLTCTR -10000.0   /* Flag to request default rotctr */

typedef struct CARSAR {    /* Sines, cosines for source rotation */
   double car,sar;
   } srcs;

typedef struct TMPLENT {   /* One entry in a (rotated) template */
   int cij;                   /* Value of c(ij) (S15) */
   int olij;                  /* Offset of l(ij) from center */
   } tple;

typedef struct TMPLROT {   /* Header for an array of TMPLENT */
   double cat,sat;            /* Cos,sin of template rotation */
   struct TMPLENT *pfte;      /* Ptr to first TMPLENT in array */
   int nte;                   /* Number of entries in array */
#if DEBUG & 2
   int dpmnxo,dpmnyo;
   int dpmxxo,dpmxyo;
#endif
   } tplr;

typedef struct TMPLDEF {   /* Structure for storing a template */
   struct TMPLDEF *pntp;      /* Ptr to next template */
   struct TMPLROT *ptro;      /* Ptr to list of rotations */
   int            *ptma;      /* Ptr to template array data */
   double         dia;        /* Diameter of template */
   int            ttx,tty;    /* Rectangular dimensions */
   } tpld;

/*---------------------------------------------------------------------*
*                                Main                                  *
*---------------------------------------------------------------------*/

int main(int argc, char *argv[]) {

   struct RFdef *pof;      /* Ptr to output file def */
   srcs  *pscs,*pscs0;     /* Ptrs to trig fns for source rots. */
   srcs  *pscth;           /* Ptr to trig fns for elevation */
   tple  *pte,*pte0;       /* Ptrs to array of rotated entries */
   tplr  *ptr,*ptr0;       /* Ptrs to array of rotation hdrs */
   tpld  *ptp;             /* Ptr to current template */
   tpld  *pftp,**ppntp;    /* Ptrs to first, next templates */
   tpld  **ppta,**ppta0;   /* Ptrs to template selection array */
   char  *tecl;            /* Temp ecl ptr for bemfmxx macros */
   struct ECLREC ecr;      /* External connection record */
   double drsubrot;        /* Subarbor rotation, radians */
   double sigrc;           /* Standard deviation in xrc,yrc */
   double thlow,delth;     /* ELEVATION theta, delta theta */
   double thx,thy;         /* Center of a box */
   double txlo,txhi;       /* Low,high x edges of template box */
   double tylo,tyhi;       /* Low,high y edges of template box */
   double xrc0,yrc0;       /* Mean values of xrc,yrc */
   double xrc,yrc;         /* Rotation center on source images */
   double zmax,zvar;       /* ELEVATION z max and z var */
#if DEBUG & 2
   float dpht;             /* Debug plot height */
   float dpscl;            /* Pixels to plot inches scale */
   float dpx,dpy;          /* Plotting coords */
   float dpsax[ndpcols],dpsay[ndpcols];   /* Subarbor coords */
#endif
   ui32  botr,topr;        /* Bottom,top of topographic range */
   ui32  icell,ncells;     /* Number of cells to generate */
   si32  iseed,jseed;      /* Random number seeds */
   si32  nrec;             /* Count of output records */

   int   item;             /* Selected template number */
   int   krfix;            /* Template rotation fixation option */
   int   mxnc;             /* Max nte for any template rotation */
   int   mnxo,mnyo;        /* Min x,y offsets in rotated templates */
   int   mxxo,mxyo;        /* Max x,y offsets in rotated templates */
   int   nix,niy,nixy;     /* Source image size */
   si32  nimux,nimuy;      /* Usable pixels along x,y */
   si32  nimu,nimuxy;      /* Number usable image pixels */
   int   nfd;              /* Number of source feature detectors */
   int   nfdptr;           /* Level 1 feat dets per template rotn */
   int   nsubarb;          /* Number subarbors per cell */
   int   nsa2;             /* nsubarb/2 */
   int   ntem;             /* Number of templates read */
   int   nth;              /* Number of elevations to select from */
   int   ntmr;             /* Number of template rotations */
   int   ntea;             /* Number TMPLENTs to allocate */
   int   smret;            /* String match return code */

   /* Define control card keys */
   static char *pcc[] =
      { "TARGET", "SOURCE", "OPTION", "ROTCTR", "ELEVATION",
        "TEMPLATE", "SEED", };

#if DEBUG & 2
   static char *dpcol[ndpcols] = { "X006", "RED", "ORANGE", "YELLOW",
      "X990", "XF80", "CYAN", "BLACK" };  /* Subarbor colors */
#endif

/* Define match code of each control card type */
#define TARGET_CARD     1
#define SOURCE_CARD     2
#define OPTION_CARD     3
#define ROTCTR_CARD     4
#define ELEV_CARD       5
#define TEMPLATE_CARD   6
#define SEED_CARD       7
#define NCC             7  /* Number of control cards */

/* Initialize for no cells, no templates, default seed, elev, rotctr */

   pte0 = NULL;
   ptp = pftp = NULL;
   ppntp = &pftp;
   thlow = 90.0, delth = 0.0;
   zmax = zvar = 0.0;
   ncells = 0;
   iseed  = 1009;
   jseed  = -1;   /* For default */
   krfix = 0;
   nix = niy = nfd = 0;
   nsubarb = ntmr =0;
   ntem = 0;
   nth = 0;
   sigrc = 0.0;
   xrc0 = yrc0 = DFLTCTR;

   RK.iexit = 0;
   settit("TITLE SENTINEL LAYER 2 CONNECTION GENERATION PROGRAM");
   setpid("mklijS1");

/* Pick up the file argument and allocate RFdef for it */

   if (argc < 2)
      abexitm(550, "No output file specified");
   if (argc > 2)
      abexitm(550, "Too many arguments: mklijS1 <output file>");
   pof = rfallo(argv[1], WRITE, BINARY, SEQUENTIAL, TOP, IGNORE,
      REWIND, RELEASE_BUFF, 8400, 12, IGNORE, ABORT);

/*---------------------------------------------------------------------*
*           Stage I:  Read control cards until end of stdin            *
*---------------------------------------------------------------------*/

   while (cryin()) {

      /* Match the card.  Exit if error (RK.iexit will be set) */
      if ((smret = smatch(0, RK.last, pcc, NCC)) <= 0) break;

      cdprnt(RK.last);        /* Print the card */

      /* Switch according to type of control card found */
      switch (smret) {

      case TARGET_CARD:
         inform("(SW1,UJ,V>I,V>I)", &ncells, &nsubarb, &ntmr, NULL);
         /* (inform guarantees ntmr > 0) */
         drsubrot = TWOPI/(double)ntmr;
         break;

      case SOURCE_CARD:
         inform("(SW1,V>I,V>I,V>I)", &nix, &niy, &nfd, NULL);
         break;

      case OPTION_CARD:
         inform("(SW1,V>=0<=2I)", &krfix, NULL);
         break;

      case ROTCTR_CARD:
         inform("(SW1,2*Q,NVQ)", &xrc0, &yrc0, &sigrc, NULL);
         sigrc /= S24;
         break;

      case ELEV_CARD:
         inform("(SW1,2*V>0<=90Q,V>I,N,2*Q)", &thlow, &delth,
            &nth, &zmax, &zvar, NULL);
         thlow *= DTOR, delth *= DTOR;
         zvar /= S24;
         break;

      case TEMPLATE_CARD: {
         int *prow,*prowe;
         int lxy;
         ptp = *ppntp = (tpld *)callocv(1, sizeof(tpld),
            "template hdr");
         ppntp = &ptp->pntp;
         inform("(SW1,V>I,V>I)", &ptp->ttx, &ptp->tty, NULL);
         lxy = ptp->ttx*ptp->tty;
         ptp->ptma = (int *)mallocv(lxy*sizeof(int), "template data");
         prowe = ptp->ptma + lxy;
         for (prow=ptp->ptma; prow<prowe; prow+=ptp->ttx)
            inform("(/W,RI2)", &ptp->ttx, prow, NULL);
         ntem += 1;
         } /* End template local scope */
         break;

      case SEED_CARD:
         inform("(SW1,N,V>IJ,V>IJ)", &iseed, &jseed, NULL);
         break;

         } /* End control card switch */

      } /* End of control cards */

/* Terminate if there were control card errors */

   if (ncells <= 0)
      cryout(RK_E1, "0***NEED NUMBER OF TARGET CELLS.", RK_LN2, NULL);

   if (nix == 0 || niy == 0 || nfd == 0)
      cryout(RK_E1, "0***BAD OR MISSING SOURCE CARD.", RK_LN2, NULL);

   if (ntem <= 0)
      cryout(RK_E1, "0***NO TEMPLATES FOUND.", RK_LN2, NULL);

   if (RK.iexit) {
      cryout(RK_P1,"0***Terminated due to input errors.",RK_LN2,NULL);
      abexit(551); }

/* Use default rotation centers if not entered */

   if (xrc0 == DFLTCTR) xrc0 = 0.5*(nix - 1);
   if (yrc0 == DFLTCTR) yrc0 = 0.5*(niy - 1);
   if (jseed < 0) jseed = iseed + 1;

/*---------------------------------------------------------------------*
*         Stage II:  Generate template records for all angles          *
*---------------------------------------------------------------------*/

/* Variables used to keep track of how far inside the original image
*  to place the template centers to avoid overhanging the edges.  */

   mnxo = mnyo =  INT_MAX;
   mxxo = mxyo = -INT_MAX;
   mxnc = 0;
   nsa2 = nsubarb/2;

/* Estimate max number of rotated entries from sum over templates
*  of area of each circle times number of rotation angles.  This
*  will be a very generous estimate, but the numbers will not be
*  very large in total.  */

   ntea = 0;
   for (ptp=pftp; ptp; ptp=ptp->pntp) {
      double d2 = (double)(ptp->ttx*ptp->ttx + ptp->tty*ptp->tty);
      ptp->dia = sqrt(d2);
      /* Add a little extra to be safe from truncation error */
      ntea += (int)ceil(PIDIV4*d2 + ptp->dia);
      }
   ntea *= ntmr;
   pte = pte0 = (tple *)mallocv(ntea*sizeof(tple),
      "rotated templates");
   ptr = ptr0 = (tplr *)mallocv(ntmr*ntem*sizeof(tplr),
      "rotated template index");

/* Loop over templates to make the rotated TMPLROTs and TMPLENTs.
*  Note:  I considered that GetNextPointInSphere() is fast enough
*  that there would be no advantage to running through the circle
*  once, storing and then playing back the ix,iy sequence on each
*  iteration of the template rotation loop.  -GNR  */

   for (ptp=pftp; ptp; ptp=ptp->pntp) {
      tplr *ptre;
      double ang = 0.0;
      double thc, thd = 0.5*ptp->dia;
      /* Local copies of ptp vars to save pointer derefs in loops */
      int  *pta = ptp->ptma;
      int  lttd = (int)ceil(ptp->dia);
      int  lttx = ptp->ttx;
      int  ltty = ptp->tty;
      thx = 0.5*(double)(ptp->ttx - 1);
      thy = 0.5*(double)(ptp->tty - 1);
      thc = 0.5*(double)(lttd - 1);
      ptp->ptro = ptr;
      ptre = ptr + ntmr;
      /* Loop over template rotations */
      for ( ; ptr<ptre; ++ptr,ang+=drsubrot) {
         IterSph ISph;
         ptr->cat = cos(ang);
         ptr->sat = sin(ang);
         ptr->pfte = pte;
         ptr->nte  = 0;
#if DEBUG & 2
         ptr->dpmnxo = ptr->dpmnyo =  INT_MAX;
         ptr->dpmxxo = ptr->dpmxyo = -INT_MAX;
#endif
         /* Loop over points in a circle containing template */
         InitSphereIndexing(&ISph, 1.0, 1.0, 1.0, thc, thc, 0.0,
            thd, lttd, lttd, 1);
         while (GetNextPointInSphere(&ISph)) {
            /* Get unrotated coords rel to center of template */
            double dux = (double)ISph.ix - thc;
            double duy = (double)ISph.iy - thc;
            /* Rotate by -ang to access unrotated template, then
            *  translate into input coordinates.  N.B.  Image
            *  coord system is left-handed (y down).  */
            double drx = ptr->cat*dux - ptr->sat*duy + thx;
            double dry = ptr->cat*duy + ptr->sat*dux + thy;
            /* Use these coordinates to interpolate c(ij)
            *  values from the stored unrotated template.
            *  (This code should give correct answer even if
            *  on an exact grid point without special tests.)  */
            double fdrx = floor(drx), cdrx = ceil(drx);
            double fdry = floor(dry), cdry = ceil(dry);
            int lox = (int)fdrx, hix = (int)cdrx;
            int loy = (int)fdry, hiy = (int)cdry;
            int loxf = (int)(S8*(1.0 + fdrx - drx));
            int hixf = (int)(S8*(drx - fdrx));
            int loxin = lox >= 0 && lox < lttx;
            int hixin = hix >= 0 && hix < lttx;
            int tcij = 0;
            if (loy >= 0 && loy < ltty) {
               int yrow = loy*lttx;
               int loyf = (int)(S8*(1.0 + fdry - dry));
               if (loxin) tcij += loxf*loyf*pta[lox + yrow];
               if (hixin) tcij += hixf*loyf*pta[hix + yrow];
               }
            if (hiy >= 0 && hiy < ltty) {
               int yrow = hiy*lttx;
               int hiyf = (int)(S8*(dry - fdry));
               if (loxin) tcij += loxf*hiyf*pta[lox + yrow];
               if (hixin) tcij += hixf*hiyf*pta[hix + yrow];
               }
            if (tcij) {    /* Store only nonzero c(ij) */
               int mydx = (int)floor(dux), mydy = (int)floor(duy);
               if (mydx < mnxo) mnxo = mydx;
               if (mydy < mnyo) mnyo = mydy;
               if (mydx > mxxo) mxxo = mydx;
               if (mydy > mxyo) mxyo = mydy;
#if DEBUG & 2
               if (mydx < ptr->dpmnxo) ptr->dpmnxo = mydx;
               if (mydy < ptr->dpmnyo) ptr->dpmnyo = mydy;
               if (mydx > ptr->dpmxxo) ptr->dpmxxo = mydx;
               if (mydy > ptr->dpmxyo) ptr->dpmxyo = mydy;
#endif
               pte->cij = SRA(tcij, sS4);
               pte->olij = mydx + nix*mydy;
               ptr->nte += 1;
               pte += 1;
               } /* End storing one sample point */
            } /* End loop over points in circle */
         if (ptr->nte > mxnc) mxnc = ptr->nte;
         } /* End loop over template rotations */
      } /* End loop over templates */

#if DEBUG & 1
   convrt("(P3,'0mnxo,mxxo = ',J0I6,',',J0I6,', mnyo,mxyo = ',"
      "J0I6,',',J0I6,'.')", &mnxo, &mxxo, &mnyo, &mxyo, NULL);
#endif

   /* Make an array of pointers to the templates to make it easier
   *  to pick one at random.  */
   ppta = ppta0 = (tpld **)mallocv(ntem*sizeof(tpld *),
      "template table");
   for (ptp=pftp; ptp; ptp=ptp->pntp) *ppta++ = ptp;

/*---------------------------------------------------------------------*
*                     Stage III:  Write the file                       *
*---------------------------------------------------------------------*/

/* Open the output file and abort if cannot */

   rfopen(pof, NULL, SAME, SAME, SAME, SAME, SAME, SAME, SAME,
      SAME, SAME, SAME, ABORT);

/* Prepare table of sines and cosines for rotating arbor centers
*  and for dealing with elevation.  */

   pscs0 = (srcs *)mallocv((nfd+nth)*sizeof(srcs), "sin/cos table");
   {  double dang = 0.0, dda = TWOPI/(double)nfd;
      int ia;
      for (pscs=pscs0,ia=0; ia < nfd; dang+=dda,++pscs,++ia) {
         pscs->car = cos(dang), pscs->sar = sin(dang);
         }
      } /* End pscs gen local scope */

   if (nth > 0) {
      double theta = thlow;
      int ith;
      pscth = pscs0 + nfd;
      for (ith=0; ith<nth; theta+=delth,++ith) {
         pscth[ith].car = cos(theta), pscth[ith].sar = sin(theta);
         }
      } /* End if nth > 0 */
   else
      pscth = NULL;     /* JIC */

/* Prepare constants for cell loop */

   botr = 0;
   nrec = 0;
   thx = 0.5*(nix - 1), thy = 0.5*(niy - 1);
   txlo = -thx - (double)mnxo, txhi = thx - (double)mxxo;
   tylo = -thy - (double)mnyo, tyhi = thy - (double)mxyo;
   nixy = nix*niy;
   nimux = nix + mnxo - mxxo;
   nimuy = niy + mnyo - mxyo;
   if (nimux <= 0 || nimuy <= 0)
      abexitm(552, "Template does not fit on image");
   nimuxy = nimux * nimuy;
   /* Number of input pixels we are willing to center on: */
   nimu = nfd * nimuxy;
   /* Load unchanging conntype number into output record */
   tecl = (char *)&ecr.jct;
   bemfmi2(tecl, 1);

/* Prepare for source offset with template rotation */
   if (nfd % ntmr)
      abexitm(555, "Target rotations do not map onto feat dets");
   nfdptr = nfd/ntmr;

#if DEBUG & 2
/* Initialize plot.  (Because xorg,yorg are not known to work in
*  smarx mfdraw, origin will be added to coords in plot calls.  */
   if (nsubarb > ndpcols) {
      cryout(RK_P1,"0***Terminated, more subarbors than colors.",
         RK_LN2,NULL);
      abexit(551); }
   dpscl = dpwd/(float)nix;
   dpht = dpscl*(float)niy;
   setmf(NULL, "", "mklijS1", NULL, 0, 0, 'B', 0, 0);
   newplt(2.0*dpoff+dpwd, 2.0*dpoff+dpht, 0, 0,
      "DEFAULT", "CYAN", "DEFAULT", SKP_META);
   rect(dpoff, dpoff, dpwd, dpht, 0);
   dpx = dpoff + dpscl*xrc0;
   dpy = dpoff + dpht - dpscl*yrc0;
   line(dpx, dpoff, dpx, dpoff+dpht);
   line(dpoff, dpy, dpoff+dpwd, dpy);
#endif

/* Loop over requested number of output cells */

   for (icell=0; icell<ncells; ++icell) {
      double dmx,dmy,dmz,drx,dry;   /* For pixel rotations */
      div_t detqr,pixqr;
      si32 ifd;               /* Selected feature detector */
      si32 imx,imy;           /* x,y of selected pixel on image */
      si32 ipix;              /* Selected pixel number */
      si32 irot,jrot;         /* Rotation selector,counter */
      int  ite;               /* tple counter */

      /* Load cell number into output record */
      tecl = (char *)&ecr.xicell;
      bemfmi4(tecl, icell);

      /* Select a template and a starting rotation at random */
      item = jm64sh(udev(&iseed), ntem, 1);
      ptp = ppta0[item];
      irot = jm64sh(udev(&iseed), ntmr, 1);

      /* Select center location on no-overlap part of image
      *  topographically, then correct to full-image offset.  */
      topr = dm64nb(icell+1, nimu, ncells, &jrot);
      ipix = botr + jm64sh(udev(&iseed), topr-botr, 1);
      detqr = div(ipix, nfd);
      ifd = (krfix  == 2) ? irot*nfdptr : detqr.rem;
      pixqr = div(detqr.quot, nimux);
      xrc = xrc0, yrc = yrc0;
      if (sigrc) {
         xrc += sigrc*(double)ndev(&jseed, 0, 1<<sS28);
         yrc += sigrc*(double)ndev(&jseed, 0, 1<<sS28);
         }
      imx = pixqr.rem - mnxo, dmx = (double)imx - xrc;
      imy = pixqr.quot - mnyo, dmy = (double)imy - yrc;

      /* If there is height adjustment, derive a random height
      *  now, as it should be the same for all the rotations.  */
      if (zmax || zvar) {
         double zm, admx = fabs(dmx), admy = fabs(dmy);
         zm = (admy*thx > admx*thy) ? admy/thy : admx/thx;
         dmz = zmax*(1.0 - zm);
         /* ndev result is on scale S24 with mean 0, sigma 1 */
         dmz += zvar*(double)ndev(&jseed, 0, 1<<sS28);
         if (dmz < 0.0) dmz = 0.0;
         }
      else
         dmz = 0.0;     /* Kill height adjustment below */

#if DEBUG & 1     /*** DEBUG ***/
      convrt("(P3,'0Cell ',J1UJ10,'uses template ',J1I6,"
         "'at rotation ',J1IJ6/' centered at (',J0IJ6,',',J0IJ6,"
         "') on  feature detector ',J0IJ6,'.')",
         &icell, &item, &irot, &imx, &imy, &ifd, NULL);
      convrt("(P3,' Relative to center this is (',J0Q10.3,',',"
         "J0Q10.3,')')", &dmx, &dmy, NULL);
#endif

/* Loop over subarbors */

      for (jrot=0; jrot<nsubarb; ++jrot) {

         int iout;            /* Bounds test loop index */
         int kout = 0;        /* Bounds test failure level */
         int krot;            /* Adjusted rotation */

         ptr = ptp->ptro + irot;
         /* Make center subarbor, not first, be at the
         *  selected location  */
         krot = jrot - nsa2;
         if (krot < 0) krot += nfd;
         pscs = pscs0 + krot;

         /* Rotate the selected center pixel to subarbor location
         *  (in LH coords where 0,0 is center of object rotation,
         *  then correct to coords where center of image is 0,0).  */
         if (nth > 0) {    /* If modifying for elevation, now do it */
            srcs *pecs = pscth + jm64sh(udev(&iseed), nth, 1);
            if (dmz) {     /* Version with height adjustment also */
               double dzc = dmz*pecs->car;
               double dmyz = dmy - dzc;
               drx = dmx*pscs->car + dmyz*pscs->sar/pecs->sar +
                  xrc - thx;
               dry = dmyz*pscs->car - dmx*pscs->sar*pecs->sar +
                  dzc + yrc - thy;
#if DEBUG & 1
         convrt("(P3,' After elev/hgt adjustment, center is at (',"
            "J0Q10.3,',',J0Q10.3,')')", &drx, &dry, NULL);
#endif
               } /* End elevation and height modification */
            else {         /* Elevation but no height */
               double esin = pecs->sar;
               drx = dmx*pscs->car + dmy*pscs->sar/esin + xrc - thx;
               dry = dmy*pscs->car - dmx*pscs->sar*esin + yrc - thy;
#if DEBUG & 1
         convrt("(P3,' After elevation adjustment, center is at (',"
            "J0Q10.3,',',J0Q10.3,')')", &drx, &dry, NULL);
#endif
               } /* End elevation, no height */
            } /* End elevation modification */
         else {            /* No elevation modification */
            drx = pscs->car * dmx + pscs->sar * dmy + xrc - thx;
            dry = pscs->car * dmy - pscs->sar * dmx + yrc - thy;
#if DEBUG & 1
         convrt("(P3,' After rotation ',J1IJ6, 'center is at (',"
            "J0Q10.3,',',J0Q10.3,')')", &jrot, &drx, &dry, NULL);
#endif
            } /* End no elevation modification */

/* If the new center point lies outside the acceptable part of the
*  image, first try moving it along the line corresponding to the
*  orientation of the selected feature detector until it is just
*  inside.  If that doesn't work, instead try moving it along the
*  line to the center of the image.  That has to work, so generate
*  an abexit 554 if a valid intersection is still not found.  (Use
*  a form of intersection test that avoids divide by zero.)  */

#define TORITEST  1           /* Template orientation bounds test */
#define SLOCTEST  2           /* Subarbor location bounds test */

         for (iout=TORITEST; iout<=SLOCTEST; ++iout) {
            double cam,sam;   /* Cos,sin of movement direction */
            double dycam,dxsam,txy1,txy2;    /* Temps */
            switch (iout) {
            case TORITEST:
               /* Change the sign of the sin to allow for
               *  left-handed image coord system.  */
               cam = ptr->cat, sam = -ptr->sat;
               break;
            case SLOCTEST: {
               /* Note:  if drx = dry = 0, TORITEST should pass
               *  and we never get here (unless it is some really
               *  bizarre situation), so abexit.  */
               double drr = sqrt(drx*drx + dry*dry);
               if (drr < 1E-12)
                  abexitm(554, "Rotn ctr outside template box");
               cam = drx/drr, sam = dry/drr;
               } /* End SLOCTEST local scope */
               break;
               } /* End iout switch */
            if ((dycam = dry - tylo) < 0.0) {      /* Low y check */
               kout |= iout;
               dycam *= cam;
               txy1 = sam * (drx - txlo) - dycam;
               txy2 = sam * (drx - txhi) - dycam;
               if (txy1 * txy2 <= 0.0) {
                  drx -= dycam/sam, dry = tylo;
                  break;
                  }
               }
            if ((dycam = dry - tyhi) > 0.0) {      /* High y check */
               kout |= iout;
               dycam *= cam;
               txy1 = sam * (drx - txlo) - dycam;
               txy2 = sam * (drx - txhi) - dycam;
               if (txy1 * txy2 <= 0.0) {
                  drx -= dycam/sam, dry = tyhi;
                  break;
                  }
               }
            if ((dxsam = drx - txlo) < 0.0) {      /* Low x check */
               kout |= iout;
               dxsam *= sam;
               txy1 = cam * (dry - tylo) - dxsam;
               txy2 = cam * (dry - tyhi) - dxsam;
               if (txy1 * txy2 <= 0.0) {
                  drx = txlo, dry -= dxsam/cam;
                  break;
                  }
               }
            if ((dxsam = drx - txhi) > 0.0) {      /* High x check */
               kout |= iout;
               dxsam *= sam;
               txy1 = cam * (dry - tylo) - dxsam;
               txy2 = cam * (dry - tyhi) - dxsam;
               if (txy1 * txy2 <= 0.0) {
                  drx = txhi, dry -= dxsam/cam;
                  break;
                  }
               }
            if (!kout) break;
            if (kout & SLOCTEST)
               abexitm(554, "Could not move template onto image");
            } /* End iout loop */

         /* Got a usable point, reduce to original image grid */
         imx = (si32)(drx + thx + 0.5);
         imy = (si32)(dry + thy + 0.5);

#if DEBUG & 1
         convrt("(P3,' After bounds correction by method ',J0I6,"
            "', this is at (',J0Q10.3,',',J0Q10.3,'), which is ('"
            "J0IJ6,',',J0IJ6,') on image')", &kout, &drx, &dry,
            &imx, &imy, NULL);
#endif

#if DEBUG & 2
         pencol(dpcol[jrot]);
         dpx = dpscl*(float)imx;
         dpy = dpscl*(float)imy;
         rect(dpoff+dpx+dpscl*(float)ptr->dpmnxo,
            dpoff+dpht-dpy+dpscl*(float)ptr->dpmnyo,
            dpscl*(float)(ptr->dpmxxo - ptr->dpmnxo),
            dpscl*(float)(ptr->dpmxyo - ptr->dpmnyo), 0);
         /* Save coord to draw lines later */
         dpsax[jrot] = dpoff + dpx;
         dpsay[jrot] = dpoff + dpht - dpy;
#endif

         ipix = nix*imy + imx;
         if (ipix < 0 || ipix >= nixy)
            abexitm(553, "Template located outside image");
         ipix += ifd * nixy;

         /* Loop over tples associated with this rotation */
         pte = ptr->pfte;
         for (ite=0; ite<ptr->nte; ++pte,++ite) {
            tecl = (char *)&ecr.ecij;
            bemfmi2(tecl, pte->cij);
            tecl = (char *)&ecr.xjcell;
            bemfmi4(tecl, ipix + pte->olij);
            rfwrite(pof, &ecr, sizeof(ecr), ABORT);
            } /* End tple loop */
         /* If fewer then mxnc tples, write skip records */
         if (ite < mxnc) {
            ecr.xjcell = -1;
            ecr.ecij = 0;
            for ( ; ite<mxnc; ++ite)
               rfwrite(pof, &ecr, sizeof(ecr), ABORT);
            }
         nrec += mxnc;
         if (krfix != 1) {
            irot += 1;
            if (irot >= ntmr) irot = 0; }
         ifd += nfdptr;
         if (ifd >= nfd) ifd -= nfd;
         } /* End rotation loop */

#if DEBUG & 2
      /* Draw lines between subarbors */
      pencol("RED");
      for (jrot=1; jrot<nsubarb; ++jrot)
         line(dpsax[jrot-1],dpsay[jrot-1],dpsax[jrot],dpsay[jrot]);
#endif
      botr = topr;            /* For next time */
      } /* End cell loop */

/* Finish up */

   convrt("(P1,'0mklij completed, ',J1IJ10,'records written.')",
      &nrec, NULL);
   convrt("(P1,' nc of CNS CONNTYPE should be ',J1I10,'per subarb.')",
      &mxnc, NULL);

   if (pte0) freev(pte0, "rotated templates");
   if (ptr0) freev(ptr0, "rotated template index");
   if (ppta0) freev(ppta0, "template table");
   ptp = pftp;
   while (ptp) {
      tpld *qntp = ptp->pntp;
      if (ptp->ptma) freev(ptp->ptma, "template data");
      freev(ptp, "template hdr");
      ptp = qntp;
      }

#if DEBUG & 2
   endplt();
#endif

   rfclose(pof, SAME, SAME, ABORT);
   cryout(RK_P1, "\0", RK_LN0+RK_FLUSH+1, NULL);
   return 0;
   } /* End mklijS1 */
