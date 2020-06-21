/* (c) Copyright 1998, Neurosciences Research Foundation, Inc. */
/***********************************************************************
*                              gobjid()                                *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void gobjid(unsigned long id)                                    *
*                                                                      *
*  DESCRIPTION:                                                        *
*     This function assigns an arbitrary number to objects plotted     *
*     by subsequent plotting commands until the next gobjid call.      *
*     In a future version of this library, the object number may       *
*     be returned when the user selects a particular object using      *
*     a pointing device.                                               *
*                                                                      *
*  ARGUMENT:                                                           *
*     id      An arbitrary, user-defined positive integer that is      *
*             to be used to identify objects drawn with subsequent     *
*             plotting commands.  In a parallel computer, the 'id'     *
*             applies only to objects drawn on the current processor   *
*             node.                                                    *
*                                                                      *
*  RETURN VALUE:                                                       *
*     None.                                                            *
*                                                                      *
*  V2A, 12/29/98, GNR - New routine, make binary metafile              *
***********************************************************************/

#include "mfint.h"
#include "plots.h"

#define GOBJIO    0     /* gks object id offset */

void gobjid(unsigned long id) {

   static char MCgobjid[] = { OpObjid, Tk, GOBJIO, Tend };

   if (_RKG.s.MFmode) {
      _RKG.gobjid = _RKG.gks[GOBJIO] = id;
      CMDPACK(MCgobjid, Lop + Li11);
      } /* End if MFmode */

$$$ CLEAR InReset BIT

   } /* End gobjid() */
