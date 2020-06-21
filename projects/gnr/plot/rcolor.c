/* (c) Copyright 1992-2011, The Rockefeller University *21114* */
/* $Id: square.c 20 2011-12-24 02:59:03Z  $ */
/***********************************************************************
*                               rcolor()                               *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void rcolor(unsigned int jc)                                     *
*                                                                      *
*  DESCRIPTION:                                                        *
*                                                                      *
*  RETURN VALUES:                                                      *
*     None                                                             *
************************************************************************
*  Version 2, 10/24/18, GMK                                            *
***********************************************************************/

#include "plots.h"
#include "mfint.h"

void rcolor(unsigned int jc){

	if(!(_RKG.s.MFActive)) return;

	mfbchk(mxlRcol);
	c2mf(hexch_to_int(OpRcol));
	k2mf(jc);
} /* End of rcolor)_ */