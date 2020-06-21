/* (c) Copyright 2011, The Rockefeller University *11114* */
/* $Id: iterrect.h 59 2017-01-13 20:34:16Z  $ */
/***********************************************************************
*                             iterrect.h                               *
*                                                                      *
*  This header file declares a pair of routines that may be used to    *
*  find successively all the points on some rectangular lattice that   *
*  fall within a given rectangle, working in a spiral from the center  *
*  to the ULHC.                                                        *
*                                                                      *
*  The user must provide an instance of the IterRect data structure    *
*  declared here for each invocation of the routines.                  *
*                                                                      *
*  V1A, 08/11/11, GNR - Initial version                                *
*  ==>, 08/11/11, GNR - Last date before committing to svn repository  *
***********************************************************************/

#ifndef ITERRECT_HDR_INCLUDED
#define ITERRECT_HDR_INCLUDED

#include "sysdef.h"

/* States of the scan generator */
enum rscgst { SSG_Right, SSG_Down, SSG_Left, SSG_Up,
   SSG_Next, SSG_Done };

/* Structure used by rectangle indexing routine to save its state and
*  to return results.  All coordinates are integer grid coordinates
*  starting from 0 in the ULHC.  */
typedef struct IterRect_t {
   /* Variables used to return results to caller */
   long ioff;              /* Returns offset to next grid point */
   long ix,iy;             /* Returns coords of current grid point */
   /* Internal variables, not for user access */
   long nsx,nsy;           /* Size of full lattice along x,y */
   long ix0,iy0;           /* Coords of ULHC of desired rectangle */
   long ix1,iy1;           /* Coords of ULHC of current scan */
   long ix2,iy2;           /* Coords of LRHC of current scan */
   enum rscgst js;         /* State of scan generator: */
   } IterRect;

#ifdef __cplusplus
extern "C" {
#endif
/* Call to initialize rectange indexing */
void InitRectangleIndexing(IterRect *Rect, long nsx, long nsy,
   long ix0, long iy0, long nrx, long nry);

/* Call to obtain next point in rectangle */
int GetNextPointInRectangle(IterRect *Rect);
#ifdef __cplusplus
}
#endif

#endif /* ifndef ITERRECT_HDR_INCLUDED */
