/* (c) Copyright 2002, Neurosciences Research Foundation, Inc. */
/***********************************************************************
*                               numbc()                                *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void numbc(float ht, double val, float angle, int ndec)          *
*                                                                      *
*  DESCRIPTION:                                                        *
*     This function converts a floating point number to a decimal      *
*        string and plots the result.  If ht >= 0, plots the text      *
*        at the current plotting position.  If ht < 0, concatenates    *
*        the text to the text plotted by the most recent symbol(),     *
*        symbc(), number(), or numbc() call.                           *
*                                                                      *
*  ARGUMENTS:                                                          *
*     ht      Height of the numbers, in inches.  If negative or        *
*             zero, use height and angle from previous call.           *
*     val     The floating point number to be converted.               *
*     angle   Angle of the drawn string.                               *
*     ndec    The number of places to be plotted after the decimal.    *
*             If ndec = 0, the integer portion of the number is        *
*             plotted with a decimal point, if ndec = -1, it is        *
*             plotted without a decimal, if ndec < -1, the number      *
*             is scaled by 10**(ndec+1) and rounded before plotting.   *
*             As a purely practical matter, ndec is restricted to      *
*             the range -16 <= ndec < 15.                              *
*                                                                      *
*  CURRENT PLOT POSITION ON RETURN:                                    *
*     Undefined, but stored in the rendering program such that a       *
*     following symbol(), symbc(), number(), or numbc() call with      *
*     ht < 0 will concatenate the strings properly.  When using        *
*     concatenation in a parallel computer, either mfbegatm() and      *
*     mfendatm() calls should be used to assure the text is pro-       *
*     cessed atomically, or all symbol(), symbc(), number(), and       *
*     numbc() calls should be made from the same processor node.       *
*                                                                      *
*  RETURN VALUE:                                                       *
*     None.                                                            *
*                                                                      *
*  V2A, 05/26/02, GNR - New routine, based on number()                 *
***********************************************************************/

#include "plots.h"
#include "mfint.h"      /* Needed to define RKD macro */
#include "rocks.h"
#include "rkxtra.h"

extern double tens[];   /* Scale factors exported by bcdout */

void numbc(float ht, double val, float angle, int ndec) {

   if (_RKG.s.MFmode) {
      int  nd = ndec + 1;
      char printme[MAXSTRING];

      if (nd >= 0) {
         if (nd > 15) abexit(288);
         bcdout(RKD(ndec,MAXSTRING), printme, val);
         }
      else {
         nd = -nd;
         if (nd > 15) abexit(288);
         bcdout(RKD(0,MAXSTRING), printme, val/tens[nd]);
         }

      symbc(ht, printme, angle, RK.length+1);
      } /* End if MFmode */

$$$ CLEAR InReset BIT

   } /* End numbc() */
