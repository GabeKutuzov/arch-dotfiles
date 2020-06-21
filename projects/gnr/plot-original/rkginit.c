/* (c) Copyright 2015, The Rockefeller University *11115* */
/* $Id: rkginit.c 5 2008-03-13 20:32:23Z  $ */
/***********************************************************************
*                     ROCKS metafile plot library                      *
*                              rkginit()                               *
*                                                                      *
*  Routine to initialize the common data structure, _RKG, on first     *
*  call to one of the routines in this library.  (This was formerly    *
*  accomplished with load-time initialization in mfint.h, but this     *
*  was becoming too difficult to maintain.)                            *
*                                                                      *
*  Default constants are simply entered here as data rather than as    *
*  pound-defs, because they are not used anyplace else by definition.  *
*  rkginit() must be called at start of setmf() or newplt() so user    *
*  overrides will replace defaults set here.  The rkgifin() finishes   *
*  the initialization using values that might have been overridden.    *
************************************************************************
*  V2A, 02/07/15, GNR - New file broken out from mfint.h               *
***********************************************************************/

#include "mfint.h"

void rkginit(void) {

   /* Initialize RKGSdef structure */
   _RKG.s.base_fact = 1.0;
   _RKG.s.fcsc = (float)(1<<DFLTLCI);
   _RKG.s.MFBuffLen = DFLTBUFLEN;
   _RKG.s.MFMode = SGX;
   _RKG.s.lci = (short)DFLTLCI;
   _RKG.s.lcf = (short)DFLTLCF;
   /* Rest of RKSGdef is filled in later, by rkgifin() */
#ifdef PAR
   _RKG.s.numnodes = 1;
#endif
   _RKG.movie_mode = MM_STILL;
   _RKG.MFFlags = MFF_DidSetup;
   } /* End rkginit() */
