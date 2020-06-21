/* (c) Copyright 2012, The Rockefeller University *11114* */
/* $Id: itertape.h 59 2017-01-13 20:34:16Z  $ */
/***********************************************************************
*                             itertape.h                               *
*                                                                      *
*  This header file declares a triple of routines that may be used to  *
*  find successively all the points in a series of boxes that lie on a *
*  rectangular tape placed on some rectangular lattice.                *
*                                                                      *
*  The user must provide an instance of the IterTape data structure    *
*  declared here for each invocation of the routines.                  *
*                                                                      *
*  V1A, 02/17/12, GNR - Initial version                                *
*  ==>, 02/21/12, GNR - Last date before committing to svn repository  *
***********************************************************************/

#ifndef ITERTAPE_HDR_INCLUDED
#define ITERTAPE_HDR_INCLUDED

#include "sysdef.h"

#define  NLENDS    2       /* Number of ends on a line segment */
#define  NVQ       4       /* Number of vertices of a quad box */

/* Structure used by tape indexing routine to save its state and
*  to return results.  All coordinates are integer grid coordinates
*  starting from 0 in the ULHC.  */
typedef struct IterTape_t {
   /* Variables used to return results to caller */
   long  ioff;             /* Returns offset to next grid point */
   long  ix,iy;            /* Returns coords of current grid point */
   /* Internal variables, not for user access */
   xyf   bxy[NVQ];         /* Four corners of a box */
   float bcos,bsin;        /* cos,sin of base edge (LH coords) */
   float slope[NVQ/2];     /* Base, edge slopes */
   float topx;             /* Largest legal x coord */
   float ybcos;            /* Current (y - y2) times bcos */
   float lox[NVQ],hix[NVQ];/* Low,high x by edge */
   long  loy[NVQ],hiy[NVQ];/* Low,high y by edge */
   long  nsx,nsy;          /* Size of full lattice along x,y */
   long  xmx;              /* Highest x at current y */
   long  ymn,ymx;          /* Starting,ending y coords */
   long  yoff;             /* Offset in lattice of current y row */
   } IterTape;

#ifdef __cplusplus
extern "C" {
#endif
/* Call to initialize tape indexing */
void InitTapeIndexing(IterTape *Tape, xyf *pb1, xyf *pb2,
   float hgt, long nsx, long nsy);
/* Call to extend tape */
int ExtendTapeIndexing(IterTape *Tape, float hgt);
/* Call to obtain next point on tape */
int GetNextPointOnTape(IterTape *Tape);
#ifdef __cplusplus
}
#endif

#endif /* ifndef ITERTAPE_HDR_INCLUDED */
