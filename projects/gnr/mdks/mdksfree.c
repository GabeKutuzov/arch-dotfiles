/* (c) Copyright 2013-2015, The Rockefeller University */
/* $Id: mdksfree.c 9 2016-01-04 19:30:24Z  $ */
/***********************************************************************
*           Multi-Dimensional Kolmogorov-Smirnov Statistic             *
*                             mdksfree.c                               *
*                                                                      *
*  This software was written by George N. Reeke in the Laboratory of   *
*  Biological Modelling at The Rockefeller University.  Please send    *
*  any corrections, suggestions, or improvements to the author by      *
*  email to reeke@mail.rockefeller.edu for possible incorporation in   *
*  future revisions.                                                   *
*                                                                      *
*  This software is distributed under GPL, version 2.  This program is *
*  free software; you can redistribute it and/or modify it under the   *
*  terms of the GNU General Public License as published by the Free    *
*  Software Foundation; either version 2 of the License, or (at your   *
*  option) any later version. Accordingly, this program is distributed *
*  in the hope that it will be useful, but WITHOUT ANY WARRANTY; with- *
*  out even the implied warranty of MERCHANTABILITY or FITNESS FOR A   *
*  PARTICULAR PURPOSE.  See the GNU General Public License for more    *
*  details.  You should have received a copy of the GNU General Public *
*  License along with this program.  If not, see                       *
*  <http://www.gnu.org/licenses/>.                                     *
*----------------------------------------------------------------------*
*                              mdksfree                                *
*                                                                      *
*  This function frees a work area allocated by mdksallox() for use in *
*  computation of the multi-dimensional Kolmogorov-Smirnov statistic.  *
*  This function is performed by a routine separate from that which    *
*  computes the statistic so that the same work area can be reused for *
*  multiple MDKS calculations.  The same routine is used for all mdks  *
*  data-type variants, hence there is no type version letter.  MATLAB  *
*  uses an alternative mdksallox() call q.v. to free the work area     *
*  storage.                                                            *
*                                                                      *
*  Usage:                                                              *
*     mdksfree() releases the storage associated with the work area    *
*  pointed to by the argument.                                         *
*                                                                      *
*  C or C++ Synopsis:                                                  *
*     int mdksfree(void *pwk)                                          *
*                                                                      *
*  Argument:                                                           *
*     pwk      This must be the pointer stored at *ppwk by a previous  *
*              call to mdksallox().                                    *
*                                                                      *
*  Return values:                                                      *
*     mdksfree() returns 0 on success and 1 if the argument 'pwk'      *
*  apparently did not point to an area allocated by an earlier call    *
*  to mdksallox().                                                     *
*                                                                      *
*  Reference:                                                          *
*     G. Fasano & A. Franceschini (1987).  A multidimensional version  *
*  of the Kolmogorov-Smirnov test.  Monthly Notices of the Royal       *
*  Astronomical Society, 225:155-170.                                  *
************************************************************************
*  V1A, 11/17/13, G.N. Reeke - New file, extracted from mdks.c         *
*  ==>, 11/29/13, GNR - Last date before committing to svn repository  *
*  R09, 12/31/15, GNR - Manage double-linked list of Wkhd blocks       *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "mdks.h"
#include "mdksint.h"

int mdksfree(void *pwk) {

   Wkhd *phd = (Wkhd *)pwk;
   if (!phd || phd->hdrlbl != HLBL) return MDKSA_ERR_BADWK;

   /* Kill the hdrlbl in case user tries to reuse freed area */
   phd->hdrlbl = 0;

   /* Remove this one from the linked list */
   if (phd->wlnk.pnwhd) phd->wlnk.pnwhd->ppwhd = phd->wlnk.ppwhd;
   phd->wlnk.ppwhd->pnwhd = phd->wlnk.pnwhd;

   free(pwk);

   return 0;
   } /* End mdksfree() */

