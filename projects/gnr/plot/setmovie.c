/* (c) Copyright 1992-2015, The Rockefeller University *11115* */
/* $Id: setmovie.c 5 2008-03-13 20:32:23Z  $ */
/***********************************************************************
*                     ROCKS metafile plot library                      *
*                             setmovie()                               *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void setmovie(int mmode, int device, int nexpose)                *
*                                                                      *
*  DESCRIPTION:                                                        *
*     Set the mode of operation, movie output device, and the number   *
*     of exposures to be made of each frame (to generate slow-motion   *
*     movies).  setmovie() is used to transfer movie parameters from   *
*     the application into the _RKG structure for control of exposures *
*     in subsequent newplt()/finplt()/endplt() calls.                  *
*                                                                      *
*     If called before the first newplt(0) or newwin(), setmovie()     *
*     sets global defaults for these parameters, otherwise, it         *
*     affects the current drawing window.                              *
*                                                                      *
*  PARAMETERS:                                                         *
*     mmode: Movie mode change switch--                                *
*              =0 (MM_NOCHNG) do not change mode                       *
*              =1 (MM_STILL)  change to still mode                     *
*              =2 (MM_MOVIE)  change to movie mode                     *
*              =3 (MM_BATCH)  change to batch mode.                    *
*     device:  =0 Use Matrix printer, = 1 use beeper.  Other devices   *
*              may be defined locally.  (Currently ignored).           *
*     nexpose: Number of times to expose each movie frame,             *
*              =0 do not expose movie                                  *
*                                                                      *
*  RETURN VALUES:                                                      *
*     None                                                             *
*                                                                      *
*  PARALLEL COMPUTATION:                                               *
*     On parallel computers, setmf() only needs to be called from      *
*     node 0.  If called on other nodes, no error occurs, but the      *
*     call is ignored.                                                 *
*                                                                      *
************************************************************************
*  Rev, 01/20/92, ROZ                                                  *
*  Rev, 11/25/96, GNR - Delete native NCUBE plotting support           *
*  ==>, 07/25/15, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include "plots.h"
#include "mfint.h"

void setmovie(int mmode, int device, int nexpose) {

   if (_RKG.MFFlags & MFF_DidIfin) {
      Win *pw = _RKG.pcw;
      if (mmode) pw->movie_mode = (byte)mmode;
      pw->movie_device = (byte)device;
      pw->nexpose = (ui16)nexpose;
      }
   else {
      if (mmode) _RKG.movie_mode = (byte)mmode;
      _RKG.movie_device = (byte)device;
      _RKG.nexpose = (ui16)nexpose;
      }

   } /* End setmovie() */
