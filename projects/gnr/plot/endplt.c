/***********************************************************************
*                                                                      *
*                               endplt.c                               *
*                                                                      *
***********************************************************************/

#include "mfint.h"
#include "plots.h"

int endplt(void){
   int rc = 0; /* return code */
#if defined(DEBUG) && DEBUG & MFDBG_FINPL
   dbgprt("endplt() called");
#endif
   mfbitpk(OpEnd, Lop);
   mfflush();

   return rc;
}/* end endplt() */
