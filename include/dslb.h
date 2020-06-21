/* (c) Copyright 1988-2008, The Rockefeller University *11115* */
/* $Id: dslb.h 59 2017-01-13 20:34:16Z  $ */
/***********************************************************************
*                          DSLB Header File                            *
*                              11/28/88                                *
*  Declaration of pixel storage block for dynamic objects.             *
*                                                                      *
*  N.B.  Space for pixel data is allocated immediately following       *
*        the DSLBDEF block using a single malloc.  Pixel data are      *
*        therefore located at (byte *)&DSLBDEF+sizeof(DSLBDEF).        *
*                                                                      *
*  N.B.  Shape linking pointer (PNXSHP) must be first field in record  *
*        (current sorting algorithm depends on this...)                *
*                                                                      *
*  V1A, 11/28/88, GNR - Initial version                                *
*  Rev, 10/29/93, GNR - Use SHP_NAME_SIZE, remove unneeded union       *
*  ==>, 07/01/02, GNR - Last date before committing to svn repository  *
*  Rev, 01/05/11, GNR - Convert SHAPE to a typedef                     *
***********************************************************************/

#ifndef DSLB_HDR_INCLUDED
#define DSLB_HDR_INCLUDED

#include "env.h"

struct DSLBDEF {
   struct DSLBDEF *pnxshp;    /* Pointer to next shape                */
   char nshp[SHP_NAME_SIZE];  /* Name of this shape                   */
   ui16  tgp;                 /* Group number                         */
   short tfm;                 /* Format (0=old, 1=a, 2=b, 3=c,
                              *  +256 if transparent pixels present)  */
   short tsx;                 /* X size                               */
   short tsy;                 /* Y size                               */
   };

/* Macro for locating DSLB data */
#define dslpix(pdslb) ((char *)pdslb + sizeof(struct DSLBDEF))

#endif   /* DSLB_HDR_INCLUDED */
