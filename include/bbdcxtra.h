/* (c) Copyright 2007-2008, The Rockefeller University *11115* */
/* $Id: bbdcxtra.h 59 2017-01-13 20:34:16Z  $ */
/***********************************************************************
*                      BBD (Client Side) Package                       *
*                             bbdcxtra.h                               *
*                                                                      *
*  This file contains prototypes and definitions needed by BBD client  *
*  programs that wish to make use of components of the BBD library     *
*  that are specific to particular support libraries that may be       *
*  available to, and used by, some but not all kinds of clients.       *
*                                                                      *
*  Definitions in this file are compiled only if a suitable compile-   *
*  time parameter is provided from the makefile indicating the         *
*  library headers that are needed and available, as follows:          *
*     BBD_ALLEGRO       Compile routines that depend on the Allegro    *
*                       computer game programming library.             *
*     [Add others here as needed.]                                     *
************************************************************************
*  V1A, 08/23/07, GNR - New header file                                *
*  V1B, 01/08/08, GNR - Add return of obstacle distances and angles    *
*  V1C, 01/21/08, GNR - Use rectangle for opponent detection           *
*  V1D, 01/31/08, GNR - RFC_NONE, distances, not hits, for "squeeze"   *
*  V1D, 02/10/08, GNR - RNGROPTs as bits, add RNGROPT_NODA             *
*  ==>, 08/07/08, GNR - Last date before committing to svn repository  *
***********************************************************************/

#ifndef BBDCXTRA_INCLUDED
#define BBDCXTRA_INCLUDED 1

/*---------------------------------------------------------------------*
*                           Allegro section                            *
*---------------------------------------------------------------------*/

#ifdef BBD_ALLEGRO
#include "allegro.h"

#define OPP_HIT_COUNT 0x1000  /* Value added for opponent hits --
                              *  This value must be a single bit */
#define YAX_HAND    1         /* +/-1 if incr angle --> incr/decr y */

/* Structures used to define virtual range finders */

typedef struct HitList_type { /* Info returned by ranger finders */
   float  hitdist;            /* Closest dist to obstacle */
   float  angclr;             /* Angle to clear obstacle */
   int    nhits;              /* Number of pixels touched */
   } HitList;

typedef struct OppPos_type {  /* Define an opponent's position */
   double hw,hh;              /* Half width, height of opponent */
   double co,so;              /* Cos,sin of opponent orientation
                              *  (height clockwise from north) */
   double rx,ry;              /* Relative position of opponent with
                              *  y axis as defined by YAX_HAND  */
   } OppPos;

typedef struct BMOff_type {   /* Define bitmap coord systems */
   double xso,yso;            /* x,y offset in search bitmap */
   double xdo,ydo;            /* x,y offset in draw bitmap */
   } BMOff;

typedef struct Ranger_type {  /* Define one ranger */
   /* The following items are 'public' and must be filled in
   *  by the user before rngrinit is called  */
   float rfang;               /* Angle of sensor pair from
                              *  straight ahead (degrees) */
   float rfdis;               /* Distance of center of circle or
                              *  rectangle or outside edge of
                              *  trapezoid from platform position */
   /* N.B.  We don't use a union here because there is no syntax
   *  in C to permit initializing other than the first element.  */
   float rfsra;               /* Sensor radius if it's a circle,
                              *  axial size if it's a trapezoid,
                              *  height if it's a rectangle */
   float rfsrl;               /* Not used if sensor is a circle,
                              *  lateral size if a trapezoid,
                              *  width if it's a rectangle */
   int   minhits;             /* Minumum number of pixel hits to
                              *  count as a sensor hit */
   int   jrfshp;              /* Shape of receptive field */
#define RFS_CIRCLE   0           /* Field is a circle */
#define RFS_TRAPEZ   1           /* Field is a trapezoid */
#define RFS_RECTGL   2           /* Field is a rectangle */
   int   jrfcid;              /* Color identification code */
#define RFC_NONE  0              /* Do not check color */
#define RFC_GRAY  1              /* Check that color is gray */
   /* The following items are 'private' and should not be
   *  accessed by the user.  */
   float angsub;              /* Half-angle subtended by sensor
                              *  at center of platform */
   float angclr;              /* Angle platform must turn to
                              *  clear sensor if collision */
   float half;                /* Half-circle control */
   float mxd;                 /* Distance to farthest point */
   int   mysize;              /* Estimated size of sensor */
   } Ranger;

typedef struct RngrSet_type { /* Define a set of rangers */
   BITMAP *psBM;              /* Ptr to search bitmap */
   BITMAP *pdBM;              /* Ptr to drawing bitmap */
   struct Ranger_type *prng;  /* Ptr to list of sensors */
   struct Ranger_type **pprh; /* Ptr to list of ptrs to Rangers
                              *  in hit list order */
   BMOff  *pbmo;              /* Ptr to BMOff data */
   float  hwid;               /* Half-width of platform */
   float  MTC;                /* Turn clearance for RNGROPT_SPEC */
   int    nrng;               /* Size of prng array */
   int    nbmo;               /* Size of pbmo array */
   int    hnas;               /* Index of first neg-angle sensor */
   int    MinGray;            /* Minimum color value for RFC_GRAY */
   int    MaxGray;            /* Maximum color value for RFC_GRAY */
   int    MaxCRad2X3;         /* Threee times square of maximum
                              *  color radius for RFC_GRAY */
   } RngrSet;

#ifdef __cplusplus
extern "C" {
#endif
/* Virtual camera with azimuth rotation */
double *aztvinit(double hoang, double scale, int nta, int ntl);
void aztvcchk(BITMAP *pBM, BBDDev *pdev, double *azrf);
void aztvgeti(BITMAP *pBM, BBDDev *pdev, double *azrf,
   double o, double x, double y);
void aztvkill(double *azrf);
/* Virtual IR or laser range finder */
RngrSet *rngrinit(BITMAP *psBM, BITMAP *pdBM, Ranger *prng,
   BMOff *pbmo, int nrng, int nbmo, int MinGray, int MaxGray,
   int MaxCRad, float hwid, float MTC);
int  rngrget(RngrSet *prs, HitList *hitlist, OppPos *opprxy,
   double o, double x, double y, double mxdist,
   int ibmo, int kopt, int nopp);
#define RNGROPT_NORM     0    /* Normal, no draw */
#define RNGROPT_SPEC_FWD 1    /* Check forward sensors only */
#define RNGROPT_SPEC_BCK 2    /* Check rear sensors only */
#define RNGROPT_DRAW     4    /* Draw sensor RFs */
#define RNGROPT_NODA     8    /* No distance and angle calc */
#define RNGROPT_DHITS   16    /* Draw sensors with hits only */
void rngrkill(RngrSet *prs);
#ifdef __cplusplus
}
#endif

#endif   /* End BBD_ALLEGRO section */

/* Define operators for Y-axis calculations */
#if YAX_HAND > 0
#define YOP +
#define YOM -
#else
#define YOP -
#define YOM +
#endif

#endif
