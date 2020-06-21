/* (c) Copyright 1998, Neurosciences Research Foundation, Inc. */
/***********************************************************************
*                               arc2()                                 *
*                                                                      *
*  SYNOPSIS:                                                           *
*     int arc2(float xc, float yc, float xs, float ys, float angle)    *
*                                                                      *
*  DESCRIPTION:                                                        *
*     This function draws an arc in the current drawing color and,     *
*     if supported, the current thickness.  Same as arc() except       *
*     current plotting position is left at end of arc on return.       *
*                                                                      *
*  ARGUMENTS:                                                          *
*     xc,yc   Corrdinates of the center of the arc.                    *
*     xs,ys   Coordinates of the starting point of the arc.            *
*     angle   The angular size of the arc in degrees.                  *
*             If 'angle' > 0, the arc is drawn counterclockwise        *
*             from (xs,ys), otherwise clockwise.                       *
*                                                                      *
*  CURRENT PLOT POSITION ON RETURN:                                    *
*     End of arc                                                       *
*                                                                      *
*  RETURN VALUE:                                                       *
*     None                                                             *
*                                                                      *
*  V2A, 12/29/98, GNR - New routine, make binary metafile              *
***********************************************************************/

#include "mfint.h"
#include "plots.h"

void arc2(float xc, float yc, float xs, float ys, float angle) {

   if (_RKG.s.MFmode) {
      double ra;
      float  ca,sa;

      /* Draw the arc */
      arc(xc, yc, xs, ys, angle);

      /* Place current coordinates at end of arc */
      ra = TORADS*angle;
      ca = cos(ra); sa = sin(ra);
      xs -= xc; ys -= yc;
      xc += (xs*ca - ys*sa);
      yc += (xs*sa + ys*ca);
      plot(xc, yc, PENUP);
      } /* End if MFmode */

$$$ CLEAR InReset BIT

   } /* End arc2() */
