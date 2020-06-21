/* (c) Copyright 2015, The Rockefeller University *11115* */
/* $Id: mfalign.c 5 2008-03-13 20:32:23Z  $ */
/***********************************************************************
*                     ROCKS metafile plot library                      *
*                              mfalign()                               *
*                                                                      *
* DESCRIPTION:                                                         *
*  mfalign() writes a NOP command into the metafile buffer with a skip *
*  count calculated to align writing on a byte boundary (where a byte  *
*  is defined as 8 bits for metafile format 'B').  This is needed when *
*  the following material is unpredictable, as at the start of a new   *
*  windown, frame, or buffer.                                          *
*                                                                      *
*  mfnxtbb() writes enough zeros into the metafile buffer to bring     *
*  the position to an even byte boundary.  This can be done whenever   *
*  the following material is predicatable, for example, internal to    *
*  a graphics command, so the OpAlign code is not needed.              *
*                                                                      *
* SYNOPSES:                                                            *
*  void mfalign(void)                                                  *
*  void mfnxtbb(void)                                                  *
*                                                                      *
* ARGUMENTS:                                                           *
*  None                                                                *
*                                                                      *
* RETURN VALUES:                                                       *
*  None.  There are no error conditions.                               *
*                                                                      *
* DESIGN NOTES:                                                        *
*  The specs for this library require that in the unlikely case of a   *
*  machine with BITSPERBYTE != 8, such machine nonetheless must write  *
*  the metafile as if BITSPERBYTE == 8.  This implies that ASCII char- *
*  acter strings must be written with mfbitpk() instead of memcpy().   *
************************************************************************
*  V1A, 04/18/15, GNR - New routine for major revision of plot lib     *
*  ==>, 05/24/15, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include "mfint.h"
#include "mfio.h"

/*=====================================================================*
*                              mfalign()                               *
*                                                                      *
*  mfalign() is uniquely allowed to write into the space at the end of *
*  the metafile buffer that is reserved for this purpose. It does this *
*  by adjusting the value of Win.MFBrem before using mfbitpk(), then   *
*  restoring it.  It computes the number of extra bytes needed to hold *
*  the NOP command and the skip count using a method that does not     *
*  assume 8 bits/byte.  This requires that LSkipCt and LNop be defined *
*  in mfint.h.                                                         *
*=====================================================================*/

void mfalign(void) {

   Win *pw = _RKG.pcw;     /* Locate current window info */
   int nb;                 /* Extra bits needed */
   int skip;               /* Actual skip count to write */
   int ualgn = BitRemainder(pw->MFBrem + 1);
   if (ualgn == 0) return;
   skip = ualgn - LNop;
   nb = BITSPERBYTE * ((skip >= 0) ?  0 : BytesInXBits(-skip));
   skip += nb;

   pw->MFBrem += nb;       /* Access the reserves */
   c2mf(OpAlign);
   mfbitpk(skip, LSkipCt);
   mfbitpk(0, skip);
   pw->MFBrem -= nb;       /* Restore normal counting */

   } /* End mfalign() */

/*=====================================================================*
*                              mfnxtbb()                               *
*                                                                      *
*  Unlike mfalign(), mfnxtbb() computes the space to the next byte     *
*  assuming 8 bits/byte, even when this is not the case.  Following    *
*  character strings will be aligned on 8-bit boundaries.              *
*=====================================================================*/

void mfnxtbb(void) {

   int skip = (_RKG.pcw->MFBrem + 1) & 7;
   if (skip) mfbitpk(0, skip);

   } /* End mfnxtbb() */
