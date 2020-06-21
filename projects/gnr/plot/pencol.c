//

#include <string.h>
#include "mfint.h"
#include "plots.h"

void pencol(const char *color){

   ui32 nc = strlen(color);
   if(!(_RKG.pcw->MFActive)) return;
   mfbchk(mxlPenCol);
   mfbitpk(OpPCol, Lop);
   mfbitpk(nc, 4);
   cs2mf(color, nc);
} /* End pencol() */