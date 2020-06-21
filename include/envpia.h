/* (c) Copyright 1992, The Rockefeller University *11115* */
/* $Id: envpia.h 59 2017-01-13 20:34:16Z  $ */
/***********************************************************************
*                         Environment Program                          *
*                         ENVPIA Header File                           *
*                                                                      *
*  This file contains the boundary condition check switch              *
*  It is included in envpia.c in four places.                          *
*                                                                      *
*  The variable 'bnd_chk' selects the kind of boundary check to        *
*  perform.  The rotated pixel is either added into or used to         *
*  replace the destination (depending on whether or not the            *
*  destination is part of the object already).                         *
*                                                                      *
*  V1A, 01/27/89, GNR - Initial version                                *
*  ==>, 07/01/02, GNR - Last date before committing to svn repository  *
***********************************************************************/

   if (pixel == 0) goto BND_LABEL1;
   switch (bnd_chk) {

/* Add/store pixel, zero boundaries */

case BND_ZERO:
      if (ixcoord < lpxmin || ixcoord > lpxmax ||
         iycoord < lpymin || iycoord > lpymax) goto BND_LABEL1;
      break;

/* Add/store pixel, limited toroidal bounds.
*  ixcoord and iycoord may be toroidally adjusted.  */

case BND_LTB:
      ixcoord = (ixcoord+lpxfix)%xwinsiz + lpxmin;
      iycoord = (iycoord+lpyfix)%ywinsiz + lpymin;
      break;

/* Add/store pixel, full toroidal bounds.
*  ixcoord and iycoord may be adjusted.  */

case BND_FTB:
      ixcoord &= llxsub1;
      iycoord &= llysub1;
      break;
      }  /* End of 'bnd_chk' switch */

/* Common code for all three boundary conditions */

   rot_pxl = inpary + iycoord + (ixcoord<<kx1);  /* Locate rotated
                                                    pixel. */
   /* See if this point is already marked by this object */
   if (*(rot_pxl + lly) == lref) {
      /* Yes, add in contribution */
      (*rot_pxl) += (byte)pixel;
      }
   else {
      /* No, just replace what was there. */
      *(rot_pxl + lly) = lref;    /* Put in reference number */
      *rot_pxl = (byte)pixel;     /* Put in pixel value      */
      }

BND_LABEL1:       /* Jump to here when out of window */
