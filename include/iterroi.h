/* (c) Copyright 2013, The Rockefeller University *11114* */
/* $Id: iterroi.h 59 2017-01-13 20:34:16Z  $ */
/***********************************************************************
*                              iterroi.h                               *
*                                                                      *
*  This header file declares a pair of routines that may be used to    *
*  find successively all the points on some rectangular lattice that   *
*  fall within a given rectangle, working in a spiral from the ULHC    *
*  in to the center (use iterrect pkg to move out from the center).    *
*                                                                      *
*  The user must provide an instance of the IterRoi data structure     *
*  declared here for each invocation of the routines.                  *
*                                                                      *
*  V1A, 02/02/13, GNR - New routine, based on iterrect pkg             *
*  ==>, 02/02/13, GNR - Last date before committing to svn repository  *
***********************************************************************/

#ifndef ITERROI_HDR_INCLUDED
#define ITERROI_HDR_INCLUDED

#include "sysdef.h"

/* States of the scan generator */
enum roigst { RSG_Right, RSG_Down, RSG_Left, RSG_Up, RSG_Done };

/* Structure used by rectangle indexing routine to save its state and
*  to return results.  All coordinates are integer grid coordinates
*  starting from 0 in the ULHC.  */
typedef struct IterRoi_t {
   /* Variables used to return results to caller */
   long ioff;              /* Returns offset to next grid point */
   long ix,iy;             /* Returns coords of current grid point */
   /* Internal variables, not for user access */
   long nsx,nsy;           /* Size of full lattice along x,y */
   long ix1,iy1;           /* Coords of ULHC of current scan */
   long ix2,iy2;           /* Coords of LRHC of current scan */
   enum roigst js;         /* State of scan generator: */
   } IterRoi;

#ifdef __cplusplus
extern "C" {
#endif
/* Call to initialize rectange indexing */
void InitOIRectIndexing(IterRoi *Rect, long nsx, long nsy,
   long ix0, long iy0, long nrx, long nry);

/* Call to obtain next point in outside-in rectangle */
int GetNextPointInOIRect(IterRoi *Rect);
#ifdef __cplusplus
}
#endif

#endif /* ifndef ITERROI_HDR_INCLUDED */
