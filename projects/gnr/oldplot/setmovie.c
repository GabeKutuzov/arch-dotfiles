/* (c) Copyright 1992-2011, The Rockefeller University *21114* */
/* $Id: setmovie.c 30 2017-01-16 19:30:14Z  $ */
/***********************************************************************
*                             setmovie()                               *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void setmovie(int mmode, int device, int nexpose)                *
*                                                                      *
*  DESCRIPTION:                                                        *
*     Set the mode of operation, movie output device and the number    *
*     of exposure to be made each frame.                               *
*  Parameters:                                                         *
*     mmode: Movie mode change switch--                                *
*              =0 (MM_NOCHNG) do not change mode                       *
*              =1 (MM_STILL)  change to still mode                     *
*              =2 (MM_MOVIE)  change to movie mode                     *
*              =3 (MM_BATCH)  change to batch mode                     *
*     device:  =0 use Matrix printer, = 1 use beeper                   *
*     nexpose: number of times to expose each movie frame,             *
*              =0 do not expose movie                                  *
*                                                                      *
*  RETURN VALUES:                                                      *
*     None                                                             *
************************************************************************
*  Rev, 01/20/92, ROZ                                                  *
*  Rev, 11/25/96, GNR - Delete native NCUBE plotting support           *
*  ==>, 01/25/07, GNR - Last mod before committing to svn repository   *
*  Rev, 10/11/11, GNR - _NCG to _RKG, gmf.h now in glu.h, etc.         *
***********************************************************************/

/* This routine is used to transfer movie parameters from the
   application into the _RKG structure for control of exposures in
   subsequent bpaus calls.  If the Matrix device is to be used, open
   the file pathway now.  */

#include "plots.h"
#include "glu.h"

void setmovie(int mmode, int device, int nexpose) {

   if (mmode) _RKG.movie_mode = mmode;
   _RKG.movie_device = device;
   _RKG.nexp = nexpose;

   } /* End setmovie */
