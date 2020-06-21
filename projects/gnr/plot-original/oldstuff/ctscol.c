/* (c) Copyright 1998, Neurosciences Research Foundation, Inc. */
/***********************************************************************
*                              ctscol()                                *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void ctscol(unsigned int icts)                                   *
*                                                                      *
*  DESCRIPTION:                                                        *
*     This function changes the drawing color to a value returned by   *
*     a previous call to ctsyn().  This combination permits the work   *
*     of converting an English color name or a hexadecimal color       *
*     specification to a color index to be performed just once, when   *
*     ctsyn() is processed.  Subsequent calls to ctscol() are proces-  *
*     sed quickly and the space taken in the metafile is less than     *
*     with pencol().                                                   *
*                                                                      *
*  ARGUMENT:                                                           *
*     icts    The index value returned by a previous call to ctsyn().  *
*                                                                      *
*  RETURN VALUE:                                                       *
*     None.                                                            *
*                                                                      *
*  V2A, 12/29/98, GNR - New routine, make binary metafile              *
***********************************************************************/

#include "mfint.h"
#include "plots.h"

#define CTSI   0     /* gks offset of icts */

void ctscol(unsigned int icts) {

   static char MCctscol[] = { OpSCol, Tk, CTSI, Tend };

   if (_RKG.s.MFmode) {
      _RKG.c[CUR] = _RKG.gks[CTSI] = icts;
      _RKG.currColType = CCT_CTSCOL;
      CMDPACK(MCctscol, Lop+Li11);
      } /* End if MFmode */

$$$ CLEAR InReset BIT

   } /* End ctscol() */
