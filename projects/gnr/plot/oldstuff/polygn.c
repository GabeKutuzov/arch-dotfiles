/* (c) Copyright 1998, Neurosciences Research Foundation, Inc. */
/***********************************************************************
*                              polygn()                                *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void polygn(float xc, float yc, float radius, int nv,            *
*        float angle, float dent, float spike, int kf)                 *
*                                                                      *
*  DESCRIPTION:                                                        *
*     This subroutine draws a regular closed polygon (or circle) in    *
*     the current color.  Each edge of the polygon may be a straight   *
*     line or a "dented" line consisting of two equal half-segments    *
*     with their center displaced inward or outward to make a "star".  *
*     In addition, each vertex may be decorated with a "spike" con-    *
*     sisting of a line segment pointing some distance radially away   *
*     from the vertex.  These options permit a variety of symmetric    *
*     marker symbols to be produced.                                   *
*                                                                      *
*  ARGUMENTS:                                                          *
*     xc,yc   Coordinates of the center of the polygon.                *
*     radius  Radius (distance from the center to each vertex)         *
*             of the polygon.                                          *
*     nv      Number of vertices in the polygon.  A value of 0 draws   *
*             a circle, a value of 2 draws a line, 3 a triangle, etc.  *
*             Negative values and 1 are invalid and cause termination  *
*             of the calling program.                                  *
*     angle   Angle through which the polygon is rotated, measured     *
*             in degrees counterclockwise from a position in which     *
*             one vertex is positioned along the positive X axis.      *
*     dent    Factor by which radius should be multiplied to obtain    *
*             the distance from the vertex to the midpoint of each     *
*             edge of the polygon.  A zero value indicates that the    *
*             edges should be straight lines.  A value of 1.0          *
*             effectively causes a polygon with twice the number of    *
*             vertices to be drawn, but with spikes only on every      *
*             other vertex.                                            *
*     spike   Factor by which radius should be multiplied to obtain    *
*             the radius of the end of a spike line drawn radially     *
*             outward (inward if spike < 1.0) from each vertex of      *
*             the polygon.  A zero value indicates that no spikes      *
*             should be drawn.                                         *
*     kf      Fill control switch:  If 'krt' = -1 (FILLED), a          *
*             filled polygon is drawn, otherwise an open one.          *
*                                                                      *
*  CURRENT PLOT POSITION ON RETURN:                                    *
*     (xc,yc)                                                          *
*                                                                      *
*  RETURN VALUE:                                                       *
*     None.                                                            *
*                                                                      *
*  V2A, 12/31/98, GNR - New routine, make binary metafile              *
***********************************************************************/

#include "mfint.h"
#include "plots.h"

#define PGF    0     /* gks fill offset */
#define PGD    1     /* gks dent offset */
#define PGS    2     /* gks spike offset */
#define PGN    3     /* gks nvertex offset */

void polygn(float xc, float yc, float radius, int nv,
   float angle, float dent, float spike, int kf) {

   static char MCpolygn[] = { OpStar, Tkn, PGF, 2, Txc, NEW,
      Tyc, NEW, Trad, Tg, PGD, Tg, PGS, Tk, PGN, Tang, Tend };

   if (_RKG.s.MFmode) {
      _RKG.gks[PGF] = (kf == FILLED);
      _RKG.x[NEW] = (long)fixxy(xc);
      _RKG.y[NEW] = (long)fixxy(yc);
      _RKG.r[NEW] = (long)fixxy(radius);
      _RKG.gks[PGD] = (long)fixg(dent);
      _RKG.gks[PGS] = (long)fixg(spike);
      _RKG.gks[PGN] = nv;
      _RKG.aa     = (long)fixaa(angle);
      CMDPACK(MCpolygn, _RKG.s.mxlStar);
      } /* End if MFmode */

   } /* End polygn() */
$$$ CLEAR InReset BIT


